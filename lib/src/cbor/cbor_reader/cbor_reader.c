/**
 * \file cbor_reader.c
 *
 * \author luisd.bianchi
 * \date   Mar 14, 2024
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

#include "../../collections/array.h"
#include <cardano/allocators.h>
#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_reader_state.h>
#include <cardano/cbor/cbor_simple_value.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

#include "cbor_reader_collections.h"
#include "cbor_reader_core.h"
#include "cbor_reader_numeric.h"
#include "cbor_reader_simple_values.h"
#include "cbor_reader_tags.h"

#include <assert.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a CBOR reader object.
 *
 * This function is responsible for properly deallocating a CBOR reader object (`cardano_cbor_reader_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the CBOR reader object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_cbor_reader_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the CBOR reader
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_cbor_reader_deallocate(void* object)
{
  assert(object != NULL);

  cardano_cbor_reader_t* cbor_reader = (cardano_cbor_reader_t*)object;

  if (cbor_reader->buffer != NULL)
  {
    cardano_buffer_unref(&cbor_reader->buffer);
  }

  if (cbor_reader->nested_items != NULL)
  {
    cardano_array_unref(&cbor_reader->nested_items);
  }

  _cardano_free(cbor_reader);
}

/* DECLARATIONS **************************************************************/

cardano_cbor_reader_t*
cardano_cbor_reader_new(const byte_t* cbor_data, const size_t size)
{
  if (cbor_data == NULL)
  {
    return NULL;
  }

  if (size == 0U)
  {
    return NULL;
  }

  cardano_cbor_reader_t* obj = (cardano_cbor_reader_t*)_cardano_malloc(sizeof(cardano_cbor_reader_t));

  if (obj == NULL)
  {
    return NULL;
  }

  obj->base.ref_count     = 1;
  obj->base.deallocator   = cardano_cbor_reader_deallocate;
  obj->base.last_error[0] = '\0';
  obj->buffer             = cardano_buffer_new_from(cbor_data, size);

  obj->offset                           = 0;
  obj->nested_items                     = cardano_array_new(32);
  obj->is_tag_context                   = false;
  obj->cached_state                     = CBOR_READER_STATE_UNDEFINED;
  obj->current_frame.type               = CBOR_MAJOR_TYPE_UNDEFINED;
  obj->current_frame.current_key_offset = -1;
  obj->current_frame.frame_offset       = 0;
  obj->current_frame.items_read         = 0;
  obj->current_frame.definite_length    = -1;

  if ((obj->buffer == NULL) || (obj->nested_items == NULL))
  {
    cardano_array_unref(&obj->nested_items);
    cardano_buffer_unref(&obj->buffer);
    _cardano_free(obj);

    return NULL;
  }

  return obj;
}

cardano_cbor_reader_t*
cardano_cbor_reader_from_hex(const char* hex_string, const size_t size)
{
  if (hex_string == NULL)
  {
    return NULL;
  }

  if (size == 0U)
  {
    return NULL;
  }

  cardano_cbor_reader_t* obj = (cardano_cbor_reader_t*)_cardano_malloc(sizeof(cardano_cbor_reader_t));

  if (obj == NULL)
  {
    return NULL;
  }

  obj->base.ref_count     = 1;
  obj->base.deallocator   = cardano_cbor_reader_deallocate;
  obj->base.last_error[0] = '\0';
  obj->buffer             = cardano_buffer_from_hex(hex_string, size);

  obj->offset                           = 0;
  obj->nested_items                     = cardano_array_new(32);
  obj->is_tag_context                   = false;
  obj->cached_state                     = CBOR_READER_STATE_UNDEFINED;
  obj->current_frame.type               = CBOR_MAJOR_TYPE_UNDEFINED;
  obj->current_frame.current_key_offset = -1;
  obj->current_frame.frame_offset       = 0;
  obj->current_frame.items_read         = 0;
  obj->current_frame.definite_length    = -1;

  if ((obj->buffer == NULL) || (obj->nested_items == NULL))
  {
    cardano_array_unref(&obj->nested_items);
    cardano_buffer_unref(&obj->buffer);
    _cardano_free(obj);

    return NULL;
  }

  return obj;
}

void
cardano_cbor_reader_unref(cardano_cbor_reader_t** cbor_reader)
{
  if ((cbor_reader == NULL) || (*cbor_reader == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*cbor_reader)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *cbor_reader = NULL;
    return;
  }
}

void
cardano_cbor_reader_ref(cardano_cbor_reader_t* cbor_reader)
{
  if (cbor_reader == NULL)
  {
    return;
  }

  cardano_object_ref(&cbor_reader->base);
}

size_t
cardano_cbor_reader_refcount(const cardano_cbor_reader_t* cbor_reader)
{
  if (cbor_reader == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&cbor_reader->base);
}

cardano_cbor_reader_t*
cardano_cbor_reader_move(cardano_cbor_reader_t* cbor_reader)
{
  if (cbor_reader == NULL)
  {
    return NULL;
  }

  cardano_object_t* object = cardano_object_move(&cbor_reader->base);

  CARDANO_UNUSED(object);

  return cbor_reader;
}

cardano_error_t
cardano_cbor_reader_peek_state(cardano_cbor_reader_t* reader, cbor_reader_state_t* state)
{
  return _cbor_reader_peek_state(reader, state);
}

cardano_error_t
cardano_cbor_reader_get_bytes_remaining(cardano_cbor_reader_t* reader, size_t* bytes_remaining)
{
  if (reader == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (bytes_remaining == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *bytes_remaining = cardano_buffer_get_size(reader->buffer) - reader->offset;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_reader_get_remainder_bytes(cardano_cbor_reader_t* reader, cardano_buffer_t** remainder_bytes)
{
  if (reader == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (remainder_bytes == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_buffer_t* slice = cardano_buffer_slice(reader->buffer, reader->offset, cardano_buffer_get_size(reader->buffer));

  if (slice == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  *remainder_bytes = slice;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_reader_skip_value(cardano_cbor_reader_t* reader)
{
  cardano_buffer_t* encoded_value = NULL;
  cardano_error_t   result        = cardano_cbor_reader_read_encoded_value(reader, &encoded_value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_buffer_unref(&encoded_value);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_reader_read_encoded_value(cardano_cbor_reader_t* reader, cardano_buffer_t** encoded_value)
{
  if (reader == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (encoded_value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  size_t initial_offset = reader->offset;
  size_t depth          = 0;

  do
  {
    cardano_error_t result = _cbor_reader_skip_next_node(reader, &depth);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }
  while (depth > 0U);

  *encoded_value = cardano_buffer_slice(reader->buffer, initial_offset, reader->offset);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_reader_read_start_array(cardano_cbor_reader_t* reader, int64_t* size)
{
  return _cbor_reader_read_start_array(reader, size);
}

cardano_error_t
cardano_cbor_reader_read_end_array(cardano_cbor_reader_t* reader)
{
  return _cbor_reader_read_end_array(reader);
}

cardano_error_t
cardano_cbor_reader_read_int(cardano_cbor_reader_t* reader, int64_t* value)
{
  return _cbor_reader_read_int(reader, value);
}

cardano_error_t
cardano_cbor_reader_read_uint(cardano_cbor_reader_t* reader, uint64_t* value)
{
  return _cbor_reader_read_uint(reader, value);
}

cardano_error_t
cardano_cbor_reader_read_double(cardano_cbor_reader_t* reader, double* value)
{
  return _cbor_reader_read_double(reader, value);
}

cardano_error_t
cardano_cbor_reader_read_simple_value(cardano_cbor_reader_t* reader, cbor_simple_value_t* value)
{
  return _cbor_reader_read_simple_value(reader, value);
}

cardano_error_t
cardano_cbor_reader_read_start_map(cardano_cbor_reader_t* reader, int64_t* size)
{
  return _cbor_reader_read_start_map(reader, size);
}

cardano_error_t
cardano_cbor_reader_read_end_map(cardano_cbor_reader_t* reader)
{
  return _cbor_reader_read_end_map(reader);
}

cardano_error_t
cardano_cbor_reader_read_boolean(cardano_cbor_reader_t* reader, bool* value)
{
  return _cbor_reader_read_boolean(reader, value);
}

cardano_error_t
cardano_cbor_reader_read_null(cardano_cbor_reader_t* reader)
{
  return _cbor_reader_read_null(reader);
}

cardano_error_t
cardano_cbor_reader_read_bytestring(cardano_cbor_reader_t* reader, cardano_buffer_t** byte_string)
{
  return _cbor_reader_read_string(reader, CBOR_MAJOR_TYPE_BYTE_STRING, byte_string);
}

cardano_error_t
cardano_cbor_reader_read_textstring(cardano_cbor_reader_t* reader, cardano_buffer_t** text_string)
{
  return _cbor_reader_read_string(reader, CBOR_MAJOR_TYPE_UTF8_STRING, text_string);
}

cardano_error_t
cardano_cbor_reader_read_tag(cardano_cbor_reader_t* reader, cbor_tag_t* tag)
{
  return _cbor_reader_read_tag(reader, tag);
}

cardano_error_t
cardano_cbor_reader_peek_tag(cardano_cbor_reader_t* reader, cbor_tag_t* tag)
{
  return _cbor_reader_peek_tag(reader, tag);
}

void
cardano_cbor_reader_set_last_error(cardano_cbor_reader_t* reader, const char* message)
{
  cardano_object_set_last_error(&reader->base, message);
}

const char*
cardano_cbor_reader_get_last_error(const cardano_cbor_reader_t* reader)
{
  return cardano_object_get_last_error(&reader->base);
}