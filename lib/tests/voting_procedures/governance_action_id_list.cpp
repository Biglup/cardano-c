/**
 * \file governance_action_id_list.cpp
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

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* GOVERNANCE_ACTION_ID_CBOR_1 = "825820000000000000000000000000000000000000000000000000000000000000000001";
static const char* GOVERNANCE_ACTION_ID_CBOR_2 = "825820000000000000000000000000000000000000000000000000000000000000000002";
static const char* GOVERNANCE_ACTION_ID_CBOR_3 = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* GOVERNANCE_ACTION_ID_CBOR_4 = "825820000000000000000000000000000000000000000000000000000000000000000004";

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
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the governance action id list.
 */
static cardano_governance_action_id_list_t*
new_default_governance_action_id_list()
{
  cardano_governance_action_id_list_t* list = NULL;

  cardano_error_t error = cardano_governance_action_id_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_governance_action_id_t* gai1 = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_1);
  cardano_governance_action_id_t* gai2 = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_2);
  cardano_governance_action_id_t* gai3 = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_3);
  cardano_governance_action_id_t* gai4 = new_default_governance_action_id(GOVERNANCE_ACTION_ID_CBOR_4);

  EXPECT_EQ(cardano_governance_action_id_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_governance_action_id_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_governance_action_id_list_add(list, gai3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_governance_action_id_list_add(list, gai4), CARDANO_SUCCESS);

  cardano_governance_action_id_unref(&gai1);
  cardano_governance_action_id_unref(&gai2);
  cardano_governance_action_id_unref(&gai3);
  cardano_governance_action_id_unref(&gai4);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_governance_action_id_list_new, createsANewInstanceOfGovernanceActionIdList)
{
  cardano_governance_action_id_list_t* list = NULL;

  EXPECT_EQ(cardano_governance_action_id_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_governance_action_id_list_get_length(list), 0);

  cardano_governance_action_id_list_unref(&list);
}

TEST(cardano_governance_action_id_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_governance_action_id_list_new(nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_governance_action_id_list_t* list = NULL;

  EXPECT_EQ(cardano_governance_action_id_list_new(&list), CARDANO_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_governance_action_id_list_t* list = NULL;

  EXPECT_EQ(cardano_governance_action_id_list_new(&list), CARDANO_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_governance_action_id_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_governance_action_id_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_governance_action_id_list_get(nullptr, 0, nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_governance_action_id_list_get((cardano_governance_action_id_list_t*)"", 0, nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_governance_action_id_list_t* list = NULL;

  cardano_error_t error = cardano_governance_action_id_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_t* action_id = NULL;
  error                                     = cardano_governance_action_id_list_get(list, 0, &action_id);

  // Assert
  ASSERT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_governance_action_id_list_unref(&list);
}

TEST(cardano_governance_action_id_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_governance_action_id_list_t* list = new_default_governance_action_id_list();

  // Act
  cardano_governance_action_id_t* id    = NULL;
  cardano_error_t                 error = cardano_governance_action_id_list_get(list, 0, &id);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t index;
  EXPECT_EQ(cardano_governance_action_id_get_index(id, &index), CARDANO_SUCCESS);

  EXPECT_EQ(index, 1);

  // Cleanup
  cardano_governance_action_id_list_unref(&list);
  cardano_governance_action_id_unref(&id);
}

TEST(cardano_governance_action_id_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_governance_action_id_list_t* list = new_default_governance_action_id_list();

  // Act
  cardano_governance_action_id_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_governance_action_id_list_t*)nullptr));
  EXPECT_EQ(cardano_governance_action_id_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_governance_action_id_list_unref(&list);
  cardano_governance_action_id_list_unref(&list);
}

TEST(cardano_governance_action_id_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_governance_action_id_list_ref(nullptr);
}

TEST(cardano_governance_action_id_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_governance_action_id_list_t* governance_action_id_list = nullptr;

  // Act
  cardano_governance_action_id_list_unref(&governance_action_id_list);
}

TEST(cardano_governance_action_id_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_governance_action_id_list_unref((cardano_governance_action_id_list_t**)nullptr);
}

TEST(cardano_governance_action_id_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_governance_action_id_list_t* list = new_default_governance_action_id_list();

  // Act
  cardano_governance_action_id_list_ref(list);
  size_t ref_count = cardano_governance_action_id_list_refcount(list);

  cardano_governance_action_id_list_unref(&list);
  size_t updated_ref_count = cardano_governance_action_id_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_governance_action_id_list_unref(&list);
}

TEST(cardano_governance_action_id_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_governance_action_id_list_t* governance_action_id_list = new_default_governance_action_id_list();

  // Act
  cardano_governance_action_id_list_ref(governance_action_id_list);
  size_t ref_count = cardano_governance_action_id_list_refcount(governance_action_id_list);

  cardano_governance_action_id_list_unref(&governance_action_id_list);
  size_t updated_ref_count = cardano_governance_action_id_list_refcount(governance_action_id_list);

  cardano_governance_action_id_list_unref(&governance_action_id_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(governance_action_id_list, (cardano_governance_action_id_list_t*)nullptr);

  // Cleanup
  cardano_governance_action_id_list_unref(&governance_action_id_list);
}

TEST(cardano_governance_action_id_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_governance_action_id_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_governance_action_id_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_governance_action_id_list_t* governance_action_id_list = nullptr;
  const char*                          message                   = "This is a test message";

  // Act
  cardano_governance_action_id_list_set_last_error(governance_action_id_list, message);

  // Assert
  EXPECT_STREQ(cardano_governance_action_id_list_get_last_error(governance_action_id_list), "Object is NULL.");
}

TEST(cardano_governance_action_id_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_governance_action_id_list_t* governance_action_id_list = new_default_governance_action_id_list();

  const char* message = nullptr;

  // Act
  cardano_governance_action_id_list_set_last_error(governance_action_id_list, message);

  // Assert
  EXPECT_STREQ(cardano_governance_action_id_list_get_last_error(governance_action_id_list), "");

  // Cleanup
  cardano_governance_action_id_list_unref(&governance_action_id_list);
}

TEST(cardano_governance_action_id_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_governance_action_id_t* id = nullptr;

  // Act
  cardano_error_t result = cardano_governance_action_id_list_add(nullptr, id);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_governance_action_id_list_add((cardano_governance_action_id_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}
