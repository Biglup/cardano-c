/**
 * \file term.cpp
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
#include <cardano/plutus_data/plutus_data.h>
#include "../../src/uplc/machine/uplc_machine.h"
#include "../../src/uplc/ast/uplc_term.h"

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/data/uplc_data.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cstring>

#include <gmock/gmock.h>

/* STATIC HELPERS ************************************************************/

static cardano_uplc_arena_t*
new_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  cardano_error_t       error = cardano_uplc_arena_new(4096U, &arena);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return arena;
}

static cardano_bigint_t*
new_bigint(int64_t value)
{
  cardano_bigint_t* bigint = nullptr;
  cardano_error_t   error  = cardano_bigint_from_int(value, &bigint);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return bigint;
}

static cardano_buffer_t*
new_buffer(const char* text)
{
  return cardano_buffer_new_from(reinterpret_cast<const byte_t*>(text), strlen(text));
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

/**
 * \brief The natural maximum alignment the arena uses when align 0 is passed.
 *
 * Mirrors the arena's internal ARENA_MAX_ALIGN so a test can bound the
 * alignment padding a single constant-node allocation may consume.
 */
static const size_t kArenaMaxAlign =
  sizeof(long double) > sizeof(void*) ? sizeof(long double) : sizeof(void*);

/**
 * \brief Builds an arena whose byte ceiling admits exactly one constant node.
 *
 * The ceiling is the size of a constant node plus the worst-case alignment
 * padding the arena's first (oversized) allocation may add, so a refcounted
 * constant constructor allocates its node successfully and then trips the
 * ceiling when it tries to register the on-free unref node. That register
 * failure surfaces as \ref CARDANO_ERROR_ILLEGAL_STATE.
 *
 * \return The new arena; the caller frees it with \ref cardano_uplc_arena_free.
 */
static cardano_uplc_arena_t*
new_arena_admitting_one_constant_node()
{
  cardano_uplc_arena_t* arena   = nullptr;
  size_t                ceiling = sizeof(cardano_uplc_constant_t) + (kArenaMaxAlign - 1U);
  cardano_error_t       error   = cardano_uplc_int_arena_new_with_ceiling(4096U, ceiling, &arena);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  return arena;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_term_tags, matchTheFlatTermTags)
{
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_VAR), 0);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_DELAY), 1);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_LAMBDA), 2);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_APPLY), 3);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_CONSTANT), 4);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_FORCE), 5);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_ERROR), 6);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_BUILTIN), 7);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_CONSTR), 8);
  EXPECT_EQ(static_cast<int>(CARDANO_UPLC_TERM_CASE), 9);
}

TEST(cardano_uplc_term_new_var, setsKindAndIndex)
{
  // Arrange
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_term_new_var(arena, 7U, &term);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_VAR);
  EXPECT_EQ(term->as.var_index, 7U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_var, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(nullptr, 1U, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_var, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_var(arena, 1U, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(term, nullptr);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_delay, setsKindAndBody)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  body  = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &body), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_term_new_delay(arena, body, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_DELAY);
  EXPECT_EQ(term->as.unary, body);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_delay, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  body  = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &body), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_term_new_delay(nullptr, body, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_delay(arena, nullptr, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_delay(arena, body, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_delay, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq = new_arena();
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_term_t*  body   = nullptr;
  cardano_uplc_term_t*  term   = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(prereq, &body), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_delay(arena, body, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_term_new_lambda, setsKindAndBody)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  body  = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &body), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_term_new_lambda(arena, body, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_LAMBDA);
  EXPECT_EQ(term->as.unary, body);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_lambda, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  body  = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &body), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_term_new_lambda(nullptr, body, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, nullptr, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, body, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_lambda, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq = new_arena();
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_term_t*  body   = nullptr;
  cardano_uplc_term_t*  term   = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(prereq, 1U, &body), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_lambda(arena, body, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_term_new_apply, setsKindAndChildren)
{
  cardano_uplc_arena_t* arena    = new_arena();
  cardano_uplc_term_t*  function = nullptr;
  cardano_uplc_term_t*  argument = nullptr;
  cardano_uplc_term_t*  term     = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &function), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 2U, &argument), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_term_new_apply(arena, function, argument, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(term->as.apply.function, function);
  EXPECT_EQ(term->as.apply.argument, argument);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_apply, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  a     = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &a), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_term_new_apply(nullptr, a, a, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, nullptr, a, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, a, nullptr, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, a, a, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_apply, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq = new_arena();
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_term_t*  a      = nullptr;
  cardano_uplc_term_t*  term   = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(prereq, 1U, &a), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_apply(arena, a, a, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_term_new_force, setsKindAndBody)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  body  = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &body), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_term_new_force(arena, body, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_FORCE);
  EXPECT_EQ(term->as.unary, body);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_force, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  body  = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &body), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_term_new_force(nullptr, body, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_force(arena, nullptr, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_force(arena, body, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_force, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq = new_arena();
  cardano_uplc_arena_t* arena  = new_arena();
  cardano_uplc_term_t*  body   = nullptr;
  cardano_uplc_term_t*  term   = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(prereq, 1U, &body), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_force(arena, body, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_term_new_error, setsKind)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  cardano_error_t error = cardano_uplc_term_new_error(arena, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_ERROR);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_error, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(nullptr, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_error(arena, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_error, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_error(arena, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_builtin, setsKindAndTag)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  cardano_error_t error = cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_BUILTIN);
  EXPECT_EQ(term->as.builtin, CARDANO_UPLC_BUILTIN_ADD_INTEGER);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_builtin, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_builtin(nullptr, CARDANO_UPLC_BUILTIN_ADD_INTEGER, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_builtin, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_constr, setsKindTagAndFields)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  f0    = nullptr;
  cardano_uplc_term_t*  f1    = nullptr;
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &f0), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 2U, &f1), CARDANO_SUCCESS);

  const cardano_uplc_term_t** fields = reinterpret_cast<const cardano_uplc_term_t**>(
    cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_term_t*) * 2U, 0U));
  fields[0] = f0;
  fields[1] = f1;

  cardano_error_t error = cardano_uplc_term_new_constr(arena, 3U, fields, 2U, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(term->as.constr.tag, 3U);
  EXPECT_EQ(term->as.constr.field_count, 2U);
  EXPECT_EQ(term->as.constr.fields[0], f0);
  EXPECT_EQ(term->as.constr.fields[1], f1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_constr, allowsEmptyFields)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  cardano_error_t error = cardano_uplc_term_new_constr(arena, 0U, nullptr, 0U, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CONSTR);
  EXPECT_EQ(term->as.constr.field_count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_constr, rejectsNullFieldsWithCount)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  cardano_error_t error = cardano_uplc_term_new_constr(arena, 0U, nullptr, 2U, &term);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_constr, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_constr(nullptr, 0U, nullptr, 0U, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_constr(arena, 0U, nullptr, 0U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_constr, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_term_t*  term  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_constr(arena, 0U, nullptr, 0U, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_case, setsKindScrutineeAndBranches)
{
  cardano_uplc_arena_t* arena     = new_arena();
  cardano_uplc_term_t*  scrutinee = nullptr;
  cardano_uplc_term_t*  b0        = nullptr;
  cardano_uplc_term_t*  term      = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &scrutinee), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 2U, &b0), CARDANO_SUCCESS);

  const cardano_uplc_term_t** branches = reinterpret_cast<const cardano_uplc_term_t**>(
    cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_term_t*), 0U));
  branches[0] = b0;

  cardano_error_t error = cardano_uplc_term_new_case(arena, scrutinee, branches, 1U, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CASE);
  EXPECT_EQ(term->as.cases.scrutinee, scrutinee);
  EXPECT_EQ(term->as.cases.branch_count, 1U);
  EXPECT_EQ(term->as.cases.branches[0], b0);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_case, allowsEmptyBranches)
{
  cardano_uplc_arena_t* arena     = new_arena();
  cardano_uplc_term_t*  scrutinee = nullptr;
  cardano_uplc_term_t*  term      = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &scrutinee), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_term_new_case(arena, scrutinee, nullptr, 0U, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->as.cases.branch_count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_case, rejectsNullBranchesWithCount)
{
  cardano_uplc_arena_t* arena     = new_arena();
  cardano_uplc_term_t*  scrutinee = nullptr;
  cardano_uplc_term_t*  term      = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &scrutinee), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_term_new_case(arena, scrutinee, nullptr, 1U, &term);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_case, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena     = new_arena();
  cardano_uplc_term_t*  scrutinee = nullptr;
  cardano_uplc_term_t*  term      = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &scrutinee), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_term_new_case(nullptr, scrutinee, nullptr, 0U, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_case(arena, nullptr, nullptr, 0U, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_case(arena, scrutinee, nullptr, 0U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_case, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* prereq    = new_arena();
  cardano_uplc_arena_t* arena     = new_arena();
  cardano_uplc_term_t*  scrutinee = nullptr;
  cardano_uplc_term_t*  term      = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(prereq, 1U, &scrutinee), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_case(arena, scrutinee, nullptr, 0U, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_term_new_constant, wrapsConstant)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_term_t*     term     = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_term_new_constant(arena, constant, &term);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(term->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(term->as.constant, constant);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_constant, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_term_t*     term     = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_term_new_constant(nullptr, constant, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, nullptr, &term), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, constant, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_term_new_constant, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*    prereq   = new_arena();
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;
  cardano_uplc_term_t*     term     = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(prereq, &constant), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_term_new_constant(arena, constant, &term);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_constant_new_integer, balancesBigintRefcountOnArenaFree)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_bigint_t*        bigint   = new_bigint(42);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_bigint_refcount(bigint);

  cardano_error_t error = cardano_uplc_constant_new_integer(arena, bigint, &constant);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_FALSE(constant->as.integer.is_small);
  EXPECT_EQ(constant->as.integer.big, bigint);
  EXPECT_EQ(cardano_bigint_refcount(bigint), before + 1U);

  cardano_uplc_arena_free(&arena);

  EXPECT_EQ(cardano_bigint_refcount(bigint), before);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_constant_new_integer, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_bigint_t*        bigint   = new_bigint(1);
  cardano_uplc_constant_t* constant = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_integer(nullptr, bigint, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, nullptr, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, bigint, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_constant_new_integer, failsOnArenaAllocationFailureWithoutLeak)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_bigint_t*        bigint   = new_bigint(1);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_bigint_refcount(bigint);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_integer(arena, bigint, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_bigint_refcount(bigint), before);

  cardano_uplc_arena_free(&arena);
  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_constant_new_integer, failsAndUnrefsWhenRegisterFails)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_bigint_t*        bigint   = new_bigint(1);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_bigint_refcount(bigint);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_integer(arena, bigint, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_bigint_refcount(bigint), before);

  cardano_uplc_arena_free(&arena);
  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_constant_new_integer, failsAndUnrefsWhenRegisterCrossesCeiling)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena_admitting_one_constant_node();
  cardano_bigint_t*        bigint   = new_bigint(1);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_bigint_refcount(bigint);

  // Act
  cardano_error_t error = cardano_uplc_constant_new_integer(arena, bigint, &constant);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ILLEGAL_STATE);
  EXPECT_EQ(cardano_bigint_refcount(bigint), before);

  // Cleanup
  cardano_uplc_arena_free(&arena);
  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_constant_new_byte_string, copiesBytesWithoutRetainingBuffer)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_buffer_t*        buffer   = new_buffer("hello");
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_buffer_refcount(buffer);

  cardano_error_t error = cardano_uplc_constant_new_byte_string(arena, buffer, &constant);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BYTE_STRING);
  EXPECT_EQ(constant->as.bytes.size, 5U);
  EXPECT_NE(constant->as.bytes.data, cardano_buffer_get_data(buffer));
  EXPECT_EQ(memcmp(constant->as.bytes.data, "hello", 5U), 0);
  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  cardano_uplc_arena_free(&arena);

  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_byte_string, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_buffer_t*        buffer   = new_buffer("x");
  cardano_uplc_constant_t* constant = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_byte_string(nullptr, buffer, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_byte_string(arena, nullptr, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_byte_string(arena, buffer, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_byte_string, failsOnArenaAllocationFailureWithoutLeak)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_buffer_t*        buffer   = new_buffer("x");
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_buffer_refcount(buffer);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_byte_string(arena, buffer, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_byte_string, failsWhenByteCopyCrossesCeiling)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena_admitting_one_constant_node();
  cardano_buffer_t*        buffer   = new_buffer("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_buffer_refcount(buffer);

  // Act
  cardano_error_t error = cardano_uplc_constant_new_byte_string(arena, buffer, &constant);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  // Cleanup
  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_string, copiesBytesWithoutRetainingBuffer)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_buffer_t*        buffer   = new_buffer("text");
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_buffer_refcount(buffer);

  cardano_error_t error = cardano_uplc_constant_new_string(arena, buffer, &constant);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_STRING);
  EXPECT_EQ(constant->as.string.size, 4U);
  EXPECT_NE(constant->as.string.data, cardano_buffer_get_data(buffer));
  EXPECT_EQ(memcmp(constant->as.string.data, "text", 4U), 0);
  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  cardano_uplc_arena_free(&arena);

  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_string, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_buffer_t*        buffer   = new_buffer("x");
  cardano_uplc_constant_t* constant = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_string(nullptr, buffer, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_string(arena, nullptr, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_string(arena, buffer, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_string, failsOnArenaAllocationFailureWithoutLeak)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_buffer_t*        buffer   = new_buffer("x");
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_buffer_refcount(buffer);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_string(arena, buffer, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_string, failsWhenByteCopyCrossesCeiling)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena_admitting_one_constant_node();
  cardano_buffer_t*        buffer   = new_buffer("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_buffer_refcount(buffer);

  // Act
  cardano_error_t error = cardano_uplc_constant_new_string(arena, buffer, &constant);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_buffer_refcount(buffer), before);

  // Cleanup
  cardano_uplc_arena_free(&arena);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_uplc_constant_new_bool, setsKindAndValue)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  cardano_error_t error = cardano_uplc_constant_new_bool(arena, true, &constant);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_BOOL);
  EXPECT_TRUE(constant->as.boolean);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_bool, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_bool(nullptr, true, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_bool(arena, true, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_bool, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_bool(arena, false, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_unit, setsKind)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  cardano_error_t error = cardano_uplc_constant_new_unit(arena, &constant);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_UNIT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_unit, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(nullptr, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_unit, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_uplc_constant_t* constant = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_unit(arena, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_data, convertsToArenaDataWithoutRetainingTheLibraryData)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_plutus_data_t*   data     = new_data(99);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_plutus_data_refcount(data);

  cardano_error_t error = cardano_uplc_constant_new_data(arena, data, &constant);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(constant->kind, CARDANO_UPLC_TYPE_DATA);
  ASSERT_NE(constant->as.data, nullptr);
  EXPECT_EQ(constant->as.data->kind, CARDANO_UPLC_DATA_KIND_INTEGER);
  EXPECT_TRUE(constant->as.data->as.integer.is_small);
  EXPECT_EQ(constant->as.data->as.integer.small, 99);
  EXPECT_EQ(cardano_plutus_data_refcount(data), before);

  cardano_uplc_arena_free(&arena);

  EXPECT_EQ(cardano_plutus_data_refcount(data), before);

  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_constant_new_data, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_plutus_data_t*   data     = new_data(1);
  cardano_uplc_constant_t* constant = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_data(nullptr, data, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, nullptr, &constant), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_data(arena, data, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_constant_new_data, failsOnArenaAllocationFailureWithoutLeak)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_plutus_data_t*   data     = new_data(1);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_plutus_data_refcount(data);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_data(arena, data, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_plutus_data_refcount(data), before);

  cardano_uplc_arena_free(&arena);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_constant_new_data, failsAndUnrefsWhenRegisterFails)
{
  cardano_uplc_arena_t*    arena    = new_arena();
  cardano_plutus_data_t*   data     = new_data(1);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_plutus_data_refcount(data);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_data(arena, data, &constant);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_plutus_data_refcount(data), before);

  cardano_uplc_arena_free(&arena);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_constant_new_data, failsWithoutRetainingTheLibraryDataWhenArenaIsExhausted)
{
  // Arrange
  cardano_uplc_arena_t*    arena    = new_arena_admitting_one_constant_node();
  cardano_plutus_data_t*   data     = new_data(1);
  cardano_uplc_constant_t* constant = nullptr;

  size_t before = cardano_plutus_data_refcount(data);

  // Act
  cardano_error_t error = cardano_uplc_constant_new_data(arena, data, &constant);

  // Assert
  EXPECT_NE(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_refcount(data), before);

  // Cleanup
  cardano_uplc_arena_free(&arena);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_constant_new_list, carriesElementTypeAndItems)
{
  cardano_uplc_arena_t*    arena        = new_arena();
  cardano_uplc_type_t*     element_type = nullptr;
  cardano_bigint_t*        bigint       = new_bigint(5);
  cardano_uplc_constant_t* item         = nullptr;
  cardano_uplc_constant_t* list         = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &element_type), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, bigint, &item), CARDANO_SUCCESS);

  const cardano_uplc_constant_t** items = reinterpret_cast<const cardano_uplc_constant_t**>(
    cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_constant_t*), 0U));
  items[0] = item;

  cardano_error_t error = cardano_uplc_constant_new_list(arena, element_type, items, 1U, &list);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(list->kind, CARDANO_UPLC_TYPE_LIST);
  EXPECT_EQ(list->as.list.element_type, element_type);
  EXPECT_EQ(list->as.list.element_type->kind, CARDANO_UPLC_TYPE_INTEGER);
  EXPECT_EQ(list->as.list.count, 1U);
  EXPECT_EQ(list->as.list.items[0], item);

  cardano_uplc_arena_free(&arena);
  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_constant_new_list, allowsEmptyList)
{
  cardano_uplc_arena_t*    arena        = new_arena();
  cardano_uplc_type_t*     element_type = nullptr;
  cardano_uplc_constant_t* list         = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BOOL, nullptr, nullptr, &element_type), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_constant_new_list(arena, element_type, nullptr, 0U, &list);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(list->as.list.count, 0U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_list, rejectsNullItemsWithCount)
{
  cardano_uplc_arena_t*    arena        = new_arena();
  cardano_uplc_type_t*     element_type = nullptr;
  cardano_uplc_constant_t* list         = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BOOL, nullptr, nullptr, &element_type), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_constant_new_list(arena, element_type, nullptr, 2U, &list);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_list, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena        = new_arena();
  cardano_uplc_type_t*     element_type = nullptr;
  cardano_uplc_constant_t* list         = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BOOL, nullptr, nullptr, &element_type), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_constant_new_list(nullptr, element_type, nullptr, 0U, &list), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_list(arena, nullptr, nullptr, 0U, &list), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_list(arena, element_type, nullptr, 0U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_array, carriesElementTypeAndItemsAndKind)
{
  cardano_uplc_arena_t*    arena        = new_arena();
  cardano_uplc_type_t*     element_type = nullptr;
  cardano_bigint_t*        bigint       = new_bigint(7);
  cardano_uplc_constant_t* item         = nullptr;
  cardano_uplc_constant_t* array        = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &element_type), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, bigint, &item), CARDANO_SUCCESS);

  const cardano_uplc_constant_t** items = reinterpret_cast<const cardano_uplc_constant_t**>(
    cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_constant_t*), 0U));
  items[0] = item;

  cardano_error_t error = cardano_uplc_constant_new_array(arena, element_type, items, 1U, &array);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(array->kind, CARDANO_UPLC_TYPE_ARRAY);
  EXPECT_EQ(array->as.list.element_type, element_type);
  EXPECT_EQ(array->as.list.count, 1U);
  EXPECT_EQ(array->as.list.items[0], item);

  cardano_uplc_arena_free(&arena);
  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_constant_new_array, allowsEmptyArrayAndRejectsBadArgs)
{
  cardano_uplc_arena_t*    arena        = new_arena();
  cardano_uplc_type_t*     element_type = nullptr;
  cardano_uplc_constant_t* array        = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BOOL, nullptr, nullptr, &element_type), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_constant_new_array(arena, element_type, nullptr, 0U, &array), CARDANO_SUCCESS);
  EXPECT_EQ(array->as.list.count, 0U);

  EXPECT_EQ(cardano_uplc_constant_new_array(arena, element_type, nullptr, 2U, &array), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_uplc_constant_new_array(nullptr, element_type, nullptr, 0U, &array), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_array(arena, nullptr, nullptr, 0U, &array), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_array(arena, element_type, nullptr, 0U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_type_new, buildsAnArrayType)
{
  cardano_uplc_arena_t* arena   = new_arena();
  cardano_uplc_type_t*  int_ty  = nullptr;
  cardano_uplc_type_t*  arr_ty  = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &int_ty), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_ARRAY, int_ty, nullptr, &arr_ty), CARDANO_SUCCESS);
  EXPECT_EQ(arr_ty->kind, CARDANO_UPLC_TYPE_ARRAY);
  EXPECT_EQ(arr_ty->fst, int_ty);

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_ARRAY, nullptr, nullptr, &arr_ty), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_ARRAY, int_ty, int_ty, &arr_ty), CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_list, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*    prereq       = new_arena();
  cardano_uplc_arena_t*    arena        = new_arena();
  cardano_uplc_type_t*     element_type = nullptr;
  cardano_uplc_constant_t* list         = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(prereq, CARDANO_UPLC_TYPE_BOOL, nullptr, nullptr, &element_type), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_list(arena, element_type, nullptr, 0U, &list);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_constant_new_pair, carriesComponents)
{
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_uplc_constant_t* fst   = nullptr;
  cardano_uplc_constant_t* snd   = nullptr;
  cardano_uplc_constant_t* pair  = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_bool(arena, true, &fst), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &snd), CARDANO_SUCCESS);

  cardano_error_t error = cardano_uplc_constant_new_pair(arena, fst, snd, &pair);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(pair->kind, CARDANO_UPLC_TYPE_PAIR);
  EXPECT_EQ(pair->as.pair.fst, fst);
  EXPECT_EQ(pair->as.pair.snd, snd);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_pair, failsOnNullArgs)
{
  cardano_uplc_arena_t*    arena = new_arena();
  cardano_uplc_constant_t* c     = nullptr;
  cardano_uplc_constant_t* pair  = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &c), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_constant_new_pair(nullptr, c, c, &pair), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, nullptr, c, &pair), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, c, nullptr, &pair), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_constant_new_pair(arena, c, c, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_new_pair, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*    prereq = new_arena();
  cardano_uplc_arena_t*    arena  = new_arena();
  cardano_uplc_constant_t* c      = nullptr;
  cardano_uplc_constant_t* pair   = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(prereq, &c), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_constant_new_pair(arena, c, c, &pair);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
  cardano_uplc_arena_free(&prereq);
}

TEST(cardano_uplc_type_new, buildsScalarTypes)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_type_t*  type  = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_DATA, nullptr, nullptr, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type->kind, CARDANO_UPLC_TYPE_DATA);
  EXPECT_EQ(type->fst, nullptr);
  EXPECT_EQ(type->snd, nullptr);

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BLS_G1, nullptr, nullptr, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type->kind, CARDANO_UPLC_TYPE_BLS_G1);

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BLS_G2, nullptr, nullptr, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type->kind, CARDANO_UPLC_TYPE_BLS_G2);

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_BLS_ML_RESULT, nullptr, nullptr, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type->kind, CARDANO_UPLC_TYPE_BLS_ML_RESULT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_type_new, buildsListAndPairTypes)
{
  cardano_uplc_arena_t* arena   = new_arena();
  cardano_uplc_type_t*  element = nullptr;
  cardano_uplc_type_t*  list    = nullptr;
  cardano_uplc_type_t*  pair    = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &element), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_LIST, element, nullptr, &list), CARDANO_SUCCESS);
  EXPECT_EQ(list->kind, CARDANO_UPLC_TYPE_LIST);
  EXPECT_EQ(list->fst, element);
  EXPECT_EQ(list->snd, nullptr);

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, element, element, &pair), CARDANO_SUCCESS);
  EXPECT_EQ(pair->kind, CARDANO_UPLC_TYPE_PAIR);
  EXPECT_EQ(pair->fst, element);
  EXPECT_EQ(pair->snd, element);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_type_new, rejectsBadComponentShape)
{
  cardano_uplc_arena_t* arena   = new_arena();
  cardano_uplc_type_t*  element = nullptr;
  cardano_uplc_type_t*  type    = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &element), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_LIST, nullptr, nullptr, &type), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_LIST, element, element, &type), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, nullptr, element, &type), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_PAIR, element, nullptr, &type), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, element, nullptr, &type), CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_type_new, rejectsUnknownKind)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_type_t*  type  = nullptr;

  cardano_error_t error = cardano_uplc_type_new(arena, static_cast<cardano_uplc_type_kind_t>(999), nullptr, nullptr, &type);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_type_new, failsOnNullArgs)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_type_t*  type  = nullptr;

  EXPECT_EQ(cardano_uplc_type_new(nullptr, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &type), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_type_new, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t* arena = new_arena();
  cardano_uplc_type_t*  type  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &type);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program, holdsVersionAndTerm)
{
  cardano_uplc_arena_t*  arena = new_arena();
  cardano_uplc_term_t*   term  = nullptr;
  cardano_uplc_program_t program = { 0U, 0U, 0U, nullptr };

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  program.version_major = 1U;
  program.version_minor = 0U;
  program.version_patch = 0U;
  program.term          = term;

  EXPECT_EQ(program.version_major, 1U);
  EXPECT_EQ(program.term, term);
  EXPECT_EQ(program.term->kind, CARDANO_UPLC_TERM_ERROR);

  cardano_uplc_arena_free(&arena);
}

static cardano_uplc_program_t*
new_program_with(cardano_uplc_arena_t* arena, const cardano_uplc_term_t* term, uint32_t major, uint32_t minor, uint32_t patch)
{
  cardano_uplc_program_t* program =
    static_cast<cardano_uplc_program_t*>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_program_t), 0U));
  EXPECT_NE(program, nullptr);

  program->version_major = major;
  program->version_minor = minor;
  program->version_patch = patch;
  program->term          = term;

  return program;
}

TEST(cardano_uplc_program_apply_data, wrapsTheTermInApplyOfDataConstant)
{
  // Arrange
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    term    = nullptr;
  cardano_plutus_data_t*  param   = new_data(7);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t* program = new_program_with(arena, term, 1U, 2U, 3U);

  // Act
  cardano_error_t error = cardano_uplc_program_apply_data(arena, program, param, &applied);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(applied, nullptr);
  EXPECT_EQ(applied->version_major, 1U);
  EXPECT_EQ(applied->version_minor, 2U);
  EXPECT_EQ(applied->version_patch, 3U);

  ASSERT_NE(applied->term, nullptr);
  EXPECT_EQ(applied->term->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(applied->term->as.apply.function, term);

  const cardano_uplc_term_t* argument = applied->term->as.apply.argument;
  ASSERT_NE(argument, nullptr);
  EXPECT_EQ(argument->kind, CARDANO_UPLC_TERM_CONSTANT);
  ASSERT_NE(argument->as.constant, nullptr);
  EXPECT_EQ(argument->as.constant->kind, CARDANO_UPLC_TYPE_DATA);
  EXPECT_TRUE(arena_data_equals(argument->as.constant->as.data, param));

  // Cleanup
  cardano_plutus_data_unref(&param);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data, failsOnNullArgs)
{
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    term    = nullptr;
  cardano_plutus_data_t*  param   = new_data(1);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t* program        = new_program_with(arena, term, 1U, 0U, 0U);
  cardano_uplc_program_t  program_no_term = { 1U, 0U, 0U, nullptr };

  EXPECT_EQ(cardano_uplc_program_apply_data(nullptr, program, param, &applied), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_apply_data(arena, nullptr, param, &applied), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_apply_data(arena, &program_no_term, param, &applied), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_apply_data(arena, program, nullptr, &applied), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_apply_data(arena, program, param, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_plutus_data_unref(&param);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data, failsWhenTheProgramNodeCannotBeAllocated)
{
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    term    = nullptr;
  cardano_plutus_data_t*  param   = new_data(1);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t* program = new_program_with(arena, term, 1U, 0U, 0U);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_program_apply_data(arena, program, param, &applied);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_plutus_data_unref(&param);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data, propagatesAFailureFromTheInnerBuilders)
{
  cardano_uplc_arena_t*   arena   = nullptr;
  cardano_uplc_term_t*    term    = nullptr;
  cardano_plutus_data_t*  param   = new_data(1);
  cardano_uplc_program_t* applied = nullptr;

  size_t          ceiling = sizeof(cardano_uplc_program_t) + sizeof(cardano_uplc_constant_t) + (kArenaMaxAlign - 1U);
  cardano_error_t mk      = cardano_uplc_int_arena_new_with_ceiling(4096U, ceiling, &arena);
  EXPECT_EQ(mk, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t program = { 1U, 0U, 0U, term };

  cardano_error_t error = cardano_uplc_program_apply_data(arena, &program, param, &applied);

  EXPECT_NE(error, CARDANO_SUCCESS);

  cardano_plutus_data_unref(&param);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data_params, foldsParamsLeftToRight)
{
  // Arrange
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    term    = nullptr;
  cardano_plutus_data_t*  p0      = new_data(10);
  cardano_plutus_data_t*  p1      = new_data(20);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t* program  = new_program_with(arena, term, 1U, 0U, 0U);
  cardano_plutus_data_t*  params[] = { p0, p1 };

  // Act
  cardano_error_t error = cardano_uplc_program_apply_data_params(arena, program, params, 2U, &applied);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(applied, nullptr);
  ASSERT_NE(applied->term, nullptr);

  EXPECT_EQ(applied->term->kind, CARDANO_UPLC_TERM_APPLY);
  const cardano_uplc_term_t* outer_arg = applied->term->as.apply.argument;
  EXPECT_EQ(outer_arg->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_TRUE(arena_data_equals(outer_arg->as.constant->as.data, p1));

  const cardano_uplc_term_t* inner = applied->term->as.apply.function;
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(inner->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(inner->as.apply.function, term);
  const cardano_uplc_term_t* inner_arg = inner->as.apply.argument;
  EXPECT_EQ(inner_arg->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_TRUE(arena_data_equals(inner_arg->as.constant->as.data, p0));

  // Cleanup
  cardano_plutus_data_unref(&p0);
  cardano_plutus_data_unref(&p1);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data_params, zeroParamsCopiesTheProgram)
{
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    term    = nullptr;
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t* program = new_program_with(arena, term, 2U, 1U, 0U);

  cardano_error_t error = cardano_uplc_program_apply_data_params(arena, program, nullptr, 0U, &applied);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(applied, nullptr);
  EXPECT_EQ(applied->version_major, 2U);
  EXPECT_EQ(applied->version_minor, 1U);
  EXPECT_EQ(applied->term, term);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data_params, failsOnNullArgsAndElements)
{
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    term    = nullptr;
  cardano_plutus_data_t*  p0      = new_data(1);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t* program  = new_program_with(arena, term, 1U, 0U, 0U);
  cardano_plutus_data_t*  params[] = { p0, nullptr };

  EXPECT_EQ(cardano_uplc_program_apply_data_params(nullptr, program, params, 2U, &applied), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_apply_data_params(arena, nullptr, params, 2U, &applied), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_apply_data_params(arena, program, params, 2U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_program_apply_data_params(arena, program, nullptr, 2U, &applied), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_uplc_program_apply_data_params(arena, program, params, 2U, &applied), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_plutus_data_unref(&p0);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data_params, failsOnArenaAllocationFailure)
{
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    term    = nullptr;
  cardano_plutus_data_t*  p0      = new_data(1);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_error(arena, &term), CARDANO_SUCCESS);

  cardano_uplc_program_t* program  = new_program_with(arena, term, 1U, 0U, 0U);
  cardano_plutus_data_t*  params[] = { p0 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_uplc_program_apply_data_params(arena, program, params, 1U, &applied);

  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_plutus_data_unref(&p0);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data, appliedProgramBindsTheParamOnEvaluation)
{
  // Arrange
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    var     = nullptr;
  cardano_uplc_term_t*    lambda  = nullptr;
  cardano_plutus_data_t*  param   = new_data(123);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &lambda), CARDANO_SUCCESS);

  cardano_uplc_program_t* program = new_program_with(arena, lambda, 1U, 0U, 0U);

  EXPECT_EQ(cardano_uplc_program_apply_data(arena, program, param, &applied), CARDANO_SUCCESS);

  cardano_uplc_budget_t      budget = { 10000000000LL, 10000000000LL };
  cardano_uplc_eval_result_t result;
  result.status = CARDANO_UPLC_EVAL_ERROR_TERM;
  result.result = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_evaluate(arena, applied, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS);
  ASSERT_NE(result.result, nullptr);
  EXPECT_EQ(result.result->kind, CARDANO_UPLC_TERM_CONSTANT);
  ASSERT_NE(result.result->as.constant, nullptr);
  EXPECT_EQ(result.result->as.constant->kind, CARDANO_UPLC_TYPE_DATA);
  EXPECT_TRUE(arena_data_equals(result.result->as.constant->as.data, param));

  // Cleanup
  cardano_plutus_data_unref(&param);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_program_apply_data, roundTripsThroughFlatCodec)
{
  // Arrange
  cardano_uplc_arena_t*   arena   = new_arena();
  cardano_uplc_term_t*    var     = nullptr;
  cardano_uplc_term_t*    lambda  = nullptr;
  cardano_plutus_data_t*  param   = new_data(55);
  cardano_uplc_program_t* applied = nullptr;

  EXPECT_EQ(cardano_uplc_term_new_var(arena, 1U, &var), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, var, &lambda), CARDANO_SUCCESS);

  cardano_uplc_program_t* program = new_program_with(arena, lambda, 1U, 0U, 0U);

  EXPECT_EQ(cardano_uplc_program_apply_data(arena, program, param, &applied), CARDANO_SUCCESS);

  cardano_buffer_t* flat = nullptr;
  EXPECT_EQ(cardano_uplc_flat_encode_program(applied, &flat), CARDANO_SUCCESS);
  ASSERT_NE(flat, nullptr);

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(applied, &cbor), CARDANO_SUCCESS);
  ASSERT_NE(cbor, nullptr);

  // Act
  const cardano_uplc_program_t* decoded = nullptr;
  cardano_error_t               error =
    cardano_uplc_program_from_script_bytes(arena, cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &decoded);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(decoded, nullptr);
  EXPECT_EQ(decoded->version_major, 1U);
  ASSERT_NE(decoded->term, nullptr);
  EXPECT_EQ(decoded->term->kind, CARDANO_UPLC_TERM_APPLY);
  EXPECT_EQ(decoded->term->as.apply.function->kind, CARDANO_UPLC_TERM_LAMBDA);
  const cardano_uplc_term_t* arg = decoded->term->as.apply.argument;
  EXPECT_EQ(arg->kind, CARDANO_UPLC_TERM_CONSTANT);
  EXPECT_EQ(arg->as.constant->kind, CARDANO_UPLC_TYPE_DATA);
  EXPECT_TRUE(arena_data_equals(arg->as.constant->as.data, param));

  // Cleanup
  cardano_buffer_unref(&flat);
  cardano_buffer_unref(&cbor);
  cardano_plutus_data_unref(&param);
  cardano_uplc_arena_free(&arena);
}
