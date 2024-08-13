/**
 * \file voting_procedures.cpp
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
#include <cardano/common/anchor.h>
#include <cardano/voting_procedures/voting_procedures.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                        = "a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* KNOWN_VOTER_CBOR            = "8202581c10000000000000000000000000000000000000000000000000000000";
static const char* KNOWN_VOTER_CBOR_2          = "8203581c20000000000000000000000000000000000000000000000000000000";
static const char* VOTER_CBOR                  = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* GOVERNANCE_ACTION_ID_CBOR_1 = "825820100000000000000000000000000000000000000000000000000000000000000003";
static const char* GOVERNANCE_ACTION_ID_CBOR_2 = "825820200000000000000000000000000000000000000000000000000000000000000003";
static const char* GOVERNANCE_ACTION_ID_CBOR_3 = "825820300000000000000000000000000000000000000000000000000000000000000003";
static const char* VOTING_PROCEDURE_CBOR       = "8200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* GOV_ACTION_IDS[]            = { GOVERNANCE_ACTION_ID_CBOR_1, GOVERNANCE_ACTION_ID_CBOR_2, GOVERNANCE_ACTION_ID_CBOR_3 };
static const char* GOV_ACTION_IDS_2[]          = { GOVERNANCE_ACTION_ID_CBOR_1, GOVERNANCE_ACTION_ID_CBOR_3 };
static const char* VOTERS[]                    = { KNOWN_VOTER_CBOR, KNOWN_VOTER_CBOR_2 };

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the voting procedure.
 * @return A new instance of the voting procedure.
 */
static cardano_voting_procedures_t*
new_default_voting_procedures()
{
  cardano_voting_procedures_t* voting_procedures = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t              result            = cardano_voting_procedures_from_cbor(reader, &voting_procedures);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voting_procedures;
};

/**
 * Creates a new default instance of the governance action id.
 * @return A new instance of the governance action id.
 */
static cardano_governance_action_id_t*
new_default_governance_action_id(const char* action_id)
{
  cardano_governance_action_id_t* governance_action_id = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(action_id, strlen(action_id));
  cardano_error_t                 result               = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return governance_action_id;
};

/**
 * Creates a new default instance of the voter.
 * @return A new instance of the voter.
 */
static cardano_voter_t*
new_default_voter(const char* voter_cbor)
{
  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(voter_cbor, strlen(voter_cbor));
  cardano_error_t        result = cardano_voter_from_cbor(reader, &voter);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_voting_procedures_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  // Act
  cardano_voting_procedures_ref(voting_procedures);

  // Assert
  EXPECT_THAT(voting_procedures, testing::Not((cardano_voting_procedures_t*)nullptr));
  EXPECT_EQ(cardano_voting_procedures_refcount(voting_procedures), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_voting_procedures_unref(&voting_procedures);
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voting_procedures_ref(nullptr);
}

TEST(cardano_voting_procedures_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = nullptr;

  // Act
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voting_procedures_unref((cardano_voting_procedures_t**)nullptr);
}

TEST(cardano_voting_procedures_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  // Act
  cardano_voting_procedures_ref(voting_procedures);
  size_t ref_count = cardano_voting_procedures_refcount(voting_procedures);

  cardano_voting_procedures_unref(&voting_procedures);
  size_t updated_ref_count = cardano_voting_procedures_refcount(voting_procedures);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  // Act
  cardano_voting_procedures_ref(voting_procedures);
  size_t ref_count = cardano_voting_procedures_refcount(voting_procedures);

  cardano_voting_procedures_unref(&voting_procedures);
  size_t updated_ref_count = cardano_voting_procedures_refcount(voting_procedures);

  cardano_voting_procedures_unref(&voting_procedures);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(voting_procedures, (cardano_voting_procedures_t*)nullptr);

  // Cleanup
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_voting_procedures_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_voting_procedures_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = nullptr;
  const char*                  message           = "This is a test message";

  // Act
  cardano_voting_procedures_set_last_error(voting_procedures, message);

  // Assert
  EXPECT_STREQ(cardano_voting_procedures_get_last_error(voting_procedures), "Object is NULL.");
}

TEST(cardano_voting_procedures_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  const char* message = nullptr;

  // Act
  cardano_voting_procedures_set_last_error(voting_procedures, message);

  // Assert
  EXPECT_STREQ(cardano_voting_procedures_get_last_error(voting_procedures), "");

  // Cleanup
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = NULL;

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(nullptr, &voting_procedures);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*       writer = cardano_cbor_writer_new();
  cardano_voting_procedures_t* cert   = new_default_voting_procedures();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_voting_procedures_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_voting_procedures_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_voting_procedures_to_cbor, returnsErrorIfProcedureIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_voting_procedures_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_voting_procedures_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedures_to_cbor((cardano_voting_procedures_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_voting_procedures_new, canCreateNewInstance)
{
  // Act
  cardano_voting_procedures_t* voting_procedures = NULL;

  cardano_error_t result = cardano_voting_procedures_new(&voting_procedures);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(voting_procedures, nullptr);

  // Cleanup
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedures_new(nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_voting_procedures_t* voting_procedures = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_voting_procedures_new(&voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voting_procedures_new, returnsErrorIfMemoryAllocationFails2)
{
  // Act
  cardano_voting_procedures_t* voting_procedures = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_error_t result = cardano_voting_procedures_new(&voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorIfVotingProcedureIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(reader, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorWhenMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_voting_procedures_t* voting_procedures = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(reader, &voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorWhenMemoryAllocationFails2)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_voting_procedures_t* voting_procedures = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_five_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(reader, &voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorIfInvalidVoter)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("a2ef02581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000", strlen("a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000"));

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(reader, &voting_procedures);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_voting_procedures_unref(&voting_procedures);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorIfInvalidNestedMap)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("a28202581c10000000000000000000000000000000000000000000000000000000ef8258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000", strlen("a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000"));
  cardano_voting_procedures_t* voting_procedures = NULL;

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(reader, &voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorIfInvalidGovernanceId)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("a28202581c10000000000000000000000000000000000000000000000000000000a3ef58201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000", strlen("a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000"));
  cardano_voting_procedures_t* voting_procedures = NULL;

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(reader, &voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedures_from_cbor, returnsErrorIfInvalidVotingProcedure)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f5ef000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000", strlen("a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000"));
  cardano_voting_procedures_t* voting_procedures = NULL;

  // Act
  cardano_error_t result = cardano_voting_procedures_from_cbor(reader, &voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_voting_procedures_insert, returnsErrorIfVotingProceduresIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedures_insert(nullptr, nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_insert, returnsErrorIfVoterIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedures_insert((cardano_voting_procedures_t*)"", nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_insert, returnsErrorIfGovActionIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedures_insert((cardano_voting_procedures_t*)"", (cardano_voter_t*)"", nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_insert, returnsErrorIfVotingProcedureIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedures_insert((cardano_voting_procedures_t*)"", (cardano_voter_t*)"", (cardano_governance_action_id_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_get, returnsErrorIfVotingProceduresIsNull)
{
  // Act
  cardano_voting_procedure_t* result = cardano_voting_procedures_get(nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);
}

TEST(cardano_voting_procedures_get, returnsErrorIfVoterIsNull)
{
  // Act
  cardano_voting_procedure_t* result = cardano_voting_procedures_get((cardano_voting_procedures_t*)"", nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);
}

TEST(cardano_voting_procedures_get, returnsErrorIfGovActionIsNull)
{
  // Act
  cardano_voting_procedure_t* result = cardano_voting_procedures_get((cardano_voting_procedures_t*)"", (cardano_voter_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);
}

TEST(cardano_voting_procedures_get, returnsErrorIfVotingProcedureDoesntExist)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  cardano_voter_t*                voter      = new_default_voter(VOTER_CBOR);
  cardano_governance_action_id_t* gov_action = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);

  // Act
  cardano_voting_procedure_t* result = cardano_voting_procedures_get(voting_procedures, voter, gov_action);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&gov_action);
  cardano_voting_procedure_unref(&result);
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_get, returnsAVotingProcedureWhenFound)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  cardano_voter_t*                voter      = new_default_voter(KNOWN_VOTER_CBOR);
  cardano_governance_action_id_t* gov_action = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);

  // Act
  cardano_voting_procedure_t* result = cardano_voting_procedures_get(voting_procedures, voter, gov_action);

  // Assert
  EXPECT_NE(result, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t cbor_result = cardano_voting_procedure_to_cbor(result, writer);

  EXPECT_EQ(cbor_result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, VOTING_PROCEDURE_CBOR);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&gov_action);
  cardano_voting_procedure_unref(&result);
  cardano_voting_procedures_unref(&voting_procedures);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_voting_procedures_get_governance_ids_by_voter, returnsIds)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  cardano_voter_t* voter = new_default_voter(KNOWN_VOTER_CBOR);
  EXPECT_NE(voter, nullptr);

  // Act
  cardano_governance_action_id_list_t* ids = NULL;

  cardano_error_t get_ids_result = cardano_voting_procedures_get_governance_ids_by_voter(voting_procedures, voter, &ids);

  EXPECT_EQ(get_ids_result, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_governance_action_id_list_get_length(ids), 3);

  for (size_t i = 0; i < cardano_governance_action_id_list_get_length(ids); ++i)
  {
    cardano_governance_action_id_t* id = NULL;
    EXPECT_EQ(cardano_governance_action_id_list_get(ids, i, &id), CARDANO_SUCCESS);

    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    cardano_error_t cbor_result = cardano_governance_action_id_to_cbor(id, writer);

    EXPECT_EQ(cbor_result, CARDANO_SUCCESS);

    size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    char*  hex      = (char*)malloc(hex_size);

    ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

    EXPECT_STREQ(hex, GOV_ACTION_IDS[i]);

    cardano_governance_action_id_unref(&id);
    cardano_cbor_writer_unref(&writer);
    free(hex);
  }

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_voting_procedures_unref(&voting_procedures);
  cardano_governance_action_id_list_unref(&ids);
}

TEST(cardano_voting_procedures_get_governance_ids_by_voter, returnsIds2)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  cardano_voter_t* voter = new_default_voter(KNOWN_VOTER_CBOR_2);
  EXPECT_NE(voter, nullptr);

  // Act
  cardano_governance_action_id_list_t* ids = NULL;

  cardano_error_t get_ids_result = cardano_voting_procedures_get_governance_ids_by_voter(voting_procedures, voter, &ids);

  EXPECT_EQ(get_ids_result, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_governance_action_id_list_get_length(ids), 2);

  for (size_t i = 0; i < cardano_governance_action_id_list_get_length(ids); ++i)
  {
    cardano_governance_action_id_t* id = NULL;
    EXPECT_EQ(cardano_governance_action_id_list_get(ids, i, &id), CARDANO_SUCCESS);

    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    cardano_error_t cbor_result = cardano_governance_action_id_to_cbor(id, writer);

    EXPECT_EQ(cbor_result, CARDANO_SUCCESS);

    size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    char*  hex      = (char*)malloc(hex_size);

    ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

    EXPECT_STREQ(hex, GOV_ACTION_IDS_2[i]);

    cardano_governance_action_id_unref(&id);
    cardano_cbor_writer_unref(&writer);
    free(hex);
  }

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_voting_procedures_unref(&voting_procedures);
  cardano_governance_action_id_list_unref(&ids);
}

TEST(cardano_voting_procedures_get_governance_ids_by_voter, returnsErrorIfVotingProceduresIsNull)
{
  // Act
  cardano_governance_action_id_list_t* ids = NULL;

  cardano_error_t get_ids_result = cardano_voting_procedures_get_governance_ids_by_voter(nullptr, nullptr, &ids);

  // Assert
  EXPECT_EQ(get_ids_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_get_governance_ids_by_voter, returnsErrorIfVoterIsNull)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  // Act
  cardano_governance_action_id_list_t* ids = NULL;

  cardano_error_t get_ids_result = cardano_voting_procedures_get_governance_ids_by_voter(voting_procedures, nullptr, &ids);

  // Assert
  EXPECT_EQ(get_ids_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_get_governance_ids_by_voter, returnsErrorIfIdsIsNull)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  cardano_voter_t* voter = new_default_voter(KNOWN_VOTER_CBOR);
  EXPECT_NE(voter, nullptr);

  // Act
  cardano_error_t get_ids_result = cardano_voting_procedures_get_governance_ids_by_voter(voting_procedures, voter, nullptr);

  // Assert
  EXPECT_EQ(get_ids_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_voter_unref(&voter);
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_get_voters, returnsVoters)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  // Act
  cardano_voter_list_t* voters = NULL;

  cardano_error_t get_voters_result = cardano_voting_procedures_get_voters(voting_procedures, &voters);

  EXPECT_EQ(get_voters_result, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_voter_list_get_length(voters), 2);

  for (size_t i = 0; i < cardano_voter_list_get_length(voters); ++i)
  {
    cardano_voter_t* voter = NULL;
    EXPECT_EQ(cardano_voter_list_get(voters, i, &voter), CARDANO_SUCCESS);

    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    cardano_error_t cbor_result = cardano_voter_to_cbor(voter, writer);

    EXPECT_EQ(cbor_result, CARDANO_SUCCESS);

    size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    char*  hex      = (char*)malloc(hex_size);

    ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

    EXPECT_STREQ(hex, VOTERS[i]);

    cardano_voter_unref(&voter);
    cardano_cbor_writer_unref(&writer);
    free(hex);
  }

  // Cleanup
  cardano_voter_list_unref(&voters);
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_voting_procedures_get_voters, returnsErrorIfVotingProceduresIsNull)
{
  // Act
  cardano_voter_list_t* voters = NULL;

  cardano_error_t get_voters_result = cardano_voting_procedures_get_voters(nullptr, &voters);

  // Assert
  EXPECT_EQ(get_voters_result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_voting_procedures_get_voters, returnsErrorIfVotersIsNull)
{
  // Arrange
  cardano_voting_procedures_t* voting_procedures = new_default_voting_procedures();
  EXPECT_NE(voting_procedures, nullptr);

  // Act
  cardano_error_t get_voters_result = cardano_voting_procedures_get_voters(voting_procedures, nullptr);

  // Assert
  EXPECT_EQ(get_voters_result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedures_unref(&voting_procedures);
}