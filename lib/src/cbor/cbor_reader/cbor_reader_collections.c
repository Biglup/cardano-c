/**
 * \file cbor_reader_collections.c
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../cbor_initial_byte.h"
#include "cbor_reader_collections.h"
#include "cbor_reader_core.h"
#include "cbor_reader_numeric.h"

#include <assert.h>

/* CONSTANTS *****************************************************************/

static const byte_t   CBOR_INITIAL_BYTE_INDEFINITE_LENGTH_BREAK = 255U;
static const int64_t  INDEFINITE_LENGTH                         = -1;
static const size_t   HEADER_BYTE_SIZE                          = 1;
static const uint64_t KEY_VALUE_PAIR_SIZE                       = 2;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Validates that the next byte in the CBOR stream is the break byte (0xFF).
 *
 * This function checks if the next byte in the CBOR data stream, as represented by the
 * reader, is the break byte (0xFF).
 *
 * \param[in] reader A pointer to an initialized cardano_cbor_reader_t structure representing
 * the state of the CBOR stream.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS
 * is returned if the byte is validated. If the operation fails, an appropriate error code is returned to
 * indicate the reason for failure.
 */
static cardano_error_t
validate_next_break_byte(cardano_cbor_reader_t* reader)
{
  byte_t          initial_byte = 0;
  cardano_error_t result       = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_UNDEFINED, &initial_byte);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (initial_byte != CBOR_INITIAL_BYTE_INDEFINITE_LENGTH_BREAK)
  {
    cardano_object_set_last_error(&reader->base, "Not at end of indefinite length data item.");
    return CARDANO_ERROR_DECODING;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Peeks at the length of the next definite length CBOR data item, without advancing the buffer's read position.
 *
 * This function examines a buffer containing CBOR encoded data to determine the length of the next data item, assuming
 * it has a definite length. The function does not consume the data; it merely calculates the length based on the
 * initial byte and any additional bytes that encode the length.
 *
 * \param[in]  buffer        A pointer to an initialized cardano_buffer_t structure containing the CBOR data to be examined.
 * \param[in]  initial_byte  The initial byte of the CBOR data item, which includes the major type and additional information
 *                           that may encode the item's length.
 * \param[out] length        A pointer to an int64_t variable where the determined length of the data item will be stored.
 *                           If the data item is of indefinite length, this is set to a negative value.
 * \param[out] bytes_read    A pointer to a size_t variable where the number of bytes used to encode the length will be
 *                           stored. This includes the initial byte.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is returned if
 * the length is successfully determined. If the operation fails, an appropriate error code is returned to
 * indicate the reason for failure.
 */
static cardano_error_t
peek_definite_length(cardano_buffer_t* buffer, const byte_t initial_byte, int64_t* length, size_t* bytes_read)
{
  assert(buffer != NULL);
  assert(length != NULL);
  assert(bytes_read != NULL);

  uint64_t definite_length = 0;
  size_t   read            = 0;

  cardano_error_t result = _cbor_reader_decode_unsigned_integer(buffer, initial_byte, &definite_length, &read);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (definite_length > (uint64_t)INT64_MAX)
  {
    return CARDANO_ERROR_DECODING;
  }

  *length     = (int64_t)definite_length;
  *bytes_read = read;

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
_cbor_reader_read_start_indefinite_length_string(cardano_cbor_reader_t* reader, const cardano_cbor_major_type_t type)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t remaining_bytes = cardano_buffer_get_size(reader->buffer);

  if ((remaining_bytes - reader->offset) < HEADER_BYTE_SIZE)
  {
    cardano_object_set_last_error(&reader->base, "Not enough bytes to read indefinite length string.");
    return CARDANO_ERROR_DECODING;
  }

  byte_t          initial_byte = 0;
  cardano_error_t result       = _cbor_reader_peek_initial_byte(reader, type, &initial_byte);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_additional_info_t info = cardano_cbor_initial_byte_get_additional_info(initial_byte);

  if (info != CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH)
  {
    cardano_object_set_last_error(&reader->base, "Not indefinite length string.");
    return CARDANO_ERROR_DECODING;
  }

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, HEADER_BYTE_SIZE);

  assert(advance_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(advance_result);

  cardano_error_t push_data_result = _cbor_reader_push_data_item(reader, type, INDEFINITE_LENGTH);

  if (push_data_result != CARDANO_SUCCESS)
  {
    return push_data_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_end_indefinite_length_string(cardano_cbor_reader_t* reader, const cardano_cbor_major_type_t type)
{
  cardano_error_t result = validate_next_break_byte(reader);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_error_t pop_data_result = _cbor_reader_pop_data_item(reader, type);

  if (pop_data_result != CARDANO_SUCCESS)
  {
    return pop_data_result;
  }

  _cbor_reader_advance_data_item_counters(reader);

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, HEADER_BYTE_SIZE);

  assert(advance_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(advance_result);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_indefinite_length_concatenated(cardano_cbor_reader_t* reader, cardano_buffer_t** buffer, size_t* encoding_length)
{
  static const size_t INITIAL_CONCAT_BUFFER_CAPACITY = 128;

  assert(reader != NULL);
  assert(buffer != NULL);
  assert(encoding_length != NULL);

  cardano_buffer_t* data   = NULL;
  cardano_error_t   result = cardano_cbor_reader_get_remainder_bytes(reader, &data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&data);
    return result;
  }

  cardano_buffer_t* concat = cardano_buffer_new(INITIAL_CONCAT_BUFFER_CAPACITY);

  size_t       i    = HEADER_BYTE_SIZE;
  const size_t size = cardano_buffer_get_size(data);

  if ((concat == NULL) || (size <= HEADER_BYTE_SIZE))
  {
    cardano_buffer_unref(&data);
    cardano_buffer_unref(&concat);

    return CARDANO_ERROR_DECODING;
  }

  byte_t initial_byte = cardano_buffer_get_data(data)[i];

  while (initial_byte != CBOR_INITIAL_BYTE_INDEFINITE_LENGTH_BREAK)
  {
    int64_t chunk_length = 0;
    size_t  bytes_read   = 0;

    cardano_buffer_t* slice = cardano_buffer_slice(data, i, cardano_buffer_get_size(data));

    if ((slice == NULL) || (cardano_buffer_get_size(slice) == 0U))
    {
      cardano_buffer_unref(&data);
      cardano_buffer_unref(&concat);

      return CARDANO_ERROR_DECODING;
    }

    cardano_error_t peek_definite_length_result = peek_definite_length(slice, initial_byte, &chunk_length, &bytes_read);

    cardano_buffer_unref(&slice);

    if (peek_definite_length_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&data);
      cardano_buffer_unref(&concat);

      return peek_definite_length_result;
    }

    size_t payload_size = bytes_read + (size_t)chunk_length;

    cardano_buffer_t* chunk = cardano_buffer_slice(data, i + (payload_size - (size_t)chunk_length), i + payload_size);

    if (chunk != NULL)
    {
      cardano_buffer_t* tmp = concat;
      concat                = cardano_buffer_concat(tmp, chunk);

      cardano_buffer_unref(&chunk);
      cardano_buffer_unref(&tmp);
    }

    i += payload_size;

    if (i >= size)
    {
      cardano_buffer_unref(&data);
      cardano_buffer_unref(&concat);

      return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
    }

    initial_byte = cardano_buffer_get_data(data)[i];
  }

  *buffer = concat;

  cardano_buffer_unref(&data);
  *encoding_length = i + HEADER_BYTE_SIZE;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_start_array(cardano_cbor_reader_t* reader, int64_t* size)
{
  byte_t header;

  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (cardano_cbor_initial_byte_get_additional_info(header) == CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH)
  {
    cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, HEADER_BYTE_SIZE);

    assert(advance_result == CARDANO_SUCCESS);
    CARDANO_UNUSED(advance_result);

    cardano_error_t push_data_result = _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, INDEFINITE_LENGTH);

    if (push_data_result != CARDANO_SUCCESS)
    {
      return push_data_result;
    }

    *size = INDEFINITE_LENGTH;
    return CARDANO_SUCCESS;
  }

  int64_t length     = 0;
  size_t  bytes_read = 0;

  cardano_buffer_t* remaining_bytes  = NULL;
  cardano_error_t   get_bytes_result = cardano_cbor_reader_get_remainder_bytes(reader, &remaining_bytes);

  if (get_bytes_result != CARDANO_SUCCESS)
  {
    return get_bytes_result;
  }

  result = peek_definite_length(remaining_bytes, header, &length, &bytes_read);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_set_last_error(reader, "Failed to read length of definite array");
    cardano_buffer_unref(&remaining_bytes);

    return result;
  }

  cardano_buffer_unref(&remaining_bytes);

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, bytes_read);

  assert(advance_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(advance_result);

  cardano_error_t push_data_result = _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, length);

  if (push_data_result != CARDANO_SUCCESS)
  {
    return push_data_result;
  }

  *size = length;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_end_array(cardano_cbor_reader_t* reader)
{
  if (reader->current_frame.definite_length == INDEFINITE_LENGTH)
  {
    cardano_error_t result = validate_next_break_byte(reader);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_error_t pop_data_result = _cbor_reader_pop_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY);

    if (pop_data_result != CARDANO_SUCCESS)
    {
      return pop_data_result;
    }

    _cbor_reader_advance_data_item_counters(reader);

    cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, HEADER_BYTE_SIZE);

    assert(advance_result == CARDANO_SUCCESS);
    CARDANO_UNUSED(advance_result);
  }
  else
  {
    cardano_error_t pop_data_result = _cbor_reader_pop_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY);

    if (pop_data_result != CARDANO_SUCCESS)
    {
      return pop_data_result;
    }

    _cbor_reader_advance_data_item_counters(reader);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_start_map(cardano_cbor_reader_t* reader, int64_t* size)
{
  byte_t header = 0;

  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_MAP, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (cardano_cbor_initial_byte_get_additional_info(header) == CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH)
  {
    cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, HEADER_BYTE_SIZE);

    assert(advance_result == CARDANO_SUCCESS);
    CARDANO_UNUSED(advance_result);

    cardano_error_t push_data_result = _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_MAP, INDEFINITE_LENGTH);

    if (push_data_result != CARDANO_SUCCESS)
    {
      return push_data_result;
    }

    *size = INDEFINITE_LENGTH;
    return CARDANO_SUCCESS;
  }

  int64_t length     = 0;
  size_t  bytes_read = 0;

  cardano_buffer_t* remaining_bytes  = NULL;
  cardano_error_t   get_bytes_result = cardano_cbor_reader_get_remainder_bytes(reader, &remaining_bytes);

  if (get_bytes_result != CARDANO_SUCCESS)
  {
    return get_bytes_result;
  }

  result = peek_definite_length(remaining_bytes, header, &length, &bytes_read);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&remaining_bytes);
    return result;
  }

  cardano_buffer_unref(&remaining_bytes);

  if ((KEY_VALUE_PAIR_SIZE * (size_t)length) > (cardano_buffer_get_size(reader->buffer) - reader->offset))
  {
    cardano_cbor_reader_set_last_error(reader, "Definite length exceeds buffer size");

    return CARDANO_ERROR_DECODING;
  }

  assert(result == CARDANO_SUCCESS);

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, bytes_read);

  assert(advance_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(advance_result);

  cardano_error_t push_data_result = _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_MAP, length * ((int64_t)KEY_VALUE_PAIR_SIZE));

  if (push_data_result != CARDANO_SUCCESS)
  {
    return push_data_result;
  }

  *size = length;

  reader->current_frame.current_key_offset = (int64_t)reader->offset;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_end_map(cardano_cbor_reader_t* reader)
{
  if (reader->current_frame.definite_length == INDEFINITE_LENGTH)
  {
    cardano_error_t result = validate_next_break_byte(reader);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if ((reader->current_frame.items_read % KEY_VALUE_PAIR_SIZE) != 0U)
    {
      cardano_cbor_reader_set_last_error(reader, "Key missing value");
      return CARDANO_ERROR_DECODING;
    }

    cardano_error_t pop_data_result = _cbor_reader_pop_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_MAP);

    if (pop_data_result != CARDANO_SUCCESS)
    {
      return pop_data_result;
    }

    _cbor_reader_advance_data_item_counters(reader);
    cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, HEADER_BYTE_SIZE);

    assert(advance_result == CARDANO_SUCCESS);
    CARDANO_UNUSED(advance_result);
  }
  else
  {
    cardano_error_t pop_data_result = _cbor_reader_pop_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_MAP);

    if (pop_data_result != CARDANO_SUCCESS)
    {
      return pop_data_result;
    }

    _cbor_reader_advance_data_item_counters(reader);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_string(cardano_cbor_reader_t* reader, const cardano_cbor_major_type_t type, cardano_buffer_t** byte_string)
{
  byte_t          header = 0;
  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, type, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (cardano_cbor_initial_byte_get_additional_info(header) == CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH)
  {
    size_t          encoding_length    = 0;
    cardano_error_t read_concat_result = _cbor_reader_read_indefinite_length_concatenated(reader, byte_string, &encoding_length);

    if (read_concat_result != CARDANO_SUCCESS)
    {
      return read_concat_result;
    }

    cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, encoding_length);

    assert(advance_result == CARDANO_SUCCESS);
    CARDANO_UNUSED(advance_result);

    _cbor_reader_advance_data_item_counters(reader);
    return CARDANO_SUCCESS;
  }

  cardano_buffer_t* buffer                 = NULL;
  cardano_error_t   remainder_bytes_result = cardano_cbor_reader_get_remainder_bytes(reader, &buffer);

  if (remainder_bytes_result != CARDANO_SUCCESS)
  {
    return remainder_bytes_result;
  }

  int64_t length = 0;

  size_t bytes_read = 0;

  result = peek_definite_length(buffer, header, &length, &bytes_read);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&buffer);
    return result;
  }

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, bytes_read + (size_t)length);

  if (advance_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&buffer);
    return advance_result;
  }

  _cbor_reader_advance_data_item_counters(reader);

  *byte_string = cardano_buffer_slice(buffer, bytes_read, bytes_read + (size_t)length);
  cardano_buffer_unref(&buffer);

  return CARDANO_SUCCESS;
}
