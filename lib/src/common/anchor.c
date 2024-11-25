/**
 * \file anchor.c
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/common/anchor.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t ANCHOR_EMBEDDED_GROUP_SIZE = 2;
static const int64_t ANCHOR_MAX_URL_LENGTH      = 128;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano anchor.
 */
typedef struct cardano_anchor_t
{
    cardano_object_t base;
    byte_t           hash_bytes[32];
    char             hash_hex[65];
    char             url[129];
} cardano_anchor_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a anchor object.
 *
 * This function is responsible for properly deallocating a anchor object (`cardano_anchor_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the anchor object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_anchor_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the anchor
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_anchor_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_anchor_new(
  const char*                   url,
  const size_t                  url_size,
  const cardano_blake2b_hash_t* hash,
  cardano_anchor_t**            anchor)
{
  if (url == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((url_size == 0U) || (url_size > (size_t)ANCHOR_MAX_URL_LENGTH))
  {
    return CARDANO_ERROR_INVALID_URL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    *anchor = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *anchor = _cardano_malloc(sizeof(cardano_anchor_t));

  if (*anchor == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*anchor)->base.deallocator   = cardano_anchor_deallocate;
  (*anchor)->base.ref_count     = 1;
  (*anchor)->base.last_error[0] = '\0';

  CARDANO_UNUSED(memset((*anchor)->hash_bytes, 0, sizeof((*anchor)->hash_bytes)));
  CARDANO_UNUSED(memset((*anchor)->hash_hex, 0, sizeof((*anchor)->hash_hex)));
  CARDANO_UNUSED(memset((*anchor)->url, 0, sizeof((*anchor)->url)));

  const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash);

  if (hash_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    cardano_anchor_unref(anchor);
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, (*anchor)->hash_bytes, sizeof((*anchor)->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, (*anchor)->hash_hex, sizeof((*anchor)->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  assert(url_size <= (size_t)ANCHOR_MAX_URL_LENGTH);
  CARDANO_UNUSED(memset((*anchor)->url, 0, sizeof((*anchor)->url)));
  cardano_safe_memcpy((*anchor)->url, ANCHOR_MAX_URL_LENGTH, url, url_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_anchor_from_hash_hex(
  const char*        url,
  const size_t       url_size,
  const char*        hex,
  size_t             hex_size,
  cardano_anchor_t** anchor)
{
  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *anchor = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex_size != ((size_t)CARDANO_BLAKE2B_HASH_SIZE_256 * 2U))
  {
    *anchor = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_hex(hex, hex_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return read_hash_result;
  }

  cardano_error_t new_anchor_result = cardano_anchor_new(url, url_size, hash, anchor);

  cardano_blake2b_hash_unref(&hash);

  if (new_anchor_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return new_anchor_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_anchor_from_hash_bytes(
  const char*        url,
  const size_t       url_size,
  const byte_t*      data,
  size_t             data_size,
  cardano_anchor_t** anchor)
{
  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *anchor = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    *anchor = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(data, data_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return read_hash_result;
  }

  cardano_error_t new_anchor_result = cardano_anchor_new(url, url_size, hash, anchor);

  cardano_blake2b_hash_unref(&hash);

  if (new_anchor_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return new_anchor_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_anchor_from_cbor(cardano_cbor_reader_t* reader, cardano_anchor_t** anchor)
{
  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *anchor = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "anchor";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)ANCHOR_EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return expect_array_result;
  }

  char                  url[129]         = { 0 };
  const cardano_error_t read_uint_result = cardano_cbor_validate_text_string_of_max_size(
    validator_name,
    reader,
    url,
    (uint32_t)ANCHOR_MAX_URL_LENGTH);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return read_uint_result;
  }

  cardano_buffer_t*     byte_string             = NULL;
  const cardano_error_t read_byte_string_result = cardano_cbor_validate_byte_string_of_size(
    validator_name,
    reader,
    &byte_string,
    CARDANO_BLAKE2B_HASH_SIZE_256);

  if (read_byte_string_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return read_byte_string_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&byte_string);
    *anchor = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t new_anchor_result = cardano_anchor_from_hash_bytes(
    url,
    cardano_safe_strlen(url, ANCHOR_MAX_URL_LENGTH),
    cardano_buffer_get_data(byte_string),
    cardano_buffer_get_size(byte_string),
    anchor);

  cardano_buffer_unref(&byte_string);

  if (new_anchor_result != CARDANO_SUCCESS)
  {
    *anchor = NULL;
    return new_anchor_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_anchor_to_cbor(
  const cardano_anchor_t* anchor,
  cardano_cbor_writer_t*  writer)
{
  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, ANCHOR_EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_url_result = cardano_cbor_writer_write_textstring(writer, anchor->url, cardano_safe_strlen(anchor->url, sizeof(anchor->url)));

  if (write_url_result != CARDANO_SUCCESS)
  {
    return write_url_result;
  }

  cardano_error_t write_bytes_result = cardano_cbor_writer_write_bytestring(writer, anchor->hash_bytes, sizeof(anchor->hash_bytes));

  if (write_bytes_result != CARDANO_SUCCESS)
  {
    return write_bytes_result;
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_anchor_get_hash(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(anchor->hash_bytes, sizeof(anchor->hash_bytes), &hash);

  assert(read_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(read_hash_result);

  return hash;
}

size_t
cardano_anchor_get_hash_bytes_size(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return 0;
  }

  return sizeof(anchor->hash_bytes);
}

const byte_t*
cardano_anchor_get_hash_bytes(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return NULL;
  }

  return anchor->hash_bytes;
}

size_t
cardano_anchor_get_hash_hex_size(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return 0;
  }

  return cardano_safe_strlen(anchor->hash_hex, sizeof(anchor->hash_hex)) + 1U;
}

const char*
cardano_anchor_get_hash_hex(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return NULL;
  }

  return anchor->hash_hex;
}

size_t
cardano_anchor_get_url_size(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return 0;
  }

  return cardano_safe_strlen(anchor->url, sizeof(anchor->url)) + 1U;
}

const char*
cardano_anchor_get_url(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return NULL;
  }

  return anchor->url;
}

cardano_error_t
cardano_anchor_set_url(cardano_anchor_t* anchor, const char* url, const size_t url_size)
{
  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (url == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((url_size == 0U) || (url_size > (size_t)ANCHOR_MAX_URL_LENGTH))
  {
    return CARDANO_ERROR_INVALID_URL;
  }

  assert(url_size < (size_t)ANCHOR_MAX_URL_LENGTH);
  CARDANO_UNUSED(memset(anchor->url, 0, sizeof(anchor->url)));
  cardano_safe_memcpy(anchor->url, ANCHOR_MAX_URL_LENGTH, url, url_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_anchor_set_hash(cardano_anchor_t* anchor, const cardano_blake2b_hash_t* hash)
{
  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash);

  if (hash_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, &anchor->hash_bytes[0], sizeof(anchor->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, anchor->hash_hex, sizeof(anchor->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

void
cardano_anchor_unref(cardano_anchor_t** anchor)
{
  if ((anchor == NULL) || (*anchor == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*anchor)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *anchor = NULL;
    return;
  }
}

void
cardano_anchor_ref(cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return;
  }

  cardano_object_ref(&anchor->base);
}

size_t
cardano_anchor_refcount(const cardano_anchor_t* anchor)
{
  if (anchor == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&anchor->base);
}

void
cardano_anchor_set_last_error(cardano_anchor_t* anchor, const char* message)
{
  cardano_object_set_last_error(&anchor->base, message);
}

const char*
cardano_anchor_get_last_error(const cardano_anchor_t* anchor)
{
  return cardano_object_get_last_error(&anchor->base);
}