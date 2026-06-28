/**
 * \file machine.cpp
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

#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include "../../src/uplc/builtins/uplc_builtin.h"
#include "../../src/uplc/machine/uplc_machine.h"
#include "../../src/uplc/ast/uplc_term.h"

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/machine/uplc_env.h"
#include "../../src/uplc/machine/uplc_frame.h"
#include "../../src/uplc/machine/uplc_state_kind.h"
#include "../../src/uplc/machine/uplc_value.h"
#include "../../src/uplc/ast/uplc_int.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* STATIC HELPERS ************************************************************/

static int64_t
int_const_i64(const cardano_uplc_constant_t* constant)
{
  if (cardano_uplc_constant_int_is_small(constant))
  {
    return cardano_uplc_constant_int_small(constant);
  }

  return cardano_bigint_to_int(constant->as.integer.big);
}

static cardano_uplc_arena_t*
new_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  cardano_error_t       error = cardano_uplc_arena_new(4096U, &arena);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return arena;
}

static const cardano_uplc_value_t*
new_unit_value(cardano_uplc_arena_t* arena)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

static const cardano_uplc_term_t*
new_error_term(cardano_uplc_arena_t* arena)
{
  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);
  return term;
}

/* UNIT TESTS - STATE AND BUDGET ********************************************/

TEST(cardano_uplc_machine_state, kindsHaveTheExpectedValues)
{
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_STATE_COMPUTE), 0);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_STATE_RETURN), 1);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_STATE_DONE), 2);
}

TEST(cardano_uplc_machine_budget, holdsSignedValuesThatMayGoNegative)
{
  // Arrange
  cardano_uplc_budget_t budget;

  // Act
  budget.cpu = 100;
  budget.mem = 50;
  budget.cpu -= 16000;
  budget.mem -= 100;

  // Assert
  EXPECT_LT(budget.cpu, 0);
  EXPECT_LT(budget.mem, 0);
  EXPECT_EQ(budget.cpu, static_cast<int64_t>(100) - 16000);
  EXPECT_EQ(budget.mem, static_cast<int64_t>(50) - 100);
}

/* UNIT TESTS - ENVIRONMENT *************************************************/

TEST(cardano_uplc_env_extend, consesAValueAtTheHead)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = new_unit_value(arena);
  const cardano_uplc_env_t*   env   = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_env_extend(arena, nullptr, value, &env);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(env->value, value);
  EXPECT_EQ(env->next, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_env_extend, sharesTheTailWithoutCopying)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* first = new_unit_value(arena);
  const cardano_uplc_value_t* second = new_unit_value(arena);
  const cardano_uplc_env_t*   tail  = nullptr;
  const cardano_uplc_env_t*   head  = nullptr;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, first, &tail), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_env_extend(arena, tail, second, &head);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(head->value, second);
  EXPECT_EQ(head->next, tail);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_env_extend, failsOnNullArgs)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = new_unit_value(arena);
  const cardano_uplc_env_t*   env   = nullptr;

  EXPECT_EQ(cardano_uplc_env_extend(nullptr, nullptr, value, &env), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, nullptr, &env), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, value, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_env_extend, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*       prereq = new_arena();
  cardano_uplc_arena_t*       arena  = new_arena();
  const cardano_uplc_value_t* value  = new_unit_value(prereq);
  const cardano_uplc_env_t*   env    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_env_extend(arena, nullptr, value, &env);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(env, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_env_lookup, resolvesByOneBasedDeBruijnIndex)
{
  // Arrange
  cardano_uplc_arena_t*       arena  = new_arena();
  const cardano_uplc_value_t* first  = new_unit_value(arena);
  const cardano_uplc_value_t* second = new_unit_value(arena);
  const cardano_uplc_value_t* third  = new_unit_value(arena);
  const cardano_uplc_env_t*   e1     = nullptr;
  const cardano_uplc_env_t*   e2     = nullptr;
  const cardano_uplc_env_t*   e3     = nullptr;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, first, &e1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_env_extend(arena, e1, second, &e2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_env_extend(arena, e2, third, &e3), CARDANO_SUCCESS);

  const cardano_uplc_value_t* out = nullptr;

  // Act + Assert: index 1 is the head (innermost binder), deeper indices walk the list.
  EXPECT_EQ(cardano_uplc_env_lookup(e3, 1U, &out), CARDANO_SUCCESS);
  EXPECT_EQ(out, third);

  out = nullptr;
  EXPECT_EQ(cardano_uplc_env_lookup(e3, 2U, &out), CARDANO_SUCCESS);
  EXPECT_EQ(out, second);

  out = nullptr;
  EXPECT_EQ(cardano_uplc_env_lookup(e3, 3U, &out), CARDANO_SUCCESS);
  EXPECT_EQ(out, first);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_env_lookup, reportsNotFoundForIndexZero)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* value = new_unit_value(arena);
  const cardano_uplc_env_t*   env   = nullptr;
  const cardano_uplc_value_t* out   = nullptr;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, value, &env), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_env_lookup(env, 0U, &out), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(out, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_env_lookup, reportsNotFoundWhenIndexWalksPastTheEnd)
{
  cardano_uplc_arena_t*       arena  = new_arena();
  const cardano_uplc_value_t* first  = new_unit_value(arena);
  const cardano_uplc_value_t* second = new_unit_value(arena);
  const cardano_uplc_env_t*   e1     = nullptr;
  const cardano_uplc_env_t*   e2     = nullptr;
  const cardano_uplc_value_t* out    = nullptr;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, first, &e1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_env_extend(arena, e1, second, &e2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_env_lookup(e2, 3U, &out), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(out, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_env_lookup, reportsNotFoundOnTheEmptyEnvironment)
{
  const cardano_uplc_value_t* out = nullptr;

  EXPECT_EQ(cardano_uplc_env_lookup(nullptr, 1U, &out), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(out, nullptr);
}

TEST(cardano_uplc_env_lookup, failsOnNullOut)
{
  EXPECT_EQ(cardano_uplc_env_lookup(nullptr, 1U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

/* UNIT TESTS - VALUES ******************************************************/

TEST(cardano_uplc_value_new_constant, setsKindAndConstant)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_value_new_constant(arena, constant, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value->kind, CARDANO_UPLC_VALUE_CONSTANT);
  EXPECT_EQ(value->as.constant, constant);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_constant, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_value_new_constant(nullptr, constant, &value), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, nullptr, &value), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_constant, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*    prereq   = new_arena();
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_value_t*    value    = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(prereq, &constant), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_value_new_constant(arena, constant, &value);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_value_new_delay, setsKindBodyAndEnv)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_term_t*  body  = new_error_term(arena);
  const cardano_uplc_value_t* held  = new_unit_value(arena);
  const cardano_uplc_env_t*   env   = nullptr;
  cardano_uplc_value_t*       value = nullptr;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, held, &env), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_value_new_delay(arena, body, env, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value->kind, CARDANO_UPLC_VALUE_DELAY);
  EXPECT_EQ(value->as.delay.body, body);
  EXPECT_EQ(value->as.delay.env, env);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_delay, acceptsTheEmptyEnvironment)
{
  cardano_uplc_arena_t*      arena = new_arena();
  const cardano_uplc_term_t* body  = new_error_term(arena);
  cardano_uplc_value_t*      value = nullptr;

  EXPECT_EQ(cardano_uplc_value_new_delay(arena, body, nullptr, &value), CARDANO_SUCCESS);
  EXPECT_EQ(value->as.delay.env, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_delay, failsOnNullArgs)
{
  cardano_uplc_arena_t*      arena = new_arena();
  const cardano_uplc_term_t* body  = new_error_term(arena);
  cardano_uplc_value_t*      value = nullptr;

  EXPECT_EQ(cardano_uplc_value_new_delay(nullptr, body, nullptr, &value), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_value_new_delay(arena, nullptr, nullptr, &value), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_value_new_delay(arena, body, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_delay, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*      prereq = new_arena();
  cardano_uplc_arena_t*      arena  = new_arena();
  const cardano_uplc_term_t* body   = new_error_term(prereq);
  cardano_uplc_value_t*      value  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_value_new_delay(arena, body, nullptr, &value);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_value_new_lambda, setsKindBodyAndEnv)
{
  // Arrange
  cardano_uplc_arena_t*      arena = new_arena();
  const cardano_uplc_term_t* body  = new_error_term(arena);
  cardano_uplc_value_t*      value = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_value_new_lambda(arena, body, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value->kind, CARDANO_UPLC_VALUE_LAMBDA);
  EXPECT_EQ(value->as.lambda.body, body);
  EXPECT_EQ(value->as.lambda.env, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_lambda, failsOnNullArgs)
{
  cardano_uplc_arena_t*      arena = new_arena();
  const cardano_uplc_term_t* body  = new_error_term(arena);
  cardano_uplc_value_t*      value = nullptr;

  EXPECT_EQ(cardano_uplc_value_new_lambda(nullptr, body, nullptr, &value), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_value_new_lambda(arena, nullptr, nullptr, &value), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_value_new_lambda(arena, body, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_lambda, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*      prereq = new_arena();
  cardano_uplc_arena_t*      arena  = new_arena();
  const cardano_uplc_term_t* body   = new_error_term(prereq);
  cardano_uplc_value_t*      value  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_value_new_lambda(arena, body, nullptr, &value);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_value_new_builtin, setsKindFuncForcesAndArgs)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* arg0  = new_unit_value(arena);
  const cardano_uplc_value_t* args[1];
  cardano_uplc_value_t*       value = nullptr;

  args[0] = arg0;

  // Act
  cardano_error_t error = cardano_uplc_value_new_builtin(
    arena,
    CARDANO_UPLC_BUILTIN_ADD_INTEGER,
    1U,
    args,
    1U,
    &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value->kind, CARDANO_UPLC_VALUE_BUILTIN);
  EXPECT_EQ(value->as.builtin.func, CARDANO_UPLC_BUILTIN_ADD_INTEGER);
  EXPECT_EQ(value->as.builtin.forces, 1U);
  EXPECT_EQ(value->as.builtin.arg_count, 1U);
  EXPECT_EQ(value->as.builtin.args[0], arg0);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_builtin, acceptsNoArgumentsYet)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  EXPECT_EQ(
    cardano_uplc_value_new_builtin(arena, CARDANO_UPLC_BUILTIN_IF_THEN_ELSE, 0U, nullptr, 0U, &value),
    CARDANO_SUCCESS);
  EXPECT_EQ(value->as.builtin.arg_count, 0U);
  EXPECT_EQ(value->as.builtin.args, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_builtin, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  EXPECT_EQ(
    cardano_uplc_value_new_builtin(nullptr, CARDANO_UPLC_BUILTIN_ADD_INTEGER, 0U, nullptr, 0U, &value),
    CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(
    cardano_uplc_value_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, 0U, nullptr, 0U, nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_builtin, failsWhenArgCountWithoutArray)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  EXPECT_EQ(
    cardano_uplc_value_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, 0U, nullptr, 2U, &value),
    CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_builtin, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error =
    cardano_uplc_value_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, 0U, nullptr, 0U, &value);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_constr, setsKindTagAndFields)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* f0    = new_unit_value(arena);
  const cardano_uplc_value_t* f1    = new_unit_value(arena);
  const cardano_uplc_value_t* fields[2];
  cardano_uplc_value_t*       value = nullptr;

  fields[0] = f0;
  fields[1] = f1;

  // Act
  cardano_error_t error = cardano_uplc_value_new_constr(arena, 7U, fields, 2U, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value->kind, CARDANO_UPLC_VALUE_CONSTR);
  EXPECT_EQ(value->as.constr.tag, 7U);
  EXPECT_EQ(value->as.constr.field_count, 2U);
  EXPECT_EQ(value->as.constr.fields[0], f0);
  EXPECT_EQ(value->as.constr.fields[1], f1);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_constr, acceptsNoFields)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  EXPECT_EQ(cardano_uplc_value_new_constr(arena, 0U, nullptr, 0U, &value), CARDANO_SUCCESS);
  EXPECT_EQ(value->as.constr.field_count, 0U);
  EXPECT_EQ(value->as.constr.fields, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_constr, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  EXPECT_EQ(cardano_uplc_value_new_constr(nullptr, 0U, nullptr, 0U, &value), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_value_new_constr(arena, 0U, nullptr, 0U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_constr, failsWhenFieldCountWithoutArray)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  EXPECT_EQ(cardano_uplc_value_new_constr(arena, 0U, nullptr, 1U, &value), CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_new_constr, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_value_t* value = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_value_new_constr(arena, 0U, nullptr, 0U, &value);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, nullptr);

  cardano_uplc_arena_free(&arena);
}

/* UNIT TESTS - FRAMES ******************************************************/

TEST(cardano_uplc_frame_new_no_frame, setsKindAndHasNoContext)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &frame), CARDANO_SUCCESS);
  EXPECT_EQ(frame->kind, CARDANO_UPLC_FRAME_NO_FRAME);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_no_frame, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(nullptr, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_no_frame, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* frame = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_frame_new_no_frame(arena, &frame);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(frame, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_await_arg, setsKindValueAndChainsContext)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* fun   = new_unit_value(arena);
  cardano_uplc_frame_t*       inner = nullptr;
  cardano_uplc_frame_t*       frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_frame_new_await_arg(arena, fun, inner, &frame);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(frame->kind, CARDANO_UPLC_FRAME_AWAIT_ARG);
  EXPECT_EQ(frame->as.await_arg.value, fun);
  EXPECT_EQ(frame->as.await_arg.ctx, inner);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_await_arg, failsOnNullArgs)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* fun   = new_unit_value(arena);
  cardano_uplc_frame_t*       inner = nullptr;
  cardano_uplc_frame_t*       frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_frame_new_await_arg(nullptr, fun, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_arg(arena, nullptr, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_arg(arena, fun, nullptr, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_arg(arena, fun, inner, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_await_arg, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*       prereq = new_arena();
  cardano_uplc_arena_t*       arena  = new_arena();
  const cardano_uplc_value_t* fun    = new_unit_value(prereq);
  cardano_uplc_frame_t*       inner  = nullptr;
  cardano_uplc_frame_t*       frame  = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(prereq, &inner), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_frame_new_await_arg(arena, fun, inner, &frame);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(frame, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_frame_new_await_fun_term, setsKindEnvTermAndChainsContext)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_term_t*  term  = new_error_term(arena);
  const cardano_uplc_value_t* held  = new_unit_value(arena);
  const cardano_uplc_env_t*   env   = nullptr;
  cardano_uplc_frame_t*       inner = nullptr;
  cardano_uplc_frame_t*       frame = nullptr;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, held, &env), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_frame_new_await_fun_term(arena, env, term, inner, &frame);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(frame->kind, CARDANO_UPLC_FRAME_AWAIT_FUN_TERM);
  EXPECT_EQ(frame->as.await_fun_term.env, env);
  EXPECT_EQ(frame->as.await_fun_term.term, term);
  EXPECT_EQ(frame->as.await_fun_term.ctx, inner);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_await_fun_term, failsOnNullArgs)
{
  cardano_uplc_arena_t*      arena = new_arena();
  const cardano_uplc_term_t* term  = new_error_term(arena);
  cardano_uplc_frame_t*      inner = nullptr;
  cardano_uplc_frame_t*      frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_frame_new_await_fun_term(nullptr, nullptr, term, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_fun_term(arena, nullptr, nullptr, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_fun_term(arena, nullptr, term, nullptr, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_fun_term(arena, nullptr, term, inner, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_await_fun_term, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*      prereq = new_arena();
  cardano_uplc_arena_t*      arena  = new_arena();
  const cardano_uplc_term_t* term   = new_error_term(prereq);
  cardano_uplc_frame_t*      inner  = nullptr;
  cardano_uplc_frame_t*      frame  = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(prereq, &inner), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_frame_new_await_fun_term(arena, nullptr, term, inner, &frame);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(frame, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_frame_new_await_fun_value, setsKindValueAndChainsContext)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* arg   = new_unit_value(arena);
  cardano_uplc_frame_t*       inner = nullptr;
  cardano_uplc_frame_t*       frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_frame_new_await_fun_value(arena, arg, inner, &frame);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(frame->kind, CARDANO_UPLC_FRAME_AWAIT_FUN_VALUE);
  EXPECT_EQ(frame->as.await_fun_value.value, arg);
  EXPECT_EQ(frame->as.await_fun_value.ctx, inner);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_await_fun_value, failsOnNullArgs)
{
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_value_t* arg   = new_unit_value(arena);
  cardano_uplc_frame_t*       inner = nullptr;
  cardano_uplc_frame_t*       frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_frame_new_await_fun_value(nullptr, arg, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_fun_value(arena, nullptr, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_fun_value(arena, arg, nullptr, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_await_fun_value(arena, arg, inner, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_await_fun_value, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*       prereq = new_arena();
  cardano_uplc_arena_t*       arena  = new_arena();
  const cardano_uplc_value_t* arg    = new_unit_value(prereq);
  cardano_uplc_frame_t*       inner  = nullptr;
  cardano_uplc_frame_t*       frame  = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(prereq, &inner), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_frame_new_await_fun_value(arena, arg, inner, &frame);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(frame, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_frame_new_force, setsKindAndChainsContext)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_frame_new_force(arena, inner, &frame);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(frame->kind, CARDANO_UPLC_FRAME_FORCE);
  EXPECT_EQ(frame->as.force.ctx, inner);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_force, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_frame_new_force(nullptr, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_force(arena, nullptr, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_force(arena, inner, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_force, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq = new_arena();
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_frame_t* inner  = nullptr;
  cardano_uplc_frame_t* frame  = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(prereq, &inner), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_frame_new_force(arena, inner, &frame);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(frame, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_frame_new_constr, setsAllFieldsAndChainsContext)
{
  // Arrange
  cardano_uplc_arena_t*       arena    = new_arena();
  const cardano_uplc_term_t*  pending  = new_error_term(arena);
  const cardano_uplc_value_t* done     = new_unit_value(arena);
  const cardano_uplc_value_t* held     = new_unit_value(arena);
  const cardano_uplc_env_t*   env      = nullptr;
  cardano_uplc_frame_t*       inner    = nullptr;
  cardano_uplc_frame_t*       frame    = nullptr;
  const cardano_uplc_term_t*  fields[1];
  const cardano_uplc_value_t* resolved[1];

  fields[0]   = pending;
  resolved[0] = done;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, held, &env), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_frame_new_constr(arena, env, 3U, fields, 1U, resolved, 1U, inner, &frame);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(frame->kind, CARDANO_UPLC_FRAME_CONSTR);
  EXPECT_EQ(frame->as.constr.env, env);
  EXPECT_EQ(frame->as.constr.tag, 3U);
  EXPECT_EQ(frame->as.constr.field_count, 1U);
  EXPECT_EQ(frame->as.constr.fields[0], pending);
  EXPECT_EQ(frame->as.constr.resolved_count, 1U);
  EXPECT_EQ(frame->as.constr.resolved[0], done);
  EXPECT_EQ(frame->as.constr.ctx, inner);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_constr, acceptsEmptyFieldAndResolvedArrays)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_uplc_frame_new_constr(arena, nullptr, 0U, nullptr, 0U, nullptr, 0U, inner, &frame),
    CARDANO_SUCCESS);
  EXPECT_EQ(frame->as.constr.field_count, 0U);
  EXPECT_EQ(frame->as.constr.resolved_count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_constr, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_uplc_frame_new_constr(nullptr, nullptr, 0U, nullptr, 0U, nullptr, 0U, inner, &frame),
    CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(
    cardano_uplc_frame_new_constr(arena, nullptr, 0U, nullptr, 0U, nullptr, 0U, nullptr, &frame),
    CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(
    cardano_uplc_frame_new_constr(arena, nullptr, 0U, nullptr, 0U, nullptr, 0U, inner, nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_constr, failsWhenCountWithoutArray)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_uplc_frame_new_constr(arena, nullptr, 0U, nullptr, 1U, nullptr, 0U, inner, &frame),
    CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(
    cardano_uplc_frame_new_constr(arena, nullptr, 0U, nullptr, 0U, nullptr, 1U, inner, &frame),
    CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_constr, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq = new_arena();
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_frame_t* inner  = nullptr;
  cardano_uplc_frame_t* frame  = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(prereq, &inner), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_frame_new_constr(arena, nullptr, 0U, nullptr, 0U, nullptr, 0U, inner, &frame);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(frame, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_frame_new_cases, setsKindEnvBranchesAndChainsContext)
{
  // Arrange
  cardano_uplc_arena_t*       arena = new_arena();
  const cardano_uplc_term_t*  b0    = new_error_term(arena);
  const cardano_uplc_value_t* held  = new_unit_value(arena);
  const cardano_uplc_env_t*   env   = nullptr;
  cardano_uplc_frame_t*       inner = nullptr;
  cardano_uplc_frame_t*       frame = nullptr;
  const cardano_uplc_term_t*  branches[1];

  branches[0] = b0;

  EXPECT_EQ(cardano_uplc_env_extend(arena, nullptr, held, &env), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_frame_new_cases(arena, env, branches, 1U, inner, &frame);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(frame->kind, CARDANO_UPLC_FRAME_CASES);
  EXPECT_EQ(frame->as.cases.env, env);
  EXPECT_EQ(frame->as.cases.branch_count, 1U);
  EXPECT_EQ(frame->as.cases.branches[0], b0);
  EXPECT_EQ(frame->as.cases.ctx, inner);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_cases, acceptsNoBranches)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_frame_new_cases(arena, nullptr, nullptr, 0U, inner, &frame), CARDANO_SUCCESS);
  EXPECT_EQ(frame->as.cases.branch_count, 0U);
  EXPECT_EQ(frame->as.cases.branches, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_cases, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_frame_new_cases(nullptr, nullptr, nullptr, 0U, inner, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_cases(arena, nullptr, nullptr, 0U, nullptr, &frame), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_frame_new_cases(arena, nullptr, nullptr, 0U, inner, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_cases, failsWhenBranchCountWithoutArray)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_frame_t* inner = nullptr;
  cardano_uplc_frame_t* frame = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(arena, &inner), CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_uplc_frame_new_cases(arena, nullptr, nullptr, 1U, inner, &frame),
    CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_frame_new_cases, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq = new_arena();
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_frame_t* inner  = nullptr;
  cardano_uplc_frame_t* frame  = nullptr;

  EXPECT_EQ(cardano_uplc_frame_new_no_frame(prereq, &inner), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_frame_new_cases(arena, nullptr, nullptr, 0U, inner, &frame);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(frame, nullptr);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

/* UNIT TESTS - EVALUATE ****************************************************/

static const int64_t STARTUP_CPU = 100;
static const int64_t STARTUP_MEM = 100;
static const int64_t STEP_CPU    = 16000;
static const int64_t STEP_MEM    = 100;

static int64_t
expected_cpu(int64_t steps)
{
  return STARTUP_CPU + (steps * STEP_CPU);
}

static int64_t
expected_mem(int64_t steps)
{
  return STARTUP_MEM + (steps * STEP_MEM);
}

static const cardano_uplc_constant_t*
new_int_constant(cardano_uplc_arena_t* arena, int64_t value)
{
  cardano_bigint_t*        bigint   = nullptr;
  cardano_uplc_constant_t* constant = nullptr;

  EXPECT_EQ(cardano_bigint_from_int(value, &bigint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, bigint, &constant), CARDANO_SUCCESS);
  cardano_bigint_unref(&bigint);

  return constant;
}

static const cardano_uplc_term_t*
new_int_term(cardano_uplc_arena_t* arena, int64_t value)
{
  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, new_int_constant(arena, value), &term), CARDANO_SUCCESS);
  return term;
}

static const cardano_uplc_term_t*
new_unit_term(cardano_uplc_arena_t* arena)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_term_t*     term     = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, constant, &term), CARDANO_SUCCESS);

  return term;
}

static cardano_uplc_program_t*
new_program(cardano_uplc_arena_t* arena, const cardano_uplc_term_t* term)
{
  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));

  EXPECT_NE(program, nullptr);

  program->version_major = 1U;
  program->version_minor = 0U;
  program->version_patch = 0U;
  program->term          = term;

  return program;
}

TEST(cardano_uplc_evaluate, aConstantEvaluatesToItselfWithTheExactBudget)
{
  // Arrange
  cardano_uplc_arena_t*      arena   = new_arena();
  const cardano_uplc_term_t* term    = new_int_term(arena, 5);
  cardano_uplc_program_t*    program = new_program(arena, term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(result.result->as.constant->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 5);
  EXPECT_EQ(result.spent.cpu, expected_cpu(1));
  EXPECT_EQ(result.spent.mem, expected_mem(1));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, identityAppliedToAConstantReturnsTheConstant)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &lambda), CARDANO_SUCCESS);

  cardano_uplc_term_t* apply = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, lambda, new_int_term(arena, 42), &apply), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, apply);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 42);
  EXPECT_EQ(result.spent.cpu, expected_cpu(4));
  EXPECT_EQ(result.spent.mem, expected_mem(4));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, forceOfADelayReturnsTheInnerValue)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* delay = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_delay(arena, new_unit_term(arena), &delay), CARDANO_SUCCESS);

  cardano_uplc_term_t* force = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_force(arena, delay, &force), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, force);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(result.result->as.constant->kind, CARDANO_UPLC_TYPE_UNIT);
  EXPECT_EQ(result.spent.cpu, expected_cpu(3));
  EXPECT_EQ(result.spent.mem, expected_mem(3));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, caseSelectsTheBranchForTheConstrTag)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* constr = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 1U, nullptr, 0U, &constr), CARDANO_SUCCESS);

  const cardano_uplc_term_t* branches[2];
  branches[0] = new_int_term(arena, 100);
  branches[1] = new_int_term(arena, 200);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, constr, branches, 2U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 200);
  EXPECT_EQ(result.spent.cpu, expected_cpu(3));
  EXPECT_EQ(result.spent.mem, expected_mem(3));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, constrWithFieldsEvaluatesEachField)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* fields[2];
  fields[0] = new_int_term(arena, 7);
  fields[1] = new_int_term(arena, 9);

  cardano_uplc_term_t* constr = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 3U, fields, 2U, &constr), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, constr);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(result.result->as.constr.tag, 3U);
  ASSERT_EQ(result.result->as.constr.field_count, 2U);
  EXPECT_EQ(int_const_i64(result.result->as.constr.fields[0]->as.constant), 7);
  EXPECT_EQ(int_const_i64(result.result->as.constr.fields[1]->as.constant), 9);
  EXPECT_EQ(result.spent.cpu, expected_cpu(3));
  EXPECT_EQ(result.spent.mem, expected_mem(3));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, theErrorTermProducesTheErrorTermStatus)
{
  // Arrange
  cardano_uplc_arena_t*      arena   = new_arena();
  cardano_uplc_program_t*    program = new_program(arena, new_error_term(arena));
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);
  EXPECT_EQ(result.result, nullptr);
  EXPECT_EQ(result.spent.cpu, STARTUP_CPU);
  EXPECT_EQ(result.spent.mem, STARTUP_MEM);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, anOpenVariableIsAScriptError)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, var);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);
  EXPECT_EQ(result.result, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, applyingANonFunctionConstantIsAScriptError)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* apply = nullptr;
  EXPECT_EQ(
    cardano_uplc_term_new_apply(arena, new_int_term(arena, 1), new_int_term(arena, 2), &apply),
    CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, apply);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);
  EXPECT_EQ(result.result, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, forcingANonDelayConstantIsAScriptError)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* force = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_force(arena, new_int_term(arena, 1), &force), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, force);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, caseWithAnOutOfRangeTagIsAScriptError)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* constr = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 5U, nullptr, 0U, &constr), CARDANO_SUCCESS);

  const cardano_uplc_term_t* branches[1];
  branches[0] = new_int_term(arena, 1);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, constr, branches, 1U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

static const cardano_uplc_term_t*
new_bool_term(cardano_uplc_arena_t* arena, bool value)
{
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_term_t*     term     = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_bool(arena, value, &constant), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, constant, &term), CARDANO_SUCCESS);

  return term;
}

static const cardano_uplc_type_t*
new_integer_type(cardano_uplc_arena_t* arena)
{
  cardano_uplc_type_t* type = nullptr;
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &type), CARDANO_SUCCESS);
  return type;
}

static const cardano_uplc_term_t*
new_int_list_term(cardano_uplc_arena_t* arena, const int64_t* values, size_t count)
{
  const cardano_uplc_constant_t** items = nullptr;

  if (count > 0U)
  {
    items = static_cast<const cardano_uplc_constant_t**>(
      cardano_uplc_arena_alloc(arena, sizeof(const cardano_uplc_constant_t*) * count, 0U));
    EXPECT_NE(items, nullptr);

    for (size_t i = 0; i < count; ++i)
    {
      items[i] = new_int_constant(arena, values[i]);
    }
  }

  cardano_uplc_constant_t* list = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_list(arena, new_integer_type(arena), items, count, &list), CARDANO_SUCCESS);

  cardano_uplc_term_t* term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, list, &term), CARDANO_SUCCESS);

  return term;
}

// (lam x (lam xs x)): a head-selecting branch for list and pair cases.
static const cardano_uplc_term_t*
new_first_of_two_lambda(cardano_uplc_arena_t* arena)
{
  cardano_uplc_term_t* var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 2U, &var), CARDANO_SUCCESS);

  cardano_uplc_term_t* inner = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &inner), CARDANO_SUCCESS);

  cardano_uplc_term_t* outer = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, inner, &outer), CARDANO_SUCCESS);

  return outer;
}

// Vector term/constant-case/bool/bool-01: (case (con bool False) (con integer 0) (con integer 1))
// -> (con integer 0), budget cpu 48100 mem 400.
TEST(cardano_uplc_evaluate, caseOnBoolFalseSelectsBranchZero)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* branches[2];
  branches[0] = new_int_term(arena, 0);
  branches[1] = new_int_term(arena, 1);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, new_bool_term(arena, false), branches, 2U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 0);
  EXPECT_EQ(result.spent.cpu, 48100);
  EXPECT_EQ(result.spent.mem, 400);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// Vector term/constant-case/bool/bool-02: (case (con bool True) (con integer 0) (con integer 1))
// -> (con integer 1), budget cpu 48100 mem 400.
TEST(cardano_uplc_evaluate, caseOnBoolTrueSelectsBranchOne)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* branches[2];
  branches[0] = new_int_term(arena, 0);
  branches[1] = new_int_term(arena, 1);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, new_bool_term(arena, true), branches, 2U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 1);
  EXPECT_EQ(result.spent.cpu, 48100);
  EXPECT_EQ(result.spent.mem, 400);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// Vector term/constant-case/integer/integer-02:
// (case (con integer 1) (con integer 42) (con integer 43) (con integer 443)) -> (con integer 43),
// budget cpu 48100 mem 400.
TEST(cardano_uplc_evaluate, caseOnIntegerSelectsBranchAtItsIndex)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* branches[3];
  branches[0] = new_int_term(arena, 42);
  branches[1] = new_int_term(arena, 43);
  branches[2] = new_int_term(arena, 443);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, new_int_term(arena, 1), branches, 3U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 43);
  EXPECT_EQ(result.spent.cpu, 48100);
  EXPECT_EQ(result.spent.mem, 400);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// Vector term/constant-case/unit/unit-01: (case (con unit ()) (con integer 42)) -> (con integer 42),
// budget cpu 48100 mem 400.
TEST(cardano_uplc_evaluate, caseOnUnitSelectsTheSoleBranch)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* branches[1];
  branches[0] = new_int_term(arena, 42);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, new_unit_term(arena), branches, 1U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 42);
  EXPECT_EQ(result.spent.cpu, 48100);
  EXPECT_EQ(result.spent.mem, 400);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// Vector term/constant-case/list/list-01:
// (case (con (list integer) [1,2,3,4]) (lam x (lam xs x)) (con integer -1)) -> (con integer 1),
// budget cpu 80100 mem 600.
TEST(cardano_uplc_evaluate, caseOnNonEmptyListSelectsBranchZeroWithHeadAndTail)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const int64_t values[4] = { 1, 2, 3, 4 };

  const cardano_uplc_term_t* branches[2];
  branches[0] = new_first_of_two_lambda(arena);
  branches[1] = new_int_term(arena, -1);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(
    cardano_uplc_term_new_case(arena, new_int_list_term(arena, values, 4U), branches, 2U, &case_term),
    CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert: the head 1 is applied to (lam x (lam xs x)) which returns it.
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 1);
  EXPECT_EQ(result.spent.cpu, 80100);
  EXPECT_EQ(result.spent.mem, 600);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// Vector term/constant-case/list/list-02:
// (case (con (list integer) []) (lam x (lam xs x)) (con integer -1)) -> (con integer -1),
// budget cpu 48100 mem 400.
TEST(cardano_uplc_evaluate, caseOnEmptyListSelectsBranchOne)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* branches[2];
  branches[0] = new_first_of_two_lambda(arena);
  branches[1] = new_int_term(arena, -1);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(
    cardano_uplc_term_new_case(arena, new_int_list_term(arena, nullptr, 0U), branches, 2U, &case_term),
    CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(result.result->as.constant), -1);
  EXPECT_EQ(result.spent.cpu, 48100);
  EXPECT_EQ(result.spent.mem, 400);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// Vector term/constant-case/pair/pair-01:
// (case (con (pair integer bool) (42, False)) (lam l (lam r l))) -> (con integer 42),
// budget cpu 80100 mem 600.
TEST(cardano_uplc_evaluate, caseOnPairSelectsBranchZeroWithFirstAndSecond)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_constant_t* fst = nullptr;
  {
    cardano_bigint_t* bigint = nullptr;
    EXPECT_EQ(cardano_bigint_from_int(42, &bigint), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_uplc_constant_new_integer(arena, bigint, &fst), CARDANO_SUCCESS);
    cardano_bigint_unref(&bigint);
  }

  cardano_uplc_constant_t* snd = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_bool(arena, false, &snd), CARDANO_SUCCESS);

  cardano_uplc_constant_t* pair = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, fst, snd, &pair), CARDANO_SUCCESS);

  cardano_uplc_term_t* pair_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, pair, &pair_term), CARDANO_SUCCESS);

  const cardano_uplc_term_t* branches[1];
  branches[0] = new_first_of_two_lambda(arena);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, pair_term, branches, 1U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert: the first component 42 is selected by (lam l (lam r l)).
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(result.result->as.constant), 42);
  EXPECT_EQ(result.spent.cpu, 80100);
  EXPECT_EQ(result.spent.mem, 600);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// An integer scrutinee whose index is out of range for the branch list is a script error.
TEST(cardano_uplc_evaluate, caseOnIntegerOutOfRangeIsAScriptError)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* branches[1];
  branches[0] = new_int_term(arena, 7);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, new_int_term(arena, 5), branches, 1U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// A negative integer scrutinee is never a valid branch index, so it is a script error.
TEST(cardano_uplc_evaluate, caseOnNegativeIntegerIsAScriptError)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();

  const cardano_uplc_term_t* branches[1];
  branches[0] = new_int_term(arena, 7);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, new_int_term(arena, -1), branches, 1U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

// A case over a value that is neither a constr nor a supported constant (here a lambda
// value, the result of forcing nothing) is a script error.
TEST(cardano_uplc_evaluate, caseOverALambdaScrutineeIsAScriptError)
{
  // Arrange: scrutinee (lam x x) is a lambda value, not a constr or constant.
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &lambda), CARDANO_SUCCESS);

  const cardano_uplc_term_t* branches[1];
  branches[0] = new_int_term(arena, 1);

  cardano_uplc_term_t* case_term = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_case(arena, lambda, branches, 1U, &case_term), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, case_term);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, aSaturatedValueBuiltinRejectsMismatchedArgsAsScriptError)
{
  // Arrange: [[(builtin unionValue) 1] 1] saturates the builtin (arity 2, no
  // forces). unionValue is implemented and expects two value constants; feeding it
  // integers is a type mismatch, which is a script failure (an error term).
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* builtin = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_UNION_VALUE, &builtin), CARDANO_SUCCESS);

  cardano_uplc_term_t* apply0 = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, builtin, new_int_term(arena, 1), &apply0), CARDANO_SUCCESS);

  cardano_uplc_term_t* apply1 = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, apply0, new_int_term(arena, 1), &apply1), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, apply1);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert: the builtin saturates and its body rejects the mismatched arguments,
  // so the run is a script error term, never UNSUPPORTED, never SUCCESS, never a
  // host failure.
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_ERROR_TERM);
  EXPECT_NE(result.status, CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN);
  EXPECT_EQ(result.result, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, anUnsaturatedBuiltinDischargesToItsTerm)
{
  // Arrange: (force (builtin ifThenElse)) is a value: one force, no args.
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* builtin = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_IF_THEN_ELSE, &builtin), CARDANO_SUCCESS);

  cardano_uplc_term_t* force = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_force(arena, builtin, &force), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, force);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_FORCE);
  EXPECT_EQ(result.result->as.unary->kind, CARDANO_UPLC_TERM_BUILTIN);
  EXPECT_EQ(result.result->as.unary->as.builtin, CARDANO_UPLC_BUILTIN_IF_THEN_ELSE);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, aLambdaDischargesAndSubstitutesItsCapturedEnvironment)
{
  // Arrange: ((lam x (lam y x)) (con integer 5)) returns the closure (lam y 5).
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* outer_var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 2U, &outer_var), CARDANO_SUCCESS);

  cardano_uplc_term_t* inner_lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, outer_var, &inner_lambda), CARDANO_SUCCESS);

  cardano_uplc_term_t* outer_lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, inner_lambda, &outer_lambda), CARDANO_SUCCESS);

  cardano_uplc_term_t* apply = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, outer_lambda, new_int_term(arena, 5), &apply), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, apply);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert: the result is (lam y (con integer 5)) - the captured x discharged in.
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_LAMBDA);
  EXPECT_EQ(result.result->as.unary->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(int_const_i64(result.result->as.unary->as.constant), 5);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, aDelayDischargesToItsTerm)
{
  // Arrange: (delay (con unit)) evaluates to itself, a delay value.
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* delay = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_delay(arena, new_unit_term(arena), &delay), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, delay);
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_DELAY);
  EXPECT_EQ(result.result->as.unary->kind, CARDANO_UPLC_TERM_CONSTANT);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, aTinyBudgetIsReportedAsOutOfBudget)
{
  // Arrange: a program of many applies exhausts a tiny budget.
  cardano_uplc_arena_t* arena = new_arena();

  cardano_uplc_term_t* var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);

  cardano_uplc_term_t* identity = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &identity), CARDANO_SUCCESS);

  const cardano_uplc_term_t* term = new_int_term(arena, 1);

  for (int i = 0; i < 1000; ++i)
  {
    cardano_uplc_term_t* apply = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_apply(arena, identity, term, &apply), CARDANO_SUCCESS);
    term = apply;
  }

  cardano_uplc_program_t*    program = new_program(arena, term);
  cardano_uplc_budget_t      budget  = { 1000, 1000 };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_OUT_OF_BUDGET);
  EXPECT_EQ(result.result, nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, aDeeplyNestedTermDoesNotOverflowTheCStack)
{
  // Arrange: 100000 nested forces over delays; the iterative loop must not recurse.
  cardano_uplc_arena_t* arena = new_arena();

  const int                  depth = 100000;
  const cardano_uplc_term_t* term  = new_unit_term(arena);

  cardano_uplc_term_t* base_delay = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_delay(arena, term, &base_delay), CARDANO_SUCCESS);
  term = base_delay;

  for (int i = 0; i < depth; ++i)
  {
    cardano_uplc_term_t* delayed = nullptr;
    cardano_uplc_term_t* forced  = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_delay(arena, term, &delayed), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_uplc_term_new_force(arena, delayed, &forced), CARDANO_SUCCESS);
    term = forced;
  }

  cardano_uplc_term_t* top_force = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_force(arena, term, &top_force), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, top_force);
  cardano_uplc_budget_t      budget  = { 100000000000LL, 100000000000LL };
  cardano_uplc_eval_result_t result;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(result.result->as.constant->kind, CARDANO_UPLC_TYPE_UNIT);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, aDeeplyNestedResultDoesNotOverflowTheDischargeStack)
{
  // Arrange: a lambda whose body nests 20000 forces over delays - far deeper than
  // PRV_DISCHARGE_MAX_DEPTH (4096). The lambda evaluates in a single step, so the depth
  // is reached only when its closure body is discharged via with_env. The discharge
  // must not recurse off the C stack: it stops with the bounded host error, never crashes.
  cardano_uplc_arena_t* arena = new_arena();

  const int depth = 20000;

  cardano_uplc_term_t* var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);

  const cardano_uplc_term_t* body = var;

  for (int i = 0; i < depth; ++i)
  {
    cardano_uplc_term_t* delayed = nullptr;
    cardano_uplc_term_t* forced  = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_delay(arena, body, &delayed), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_uplc_term_new_force(arena, delayed, &forced), CARDANO_SUCCESS);
    body = forced;
  }

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, body, &lambda), CARDANO_SUCCESS);

  cardano_uplc_program_t*    program = new_program(arena, lambda);
  cardano_uplc_budget_t      budget  = { 100000000000LL, 100000000000LL };
  cardano_uplc_eval_result_t result;

  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert: the bound is hit while discharging the deep closure body and reported as a
  // host error, distinct from a script outcome.
  EXPECT_EQ(error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, failsOnNullArgs)
{
  cardano_uplc_arena_t*      arena   = new_arena();
  cardano_uplc_program_t*    program = new_program(arena, new_int_term(arena, 1));
  cardano_uplc_budget_t      budget  = { 1000, 1000 };
  cardano_uplc_eval_result_t result;

  EXPECT_EQ(
    cardano_uplc_evaluate(nullptr, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result),
    CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(
    cardano_uplc_evaluate(arena, nullptr, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result),
    CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(
    cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, failsOnNullProgramTerm)
{
  cardano_uplc_arena_t*      arena   = new_arena();
  cardano_uplc_program_t*    program = new_program(arena, nullptr);
  cardano_uplc_budget_t      budget  = { 1000, 1000 };
  cardano_uplc_eval_result_t result;

  EXPECT_EQ(
    cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result),
    CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, selectsTheV1CostTableForV1Version)
{
  cardano_uplc_arena_t*      arena   = new_arena();
  cardano_uplc_program_t*    program = new_program(arena, new_int_term(arena, 1));
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  EXPECT_EQ(
    cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V1, budget, &result),
    CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  EXPECT_EQ(result.spent.cpu, expected_cpu(1));
  EXPECT_EQ(result.spent.mem, expected_mem(1));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, selectsTheV2CostTableForV2Version)
{
  cardano_uplc_arena_t*      arena   = new_arena();
  cardano_uplc_program_t*    program = new_program(arena, new_int_term(arena, 1));
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  EXPECT_EQ(
    cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V2, budget, &result),
    CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, anUnknownVersionDefaultsToV3)
{
  cardano_uplc_arena_t*      arena   = new_arena();
  cardano_uplc_program_t*    program = new_program(arena, new_int_term(arena, 1));
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  EXPECT_EQ(
    cardano_uplc_evaluate(arena, program, static_cast<cardano_uplc_machine_version_t>(99), budget, &result),
    CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  EXPECT_EQ(result.spent.cpu, expected_cpu(1));

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_evaluate, failsCleanlyWhenTheArenaCannotAllocateTheNoFrame)
{
  cardano_uplc_arena_t*      prereq  = new_arena();
  cardano_uplc_arena_t*      arena   = nullptr;
  cardano_uplc_program_t*    program = new_program(prereq, new_int_term(prereq, 1));
  cardano_uplc_budget_t      budget  = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;

  EXPECT_EQ(cardano_uplc_arena_new(8U, &arena), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_evaluate, failsCleanlyWhenAnInteriorAllocationFails)
{
  cardano_uplc_arena_t* prereq = new_arena();

  cardano_uplc_term_t* var = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_var(prereq, 1U, &var), CARDANO_SUCCESS);

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(prereq, var, &lambda), CARDANO_SUCCESS);

  cardano_uplc_term_t* apply = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(prereq, lambda, new_int_term(prereq, 7), &apply), CARDANO_SUCCESS);

  cardano_uplc_program_t* program = new_program(prereq, apply);
  cardano_uplc_budget_t   budget  = { 10000000000LL, 10000000000LL };

  for (int nth = 1; nth <= 40; ++nth)
  {
    cardano_uplc_arena_t*      arena = nullptr;
    cardano_uplc_eval_result_t result;

    EXPECT_EQ(cardano_uplc_arena_new(8U, &arena), CARDANO_SUCCESS);

    result.status = CARDANO_UPLC_EVAL_OUT_OF_BUDGET;

    reset_allocators_run_count();
    set_malloc_limit(nth);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t error = cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

    cardano_set_allocators(malloc, realloc, free);
    reset_limited_malloc();

    EXPECT_TRUE((error == CARDANO_SUCCESS) || (error == CARDANO_ERROR_MEMORY_ALLOCATION_FAILED));

    cardano_uplc_arena_free(&arena);
  }

  cardano_uplc_arena_free(&prereq);
}
