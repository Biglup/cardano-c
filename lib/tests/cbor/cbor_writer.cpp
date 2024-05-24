/**
 * \file cbor.cpp
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

#include <cardano/cbor/cbor_writer.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/buffer.h>
#include <gmock/gmock.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * Writes a typed value to the buffer.
 *
 * @param writer The CBOR writer.
 * @param value The value.
 * @param hex The expected hex string.
 */
static void
test_unsigned_int(cardano_cbor_writer_t* writer, uint64_t value, const char* hex)
{
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, value), CARDANO_SUCCESS);
  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), hex);

  free(encoded_hex);

  EXPECT_EQ(cardano_cbor_writer_reset(writer), CARDANO_SUCCESS);
}

/**
 * Writes a typed value to the buffer.
 *
 * @param writer The CBOR writer.
 * @param value The value.
 * @param hex The expected hex string.
 */
static void
test_signed_int(cardano_cbor_writer_t* writer, int64_t value, const char* hex)
{
  EXPECT_EQ(cardano_cbor_writer_write_signed_int(writer, value), CARDANO_SUCCESS);
  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), hex);

  free(encoded_hex);

  EXPECT_EQ(cardano_cbor_writer_reset(writer), CARDANO_SUCCESS);
}

/**
 * Writes a typed value to the buffer.
 *
 * @param writer The CBOR writer.
 * @param text The text to be encoded.
 * @param hex The expected hex string.
 */
static void
test_text_string(cardano_cbor_writer_t* writer, const char* text, const char* hex)
{
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, text, strlen(text)), CARDANO_SUCCESS);
  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), hex);

  free(encoded_hex);

  EXPECT_EQ(cardano_cbor_writer_reset(writer), CARDANO_SUCCESS);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_writer_new, createsANewObjectWithRefCountOne)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  // Act
  writer = cardano_cbor_writer_new();

  // Assert
  EXPECT_THAT(writer, testing::Not((cardano_cbor_writer_t*)nullptr));
  EXPECT_EQ(cardano_cbor_writer_refcount(writer), 1);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  // Act
  writer = cardano_cbor_writer_new();
  cardano_cbor_writer_ref(writer);

  // Assert
  EXPECT_THAT(writer, testing::Not((cardano_cbor_writer_t*)nullptr));
  EXPECT_EQ(cardano_cbor_writer_refcount(writer), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_new, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  writer = cardano_cbor_writer_new();

  // Assert
  EXPECT_EQ(writer, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_cbor_writer_new, returnsNullIfMemoryAllocationEventuallyFails)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  writer = cardano_cbor_writer_new();

  // Assert
  EXPECT_EQ(writer, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_cbor_writer_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cbor_writer_ref(nullptr);
}

TEST(cardano_cbor_writer_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  // Act
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cbor_writer_unref((cardano_cbor_writer_t**)nullptr);
}

TEST(cardano_cbor_writer_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ;

  // Act
  cardano_cbor_writer_ref(writer);
  size_t ref_count = cardano_cbor_writer_refcount(writer);

  cardano_cbor_writer_unref(&writer);
  size_t updated_ref_count = cardano_cbor_writer_refcount(writer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_cbor_writer_ref(writer);
  size_t ref_count = cardano_cbor_writer_refcount(writer);

  cardano_cbor_writer_unref(&writer);
  size_t updated_ref_count = cardano_cbor_writer_refcount(writer);

  cardano_cbor_writer_unref(&writer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(writer, (cardano_cbor_writer_t*)nullptr);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_cbor_writer_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_cbor_writer_tag, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_tag(nullptr, CARDANO_CBOR_TAG_UNIX_TIME_SECONDS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_bool, returnsErrorIfGivenANullPtr)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_bool(nullptr, true);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_tag, writesATag)
{
  // Arrange
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  byte_t                 expected[] = { 0xC1 };
  byte_t                 buffer[10] = { 0 };

  // Act

  cardano_error_t write_result  = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_UNIX_TIME_SECONDS);
  size_t          required_size = cardano_cbor_writer_get_encode_size(writer);
  cardano_error_t encode_result = cardano_cbor_writer_encode(writer, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  EXPECT_EQ(encode_result, CARDANO_SUCCESS);
  EXPECT_THAT(expected, testing::ElementsAreArray(buffer, required_size));

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_tag, writesNestedTaggedValues)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_DATE_TIME_STRING), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_DATE_TIME_STRING), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_DATE_TIME_STRING), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "2013-03-21T20:04:00Z", strlen("2013-03-21T20:04:00Z")), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(std::string(encoded_hex), "c0c0c074323031332d30332d32315432303a30343a30305a");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_tag, writesSingleTaggedUnixTimeSeconds)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result      = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_UNIX_TIME_SECONDS);
  cardano_error_t write_uint_result = cardano_cbor_writer_write_unsigned_int(writer, 1363896240);
  const size_t    hex_string_size   = cardano_cbor_writer_get_hex_size(writer);
  char*           encoded_hex       = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  EXPECT_EQ(write_uint_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "c11a514b67b0");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  _cardano_free(encoded_hex);
}

TEST(cardano_cbor_writer_write_big_integer, writesTheValueAsATaggedBignumEncoding)
{
  // Arrange
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  byte_t                 expected[] = { 0xC2, 0x02 };
  byte_t                 buffer[10] = { 0 };

  // Act
  cardano_error_t write_result  = cardano_cbor_writer_write_big_integer(writer, 2);
  size_t          required_size = cardano_cbor_writer_get_encode_size(writer);
  cardano_error_t encode_result = cardano_cbor_writer_encode(writer, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  EXPECT_EQ(encode_result, CARDANO_SUCCESS);
  EXPECT_EQ(required_size, 2);
  EXPECT_THAT(expected, testing::ElementsAreArray(buffer, required_size));

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_big_integer, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_big_integer(nullptr, 2);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_start_array, writesTheStartOfAnArray)
{
  // Arrange
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  byte_t                 expected[] = { 0x82 };
  byte_t                 buffer[10] = { 0 };

  // Act
  cardano_error_t write_result  = cardano_cbor_writer_write_start_array(writer, 2);
  size_t          required_size = cardano_cbor_writer_get_encode_size(writer);
  cardano_error_t encode_result = cardano_cbor_writer_encode(writer, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  EXPECT_EQ(encode_result, CARDANO_SUCCESS);
  EXPECT_EQ(required_size, 1);
  EXPECT_THAT(expected, testing::ElementsAreArray(buffer, required_size));

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_start_array, writeAnIndefiniteSizeArray)
{
  // Arrange
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  byte_t                 expected[] = { 0x9F, 0xff };
  byte_t                 buffer[10] = { 0 };

  // Assert
  EXPECT_EQ(cardano_cbor_writer_write_start_array(writer, -1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_end_array(writer), CARDANO_SUCCESS);
  size_t required_size = cardano_cbor_writer_get_encode_size(writer);
  EXPECT_EQ(cardano_cbor_writer_encode(writer, buffer, sizeof(buffer)), CARDANO_SUCCESS);
  EXPECT_EQ(required_size, 2);
  EXPECT_THAT(expected, testing::ElementsAreArray(buffer, required_size));

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_start_array, writeAnIndefiniteSizeArrayWithAnElement)
{
  // Arrange
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  byte_t                 expected[] = { 0x9F, 0x18, 0x2a, 0xff };
  byte_t                 buffer[10] = { 0 };

  // Assert
  EXPECT_EQ(cardano_cbor_writer_write_start_array(writer, -1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_signed_int(writer, 42), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_end_array(writer), CARDANO_SUCCESS);
  size_t required_size = cardano_cbor_writer_get_encode_size(writer);
  EXPECT_EQ(cardano_cbor_writer_encode(writer, buffer, sizeof(buffer)), CARDANO_SUCCESS);
  EXPECT_EQ(required_size, 4);
  EXPECT_THAT(expected, testing::ElementsAreArray(buffer, required_size));

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_start_array, writeAnIndefiniteSizeArrayWithSeveralElements)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Assert

  EXPECT_EQ(cardano_cbor_writer_write_start_array(writer, -1), CARDANO_SUCCESS);

  for (size_t i = 0; i < 25; ++i)
  {
    EXPECT_EQ(cardano_cbor_writer_write_signed_int(writer, i + 1), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_cbor_writer_write_end_array(writer), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  ASSERT_EQ(std::string(encoded_hex), "9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_start_array, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_start_array(nullptr, 2);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_start_array, writeArrayWithOneUnsignedNumber)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result      = cardano_cbor_writer_write_start_array(writer, 1);
  cardano_error_t write_uint_result = cardano_cbor_writer_write_unsigned_int(writer, 42);
  const size_t    hex_string_size   = cardano_cbor_writer_get_hex_size(writer);
  char*           encoded_hex       = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  EXPECT_EQ(write_uint_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "81182a");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_start_array, writeArrayWithSeveralUnsignedNumber)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_start_array(writer, 25);

  for (size_t i = 0; i < 25; ++i)
  {
    EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, i + 1), CARDANO_SUCCESS);
  }

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "98190102030405060708090a0b0c0d0e0f101112131415161718181819");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_start_array, writeArrayWithMixedTypes)
{
  // Arrange
  cardano_cbor_writer_t* writer   = cardano_cbor_writer_new();
  byte_t                 array[1] = { 7 };

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_start_array(writer, 4);

  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_signed_int(writer, -1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "", strlen("")), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_byte_string(writer, array, sizeof(array)), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "840120604107");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_start_array, writeArrayOfStrings)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_start_array(writer, 3);

  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "lorem", strlen("lorem")), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "ipsum", strlen("ipsum")), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "dolor", strlen("dolor")), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "83656c6f72656d65697073756d65646f6c6f72");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_start_array, writeArrayWithSimpleValues)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_start_array(writer, 3);

  EXPECT_EQ(cardano_cbor_writer_write_bool(writer, false), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_bool(writer, true), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_null(writer), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_undefined(writer), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "83f4f5f6f7");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_start_array, writeArrayWithNestedArrays)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_cbor_writer_write_start_array(writer, 3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_start_array(writer, 2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_start_array(writer, 2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 5), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(std::string(encoded_hex), "8301820203820405");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_byte_string, writeByteString)
{
  // Arrange
  cardano_cbor_writer_t* writer   = cardano_cbor_writer_new();
  byte_t                 array[4] = { 0x01, 0x02, 0x03, 0x04 };

  // Act
  cardano_error_t write_result    = cardano_cbor_writer_write_byte_string(writer, array, sizeof(array));
  const size_t    hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*           encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "4401020304");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_unsigned_int, writeUnsignedIntegers)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Assert
  test_unsigned_int(writer, 0, "00");
  test_unsigned_int(writer, 1, "01");
  test_unsigned_int(writer, 10, "0a");
  test_unsigned_int(writer, 23, "17");
  test_unsigned_int(writer, 24, "1818");
  test_unsigned_int(writer, 25, "1819");
  test_unsigned_int(writer, 100, "1864");
  test_unsigned_int(writer, 1000, "1903e8");
  test_unsigned_int(writer, 1000000, "1a000f4240");
  test_unsigned_int(writer, 1000000000000, "1b000000e8d4a51000");
  test_unsigned_int(writer, 255, "18ff");
  test_unsigned_int(writer, 256, "190100");
  test_unsigned_int(writer, 4294967295, "1affffffff");
  test_unsigned_int(writer, 9223372036854775807, "1b7fffffffffffffff");
  test_unsigned_int(writer, 4294967296, "1b0000000100000000");
  test_unsigned_int(writer, 65535, "19ffff");
  test_unsigned_int(writer, 65536, "1a00010000");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_signed_int, writeSignedIntegers)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Assert
  test_signed_int(writer, -1, "20");
  test_signed_int(writer, -10, "29");
  test_signed_int(writer, -24, "37");
  test_signed_int(writer, -100, "3863");
  test_signed_int(writer, -1000, "3903e7");
  test_signed_int(writer, -256, "38ff");
  test_signed_int(writer, -257, "390100");
  test_signed_int(writer, -65536, "39ffff");
  test_signed_int(writer, -65537, "3a00010000");
  test_signed_int(writer, -4294967296, "3affffffff");
  test_signed_int(writer, -4294967297, "3b0000000100000000");
  test_signed_int(writer, -9223372036854775807, "3b7ffffffffffffffe");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_text_string, canWriteFixedLenghtStrings)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Assert
  test_text_string(writer, "", "60");
  test_text_string(writer, "a", "6161");
  test_text_string(writer, "IETF", "6449455446");
  test_text_string(writer, "\"\\", "62225c");
  test_text_string(writer, "\u00FC", "62c3bc");
  test_text_string(writer, "\u6C34", "63e6b0b4");
  test_text_string(writer, "\u03BB", "62cebb");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_start_map, carnWriteFixedLengthMapsWithNestedTypes)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_cbor_writer_write_start_map(writer, 2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "a", strlen("a")), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_start_map(writer, 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "b", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_start_map(writer, 2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "x", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_signed_int(writer, -1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "y", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_start_map(writer, 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "z", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 0), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(std::string(encoded_hex), "a26161a102036162a26178206179a1617a00");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_start_map, canWriteUndefiniteLenghtMaps)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_cbor_writer_write_start_map(writer, -1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "a", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "A", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "b", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "B", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "c", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "C", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "d", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "D", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "e", 1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_write_text_string(writer, "E", 1), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_cbor_writer_write_end_map(writer), CARDANO_SUCCESS);

  const size_t hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*        encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(std::string(encoded_hex), "bf6161614161626142616361436164614461656145ff");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_write_encoded, writesEncodedValues)
{
  // Arrange
  cardano_cbor_writer_t* writer   = cardano_cbor_writer_new();
  byte_t                 array[4] = { 0x01, 0x02, 0x03, 0x04 };

  // Act
  cardano_error_t write_result    = cardano_cbor_writer_write_encoded(writer, array, sizeof(array));
  const size_t    hex_string_size = cardano_cbor_writer_get_hex_size(writer);
  char*           encoded_hex     = (char*)malloc(hex_string_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, encoded_hex, hex_string_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  ASSERT_EQ(std::string(encoded_hex), "01020304");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  free(encoded_hex);
}

TEST(cardano_cbor_writer_encode_hex, returnsErrorIfGivenANullWriter)
{
  // Arrange
  char buffer[10] = { 0 };

  // Act
  cardano_error_t encode_result = cardano_cbor_writer_encode_hex(nullptr, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(encode_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_encode_hex, returnsErrorIfGivenNullData)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t encode_result = cardano_cbor_writer_encode_hex(writer, nullptr, 0);

  // Assert
  EXPECT_EQ(encode_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_get_hex_size, returnsZeroIfGivenANullWriter)
{
  // Act
  size_t hex_size = cardano_cbor_writer_get_hex_size(nullptr);

  // Assert
  EXPECT_EQ(hex_size, 0);
}

TEST(cardano_cbor_writer_write_byte_string, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_byte_string(nullptr, nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_byte_string, returnsErrorIfGivenNullData)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_byte_string(writer, nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_text_string, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_text_string(nullptr, nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_text_string, returnsErrorIfGivenNullData)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_text_string(writer, nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_encoded, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_encoded(nullptr, nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_encoded, returnsErrorIfGivenNullData)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_encoded(writer, nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_write_end_array, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_end_array(nullptr);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_start_map, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_start_map(nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_unsigned_int, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_unsigned_int(nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_signed_int, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_signed_int(nullptr, 0);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_null, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_null(nullptr);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_write_undefined, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_undefined(nullptr);

  // Assert
  EXPECT_EQ(write_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_encode, returnsErrorIfGivenANullWriter)
{
  // Arrange
  byte_t buffer[10] = { 0 };

  // Act
  cardano_error_t encode_result = cardano_cbor_writer_encode(nullptr, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(encode_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_encode, returnsErrorIfGivenNullData)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t encode_result = cardano_cbor_writer_encode(writer, nullptr, 0);

  // Assert
  EXPECT_EQ(encode_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_reset, returnsErrorIfGivenANullWriter)
{
  // Act
  cardano_error_t reset_result = cardano_cbor_writer_reset(nullptr);

  // Assert
  EXPECT_EQ(reset_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_encode, returnErrorWhenOutputBufferIsInsufficient)
{
  // Arrange
  cardano_cbor_writer_t* writer    = cardano_cbor_writer_new();
  byte_t                 array[4]  = { 0x01, 0x02, 0x03, 0x04 };
  byte_t                 output[1] = { 0x00 };

  // Act
  cardano_error_t write_result = cardano_cbor_writer_write_encoded(writer, array, sizeof(array));

  cardano_error_t encode_result = cardano_cbor_writer_encode(writer, output, sizeof(output));

  // Assert
  EXPECT_EQ(write_result, CARDANO_SUCCESS);
  EXPECT_EQ(encode_result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_cbor_writer_t* writer  = cardano_cbor_writer_new();
  const char*            message = "This is a test message";

  // Act
  cardano_cbor_writer_set_last_error(writer, message);
  const char* last_error = cardano_cbor_writer_get_last_error(writer);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  // Act
  const char* last_error = cardano_cbor_writer_get_last_error(writer);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_cbor_writer_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer  = nullptr;
  const char*            message = "This is a test message";

  // Act
  cardano_cbor_writer_set_last_error(writer, message);

  // Assert
  EXPECT_STREQ(cardano_cbor_writer_get_last_error(writer), "Object is NULL.");
}

TEST(cardano_cbor_writer_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer  = cardano_cbor_writer_new();
  const char*            message = nullptr;

  // Act
  cardano_cbor_writer_set_last_error(writer, message);

  // Assert
  EXPECT_STREQ(cardano_cbor_writer_get_last_error(writer), "");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_get_encode_size, returnsZeroIfGivenaNullPtr)
{
  // Act
  size_t encode_size = cardano_cbor_writer_get_encode_size(nullptr);

  // Assert
  EXPECT_EQ(encode_size, 0);
}

TEST(cardano_cbor_writer_encode_in_buffer, returnsErrorIfGivenANullWriter)
{
  // Arrange
  cardano_buffer_t* buffer = NULL;

  // Act
  cardano_error_t encode_result = cardano_cbor_writer_encode_in_buffer(NULL, &buffer);

  // Assert
  EXPECT_EQ(encode_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_writer_encode_in_buffer, returnsErrorIfGivenANullBuffer)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t encode_result = cardano_cbor_writer_encode_in_buffer(writer, NULL);

  // Assert
  EXPECT_EQ(encode_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_encode_in_buffer, returnsEncodedDataInBuffer)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_buffer_t*      buffer = NULL;

  // Act
  EXPECT_EQ(cardano_cbor_writer_write_unsigned_int(writer, 42), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &buffer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_buffer_get_size(buffer), 2);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[0], 0x18);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[1], 0x2a);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  cardano_buffer_unref(&buffer);
}
