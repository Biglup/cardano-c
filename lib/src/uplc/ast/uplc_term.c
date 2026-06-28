/**
 * \file uplc_term.c
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

#include "uplc_term.h"

#include "../arena/uplc_arena.h"

#include <stddef.h>
#include <stdint.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Allocates a zero-initialized term from the arena.
 *
 * \param[in] arena The arena to allocate from.
 *
 * \return The new term, or NULL if the arena cannot serve the node.
 */
static cardano_uplc_term_t*
alloc_term(cardano_uplc_arena_t* arena)
{
  cardano_uplc_term_t* term = (cardano_uplc_term_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_term_t), 0U);

  return term;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_term_new_var(
  cardano_uplc_arena_t* arena,
  uint64_t              index,
  cardano_uplc_term_t** term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind         = CARDANO_UPLC_TERM_VAR;
  result->as.var_index = index;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_delay(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_term_t* body,
  cardano_uplc_term_t**      term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (body == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind     = CARDANO_UPLC_TERM_DELAY;
  result->as.unary = body;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_lambda(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_term_t* body,
  cardano_uplc_term_t**      term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (body == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind     = CARDANO_UPLC_TERM_LAMBDA;
  result->as.unary = body;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_apply(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_term_t* function,
  const cardano_uplc_term_t* argument,
  cardano_uplc_term_t**      term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (function == NULL) || (argument == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind              = CARDANO_UPLC_TERM_APPLY;
  result->as.apply.function = function;
  result->as.apply.argument = argument;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_constant(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* constant,
  cardano_uplc_term_t**          term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (constant == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind        = CARDANO_UPLC_TERM_CONSTANT;
  result->as.constant = constant;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_force(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_term_t* body,
  cardano_uplc_term_t**      term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (body == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind     = CARDANO_UPLC_TERM_FORCE;
  result->as.unary = body;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_error(
  cardano_uplc_arena_t* arena,
  cardano_uplc_term_t** term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind = CARDANO_UPLC_TERM_ERROR;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_builtin(
  cardano_uplc_arena_t*  arena,
  cardano_uplc_builtin_t builtin,
  cardano_uplc_term_t**  term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind       = CARDANO_UPLC_TERM_BUILTIN;
  result->as.builtin = builtin;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_constr(
  cardano_uplc_arena_t*             arena,
  uint64_t                          tag,
  const cardano_uplc_term_t* const* fields,
  size_t                            field_count,
  cardano_uplc_term_t**             term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((fields == NULL) && (field_count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                  = CARDANO_UPLC_TERM_CONSTR;
  result->as.constr.tag         = tag;
  result->as.constr.fields      = fields;
  result->as.constr.field_count = field_count;

  *term = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_term_new_case(
  cardano_uplc_arena_t*             arena,
  const cardano_uplc_term_t*        scrutinee,
  const cardano_uplc_term_t* const* branches,
  size_t                            branch_count,
  cardano_uplc_term_t**             term)
{
  cardano_uplc_term_t* result = NULL;

  if ((arena == NULL) || (scrutinee == NULL) || (term == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((branches == NULL) && (branch_count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = alloc_term(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                  = CARDANO_UPLC_TERM_CASE;
  result->as.cases.scrutinee    = scrutinee;
  result->as.cases.branches     = branches;
  result->as.cases.branch_count = branch_count;

  *term = result;

  return CARDANO_SUCCESS;
}
