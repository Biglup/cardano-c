/**
 * \file treasury_withdrawals_action.cpp
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                     = "8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";
static const char* CBOR_WITHOUT_POLICY_HASH = "8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01f6";
static const char* POLICY_HASH              = "8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";
static const char* WITHDRAWAL_CBOR          = "a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the treasury_withdrawals_action.
 * @return A new instance of the treasury_withdrawals_action.
 */
static cardano_treasury_withdrawals_action_t*
new_default_treasury_withdrawals_action()
{
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                        result                      = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return treasury_withdrawals_action;
};

/**
 * Creates a new default instance of the hash.
 * @return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
new_default_hash(const char* hash)
{
  cardano_blake2b_hash_t* hash_instance = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(hash, strlen(hash), &hash_instance);

  EXPECT_THAT(error, CARDANO_SUCCESS);

  return hash_instance;
}

/**
 * Creates a new default instance of the withdrawal map.
 * @return A new instance of the withdrawal map.
 */
static cardano_withdrawal_map_t*
new_default_withdrawal_map(const char* cbor)
{
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, &withdrawal_map);

  EXPECT_THAT(error, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return withdrawal_map;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_treasury_withdrawals_action_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  EXPECT_NE(treasury_withdrawals_action, nullptr);

  // Act
  cardano_treasury_withdrawals_action_ref(treasury_withdrawals_action);

  // Assert
  EXPECT_THAT(treasury_withdrawals_action, testing::Not((cardano_treasury_withdrawals_action_t*)nullptr));
  EXPECT_EQ(cardano_treasury_withdrawals_action_refcount(treasury_withdrawals_action), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
}

TEST(cardano_treasury_withdrawals_action_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_treasury_withdrawals_action_ref(nullptr);
}

TEST(cardano_treasury_withdrawals_action_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = nullptr;

  // Act
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
}

TEST(cardano_treasury_withdrawals_action_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_treasury_withdrawals_action_unref((cardano_treasury_withdrawals_action_t**)nullptr);
}

TEST(cardano_treasury_withdrawals_action_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  EXPECT_NE(treasury_withdrawals_action, nullptr);

  // Act
  cardano_treasury_withdrawals_action_ref(treasury_withdrawals_action);
  size_t ref_count = cardano_treasury_withdrawals_action_refcount(treasury_withdrawals_action);

  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  size_t updated_ref_count = cardano_treasury_withdrawals_action_refcount(treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
}

TEST(cardano_treasury_withdrawals_action_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  EXPECT_NE(treasury_withdrawals_action, nullptr);

  // Act
  cardano_treasury_withdrawals_action_ref(treasury_withdrawals_action);
  size_t ref_count = cardano_treasury_withdrawals_action_refcount(treasury_withdrawals_action);

  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  size_t updated_ref_count = cardano_treasury_withdrawals_action_refcount(treasury_withdrawals_action);

  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(treasury_withdrawals_action, (cardano_treasury_withdrawals_action_t*)nullptr);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
}

TEST(cardano_treasury_withdrawals_action_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_treasury_withdrawals_action_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_treasury_withdrawals_action_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = nullptr;
  const char*                            message                     = "This is a test message";

  // Act
  cardano_treasury_withdrawals_action_set_last_error(treasury_withdrawals_action, message);

  // Assert
  EXPECT_STREQ(cardano_treasury_withdrawals_action_get_last_error(treasury_withdrawals_action), "Object is NULL.");
}

TEST(cardano_treasury_withdrawals_action_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  EXPECT_NE(treasury_withdrawals_action, nullptr);

  const char* message = nullptr;

  // Act
  cardano_treasury_withdrawals_action_set_last_error(treasury_withdrawals_action, message);

  // Assert
  EXPECT_STREQ(cardano_treasury_withdrawals_action_get_last_error(treasury_withdrawals_action), "");

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(nullptr, &treasury_withdrawals_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_treasury_withdrawals_action_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*                 writer = cardano_cbor_writer_new();
  cardano_treasury_withdrawals_action_t* cert   = new_default_treasury_withdrawals_action();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_treasury_withdrawals_action_to_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_treasury_withdrawals_action_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_to_cbor((cardano_treasury_withdrawals_action_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Action specific tests

TEST(cardano_treasury_withdrawals_action_new, canCreateNewInstanceWithoutPolicyHash)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = new_default_withdrawal_map(WITHDRAWAL_CBOR);

  // Act
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  cardano_error_t result = cardano_treasury_withdrawals_action_new(withdrawal_map, nullptr, &treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(treasury_withdrawals_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_treasury_withdrawals_action_to_cbor(treasury_withdrawals_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_POLICY_HASH);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_treasury_withdrawals_action_new, canCreateNewInstanceWithPolicyHash)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = new_default_withdrawal_map(WITHDRAWAL_CBOR);
  cardano_blake2b_hash_t*   policy_hash    = new_default_hash(POLICY_HASH);

  // Act
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  cardano_error_t result = cardano_treasury_withdrawals_action_new(withdrawal_map, policy_hash, &treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(treasury_withdrawals_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_treasury_withdrawals_action_to_cbor(treasury_withdrawals_action, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

  char* hex = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_treasury_withdrawals_action_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  cardano_error_t result = cardano_treasury_withdrawals_action_new(nullptr, nullptr, &treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_treasury_withdrawals_action_new, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = new_default_withdrawal_map(WITHDRAWAL_CBOR);

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_new(withdrawal_map, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_treasury_withdrawals_action_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_withdrawal_map_t* withdrawal_map = new_default_withdrawal_map(WITHDRAWAL_CBOR);

  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_treasury_withdrawals_action_new(withdrawal_map, nullptr, &treasury_withdrawals_action);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, returnsErrorIfInvalidId)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("83effe820103", strlen("8301fe820103"));
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, returnsErrorIfInvalidWithdrawal)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("8302ef820103", strlen("8301ef820103"));
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, returnsErrorIfInvalidPolicyHash)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex("8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581cef", strlen("8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581cf6"));
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_treasury_withdrawals_action_from_cbor, canDeserializePolicyHash)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex(CBOR_WITHOUT_POLICY_HASH, strlen(CBOR_WITHOUT_POLICY_HASH));
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(treasury_withdrawals_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_treasury_withdrawals_action_to_cbor(treasury_withdrawals_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_POLICY_HASH);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

// Getters and Setters

TEST(cardano_treasury_withdrawals_action_set_withdrawals, canSetWithdrawals)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  cardano_withdrawal_map_t*              withdrawal_map              = new_default_withdrawal_map(WITHDRAWAL_CBOR);

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_set_withdrawals(treasury_withdrawals_action, withdrawal_map);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_treasury_withdrawals_action_set_withdrawals, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = new_default_withdrawal_map(WITHDRAWAL_CBOR);

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_set_withdrawals(nullptr, withdrawal_map);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_treasury_withdrawals_action_set_withdrawals, returnsErrorIfWithdrawalsIsNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_set_withdrawals(treasury_withdrawals_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
}

TEST(cardano_treasury_withdrawals_action_get_withdrawals, canGetWithdrawals)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  cardano_withdrawal_map_t*              withdrawal_map              = new_default_withdrawal_map(WITHDRAWAL_CBOR);

  EXPECT_EQ(cardano_treasury_withdrawals_action_set_withdrawals(treasury_withdrawals_action, withdrawal_map), CARDANO_SUCCESS);

  // Act
  cardano_withdrawal_map_t* withdrawal_map_out = cardano_treasury_withdrawals_action_get_withdrawals(treasury_withdrawals_action);

  // Assert
  EXPECT_NE(withdrawal_map_out, nullptr);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_withdrawal_map_unref(&withdrawal_map_out);
}

TEST(cardano_treasury_withdrawals_action_get_withdrawals, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_withdrawal_map_t* withdrawal_map = cardano_treasury_withdrawals_action_get_withdrawals(nullptr);

  // Assert
  EXPECT_EQ(withdrawal_map, nullptr);
}

TEST(cardano_treasury_withdrawals_action_set_policy_hash, canSetPolicyHash)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  cardano_blake2b_hash_t*                policy_hash                 = new_default_hash(POLICY_HASH);

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_set_policy_hash(treasury_withdrawals_action, policy_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_treasury_withdrawals_action_set_policy_hash, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* policy_hash = new_default_hash(POLICY_HASH);

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_set_policy_hash(nullptr, policy_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_treasury_withdrawals_action_set_policy_hash, canSetPolicyHashToNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();

  // Act
  cardano_error_t result = cardano_treasury_withdrawals_action_set_policy_hash(treasury_withdrawals_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
}

TEST(cardano_treasury_withdrawals_action_get_policy_hash, canGetPolicyHash)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = new_default_treasury_withdrawals_action();
  cardano_blake2b_hash_t*                policy_hash                 = new_default_hash(POLICY_HASH);

  EXPECT_EQ(cardano_treasury_withdrawals_action_set_policy_hash(treasury_withdrawals_action, policy_hash), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* policy_hash_out = cardano_treasury_withdrawals_action_get_policy_hash(treasury_withdrawals_action);

  // Assert
  EXPECT_NE(policy_hash_out, nullptr);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_blake2b_hash_unref(&policy_hash_out);
}

TEST(cardano_treasury_withdrawals_action_get_policy_hash, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_blake2b_hash_t* policy_hash = cardano_treasury_withdrawals_action_get_policy_hash(nullptr);

  // Assert
  EXPECT_EQ(policy_hash, nullptr);
}
