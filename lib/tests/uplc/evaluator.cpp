/**
 * \file evaluator.cpp
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
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/uplc/uplc_apply_params.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/data/uplc_data.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>
#include <string>

/* CONSTANTS *****************************************************************/

/**
 * \brief A real parameterized Plutus V3 validator, CBOR-wrapped flat, as hex.
 *
 * Taken verbatim from the cometa.py apply-params test corpus
 * (tests/aiken/test_aiken.py COMPILED_CODE), so it is aiken-produced canonical
 * output. Applying zero parameters to it must round-trip to these same bytes.
 */
static const char* kCompiledCode =
  "590221010000323232323232323232323223222232533300b32323232533300f3370e9000180700089"
  "919191919191919191919299980e98100010991919299980e99b87480000044c94ccc078cdc3a4000"
  "603a002264a66603e66e1c011200213371e00a0322940c07000458c8cc004004030894ccc0880045"
  "30103d87a80001323253330213375e6603a603e004900000d099ba548000cc0940092f5c02660080"
  "08002604c00460480022a66603a66e1c009200113371e00602e2940c06c050dd6980e8011bae301b"
  "00116301e001323232533301b3370e90010008a5eb7bdb1804c8dd59810800980c801180c8009919"
  "80080080111299980f0008a6103d87a8000132323232533301f3371e01e004266e952000330233"
  "74c00297ae0133006006003375660400066eb8c078008c088008c080004c8cc004004008894ccc07"
  "400452f5bded8c0264646464a66603c66e3d221000021003133022337606ea4008dd3000998030030"
  "019bab301f003375c603a0046042004603e0026eacc070004c070004c06c004c068004c064008dd6"
  "180b80098078029bae3015001300d001163013001301300230110013009002149858c94ccc02ccdc"
  "3a40000022a66601c60120062930b0a99980599b874800800454ccc038c02400c52616163009002"
  "375c0026600200290001111199980399b8700100300c233330050053370000890011807000801001"
  "118029baa001230033754002ae6955ceaab9e5573eae815d0aba201";

/* STATIC HELPERS ************************************************************/

static cardano_uplc_arena_t*
new_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  cardano_error_t       error = cardano_uplc_arena_new(4096U, &arena);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return arena;
}

static cardano_plutus_data_t*
new_data(int64_t value)
{
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_int(value, &data);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return data;
}

static bool
arena_data_equals(const cardano_uplc_data_t* node, const cardano_plutus_data_t* expected)
{
  cardano_plutus_data_t* converted = nullptr;
  bool                   equal     = false;

  EXPECT_EQ(cardano_uplc_data_to_plutus_data(node, &converted), CARDANO_SUCCESS);
  equal = cardano_plutus_data_equals(converted, expected);
  cardano_plutus_data_unref(&converted);

  return equal;
}

static cardano_plutus_list_t*
new_list()
{
  cardano_plutus_list_t* list  = nullptr;
  cardano_error_t        error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return list;
}

static void
list_append_int(cardano_plutus_list_t* list, int64_t value)
{
  cardano_plutus_data_t* data = new_data(value);
  EXPECT_EQ(cardano_plutus_list_add(list, data), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&data);
}

/**
 * \brief Builds the CBOR-wrapped flat bytes of (program 1.0.0 (lam (var 1))).
 *
 * The identity validator: a single lambda returning its argument. Encoded with
 * the public encoder so it is in the exact form the apply API expects.
 *
 * \return A buffer the caller releases with \ref cardano_buffer_unref.
 */
static cardano_buffer_t*
build_identity_script()
{
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_term_t*  var    = nullptr;
  cardano_uplc_term_t*  lambda = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &lambda), CARDANO_SUCCESS);

  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));
  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;
  program->term          = lambda;

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(program, &cbor), CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);

  return cbor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_apply_params_to_script, zeroParamsRoundTripsARealScriptByteForByte)
{
  // Arrange
  cardano_buffer_t* script = cardano_buffer_from_hex(kCompiledCode, strlen(kCompiledCode));
  ASSERT_NE(script, nullptr);

  cardano_plutus_list_t* params = new_list();
  cardano_buffer_t*      out    = nullptr;

  // Act
  cardano_error_t error =
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(out, nullptr);
  ASSERT_EQ(cardano_buffer_get_size(out), cardano_buffer_get_size(script));
  EXPECT_EQ(memcmp(cardano_buffer_get_data(out), cardano_buffer_get_data(script), cardano_buffer_get_size(out)), 0);

  // Cleanup
  cardano_buffer_unref(&script);
  cardano_buffer_unref(&out);
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, nullParamsRoundTripsTheScript)
{
  // Arrange
  cardano_buffer_t* script = build_identity_script();
  cardano_buffer_t* out    = nullptr;

  // Act
  cardano_error_t error =
    cardano_uplc_apply_params_to_script(nullptr, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(out, nullptr);
  ASSERT_EQ(cardano_buffer_get_size(out), cardano_buffer_get_size(script));
  EXPECT_EQ(memcmp(cardano_buffer_get_data(out), cardano_buffer_get_data(script), cardano_buffer_get_size(out)), 0);

  // Cleanup
  cardano_buffer_unref(&script);
  cardano_buffer_unref(&out);
}

TEST(cardano_uplc_apply_params_to_script, oneParamWrapsTheTermInApplyOfDataConstant)
{
  // Arrange
  cardano_buffer_t*      script = build_identity_script();
  cardano_plutus_list_t* params = new_list();
  list_append_int(params, 123);

  cardano_buffer_t* out = nullptr;

  // Act
  cardano_error_t error =
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(out, nullptr);

  cardano_uplc_arena_t*         arena   = new_arena();
  const cardano_uplc_program_t* decoded = nullptr;
  EXPECT_EQ(
    cardano_uplc_program_from_script_bytes(arena, cardano_buffer_get_data(out), cardano_buffer_get_size(out), &decoded),
    CARDANO_SUCCESS);
  ASSERT_NE(decoded, nullptr);
  ASSERT_NE(decoded->term, nullptr);
  EXPECT_EQ(decoded->term->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(decoded->term->as.apply.function->kind, CARDANO_UPLC_TERM_LAMBDA);

  const cardano_uplc_term_t* arg = decoded->term->as.apply.argument;
  ASSERT_EQ(arg->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(arg->as.constant->kind, CARDANO_UPLC_TYPE_DATA);

  cardano_plutus_data_t* expected = new_data(123);
  EXPECT_TRUE(arena_data_equals(arg->as.constant->as.data, expected));

  // Cleanup
  cardano_plutus_data_unref(&expected);
  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&script);
  cardano_buffer_unref(&out);
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, appliedScriptEvaluatesToTheBoundParam)
{
  // Arrange
  cardano_buffer_t*      script = build_identity_script();
  cardano_plutus_list_t* params = new_list();
  list_append_int(params, 777);

  cardano_buffer_t* out = nullptr;
  EXPECT_EQ(
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out),
    CARDANO_SUCCESS);
  ASSERT_NE(out, nullptr);

  cardano_uplc_arena_t*         arena   = new_arena();
  const cardano_uplc_program_t* decoded = nullptr;
  EXPECT_EQ(
    cardano_uplc_program_from_script_bytes(arena, cardano_buffer_get_data(out), cardano_buffer_get_size(out), &decoded),
    CARDANO_SUCCESS);

  cardano_uplc_budget_t      budget = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;
  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, decoded, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  ASSERT_NE(result.result->as.constant, nullptr);
  EXPECT_EQ(result.result->as.constant->kind, CARDANO_UPLC_TYPE_DATA);

  cardano_plutus_data_t* expected = new_data(777);
  EXPECT_TRUE(arena_data_equals(result.result->as.constant->as.data, expected));

  // Cleanup
  cardano_plutus_data_unref(&expected);
  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&script);
  cardano_buffer_unref(&out);
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, multipleParamsApplyLeftToRight)
{
  // Arrange a validator that takes two args and returns the first: (lam (lam (var 2))).
  cardano_uplc_arena_t* build_arena = new_arena();
  cardano_uplc_term_t*  var         = nullptr;
  cardano_uplc_term_t*  inner       = nullptr;
  cardano_uplc_term_t*  outer       = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(build_arena, 2U, &var), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_lambda(build_arena, var, &inner), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_lambda(build_arena, inner, &outer), CARDANO_SUCCESS);

  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(build_arena, sizeof(cardano_uplc_program_t), 0U));
  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;
  program->term          = outer;

  cardano_buffer_t* script = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(program, &script), CARDANO_SUCCESS);
  cardano_uplc_arena_free(&build_arena);

  cardano_plutus_list_t* params = new_list();
  list_append_int(params, 11);
  list_append_int(params, 22);

  cardano_buffer_t* out = nullptr;
  EXPECT_EQ(
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out),
    CARDANO_SUCCESS);
  ASSERT_NE(out, nullptr);

  cardano_uplc_arena_t*         arena   = new_arena();
  const cardano_uplc_program_t* decoded = nullptr;
  EXPECT_EQ(
    cardano_uplc_program_from_script_bytes(arena, cardano_buffer_get_data(out), cardano_buffer_get_size(out), &decoded),
    CARDANO_SUCCESS);

  // The term is [[outer 11] 22]: outermost apply argument is the last param.
  ASSERT_EQ(decoded->term->kind, CARDANO_UPLC_TERM_APPLY);
  const cardano_uplc_term_t* outer_arg = decoded->term->as.apply.argument;
  ASSERT_EQ(outer_arg->kind, CARDANO_UPLC_TERM_CONSTANT);
  cardano_plutus_data_t* twenty_two = new_data(22);
  EXPECT_TRUE(arena_data_equals(outer_arg->as.constant->as.data, twenty_two));

  const cardano_uplc_term_t* inner_apply = decoded->term->as.apply.function;
  ASSERT_EQ(inner_apply->kind, CARDANO_UPLC_TERM_APPLY);
  const cardano_uplc_term_t* inner_arg = inner_apply->as.apply.argument;
  ASSERT_EQ(inner_arg->kind, CARDANO_UPLC_TERM_CONSTANT);
  cardano_plutus_data_t* eleven = new_data(11);
  EXPECT_TRUE(arena_data_equals(inner_arg->as.constant->as.data, eleven));

  // Evaluating binds both: the result is the first param, 11.
  cardano_uplc_budget_t      budget = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;
  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;
  EXPECT_EQ(cardano_uplc_evaluate(arena, decoded, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result), CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  ASSERT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_TRUE(arena_data_equals(result.result->as.constant->as.data, eleven));

  // Cleanup
  cardano_plutus_data_unref(&twenty_two);
  cardano_plutus_data_unref(&eleven);
  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&script);
  cardano_buffer_unref(&out);
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, differentParamsProduceDifferentOutput)
{
  // Arrange
  cardano_buffer_t*      script   = build_identity_script();
  cardano_plutus_list_t* params_a = new_list();
  cardano_plutus_list_t* params_b = new_list();
  list_append_int(params_a, 1);
  list_append_int(params_b, 2);

  cardano_buffer_t* out_a = nullptr;
  cardano_buffer_t* out_b = nullptr;

  // Act
  EXPECT_EQ(
    cardano_uplc_apply_params_to_script(params_a, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out_a),
    CARDANO_SUCCESS);
  EXPECT_EQ(
    cardano_uplc_apply_params_to_script(params_b, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out_b),
    CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(out_a, nullptr);
  ASSERT_NE(out_b, nullptr);
  bool same = (cardano_buffer_get_size(out_a) == cardano_buffer_get_size(out_b)) &&
    (memcmp(cardano_buffer_get_data(out_a), cardano_buffer_get_data(out_b), cardano_buffer_get_size(out_a)) == 0);
  EXPECT_FALSE(same);

  // Cleanup
  cardano_buffer_unref(&script);
  cardano_buffer_unref(&out_a);
  cardano_buffer_unref(&out_b);
  cardano_plutus_list_unref(&params_a);
  cardano_plutus_list_unref(&params_b);
}

TEST(cardano_uplc_apply_params_to_script, realScriptWithParamsDecodesAndEvaluatesUnreachable)
{
  // Arrange: apply a param to the real cometa script, then decode the result.
  cardano_buffer_t* script = cardano_buffer_from_hex(kCompiledCode, strlen(kCompiledCode));
  ASSERT_NE(script, nullptr);

  cardano_plutus_list_t* params = new_list();
  list_append_int(params, 42);

  cardano_buffer_t* out = nullptr;

  // Act
  cardano_error_t error =
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(out, nullptr);
  EXPECT_NE(cardano_buffer_get_size(out), cardano_buffer_get_size(script));

  cardano_uplc_arena_t*         arena   = new_arena();
  const cardano_uplc_program_t* decoded = nullptr;
  EXPECT_EQ(
    cardano_uplc_program_from_script_bytes(arena, cardano_buffer_get_data(out), cardano_buffer_get_size(out), &decoded),
    CARDANO_SUCCESS);
  ASSERT_NE(decoded, nullptr);
  EXPECT_EQ(decoded->term->kind, CARDANO_UPLC_TERM_APPLY);

  const cardano_uplc_term_t* arg = decoded->term->as.apply.argument;
  ASSERT_EQ(arg->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(arg->as.constant->kind, CARDANO_UPLC_TYPE_DATA);
  cardano_plutus_data_t* expected = new_data(42);
  EXPECT_TRUE(arena_data_equals(arg->as.constant->as.data, expected));

  // Cleanup
  cardano_plutus_data_unref(&expected);
  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&script);
  cardano_buffer_unref(&out);
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, rejectsMalformedScriptWithDecoding)
{
  // Arrange: raw flat bytes with no CBOR wrapper are not accepted.
  const byte_t           garbage[] = { 0x01U, 0x02U, 0x03U, 0x04U };
  cardano_plutus_list_t* params    = new_list();
  cardano_buffer_t*      out       = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_apply_params_to_script(params, garbage, sizeof(garbage), &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, nullptr);

  // Cleanup
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, rejectsEmptyScriptWithDecoding)
{
  // Arrange
  cardano_plutus_list_t* params = new_list();
  cardano_buffer_t*      out    = nullptr;
  const byte_t           dummy  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_apply_params_to_script(params, &dummy, 0U, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, nullptr);

  // Cleanup
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, failsOnNullArgs)
{
  cardano_buffer_t*      script = build_identity_script();
  cardano_plutus_list_t* params = new_list();
  cardano_buffer_t*      out    = nullptr;

  EXPECT_EQ(
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_apply_params_to_script(params, nullptr, 4U, &out), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(out, nullptr);

  cardano_buffer_unref(&script);
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, failsWhenTheArenaCannotBeAllocated)
{
  cardano_buffer_t*      script = build_identity_script();
  cardano_plutus_list_t* params = new_list();
  cardano_buffer_t*      out    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error =
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, nullptr);

  cardano_buffer_unref(&script);
  cardano_plutus_list_unref(&params);
}

TEST(cardano_uplc_apply_params_to_script, failsWhenAnInteriorAllocationFails)
{
  cardano_buffer_t*      script = build_identity_script();
  cardano_plutus_list_t* params = new_list();
  list_append_int(params, 5);

  cardano_buffer_t* out = nullptr;

  reset_allocators_run_count();
  set_malloc_limit(3);
  cardano_set_allocators(fail_malloc_at_limit, realloc, free);

  cardano_error_t error =
    cardano_uplc_apply_params_to_script(params, cardano_buffer_get_data(script), cardano_buffer_get_size(script), &out);

  cardano_set_allocators(malloc, realloc, free);
  reset_limited_malloc();

  EXPECT_NE(error, CARDANO_SUCCESS);
  EXPECT_EQ(out, nullptr);

  cardano_buffer_unref(&script);
  cardano_plutus_list_unref(&params);
}
