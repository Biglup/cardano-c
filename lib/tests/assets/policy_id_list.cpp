/**
 * \file policy_id_list.cpp
 *
 * \author angel.castillo
 * \date   Sep 15, 2024
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

#include <cardano/assets/policy_id_list.h>
#include <cardano/crypto/blake2b_hash.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* POLICY_ID_HEX_1 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* POLICY_ID_HEX_2 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9b";
static const char* POLICY_ID_HEX_3 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9c";
static const char* POLICY_ID_HEX_4 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9d";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the blake2b_hash.
 * @return A new instance of the blake2b_hash.
 */
static cardano_blake2b_hash_t*
new_default_blake2b_hash(const char* hash)
{
  cardano_blake2b_hash_t* blake2b_hash = NULL;
  cardano_error_t         result       = cardano_blake2b_hash_from_hex(hash, strlen(hash), &blake2b_hash);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return blake2b_hash;
};

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the blake2b_hash list.
 */
static cardano_policy_id_list_t*
new_default_policy_id_list()
{
  cardano_policy_id_list_t* list = NULL;

  cardano_error_t error = cardano_policy_id_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* gai1 = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_blake2b_hash_t* gai2 = new_default_blake2b_hash(POLICY_ID_HEX_2);
  cardano_blake2b_hash_t* gai3 = new_default_blake2b_hash(POLICY_ID_HEX_3);
  cardano_blake2b_hash_t* gai4 = new_default_blake2b_hash(POLICY_ID_HEX_4);

  EXPECT_EQ(cardano_policy_id_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_policy_id_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_policy_id_list_add(list, gai3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_policy_id_list_add(list, gai4), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&gai1);
  cardano_blake2b_hash_unref(&gai2);
  cardano_blake2b_hash_unref(&gai3);
  cardano_blake2b_hash_unref(&gai4);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_policy_id_list_new, createsANewInstanceOfGovernanceActionIdList)
{
  cardano_policy_id_list_t* list = NULL;

  EXPECT_EQ(cardano_policy_id_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_policy_id_list_get_length(list), 0);

  cardano_policy_id_list_unref(&list);
}

TEST(cardano_policy_id_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_policy_id_list_new(nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_policy_id_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_policy_id_list_t* list = NULL;

  EXPECT_EQ(cardano_policy_id_list_new(&list), CARDANO_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_policy_id_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_policy_id_list_t* list = NULL;

  EXPECT_EQ(cardano_policy_id_list_new(&list), CARDANO_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_policy_id_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_policy_id_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_policy_id_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_policy_id_list_get(nullptr, 0, nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_policy_id_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_policy_id_list_get((cardano_policy_id_list_t*)"", 0, nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_policy_id_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_policy_id_list_t* list = NULL;

  cardano_error_t error = cardano_policy_id_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* hash = NULL;
  error                        = cardano_policy_id_list_get(list, 0, &hash);

  // Assert
  ASSERT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_policy_id_list_unref(&list);
}

TEST(cardano_policy_id_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_policy_id_list_t* list = new_default_policy_id_list();

  // Act
  cardano_blake2b_hash_t* name  = NULL;
  cardano_error_t         error = cardano_policy_id_list_get(list, 0, &name);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_blake2b_hash_get_hex_size(name);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_blake2b_hash_to_hex(name, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, POLICY_ID_HEX_1);

  // Cleanup
  cardano_policy_id_list_unref(&list);
  cardano_blake2b_hash_unref(&name);
  free(hex);
}

TEST(cardano_policy_id_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_policy_id_list_t* list = new_default_policy_id_list();

  // Act
  cardano_policy_id_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_policy_id_list_t*)nullptr));
  EXPECT_EQ(cardano_policy_id_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_policy_id_list_unref(&list);
  cardano_policy_id_list_unref(&list);
}

TEST(cardano_policy_id_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_policy_id_list_ref(nullptr);
}

TEST(cardano_policy_id_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_policy_id_list_t* policy_id_list = nullptr;

  // Act
  cardano_policy_id_list_unref(&policy_id_list);
}

TEST(cardano_policy_id_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_policy_id_list_unref((cardano_policy_id_list_t**)nullptr);
}

TEST(cardano_policy_id_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_policy_id_list_t* list = new_default_policy_id_list();

  // Act
  cardano_policy_id_list_ref(list);
  size_t ref_count = cardano_policy_id_list_refcount(list);

  cardano_policy_id_list_unref(&list);
  size_t updated_ref_count = cardano_policy_id_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_policy_id_list_unref(&list);
}

TEST(cardano_policy_id_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_policy_id_list_t* policy_id_list = new_default_policy_id_list();

  // Act
  cardano_policy_id_list_ref(policy_id_list);
  size_t ref_count = cardano_policy_id_list_refcount(policy_id_list);

  cardano_policy_id_list_unref(&policy_id_list);
  size_t updated_ref_count = cardano_policy_id_list_refcount(policy_id_list);

  cardano_policy_id_list_unref(&policy_id_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(policy_id_list, (cardano_policy_id_list_t*)nullptr);

  // Cleanup
  cardano_policy_id_list_unref(&policy_id_list);
}

TEST(cardano_policy_id_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_policy_id_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_policy_id_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_policy_id_list_t* policy_id_list = nullptr;
  const char*               message        = "This is a test message";

  // Act
  cardano_policy_id_list_set_last_error(policy_id_list, message);

  // Assert
  EXPECT_STREQ(cardano_policy_id_list_get_last_error(policy_id_list), "Object is NULL.");
}

TEST(cardano_policy_id_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_policy_id_list_t* policy_id_list = new_default_policy_id_list();

  const char* message = nullptr;

  // Act
  cardano_policy_id_list_set_last_error(policy_id_list, message);

  // Assert
  EXPECT_STREQ(cardano_policy_id_list_get_last_error(policy_id_list), "");

  // Cleanup
  cardano_policy_id_list_unref(&policy_id_list);
}

TEST(cardano_policy_id_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* name = nullptr;

  // Act
  cardano_error_t result = cardano_policy_id_list_add(nullptr, name);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_policy_id_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_policy_id_list_add((cardano_policy_id_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}
