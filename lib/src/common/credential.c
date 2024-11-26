/**
 * \file credential.c
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

#include <cardano/common/credential.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t CREDENTIAL_ARRAY_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano credential.
 */
typedef struct cardano_credential_t
{
    cardano_object_t          base;
    cardano_credential_type_t type;
    byte_t                    hash_bytes[28];
    char                      hash_hex[57];
} cardano_credential_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a credential object.
 *
 * This function is responsible for properly deallocating a credential object (`cardano_credential_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the credential object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_credential_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the credential
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_credential_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_credential_new(const cardano_blake2b_hash_t* hash, const cardano_credential_type_t type, cardano_credential_t** credential)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    *credential = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *credential = _cardano_malloc(sizeof(cardano_credential_t));

  if (*credential == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*credential)->base.deallocator   = cardano_credential_deallocate;
  (*credential)->base.ref_count     = 1;
  (*credential)->base.last_error[0] = '\0';
  (*credential)->type               = type;

  const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash);

  if (hash_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_224)
  {
    cardano_credential_unref(credential);
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, (*credential)->hash_bytes, sizeof((*credential)->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, (*credential)->hash_hex, sizeof((*credential)->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_credential_from_hash_hex(const char* hex, size_t hex_size, cardano_credential_type_t type, cardano_credential_t** credential)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *credential = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex_size != ((size_t)CARDANO_BLAKE2B_HASH_SIZE_224 * 2U))
  {
    *credential = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_hex(hex, hex_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return read_hash_result;
  }

  cardano_error_t new_credential_result = cardano_credential_new(hash, type, credential);

  cardano_blake2b_hash_unref(&hash);

  if (new_credential_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return new_credential_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_credential_from_hash_bytes(
  const byte_t*             data,
  size_t                    data_size,
  cardano_credential_type_t type,
  cardano_credential_t**    credential)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *credential = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_224)
  {
    *credential = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(data, data_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return read_hash_result;
  }

  cardano_error_t new_credential_result = cardano_credential_new(hash, type, credential);

  cardano_blake2b_hash_unref(&hash);

  if (new_credential_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return new_credential_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_credential_from_cbor(cardano_cbor_reader_t* reader, cardano_credential_t** credential)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *credential = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "Credential";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)CREDENTIAL_ARRAY_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "credential_type",
    reader,
    &type,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return read_uint_result;
  }

  cardano_buffer_t*     byte_string             = NULL;
  const cardano_error_t read_byte_string_result = cardano_cbor_validate_byte_string_of_size(
    validator_name,
    reader,
    &byte_string,
    CARDANO_BLAKE2B_HASH_SIZE_224);

  if (read_byte_string_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return read_byte_string_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&byte_string);
    *credential = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t new_credential_result = cardano_credential_from_hash_bytes(
    cardano_buffer_get_data(byte_string),
    cardano_buffer_get_size(byte_string),
    (cardano_credential_type_t)type,
    credential);

  cardano_buffer_unref(&byte_string);

  if (new_credential_result != CARDANO_SUCCESS)
  {
    *credential = NULL;
    return new_credential_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_credential_to_cbor(
  const cardano_credential_t* credential,
  cardano_cbor_writer_t*      writer)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, CREDENTIAL_ARRAY_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, credential->type);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  cardano_error_t write_bytes_result = cardano_cbor_writer_write_bytestring(writer, credential->hash_bytes, sizeof(credential->hash_bytes));

  if (write_bytes_result != CARDANO_SUCCESS)
  {
    return write_bytes_result;
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_credential_get_hash(const cardano_credential_t* credential)
{
  if (credential == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(credential->hash_bytes, sizeof(credential->hash_bytes), &hash);

  CARDANO_UNUSED(read_hash_result);

  return hash;
}

size_t
cardano_credential_get_hash_bytes_size(const cardano_credential_t* credential)
{
  if (credential == NULL)
  {
    return 0U;
  }

  return sizeof(credential->hash_bytes);
}

const byte_t*
cardano_credential_get_hash_bytes(const cardano_credential_t* credential)
{
  if (credential == NULL)
  {
    return NULL;
  }

  return credential->hash_bytes;
}

size_t
cardano_credential_get_hash_hex_size(const cardano_credential_t* credential)
{
  if (credential == NULL)
  {
    return 0U;
  }

  return cardano_safe_strlen(credential->hash_hex, sizeof(credential->hash_hex)) + 1U;
}

const char*
cardano_credential_get_hash_hex(const cardano_credential_t* credential)
{
  if (credential == NULL)
  {
    return NULL;
  }

  return credential->hash_hex;
}

cardano_error_t
cardano_credential_get_type(const cardano_credential_t* credential, cardano_credential_type_t* type)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = credential->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_credential_set_type(cardano_credential_t* credential, cardano_credential_type_t type)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((type != CARDANO_CREDENTIAL_TYPE_KEY_HASH) && (type != CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH))
  {
    return CARDANO_ERROR_INVALID_CREDENTIAL_TYPE;
  }

  credential->type = type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_credential_set_hash(cardano_credential_t* credential, const cardano_blake2b_hash_t* hash)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash);

  if (hash_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_224)
  {
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, &credential->hash_bytes[0], sizeof(credential->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, credential->hash_hex, sizeof(credential->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

bool
cardano_credential_equals(
  const cardano_credential_t* lhs,
  const cardano_credential_t* rhs)
{
  return cardano_credential_compare(lhs, rhs) == 0;
}

int32_t
cardano_credential_compare(
  const cardano_credential_t* lhs,
  const cardano_credential_t* rhs)
{
  if (lhs == rhs)
  {
    return 0;
  }

  if (lhs == NULL)
  {
    return -1;
  }

  if (rhs == NULL)
  {
    return 1;
  }

  if (lhs->type != rhs->type)
  {
    return (lhs->type < rhs->type) ? -1 : 1;
  }

  return memcmp(lhs->hash_bytes, rhs->hash_bytes, sizeof(lhs->hash_bytes));
}

void
cardano_credential_unref(cardano_credential_t** credential)
{
  if ((credential == NULL) || (*credential == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*credential)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *credential = NULL;
    return;
  }
}

void
cardano_credential_ref(cardano_credential_t* credential)
{
  if (credential == NULL)
  {
    return;
  }

  cardano_object_ref(&credential->base);
}

size_t
cardano_credential_refcount(const cardano_credential_t* credential)
{
  if (credential == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&credential->base);
}

void
cardano_credential_set_last_error(cardano_credential_t* credential, const char* message)
{
  cardano_object_set_last_error(&credential->base, message);
}

const char*
cardano_credential_get_last_error(const cardano_credential_t* credential)
{
  return cardano_object_get_last_error(&credential->base);
}