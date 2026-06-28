/**
 * \file uplc_frame.c
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

#include "uplc_frame.h"

#include "../arena/uplc_arena.h"

#include <stddef.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Allocates a zero-initialized frame from the arena.
 *
 * \param[in] arena The arena to allocate from.
 *
 * \return The new frame, or NULL if the arena cannot serve the node.
 */
static cardano_uplc_frame_t*
alloc_frame(cardano_uplc_arena_t* arena)
{
  return (cardano_uplc_frame_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_frame_t), 0U);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_frame_new_no_frame(
  cardano_uplc_arena_t*  arena,
  cardano_uplc_frame_t** out)
{
  cardano_uplc_frame_t* result = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_frame(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind = CARDANO_UPLC_FRAME_NO_FRAME;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_frame_new_await_arg(
  cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* value,
  const cardano_uplc_frame_t* ctx,
  cardano_uplc_frame_t**      out)
{
  cardano_uplc_frame_t* result = NULL;

  if ((arena == NULL) || (value == NULL) || (ctx == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_frame(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind               = CARDANO_UPLC_FRAME_AWAIT_ARG;
  result->as.await_arg.value = value;
  result->as.await_arg.ctx   = ctx;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_frame_new_await_fun_term(
  cardano_uplc_arena_t*       arena,
  const cardano_uplc_env_t*   env,
  const cardano_uplc_term_t*  term,
  const cardano_uplc_frame_t* ctx,
  cardano_uplc_frame_t**      out)
{
  cardano_uplc_frame_t* result = NULL;

  if ((arena == NULL) || (term == NULL) || (ctx == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_frame(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                   = CARDANO_UPLC_FRAME_AWAIT_FUN_TERM;
  result->as.await_fun_term.env  = env;
  result->as.await_fun_term.term = term;
  result->as.await_fun_term.ctx  = ctx;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_frame_new_await_fun_value(
  cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* value,
  const cardano_uplc_frame_t* ctx,
  cardano_uplc_frame_t**      out)
{
  cardano_uplc_frame_t* result = NULL;

  if ((arena == NULL) || (value == NULL) || (ctx == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_frame(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                     = CARDANO_UPLC_FRAME_AWAIT_FUN_VALUE;
  result->as.await_fun_value.value = value;
  result->as.await_fun_value.ctx   = ctx;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_frame_new_force(
  cardano_uplc_arena_t*       arena,
  const cardano_uplc_frame_t* ctx,
  cardano_uplc_frame_t**      out)
{
  cardano_uplc_frame_t* result = NULL;

  if ((arena == NULL) || (ctx == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_frame(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind         = CARDANO_UPLC_FRAME_FORCE;
  result->as.force.ctx = ctx;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_frame_new_constr(
  cardano_uplc_arena_t*              arena,
  const cardano_uplc_env_t*          env,
  uint64_t                           tag,
  const cardano_uplc_term_t* const*  fields,
  size_t                             field_count,
  const cardano_uplc_value_t* const* resolved,
  size_t                             resolved_count,
  const cardano_uplc_frame_t*        ctx,
  cardano_uplc_frame_t**             out)
{
  cardano_uplc_frame_t* result = NULL;

  if ((arena == NULL) || (ctx == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (((fields == NULL) && (field_count != 0U)) || ((resolved == NULL) && (resolved_count != 0U)))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = alloc_frame(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                     = CARDANO_UPLC_FRAME_CONSTR;
  result->as.constr.env            = env;
  result->as.constr.tag            = tag;
  result->as.constr.fields         = fields;
  result->as.constr.field_count    = field_count;
  result->as.constr.resolved       = resolved;
  result->as.constr.resolved_count = resolved_count;
  result->as.constr.ctx            = ctx;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_frame_new_cases(
  cardano_uplc_arena_t*             arena,
  const cardano_uplc_env_t*         env,
  const cardano_uplc_term_t* const* branches,
  size_t                            branch_count,
  const cardano_uplc_frame_t*       ctx,
  cardano_uplc_frame_t**            out)
{
  cardano_uplc_frame_t* result = NULL;

  if ((arena == NULL) || (ctx == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((branches == NULL) && (branch_count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = alloc_frame(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                  = CARDANO_UPLC_FRAME_CASES;
  result->as.cases.env          = env;
  result->as.cases.branches     = branches;
  result->as.cases.branch_count = branch_count;
  result->as.cases.ctx          = ctx;

  *out = result;

  return CARDANO_SUCCESS;
}
