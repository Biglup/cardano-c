/**
 * \file uplc_constant.c
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

#include "uplc_constant.h"
#include "uplc_int.h"

#include "../arena/uplc_arena.h"
#include "../data/uplc_data.h"

#include <cardano/buffer.h>

#include <stddef.h>
#include <stdint.h>

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_constant_new_integer(
  cardano_uplc_arena_t*     arena,
  cardano_bigint_t*         value,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result       = NULL;
  cardano_error_t          register_res = CARDANO_SUCCESS;

  if ((arena == NULL) || (value == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_bigint_ref(value);

  register_res = cardano_uplc_arena_register_unref(arena, value, &cardano_uplc_int_unref_bigint);

  if (register_res != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&value);

    return register_res;
  }

  result->kind                = CARDANO_UPLC_TYPE_INTEGER;
  result->as.integer.is_small = false;
  result->as.integer.small    = 0;
  result->as.integer.big      = value;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_integer_small(
  cardano_uplc_arena_t*     arena,
  int64_t                   value,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                = CARDANO_UPLC_TYPE_INTEGER;
  result->as.integer.is_small = true;
  result->as.integer.small    = value;
  result->as.integer.big      = NULL;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_byte_string(
  cardano_uplc_arena_t*     arena,
  cardano_buffer_t*         bytes,
  cardano_uplc_constant_t** constant)
{
  if ((arena == NULL) || (bytes == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_uplc_int_constant_new_byte_string_copy(
    arena,
    cardano_buffer_get_data(bytes),
    cardano_buffer_get_size(bytes),
    constant);
}

cardano_error_t
cardano_uplc_constant_new_string(
  cardano_uplc_arena_t*     arena,
  cardano_buffer_t*         string,
  cardano_uplc_constant_t** constant)
{
  if ((arena == NULL) || (string == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_uplc_int_constant_new_string_copy(
    arena,
    cardano_buffer_get_data(string),
    cardano_buffer_get_size(string),
    constant);
}

cardano_error_t
cardano_uplc_constant_new_bool(
  cardano_uplc_arena_t*     arena,
  bool                      value,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind       = CARDANO_UPLC_TYPE_BOOL;
  result->as.boolean = value;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_unit(
  cardano_uplc_arena_t*     arena,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind = CARDANO_UPLC_TYPE_UNIT;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_data(
  cardano_uplc_arena_t*     arena,
  cardano_plutus_data_t*    data,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_data_t* node   = NULL;
  cardano_error_t      result = CARDANO_SUCCESS;

  if ((arena == NULL) || (data == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_data_from_plutus_data(arena, data, &node);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_uplc_int_constant_new_data_node(arena, node, constant);
}

cardano_error_t
cardano_uplc_constant_new_bls(
  cardano_uplc_arena_t*     arena,
  cardano_uplc_type_kind_t  kind,
  const void*               data,
  size_t                    size,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result = NULL;
  void*                    blob   = NULL;
  const byte_t*            src    = (const byte_t*)data;
  byte_t*                  dst    = NULL;
  size_t                   i      = 0U;

  if ((arena == NULL) || (data == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (((kind != CARDANO_UPLC_TYPE_BLS_G1) && (kind != CARDANO_UPLC_TYPE_BLS_G2) && (kind != CARDANO_UPLC_TYPE_BLS_ML_RESULT)) || (size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  blob = cardano_uplc_arena_alloc(arena, size, 0U);

  if (blob == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  dst = (byte_t*)blob;

  for (i = 0U; i < size; ++i)
  {
    dst[i] = src[i];
  }

  result->kind   = kind;
  result->as.bls = blob;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_list(
  cardano_uplc_arena_t*                 arena,
  const cardano_uplc_type_t*            element_type,
  const cardano_uplc_constant_t* const* items,
  size_t                                count,
  cardano_uplc_constant_t**             constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (element_type == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((items == NULL) && (count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                 = CARDANO_UPLC_TYPE_LIST;
  result->as.list.element_type = element_type;
  result->as.list.items        = items;
  result->as.list.count        = count;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_array(
  cardano_uplc_arena_t*                 arena,
  const cardano_uplc_type_t*            element_type,
  const cardano_uplc_constant_t* const* items,
  size_t                                count,
  cardano_uplc_constant_t**             constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (element_type == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((items == NULL) && (count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                 = CARDANO_UPLC_TYPE_ARRAY;
  result->as.list.element_type = element_type;
  result->as.list.items        = items;
  result->as.list.count        = count;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_value(
  cardano_uplc_arena_t*                 arena,
  const cardano_uplc_type_t*            element_type,
  const cardano_uplc_constant_t* const* items,
  size_t                                count,
  cardano_uplc_constant_t**             constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (element_type == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((items == NULL) && (count != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind                 = CARDANO_UPLC_TYPE_VALUE;
  result->as.list.element_type = element_type;
  result->as.list.items        = items;
  result->as.list.count        = count;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_constant_new_pair(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* fst,
  const cardano_uplc_constant_t* snd,
  cardano_uplc_constant_t**      constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (fst == NULL) || (snd == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind        = CARDANO_UPLC_TYPE_PAIR;
  result->as.pair.fst = fst;
  result->as.pair.snd = snd;

  *constant = result;

  return CARDANO_SUCCESS;
}
