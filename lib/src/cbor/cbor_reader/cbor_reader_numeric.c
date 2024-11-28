/**
 * \file cbor_reader_numeric.c
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

#include "cbor_reader_numeric.h"
#include "cbor_reader_core.h"

#include <assert.h>
#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/error.h>

#include "../cbor_additional_info.h"
#include "../cbor_initial_byte.h"

#include <math.h>
#include <stdio.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Peeks at the next signed integer in the CBOR stream without advancing the reader.
 *
 * This function inspects the CBOR stream to determine the value of the next signed integer
 * (major types 0 and 1 for unsigned and negative integers, respectively) without consuming
 * any data. This allows the caller to make decisions based on the integer's value without
 * actually removing it from the stream.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance being operated on.
 *                   The reader's position remains unchanged after calling this function.
 * \param[out] signed_int A pointer to an \c int64_t variable where the function will store
 *                        the value of the signed integer.
 * \param[out] bytes_read A pointer to a \c size_t variable where the function will store
 *                        the total number of bytes that the signed integer occupies in the CBOR stream.
 *
 * \return A \ref cardano_error_t indicating the result of the operation. \ref CARDANO_SUCCESS is
 * returned if a signed integer is successfully peeked at. If the next item in the stream is not
 * a signed integer or if an error occurs during processing, an appropriate error code is
 * returned to indicate the failure reason.
 */
static cardano_error_t
peek_signed_integer(cardano_cbor_reader_t* reader, int64_t* signed_int, size_t* bytes_read)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(signed_int != NULL);
  assert(bytes_read != NULL);

  byte_t          header = 0;
  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_UNDEFINED, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_major_type_t major_type = cardano_cbor_initial_byte_get_major_type(header);

  switch (major_type)
  {
    case CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER:
    {
      uint64_t unsigned_int = 0;
      size_t   read         = 0;

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

      *signed_int = (int64_t)unsigned_int;
      *bytes_read = read;
      break;
    }
    case CARDANO_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER:
    {
      uint64_t unsigned_int = 0;
      size_t   read         = 0;

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

      *signed_int = (int64_t)(-1) - (int64_t)unsigned_int;
      *bytes_read = read;
      break;
    }
    default:
    {
      char buffer[64] = { 0 };

      int32_t written = snprintf(buffer, sizeof(buffer), "Reader type mismatch, expected %d or %d but got %d.", CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, CARDANO_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER, major_type);

      if (written < 0)
      {
        cardano_object_set_last_error(&reader->base, "Reader type mismatch (Failed to write detailed error message).");
        return CARDANO_ERROR_DECODING;
      }

      cardano_object_set_last_error(&reader->base, buffer);

      return CARDANO_ERROR_DECODING;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Peeks at the next unsigned integer in the CBOR stream without advancing the reader.
 *
 * This function examines the CBOR stream to ascertain the value of the next unsigned integer
 * (major type 0) without consuming any data from the stream.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance being processed.
 *                   The reader's current position within the stream is unchanged after this
 *                   function is called.
 *
 * \param[out] unsigned_int A pointer to a \c uint64_t variable where the function will store
 *                          the value of the unsigned integer found in the stream.
 *
 * \param[out] bytes_read A pointer to a \c size_t variable where the function will store
 *                        the total number of bytes occupied by the unsigned integer within
 *                        the CBOR stream.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. If an unsigned integer
 * is successfully identified, \ref CARDANO_SUCCESS is returned. If the next item in the stream is
 * not an unsigned integer or if a processing error occurs, an appropriate error code will be
 * returned to denote the specific reason for failure.
 */
static cardano_error_t
peek_unsigned_integer(cardano_cbor_reader_t* reader, uint64_t* signed_int, size_t* bytes_read)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(signed_int != NULL);
  assert(bytes_read != NULL);

  byte_t          header = 0;
  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_UNDEFINED, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_major_type_t major_type = cardano_cbor_initial_byte_get_major_type(header);

  switch (major_type)
  {
    case CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER:
    {
      uint64_t unsigned_int = 0;
      size_t   read         = 0;

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

      *signed_int = unsigned_int;
      *bytes_read = read;
      return CARDANO_SUCCESS;
    }
    case CARDANO_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER:
    {
      cardano_object_set_last_error(&reader->base, "Integer overflow.");
      return CARDANO_ERROR_DECODING;
    }
    default:
    {
      char buffer[64] = { 0 };

      int32_t written = snprintf(buffer, sizeof(buffer), "Reader type mismatch, expected %d but got %d.", CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, major_type);

      if (written < 0)
      {
        cardano_object_set_last_error(&reader->base, "Reader type mismatch (Failed to write detailed error message).");
        return CARDANO_ERROR_DECODING;
      }

      cardano_object_set_last_error(&reader->base, buffer);

      return CARDANO_ERROR_DECODING;
    }
  }
}

/**
 * \brief Decodes a half-precision floating point number from the buffer into a double.
 *
 * This function decodes a floating point number encoded in half-precision (16 bits) format
 * stored in a given buffer and converts it into a double precision floating point number.
 *
 * \param[in] buffer A pointer to the \ref cardano_buffer_t instance that contains the half-precision
 *                   floating point number to be decoded. The buffer should contain at least 2 bytes of
 *                   data representing the half-precision float.
 * \param[out] value A pointer to a double variable where the decoded double precision floating point
 *                   number will be stored. The decoded value is calculated by extending the precision
 *                   of the half-precision float to fit the double precision format.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. If the half-precision float is
 * successfully decoded, \ref CARDANO_SUCCESS is returned. If an error occurs during decoding, such as if the
 * buffer does not contain sufficient data to represent a half-precision float, an appropriate error code will
 * be returned to denote the specific reason for failure.
 */
static cardano_error_t
decode_half_precision_float(cardano_buffer_t* buffer, double* value)
{
  assert(buffer != NULL);
  assert(value != NULL);

  uint16_t              half   = 0;
  const cardano_error_t result = cardano_buffer_read_uint16_be(buffer, &half);

  assert(result == CARDANO_SUCCESS);
  CARDANO_UNUSED(result);

  const uint16_t exp  = (half >> 10) & (byte_t)0x1f;
  const uint16_t mant = half & (uint16_t)0x03ff;

  double val = 0.0;

  if (exp == 0U)
  {
    val = ldexp(mant, -24);
  }
  else if (exp != 31U)
  {
    val = ldexp(mant + 1024u, (int16_t)exp - 25);
  }
  else
  {
    val = (mant == 0U) ? INFINITY : NAN;
  }

  *value = (half & (uint16_t)0x8000) ? -val : val;

  return CARDANO_SUCCESS;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
_cbor_reader_decode_unsigned_integer(cardano_buffer_t* buffer, byte_t header, uint64_t* unsigned_int, size_t* bytes_read)
{
  static const byte_t additional_information_mask = 0b00011111;

  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(unsigned_int != NULL);
  assert(bytes_read != NULL);

  if ((header & additional_information_mask) < (byte_t)CARDANO_CBOR_ADDITIONAL_INFO_8BIT_DATA)
  {
    *bytes_read   = 1;
    *unsigned_int = cardano_cbor_initial_byte_get_additional_info(header);
    return CARDANO_SUCCESS;
  }

  switch (cardano_cbor_initial_byte_get_additional_info(header))
  {
    case CARDANO_CBOR_ADDITIONAL_INFO_8BIT_DATA:
    {
      if (cardano_buffer_get_size(buffer) < 2U)
      {
        return CARDANO_ERROR_DECODING;
      }

      byte_t          data[2] = { 0, 0 };
      cardano_error_t result  = cardano_buffer_read(buffer, &data[0], 2);

      assert(result == CARDANO_SUCCESS);
      CARDANO_UNUSED(result);

      *bytes_read   = 2;
      *unsigned_int = (uint64_t)data[1];

      break;
    }
    case CARDANO_CBOR_ADDITIONAL_INFO_16BIT_DATA:
    {
      if (cardano_buffer_get_size(buffer) < 3U)
      {
        return CARDANO_ERROR_DECODING;
      }

      uint16_t data    = 0;
      byte_t   to_skip = 0;

      cardano_error_t result = cardano_buffer_read(buffer, &to_skip, 1);

      assert(result == CARDANO_SUCCESS);
      CARDANO_UNUSED(result);

      result = cardano_buffer_read_uint16_be(buffer, &data);

      assert(result == CARDANO_SUCCESS);
      CARDANO_UNUSED(result);

      *bytes_read   = 3;
      *unsigned_int = (uint64_t)data;

      break;
    }
    case CARDANO_CBOR_ADDITIONAL_INFO_32BIT_DATA:
    {
      if (cardano_buffer_get_size(buffer) < 5U)
      {
        return CARDANO_ERROR_DECODING;
      }

      uint32_t data    = 0;
      byte_t   to_skip = 0;

      cardano_error_t result = cardano_buffer_read(buffer, &to_skip, 1);

      assert(result == CARDANO_SUCCESS);
      CARDANO_UNUSED(result);

      result = cardano_buffer_read_uint32_be(buffer, &data);

      assert(result == CARDANO_SUCCESS);
      CARDANO_UNUSED(result);

      *bytes_read   = 5;
      *unsigned_int = (uint64_t)data;

      break;
    }
    case CARDANO_CBOR_ADDITIONAL_INFO_64BIT_DATA:
    {
      if (cardano_buffer_get_size(buffer) < 9U)
      {
        return CARDANO_ERROR_DECODING;
      }

      uint64_t data    = 0;
      byte_t   to_skip = 0;

      cardano_error_t result = cardano_buffer_read(buffer, &to_skip, 1);

      assert(result == CARDANO_SUCCESS);
      CARDANO_UNUSED(result);

      result = cardano_buffer_read_uint64_be(buffer, &data);

      assert(result == CARDANO_SUCCESS);
      CARDANO_UNUSED(result);

      *bytes_read   = 9;
      *unsigned_int = data;

      break;
    }
    default:
      return CARDANO_ERROR_DECODING;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_double(cardano_cbor_reader_t* reader, double* value)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t          header = 0;
  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_SIMPLE, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_additional_info_t additional_info = cardano_cbor_initial_byte_get_additional_info(header);

  cardano_buffer_t* buffer                 = NULL;
  cardano_error_t   remainder_bytes_result = cardano_cbor_reader_get_remainder_bytes(reader, &buffer);

  if (remainder_bytes_result != CARDANO_SUCCESS)
  {
    return remainder_bytes_result;
  }

  byte_t          to_skip     = 0;
  cardano_error_t skip_result = cardano_buffer_read(buffer, &to_skip, 1);

  assert(skip_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(skip_result);

  switch (additional_info)
  {
    case CARDANO_CBOR_ADDITIONAL_INFO_16BIT_DATA:
    {
      if (cardano_buffer_get_size(buffer) < 3U)
      {
        cardano_buffer_unref(&buffer);
        return CARDANO_ERROR_DECODING;
      }

      double half_precision = 0.0;

      cardano_error_t decode_result = decode_half_precision_float(buffer, &half_precision);

      assert(decode_result == CARDANO_SUCCESS);
      CARDANO_UNUSED(decode_result);

      cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, 3);

      assert(advance_result == CARDANO_SUCCESS);
      CARDANO_UNUSED(advance_result);

      _cbor_reader_advance_data_item_counters(reader);
      cardano_buffer_unref(&buffer);

      *value = half_precision;
      break;
    }
    case CARDANO_CBOR_ADDITIONAL_INFO_32BIT_DATA:
    {
      if (cardano_buffer_get_size(buffer) < 5U)
      {
        cardano_buffer_unref(&buffer);
        return CARDANO_ERROR_DECODING;
      }

      float single_precision = 0.0f;

      cardano_error_t decode_result = cardano_buffer_read_float_be(buffer, &single_precision);

      assert(decode_result == CARDANO_SUCCESS);
      CARDANO_UNUSED(decode_result);

      cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, 5);

      assert(advance_result == CARDANO_SUCCESS);
      CARDANO_UNUSED(advance_result);

      _cbor_reader_advance_data_item_counters(reader);
      cardano_buffer_unref(&buffer);
      *value = single_precision;
      break;
    }
    case CARDANO_CBOR_ADDITIONAL_INFO_64BIT_DATA:
    {
      if (cardano_buffer_get_size(buffer) < 9U)
      {
        cardano_buffer_unref(&buffer);
        return CARDANO_ERROR_DECODING;
      }

      double          double_precision = 0.0;
      cardano_error_t decode_result    = cardano_buffer_read_double_be(buffer, &double_precision);

      assert(decode_result == CARDANO_SUCCESS);
      CARDANO_UNUSED(decode_result);

      cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, 9);

      assert(advance_result == CARDANO_SUCCESS);
      CARDANO_UNUSED(advance_result);

      _cbor_reader_advance_data_item_counters(reader);
      cardano_buffer_unref(&buffer);

      *value = double_precision;
      break;
    }
    default:
      cardano_buffer_unref(&buffer);
      cardano_cbor_reader_set_last_error(reader, "Not a float encoding");
      return CARDANO_ERROR_DECODING;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_int(cardano_cbor_reader_t* reader, int64_t* value)
{
  int64_t signed_int = 0;
  size_t  bytes_read = 0;

  cardano_error_t result = peek_signed_integer(reader, &signed_int, &bytes_read);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, bytes_read);

  assert(advance_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(advance_result);

  _cbor_reader_advance_data_item_counters(reader);
  *value = signed_int;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_uint(cardano_cbor_reader_t* reader, uint64_t* value)
{
  uint64_t unsigned_int = 0;
  size_t   bytes_read   = 0;

  cardano_error_t result = peek_unsigned_integer(reader, &unsigned_int, &bytes_read);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_error_t advance_result = _cbor_reader_advance_buffer(reader, bytes_read);

  assert(advance_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(advance_result);

  _cbor_reader_advance_data_item_counters(reader);
  *value = unsigned_int;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_reader_read_bigint(cardano_cbor_reader_t* reader, cardano_bigint_t** bigint)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bigint == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_tag_t tag = CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM;

  cardano_error_t result = cardano_cbor_reader_read_tag(reader, &tag);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_buffer_t* buffer = NULL;
  result                   = cardano_cbor_reader_read_bytestring(reader, &buffer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&buffer);
    return result;
  }

  result = cardano_bigint_from_bytes(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), CARDANO_BYTE_ORDER_BIG_ENDIAN, bigint);

  cardano_buffer_unref(&buffer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_bigint_t* copy = NULL;
  result                 = cardano_bigint_clone(*bigint, &copy);

  if (result != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(bigint);
    return result;
  }

  if (tag == CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM)
  {
    cardano_bigint_negate(copy, *bigint);
  }

  cardano_bigint_unref(&copy);

  return CARDANO_SUCCESS;
}