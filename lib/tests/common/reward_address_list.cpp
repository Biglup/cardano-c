/**
 * \file reward_address_list.cpp
 *
 * \author angel.castillo
 * \date   May 21, 2024
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
#include <cardano/common/reward_address_list.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const char* rewardKey    = "stake1uyehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gh6ffgw";
static const char* rewardScript = "stake178phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcccycj5";

/* STATIC FUNCTIONS **********************************************************/

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

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the reward address list.
 */
static cardano_reward_address_list_t*
new_default_reward_address_list()
{
  cardano_reward_address_list_t* list = NULL;

  cardano_error_t error = cardano_reward_address_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* gai1 = new_default_reward_address(rewardKey);
  cardano_reward_address_t* gai2 = new_default_reward_address(rewardScript);

  EXPECT_EQ(cardano_reward_address_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_list_add(list, gai2), CARDANO_SUCCESS);

  cardano_reward_address_unref(&gai1);
  cardano_reward_address_unref(&gai2);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_reward_address_list_new, createsANewInstanceOfRewardAddressList)
{
  cardano_reward_address_list_t* list = NULL;

  EXPECT_EQ(cardano_reward_address_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_reward_address_list_get_length(list), 0);

  cardano_reward_address_list_unref(&list);
}

TEST(cardano_reward_address_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_reward_address_list_new(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_reward_address_list_t* list = NULL;

  EXPECT_EQ(cardano_reward_address_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_reward_address_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_reward_address_list_t* list = NULL;

  EXPECT_EQ(cardano_reward_address_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_reward_address_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_reward_address_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_reward_address_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_reward_address_list_get(nullptr, 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_reward_address_list_get((cardano_reward_address_list_t*)"", 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_reward_address_list_t* list = NULL;

  cardano_error_t error = cardano_reward_address_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_t* reward_address = NULL;
  error                                    = cardano_reward_address_list_get(list, 0, &reward_address);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_reward_address_list_unref(&list);
}

TEST(cardano_reward_address_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_reward_address_list_t* list = new_default_reward_address_list();

  // Act
  cardano_reward_address_t* address = NULL;
  cardano_error_t           error   = cardano_reward_address_list_get(list, 0, &address);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  char bech32[120] = { 0 };
  EXPECT_EQ(cardano_reward_address_to_bech32(address, bech32, 120), CARDANO_SUCCESS);

  EXPECT_STREQ(rewardKey, bech32);

  // Cleanup
  cardano_reward_address_list_unref(&list);
  cardano_reward_address_unref(&address);
}

TEST(cardano_reward_address_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_reward_address_list_t* list = new_default_reward_address_list();

  // Act
  cardano_reward_address_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_reward_address_list_t*)nullptr));
  EXPECT_EQ(cardano_reward_address_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_reward_address_list_unref(&list);
  cardano_reward_address_list_unref(&list);
}

TEST(cardano_reward_address_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_reward_address_list_ref(nullptr);
}

TEST(cardano_reward_address_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_reward_address_list_t* reward_address_list = nullptr;

  // Act
  cardano_reward_address_list_unref(&reward_address_list);
}

TEST(cardano_reward_address_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_reward_address_list_unref((cardano_reward_address_list_t**)nullptr);
}

TEST(cardano_reward_address_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_reward_address_list_t* list = new_default_reward_address_list();

  // Act
  cardano_reward_address_list_ref(list);
  size_t ref_count = cardano_reward_address_list_refcount(list);

  cardano_reward_address_list_unref(&list);
  size_t updated_ref_count = cardano_reward_address_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_reward_address_list_unref(&list);
}

TEST(cardano_reward_address_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_reward_address_list_t* reward_address_list = new_default_reward_address_list();

  // Act
  cardano_reward_address_list_ref(reward_address_list);
  size_t ref_count = cardano_reward_address_list_refcount(reward_address_list);

  cardano_reward_address_list_unref(&reward_address_list);
  size_t updated_ref_count = cardano_reward_address_list_refcount(reward_address_list);

  cardano_reward_address_list_unref(&reward_address_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(reward_address_list, (cardano_reward_address_list_t*)nullptr);

  // Cleanup
  cardano_reward_address_list_unref(&reward_address_list);
}

TEST(cardano_reward_address_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_reward_address_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_reward_address_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_reward_address_list_t* reward_address_list = nullptr;
  const char*                    message             = "This is a test message";

  // Act
  cardano_reward_address_list_set_last_error(reward_address_list, message);

  // Assert
  EXPECT_STREQ(cardano_reward_address_list_get_last_error(reward_address_list), "Object is NULL.");
}

TEST(cardano_reward_address_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_reward_address_list_t* reward_address_list = new_default_reward_address_list();

  const char* message = nullptr;

  // Act
  cardano_reward_address_list_set_last_error(reward_address_list, message);

  // Assert
  EXPECT_STREQ(cardano_reward_address_list_get_last_error(reward_address_list), "");

  // Cleanup
  cardano_reward_address_list_unref(&reward_address_list);
}

TEST(cardano_reward_address_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_reward_address_t* id = nullptr;

  // Act
  cardano_error_t result = cardano_reward_address_list_add(nullptr, id);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_reward_address_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_reward_address_list_add((cardano_reward_address_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}
