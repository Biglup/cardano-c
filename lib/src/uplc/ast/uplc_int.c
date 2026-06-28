/**
 * \file uplc_int.c
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

#include "uplc_int.h"

#include "../arena/uplc_arena.h"
#include "../data/uplc_data.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Copies a byte span into the arena and writes the inline descriptor.
 *
 * Bump-allocates \p size bytes from \p arena and copies \p data into them, then
 * stores the resulting pointer and length in \p out_data and \p out_size. An empty
 * span allocates nothing and yields a NULL pointer with a zero size, the canonical
 * empty byte string.
 *
 * \param[in] arena The arena the copy is allocated from.
 * \param[in] data The bytes to copy, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] out_data On success, the arena pointer (NULL for an empty span).
 * \param[out] out_size On success, the byte count.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         copy.
 */
static cardano_error_t
copy_bytes(
  cardano_uplc_arena_t* arena,
  const byte_t*         data,
  size_t                size,
  const byte_t**        out_data,
  size_t*               out_size)
{
  if (size == 0U)
  {
    *out_data = NULL;
    *out_size = 0U;

    return CARDANO_SUCCESS;
  }

  {
    byte_t* copy = (byte_t*)cardano_uplc_arena_alloc(arena, size, 1U);

    if (copy == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    memcpy(copy, data, size);

    *out_data = copy;
    *out_size = size;

    return CARDANO_SUCCESS;
  }
}

/* DEFINITIONS ***************************************************************/

cardano_uplc_constant_t*
cardano_uplc_int_alloc_constant(cardano_uplc_arena_t* arena)
{
  cardano_uplc_constant_t* constant = (cardano_uplc_constant_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_constant_t), 0U);

  return constant;
}

void
cardano_uplc_int_unref_bigint(void* object)
{
  cardano_bigint_t* bigint = (cardano_bigint_t*)object;

  cardano_bigint_unref(&bigint);
}

bool
cardano_uplc_int_bigint_fits_int64(const cardano_bigint_t* value, int64_t* out)
{
  size_t bits = 0U;

  if ((value == NULL) || (out == NULL))
  {
    return false;
  }

  bits = cardano_bigint_bit_length(value);

  if (bits <= 63U)
  {
    *out = cardano_bigint_to_int(value);

    return true;
  }

  if ((bits == 64U) && (cardano_bigint_signum(value) < 0))
  {
    const int64_t candidate = cardano_bigint_to_int(value);

    if (candidate == INT64_MIN)
    {
      *out = candidate;

      return true;
    }
  }

  return false;
}

bool
cardano_uplc_constant_int_is_small(const cardano_uplc_constant_t* constant)
{
  return constant->as.integer.is_small;
}

int64_t
cardano_uplc_constant_int_small(const cardano_uplc_constant_t* constant)
{
  return constant->as.integer.small;
}

cardano_error_t
cardano_uplc_constant_int_materialize(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* constant,
  const cardano_bigint_t**       out)
{
  cardano_uplc_constant_t* mutable_constant = (cardano_uplc_constant_t*)((void*)constant);
  cardano_bigint_t*        built            = NULL;
  cardano_error_t          error            = CARDANO_SUCCESS;

  if (constant->as.integer.big != NULL)
  {
    *out = constant->as.integer.big;

    return CARDANO_SUCCESS;
  }

  error = cardano_bigint_from_int(constant->as.integer.small, &built);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_arena_register_unref(arena, built, &cardano_uplc_int_unref_bigint);

  if (error != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&built);

    return error;
  }

  mutable_constant->as.integer.big = built;
  *out                             = built;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_int_constant_new_byte_string_copy(
  cardano_uplc_arena_t*     arena,
  const byte_t*             data,
  size_t                    size,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result = NULL;
  cardano_error_t          error  = CARDANO_SUCCESS;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((data == NULL) && (size != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  error = copy_bytes(arena, data, size, &result->as.bytes.data, &result->as.bytes.size);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  result->kind = CARDANO_UPLC_TYPE_BYTE_STRING;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_int_constant_new_byte_string_take(
  cardano_uplc_arena_t*     arena,
  const byte_t*             data,
  size_t                    size,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((data == NULL) && (size != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind          = CARDANO_UPLC_TYPE_BYTE_STRING;
  result->as.bytes.data = (size == 0U) ? NULL : data;
  result->as.bytes.size = size;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_int_constant_new_string_copy(
  cardano_uplc_arena_t*     arena,
  const byte_t*             data,
  size_t                    size,
  cardano_uplc_constant_t** constant)
{
  cardano_uplc_constant_t* result = NULL;
  cardano_error_t          error  = CARDANO_SUCCESS;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((data == NULL) && (size != 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  error = copy_bytes(arena, data, size, &result->as.string.data, &result->as.string.size);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  result->kind = CARDANO_UPLC_TYPE_STRING;

  *constant = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_int_constant_new_data_node(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_data_t* data,
  cardano_uplc_constant_t**  constant)
{
  cardano_uplc_constant_t* result = NULL;

  if ((arena == NULL) || (data == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_int_alloc_constant(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind    = CARDANO_UPLC_TYPE_DATA;
  result->as.data = data;

  *constant = result;

  return CARDANO_SUCCESS;
}
