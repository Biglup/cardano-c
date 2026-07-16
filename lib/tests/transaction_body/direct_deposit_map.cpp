/**
 * \file direct_deposit_map.cpp
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
 *
 * Copyright 2026 Biglup Labs
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
#include <cardano/transaction_body/direct_deposit_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* ACCOUNT_A_BECH32 = "stake_test1uqfu74w3wh4gfzu8m6e7j987h4lq9r3t7ef5gaw497uu85qsqfy27";
static const char* ACCOUNT_B_BECH32 = "stake_test1upqykkjq3zhf4085s6n70w8cyp57dl87r0ezduv9rnnj2uqk5zmdv";

static const char* CBOR                = "a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01903e8581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce725701907d0";
static const char* REVERSED_CBOR       = "a2581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce725701907d0581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01903e8";
static const char* SINGLE_ACCOUNT_CBOR = "a1581de100112233445566778899aabbccddeeff00112233445566778899aabb1a000f4240";
static const char* EMPTY_MAP_CBOR      = "a0";
static const char* DUPLICATE_CBOR      = "a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01903e8581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01907d0";

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
 * Decodes the given CBOR hex string into a direct deposit map.
 * @return A new instance of the direct deposit map.
 */
static cardano_direct_deposit_map_t*
new_default_direct_deposit_map(const char* cbor)
{
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_direct_deposit_map_unref(&direct_deposit_map);
    return nullptr;
  }

  return direct_deposit_map;
}

/**
 * Encodes the given direct deposit map to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_direct_deposit_map(cardano_direct_deposit_map_t* direct_deposit_map)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_direct_deposit_map_to_cbor(direct_deposit_map, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_direct_deposit_map_new, canCreateDirectDepositMap)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_new(&direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_new, returnsErrorIfDirectDepositMapIsNull)
{
  // Act
  cardano_error_t error = cardano_direct_deposit_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_new(&direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_direct_deposit_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_new(&direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_direct_deposit_map_from_cbor, canDeserializeDirectDepositMap)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));
  EXPECT_EQ(cardano_direct_deposit_map_get_length(direct_deposit_map), 2);

  cardano_reward_address_t* key1   = nullptr;
  cardano_reward_address_t* key2   = nullptr;
  uint64_t                  value1 = 0;
  uint64_t                  value2 = 0;

  EXPECT_EQ(cardano_direct_deposit_map_get_key_value_at(direct_deposit_map, 0, &key1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_get_key_value_at(direct_deposit_map, 1, &key2, &value2), CARDANO_SUCCESS);

  EXPECT_STREQ(cardano_reward_address_get_string(key1), ACCOUNT_A_BECH32);
  EXPECT_EQ(value1, 1000);

  EXPECT_STREQ(cardano_reward_address_get_string(key2), ACCOUNT_B_BECH32);
  EXPECT_EQ(value2, 2000);

  // Cleanup
  cardano_reward_address_unref(&key1);
  cardano_reward_address_unref(&key2);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfEmptyMap)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(EMPTY_MAP_CBOR, strlen(EMPTY_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_MAP_SIZE);
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'direct_deposit_map', the map must not be empty.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfDuplicatedKey)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(DUPLICATE_CBOR, strlen(DUPLICATE_CBOR));

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'direct_deposit_map', the map must not contain duplicated keys.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfNotAMap)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfInvalidKey)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("a1011903e8", 10);

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfInvalidRewardAddress)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("a144deadbeef1903e8", 18);

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfInvalidValue)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0f6", 66);

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfDirectDepositMapIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(nullptr, &direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_direct_deposit_map_from_cbor(reader, &direct_deposit_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_direct_deposit_map_to_cbor, canRoundTripTwoAccounts)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  // Act
  char* actual_cbor = encode_direct_deposit_map(direct_deposit_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
  free(actual_cbor);
}

TEST(cardano_direct_deposit_map_to_cbor, canRoundTripSingleAccountVector)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(SINGLE_ACCOUNT_CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  EXPECT_EQ(cardano_direct_deposit_map_get_length(direct_deposit_map), 1);

  uint64_t amount = 0;
  EXPECT_EQ(cardano_direct_deposit_map_get_value_at(direct_deposit_map, 0, &amount), CARDANO_SUCCESS);
  EXPECT_EQ(amount, 1000000);

  // Act
  char* actual_cbor = encode_direct_deposit_map(direct_deposit_map);

  // Assert
  EXPECT_STREQ(actual_cbor, SINGLE_ACCOUNT_CBOR);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
  free(actual_cbor);
}

TEST(cardano_direct_deposit_map_to_cbor, preservesKeyInsertionOrder)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(REVERSED_CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  cardano_reward_address_t* key = nullptr;
  EXPECT_EQ(cardano_direct_deposit_map_get_key_at(direct_deposit_map, 0, &key), CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(key), ACCOUNT_B_BECH32);

  // Act
  char* actual_cbor = encode_direct_deposit_map(direct_deposit_map);

  // Assert
  EXPECT_STREQ(actual_cbor, REVERSED_CBOR);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
  free(actual_cbor);
}

TEST(cardano_direct_deposit_map_to_cbor, canSerializeManuallyBuiltMap)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;

  cardano_error_t error = cardano_direct_deposit_map_new(&direct_deposit_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposit_map, ACCOUNT_A_BECH32, strlen(ACCOUNT_A_BECH32), 1000), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposit_map, ACCOUNT_B_BECH32, strlen(ACCOUNT_B_BECH32), 2000), CARDANO_SUCCESS);

  // Act
  char* actual_cbor = encode_direct_deposit_map(direct_deposit_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
  free(actual_cbor);
}

TEST(cardano_direct_deposit_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_direct_deposit_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_direct_deposit_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;

  cardano_error_t error = cardano_direct_deposit_map_new(&direct_deposit_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_direct_deposit_map_to_cbor(direct_deposit_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_length, returnsZeroIfDirectDepositMapIsNull)
{
  // Act
  size_t length = cardano_direct_deposit_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_direct_deposit_map_get_length, returnsZeroIfDirectDepositMapIsEmpty)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_direct_deposit_map_get_length(direct_deposit_map);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get, returnsErrorIfDirectDepositMapIsNull)
{
  // Arrange
  cardano_reward_address_t* key    = new_default_reward_address(ACCOUNT_A_BECH32);
  uint64_t                  amount = 0;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get(nullptr, key, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&key);
}

TEST(cardano_direct_deposit_map_get, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t amount = 0;

  // Act
  error = cardano_direct_deposit_map_get(direct_deposit_map, nullptr, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* key = new_default_reward_address(ACCOUNT_A_BECH32);

  // Act
  error = cardano_direct_deposit_map_get(direct_deposit_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(SINGLE_ACCOUNT_CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  cardano_reward_address_t* key    = new_default_reward_address(ACCOUNT_B_BECH32);
  uint64_t                  amount = 0;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get(direct_deposit_map, key, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get, returnsTheAmountForTheGivenKey)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  cardano_reward_address_t* key    = new_default_reward_address(ACCOUNT_B_BECH32);
  uint64_t                  amount = 0;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get(direct_deposit_map, key, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(amount, 2000);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_insert, returnsErrorIfDirectDepositMapIsNull)
{
  // Arrange
  cardano_reward_address_t* key = new_default_reward_address(ACCOUNT_A_BECH32);

  // Act
  cardano_error_t error = cardano_direct_deposit_map_insert(nullptr, key, 1000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&key);
}

TEST(cardano_direct_deposit_map_insert, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_direct_deposit_map_insert(direct_deposit_map, nullptr, 1000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_insert, returnsErrorIfKeyIsDuplicated)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* key = new_default_reward_address(ACCOUNT_A_BECH32);

  // Act
  EXPECT_EQ(cardano_direct_deposit_map_insert(direct_deposit_map, key, 1000), CARDANO_SUCCESS);
  error = cardano_direct_deposit_map_insert(direct_deposit_map, key, 2000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(cardano_direct_deposit_map_get_length(direct_deposit_map), 1);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_insert, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* key = new_default_reward_address(ACCOUNT_A_BECH32);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_direct_deposit_map_insert(direct_deposit_map, key, 1000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_reward_address_unref(&key);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_insert_ex, returnsErrorIfDirectDepositMapIsNull)
{
  // Act
  cardano_error_t error = cardano_direct_deposit_map_insert_ex(nullptr, ACCOUNT_A_BECH32, strlen(ACCOUNT_A_BECH32), 1000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_insert_ex, returnsErrorIfRewardAddressIsInvalid)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_direct_deposit_map_insert_ex(direct_deposit_map, "invalid_address", strlen("invalid_address"), 1000);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_insert_ex, canInsertADepositEntry)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_direct_deposit_map_insert_ex(direct_deposit_map, ACCOUNT_A_BECH32, strlen(ACCOUNT_A_BECH32), 1000);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_get_length(direct_deposit_map), 1);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_keys, returnsErrorIfDirectDepositMapIsNull)
{
  // Arrange
  cardano_reward_address_list_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get_keys(nullptr, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_get_keys, returnsErrorIfKeysIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_direct_deposit_map_get_keys(direct_deposit_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_keys, returnsTheListOfKeys)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  cardano_reward_address_list_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get_keys(direct_deposit_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_list_get_length(keys), 2);

  cardano_reward_address_t* key = nullptr;
  EXPECT_EQ(cardano_reward_address_list_get(keys, 0, &key), CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(key), ACCOUNT_A_BECH32);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_reward_address_list_unref(&keys);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_key_at, returnsErrorIfDirectDepositMapIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get_key_at(nullptr, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_get_key_at, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_direct_deposit_map_get_key_at(direct_deposit_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;

  // Act
  error = cardano_direct_deposit_map_get_key_at(direct_deposit_map, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  // Act
  cardano_reward_address_t* reward_address = nullptr;
  cardano_error_t           error          = cardano_direct_deposit_map_get_key_at(direct_deposit_map, 1, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(reward_address), ACCOUNT_B_BECH32);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_value_at, returnsErrorIfDirectDepositMapIsNull)
{
  // Arrange
  uint64_t amount = 0;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get_value_at(nullptr, 0, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_get_value_at, returnsErrorIfAmountIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_direct_deposit_map_get_value_at(direct_deposit_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t amount = 0;

  // Act
  error = cardano_direct_deposit_map_get_value_at(direct_deposit_map, 0, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_value_at, canReturnTheRightValue)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  // Act
  uint64_t first_amount  = 0;
  uint64_t second_amount = 0;

  EXPECT_EQ(cardano_direct_deposit_map_get_value_at(direct_deposit_map, 0, &first_amount), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_get_value_at(direct_deposit_map, 1, &second_amount), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(first_amount, 1000);
  EXPECT_EQ(second_amount, 2000);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_key_value_at, returnsErrorIfDirectDepositMapIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;
  uint64_t                  amount         = 0;

  // Act
  cardano_error_t error = cardano_direct_deposit_map_get_key_value_at(nullptr, 0, &reward_address, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_direct_deposit_map_get_key_value_at, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t amount = 0;

  // Act
  error = cardano_direct_deposit_map_get_key_value_at(direct_deposit_map, 0, nullptr, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_key_value_at, returnsErrorIfAmountIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;

  // Act
  error = cardano_direct_deposit_map_get_key_value_at(direct_deposit_map, 0, &reward_address, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;
  uint64_t                  amount         = 0;

  // Act
  error = cardano_direct_deposit_map_get_key_value_at(direct_deposit_map, 0, &reward_address, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_get_key_value_at, canReturnTheRightKeyValuePair)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = new_default_direct_deposit_map(CBOR);
  ASSERT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));

  // Act
  cardano_reward_address_t* reward_address = nullptr;
  uint64_t                  amount         = 0;

  cardano_error_t error = cardano_direct_deposit_map_get_key_value_at(direct_deposit_map, 0, &reward_address, &amount);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(reward_address), ACCOUNT_A_BECH32);
  EXPECT_EQ(amount, 1000);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_direct_deposit_map_ref(direct_deposit_map);

  // Assert
  EXPECT_THAT(direct_deposit_map, testing::Not((cardano_direct_deposit_map_t*)nullptr));
  EXPECT_EQ(cardano_direct_deposit_map_refcount(direct_deposit_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_direct_deposit_map_unref(&direct_deposit_map);
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_direct_deposit_map_ref(nullptr);
}

TEST(cardano_direct_deposit_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;

  // Act
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_direct_deposit_map_unref((cardano_direct_deposit_map_t**)nullptr);
}

TEST(cardano_direct_deposit_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_direct_deposit_map_ref(direct_deposit_map);
  size_t ref_count = cardano_direct_deposit_map_refcount(direct_deposit_map);

  cardano_direct_deposit_map_unref(&direct_deposit_map);
  size_t updated_ref_count = cardano_direct_deposit_map_refcount(direct_deposit_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_direct_deposit_map_ref(direct_deposit_map);
  size_t ref_count = cardano_direct_deposit_map_refcount(direct_deposit_map);

  cardano_direct_deposit_map_unref(&direct_deposit_map);
  size_t updated_ref_count = cardano_direct_deposit_map_refcount(direct_deposit_map);

  cardano_direct_deposit_map_unref(&direct_deposit_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(direct_deposit_map, (cardano_direct_deposit_map_t*)nullptr);

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}

TEST(cardano_direct_deposit_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_direct_deposit_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_direct_deposit_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_direct_deposit_map_set_last_error(direct_deposit_map, message);

  // Assert
  EXPECT_STREQ(cardano_direct_deposit_map_get_last_error(direct_deposit_map), "Object is NULL.");
}

TEST(cardano_direct_deposit_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_direct_deposit_map_t* direct_deposit_map = nullptr;
  cardano_error_t               error              = cardano_direct_deposit_map_new(&direct_deposit_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_direct_deposit_map_set_last_error(direct_deposit_map, message);

  // Assert
  EXPECT_STREQ(cardano_direct_deposit_map_get_last_error(direct_deposit_map), "");

  // Cleanup
  cardano_direct_deposit_map_unref(&direct_deposit_map);
}
