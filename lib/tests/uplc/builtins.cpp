/**
 * \file builtins.cpp
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

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include "../../src/uplc/builtins/uplc_builtin.h"
#include "../../src/uplc/machine/uplc_machine.h"
#include "../../src/uplc/ast/uplc_term.h"

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/builtins/builtins.h"
#include "../../src/uplc/machine/uplc_value.h"
#include "../../src/uplc/syntax/pretty.h"
#include "../../src/uplc/syntax/text_parser.h"

#include <cstdio>
#include <cstring>
#include <gmock/gmock.h>
#include <string>

/* STATIC HELPERS ************************************************************/

static cardano_uplc_arena_t*
new_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);
  return arena;
}

static const cardano_uplc_value_t*
int_value(cardano_uplc_arena_t* arena, int64_t n)
{
  cardano_bigint_t*        bigint   = nullptr;
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_bigint_from_int(n, &bigint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, bigint, &constant), CARDANO_SUCCESS);
  cardano_bigint_unref(&bigint);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
bytes_value(cardano_uplc_arena_t* arena, const std::string& bytes)
{
  cardano_buffer_t*        buffer   = cardano_buffer_new_from(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_byte_string(arena, buffer, &constant), CARDANO_SUCCESS);
  cardano_buffer_unref(&buffer);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
string_value(cardano_uplc_arena_t* arena, const char* utf8)
{
  size_t                   len      = std::strlen(utf8);
  cardano_buffer_t*        buffer   = cardano_buffer_new_from(reinterpret_cast<const uint8_t*>(utf8), len);
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_string(arena, buffer, &constant), CARDANO_SUCCESS);
  cardano_buffer_unref(&buffer);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
bool_value(cardano_uplc_arena_t* arena, bool b)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_bool(arena, b, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
unit_value(cardano_uplc_arena_t* arena)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
unit_list_value(cardano_uplc_arena_t* arena)
{
  cardano_uplc_type_t*     element  = nullptr;
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_UNIT, nullptr, nullptr, &element), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_list(arena, element, nullptr, 0U, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
pair_value(cardano_uplc_arena_t* arena)
{
  cardano_uplc_constant_t* fst      = nullptr;
  cardano_uplc_constant_t* snd      = nullptr;
  cardano_bigint_t*        a        = nullptr;
  cardano_bigint_t*        b        = nullptr;
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_bigint_from_int(1, &a), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, a, &fst), CARDANO_SUCCESS);
  cardano_bigint_unref(&a);

  EXPECT_EQ(cardano_bigint_from_int(2, &b), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, b, &snd), CARDANO_SUCCESS);
  cardano_bigint_unref(&b);

  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, fst, snd, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
data_value(cardano_uplc_arena_t* arena, int64_t n)
{
  cardano_plutus_data_t*   data     = nullptr;
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(n, &data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, data, &constant), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&data);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_value_t*
delay_value(cardano_uplc_arena_t* arena)
{
  cardano_uplc_term_t*  body  = nullptr;
  cardano_uplc_value_t* value = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &body), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_value_new_delay(arena, body, nullptr, &value), CARDANO_SUCCESS);

  return value;
}

static cardano_uplc_eval_status_t
evaluate_program(const char* source)
{
  cardano_uplc_arena_t*         arena   = new_arena();
  const cardano_uplc_program_t* program = nullptr;
  size_t                        offset  = 0U;

  EXPECT_EQ(
    cardano_uplc_parse_program(arena, source, std::strlen(source), &program, &offset),
    CARDANO_SUCCESS)
    << "failed to parse at offset " << offset;

  const cardano_uplc_budget_t      initial = { 100000000000LL, 100000000000LL };
  cardano_uplc_eval_result_t       result  = {};
  const cardano_error_t            host    =
    cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, initial, &result);

  EXPECT_EQ(host, CARDANO_SUCCESS);

  const cardano_uplc_eval_status_t status = result.status;

  cardano_uplc_arena_free(&arena);

  return status;
}

/* UNWRAP HELPERS - SUCCESS *************************************************/

TEST(cardano_uplc_builtin_as_integer, returnsTheIntegerPayload)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = int_value(arena, 42);
  const cardano_bigint_t*     out   = nullptr;

  EXPECT_TRUE(cardano_uplc_builtin_as_integer(value, &out));
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(cardano_bigint_to_int(out), 42);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_byte_string, returnsTheByteStringPayload)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = bytes_value(arena, std::string("\x01\x02\x03", 3));
  cardano_uplc_byte_view_t    out   = { nullptr, 0U };

  EXPECT_TRUE(cardano_uplc_builtin_as_byte_string(value, &out));
  ASSERT_NE(out.data, nullptr);
  EXPECT_EQ(out.size, 3U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_string, returnsTheStringPayload)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = string_value(arena, "hello");
  cardano_uplc_byte_view_t    out   = { nullptr, 0U };

  EXPECT_TRUE(cardano_uplc_builtin_as_string(value, &out));
  ASSERT_NE(out.data, nullptr);
  EXPECT_EQ(out.size, 5U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_bool, returnsTheBooleanPayload)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = bool_value(arena, true);
  bool                        out   = false;

  EXPECT_TRUE(cardano_uplc_builtin_as_bool(value, &out));
  EXPECT_TRUE(out);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_unit, acceptsTheUnitConstant)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = unit_value(arena);

  EXPECT_TRUE(cardano_uplc_builtin_as_unit(value));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_list, returnsTheElementTypeAndItems)
{
  cardano_uplc_arena_t*                  arena   = new_arena();
  const cardano_uplc_value_t*            value   = unit_list_value(arena);
  const cardano_uplc_type_t*             element = nullptr;
  const cardano_uplc_constant_t* const*  items   = nullptr;
  size_t                                 count   = 99U;

  EXPECT_TRUE(cardano_uplc_builtin_as_list(value, &element, &items, &count));
  ASSERT_NE(element, nullptr);
  EXPECT_EQ(element->kind, CARDANO_UPLC_TYPE_UNIT);
  EXPECT_EQ(count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_pair, returnsBothComponents)
{
  cardano_uplc_arena_t*          arena = new_arena();
  const cardano_uplc_value_t*    value = pair_value(arena);
  const cardano_uplc_constant_t* fst   = nullptr;
  const cardano_uplc_constant_t* snd   = nullptr;

  EXPECT_TRUE(cardano_uplc_builtin_as_pair(value, &fst, &snd));
  ASSERT_NE(fst, nullptr);
  ASSERT_NE(snd, nullptr);
  EXPECT_EQ(fst->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(snd->kind, CARDANO_UPLC_TYPE_INTEGER);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_data, returnsThePlutusDataPayload)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = data_value(arena, 42);
  const cardano_uplc_data_t*  out   = nullptr;

  // Act
  const bool ok = cardano_uplc_builtin_as_data(value, &out);

  // Assert
  EXPECT_TRUE(ok);
  ASSERT_NE(out, nullptr);

  EXPECT_EQ(out->kind, CARDANO_UPLC_DATA_KIND_INTEGER);
  EXPECT_TRUE(out->as.integer.is_small);
  EXPECT_EQ(out->as.integer.small, 42);

  cardano_uplc_arena_free(&arena);
}

/* UNWRAP HELPERS - TYPE MISMATCH (SCRIPT ERROR) ***************************/

TEST(cardano_uplc_builtin_as_integer, rejectsAByteStringValue)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = bytes_value(arena, std::string("\x01", 1));
  const cardano_bigint_t*     out   = nullptr;

  EXPECT_FALSE(cardano_uplc_builtin_as_integer(value, &out));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_byte_string, rejectsAnIntegerValue)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = int_value(arena, 7);
  cardano_uplc_byte_view_t    out   = { nullptr, 0U };

  EXPECT_FALSE(cardano_uplc_builtin_as_byte_string(value, &out));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_bool, rejectsAnIntegerValue)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = int_value(arena, 0);
  bool                        out   = true;

  EXPECT_FALSE(cardano_uplc_builtin_as_bool(value, &out));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_unit, rejectsAnIntegerValue)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = int_value(arena, 0);

  EXPECT_FALSE(cardano_uplc_builtin_as_unit(value));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_list, rejectsAPairValue)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = pair_value(arena);

  EXPECT_FALSE(cardano_uplc_builtin_as_list(value, nullptr, nullptr, nullptr));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_pair, rejectsAListValue)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = unit_list_value(arena);

  EXPECT_FALSE(cardano_uplc_builtin_as_pair(value, nullptr, nullptr));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_data, rejectsAnIntegerValue)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = int_value(arena, 7);
  const cardano_uplc_data_t*  out   = nullptr;

  // Act
  const bool ok = cardano_uplc_builtin_as_data(value, &out);

  // Assert
  EXPECT_FALSE(ok);
  EXPECT_EQ(out, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_string, rejectsAByteStringValue)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = bytes_value(arena, std::string("\x01", 1));
  cardano_uplc_byte_view_t    out   = { nullptr, 0U };

  // Act
  const bool ok = cardano_uplc_builtin_as_string(value, &out);

  // Assert
  EXPECT_FALSE(ok);
  EXPECT_EQ(out.data, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_integer, rejectsANonConstantValue)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = delay_value(arena);
  const cardano_bigint_t*     out   = nullptr;

  EXPECT_FALSE(cardano_uplc_builtin_as_integer(value, &out));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_as_integer, rejectsANullOut)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = int_value(arena, 1);

  EXPECT_FALSE(cardano_uplc_builtin_as_integer(value, nullptr));

  cardano_uplc_arena_free(&arena);
}

/* DISPATCH - SATURATED VALUE BUILTIN ************************************/

TEST(cardano_uplc_builtin_run, saturatedValueBuiltinRejectsMismatchedArgsAsScriptError)
{
  // unionValue is implemented and takes two value arguments. Feeding it integer
  // arguments is a type mismatch, which is a script failure (an error term),
  // distinct from the UNSUPPORTED status reserved for unimplemented builtins.
  const cardano_uplc_eval_status_t status =
    evaluate_program("(program 1.0.0 [ [ (builtin unionValue) (con integer 0) ] (con integer 0) ])");

  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
  EXPECT_NE(status, CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN);
}

TEST(cardano_uplc_builtin_run, saturatedValueBuiltinEvaluatesValueArgs)
{
  // unionValue applied to two value constants is implemented and succeeds.
  const cardano_uplc_eval_status_t status =
    evaluate_program("(program 1.0.0 [ [ (builtin unionValue) (con value [(#, [(#, 1)])]) ] (con value [(#, [(#, 1)])]) ])");

  EXPECT_EQ(status, CARDANO_UPLC_EVAL_SUCCESS);
}

/* DISPATCH - UNSATURATED BUILTIN STILL DISCHARGES *************************/

TEST(cardano_uplc_builtin_run, unsaturatedBuiltinEvaluatesToAValue)
{
  // A builtin applied to fewer than its arity arguments is not run; it discharges
  // to the partially applied builtin term, a successful evaluation.
  const cardano_uplc_eval_status_t status =
    evaluate_program("(program 1.0.0 [ (builtin addInteger) (con integer 1) ])");

  EXPECT_EQ(status, CARDANO_UPLC_EVAL_SUCCESS);
}

TEST(cardano_uplc_builtin_run, bareBuiltinEvaluatesToAValue)
{
  const cardano_uplc_eval_status_t status =
    evaluate_program("(program 1.0.0 (builtin addInteger))");

  EXPECT_EQ(status, CARDANO_UPLC_EVAL_SUCCESS);
}

/* BUILTIN BODIES - RESULT AND ERROR PATHS ********************************/

static std::string
evaluate_to_render(const char* source, cardano_uplc_eval_status_t* status_out)
{
  cardano_uplc_arena_t*         arena   = new_arena();
  const cardano_uplc_program_t* program = nullptr;
  size_t                        offset  = 0U;

  EXPECT_EQ(
    cardano_uplc_parse_program(arena, source, std::strlen(source), &program, &offset),
    CARDANO_SUCCESS)
    << "failed to parse at offset " << offset;

  const cardano_uplc_budget_t initial = { 100000000000LL, 100000000000LL };
  cardano_uplc_eval_result_t  result  = {};
  EXPECT_EQ(cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, initial, &result), CARDANO_SUCCESS);

  *status_out = result.status;

  std::string rendered;
  if ((result.status == CARDANO_UPLC_EVAL_SUCCESS) && (result.result != nullptr))
  {
    cardano_buffer_t* buffer = nullptr;
    if (cardano_uplc_pretty_print_term(result.result, &buffer) == CARDANO_SUCCESS)
    {
      rendered.assign(reinterpret_cast<const char*>(cardano_buffer_get_data(buffer)), cardano_buffer_get_size(buffer));
      cardano_buffer_unref(&buffer);
      while (!rendered.empty() && (rendered.back() == '\0'))
      {
        rendered.pop_back();
      }
    }
  }

  cardano_uplc_arena_free(&arena);

  return rendered;
}

static std::string
eval_ok(const char* source)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_ERROR_TERM;
  std::string                out    = evaluate_to_render(source, &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_SUCCESS);
  return out;
}

TEST(cardano_uplc_builtin_body, blsG1ScalarMulPositiveNegativeAndZero)
{
  static const char* const kG =
    "(con bls12_381_G1_element 0x97f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb)";

  std::string mul_by_one =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G1_equal) [ [ (builtin bls12_381_G1_scalarMul) (con integer 1) ] ") + kG + " ] ] " + kG + " ])";
  EXPECT_EQ(eval_ok(mul_by_one.c_str()), "(con bool True)");

  std::string mul_by_neg_one =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G1_equal) [ [ (builtin bls12_381_G1_scalarMul) (con integer -1) ] ") + kG + " ] ] [ (builtin bls12_381_G1_neg) " + kG + " ] ])";
  EXPECT_EQ(eval_ok(mul_by_neg_one.c_str()), "(con bool True)");

  std::string mul_by_zero =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G1_equal) [ [ (builtin bls12_381_G1_scalarMul) (con integer 0) ] ") + kG + " ] ] [ [ (builtin bls12_381_G1_add) " + kG + " ] [ (builtin bls12_381_G1_neg) " + kG + " ] ] ])";
  EXPECT_EQ(eval_ok(mul_by_zero.c_str()), "(con bool True)");
}

TEST(cardano_uplc_builtin_body, blsG2ScalarMulPositiveNegativeAndZero)
{
  static const char* const kG =
    "(con bls12_381_G2_element 0x93e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb8)";

  std::string mul_by_one =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G2_equal) [ [ (builtin bls12_381_G2_scalarMul) (con integer 1) ] ") + kG + " ] ] " + kG + " ])";
  EXPECT_EQ(eval_ok(mul_by_one.c_str()), "(con bool True)");

  std::string mul_by_neg_one =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G2_equal) [ [ (builtin bls12_381_G2_scalarMul) (con integer -1) ] ") + kG + " ] ] [ (builtin bls12_381_G2_neg) " + kG + " ] ])";
  EXPECT_EQ(eval_ok(mul_by_neg_one.c_str()), "(con bool True)");

  std::string mul_by_zero =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G2_equal) [ [ (builtin bls12_381_G2_scalarMul) (con integer 0) ] ") + kG + " ] ] [ [ (builtin bls12_381_G2_add) " + kG + " ] [ (builtin bls12_381_G2_neg) " + kG + " ] ] ])";
  EXPECT_EQ(eval_ok(mul_by_zero.c_str()), "(con bool True)");
}

TEST(cardano_uplc_builtin_body, dropListDropsAPrefixAndKeepsTheElementType)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin dropList)) (con integer 2) ] (con (list integer) [1, 2, 3, 4, 5]) ])"),
    "(con (list integer) [3, 4, 5])");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin dropList)) (con integer 0) ] (con (list integer) [1, 2, 3]) ])"),
    "(con (list integer) [1, 2, 3])");
}

TEST(cardano_uplc_builtin_body, dropListNegativeCountKeepsTheWholeListAndAtOrAboveLengthIsEmpty)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin dropList)) (con integer -3) ] (con (list integer) [1, 2, 3]) ])"),
    "(con (list integer) [1, 2, 3])");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin dropList)) (con integer 9) ] (con (list integer) [1, 2, 3]) ])"),
    "(con (list integer) [])");
}

TEST(cardano_uplc_builtin_body, dropListRejectsANonListSecondArgument)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ [ (force (builtin dropList)) (con integer 1) ] (con integer 7) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, listToArrayThenLengthOfArray)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (builtin listToArray)) (con (list integer) [10, 20, 30]) ])"),
    "(con (array integer) [10, 20, 30])");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (builtin lengthOfArray)) (con (array integer) [10, 20, 30]) ])"),
    "(con integer 3)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (builtin lengthOfArray)) (con (array bool) []) ])"),
    "(con integer 0)");
}

TEST(cardano_uplc_builtin_body, lengthOfArrayRejectsAList)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ (force (builtin lengthOfArray)) (con (list integer) [1, 2]) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, indexArrayReturnsTheElementAtAnIndex)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin indexArray)) (con (array integer) [10, 20, 30]) ] (con integer 0) ])"),
    "(con integer 10)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin indexArray)) (con (array integer) [10, 20, 30]) ] (con integer 2) ])"),
    "(con integer 30)");
}

TEST(cardano_uplc_builtin_body, indexArrayNegativeOrOutOfRangeIsAScriptError)
{
  cardano_uplc_eval_status_t negative = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ [ (force (builtin indexArray)) (con (array integer) [10, 20]) ] (con integer -1) ])", &negative);
  EXPECT_EQ(negative, CARDANO_UPLC_EVAL_ERROR_TERM);

  cardano_uplc_eval_status_t past_end = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ [ (force (builtin indexArray)) (con (array integer) [10, 20]) ] (con integer 2) ])", &past_end);
  EXPECT_EQ(past_end, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, indexArrayRejectsAList)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ [ (force (builtin indexArray)) (con (list integer) [1, 2]) ] (con integer 0) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, blsG1MultiScalarMulMatchesScalarMulAndIsIdentityWhenEmpty)
{
  static const char* const kG =
    "(con bls12_381_G1_element 0x97f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb)";

  std::string g1_point = "0x97f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb";
  std::string msm =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G1_equal) ")
    + "[ [ (builtin bls12_381_G1_multiScalarMul) (con (list integer) [3]) ] (con (list bls12_381_G1_element) [" + g1_point + "]) ] ] "
    + "[ [ (builtin bls12_381_G1_scalarMul) (con integer 3) ] " + std::string(kG) + " ] ])";
  EXPECT_EQ(eval_ok(msm.c_str()), "(con bool True)");

  std::string empty =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G1_equal) ")
    + "[ [ (builtin bls12_381_G1_multiScalarMul) (con (list integer) []) ] (con (list bls12_381_G1_element) []) ] ] "
    + "(con bls12_381_G1_element 0xc00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000) ])";
  EXPECT_EQ(eval_ok(empty.c_str()), "(con bool True)");
}

TEST(cardano_uplc_builtin_body, blsG2MultiScalarMulMatchesScalarMul)
{
  static const char* const kG =
    "(con bls12_381_G2_element 0x93e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb8)";

  std::string g2_point =
    "0x93e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb8";
  std::string msm =
    std::string("(program 1.0.0 [ [ (builtin bls12_381_G2_equal) ")
    + "[ [ (builtin bls12_381_G2_multiScalarMul) (con (list integer) [5]) ] (con (list bls12_381_G2_element) [" + g2_point + "]) ] ] "
    + "[ [ (builtin bls12_381_G2_scalarMul) (con integer 5) ] " + std::string(kG) + " ] ])";
  EXPECT_EQ(eval_ok(msm.c_str()), "(con bool True)");
}

TEST(cardano_uplc_builtin_body, addAndSubtractAndMultiply)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin addInteger) (con integer 2) ] (con integer 3) ])"), "(con integer 5)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin subtractInteger) (con integer 2) ] (con integer 5) ])"), "(con integer -3)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin multiplyInteger) (con integer -4) ] (con integer 3) ])"), "(con integer -12)");
}

TEST(cardano_uplc_builtin_body, divideAndModFloorRoundTowardNegativeInfinity)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin divideInteger) (con integer 7) ] (con integer 2) ])"), "(con integer 3)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin divideInteger) (con integer -7) ] (con integer 2) ])"), "(con integer -4)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin divideInteger) (con integer 7) ] (con integer -2) ])"), "(con integer -4)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin divideInteger) (con integer -7) ] (con integer -2) ])"), "(con integer 3)");

  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin modInteger) (con integer -7) ] (con integer 2) ])"), "(con integer 1)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin modInteger) (con integer 7) ] (con integer -2) ])"), "(con integer -1)");
}

TEST(cardano_uplc_builtin_body, quotientAndRemainderTruncateTowardZero)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin quotientInteger) (con integer -7) ] (con integer 2) ])"), "(con integer -3)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin quotientInteger) (con integer 7) ] (con integer -2) ])"), "(con integer -3)");

  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin remainderInteger) (con integer -7) ] (con integer 2) ])"), "(con integer -1)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin remainderInteger) (con integer 7) ] (con integer -2) ])"), "(con integer 1)");
}

TEST(cardano_uplc_builtin_body, divideByZeroIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ [ (builtin divideInteger) (con integer 7) ] (con integer 0) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, integerComparisons)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin equalsInteger) (con integer 3) ] (con integer 3) ])"), "(con bool True)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin lessThanInteger) (con integer 2) ] (con integer 3) ])"), "(con bool True)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin lessThanEqualsInteger) (con integer 3) ] (con integer 3) ])"), "(con bool True)");
}

TEST(cardano_uplc_builtin_body, appendConsAndSlice)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin appendByteString) (con bytestring #0102) ] (con bytestring #0304) ])"), "(con bytestring #01020304)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin consByteString) (con integer 255) ] (con bytestring #0102) ])"), "(con bytestring #ff0102)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ [ (builtin sliceByteString) (con integer 1) ] (con integer 2) ] (con bytestring #01020304) ])"), "(con bytestring #0203)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ [ (builtin sliceByteString) (con integer 2) ] (con integer 99) ] (con bytestring #01020304) ])"), "(con bytestring #0304)");
}

TEST(cardano_uplc_builtin_body, consByteStringRangeCheckRejectsOutOfRangeByte)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ [ (builtin consByteString) (con integer 256) ] (con bytestring #01) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, lengthAndIndex)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ (builtin lengthOfByteString) (con bytestring #010203) ])"), "(con integer 3)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin indexByteString) (con bytestring #0a0b0c) ] (con integer 1) ])"), "(con integer 11)");
}

TEST(cardano_uplc_builtin_body, indexByteStringOutOfRangeIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ [ (builtin indexByteString) (con bytestring #0a0b) ] (con integer 2) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, byteStringComparisons)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin equalsByteString) (con bytestring #0102) ] (con bytestring #0102) ])"), "(con bool True)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin lessThanByteString) (con bytestring #0102) ] (con bytestring #0103) ])"), "(con bool True)");
}

TEST(cardano_uplc_builtin_body, bitwiseAndOrXorWithPad)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ [ (builtin andByteString) (con bool False) ] (con bytestring #ff0f) ] (con bytestring #0fff) ])"), "(con bytestring #0f0f)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ [ (builtin orByteString) (con bool False) ] (con bytestring #f000) ] (con bytestring #000f) ])"), "(con bytestring #f00f)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ [ (builtin xorByteString) (con bool True) ] (con bytestring #ff) ] (con bytestring #ff00) ])"), "(con bytestring #0000)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ (builtin complementByteString) (con bytestring #0fff) ])"), "(con bytestring #f000)");
}

TEST(cardano_uplc_builtin_body, countAndFindBits)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ (builtin countSetBits) (con bytestring #ff01) ])"), "(con integer 9)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ (builtin findFirstSetBit) (con bytestring #0080) ])"), "(con integer 7)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ (builtin findFirstSetBit) (con bytestring #0000) ])"), "(con integer -1)");
}

TEST(cardano_uplc_builtin_body, shiftAndRotateAndReplicate)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin shiftByteString) (con bytestring #01) ] (con integer 1) ])"), "(con bytestring #02)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin shiftByteString) (con bytestring #80) ] (con integer -1) ])"), "(con bytestring #40)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin rotateByteString) (con bytestring #80) ] (con integer 1) ])"), "(con bytestring #01)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin replicateByte) (con integer 3) ] (con integer 171) ])"), "(con bytestring #ababab)");
}

TEST(cardano_uplc_builtin_body, integerByteStringRoundTrip)
{
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ [ (builtin integerToByteString) (con bool True) ] (con integer 4) ] (con integer 258) ])"), "(con bytestring #00000102)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ [ (builtin integerToByteString) (con bool False) ] (con integer 4) ] (con integer 258) ])"), "(con bytestring #02010000)");
  EXPECT_EQ(eval_ok("(program 1.0.0 [ [ (builtin byteStringToInteger) (con bool True) ] (con bytestring #0102) ])"), "(con integer 258)");
}

/* BOOL, UNIT, STRING, PAIR AND LIST BODIES ******************************/

TEST(cardano_uplc_builtin_body, ifThenElseSelectsTheChosenBranch)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ [ (force (builtin ifThenElse)) (con bool True) ] (con integer 1) ] (con integer 2) ])"),
    "(con integer 1)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ [ (force (builtin ifThenElse)) (con bool False) ] (con integer 1) ] (con integer 2) ])"),
    "(con integer 2)");
}

TEST(cardano_uplc_builtin_body, chooseUnitTypeChecksAndReturnsSecond)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin chooseUnit)) (con unit ()) ] (con integer 7) ])"),
    "(con integer 7)");
}

TEST(cardano_uplc_builtin_body, chooseUnitOnNonUnitIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render(
    "(program 1.0.0 [ [ (force (builtin chooseUnit)) (con integer 0) ] (con integer 7) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, traceTypeChecksAndReturnsSecond)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin trace)) (con string \"hi\") ] (con integer 9) ])"),
    "(con integer 9)");
}

TEST(cardano_uplc_builtin_body, traceOnNonStringIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render(
    "(program 1.0.0 [ [ (force (builtin trace)) (con integer 0) ] (con integer 9) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, appendAndEqualsString)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin appendString) (con string \"Ola \") ] (con string \"mundo!\") ])"),
    "(con string \"Ola mundo!\")");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin equalsString) (con string \"ab\") ] (con string \"ab\") ])"),
    "(con bool True)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin equalsString) (con string \"ab\") ] (con string \"ac\") ])"),
    "(con bool False)");
}

TEST(cardano_uplc_builtin_body, encodeAndDecodeUtf8RoundTrip)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin encodeUtf8) (con string \"Ola\") ])"),
    "(con bytestring #4f6c61)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin decodeUtf8) (con bytestring #4f6c61) ])"),
    "(con string \"Ola\")");
}

TEST(cardano_uplc_builtin_body, decodeUtf8OnInvalidBytesIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ (builtin decodeUtf8) (con bytestring #ff) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, fstPairAndSndPair)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (force (builtin fstPair))) (con (pair integer bool) (42, False)) ])"),
    "(con integer 42)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (force (builtin sndPair))) (con (pair integer bool) (42, False)) ])"),
    "(con bool False)");
}

TEST(cardano_uplc_builtin_body, chooseListBranchesOnEmptiness)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ [ (force (force (builtin chooseList))) (con (list integer) []) ] (con integer 1) ] (con integer 2) ])"),
    "(con integer 1)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ [ (force (force (builtin chooseList))) (con (list integer) [9]) ] (con integer 1) ] (con integer 2) ])"),
    "(con integer 2)");
}

TEST(cardano_uplc_builtin_body, mkConsPrependsAnElement)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (force (builtin mkCons)) (con integer 0) ] (con (list integer) [1,2]) ])"),
    "(con (list integer) [0, 1, 2])");
}

TEST(cardano_uplc_builtin_body, mkConsOnTypeMismatchIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render(
    "(program 1.0.0 [ [ (force (builtin mkCons)) (con bool True) ] (con (list integer) [1,2]) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, headAndTailList)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (builtin headList)) (con (list integer) [3,4,5]) ])"),
    "(con integer 3)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (builtin tailList)) (con (list integer) [3,4,5]) ])"),
    "(con (list integer) [4, 5])");
}

TEST(cardano_uplc_builtin_body, headListOnEmptyIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ (force (builtin headList)) (con (list integer) []) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, tailListOnEmptyIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ (force (builtin tailList)) (con (list integer) []) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, nullList)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (builtin nullList)) (con (list integer) []) ])"),
    "(con bool True)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (builtin nullList)) (con (list integer) [1]) ])"),
    "(con bool False)");
}

/* DATA BUILTINS **********************************************************/

TEST(cardano_uplc_builtin_body, iDataAndUnIDataRoundTrip)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin iData) (con integer 42) ])"),
    "(con data (I 42))");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin unIData) [ (builtin iData) (con integer -7) ] ])"),
    "(con integer -7)");
}

TEST(cardano_uplc_builtin_body, bDataAndUnBDataRoundTrip)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin bData) (con bytestring #deadbeef) ])"),
    "(con data (B #deadbeef))");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin unBData) [ (builtin bData) (con bytestring #cafe) ] ])"),
    "(con bytestring #cafe)");
}

TEST(cardano_uplc_builtin_body, constrDataStoresTheRawAlternative)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin constrData) (con integer 1) ] (con (list data) [I 7, I 9]) ])"),
    "(con data (Constr 1 [I 7, I 9]))");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin constrData) (con integer 200) ] (con (list data) []) ])"),
    "(con data (Constr 200 []))");
}

TEST(cardano_uplc_builtin_body, unConstrDataSurfacesTagAndFields)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (force (builtin fstPair))) "
            "[ (builtin unConstrData) (con data (Constr 5 [I 1])) ] ])"),
    "(con integer 5)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (force (builtin sndPair))) "
            "[ (builtin unConstrData) (con data (Constr 5 [I 1, I 2])) ] ])"),
    "(con (list data) [I 1, I 2])");
}

TEST(cardano_uplc_builtin_body, listDataAndUnListDataRoundTrip)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin listData) (con (list data) [I 1, I 2]) ])"),
    "(con data (List [I 1, I 2]))");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin unListData) (con data (List [I 3, I 4])) ])"),
    "(con (list data) [I 3, I 4])");
}

TEST(cardano_uplc_builtin_body, mapDataAndUnMapDataRoundTrip)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin mapData) "
            "(con (list (pair data data)) [(I 1, B #ff)]) ])"),
    "(con data (Map [(I 1, B #ff)]))");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin unMapData) "
            "(con data (Map [(I 1, I 2)])) ])"),
    "(con (list (pair data data)) [(I 1, I 2)])");
}

TEST(cardano_uplc_builtin_body, chooseDataSelectsByKind)
{
  const char* tmpl =
    "(program 1.0.0 [ [ [ [ [ [ (force (builtin chooseData)) %s ] "
    "(con integer 1) ] (con integer 2) ] (con integer 3) ] (con integer 4) ] (con integer 5) ])";

  char buffer[512];

  std::snprintf(buffer, sizeof(buffer), tmpl, "(con data (Constr 0 []))");
  EXPECT_EQ(eval_ok(buffer), "(con integer 1)");

  std::snprintf(buffer, sizeof(buffer), tmpl, "(con data (Map []))");
  EXPECT_EQ(eval_ok(buffer), "(con integer 2)");

  std::snprintf(buffer, sizeof(buffer), tmpl, "(con data (List []))");
  EXPECT_EQ(eval_ok(buffer), "(con integer 3)");

  std::snprintf(buffer, sizeof(buffer), tmpl, "(con data (I 99))");
  EXPECT_EQ(eval_ok(buffer), "(con integer 4)");

  std::snprintf(buffer, sizeof(buffer), tmpl, "(con data (B #ab))");
  EXPECT_EQ(eval_ok(buffer), "(con integer 5)");
}

TEST(cardano_uplc_builtin_body, equalsDataTrueAndFalse)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin equalsData) (con data (I 5)) ] (con data (I 5)) ])"),
    "(con bool True)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin equalsData) (con data (I 5)) ] (con data (I 6)) ])"),
    "(con bool False)");
}

TEST(cardano_uplc_builtin_body, mkPairDataBuildsAProtoPair)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ [ (builtin mkPairData) (con data (I 1)) ] (con data (B #ff)) ])"),
    "(con (pair data data) (I 1, B #ff))");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (force (force (builtin fstPair))) "
            "[ [ (builtin mkPairData) (con data (I 1)) ] (con data (B #ff)) ] ])"),
    "(con data (I 1))");
}

TEST(cardano_uplc_builtin_body, mkNilDataAndMkNilPairDataBuildEmptyLists)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin mkNilData) (con unit ()) ])"),
    "(con (list data) [])");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin mkNilPairData) (con unit ()) ])"),
    "(con (list (pair data data)) [])");
}

TEST(cardano_uplc_builtin_body, serialiseDataProducesCanonicalCbor)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin serialiseData) (con data (I 5)) ])"),
    "(con bytestring #05)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin serialiseData) (con data (B #deadbeef)) ])"),
    "(con bytestring #44deadbeef)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin serialiseData) (con data (Constr 0 [I 1])) ])"),
    "(con bytestring #d8799f01ff)");
}

TEST(cardano_uplc_builtin_body, dataDeconstructorOnWrongShapeIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ (builtin unIData) (con data (B #ff)) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);

  status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ (builtin unConstrData) (con data (I 1)) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);

  status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render("(program 1.0.0 [ (builtin unBData) (con data (List [])) ])", &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

/* CRYPTO BUILTINS - KNOWN-ANSWER TESTS **********************************/

TEST(cardano_uplc_builtin_body, sha2256KnownAnswers)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin sha2_256) (con bytestring #) ])"),
    "(con bytestring #e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin sha2_256) (con bytestring #616263) ])"),
    "(con bytestring #ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad)");
}

TEST(cardano_uplc_builtin_body, blake2b256KnownAnswers)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin blake2b_256) (con bytestring #) ])"),
    "(con bytestring #0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin blake2b_256) (con bytestring #616263) ])"),
    "(con bytestring #bddd813c634239723171ef3fee98579b94964e3bb1cb3e427262c8c068d52319)");
}

TEST(cardano_uplc_builtin_body, blake2b224KnownAnswer)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin blake2b_224) (con bytestring #616263) ])"),
    "(con bytestring #9bd237b02a29e43bdd6738afa5b53ff0eee178d6210b618e4511aec8)");
}

TEST(cardano_uplc_builtin_body, sha3256KnownAnswers)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin sha3_256) (con bytestring #) ])"),
    "(con bytestring #a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin sha3_256) (con bytestring #616263) ])"),
    "(con bytestring #3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532)");
}

TEST(cardano_uplc_builtin_body, keccak256KnownAnswers)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin keccak_256) (con bytestring #) ])"),
    "(con bytestring #c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin keccak_256) (con bytestring #616263) ])"),
    "(con bytestring #4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45)");
}

TEST(cardano_uplc_builtin_body, ripemd160KnownAnswers)
{
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin ripemd_160) (con bytestring #) ])"),
    "(con bytestring #9c1185a5c5e9fc54612808977ee8f548b2258d31)");
  EXPECT_EQ(
    eval_ok("(program 1.0.0 [ (builtin ripemd_160) (con bytestring #616263) ])"),
    "(con bytestring #8eb208f7e05d987a9b044a8e98c6b087f15a0bfc)");
}

TEST(cardano_uplc_builtin_body, verifyEd25519SignatureValidIsTrue)
{
  EXPECT_EQ(
    eval_ok(
      "(program 1.0.0 [ [ [ (builtin verifyEd25519Signature) "
      "(con bytestring #3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c) ] "
      "(con bytestring #72) ] "
      "(con bytestring #92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00) ])"),
    "(con bool True)");
}

TEST(cardano_uplc_builtin_body, verifyEd25519SignatureTamperedIsFalse)
{
  EXPECT_EQ(
    eval_ok(
      "(program 1.0.0 [ [ [ (builtin verifyEd25519Signature) "
      "(con bytestring #3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c) ] "
      "(con bytestring #72) ] "
      "(con bytestring #92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c01) ])"),
    "(con bool False)");
}

TEST(cardano_uplc_builtin_body, verifyEd25519SignatureWrongKeyLengthIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render(
    "(program 1.0.0 [ [ [ (builtin verifyEd25519Signature) "
    "(con bytestring #3d4017) ] "
    "(con bytestring #72) ] "
    "(con bytestring #92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00) ])",
    &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, verifyEd25519SignatureWrongSignatureLengthIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render(
    "(program 1.0.0 [ [ [ (builtin verifyEd25519Signature) "
    "(con bytestring #3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c) ] "
    "(con bytestring #72) ] "
    "(con bytestring #92a00900) ])",
    &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, verifyEcdsaSecp256k1SignatureValidIsTrue)
{
  EXPECT_EQ(
    eval_ok(
      "(program 1.0.0 [ [ [ (builtin verifyEcdsaSecp256k1Signature) "
      "(con bytestring #0284bf7562262bbd6940085748f3be6afa52ae317155181ece31b66351ccffa4b0) ] "
      "(con bytestring #a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf) ] "
      "(con bytestring #b8ce6c0e1cf885cecd1a17c28c67eaa8ec94d7c7fe89be6cf08db7db759d14a9"
      "7d07482d174a2aeb9ce750dfd61a9acb3d58d75cf78c0c22e709c2b353bed95f) ])"),
    "(con bool True)");
}

TEST(cardano_uplc_builtin_body, verifyEcdsaSecp256k1SignatureTamperedIsFalse)
{
  EXPECT_EQ(
    eval_ok(
      "(program 1.0.0 [ [ [ (builtin verifyEcdsaSecp256k1Signature) "
      "(con bytestring #0284bf7562262bbd6940085748f3be6afa52ae317155181ece31b66351ccffa4b0) ] "
      "(con bytestring #a1a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf) ] "
      "(con bytestring #b8ce6c0e1cf885cecd1a17c28c67eaa8ec94d7c7fe89be6cf08db7db759d14a9"
      "7d07482d174a2aeb9ce750dfd61a9acb3d58d75cf78c0c22e709c2b353bed95f) ])"),
    "(con bool False)");
}

TEST(cardano_uplc_builtin_body, verifyEcdsaSecp256k1SignatureWrongKeyLengthIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render(
    "(program 1.0.0 [ [ [ (builtin verifyEcdsaSecp256k1Signature) "
    "(con bytestring #0284bf) ] "
    "(con bytestring #a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf) ] "
    "(con bytestring #b8ce6c0e1cf885cecd1a17c28c67eaa8ec94d7c7fe89be6cf08db7db759d14a9"
    "7d07482d174a2aeb9ce750dfd61a9acb3d58d75cf78c0c22e709c2b353bed95f) ])",
    &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}

TEST(cardano_uplc_builtin_body, verifySchnorrSecp256k1SignatureValidIsTrue)
{
  EXPECT_EQ(
    eval_ok(
      "(program 1.0.0 [ [ [ (builtin verifySchnorrSecp256k1Signature) "
      "(con bytestring #f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9) ] "
      "(con bytestring #0000000000000000000000000000000000000000000000000000000000000000) ] "
      "(con bytestring #e907831f80848d1069a5371b402410364bdf1c5f8307b0084c55f1ce2dca8215"
      "25f66a4a85ea8b71e482a74f382d2ce5ebeee8fdb2172f477df4900d310536c0) ])"),
    "(con bool True)");
}

TEST(cardano_uplc_builtin_body, verifySchnorrSecp256k1SignatureTamperedIsFalse)
{
  EXPECT_EQ(
    eval_ok(
      "(program 1.0.0 [ [ [ (builtin verifySchnorrSecp256k1Signature) "
      "(con bytestring #f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9) ] "
      "(con bytestring #0000000000000000000000000000000000000000000000000000000000000000) ] "
      "(con bytestring #e807831f80848d1069a5371b402410364bdf1c5f8307b0084c55f1ce2dca8215"
      "25f66a4a85ea8b71e482a74f382d2ce5ebeee8fdb2172f477df4900d310536c0) ])"),
    "(con bool False)");
}

TEST(cardano_uplc_builtin_body, verifySchnorrSecp256k1SignatureWrongSignatureLengthIsAScriptError)
{
  cardano_uplc_eval_status_t status = CARDANO_UPLC_EVAL_SUCCESS;
  (void)evaluate_to_render(
    "(program 1.0.0 [ [ [ (builtin verifySchnorrSecp256k1Signature) "
    "(con bytestring #f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9) ] "
    "(con bytestring #0000000000000000000000000000000000000000000000000000000000000000) ] "
    "(con bytestring #e907831f) ])",
    &status);
  EXPECT_EQ(status, CARDANO_UPLC_EVAL_ERROR_TERM);
}
