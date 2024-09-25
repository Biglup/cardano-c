/**
 * \file new_constitution_action.cpp
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
#include <cardano/proposal_procedures/new_constitution_action.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                    = "830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_WITHOUT_GOV_ACTION = "8305f682827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";
static const char* GOV_ACTION_CBOR         = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* CONSTITUTION_CBOR       = "82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the new_constitution_action.
 * @return A new instance of the new_constitution_action.
 */
static cardano_new_constitution_action_t*
new_default_new_constitution_action()
{
  cardano_new_constitution_action_t* new_constitution_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                    result                  = cardano_new_constitution_action_from_cbor(reader, &new_constitution_action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return new_constitution_action;
};

/**
 * Creates a new default instance of the constitution.
 * @return A new instance of the constitution.
 */
static cardano_constitution_t*
new_default_constitution(const char* cbor)
{
  cardano_constitution_t* constitution = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_constitution_from_cbor(reader, &constitution);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return constitution;
}

/**
 * Creates a new default instance of the governance action id.
 * @return A new instance of the governance action id.
 */
static cardano_governance_action_id_t*
new_default_governance_action_id(const char* cbor)
{
  cardano_governance_action_id_t* governance_action_id = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return governance_action_id;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_new_constitution_action_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  EXPECT_NE(new_constitution_action, nullptr);

  // Act
  cardano_new_constitution_action_ref(new_constitution_action);

  // Assert
  EXPECT_THAT(new_constitution_action, testing::Not((cardano_new_constitution_action_t*)nullptr));
  EXPECT_EQ(cardano_new_constitution_action_refcount(new_constitution_action), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_new_constitution_action_unref(&new_constitution_action);
}

TEST(cardano_new_constitution_action_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_new_constitution_action_ref(nullptr);
}

TEST(cardano_new_constitution_action_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = nullptr;

  // Act
  cardano_new_constitution_action_unref(&new_constitution_action);
}

TEST(cardano_new_constitution_action_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_new_constitution_action_unref((cardano_new_constitution_action_t**)nullptr);
}

TEST(cardano_new_constitution_action_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  EXPECT_NE(new_constitution_action, nullptr);

  // Act
  cardano_new_constitution_action_ref(new_constitution_action);
  size_t ref_count = cardano_new_constitution_action_refcount(new_constitution_action);

  cardano_new_constitution_action_unref(&new_constitution_action);
  size_t updated_ref_count = cardano_new_constitution_action_refcount(new_constitution_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
}

TEST(cardano_new_constitution_action_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  EXPECT_NE(new_constitution_action, nullptr);

  // Act
  cardano_new_constitution_action_ref(new_constitution_action);
  size_t ref_count = cardano_new_constitution_action_refcount(new_constitution_action);

  cardano_new_constitution_action_unref(&new_constitution_action);
  size_t updated_ref_count = cardano_new_constitution_action_refcount(new_constitution_action);

  cardano_new_constitution_action_unref(&new_constitution_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(new_constitution_action, (cardano_new_constitution_action_t*)nullptr);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
}

TEST(cardano_new_constitution_action_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_new_constitution_action_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_new_constitution_action_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = nullptr;
  const char*                        message                 = "This is a test message";

  // Act
  cardano_new_constitution_action_set_last_error(new_constitution_action, message);

  // Assert
  EXPECT_STREQ(cardano_new_constitution_action_get_last_error(new_constitution_action), "Object is NULL.");
}

TEST(cardano_new_constitution_action_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  EXPECT_NE(new_constitution_action, nullptr);

  const char* message = nullptr;

  // Act
  cardano_new_constitution_action_set_last_error(new_constitution_action, message);

  // Assert
  EXPECT_STREQ(cardano_new_constitution_action_get_last_error(new_constitution_action), "");

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
}

TEST(cardano_new_constitution_action_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(nullptr, &new_constitution_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_new_constitution_action_from_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_new_constitution_action_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*             writer = cardano_cbor_writer_new();
  cardano_new_constitution_action_t* cert   = new_default_new_constitution_action();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_new_constitution_action_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_new_constitution_action_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_new_constitution_action_to_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_new_constitution_action_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_new_constitution_action_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_new_constitution_action_to_cbor((cardano_new_constitution_action_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Action specific tests

TEST(cardano_new_constitution_action_new, canCreateNewInstanceWithoutGovAction)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  cardano_error_t result = cardano_new_constitution_action_new(constitution, nullptr, &new_constitution_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(new_constitution_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_new_constitution_action_to_cbor(new_constitution_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_constitution_unref(&constitution);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_new_constitution_action_new, canCreateNewInstanceWithGovAction)
{
  // Arrange
  cardano_constitution_t*         constitution         = new_default_constitution(CONSTITUTION_CBOR);
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  cardano_error_t result = cardano_new_constitution_action_new(constitution, governance_action_id, &new_constitution_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(new_constitution_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_new_constitution_action_to_cbor(new_constitution_action, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

  char* hex = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_constitution_unref(&constitution);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_new_constitution_action_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  cardano_error_t result = cardano_new_constitution_action_new(nullptr, nullptr, &new_constitution_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_new_constitution_action_new, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_error_t result = cardano_new_constitution_action_new(constitution, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_constitution_unref(&constitution);
}

TEST(cardano_new_constitution_action_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_constitution_t* constitution = new_default_constitution(CONSTITUTION_CBOR);

  cardano_new_constitution_action_t* new_constitution_action = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_new_constitution_action_new(constitution, nullptr, &new_constitution_action);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_constitution_unref(&constitution);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_new_constitution_action_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(reader, &new_constitution_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_new_constitution_action_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(reader, &new_constitution_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_new_constitution_action_from_cbor, returnsErrorIfInvalidId)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("83effe820103", strlen("8301fe820103"));
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(reader, &new_constitution_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_new_constitution_action_from_cbor, returnsErrorIfInvalidGovAction)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8305ef820103", strlen("8301ef820103"));
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(reader, &new_constitution_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_new_constitution_action_from_cbor, returnsErrorIfInvalidConstitution)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8305f6ef0103", strlen("8301f6820103"));
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(reader, &new_constitution_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_new_constitution_action_from_cbor, canDeserializeWithoutGovId)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_WITHOUT_GOV_ACTION, strlen(CBOR_WITHOUT_GOV_ACTION));
  cardano_new_constitution_action_t* new_constitution_action = NULL;

  // Act
  cardano_error_t result = cardano_new_constitution_action_from_cbor(reader, &new_constitution_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(new_constitution_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_new_constitution_action_to_cbor(new_constitution_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

// Getters and Setters

TEST(cardano_new_constitution_action_set_constitution, canSetConstitution)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  cardano_constitution_t*            constitution            = new_default_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_error_t result = cardano_new_constitution_action_set_constitution(new_constitution_action, constitution);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_new_constitution_action_set_constitution, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_error_t result = cardano_new_constitution_action_set_constitution(nullptr, constitution);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_constitution_unref(&constitution);
}

TEST(cardano_new_constitution_action_set_constitution, returnsErrorIfConstitutionIsNull)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();

  // Act
  cardano_error_t result = cardano_new_constitution_action_set_constitution(new_constitution_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
}

TEST(cardano_new_constitution_action_get_constitution, canGetConstitution)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  cardano_constitution_t*            constitution            = new_default_constitution(CONSTITUTION_CBOR);

  EXPECT_EQ(cardano_new_constitution_action_set_constitution(new_constitution_action, constitution), CARDANO_SUCCESS);

  // Act
  cardano_constitution_t* constitution_out = cardano_new_constitution_action_get_constitution(new_constitution_action);

  // Assert
  EXPECT_NE(constitution_out, nullptr);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_constitution_unref(&constitution);
  cardano_constitution_unref(&constitution_out);
}

TEST(cardano_new_constitution_action_get_constitution, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_constitution_t* constitution = cardano_new_constitution_action_get_constitution(nullptr);

  // Assert
  EXPECT_EQ(constitution, nullptr);
}

TEST(cardano_new_constitution_action_set_governance_action_id, canSetGovernanceActionId)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  cardano_governance_action_id_t*    governance_action_id    = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_new_constitution_action_set_governance_action_id(new_constitution_action, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_new_constitution_action_set_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_new_constitution_action_set_governance_action_id(nullptr, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_new_constitution_action_set_governance_action_id, canSetGovActionToNull)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();

  // Act
  cardano_error_t result = cardano_new_constitution_action_set_governance_action_id(new_constitution_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
}

TEST(cardano_new_constitution_action_get_governance_action_id, canGetGovernanceActionId)
{
  // Arrange
  cardano_new_constitution_action_t* new_constitution_action = new_default_new_constitution_action();
  cardano_governance_action_id_t*    governance_action_id    = new_default_governance_action_id(GOV_ACTION_CBOR);

  EXPECT_EQ(cardano_new_constitution_action_set_governance_action_id(new_constitution_action, governance_action_id), CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_t* governance_action_id_out = cardano_new_constitution_action_get_governance_action_id(new_constitution_action);

  // Assert
  EXPECT_NE(governance_action_id_out, nullptr);

  // Cleanup
  cardano_new_constitution_action_unref(&new_constitution_action);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_governance_action_id_unref(&governance_action_id_out);
}

TEST(cardano_new_constitution_action_get_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_governance_action_id_t* governance_action_id = cardano_new_constitution_action_get_governance_action_id(nullptr);

  // Assert
  EXPECT_EQ(governance_action_id, nullptr);
}
