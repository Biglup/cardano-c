/**
 * \file flat_writer.c
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#include "flat_writer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Number of bits in a byte; the bit cursor wraps at this count.
 */
static const uint8_t FLAT_BITS_PER_BYTE = 8U;

/**
 * \brief Mask selecting the most significant bit of a byte, written first.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_MSB_MASK = 0x80U;

/**
 * \brief Number of payload bits carried by each continuation group of a word.
 */
static const uint8_t FLAT_WORD_GROUP_BITS = 7U;

/**
 * \brief Mask selecting the 7 payload bits of a word continuation group.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint8_t FLAT_WORD_PAYLOAD_MASK = 0x7FU;

/**
 * \brief Continuation flag (high bit) set on a non-final word group.
 */
static const uint8_t FLAT_WORD_CONTINUATION = 0x80U;

/**
 * \brief Largest length a single flat bytestring block can carry.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t FLAT_BYTES_MAX_BLOCK = 255U;

/**
 * \brief Initial capacity requested for the writer's backing buffer.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t FLAT_WRITER_INITIAL_CAPACITY = 64U;

/* STATIC FUNCTIONS *********************************************************/

/**
 * \brief Applies the flat zig-zag encode to a signed value into a new magnitude.
 *
 * Maps a non-negative \c n to <tt>2n</tt> and a negative \c n to <tt>-2n - 1</tt>,
 * the inverse of \c decode_zigzag in flat_reader.c.
 *
 * \param[in] value The signed value to encode. Must not be NULL.
 * \param[out] magnitude On success, a newly created non-negative magnitude the
 *             caller releases with \ref cardano_bigint_unref; left untouched on
 *             failure.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *         if the magnitude cannot be created.
 */
static cardano_error_t
encode_zigzag(const cardano_bigint_t* value, cardano_bigint_t** magnitude)
{
  cardano_bigint_t* result = NULL;
  cardano_error_t   error  = cardano_bigint_clone(value, &result);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  cardano_bigint_shift_left(result, 1U, result);

  if (cardano_bigint_signum(value) < 0)
  {
    cardano_bigint_negate(result, result);
    cardano_bigint_decrement(result);
  }

  *magnitude = result;

  return CARDANO_SUCCESS;
}

/**
 * \brief Appends a fully assembled byte to the writer's backing buffer.
 *
 * \param[in,out] writer The writer whose buffer receives \p value.
 * \param[in] value The byte to append.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *         if the buffer cannot be grown.
 */
static cardano_error_t
push_byte(cardano_uplc_flat_writer_t* writer, const uint8_t value)
{
  return cardano_buffer_write(writer->buffer, &value, 1U);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_flat_writer_init(cardano_uplc_flat_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  writer->buffer  = cardano_buffer_new(FLAT_WRITER_INITIAL_CAPACITY);
  writer->current = 0U;
  writer->used    = 0U;

  if (writer->buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

void
cardano_uplc_flat_writer_dispose(cardano_uplc_flat_writer_t* writer)
{
  if (writer == NULL)
  {
    return;
  }

  cardano_buffer_unref(&writer->buffer);
  writer->current = 0U;
  writer->used    = 0U;
}

cardano_error_t
cardano_uplc_flat_writer_bit(cardano_uplc_flat_writer_t* writer, const uint8_t value)
{
  cardano_error_t result = CARDANO_SUCCESS;

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value != 0U)
  {
    writer->current = (uint8_t)(writer->current | (uint8_t)(FLAT_MSB_MASK >> writer->used));
  }

  if (writer->used == (FLAT_BITS_PER_BYTE - 1U))
  {
    result = push_byte(writer, writer->current);

    writer->current = 0U;
    writer->used    = 0U;
  }
  else
  {
    writer->used += 1U;
  }

  return result;
}

cardano_error_t
cardano_uplc_flat_writer_bits8(cardano_uplc_flat_writer_t* writer, const uint8_t value, const uint8_t count)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (count > FLAT_BITS_PER_BYTE)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  for (uint8_t i = 0U; i < count; ++i)
  {
    const uint8_t   shift = (uint8_t)(count - 1U - i);
    const uint8_t   bit   = (uint8_t)((value >> shift) & 1U);
    cardano_error_t error = cardano_uplc_flat_writer_bit(writer, bit);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_writer_word(cardano_uplc_flat_writer_t* writer, const size_t value)
{
  size_t remaining = value;
  bool   more      = true;

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  while (more)
  {
    uint8_t         group = (uint8_t)(remaining & (size_t)FLAT_WORD_PAYLOAD_MASK);
    cardano_error_t error = CARDANO_SUCCESS;

    remaining >>= FLAT_WORD_GROUP_BITS;

    if (remaining != 0U)
    {
      group = (uint8_t)(group | FLAT_WORD_CONTINUATION);
    }
    else
    {
      more = false;
    }

    error = cardano_uplc_flat_writer_bits8(writer, group, FLAT_BITS_PER_BYTE);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_writer_big_word(cardano_uplc_flat_writer_t* writer, const cardano_bigint_t* magnitude)
{
  cardano_bigint_t* temp   = NULL;
  cardano_error_t   result = CARDANO_SUCCESS;
  bool              more   = true;

  if ((writer == NULL) || (magnitude == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_bigint_clone(magnitude, &temp);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  while (more)
  {
    uint8_t group = 0U;

    for (uint8_t i = 0U; i < FLAT_WORD_GROUP_BITS; ++i)
    {
      if (cardano_bigint_test_bit(temp, i))
      {
        group = (uint8_t)(group | (uint8_t)(1U << i));
      }
    }

    cardano_bigint_shift_right(temp, FLAT_WORD_GROUP_BITS, temp);

    if (!cardano_bigint_is_zero(temp))
    {
      group = (uint8_t)(group | FLAT_WORD_CONTINUATION);
    }
    else
    {
      more = false;
    }

    result = cardano_uplc_flat_writer_bits8(writer, group, FLAT_BITS_PER_BYTE);

    if (result != CARDANO_SUCCESS)
    {
      cardano_bigint_unref(&temp);

      return result;
    }
  }

  cardano_bigint_unref(&temp);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_flat_writer_integer(cardano_uplc_flat_writer_t* writer, const cardano_bigint_t* value)
{
  cardano_bigint_t* magnitude = NULL;
  cardano_error_t   result    = CARDANO_SUCCESS;

  if ((writer == NULL) || (value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = encode_zigzag(value, &magnitude);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_flat_writer_big_word(writer, magnitude);

  cardano_bigint_unref(&magnitude);

  return result;
}

cardano_error_t
cardano_uplc_flat_writer_filler(cardano_uplc_flat_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  while (writer->used != (FLAT_BITS_PER_BYTE - 1U))
  {
    cardano_error_t error = cardano_uplc_flat_writer_bit(writer, 0U);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }

  return cardano_uplc_flat_writer_bit(writer, 1U);
}

cardano_error_t
cardano_uplc_flat_writer_bytes(cardano_uplc_flat_writer_t* writer, const byte_t* data, const size_t size)
{
  size_t          offset = 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((data == NULL) && (size != 0U))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_flat_writer_filler(writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  while (offset < size)
  {
    const size_t remaining = size - offset;
    const size_t block_len = (remaining > FLAT_BYTES_MAX_BLOCK) ? FLAT_BYTES_MAX_BLOCK : remaining;

    result = push_byte(writer, (uint8_t)block_len);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_buffer_write(writer->buffer, &data[offset], block_len);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    offset += block_len;
  }

  return push_byte(writer, 0U);
}

cardano_error_t
cardano_uplc_flat_writer_finish(cardano_uplc_flat_writer_t* writer, cardano_buffer_t** out)
{
  if ((writer == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer->used != 0U)
  {
    cardano_error_t result = push_byte(writer, writer->current);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    writer->current = 0U;
    writer->used    = 0U;
  }

  *out           = writer->buffer;
  writer->buffer = NULL;

  return CARDANO_SUCCESS;
}
