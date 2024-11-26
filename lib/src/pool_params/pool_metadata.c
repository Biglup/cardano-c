/**
 * \file pool_metadata.c
 *
 * \author angel.castillo
 * \date   jun 26, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license") {}
 * you may not use this file except in compliance with the license.
 * you may obtain a copy of the license at
 *
 *     www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */

/* INCLUDES ******************************************************************/

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/object.h>
#include <cardano/pool_params/pool_metadata.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an pool metadata.
 *
 * Each instance of `cardano_pool_metadata_t` holds a single pool metadata.
 */
typedef struct cardano_pool_metadata_t
{
    cardano_object_t        base;
    char                    url[65];
    cardano_blake2b_hash_t* hash;
} cardano_pool_metadata_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a pool_metadata object.
 *
 * This function is responsible for properly deallocating a pool_metadata object (`cardano_pool_metadata_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the pool_metadata object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_pool_metadata_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the pool_metadata
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_pool_metadata_deallocate(void* object)
{
  assert(object != NULL);

  cardano_pool_metadata_t* pool_metadata = (cardano_pool_metadata_t*)object;

  cardano_blake2b_hash_unref(&pool_metadata->hash);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_pool_metadata_new(
  const char*               url,
  const size_t              url_length,
  cardano_blake2b_hash_t*   hash,
  cardano_pool_metadata_t** pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (url == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (url_length > 64U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *pool_metadata = _cardano_malloc(sizeof(cardano_pool_metadata_t));

  if (*pool_metadata == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*pool_metadata)->base.deallocator   = cardano_pool_metadata_deallocate;
  (*pool_metadata)->base.ref_count     = 1;
  (*pool_metadata)->base.last_error[0] = '\0';

  CARDANO_UNUSED(memset((*pool_metadata)->url, 0, 65));

  cardano_safe_memcpy((*pool_metadata)->url, 65, url, url_length);

  const size_t url_size           = cardano_safe_strlen((*pool_metadata)->url, 64);
  (*pool_metadata)->url[url_size] = '\0';

  cardano_blake2b_hash_ref(hash);
  (*pool_metadata)->hash = hash;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_metadata_from_hash_hex(
  const char*               url,
  size_t                    url_size,
  const char*               hash,
  size_t                    hash_size,
  cardano_pool_metadata_t** pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (url == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (url_size > 64U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash_size != 64U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_blake2b_hash_t* blake2b_hash = NULL;

  const cardano_error_t hash_result = cardano_blake2b_hash_from_hex(hash, hash_size, &blake2b_hash);

  if (hash_result != CARDANO_SUCCESS)
  {
    *pool_metadata = NULL;
    return hash_result;
  }

  cardano_error_t new_metadata_result = cardano_pool_metadata_new(url, url_size, blake2b_hash, pool_metadata);

  cardano_blake2b_hash_unref(&blake2b_hash);

  return new_metadata_result;
}

cardano_error_t
cardano_pool_metadata_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_metadata_t** pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *pool_metadata = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "pool_metadata";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *pool_metadata = NULL;
    return expect_array_result;
  }

  cardano_buffer_t*     url                = NULL;
  const cardano_error_t read_string_result = cardano_cbor_reader_read_textstring(reader, &url);

  if (read_string_result != CARDANO_SUCCESS)
  {
    *pool_metadata = NULL;
    return read_string_result;
  }

  cardano_blake2b_hash_t* hash = NULL;

  const cardano_error_t read_hash_result = cardano_blake2b_hash_from_cbor(reader, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&url);
    *pool_metadata = NULL;
    return read_hash_result;
  }

  cardano_error_t new_result = cardano_pool_metadata_new(
    (char*)((void*)cardano_buffer_get_data(url)),
    cardano_buffer_get_size(url),
    hash,
    pool_metadata);

  cardano_buffer_unref(&url);
  cardano_blake2b_hash_unref(&hash);

  if (new_result != CARDANO_SUCCESS)
  {
    *pool_metadata = NULL;
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_pool_metadata_to_cbor(const cardano_pool_metadata_t* pool_metadata, cardano_cbor_writer_t* writer)
{
  if (pool_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_string_result = cardano_cbor_writer_write_textstring(
    writer,
    pool_metadata->url,
    cardano_safe_strlen(pool_metadata->url, 64));

  if (write_string_result != CARDANO_SUCCESS)
  {
    return write_string_result;
  }

  return cardano_blake2b_hash_to_cbor(pool_metadata->hash, writer);
}

size_t
cardano_pool_metadata_get_url_size(
  const cardano_pool_metadata_t* pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return 0;
  }

  return cardano_safe_strlen(pool_metadata->url, 64);
}

const char*
cardano_pool_metadata_get_url(
  const cardano_pool_metadata_t* pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return NULL;
  }

  return pool_metadata->url;
}

cardano_error_t
cardano_pool_metadata_set_url(
  const char*              url,
  size_t                   url_size,
  cardano_pool_metadata_t* pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (url == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (url_size > 64U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_safe_memcpy(pool_metadata->url, 65, url, url_size);

  const size_t url_length        = cardano_safe_strlen(pool_metadata->url, 64);
  pool_metadata->url[url_length] = '\0';

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_metadata_get_hash(
  cardano_pool_metadata_t* pool_metadata,
  cardano_blake2b_hash_t** hash)
{
  if (pool_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(pool_metadata->hash);
  *hash = pool_metadata->hash;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_metadata_set_hash(
  cardano_pool_metadata_t* pool_metadata,
  cardano_blake2b_hash_t*  hash)
{
  if (pool_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(hash);
  cardano_blake2b_hash_unref(&pool_metadata->hash);
  pool_metadata->hash = hash;

  return CARDANO_SUCCESS;
}

void
cardano_pool_metadata_unref(cardano_pool_metadata_t** pool_metadata)
{
  if ((pool_metadata == NULL) || (*pool_metadata == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*pool_metadata)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *pool_metadata = NULL;
    return;
  }
}

void
cardano_pool_metadata_ref(cardano_pool_metadata_t* pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return;
  }

  cardano_object_ref(&pool_metadata->base);
}

size_t
cardano_pool_metadata_refcount(const cardano_pool_metadata_t* pool_metadata)
{
  if (pool_metadata == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&pool_metadata->base);
}

void
cardano_pool_metadata_set_last_error(cardano_pool_metadata_t* pool_metadata, const char* message)
{
  cardano_object_set_last_error(&pool_metadata->base, message);
}

const char*
cardano_pool_metadata_get_last_error(const cardano_pool_metadata_t* pool_metadata)
{
  return cardano_object_get_last_error(&pool_metadata->base);
}