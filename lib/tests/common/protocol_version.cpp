/**
 * \file protocol_version.cpp
 *
 * \author angel.castillo
 * \date   Apr 14, 2024
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

#include <cardano/error.h>

#include <cardano/buffer.h>
#include <cardano/common/protocol_version.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PROTOCOL_VERSION_CBOR = "820103";

/* UNIT TESTS ****************************************************************/

TEST(cardano_protocol_version_new, canCreateProtocolVersion)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_version_new(1, 3, &protocol_version);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(protocol_version, testing::Not((cardano_protocol_version_t*)nullptr));

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_new, returnsErrorIfProtocolVersionIsNull)
{
  // Act
  cardano_error_t error = cardano_protocol_version_new(1, 3, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_version_new, returnsErrorIfmajorAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_protocol_version_t* protocol_version = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_version_new(1, 3, &protocol_version);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(protocol_version, (cardano_protocol_version_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_version_to_cbor, canSerializeProtocolVersion)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_protocol_version_to_cbor(protocol_version, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(PROTOCOL_VERSION_CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, PROTOCOL_VERSION_CBOR);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_protocol_version_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_protocol_version_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_protocol_version_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;

  cardano_error_t error = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_protocol_version_to_cbor(protocol_version, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_from_cbor, canDeserializeProtocolVersion)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(PROTOCOL_VERSION_CBOR, strlen(PROTOCOL_VERSION_CBOR));

  // Act
  cardano_error_t error = cardano_protocol_version_from_cbor(reader, &protocol_version);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(protocol_version, testing::Not((cardano_protocol_version_t*)nullptr));

  const uint64_t minor = cardano_protocol_version_get_minor(protocol_version);
  const uint64_t major = cardano_protocol_version_get_major(protocol_version);

  EXPECT_EQ(minor, 3);
  EXPECT_EQ(major, 1);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_version_from_cbor, returnErrorIfProtocolVersionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PROTOCOL_VERSION_CBOR, strlen(PROTOCOL_VERSION_CBOR));

  // Act
  cardano_error_t error = cardano_protocol_version_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_version_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_version_from_cbor(nullptr, &protocol_version);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_version_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_protocol_version_from_cbor(reader, &protocol_version);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'protocol_version', expected a 'Major Type: Byte String' (2) of 2 element(s) but got a 'Major Type: Byte String' (2) of 1 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_version_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotUint)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex("82ff", 4);

  // Act
  cardano_error_t error = cardano_protocol_version_from_cbor(reader, &protocol_version);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected break byte.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_version_from_cbor, returnErrorIfCborDataSecondElementIsNotUint)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex("8200ff", 4);

  // Act
  cardano_error_t error = cardano_protocol_version_from_cbor(reader, &protocol_version);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected end of buffer.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_version_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_version_ref(protocol_version);

  // Assert
  EXPECT_THAT(protocol_version, testing::Not((cardano_protocol_version_t*)nullptr));
  EXPECT_EQ(cardano_protocol_version_refcount(protocol_version), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_protocol_version_unref(&protocol_version);
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_protocol_version_ref(nullptr);
}

TEST(cardano_protocol_version_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;

  // Act
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_protocol_version_unref((cardano_protocol_version_t**)nullptr);
}

TEST(cardano_protocol_version_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_version_ref(protocol_version);
  size_t ref_count = cardano_protocol_version_refcount(protocol_version);

  cardano_protocol_version_unref(&protocol_version);
  size_t updated_ref_count = cardano_protocol_version_refcount(protocol_version);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_version_ref(protocol_version);
  size_t ref_count = cardano_protocol_version_refcount(protocol_version);

  cardano_protocol_version_unref(&protocol_version);
  size_t updated_ref_count = cardano_protocol_version_refcount(protocol_version);

  cardano_protocol_version_unref(&protocol_version);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(protocol_version, (cardano_protocol_version_t*)nullptr);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_protocol_version_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_protocol_version_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  const char*                 message          = "This is a test message";

  // Act
  cardano_protocol_version_set_last_error(protocol_version, message);

  // Assert
  EXPECT_STREQ(cardano_protocol_version_get_last_error(protocol_version), "Object is NULL.");
}

TEST(cardano_protocol_version_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_protocol_version_set_last_error(protocol_version, message);

  // Assert
  EXPECT_STREQ(cardano_protocol_version_get_last_error(protocol_version), "");

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_get_major, returnsThemajorValue)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const uint64_t major = cardano_protocol_version_get_major(protocol_version);

  // Assert
  EXPECT_EQ(major, 1);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_get_major, returnZeroIfProtocolVersionIsNull)
{
  // Act
  const uint64_t major = cardano_protocol_version_get_major(nullptr);

  // Assert
  EXPECT_EQ(major, 0);
}

TEST(cardano_protocol_version_get_minor, returnsTheminorStepsValue)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const uint64_t minor = cardano_protocol_version_get_minor(protocol_version);

  // Assert
  EXPECT_EQ(minor, 3);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_get_minor, returnZeroIfProtocolVersionIsNull)
{
  // Act
  const uint64_t minor = cardano_protocol_version_get_minor(nullptr);

  // Assert
  EXPECT_EQ(minor, 0);
}

TEST(cardano_protocol_version_set_major, setsThemajorValue)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_protocol_version_set_major(protocol_version, 123456789);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_version_get_major(protocol_version), 123456789);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_set_major, returnErrorIfProtocolVersionIsNull)
{
  // Act
  cardano_error_t error = cardano_protocol_version_set_major(nullptr, 123456789);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_version_set_minor, setsTheminorStepsValue)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;
  cardano_error_t             error            = cardano_protocol_version_new(1, 3, &protocol_version);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_protocol_version_set_minor(protocol_version, 987654321);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_version_get_minor(protocol_version), 987654321);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_version_set_minor, returnErrorIfProtocolVersionIsNull)
{
  // Act
  cardano_error_t error = cardano_protocol_version_set_minor(nullptr, 987654321);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}
