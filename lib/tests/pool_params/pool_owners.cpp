/**
 * \file pool_owners.cpp
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

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/pool_params/pool_owners.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR             = "d9010285581c00000000000000000000000000000000000000000000000000000000581c11111111111111111111111111111111111111111111111111111111581c22222222222222222222222222222222222222222222222222222222581c33333333333333333333333333333333333333333333333333333333581c44444444444444444444444444444444444444444444444444444444";
static const char* CBOR_WITHOUT_TAG = "85581c00000000000000000000000000000000000000000000000000000000581c11111111111111111111111111111111111111111111111111111111581c22222222222222222222222222222222222222222222222222222222581c33333333333333333333333333333333333333333333333333333333581c44444444444444444444444444444444444444444444444444444444";

static const char* POOL_HASH1 = "00000000000000000000000000000000000000000000000000000000";
static const char* POOL_HASH2 = "11111111111111111111111111111111111111111111111111111111";
static const char* POOL_HASH3 = "22222222222222222222222222222222222222222222222222222222";
static const char* POOL_HASH4 = "33333333333333333333333333333333333333333333333333333333";
static const char* POOL_HASH5 = "44444444444444444444444444444444444444444444444444444444";

/* UNIT TESTS ****************************************************************/

TEST(cardano_pool_owners_new, canCreatePoolOwners)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;

  // Act
  cardano_error_t error = cardano_pool_owners_new(&pool_owners);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(pool_owners, testing::Not((cardano_pool_owners_t*)nullptr));

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_new, returnsErrorIfPoolOwnersIsNull)
{
  // Act
  cardano_error_t error = cardano_pool_owners_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_owners_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_pool_owners_t* pool_owners = nullptr;

  // Act
  cardano_error_t error = cardano_pool_owners_new(&pool_owners);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pool_owners, (cardano_pool_owners_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_pool_owners_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_pool_owners_t* pool_owners = nullptr;

  // Act
  cardano_error_t error = cardano_pool_owners_new(&pool_owners);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pool_owners, (cardano_pool_owners_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_pool_owners_to_cbor, canSerializeAnEmptyPoolOwners)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_pool_owners_new(&pool_owners);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_owners_to_cbor(pool_owners, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_pool_owners_to_cbor, canSerializePoolOwners)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_pool_owners_new(&pool_owners);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* hashes[] = { POOL_HASH1, POOL_HASH2, POOL_HASH3, POOL_HASH4, POOL_HASH5 };

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_blake2b_hash_t* hash = NULL;

    EXPECT_EQ(cardano_blake2b_hash_from_hex(hashes[i], 56, &hash), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_pool_owners_add(pool_owners, hash), CARDANO_SUCCESS);

    cardano_blake2b_hash_unref(&hash);
  }

  // Act
  error = cardano_pool_owners_to_cbor(pool_owners, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_pool_owners_to_cbor, canSerializePoolOwnersSorted)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_pool_owners_new(&pool_owners);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* hashes[] = { POOL_HASH5, POOL_HASH3, POOL_HASH1, POOL_HASH4, POOL_HASH2 };

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_blake2b_hash_t* hash = NULL;

    EXPECT_EQ(cardano_blake2b_hash_from_hex(hashes[i], 56, &hash), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_pool_owners_add(pool_owners, hash), CARDANO_SUCCESS);

    cardano_blake2b_hash_unref(&hash);
  }

  // Act
  error = cardano_pool_owners_to_cbor(pool_owners, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_pool_owners_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_pool_owners_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_pool_owners_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;

  cardano_error_t error = cardano_pool_owners_new(&pool_owners);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_owners_to_cbor(pool_owners, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &pool_owners);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_pool_owners_to_cbor(pool_owners, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_pool_owners_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &pool_owners);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_pool_owners_to_cbor(pool_owners, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_pool_owners_from_cbor, canDeserializePoolOwners)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &pool_owners);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(pool_owners, testing::Not((cardano_pool_owners_t*)nullptr));

  const size_t length = cardano_pool_owners_get_length(pool_owners);

  EXPECT_EQ(length, 5);

  cardano_blake2b_hash_t* elem1 = NULL;
  cardano_blake2b_hash_t* elem2 = NULL;
  cardano_blake2b_hash_t* elem3 = NULL;
  cardano_blake2b_hash_t* elem4 = NULL;
  cardano_blake2b_hash_t* elem5 = NULL;

  EXPECT_EQ(cardano_pool_owners_get(pool_owners, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_pool_owners_get(pool_owners, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_pool_owners_get(pool_owners, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_pool_owners_get(pool_owners, 3, &elem4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_pool_owners_get(pool_owners, 4, &elem5), CARDANO_SUCCESS);

  char* data1 = (char*)malloc(57);
  char* data2 = (char*)malloc(57);
  char* data3 = (char*)malloc(57);
  char* data4 = (char*)malloc(57);
  char* data5 = (char*)malloc(57);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(elem1, data1, 57), CARDANO_SUCCESS);
  EXPECT_STREQ(data1, POOL_HASH1);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(elem2, data2, 57), CARDANO_SUCCESS);
  EXPECT_STREQ(data2, POOL_HASH2);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(elem3, data3, 57), CARDANO_SUCCESS);
  EXPECT_STREQ(data3, POOL_HASH3);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(elem4, data4, 57), CARDANO_SUCCESS);
  EXPECT_STREQ(data4, POOL_HASH4);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(elem5, data5, 57), CARDANO_SUCCESS);
  EXPECT_STREQ(data5, POOL_HASH5);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
  cardano_cbor_reader_unref(&reader);

  cardano_blake2b_hash_unref(&elem1);
  cardano_blake2b_hash_unref(&elem2);
  cardano_blake2b_hash_unref(&elem3);
  cardano_blake2b_hash_unref(&elem4);
  cardano_blake2b_hash_unref(&elem5);

  free(data1);
  free(data2);
  free(data3);
  free(data4);
  free(data5);
}

TEST(cardano_pool_owners_from_cbor, returnErrorIfPoolOwnersIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_owners_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(nullptr, &pool_owners);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_owners_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &pool_owners);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pool_owners, (cardano_pool_owners_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_owners_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_pool_owners_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_owners_from_cbor, returnErrorIfInvalidRelayElements)
{
  // Arrange
  cardano_pool_owners_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_owners_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_pool_owners_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_owners_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_pool_owners_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_pool_owners_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_owners_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_owners_ref(pool_owners);

  // Assert
  EXPECT_THAT(pool_owners, testing::Not((cardano_pool_owners_t*)nullptr));
  EXPECT_EQ(cardano_pool_owners_refcount(pool_owners), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_pool_owners_unref(&pool_owners);
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_owners_ref(nullptr);
}

TEST(cardano_pool_owners_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;

  // Act
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_owners_unref((cardano_pool_owners_t**)nullptr);
}

TEST(cardano_pool_owners_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_owners_ref(pool_owners);
  size_t ref_count = cardano_pool_owners_refcount(pool_owners);

  cardano_pool_owners_unref(&pool_owners);
  size_t updated_ref_count = cardano_pool_owners_refcount(pool_owners);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_owners_ref(pool_owners);
  size_t ref_count = cardano_pool_owners_refcount(pool_owners);

  cardano_pool_owners_unref(&pool_owners);
  size_t updated_ref_count = cardano_pool_owners_refcount(pool_owners);

  cardano_pool_owners_unref(&pool_owners);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(pool_owners, (cardano_pool_owners_t*)nullptr);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_pool_owners_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_pool_owners_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_pool_owners_set_last_error(pool_owners, message);

  // Assert
  EXPECT_STREQ(cardano_pool_owners_get_last_error(pool_owners), "Object is NULL.");
}

TEST(cardano_pool_owners_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_pool_owners_set_last_error(pool_owners, message);

  // Assert
  EXPECT_STREQ(cardano_pool_owners_get_last_error(pool_owners), "");

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_get_length, returnsZeroIfPoolOwnersIsNull)
{
  // Act
  size_t length = cardano_pool_owners_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_pool_owners_get_length, returnsZeroIfPoolOwnersIsEmpty)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_pool_owners_get_length(pool_owners);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_get, returnsErrorIfPoolOwnersIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_pool_owners_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_owners_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_owners_get(pool_owners, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* data = nullptr;
  error                        = cardano_pool_owners_get(pool_owners, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}

TEST(cardano_pool_owners_add, returnsErrorIfPoolOwnersIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_pool_owners_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_owners_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_pool_owners_t* pool_owners = nullptr;
  cardano_error_t        error       = cardano_pool_owners_new(&pool_owners);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_pool_owners_add(pool_owners, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_owners_unref(&pool_owners);
}
