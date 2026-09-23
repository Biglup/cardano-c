/**
 * \file account_balance_intervals_map.cpp
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
#include <cardano/transaction_body/account_balance_interval.h>
#include <cardano/transaction_body/account_balance_intervals_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* ACCOUNT_A_BECH32 = "stake_test1uqfu74w3wh4gfzu8m6e7j987h4lq9r3t7ef5gaw497uu85qsqfy27";
static const char* ACCOUNT_B_BECH32 = "stake_test1upqykkjq3zhf4085s6n70w8cyp57dl87r0ezduv9rnnj2uqk5zmdv";
static const char* ACCOUNT_C_BECH32 = "stake_test17z4thnxaamlsqyfzxdz92enh3zv64w7vmhh07qq3yge5g4g53ps7x";

static const char* BOTH_BOUNDS_INTERVAL_CBOR = "821864191388";
static const char* LOWER_ONLY_INTERVAL_CBOR  = "821901f4f6";
static const char* UPPER_ONLY_INTERVAL_CBOR  = "82f6192710";
static const char* EXACT_INTERVAL_CBOR       = "182a";

static const char* CBOR                 = "a3581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce7257082f6192710581df0aabbccddeeff00112233445566778899aabbccddeeff001122334455182a";
static const char* RANGES_CBOR          = "a3581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570821901f4f6581df0aabbccddeeff00112233445566778899aabbccddeeff00112233445582f6192710";
static const char* REVERSED_CBOR        = "a2581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce7257082f6192710581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388";
static const char* EMPTY_MAP_CBOR       = "a0";
static const char* DUPLICATE_CBOR       = "a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0821864191388581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d082f6192710";
static const char* INVALID_KEY_CBOR     = "a101821864191388";
static const char* INVALID_ADDRESS_CBOR = "a144deadbeef821864191388";
static const char* INVALID_VALUE_CBOR   = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0f6";
static const char* NIL_BOUNDS_CBOR      = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d082f6f6";

static const uint64_t INCLUSIVE_LOWER_BOUND = 100;
static const uint64_t EXCLUSIVE_UPPER_BOUND = 5000;
static const uint64_t UPPER_ONLY_BOUND      = 10000;
static const uint64_t EXACT_BALANCE         = 42;

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
 * Creates a new default instance of the account balance interval.
 * @return A new instance of the account balance interval.
 */
static cardano_account_balance_interval_t*
new_default_account_balance_interval(const char* cbor)
{
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_account_balance_interval_unref(&account_balance_interval);
    return nullptr;
  }

  return account_balance_interval;
}

/**
 * Decodes the given CBOR hex string into an account balance intervals map.
 * @return A new instance of the account balance intervals map.
 */
static cardano_account_balance_intervals_map_t*
new_default_account_balance_intervals_map(const char* cbor)
{
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
    return nullptr;
  }

  return account_balance_intervals_map;
}

/**
 * Encodes the given account balance intervals map to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_account_balance_intervals_map(cardano_account_balance_intervals_map_t* account_balance_intervals_map)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_account_balance_intervals_map_to_cbor(account_balance_intervals_map, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/**
 * Encodes the given account balance interval to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_account_balance_interval(cardano_account_balance_interval_t* account_balance_interval)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_account_balance_interval_to_cbor(account_balance_interval, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_account_balance_intervals_map_new, canCreateAccountBalanceIntervalsMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_new, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_account_balance_intervals_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_account_balance_intervals_map_from_cbor, canDeserializeAccountBalanceIntervalsMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(account_balance_intervals_map), 3);

  cardano_reward_address_t*           key1   = nullptr;
  cardano_reward_address_t*           key2   = nullptr;
  cardano_reward_address_t*           key3   = nullptr;
  cardano_account_balance_interval_t* value1 = nullptr;
  cardano_account_balance_interval_t* value2 = nullptr;
  cardano_account_balance_interval_t* value3 = nullptr;

  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &key1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 1, &key2, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 2, &key3, &value3), CARDANO_SUCCESS);

  EXPECT_STREQ(cardano_reward_address_get_string(key1), ACCOUNT_A_BECH32);
  EXPECT_STREQ(cardano_reward_address_get_string(key2), ACCOUNT_B_BECH32);
  EXPECT_STREQ(cardano_reward_address_get_string(key3), ACCOUNT_C_BECH32);

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(value1);
  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(value1);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, INCLUSIVE_LOWER_BOUND);
  EXPECT_EQ(*exclusive_upper_bound, EXCLUSIVE_UPPER_BOUND);

  EXPECT_EQ(cardano_account_balance_interval_get_inclusive_lower_bound(value2), (const uint64_t*)nullptr);
  exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(value2);

  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*exclusive_upper_bound, UPPER_ONLY_BOUND);

  EXPECT_TRUE(cardano_account_balance_interval_is_exact(value3));

  const uint64_t* exact_balance = cardano_account_balance_interval_get_exact_balance(value3);

  ASSERT_THAT(exact_balance, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*exact_balance, EXACT_BALANCE);

  // Cleanup
  cardano_reward_address_unref(&key1);
  cardano_reward_address_unref(&key2);
  cardano_reward_address_unref(&key3);
  cardano_account_balance_interval_unref(&value1);
  cardano_account_balance_interval_unref(&value2);
  cardano_account_balance_interval_unref(&value3);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfEmptyMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(EMPTY_MAP_CBOR, strlen(EMPTY_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_MAP_SIZE);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'account_balance_intervals_map', the map must not be empty.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfDuplicatedKey)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(DUPLICATE_CBOR, strlen(DUPLICATE_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'account_balance_intervals_map', the map must not contain duplicated keys.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfNotAMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfInvalidKey)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(INVALID_KEY_CBOR, strlen(INVALID_KEY_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfInvalidRewardAddress)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(INVALID_ADDRESS_CBOR, strlen(INVALID_ADDRESS_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfInvalidValue)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(INVALID_VALUE_CBOR, strlen(INVALID_VALUE_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfValueHasBothBoundsNil)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(NIL_BOUNDS_CBOR, strlen(NIL_BOUNDS_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Both interval bounds cannot be nil.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(nullptr, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_to_cbor, canRoundTripAllIntervalShapes)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, canRoundTripAllRangeShapes)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(RANGES_CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, RANGES_CBOR);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, preservesKeyInsertionOrder)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(REVERSED_CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_reward_address_t* key = nullptr;
  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 0, &key), CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(key), ACCOUNT_B_BECH32);

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, REVERSED_CBOR);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, canSerializeManuallyBuiltMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* key_a = new_default_reward_address(ACCOUNT_A_BECH32);
  cardano_reward_address_t* key_b = new_default_reward_address(ACCOUNT_B_BECH32);
  cardano_reward_address_t* key_c = new_default_reward_address(ACCOUNT_C_BECH32);

  cardano_account_balance_interval_t* both_bounds_interval = nullptr;
  cardano_account_balance_interval_t* upper_only_interval  = nullptr;
  cardano_account_balance_interval_t* exact_interval       = nullptr;

  ASSERT_EQ(cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &both_bounds_interval), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_account_balance_interval_new(nullptr, &UPPER_ONLY_BOUND, &upper_only_interval), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_account_balance_interval_new_exact(EXACT_BALANCE, &exact_interval), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key_a, both_bounds_interval), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key_b, upper_only_interval), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key_c, exact_interval), CARDANO_SUCCESS);

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_reward_address_unref(&key_a);
  cardano_reward_address_unref(&key_b);
  cardano_reward_address_unref(&key_c);
  cardano_account_balance_interval_unref(&both_bounds_interval);
  cardano_account_balance_interval_unref(&upper_only_interval);
  cardano_account_balance_interval_unref(&exact_interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, canSerializeMapBuiltFromBech32Keys)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* both_bounds_interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);
  cardano_account_balance_interval_t* lower_only_interval  = new_default_account_balance_interval(LOWER_ONLY_INTERVAL_CBOR);
  cardano_account_balance_interval_t* upper_only_interval  = new_default_account_balance_interval(UPPER_ONLY_INTERVAL_CBOR);

  EXPECT_EQ(cardano_account_balance_intervals_map_insert_ex(account_balance_intervals_map, ACCOUNT_A_BECH32, strlen(ACCOUNT_A_BECH32), both_bounds_interval), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_insert_ex(account_balance_intervals_map, ACCOUNT_B_BECH32, strlen(ACCOUNT_B_BECH32), lower_only_interval), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_insert_ex(account_balance_intervals_map, ACCOUNT_C_BECH32, strlen(ACCOUNT_C_BECH32), upper_only_interval), CARDANO_SUCCESS);

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, RANGES_CBOR);

  // Cleanup
  cardano_account_balance_interval_unref(&both_bounds_interval);
  cardano_account_balance_interval_unref(&lower_only_interval);
  cardano_account_balance_interval_unref(&upper_only_interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_account_balance_intervals_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_to_cbor(account_balance_intervals_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_length, returnsZeroIfAccountBalanceIntervalsMapIsNull)
{
  // Act
  size_t length = cardano_account_balance_intervals_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_account_balance_intervals_map_get_length, returnsZeroIfAccountBalanceIntervalsMapIsEmpty)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_account_balance_intervals_map_get_length(account_balance_intervals_map);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_reward_address_t*           key      = new_default_reward_address(ACCOUNT_A_BECH32);
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get(nullptr, key, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&key);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, nullptr, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* key = new_default_reward_address(ACCOUNT_A_BECH32);

  // Act
  error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(REVERSED_CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_reward_address_t*           key      = new_default_reward_address(ACCOUNT_C_BECH32);
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsIntervalForRewardAddress)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_reward_address_t*           key      = new_default_reward_address(ACCOUNT_B_BECH32);
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  char* interval_cbor = encode_account_balance_interval(interval);
  EXPECT_STREQ(interval_cbor, UPPER_ONLY_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_reward_address_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsExactIntervalForRewardAddress)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_reward_address_t*           key      = new_default_reward_address(ACCOUNT_C_BECH32);
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(interval, testing::Not((cardano_account_balance_interval_t*)nullptr));
  EXPECT_TRUE(cardano_account_balance_interval_is_exact(interval));

  char* interval_cbor = encode_account_balance_interval(interval);
  EXPECT_STREQ(interval_cbor, EXACT_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_reward_address_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_reward_address_t*           key      = new_default_reward_address(ACCOUNT_A_BECH32);
  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_insert(nullptr, key, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, nullptr, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* key = new_default_reward_address(ACCOUNT_A_BECH32);

  // Act
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfKeyIsDuplicated)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t*           key      = new_default_reward_address(ACCOUNT_A_BECH32);
  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, interval), CARDANO_SUCCESS);
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(account_balance_intervals_map), 1);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t*           key      = new_default_reward_address(ACCOUNT_A_BECH32);
  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_reward_address_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert_ex, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_insert_ex(nullptr, ACCOUNT_A_BECH32, strlen(ACCOUNT_A_BECH32), interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_account_balance_intervals_map_insert_ex, returnsErrorIfRewardAddressIsInvalid)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  error = cardano_account_balance_intervals_map_insert_ex(account_balance_intervals_map, "invalid_address", strlen("invalid_address"), interval);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(account_balance_intervals_map), 0);

  // Cleanup
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert_ex, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_insert_ex(account_balance_intervals_map, ACCOUNT_A_BECH32, strlen(ACCOUNT_A_BECH32), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert_ex, canInsertAnIntervalEntry)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(EXACT_INTERVAL_CBOR);

  // Act
  error = cardano_account_balance_intervals_map_insert_ex(account_balance_intervals_map, ACCOUNT_A_BECH32, strlen(ACCOUNT_A_BECH32), interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(account_balance_intervals_map), 1);

  cardano_reward_address_t* key = nullptr;
  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 0, &key), CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(key), ACCOUNT_A_BECH32);

  // Cleanup
  cardano_reward_address_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_reward_address_list_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_keys(nullptr, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsErrorIfKeysIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_get_keys(account_balance_intervals_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsEmptyListIfNoElements)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_list_t* keys = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_keys(account_balance_intervals_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_list_get_length(keys), 0);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_reward_address_list_unref(&keys);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsTheKeysInInsertionOrder)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_reward_address_list_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_keys(account_balance_intervals_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_list_get_length(keys), 3);

  const char* expected_accounts[] = { ACCOUNT_A_BECH32, ACCOUNT_B_BECH32, ACCOUNT_C_BECH32 };

  for (size_t i = 0; i < 3; ++i)
  {
    cardano_reward_address_t* key = nullptr;
    EXPECT_EQ(cardano_reward_address_list_get(keys, i, &key), CARDANO_SUCCESS);

    EXPECT_STREQ(cardano_reward_address_get_string(key), expected_accounts[i]);

    cardano_reward_address_unref(&key);
  }

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_reward_address_list_unref(&keys);
}

TEST(cardano_account_balance_intervals_map_get_key_at, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_key_at(nullptr, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_key_at, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  cardano_reward_address_t* reward_address = nullptr;
  cardano_error_t           error          = cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 2, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(reward_address), ACCOUNT_C_BECH32);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_value_at, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_value_at(nullptr, 0, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_value_at, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_get_value_at(account_balance_intervals_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_value_at(account_balance_intervals_map, 0, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_value_at, canReturnTheRightValue)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  cardano_account_balance_interval_t* interval = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_get_value_at(account_balance_intervals_map, 1, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  char* interval_cbor = encode_account_balance_interval(interval);
  EXPECT_STREQ(interval_cbor, UPPER_ONLY_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_reward_address_t*           reward_address = nullptr;
  cardano_account_balance_interval_t* interval       = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_key_value_at(nullptr, 0, &reward_address, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, nullptr, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &reward_address, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t*           reward_address = nullptr;
  cardano_account_balance_interval_t* interval       = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &reward_address, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, canReturnTheRightKeyValuePair)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  cardano_reward_address_t*           reward_address = nullptr;
  cardano_account_balance_interval_t* interval       = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &reward_address, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_reward_address_get_string(reward_address), ACCOUNT_A_BECH32);

  char* interval_cbor = encode_account_balance_interval(interval);
  EXPECT_STREQ(interval_cbor, BOTH_BOUNDS_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_reward_address_unref(&reward_address);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_intervals_map_ref(account_balance_intervals_map);

  // Assert
  EXPECT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));
  EXPECT_EQ(cardano_account_balance_intervals_map_refcount(account_balance_intervals_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_account_balance_intervals_map_ref(nullptr);
}

TEST(cardano_account_balance_intervals_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_account_balance_intervals_map_unref((cardano_account_balance_intervals_map_t**)nullptr);
}

TEST(cardano_account_balance_intervals_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_intervals_map_ref(account_balance_intervals_map);
  size_t ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  size_t updated_ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_intervals_map_ref(account_balance_intervals_map);
  size_t ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  size_t updated_ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_account_balance_intervals_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_account_balance_intervals_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  const char*                              message                       = "This is a test message";

  // Act
  cardano_account_balance_intervals_map_set_last_error(account_balance_intervals_map, message);

  // Assert
  EXPECT_STREQ(cardano_account_balance_intervals_map_get_last_error(account_balance_intervals_map), "Object is NULL.");
}

TEST(cardano_account_balance_intervals_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_account_balance_intervals_map_set_last_error(account_balance_intervals_map, message);

  // Assert
  EXPECT_STREQ(cardano_account_balance_intervals_map_get_last_error(account_balance_intervals_map), "");

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfGrowingTheArrayFails)
{
  // Arrange
  cardano_account_balance_intervals_map_t* map      = nullptr;
  cardano_account_balance_interval_t*      interval = new_default_account_balance_interval(EXACT_INTERVAL_CBOR);

  EXPECT_EQ(cardano_account_balance_intervals_map_new(&map), CARDANO_SUCCESS);

  for (size_t i = 0U; i < 31U; ++i)
  {
    byte_t hash_bytes[28]        = { 0 };
    hash_bytes[0]                = (byte_t)i;
    cardano_blake2b_hash_t* hash = nullptr;

    EXPECT_EQ(cardano_blake2b_hash_from_bytes(hash_bytes, sizeof(hash_bytes), &hash), CARDANO_SUCCESS);

    cardano_credential_t* credential = nullptr;

    EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);

    cardano_reward_address_t* address = nullptr;

    EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, credential, &address), CARDANO_SUCCESS);

    EXPECT_EQ(cardano_account_balance_intervals_map_insert(map, address, interval), CARDANO_SUCCESS);

    cardano_blake2b_hash_unref(&hash);
    cardano_credential_unref(&credential);
    cardano_reward_address_unref(&address);
  }

  const size_t i = 31U;

  byte_t hash_bytes[28]        = { 0 };
  hash_bytes[0]                = (byte_t)i;
  cardano_blake2b_hash_t* hash = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_bytes(hash_bytes, sizeof(hash_bytes), &hash), CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);

  cardano_reward_address_t* address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, credential, &address), CARDANO_SUCCESS);

  const size_t address_ref_count  = cardano_reward_address_refcount(address);
  const size_t interval_ref_count = cardano_account_balance_interval_refcount(interval);

  reset_allocators_run_count();
  set_realloc_limit(0);
  cardano_set_allocators(malloc, fail_realloc_at_limit, free);

  // Act
  cardano_error_t result = cardano_account_balance_intervals_map_insert(map, address, interval);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(map), 31U);
  EXPECT_EQ(cardano_reward_address_refcount(address), address_ref_count);
  EXPECT_EQ(cardano_account_balance_interval_refcount(interval), interval_ref_count);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  reset_limited_realloc();
  cardano_account_balance_intervals_map_unref(&map);
  cardano_blake2b_hash_unref(&hash);
  cardano_credential_unref(&credential);
  cardano_reward_address_unref(&address);
  cardano_account_balance_interval_unref(&interval);
}
