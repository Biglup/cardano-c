/**
 * \file voter.cpp
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
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/voting_procedures/voter.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR            = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_2          = "8200581c00000000000000000000000000000000000000000000000000000001";
static const char* CREDENTIAL_CBOR = "8200581c00000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the voter.
 * @return A new instance of the voter.
 */
static cardano_voter_t*
new_default_voter()
{
  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t        result = cardano_voter_from_cbor(reader, &voter);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
};

/**
 * Creates a new default instance of the voter.
 * @return A new instance of the voter.
 */
static cardano_voter_t*
new_default_voter2()
{
  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_2, strlen(CBOR_2));
  cardano_error_t        result = cardano_voter_from_cbor(reader, &voter);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
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

/* UNIT TESTS ****************************************************************/

TEST(cardano_voter_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();
  EXPECT_NE(voter, nullptr);

  // Act
  cardano_voter_ref(voter);

  // Assert
  EXPECT_THAT(voter, testing::Not((cardano_voter_t*)nullptr));
  EXPECT_EQ(cardano_voter_refcount(voter), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_voter_unref(&voter);
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voter_ref(nullptr);
}

TEST(cardano_voter_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_voter_t* voter = nullptr;

  // Act
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voter_unref((cardano_voter_t**)nullptr);
}

TEST(cardano_voter_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();
  EXPECT_NE(voter, nullptr);

  // Act
  cardano_voter_ref(voter);
  size_t ref_count = cardano_voter_refcount(voter);

  cardano_voter_unref(&voter);
  size_t updated_ref_count = cardano_voter_refcount(voter);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();
  EXPECT_NE(voter, nullptr);

  // Act
  cardano_voter_ref(voter);
  size_t ref_count = cardano_voter_refcount(voter);

  cardano_voter_unref(&voter);
  size_t updated_ref_count = cardano_voter_refcount(voter);

  cardano_voter_unref(&voter);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(voter, (cardano_voter_t*)nullptr);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_voter_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_voter_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_voter_t* voter   = nullptr;
  const char*      message = "This is a test message";

  // Act
  cardano_voter_set_last_error(voter, message);

  // Assert
  EXPECT_STREQ(cardano_voter_get_last_error(voter), "Object is NULL.");
}

TEST(cardano_voter_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();
  EXPECT_NE(voter, nullptr);

  const char* message = nullptr;

  // Act
  cardano_voter_set_last_error(voter, message);

  // Assert
  EXPECT_STREQ(cardano_voter_get_last_error(voter), "");

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_voter_t* voter = NULL;

  // Act
  cardano_error_t result = cardano_voter_from_cbor(nullptr, &voter);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_voter_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voter_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_voter_t*       cert   = new_default_voter();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_voter_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_voter_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_voter_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_voter_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_voter_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_voter_to_cbor((cardano_voter_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_voter_new, canCreateNewInstance)
{
  // Act
  cardano_credential_t* cred = new_default_cred();

  cardano_voter_t* voter = NULL;

  cardano_error_t result = cardano_voter_new(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH, cred, &voter);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(voter, nullptr);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_credential_unref(&cred);
}

TEST(cardano_voter_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_voter_t* voter = NULL;

  cardano_error_t result = cardano_voter_new(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH, nullptr, &voter);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_new, returnsErrorIfCertIsNull)
{
  // Act

  cardano_error_t result = cardano_voter_new(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH, (cardano_credential_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_voter_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t* cred = new_default_cred();

  // Act
  cardano_voter_t* voter = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_voter_new(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH, cred, &voter);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_credential_unref(&cred);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voter_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_voter_from_cbor(reader, &voter);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voter_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_voter_from_cbor(reader, &voter);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voter_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82ef", strlen("82ef"));
  cardano_voter_t*       voter  = NULL;

  // Act
  cardano_error_t result = cardano_voter_from_cbor(reader, &voter);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voter_from_cbor, returnsErrorIfInvalidFirstCredential)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200ef1c00000000000000000000000000000000000000000000000000000000", strlen("8200581c00000000000000000000000000000000000000000000000000000000"));
  cardano_voter_t*       voter  = NULL;

  // Act
  cardano_error_t result = cardano_voter_from_cbor(reader, &voter);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_voter_set_credential, canSetCredential)
{
  // Arrange
  cardano_voter_t*      voter = new_default_voter();
  cardano_credential_t* cred  = new_default_cred();

  // Act
  cardano_error_t result = cardano_voter_set_credential(voter, cred);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_credential_unref(&cred);
}

TEST(cardano_voter_set_credential, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cred = new_default_cred();

  // Act
  cardano_error_t result = cardano_voter_set_credential(nullptr, cred);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&cred);
}

TEST(cardano_voter_set_credential, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();

  // Act
  cardano_error_t result = cardano_voter_set_credential(voter, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_get_credential, canGetCredential)
{
  // Arrange
  cardano_voter_t*      voter = new_default_voter();
  cardano_credential_t* cred  = new_default_cred();

  EXPECT_EQ(cardano_voter_set_credential(voter, cred), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* cred2 = cardano_voter_get_credential(voter);

  // Assert
  EXPECT_NE(cred2, nullptr);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_credential_unref(&cred);
  cardano_credential_unref(&cred2);
}

TEST(cardano_voter_get_credential, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cred = cardano_voter_get_credential(nullptr);

  // Assert
  EXPECT_EQ(cred, nullptr);
}

// Setter and Getters

TEST(cardano_voter_get_type, canGetType)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();

  // Act
  cardano_voter_type_t type;
  cardano_error_t      result = cardano_voter_get_type(voter, &type);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_get_type, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_voter_type_t type;
  cardano_error_t      result = cardano_voter_get_type(nullptr, &type);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_get_type, returnsErrorIfTypeIsNull)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();

  // Act
  cardano_error_t result = cardano_voter_get_type(voter, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_set_type, canSetType)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();

  // Act
  cardano_error_t result = cardano_voter_set_type(voter, CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_set_type, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_voter_set_type(nullptr, CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_equals, canCompare)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter();
  cardano_voter_t* voter2 = new_default_voter();

  // Act
  bool result = cardano_voter_equals(voter1, voter2);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_voter_unref(&voter1);
  cardano_voter_unref(&voter2);
}

TEST(cardano_voter_equals, returnsFalseIfObjectsAreDifferent)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter();
  cardano_voter_t* voter2 = new_default_voter();

  EXPECT_EQ(cardano_voter_set_type(voter2, CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH), CARDANO_SUCCESS);

  // Act
  bool result = cardano_voter_equals(voter1, voter2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_voter_unref(&voter1);
  cardano_voter_unref(&voter2);
}

TEST(cardano_voter_equals, returnsFalseIfOneObjectIsNull)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter();
  cardano_voter_t* voter2 = nullptr;

  // Act
  bool result = cardano_voter_equals(voter1, voter2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_voter_unref(&voter1);
}

TEST(cardano_voter_equals, returnsFalseIfOneObjectIsNull2)
{
  // Arrange
  cardano_voter_t* voter1 = nullptr;
  cardano_voter_t* voter2 = new_default_voter();

  // Act
  bool result = cardano_voter_equals(voter1, voter2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_voter_unref(&voter2);
}

TEST(cardano_voter_equals, returnsTrueIfBothObjectsAreNull)
{
  // Act
  bool result = cardano_voter_equals(nullptr, nullptr);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_voter_compare, returnsZeroIfBothAreNullPtr)
{
  // Act
  int result = cardano_voter_compare(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, 0);
}

TEST(cardano_voter_compare, returnsMinusOneIfFirstVoterIsNull)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();

  // Act
  int result = cardano_voter_compare(nullptr, voter);

  // Assert
  EXPECT_EQ(result, -1);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_compare, returnsOneIfSecondVoterIsNull)
{
  // Arrange
  cardano_voter_t* voter = new_default_voter();

  // Act
  int result = cardano_voter_compare(voter, nullptr);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(cardano_voter_compare, returnsZeroIfVotersAreEqual)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter();
  cardano_voter_t* voter2 = new_default_voter();

  // Act
  int result = cardano_voter_compare(voter1, voter2);

  // Assert
  EXPECT_EQ(result, 0);

  // Cleanup
  cardano_voter_unref(&voter1);
  cardano_voter_unref(&voter2);
}

TEST(cardano_voter_compare, returnsNegativeIfFirstVoterIsLessThanSecond)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter();
  cardano_voter_t* voter2 = new_default_voter();

  EXPECT_EQ(cardano_voter_set_type(voter2, CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH), CARDANO_SUCCESS);

  // Act
  int result = cardano_voter_compare(voter1, voter2);

  // Assert
  EXPECT_LT(result, 0);

  // Cleanup
  cardano_voter_unref(&voter1);
  cardano_voter_unref(&voter2);
}

TEST(cardano_voter_compare, returnsPositiveIfFirstVoterIsGreaterThanSecond)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter();
  cardano_voter_t* voter2 = new_default_voter();

  EXPECT_EQ(cardano_voter_set_type(voter1, CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH), CARDANO_SUCCESS);

  // Act
  int result = cardano_voter_compare(voter1, voter2);

  // Assert
  EXPECT_GT(result, 0);

  // Cleanup
  cardano_voter_unref(&voter1);
  cardano_voter_unref(&voter2);
}

TEST(cardano_voter_compare, returnsNegativeIfFirstVoterHashIsLessThanSecond)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter();
  cardano_voter_t* voter2 = new_default_voter2();

  // Act
  int result = cardano_voter_compare(voter1, voter2);

  // Assert
  EXPECT_LT(result, 0);

  // Cleanup
  cardano_voter_unref(&voter1);
  cardano_voter_unref(&voter2);
}

TEST(cardano_voter_compare, returnsPositiveIfFirstvoterTypeIsGreaterThanSecond)
{
  // Arrange
  cardano_voter_t* voter1 = new_default_voter2();
  cardano_voter_t* voter2 = new_default_voter();

  // Act
  int result = cardano_voter_compare(voter1, voter2);

  // Assert
  EXPECT_GT(result, 0);

  // Cleanup
  cardano_voter_unref(&voter1);
  cardano_voter_unref(&voter2);
}
