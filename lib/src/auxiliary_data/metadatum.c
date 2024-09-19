/**
 * \file metadatum.c
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/metadatum_kind.h>
#include <cardano/auxiliary_data/metadatum_list.h>
#include <cardano/auxiliary_data/metadatum_map.h>
#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano metadatum.
 */
typedef struct cardano_metadatum_t
{
    cardano_object_t          base;
    cardano_metadatum_map_t*  map;
    cardano_metadatum_list_t* list;
    cardano_bigint_t*         integer;
    cardano_buffer_t*         bytes;
    cardano_buffer_t*         text;
    cardano_metadatum_kind_t  kind;
} cardano_metadatum_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a metadatum object.
 *
 * This function is responsible for properly deallocating a metadatum object (`cardano_metadatum_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the metadatum object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_metadatum_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the metadatum
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_metadatum_deallocate(void* object)
{
  assert(object != NULL);

  cardano_metadatum_t* data = (cardano_metadatum_t*)object;

  cardano_metadatum_map_unref(&data->map);
  cardano_metadatum_list_unref(&data->list);
  cardano_buffer_unref(&data->bytes);
  cardano_buffer_unref(&data->text);
  cardano_bigint_unref(&data->integer);

  data->integer = NULL;

  _cardano_free(data);
}

/**
 * \brief Creates a new metadatum object.
 *
 * \return A pointer to the newly created metadatum object, or `NULL` if the operation failed.
 */
static cardano_metadatum_t*
cardano_metadatum_new(void)
{
  cardano_metadatum_t* data = _cardano_malloc(sizeof(cardano_metadatum_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_metadatum_deallocate;

  data->map     = NULL;
  data->list    = NULL;
  data->integer = NULL;
  data->bytes   = NULL;
  data->text    = NULL;
  data->kind    = CARDANO_METADATUM_KIND_MAP;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_metadatum_new_map(
  cardano_metadatum_map_t* map,
  cardano_metadatum_t**    metadatum)
{
  if (map == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_metadatum_t* data = cardano_metadatum_new();

  if (data == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  cardano_metadatum_map_ref(map);

  data->map  = map;
  data->kind = CARDANO_METADATUM_KIND_MAP;

  *metadatum = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_new_list(
  cardano_metadatum_list_t* list,
  cardano_metadatum_t**     metadatum)
{
  if (list == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_metadatum_t* data = cardano_metadatum_new();

  if (data == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  cardano_metadatum_list_ref(list);

  data->list = list;
  data->kind = CARDANO_METADATUM_KIND_LIST;

  *metadatum = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_new_integer(
  const cardano_bigint_t* bigint,
  cardano_metadatum_t**   metadatum)
{
  if (bigint == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_metadatum_t* data = cardano_metadatum_new();

  if (data == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_bigint_clone(bigint, &data->integer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_metadatum_deallocate(data);
    return result;
  }

  data->kind = CARDANO_METADATUM_KIND_INTEGER;

  *metadatum = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_new_integer_from_int(
  const int64_t         integer,
  cardano_metadatum_t** metadatum)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_bigint_t* bigint = NULL;
  cardano_error_t   result = cardano_bigint_from_int(integer, &bigint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_metadatum_new_integer(bigint, metadatum);

  cardano_bigint_unref(&bigint);

  return result;
}

cardano_error_t
cardano_metadatum_new_integer_from_uint(
  const uint64_t        integer,
  cardano_metadatum_t** metadatum)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_bigint_t* bigint = NULL;
  cardano_error_t   result = cardano_bigint_from_unsigned_int(integer, &bigint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_metadatum_new_integer(bigint, metadatum);

  cardano_bigint_unref(&bigint);

  return result;
}

cardano_error_t
cardano_metadatum_new_integer_from_string(
  const char*           string,
  size_t                size,
  int32_t               base,
  cardano_metadatum_t** metadatum)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (string == NULL)
  {
    *metadatum = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_bigint_t* bigint = NULL;
  cardano_error_t   result = cardano_bigint_from_string(string, size, base, &bigint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_metadatum_new_integer(bigint, metadatum);

  cardano_bigint_unref(&bigint);

  return result;
}

cardano_error_t
cardano_metadatum_new_bytes(
  const byte_t*         bytes,
  size_t                size,
  cardano_metadatum_t** metadatum)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (bytes == NULL)
  {
    *metadatum = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_metadatum_t* data = cardano_metadatum_new();

  if (data == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = cardano_buffer_new_from(bytes, size);

  if (buffer == NULL)
  {
    cardano_metadatum_deallocate(data);
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  data->bytes = buffer;
  data->kind  = CARDANO_METADATUM_KIND_BYTES;

  *metadatum = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_new_bytes_from_hex(
  const char*           hex,
  size_t                size,
  cardano_metadatum_t** metadatum)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *metadatum = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_metadatum_t* data = cardano_metadatum_new();

  if (data == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = cardano_buffer_from_hex(hex, size);

  if (buffer == NULL)
  {
    cardano_metadatum_deallocate(data);
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  data->bytes = buffer;
  data->kind  = CARDANO_METADATUM_KIND_BYTES;

  *metadatum = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_new_string(
  const char*           string,
  size_t                size,
  cardano_metadatum_t** metadatum)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (string == NULL)
  {
    *metadatum = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_metadatum_t* data = cardano_metadatum_new();

  if (data == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = cardano_buffer_new_from((const byte_t*)((const void*)string), size);

  if (buffer == NULL)
  {
    cardano_metadatum_deallocate(data);
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  data->text = buffer;
  data->kind = CARDANO_METADATUM_KIND_TEXT;

  *metadatum = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_from_cbor(cardano_cbor_reader_t* reader, cardano_metadatum_t** metadatum)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *metadatum = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_metadatum_t* data = cardano_metadatum_new();

  if (data == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  result = cardano_cbor_reader_peek_state(reader, &state);

  if (result != CARDANO_SUCCESS)
  {
    /* LCOV_EXCL_START */
    cardano_metadatum_deallocate(data);
    return result;
    /* LCOV_EXCL_STOP */
  }

  switch (state)
  {
    case CARDANO_CBOR_READER_STATE_TAG:
    {
      cardano_cbor_tag_t tag = 0;

      result = cardano_cbor_reader_peek_tag(reader, &tag);

      if (result != CARDANO_SUCCESS)
      {
        /* LCOV_EXCL_START */
        cardano_metadatum_deallocate(data);
        return result;
        /* LCOV_EXCL_STOP */
      }

      switch (tag)
      {
        case CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM:
        case CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM:
        {
          result = cardano_cbor_reader_read_bigint(reader, &data->integer);

          if (result != CARDANO_SUCCESS)
          {
            cardano_metadatum_deallocate(data);
            return result;
          }

          data->kind = CARDANO_METADATUM_KIND_INTEGER;

          break;
        }
        // LCOV_EXCL_START
        default:
        {
          cardano_metadatum_deallocate(data);

          cardano_cbor_reader_set_last_error(reader, "Invalid CBOR data item type for metadatum.");
          return CARDANO_ERROR_DECODING;
        }
          // LCOV_EXCL_STOP
      }
      break;
    }
    case CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER:
    {
      uint64_t integer = 0;

      result = cardano_cbor_reader_read_uint(reader, &integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      result = cardano_bigint_from_unsigned_int(integer, &data->integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      data->kind = CARDANO_METADATUM_KIND_INTEGER;

      break;
    }
    case CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER:
    {
      int64_t integer = 0;

      result = cardano_cbor_reader_read_int(reader, &integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      result = cardano_bigint_from_int(integer, &data->integer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      data->kind = CARDANO_METADATUM_KIND_INTEGER;

      break;
    }
    case CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING:
    case CARDANO_CBOR_READER_STATE_BYTESTRING:
    {
      cardano_buffer_t* bytes = NULL;

      result = cardano_cbor_reader_read_bytestring(reader, &bytes);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      data->bytes = bytes;
      data->kind  = CARDANO_METADATUM_KIND_BYTES;

      break;
    }
    case CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING:
    case CARDANO_CBOR_READER_STATE_TEXTSTRING:
    {
      cardano_buffer_t* bytes = NULL;

      result = cardano_cbor_reader_read_textstring(reader, &bytes);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      data->text = bytes;
      data->kind = CARDANO_METADATUM_KIND_TEXT;

      break;
    }
    case CARDANO_CBOR_READER_STATE_START_ARRAY:
    {
      cardano_metadatum_list_t* list = NULL;

      result = cardano_metadatum_list_from_cbor(reader, &list);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      data->list = list;
      data->kind = CARDANO_METADATUM_KIND_LIST;

      break;
    }
    case CARDANO_CBOR_READER_STATE_START_MAP:
    {
      cardano_metadatum_map_t* map = NULL;

      result = cardano_metadatum_map_from_cbor(reader, &map);

      if (result != CARDANO_SUCCESS)
      {
        cardano_metadatum_deallocate(data);
        return result;
      }

      data->map  = map;
      data->kind = CARDANO_METADATUM_KIND_MAP;

      break;
    }
    default:
    {
      cardano_metadatum_deallocate(data);

      cardano_cbor_reader_set_last_error(reader, "Invalid CBOR data item type for metadatum.");
      return CARDANO_ERROR_DECODING;
    }
  }

  *metadatum = data;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_to_cbor(const cardano_metadatum_t* metadatum, cardano_cbor_writer_t* writer)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  switch (metadatum->kind)
  {
    case CARDANO_METADATUM_KIND_MAP:
    {
      result = cardano_metadatum_map_to_cbor(metadatum->map, writer);

      if (result != CARDANO_SUCCESS)
      {
        return result; // LCOV_EXCL_LINE
      }

      break;
    }
    case CARDANO_METADATUM_KIND_LIST:
    {
      result = cardano_metadatum_list_to_cbor(metadatum->list, writer);

      if (result != CARDANO_SUCCESS)
      {
        return result; // LCOV_EXCL_LINE
      }

      break;
    }
    case CARDANO_METADATUM_KIND_INTEGER:
    {
      size_t bit_length = cardano_bigint_bit_length(metadatum->integer);

      if ((cardano_bigint_signum(metadatum->integer) < 0) && (bit_length <= 64U))
      {
        result = cardano_cbor_writer_write_signed_int(writer, cardano_bigint_to_int(metadatum->integer));
      }
      else if (bit_length <= 64U)
      {
        result = cardano_cbor_writer_write_unsigned_int(writer, cardano_bigint_to_unsigned_int(metadatum->integer));
      }
      else
      {
        result = cardano_cbor_writer_write_bigint(writer, metadatum->integer);
      }

      if (result != CARDANO_SUCCESS)
      {
        return result; // LCOV_EXCL_LINE
      }

      break;
    }
    case CARDANO_METADATUM_KIND_BYTES:
    {
      static const uint64_t max_byte_string_chunk_size = 64;

      const size_t size = cardano_buffer_get_size(metadatum->bytes);

      if (size > max_byte_string_chunk_size)
      {
        return CARDANO_ERROR_INVALID_METADATUM_BOUNDED_BYTES_SIZE; // LCOV_EXCL_LINE
      }

      result = cardano_cbor_writer_write_bytestring(writer, cardano_buffer_get_data(metadatum->bytes), size);

      if (result != CARDANO_SUCCESS)
      {
        return result; /* LCOV_EXCL_LINE */
      }

      break;
    }
    case CARDANO_METADATUM_KIND_TEXT:
    {
      static const uint64_t max_text_string_chunk_size = 64;

      const size_t size = cardano_buffer_get_size(metadatum->text);

      if (size > max_text_string_chunk_size)
      {
        return CARDANO_ERROR_INVALID_METADATUM_TEXT_STRING_SIZE; // LCOV_EXCL_LINE
      }

      result = cardano_cbor_writer_write_textstring(writer, (const char*)((const void*)cardano_buffer_get_data(metadatum->text)), size);

      if (result != CARDANO_SUCCESS)
      {
        return result; /* LCOV_EXCL_LINE */
      }

      break;
    }
    /* LCOV_EXCL_START */
    default:
    {
      cardano_cbor_writer_set_last_error(writer, "Invalid metadatum kind");
      return CARDANO_ERROR_ENCODING;
    }
      /* LCOV_EXCL_STOP */
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_get_kind(
  const cardano_metadatum_t* metadatum,
  cardano_metadatum_kind_t*  kind)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (kind == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *kind = metadatum->kind;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_to_map(
  cardano_metadatum_t*      metadatum,
  cardano_metadatum_map_t** map)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (map == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum->kind != CARDANO_METADATUM_KIND_MAP)
  {
    return CARDANO_ERROR_INVALID_METADATUM_CONVERSION;
  }

  cardano_metadatum_map_ref(metadatum->map);
  *map = metadatum->map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_to_list(
  cardano_metadatum_t*       metadatum,
  cardano_metadatum_list_t** list)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (list == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum->kind != CARDANO_METADATUM_KIND_LIST)
  {
    return CARDANO_ERROR_INVALID_METADATUM_CONVERSION;
  }

  cardano_metadatum_list_ref(metadatum->list);
  *list = metadatum->list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_to_integer(
  const cardano_metadatum_t* metadatum,
  cardano_bigint_t**         integer)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (integer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum->kind != CARDANO_METADATUM_KIND_INTEGER)
  {
    return CARDANO_ERROR_INVALID_METADATUM_CONVERSION;
  }

  cardano_error_t error = cardano_bigint_clone(metadatum->integer, integer);

  return error;
}

cardano_error_t
cardano_metadatum_to_bounded_bytes(
  cardano_metadatum_t* metadatum,
  cardano_buffer_t**   bounded_bytes)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (bounded_bytes == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum->kind != CARDANO_METADATUM_KIND_BYTES)
  {
    return CARDANO_ERROR_INVALID_METADATUM_CONVERSION;
  }

  cardano_buffer_ref(metadatum->bytes);
  *bounded_bytes = metadatum->bytes;

  return CARDANO_SUCCESS;
}

size_t
cardano_metadatum_get_string_size(cardano_metadatum_t* metadatum)
{
  if (metadatum == NULL)
  {
    return 0;
  }

  if (metadatum->kind != CARDANO_METADATUM_KIND_TEXT)
  {
    return 0;
  }

  return cardano_buffer_get_size(metadatum->text) + 1U;
}

cardano_error_t
cardano_metadatum_to_string(
  cardano_metadatum_t* metadatum,
  char*                buffer,
  size_t               buffer_size)
{
  if (metadatum == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (metadatum->kind != CARDANO_METADATUM_KIND_TEXT)
  {
    return CARDANO_ERROR_INVALID_METADATUM_CONVERSION;
  }

  size_t size = cardano_buffer_get_size(metadatum->text);

  if ((size + 1U) > buffer_size)
  {
    return CARDANO_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(buffer, size, cardano_buffer_get_data(metadatum->text), cardano_buffer_get_size(metadatum->text));

  buffer[size] = '\0';

  return CARDANO_SUCCESS;
}

bool
cardano_metadatum_equals(const cardano_metadatum_t* lhs, const cardano_metadatum_t* rhs)
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
    case CARDANO_METADATUM_KIND_MAP:
    {
      return cardano_metadatum_map_equals(lhs->map, rhs->map);
    }
    case CARDANO_METADATUM_KIND_LIST:
    {
      return cardano_metadatum_list_equals(lhs->list, rhs->list);
    }
    case CARDANO_METADATUM_KIND_INTEGER:
    {
      return cardano_bigint_equals(lhs->integer, rhs->integer);
    }
    case CARDANO_METADATUM_KIND_BYTES:
    {
      return cardano_buffer_equals(lhs->bytes, rhs->bytes);
    }
    case CARDANO_METADATUM_KIND_TEXT:
    {
      return cardano_buffer_equals(lhs->text, rhs->text);
    }
    /* LCOV_EXCL_START */
    default:
    {
      return false;
    }
      /* LCOV_EXCL_STOP */
  }
}

void
cardano_metadatum_unref(cardano_metadatum_t** metadatum)
{
  if ((metadatum == NULL) || (*metadatum == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*metadatum)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *metadatum = NULL;
    return;
  }
}

void
cardano_metadatum_ref(cardano_metadatum_t* metadatum)
{
  if (metadatum == NULL)
  {
    return;
  }

  cardano_object_ref(&metadatum->base);
}

size_t
cardano_metadatum_refcount(const cardano_metadatum_t* metadatum)
{
  if (metadatum == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&metadatum->base);
}

void
cardano_metadatum_set_last_error(cardano_metadatum_t* metadatum, const char* message)
{
  cardano_object_set_last_error(&metadatum->base, message);
}

const char*
cardano_metadatum_get_last_error(const cardano_metadatum_t* metadatum)
{
  return cardano_object_get_last_error(&metadatum->base);
}
