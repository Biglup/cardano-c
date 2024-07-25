/**
 * \file genesis_key_delegation_cert.cpp
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

#include "tests/allocators_helpers.h"
#include <cardano/cbor/cbor_reader.h>
#include <cardano/certs/genesis_key_delegation_cert.h>
#include <cardano/crypto/blake2b_hash.h>

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                  = "8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003";
static const char* GENESIS_HASH          = "00010001000100010001000100010001000100010001000100010001";
static const char* GENESIS_DELEGATE_HASH = "00020002000200020002000200020002000200020002000200020002";
static const char* VRF_KEY_HASH          = "0003000300030003000300030003000300030003000300030003000300030003";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_genesis_key_delegation_cert_t*
new_default_cert()
{
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                        result                      = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return genesis_key_delegation_cert;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_genesis_key_delegation_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_genesis_key_delegation_cert_ref(genesis_key_delegation_cert);

  // Assert
  EXPECT_THAT(genesis_key_delegation_cert, testing::Not((cardano_genesis_key_delegation_cert_t*)nullptr));
  EXPECT_EQ(cardano_genesis_key_delegation_cert_refcount(genesis_key_delegation_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_genesis_key_delegation_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_genesis_key_delegation_cert_ref(nullptr);
}

TEST(cardano_genesis_key_delegation_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = nullptr;

  // Act
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_genesis_key_delegation_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_genesis_key_delegation_cert_unref((cardano_genesis_key_delegation_cert_t**)nullptr);
}

TEST(cardano_genesis_key_delegation_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_genesis_key_delegation_cert_ref(genesis_key_delegation_cert);
  size_t ref_count = cardano_genesis_key_delegation_cert_refcount(genesis_key_delegation_cert);

  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  size_t updated_ref_count = cardano_genesis_key_delegation_cert_refcount(genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_genesis_key_delegation_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_genesis_key_delegation_cert_ref(genesis_key_delegation_cert);
  size_t ref_count = cardano_genesis_key_delegation_cert_refcount(genesis_key_delegation_cert);

  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  size_t updated_ref_count = cardano_genesis_key_delegation_cert_refcount(genesis_key_delegation_cert);

  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(genesis_key_delegation_cert, (cardano_genesis_key_delegation_cert_t*)nullptr);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_genesis_key_delegation_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_genesis_key_delegation_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_genesis_key_delegation_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = nullptr;
  const char*                            message                     = "This is a test message";

  // Act
  cardano_genesis_key_delegation_cert_set_last_error(genesis_key_delegation_cert, message);

  // Assert
  EXPECT_STREQ(cardano_genesis_key_delegation_cert_get_last_error(genesis_key_delegation_cert), "Object is NULL.");
}

TEST(cardano_genesis_key_delegation_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  const char* message = nullptr;

  // Act
  cardano_genesis_key_delegation_cert_set_last_error(genesis_key_delegation_cert, message);

  // Assert
  EXPECT_STREQ(cardano_genesis_key_delegation_cert_get_last_error(genesis_key_delegation_cert), "");

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(nullptr, &genesis_key_delegation_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_genesis_key_delegation_cert_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*                 writer = cardano_cbor_writer_new();
  cardano_genesis_key_delegation_cert_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_genesis_key_delegation_cert_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_genesis_key_delegation_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_to_cbor((cardano_genesis_key_delegation_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_genesis_key_delegation_cert_new, canCreateNewInstance)
{
  // Act
  cardano_blake2b_hash_t* genesis_hash          = NULL;
  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;
  cardano_blake2b_hash_t* vrf_key_hash          = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_HASH, strlen(GENESIS_HASH), &genesis_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_DELEGATE_HASH, strlen(GENESIS_DELEGATE_HASH), &genesis_delegate_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(VRF_KEY_HASH, strlen(VRF_KEY_HASH), &vrf_key_hash), CARDANO_SUCCESS);

  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  cardano_error_t result = cardano_genesis_key_delegation_cert_new(genesis_hash, genesis_delegate_hash, vrf_key_hash, &genesis_key_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
  cardano_blake2b_hash_unref(&vrf_key_hash);
}

TEST(cardano_genesis_key_delegation_cert_new, returnsErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;
  cardano_blake2b_hash_t* vrf_key_hash          = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_DELEGATE_HASH, strlen(GENESIS_DELEGATE_HASH), &genesis_delegate_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(VRF_KEY_HASH, strlen(VRF_KEY_HASH), &vrf_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  cardano_error_t result = cardano_genesis_key_delegation_cert_new(nullptr, genesis_delegate_hash, vrf_key_hash, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
  cardano_blake2b_hash_unref(&vrf_key_hash);
}

TEST(cardano_genesis_key_delegation_cert_new, returnsErrorIfSecondArgIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* genesis_hash = NULL;
  cardano_blake2b_hash_t* vrf_key_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_HASH, strlen(GENESIS_HASH), &genesis_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(VRF_KEY_HASH, strlen(VRF_KEY_HASH), &vrf_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  cardano_error_t result = cardano_genesis_key_delegation_cert_new(genesis_hash, nullptr, vrf_key_hash, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_blake2b_hash_unref(&vrf_key_hash);
}

TEST(cardano_genesis_key_delegation_cert_new, returnsErrorIfThirdArgIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* genesis_hash          = NULL;
  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_HASH, strlen(GENESIS_HASH), &genesis_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_DELEGATE_HASH, strlen(GENESIS_DELEGATE_HASH), &genesis_delegate_hash), CARDANO_SUCCESS);

  // Act
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  cardano_error_t result = cardano_genesis_key_delegation_cert_new(genesis_hash, genesis_delegate_hash, nullptr, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
}

TEST(cardano_genesis_key_delegation_cert_new, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* genesis_hash          = NULL;
  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;
  cardano_blake2b_hash_t* vrf_key_hash          = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_HASH, strlen(GENESIS_HASH), &genesis_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_DELEGATE_HASH, strlen(GENESIS_DELEGATE_HASH), &genesis_delegate_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(VRF_KEY_HASH, strlen(VRF_KEY_HASH), &vrf_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_new(genesis_hash, genesis_delegate_hash, vrf_key_hash, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
  cardano_blake2b_hash_unref(&vrf_key_hash);
}

TEST(cardano_genesis_key_delegation_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* genesis_hash          = NULL;
  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;
  cardano_blake2b_hash_t* vrf_key_hash          = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_HASH, strlen(GENESIS_HASH), &genesis_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(GENESIS_DELEGATE_HASH, strlen(GENESIS_DELEGATE_HASH), &genesis_delegate_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(VRF_KEY_HASH, strlen(VRF_KEY_HASH), &vrf_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_genesis_key_delegation_cert_new(genesis_hash, genesis_delegate_hash, vrf_key_hash, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
  cardano_blake2b_hash_unref(&vrf_key_hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("83ef", strlen("83ef"));
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfInvalidCertType)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("8400", strlen("8400"));
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfInvalidFirstHash)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("8405ef1c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003", strlen("8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003"));
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfInvalidSecondHash)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("8405581c00010001000100010001000100010001000100010001000100010001ef1c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003", strlen("8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003"));
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_genesis_key_delegation_cert_from_cbor, returnsErrorIfInvalidThirdHash)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("8405581c00010001000100010001000100010001000100010001000100010001581c00020002000200020002000200020002000200020002000200020002ef200003000300030003000300030003000300030003000300030003000300030003", strlen("8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003"));
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_genesis_key_delegation_cert_get_genesis_hash, canGetGenesisHash)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_blake2b_hash_t* genesis_hash = cardano_genesis_key_delegation_cert_get_genesis_hash(genesis_key_delegation_cert);

  // Assert
  EXPECT_NE(genesis_hash, nullptr);

  const size_t size = cardano_blake2b_hash_get_hex_size(genesis_hash);
  char*        hex  = (char*)malloc(size);

  cardano_error_t result = cardano_blake2b_hash_to_hex(genesis_hash, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, GENESIS_HASH);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  free(hex);
}

TEST(cardano_genesis_key_delegation_cert_get_genesis_hash, returnsNullIfCertIsNull)
{
  // Act
  cardano_blake2b_hash_t* genesis_hash = cardano_genesis_key_delegation_cert_get_genesis_hash(nullptr);

  // Assert
  EXPECT_EQ(genesis_hash, nullptr);
}

TEST(cardano_genesis_key_delegation_cert_set_genesis_hash, canSetGenesisHash)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  cardano_blake2b_hash_t* genesis_hash = NULL;
  cardano_error_t         result       = cardano_blake2b_hash_from_hex("00020002000200020002000200020002000200020002000200020002", strlen("00020002000200020002000200020002000200020002000200020002"), &genesis_hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_genesis_key_delegation_cert_set_genesis_hash(genesis_key_delegation_cert, genesis_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* new_genesis_hash = cardano_genesis_key_delegation_cert_get_genesis_hash(genesis_key_delegation_cert);

  const size_t size = cardano_blake2b_hash_get_hex_size(new_genesis_hash);
  char*        hex  = (char*)malloc(size);

  result = cardano_blake2b_hash_to_hex(new_genesis_hash, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "00020002000200020002000200020002000200020002000200020002");

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_blake2b_hash_unref(&new_genesis_hash);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  free(hex);
}

TEST(cardano_genesis_key_delegation_cert_set_genesis_hash, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* genesis_hash = NULL;
  cardano_error_t         result       = cardano_blake2b_hash_from_hex("00020002000200020002000200020002000200020002000200020002", strlen("00020002000200020002000200020002000200020002000200020002"), &genesis_hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_genesis_key_delegation_cert_set_genesis_hash(nullptr, genesis_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_hash);
}

TEST(cardano_genesis_key_delegation_cert_set_genesis_hash, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_set_genesis_hash(genesis_key_delegation_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_genesis_key_delegation_cert_get_genesis_delegate_hash, canGetGenesisDelegateHash)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_blake2b_hash_t* genesis_delegate_hash = cardano_genesis_key_delegation_cert_get_genesis_delegate_hash(genesis_key_delegation_cert);

  // Assert
  EXPECT_NE(genesis_delegate_hash, nullptr);

  const size_t size = cardano_blake2b_hash_get_hex_size(genesis_delegate_hash);
  char*        hex  = (char*)malloc(size);

  cardano_error_t result = cardano_blake2b_hash_to_hex(genesis_delegate_hash, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, GENESIS_DELEGATE_HASH);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  free(hex);
}

TEST(cardano_genesis_key_delegation_cert_get_genesis_delegate_hash, returnsNullIfCertIsNull)
{
  // Act
  cardano_blake2b_hash_t* genesis_delegate_hash = cardano_genesis_key_delegation_cert_get_genesis_delegate_hash(nullptr);

  // Assert
  EXPECT_EQ(genesis_delegate_hash, nullptr);
}

TEST(cardano_genesis_key_delegation_cert_set_genesis_delegate_hash, canSetGenesisDelegateHash)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;
  cardano_error_t         result                = cardano_blake2b_hash_from_hex("0003000300030003000300030003000300030003000300030003000300030003", strlen("0003000300030003000300030003000300030003000300030003000300030003"), &genesis_delegate_hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_genesis_key_delegation_cert_set_genesis_delegate_hash(genesis_key_delegation_cert, genesis_delegate_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* new_genesis_delegate_hash = cardano_genesis_key_delegation_cert_get_genesis_delegate_hash(genesis_key_delegation_cert);

  const size_t size = cardano_blake2b_hash_get_hex_size(new_genesis_delegate_hash);
  char*        hex  = (char*)malloc(size);

  result = cardano_blake2b_hash_to_hex(new_genesis_delegate_hash, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "0003000300030003000300030003000300030003000300030003000300030003");

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
  cardano_blake2b_hash_unref(&new_genesis_delegate_hash);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  free(hex);
}

TEST(cardano_genesis_key_delegation_cert_set_genesis_delegate_hash, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;
  cardano_error_t         result                = cardano_blake2b_hash_from_hex("0003000300030003000300030003000300030003000300030003000300030003", strlen("0003000300030003000300030003000300030003000300030003000300030003"), &genesis_delegate_hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_genesis_key_delegation_cert_set_genesis_delegate_hash(nullptr, genesis_delegate_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
}

TEST(cardano_genesis_key_delegation_cert_set_genesis_delegate_hash, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_set_genesis_delegate_hash(genesis_key_delegation_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_genesis_key_delegation_cert_get_vrf_key_hash, canGetVrfKeyHash)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_blake2b_hash_t* vrf_key_hash = cardano_genesis_key_delegation_cert_get_vrf_key_hash(genesis_key_delegation_cert);

  // Assert
  EXPECT_NE(vrf_key_hash, nullptr);

  const size_t size = cardano_blake2b_hash_get_hex_size(vrf_key_hash);
  char*        hex  = (char*)malloc(size);

  cardano_error_t result = cardano_blake2b_hash_to_hex(vrf_key_hash, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, VRF_KEY_HASH);

  // Cleanup
  cardano_blake2b_hash_unref(&vrf_key_hash);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  free(hex);
}

TEST(cardano_genesis_key_delegation_cert_get_vrf_key_hash, returnsNullIfCertIsNull)
{
  // Act
  cardano_blake2b_hash_t* vrf_key_hash = cardano_genesis_key_delegation_cert_get_vrf_key_hash(nullptr);

  // Assert
  EXPECT_EQ(vrf_key_hash, nullptr);
}

TEST(cardano_genesis_key_delegation_cert_set_vrf_key_hash, canSetVrfKeyHash)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  cardano_blake2b_hash_t* vrf_key_hash = NULL;
  cardano_error_t         result       = cardano_blake2b_hash_from_hex("00010001000100010001000100010001000100010001000100010001", strlen("00010001000100010001000100010001000100010001000100010001"), &vrf_key_hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_genesis_key_delegation_cert_set_vrf_key_hash(genesis_key_delegation_cert, vrf_key_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* new_vrf_key_hash = cardano_genesis_key_delegation_cert_get_vrf_key_hash(genesis_key_delegation_cert);

  const size_t size = cardano_blake2b_hash_get_hex_size(new_vrf_key_hash);
  char*        hex  = (char*)malloc(size);

  result = cardano_blake2b_hash_to_hex(new_vrf_key_hash, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "00010001000100010001000100010001000100010001000100010001");

  // Cleanup
  cardano_blake2b_hash_unref(&vrf_key_hash);
  cardano_blake2b_hash_unref(&new_vrf_key_hash);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  free(hex);
}

TEST(cardano_genesis_key_delegation_cert_set_vrf_key_hash, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* vrf_key_hash = NULL;
  cardano_error_t         result       = cardano_blake2b_hash_from_hex("00010001000100010001000100010001000100010001000100010001", strlen("00010001000100010001000100010001000100010001000100010001"), &vrf_key_hash);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_genesis_key_delegation_cert_set_vrf_key_hash(nullptr, vrf_key_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&vrf_key_hash);
}

TEST(cardano_genesis_key_delegation_cert_set_vrf_key_hash, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = new_default_cert();
  EXPECT_NE(genesis_key_delegation_cert, nullptr);

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_set_vrf_key_hash(genesis_key_delegation_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}
