/**
 * \file blake2b_hash.c
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
 *
 * Copyright 2024 Biglup Labs
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

#include <cardano/crypto/blake2b_hash.h>

#include <cardano/allocators.h>
#include <cardano/buffer.h>
#include <cardano/export.h>
#include <cardano/object.h>

#include <assert.h>
#include <sodium.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a BLAKE2b cryptographic hash.
 */
typedef struct cardano_blake2b_hash_t
{
    cardano_object_t  base;
    cardano_buffer_t* buffer;
} cardano_blake2b_hash_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a Blake2b hash object.
 *
 * This function is responsible for properly deallocating a Blake2b hash object (`cardano_blake2b_hash_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the Blake2b hash object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_blake2b_hash_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the Blake2b hash
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_blake2b_hash_deallocate(void* object)
{
  assert(object != NULL);

  cardano_blake2b_hash_t* hash = (cardano_blake2b_hash_t*)object;

  if (hash->buffer != NULL)
  {
    cardano_buffer_unref(&hash->buffer);
  }

  _cardano_free(hash);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_blake2b_compute_hash(
  const byte_t*            data,
  const size_t             data_length,
  const size_t             hash_length,
  cardano_blake2b_hash_t** hash)
{
  if (hash == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *hash = NULL;

    return CARDANO_POINTER_IS_NULL;
  }

  if (data_length == 0U)
  {
    *hash = NULL;

    return CARDANO_OUT_OF_BOUNDS_MEMORY_READ;
  }

  if (hash_length == 0U)
  {
    *hash = NULL;

    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* obj = (cardano_blake2b_hash_t*)_cardano_malloc(sizeof(cardano_blake2b_hash_t));

  if (obj == NULL)
  {
    *hash = NULL;

    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  obj->base.ref_count     = 1;
  obj->base.deallocator   = cardano_blake2b_hash_deallocate;
  obj->base.last_error[0] = '\0';
  obj->buffer             = cardano_buffer_new(hash_length);

  if (obj->buffer == NULL)
  {
    _cardano_free(obj);
    *hash = NULL;

    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  int init_result = sodium_init();

  if (init_result == -1)
  {
    /* LCOV_EXCL_START */
    cardano_blake2b_hash_deallocate(obj);
    *hash = NULL;

    return CARDANO_ERROR_GENERIC;
    /* LCOV_EXCL_STOP */
  }

  int hashing_result = crypto_generichash(
    cardano_buffer_get_data(obj->buffer),
    hash_length,
    data,
    data_length,
    NULL,
    0U);

  if (hashing_result == -1)
  {
    /* LCOV_EXCL_START */
    cardano_blake2b_hash_deallocate(obj);
    *hash = NULL;

    return CARDANO_ERROR_GENERIC;
    /* LCOV_EXCL_STOP */
  }

  cardano_error_t set_size_result = cardano_buffer_set_size(obj->buffer, hash_length);

  assert(set_size_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(set_size_result);

  *hash = obj;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_blake2b_hash_from_bytes(
  const byte_t*            data,
  const size_t             data_length,
  cardano_blake2b_hash_t** hash)
{
  if (hash == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *hash = NULL;

    return CARDANO_POINTER_IS_NULL;
  }

  if (data_length == 0U)
  {
    *hash = NULL;

    return CARDANO_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_blake2b_hash_t* blake2b_hash = (cardano_blake2b_hash_t*)_cardano_malloc(sizeof(cardano_blake2b_hash_t));

  if (blake2b_hash == NULL)
  {
    *hash = NULL;

    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  blake2b_hash->base.ref_count     = 1;
  blake2b_hash->base.deallocator   = cardano_blake2b_hash_deallocate;
  blake2b_hash->base.last_error[0] = '\0';
  blake2b_hash->buffer             = cardano_buffer_new_from(data, data_length);

  if (blake2b_hash->buffer == NULL)
  {
    *hash = NULL;
    _cardano_free(blake2b_hash);

    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  *hash = blake2b_hash;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_blake2b_hash_from_hex(
  const char*              hex,
  const size_t             hex_length,
  cardano_blake2b_hash_t** hash)
{
  if (hex == NULL)
  {
    *hash = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (hex_length == 0U)
  {
    *hash = NULL;
    return CARDANO_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_blake2b_hash_t* blake2b_hash = (cardano_blake2b_hash_t*)_cardano_malloc(sizeof(cardano_blake2b_hash_t));

  if (blake2b_hash == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  blake2b_hash->base.ref_count     = 1;
  blake2b_hash->base.deallocator   = cardano_blake2b_hash_deallocate;
  blake2b_hash->base.last_error[0] = '\0';
  blake2b_hash->buffer             = cardano_buffer_from_hex(hex, hex_length);

  if (blake2b_hash->buffer == NULL)
  {
    _cardano_free(blake2b_hash);
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  *hash = blake2b_hash;

  return CARDANO_SUCCESS;
}

void
cardano_blake2b_hash_unref(cardano_blake2b_hash_t** blake2b_hash)
{
  if ((blake2b_hash == NULL) || (*blake2b_hash == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*blake2b_hash)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *blake2b_hash = NULL;
    return;
  }
}

void
cardano_blake2b_hash_ref(cardano_blake2b_hash_t* blake2b_hash)
{
  if (blake2b_hash == NULL)
  {
    return;
  }

  cardano_object_ref(&blake2b_hash->base);
}

size_t
cardano_blake2b_hash_refcount(const cardano_blake2b_hash_t* blake2b_hash)
{
  if (blake2b_hash == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&blake2b_hash->base);
}

cardano_blake2b_hash_t*
cardano_blake2b_hash_move(cardano_blake2b_hash_t* blake2b_hash)
{
  if (blake2b_hash == NULL)
  {
    return NULL;
  }

  cardano_object_t* object = cardano_object_move(&blake2b_hash->base);

  CARDANO_UNUSED(object);

  return blake2b_hash;
}

const byte_t*
cardano_blake2b_hash_get_data(const cardano_blake2b_hash_t* blake2b_hash)
{
  return cardano_buffer_get_data(blake2b_hash->buffer);
}

size_t
cardano_blake2b_hash_get_bytes_size(const cardano_blake2b_hash_t* blake2b_hash)
{
  return cardano_buffer_get_size(blake2b_hash->buffer);
}

cardano_error_t
cardano_blake2b_hash_to_bytes(
  const cardano_blake2b_hash_t* blake2b_hash,
  byte_t*                       hash,
  const size_t                  hash_length)
{
  if (blake2b_hash == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  return cardano_buffer_copy_bytes(blake2b_hash->buffer, hash, hash_length);
}

size_t
cardano_blake2b_hash_get_hex_size(const cardano_blake2b_hash_t* blake2b_hash)
{
  return cardano_buffer_get_hex_size(blake2b_hash->buffer);
}

cardano_error_t
cardano_blake2b_hash_to_hex(
  const cardano_blake2b_hash_t* blake2b_hash,
  char*                         hex_hash,
  const size_t                  hash_len)
{
  return cardano_buffer_to_hex(blake2b_hash->buffer, hex_hash, hash_len);
}