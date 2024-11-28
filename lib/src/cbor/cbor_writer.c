/**
 * \file cbor_writer.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * Copyright 2023 Biglup Labs
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

#include "cbor_additional_info.h"

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_major_type.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief A simple writer for Concise Binary Object Representation (CBOR) encoded data.
 */
typedef struct cardano_cbor_writer_t
{
    cardano_object_t  base;
    cardano_buffer_t* buffer;
} cardano_cbor_writer_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Writes a value with a specified CBOR major type to a buffer.
 *
 * This function serializes a given value into a format specified by the CBOR
 * (Concise Binary Object Representation) encoding standard, targeting the provided
 * buffer. The major type parameter determines how the value is interpreted and encoded
 * according to CBOR's major type specification.
 *
 * \param buffer A pointer to the \c cardano_buffer_t structure representing the target buffer
 *               where the encoded data will be written.
 * \param major_type The CBOR major type of the value to write. This parameter defines the data type
 *                   and format of the value in the CBOR encoding (e.g., unsigned integer, byte string, etc.).
 *                   It must be one of the values defined by the \c cardano_cbor_major_type_t enumeration.
 * \param value The value to be encoded and written to the buffer. The function interprets and encodes
 *              this value according to the specified CBOR major type.
 *
 * \return A \c cardano_error_t indicating the result of the operation. Returns \c CARDANO_SUCCESS if
 *         the value is successfully encoded and written to the buffer. If an error occurs during the
 *         operation, a corresponding error code is returned, indicating the failure reason.
 */
static cardano_error_t
write_type_value(cardano_buffer_t* buffer, const cardano_cbor_major_type_t major_type, const uint64_t value)
{
  const byte_t type = (byte_t)major_type << 5;

  if (value < 24U)
  {
    byte_t          header = type | (byte_t)value;
    cardano_error_t result = cardano_buffer_write(buffer, &header, sizeof(header));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }
  else if (value < 256U)
  {
    byte_t          header = type | (byte_t)CARDANO_CBOR_ADDITIONAL_INFO_8BIT_DATA;
    cardano_error_t result = cardano_buffer_write(buffer, &header, sizeof(header));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    byte_t val = (byte_t)value;
    return cardano_buffer_write(buffer, &val, 1);
  }
  else if (value < 65536U)
  {
    byte_t          header = type | (byte_t)CARDANO_CBOR_ADDITIONAL_INFO_16BIT_DATA;
    cardano_error_t result = cardano_buffer_write(buffer, &header, sizeof(header));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    return cardano_buffer_write_uint16_be(buffer, (uint16_t)value);
  }
  else if (value < 4294967296U)
  {
    byte_t          header = type | (byte_t)CARDANO_CBOR_ADDITIONAL_INFO_32BIT_DATA;
    cardano_error_t result = cardano_buffer_write(buffer, &header, sizeof(header));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    return cardano_buffer_write_uint32_be(buffer, (uint32_t)value);
  }
  else
  {
    byte_t          header = type | (byte_t)CARDANO_CBOR_ADDITIONAL_INFO_64BIT_DATA;
    cardano_error_t result = cardano_buffer_write(buffer, &header, sizeof(header));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    return cardano_buffer_write_uint64_be(buffer, value);
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Deallocates a CBOR writer object.
 *
 * This function is responsible for properly deallocating a CBOR writer object (`cardano_cbor_writer_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the CBOR writer object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_cbor_writer_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the CBOR writer
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_cbor_writer_deallocate(void* object)
{
  assert(object != NULL);

  cardano_cbor_writer_t* cbor_writer = (cardano_cbor_writer_t*)object;

  if (cbor_writer->buffer != NULL)
  {
    cardano_buffer_unref(&cbor_writer->buffer);
  }

  _cardano_free(cbor_writer);
}

/* DECLARATIONS **************************************************************/

cardano_cbor_writer_t*
cardano_cbor_writer_new(void)
{
  cardano_cbor_writer_t* obj = (cardano_cbor_writer_t*)_cardano_malloc(sizeof(cardano_cbor_writer_t));

  if (obj == NULL)
  {
    return NULL;
  }

  obj->base.ref_count     = 1;
  obj->base.deallocator   = cardano_cbor_writer_deallocate;
  obj->base.last_error[0] = '\0';
  obj->buffer             = cardano_buffer_new(128);

  if (obj->buffer == NULL)
  {
    _cardano_free(obj);
    return NULL;
  }

  return obj;
}

void
cardano_cbor_writer_unref(cardano_cbor_writer_t** cbor_writer)
{
  if ((cbor_writer == NULL) || (*cbor_writer == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*cbor_writer)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *cbor_writer = NULL;
    return;
  }
}

void
cardano_cbor_writer_ref(cardano_cbor_writer_t* cbor_writer)
{
  if (cbor_writer == NULL)
  {
    return;
  }

  cardano_object_ref(&cbor_writer->base);
}

size_t
cardano_cbor_writer_refcount(const cardano_cbor_writer_t* cbor_writer)
{
  if (cbor_writer == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&cbor_writer->base);
}

cardano_error_t
cardano_cbor_writer_write_bigint(cardano_cbor_writer_t* writer, const cardano_bigint_t* bigint)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bigint == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bigint_t* copy   = NULL;
  cardano_error_t   result = cardano_bigint_clone(bigint, &copy);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_tag_t tag = CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM;

  if (cardano_bigint_signum(copy) < 0)
  {
    tag = CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM;
    cardano_bigint_negate(bigint, copy);
  }

  result = cardano_cbor_writer_write_tag(writer, tag);

  if (result != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&copy);

    return result;
  }

  const size_t size = cardano_bigint_get_bytes_size(copy);
  byte_t*      data = (byte_t*)_cardano_malloc(size);

  if (data == NULL)
  {
    cardano_bigint_unref(&copy);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_bigint_to_bytes(copy, CARDANO_BYTE_ORDER_BIG_ENDIAN, data, size);

  cardano_bigint_unref(&copy);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(data);

    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, data, size);

  _cardano_free(data);

  return result;
}

cardano_error_t
cardano_cbor_writer_write_bool(cardano_cbor_writer_t* writer, const bool value)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t cbor_bool_val = (byte_t)(value ? 245 : 244);

  return cardano_buffer_write(writer->buffer, &cbor_bool_val, 1);
}

cardano_error_t
cardano_cbor_writer_write_bytestring(cardano_cbor_writer_t* writer, const byte_t* data, const size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING, size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_buffer_write(writer->buffer, data, size);
}

cardano_error_t
cardano_cbor_writer_write_textstring(cardano_cbor_writer_t* writer, const char* data, const size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING, size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_buffer_write(writer->buffer, (const byte_t*)data, size);
}

cardano_error_t
cardano_cbor_writer_write_encoded(cardano_cbor_writer_t* writer, const byte_t* data, const size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_write(writer->buffer, data, size);
}

cardano_error_t
cardano_cbor_writer_write_start_array(cardano_cbor_writer_t* writer, const int64_t size)
{
  static byte_t indefinite_length_array = 0x9fU;

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < 0)
  {
    return cardano_buffer_write(writer->buffer, &indefinite_length_array, sizeof(indefinite_length_array));
  }

  return write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_ARRAY, size);
}

cardano_error_t
cardano_cbor_writer_write_end_array(cardano_cbor_writer_t* writer)
{
  static byte_t indefiniteLengthBreakByte = 0xffU;

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_write(writer->buffer, &indefiniteLengthBreakByte, sizeof(indefiniteLengthBreakByte));
}

cardano_error_t
cardano_cbor_writer_write_start_map(cardano_cbor_writer_t* writer, const int64_t size)
{
  static byte_t indefinite_length_map = 0xbfU;

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < 0)
  {
    return cardano_buffer_write(writer->buffer, &indefinite_length_map, sizeof(indefinite_length_map));
  }

  return write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_MAP, size);
}

cardano_error_t
cardano_cbor_writer_write_end_map(cardano_cbor_writer_t* writer)
{
  return cardano_cbor_writer_write_end_array(writer);
}

cardano_error_t
cardano_cbor_writer_write_uint(cardano_cbor_writer_t* writer, const uint64_t value)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value);
}

cardano_error_t
cardano_cbor_writer_write_signed_int(cardano_cbor_writer_t* writer, const int64_t value)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value < 0)
  {
    return write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER, -1 - value);
  }

  return write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value);
}

cardano_error_t
cardano_cbor_writer_write_null(cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static byte_t cbor_null = 0xf6U;

  return cardano_buffer_write(writer->buffer, &cbor_null, sizeof(cbor_null));
}

cardano_error_t
cardano_cbor_writer_write_undefined(cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static byte_t cbor_undefined = 0xf7U;

  return cardano_buffer_write(writer->buffer, &cbor_undefined, sizeof(cbor_undefined));
}

cardano_error_t
cardano_cbor_writer_write_tag(cardano_cbor_writer_t* writer, const cardano_cbor_tag_t tag)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return write_type_value(writer->buffer, CARDANO_CBOR_MAJOR_TYPE_TAG, tag);
}

size_t
cardano_cbor_writer_get_encode_size(cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return 0U;
  }

  return cardano_buffer_get_size(writer->buffer);
}

cardano_error_t
cardano_cbor_writer_encode(cardano_cbor_writer_t* writer, byte_t* data, const size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t buffer_size = cardano_buffer_get_size(writer->buffer);

  if (buffer_size > size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(data, size, cardano_buffer_get_data(writer->buffer), buffer_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_writer_encode_in_buffer(cardano_cbor_writer_t* writer, cardano_buffer_t** buffer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *buffer = cardano_buffer_new(cardano_buffer_get_size(writer->buffer));

  if (*buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_buffer_write(*buffer, cardano_buffer_get_data(writer->buffer), cardano_buffer_get_size(writer->buffer));

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(buffer);
  }

  return result;
}

size_t
cardano_cbor_writer_get_hex_size(const cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return 0U;
  }

  return cardano_buffer_get_hex_size(writer->buffer);
}

cardano_error_t
cardano_cbor_writer_encode_hex(const cardano_cbor_writer_t* writer, char* dest, const size_t dest_size)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (dest == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_to_hex(writer->buffer, dest, dest_size);
}

cardano_error_t
cardano_cbor_writer_reset(cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_unref(&writer->buffer);
  writer->buffer = cardano_buffer_new(128);

  return CARDANO_SUCCESS;
}

void
cardano_cbor_writer_set_last_error(cardano_cbor_writer_t* writer, const char* message)
{
  cardano_object_set_last_error(&writer->base, message);
}

const char*
cardano_cbor_writer_get_last_error(const cardano_cbor_writer_t* writer)
{
  return cardano_object_get_last_error(&writer->base);
}
