/**
 * \file asset_name.cpp
 *
 * \author angel.castillo
 * \date   Sep 14, 2024
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

#include <cardano/assets/asset_name.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char*  ASSET_NAME_HEX     = "736b7977616c6b6572";
static const byte_t ASSET_NAME_BYTES[] = {
  // clang-format off
  0x73, 0x6b, 0x79, 0x77, 0x61, 0x6c, 0x6b, 0x65, 0x72
  // clang-format on
};
static const char* ASSET_NAME = "skywalker";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the asset_name.
 * @return A new instance of the asset_name.
 */
static cardano_asset_name_t*
new_default_asset_name()
{
  cardano_asset_name_t* asset_name = NULL;
  cardano_error_t       result     = cardano_asset_name_from_hex(ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), &asset_name);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return asset_name;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_asset_name_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  cardano_asset_name_ref(asset_name);

  // Assert
  EXPECT_THAT(asset_name, testing::Not((cardano_asset_name_t*)nullptr));
  EXPECT_EQ(cardano_asset_name_refcount(asset_name), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_asset_name_unref(&asset_name);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_name_ref(nullptr);
}

TEST(cardano_asset_name_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_asset_name_t* asset_name = nullptr;

  // Act
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_name_unref((cardano_asset_name_t**)nullptr);
}

TEST(cardano_asset_name_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  cardano_asset_name_ref(asset_name);
  size_t ref_count = cardano_asset_name_refcount(asset_name);

  cardano_asset_name_unref(&asset_name);
  size_t updated_ref_count = cardano_asset_name_refcount(asset_name);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  cardano_asset_name_ref(asset_name);
  size_t ref_count = cardano_asset_name_refcount(asset_name);

  cardano_asset_name_unref(&asset_name);
  size_t updated_ref_count = cardano_asset_name_refcount(asset_name);

  cardano_asset_name_unref(&asset_name);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(asset_name, (cardano_asset_name_t*)nullptr);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_asset_name_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_asset_name_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_asset_name_t* asset_name = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_asset_name_set_last_error(asset_name, message);

  // Assert
  EXPECT_STREQ(cardano_asset_name_get_last_error(asset_name), "Object is NULL.");
}

TEST(cardano_asset_name_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  const char* message = nullptr;

  // Act
  cardano_asset_name_set_last_error(asset_name, message);

  // Assert
  EXPECT_STREQ(cardano_asset_name_get_last_error(asset_name), "");

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_from_bytes, returnsErrorIfDataIsNull)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_bytes(nullptr, 1, &asset_name);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_name, nullptr);
}

TEST(cardano_asset_name_from_bytes, canCreateEmptyAssetName)
{
  // Arrange
  byte_t data[] = { 0 };

  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_bytes(data, 0, &asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_get_bytes_size(asset_name), 0);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_from_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_asset_name_from_bytes(ASSET_NAME_BYTES, sizeof(ASSET_NAME_BYTES), &asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_name, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_from_bytes, returnsErrorIfAssetIdIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_name_from_bytes(ASSET_NAME_BYTES, sizeof(ASSET_NAME_BYTES), nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_name_from_hex, returnsErrorIfDataIsNull)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_hex(nullptr, 1, &asset_name);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_name, nullptr);
}

TEST(cardano_asset_name_from_hex, returnsErrorIfDataSizeIsZero)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_hex("", 0, &asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_asset_name_get_string(asset_name), "");

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_from_hex, returnsErrorIfHexIsNotDivisibleBy2)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_hex("f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657", strlen("f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657"), &asset_name);

  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Assert
  EXPECT_EQ(asset_name, nullptr);
}

TEST(cardano_asset_name_from_hex, returnsErrorIfAssetIdIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_name_from_hex(ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Getters and Setters

TEST(cardano_asset_name_get_bytes, returnsTheBytes)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  const byte_t* bytes = cardano_asset_name_get_bytes(asset_name);
  const size_t  size  = cardano_asset_name_get_bytes_size(asset_name);

  // Assert
  EXPECT_EQ(size, sizeof(ASSET_NAME_BYTES));
  EXPECT_THAT(bytes, testing::Not((byte_t*)nullptr));

  for (size_t i = 0; i < size; i++)
  {
    EXPECT_EQ(bytes[i], ASSET_NAME_BYTES[i]);
  }

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_get_bytes, returnsNullIfAssetIdIsNull)
{
  // Act
  const byte_t* bytes = cardano_asset_name_get_bytes(nullptr);

  // Assert
  EXPECT_EQ(bytes, nullptr);
}

TEST(cardano_asset_name_get_bytes_size, returnsZeroIfAssetIdIsNull)
{
  // Act
  size_t size = cardano_asset_name_get_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_asset_name_get_hex, returnsTheHex)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  const char* hex  = cardano_asset_name_get_hex(asset_name);
  size_t      size = cardano_asset_name_get_hex_size(asset_name);

  // Assert
  EXPECT_EQ(size, strlen(ASSET_NAME_HEX) + 1);
  EXPECT_THAT(hex, testing::Not((char*)nullptr));
  EXPECT_STREQ(hex, ASSET_NAME_HEX);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_get_hex, returnsNullIfAssetIdIsNull)
{
  // Act
  const char* hex = cardano_asset_name_get_hex(nullptr);

  // Assert
  EXPECT_EQ(hex, nullptr);
}

TEST(cardano_asset_name_get_hex_size, returnsZeroIfAssetIdIsNull)
{
  // Act
  size_t size = cardano_asset_name_get_hex_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_asset_name_from_string, returnsErrorIfDataIsNull)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_string(nullptr, 1, &asset_name);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_name, nullptr);
}

TEST(cardano_asset_name_from_string, canCreateFromEmptyString)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_string("", 0, &asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_get_bytes_size(asset_name), 0);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_from_string, canCreate)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_string(ASSET_NAME, strlen(ASSET_NAME), &asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_get_bytes_size(asset_name), strlen(ASSET_NAME));

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_from_string, returnsErrorIfAssetIdIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_name_from_string(ASSET_NAME, strlen(ASSET_NAME), nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_name_get_string, returnsTheString)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  const char* string = cardano_asset_name_get_string(asset_name);

  // Assert
  EXPECT_THAT(string, testing::Not((char*)nullptr));
  EXPECT_STREQ(string, ASSET_NAME);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_get_string, returnsNullIfAssetIdIsNull)
{
  // Act
  const char* string = cardano_asset_name_get_string(nullptr);

  // Assert
  EXPECT_EQ(string, nullptr);
}

TEST(cardano_asset_name_get_string_size, returnsZeroIfAssetIdIsNull)
{
  // Act
  size_t size = cardano_asset_name_get_string_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_asset_name_get_string_size, canGetStringSize)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  size_t size = cardano_asset_name_get_string_size(asset_name);

  // Assert
  EXPECT_EQ(size, strlen(ASSET_NAME) + 1);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_to_cbor, returnsErrorIfAssetIdIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_name_to_cbor(nullptr, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_name_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  // Act
  cardano_error_t result = cardano_asset_name_to_cbor(asset_name, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_to_cbor, canEncodeToCbor)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_NE(writer, nullptr);

  // Act

  cardano_error_t result = cardano_asset_name_to_cbor(asset_name, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, "49736b7977616c6b6572");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  cardano_asset_name_unref(&asset_name);
  free(hex);
}

TEST(cardano_asset_name_to_cbor, canEncodeToCborEmptyAssetName)
{
  // Arrange
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_bytes(NULL, 0, &asset_name);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_NE(writer, nullptr);

  // Act
  result = cardano_asset_name_to_cbor(asset_name, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, "40");

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  cardano_asset_name_unref(&asset_name);
  free(hex);
}

TEST(cardano_asset_name_from_cbor, returnsErrorIfReaderIsNull)
{
  // Act
  cardano_asset_name_t* asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_cbor(NULL, &asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(asset_name, nullptr);
}

TEST(cardano_asset_name_from_cbor, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("40", strlen("40"));
  EXPECT_NE(reader, nullptr);

  // Act
  cardano_error_t result = cardano_asset_name_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_from_cbor, canDecodeFromCbor)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();
  EXPECT_NE(asset_name, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_NE(writer, nullptr);

  ASSERT_EQ(cardano_asset_name_to_cbor(asset_name, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(hex, strlen(hex));
  EXPECT_NE(reader, nullptr);

  // Act
  cardano_asset_name_t* decoded_asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_cbor(reader, &decoded_asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_THAT(decoded_asset_name, testing::Not((cardano_asset_name_t*)nullptr));

  const byte_t* bytes = cardano_asset_name_get_bytes(decoded_asset_name);
  const size_t  size  = cardano_asset_name_get_bytes_size(decoded_asset_name);

  EXPECT_EQ(size, sizeof(ASSET_NAME_BYTES));
  EXPECT_THAT(bytes, testing::Not((byte_t*)nullptr));

  for (size_t i = 0; i < size; i++)
  {
    EXPECT_EQ(bytes[i], ASSET_NAME_BYTES[i]);
  }

  // Cleanup
  cardano_asset_name_unref(&asset_name);
  cardano_asset_name_unref(&decoded_asset_name);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_asset_name_from_cbor, canDecodeFromCborEmptyAssetName)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("40", strlen("40"));
  EXPECT_NE(reader, nullptr);

  // Act
  cardano_asset_name_t* decoded_asset_name = NULL;

  cardano_error_t result = cardano_asset_name_from_cbor(reader, &decoded_asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_THAT(decoded_asset_name, testing::Not((cardano_asset_name_t*)nullptr));

  const byte_t* bytes = cardano_asset_name_get_bytes(decoded_asset_name);
  const size_t  size  = cardano_asset_name_get_bytes_size(decoded_asset_name);

  EXPECT_EQ(size, 0);
  EXPECT_THAT(bytes, testing::Not((byte_t*)nullptr));

  // Cleanup
  cardano_asset_name_unref(&decoded_asset_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_from_cbor, returnErroIfInvalidByteString)
{
  // Arrange
  cardano_asset_name_t* asset_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("ef", strlen("ef"));
  EXPECT_NE(reader, nullptr);

  // Act
  cardano_error_t result = cardano_asset_name_from_cbor(reader, &asset_name);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}