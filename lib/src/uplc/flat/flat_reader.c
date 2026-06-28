/**
 * \file flat_reader.c
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
 *
 * Copyright 2026 Biglup Labs
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

#include "flat_reader.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Number of bits in a byte; the bit cursor wraps at this count.
 */
static const uint8_t FLAT_BITS_PER_BYTE = 8U;

/**
 * \brief Mask selecting the most significant bit of a byte, read first.
 */
static const uint8_t FLAT_MSB_MASK = 0x80U;

/**
 * \brief Number of payload bits carried by each continuation group of a word.
 */
static const uint8_t FLAT_WORD_GROUP_BITS = 7U;

/**
 * \brief Mask selecting the 7 payload bits of a word continuation group.
 */
static const uint8_t FLAT_WORD_PAYLOAD_MASK = 0x7FU;

/**
 * \brief Mask selecting the continuation flag (high bit) of a word group.
 */
static const uint8_t FLAT_WORD_CONTINUATION_MASK = 0x80U;

/**
 * \brief Initial capacity requested for the bytestring accumulator.
 */
static const size_t FLAT_BYTES_INITIAL_CAPACITY = 64U;

/* STATIC FUNCTIONS *********************************************************/

/**
 * \brief Applies the flat zig-zag decode to an unsigned magnitude in place.
 *
 * Maps the unsigned encoding back to its signed value: an even magnitude \c u
 * decodes to <tt>u >> 1</tt>, an odd magnitude to <tt>-((u >> 1) + 1)</tt>. This
 * is the <tt>(u >> 1) xor -(u and 1)</tt> transform over the bigint.
 *
 * \param[in,out] magnitude The unsigned magnitude, replaced by its signed value.
 */
static void
decode_zigzag(cardano_bigint_t* magnitude)
{
  const bool is_odd = cardano_bigint_test_bit(magnitude, 0U);

  cardano_bigint_shift_right(magnitude, 1U, magnitude);

  if (is_odd)
  {
    cardano_bigint_negate(magnitude, magnitude);
    cardano_bigint_decrement(magnitude);
  }
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_flat_reader_init(cardano_uplc_flat_reader_t* reader, const byte_t* buffer, const size_t size)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((buffer == NULL) && (size != 0U))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  reader->buffer   = buffer;
  reader->size     = size;
  reader->byte_pos = 0U;
  reader->bit_pos  = 0U;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_reader_bit(cardano_uplc_flat_reader_t* reader, uint8_t* value)
{
  if ((reader == NULL) || (value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader->byte_pos >= reader->size)
  {
    return CARDANO_ERROR_DECODING;
  }

  const uint8_t selector = (uint8_t)(FLAT_MSB_MASK >> reader->bit_pos);

  *value = ((reader->buffer[reader->byte_pos] & selector) != 0U) ? 1U : 0U;

  if (reader->bit_pos == (FLAT_BITS_PER_BYTE - 1U))
  {
    reader->bit_pos = 0U;
    reader->byte_pos += 1U;
  }
  else
  {
    reader->bit_pos += 1U;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_reader_bits8(cardano_uplc_flat_reader_t* reader, const uint8_t count, uint8_t* value)
{
  if ((reader == NULL) || (value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (count > FLAT_BITS_PER_BYTE)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (count == 0U)
  {
    *value = 0U;
    return CARDANO_SUCCESS;
  }

  if (reader->byte_pos >= reader->size)
  {
    return CARDANO_ERROR_DECODING;
  }

  const uint8_t available = (uint8_t)(FLAT_BITS_PER_BYTE - reader->bit_pos);
  uint8_t       result    = 0U;

  if (count <= available)
  {
    const uint8_t shift = (uint8_t)(available - count);

    result = (uint8_t)((reader->buffer[reader->byte_pos] >> shift) & (uint8_t)((1U << count) - 1U));

    reader->bit_pos = (uint8_t)(reader->bit_pos + count);

    if (reader->bit_pos == FLAT_BITS_PER_BYTE)
    {
      reader->bit_pos = 0U;
      reader->byte_pos += 1U;
    }
  }
  else
  {
    if ((reader->byte_pos + 1U) >= reader->size)
    {
      return CARDANO_ERROR_DECODING;
    }

    const uint8_t from_second = (uint8_t)(count - available);
    const uint8_t high        = (uint8_t)(reader->buffer[reader->byte_pos] & (uint8_t)((1U << available) - 1U));
    const uint8_t low         = (uint8_t)(reader->buffer[reader->byte_pos + 1U] >> (FLAT_BITS_PER_BYTE - from_second));

    result = (uint8_t)(((uint32_t)high << from_second) | low);

    reader->byte_pos += 1U;
    reader->bit_pos = from_second;
  }

  *value = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_reader_word(cardano_uplc_flat_reader_t* reader, size_t* value)
{
  if ((reader == NULL) || (value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t size_t_bits = sizeof(size_t) * (size_t)FLAT_BITS_PER_BYTE;

  size_t result = 0U;
  size_t shift  = 0U;
  bool   more   = true;

  while (more)
  {
    uint8_t         group = 0U;
    cardano_error_t error = cardano_uplc_flat_reader_bits8(reader, FLAT_BITS_PER_BYTE, &group);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }

    const uint8_t payload_bits = (uint8_t)(group & FLAT_WORD_PAYLOAD_MASK);
    const size_t  payload      = (size_t)payload_bits;

    if (shift >= size_t_bits)
    {
      if (payload != 0U)
      {
        return CARDANO_ERROR_DECODING;
      }
    }
    else
    {
      if (payload > (SIZE_MAX >> shift))
      {
        return CARDANO_ERROR_DECODING;
      }

      result |= (payload << shift);
      shift += FLAT_WORD_GROUP_BITS;
    }

    more = (group & FLAT_WORD_CONTINUATION_MASK) != 0U;
  }

  *value = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_reader_filler(cardano_uplc_flat_reader_t* reader)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  bool terminated = false;

  while (!terminated)
  {
    uint8_t         bit   = 0U;
    cardano_error_t error = cardano_uplc_flat_reader_bit(reader, &bit);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }

    terminated = (bit != 0U);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_reader_big_word(cardano_uplc_flat_reader_t* reader, cardano_bigint_t** value)
{
  if ((reader == NULL) || (value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bigint_t* result = NULL;
  cardano_error_t   error  = cardano_bigint_from_int(0, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  uint32_t shift = 0U;
  bool     more  = true;

  while (more)
  {
    uint8_t group = 0U;

    error = cardano_uplc_flat_reader_bits8(reader, FLAT_BITS_PER_BYTE, &group);

    if (error != CARDANO_SUCCESS)
    {
      cardano_bigint_unref(&result);

      return error;
    }

    const uint8_t payload = (uint8_t)(group & FLAT_WORD_PAYLOAD_MASK);

    if (payload != 0U)
    {
      cardano_bigint_t* group_value = NULL;

      error = cardano_bigint_from_unsigned_int((uint64_t)payload, &group_value);

      if (error != CARDANO_SUCCESS)
      {
        cardano_bigint_unref(&result);

        return error;
      }

      cardano_bigint_shift_left(group_value, shift, group_value);
      cardano_bigint_or(result, group_value, result);
      cardano_bigint_unref(&group_value);
    }

    shift += FLAT_WORD_GROUP_BITS;
    more = (group & FLAT_WORD_CONTINUATION_MASK) != 0U;
  }

  *value = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_reader_integer(cardano_uplc_flat_reader_t* reader, cardano_bigint_t** value)
{
  if ((reader == NULL) || (value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bigint_t* magnitude = NULL;
  cardano_error_t   error     = cardano_uplc_flat_reader_big_word(reader, &magnitude);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  decode_zigzag(magnitude);

  *value = magnitude;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_reader_bytes(cardano_uplc_flat_reader_t* reader, cardano_buffer_t** value)
{
  if ((reader == NULL) || (value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t error = cardano_uplc_flat_reader_filler(reader);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  cardano_buffer_t* result = cardano_buffer_new(FLAT_BYTES_INITIAL_CAPACITY);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  bool done = false;

  while (!done)
  {
    if (reader->byte_pos >= reader->size)
    {
      cardano_buffer_unref(&result);

      return CARDANO_ERROR_DECODING;
    }

    const size_t block_len = (size_t)reader->buffer[reader->byte_pos];

    reader->byte_pos += 1U;

    if (block_len == 0U)
    {
      done = true;
    }
    else
    {
      if (block_len > (reader->size - reader->byte_pos))
      {
        cardano_buffer_unref(&result);

        return CARDANO_ERROR_DECODING;
      }

      error = cardano_buffer_write(result, &reader->buffer[reader->byte_pos], block_len);

      if (error != CARDANO_SUCCESS)
      {
        cardano_buffer_unref(&result);

        return error;
      }

      reader->byte_pos += block_len;
    }
  }

  *value = result;

  return CARDANO_SUCCESS;
}
