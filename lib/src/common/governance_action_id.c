/**
 * \file governance_action_id.c
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

#include <cardano/common/governance_action_id.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t GOVERNANCE_ACTION_ID_ARRAY_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano governance_action_id.
 */
typedef struct cardano_governance_action_id_t
{
    cardano_object_t base;
    uint64_t         index;
    byte_t           hash_bytes[32];
    char             hash_hex[65];
} cardano_governance_action_id_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a governance_action_id object.
 *
 * This function is responsible for properly deallocating a governance action id object (`cardano_governance_action_id_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the governance_action_id object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_governance_action_id_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the governance_action_id
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_governance_action_id_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_governance_action_id_new(const cardano_blake2b_hash_t* hash, const uint64_t index, cardano_governance_action_id_t** governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  *governance_action_id = _cardano_malloc(sizeof(cardano_governance_action_id_t));

  if (*governance_action_id == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  (*governance_action_id)->base.deallocator   = cardano_governance_action_id_deallocate;
  (*governance_action_id)->base.ref_count     = 1;
  (*governance_action_id)->base.last_error[0] = '\0';
  (*governance_action_id)->index              = index;

  const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash);

  if (hash_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    cardano_governance_action_id_unref(governance_action_id);
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, (*governance_action_id)->hash_bytes, sizeof((*governance_action_id)->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, (*governance_action_id)->hash_hex, sizeof((*governance_action_id)->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_governance_action_id_from_hash_hex(const char* hex, size_t hex_size, uint64_t index, cardano_governance_action_id_t** governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  if (hex_size != ((size_t)CARDANO_BLAKE2B_HASH_SIZE_256 * 2U))
  {
    *governance_action_id = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_hex(hex, hex_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *governance_action_id = NULL;
    return read_hash_result;
  }

  cardano_error_t new_governance_action_id_result = cardano_governance_action_id_new(hash, index, governance_action_id);

  cardano_blake2b_hash_unref(&hash);

  if (new_governance_action_id_result != CARDANO_SUCCESS)
  {
    *governance_action_id = NULL;
    return new_governance_action_id_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_governance_action_id_from_hash_bytes(
  const byte_t*                    data,
  size_t                           data_size,
  uint64_t                         index,
  cardano_governance_action_id_t** governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  if (data_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    *governance_action_id = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(data, data_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *governance_action_id = NULL;
    return read_hash_result;
  }

  cardano_error_t new_governance_action_id_result = cardano_governance_action_id_new(hash, index, governance_action_id);

  cardano_blake2b_hash_unref(&hash);

  if (new_governance_action_id_result != CARDANO_SUCCESS)
  {
    *governance_action_id = NULL;
    return new_governance_action_id_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_governance_action_id_from_cbor(cardano_cbor_reader_t* reader, cardano_governance_action_id_t** governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  static const char* validator_name = "governance_action_id";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, GOVERNANCE_ACTION_ID_ARRAY_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *governance_action_id = NULL;
    return expect_array_result;
  }

  cardano_buffer_t*     byte_string             = NULL;
  const cardano_error_t read_byte_string_result = cardano_cbor_validate_byte_string_of_size(
    validator_name,
    reader,
    &byte_string,
    CARDANO_BLAKE2B_HASH_SIZE_256);

  if (read_byte_string_result != CARDANO_SUCCESS)
  {
    *governance_action_id = NULL;
    return read_byte_string_result;
  }

  uint64_t              index            = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "governance_action_id_type",
    reader,
    &index,
    0,
    UINT64_MAX);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&byte_string);

    *governance_action_id = NULL;
    return read_uint_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    /* LCOV_EXCL_START */
    cardano_buffer_unref(&byte_string);
    *governance_action_id = NULL;

    return expect_end_array_result;
    /* LCOV_EXCL_STOP */
  }

  const cardano_error_t new_governance_action_id_result = cardano_governance_action_id_from_hash_bytes(
    cardano_buffer_get_data(byte_string),
    cardano_buffer_get_size(byte_string),
    index,
    governance_action_id);

  cardano_buffer_unref(&byte_string);

  if (new_governance_action_id_result != CARDANO_SUCCESS)
  {
    /* LCOV_EXCL_START */
    *governance_action_id = NULL;
    return new_governance_action_id_result;
    /* LCOV_EXCL_STOP */
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_governance_action_id_to_cbor(
  const cardano_governance_action_id_t* governance_action_id,
  cardano_cbor_writer_t*                writer)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, GOVERNANCE_ACTION_ID_ARRAY_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result; /* LCOV_EXCL_LINE */
  }

  cardano_error_t write_bytes_result = cardano_cbor_writer_write_byte_string(writer, governance_action_id->hash_bytes, sizeof(governance_action_id->hash_bytes));

  if (write_bytes_result != CARDANO_SUCCESS)
  {
    return write_bytes_result; /* LCOV_EXCL_LINE */
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_unsigned_int(writer, governance_action_id->index);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result; /* LCOV_EXCL_LINE */
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_governance_action_id_get_hash(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(governance_action_id->hash_bytes, sizeof(governance_action_id->hash_bytes), &hash);

  assert(read_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(read_hash_result);

  return hash;
}

size_t
cardano_governance_action_id_get_hash_bytes_size(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return 0U;
  }

  return sizeof(governance_action_id->hash_bytes);
}

const byte_t*
cardano_governance_action_id_get_hash_bytes(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return NULL;
  }

  return governance_action_id->hash_bytes;
}

size_t
cardano_governance_action_id_get_hash_hex_size(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return 0U;
  }

  return cardano_safe_strlen(governance_action_id->hash_hex, sizeof(governance_action_id->hash_hex)) + 1U;
}

const char*
cardano_governance_action_id_get_hash_hex(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return NULL;
  }

  return governance_action_id->hash_hex;
}

cardano_error_t
cardano_governance_action_id_get_index(
  const cardano_governance_action_id_t* governance_action_id,
  uint64_t*                             index)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (index == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *index = governance_action_id->index;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_governance_action_id_set_index(
  cardano_governance_action_id_t* governance_action_id,
  uint64_t                        index)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  governance_action_id->index = index;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_governance_action_id_set_hash(cardano_governance_action_id_t* governance_action_id, const cardano_blake2b_hash_t* hash)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash);

  if (hash_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, &governance_action_id->hash_bytes[0], sizeof(governance_action_id->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, governance_action_id->hash_hex, sizeof(governance_action_id->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

void
cardano_governance_action_id_unref(cardano_governance_action_id_t** governance_action_id)
{
  if ((governance_action_id == NULL) || (*governance_action_id == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*governance_action_id)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *governance_action_id = NULL;
    return;
  }
}

void
cardano_governance_action_id_ref(cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return;
  }

  cardano_object_ref(&governance_action_id->base);
}

size_t
cardano_governance_action_id_refcount(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&governance_action_id->base);
}

void
cardano_governance_action_id_set_last_error(cardano_governance_action_id_t* governance_action_id, const char* message)
{
  cardano_object_set_last_error(&governance_action_id->base, message);
}

const char*
cardano_governance_action_id_get_last_error(const cardano_governance_action_id_t* governance_action_id)
{
  return cardano_object_get_last_error(&governance_action_id->base);
}