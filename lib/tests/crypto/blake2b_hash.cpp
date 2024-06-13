/**
 * \file blake2b_hash.cpp
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_size.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* HASH_CBOR = "581c00000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Verifies a test vector for the Blake2b hash function.
 *
 * \param data_hex          The data to hash, in hexadecimal format.
 * \param expected_hash_hex The expected hash, in hexadecimal format.
 * \param hash_size         The size of the hash in bytes.
 */
static void
verify_test_vector(const char* data_hex, const char* expected_hash_hex, const size_t size)
{
  cardano_buffer_t* buffer = cardano_buffer_from_hex(data_hex, strlen(data_hex));
  ASSERT_EQ(buffer != nullptr, true);

  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), size, &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(hash != nullptr, true);

  const size_t hash_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hash_hex  = (char*)malloc(hash_size);

  error = cardano_blake2b_hash_to_hex(hash, &hash_hex[0], hash_size);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  ASSERT_STREQ(hash_hex, expected_hash_hex);

  cardano_buffer_unref(&buffer);
  cardano_blake2b_hash_unref(&hash);
  free(hash_hex);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_blake2b_hash, canGenerateTheCorrectHash)
{
  verify_test_vector("00", "0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4", CARDANO_BLAKE2B_HASH_SIZE_224);
  verify_test_vector("0001", "9430be1d5e37ea654ddb63370a3d04a8a0a171abb5c3710a9bc372f8", CARDANO_BLAKE2B_HASH_SIZE_224);
  verify_test_vector("000102", "495734948024c1ac1cc6dce8d3ab2aad5b8c4194203aaaa460af9437", CARDANO_BLAKE2B_HASH_SIZE_224);
  verify_test_vector("000102030405060708090a0b0c", "7b71eb4635c7fe17ef96c86ddd6230faa408657e79fb7451a47981ca", CARDANO_BLAKE2B_HASH_SIZE_224);

  verify_test_vector("00", "03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314", CARDANO_BLAKE2B_HASH_SIZE_256);
  verify_test_vector("0001", "01cf79da4945c370c68b265ef70641aaa65eaa8f5953e3900d97724c2c5aa095", CARDANO_BLAKE2B_HASH_SIZE_256);
  verify_test_vector("000102", "3d8c3d594928271f44aad7a04b177154806867bcf918e1549c0bc16f9da2b09b", CARDANO_BLAKE2B_HASH_SIZE_256);
  verify_test_vector("000102030405060708090a0b0c", "695e93b723e0a08e8dd8dd4656389363519564daf4cde5fe95a6a0ca71d3705e", CARDANO_BLAKE2B_HASH_SIZE_256);

  verify_test_vector("00", "2fa3f686df876995167e7c2e5d74c4c7b6e48f8068fe0e44208344d480f7904c36963e44115fe3eb2a3ac8694c28bcb4f5a0f3276f2e79487d8219057a506e4b", CARDANO_BLAKE2B_HASH_SIZE_512);
  verify_test_vector("0001", "1c08798dc641aba9dee435e22519a4729a09b2bfe0ff00ef2dcd8ed6f8a07d15eaf4aee52bbf18ab5608a6190f70b90486c8a7d4873710b1115d3debbb4327b5", CARDANO_BLAKE2B_HASH_SIZE_512);
  verify_test_vector("000102", "40a374727302d9a4769c17b5f409ff32f58aa24ff122d7603e4fda1509e919d4107a52c57570a6d94e50967aea573b11f86f473f537565c66f7039830a85d186", CARDANO_BLAKE2B_HASH_SIZE_512);
  verify_test_vector("000102030405060708090a0b0c", "dea9101cac62b8f6a3c650f90eea5bfae2653a4eafd63a6d1f0f132db9e4f2b1b662432ec85b17bcac41e775637881f6aab38dd66dcbd080f0990a7a6e9854fe", CARDANO_BLAKE2B_HASH_SIZE_512);
}

TEST(cardano_blake2b_hash, returnErrorIfMemoryAllocationFails)
{
  cardano_blake2b_hash_t* hash = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);

  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(hash, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash, returnErrorIfMemoryAllocationEventuallyFails)
{
  cardano_blake2b_hash_t* hash = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);

  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(hash, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash, returnErrorWhenDataIsNull)
{
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash(nullptr, 0, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);

  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
  ASSERT_EQ(hash, nullptr);
}

TEST(cardano_blake2b_hash, returnErrorWhenDataLengthIsZero)
{
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash((const byte_t*)"data", 0, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);

  ASSERT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);
  ASSERT_EQ(hash, nullptr);
}

TEST(cardano_blake2b_hash, returnErrorWhenHashLengthIsZero)
{
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, 0, &hash);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
  ASSERT_EQ(hash, nullptr);
}

TEST(cardano_blake2b_hash_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_ref(hash);

  // Assert
  EXPECT_THAT(hash, testing::Not((cardano_blake2b_hash_t*)nullptr));
  EXPECT_EQ(cardano_blake2b_hash_refcount(hash), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_ref(nullptr);
}

TEST(cardano_blake2b_hash_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_unref((cardano_blake2b_hash_t**)nullptr);
}

TEST(cardano_blake2b_hash_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_ref(hash);
  size_t ref_count = cardano_blake2b_hash_refcount(hash);

  cardano_blake2b_hash_unref(&hash);
  size_t updated_ref_count = cardano_blake2b_hash_refcount(hash);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_ref(hash);
  size_t ref_count = cardano_blake2b_hash_refcount(hash);

  cardano_blake2b_hash_unref(&hash);
  size_t updated_ref_count = cardano_blake2b_hash_refcount(hash);

  cardano_blake2b_hash_unref(&hash);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_blake2b_hash_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_blake2b_hash_from_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_hash_from_bytes(nullptr, 0, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_blake2b_hash_from_bytes, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_hash_from_bytes((const byte_t*)"data", 0, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_blake2b_hash_from_bytes, returnsNullIfHashIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_hash_from_bytes((const byte_t*)"data", 4, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_from_bytes, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_blake2b_hash_from_bytes((const byte_t*)"data", 4, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_from_bytes, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_blake2b_hash_from_bytes((const byte_t*)"data", 4, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_from_hex, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_hash_from_hex(nullptr, 0, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_blake2b_hash_from_hex, returnsNullIfHashIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_hash_from_hex("data", 4, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_from_hex, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_hash_from_hex("data", 0, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_blake2b_hash_from_hex, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_blake2b_hash_from_hex("data", 4, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_from_hex, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_blake2b_hash_from_hex("data", 4, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_from_hex, returnsHashObjectWithHashBytes)
{
  // Arrange
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_hash_from_hex("2fa3f686df876995167e7c2e5d74c4c7b6e48f8068fe0e44208344d480f7904c36963e44115fe3eb2a3ac8694c28bcb4f5a0f3276f2e79487d8219057a506e4b", 128, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(hash, testing::Not((cardano_blake2b_hash_t*)nullptr));
  EXPECT_EQ(cardano_blake2b_hash_get_bytes_size(hash), 64);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_from_bytes, returnsHashObjectWithHashBytes)
{
  // Arrange
  cardano_blake2b_hash_t* hash   = nullptr;
  const byte_t            data[] = { 'd', 'a', 't', 'a' };
  cardano_error_t         error  = cardano_blake2b_hash_from_bytes(&data[0], 4, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(hash, testing::Not((cardano_blake2b_hash_t*)nullptr));
  EXPECT_EQ(cardano_blake2b_hash_get_bytes_size(hash), 4);

  const byte_t* hash_data = cardano_blake2b_hash_get_data(hash);

  for (size_t i = 0; i < 4; i++)
  {
    EXPECT_EQ(hash_data[i], data[i]);
  }

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_bytes(hash, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_bytes, returnsErrorIfBufferLengthIsZero)
{
  // Arrange
  cardano_blake2b_hash_t* hash   = nullptr;
  const byte_t            data[] = { 'd', 'a', 't', 'a' };
  cardano_error_t         error  = cardano_blake2b_hash_from_bytes(&data[0], 4, &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_to_bytes(hash, (byte_t*)"data", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_to_bytes, returnsErrorIfHashLengthIsGreaterThanBufferLength)
{
  // Arrange
  cardano_blake2b_hash_t* hash   = nullptr;
  const byte_t            data[] = { 'd', 'a', 't', 'a' };
  cardano_error_t         error  = cardano_blake2b_hash_from_bytes(&data[0], 4, &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  // Act
  error = cardano_blake2b_hash_to_bytes(hash, (byte_t*)"data", 3);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_to_bytes, returnsErrorIfHashLengthIsZero)
{
  // Arrange
  cardano_blake2b_hash_t* hash   = nullptr;
  byte_t                  data[] = { 'd', 'a', 't', 'a' };
  cardano_error_t         error  = cardano_blake2b_hash_from_bytes(&data[0], 4, &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_to_bytes(hash, &data[0], 0);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_to_bytes, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash   = nullptr;
  byte_t                  data[] = { 'd', 'a', 't', 'a' };
  cardano_error_t         error  = cardano_blake2b_hash_from_bytes(&data[0], 4, &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_to_bytes(hash, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_compute_hash, returnErrorIfHashIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_bytes, returnsHashBytes)
{
  // Arrange
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t buffer[64] = { 0 };

  // Act
  error = cardano_blake2b_hash_to_bytes(hash, buffer, 64);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const byte_t* hash_data = cardano_blake2b_hash_get_data(hash);

  for (size_t i = 0; i < 64; i++)
  {
    EXPECT_EQ(buffer[i], hash_data[i]);
  }

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_from_cbor, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash  = nullptr;
  cardano_error_t         error = cardano_blake2b_hash_from_cbor(nullptr, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_blake2b_hash_from_cbor, returnsNullIfHashIsNull)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(HASH_CBOR, strlen(HASH_CBOR));

  // Act
  cardano_error_t error = cardano_blake2b_hash_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_from_cbor, returnErrorIfGivenInvalidCbor)
{
  // Arrange
  cardano_cbor_reader_t*  reader = cardano_cbor_reader_from_hex("00", 2);
  cardano_blake2b_hash_t* hash   = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_from_cbor(reader, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_from_cbor, canDecodeHashFromValidCbor)
{
  // Arrange
  cardano_cbor_reader_t*  reader = cardano_cbor_reader_from_hex(HASH_CBOR, strlen(HASH_CBOR));
  cardano_blake2b_hash_t* hash   = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_from_cbor(reader, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(hash, testing::Not((cardano_blake2b_hash_t*)nullptr));

  // compare bytes
  const byte_t* hash_data              = cardano_blake2b_hash_get_data(hash);
  const byte_t  expected_hash_data[28] = { 0x00 };

  for (size_t i = 0; i < sizeof(expected_hash_data); i++)
  {
    EXPECT_EQ(hash_data[i], expected_hash_data[i]);
  }

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_to_cbor, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_blake2b_hash_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_hash_to_cbor((cardano_blake2b_hash_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_cbor, canEncodeHashToCbor)
{
  // Arrange
  cardano_blake2b_hash_t* hash   = nullptr;
  cardano_cbor_writer_t*  writer = cardano_cbor_writer_new();
  cardano_error_t         error  = cardano_blake2b_hash_from_hex("00000000000000000000000000000000000000000000000000000000", 56, &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_to_cbor(hash, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t size     = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex = (char*)malloc(size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor_hex, size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor_hex, HASH_CBOR);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_cbor_writer_unref(&writer);
  free(cbor_hex);
}

TEST(cardano_blake2b_hash_equals, returnsTrueIfBothAreNull)
{
  // Act
  bool result = cardano_blake2b_hash_equals(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, true);
}

TEST(cardano_blake2b_hash_equals, returnsFalseIfOtherHashIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash1 = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_blake2b_hash_equals(hash1, nullptr);
  EXPECT_EQ(result, false);

  result = cardano_blake2b_hash_equals(nullptr, hash1);
  EXPECT_EQ(result, false);

  // Cleanup
  cardano_blake2b_hash_unref(&hash1);
}

TEST(cardano_blake2b_hash_equals, returnsFalseIfHashesAreDifferent)
{
  // Arrange
  cardano_blake2b_hash_t* hash1 = nullptr;
  cardano_blake2b_hash_t* hash2 = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_compute_hash((const byte_t*)"data2", 5, CARDANO_BLAKE2B_HASH_SIZE_512, &hash2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_blake2b_hash_equals(hash1, hash2);

  // Assert
  EXPECT_EQ(result, false);

  // Cleanup
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_blake2b_hash_equals, returnsTrueIfHashesAreEqual)
{
  // Arrange
  cardano_blake2b_hash_t* hash1 = nullptr;
  cardano_blake2b_hash_t* hash2 = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_blake2b_hash_equals(hash1, hash2);

  // Assert
  EXPECT_EQ(result, true);

  // Cleanup
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_blake2b_hash_compare, returnsZeroIfHashesAreEqual)
{
  // Arrange
  cardano_blake2b_hash_t* hash1 = nullptr;
  cardano_blake2b_hash_t* hash2 = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int result = cardano_blake2b_hash_compare(hash1, hash2);

  // Assert
  EXPECT_EQ(result, 0);

  // Cleanup
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_blake2b_hash_compare, returnsNegativeIfFirstHashIsSmaller)
{
  // Arrange
  cardano_blake2b_hash_t* hash1 = nullptr;
  cardano_blake2b_hash_t* hash2 = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data2", 5, CARDANO_BLAKE2B_HASH_SIZE_512, &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int result = cardano_blake2b_hash_compare(hash1, hash2);

  // Assert
  EXPECT_LT(result, 0);

  // Cleanup
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_blake2b_hash_compare, returnsPositiveIfFirstHashIsLarger)
{
  // Arrange
  cardano_blake2b_hash_t* hash1 = nullptr;
  cardano_blake2b_hash_t* hash2 = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_compute_hash((const byte_t*)"data2", 5, CARDANO_BLAKE2B_HASH_SIZE_512, &hash2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int result = cardano_blake2b_hash_compare(hash1, hash2);

  // Assert
  EXPECT_GT(result, 0);

  // Cleanup
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_blake2b_hash_compare, returnZeroIfBothAreNull)
{
  // Act
  int result = cardano_blake2b_hash_compare(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, 0);
}

TEST(cardano_blake2b_hash_compare, returnNegativeIfLhsIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int result = cardano_blake2b_hash_compare(nullptr, hash);

  // Assert
  EXPECT_LT(result, 0);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_blake2b_hash_compare, returnPositiveIfRhsIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_compute_hash((const byte_t*)"data", 4, CARDANO_BLAKE2B_HASH_SIZE_512, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int result = cardano_blake2b_hash_compare(hash, nullptr);

  // Assert
  EXPECT_GT(result, 0);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}
