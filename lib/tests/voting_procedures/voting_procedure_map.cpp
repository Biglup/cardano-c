/**
 * \file voting_procedure_map.cpp
 *
 * \author angel.castillo
 * \date   May 14, 2024
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

#include <cardano/common/governance_action_id.h>
#include <cardano/voting_procedures/governance_action_id_list.h>
#include <cardano/voting_procedures/voting_procedure.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"
#include "../src/voting_procedures/voting_procedure_list.h"
#include "../src/voting_procedures/voting_procedure_map.h"

#include <cardano/plutus_data/constr_plutus_data.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* GOVERNANCE_ACTION_ID_CBOR_1 = "825820000000000000000000000000000000000000000000000000000000000000000001";
static const char* GOVERNANCE_ACTION_ID_CBOR_2 = "825820000000000000000000000000000000000000000000000000000000000000000002";

static const char* VOTING_PROCEDURE_CBOR_1 = "8200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

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
 * Creates a new default instance of the voting_procedure.
 * @return A new instance of the voting_procedure.
 */
static cardano_voting_procedure_t*
new_default_voting_procedure(const char* action_id)
{
  cardano_voting_procedure_t* voting_procedure = NULL;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(action_id, strlen(action_id));
  cardano_error_t             result           = cardano_voting_procedure_from_cbor(reader, &voting_procedure);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voting_procedure;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_voting_procedure_map_new, canCreateVotingProcedureMap)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;

  // Act
  cardano_error_t error = cardano_voting_procedure_map_new(&voting_procedure_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(voting_procedure_map, testing::Not((cardano_voting_procedure_map_t*)nullptr));

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_new, returnsErrorIfVotingProcedureMapIsNull)
{
  // Act
  cardano_error_t error = cardano_voting_procedure_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;

  // Act
  cardano_error_t error = cardano_voting_procedure_map_new(&voting_procedure_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(voting_procedure_map, (cardano_voting_procedure_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voting_procedure_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;

  // Act
  cardano_error_t error = cardano_voting_procedure_map_new(&voting_procedure_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(voting_procedure_map, (cardano_voting_procedure_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voting_procedure_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_voting_procedure_map_ref(voting_procedure_map);

  // Assert
  EXPECT_THAT(voting_procedure_map, testing::Not((cardano_voting_procedure_map_t*)nullptr));
  EXPECT_EQ(cardano_voting_procedure_map_refcount(voting_procedure_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_voting_procedure_map_unref(&voting_procedure_map);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voting_procedure_map_ref(nullptr);
}

TEST(cardano_voting_procedure_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;

  // Act
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voting_procedure_map_unref((cardano_voting_procedure_map_t**)nullptr);
}

TEST(cardano_voting_procedure_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_voting_procedure_map_ref(voting_procedure_map);
  size_t ref_count = cardano_voting_procedure_map_refcount(voting_procedure_map);

  cardano_voting_procedure_map_unref(&voting_procedure_map);
  size_t updated_ref_count = cardano_voting_procedure_map_refcount(voting_procedure_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_voting_procedure_map_ref(voting_procedure_map);
  size_t ref_count = cardano_voting_procedure_map_refcount(voting_procedure_map);

  cardano_voting_procedure_map_unref(&voting_procedure_map);
  size_t updated_ref_count = cardano_voting_procedure_map_refcount(voting_procedure_map);

  cardano_voting_procedure_map_unref(&voting_procedure_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(voting_procedure_map, (cardano_voting_procedure_map_t*)nullptr);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_voting_procedure_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_voting_procedure_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  const char*                     message              = "This is a test message";

  // Act
  cardano_voting_procedure_map_set_last_error(voting_procedure_map, message);

  // Assert
  EXPECT_STREQ(cardano_voting_procedure_map_get_last_error(voting_procedure_map), "Object is NULL.");
}

TEST(cardano_voting_procedure_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_voting_procedure_map_set_last_error(voting_procedure_map, message);

  // Assert
  EXPECT_STREQ(cardano_voting_procedure_map_get_last_error(voting_procedure_map), "");

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_length, returnsZeroIfVotingProcedureMapIsNull)
{
  // Act
  size_t length = cardano_voting_procedure_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_voting_procedure_map_get_length, returnsZeroIfVotingProcedureMapIsEmpty)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_voting_procedure_map_get_length(voting_procedure_map);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get, returnsErrorIfVotingProcedureMapIsNull)
{
  // Arrange
  cardano_voting_procedure_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_voting_procedure_map_get(nullptr, (cardano_governance_action_id_t*)"", &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_map_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_voting_procedure_map_get(voting_procedure_map, (cardano_governance_action_id_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_voting_procedure_map_get(voting_procedure_map, nullptr, (cardano_voting_procedure_t**)"");

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get, returnsErrorIfKeyNotFound)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_voting_procedure_t*     val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  EXPECT_EQ(cardano_voting_procedure_map_insert(voting_procedure_map, key, val), CARDANO_SUCCESS);

  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_unref(&val);

  cardano_voting_procedure_t* data = nullptr;

  // Act
  cardano_governance_action_id_t* find = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_2);

  error = cardano_voting_procedure_map_get(voting_procedure_map, find, &data);

  cardano_governance_action_id_unref(&find);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get, returnsElementIfFound)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_voting_procedure_t*     val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  EXPECT_EQ(cardano_voting_procedure_map_insert(voting_procedure_map, key, val), CARDANO_SUCCESS);

  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_unref(&val);

  cardano_voting_procedure_t* data = nullptr;

  // Act
  cardano_governance_action_id_t* find = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);

  error = cardano_voting_procedure_map_get(voting_procedure_map, find, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(data, testing::Not((cardano_voting_procedure_t*)nullptr));

  cardano_vote_t result = cardano_voting_procedure_get_vote(data);

  EXPECT_EQ(result, 0);

  // Cleanup
  cardano_voting_procedure_unref(&data);
  cardano_governance_action_id_unref(&find);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_voting_procedure_t*     val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  EXPECT_EQ(cardano_voting_procedure_map_insert(voting_procedure_map, key, val), CARDANO_SUCCESS);

  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_unref(&val);

  // Act
  error = cardano_voting_procedure_map_get(voting_procedure_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_insert, returnsErrorIfVotingProcedureMapIsNull)
{
  // Arrange
  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_voting_procedure_t*     val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  // Act
  cardano_error_t error = cardano_voting_procedure_map_insert(nullptr, key, val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_unref(&val);
}

TEST(cardano_voting_procedure_map_insert, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_voting_procedure_t* val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  // Act
  error = cardano_voting_procedure_map_insert(voting_procedure_map, nullptr, val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_unref(&val);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_insert, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);

  // Act
  error = cardano_voting_procedure_map_insert(voting_procedure_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_insert, returnsErrorIfMemoryAllocationFailes)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_voting_procedure_t*     val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_voting_procedure_map_insert(voting_procedure_map, key, val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_unref(&val);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_keys, returnsErrorIfVotingProcedureMapIsNull)
{
  // Arrange
  cardano_governance_action_id_list_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_voting_procedure_map_get_keys(nullptr, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_map_get_keys, returnsErrorIfKeysIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_voting_procedure_map_get_keys(voting_procedure_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_keys, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_governance_action_id_list_t* keys = nullptr;

  // Act
  error = cardano_voting_procedure_map_get_keys(voting_procedure_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(keys, (cardano_governance_action_id_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_keys, returnsEmptyListIfVotingProcedureMapIsEmpty)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_list_t* keys = nullptr;

  // Act
  error = cardano_voting_procedure_map_get_keys(voting_procedure_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(keys, testing::Not((cardano_governance_action_id_list_t*)nullptr));

  size_t length = cardano_governance_action_id_list_get_length(keys);

  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_governance_action_id_list_unref(&keys);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_keys, returnsListOfKeys)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_voting_procedure_t*     val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  EXPECT_EQ(cardano_voting_procedure_map_insert(voting_procedure_map, key, val), CARDANO_SUCCESS);

  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_unref(&val);

  cardano_governance_action_id_list_t* keys = nullptr;

  // Act
  error = cardano_voting_procedure_map_get_keys(voting_procedure_map, &keys);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* value = nullptr;
  EXPECT_EQ(cardano_governance_action_id_list_get(keys, 0, &value), CARDANO_SUCCESS);

  uint64_t result = 0U;
  EXPECT_EQ(cardano_governance_action_id_get_index(value, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 1);

  cardano_governance_action_id_unref(&value);

  size_t length = cardano_governance_action_id_list_get_length(keys);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_governance_action_id_list_unref(&keys);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_values, returnsErrorIfVotingProcedureMapIsNull)
{
  // Arrange
  cardano_voting_procedure_list_t* values = nullptr;

  // Act
  cardano_error_t error = cardano_voting_procedure_map_get_values(nullptr, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_map_get_values, returnsErrorIfValuesIsNull)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_voting_procedure_map_get_values(voting_procedure_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_values, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_voting_procedure_list_t* values = nullptr;

  // Act
  error = cardano_voting_procedure_map_get_values(voting_procedure_map, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(values, (cardano_voting_procedure_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_values, returnsEmptyListIfVotingProcedureMapIsEmpty)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_voting_procedure_list_t* values = nullptr;

  // Act
  error = cardano_voting_procedure_map_get_values(voting_procedure_map, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(values, testing::Not((cardano_voting_procedure_list_t*)nullptr));

  size_t length = cardano_voting_procedure_list_get_length(values);

  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_voting_procedure_list_unref(&values);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}

TEST(cardano_voting_procedure_map_get_values, returnsListOfValues)
{
  // Arrange
  cardano_voting_procedure_map_t* voting_procedure_map = nullptr;
  cardano_error_t                 error                = cardano_voting_procedure_map_new(&voting_procedure_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* key = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_voting_procedure_t*     val = new_default_voting_procedure(VOTING_PROCEDURE_CBOR_1);

  EXPECT_EQ(cardano_voting_procedure_map_insert(voting_procedure_map, key, val), CARDANO_SUCCESS);

  cardano_governance_action_id_unref(&key);
  cardano_voting_procedure_unref(&val);

  cardano_voting_procedure_list_t* values = nullptr;

  // Act
  error = cardano_voting_procedure_map_get_values(voting_procedure_map, &values);

  cardano_voting_procedure_t* value = nullptr;
  EXPECT_EQ(cardano_voting_procedure_list_get(values, 0, &value), CARDANO_SUCCESS);

  cardano_vote_t result = cardano_voting_procedure_get_vote(value);

  cardano_voting_procedure_unref(&value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result, 0);
  EXPECT_THAT(values, testing::Not((cardano_voting_procedure_list_t*)nullptr));

  size_t length = cardano_voting_procedure_list_get_length(values);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_voting_procedure_list_unref(&values);
  cardano_voting_procedure_map_unref(&voting_procedure_map);
}
