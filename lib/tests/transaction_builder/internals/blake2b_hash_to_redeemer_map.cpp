/**
 * \file blake2b_hash_to_redeemer_map.cpp
 *
 * \author angel.castillo
 * \date   Nov 16, 2024
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

#include "../../../src/transaction_builder/internals/blake2b_hash_to_redeemer_map.h"

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/crypto/blake2b_hash.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* HASH1         = "00000000000000000000000000000000000000000000000000000000";
static const char* HASH2         = "10000000000000000000000000000000000000000000000000000000";
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
 * Creates a new default instance of the blake2b hash.
 * @return A new instance of the blake2b hash.
 */
static cardano_blake2b_hash_t*
new_default_blake2b_hash(const char* blake2b_hash)
{
  cardano_blake2b_hash_t* blake2b_hash_obj = NULL;
  cardano_error_t         result           = cardano_blake2b_hash_from_hex(blake2b_hash, strlen(blake2b_hash), &blake2b_hash_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return blake2b_hash_obj;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_blake2b_hash_to_redeemer_map_new, canCreateMap)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(blake2b_hash_to_redeemer_map, testing::Not((cardano_blake2b_hash_to_redeemer_map_t*)nullptr));

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_new, returnsErrorIfProposedParamUpdatesIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_to_redeemer_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_to_redeemer_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_to_redeemer_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_to_redeemer_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_to_redeemer_map_ref(blake2b_hash_to_redeemer_map);

  // Assert
  EXPECT_THAT(blake2b_hash_to_redeemer_map, testing::Not((cardano_blake2b_hash_to_redeemer_map_t*)nullptr));
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_refcount(blake2b_hash_to_redeemer_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_to_redeemer_map_ref(nullptr);
}

TEST(cardano_blake2b_hash_to_redeemer_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;

  // Act
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_to_redeemer_map_unref((cardano_blake2b_hash_to_redeemer_map_t**)nullptr);
}

TEST(cardano_blake2b_hash_to_redeemer_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_to_redeemer_map_ref(blake2b_hash_to_redeemer_map);
  size_t ref_count = cardano_blake2b_hash_to_redeemer_map_refcount(blake2b_hash_to_redeemer_map);

  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  size_t updated_ref_count = cardano_blake2b_hash_to_redeemer_map_refcount(blake2b_hash_to_redeemer_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_to_redeemer_map_ref(blake2b_hash_to_redeemer_map);
  size_t ref_count = cardano_blake2b_hash_to_redeemer_map_refcount(blake2b_hash_to_redeemer_map);

  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  size_t updated_ref_count = cardano_blake2b_hash_to_redeemer_map_refcount(blake2b_hash_to_redeemer_map);

  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_to_redeemer_map_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_blake2b_hash_to_redeemer_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_blake2b_hash_to_redeemer_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  const char*                             message                      = "This is a test message";

  // Act
  cardano_blake2b_hash_to_redeemer_map_set_last_error(blake2b_hash_to_redeemer_map, message);

  // Assert
  EXPECT_STREQ(cardano_blake2b_hash_to_redeemer_map_get_last_error(blake2b_hash_to_redeemer_map), "Object is NULL.");
}

TEST(cardano_blake2b_hash_to_redeemer_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_blake2b_hash_to_redeemer_map_set_last_error(blake2b_hash_to_redeemer_map, message);

  // Assert
  EXPECT_STREQ(cardano_blake2b_hash_to_redeemer_map_get_last_error(blake2b_hash_to_redeemer_map), "");

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_blake2b_hash_to_redeemer_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_size, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash     = new_default_blake2b_hash(HASH1);
  cardano_redeemer_t*     redeemer = new_default_redeemer();

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, ((cardano_blake2b_hash_t*)(void*)hash), redeemer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_blake2b_hash_to_redeemer_map_get_length(blake2b_hash_to_redeemer_map);

  // Assert
  EXPECT_EQ(size, 1);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_blake2b_hash_to_redeemer_map_insert, returnsErrorIfObjectIsNull)
{
  // Act
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_insert(nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_insert((cardano_blake2b_hash_to_redeemer_map_t*)"", nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_insert((cardano_blake2b_hash_to_redeemer_map_t*)"", (cardano_blake2b_hash_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_insert, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash     = new_default_blake2b_hash(HASH1);
  cardano_redeemer_t*     redeemer = new_default_redeemer();

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash, redeemer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get, returnsErrorIfObjectIsNull)
{
  // Act
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get(nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get((cardano_blake2b_hash_to_redeemer_map_t*)"", nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get((cardano_blake2b_hash_to_redeemer_map_t*)"", (cardano_blake2b_hash_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t*     value = NULL;
  cardano_blake2b_hash_t* hash  = new_default_blake2b_hash(HASH1);

  // Act
  error = cardano_blake2b_hash_to_redeemer_map_get(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_unref(&value);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get, returnsTheElement)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash     = new_default_blake2b_hash(HASH1);
  cardano_redeemer_t*     redeemer = new_default_redeemer();

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash, redeemer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value = NULL;
  error                     = cardano_blake2b_hash_to_redeemer_map_get(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, redeemer);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_unref(&redeemer);
  cardano_redeemer_unref(&value);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash1     = new_default_blake2b_hash(HASH1);
  cardano_blake2b_hash_t* hash2     = new_default_blake2b_hash(HASH2);
  cardano_redeemer_t*     redeemer1 = new_default_redeemer();
  cardano_redeemer_t*     redeemer2 = new_default_redeemer();

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash1, redeemer1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash2, redeemer2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value = NULL;
  error                     = cardano_blake2b_hash_to_redeemer_map_get(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash2, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, redeemer2);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
  cardano_redeemer_unref(&redeemer1);
  cardano_redeemer_unref(&redeemer2);
  cardano_redeemer_unref(&value);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get, returnsTheRightElementIfMoreThanOne2)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash1     = new_default_blake2b_hash(HASH1);
  cardano_blake2b_hash_t* hash2     = new_default_blake2b_hash(HASH2);
  cardano_redeemer_t*     redeemer1 = new_default_redeemer();
  cardano_redeemer_t*     redeemer2 = new_default_redeemer();

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash1, redeemer1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash2, redeemer2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value = NULL;
  error                     = cardano_blake2b_hash_to_redeemer_map_get(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash2, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, redeemer2);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
  cardano_redeemer_unref(&redeemer1);
  cardano_redeemer_unref(&redeemer2);
  cardano_redeemer_unref(&value);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* blake2b_hash = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_get_key_at(nullptr, 0, &blake2b_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_get_key_at((cardano_blake2b_hash_to_redeemer_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* blake2b_hash = nullptr;

  // Act
  error = cardano_blake2b_hash_to_redeemer_map_get_key_at(blake2b_hash_to_redeemer_map, 0, &blake2b_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash1     = new_default_blake2b_hash(HASH1);
  cardano_blake2b_hash_t* hash2     = new_default_blake2b_hash(HASH2);
  cardano_redeemer_t*     redeemer1 = new_default_redeemer();
  cardano_redeemer_t*     redeemer2 = new_default_redeemer();

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash1, redeemer1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash2, redeemer2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* blake2b_hash = nullptr;
  error                                = cardano_blake2b_hash_to_redeemer_map_get_key_at(blake2b_hash_to_redeemer_map, 0, &blake2b_hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(blake2b_hash, (cardano_blake2b_hash_t*)(void*)hash1);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
  cardano_redeemer_unref(&redeemer1);
  cardano_redeemer_unref(&redeemer2);
  cardano_blake2b_hash_unref(&blake2b_hash);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_redeemer_t* value = NULL;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_get_value_at(nullptr, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_get_value_at((cardano_blake2b_hash_to_redeemer_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t* value = NULL;

  // Act
  error = cardano_blake2b_hash_to_redeemer_map_get_value_at(blake2b_hash_to_redeemer_map, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t* value = new_default_redeemer();

  cardano_blake2b_hash_t* blake2b_hash = new_default_blake2b_hash(HASH1);

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)blake2b_hash, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* value_out = NULL;
  error                         = cardano_blake2b_hash_to_redeemer_map_get_value_at(blake2b_hash_to_redeemer_map, 0, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&blake2b_hash);
  cardano_redeemer_unref(&value);
  cardano_redeemer_unref(&value_out);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* blake2b_hash = nullptr;
  cardano_redeemer_t*     value        = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_get_key_value_at(nullptr, 0, &blake2b_hash, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_value_at, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_redeemer_t* value = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_get_key_value_at((cardano_blake2b_hash_to_redeemer_map_t*)"", 0, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_value_at, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* blake2b_hash = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_to_redeemer_map_get_key_value_at((cardano_blake2b_hash_to_redeemer_map_t*)"", 0, &blake2b_hash, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* blake2b_hash = nullptr;
  cardano_redeemer_t*     value        = nullptr;

  // Act
  error = cardano_blake2b_hash_to_redeemer_map_get_key_value_at(blake2b_hash_to_redeemer_map, 0, &blake2b_hash, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
}

TEST(cardano_blake2b_hash_to_redeemer_map_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_t* value = new_default_redeemer();

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* blake2b_hash = new_default_blake2b_hash(HASH1);

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)blake2b_hash, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* blake2b_hash_out = nullptr;
  cardano_redeemer_t*     value_out        = nullptr;
  error                                    = cardano_blake2b_hash_to_redeemer_map_get_key_value_at(blake2b_hash_to_redeemer_map, 0, &blake2b_hash_out, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ((cardano_blake2b_hash_t*)blake2b_hash, blake2b_hash_out);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&blake2b_hash);
  cardano_redeemer_unref(&value);
  cardano_redeemer_unref(&value_out);
  cardano_blake2b_hash_unref(&blake2b_hash_out);
}

TEST(cardano_blake2b_hash_to_redeemer_map_update_redeemer_index, returnsErrorIfObjectIsNull)
{
  // Act
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_update_redeemer_index(nullptr, nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_update_redeemer_index((cardano_blake2b_hash_to_redeemer_map_t*)"", nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_to_redeemer_map_update_redeemer_index, doesntReturnErrorIfNotFound)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash     = new_default_blake2b_hash(HASH1);
  cardano_redeemer_t*     redeemer = new_default_redeemer();

  // Act
  error = cardano_blake2b_hash_to_redeemer_map_update_redeemer_index(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_blake2b_hash_to_redeemer_map_update_redeemer_index, updatesIndexIfFound)
{
  // Arrange
  cardano_blake2b_hash_to_redeemer_map_t* blake2b_hash_to_redeemer_map = nullptr;
  cardano_error_t                         error                        = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash     = new_default_blake2b_hash(HASH1);
  cardano_redeemer_t*     redeemer = new_default_redeemer();

  error = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash, redeemer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_to_redeemer_map_update_redeemer_index(blake2b_hash_to_redeemer_map, (cardano_blake2b_hash_t*)(void*)hash, 77);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_get_index(redeemer), 77);

  // Cleanup
  cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_unref(&redeemer);
}