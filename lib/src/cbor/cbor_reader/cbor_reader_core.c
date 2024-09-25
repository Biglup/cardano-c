/**
 * \file cbor_reader_core.c
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

#include <cardano/cbor/cbor_major_type.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../../allocators.h"
#include "../cbor_initial_byte.h"
#include "cbor_reader_collections.h"
#include "cbor_reader_core.h"

#include <assert.h>
#include <stdio.h>

/* CONSTANTS *****************************************************************/

static const byte_t  CBOR_INITIAL_BYTE_INDEFINITE_LENGTH_BREAK = 255U;
static const int64_t INDEFINITE_LENGTH                         = -1;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief This function is intended to be called when a CBOR reader encounters the end of a data item
 * that has a definite length. It updates the reader's state to reflect the completion of reading
 * this data item.
 *
 * \param[in,out] reader A pointer to the \ref cardano_cbor_reader_t structure representing the
 *                       current state of the CBOR reader. This reader's state is updated based on
 *                       the processing of the end of the definite length item.
 * \param[out]    state  A pointer to the \ref cardano_cbor_reader_state_t structure where the updated state
 *                       of the reader will be stored.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. \c CARDANO_SUCCESS is
 *         returned if the end of the definite length data item was processed successfully. If an
 *         error occurs, an appropriate error code is returned to indicate the failure reason.
 *         Error codes are detailed in the \c cardano_error_t documentation.
 */
static cardano_error_t
process_end_of_definite_length(cardano_cbor_reader_t* reader, cardano_cbor_reader_state_t* state)
{
  assert(reader != NULL);
  assert(state != NULL);

  if (reader->current_frame.type == CARDANO_CBOR_MAJOR_TYPE_UNDEFINED)
  {
    *state = CARDANO_CBOR_READER_STATE_FINISHED;
    return CARDANO_SUCCESS;
  }

  switch (reader->current_frame.type)
  {
    case CARDANO_CBOR_MAJOR_TYPE_ARRAY:
      *state = CARDANO_CBOR_READER_STATE_END_ARRAY;
      return CARDANO_SUCCESS;
    case CARDANO_CBOR_MAJOR_TYPE_MAP:
      *state = CARDANO_CBOR_READER_STATE_END_MAP;
      return CARDANO_SUCCESS;
    default:
      cardano_object_set_last_error(&reader->base, "Invalid CBOR major type pushed to stack.");
      return CARDANO_ERROR_DECODING;
  }
}

/**
 * \brief This function is called when a CBOR reader encounters the termination byte for an indefinite
 * length data item. It updates the reader's state to correctly reflect the closure of an indefinite
 * length data item.
 *
 * \param[in,out] reader A pointer to the \ref cardano_cbor_reader_t structure representing the
 *                       current state of the CBOR reader. The reader's state is adjusted to account
 *                       for the end of the indefinite length item.
 * \param[out]    state  A pointer to the \ref cardano_cbor_reader_state_t structure where the updated state
 *                       of the reader is stored.
 *
 * \return A \c cardano_error_t indicating the outcome of the function. \c CARDANO_SUCCESS indicates
 *         that the end of the indefinite length item was processed correctly. Should an error occur,
 *         a suitable error code will be returned to signify the specific issue encountered during
 *         processing. Refer to the documentation on \c cardano_error_t for more details on possible
 *         error codes.
 */
static cardano_error_t
process_end_of_indefinite_length(cardano_cbor_reader_t* reader, cardano_cbor_reader_state_t* state)
{
  assert(reader != NULL);
  assert(state != NULL);

  if (reader->is_tag_context)
  {
    cardano_object_set_last_error(&reader->base, "Tag not followed by value.");
    return CARDANO_ERROR_DECODING;
  }

  if (reader->current_frame.definite_length == INDEFINITE_LENGTH)
  {
    switch (reader->current_frame.type)
    {
      case CARDANO_CBOR_MAJOR_TYPE_UNDEFINED:
        cardano_object_set_last_error(&reader->base, "Unexpected break byte.");
        return CARDANO_ERROR_DECODING;
      case CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING:
        *state = CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_BYTESTRING;
        return CARDANO_SUCCESS;
      case CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING:
        *state = CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_TEXTSTRING;
        return CARDANO_SUCCESS;
      case CARDANO_CBOR_MAJOR_TYPE_ARRAY:
        *state = CARDANO_CBOR_READER_STATE_END_ARRAY;
        return CARDANO_SUCCESS;
      case CARDANO_CBOR_MAJOR_TYPE_MAP:
        if ((reader->current_frame.items_read % 2U) == 0U)
        {
          *state = CARDANO_CBOR_READER_STATE_END_MAP;
          return CARDANO_SUCCESS;
        }

        cardano_object_set_last_error(&reader->base, "Key missing value.");
        return CARDANO_ERROR_DECODING;
      default:
        cardano_object_set_last_error(&reader->base, "Invalid CBOR major type pushed to stack.");
        return CARDANO_ERROR_DECODING;
    }
  }
  else
  {
    cardano_object_set_last_error(&reader->base, "Unexpected break byte.");
    return CARDANO_ERROR_DECODING;
  }
}

/**
 * \brief This function is invoked when the CBOR reader reaches the end of the buffer while parsing
 * CBOR data. It ensures the reader's state is accurately updated to reflect the completion of parsing
 * or to indicate that the data stream has been prematurely terminated.
 *
 * \param[in,out] reader A pointer to the \ref cardano_cbor_reader_t structure that represents the
 *                       current state of the CBOR reader. This reader's state will be updated to
 *                       reflect the processing of the buffer's end.
 * \param[in] buffer_size The total size of the CBOR buffer that the reader is parsing. This size is
 *                        used to determine whether the reader has encountered an actual end of data
 *                        or an unexpected termination of the buffer.
 * \param[out]    state   A pointer to the \ref cardano_cbor_reader_state_t structure where the reader's new
 *                        state will be stored after processing the end of the buffer.
 *
 * \return A \c cardano_error_t indicating the outcome of processing the end of the buffer. \c CARDANO_SUCCESS
 *         is returned if the end of the buffer was reached under expected conditions and the reader's state
 *         has been appropriately updated. If an error occurs, such as encountering truncated data, an
 *         appropriate error code will be returned to indicate the issue. Refer to the \c cardano_error_t
 *         documentation for details on possible error codes.
 */
static cardano_error_t
process_end_of_buffer(cardano_cbor_reader_t* reader, const size_t buffer_size, cardano_cbor_reader_state_t* state)
{
  assert(reader != NULL);
  assert(state != NULL);

  if (reader->offset > buffer_size)
  {
    cardano_object_set_last_error(&reader->base, "Unexpected end of buffer.");
    return CARDANO_ERROR_DECODING;
  }

  if ((reader->current_frame.type == CARDANO_CBOR_MAJOR_TYPE_UNDEFINED) && (reader->current_frame.definite_length == INDEFINITE_LENGTH))
  {
    *state = CARDANO_CBOR_READER_STATE_FINISHED;
    return CARDANO_SUCCESS;
  }

  cardano_object_set_last_error(&reader->base, "Unexpected end of buffer.");
  return CARDANO_ERROR_DECODING;
}

/**
 * \brief This function translates a simple value from CBOR's representation into the corresponding
 * state used by the CBOR reader.
 *
 * \param[in] value The simple value as defined by CBOR's \ref cardano_cbor_additional_info_t enumeration.
 *                  This value specifies the type of simple value (e.g., boolean, null, undefined)
 *                  to be mapped to a reader state.
 *
 * \return A \ref cardano_cbor_reader_state_t enumeration value that corresponds to the CBOR reader's internal
 *         state representation of the given simple value. This state can then be used by the reader
 *         to correctly interpret and proceed with parsing the CBOR data.
 */
static cardano_cbor_reader_state_t
map_simple_value_data_to_reader_state(const cardano_cbor_additional_info_t value)
{
  cardano_cbor_reader_state_t result = CARDANO_CBOR_READER_STATE_SIMPLE_VALUE;

  switch (value)
  {
    case CARDANO_CBOR_ADDITIONAL_INFO_NULL:
      result = CARDANO_CBOR_READER_STATE_NULL;
      break;
    case CARDANO_CBOR_ADDITIONAL_INFO_FALSE:
    case CARDANO_CBOR_ADDITIONAL_INFO_TRUE:
      result = CARDANO_CBOR_READER_STATE_BOOLEAN;
      break;
    case CARDANO_CBOR_ADDITIONAL_INFO_16BIT_DATA:
      result = CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT;
      break;
    case CARDANO_CBOR_ADDITIONAL_INFO_32BIT_DATA:
      result = CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT;
      break;
    case CARDANO_CBOR_ADDITIONAL_INFO_64BIT_DATA:
      result = CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT;
      break;
    default:
      break;
  }

  return result;
}

/**
 * \brief Maps a CBOR major type to the corresponding reader state.
 *
 * This function takes the current major type read from the CBOR data stream and translates it into
 * a corresponding state for the CBOR reader.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance.
 * \param[out] state A pointer to a \ref cardano_cbor_reader_state_t variable where the resulting reader
 *                   state will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation. This can be
 *         \c CARDANO_SUCCESS if the major type was successfully mapped to a reader state, or an
 *         appropriate error code if an issue occurred during the mapping process.
 */
static cardano_error_t
map_major_type_to_reader_state(cardano_cbor_reader_t* reader, cardano_cbor_reader_state_t* state)
{
  assert(reader != NULL);
  assert(state != NULL);

  byte_t*                              buffer_data     = cardano_buffer_get_data(reader->buffer);
  const byte_t                         initial_byte    = buffer_data[reader->offset];
  const cardano_cbor_additional_info_t additional_info = cardano_cbor_initial_byte_get_additional_info(initial_byte);
  const cardano_cbor_major_type_t      major_type      = cardano_cbor_initial_byte_get_major_type(initial_byte);

  switch (major_type)
  {
    case CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER:
      *state = CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER;
      break;
    case CARDANO_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER:
      *state = CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER;
      break;
    case CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING:
      *state = (additional_info == CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH)
        ? CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING
        : CARDANO_CBOR_READER_STATE_BYTESTRING;
      break;
    case CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING:
      *state = (additional_info == CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH)
        ? CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING
        : CARDANO_CBOR_READER_STATE_TEXTSTRING;
      break;
    case CARDANO_CBOR_MAJOR_TYPE_ARRAY:
      *state = CARDANO_CBOR_READER_STATE_START_ARRAY;
      break;
    case CARDANO_CBOR_MAJOR_TYPE_MAP:
      *state = CARDANO_CBOR_READER_STATE_START_MAP;
      break;
    case CARDANO_CBOR_MAJOR_TYPE_TAG:
      *state = CARDANO_CBOR_READER_STATE_TAG;
      break;
    case CARDANO_CBOR_MAJOR_TYPE_SIMPLE:
      *state = map_simple_value_data_to_reader_state(additional_info);
      break;
    default:
      cardano_object_set_last_error(&reader->base, "Invalid CBOR major type.");
      return CARDANO_ERROR_DECODING;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief his function is used to revert the state of the CBOR reader to a specific point previously
 * saved in a stack frame.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that is processing the
 *                   CBOR data stream. The reader's current state will be modified to match that
 *                   of the provided stack frame.
 * \param[in] frame A pointer to the \ref cbor_reader_stack_frame_t instance representing the
 *                  stack frame to which the reader's state should be restored.
 */
static void
cbor_reader_restore_stack_frame(cardano_cbor_reader_t* reader, cbor_reader_stack_frame_t* frame)
{
  assert(reader != NULL);
  assert(frame != NULL);

  reader->current_frame.type               = frame->type;
  reader->current_frame.definite_length    = frame->definite_length;
  reader->current_frame.frame_offset       = frame->frame_offset;
  reader->current_frame.items_read         = frame->items_read;
  reader->current_frame.current_key_offset = frame->current_key_offset;
  reader->cached_state                     = CARDANO_CBOR_READER_STATE_UNDEFINED;
}

/**
 * \brief This function examines the next portion of the CBOR data stream to determine the state
 * that the reader will transition into upon reading this data. It allows the caller to anticipate
 * the type of data (e.g., integer, string, map start) that will be read next without actually
 * consuming any data from the stream. This is particularly useful for decision-making processes
 * that depend on the structure of the incoming CBOR data.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance. This reader is processing the
 *                   CBOR data stream and needs to peek ahead at the next data item's type.
 * \param[out] state A pointer to a \ref cardano_cbor_reader_state_t variable where the predicted reader state
 *                   will be stored. This state represents the type and nature of the next data item that
 *                   would be read by the reader.
 *
 * \return A \ref cardano_error_t indicating the success of the operation. \c CARDANO_SUCCESS is returned
 *         if the function successfully peeks at the next state, allowing for accurate anticipation of the
 *         next read operation. If an error occurs during peeking, an appropriate error code is returned.
 *
 * \note This function does not modify the reader's current position within the CBOR data stream, ensuring
 *       that the actual read operations can proceed unaffected by the peek operation. It is a critical
 *       tool for complex parsing logic where the handling strategy may vary depending on upcoming data.
 */
static cardano_error_t
peek_state_core(cardano_cbor_reader_t* reader, cardano_cbor_reader_state_t* state)
{
  assert(reader != NULL);
  assert(state != NULL);

  const size_t buffer_size = cardano_buffer_get_size(reader->buffer);
  byte_t*      buffer_data = cardano_buffer_get_data(reader->buffer);

  if ((reader->current_frame.definite_length != INDEFINITE_LENGTH) && ((reader->current_frame.definite_length - (int64_t)reader->current_frame.items_read) == 0))
  {
    return process_end_of_definite_length(reader, state);
  }

  if (reader->offset >= buffer_size)
  {
    return process_end_of_buffer(reader, buffer_size, state);
  }

  const byte_t                    initial_byte = buffer_data[reader->offset];
  const cardano_cbor_major_type_t major_type   = cardano_cbor_initial_byte_get_major_type(initial_byte);

  if (initial_byte == CBOR_INITIAL_BYTE_INDEFINITE_LENGTH_BREAK)
  {
    return process_end_of_indefinite_length(reader, state);
  }

  if ((reader->current_frame.type != CARDANO_CBOR_MAJOR_TYPE_UNDEFINED) && (reader->current_frame.definite_length != INDEFINITE_LENGTH))
  {
    switch (reader->current_frame.type)
    {
      case CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING:
      case CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING:
        if (major_type != reader->current_frame.type)
        {
          cardano_object_set_last_error(&reader->base, "Indefinite length string contains invalid data item.");
          return CARDANO_ERROR_DECODING;
        }
        break;
      default:
        break;
    }
  }

  return map_major_type_to_reader_state(reader, state);
}

/* INCLUDES ******************************************************************/

cardano_error_t
_cbor_reader_push_data_item(cardano_cbor_reader_t* reader, const cardano_cbor_major_type_t type, const int64_t definite_length)
{
  assert(reader != NULL);

  cbor_reader_stack_frame_t* frame = _cardano_malloc(sizeof(cbor_reader_stack_frame_t));

  if (frame == NULL)
  {
    cardano_object_set_last_error(&reader->base, "Failed to allocate memory for stack frame.");
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  frame->base.ref_count     = 0;
  frame->base.deallocator   = _cardano_free;
  frame->base.last_error[0] = '\0';

  frame->type               = reader->current_frame.type;
  frame->frame_offset       = reader->current_frame.frame_offset;
  frame->definite_length    = reader->current_frame.definite_length;
  frame->items_read         = reader->current_frame.items_read;
  frame->current_key_offset = reader->current_frame.current_key_offset;

  const size_t old_size = cardano_array_get_size(reader->nested_items);
  const size_t new_size = cardano_array_push(reader->nested_items, &frame->base);

  if (new_size <= old_size)
  {
    cardano_object_unref((cardano_object_t**)((void*)&frame));
    cardano_object_set_last_error(&reader->base, "Failed to add stack frame to nested items.");

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  reader->current_frame.type               = type;
  reader->current_frame.definite_length    = definite_length;
  reader->current_frame.frame_offset       = reader->offset;
  reader->current_frame.items_read         = 0;
  reader->current_frame.current_key_offset = -1;
  reader->is_tag_context                   = false;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_pop_data_item(cardano_cbor_reader_t* reader, const cardano_cbor_major_type_t expected_type)
{
  assert(reader != NULL);

  if ((reader->current_frame.type == CARDANO_CBOR_MAJOR_TYPE_UNDEFINED) || (cardano_array_get_size(reader->nested_items) == 0U))
  {
    cardano_object_set_last_error(&reader->base, "Is at root context.");
    return CARDANO_ERROR_DECODING;
  }

  if (expected_type != reader->current_frame.type)
  {
    char buffer[64] = { 0 };

    const int written = snprintf(buffer, sizeof(buffer), "Pop major type mismatch, expected %d but got %d.", expected_type, reader->current_frame.type);

    if (written < 0)
    {
      cardano_object_set_last_error(&reader->base, "Pop major type mismatch (Failed to write detailed error message).");
      return CARDANO_ERROR_DECODING;
    }

    cardano_object_set_last_error(&reader->base, buffer);

    return CARDANO_ERROR_DECODING;
  }

  if ((reader->current_frame.definite_length != INDEFINITE_LENGTH) && ((reader->current_frame.definite_length - (int64_t)reader->current_frame.items_read) > 0))
  {
    cardano_object_set_last_error(&reader->base, "Not at end of definite length data item.");

    return CARDANO_ERROR_DECODING;
  }

  if (reader->is_tag_context)
  {
    cardano_object_set_last_error(&reader->base, "Tag not followed by value.");

    return CARDANO_ERROR_DECODING;
  }

  cardano_object_t* item = cardano_array_pop(reader->nested_items);

  cbor_reader_restore_stack_frame(reader, (cbor_reader_stack_frame_t*)((void*)item));

  cardano_object_unref(&item);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_peek_initial_byte(cardano_cbor_reader_t* reader, const cardano_cbor_major_type_t expected_type, byte_t* initial_byte)
{
  assert(reader != NULL);
  assert(initial_byte != NULL);

  const size_t buffer_size = cardano_buffer_get_size(reader->buffer);

  if (reader->offset >= buffer_size)
  {
    if ((reader->current_frame.type == CARDANO_CBOR_MAJOR_TYPE_UNDEFINED) && (reader->current_frame.definite_length == INDEFINITE_LENGTH) && (reader->offset > 0U))
    {
      cardano_object_set_last_error(&reader->base, "End of root-level. No more data items to read.");
      return CARDANO_ERROR_DECODING;
    }

    cardano_object_set_last_error(&reader->base, "Unexpected end of buffer.");
    return CARDANO_ERROR_DECODING;
  }

  byte_t*                              buffer_data      = cardano_buffer_get_data(reader->buffer);
  const byte_t                         tmp_initial_byte = buffer_data[reader->offset];
  const cardano_cbor_major_type_t      major_type       = cardano_cbor_initial_byte_get_major_type(tmp_initial_byte);
  const cardano_cbor_additional_info_t additional_info  = cardano_cbor_initial_byte_get_additional_info(tmp_initial_byte);

  if ((reader->current_frame.definite_length != INDEFINITE_LENGTH) && ((reader->current_frame.definite_length - (int64_t)reader->current_frame.items_read) == 0))
  {
    cardano_object_set_last_error(&reader->base, "No more data items to read.");
    return CARDANO_ERROR_DECODING;
  }

  switch (reader->current_frame.type)
  {
    case CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING:
    case CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING:
      if (tmp_initial_byte == CBOR_INITIAL_BYTE_INDEFINITE_LENGTH_BREAK)
      {
        break;
      }

      if (major_type == reader->current_frame.type)
      {
        if (additional_info != CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH)
        {
          break;
        }
      }
      else
      {
        cardano_object_set_last_error(&reader->base, "Indefinite length string contains invalid data item.");
        return CARDANO_ERROR_DECODING;
      }
      break;
    default:
      break;
  }

  if ((expected_type != CARDANO_CBOR_MAJOR_TYPE_UNDEFINED) && (expected_type != major_type))
  {
    cardano_object_set_last_error(&reader->base, "Major type mismatch.");
    return CARDANO_ERROR_DECODING;
  }

  *initial_byte = tmp_initial_byte;
  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_advance_buffer(cardano_cbor_reader_t* reader, const size_t length)
{
  assert(reader != NULL);

  const size_t buffer_size = cardano_buffer_get_size(reader->buffer);

  if ((reader->offset + length) > buffer_size)
  {
    cardano_object_set_last_error(&reader->base, "Buffer offset out of bounds.");
    return CARDANO_ERROR_DECODING;
  }

  reader->offset       += length;
  reader->cached_state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  return CARDANO_SUCCESS;
}

void
_cbor_reader_advance_data_item_counters(cardano_cbor_reader_t* reader)
{
  assert(reader != NULL);

  reader->current_frame.items_read++;
  reader->is_tag_context = false;
}

cardano_error_t
_cbor_reader_skip_next_node(cardano_cbor_reader_t* reader, size_t* depth)
{
  assert(reader != NULL);
  assert(depth != NULL);

  cardano_cbor_reader_state_t state             = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const cardano_error_t       peek_state_result = peek_state_core(reader, &state);

  if (peek_state_result != CARDANO_SUCCESS)
  {
    return peek_state_result;
  }

  while (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    cardano_cbor_tag_t    tag    = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;
    const cardano_error_t result = cardano_cbor_reader_read_tag(reader, &tag);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    CARDANO_UNUSED(tag);

    cardano_error_t peek_result = peek_state_core(reader, &state);

    if (peek_result != CARDANO_SUCCESS)
    {
      return peek_result;
    }
  }

  switch (state)
  {
    case CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER:
    {
      uint64_t              value  = 0;
      const cardano_error_t result = cardano_cbor_reader_read_uint(reader, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      CARDANO_UNUSED(value);
    }
    break;
    case CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER:
    {
      int64_t               value  = 0;
      const cardano_error_t result = cardano_cbor_reader_read_int(reader, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      CARDANO_UNUSED(value);
    }
    break;
    case CARDANO_CBOR_READER_STATE_BYTESTRING:
    {
      cardano_buffer_t*     byte_string = NULL;
      const cardano_error_t result      = cardano_cbor_reader_read_bytestring(reader, &byte_string);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      cardano_buffer_unref(&byte_string);
    }
    break;
    case CARDANO_CBOR_READER_STATE_TEXTSTRING:
    {
      cardano_buffer_t*     text_string = NULL;
      const cardano_error_t result      = cardano_cbor_reader_read_textstring(reader, &text_string);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      cardano_buffer_unref(&text_string);
    }
    break;
    case CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING:
    {
      const cardano_error_t result = _cbor_reader_read_start_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth + 1;
      *depth                = newDepth;
    }
    break;
    case CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_BYTESTRING:
    {
      const cardano_error_t result = _cbor_reader_read_end_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth - 1;
      *depth                = newDepth;
    }
    break;
    case CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING:
    {
      const cardano_error_t result = _cbor_reader_read_start_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth + 1;
      *depth                = newDepth;
    }
    break;
    case CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_TEXTSTRING:
    {
      const cardano_error_t result = _cbor_reader_read_end_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth - 1;
      *depth                = newDepth;
    }
    break;
    case CARDANO_CBOR_READER_STATE_START_ARRAY:
    {
      int64_t               value  = 0;
      const cardano_error_t result = cardano_cbor_reader_read_start_array(reader, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth + 1;
      *depth                = newDepth;

      CARDANO_UNUSED(value);
    }
    break;
    case CARDANO_CBOR_READER_STATE_END_ARRAY:
    {
      const cardano_error_t result = cardano_cbor_reader_read_end_array(reader);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth - 1;
      *depth                = newDepth;
    }
    break;
    case CARDANO_CBOR_READER_STATE_START_MAP:
    {
      int64_t               value  = 0;
      const cardano_error_t result = cardano_cbor_reader_read_start_map(reader, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth + 1;
      *depth                = newDepth;

      CARDANO_UNUSED(value);
    }
    break;
    case CARDANO_CBOR_READER_STATE_END_MAP:
    {
      const cardano_error_t result = cardano_cbor_reader_read_end_map(reader);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      const size_t newDepth = *depth - 1;
      *depth                = newDepth;
    }
    break;
    case CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT:
    case CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT:
    case CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT:
    {
      double                value  = 0.0;
      const cardano_error_t result = cardano_cbor_reader_read_double(reader, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      CARDANO_UNUSED(value);
    }
    break;
    case CARDANO_CBOR_READER_STATE_NULL:
    case CARDANO_CBOR_READER_STATE_BOOLEAN:
    case CARDANO_CBOR_READER_STATE_SIMPLE_VALUE:
    {
      cardano_cbor_simple_value_t value  = CARDANO_CBOR_SIMPLE_VALUE_UNDEFINED;
      const cardano_error_t       result = cardano_cbor_reader_read_simple_value(reader, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      CARDANO_UNUSED(value);
    }
    break;
    default:
      cardano_cbor_reader_set_last_error(reader, "Skip invalid state.");
      return CARDANO_ERROR_DECODING;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_peek_state(cardano_cbor_reader_t* reader, cardano_cbor_reader_state_t* state)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (state == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader->cached_state == CARDANO_CBOR_READER_STATE_UNDEFINED)
  {
    const cardano_error_t error = peek_state_core(reader, &reader->cached_state);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }

  *state = reader->cached_state;

  return CARDANO_SUCCESS;
}