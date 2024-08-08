/**
 * \file pool_retirement_cert.cpp
 *
 * \author angel.castillo
 * \date   Aug 05, 2024
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/certs/pool_retirement_cert.h>
#include <cardano/crypto/blake2b_hash.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8";
static const char* HASH = "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_pool_retirement_cert_t*
new_default_cert()
{
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                 result               = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return pool_retirement_cert;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_pool_retirement_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = new_default_cert();
  EXPECT_NE(pool_retirement_cert, nullptr);

  // Act
  cardano_pool_retirement_cert_ref(pool_retirement_cert);

  // Assert
  EXPECT_THAT(pool_retirement_cert, testing::Not((cardano_pool_retirement_cert_t*)nullptr));
  EXPECT_EQ(cardano_pool_retirement_cert_refcount(pool_retirement_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
}

TEST(cardano_pool_retirement_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_retirement_cert_ref(nullptr);
}

TEST(cardano_pool_retirement_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = nullptr;

  // Act
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
}

TEST(cardano_pool_retirement_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_retirement_cert_unref((cardano_pool_retirement_cert_t**)nullptr);
}

TEST(cardano_pool_retirement_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = new_default_cert();
  EXPECT_NE(pool_retirement_cert, nullptr);

  // Act
  cardano_pool_retirement_cert_ref(pool_retirement_cert);
  size_t ref_count = cardano_pool_retirement_cert_refcount(pool_retirement_cert);

  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
  size_t updated_ref_count = cardano_pool_retirement_cert_refcount(pool_retirement_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
}

TEST(cardano_pool_retirement_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = new_default_cert();
  EXPECT_NE(pool_retirement_cert, nullptr);

  // Act
  cardano_pool_retirement_cert_ref(pool_retirement_cert);
  size_t ref_count = cardano_pool_retirement_cert_refcount(pool_retirement_cert);

  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
  size_t updated_ref_count = cardano_pool_retirement_cert_refcount(pool_retirement_cert);

  cardano_pool_retirement_cert_unref(&pool_retirement_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(pool_retirement_cert, (cardano_pool_retirement_cert_t*)nullptr);

  // Cleanup
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
}

TEST(cardano_pool_retirement_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_pool_retirement_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_pool_retirement_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = nullptr;
  const char*                     message              = "This is a test message";

  // Act
  cardano_pool_retirement_cert_set_last_error(pool_retirement_cert, message);

  // Assert
  EXPECT_STREQ(cardano_pool_retirement_cert_get_last_error(pool_retirement_cert), "Object is NULL.");
}

TEST(cardano_pool_retirement_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = new_default_cert();
  EXPECT_NE(pool_retirement_cert, nullptr);

  const char* message = nullptr;

  // Act
  cardano_pool_retirement_cert_set_last_error(pool_retirement_cert, message);

  // Assert
  EXPECT_STREQ(cardano_pool_retirement_cert_get_last_error(pool_retirement_cert), "");

  // Cleanup
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
}

TEST(cardano_pool_retirement_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(nullptr, &pool_retirement_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_retirement_cert_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_retirement_cert_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*          writer = cardano_cbor_writer_new();
  cardano_pool_retirement_cert_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_pool_retirement_cert_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_pool_retirement_cert_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_pool_retirement_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_pool_retirement_cert_to_cbor((cardano_pool_retirement_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_pool_retirement_cert_new, canCreateNewInstance)
{
  // Act
  cardano_blake2b_hash_t* hash = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  result = cardano_pool_retirement_cert_new(hash, 0, &pool_retirement_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(pool_retirement_cert, nullptr);

  // Cleanup
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_retirement_cert_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  cardano_error_t result = cardano_pool_retirement_cert_new(nullptr, 0, &pool_retirement_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_retirement_cert_new, returnsErrorIfSecondArgIsNull)
{
  // Act
  cardano_blake2b_hash_t* hash = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  result = cardano_pool_retirement_cert_new(hash, 0, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_retirement_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* hash   = NULL;
  cardano_error_t         result = cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_pool_retirement_cert_new(hash, 0, &pool_retirement_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_pool_retirement_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_retirement_cert_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("83ef", strlen("83ef"));
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_retirement_cert_from_cbor, returnsErrorIfInvalidHash)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("8304ef1cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8", strlen("8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8"));
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_retirement_cert_from_cbor, returnsErrorIfInvalidDeposit)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92efefe8", strlen("8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8"));
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_pool_retirement_cert_get_pool_key_hash, returnsNullIfCertIsNull)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_pool_retirement_cert_get_pool_key_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, nullptr);
}

TEST(cardano_pool_retirement_cert_get_pool_key_hash, returnsTheHash)
{
  // Arrange
  cardano_pool_retirement_cert_t* cert = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_blake2b_hash_t* hash = cardano_pool_retirement_cert_get_pool_key_hash(cert);

  // Assert
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_pool_retirement_cert_unref(&cert);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_retirement_cert_set_pool_key_hash, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_pool_retirement_cert_set_pool_key_hash(nullptr, hash);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_retirement_cert_set_pool_key_hash, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_pool_retirement_cert_t* cert = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_set_pool_key_hash(cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_retirement_cert_unref(&cert);
}

TEST(cardano_pool_retirement_cert_set_pool_key_hash, setsTheHash)
{
  // Arrange
  cardano_pool_retirement_cert_t* cert = new_default_cert();
  EXPECT_NE(cert, nullptr);

  cardano_blake2b_hash_t* hash = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(HASH, strlen(HASH), &hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_pool_retirement_cert_set_pool_key_hash(cert, hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_retirement_cert_unref(&cert);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_pool_retirement_cert_get_epoch, returnsZeroIfCertIsNull)
{
  // Act
  uint64_t epoch = cardano_pool_retirement_cert_get_epoch(nullptr);

  // Assert
  EXPECT_EQ(epoch, 0);
}

TEST(cardano_pool_retirement_cert_get_epoch, returnsTheEpoch)
{
  // Arrange
  cardano_pool_retirement_cert_t* cert = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  uint64_t epoch = cardano_pool_retirement_cert_get_epoch(cert);

  // Assert
  EXPECT_EQ(epoch, 1000);

  // Cleanup
  cardano_pool_retirement_cert_unref(&cert);
}

TEST(cardano_pool_retirement_cert_set_epoch, returnsErrorIfCertIsNull)
{
  // Act
  cardano_error_t result = cardano_pool_retirement_cert_set_epoch(nullptr, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_retirement_cert_set_epoch, setsTheEpoch)
{
  // Arrange
  cardano_pool_retirement_cert_t* cert = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_set_epoch(cert, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_retirement_cert_unref(&cert);
}
