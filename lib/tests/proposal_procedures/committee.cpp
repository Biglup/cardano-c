/**
 * \file committee.cpp
 *
 * \author angel.castillo
 * \date   Aug 31, 2024
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

#include <cardano/address/reward_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/proposal_procedures/committee.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR             = "82a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003d81e820502";
static const char* CREDENTIAL1_CBOR = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL2_CBOR = "8200581c10000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the committee.
 * @return A new instance of the committee.
 */
static cardano_committee_t*
new_default_committee()
{
  cardano_committee_t*   committee = NULL;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t        result    = cardano_committee_from_cbor(reader, &committee);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return committee;
};

/**
 * Creates a new default instance of the credential.
 * @return A new instance of the credential.
 */
static cardano_credential_t*
new_default_credential(const char* cbor)
{
  cardano_credential_t*  credential = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_credential_from_cbor(reader, &credential);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);
    return nullptr;
  }

  return credential;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_committee_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_committee_t* committee = new_default_committee();
  EXPECT_NE(committee, nullptr);

  // Act
  cardano_committee_ref(committee);

  // Assert
  EXPECT_THAT(committee, testing::Not((cardano_committee_t*)nullptr));
  EXPECT_EQ(cardano_committee_refcount(committee), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_committee_unref(&committee);
  cardano_committee_unref(&committee);
}

TEST(cardano_committee_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_committee_ref(nullptr);
}

TEST(cardano_committee_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_committee_t* committee = nullptr;

  // Act
  cardano_committee_unref(&committee);
}

TEST(cardano_committee_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_committee_unref((cardano_committee_t**)nullptr);
}

TEST(cardano_committee_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_committee_t* committee = new_default_committee();
  EXPECT_NE(committee, nullptr);

  // Act
  cardano_committee_ref(committee);
  size_t ref_count = cardano_committee_refcount(committee);

  cardano_committee_unref(&committee);
  size_t updated_ref_count = cardano_committee_refcount(committee);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_committee_unref(&committee);
}

TEST(cardano_committee_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_committee_t* committee = new_default_committee();
  EXPECT_NE(committee, nullptr);

  // Act
  cardano_committee_ref(committee);
  size_t ref_count = cardano_committee_refcount(committee);

  cardano_committee_unref(&committee);
  size_t updated_ref_count = cardano_committee_refcount(committee);

  cardano_committee_unref(&committee);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(committee, (cardano_committee_t*)nullptr);

  // Cleanup
  cardano_committee_unref(&committee);
}

TEST(cardano_committee_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_committee_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_committee_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_committee_t* committee = nullptr;
  const char*          message   = "This is a test message";

  // Act
  cardano_committee_set_last_error(committee, message);

  // Assert
  EXPECT_STREQ(cardano_committee_get_last_error(committee), "Object is NULL.");
}

TEST(cardano_committee_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_committee_t* committee = new_default_committee();
  EXPECT_NE(committee, nullptr);

  const char* message = nullptr;

  // Act
  cardano_committee_set_last_error(committee, message);

  // Assert
  EXPECT_STREQ(cardano_committee_get_last_error(committee), "");

  // Cleanup
  cardano_committee_unref(&committee);
}

TEST(cardano_committee_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_committee_t* committee = NULL;

  // Act
  cardano_error_t result = cardano_committee_from_cbor(nullptr, &committee);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_from_cbor, returnsErrorIfCommitteeIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_committee_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_committee_t*   cert   = new_default_committee();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_committee_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_committee_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_committee_to_cbor, returnsErrorIfCommitteeIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_committee_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_committee_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_committee_to_cbor((cardano_committee_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Committee specific tests

TEST(cardano_committee_new, canCreateNewInstance)
{
  // Act
  cardano_credential_t* cred = new_default_credential(CREDENTIAL1_CBOR);

  cardano_committee_t*     committee        = NULL;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t result = cardano_committee_new(quorum_threshold, &committee);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(committee, nullptr);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&cred);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_committee_t* committee = NULL;

  cardano_error_t result = cardano_committee_new(nullptr, &committee);
  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_new, returnsErrorIfCommitteeIsNull)
{
  // Act
  cardano_credential_t* cred = new_default_credential(CREDENTIAL1_CBOR);

  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t result = cardano_committee_new(quorum_threshold, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&cred);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_credential_t* cred = new_default_credential(CREDENTIAL1_CBOR);

  cardano_committee_t*     committee        = NULL;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_committee_new(quorum_threshold, &committee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&cred);
  cardano_unit_interval_unref(&quorum_threshold);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_committee_new, returnsErrorIfMemoryAllocationFails2)
{
  // Act
  cardano_credential_t* cred = new_default_credential(CREDENTIAL1_CBOR);

  cardano_committee_t*     committee        = NULL;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_error_t result = cardano_committee_new(quorum_threshold, &committee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&cred);
  cardano_unit_interval_unref(&quorum_threshold);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_committee_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_committee_t*   committee = NULL;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_committee_from_cbor(reader, &committee);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_committee_t*   committee = NULL;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_committee_from_cbor(reader, &committee);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_from_cbor, returnsErrorIfInvalidMap)
{
  // Arrange
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("82ef", strlen("82ef"));
  cardano_committee_t*   committee = NULL;

  // Act
  cardano_error_t result = cardano_committee_from_cbor(reader, &committee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_from_cbor, returnsErrorIfInvalidThreshold)
{
  // Arrange
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("82a0ef", strlen("82a0ef"));
  cardano_committee_t*   committee = NULL;

  // Act
  cardano_error_t result = cardano_committee_from_cbor(reader, &committee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_committee_get_quorum_threshold, canGetQuorumThreshold)
{
  // Arrange
  cardano_committee_t*     committee        = new_default_committee();
  cardano_unit_interval_t* quorum_threshold = cardano_committee_get_quorum_threshold(committee);

  // Assert
  EXPECT_NE(quorum_threshold, nullptr);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_quorum_threshold, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_unit_interval_t* quorum_threshold = cardano_committee_get_quorum_threshold(nullptr);

  // Assert
  EXPECT_EQ(quorum_threshold, nullptr);
}

TEST(cardano_committee_set_quorum_threshold, canSetQuorumThreshold)
{
  // Arrange
  cardano_committee_t*     committee        = new_default_committee();
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_committee_set_quorum_threshold(committee, quorum_threshold);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_set_quorum_threshold, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_unit_interval_t* quorum_threshold = NULL;

  // Act
  cardano_error_t result = cardano_committee_set_quorum_threshold(nullptr, quorum_threshold);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_set_quorum_threshold, returnsErrorIfQuorumThresholdIsNull)
{
  // Arrange
  cardano_committee_t* committee = new_default_committee();

  // Act
  cardano_error_t result = cardano_committee_set_quorum_threshold(committee, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_unref(&committee);
}

TEST(cardano_committee_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* committee = nullptr;

  // Act
  cardano_error_t error = cardano_committee_get_key_at(nullptr, 0, &committee);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_committee_get_key_at((cardano_committee_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  // Act
  error = cardano_committee_get_key_at(committee, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);

  error = cardano_committee_add_member(committee, credential1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_add_member(committee, credential2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_t* credential = nullptr;
  error                            = cardano_committee_get_key_at(committee, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_compare(credential1, credential), 0);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_committee_unref(&committee);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
  cardano_credential_unref(&credential);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t error = cardano_committee_get_value_at(nullptr, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_committee_get_value_at((cardano_committee_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 0;

  // Act
  error = cardano_committee_get_value_at(committee, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 2;

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  error = cardano_committee_add_member(committee, credential, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value_out = 2;
  error              = cardano_committee_get_value_at(committee, 0, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&credential);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  uint64_t              value      = 0;

  // Act
  cardano_error_t error = cardano_committee_get_key_value_at(nullptr, 0, &credential, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_key_value_at, returnsErrorIfHashIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t error = cardano_committee_get_key_value_at((cardano_committee_t*)"", 0, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_key_value_at, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_committee_get_key_value_at((cardano_committee_t*)"", 0, &credential, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;
  uint64_t              value      = 0;

  // Act
  error = cardano_committee_get_key_value_at(committee, 0, &credential, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 10;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  error = cardano_committee_add_member(committee, credential, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_t* credential_out = nullptr;
  uint64_t              value_out      = 0;
  error                                = cardano_committee_get_key_value_at(committee, 0, &credential_out, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_compare(credential, credential_out), 0);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&credential_out);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_keys, returnsNullIfObjectIsNull)
{
  // Assert
  EXPECT_EQ(cardano_committee_members_keys(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_get_keys, returnsNullIfKeysIsNull)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_committee_members_keys(committee, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_keys, returnsEmptyArrayIfNoElements)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_set_t* keys = nullptr;

  // Act
  error = cardano_committee_members_keys(committee, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_set_get_length(keys), 0);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_set_unref(&keys);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_keys, returnsTheKeys)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);

  error = cardano_committee_add_member(committee, credential1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_add_member(committee, credential2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_set_t* keys = nullptr;

  // Act
  error = cardano_committee_members_keys(committee, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_set_get_length(keys), 2);

  cardano_credential_t* key = nullptr;

  error = cardano_credential_set_get(keys, 0, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_credential_compare(credential1, key), 0);

  cardano_credential_unref(&key);

  error = cardano_credential_set_get(keys, 1, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_credential_compare(credential2, key), 0);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
  cardano_credential_set_unref(&keys);
  cardano_credential_unref(&key);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_add_member, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  // Act
  cardano_error_t error = cardano_committee_add_member(nullptr, credential, 1);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_committee_add_member, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_committee_t*  committee  = new_default_committee();
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_committee_add_member(committee, credential, 1);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_unref(&committee);
}

TEST(cardano_committee_get_member_epoch, returnsZeroIfObjectIsNull)
{
  // Arrange
  uint64_t epoch = cardano_committee_get_member_epoch(nullptr, 0);

  // Assert
  EXPECT_EQ(epoch, 0);
}

TEST(cardano_committee_get_member_epoch, returnsZeroIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_committee_t*     committee        = new_default_committee();
  cardano_unit_interval_t* quorum_threshold = cardano_committee_get_quorum_threshold(committee);

  // Act
  uint64_t epoch = cardano_committee_get_member_epoch(committee, 0);

  // Assert
  EXPECT_EQ(epoch, 0);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_member_epoch, returnsTheEpoch)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  error = cardano_committee_add_member(committee, credential, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t epoch = cardano_committee_get_member_epoch(committee, credential);

  // Assert
  EXPECT_EQ(epoch, 1);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&credential);
  cardano_unit_interval_unref(&quorum_threshold);
}

TEST(cardano_committee_get_member_epoch, returnsZeroIfCredentialIsNotInList)
{
  // Arrange
  cardano_committee_t*     committee        = nullptr;
  cardano_unit_interval_t* quorum_threshold = NULL;

  EXPECT_EQ(cardano_unit_interval_new(2U, 5U, &quorum_threshold), CARDANO_SUCCESS);

  cardano_error_t error = cardano_committee_new(quorum_threshold, &committee);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  error = cardano_committee_add_member(committee, credential, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);

  // Act
  uint64_t epoch = cardano_committee_get_member_epoch(committee, credential2);

  // Assert
  EXPECT_EQ(epoch, 0);

  // Cleanup
  cardano_committee_unref(&committee);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&credential2);
  cardano_unit_interval_unref(&quorum_threshold);
}
