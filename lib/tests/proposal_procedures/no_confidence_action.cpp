/**
 * \file no_confidence_action.cpp
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
#include <cardano/proposal_procedures/no_confidence_action.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                    = "8203825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* CBOR_WITHOUT_GOV_ACTION = "8203f6";
static const char* GOV_ACTION_CBOR         = "825820000000000000000000000000000000000000000000000000000000000000000003";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the no_confidence_action.
 * @return A new instance of the no_confidence_action.
 */
static cardano_no_confidence_action_t*
new_default_no_confidence_action()
{
  cardano_no_confidence_action_t* no_confidence_action = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                 result               = cardano_no_confidence_action_from_cbor(reader, &no_confidence_action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return no_confidence_action;
};

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

TEST(cardano_no_confidence_action_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = new_default_no_confidence_action();
  EXPECT_NE(no_confidence_action, nullptr);

  // Act
  cardano_no_confidence_action_ref(no_confidence_action);

  // Assert
  EXPECT_THAT(no_confidence_action, testing::Not((cardano_no_confidence_action_t*)nullptr));
  EXPECT_EQ(cardano_no_confidence_action_refcount(no_confidence_action), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_no_confidence_action_unref(&no_confidence_action);
}

TEST(cardano_no_confidence_action_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_no_confidence_action_ref(nullptr);
}

TEST(cardano_no_confidence_action_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = nullptr;

  // Act
  cardano_no_confidence_action_unref(&no_confidence_action);
}

TEST(cardano_no_confidence_action_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_no_confidence_action_unref((cardano_no_confidence_action_t**)nullptr);
}

TEST(cardano_no_confidence_action_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = new_default_no_confidence_action();
  EXPECT_NE(no_confidence_action, nullptr);

  // Act
  cardano_no_confidence_action_ref(no_confidence_action);
  size_t ref_count = cardano_no_confidence_action_refcount(no_confidence_action);

  cardano_no_confidence_action_unref(&no_confidence_action);
  size_t updated_ref_count = cardano_no_confidence_action_refcount(no_confidence_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
}

TEST(cardano_no_confidence_action_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = new_default_no_confidence_action();
  EXPECT_NE(no_confidence_action, nullptr);

  // Act
  cardano_no_confidence_action_ref(no_confidence_action);
  size_t ref_count = cardano_no_confidence_action_refcount(no_confidence_action);

  cardano_no_confidence_action_unref(&no_confidence_action);
  size_t updated_ref_count = cardano_no_confidence_action_refcount(no_confidence_action);

  cardano_no_confidence_action_unref(&no_confidence_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(no_confidence_action, (cardano_no_confidence_action_t*)nullptr);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
}

TEST(cardano_no_confidence_action_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_no_confidence_action_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_no_confidence_action_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = nullptr;
  const char*                     message              = "This is a test message";

  // Act
  cardano_no_confidence_action_set_last_error(no_confidence_action, message);

  // Assert
  EXPECT_STREQ(cardano_no_confidence_action_get_last_error(no_confidence_action), "Object is NULL.");
}

TEST(cardano_no_confidence_action_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = new_default_no_confidence_action();
  EXPECT_NE(no_confidence_action, nullptr);

  const char* message = nullptr;

  // Act
  cardano_no_confidence_action_set_last_error(no_confidence_action, message);

  // Assert
  EXPECT_STREQ(cardano_no_confidence_action_get_last_error(no_confidence_action), "");

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
}

TEST(cardano_no_confidence_action_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = NULL;

  // Act
  cardano_error_t result = cardano_no_confidence_action_from_cbor(nullptr, &no_confidence_action);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_no_confidence_action_from_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_no_confidence_action_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_no_confidence_action_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*          writer = cardano_cbor_writer_new();
  cardano_no_confidence_action_t* cert   = new_default_no_confidence_action();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_no_confidence_action_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_no_confidence_action_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_no_confidence_action_to_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_no_confidence_action_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_no_confidence_action_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_no_confidence_action_to_cbor((cardano_no_confidence_action_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Action specific tests

TEST(cardano_no_confidence_action_new, canCreateNewInstanceWithoutGovAction)
{
  // Act
  cardano_no_confidence_action_t* no_confidence_action = NULL;

  cardano_error_t result = cardano_no_confidence_action_new(nullptr, &no_confidence_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(no_confidence_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_no_confidence_action_to_cbor(no_confidence_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_no_confidence_action_new, canCreateNewInstanceWithGovAction)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_no_confidence_action_t* no_confidence_action = NULL;

  cardano_error_t result = cardano_no_confidence_action_new(governance_action_id, &no_confidence_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(no_confidence_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_no_confidence_action_to_cbor(no_confidence_action, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

  char* hex = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_no_confidence_action_new, returnsErrorIfActionIsNull)
{
  // Act
  cardano_error_t result = cardano_no_confidence_action_new(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_no_confidence_action_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_no_confidence_action_t* no_confidence_action = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_no_confidence_action_new(nullptr, &no_confidence_action);

  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_no_confidence_action_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_no_confidence_action_from_cbor(reader, &no_confidence_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_no_confidence_action_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_no_confidence_action_from_cbor(reader, &no_confidence_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_no_confidence_action_from_cbor, returnsErrorIfInvalidId)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("82effe820103", strlen("8201fe820103"));
  cardano_no_confidence_action_t* no_confidence_action = NULL;

  // Act
  cardano_error_t result = cardano_no_confidence_action_from_cbor(reader, &no_confidence_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_no_confidence_action_from_cbor, returnsErrorIfInvalidGovAction)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("8203ef820103", strlen("8203ef820103"));
  cardano_no_confidence_action_t* no_confidence_action = NULL;

  // Act
  cardano_error_t result = cardano_no_confidence_action_from_cbor(reader, &no_confidence_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_no_confidence_action_from_cbor, canDeserializeWithoutGovId)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR_WITHOUT_GOV_ACTION, strlen(CBOR_WITHOUT_GOV_ACTION));
  cardano_no_confidence_action_t* no_confidence_action = NULL;

  // Act
  cardano_error_t result = cardano_no_confidence_action_from_cbor(reader, &no_confidence_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(no_confidence_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_no_confidence_action_to_cbor(no_confidence_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

// Getters and Setters

TEST(cardano_no_confidence_action_set_governance_action_id, canSetGovernanceActionId)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = new_default_no_confidence_action();
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_no_confidence_action_set_governance_action_id(no_confidence_action, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_no_confidence_action_set_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_no_confidence_action_set_governance_action_id(nullptr, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_no_confidence_action_set_governance_action_id, canSetGovActionToNull)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = new_default_no_confidence_action();

  // Act
  cardano_error_t result = cardano_no_confidence_action_set_governance_action_id(no_confidence_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
}

TEST(cardano_no_confidence_action_get_governance_action_id, canGetGovernanceActionId)
{
  // Arrange
  cardano_no_confidence_action_t* no_confidence_action = new_default_no_confidence_action();
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  EXPECT_EQ(cardano_no_confidence_action_set_governance_action_id(no_confidence_action, governance_action_id), CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_t* governance_action_id_out = cardano_no_confidence_action_get_governance_action_id(no_confidence_action);

  // Assert
  EXPECT_NE(governance_action_id_out, nullptr);

  // Cleanup
  cardano_no_confidence_action_unref(&no_confidence_action);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_governance_action_id_unref(&governance_action_id_out);
}

TEST(cardano_no_confidence_action_get_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_governance_action_id_t* governance_action_id = cardano_no_confidence_action_get_governance_action_id(nullptr);

  // Assert
  EXPECT_EQ(governance_action_id, nullptr);
}
