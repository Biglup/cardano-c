/**
 * \file datum.c
 *
 * \author luisd.bianchi
 * \date   May 15, 2024
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

#include <cardano/common/datum.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>
#include <cardano/plutus_data/plutus_data.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t DATUM_ARRAY_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano datum.
 */
typedef struct cardano_datum_t
{
    cardano_object_t       base;
    cardano_datum_type_t   type;
    byte_t                 hash_bytes[32];
    char                   hash_hex[65];
    cardano_plutus_data_t* inline_data;
} cardano_datum_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a datum object.
 *
 * This function is responsible for properly deallocating a datum object (`cardano_datum_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the datum object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_datum_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the datum
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_datum_deallocate(void* object)
{
  assert(object != NULL);

  cardano_plutus_data_unref(&((cardano_datum_t*)object)->inline_data);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_datum_new_data_hash(const cardano_blake2b_hash_t* hash, cardano_datum_t** datum)
{
  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    *datum = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *datum = _cardano_malloc(sizeof(cardano_datum_t));

  if (*datum == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*datum)->base.deallocator   = cardano_datum_deallocate;
  (*datum)->base.ref_count     = 1;
  (*datum)->base.last_error[0] = '\0';
  (*datum)->type               = CARDANO_DATUM_TYPE_DATA_HASH;
  (*datum)->inline_data        = NULL;

  const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash);

  if (hash_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    cardano_datum_unref(datum);
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, (*datum)->hash_bytes, sizeof((*datum)->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, (*datum)->hash_hex, sizeof((*datum)->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_datum_new_data_hash_hex(const char* hex, size_t hex_size, cardano_datum_t** datum)
{
  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *datum = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex_size != ((size_t)CARDANO_BLAKE2B_HASH_SIZE_256 * 2U))
  {
    *datum = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_hex(hex, hex_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return read_hash_result;
  }

  cardano_error_t new_datum_result = cardano_datum_new_data_hash(hash, datum);

  cardano_blake2b_hash_unref(&hash);

  if (new_datum_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return new_datum_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_datum_new_data_hash_bytes(const byte_t* data, size_t data_size, cardano_datum_t** datum)
{
  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *datum = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_size != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    *datum = NULL;
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(data, data_size, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return read_hash_result;
  }

  cardano_error_t new_datum_result = cardano_datum_new_data_hash(hash, datum);

  cardano_blake2b_hash_unref(&hash);

  if (new_datum_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return new_datum_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_datum_new_inline_data(
  cardano_plutus_data_t* data,
  cardano_datum_t**      datum)
{
  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *datum = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *datum = _cardano_malloc(sizeof(cardano_datum_t));

  if (*datum == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*datum)->base.deallocator   = cardano_datum_deallocate;
  (*datum)->base.ref_count     = 1;
  (*datum)->base.last_error[0] = '\0';
  (*datum)->type               = CARDANO_DATUM_TYPE_INLINE_DATA;
  (*datum)->inline_data        = data;

  cardano_plutus_data_ref(data);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_datum_from_cbor(cardano_cbor_reader_t* reader, cardano_datum_t** datum)
{
  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *datum = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "Datum";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)DATUM_ARRAY_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "datum_type",
    reader,
    &type,
    CARDANO_DATUM_TYPE_DATA_HASH,
    CARDANO_DATUM_TYPE_INLINE_DATA);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return read_uint_result;
  }

  if (type == (uint64_t)CARDANO_DATUM_TYPE_DATA_HASH)
  {
    cardano_buffer_t*     byte_string             = NULL;
    const cardano_error_t read_byte_string_result = cardano_cbor_validate_byte_string_of_size(
      validator_name,
      reader,
      &byte_string,
      CARDANO_BLAKE2B_HASH_SIZE_256);

    if (read_byte_string_result != CARDANO_SUCCESS)
    {
      *datum = NULL;
      return read_byte_string_result;
    }

    const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

    if (expect_end_array_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&byte_string);
      *datum = NULL;

      return expect_end_array_result;
    }

    const cardano_error_t new_datum_result = cardano_datum_new_data_hash_bytes(
      cardano_buffer_get_data(byte_string),
      cardano_buffer_get_size(byte_string),
      datum);

    cardano_buffer_unref(&byte_string);

    if (new_datum_result != CARDANO_SUCCESS)
    {
      *datum = NULL;
      return new_datum_result;
    }

    return CARDANO_SUCCESS;
  }

  cardano_cbor_tag_t tag;

  const cardano_error_t read_tag_result = cardano_cbor_reader_read_tag(reader, &tag);

  if (read_tag_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return read_tag_result;
  }

  if (tag != CARDANO_ENCODED_CBOR_DATA_ITEM)
  {
    *datum = NULL;
    return CARDANO_ERROR_INVALID_CBOR_VALUE;
  }

  cardano_buffer_t* byte_string = NULL;

  const cardano_error_t read_byte_string_result = cardano_cbor_reader_read_bytestring(reader, &byte_string);

  if (read_byte_string_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return read_byte_string_result;
  }

  cardano_cbor_reader_t* plutus_data_reader = cardano_cbor_reader_new(cardano_buffer_get_data(byte_string), cardano_buffer_get_size(byte_string));
  cardano_buffer_unref(&byte_string);

  cardano_plutus_data_t* inline_data = NULL;

  const cardano_error_t read_inline_data_result = cardano_plutus_data_from_cbor(plutus_data_reader, &inline_data);

  cardano_cbor_reader_unref(&plutus_data_reader);

  if (read_inline_data_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return read_inline_data_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_plutus_data_unref(&inline_data);
    *datum = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t new_datum_result = cardano_datum_new_inline_data(inline_data, datum);

  cardano_plutus_data_unref(&inline_data);

  if (new_datum_result != CARDANO_SUCCESS)
  {
    *datum = NULL;
    return new_datum_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_datum_to_cbor(
  const cardano_datum_t* datum,
  cardano_cbor_writer_t* writer)
{
  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, DATUM_ARRAY_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, datum->type);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  switch (datum->type)
  {
    case CARDANO_DATUM_TYPE_DATA_HASH:
    {
      cardano_error_t write_bytes_result = cardano_cbor_writer_write_bytestring(writer, datum->hash_bytes, sizeof(datum->hash_bytes));

      if (write_bytes_result != CARDANO_SUCCESS)
      {
        return write_bytes_result;
      }

      break;
    }
    case CARDANO_DATUM_TYPE_INLINE_DATA:
    {
      cardano_error_t write_tag_result = cardano_cbor_writer_write_tag(writer, CARDANO_ENCODED_CBOR_DATA_ITEM);

      if (write_tag_result != CARDANO_SUCCESS)
      {
        return write_tag_result;
      }

      cardano_cbor_writer_t* plutus_data_writer       = cardano_cbor_writer_new();
      cardano_error_t        write_plutus_data_result = cardano_plutus_data_to_cbor(datum->inline_data, plutus_data_writer);

      if (write_plutus_data_result != CARDANO_SUCCESS)
      {
        cardano_cbor_writer_unref(&plutus_data_writer);
        return write_plutus_data_result;
      }

      cardano_buffer_t* byte_string = NULL;

      cardano_error_t encode_result = cardano_cbor_writer_encode_in_buffer(plutus_data_writer, &byte_string);

      cardano_cbor_writer_unref(&plutus_data_writer);

      if (encode_result != CARDANO_SUCCESS)
      {
        return encode_result;
      }

      cardano_error_t write_byte_string_result = cardano_cbor_writer_write_bytestring(writer, cardano_buffer_get_data(byte_string), cardano_buffer_get_size(byte_string));

      cardano_buffer_unref(&byte_string);

      if (write_byte_string_result != CARDANO_SUCCESS)
      {
        return write_byte_string_result;
      }

      break;
    }

    default:
    {
      cardano_cbor_writer_set_last_error(writer, "Invalid datum type");
      return CARDANO_ERROR_INVALID_DATUM_TYPE;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_plutus_data_t*
cardano_datum_get_inline_data(cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return NULL;
  }

  cardano_plutus_data_ref(datum->inline_data);

  return datum->inline_data;
}

cardano_blake2b_hash_t*
cardano_datum_get_data_hash(const cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_t* hash             = NULL;
  cardano_error_t         read_hash_result = cardano_blake2b_hash_from_bytes(datum->hash_bytes, sizeof(datum->hash_bytes), &hash);

  assert(read_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(read_hash_result);

  return hash;
}

size_t
cardano_datum_get_data_hash_bytes_size(const cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return 0U;
  }

  return sizeof(datum->hash_bytes);
}

const byte_t*
cardano_datum_get_data_hash_bytes(const cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return NULL;
  }

  return datum->hash_bytes;
}

size_t
cardano_datum_get_data_hash_hex_size(const cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return 0U;
  }

  return cardano_safe_strlen(datum->hash_hex, sizeof(datum->hash_hex)) + 1U;
}

const char*
cardano_datum_get_data_hash_hex(const cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return NULL;
  }

  return datum->hash_hex;
}

cardano_error_t
cardano_datum_get_type(const cardano_datum_t* datum, cardano_datum_type_t* type)
{
  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = datum->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_datum_set_data_hash(cardano_datum_t* datum, const cardano_blake2b_hash_t* hash)
{
  if (datum == NULL)
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

  cardano_error_t copy_hash_result = cardano_blake2b_hash_to_bytes(hash, datum->hash_bytes, sizeof(datum->hash_bytes));

  assert(copy_hash_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hash_result);

  cardano_error_t copy_hex_result = cardano_blake2b_hash_to_hex(hash, datum->hash_hex, sizeof(datum->hash_hex));

  assert(copy_hex_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(copy_hex_result);

  return CARDANO_SUCCESS;
}

bool
cardano_datum_equals(const cardano_datum_t* lhs, const cardano_datum_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (lhs->type != rhs->type)
  {
    return false;
  }

  if (lhs->type == CARDANO_DATUM_TYPE_DATA_HASH)
  {
    return (memcmp(lhs->hash_bytes, rhs->hash_bytes, sizeof(lhs->hash_bytes)) == 0);
  }

  return cardano_plutus_data_equals(lhs->inline_data, rhs->inline_data);
}

void
cardano_datum_unref(cardano_datum_t** datum)
{
  if ((datum == NULL) || (*datum == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*datum)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *datum = NULL;
    return;
  }
}

void
cardano_datum_ref(cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return;
  }

  cardano_object_ref(&datum->base);
}

size_t
cardano_datum_refcount(const cardano_datum_t* datum)
{
  if (datum == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&datum->base);
}

void
cardano_datum_set_last_error(cardano_datum_t* datum, const char* message)
{
  cardano_object_set_last_error(&datum->base, message);
}

const char*
cardano_datum_get_last_error(const cardano_datum_t* datum)
{
  return cardano_object_get_last_error(&datum->base);
}