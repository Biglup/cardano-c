/**
 * \file stake_vote_delegation_cert.cpp
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
#include <cardano/certs/stake_vote_delegation_cert.h>
#include <cardano/common/drep.h>
#include <cardano/crypto/blake2b_hash.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR            = "840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL_CBOR = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* DREP_CBOR       = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* POOL_KEY_HASH   = "0000000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_stake_vote_delegation_cert_t*
new_default_cert()
{
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                       result                     = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return stake_vote_delegation_cert;
};

/**
 * Creates a new default instance of the credential.
 *
 * @return A new instance of the credential.
 */
static cardano_credential_t*
new_default_cred()
{
  cardano_credential_t*  cred   = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CREDENTIAL_CBOR, strlen(CREDENTIAL_CBOR));
  cardano_error_t        result = cardano_credential_from_cbor(reader, &cred);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return cred;
}

/**
 * Creates a new default instance of the drep.
 * @return A new instance of the drep.
 */
static cardano_drep_t*
new_default_drep()
{
  cardano_drep_t*        drep   = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_CBOR, strlen(DREP_CBOR));
  cardano_error_t        result = cardano_drep_from_cbor(reader, &drep);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return drep;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_stake_vote_delegation_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  EXPECT_NE(stake_vote_delegation_cert, nullptr);

  // Act
  cardano_stake_vote_delegation_cert_ref(stake_vote_delegation_cert);

  // Assert
  EXPECT_THAT(stake_vote_delegation_cert, testing::Not((cardano_stake_vote_delegation_cert_t*)nullptr));
  EXPECT_EQ(cardano_stake_vote_delegation_cert_refcount(stake_vote_delegation_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}

TEST(cardano_stake_vote_delegation_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_stake_vote_delegation_cert_ref(nullptr);
}

TEST(cardano_stake_vote_delegation_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = nullptr;

  // Act
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}

TEST(cardano_stake_vote_delegation_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_stake_vote_delegation_cert_unref((cardano_stake_vote_delegation_cert_t**)nullptr);
}

TEST(cardano_stake_vote_delegation_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  EXPECT_NE(stake_vote_delegation_cert, nullptr);

  // Act
  cardano_stake_vote_delegation_cert_ref(stake_vote_delegation_cert);
  size_t ref_count = cardano_stake_vote_delegation_cert_refcount(stake_vote_delegation_cert);

  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  size_t updated_ref_count = cardano_stake_vote_delegation_cert_refcount(stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}

TEST(cardano_stake_vote_delegation_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  EXPECT_NE(stake_vote_delegation_cert, nullptr);

  // Act
  cardano_stake_vote_delegation_cert_ref(stake_vote_delegation_cert);
  size_t ref_count = cardano_stake_vote_delegation_cert_refcount(stake_vote_delegation_cert);

  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  size_t updated_ref_count = cardano_stake_vote_delegation_cert_refcount(stake_vote_delegation_cert);

  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(stake_vote_delegation_cert, (cardano_stake_vote_delegation_cert_t*)nullptr);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}

TEST(cardano_stake_vote_delegation_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_stake_vote_delegation_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_stake_vote_delegation_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = nullptr;
  const char*                           message                    = "This is a test message";

  // Act
  cardano_stake_vote_delegation_cert_set_last_error(stake_vote_delegation_cert, message);

  // Assert
  EXPECT_STREQ(cardano_stake_vote_delegation_cert_get_last_error(stake_vote_delegation_cert), "Object is NULL.");
}

TEST(cardano_stake_vote_delegation_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  EXPECT_NE(stake_vote_delegation_cert, nullptr);

  const char* message = nullptr;

  // Act
  cardano_stake_vote_delegation_cert_set_last_error(stake_vote_delegation_cert, message);

  // Assert
  EXPECT_STREQ(cardano_stake_vote_delegation_cert_get_last_error(stake_vote_delegation_cert), "");

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}

TEST(cardano_stake_vote_delegation_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(nullptr, &stake_vote_delegation_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_vote_delegation_cert_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_vote_delegation_cert_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*                writer = cardano_cbor_writer_new();
  cardano_stake_vote_delegation_cert_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_stake_vote_delegation_cert_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_stake_vote_delegation_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_to_cbor((cardano_stake_vote_delegation_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_stake_vote_delegation_cert_new, canCreateNewInstance)
{
  // Act
  cardano_credential_t*   cred          = new_default_cred();
  cardano_blake2b_hash_t* pool_key_hash = NULL;
  cardano_drep_t*         drep          = new_default_drep();

  EXPECT_EQ(cardano_blake2b_hash_from_hex(POOL_KEY_HASH, strlen(POOL_KEY_HASH), &pool_key_hash), CARDANO_SUCCESS);

  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  cardano_error_t result = cardano_stake_vote_delegation_cert_new(cred, pool_key_hash, drep, &stake_vote_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(stake_vote_delegation_cert, nullptr);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_credential_unref(&cred);
  cardano_blake2b_hash_unref(&pool_key_hash);
  cardano_drep_unref(&drep);
}

TEST(cardano_stake_vote_delegation_cert_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  cardano_error_t result = cardano_stake_vote_delegation_cert_new(nullptr, nullptr, nullptr, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_vote_delegation_cert_new, returnsErrorIfSecondArgIsNull)
{
  // Act
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  cardano_error_t result = cardano_stake_vote_delegation_cert_new((cardano_credential_t*)"", nullptr, nullptr, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_vote_delegation_cert_new, returnsErrorIfThirdArgIsNull)
{
  // Act
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  cardano_error_t result = cardano_stake_vote_delegation_cert_new((cardano_credential_t*)"", (cardano_blake2b_hash_t*)"", nullptr, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_vote_delegation_cert_new, returnsErrorIfCertIsNull)
{
  // Act

  cardano_error_t result = cardano_stake_vote_delegation_cert_new((cardano_credential_t*)"", (cardano_blake2b_hash_t*)"", (cardano_drep_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_stake_vote_delegation_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*   cred          = new_default_cred();
  cardano_blake2b_hash_t* pool_key_hash = NULL;
  cardano_drep_t*         drep          = new_default_drep();

  EXPECT_EQ(cardano_blake2b_hash_from_hex(POOL_KEY_HASH, strlen(POOL_KEY_HASH), &pool_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_stake_vote_delegation_cert_new(cred, pool_key_hash, drep, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_credential_unref(&cred);
  cardano_blake2b_hash_unref(&pool_key_hash);
  cardano_drep_unref(&drep);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_stake_vote_delegation_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_vote_delegation_cert_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex("84ef", strlen("84ef"));
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_vote_delegation_cert_from_cbor, returnsErrorIfInvalidFirstCredential)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex("840aef00581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000", strlen("840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000"));
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_vote_delegation_cert_from_cbor, returnsErrorIfInvalidPoolHash)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex("840a8200581c00000000000000000000000000000000000000000000000000000000ef1c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000", strlen("840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000"));
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_vote_delegation_cert_from_cbor, returnsErrorIfInvalidDrep)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex("840a8200581c00000000000000000000000000000000000000000000000000000000581c00000000000000000000000000000000000000000000000000000000ef00581c00000000000000000000000000000000000000000000000000000000", strlen("840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000"));
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_stake_vote_delegation_cert_set_credential, canSetCredential)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  cardano_credential_t*                 cred                       = new_default_cred();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_credential(stake_vote_delegation_cert, cred);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_credential_unref(&cred);
}

TEST(cardano_stake_vote_delegation_cert_set_credential, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cred = new_default_cred();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_credential(nullptr, cred);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&cred);
}

TEST(cardano_stake_vote_delegation_cert_set_credential, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_credential(stake_vote_delegation_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}

TEST(cardano_stake_vote_delegation_cert_get_credential, canGetCredential)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  cardano_credential_t*                 cred                       = new_default_cred();

  EXPECT_EQ(cardano_stake_vote_delegation_cert_set_credential(stake_vote_delegation_cert, cred), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* cred2 = cardano_stake_vote_delegation_cert_get_credential(stake_vote_delegation_cert);

  // Assert
  EXPECT_NE(cred2, nullptr);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_credential_unref(&cred);
  cardano_credential_unref(&cred2);
}

TEST(cardano_stake_vote_delegation_cert_get_credential, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cred = cardano_stake_vote_delegation_cert_get_credential(nullptr);

  // Assert
  EXPECT_EQ(cred, nullptr);
}

TEST(cardano_stake_vote_delegation_cert_get_pool_key_hash, canGetPoolKeyHash)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  cardano_blake2b_hash_t*               pool_key_hash              = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(POOL_KEY_HASH, strlen(POOL_KEY_HASH), &pool_key_hash), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_stake_vote_delegation_cert_set_pool_key_hash(stake_vote_delegation_cert, pool_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* pool_key_hash2 = cardano_stake_vote_delegation_cert_get_pool_key_hash(stake_vote_delegation_cert);

  // Assert
  EXPECT_NE(pool_key_hash2, nullptr);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_blake2b_hash_unref(&pool_key_hash);
  cardano_blake2b_hash_unref(&pool_key_hash2);
}

TEST(cardano_stake_vote_delegation_cert_get_pool_key_hash, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* pool_key_hash = cardano_stake_vote_delegation_cert_get_pool_key_hash(nullptr);

  // Assert
  EXPECT_EQ(pool_key_hash, nullptr);
}

TEST(cardano_stake_vote_delegation_cert_set_pool_key_hash, canSetPoolKeyHash)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  cardano_blake2b_hash_t*               pool_key_hash              = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(POOL_KEY_HASH, strlen(POOL_KEY_HASH), &pool_key_hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_pool_key_hash(stake_vote_delegation_cert, pool_key_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_blake2b_hash_unref(&pool_key_hash);
}

TEST(cardano_stake_vote_delegation_cert_set_pool_key_hash, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* pool_key_hash = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_pool_key_hash(nullptr, pool_key_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_stake_vote_delegation_cert_set_pool_key_hash, returnsErrorIfPoolKeyHashIsNull)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_pool_key_hash(stake_vote_delegation_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}

TEST(cardano_stake_vote_delegation_cert_get_drep, canGetDrep)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  cardano_drep_t*                       drep                       = new_default_drep();

  EXPECT_EQ(cardano_stake_vote_delegation_cert_set_drep(stake_vote_delegation_cert, drep), CARDANO_SUCCESS);

  // Act
  cardano_drep_t* drep2 = cardano_stake_vote_delegation_cert_get_drep(stake_vote_delegation_cert);

  // Assert
  EXPECT_NE(drep2, nullptr);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_drep_unref(&drep);
  cardano_drep_unref(&drep2);
}

TEST(cardano_stake_vote_delegation_cert_get_drep, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_drep_t* drep = cardano_stake_vote_delegation_cert_get_drep(nullptr);

  // Assert
  EXPECT_EQ(drep, nullptr);
}

TEST(cardano_stake_vote_delegation_cert_set_drep, canSetDrep)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();
  cardano_drep_t*                       drep                       = new_default_drep();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_drep(stake_vote_delegation_cert, drep);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_drep_unref(&drep);
}

TEST(cardano_stake_vote_delegation_cert_set_drep, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_drep_t* drep = new_default_drep();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_drep(nullptr, drep);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_stake_vote_delegation_cert_set_drep, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_set_drep(stake_vote_delegation_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
}
