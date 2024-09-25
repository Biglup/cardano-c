/**
 * \file pool_metadata.cpp
 *
 * \author angel.castillo
 * \date   Jun 28, 2024
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
#include <cardano/pool_params/pool_metadata.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* URL  = "https://example.com";
static const char* HASH = "0f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";

/* UNIT TESTS ****************************************************************/

TEST(cardano_pool_metadata_new, canCreatePoolMetadata)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_new(URL, strlen(URL), hash, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(pool_metadata, testing::Not((cardano_pool_metadata_t*)nullptr));

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_new, returnsErrorIfUrlIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_new(nullptr, 0, hash, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_new, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  // Act
  cardano_error_t error = cardano_pool_metadata_new(URL, strlen(URL), nullptr, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_metadata_new, returnsErrorIfPoolMetadataIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_new(URL, strlen(URL), hash, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_new, returnsErrorIfUrlBiggerThan64)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_new(URL, 65, hash, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_from_hash_hex, returnsErrorIfPoolMetadataIsNull)
{
  // Act
  cardano_blake2b_hash_t* hash = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_new, returnsErrorIfAllocationFails)
{
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_pool_metadata_new(URL, strlen(URL), hash, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pool_metadata, (cardano_pool_metadata_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_from_hash_hex, returnsErrorIfUrlIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_from_hash_hex(nullptr, 0, HASH, strlen(HASH), &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_from_hash_hex, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), nullptr, 0, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_from_hash_hex, returnsErrorIfHashDifferentThan64)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, 63, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_new, returnsErrorIfUrlIsBiggerThan64)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, 65, HASH, strlen(HASH), &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_metadata_to_cbor, canSerializePoolMetadata)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_to_cbor(pool_metadata, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_pool_metadata_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_pool_metadata_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_pool_metadata_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_to_cbor(pool_metadata, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_from_cbor, canDeserializePoolMetadata)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_pool_metadata_from_cbor(reader, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(pool_metadata, testing::Not((cardano_pool_metadata_t*)nullptr));

  EXPECT_STREQ(cardano_pool_metadata_get_url(pool_metadata), URL);
  EXPECT_EQ(cardano_pool_metadata_get_hash(pool_metadata, &hash), CARDANO_SUCCESS);

  const size_t hex_size    = cardano_blake2b_hash_get_hex_size(hash);
  char*        actual_hash = (char*)malloc(hex_size);

  cardano_error_t hash_error = cardano_blake2b_hash_to_hex(hash, actual_hash, hex_size);

  EXPECT_EQ(hash_error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_hash, HASH);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(actual_hash);
}

TEST(cardano_pool_metadata_from_cbor, returnErrorIfPoolMetadataIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_pool_metadata_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_metadata_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  // Act
  cardano_error_t error = cardano_pool_metadata_from_cbor(nullptr, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_metadata_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_pool_metadata_from_cbor(reader, &pool_metadata);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'pool_metadata', expected a 'Major Type: Byte String' (2) of 2 element(s) but got a 'Major Type: Byte String' (2) of 1 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_metadata_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotTextString)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("82ef7368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("82ef7368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));

  // Act
  cardano_error_t error = cardano_pool_metadata_from_cbor(reader, &pool_metadata);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_metadata_from_cbor, returnErrorIfCborDataSecondElementIsHash)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("827368747470733a2f2f6578616d706c652e636f6def", strlen("827368747470733a2f2f6578616d706c652e636f6def"));

  // Act
  cardano_error_t error = cardano_pool_metadata_from_cbor(reader, &pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_metadata_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_error_t          error         = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_metadata_ref(pool_metadata);

  // Assert
  EXPECT_THAT(pool_metadata, testing::Not((cardano_pool_metadata_t*)nullptr));
  EXPECT_EQ(cardano_pool_metadata_refcount(pool_metadata), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_pool_metadata_unref(&pool_metadata);
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_metadata_ref(nullptr);
}

TEST(cardano_pool_metadata_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  // Act
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_metadata_unref((cardano_pool_metadata_t**)nullptr);
}

TEST(cardano_pool_metadata_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_error_t          error         = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_metadata_ref(pool_metadata);
  size_t ref_count = cardano_pool_metadata_refcount(pool_metadata);

  cardano_pool_metadata_unref(&pool_metadata);
  size_t updated_ref_count = cardano_pool_metadata_refcount(pool_metadata);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_error_t          error         = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_metadata_ref(pool_metadata);
  size_t ref_count = cardano_pool_metadata_refcount(pool_metadata);

  cardano_pool_metadata_unref(&pool_metadata);
  size_t updated_ref_count = cardano_pool_metadata_refcount(pool_metadata);

  cardano_pool_metadata_unref(&pool_metadata);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(pool_metadata, (cardano_pool_metadata_t*)nullptr);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_pool_metadata_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_pool_metadata_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_pool_metadata_set_last_error(pool_metadata, message);

  // Assert
  EXPECT_STREQ(cardano_pool_metadata_get_last_error(pool_metadata), "Object is NULL.");
}

TEST(cardano_pool_metadata_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_error_t          error         = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_pool_metadata_set_last_error(pool_metadata, message);

  // Assert
  EXPECT_STREQ(cardano_pool_metadata_get_last_error(pool_metadata), "");

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_get_url_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t url_size = cardano_pool_metadata_get_url_size(nullptr);

  // Assert
  EXPECT_EQ(url_size, 0);
}

TEST(cardano_pool_metadata_get_url_size, returnsTheSizeOfTheUrl)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_error_t          error         = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t url_size = cardano_pool_metadata_get_url_size(pool_metadata);

  // Assert
  EXPECT_EQ(url_size, strlen(URL));

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_get_url, returnsNullIfGivenANullPtr)
{
  // Act
  const char* url = cardano_pool_metadata_get_url(nullptr);

  // Assert
  EXPECT_EQ(url, (const char*)nullptr);
}

TEST(cardano_pool_metadata_get_url, returnsTheUrl)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_error_t          error         = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const char* url = cardano_pool_metadata_get_url(pool_metadata);

  // Assert
  EXPECT_STREQ(url, URL);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_set_url, returnsErrorIfGivenANullPtr)
{
  // Act
  cardano_error_t error = cardano_pool_metadata_set_url(URL, strlen(URL), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_metadata_set_url, returnsErrorIfUrlIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_set_url(nullptr, 0, pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_set_url, returnsErrorIfUrlIsBiggerThan64)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_set_url("https://example.com/this-is-a-very-long-url/this-is-a-very-long-url", strlen("https://example.com/this-is-a-very-long-url/this-is-a-very-long-url"), pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_set_url, setsTheUrl)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_set_url("https://example.com/this-is-a-long-url", 40, pool_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_pool_metadata_get_url(pool_metadata), "https://example.com/this-is-a-long-url");

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_get_hash, returnsErrorIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_pool_metadata_get_hash(nullptr, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_metadata_get_hash, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_get_hash(pool_metadata, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_get_hash, returnsTheHash)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_get_hash(pool_metadata, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_blake2b_hash_get_hex_size(hash);
  char*        actual_hash = (char*)malloc(hex_size);

  cardano_error_t hash_error = cardano_blake2b_hash_to_hex(hash, actual_hash, hex_size);

  EXPECT_EQ(hash_error, CARDANO_SUCCESS);
  EXPECT_STREQ(actual_hash, HASH);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
  cardano_blake2b_hash_unref(&hash);
  free(actual_hash);
}

TEST(cardano_pool_metadata_set_hash, returnsErrorIfGivenANullPtr)
{
  // Act
  cardano_error_t error = cardano_pool_metadata_set_hash(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pool_metadata_set_hash, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_set_hash(pool_metadata, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
}

TEST(cardano_pool_metadata_set_hash, setsTheHash)
{
  // Arrange
  cardano_pool_metadata_t* pool_metadata = nullptr;
  cardano_blake2b_hash_t*  hash          = nullptr;

  cardano_error_t error = cardano_pool_metadata_from_hash_hex(URL, strlen(URL), HASH, strlen(HASH), &pool_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_from_hex("0f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", 64, &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_metadata_set_hash(pool_metadata, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* actual_hash = nullptr;

  error = cardano_pool_metadata_get_hash(pool_metadata, &actual_hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size        = cardano_blake2b_hash_get_hex_size(actual_hash);
  char*        actual_hash_str = (char*)malloc(hex_size);

  cardano_error_t hash_error = cardano_blake2b_hash_to_hex(actual_hash, actual_hash_str, hex_size);

  EXPECT_EQ(hash_error, CARDANO_SUCCESS);
  EXPECT_STREQ(actual_hash_str, "0f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5");

  // Cleanup
  cardano_pool_metadata_unref(&pool_metadata);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&actual_hash);
  free(actual_hash_str);
}