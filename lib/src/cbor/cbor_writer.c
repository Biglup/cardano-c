/**
 * \file cbor_writer.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * \section LICENSE
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

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_major_type.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/object.h>

#include <stdlib.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

typedef struct cardano_cbor_writer_t
{
    cardano_object_t  base;
    cardano_buffer_t* buffer;
} cardano_cbor_writer_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * Writes a typed value to the buffer.
 *
 * \param major_type The major type of the value.
 * \param value The value.
 */
static cardano_error_t
write_type_value(cardano_buffer_t* buffer, const cbor_major_type_t major_type, const uint64_t value)
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
    byte_t          header = type | 24U;
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
    byte_t          header = type | 25U;
    cardano_error_t result = cardano_buffer_write(buffer, &header, sizeof(header));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    return cardano_buffer_write_uint16_be(buffer, (uint16_t)value);
  }
  else if (value < 4294967296U)
  {
    byte_t          header = type | 26U;
    cardano_error_t result = cardano_buffer_write(buffer, &header, sizeof(header));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    return cardano_buffer_write_uint32_be(buffer, (uint32_t)value);
  }
  else
  {
    byte_t          header = type | 27U;
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
  cardano_cbor_writer_t* cbor_writer = (cardano_cbor_writer_t*)object;

  if (cbor_writer == NULL)
  {
    return;
  }

  if (cbor_writer->buffer != NULL)
  {
    cardano_buffer_unref(&cbor_writer->buffer);
  }

  free(cbor_writer);
}

/* DECLARATIONS **************************************************************/

cardano_cbor_writer_t*
cardano_cbor_writer_new(void)
{
  cardano_cbor_writer_t* obj = (cardano_cbor_writer_t*)malloc(sizeof(cardano_cbor_writer_t));

  if (obj == NULL)
  {
    return NULL;
  }

  obj->base.ref_count   = 1;
  obj->base.deallocator = cardano_cbor_writer_deallocate;
  obj->buffer           = cardano_buffer_new(128);

  if (obj->buffer == NULL)
  {
    free(obj);
    return NULL;
  }

  return obj;
}

void
cardano_cbor_writer_unref(cardano_cbor_writer_t** cbor_writer)
{
  cardano_object_unref((cardano_object_t**)cbor_writer);
}

void
cardano_cbor_writer_ref(cardano_cbor_writer_t* cbor_writer)
{
  cardano_object_ref((cardano_object_t*)cbor_writer);
}

size_t
cardano_cbor_writer_refcount(const cardano_cbor_writer_t* cbor_writer)
{
  return cardano_object_refcount((const cardano_object_t*)cbor_writer);
}

cardano_cbor_writer_t*
cardano_cbor_writer_move(cardano_cbor_writer_t* cbor_writer)
{
  return (cardano_cbor_writer_t*)cardano_object_move((cardano_object_t*)cbor_writer);
}

cardano_error_t
cardano_cbor_writer_write_big_integer(cardano_cbor_writer_t* writer, uint64_t value)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_cbor_writer_write_tag(writer, CBOR_TAG_UNSIGNED_BIG_NUM);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_cbor_writer_write_unsigned_int(writer, value);
}

cardano_error_t
cardano_cbor_writer_write_bool(cardano_cbor_writer_t* writer, bool value)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  byte_t cbor_bool_val = (byte_t)(value ? 245 : 244);

  return cardano_buffer_write(writer->buffer, &cbor_bool_val, 1);
}

cardano_error_t
cardano_cbor_writer_write_byte_string(cardano_cbor_writer_t* writer, byte_t* data, size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_error_t result = write_type_value(writer->buffer, CBOR_MAJOR_TYPE_BYTE_STRING, size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_buffer_write(writer->buffer, data, size);
}

cardano_error_t
cardano_cbor_writer_write_text_string(cardano_cbor_writer_t* writer, const char* data, size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_error_t result = write_type_value(writer->buffer, CBOR_MAJOR_TYPE_UTF8_STRING, size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_buffer_write(writer->buffer, (const byte_t*)data, size);
}

cardano_error_t
cardano_cbor_writer_write_encoded(cardano_cbor_writer_t* writer, byte_t* data, size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  return cardano_buffer_write(writer->buffer, data, size);
}

cardano_error_t
cardano_cbor_writer_write_start_array(cardano_cbor_writer_t* writer, size_t size)
{
  static byte_t indefinite_length_array = 0x9fU;

  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (size == 0U)
  {
    return cardano_buffer_write(writer->buffer, &indefinite_length_array, sizeof(indefinite_length_array));
  }

  return write_type_value(writer->buffer, CBOR_MAJOR_TYPE_ARRAY, size);
}

cardano_error_t
cardano_cbor_writer_write_end_array(cardano_cbor_writer_t* writer)
{
  static byte_t indefiniteLengthBreakByte = 0xffU;

  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  return cardano_buffer_write(writer->buffer, &indefiniteLengthBreakByte, sizeof(indefiniteLengthBreakByte));
}

cardano_error_t
cardano_cbor_writer_write_start_map(cardano_cbor_writer_t* writer, size_t size)
{
  static byte_t indefinite_length_map = 0xbfU;

  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (size == 0U)
  {
    return cardano_buffer_write(writer->buffer, &indefinite_length_map, sizeof(indefinite_length_map));
  }

  return write_type_value(writer->buffer, CBOR_MAJOR_TYPE_MAP, size);
}

cardano_error_t
cardano_cbor_writer_write_end_map(cardano_cbor_writer_t* writer)
{
  return cardano_cbor_writer_write_end_array(writer);
}

cardano_error_t
cardano_cbor_writer_write_unsigned_int(cardano_cbor_writer_t* writer, uint64_t value)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  return write_type_value(writer->buffer, CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value);
}

cardano_error_t
cardano_cbor_writer_write_signed_int(cardano_cbor_writer_t* writer, int64_t value)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value < 0)
  {
    return write_type_value(writer->buffer, CBOR_MAJOR_TYPE_NEGATIVE_INTEGER, -1 - value);
  }

  return write_type_value(writer->buffer, CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, value);
}

cardano_error_t
cardano_cbor_writer_write_null(cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  static byte_t cbor_null = 0xf6U;

  return cardano_buffer_write(writer->buffer, &cbor_null, sizeof(cbor_null));
}

cardano_error_t
cardano_cbor_writer_write_undefined(cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  static byte_t cbor_undefined = 0xf7U;

  return cardano_buffer_write(writer->buffer, &cbor_undefined, sizeof(cbor_undefined));
}

cardano_error_t
cardano_cbor_writer_write_tag(cardano_cbor_writer_t* writer, cbor_tag_t tag)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  return write_type_value(writer->buffer, CBOR_MAJOR_TYPE_TAG, tag);
}

cardano_error_t
cardano_cbor_writer_encode(cardano_cbor_writer_t* writer, byte_t* data, size_t size, size_t* written)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t buffer_size = cardano_buffer_get_size(writer->buffer);

  if (buffer_size > size)
  {
    return CARDANO_INSUFFICIENT_BUFFER_SIZE;
  }

  (void)memcpy(data, cardano_buffer_get_data(writer->buffer), buffer_size);

  *written = buffer_size;

  return CARDANO_SUCCESS;
}

char*
cardano_cbor_writer_encode_hex(cardano_cbor_writer_t* writer)
{
  return cardano_buffer_to_hex(writer->buffer);
}

cardano_error_t
cardano_cbor_writer_reset(cardano_cbor_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_buffer_unref(&writer->buffer);
  writer->buffer = cardano_buffer_new(128);

  return CARDANO_SUCCESS;
}

void
cardano_cbor_writer_set_last_error(cardano_cbor_writer_t* writer, const char* message)
{
  cardano_object_set_last_error((cardano_object_t*)writer, message);
}

const char*
cardano_cbor_writer_get_last_error(const cardano_cbor_writer_t* writer)
{
  return cardano_object_get_last_error((const cardano_object_t*)writer);
}