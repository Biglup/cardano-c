/**
 * \file cbor_reader_tags.c
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

#include "cbor_reader_tags.h"
#include "cbor_reader_core.h"
#include "cbor_reader_numeric.h"

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_tag.h>
#include <cardano/error.h>

#include <assert.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Peeks at the next CBOR tag in the stream without advancing the reader's position.
 *
 * This internal function examines the next data item in the CBOR stream, expected to be a tag (major type 6),
 * and retrieves its value. It provides a way to inspect the tag value without affecting the reader's current
 * position in the data stream.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance representing the CBOR stream to peek into.
 * \param[out] tag A pointer to a `cardano_cbor_tag_t` variable where the peeked tag value will be stored.
 * \param[out] bytes_read A pointer to a `size_t` variable where the number of bytes that the tag occupies in
 * the stream will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the peek operation. \ref CARDANO_SUCCESS is returned
 * if a tag is successfully peeked from the stream. If the operation fails due to reasons such as incorrect
 * stream positioning, unexpected data format, or if the next item in the stream is not a tag, an appropriate
 * error code will be returned to indicate the failure reason.
 */
static cardano_error_t
peek_tag_core(cardano_cbor_reader_t* reader, cardano_cbor_tag_t* tag, size_t* bytes_read)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(tag != NULL);
  assert(bytes_read != NULL);

  byte_t          header = 0;
  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_TAG, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  uint64_t unsigned_int = 0;

  size_t read = 0;

  cardano_buffer_t* buffer = NULL;

  cardano_error_t remainder_bytes_result = cardano_cbor_reader_get_remainder_bytes(reader, &buffer);

  if (remainder_bytes_result != CARDANO_SUCCESS)
  {
    return remainder_bytes_result;
  }

  cardano_error_t decode_result = _cbor_reader_decode_unsigned_integer(buffer, header, &unsigned_int, &read);

  cardano_buffer_unref(&buffer);

  if (decode_result != CARDANO_SUCCESS)
  {
    return decode_result;
  }

  *tag        = (cardano_cbor_tag_t)unsigned_int;
  *bytes_read = read;

  return CARDANO_SUCCESS;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
_cbor_reader_read_tag(cardano_cbor_reader_t* reader, cardano_cbor_tag_t* tag)
{
  cardano_cbor_tag_t read_tag  = 0;
  size_t             bytesRead = 0;

  cardano_error_t result = peek_tag_core(reader, &read_tag, &bytesRead);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, bytesRead);

  assert(advance_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(advance_result);

  reader->is_tag_context = true;
  *tag                   = read_tag;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_peek_tag(cardano_cbor_reader_t* reader, cardano_cbor_tag_t* tag)
{
  cardano_cbor_tag_t read_tag  = 0;
  size_t             bytesRead = 0;

  cardano_error_t result = peek_tag_core(reader, &read_tag, &bytesRead);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *tag = read_tag;

  return CARDANO_SUCCESS;
}
