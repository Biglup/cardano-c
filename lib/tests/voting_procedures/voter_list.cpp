/**
 * \file voter_list.cpp
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

#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* VOTER_CBOR_1 = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* VOTER_CBOR_2 = "8201581c00000000000000000000000000000000000000000000000000000000";
static const char* VOTER_CBOR_3 = "8202581c00000000000000000000000000000000000000000000000000000000";
static const char* VOTER_CBOR_4 = "8203581c00000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the voter.
 * @return A new instance of the voter.
 */
static cardano_voter_t*
new_default_voter(const char* action_id)
{
  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(action_id, strlen(action_id));
  cardano_error_t        result = cardano_voter_from_cbor(reader, &voter);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
};

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the voter list.
 */
static cardano_voter_list_t*
new_default_voter_list()
{
  cardano_voter_list_t* list = NULL;

  cardano_error_t error = cardano_voter_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_voter_t* gai1 = new_default_voter(VOTER_CBOR_1);
  cardano_voter_t* gai2 = new_default_voter(VOTER_CBOR_2);
  cardano_voter_t* gai3 = new_default_voter(VOTER_CBOR_3);
  cardano_voter_t* gai4 = new_default_voter(VOTER_CBOR_4);

  EXPECT_EQ(cardano_voter_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voter_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voter_list_add(list, gai3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voter_list_add(list, gai4), CARDANO_SUCCESS);

  cardano_voter_unref(&gai1);
  cardano_voter_unref(&gai2);
  cardano_voter_unref(&gai3);
  cardano_voter_unref(&gai4);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_voter_list_new, createsANewInstanceOfGovernanceActionIdList)
{
  cardano_voter_list_t* list = NULL;

  EXPECT_EQ(cardano_voter_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_voter_list_get_length(list), 0);

  cardano_voter_list_unref(&list);
}

TEST(cardano_voter_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_voter_list_new(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_voter_list_t* list = NULL;

  EXPECT_EQ(cardano_voter_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voter_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_voter_list_t* list = NULL;

  EXPECT_EQ(cardano_voter_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voter_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_voter_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_voter_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_voter_list_get(nullptr, 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_voter_list_get((cardano_voter_list_t*)"", 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_voter_list_t* list = NULL;

  cardano_error_t error = cardano_voter_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_voter_t* action_id = NULL;
  error                      = cardano_voter_list_get(list, 0, &action_id);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_voter_list_unref(&list);
}

TEST(cardano_voter_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_voter_list_t* list = new_default_voter_list();

  // Act
  cardano_voter_t* id    = NULL;
  cardano_error_t  error = cardano_voter_list_get(list, 0, &id);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_voter_type_t type;
  EXPECT_EQ(cardano_voter_get_type(id, &type), CARDANO_SUCCESS);

  EXPECT_EQ(type, 0);

  // Cleanup
  cardano_voter_list_unref(&list);
  cardano_voter_unref(&id);
}

TEST(cardano_voter_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_voter_list_t* list = new_default_voter_list();

  // Act
  cardano_voter_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_voter_list_t*)nullptr));
  EXPECT_EQ(cardano_voter_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_voter_list_unref(&list);
  cardano_voter_list_unref(&list);
}

TEST(cardano_voter_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voter_list_ref(nullptr);
}

TEST(cardano_voter_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_voter_list_t* voter_list = nullptr;

  // Act
  cardano_voter_list_unref(&voter_list);
}

TEST(cardano_voter_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voter_list_unref((cardano_voter_list_t**)nullptr);
}

TEST(cardano_voter_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_voter_list_t* list = new_default_voter_list();

  // Act
  cardano_voter_list_ref(list);
  size_t ref_count = cardano_voter_list_refcount(list);

  cardano_voter_list_unref(&list);
  size_t updated_ref_count = cardano_voter_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_voter_list_unref(&list);
}

TEST(cardano_voter_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_voter_list_t* voter_list = new_default_voter_list();

  // Act
  cardano_voter_list_ref(voter_list);
  size_t ref_count = cardano_voter_list_refcount(voter_list);

  cardano_voter_list_unref(&voter_list);
  size_t updated_ref_count = cardano_voter_list_refcount(voter_list);

  cardano_voter_list_unref(&voter_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(voter_list, (cardano_voter_list_t*)nullptr);

  // Cleanup
  cardano_voter_list_unref(&voter_list);
}

TEST(cardano_voter_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_voter_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_voter_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_voter_list_t* voter_list = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_voter_list_set_last_error(voter_list, message);

  // Assert
  EXPECT_STREQ(cardano_voter_list_get_last_error(voter_list), "Object is NULL.");
}

TEST(cardano_voter_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_voter_list_t* voter_list = new_default_voter_list();

  const char* message = nullptr;

  // Act
  cardano_voter_list_set_last_error(voter_list, message);

  // Assert
  EXPECT_STREQ(cardano_voter_list_get_last_error(voter_list), "");

  // Cleanup
  cardano_voter_list_unref(&voter_list);
}

TEST(cardano_voter_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_voter_t* id = nullptr;

  // Act
  cardano_error_t result = cardano_voter_list_add(nullptr, id);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voter_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_voter_list_add((cardano_voter_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}
