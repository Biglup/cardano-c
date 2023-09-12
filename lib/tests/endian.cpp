/**
 * \file endian.cpp
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include <cardano/endian.h>
#include <cardano/error.h>

#include <gmock/gmock.h>

#define IS_BIG_ENDIAN (*(uint16_t*)"\0\xff" < 0x100)

/* UNIT TESTS ****************************************************************/

TEST(cardano_is_little_endian, correctlyTestTheSystemForLittleEndian)
{
  // Assert
  if (IS_BIG_ENDIAN)
  {
    ASSERT_EQ(cardano_is_little_endian(), false);
  }
  else
  {
    ASSERT_EQ(cardano_is_little_endian(), true);
  }
}

TEST(cardano_is_big_endian, correctlyTestTheSystemForBigEndian)
{
  // Assert
  if (IS_BIG_ENDIAN)
  {
    ASSERT_EQ(cardano_is_big_endian(), true);
  }
  else
  {
    ASSERT_EQ(cardano_is_big_endian(), false);
  }
}

TEST(cardano_write_uint16_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint16_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint16_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[5] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint16_le(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint16_le, serializesToLittleEndian)
{
  // Arrange
  byte_t   buffer[2]   = { 0 };
  byte_t   expected[2] = { 0xEA, 0x04 };
  size_t   size        = sizeof(buffer);
  uint16_t value       = 1258;

  // Act
  cardano_error_t result = cardano_write_uint16_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint16_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t   expected[7] = { 0, 0, 0, 0, 0, 0xEA, 0x04 };
  size_t   size        = sizeof(buffer);
  uint16_t value       = 1258;

  // Act
  cardano_error_t result = cardano_write_uint16_le(value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint32_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint32_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint32_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[5] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint32_le(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint32_le, serializesToLittleEndian)
{
  // Arrange
  byte_t   buffer[4]   = { 0 };
  byte_t   expected[4] = { 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size        = sizeof(buffer);
  uint32_t value       = 82452650;

  // Act
  cardano_error_t result = cardano_write_uint32_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint32_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t   expected[7] = { 0, 0, 0, 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size        = sizeof(buffer);
  uint32_t value       = 82452650;

  // Act
  cardano_error_t result = cardano_write_uint32_le(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint64_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint64_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint64_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[5] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint64_le(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint64_le, serializesToLittleEndian)
{
  // Arrange
  byte_t   buffer[8]   = { 0 };
  byte_t   expected[8] = { 0xAA, 0x20, 0xEA, 0x04, 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size        = sizeof(buffer);
  uint64_t value       = 354131435300987050;

  // Act
  cardano_error_t result = cardano_write_uint64_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint64_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[12]   = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  byte_t   expected[12] = { 0, 0, 0, 0xAA, 0x20, 0xEA, 0x04, 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size         = sizeof(buffer);
  uint64_t value        = 354131435300987050;

  // Act
  cardano_error_t result = cardano_write_uint64_le(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int16_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int16_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int16_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[5] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int16_le(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int16_le, serializesToLittleEndian)
{
  // Arrange
  byte_t  buffer[2]   = { 0 };
  byte_t  expected[2] = { 0x16, 0xFB };
  size_t  size        = sizeof(buffer);
  int16_t value       = -1258;

  // Act
  cardano_error_t result = cardano_write_int16_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int16_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t  expected[7] = { 0, 0, 0, 0, 0, 0x16, 0xFB };
  size_t  size        = sizeof(buffer);
  int16_t value       = -1258;

  // Act
  cardano_error_t result = cardano_write_int16_le(value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int32_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int32_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int32_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[5] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int32_le(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int32_le, serializesToLittleEndian)
{
  // Arrange
  byte_t  buffer[4]   = { 0 };
  byte_t  expected[4] = { 0x56, 0xDF, 0x15, 0xFB };
  size_t  size        = sizeof(buffer);
  int32_t value       = -82452650;

  // Act
  cardano_error_t result = cardano_write_int32_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int32_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t  expected[7] = { 0, 0, 0, 0x56, 0xDF, 0x15, 0xFB };
  size_t  size        = sizeof(buffer);
  int32_t value       = -82452650;

  // Act
  cardano_error_t result = cardano_write_int32_le(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int64_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int64_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int64_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[5] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int64_le(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int64_le, serializesToLittleEndian)
{
  // Arrange
  byte_t  buffer[8]   = { 0 };
  byte_t  expected[8] = { 0x56, 0xD1, 0x5F, 0xB5, 0x5D, 0xF1, 0x5F, 0xB0 };
  size_t  size        = sizeof(buffer);
  int64_t value       = -5737602015469514410;

  // Act
  cardano_error_t result = cardano_write_int64_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int64_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[11]   = { 0 };
  byte_t  expected[11] = { 0, 0, 0, 0x56, 0xD1, 0x5F, 0xB5, 0x5D, 0xF1, 0x5F, 0xB0 };
  size_t  size         = sizeof(buffer);
  int64_t value        = -5737602015469514410;

  // Act
  cardano_error_t result = cardano_write_int64_le(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float32_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[1] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = -26.0;

  // Act
  cardano_error_t result = cardano_write_float32_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_float32_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[5] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = -26.0;

  // Act
  cardano_error_t result = cardano_write_float32_le(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_float32_le, serializesToLittleEndian)
{
  // Arrange
  byte_t    buffer[4]   = { 0 };
  byte_t    expected[4] = { 0x47, 0x55, 0x93, 0x3f };
  size_t    size        = sizeof(buffer);
  float32_t value       = 1.15104;

  // Act
  cardano_error_t result = cardano_write_float32_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float32_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[7]   = {};
  byte_t    expected[7] = { 0, 0, 0, 0x47, 0x55, 0x93, 0x3f };
  size_t    size        = sizeof(buffer);
  float32_t value       = 1.15104;

  // Act
  cardano_error_t result = cardano_write_float32_le(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float64_le, serializesToLittleEndian)
{
  // Arrange
  byte_t    buffer[8]   = { 0 };
  byte_t    expected[8] = { 0x44, 0xa6, 0x65, 0x6c, 0x34, 0x1d, 0xfa, 0x3f };
  size_t    size        = sizeof(buffer);
  float64_t value       = 1.632130073;

  // Act
  cardano_error_t result = cardano_write_float64_le(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float64_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[11]   = { 0 };
  byte_t    expected[11] = { 0, 0, 0, 0x44, 0xa6, 0x65, 0x6c, 0x34, 0x1d, 0xfa, 0x3f };
  size_t    size         = sizeof(buffer);
  float64_t value        = 1.632130073;

  // Act
  cardano_error_t result = cardano_write_float64_le(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint16_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint16_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint16_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[5] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint16_be(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint16_be, serializesToBigEndian)
{
  // Arrange
  byte_t   buffer[2]   = { 0 };
  byte_t   expected[2] = { 0x04, 0xEA };
  size_t   size        = sizeof(buffer);
  uint16_t value       = 1258;

  // Act
  cardano_error_t result = cardano_write_uint16_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint16_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t   expected[7] = { 0, 0, 0, 0, 0, 0x04, 0xEA };
  size_t   size        = sizeof(buffer);
  uint16_t value       = 1258;

  // Act
  cardano_error_t result = cardano_write_uint16_be(value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint32_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint32_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint32_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[5] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint32_be(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint32_be, serializesToBigEndian)
{
  // Arrange
  byte_t   buffer[4]   = { 0 };
  byte_t   expected[4] = { 0x04, 0xEA, 0x20, 0xAA };
  size_t   size        = sizeof(buffer);
  uint32_t value       = 82452650;

  // Act
  cardano_error_t result = cardano_write_uint32_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint32_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t   expected[7] = { 0, 0, 0, 0x04, 0xEA, 0x20, 0xAA };
  size_t   size        = sizeof(buffer);
  uint32_t value       = 82452650;

  // Act
  cardano_error_t result = cardano_write_uint32_be(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint64_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint64_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint64_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[5] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 26;

  // Act
  cardano_error_t result = cardano_write_uint64_be(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_uint64_be, serializesToBigEndian)
{
  // Arrange
  byte_t   buffer[8]   = { 0 };
  byte_t   expected[8] = { 0x04, 0xEA, 0x20, 0xAA, 0x04, 0xEA, 0x20, 0xAA };
  size_t   size        = sizeof(buffer);
  uint64_t value       = 354131435300987050;

  // Act
  cardano_error_t result = cardano_write_uint64_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_uint64_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[12]   = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  byte_t   expected[12] = { 0, 0, 0, 0x04, 0xEA, 0x20, 0xAA, 0x04, 0xEA, 0x20, 0xAA };
  size_t   size         = sizeof(buffer);
  uint64_t value        = 354131435300987050;

  // Act
  cardano_error_t result = cardano_write_uint64_be(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int16_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int16_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int16_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[5] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int16_be(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int16_be, serializesToBigEndian)
{
  // Arrange
  byte_t  buffer[2]   = { 0 };
  byte_t  expected[2] = { 0xFB, 0x16 };
  size_t  size        = sizeof(buffer);
  int16_t value       = -1258;

  // Act
  cardano_error_t result = cardano_write_int16_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int16_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t  expected[7] = { 0, 0, 0, 0, 0, 0xFB, 0x16 };
  size_t  size        = sizeof(buffer);
  int16_t value       = -1258;

  // Act
  cardano_error_t result = cardano_write_int16_be(value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int32_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int32_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int32_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[5] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int32_be(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int32_be, serializesToBigEndian)
{
  // Arrange
  byte_t  buffer[4]   = { 0 };
  byte_t  expected[4] = { 0xFB, 0x15, 0xDF, 0x56 };
  size_t  size        = sizeof(buffer);
  int32_t value       = -82452650;

  // Act
  cardano_error_t result = cardano_write_int32_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int32_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[7]   = { 0, 0, 0, 0, 0, 0, 0 };
  byte_t  expected[7] = { 0, 0, 0, 0xFB, 0x15, 0xDF, 0x56 };
  size_t  size        = sizeof(buffer);
  int32_t value       = -82452650;

  // Act
  cardano_error_t result = cardano_write_int32_be(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int64_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int64_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int64_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[5] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = -26;

  // Act
  cardano_error_t result = cardano_write_int64_be(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_int64_be, serializesToBigEndian)
{
  // Arrange
  byte_t  buffer[8]   = { 0 };
  byte_t  expected[8] = { 0xB0, 0x5F, 0xF1, 0x5D, 0xB5, 0x5F, 0xD1, 0x56 };
  size_t  size        = sizeof(buffer);
  int64_t value       = -5737602015469514410;

  // Act
  cardano_error_t result = cardano_write_int64_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_int64_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[11]   = { 0 };
  byte_t  expected[11] = { 0, 0, 0, 0xB0, 0x5F, 0xF1, 0x5D, 0xB5, 0x5F, 0xD1, 0x56 };
  size_t  size         = sizeof(buffer);
  int64_t value        = -5737602015469514410;

  // Act
  cardano_error_t result = cardano_write_int64_be(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float32_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[1] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = -26.0;

  // Act
  cardano_error_t result = cardano_write_float32_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_float32_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[5] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = -26.0;

  // Act
  cardano_error_t result = cardano_write_float32_be(value, buffer, size, 4);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_write_float32_be, serializesToBigEndian)
{
  // Arrange
  byte_t    buffer[4]   = { 0 };
  byte_t    expected[4] = { 0x3f, 0x93, 0x55, 0x47 };
  size_t    size        = sizeof(buffer);
  float32_t value       = 1.15104;

  // Act
  cardano_error_t result = cardano_write_float32_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float32_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[7]   = {};
  byte_t    expected[7] = { 0, 0, 0, 0x3f, 0x93, 0x55, 0x47 };
  size_t    size        = sizeof(buffer);
  float32_t value       = 1.15104;

  // Act
  cardano_error_t result = cardano_write_float32_be(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float64_be, serializesToBigEndian)
{
  // Arrange
  byte_t    buffer[8]   = { 0 };
  byte_t    expected[8] = { 0x3f, 0xfa, 0x1d, 0x34, 0x6c, 0x65, 0xa6, 0x44 };
  size_t    size        = sizeof(buffer);
  float64_t value       = 1.632130073;

  // Act
  cardano_error_t result = cardano_write_float64_be(value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_write_float64_be, serializesToBigEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[11]   = { 0 };
  byte_t    expected[11] = { 0, 0, 0, 0x3f, 0xfa, 0x1d, 0x34, 0x6c, 0x65, 0xa6, 0x44 };
  size_t    size         = sizeof(buffer);
  float64_t value        = 1.632130073;

  // Act
  cardano_error_t result = cardano_write_float64_be(value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(buffer, testing::ElementsAreArray(expected));
}

TEST(cardano_read_uint16_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint16_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[4] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint16_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t   buffer[2] = { 0xEA, 0x04 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 1258);
}

TEST(cardano_read_uint16_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[7] = { 0, 0, 0, 0, 0, 0xEA, 0x04 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 1258);
}

TEST(cardano_read_uint32_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint32_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[4] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint32_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t   buffer[4] = { 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 82452650);
}

TEST(cardano_read_uint32_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[9] = { 0, 0, 0, 0, 0, 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 82452650);
}

TEST(cardano_read_uint64_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint64_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[4] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint64_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t   buffer[8] = { 0xAA, 0x20, 0xEA, 0x04, 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 354131435300987050);
}

TEST(cardano_read_uint64_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[13] = { 0, 0, 0, 0, 0, 0xAA, 0x20, 0xEA, 0x04, 0xAA, 0x20, 0xEA, 0x04 };
  size_t   size       = sizeof(buffer);
  uint64_t value      = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 354131435300987050);
}

TEST(cardano_read_int16_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int16_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[4] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int16_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t  buffer[2] = { 0x16, 0xFB };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -1258);
}

TEST(cardano_read_int16_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[7] = { 0, 0, 0, 0, 0, 0x16, 0xFB };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -1258);
}

TEST(cardano_read_int32_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int32_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[4] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int32_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t  buffer[4] = { 0x56, 0xDF, 0x15, 0xFB };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -82452650);
}

TEST(cardano_read_int32_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[9] = { 0, 0, 0, 0, 0, 0x56, 0xDF, 0x15, 0xFB };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -82452650);
}

TEST(cardano_read_int64_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int64_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int64_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[4] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int64_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int64_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t  buffer[8] = { 0x56, 0xD1, 0x5F, 0xB5, 0x5D, 0xF1, 0x5F, 0xB0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int64_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -5737602015469514410);
}

TEST(cardano_read_int64_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[13] = { 0, 0, 0, 0, 0, 0x56, 0xD1, 0x5F, 0xB5, 0x5D, 0xF1, 0x5F, 0xB0 };
  size_t  size       = sizeof(buffer);
  int64_t value      = 0;

  // Act
  cardano_error_t result = cardano_read_int64_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -5737602015469514410);
}

TEST(cardano_read_float32_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[1] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float32_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[4] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float32_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t    buffer[4] = { 0x47, 0x55, 0x93, 0x3f };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.15104, 0.0000001);
}

TEST(cardano_read_float32_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[9] = { 0, 0, 0, 0, 0, 0x47, 0x55, 0x93, 0x3f };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.15104, 0.0000001);
}

TEST(cardano_read_float64_le, bufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[1] = { 0 };
  size_t    size      = sizeof(buffer);
  float64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float64_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float64_le, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[4] = { 0 };
  size_t    size      = sizeof(buffer);
  float64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float64_le(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float64_le, deserializesFromBigEndian)
{
  // Arrange
  byte_t    buffer[8] = { 0x44, 0xa6, 0x65, 0x6c, 0x34, 0x1d, 0xfa, 0x3f };
  size_t    size      = sizeof(buffer);
  float64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float64_le(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.632130073, 0.000000001);
}

TEST(cardano_read_float64_le, serializesToLittleEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[13] = { 0, 0, 0, 0, 0, 0x44, 0xa6, 0x65, 0x6c, 0x34, 0x1d, 0xfa, 0x3f };
  size_t    size       = sizeof(buffer);
  float64_t value      = 0;

  // Act
  cardano_error_t result = cardano_read_float64_le(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.632130073, 0.000000001);
}

TEST(cardano_read_uint16_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint16_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[4] = { 0 };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint16_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t   buffer[2] = { 0x04, 0xEA };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 1258);
}

TEST(cardano_read_uint16_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[7] = { 0, 0, 0, 0, 0, 0x04, 0xEA };
  size_t   size      = sizeof(buffer);
  uint16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint16_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 1258);
}

TEST(cardano_read_uint32_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint32_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[4] = { 0 };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint32_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t   buffer[4] = { 0x04, 0xEA, 0x20, 0xAA };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 82452650);
}

TEST(cardano_read_uint32_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[9] = { 0, 0, 0, 0, 0, 0x04, 0xEA, 0x20, 0xAA };
  size_t   size      = sizeof(buffer);
  uint32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint32_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 82452650);
}

TEST(cardano_read_uint64_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[1] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint64_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t   buffer[4] = { 0 };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_uint64_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t   buffer[8] = { 0x04, 0xEA, 0x20, 0xAA, 0x04, 0xEA, 0x20, 0xAA };
  size_t   size      = sizeof(buffer);
  uint64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 354131435300987050);
}

TEST(cardano_read_uint64_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t   buffer[13] = { 0, 0, 0, 0, 0, 0x04, 0xEA, 0x20, 0xAA, 0x04, 0xEA, 0x20, 0xAA };
  size_t   size       = sizeof(buffer);
  uint64_t value      = 0;

  // Act
  cardano_error_t result = cardano_read_uint64_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 354131435300987050);
}

TEST(cardano_read_int16_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int16_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[4] = { 0 };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int16_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t  buffer[2] = { 0xFB, 0x16 };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -1258);
}

TEST(cardano_read_int16_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[7] = { 0, 0, 0, 0, 0, 0xFB, 0x16 };
  size_t  size      = sizeof(buffer);
  int16_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int16_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -1258);
}

TEST(cardano_read_int32_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int32_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[4] = { 0 };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int32_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t  buffer[4] = { 0xFB, 0x15, 0xDF, 0x56 };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -82452650);
}

TEST(cardano_read_int32_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[9] = { 0, 0, 0, 0, 0, 0xFB, 0x15, 0xDF, 0x56 };
  size_t  size      = sizeof(buffer);
  int32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int32_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -82452650);
}

TEST(cardano_read_int64_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[1] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int64_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int64_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t  buffer[4] = { 0 };
  size_t  size      = sizeof(buffer);
  int64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int64_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_int64_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t  buffer[8] = { 0xB0, 0x5F, 0xF1, 0x5D, 0xB5, 0x5F, 0xD1, 0x56 };
  size_t  size      = sizeof(buffer);
  int64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_int64_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -5737602015469514410);
}

TEST(cardano_read_int64_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t  buffer[13] = { 0, 0, 0, 0, 0, 0xB0, 0x5F, 0xF1, 0x5D, 0xB5, 0x5F, 0xD1, 0x56 };
  size_t  size       = sizeof(buffer);
  int64_t value      = 0;

  // Act
  cardano_error_t result = cardano_read_int64_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -5737602015469514410);
}

TEST(cardano_read_float32_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[1] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float32_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[4] = { 0 };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float32_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t    buffer[4] = { 0x3f, 0x93, 0x55, 0x47 };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.15104, 0.0000001);
}

TEST(cardano_read_float32_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[9] = { 0, 0, 0, 0, 0, 0x3f, 0x93, 0x55, 0x47 };
  size_t    size      = sizeof(buffer);
  float32_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float32_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.15104, 0.0000001);
}

TEST(cardano_read_float64_be, bufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[1] = { 0 };
  size_t    size      = sizeof(buffer);
  float64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float64_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float64_be, positiveOffsetBufferIsInsufficientSize)
{
  // Arrange
  byte_t    buffer[4] = { 0 };
  size_t    size      = sizeof(buffer);
  float64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float64_be(&value, buffer, size, 3);

  // Assert
  ASSERT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_read_float64_be, deserializesFromBigEndian)
{
  // Arrange
  byte_t    buffer[8] = { 0x3f, 0xfa, 0x1d, 0x34, 0x6c, 0x65, 0xa6, 0x44 };
  size_t    size      = sizeof(buffer);
  float64_t value     = 0;

  // Act
  cardano_error_t result = cardano_read_float64_be(&value, buffer, size, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.632130073, 0.000000001);
}

TEST(cardano_read_float64_be, serializesToLittbeEndianWithPositiveOffset)
{
  // Arrange
  byte_t    buffer[13] = { 0, 0, 0, 0, 0, 0x3f, 0xfa, 0x1d, 0x34, 0x6c, 0x65, 0xa6, 0x44 };
  size_t    size       = sizeof(buffer);
  float64_t value      = 0;

  // Act
  cardano_error_t result = cardano_read_float64_be(&value, buffer, size, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.632130073, 0.000000001);
}
