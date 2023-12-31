/**
 * \file buffer.cpp
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

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_buffer_new, createsANewBufferWithTheGivenCapacity)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  buffer = cardano_buffer_new(1000);

  // Assert
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));
  EXPECT_EQ(cardano_buffer_get_size(buffer), 0);
  EXPECT_EQ(cardano_buffer_get_capacity(buffer), 1000);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_new_from, returnsNullIfGivenNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  buffer = cardano_buffer_new_from(nullptr, 8);

  // Assert
  EXPECT_EQ(buffer, nullptr);
}

TEST(cardano_buffer_new_from, createsANewBufferWithTheGivenCapacity)
{
  // Arrange
  cardano_buffer_t* buffer      = nullptr;
  byte_t            expected[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

  // Act
  buffer = cardano_buffer_new_from(&expected[0], 8);

  // Assert
  EXPECT_EQ(cardano_buffer_get_size(buffer), 8);
  ASSERT_EQ(cardano_buffer_refcount(buffer), 1);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  buffer = cardano_buffer_new(1);
  cardano_buffer_ref(buffer);

  // Assert
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));
  EXPECT_EQ(cardano_buffer_refcount(buffer), 2);

  // Cleanup
  cardano_buffer_unref(&buffer);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_ref, doesntCrashIfBufferIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  cardano_buffer_ref(nullptr);

  // Assert
  EXPECT_EQ(buffer, nullptr);
}

TEST(cardano_buffer_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_buffer_unref((cardano_buffer_t**)nullptr);
}

TEST(cardano_buffer_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(1);

  // Act
  cardano_buffer_ref(buffer);
  size_t ref_count = cardano_buffer_refcount(buffer);

  cardano_buffer_unref(&buffer);
  size_t updated_ref_count = cardano_buffer_refcount(buffer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(1);

  // Act
  cardano_buffer_ref(buffer);
  size_t ref_count = cardano_buffer_refcount(buffer);

  cardano_buffer_unref(&buffer);
  size_t updated_ref_count = cardano_buffer_refcount(buffer);

  cardano_buffer_unref(&buffer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(buffer, (cardano_buffer_t*)nullptr);
}

TEST(cardano_buffer_move, returnsNullIfBufferIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  cardano_buffer_t* moved = cardano_buffer_move(buffer);

  // Assert
  EXPECT_EQ(moved, nullptr);
}

TEST(cardano_buffer_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(1);

  // Act
  EXPECT_THAT(cardano_buffer_move(buffer), testing::Not((cardano_buffer_t*)nullptr));
  size_t ref_count = cardano_buffer_refcount(buffer);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_get_data, returnsNullIfBufferIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  byte_t* data = cardano_buffer_get_data(buffer);

  // Assert
  EXPECT_EQ(data, nullptr);
}

TEST(cardano_buffer_get_size, returnsZeroIfBufferIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  size_t size = cardano_buffer_get_size(buffer);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_buffer_get_capacity, returnsZeroIfBufferIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  size_t size = cardano_buffer_get_capacity(buffer);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_buffer_refcount, returnsZeroIfBufferIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  size_t size = cardano_buffer_refcount(buffer);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_buffer_concat, lhsIsNull)
{
  // Arrange
  cardano_buffer_t* lhs = nullptr;
  cardano_buffer_t* rhs = cardano_buffer_new(1);

  // Act
  cardano_buffer_t* concatenated = cardano_buffer_concat(lhs, rhs);

  // Assert
  ASSERT_EQ(concatenated, (cardano_buffer_t*)nullptr);

  // Cleanup
  cardano_buffer_unref(&rhs);
}

TEST(cardano_buffer_concat, rhsIsNull)
{
  // Arrange
  cardano_buffer_t* lhs = cardano_buffer_new(1);
  cardano_buffer_t* rhs = nullptr;

  // Act
  cardano_buffer_t* concatenated = cardano_buffer_concat(lhs, rhs);

  // Assert
  ASSERT_EQ(concatenated, (cardano_buffer_t*)nullptr);

  // Cleanup
  cardano_buffer_unref(&lhs);
}

TEST(cardano_buffer_concat, returnsTheConcatenatedBuffer)
{
  // Arrange
  cardano_buffer_t* lhs         = cardano_buffer_new(4);
  cardano_buffer_t* rhs         = cardano_buffer_new(4);
  byte_t            expected[8] = { 1, 0, 0, 0, 2, 0, 0, 0 };

  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write_int32_le(lhs, 1));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write_int32_le(rhs, 2));

  // Act
  cardano_buffer_t* concatenated = cardano_buffer_concat(lhs, rhs);

  // Assert
  ASSERT_EQ(cardano_buffer_refcount(concatenated), 1);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(concatenated), cardano_buffer_get_size(concatenated)));

  // Cleanup
  cardano_buffer_unref(&lhs);
  cardano_buffer_unref(&rhs);
  cardano_buffer_unref(&concatenated);
}

TEST(cardano_buffer_slice, bufferIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  cardano_buffer_t* new_slice = cardano_buffer_slice(buffer, 0, 10);

  // Assert
  ASSERT_EQ(new_slice, (cardano_buffer_t*)nullptr);
}

TEST(cardano_buffer_slice, returnNullIfStartOutOfBounds)
{
  // Arrange
  byte_t actual[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };

  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(actual));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &actual[0], sizeof(actual)));

  // Act
  cardano_buffer_t* slice = cardano_buffer_slice(buffer, 100, 4);

  // Assert
  ASSERT_EQ(slice, nullptr);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_slice, returnNullIfEndOutOfBounds)
{
  // Arrange
  byte_t actual[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };

  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(actual));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &actual[0], sizeof(actual)));

  // Act
  cardano_buffer_t* slice = cardano_buffer_slice(buffer, 0, 400);

  // Assert
  ASSERT_EQ(slice, nullptr);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_slice, returnNullIfEndLessThanStart)
{
  // Arrange
  byte_t actual[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };

  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(actual));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &actual[0], sizeof(actual)));

  // Act
  cardano_buffer_t* slice = cardano_buffer_slice(buffer, 3, 1);

  // Assert
  ASSERT_EQ(slice, nullptr);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_slice, returnsTheRightSlice)
{
  // Arrange
  byte_t actual[5]   = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
  byte_t expected[3] = { 0xBB, 0xCC, 0xDD };

  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(actual));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &actual[0], sizeof(actual)));

  // Act
  cardano_buffer_t* slice = cardano_buffer_slice(buffer, 1, 4);

  // Assert
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(slice), cardano_buffer_get_size(slice)));

  // Cleanup
  cardano_buffer_unref(&buffer);
  cardano_buffer_unref(&slice);
}

TEST(cardano_buffer_to_hex, whenGivenANullPtrReturnNull)
{
  // Act
  char* encoded_hex = cardano_buffer_to_hex(nullptr);

  // Assert
  ASSERT_EQ(encoded_hex, nullptr);
}

TEST(cardano_buffer_to_hex, convertBytesToHex)
{
  // Arrange
  std::string       hex          = "aabbccddeeff00112233445566778899";
  byte_t            expected[16] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
  cardano_buffer_t* buffer       = cardano_buffer_new(16);

  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], 16));

  // Act
  char* encoded_hex = cardano_buffer_to_hex(buffer);

  // Assert
  ASSERT_EQ(std::string(hex), std::string(encoded_hex));

  // Cleanup
  cardano_buffer_unref(&buffer);
  free(encoded_hex);
}

TEST(cardano_buffer_from_hex, whenGivenANullPtrReturnNull)
{
  // Arrange
  cardano_buffer_t* buffer     = nullptr;
  const char*       hex_string = nullptr;

  // Act
  buffer = cardano_buffer_from_hex(nullptr, 32);

  // Assert
  ASSERT_EQ(buffer, nullptr);
}

TEST(cardano_buffer_from_hex, whenGivenUnevenCharCountReturnNull)
{
  // Arrange
  cardano_buffer_t* buffer     = nullptr;
  const char*       hex_string = "aabbccddeeff0011223344556677889";

  // Act
  buffer = cardano_buffer_from_hex(hex_string, 31);

  // Assert
  ASSERT_EQ(buffer, nullptr);
}

TEST(cardano_buffer_from_hex, convertHexToBytes)
{
  // Arrange
  cardano_buffer_t* buffer       = nullptr;
  const char*       hex_string   = "aabbccddeeff00112233445566778899";
  byte_t            expected[16] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };

  // Act
  buffer = cardano_buffer_from_hex(hex_string, 32);

  // Assert
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write, returnsErrorIfGivenNullBuffer)
{
  // Arrange
  byte_t actual[5] = { 0x00 };

  // Act
  cardano_error_t result = cardano_buffer_write(nullptr, &actual[0], 5);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write, returnsErrorIfGivenNullInputPtr)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(16);

  // Act
  cardano_error_t result = cardano_buffer_write(buffer, nullptr, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read, returnsErrorIfGivenNullBuffer)
{
  // Act
  cardano_error_t result = cardano_buffer_read(nullptr, nullptr, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read, returnsErrorIfGivenNullOutputPointer)
{
  // Arrange
  byte_t            bytes[16] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
  cardano_buffer_t* buffer    = cardano_buffer_new(16);

  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &bytes[0], 16));

  // Act
  cardano_error_t result = cardano_buffer_read(buffer, nullptr, 5);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read, returnsErrorIfTriesToReadOutOfBounds)
{
  // Arrange
  byte_t            bytes[16] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
  byte_t            actual[5] = { 0x00 };
  cardano_buffer_t* buffer    = cardano_buffer_new(16);

  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &bytes[0], 16));

  // Act
  cardano_error_t result = cardano_buffer_read(buffer, &actual[0], 100);

  // Assert
  ASSERT_EQ(result, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read, readBytes)
{
  // Arrange
  byte_t            bytes[16]   = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
  byte_t            expected[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
  byte_t            actual[5]   = { 0x00 };
  cardano_buffer_t* buffer      = cardano_buffer_new(16);

  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &bytes[0], 16));

  // Act
  cardano_error_t result = cardano_buffer_read(buffer, &actual[0], 5);

  // Assert
  ASSERT_THAT(actual, testing::ElementsAreArray(&expected[0], 5));
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_uint16_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_uint16_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_uint16_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[2] = { 0xEA, 0x04 };
  uint16_t          value       = 1258;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_uint16_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_uint32_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_uint32_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_uint32_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[4] = { 0xAA, 0x20, 0xEA, 0x04 };
  uint32_t          value       = 82452650;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_uint32_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_uint64_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_uint64_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_uint64_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[8] = { 0xAA, 0x20, 0xEA, 0x04, 0xAA, 0x20, 0xEA, 0x04 };
  uint64_t          value       = 354131435300987050;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_uint64_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_int16_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_int16_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_int16_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[2] = { 0x16, 0xFB };
  int16_t           value       = -1258;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_int16_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_int32_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_int32_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_int32_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[4] = { 0x56, 0xDF, 0x15, 0xFB };
  int32_t           value       = -82452650;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_int32_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_int64_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_int64_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_int64_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[8] = { 0x56, 0xD1, 0x5F, 0xB5, 0x5D, 0xF1, 0x5F, 0xB0 };
  int64_t           value       = -5737602015469514410;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_int64_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_float_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_float_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_float_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[4] = { 0x47, 0x55, 0x93, 0x3f };
  float             value       = 1.15104;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_float_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_double_le, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_double_le(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_double_le, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[8] = { 0x44, 0xa6, 0x65, 0x6c, 0x34, 0x1d, 0xfa, 0x3f };
  double            value       = 1.632130073;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_double_le(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_uint16_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_uint16_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_uint16_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[2] = { 0x04, 0xEA };
  uint16_t          value       = 1258;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_uint16_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_uint32_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_uint32_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_uint32_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[4] = { 0x04, 0xEA, 0x20, 0xAA };
  uint32_t          value       = 82452650;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_uint32_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_uint64_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_uint64_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_uint64_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[8] = { 0x04, 0xEA, 0x20, 0xAA, 0x04, 0xEA, 0x20, 0xAA };
  uint64_t          value       = 354131435300987050;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_uint64_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_int16_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_int16_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_int16_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[2] = { 0xFB, 0x16 };
  int16_t           value       = -1258;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_int16_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_int32_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_int32_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_int32_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[4] = { 0xFB, 0x15, 0xDF, 0x56 };
  int32_t           value       = -82452650;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_int32_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_int64_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_int64_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_int64_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[8] = { 0xB0, 0x5F, 0xF1, 0x5D, 0xB5, 0x5F, 0xD1, 0x56 };
  int64_t           value       = -5737602015469514410;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_int64_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_float_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_float_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_float_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[4] = { 0x3f, 0x93, 0x55, 0x47 };
  float             value       = 1.15104;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_float_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_write_double_be, bufferIsNullResultsInError)
{
  // Act
  cardano_error_t result = cardano_buffer_write_double_be(nullptr, 1);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_write_double_be, serializesToLittleEndian)
{
  // Arrange
  byte_t            expected[8] = { 0x3f, 0xfa, 0x1d, 0x34, 0x6c, 0x65, 0xa6, 0x44 };
  double            value       = 1.632130073;
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));

  // Act
  cardano_error_t result = cardano_buffer_write_double_be(buffer, value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer)));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint16_le, bufferIsNull)
{
  // Arrange
  uint16_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_uint16_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_uint16_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(uint16_t));

  // Act
  cardano_error_t result = cardano_buffer_read_uint16_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint16_le, canDeserializeValue)
{
  // Arrange
  uint16_t          value       = 0;
  byte_t            expected[2] = { 0xEA, 0x04 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_uint16_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 1258);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint32_le, bufferIsNull)
{
  // Arrange
  uint32_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_uint32_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_uint32_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(uint32_t));

  // Act
  cardano_error_t result = cardano_buffer_read_uint32_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint32_le, canDeserializeValue)
{
  // Arrange
  uint32_t          value       = 0;
  byte_t            expected[4] = { 0xAA, 0x20, 0xEA, 0x04 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_uint32_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 82452650);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint64_le, bufferIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_uint64_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_uint64_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(uint64_t));

  // Act
  cardano_error_t result = cardano_buffer_read_uint64_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint64_le, canDeserializeValue)
{
  // Arrange
  uint64_t          value       = 0;
  byte_t            expected[8] = { 0xAA, 0x20, 0xEA, 0x04, 0xAA, 0x20, 0xEA, 0x04 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_uint64_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 354131435300987050);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int16_le, bufferIsNull)
{
  // Arrange
  int16_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_int16_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_int16_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(int16_t));

  // Act
  cardano_error_t result = cardano_buffer_read_int16_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int16_le, canDeserializeValue)
{
  // Arrange
  int16_t           value       = 0;
  byte_t            expected[2] = { 0x16, 0xFB };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_int16_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -1258);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int32_le, bufferIsNull)
{
  // Arrange
  int32_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_int32_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_int32_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(int32_t));

  // Act
  cardano_error_t result = cardano_buffer_read_int32_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int32_le, canDeserializeValue)
{
  // Arrange
  int32_t           value       = 0;
  byte_t            expected[4] = { 0x56, 0xDF, 0x15, 0xFB };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_int32_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -82452650);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int64_le, bufferIsNull)
{
  // Arrange
  int64_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_int64_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_int64_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(int64_t));

  // Act
  cardano_error_t result = cardano_buffer_read_int64_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int64_le, canDeserializeValue)
{
  // Arrange
  int64_t           value       = 0;
  byte_t            expected[8] = { 0x56, 0xD1, 0x5F, 0xB5, 0x5D, 0xF1, 0x5F, 0xB0 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_int64_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -5737602015469514410);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_float_le, bufferIsNull)
{
  // Arrange
  float value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_float_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_float_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(float));

  // Act
  cardano_error_t result = cardano_buffer_read_float_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_float_le, canDeserializeValue)
{
  // Arrange
  float             value       = 0;
  byte_t            expected[4] = { 0x47, 0x55, 0x93, 0x3f };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_float_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.15104, 0.0000001);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_double_le, bufferIsNull)
{
  // Arrange
  double value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_double_le(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_double_le, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(double));

  // Act
  cardano_error_t result = cardano_buffer_read_double_le(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_double_le, canDeserializeValue)
{
  // Arrange
  double            value       = 0;
  byte_t            expected[8] = { 0x44, 0xa6, 0x65, 0x6c, 0x34, 0x1d, 0xfa, 0x3f };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_double_le(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.632130073, 0.000000001);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint16_be, bufferIsNull)
{
  // Arrange
  uint16_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_uint16_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_uint16_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(uint16_t));

  // Act
  cardano_error_t result = cardano_buffer_read_uint16_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint16_be, canDeserializeValue)
{
  // Arrange
  uint16_t          value       = 0;
  byte_t            expected[2] = { 0x04, 0xEA };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_uint16_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 1258);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint32_be, bufferIsNull)
{
  // Arrange
  uint32_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_uint32_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_uint32_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(uint32_t));

  // Act
  cardano_error_t result = cardano_buffer_read_uint32_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint32_be, canDeserializeValue)
{
  // Arrange
  uint32_t          value       = 0;
  byte_t            expected[4] = { 0x04, 0xEA, 0x20, 0xAA };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_uint32_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 82452650);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint64_be, bufferIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_uint64_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_uint64_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(uint64_t));

  // Act
  cardano_error_t result = cardano_buffer_read_uint64_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_uint64_be, canDeserializeValue)
{
  // Arrange
  uint64_t          value       = 0;
  byte_t            expected[8] = { 0x04, 0xEA, 0x20, 0xAA, 0x04, 0xEA, 0x20, 0xAA };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_uint64_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 354131435300987050);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int16_be, bufferIsNull)
{
  // Arrange
  int16_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_int16_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_int16_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(int16_t));

  // Act
  cardano_error_t result = cardano_buffer_read_int16_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int16_be, canDeserializeValue)
{
  // Arrange
  int16_t           value       = 0;
  byte_t            expected[2] = { 0xFB, 0x16 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_int16_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -1258);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int32_be, bufferIsNull)
{
  // Arrange
  int32_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_int32_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_int32_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(int32_t));

  // Act
  cardano_error_t result = cardano_buffer_read_int32_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int32_be, canDeserializeValue)
{
  // Arrange
  int32_t           value       = 0;
  byte_t            expected[4] = { 0xFB, 0x15, 0xDF, 0x56 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_int32_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -82452650);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int64_be, bufferIsNull)
{
  // Arrange
  int64_t value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_int64_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_int64_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(int64_t));

  // Act
  cardano_error_t result = cardano_buffer_read_int64_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_int64_be, canDeserializeValue)
{
  // Arrange
  int64_t           value       = 0;
  byte_t            expected[8] = { 0xB0, 0x5F, 0xF1, 0x5D, 0xB5, 0x5F, 0xD1, 0x56 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_int64_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, -5737602015469514410);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_float_be, bufferIsNull)
{
  // Arrange
  float value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_float_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_float_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(float));

  // Act
  cardano_error_t result = cardano_buffer_read_float_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_float_be, canDeserializeValue)
{
  // Arrange
  float             value       = 0;
  byte_t            expected[4] = { 0x3f, 0x93, 0x55, 0x47 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_float_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.15104, 0.0000001);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_double_be, bufferIsNull)
{
  // Arrange
  double value = 0;

  // Act
  cardano_error_t result = cardano_buffer_read_double_be(nullptr, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_buffer_read_double_be, valueIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(sizeof(double));

  // Act
  cardano_error_t result = cardano_buffer_read_double_be(buffer, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_read_double_be, canDeserializeValue)
{
  // Arrange
  double            value       = 0;
  byte_t            expected[8] = { 0x3f, 0xfa, 0x1d, 0x34, 0x6c, 0x65, 0xa6, 0x44 };
  cardano_buffer_t* buffer      = cardano_buffer_new(sizeof(value));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_buffer_write(buffer, &expected[0], sizeof(value)));

  // Act
  cardano_error_t result = cardano_buffer_read_double_be(buffer, &value);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NEAR(value, 1.632130073, 0.000000001);
  cardano_buffer_unref(&buffer);
}