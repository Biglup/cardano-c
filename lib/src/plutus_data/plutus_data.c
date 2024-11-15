/**
 * \file plutus_data.c
 *
 * \author angel.castillo
 * \date   May 12, 2024
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

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/object.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_data_kind.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include "../allocators.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano plutus data.
 */
typedef struct cardano_plutus_data_t
{
    cardano_object_t              base;
    cardano_plutus_map_t*         map;
    cardano_plutus_list_t*        list;
    cardano_bigint_t*             integer;
    cardano_buffer_t*             bytes;
    cardano_constr_plutus_data_t* constr;
    cardano_plutus_data_kind_t    kind;
    cardano_buffer_t*             cbor_cache;
} cardano_plutus_data_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a plutus data object.
 *
 * This function is responsible for properly deallocating a plutus data object (`cardano_plutus_data_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the plutus_data object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_plutus_data_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the plutus_data
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_plutus_data_deallocate(void* object)
{
  assert(object != NULL);

  cardano_plutus_data_t* data = (cardano_plutus_data_t*)object;

  cardano_plutus_map_unref(&data->map);
  cardano_plutus_list_unref(&data->list);
  cardano_buffer_unref(&data->bytes);
  cardano_constr_plutus_data_unref(&data->constr);
  cardano_bigint_unref(&data->integer);
  cardano_buffer_unref(&data->cbor_cache);

  data->integer = NULL;

  _cardano_free(data);
}

/**
 * \brief Creates a new plutus data object.
 *
 * \return A pointer to the newly created plutus data object, or `NULL` if the operation failed.
 */
static cardano_plutus_data_t*
cardano_plutus_data_new(void)
{
  cardano_plutus_data_t* data = _cardano_malloc(sizeof(cardano_plutus_data_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_plutus_data_deallocate;
  data->cbor_cache         = NULL;

  data->map     = NULL;
  data->list    = NULL;
  data->integer = NULL;
  data->bytes   = NULL;
  data->constr  = NULL;
  data->kind    = CARDANO_PLUTUS_DATA_KIND_CONSTR;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_plutus_data_new_constr(
  cardano_constr_plutus_data_t* constr,
  cardano_plutus_data_t**       plutus_data)
{
  if (constr == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* data = cardano_plutus_data_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_constr_plutus_data_ref(constr);

  data->constr = (cardano_constr_plutus_data_t*)constr;
  data->kind   = CARDANO_PLUTUS_DATA_KIND_CONSTR;

  *plutus_data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_new_map(
  cardano_plutus_map_t*   map,
  cardano_plutus_data_t** plutus_data)
{
  if (map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* data = cardano_plutus_data_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_plutus_map_ref(map);

  data->map  = map;
  data->kind = CARDANO_PLUTUS_DATA_KIND_MAP;

  *plutus_data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_new_list(
  cardano_plutus_list_t*  list,
  cardano_plutus_data_t** plutus_data)
{
  if (list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* data = cardano_plutus_data_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_plutus_list_ref(list);

  data->list = list;
  data->kind = CARDANO_PLUTUS_DATA_KIND_LIST;

  *plutus_data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_new_integer(
  const cardano_bigint_t* bigint,
  cardano_plutus_data_t** plutus_data)
{
  if (bigint == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* data = cardano_plutus_data_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_bigint_clone(bigint, &data->integer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_data_deallocate(data);
    return result;
  }

  data->kind = CARDANO_PLUTUS_DATA_KIND_INTEGER;

  *plutus_data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_new_integer_from_int(
  const int64_t           integer,
  cardano_plutus_data_t** plutus_data)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bigint_t* bigint = NULL;
  cardano_error_t   result = cardano_bigint_from_int(integer, &bigint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_data_new_integer(bigint, plutus_data);

  cardano_bigint_unref(&bigint);

  return result;
}

cardano_error_t
cardano_plutus_data_new_integer_from_uint(
  const uint64_t          integer,
  cardano_plutus_data_t** plutus_data)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bigint_t* bigint = NULL;
  cardano_error_t   result = cardano_bigint_from_unsigned_int(integer, &bigint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_data_new_integer(bigint, plutus_data);

  cardano_bigint_unref(&bigint);

  return result;
}

cardano_error_t
cardano_plutus_data_new_integer_from_string(
  const char*             string,
  size_t                  size,
  int32_t                 base,
  cardano_plutus_data_t** plutus_data)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (string == NULL)
  {
    *plutus_data = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bigint_t* bigint = NULL;
  cardano_error_t   result = cardano_bigint_from_string(string, size, base, &bigint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_data_new_integer(bigint, plutus_data);

  cardano_bigint_unref(&bigint);

  return result;
}

cardano_error_t
cardano_plutus_data_new_bytes(
  const byte_t*           bytes,
  size_t                  size,
  cardano_plutus_data_t** plutus_data)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bytes == NULL)
  {
    *plutus_data = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* data = cardano_plutus_data_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = cardano_buffer_new_from(bytes, size);

  if (buffer == NULL)
  {
    cardano_plutus_data_deallocate(data);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->bytes = buffer;
  data->kind  = CARDANO_PLUTUS_DATA_KIND_BYTES;

  *plutus_data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_new_bytes_from_hex(
  const char*             hex,
  size_t                  size,
  cardano_plutus_data_t** plutus_data)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *plutus_data = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* data = cardano_plutus_data_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = cardano_buffer_from_hex(hex, size);

  if (buffer == NULL)
  {
    cardano_plutus_data_deallocate(data);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->bytes = buffer;
  data->kind  = CARDANO_PLUTUS_DATA_KIND_BYTES;

  *plutus_data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_data_t** plutus_data)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *plutus_data = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t*      cbor_cache  = NULL;
  cardano_cbor_reader_t* reader_copy = NULL;
  cardano_error_t        copy_result = cardano_cbor_reader_clone(reader, &reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    return copy_result;
  }

  copy_result = cardano_cbor_reader_read_encoded_value(reader_copy, &cbor_cache);
  cardano_cbor_reader_unref(&reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    *plutus_data = NULL;
    return copy_result;
  }

  cardano_plutus_data_t* data = cardano_plutus_data_new();

  if (data == NULL)
  {
    cardano_buffer_unref(&cbor_cache);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->cbor_cache = cbor_cache;

  cardano_error_t result = CARDANO_SUCCESS;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  result = cardano_cbor_reader_peek_state(reader, &state);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_data_deallocate(data);
    return result;
  }

  switch (state)
  {
    case CARDANO_CBOR_READER_STATE_TAG:
    {
      cardano_cbor_tag_t tag = 0;

      result = cardano_cbor_reader_peek_tag(reader, &tag);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      switch (tag)
      {
        case CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM:
        case CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM:
        {
          result = cardano_cbor_reader_read_bigint(reader, &data->integer);

          if (result != CARDANO_SUCCESS)
          {
            cardano_plutus_data_deallocate(data);
            return result;
          }

          data->kind = CARDANO_PLUTUS_DATA_KIND_INTEGER;

          break;
        }
        default:
        {
          result = cardano_constr_plutus_data_from_cbor(reader, &data->constr);

          if (result != CARDANO_SUCCESS)
          {
            cardano_plutus_data_deallocate(data);
            return result;
          }

          data->kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;

          break;
        }
      }
      break;
    }
    case CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER:
    {
      uint64_t integer = 0;

      result = cardano_cbor_reader_read_uint(reader, &integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      result = cardano_bigint_from_unsigned_int(integer, &data->integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      data->kind = CARDANO_PLUTUS_DATA_KIND_INTEGER;

      break;
    }
    case CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER:
    {
      int64_t integer = 0;

      result = cardano_cbor_reader_read_int(reader, &integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      result = cardano_bigint_from_int(integer, &data->integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      data->kind = CARDANO_PLUTUS_DATA_KIND_INTEGER;

      break;
    }
    case CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING:
    case CARDANO_CBOR_READER_STATE_BYTESTRING:
    {
      cardano_buffer_t* bytes = NULL;

      result = cardano_cbor_reader_read_bytestring(reader, &bytes);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      data->bytes = bytes;
      data->kind  = CARDANO_PLUTUS_DATA_KIND_BYTES;

      break;
    }
    case CARDANO_CBOR_READER_STATE_START_ARRAY:
    {
      cardano_plutus_list_t* list = NULL;

      result = cardano_plutus_list_from_cbor(reader, &list);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      data->list = list;
      data->kind = CARDANO_PLUTUS_DATA_KIND_LIST;

      break;
    }
    case CARDANO_CBOR_READER_STATE_START_MAP:
    {
      cardano_plutus_map_t* map = NULL;

      result = cardano_plutus_map_from_cbor(reader, &map);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_deallocate(data);
        return result;
      }

      data->map  = map;
      data->kind = CARDANO_PLUTUS_DATA_KIND_MAP;

      break;
    }
    default:
    {
      cardano_plutus_data_deallocate(data);

      cardano_cbor_reader_set_last_error(reader, "Invalid CBOR data item type for plutus data.");
      return CARDANO_ERROR_DECODING;
    }
  }

  *plutus_data = data;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_to_cbor(const cardano_plutus_data_t* plutus_data, cardano_cbor_writer_t* writer)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(plutus_data->cbor_cache), cardano_buffer_get_size(plutus_data->cbor_cache));
  }

  cardano_error_t result = CARDANO_SUCCESS;

  switch (plutus_data->kind)
  {
    case CARDANO_PLUTUS_DATA_KIND_CONSTR:
    {
      result = cardano_constr_plutus_data_to_cbor(plutus_data->constr, writer);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    case CARDANO_PLUTUS_DATA_KIND_MAP:
    {
      result = cardano_plutus_map_to_cbor(plutus_data->map, writer);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    case CARDANO_PLUTUS_DATA_KIND_LIST:
    {
      result = cardano_plutus_list_to_cbor(plutus_data->list, writer);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    case CARDANO_PLUTUS_DATA_KIND_INTEGER:
    {
      size_t bit_length = cardano_bigint_bit_length(plutus_data->integer);

      if ((cardano_bigint_signum(plutus_data->integer) < 0) && (bit_length <= 64U))
      {
        result = cardano_cbor_writer_write_signed_int(writer, cardano_bigint_to_int(plutus_data->integer));
      }
      else if (bit_length <= 64U)
      {
        result = cardano_cbor_writer_write_uint(writer, cardano_bigint_to_unsigned_int(plutus_data->integer));
      }
      else
      {
        const size_t size  = cardano_bigint_get_bytes_size(plutus_data->integer);
        byte_t*      bytes = _cardano_malloc(size);

        result = cardano_bigint_to_bytes(plutus_data->integer, CARDANO_BYTE_ORDER_BIG_ENDIAN, bytes, size);

        if (result != CARDANO_SUCCESS)
        {
          _cardano_free(bytes);
          return result;
        }

        result = cardano_cbor_writer_write_tag(writer, (cardano_bigint_signum(plutus_data->integer) < 0) ? CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM : CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM);

        if (result != CARDANO_SUCCESS)
        {
          _cardano_free(bytes);
          return result;
        }

        static const uint64_t max_byte_string_chunk_size = 64;

        if (size <= max_byte_string_chunk_size)
        {
          result = cardano_cbor_writer_write_bytestring(writer, bytes, size);
          _cardano_free(bytes);

          return result;
        }

        static const byte_t indefinite_byte_string = 95;

        const size_t chunks = size / max_byte_string_chunk_size;
        const size_t rest   = size % max_byte_string_chunk_size;
        result              = cardano_cbor_writer_write_encoded(writer, &indefinite_byte_string, sizeof(indefinite_byte_string));

        if (result != CARDANO_SUCCESS)
        {
          _cardano_free(bytes);
          return result;
        }

        for (size_t i = 0; i < chunks; ++i)
        {
          result = cardano_cbor_writer_write_bytestring(
            writer, &bytes[i * max_byte_string_chunk_size], max_byte_string_chunk_size);

          if (result != CARDANO_SUCCESS)
          {
            _cardano_free(bytes);
            return result;
          }
        }

        if (rest > 0U)
        {
          result = cardano_cbor_writer_write_bytestring(
            writer, &bytes[chunks * max_byte_string_chunk_size], rest);

          if (result != CARDANO_SUCCESS)
          {
            _cardano_free(bytes);
            return result;
          }
        }

        _cardano_free(bytes);
        result = cardano_cbor_writer_write_end_array(writer);
      }

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }
    // Note [The 64-byte limit]: See https://github.com/input-output-hk/plutus/blob/1f31e640e8a258185db01fa899da63f9018c0e85/plutus-core/plutus-core/src/PlutusCore/Data.hs#L61-L105
    // If the bytestring is >64bytes, we encode it as indefinite-length bytestrings with 64-byte chunks. We have to write
    // our own encoders/decoders so we can produce chunks of the right size and check
    // the sizes when we decode.
    case CARDANO_PLUTUS_DATA_KIND_BYTES:
    {
      static const uint64_t max_byte_string_chunk_size = 64;

      const size_t size = cardano_buffer_get_size(plutus_data->bytes);

      if (size <= max_byte_string_chunk_size)
      {
        result = cardano_cbor_writer_write_bytestring(writer, cardano_buffer_get_data(plutus_data->bytes), size);
      }
      else
      {
        static const byte_t indefinite_byte_string = 95;

        const size_t chunks = size / max_byte_string_chunk_size;
        const size_t rest   = size % max_byte_string_chunk_size;
        result              = cardano_cbor_writer_write_encoded(writer, &indefinite_byte_string, sizeof(indefinite_byte_string));

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        for (size_t i = 0; i < chunks; ++i)
        {
          result = cardano_cbor_writer_write_bytestring(
            writer, &cardano_buffer_get_data(plutus_data->bytes)[i * max_byte_string_chunk_size], max_byte_string_chunk_size);

          if (result != CARDANO_SUCCESS)
          {
            return result;
          }
        }

        if (rest > 0U)
        {
          result = cardano_cbor_writer_write_bytestring(
            writer, &cardano_buffer_get_data(plutus_data->bytes)[chunks * max_byte_string_chunk_size], rest);

          if (result != CARDANO_SUCCESS)
          {
            return result;
          }
        }

        result = cardano_cbor_writer_write_end_array(writer);
      }

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      break;
    }

    default:
    {
      cardano_cbor_writer_set_last_error(writer, "Invalid plutus data kind");
      return CARDANO_ERROR_ENCODING;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_get_kind(
  const cardano_plutus_data_t* plutus_data,
  cardano_plutus_data_kind_t*  kind)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (kind == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *kind = plutus_data->kind;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_to_constr(
  cardano_plutus_data_t*         plutus_data,
  cardano_constr_plutus_data_t** constr)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constr == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data->kind != CARDANO_PLUTUS_DATA_KIND_CONSTR)
  {
    return CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION;
  }

  cardano_constr_plutus_data_ref(plutus_data->constr);
  *constr = plutus_data->constr;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_to_map(
  cardano_plutus_data_t* plutus_data,
  cardano_plutus_map_t** map)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data->kind != CARDANO_PLUTUS_DATA_KIND_MAP)
  {
    return CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION;
  }

  cardano_plutus_map_ref(plutus_data->map);
  *map = plutus_data->map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_to_list(
  cardano_plutus_data_t*  plutus_data,
  cardano_plutus_list_t** list)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data->kind != CARDANO_PLUTUS_DATA_KIND_LIST)
  {
    return CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION;
  }

  cardano_plutus_list_ref(plutus_data->list);
  *list = plutus_data->list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_data_to_integer(
  const cardano_plutus_data_t* plutus_data,
  cardano_bigint_t**           integer)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (integer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data->kind != CARDANO_PLUTUS_DATA_KIND_INTEGER)
  {
    return CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION;
  }

  cardano_error_t error = cardano_bigint_clone(plutus_data->integer, integer);

  return error;
}

cardano_error_t
cardano_plutus_data_to_bounded_bytes(
  cardano_plutus_data_t* plutus_data,
  cardano_buffer_t**     bounded_bytes)
{
  if (plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bounded_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_data->kind != CARDANO_PLUTUS_DATA_KIND_BYTES)
  {
    return CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION;
  }

  cardano_buffer_ref(plutus_data->bytes);
  *bounded_bytes = plutus_data->bytes;

  return CARDANO_SUCCESS;
}

bool
cardano_plutus_data_equals(const cardano_plutus_data_t* lhs, const cardano_plutus_data_t* rhs)
{
  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (lhs == rhs)
  {
    return true;
  }

  if (lhs->kind != rhs->kind)
  {
    return false;
  }

  switch (lhs->kind)
  {
    case CARDANO_PLUTUS_DATA_KIND_CONSTR:
    {
      return cardano_constr_plutus_equals(lhs->constr, rhs->constr);
    }
    case CARDANO_PLUTUS_DATA_KIND_MAP:
    {
      return cardano_plutus_map_equals(lhs->map, rhs->map);
    }
    case CARDANO_PLUTUS_DATA_KIND_LIST:
    {
      return cardano_plutus_list_equals(lhs->list, rhs->list);
    }
    case CARDANO_PLUTUS_DATA_KIND_INTEGER:
    {
      return cardano_bigint_equals(lhs->integer, rhs->integer);
    }
    case CARDANO_PLUTUS_DATA_KIND_BYTES:
    {
      return cardano_buffer_equals(lhs->bytes, rhs->bytes);
    }

    default:
    {
      return false;
    }
  }
}

void
cardano_plutus_data_clear_cbor_cache(cardano_plutus_data_t* plutus_data)
{
  if (plutus_data == NULL)
  {
    return;
  }

  cardano_buffer_unref(&plutus_data->cbor_cache);
  cardano_plutus_list_clear_cbor_cache(plutus_data->list);
  cardano_plutus_map_clear_cbor_cache(plutus_data->map);
  cardano_constr_plutus_data_clear_cbor_cache(plutus_data->constr);

  plutus_data->cbor_cache = NULL;
}

void
cardano_plutus_data_unref(cardano_plutus_data_t** plutus_data)
{
  if ((plutus_data == NULL) || (*plutus_data == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*plutus_data)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *plutus_data = NULL;
    return;
  }
}

void
cardano_plutus_data_ref(cardano_plutus_data_t* plutus_data)
{
  if (plutus_data == NULL)
  {
    return;
  }

  cardano_object_ref(&plutus_data->base);
}

size_t
cardano_plutus_data_refcount(const cardano_plutus_data_t* plutus_data)
{
  if (plutus_data == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&plutus_data->base);
}

void
cardano_plutus_data_set_last_error(cardano_plutus_data_t* plutus_data, const char* message)
{
  cardano_object_set_last_error(&plutus_data->base, message);
}

const char*
cardano_plutus_data_get_last_error(const cardano_plutus_data_t* plutus_data)
{
  return cardano_object_get_last_error(&plutus_data->base);
}
