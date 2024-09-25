/**
 * \file asset_name_list.cpp
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

#include <cardano/assets/asset_name.h>
#include <cardano/assets/asset_name_list.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* ASSET_NAME_HEX_1 = "736b7977616c6b657241";
static const char* ASSET_NAME_HEX_2 = "736b7977616c6b657242";
static const char* ASSET_NAME_HEX_3 = "736b7977616c6b657243";
static const char* ASSET_NAME_HEX_4 = "736b7977616c6b657244";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the asset_name.
 * @return A new instance of the asset_name.
 */
static cardano_asset_name_t*
new_default_asset_name(const char* action_name)
{
  cardano_asset_name_t* asset_name = NULL;
  cardano_error_t       result     = cardano_asset_name_from_hex(action_name, strlen(action_name), &asset_name);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return asset_name;
};

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the asset_name list.
 */
static cardano_asset_name_list_t*
new_default_asset_name_list()
{
  cardano_asset_name_list_t* list = NULL;

  cardano_error_t error = cardano_asset_name_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* gai1 = new_default_asset_name(ASSET_NAME_HEX_1);
  cardano_asset_name_t* gai2 = new_default_asset_name(ASSET_NAME_HEX_2);
  cardano_asset_name_t* gai3 = new_default_asset_name(ASSET_NAME_HEX_3);
  cardano_asset_name_t* gai4 = new_default_asset_name(ASSET_NAME_HEX_4);

  EXPECT_EQ(cardano_asset_name_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_list_add(list, gai3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_list_add(list, gai4), CARDANO_SUCCESS);

  cardano_asset_name_unref(&gai1);
  cardano_asset_name_unref(&gai2);
  cardano_asset_name_unref(&gai3);
  cardano_asset_name_unref(&gai4);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_asset_name_list_new, createsANewInstanceOfGovernanceActionIdList)
{
  cardano_asset_name_list_t* list = NULL;

  EXPECT_EQ(cardano_asset_name_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_asset_name_list_get_length(list), 0);

  cardano_asset_name_list_unref(&list);
}

TEST(cardano_asset_name_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_asset_name_list_new(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_asset_name_list_t* list = NULL;

  EXPECT_EQ(cardano_asset_name_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_asset_name_list_t* list = NULL;

  EXPECT_EQ(cardano_asset_name_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_asset_name_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_asset_name_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_asset_name_list_get(nullptr, 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_asset_name_list_get((cardano_asset_name_list_t*)"", 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_asset_name_list_t* list = NULL;

  cardano_error_t error = cardano_asset_name_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_name_t* action_name = NULL;
  error                             = cardano_asset_name_list_get(list, 0, &action_name);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_asset_name_list_unref(&list);
}

TEST(cardano_asset_name_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_asset_name_list_t* list = new_default_asset_name_list();

  // Act
  cardano_asset_name_t* name  = NULL;
  cardano_error_t       error = cardano_asset_name_list_get(list, 0, &name);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cardano_asset_name_get_string(name), "skywalkerA");

  // Cleanup
  cardano_asset_name_list_unref(&list);
  cardano_asset_name_unref(&name);
}

TEST(cardano_asset_name_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_asset_name_list_t* list = new_default_asset_name_list();

  // Act
  cardano_asset_name_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_asset_name_list_t*)nullptr));
  EXPECT_EQ(cardano_asset_name_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_asset_name_list_unref(&list);
  cardano_asset_name_list_unref(&list);
}

TEST(cardano_asset_name_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_name_list_ref(nullptr);
}

TEST(cardano_asset_name_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_asset_name_list_t* asset_name_list = nullptr;

  // Act
  cardano_asset_name_list_unref(&asset_name_list);
}

TEST(cardano_asset_name_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_name_list_unref((cardano_asset_name_list_t**)nullptr);
}

TEST(cardano_asset_name_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_asset_name_list_t* list = new_default_asset_name_list();

  // Act
  cardano_asset_name_list_ref(list);
  size_t ref_count = cardano_asset_name_list_refcount(list);

  cardano_asset_name_list_unref(&list);
  size_t updated_ref_count = cardano_asset_name_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_asset_name_list_unref(&list);
}

TEST(cardano_asset_name_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_asset_name_list_t* asset_name_list = new_default_asset_name_list();

  // Act
  cardano_asset_name_list_ref(asset_name_list);
  size_t ref_count = cardano_asset_name_list_refcount(asset_name_list);

  cardano_asset_name_list_unref(&asset_name_list);
  size_t updated_ref_count = cardano_asset_name_list_refcount(asset_name_list);

  cardano_asset_name_list_unref(&asset_name_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(asset_name_list, (cardano_asset_name_list_t*)nullptr);

  // Cleanup
  cardano_asset_name_list_unref(&asset_name_list);
}

TEST(cardano_asset_name_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_asset_name_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_asset_name_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_asset_name_list_t* asset_name_list = nullptr;
  const char*                message         = "This is a test message";

  // Act
  cardano_asset_name_list_set_last_error(asset_name_list, message);

  // Assert
  EXPECT_STREQ(cardano_asset_name_list_get_last_error(asset_name_list), "Object is NULL.");
}

TEST(cardano_asset_name_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_asset_name_list_t* asset_name_list = new_default_asset_name_list();

  const char* message = nullptr;

  // Act
  cardano_asset_name_list_set_last_error(asset_name_list, message);

  // Assert
  EXPECT_STREQ(cardano_asset_name_list_get_last_error(asset_name_list), "");

  // Cleanup
  cardano_asset_name_list_unref(&asset_name_list);
}

TEST(cardano_asset_name_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_asset_name_t* name = nullptr;

  // Act
  cardano_error_t result = cardano_asset_name_list_add(nullptr, name);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_name_list_add((cardano_asset_name_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}
