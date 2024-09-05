/**
 * \file asset_id_list.cpp
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

#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_id_list.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* ASSET_ID_HEX_1 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657241";
static const char* ASSET_ID_HEX_2 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657242";
static const char* ASSET_ID_HEX_3 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657243";
static const char* ASSET_ID_HEX_4 = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657244";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the asset_id.
 * @return A new instance of the asset_id.
 */
static cardano_asset_id_t*
new_default_asset_id(const char* action_id)
{
  cardano_asset_id_t* asset_id = NULL;
  cardano_error_t     result   = cardano_asset_id_from_hex(action_id, strlen(action_id), &asset_id);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return asset_id;
};

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the asset_id list.
 */
static cardano_asset_id_list_t*
new_default_asset_id_list()
{
  cardano_asset_id_list_t* list = NULL;

  cardano_error_t error = cardano_asset_id_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_id_t* gai1 = new_default_asset_id(ASSET_ID_HEX_1);
  cardano_asset_id_t* gai2 = new_default_asset_id(ASSET_ID_HEX_2);
  cardano_asset_id_t* gai3 = new_default_asset_id(ASSET_ID_HEX_3);
  cardano_asset_id_t* gai4 = new_default_asset_id(ASSET_ID_HEX_4);

  EXPECT_EQ(cardano_asset_id_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_list_add(list, gai3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_list_add(list, gai4), CARDANO_SUCCESS);

  cardano_asset_id_unref(&gai1);
  cardano_asset_id_unref(&gai2);
  cardano_asset_id_unref(&gai3);
  cardano_asset_id_unref(&gai4);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_asset_id_list_new, createsANewInstanceOfGovernanceActionIdList)
{
  cardano_asset_id_list_t* list = NULL;

  EXPECT_EQ(cardano_asset_id_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_asset_id_list_get_length(list), 0);

  cardano_asset_id_list_unref(&list);
}

TEST(cardano_asset_id_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_asset_id_list_new(nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_id_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_asset_id_list_t* list = NULL;

  EXPECT_EQ(cardano_asset_id_list_new(&list), CARDANO_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_id_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_asset_id_list_t* list = NULL;

  EXPECT_EQ(cardano_asset_id_list_new(&list), CARDANO_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_id_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_asset_id_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_asset_id_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_asset_id_list_get(nullptr, 0, nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_id_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_asset_id_list_get((cardano_asset_id_list_t*)"", 0, nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_id_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_asset_id_list_t* list = NULL;

  cardano_error_t error = cardano_asset_id_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_id_t* action_id = NULL;
  error                         = cardano_asset_id_list_get(list, 0, &action_id);

  // Assert
  ASSERT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_asset_id_list_unref(&list);
}

TEST(cardano_asset_id_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_asset_id_list_t* list = new_default_asset_id_list();

  // Act
  cardano_asset_id_t* id    = NULL;
  cardano_error_t     error = cardano_asset_id_list_get(list, 0, &id);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = cardano_asset_id_get_asset_name(id);
  ASSERT_NE(asset_name, nullptr);

  EXPECT_STREQ(cardano_asset_name_get_string(asset_name), "skywalkerA");

  // Cleanup
  cardano_asset_id_list_unref(&list);
  cardano_asset_id_unref(&id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_id_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_asset_id_list_t* list = new_default_asset_id_list();

  // Act
  cardano_asset_id_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_asset_id_list_t*)nullptr));
  EXPECT_EQ(cardano_asset_id_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_asset_id_list_unref(&list);
  cardano_asset_id_list_unref(&list);
}

TEST(cardano_asset_id_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_id_list_ref(nullptr);
}

TEST(cardano_asset_id_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_asset_id_list_t* asset_id_list = nullptr;

  // Act
  cardano_asset_id_list_unref(&asset_id_list);
}

TEST(cardano_asset_id_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_id_list_unref((cardano_asset_id_list_t**)nullptr);
}

TEST(cardano_asset_id_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_asset_id_list_t* list = new_default_asset_id_list();

  // Act
  cardano_asset_id_list_ref(list);
  size_t ref_count = cardano_asset_id_list_refcount(list);

  cardano_asset_id_list_unref(&list);
  size_t updated_ref_count = cardano_asset_id_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_asset_id_list_unref(&list);
}

TEST(cardano_asset_id_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_asset_id_list_t* asset_id_list = new_default_asset_id_list();

  // Act
  cardano_asset_id_list_ref(asset_id_list);
  size_t ref_count = cardano_asset_id_list_refcount(asset_id_list);

  cardano_asset_id_list_unref(&asset_id_list);
  size_t updated_ref_count = cardano_asset_id_list_refcount(asset_id_list);

  cardano_asset_id_list_unref(&asset_id_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(asset_id_list, (cardano_asset_id_list_t*)nullptr);

  // Cleanup
  cardano_asset_id_list_unref(&asset_id_list);
}

TEST(cardano_asset_id_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_asset_id_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_asset_id_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_asset_id_list_t* asset_id_list = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_asset_id_list_set_last_error(asset_id_list, message);

  // Assert
  EXPECT_STREQ(cardano_asset_id_list_get_last_error(asset_id_list), "Object is NULL.");
}

TEST(cardano_asset_id_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_asset_id_list_t* asset_id_list = new_default_asset_id_list();

  const char* message = nullptr;

  // Act
  cardano_asset_id_list_set_last_error(asset_id_list, message);

  // Assert
  EXPECT_STREQ(cardano_asset_id_list_get_last_error(asset_id_list), "");

  // Cleanup
  cardano_asset_id_list_unref(&asset_id_list);
}

TEST(cardano_asset_id_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_asset_id_t* id = nullptr;

  // Act
  cardano_error_t result = cardano_asset_id_list_add(nullptr, id);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_id_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_id_list_add((cardano_asset_id_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}
