/**
 * \file flat_reader.cpp
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

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>

#include "../../src/uplc/flat/flat_reader.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cstdint>
#include <cstring>
#include <gmock/gmock.h>
#include <string>
#include <vector>

/* STATIC HELPERS ************************************************************/

static cardano_uplc_flat_reader_t
make_reader(const byte_t* buffer, const size_t size)
{
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };
  EXPECT_EQ(cardano_uplc_flat_reader_init(&reader, buffer, size), CARDANO_SUCCESS);
  return reader;
}

static std::string
bigint_to_decimal(const cardano_bigint_t* value)
{
  size_t      size = cardano_bigint_get_string_size(value, 10);
  std::string buffer(size, '\0');
  EXPECT_EQ(cardano_bigint_to_string(value, &buffer[0], size, 10), CARDANO_SUCCESS);
  buffer.resize(strlen(buffer.c_str()));
  return buffer;
}

/* INIT *********************************************************************/

TEST(cardano_uplc_flat_reader_init, setsCursorToStart)
{
  // Arrange
  const byte_t               data[] = { 0xFFU };
  cardano_uplc_flat_reader_t reader = { reinterpret_cast<const byte_t*>(0x1), 99U, 99U, 99U };

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_init(&reader, data, sizeof(data));

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(reader.buffer, data);
  EXPECT_EQ(reader.size, sizeof(data));
  EXPECT_EQ(reader.byte_pos, 0U);
  EXPECT_EQ(reader.bit_pos, 0U);
}

TEST(cardano_uplc_flat_reader_init, allowsNullBufferWhenSizeZero)
{
  // Arrange
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_init(&reader, nullptr, 0U);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
}

TEST(cardano_uplc_flat_reader_init, returnsErrorWhenReaderIsNull)
{
  // Arrange
  const byte_t data[] = { 0x00U };

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_init(nullptr, data, sizeof(data));

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_init, returnsErrorWhenBufferNullButSizeNonZero)
{
  // Arrange
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_init(&reader, nullptr, 4U);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

/* BIT **********************************************************************/

TEST(cardano_uplc_flat_reader_bit, readsMsbFirst)
{
  // Arrange
  const byte_t               data[]   = { 0xB1U };
  cardano_uplc_flat_reader_t reader   = make_reader(data, sizeof(data));
  const uint8_t              expect[] = { 1U, 0U, 1U, 1U, 0U, 0U, 0U, 1U };

  // Act + Assert
  for (size_t i = 0U; i < sizeof(expect); ++i)
  {
    uint8_t bit = 0xFFU;
    EXPECT_EQ(cardano_uplc_flat_reader_bit(&reader, &bit), CARDANO_SUCCESS);
    EXPECT_EQ(bit, expect[i]);
  }
}

TEST(cardano_uplc_flat_reader_bit, crossesByteBoundary)
{
  // Arrange
  const byte_t               data[] = { 0x01U, 0x80U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    bit    = 0U;

  // Act + Assert
  for (size_t i = 0U; i < 7U; ++i)
  {
    EXPECT_EQ(cardano_uplc_flat_reader_bit(&reader, &bit), CARDANO_SUCCESS);
    EXPECT_EQ(bit, 0U);
  }

  EXPECT_EQ(cardano_uplc_flat_reader_bit(&reader, &bit), CARDANO_SUCCESS);
  EXPECT_EQ(bit, 1U);
  EXPECT_EQ(reader.byte_pos, 1U);
  EXPECT_EQ(reader.bit_pos, 0U);

  EXPECT_EQ(cardano_uplc_flat_reader_bit(&reader, &bit), CARDANO_SUCCESS);
  EXPECT_EQ(bit, 1U);
  EXPECT_EQ(reader.bit_pos, 1U);
}

TEST(cardano_uplc_flat_reader_bit, returnsDecodingWhenExhausted)
{
  // Arrange
  const byte_t               data[] = { 0xFFU };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    bit    = 0U;

  // Act
  for (size_t i = 0U; i < 8U; ++i)
  {
    EXPECT_EQ(cardano_uplc_flat_reader_bit(&reader, &bit), CARDANO_SUCCESS);
  }

  cardano_error_t error = cardano_uplc_flat_reader_bit(&reader, &bit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_uplc_flat_reader_bit, returnsDecodingOnEmptyStream)
{
  // Arrange
  cardano_uplc_flat_reader_t reader = make_reader(nullptr, 0U);
  uint8_t                    bit    = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bit(&reader, &bit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_uplc_flat_reader_bit, returnsErrorWhenReaderIsNull)
{
  // Arrange
  uint8_t bit = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bit(nullptr, &bit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_bit, returnsErrorWhenValueIsNull)
{
  // Arrange
  const byte_t               data[] = { 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bit(&reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

/* BITS8 ********************************************************************/

TEST(cardano_uplc_flat_reader_bits8, readsFullByte)
{
  // Arrange
  const byte_t               data[] = { 0xA5U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bits8(&reader, 8U, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 0xA5U);
}

TEST(cardano_uplc_flat_reader_bits8, readsZeroBitsAsZero)
{
  // Arrange
  const byte_t               data[] = { 0xFFU };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    value  = 0x7BU;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bits8(&reader, 0U, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 0U);
  EXPECT_EQ(reader.byte_pos, 0U);
  EXPECT_EQ(reader.bit_pos, 0U);
}

TEST(cardano_uplc_flat_reader_bits8, readsAcrossByteBoundary)
{
  // Arrange
  // 0x1F 0x40 = 0001 1111 0100 0000. Skip 3 bits, then read 8 bits:
  // 1 1111 010 = 0xFA.
  const byte_t               data[] = { 0x1FU, 0x40U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    skip   = 0U;

  EXPECT_EQ(cardano_uplc_flat_reader_bits8(&reader, 3U, &skip), CARDANO_SUCCESS);
  EXPECT_EQ(skip, 0U);

  // Act
  uint8_t         value = 0U;
  cardano_error_t error = cardano_uplc_flat_reader_bits8(&reader, 8U, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 0xFAU);
}

TEST(cardano_uplc_flat_reader_bits8, returnsInvalidArgumentWhenCountTooLarge)
{
  // Arrange
  const byte_t               data[] = { 0xFFU, 0xFFU };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bits8(&reader, 9U, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_uplc_flat_reader_bits8, returnsDecodingOnTruncation)
{
  // Arrange
  const byte_t               data[] = { 0xFFU };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    value  = 0U;

  EXPECT_EQ(cardano_uplc_flat_reader_bits8(&reader, 5U, &value), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bits8(&reader, 8U, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_uplc_flat_reader_bits8, returnsErrorWhenReaderIsNull)
{
  // Arrange
  uint8_t value = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bits8(nullptr, 4U, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_bits8, returnsErrorWhenValueIsNull)
{
  // Arrange
  const byte_t               data[] = { 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bits8(&reader, 4U, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

/* WORD *********************************************************************/

TEST(cardano_uplc_flat_reader_word, readsSingleByte)
{
  // Arrange
  const byte_t               data[] = { 0x01U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 1U);
}

TEST(cardano_uplc_flat_reader_word, readsSingleByteMax)
{
  // Arrange
  const byte_t               data[] = { 0x7FU };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 127U);
}

TEST(cardano_uplc_flat_reader_word, readsMultiByteContinuation)
{
  // Arrange
  // 300 = 0b100101100. Low group 0b0101100 | 0x80 = 0xAC, high group 0b0000010 = 0x02.
  const byte_t               data[] = { 0xACU, 0x02U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 300U);
}

TEST(cardano_uplc_flat_reader_word, readsMaximumEncodableSizeT)
{
  // Arrange
  // SIZE_MAX on a 64-bit size_t is 2^64 - 1 = ten 0x7f groups (continued) plus a
  // final 0x01 group (10 * 7 = 70 bits, the top group carries the single high bit).
  const byte_t               data[] = { 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x01U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, SIZE_MAX);
}

TEST(cardano_uplc_flat_reader_word, rejectsOverLongOverflowingWord)
{
  // Arrange
  // Ten 0xff continuation groups then 0x02 sets a bit above the 64-bit width.
  const byte_t               data[] = { 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x02U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_uplc_flat_reader_word, rejectsWordWithNonZeroGroupPastWidth)
{
  // Arrange
  // Eleven continuation groups push shift past the 64-bit width with a non-zero
  // payload in the final group, which must be rejected rather than truncated.
  const byte_t               data[] = { 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x01U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_uplc_flat_reader_word, acceptsZeroPayloadGroupPastWidth)
{
  // Arrange
  // Ten 0x80 groups (continued, all-zero payload) then a 0x00 terminator: the
  // groups past the width carry no value, so the decoded word is 0.
  const byte_t               data[] = { 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 1U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 0U);
}

TEST(cardano_uplc_flat_reader_word, returnsDecodingOnTruncatedContinuation)
{
  // Arrange
  const byte_t               data[] = { 0x80U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  size_t                     value  = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_uplc_flat_reader_word, returnsErrorWhenReaderIsNull)
{
  // Arrange
  size_t value = 0U;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_word, returnsErrorWhenValueIsNull)
{
  // Arrange
  const byte_t               data[] = { 0x01U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_word(&reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

/* FILLER *******************************************************************/

TEST(cardano_uplc_flat_reader_filler, consumesZeroBitsThenOneBit)
{
  // Arrange
  // 0x01 = 0000 0001: seven zero filler bits then the terminating one bit, which
  // lands exactly on the byte boundary.
  const byte_t               data[] = { 0x01U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_filler(&reader);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(reader.byte_pos, 1U);
  EXPECT_EQ(reader.bit_pos, 0U);
}

TEST(cardano_uplc_flat_reader_filler, terminatesOnImmediateOneBit)
{
  // Arrange
  // 0x80 = 1000 0000: the first bit is the terminator, leaving seven bits in place.
  const byte_t               data[] = { 0x80U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_filler(&reader);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(reader.byte_pos, 0U);
  EXPECT_EQ(reader.bit_pos, 1U);
}

TEST(cardano_uplc_flat_reader_filler, returnsDecodingWhenNoOneBit)
{
  // Arrange
  const byte_t               data[] = { 0x00U, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_filler(&reader);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_uplc_flat_reader_filler, returnsErrorWhenReaderIsNull)
{
  // Act
  cardano_error_t error = cardano_uplc_flat_reader_filler(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

/* BYTES ********************************************************************/

TEST(cardano_uplc_flat_reader_bytes, readsEmptyBytestring)
{
  // Arrange
  // 0x01 filler aligns to byte 1, then 0x00 is the terminating zero-length block.
  const byte_t               data[] = { 0x01U, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_buffer_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_THAT(out, testing::Not((cardano_buffer_t*)nullptr));
  EXPECT_EQ(cardano_buffer_get_size(out), 0U);
  EXPECT_EQ(reader.byte_pos, 2U);

  // Cleanup
  cardano_buffer_unref(&out);
}

TEST(cardano_uplc_flat_reader_bytes, readsSingleBlock)
{
  // Arrange
  const byte_t               data[] = { 0x01U, 0x03U, 0xAAU, 0xBBU, 0xCCU, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_buffer_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_THAT(out, testing::Not((cardano_buffer_t*)nullptr));
  ASSERT_EQ(cardano_buffer_get_size(out), 3U);

  const byte_t* bytes = cardano_buffer_get_data(out);
  EXPECT_EQ(bytes[0], 0xAAU);
  EXPECT_EQ(bytes[1], 0xBBU);
  EXPECT_EQ(bytes[2], 0xCCU);
  EXPECT_EQ(reader.byte_pos, sizeof(data));

  // Cleanup
  cardano_buffer_unref(&out);
}

TEST(cardano_uplc_flat_reader_bytes, byteAlignsViaFillerBeforeReading)
{
  // Arrange
  // A leading partial term occupies the first three bits, then a filler run
  // (0 0001) aligns to byte 1, where the length-prefixed blocks begin. Byte 0 is
  // 000 00001 = 0x01, encoding three payload bits then the filler terminator.
  const byte_t               data[] = { 0x01U, 0x03U, 0xAAU, 0xBBU, 0xCCU, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  uint8_t                    prefix = 0U;

  EXPECT_EQ(cardano_uplc_flat_reader_bits8(&reader, 3U, &prefix), CARDANO_SUCCESS);

  cardano_buffer_t* out = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(cardano_buffer_get_size(out), 3U);

  const byte_t* bytes = cardano_buffer_get_data(out);
  EXPECT_EQ(bytes[0], 0xAAU);
  EXPECT_EQ(bytes[1], 0xBBU);
  EXPECT_EQ(bytes[2], 0xCCU);
  EXPECT_EQ(reader.byte_pos, sizeof(data));

  // Cleanup
  cardano_buffer_unref(&out);
}

TEST(cardano_uplc_flat_reader_bytes, readsMultipleBlocks)
{
  // Arrange
  const byte_t               data[] = { 0x01U, 0x02U, 0x11U, 0x22U, 0x02U, 0x33U, 0x44U, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_buffer_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(cardano_buffer_get_size(out), 4U);

  const byte_t* bytes = cardano_buffer_get_data(out);
  EXPECT_EQ(bytes[0], 0x11U);
  EXPECT_EQ(bytes[1], 0x22U);
  EXPECT_EQ(bytes[2], 0x33U);
  EXPECT_EQ(bytes[3], 0x44U);

  // Cleanup
  cardano_buffer_unref(&out);
}

TEST(cardano_uplc_flat_reader_bytes, readsMaximumBlockLength)
{
  // Arrange
  std::vector<byte_t> data;
  data.push_back(0x01U);
  data.push_back(0xFFU);
  for (size_t i = 0U; i < 255U; ++i)
  {
    data.push_back(static_cast<byte_t>(i));
  }
  data.push_back(0x00U);

  cardano_uplc_flat_reader_t reader = make_reader(data.data(), data.size());
  cardano_buffer_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(cardano_buffer_get_size(out), 255U);

  const byte_t* bytes = cardano_buffer_get_data(out);
  for (size_t i = 0U; i < 255U; ++i)
  {
    EXPECT_EQ(bytes[i], static_cast<byte_t>(i));
  }

  // Cleanup
  cardano_buffer_unref(&out);
}

TEST(cardano_uplc_flat_reader_bytes, returnsDecodingWhenBlockRunsPastBuffer)
{
  // Arrange
  // Declares a 5-byte block but only 2 bytes follow.
  const byte_t               data[] = { 0x01U, 0x05U, 0xAAU, 0xBBU };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_buffer_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, (cardano_buffer_t*)nullptr);
}

TEST(cardano_uplc_flat_reader_bytes, returnsDecodingWhenUnterminated)
{
  // Arrange
  // A complete block followed by no length byte at all.
  const byte_t               data[] = { 0x01U, 0x02U, 0xAAU, 0xBBU };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_buffer_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, (cardano_buffer_t*)nullptr);
}

TEST(cardano_uplc_flat_reader_bytes, returnsDecodingWhenFillerExhausted)
{
  // Arrange
  // No one bit anywhere, so the byte-alignment filler runs off the end.
  const byte_t               data[] = { 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_buffer_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, (cardano_buffer_t*)nullptr);
}

TEST(cardano_uplc_flat_reader_bytes, returnsErrorWhenReaderIsNull)
{
  // Arrange
  cardano_buffer_t* out = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(nullptr, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_bytes, returnsErrorWhenValueIsNull)
{
  // Arrange
  const byte_t               data[] = { 0x01U, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_bytes, returnsErrorWhenBufferAllocationFails)
{
  // Arrange
  const byte_t               data[] = { 0x01U, 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_buffer_t*          out    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, (cardano_buffer_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_uplc_flat_reader_bytes, returnsErrorWhenBufferWriteFails)
{
  // Arrange
  // A 255-byte block forces the buffer to grow past its initial capacity; the
  // reallocation backing that growth is failed.
  std::vector<byte_t> big;
  big.push_back(0x01U);
  big.push_back(0xFFU);
  for (size_t i = 0U; i < 255U; ++i)
  {
    big.push_back(static_cast<byte_t>(i));
  }
  big.push_back(0x00U);

  cardano_uplc_flat_reader_t reader = make_reader(big.data(), big.size());
  cardano_buffer_t*          out    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(malloc, fail_right_away_realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_bytes(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, (cardano_buffer_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

/* BIG_WORD *****************************************************************/

TEST(cardano_uplc_flat_reader_big_word, readsSingleGroup)
{
  // Arrange
  const byte_t               data[] = { 0x05U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_THAT(out, testing::Not((cardano_bigint_t*)nullptr));
  EXPECT_EQ(bigint_to_decimal(out), "5");
  EXPECT_EQ(reader.byte_pos, 1U);

  // Cleanup
  cardano_bigint_unref(&out);
}

TEST(cardano_uplc_flat_reader_big_word, readsMultiGroup)
{
  // Arrange
  // 300 = 0b100101100. Low group 0b0101100 | 0x80 = 0xAC, high group 0b0000010 = 0x02.
  const byte_t               data[] = { 0xACU, 0x02U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(bigint_to_decimal(out), "300");
  EXPECT_EQ(reader.byte_pos, 2U);

  // Cleanup
  cardano_bigint_unref(&out);
}

TEST(cardano_uplc_flat_reader_big_word, readsMagnitudeFarBeyondSixtyFourBits)
{
  // Arrange
  // 2^200 has its only set bit at position 200, which falls in group 28 at
  // offset 4 (group value 0x10). Groups 0..27 are zero with the continuation
  // flag set (0x80); group 28 (0x10) terminates.
  std::vector<byte_t> data(28U, 0x80U);
  data.push_back(0x10U);

  cardano_uplc_flat_reader_t reader = make_reader(data.data(), data.size());
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(bigint_to_decimal(out), "1606938044258990275541962092341162602522202993782792835301376");
  EXPECT_EQ(reader.byte_pos, 29U);

  // Cleanup
  cardano_bigint_unref(&out);
}

TEST(cardano_uplc_flat_reader_big_word, returnsDecodingOnTruncatedContinuation)
{
  // Arrange
  const byte_t               data[] = { 0x80U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, (cardano_bigint_t*)nullptr);
}

TEST(cardano_uplc_flat_reader_big_word, returnsDecodingOnTruncationMidMagnitude)
{
  // Arrange
  // Two continuation groups promise a third that never arrives.
  const byte_t               data[] = { 0xACU, 0x82U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, (cardano_bigint_t*)nullptr);
}

TEST(cardano_uplc_flat_reader_big_word, returnsErrorWhenResultAllocationFails)
{
  // Arrange
  const byte_t               data[] = { 0x05U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, (cardano_bigint_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_uplc_flat_reader_big_word, returnsErrorWhenGroupAllocationFails)
{
  // Arrange
  // The accumulator bigint is created first; the per-group bigint for the
  // non-zero payload is the second allocation and is the one failed here.
  const byte_t               data[] = { 0x05U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, (cardano_bigint_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_uplc_flat_reader_big_word, returnsErrorWhenReaderIsNull)
{
  // Arrange
  cardano_bigint_t* out = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(nullptr, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_big_word, returnsErrorWhenValueIsNull)
{
  // Arrange
  const byte_t               data[] = { 0x05U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_big_word(&reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

/* INTEGER ******************************************************************/

TEST(cardano_uplc_flat_reader_integer, decodesCanonicalZigZagSequence)
{
  // Arrange
  // Unsigned magnitudes 0,1,2,3,4 decode to the signed run 0,-1,1,-2,2.
  const byte_t encoded[]  = { 0x00U, 0x01U, 0x02U, 0x03U, 0x04U };
  const char*  expected[] = { "0", "-1", "1", "-2", "2" };

  // Act + Assert
  for (size_t i = 0U; i < sizeof(encoded); ++i)
  {
    cardano_uplc_flat_reader_t reader = make_reader(&encoded[i], 1U);
    cardano_bigint_t*          out    = nullptr;

    EXPECT_EQ(cardano_uplc_flat_reader_integer(&reader, &out), CARDANO_SUCCESS);
    EXPECT_EQ(bigint_to_decimal(out), expected[i]);

    cardano_bigint_unref(&out);
  }
}

TEST(cardano_uplc_flat_reader_integer, decodesLargePositiveMagnitude)
{
  // Arrange
  // Magnitude 2^200 is even, so it decodes to +2^199.
  std::vector<byte_t> data(28U, 0x80U);
  data.push_back(0x10U);

  cardano_uplc_flat_reader_t reader = make_reader(data.data(), data.size());
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_integer(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(bigint_to_decimal(out), "803469022129495137770981046170581301261101496891396417650688");

  // Cleanup
  cardano_bigint_unref(&out);
}

TEST(cardano_uplc_flat_reader_integer, decodesLargeNegativeMagnitude)
{
  // Arrange
  // Magnitude 2^200 - 1 is odd, so it decodes to -2^199. Its 200 set bits split
  // into 28 full 7-bit groups (0x7F, continued) and a final 4-bit group 0x0F.
  std::vector<byte_t> data(28U, 0xFFU);
  data.push_back(0x0FU);

  cardano_uplc_flat_reader_t reader = make_reader(data.data(), data.size());
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_integer(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(bigint_to_decimal(out), "-803469022129495137770981046170581301261101496891396417650688");

  // Cleanup
  cardano_bigint_unref(&out);
}

TEST(cardano_uplc_flat_reader_integer, roundTripsAcrossWidthAndSignBoundaries)
{
  // Arrange
  // Encode value v with the inverse zig-zag (u = v >= 0 ? 2v : -2v - 1), feed the
  // 7-bit little-endian groups back through integer, and expect v.
  const int64_t values[] = { 0, 1, -1, 2, -2, 127, -128, 4294967295LL, -4294967296LL, 9223372036854775807LL };

  // Act + Assert
  for (size_t i = 0U; i < (sizeof(values) / sizeof(values[0])); ++i)
  {
    const int64_t       v = values[i];
    uint64_t            u = (v >= 0) ? ((uint64_t)v << 1U) : (((uint64_t)(-(v + 1)) << 1U) | 1U);
    std::vector<byte_t> data;

    do
    {
      byte_t group = (byte_t)(u & 0x7FU);
      u            >>= 7U;

      if (u != 0U)
      {
        group |= 0x80U;
      }

      data.push_back(group);
    }
    while (u != 0U);

    cardano_uplc_flat_reader_t reader = make_reader(data.data(), data.size());
    cardano_bigint_t*          out    = nullptr;

    EXPECT_EQ(cardano_uplc_flat_reader_integer(&reader, &out), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_bigint_to_int(out), v);

    cardano_bigint_unref(&out);
  }
}

TEST(cardano_uplc_flat_reader_integer, returnsDecodingOnTruncation)
{
  // Arrange
  const byte_t               data[] = { 0x80U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_integer(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(out, (cardano_bigint_t*)nullptr);
}

TEST(cardano_uplc_flat_reader_integer, returnsErrorWhenMagnitudeAllocationFails)
{
  // Arrange
  const byte_t               data[] = { 0x03U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));
  cardano_bigint_t*          out    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_integer(&reader, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(out, (cardano_bigint_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_uplc_flat_reader_integer, returnsErrorWhenReaderIsNull)
{
  // Arrange
  cardano_bigint_t* out = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_integer(nullptr, &out);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_flat_reader_integer, returnsErrorWhenValueIsNull)
{
  // Arrange
  const byte_t               data[] = { 0x00U };
  cardano_uplc_flat_reader_t reader = make_reader(data, sizeof(data));

  // Act
  cardano_error_t error = cardano_uplc_flat_reader_integer(&reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}
