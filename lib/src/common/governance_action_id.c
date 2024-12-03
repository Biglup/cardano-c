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
#include <cardano/encoding/bech32.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t GOVERNANCE_ACTION_ID_ARRAY_SIZE  = 2;
static const char*   GOVERNANCE_ACTION_ID_PREFIX      = "gov_action";
static const size_t  GOVERNANCE_ACTION_ID_PREFIX_SIZE = 10;

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
    char             cip129_str[1024];
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

/**
 * \brief Encodes a transaction ID and index into a CIP-29 compliant governance action id Bech32 format.
 *
 * The Cardano Conway era introduces governance actions (gov actions), which are uniquely identified by the transaction ID
 * they were submitted in and an index within the transaction. This function combines the transaction ID (32 bytes) and the index
 * (1 byte) into a single byte string, then encodes it into a CIP-29 compliant Bech32 string representation.
 *
 * \param[in]  bytes       A pointer to the 32-byte transaction ID. This parameter must not be NULL.
 * \param[in]  bytes_size  The size of the `bytes` array in bytes. Must be 32 to represent a valid transaction ID.
 * \param[in]  index       A single byte representing the governance action index within the transaction.
 * \param[out] dest        A pointer to a buffer where the encoded Bech32 string will be stored.
 * \param[in]  dest_size   The size of the buffer in bytes. Must be large enough to hold the Bech32 string including null-termination.
 *
 * \return \ref CARDANO_SUCCESS if the encoding was successful, or an appropriate error code indicating the failure reason.
 */
static cardano_error_t
to_cip29_bech32(
  const byte_t* bytes,
  const size_t  bytes_size,
  const byte_t  index,
  char*         dest,
  const size_t  dest_size)
{
  if (bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t cip29_payload[33] = { 0 };

  cardano_safe_memcpy(cip29_payload, 32, bytes, bytes_size);
  cip29_payload[32] = index;

  cardano_error_t result = cardano_encoding_bech32_encode(GOVERNANCE_ACTION_ID_PREFIX, GOVERNANCE_ACTION_ID_PREFIX_SIZE, cip29_payload, 33, dest, dest_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_governance_action_id_new(const cardano_blake2b_hash_t* hash, const uint64_t index, cardano_governance_action_id_t** governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *governance_action_id = _cardano_malloc(sizeof(cardano_governance_action_id_t));

  if (*governance_action_id == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*governance_action_id)->base.deallocator   = cardano_governance_action_id_deallocate;
  (*governance_action_id)->base.ref_count     = 1;
  (*governance_action_id)->base.last_error[0] = '\0';
  (*governance_action_id)->index              = index;

  CARDANO_UNUSED(memset((*governance_action_id)->hash_bytes, 0, sizeof((*governance_action_id)->hash_bytes)));
  CARDANO_UNUSED(memset((*governance_action_id)->hash_hex, 0, sizeof((*governance_action_id)->hash_hex)));
  CARDANO_UNUSED(memset((*governance_action_id)->cip129_str, 0, sizeof((*governance_action_id)->cip129_str)));

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

  return to_cip29_bech32((*governance_action_id)->hash_bytes, sizeof((*governance_action_id)->hash_bytes), (byte_t)index, (*governance_action_id)->cip129_str, sizeof((*governance_action_id)->cip129_str));
}

cardano_error_t
cardano_governance_action_id_from_bech32(
  const char*                      data,
  const size_t                     size,
  cardano_governance_action_id_t** action_id)
{
  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size == 0U)
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  if (action_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t       hrp_size  = 0;
  const size_t data_size = cardano_encoding_bech32_get_decoded_length(data, size, &hrp_size);

  // 32 bytes for the hash and 1 byte for the index
  if (data_size != 33U)
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  char*   hrp          = _cardano_malloc(hrp_size);
  byte_t* decoded_data = _cardano_malloc(data_size);

  const cardano_error_t decode_result = cardano_encoding_bech32_decode(data, size, hrp, hrp_size, decoded_data, data_size);

  if (decode_result != CARDANO_SUCCESS)
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return decode_result;
  }

  hrp_size -= 1U;

  if ((hrp_size != cardano_safe_strlen(GOVERNANCE_ACTION_ID_PREFIX, GOVERNANCE_ACTION_ID_PREFIX_SIZE)) || (strncmp(hrp, GOVERNANCE_ACTION_ID_PREFIX, hrp_size) != 0))
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  byte_t          index  = decoded_data[data_size - 1U];
  cardano_error_t result = cardano_governance_action_id_from_hash_bytes(decoded_data, data_size - 1U, index, action_id);

  _cardano_free(hrp);
  _cardano_free(decoded_data);

  return result;
}

cardano_error_t
cardano_governance_action_id_from_hash_hex(const char* hex, size_t hex_size, uint64_t index, cardano_governance_action_id_t** governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
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
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
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
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *governance_action_id = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "governance_action_id";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)GOVERNANCE_ACTION_ID_ARRAY_SIZE);

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
    cardano_buffer_unref(&byte_string);
    *governance_action_id = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t new_governance_action_id_result = cardano_governance_action_id_from_hash_bytes(
    cardano_buffer_get_data(byte_string),
    cardano_buffer_get_size(byte_string),
    index,
    governance_action_id);

  cardano_buffer_unref(&byte_string);

  if (new_governance_action_id_result != CARDANO_SUCCESS)
  {
    *governance_action_id = NULL;
    return new_governance_action_id_result;
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
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, GOVERNANCE_ACTION_ID_ARRAY_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_bytes_result = cardano_cbor_writer_write_bytestring(writer, governance_action_id->hash_bytes, sizeof(governance_action_id->hash_bytes));

  if (write_bytes_result != CARDANO_SUCCESS)
  {
    return write_bytes_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, governance_action_id->index);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_governance_action_id_get_bech32_size(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return 0U;
  }

  return cardano_safe_strlen(governance_action_id->cip129_str, sizeof(governance_action_id->cip129_str)) + 1U;
}

cardano_error_t
cardano_governance_action_id_to_bech32(
  const cardano_governance_action_id_t* governance_action_id,
  char*                                 data,
  size_t                                size)
{
  if (governance_action_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t cip29_size = cardano_governance_action_id_get_bech32_size(governance_action_id);

  if (size < cip29_size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(data, size, governance_action_id->cip129_str, cip29_size - 1U);
  data[cip29_size - 1U] = '\0';

  return CARDANO_SUCCESS;
}

const char*
cardano_governance_action_id_get_string(const cardano_governance_action_id_t* governance_action_id)
{
  if (governance_action_id == NULL)
  {
    return NULL;
  }

  return governance_action_id->cip129_str;
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
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
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
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  governance_action_id->index = index;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_governance_action_id_set_hash(cardano_governance_action_id_t* governance_action_id, const cardano_blake2b_hash_t* hash)
{
  if (governance_action_id == NULL)
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

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, &governance_action_id->hash_bytes[0], sizeof(governance_action_id->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, governance_action_id->hash_hex, sizeof(governance_action_id->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

bool
cardano_governance_action_id_equals(
  const cardano_governance_action_id_t* lhs,
  const cardano_governance_action_id_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if ((lhs == NULL) || (rhs == NULL))
  {
    return false;
  }

  if (lhs->index != rhs->index)
  {
    return false;
  }

  return (memcmp(lhs->hash_bytes, rhs->hash_bytes, sizeof(lhs->hash_bytes)) == 0);
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