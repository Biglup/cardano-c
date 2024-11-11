/**
 * \file reward_address_to_redeemer_map.cpp
 *
 * \author angel.castillo
 * \date   Nov 11, 2024
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

#include "../../../src/transaction_builder/internals/reward_address_to_redeemer_map.h"

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/address/reward_address.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* rewardKey     = "stake1uyehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gh6ffgw";
static const char* rewardScript  = "stake178phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcccycj5";
static const char* REDEEMER_CBOR = "840000d8799f0102030405ff821821182c";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the redeemer.
 * @return A new instance of the redeemer.
 */
static cardano_redeemer_t*
new_default_redeemer()
{
  cardano_redeemer_t*    redeemer = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(REDEEMER_CBOR, strlen(REDEEMER_CBOR));
  cardano_error_t        result   = cardano_redeemer_from_cbor(reader, &redeemer);

  cardano_redeemer_clear_cbor_cache(redeemer);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return redeemer;
};

/**
 * Creates a new default instance of the reward address.
 * @return A new instance of the reward address.
 */
static cardano_reward_address_t*
new_default_reward_address(const char* reward_address)
{
  cardano_reward_address_t* reward_address_obj = NULL;
  cardano_error_t           result             = cardano_reward_address_from_bech32(reward_address, strlen(reward_address), &reward_address_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return reward_address_obj;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_reward_address_to_redeemer_map_new, canCreateMap)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(reward_address_to_redeemer_map, testing::Not((cardano_reward_address_to_redeemer_map_t*)nullptr));

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_new, returnsErrorIfProposedParamUpdatesIsNull)
{
  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(reward_address_to_redeemer_map, (cardano_reward_address_to_redeemer_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_reward_address_to_redeemer_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(reward_address_to_redeemer_map, (cardano_reward_address_to_redeemer_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_reward_address_to_redeemer_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_to_redeemer_map_ref(reward_address_to_redeemer_map);

  // Assert
  EXPECT_THAT(reward_address_to_redeemer_map, testing::Not((cardano_reward_address_to_redeemer_map_t*)nullptr));
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_refcount(reward_address_to_redeemer_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_reward_address_to_redeemer_map_ref(nullptr);
}

TEST(cardano_reward_address_to_redeemer_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;

  // Act
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_reward_address_to_redeemer_map_unref((cardano_reward_address_to_redeemer_map_t**)nullptr);
}

TEST(cardano_reward_address_to_redeemer_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_to_redeemer_map_ref(reward_address_to_redeemer_map);
  size_t ref_count = cardano_reward_address_to_redeemer_map_refcount(reward_address_to_redeemer_map);

  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  size_t updated_ref_count = cardano_reward_address_to_redeemer_map_refcount(reward_address_to_redeemer_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_to_redeemer_map_ref(reward_address_to_redeemer_map);
  size_t ref_count = cardano_reward_address_to_redeemer_map_refcount(reward_address_to_redeemer_map);

  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  size_t updated_ref_count = cardano_reward_address_to_redeemer_map_refcount(reward_address_to_redeemer_map);

  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(reward_address_to_redeemer_map, (cardano_reward_address_to_redeemer_map_t*)nullptr);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_reward_address_to_redeemer_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_reward_address_to_redeemer_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  const char*                               message                        = "This is a test message";

  // Act
  cardano_reward_address_to_redeemer_map_set_last_error(reward_address_to_redeemer_map, message);

  // Assert
  EXPECT_STREQ(cardano_reward_address_to_redeemer_map_get_last_error(reward_address_to_redeemer_map), "Object is NULL.");
}

TEST(cardano_reward_address_to_redeemer_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_reward_address_to_redeemer_map_set_last_error(reward_address_to_redeemer_map, message);

  // Assert
  EXPECT_STREQ(cardano_reward_address_to_redeemer_map_get_last_error(reward_address_to_redeemer_map), "");

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_get_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_reward_address_to_redeemer_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_reward_address_to_redeemer_map_get_size, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address  = new_default_reward_address(rewardKey);
  cardano_redeemer_t*       redeemer = new_default_redeemer();

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, ((cardano_reward_address_t*)(void*)address), redeemer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_reward_address_to_redeemer_map_get_length(reward_address_to_redeemer_map);

  // Assert
  EXPECT_EQ(size, 1);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_reward_address_to_redeemer_map_insert, returnsErrorIfObjectIsNull)
{
  // Act
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_insert(nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_insert((cardano_reward_address_to_redeemer_map_t*)"", nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_insert((cardano_reward_address_to_redeemer_map_t*)"", (cardano_reward_address_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_insert, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address  = new_default_reward_address(rewardKey);
  cardano_redeemer_t*       redeemer = new_default_redeemer();

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address, redeemer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address);
  cardano_set_allocators(malloc, realloc, free);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_reward_address_to_redeemer_map_get, returnsErrorIfObjectIsNull)
{
  // Act
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_get(nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_get((cardano_reward_address_to_redeemer_map_t*)"", nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_get((cardano_reward_address_to_redeemer_map_t*)"", (cardano_reward_address_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t*       value   = NULL;
  cardano_reward_address_t* address = new_default_reward_address(rewardKey);

  // Act
  error = cardano_reward_address_to_redeemer_map_get(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address);
  cardano_redeemer_unref(&value);
}

TEST(cardano_reward_address_to_redeemer_map_get, returnsTheElement)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address  = new_default_reward_address(rewardKey);
  cardano_redeemer_t*       redeemer = new_default_redeemer();

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address, redeemer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value = NULL;
  error                     = cardano_reward_address_to_redeemer_map_get(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, redeemer);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address);
  cardano_redeemer_unref(&redeemer);
  cardano_redeemer_unref(&value);
}

TEST(cardano_reward_address_to_redeemer_map_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1  = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2  = new_default_reward_address(rewardScript);
  cardano_redeemer_t*       redeemer1 = new_default_redeemer();
  cardano_redeemer_t*       redeemer2 = new_default_redeemer();

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address1, redeemer1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address2, redeemer2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value = NULL;
  error                     = cardano_reward_address_to_redeemer_map_get(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address2, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, redeemer2);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
  cardano_redeemer_unref(&redeemer1);
  cardano_redeemer_unref(&redeemer2);
  cardano_redeemer_unref(&value);
}

TEST(cardano_reward_address_to_redeemer_map_get, returnsTheRightElementIfMoreThanOne2)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1  = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2  = new_default_reward_address(rewardScript);
  cardano_redeemer_t*       redeemer1 = new_default_redeemer();
  cardano_redeemer_t*       redeemer2 = new_default_redeemer();

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address1, redeemer1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address2, redeemer2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value = NULL;
  error                     = cardano_reward_address_to_redeemer_map_get(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address2, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, redeemer2);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
  cardano_redeemer_unref(&redeemer1);
  cardano_redeemer_unref(&redeemer2);
  cardano_redeemer_unref(&value);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_get_key_at(nullptr, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_get_key_at((cardano_reward_address_to_redeemer_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;

  // Act
  error = cardano_reward_address_to_redeemer_map_get_key_at(reward_address_to_redeemer_map, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1  = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2  = new_default_reward_address(rewardScript);
  cardano_redeemer_t*       redeemer1 = new_default_redeemer();
  cardano_redeemer_t*       redeemer2 = new_default_redeemer();

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address1, redeemer1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address2, redeemer2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_t* reward_address = nullptr;
  error                                    = cardano_reward_address_to_redeemer_map_get_key_at(reward_address_to_redeemer_map, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(reward_address, (cardano_reward_address_t*)(void*)address1);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
  cardano_redeemer_unref(&redeemer1);
  cardano_redeemer_unref(&redeemer2);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_to_redeemer_map_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_redeemer_t* value = NULL;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_get_value_at(nullptr, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_get_value_at((cardano_reward_address_to_redeemer_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t* value = NULL;

  // Act
  error = cardano_reward_address_to_redeemer_map_get_value_at(reward_address_to_redeemer_map, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t* value = new_default_redeemer();

  cardano_reward_address_t* reward_address = new_default_reward_address(rewardKey);

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)reward_address, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value_out = NULL;
  error                         = cardano_reward_address_to_redeemer_map_get_value_at(reward_address_to_redeemer_map, 0, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&reward_address);
  cardano_redeemer_unref(&value);
  cardano_redeemer_unref(&value_out);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;
  cardano_redeemer_t*       value          = nullptr;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_get_key_value_at(nullptr, 0, &reward_address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_value_at, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_redeemer_t* value = nullptr;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_get_key_value_at((cardano_reward_address_to_redeemer_map_t*)"", 0, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_value_at, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  cardano_error_t error = cardano_reward_address_to_redeemer_map_get_key_value_at((cardano_reward_address_to_redeemer_map_t*)"", 0, &reward_address, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;
  cardano_redeemer_t*       value          = nullptr;

  // Act
  error = cardano_reward_address_to_redeemer_map_get_key_value_at(reward_address_to_redeemer_map, 0, &reward_address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
}

TEST(cardano_reward_address_to_redeemer_map_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t* value = new_default_redeemer();

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_default_reward_address(rewardKey);

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)reward_address, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_t* reward_address_out = nullptr;
  cardano_redeemer_t*       value_out          = nullptr;
  error                                        = cardano_reward_address_to_redeemer_map_get_key_value_at(reward_address_to_redeemer_map, 0, &reward_address_out, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ((cardano_reward_address_t*)reward_address, reward_address_out);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&reward_address);
  cardano_redeemer_unref(&value);
  cardano_redeemer_unref(&value_out);
  cardano_reward_address_unref(&reward_address_out);
}

TEST(cardano_reward_address_to_redeemer_map_update_redeemer_index, returnsErrorIfObjectIsNull)
{
  // Act
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_update_redeemer_index(nullptr, nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_reward_address_to_redeemer_map_update_redeemer_index((cardano_reward_address_to_redeemer_map_t*)"", nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_redeemer_map_update_redeemer_index, doesntReturnErrorIfNotFound)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address  = new_default_reward_address(rewardKey);
  cardano_redeemer_t*       redeemer = new_default_redeemer();

  // Act
  error = cardano_reward_address_to_redeemer_map_update_redeemer_index(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_reward_address_to_redeemer_map_update_redeemer_index, updatesIndexIfFound)
{
  // Arrange
  cardano_reward_address_to_redeemer_map_t* reward_address_to_redeemer_map = nullptr;
  cardano_error_t                           error                          = cardano_reward_address_to_redeemer_map_new(&reward_address_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address  = new_default_reward_address(rewardKey);
  cardano_redeemer_t*       redeemer = new_default_redeemer();

  error = cardano_reward_address_to_redeemer_map_insert(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address, redeemer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_reward_address_to_redeemer_map_update_redeemer_index(reward_address_to_redeemer_map, (cardano_reward_address_t*)(void*)address, 77);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_get_index(redeemer), 77);

  // Cleanup
  cardano_reward_address_to_redeemer_map_unref(&reward_address_to_redeemer_map);
  cardano_reward_address_unref(&address);
  cardano_redeemer_unref(&redeemer);
}