/**
 * \file int_overflow.cpp
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/ast/uplc_int.h"
#include "../../src/uplc/flat/flat_decode.h"

#include <gmock/gmock.h>
#include <string>

/* STATIC HELPERS ************************************************************/

namespace
{

cardano_uplc_arena_t*
new_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);
  return arena;
}

// Builds an inline (small) integer constant term, exercising the no-allocation path.
const cardano_uplc_term_t*
small_int_term(cardano_uplc_arena_t* arena, int64_t value)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_term_t*     term     = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_integer_small(arena, value, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, constant, &term), CARDANO_SUCCESS);

  return term;
}

const cardano_uplc_term_t*
builtin_term(cardano_uplc_arena_t* arena, cardano_uplc_builtin_t func)
{
  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_builtin(arena, func, &term), CARDANO_SUCCESS);
  return term;
}

const cardano_uplc_term_t*
apply(cardano_uplc_arena_t* arena, const cardano_uplc_term_t* fn, const cardano_uplc_term_t* arg)
{
  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, fn, arg, &term), CARDANO_SUCCESS);
  return term;
}

// Builds the program [[(builtin func) a] b] with inline integer operands.
cardano_uplc_program_t*
binop_program(cardano_uplc_arena_t* arena, cardano_uplc_builtin_t func, int64_t a, int64_t b)
{
  const cardano_uplc_term_t* applied =
    apply(arena, apply(arena, builtin_term(arena, func), small_int_term(arena, a)), small_int_term(arena, b));

  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));

  EXPECT_NE(program, nullptr);
  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;
  program->term          = applied;

  return program;
}

// Renders an integer constant in decimal, materializing a bigint when it is not inline.
std::string
int_const_decimal(cardano_uplc_arena_t* arena, const cardano_uplc_constant_t* constant)
{
  if (cardano_uplc_constant_int_is_small(constant))
  {
    return std::to_string(cardano_uplc_constant_int_small(constant));
  }

  const cardano_bigint_t* big = nullptr;
  EXPECT_EQ(cardano_uplc_constant_int_materialize(arena, constant, &big), CARDANO_SUCCESS);

  size_t      size = cardano_bigint_get_string_size(big, 10);
  std::string out(size, '\0');
  EXPECT_EQ(cardano_bigint_to_string(big, &out[0], size, 10), CARDANO_SUCCESS);

  if (!out.empty() && (out.back() == '\0'))
  {
    out.pop_back();
  }

  return out;
}

// Evaluates a binary integer builtin over inline operands and returns the decimal result.
std::string
eval_binop(cardano_uplc_arena_t* arena, cardano_uplc_builtin_t func, int64_t a, int64_t b, bool* is_small)
{
  cardano_uplc_program_t*    program = binop_program(arena, func, a, b);
  cardano_uplc_budget_t      budget  = { 100000000000LL, 100000000000LL };
  cardano_uplc_eval_result_t result;

  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;

  EXPECT_EQ(cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result), CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);

  if ((result.result == nullptr) || (result.result->kind != CARDANO_UPLC_TERM_CONSTANT))
  {
    return std::string("<error>");
  }

  if (is_small != nullptr)
  {
    *is_small = cardano_uplc_constant_int_is_small(result.result->as.constant);
  }

  return int_const_decimal(arena, result.result->as.constant);
}

} // namespace

/* OVERFLOW BOUNDARY TESTS *************************************************/

TEST(uplc_int_overflow, addOverflowPromotesToBigWithoutWrapping)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = true;

  // INT64_MAX + 1 overflows and must yield 9223372036854775808, not a wrapped value.
  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, INT64_MAX, 1, &is_small), "9223372036854775808");
  EXPECT_FALSE(is_small);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, subUnderflowPromotesToBigWithoutWrapping)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = true;

  // INT64_MIN - 1 underflows and must yield -9223372036854775809.
  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER, INT64_MIN, 1, &is_small), "-9223372036854775809");
  EXPECT_FALSE(is_small);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, mulOverflowPromotesToBigWithoutWrapping)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = true;

  // INT64_MAX * 2 overflows and must yield 18446744073709551614.
  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER, INT64_MAX, 2, &is_small), "18446744073709551614");
  EXPECT_FALSE(is_small);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, negateOfMinViaSubtractPromotesToBig)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = true;

  // 0 - INT64_MIN overflows (negate INT64_MIN) and must yield 9223372036854775808.
  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER, 0, INT64_MIN, &is_small), "9223372036854775808");
  EXPECT_FALSE(is_small);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, minDividedByMinusOnePromotesToBig)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = true;

  // INT64_MIN / -1 overflows int64 and must yield 9223372036854775808.
  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER, INT64_MIN, -1, &is_small), "9223372036854775808");
  EXPECT_FALSE(is_small);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, minModMinusOneStaysInlineZero)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = false;

  // INT64_MIN mod -1 is 0; even though the matching quotient overflows, the remainder re-inlines.
  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_MOD_INTEGER, INT64_MIN, -1, &is_small), "0");
  EXPECT_TRUE(is_small);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, bigResultThatShrinksReInlines)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = false;

  // (INT64_MAX + 1) - INT64_MAX shrinks back into int64 range and must re-inline as 1.
  cardano_uplc_program_t* program = nullptr;

  const cardano_uplc_term_t* sum =
    apply(arena, apply(arena, builtin_term(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER), small_int_term(arena, INT64_MAX)), small_int_term(arena, 1));

  const cardano_uplc_term_t* diff =
    apply(arena, apply(arena, builtin_term(arena, CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER), sum), small_int_term(arena, INT64_MAX));

  program = static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));
  ASSERT_NE(program, nullptr);
  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;
  program->term          = diff;

  cardano_uplc_budget_t      budget = { 100000000000LL, 100000000000LL };
  cardano_uplc_eval_result_t result;
  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;

  EXPECT_EQ(cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result), CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  ASSERT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);

  is_small = cardano_uplc_constant_int_is_small(result.result->as.constant);
  EXPECT_TRUE(is_small);
  EXPECT_EQ(int_const_decimal(arena, result.result->as.constant), "1");

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, smallArithmeticStaysInline)
{
  cardano_uplc_arena_t* arena    = new_arena();
  bool                  is_small = false;

  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, 2, 3, &is_small), "5");
  EXPECT_TRUE(is_small);

  EXPECT_EQ(eval_binop(arena, CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER, -6, 7, &is_small), "-42");
  EXPECT_TRUE(is_small);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, comparisonMixesInlineAndBigCorrectly)
{
  cardano_uplc_arena_t* arena = new_arena();

  // Build a big value (INT64_MAX + 1) and compare it against a small value via lessThanInteger.
  const cardano_uplc_term_t* big_sum =
    apply(arena, apply(arena, builtin_term(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER), small_int_term(arena, INT64_MAX)), small_int_term(arena, 1));

  // [[lessThanInteger 5] big] should be true (5 < 2^63).
  const cardano_uplc_term_t* cmp =
    apply(arena, apply(arena, builtin_term(arena, CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER), small_int_term(arena, 5)), big_sum);

  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));
  ASSERT_NE(program, nullptr);
  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;
  program->term          = cmp;

  cardano_uplc_budget_t      budget = { 100000000000LL, 100000000000LL };
  cardano_uplc_eval_result_t result;
  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;

  EXPECT_EQ(cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result), CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  ASSERT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(result.result->as.constant->kind, CARDANO_UPLC_TYPE_BOOL);
  EXPECT_TRUE(result.result->as.constant->as.boolean);

  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, inlineBigRoundTripThroughEncodeDecode)
{
  cardano_uplc_arena_t* arena = new_arena();

  // A program carrying both an inline and a big integer constant must survive
  // encode then decode with the same values and the same inline/big classification.
  const cardano_uplc_term_t* big_sum =
    apply(arena, apply(arena, builtin_term(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER), small_int_term(arena, INT64_MAX)), small_int_term(arena, 1));

  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));
  ASSERT_NE(program, nullptr);
  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;

  // Evaluate to obtain the big constant, then wrap it in a fresh constant program.
  {
    cardano_uplc_program_t* run =
      static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));
    ASSERT_NE(run, nullptr);
    run->version_major = 1U;
    run->version_minor = 0U;
    run->version_patch = 0U;
    run->term          = big_sum;

    cardano_uplc_budget_t      budget = { 100000000000LL, 100000000000LL };
    cardano_uplc_eval_result_t result;
    result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
    result.result = nullptr;

    EXPECT_EQ(cardano_uplc_evaluate(arena, run, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result), CARDANO_SUCCESS);
    ASSERT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
    ASSERT_NE(result.result, nullptr);

    cardano_uplc_term_t* big_term = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_constant(arena, result.result->as.constant, &big_term), CARDANO_SUCCESS);
    program->term = big_term;
  }

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(program, &cbor), CARDANO_SUCCESS);
  ASSERT_NE(cbor, nullptr);

  cardano_uplc_arena_t*         arena2  = new_arena();
  const cardano_uplc_program_t* decoded = nullptr;
  EXPECT_EQ(
    cardano_uplc_program_from_script_bytes(arena2, cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &decoded),
    CARDANO_SUCCESS);
  ASSERT_NE(decoded, nullptr);
  ASSERT_EQ(decoded->term->kind, CARDANO_UPLC_TERM_CONSTANT);
  ASSERT_EQ(decoded->term->as.constant->kind, CARDANO_UPLC_TYPE_INTEGER);

  // The big value stays big and round-trips to the same decimal value.
  EXPECT_FALSE(cardano_uplc_constant_int_is_small(decoded->term->as.constant));
  EXPECT_EQ(int_const_decimal(arena2, decoded->term->as.constant), "9223372036854775808");

  cardano_buffer_unref(&cbor);
  cardano_uplc_arena_free(&arena2);
  cardano_uplc_arena_free(&arena);
}

TEST(uplc_int_overflow, smallConstantDecodesAsInline)
{
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));
  ASSERT_NE(program, nullptr);
  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;
  program->term          = small_int_term(arena, -123456789);

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(program, &cbor), CARDANO_SUCCESS);
  ASSERT_NE(cbor, nullptr);

  cardano_uplc_arena_t*         arena2  = new_arena();
  const cardano_uplc_program_t* decoded = nullptr;
  EXPECT_EQ(
    cardano_uplc_program_from_script_bytes(arena2, cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &decoded),
    CARDANO_SUCCESS);
  ASSERT_NE(decoded, nullptr);
  ASSERT_EQ(decoded->term->as.constant->kind, CARDANO_UPLC_TYPE_INTEGER);

  EXPECT_TRUE(cardano_uplc_constant_int_is_small(decoded->term->as.constant));
  EXPECT_EQ(cardano_uplc_constant_int_small(decoded->term->as.constant), -123456789);

  cardano_buffer_unref(&cbor);
  cardano_uplc_arena_free(&arena2);
  cardano_uplc_arena_free(&arena);
}
