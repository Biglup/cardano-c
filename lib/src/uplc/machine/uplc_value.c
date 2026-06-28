/**
 * \file uplc_value.c
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

#include "uplc_value.h"

#include "../arena/uplc_arena.h"

#include <stddef.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Allocates a zero-initialized value from the arena.
 *
 * \param[in] arena The arena to allocate from.
 *
 * \return The new value, or NULL if the arena cannot serve the node.
 */
static cardano_uplc_value_t*
alloc_value(cardano_uplc_arena_t* arena)
{
  cardano_uplc_value_t* value = (cardano_uplc_value_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_value_t), 0U);

  if (value != NULL)
  {
    value->ex_mem = CARDANO_UPLC_VALUE_EX_MEM_UNCOMPUTED;
  }

  return value;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_value_new_constant(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* constant,
  cardano_uplc_value_t**         out)
{
  cardano_uplc_value_t* result = NULL;

  if ((arena == NULL) || (constant == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_value(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind        = CARDANO_UPLC_VALUE_CONSTANT;
  result->as.constant = constant;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_value_new_delay(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_term_t* body,
  const cardano_uplc_env_t*  env,
  cardano_uplc_value_t**     out)
{
  cardano_uplc_value_t* result = NULL;

  if ((arena == NULL) || (body == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_value(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind          = CARDANO_UPLC_VALUE_DELAY;
  result->as.delay.body = body;
  result->as.delay.env  = env;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_value_new_lambda(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_term_t* body,
  const cardano_uplc_env_t*  env,
  cardano_uplc_value_t**     out)
{
  cardano_uplc_value_t* result = NULL;

  if ((arena == NULL) || (body == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_value(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind           = CARDANO_UPLC_VALUE_LAMBDA;
  result->as.lambda.body = body;
  result->as.lambda.env  = env;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_value_new_builtin(
  cardano_uplc_arena_t*              arena,
  cardano_uplc_builtin_t             func,
  size_t                             forces,
  const cardano_uplc_value_t* const* args,
  size_t                             arg_count,
  cardano_uplc_value_t**             out)
{
  cardano_uplc_value_t* result = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((args == NULL) && (arg_count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = alloc_value(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                 = CARDANO_UPLC_VALUE_BUILTIN;
  result->as.builtin.func      = func;
  result->as.builtin.forces    = forces;
  result->as.builtin.args      = args;
  result->as.builtin.arg_count = arg_count;

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_value_new_constr(
  cardano_uplc_arena_t*              arena,
  uint64_t                           tag,
  const cardano_uplc_value_t* const* fields,
  size_t                             field_count,
  cardano_uplc_value_t**             out)
{
  cardano_uplc_value_t* result = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((fields == NULL) && (field_count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = alloc_value(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                  = CARDANO_UPLC_VALUE_CONSTR;
  result->as.constr.tag         = tag;
  result->as.constr.fields      = fields;
  result->as.constr.field_count = field_count;

  *out = result;

  return CARDANO_SUCCESS;
}
