/**
 * \file metadatum_label_list.cpp
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#include <cardano/auxiliary_data/metadatum_label_list.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the reward label list.
 */
static cardano_metadatum_label_list_t*
new_default_metadatum_label_list()
{
  cardano_metadatum_label_list_t* list = NULL;

  cardano_error_t error = cardano_metadatum_label_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_label_list_add(list, 725), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_label_list_add(list, 800), CARDANO_SUCCESS);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_metadatum_label_list_new, createsANewInstanceOfRewardAddressList)
{
  cardano_metadatum_label_list_t* list = NULL;

  EXPECT_EQ(cardano_metadatum_label_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_metadatum_label_list_get_length(list), 0);

  cardano_metadatum_label_list_unref(&list);
}

TEST(cardano_metadatum_label_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_metadatum_label_list_new(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_label_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_label_list_t* list = NULL;

  EXPECT_EQ(cardano_metadatum_label_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_label_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_label_list_t* list = NULL;

  EXPECT_EQ(cardano_metadatum_label_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_label_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_metadatum_label_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_metadatum_label_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_metadatum_label_list_get(nullptr, 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_label_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_metadatum_label_list_get((cardano_metadatum_label_list_t*)"", 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_label_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_metadatum_label_list_t* list = NULL;

  cardano_error_t error = cardano_metadatum_label_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t metadatum_label = 0;
  error                    = cardano_metadatum_label_list_get(list, 0, &metadatum_label);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_metadatum_label_list_unref(&list);
}

TEST(cardano_metadatum_label_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_metadatum_label_list_t* list = new_default_metadatum_label_list();

  // Act
  uint64_t        label = 0;
  cardano_error_t error = cardano_metadatum_label_list_get(list, 0, &label);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(label, 725);

  // Cleanup
  cardano_metadatum_label_list_unref(&list);
}

TEST(cardano_metadatum_label_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_label_list_t* list = new_default_metadatum_label_list();

  // Act
  cardano_metadatum_label_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_metadatum_label_list_t*)nullptr));
  EXPECT_EQ(cardano_metadatum_label_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_metadatum_label_list_unref(&list);
  cardano_metadatum_label_list_unref(&list);
}

TEST(cardano_metadatum_label_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_label_list_ref(nullptr);
}

TEST(cardano_metadatum_label_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_metadatum_label_list_t* metadatum_label_list = nullptr;

  // Act
  cardano_metadatum_label_list_unref(&metadatum_label_list);
}

TEST(cardano_metadatum_label_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_label_list_unref((cardano_metadatum_label_list_t**)nullptr);
}

TEST(cardano_metadatum_label_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_label_list_t* list = new_default_metadatum_label_list();

  // Act
  cardano_metadatum_label_list_ref(list);
  size_t ref_count = cardano_metadatum_label_list_refcount(list);

  cardano_metadatum_label_list_unref(&list);
  size_t updated_ref_count = cardano_metadatum_label_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_metadatum_label_list_unref(&list);
}

TEST(cardano_metadatum_label_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_metadatum_label_list_t* metadatum_label_list = new_default_metadatum_label_list();

  // Act
  cardano_metadatum_label_list_ref(metadatum_label_list);
  size_t ref_count = cardano_metadatum_label_list_refcount(metadatum_label_list);

  cardano_metadatum_label_list_unref(&metadatum_label_list);
  size_t updated_ref_count = cardano_metadatum_label_list_refcount(metadatum_label_list);

  cardano_metadatum_label_list_unref(&metadatum_label_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(metadatum_label_list, (cardano_metadatum_label_list_t*)nullptr);

  // Cleanup
  cardano_metadatum_label_list_unref(&metadatum_label_list);
}

TEST(cardano_metadatum_label_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_metadatum_label_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_metadatum_label_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_metadatum_label_list_t* metadatum_label_list = nullptr;
  const char*                     message              = "This is a test message";

  // Act
  cardano_metadatum_label_list_set_last_error(metadatum_label_list, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_label_list_get_last_error(metadatum_label_list), "Object is NULL.");
}

TEST(cardano_metadatum_label_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_metadatum_label_list_t* metadatum_label_list = new_default_metadatum_label_list();

  const char* message = nullptr;

  // Act
  cardano_metadatum_label_list_set_last_error(metadatum_label_list, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_label_list_get_last_error(metadatum_label_list), "");

  // Cleanup
  cardano_metadatum_label_list_unref(&metadatum_label_list);
}

TEST(cardano_metadatum_label_list_add, returnsErrorIfListIsNull)
{
  // Act
  cardano_error_t result = cardano_metadatum_label_list_add(nullptr, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}
