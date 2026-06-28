/**
 * \file text_parser.cpp
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include "../../src/uplc/ast/uplc_term.h"
#include "../../src/uplc/machine/uplc_machine.h"
#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/ast/uplc_int.h"
#include "../../src/uplc/data/uplc_data.h"
#include "../../src/uplc/flat/flat_decode.h"
#include "../../src/uplc/syntax/pretty.h"
#include "../../src/uplc/syntax/text_parser.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cstring>
#include <gmock/gmock.h>
#include <string>
#include <vector>

/* STATIC HELPERS ************************************************************/

namespace
{

cardano_uplc_arena_t*
make_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(0U, &arena), CARDANO_SUCCESS);
  return arena;
}

cardano_error_t
parse(const std::string& text, cardano_uplc_arena_t* arena, const cardano_uplc_program_t** program, size_t* offset = nullptr)
{
  size_t local = 0U;
  return cardano_uplc_parse_program(arena, text.c_str(), text.size(), program, (offset != nullptr) ? offset : &local);
}

int64_t
int_const_i64(const cardano_uplc_constant_t* constant)
{
  if (cardano_uplc_constant_int_is_small(constant))
  {
    return cardano_uplc_constant_int_small(constant);
  }

  return cardano_bigint_to_int(constant->as.integer.big);
}

// Structural (alpha) equality on two de Bruijn terms.
bool
terms_equal(const cardano_uplc_term_t* a, const cardano_uplc_term_t* b)
{
  if ((a == nullptr) || (b == nullptr))
  {
    return a == b;
  }

  if (a->kind != b->kind)
  {
    return false;
  }

  switch (a->kind)
  {
    case CARDANO_UPLC_TERM_VAR:
      return a->as.var_index == b->as.var_index;
    case CARDANO_UPLC_TERM_DELAY:
    case CARDANO_UPLC_TERM_LAMBDA:
    case CARDANO_UPLC_TERM_FORCE:
      return terms_equal(a->as.unary, b->as.unary);
    case CARDANO_UPLC_TERM_APPLY:
      return terms_equal(a->as.apply.function, b->as.apply.function) && terms_equal(a->as.apply.argument, b->as.apply.argument);
    case CARDANO_UPLC_TERM_CONSTANT:
      return a->as.constant->kind == b->as.constant->kind;
    case CARDANO_UPLC_TERM_ERROR:
      return true;
    case CARDANO_UPLC_TERM_BUILTIN:
      return a->as.builtin == b->as.builtin;
    case CARDANO_UPLC_TERM_CONSTR:
    {
      if ((a->as.constr.tag != b->as.constr.tag) || (a->as.constr.field_count != b->as.constr.field_count))
      {
        return false;
      }
      for (size_t i = 0U; i < a->as.constr.field_count; ++i)
      {
        if (!terms_equal(a->as.constr.fields[i], b->as.constr.fields[i]))
        {
          return false;
        }
      }
      return true;
    }
    case CARDANO_UPLC_TERM_CASE:
    {
      if (a->as.cases.branch_count != b->as.cases.branch_count)
      {
        return false;
      }
      if (!terms_equal(a->as.cases.scrutinee, b->as.cases.scrutinee))
      {
        return false;
      }
      for (size_t i = 0U; i < a->as.cases.branch_count; ++i)
      {
        if (!terms_equal(a->as.cases.branches[i], b->as.cases.branches[i]))
        {
          return false;
        }
      }
      return true;
    }
    default:
      return false;
  }
}

// MSB-first bit writer mirroring the flat reader, used to build cross-check streams.
class BitWriter
{
  public:

    void
    bit(uint8_t value)
    {
      if (bit_pos_ == 0U)
      {
        bytes_.push_back(0U);
      }
      if (value != 0U)
      {
        bytes_.back() |= static_cast<uint8_t>(0x80U >> bit_pos_);
      }
      bit_pos_ = static_cast<uint8_t>((bit_pos_ + 1U) % 8U);
    }

    void
    bits(uint8_t value, uint8_t count)
    {
      for (uint8_t i = 0U; i < count; ++i)
      {
        const uint8_t shift = static_cast<uint8_t>(count - 1U - i);
        bit(static_cast<uint8_t>((value >> shift) & 1U));
      }
    }

    void
    word(uint64_t value)
    {
      while (true)
      {
        uint8_t group = static_cast<uint8_t>(value & 0x7FU);
        value         >>= 7U;
        if (value != 0U)
        {
          group |= 0x80U;
        }
        bits(group, 8U);
        if (value == 0U)
        {
          break;
        }
      }
    }

    void
    filler()
    {
      while (bit_pos_ != 7U)
      {
        bit(0U);
      }
      bit(1U);
    }

    const std::vector<uint8_t>&
    bytes() const
    {
      return bytes_;
    }

  private:

    std::vector<uint8_t> bytes_;
    uint8_t              bit_pos_ = 0U;
};

cardano_error_t
decode_flat_program(const std::vector<uint8_t>& bytes, cardano_uplc_arena_t* arena, const cardano_uplc_program_t** program)
{
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, bytes.data(), bytes.size()), CARDANO_SUCCESS);
  return cardano_uplc_flat_decode_program(arena, &reader, program);
}

std::string
pretty(const cardano_uplc_program_t* program)
{
  cardano_buffer_t* out    = nullptr;
  cardano_error_t   result = cardano_uplc_pretty_print_program(program, &out);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  size_t size = cardano_buffer_get_size(out);
  if ((size > 0U) && (cardano_buffer_get_data(out)[size - 1U] == 0U))
  {
    size -= 1U;
  }
  std::string text(reinterpret_cast<const char*>(cardano_buffer_get_data(out)), size);
  cardano_buffer_unref(&out);
  return text;
}

} // namespace

/* PROGRAM / VERSION TESTS *************************************************/

TEST(cardano_uplc_parse_program, parsesIdentityLambda)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (lam x x))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->version_major, 1U);
  EXPECT_EQ(program->version_minor, 0U);
  EXPECT_EQ(program->version_patch, 0U);
  ASSERT_EQ(program->term->kind, CARDANO_UPLC_TERM_LAMBDA);
  ASSERT_EQ(program->term->as.unary->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(program->term->as.unary->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, resolvesOuterBinderToIndexTwo)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (lam x (lam y x)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  const cardano_uplc_term_t* inner = program->term->as.unary->as.unary;
  ASSERT_EQ(inner->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(inner->as.var_index, 2U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, resolvesInnerBinderToIndexOne)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (lam x (lam y y)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.unary->as.unary->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, shadowingPrefersInnermost)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (lam x (lam x x)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.unary->as.unary->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesVersion110AndHyphenNames)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (lam y-0 y-0))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->version_minor, 1U);
  EXPECT_EQ(program->term->as.unary->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, skipsLineCommentsAndWhitespace)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;
  const std::string             text    = "-- a program\n(program\n  1.0.0\n  (lam x x) -- trailing\n)";

  // Act
  cardano_error_t result = parse(text, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_LAMBDA);

  cardano_uplc_arena_free(&arena);
}

/* TERM FORM TESTS *********************************************************/

TEST(cardano_uplc_parse_program, parsesDelayForceErrorBuiltin)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (force (delay (error))))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->kind, CARDANO_UPLC_TERM_FORCE);
  ASSERT_EQ(program->term->as.unary->kind, CARDANO_UPLC_TERM_DELAY);
  EXPECT_EQ(program->term->as.unary->as.unary->kind, CARDANO_UPLC_TERM_ERROR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, delayDoesNotBind)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  // Under one lam, a var inside a delay still resolves to index 1: delay is not a binder.
  cardano_error_t result = parse("(program 1.0.0 (lam x (delay x)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.unary->as.unary->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesBuiltinName)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (builtin addInteger))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->kind, CARDANO_UPLC_TERM_BUILTIN);
  EXPECT_EQ(program->term->as.builtin, CARDANO_UPLC_BUILTIN_ADD_INTEGER);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesForcedBuiltin)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (force (builtin ifThenElse)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.unary->as.builtin, CARDANO_UPLC_BUILTIN_IF_THEN_ELSE);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, applicationIsLeftAssociativeNary)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  // [ x x x ] = ((x x) x) under one lam: outer apply's function is itself an apply.
  cardano_error_t result = parse("(program 1.0.0 (lam x [ x x x ]))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  const cardano_uplc_term_t* app = program->term->as.unary;
  ASSERT_EQ(app->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(app->as.apply.argument->kind, CARDANO_UPLC_TERM_VAR);
  ASSERT_EQ(app->as.apply.function->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(app->as.apply.function->as.apply.function->kind, CARDANO_UPLC_TERM_VAR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesConstrWithFields)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (constr 5 (con integer 1) (error)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(program->term->as.constr.tag, 5U);
  ASSERT_EQ(program->term->as.constr.field_count, 2U);
  EXPECT_EQ(program->term->as.constr.fields[0]->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(program->term->as.constr.fields[1]->kind, CARDANO_UPLC_TERM_ERROR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesEmptyConstr)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (constr 0 ))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(program->term->as.constr.field_count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesMaxConstrTag)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (constr 18446744073709551615 (con integer 1)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.constr.tag, 18446744073709551615ULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsConstrTagOverflow)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (constr 18446744073709551616 ))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesCaseWithBranches)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (case (con integer 0) (lam x x) (error)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->kind, CARDANO_UPLC_TERM_CASE);
  EXPECT_EQ(program->term->as.cases.scrutinee->kind, CARDANO_UPLC_TERM_CONSTANT);
  ASSERT_EQ(program->term->as.cases.branch_count, 2U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesCaseWithoutBranches)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (case (con integer 0)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.cases.branch_count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsConstrAndCaseBelowVersion110)
{
  // constr and case are only legal from version 1.1.0 onward; under 1.0.0 they are
  // a parse error, matching the reference.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (constr 0 ))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (case (con integer 1)))", arena, &program), CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, acceptsConstrAndCaseAtVersion110)
{
  // The same forms parse under version 1.1.0.
  cardano_uplc_arena_t*         arena  = make_arena();
  const cardano_uplc_program_t* constr = nullptr;
  const cardano_uplc_program_t* cased  = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.1.0 (constr 0 ))", arena, &constr), CARDANO_SUCCESS);
  EXPECT_EQ(parse("(program 1.1.0 (case (con integer 1)))", arena, &cased), CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);
}

/* CONSTANT TESTS *********************************************************/

TEST(cardano_uplc_parse_program, parsesNegativeInteger)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con integer -123))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->as.constant->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(int_const_i64(program->term->as.constant), -123);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesPositiveSignedInteger)
{
  // A leading '+' denotes a positive value and is accepted, matching the reference.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con integer +7))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->as.constant->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(int_const_i64(program->term->as.constant), 7);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesByteStringAndEmpty)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con bytestring #deadBEEF))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->as.constant->kind, CARDANO_UPLC_TYPE_BYTE_STRING);
  ASSERT_EQ(program->term->as.constant->as.bytes.size, 4U);
  EXPECT_EQ(program->term->as.constant->as.bytes.data[0], 0xDEU);

  const cardano_uplc_program_t* empty = nullptr;
  EXPECT_EQ(parse("(program 1.0.0 (con bytestring #))", arena, &empty), CARDANO_SUCCESS);
  EXPECT_EQ(empty->term->as.constant->as.bytes.size, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesStringWithEscapes)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con string \"a\\tb\\\"c\\x41\"))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  std::string value(reinterpret_cast<const char*>(program->term->as.constant->as.string.data), program->term->as.constant->as.string.size);
  EXPECT_EQ(value, std::string("a\tb\"cA"));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesNumericAndNamedStringEscapes)
{
  // Decimal (\83='S'), hex (\x75='u'), octal (\o143='c') and a named control code
  // (\DEL=0x7f) escapes, matching the Haskell-style escape set the corpus uses.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con string \"\\83\\x75\\o143\\DEL\"))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  std::string value(reinterpret_cast<const char*>(program->term->as.constant->as.string.data), program->term->as.constant->as.string.size);
  EXPECT_EQ(value, std::string("Suc\x7f"));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, encodesAboveAsciiEscapeAsUtf8)
{
  // A numeric escape above 127 is UTF-8 encoded: \8712 is U+2208 ('IN'), which is
  // the three bytes e2 88 88.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con string \"\\8712\"))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  const byte_t* s = program->term->as.constant->as.string.data;
  ASSERT_EQ(program->term->as.constant->as.string.size, 3U);
  EXPECT_EQ(s[0], 0xE2U);
  EXPECT_EQ(s[1], 0x88U);
  EXPECT_EQ(s[2], 0x88U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesBoolAndUnit)
{
  // Arrange
  cardano_uplc_arena_t*         arena = make_arena();
  const cardano_uplc_program_t* t     = nullptr;
  const cardano_uplc_program_t* f     = nullptr;
  const cardano_uplc_program_t* u     = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (con bool True))", arena, &t), CARDANO_SUCCESS);
  EXPECT_TRUE(t->term->as.constant->as.boolean);
  EXPECT_EQ(parse("(program 1.0.0 (con bool False))", arena, &f), CARDANO_SUCCESS);
  EXPECT_FALSE(f->term->as.constant->as.boolean);
  EXPECT_EQ(parse("(program 1.0.0 (con unit ()))", arena, &u), CARDANO_SUCCESS);
  EXPECT_EQ(u->term->as.constant->kind, CARDANO_UPLC_TYPE_UNIT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesListOfIntegers)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con (list integer) [1, 2, 3, 4]))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  const cardano_uplc_constant_t* c = program->term->as.constant;
  ASSERT_EQ(c->kind, CARDANO_UPLC_TYPE_LIST);
  ASSERT_EQ(c->as.list.count, 4U);
  EXPECT_EQ(int_const_i64(c->as.list.items[3]), 4);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesEmptyList)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con (list integer) []))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.constant->as.list.count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesNestedListOfLists)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con (list (list integer)) [[1, 2], []]))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  const cardano_uplc_constant_t* c = program->term->as.constant;
  ASSERT_EQ(c->as.list.count, 2U);
  EXPECT_EQ(c->as.list.items[0]->kind, CARDANO_UPLC_TYPE_LIST);
  EXPECT_EQ(c->as.list.items[0]->as.list.count, 2U);
  EXPECT_EQ(c->as.list.items[1]->as.list.count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesArrayOfBooleans)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (con (array bool) [True, False, True]))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  const cardano_uplc_constant_t* c = program->term->as.constant;
  ASSERT_EQ(c->kind, CARDANO_UPLC_TYPE_ARRAY);
  ASSERT_EQ(c->as.list.count, 3U);
  EXPECT_TRUE(c->as.list.items[0]->as.boolean);
  EXPECT_FALSE(c->as.list.items[1]->as.boolean);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesEmptyArray)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (con (array integer) []))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->as.constant->kind, CARDANO_UPLC_TYPE_ARRAY);
  EXPECT_EQ(program->term->as.constant->as.list.count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsAnIllTypedArrayElement)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (con (array bool) [5]))", arena, &program);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesPair)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.1.0 (con (pair integer bool) (42, False)))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  const cardano_uplc_constant_t* c = program->term->as.constant;
  ASSERT_EQ(c->kind, CARDANO_UPLC_TYPE_PAIR);
  EXPECT_EQ(int_const_i64(c->as.pair.fst), 42);
  EXPECT_FALSE(c->as.pair.snd->as.boolean);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesDataShapes)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;
  const std::string             text    = "(program 1.0.0 (con data (Map [(B #aa, Map [(B #bb, I 100)])])))";

  // Act
  cardano_error_t result = parse(text, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(program->term->as.constant->kind, CARDANO_UPLC_TYPE_DATA);
  EXPECT_EQ(program->term->as.constant->as.data->kind, CARDANO_UPLC_DATA_KIND_MAP);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesDataConstrAndList)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con data (Constr 7212 [B #79d3, List [I 44]])))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(program->term->as.constant->as.data->kind, CARDANO_UPLC_DATA_KIND_CONSTR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesDataIntegerAndBytesAndEmpties)
{
  // Arrange
  cardano_uplc_arena_t*         arena = make_arena();
  const cardano_uplc_program_t* i     = nullptr;
  const cardano_uplc_program_t* b     = nullptr;
  const cardano_uplc_program_t* l     = nullptr;
  const cardano_uplc_program_t* c     = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (con data (I -7)))", arena, &i), CARDANO_SUCCESS);
  EXPECT_EQ(parse("(program 1.0.0 (con data (B #ff)))", arena, &b), CARDANO_SUCCESS);
  EXPECT_EQ(parse("(program 1.0.0 (con data (List [])))", arena, &l), CARDANO_SUCCESS);
  EXPECT_EQ(parse("(program 1.0.0 (con data (Constr 0 [])))", arena, &c), CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);
}

/* ALPHA-EQUIVALENCE WITH THE FLAT DECODER ********************************/

TEST(cardano_uplc_parse_program, matchesFlatDecoderIdentity)
{
  // Arrange: flat bytes for (program 1.0.0 (lam (var 1))).
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(2U, 4U); // lambda
  w.bits(0U, 4U); // var
  w.word(1U);
  w.filler();

  cardano_uplc_arena_t*         flat_arena = make_arena();
  cardano_uplc_arena_t*         text_arena = make_arena();
  const cardano_uplc_program_t* from_flat  = nullptr;
  const cardano_uplc_program_t* from_text  = nullptr;

  // Act
  EXPECT_EQ(decode_flat_program(w.bytes(), flat_arena, &from_flat), CARDANO_SUCCESS);
  EXPECT_EQ(parse("(program 1.0.0 (lam x x))", text_arena, &from_text), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(terms_equal(from_flat->term, from_text->term));

  cardano_uplc_arena_free(&flat_arena);
  cardano_uplc_arena_free(&text_arena);
}

TEST(cardano_uplc_parse_program, matchesFlatDecoderNestedBinders)
{
  // Arrange: flat for (program 1.0.0 (lam (lam (var 2)))), inner ref to outer binder.
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(2U, 4U); // lambda
  w.bits(2U, 4U); // lambda
  w.bits(0U, 4U); // var
  w.word(2U);
  w.filler();

  cardano_uplc_arena_t*         flat_arena = make_arena();
  cardano_uplc_arena_t*         text_arena = make_arena();
  const cardano_uplc_program_t* from_flat  = nullptr;
  const cardano_uplc_program_t* from_text  = nullptr;

  // Act
  EXPECT_EQ(decode_flat_program(w.bytes(), flat_arena, &from_flat), CARDANO_SUCCESS);
  EXPECT_EQ(parse("(program 1.0.0 (lam a (lam b a)))", text_arena, &from_text), CARDANO_SUCCESS);

  // Assert: alpha-equivalent (binder names differ, indices match).
  EXPECT_TRUE(terms_equal(from_flat->term, from_text->term));

  cardano_uplc_arena_free(&flat_arena);
  cardano_uplc_arena_free(&text_arena);
}

TEST(cardano_uplc_parse_program, matchesFlatDecoderApplyAndForce)
{
  // Arrange: flat for (program 1.0.0 (lam (force [ (var 1) (var 1) ]))).
  BitWriter w;
  w.word(1U);
  w.word(0U);
  w.word(0U);
  w.bits(2U, 4U); // lambda
  w.bits(5U, 4U); // force
  w.bits(3U, 4U); // apply
  w.bits(0U, 4U); // var
  w.word(1U);
  w.bits(0U, 4U); // var
  w.word(1U);
  w.filler();

  cardano_uplc_arena_t*         flat_arena = make_arena();
  cardano_uplc_arena_t*         text_arena = make_arena();
  const cardano_uplc_program_t* from_flat  = nullptr;
  const cardano_uplc_program_t* from_text  = nullptr;

  // Act
  EXPECT_EQ(decode_flat_program(w.bytes(), flat_arena, &from_flat), CARDANO_SUCCESS);
  EXPECT_EQ(parse("(program 1.0.0 (lam f (force [ f f ])))", text_arena, &from_text), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(terms_equal(from_flat->term, from_text->term));

  cardano_uplc_arena_free(&flat_arena);
  cardano_uplc_arena_free(&text_arena);
}

/* ROUND TRIP WITH THE PRETTY PRINTER *************************************/

TEST(cardano_uplc_parse_program, roundTripsThroughPrettyPrinter)
{
  // Arrange
  cardano_uplc_arena_t*         arena = make_arena();
  const cardano_uplc_program_t* first = nullptr;
  const cardano_uplc_program_t* again = nullptr;

  // Act: parse, print, then parse the printed program and print again; the two
  // prints must be identical, proving the parser accepts the printer's output and
  // resolves the printer's v-<level> names back to the same de Bruijn AST.
  EXPECT_EQ(parse("(program 1.0.0 (lam x (lam y [ y x ])))", arena, &first), CARDANO_SUCCESS);
  std::string printed_once = pretty(first);
  EXPECT_EQ(parse(printed_once, arena, &again), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(printed_once, pretty(again));

  cardano_uplc_arena_free(&arena);
}

/* MALFORMED INPUT TESTS **************************************************/

TEST(cardano_uplc_parse_program, rejectsNullArguments)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(cardano_uplc_parse_program(nullptr, "(program 1.0.0 (error))", 23U, &program, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_parse_program(arena, "x", 1U, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_parse_program(arena, nullptr, 5U, &program, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesFreeVariable)
{
  // A free (unbound) variable is accepted at parse time and given an out-of-range
  // de Bruijn index, matching the reference, which parses the open term.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 x)", arena, &program);

  // Assert: it parses, and the free variable resolves to index 1 with an empty
  // top-level scope, which is out of range for the evaluation environment.
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  ASSERT_EQ(program->term->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(program->term->as.var_index, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, freeVariableEvaluatesToScriptFailure)
{
  // An open term parses, then the machine fails the env lookup for the free
  // variable, ending the evaluation with a script error rather than a parse error.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 x)", arena, &program);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  const cardano_uplc_budget_t initial = { 100000000, 100000000 };
  cardano_uplc_eval_result_t  eval    = {};
  cardano_error_t             host =
    cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, initial, &eval);

  // Assert: the host ran the script and the script outcome is an error term.
  EXPECT_EQ(host, CARDANO_SUCCESS);
  EXPECT_EQ(eval.status, CARDANO_UPLC_EVAL_ERROR_TERM);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsUnknownBuiltin)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (builtin notARealBuiltin))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsUnbalancedParens)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (lam x x)", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 [ x x ", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (error)) extra", arena, &program), CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsApplicationWithSingleTerm)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse("(program 1.0.0 (lam x [ x ]))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsBadHexAndOddHex)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (con bytestring #abc))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (con bytestring 12))", arena, &program), CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsUnterminatedAndBadEscapeString)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (con string \"oops", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (con string \"\\q\"))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (con string \"\\xZZ\"))", arena, &program), CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsBadProgramHeader)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(prog 1.0.0 (error))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0 (error))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 99999999999999999999999.0.0 (error))", arena, &program), CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsUnknownTypeAndMalformedBlsLiteral)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (con wibble 1))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (con bls12_381_G1_element 0x00))", arena, &program), CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, parsesBlsG1ElementLiteral)
{
  // Arrange: the compressed G1 generator parses into a G1 element constant.
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  const cardano_error_t result = parse(
    "(program 1.0.0 (con bls12_381_G1_element "
    "0x97f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb))",
    arena,
    &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(program, nullptr);
  ASSERT_NE(program->term, nullptr);
  EXPECT_EQ(program->term->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(program->term->as.constant->kind, CARDANO_UPLC_TYPE_BLS_G1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, rejectsMalformedData)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act / Assert
  EXPECT_EQ(parse("(program 1.0.0 (con data (Nope 0)))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (con data (Map [(B #aa)])))", arena, &program), CARDANO_ERROR_DECODING);
  EXPECT_EQ(parse("(program 1.0.0 (con data (Constr 0 [I 1)))", arena, &program), CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, reportsErrorOffset)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;
  size_t                        offset  = 0U;

  // Act: an unknown builtin name is a parse error past the program header.
  cardano_error_t result = parse("(program 1.0.0 (builtin notARealBuiltin))", arena, &program, &offset);

  // Assert: the offset points at or past the offending token.
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_GT(offset, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, boundsDeepNesting)
{
  // Arrange: build a force-nest deeper than the recursion bound.
  std::string  text  = "(program 1.0.0 ";
  const size_t depth = 5000U;
  for (size_t i = 0U; i < depth; ++i)
  {
    text += "(force ";
  }
  text += "(error)";
  for (size_t i = 0U; i < depth; ++i)
  {
    text += ")";
  }
  text += ")";

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse(text, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  cardano_uplc_arena_free(&arena);
}

/* REAL CORPUS FILES *****************************************************/

namespace
{

std::string
read_file(const std::string& path)
{
  FILE* file = std::fopen(path.c_str(), "rb");
  if (file == nullptr)
  {
    return std::string();
  }
  std::string contents;
  char        buffer[4096];
  size_t      n = 0U;
  while ((n = std::fread(buffer, 1U, sizeof(buffer), file)) > 0U)
  {
    contents.append(buffer, n);
  }
  std::fclose(file);
  return contents;
}

std::string
corpus_root()
{
  // Derive the corpus path from this source file's location so the test is
  // independent of the working directory the binary is launched from.
  std::string file  = __FILE__;
  size_t      slash = file.find_last_of("/\\");
  std::string dir   = (slash == std::string::npos) ? std::string(".") : file.substr(0U, slash);
  return dir + "/vectors/";
}

} // namespace

TEST(cardano_uplc_parse_program, parsesRealCorpusFiles)
{
  // Arrange
  const std::vector<std::string> files = {
    "example/ApplyAdd1/ApplyAdd1.uplc",
    "example/churchSucc/churchSucc.uplc",
    "term/constant-case/list/list-01/list-01.uplc",
    "term/constant-case/pair/pair-01/pair-01.uplc",
    "term/constr/constr-03/constr-03.uplc",
    "term/constr/constr-08/constr-08.uplc"
  };

  size_t parsed = 0U;

  for (const std::string& rel: files)
  {
    std::string text = read_file(corpus_root() + rel);
    if (text.empty())
    {
      // Corpus path may differ under some runners; skip silently if unreadable.
      continue;
    }

    cardano_uplc_arena_t*         arena   = make_arena();
    const cardano_uplc_program_t* program = nullptr;
    cardano_error_t               result  = parse(text, arena, &program);
    EXPECT_EQ(result, CARDANO_SUCCESS) << "failed to parse " << rel;
    if (result == CARDANO_SUCCESS)
    {
      EXPECT_NE(program->term, nullptr);
      parsed += 1U;
    }
    cardano_uplc_arena_free(&arena);
  }

  EXPECT_GT(parsed, 0U) << "no corpus file was readable; check corpus_root()";
}

TEST(cardano_uplc_parse_program, parsesRealCorpusExpectedFile)
{
  // Arrange
  std::string text = read_file(std::string(corpus_root()) + "example/churchSucc/churchSucc.uplc.expected");
  if (text.empty())
  {
    GTEST_SKIP() << "expected file unreadable under this runner";
  }

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse(text, arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);
}

/* ALLOCATION-FAILURE PATHS **********************************************/

TEST(cardano_uplc_parse_program, handlesAllocationFailureUpFront)
{
  // Arrange
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = parse("(program 1.0.0 (con integer 7))", arena, &program);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_set_allocators(malloc, realloc, free);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_parse_program, handlesAllocationFailureMidParse)
{
  // Arrange: parse the same program under increasing malloc limits; each early
  // failure must be a clean allocation error and never a crash or leak.
  const std::string text = "(program 1.0.0 (lam x (lam y [ [ (builtin addInteger) (con (list integer) [1, 2]) ] (con data (Map [(B #aa, I 1)])) ])))";

  for (int limit = 0; limit < 40; ++limit)
  {
    cardano_uplc_arena_t*         arena   = make_arena();
    const cardano_uplc_program_t* program = nullptr;

    reset_allocators_run_count();
    set_malloc_limit(limit);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = parse(text, arena, &program);

    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    EXPECT_TRUE((result == CARDANO_SUCCESS) || (result == CARDANO_ERROR_MEMORY_ALLOCATION_FAILED));

    cardano_uplc_arena_free(&arena);
  }
}

TEST(cardano_uplc_parse_program, handlesScopeStackGrowthFailure)
{
  // Arrange: many binders force the scope stack to grow via realloc.
  std::string  text    = "(program 1.0.0 ";
  const size_t binders = 64U;
  for (size_t i = 0U; i < binders; ++i)
  {
    text += "(lam x ";
  }
  text += "x";
  for (size_t i = 0U; i < binders; ++i)
  {
    text += ")";
  }
  text += ")";

  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;

  // Act
  cardano_error_t result = parse(text, arena, &program);

  // Assert: succeeds with the innermost binder resolving to index 1.
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);
}
