/**
 * \file credential.cpp
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

#include <cardano/common/credential.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* KEY_HASH_HEX             = "00000000000000000000000000000000000000000000000000000000";
static const char* KEY_HASH_HEX_2           = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
static const char* INVALID_KEY_HASH_HEX     = "000000000000000000000000000000000000000000000000";
static const char* KEY_HASH_CREDENTIAL_CBOR = "8200581c00000000000000000000000000000000000000000000000000000000";

/* UNIT TESTS ****************************************************************/

TEST(cardano_credential_to_cbor, canSerializeKeyHashCredential)
{
  // Arrange
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  cardano_credential_t*  credential = nullptr;
  cardano_error_t        error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_to_cbor(credential, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex      = (char*)malloc(cbor_hex_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, KEY_HASH_CREDENTIAL_CBOR);

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_to_cbor(credential, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_to_cbor, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_credential_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_credential_from_cbor, canDeserializeKeyHashCredential)
{
  // Arrange
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(KEY_HASH_CREDENTIAL_CBOR, strlen(KEY_HASH_CREDENTIAL_CBOR));
  cardano_credential_t*  credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_cbor(reader, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(credential, testing::Not((cardano_credential_t*)nullptr));

  cardano_blake2b_hash_t* hash = cardano_credential_get_hash(credential);
  const char*             hex  = cardano_credential_get_hash_hex(credential);

  EXPECT_STREQ(hex, KEY_HASH_HEX);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  error                          = cardano_credential_get_type(credential, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_credential_from_cbor, returnErrorIfInvalidArraySize)
{
  // Arrange
  const char*            invalid_cbor = "8100581c00000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_cbor(reader, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  const char* error_msg = cardano_cbor_reader_get_last_error(reader);
  EXPECT_STREQ(error_msg, "There was an error decoding 'Credential', expected a 'Major Type: Byte String' (2) of 2 element(s) but got a 'Major Type: Byte String' (2) of 1 element(s).");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_credential_from_cbor, returnErrorIfInvalidCredentialType)
{
  // Arrange
  const char*            invalid_cbor = "8203581c00000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_cbor(reader, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  const char* error_msg = cardano_cbor_reader_get_last_error(reader);
  EXPECT_STREQ(error_msg, "There was an error decoding 'Credential', 'credential_type' must have a value between 0 and 1, but got 3.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_credential_from_cbor, returnErrorIfInvalidByteStringSize)
{
  // Arrange
  const char*            invalid_cbor = "8200581b0000000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_cbor(reader, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_credential_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_cbor(nullptr, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_credential_from_cbor, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(KEY_HASH_CREDENTIAL_CBOR, strlen(KEY_HASH_CREDENTIAL_CBOR));

  // Act
  cardano_error_t error = cardano_credential_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_credential_new, canCreateKeyHashCredential)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_new(
    hash,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(credential, testing::Not((cardano_credential_t*)nullptr));

  cardano_blake2b_hash_t* hash2       = cardano_credential_get_hash(credential);
  const byte_t*           hash2_bytes = cardano_blake2b_hash_get_data(hash2);
  const byte_t*           hash3_bytes = cardano_credential_get_hash_bytes(credential);
  const char*             hex         = cardano_credential_get_hash_hex(credential);

  EXPECT_EQ(memcmp(hash2_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_EQ(memcmp(hash3_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, KEY_HASH_HEX);
  EXPECT_EQ(cardano_credential_get_hash_bytes_size(credential), cardano_blake2b_hash_get_bytes_size(hash));
  EXPECT_EQ(cardano_credential_get_hash_hex_size(credential), cardano_blake2b_hash_get_hex_size(hash));

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  error                          = cardano_credential_get_type(credential, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_credential_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_credential_new(
    hash,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_credential_new, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_new(
    hash,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_credential_from_hash_hex, canCreateKeyHashCredential)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(credential, testing::Not((cardano_credential_t*)nullptr));

  cardano_blake2b_hash_t* hash2 = cardano_credential_get_hash(credential);
  const char*             hex   = cardano_credential_get_hash_hex(credential);

  EXPECT_STREQ(hex, KEY_HASH_HEX);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  error                          = cardano_credential_get_type(credential, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_credential_from_hash_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_credential_from_hash_hex, returnsErrorIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_credential_from_hash_hex, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_hash_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
}

TEST(cardano_credential_from_hash_hex, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_hash_hex(
    nullptr,
    0,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);
}

TEST(cardano_credential_from_hash_hex, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_new, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_new(
    nullptr,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);
}

TEST(cardano_credential_new, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_new(
    hash,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_credential_from_hash_bytes, canCreateKeyHashCredential)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(credential, testing::Not((cardano_credential_t*)nullptr));

  cardano_blake2b_hash_t* hash2       = cardano_credential_get_hash(credential);
  const byte_t*           hash2_bytes = cardano_blake2b_hash_get_data(hash2);
  const byte_t*           hash3_bytes = cardano_credential_get_hash_bytes(credential);
  const char*             hex         = cardano_credential_get_hash_hex(credential);

  EXPECT_EQ(memcmp(hash2_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_EQ(memcmp(hash3_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, KEY_HASH_HEX);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  error                          = cardano_credential_get_type(credential, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_credential_from_hash_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_credential_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_credential_from_hash_bytes, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  error = cardano_credential_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_credential_from_hash_bytes, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_credential_from_hash_bytes, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_hash_bytes(
    nullptr,
    0,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);
}

TEST(cardano_credential_from_hash_bytes, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_credential_from_hash_bytes, returnsErrorIfHashIsInvalid)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_credential_from_hash_bytes(
    nullptr,
    0,
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);
}

TEST(cardano_credential_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_ref(credential);

  // Assert
  EXPECT_THAT(credential, testing::Not((cardano_credential_t*)nullptr));
  EXPECT_EQ(cardano_credential_refcount(credential), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_credential_unref(&credential);
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_credential_ref(nullptr);
}

TEST(cardano_credential_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_credential_unref((cardano_credential_t**)nullptr);
}

TEST(cardano_credential_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_ref(credential);
  size_t ref_count = cardano_credential_refcount(credential);

  cardano_credential_unref(&credential);
  size_t updated_ref_count = cardano_credential_refcount(credential);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_ref(credential);
  size_t ref_count = cardano_credential_refcount(credential);

  cardano_credential_unref(&credential);
  size_t updated_ref_count = cardano_credential_refcount(credential);

  cardano_credential_unref(&credential);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_credential_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_credential_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_credential_set_last_error(credential, message);

  // Assert
  EXPECT_STREQ(cardano_credential_get_last_error(credential), "Object is NULL.");
}

TEST(cardano_credential_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_credential_set_last_error(credential, message);

  // Assert
  EXPECT_STREQ(cardano_credential_get_last_error(credential), "");

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_get_hash, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_credential_get_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_credential_get_hash_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  const byte_t* hash = cardano_credential_get_hash_bytes(nullptr);

  // Assert
  EXPECT_EQ(hash, (const byte_t*)nullptr);
}

TEST(cardano_credential_get_hash_hex, returnsNullIfGivenANullPtr)
{
  // Act
  const char* hash = cardano_credential_get_hash_hex(nullptr);

  // Assert
  EXPECT_EQ(hash, (const char*)nullptr);
}

TEST(cardano_credential_get_type, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

  // Act
  cardano_error_t error = cardano_credential_get_type(nullptr, &type);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_credential_get_type, returnsErrorIfTypeIsNull)
{
  // Act
  cardano_error_t error = cardano_credential_get_type((cardano_credential_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_credential_set_type, returnsErrorIfGivenANullPtr)
{
  // Act
  cardano_error_t error = cardano_credential_set_type(nullptr, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_credential_set_type, returnsErrorIfTypeIsInvalid)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_set_type(credential, (cardano_credential_type_t)3);

  // Assert
  EXPECT_EQ(error, CARDANO_INVALID_CREDENTIAL_TYPE);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_set_type, canSetType)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_set_type(credential, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  error                          = cardano_credential_get_type(credential, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_credential_set_hash, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_error_t error = cardano_credential_set_hash(nullptr, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_credential_set_hash, returnsErrorIfHashIsNull)
{
  // Act
  cardano_error_t error = cardano_credential_set_hash((cardano_credential_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_credential_set_hash, canSetHash)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX_2,
    strlen(KEY_HASH_HEX_2),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_set_hash(credential, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash2 = cardano_credential_get_hash(credential);
  const char*             hex   = cardano_credential_get_hash_hex(credential);

  EXPECT_EQ(memcmp(cardano_blake2b_hash_get_data(hash2), cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, KEY_HASH_HEX_2);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_credential_set_hash, returnErrorIfWorngHashSize)
{
  // Arrange
  cardano_credential_t*   credential = nullptr;
  cardano_blake2b_hash_t* hash       = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_credential_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_credential_set_hash(credential, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_credential_get_hash_hex_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_credential_get_hash_hex_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_credential_get_hash_bytes_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_credential_get_hash_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}