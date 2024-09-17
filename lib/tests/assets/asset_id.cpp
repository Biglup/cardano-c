/**
 * \file asset_id.cpp
 *
 * \author angel.castillo
 * \date   Sep 14, 2024
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
#include <cardano/crypto/blake2b_hash.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char*  ASSET_ID_HEX      = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b6572";
static const char*  POLICY_ID_HEX     = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const byte_t POLICY_ID_BYTES[] = {
  // clang-format off
  0xf0, 0xff, 0x48, 0xbb, 0xb7, 0xbb, 0xe9, 0xd5, 0x9a, 0x40, 0xf1, 0xce, 0x90, 0xe9, 0xe9, 0xd0, 0xff, 0x50, 0x02, 0xec, 0x48, 0xf2, 0x32, 0xb4, 0x9c, 0xa0, 0xfb, 0x9a
  // clang-format on
};
static const byte_t ASSET_ID_BYTES[] = {
  // clang-format off
  0xf0, 0xff, 0x48, 0xbb, 0xb7, 0xbb, 0xe9, 0xd5, 0x9a, 0x40, 0xf1, 0xce, 0x90, 0xe9, 0xe9, 0xd0, 0xff, 0x50, 0x02, 0xec, 0x48, 0xf2, 0x32, 0xb4, 0x9c, 0xa0, 0xfb, 0x9a, 0x73, 0x6b, 0x79, 0x77, 0x61, 0x6c, 0x6b, 0x65, 0x72
  // clang-format on
};
static const char* INVALID_POLICY_ID_HEX = "e9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* ASSET_NAME            = "skywalker";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the asset_id.
 * @return A new instance of the asset_id.
 */
static cardano_asset_id_t*
new_default_asset_id()
{
  cardano_asset_id_t* asset_id = NULL;
  cardano_error_t     result   = cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return asset_id;
};

/**
 * Creates a new default instance of the asset name.
 * @return A new instance of the asset name.
 */
static cardano_asset_name_t*
new_default_asset_name()
{
  cardano_asset_name_t* asset_name = NULL;
  cardano_error_t       result     = cardano_asset_name_from_string(ASSET_NAME, strlen(ASSET_NAME), &asset_name);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return asset_name;
}

/**
 * Creates a new default instance of the policy id.
 * @return A new instance of the policy id.
 */
static cardano_blake2b_hash_t*
new_default_policy_id()
{
  cardano_blake2b_hash_t* policy_id = NULL;
  cardano_error_t         result    = cardano_blake2b_hash_from_hex(POLICY_ID_HEX, strlen(POLICY_ID_HEX), &policy_id);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return policy_id;
}

/**
 * Creates a new default instance of the policy id.
 * @return A new instance of the policy id.
 */
static cardano_blake2b_hash_t*
invalid_policy_id()
{
  cardano_blake2b_hash_t* policy_id = NULL;
  cardano_error_t         result    = cardano_blake2b_hash_from_hex(INVALID_POLICY_ID_HEX, strlen(INVALID_POLICY_ID_HEX), &policy_id);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return policy_id;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_asset_id_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  cardano_asset_id_ref(asset_id);

  // Assert
  EXPECT_THAT(asset_id, testing::Not((cardano_asset_id_t*)nullptr));
  EXPECT_EQ(cardano_asset_id_refcount(asset_id), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_asset_id_unref(&asset_id);
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_id_ref(nullptr);
}

TEST(cardano_asset_id_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_asset_id_t* asset_id = nullptr;

  // Act
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_id_unref((cardano_asset_id_t**)nullptr);
}

TEST(cardano_asset_id_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  cardano_asset_id_ref(asset_id);
  size_t ref_count = cardano_asset_id_refcount(asset_id);

  cardano_asset_id_unref(&asset_id);
  size_t updated_ref_count = cardano_asset_id_refcount(asset_id);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  cardano_asset_id_ref(asset_id);
  size_t ref_count = cardano_asset_id_refcount(asset_id);

  cardano_asset_id_unref(&asset_id);
  size_t updated_ref_count = cardano_asset_id_refcount(asset_id);

  cardano_asset_id_unref(&asset_id);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(asset_id, (cardano_asset_id_t*)nullptr);

  // Cleanup
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_asset_id_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_asset_id_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_asset_id_t* asset_id = nullptr;
  const char*         message  = "This is a test message";

  // Act
  cardano_asset_id_set_last_error(asset_id, message);

  // Assert
  EXPECT_STREQ(cardano_asset_id_get_last_error(asset_id), "Object is NULL.");
}

TEST(cardano_asset_id_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  const char* message = nullptr;

  // Act
  cardano_asset_id_set_last_error(asset_id, message);

  // Assert
  EXPECT_STREQ(cardano_asset_id_get_last_error(asset_id), "");

  // Cleanup
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_new, returnsErrorIfPolicyIdIsNull)
{
  // Arrange
  cardano_asset_name_t* asset_name = new_default_asset_name();

  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_new(nullptr, asset_name, &asset_id);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_id_new, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* policy_id = new_default_policy_id();

  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_new(policy_id, nullptr, &asset_id);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_asset_id_new, returnsErrorIfAssetIdIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* policy_id  = new_default_policy_id();
  cardano_asset_name_t*   asset_name = new_default_asset_name();

  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_new(policy_id, asset_name, nullptr);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_id_new, returnsErrorIfInvalidHashSize)
{
  // Arrange
  cardano_blake2b_hash_t* policy_id  = invalid_policy_id();
  cardano_asset_name_t*   asset_name = new_default_asset_name();

  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_new(policy_id, asset_name, &asset_id);

  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Assert
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_id_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* policy_id  = new_default_policy_id();
  cardano_asset_name_t*   asset_name = new_default_asset_name();

  // Act
  cardano_asset_id_t* asset_id = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_asset_id_new(policy_id, asset_name, &asset_id);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_id_new_lovelace, returnsLovelaceAssetId)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_new_lovelace(&asset_id);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_THAT(asset_id, testing::Not((cardano_asset_id_t*)nullptr));
  EXPECT_EQ(cardano_asset_id_is_lovelace(asset_id), true);

  // Cleanup
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_new_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_asset_id_new_lovelace(&asset_id);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_id_new_lovelace, returnsErrorIfAssetIdIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_id_new_lovelace(NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_id_from_bytes, returnsErrorIfDataIsNull)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_from_bytes(nullptr, 1, &asset_id);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_id, nullptr);
}

TEST(cardano_asset_id_from_bytes, returnsErrorIfDataSizeIsZero)
{
  // Arrange
  byte_t data[] = { 0 };

  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_from_bytes(data, 0, &asset_id);

  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Assert
  EXPECT_EQ(asset_id, nullptr);
}

TEST(cardano_asset_id_from_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_asset_id_from_bytes(POLICY_ID_BYTES, sizeof(POLICY_ID_BYTES), &asset_id);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_id_from_bytes, returnsErrorIfMemoryAllocationFails1)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  cardano_error_t result = cardano_asset_id_from_bytes(POLICY_ID_BYTES, sizeof(POLICY_ID_BYTES), &asset_id);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_id_from_bytes, returnsErrorIfMemoryAllocationFails2)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_four_malloc, realloc, free);

  cardano_error_t result = cardano_asset_id_from_bytes(POLICY_ID_BYTES, sizeof(POLICY_ID_BYTES), &asset_id);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_id, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_id_from_bytes, returnsErrorIfAssetIdIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_id_from_bytes(POLICY_ID_BYTES, sizeof(POLICY_ID_BYTES), nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_asset_id_from_hex, returnsErrorIfDataIsNull)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_from_hex(nullptr, 1, &asset_id);

  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(asset_id, nullptr);
}

TEST(cardano_asset_id_from_hex, returnsErrorIfDataSizeIsZero)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_from_hex("", 0, &asset_id);

  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Assert
  EXPECT_EQ(asset_id, nullptr);
}

TEST(cardano_asset_id_from_hex, returnsErrorIfHexIsNotDivisibleBy2)
{
  // Act
  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t result = cardano_asset_id_from_hex("f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657", strlen("f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a736b7977616c6b657"), &asset_id);

  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Assert
  EXPECT_EQ(asset_id, nullptr);
}

TEST(cardano_asset_id_from_hex, returnsErrorIfAssetIdIsNull)
{
  // Act
  cardano_error_t result = cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Getters and Setters

TEST(cardano_asset_id_get_bytes, returnsTheBytes)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  const byte_t* bytes = cardano_asset_id_get_bytes(asset_id);
  const size_t  size  = cardano_asset_id_get_bytes_size(asset_id);

  // Assert
  EXPECT_EQ(size, sizeof(ASSET_ID_BYTES));
  EXPECT_THAT(bytes, testing::Not((byte_t*)nullptr));

  for (size_t i = 0; i < size; i++)
  {
    EXPECT_EQ(bytes[i], ASSET_ID_BYTES[i]);
  }

  // Cleanup
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_get_bytes, returnsNullIfAssetIdIsNull)
{
  // Act
  const byte_t* bytes = cardano_asset_id_get_bytes(nullptr);

  // Assert
  EXPECT_EQ(bytes, nullptr);
}

TEST(cardano_asset_id_get_bytes_size, returnsZeroIfAssetIdIsNull)
{
  // Act
  size_t size = cardano_asset_id_get_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_asset_id_get_hex, returnsTheHex)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  const char* hex  = cardano_asset_id_get_hex(asset_id);
  size_t      size = cardano_asset_id_get_hex_size(asset_id);

  // Assert
  EXPECT_EQ(size, strlen(ASSET_ID_HEX) + 1);
  EXPECT_THAT(hex, testing::Not((char*)nullptr));
  EXPECT_STREQ(hex, ASSET_ID_HEX);

  // Cleanup
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_get_hex, returnsNullIfAssetIdIsNull)
{
  // Act
  const char* hex = cardano_asset_id_get_hex(nullptr);

  // Assert
  EXPECT_EQ(hex, nullptr);
}

TEST(cardano_asset_id_get_hex_size, returnsZeroIfAssetIdIsNull)
{
  // Act
  size_t size = cardano_asset_id_get_hex_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_asset_id_is_lovelace, returnsFalseIfAssetIdIsNull)
{
  // Act
  bool is_lovelace = cardano_asset_id_is_lovelace(nullptr);

  // Assert
  EXPECT_EQ(is_lovelace, false);
}

TEST(cardano_asset_id_get_asset_name, returnsTheAssetName)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  cardano_asset_name_t* asset_name = cardano_asset_id_get_asset_name(asset_id);

  // Assert
  EXPECT_THAT(asset_name, testing::Not((cardano_asset_name_t*)nullptr));
  EXPECT_STREQ(cardano_asset_name_get_string(asset_name), ASSET_NAME);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_get_policy_id, returnsThePolicyId)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  cardano_blake2b_hash_t* policy_id = cardano_asset_id_get_policy_id(asset_id);

  // Assert
  EXPECT_THAT(policy_id, testing::Not((cardano_blake2b_hash_t*)nullptr));

  // compare
  EXPECT_EQ(memcmp(cardano_blake2b_hash_get_data(policy_id), POLICY_ID_BYTES, sizeof(POLICY_ID_BYTES)), 0);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_asset_id_get_policy_id, returnsNullIfAssetIdIsNull)
{
  // Act
  cardano_blake2b_hash_t* policy_id = cardano_asset_id_get_policy_id(nullptr);

  // Assert
  EXPECT_EQ(policy_id, nullptr);
}

TEST(cardano_asset_id_get_asset_name, returnsNullIfAssetIdIsNull)
{
  // Act
  cardano_asset_name_t* asset_name = cardano_asset_id_get_asset_name(nullptr);

  // Assert
  EXPECT_EQ(asset_name, nullptr);
}

TEST(cardano_asset_id_get_asset_name, canGetAssetNAme)
{
  // Arrange
  cardano_asset_id_t* asset_id = new_default_asset_id();
  EXPECT_NE(asset_id, nullptr);

  // Act
  cardano_asset_name_t* asset_name = cardano_asset_id_get_asset_name(asset_id);

  // Assert
  EXPECT_THAT(asset_name, testing::Not((cardano_asset_name_t*)nullptr));
  EXPECT_STREQ(cardano_asset_name_get_string(asset_name), ASSET_NAME);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
  cardano_asset_id_unref(&asset_id);
}
