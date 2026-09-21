/**
 * \file builder_entities.cpp
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
 *
 * \section LICENSE
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

#include "../../../src/transaction_builder/internals/builder_entities.h"

#include <cardano/transaction_body/account_balance_intervals_map.h>
#include <cardano/transaction_body/direct_deposit_map.h>
#include <cardano/transaction_body/transaction_body.h>

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";

static const char* REWARD_ADDRESS         = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
static const char* REWARD_ADDRESS2        = "stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn";
static const char* INVALID_REWARD_ADDRESS = "stake_test1invalid";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the protocol parameters.
 * @return A new instance of the protocol parameters.
 */
static cardano_protocol_parameters_t*
init_protocol_parameters()
{
  cardano_protocol_parameters_t* params = NULL;

  cardano_error_t result = cardano_protocol_parameters_new(&params);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(COSTMDLS_ALL_CBOR, strlen(COSTMDLS_ALL_CBOR));

  cardano_costmdls_t* costmdls = NULL;
  result                       = cardano_costmdls_from_cbor(reader, &costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_cost_models(params, costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
  cardano_costmdls_unref(&costmdls);

  return params;
}

/**
 * Creates a reward address from its bech32 representation.
 * @param bech32 The bech32 string of the reward address.
 * @return A new instance of the reward address.
 */
static cardano_reward_address_t*
new_reward_address(const char* bech32)
{
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(cardano_reward_address_from_bech32(bech32, strlen(bech32), &reward_address), CARDANO_SUCCESS);

  return reward_address;
}

/**
 * Creates an account balance interval that requires an exact balance.
 * @param balance The exact balance in lovelace.
 * @return A new instance of the account balance interval.
 */
static cardano_account_balance_interval_t*
new_exact_interval(const uint64_t balance)
{
  cardano_account_balance_interval_t* interval = NULL;

  EXPECT_EQ(cardano_account_balance_interval_new_exact(balance, &interval), CARDANO_SUCCESS);

  return interval;
}

/**
 * Gets a borrowed reference to the direct deposit map of the transaction body.
 * @param state The builder state holding the transaction.
 * @return The direct deposit map of the body, or NULL when it was not created.
 */
static cardano_direct_deposit_map_t*
get_direct_deposits(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_direct_deposit_map_t* deposits = cardano_transaction_body_get_direct_deposits(body);
  cardano_direct_deposit_map_unref(&deposits);

  return deposits;
}

/**
 * Gets a borrowed reference to the account balance intervals map of the transaction body.
 * @param state The builder state holding the transaction.
 * @return The account balance intervals map of the body, or NULL when it was not created.
 */
static cardano_account_balance_intervals_map_t*
get_account_balance_intervals(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_account_balance_intervals_map_t* intervals = cardano_transaction_body_get_account_balance_intervals(body);
  cardano_account_balance_intervals_map_unref(&intervals);

  return intervals;
}

/**
 * Gets a borrowed reference to the starting account balance intervals map of the transaction body.
 * @param state The builder state holding the transaction.
 * @return The starting account balance intervals map of the body, or NULL when it was not created.
 */
static cardano_account_balance_intervals_map_t*
get_starting_account_balance_intervals(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_account_balance_intervals_map_t* intervals = cardano_transaction_body_get_starting_account_balance_intervals(body);
  cardano_account_balance_intervals_map_unref(&intervals);

  return intervals;
}

/**
 * Gets the amount deposited to a reward account.
 * @param deposits The direct deposit map.
 * @param reward_address The reward account to look up.
 * @return The deposited amount, or zero when the account has no direct deposit.
 */
static uint64_t
get_deposit(cardano_direct_deposit_map_t* deposits, cardano_reward_address_t* reward_address)
{
  uint64_t amount = 0U;

  if (cardano_direct_deposit_map_get(deposits, reward_address, &amount) != CARDANO_SUCCESS)
  {
    return 0U;
  }

  return amount;
}

/**
 * Gets a borrowed reference to the interval stored at the given position of the map.
 * @param intervals The account balance intervals map.
 * @param index The position of the entry.
 * @return The interval at that position, or NULL when the position is out of bounds.
 */
static cardano_account_balance_interval_t*
get_interval_at(cardano_account_balance_intervals_map_t* intervals, const size_t index)
{
  cardano_account_balance_interval_t* interval = NULL;

  if (cardano_account_balance_intervals_map_get_value_at(intervals, index, &interval) != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_account_balance_interval_unref(&interval);

  return interval;
}

/**
 * Checks whether the reward address stored at the given position of a direct deposit map matches.
 * @param deposits The direct deposit map.
 * @param index The position of the entry.
 * @param bech32 The expected reward address.
 * @return true if the key at that position is the expected reward address.
 */
static bool
deposit_key_at_is(cardano_direct_deposit_map_t* deposits, const size_t index, const char* bech32)
{
  cardano_reward_address_t* key = NULL;

  if (cardano_direct_deposit_map_get_key_at(deposits, index, &key) != CARDANO_SUCCESS)
  {
    return false;
  }

  const bool matches = strcmp(cardano_reward_address_get_string(key), bech32) == 0;

  cardano_reward_address_unref(&key);

  return matches;
}

/**
 * Checks whether the reward address stored at the given position of an intervals map matches.
 * @param intervals The account balance intervals map.
 * @param index The position of the entry.
 * @param bech32 The expected reward address.
 * @return true if the key at that position is the expected reward address.
 */
static bool
interval_key_at_is(cardano_account_balance_intervals_map_t* intervals, const size_t index, const char* bech32)
{
  cardano_reward_address_t* key = NULL;

  if (cardano_account_balance_intervals_map_get_key_at(intervals, index, &key) != CARDANO_SUCCESS)
  {
    return false;
  }

  const bool matches = strcmp(cardano_reward_address_get_string(key), bech32) == 0;

  cardano_reward_address_unref(&key);

  return matches;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_builder_add_direct_deposit, canAddDirectDeposit)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Act
  const cardano_error_t result = cardano_builder_add_direct_deposit(&state, reward_address, 2000000U, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(error_message, nullptr);
  EXPECT_EQ(cardano_direct_deposit_map_get_length(get_direct_deposits(&state)), 1U);
  EXPECT_EQ(get_deposit(get_direct_deposits(&state), reward_address), 2000000U);
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_direct_deposit, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_direct_deposit(&state, nullptr, 2000000U, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL.");
  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_direct_deposit, returnsErrorIfAmountIsZero)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_direct_deposit(&state, reward_address, 0U, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(error_message, "Direct deposit amount must be greater than zero.");
  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_direct_deposit, accumulatesAmountsOfTheSameRewardAccount)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address  = new_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t* reward_address2 = new_reward_address(REWARD_ADDRESS2);
  const char*               error_message   = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address, 1000U, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address2, 50U, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address, 234U, &error_message), CARDANO_SUCCESS);

  // Assert
  cardano_direct_deposit_map_t* deposits = get_direct_deposits(&state);

  EXPECT_EQ(cardano_direct_deposit_map_get_length(deposits), 2U);
  EXPECT_EQ(get_deposit(deposits, reward_address), 1234U);
  EXPECT_EQ(get_deposit(deposits, reward_address2), 50U);
  EXPECT_TRUE(deposit_key_at_is(deposits, 0U, REWARD_ADDRESS));
  EXPECT_TRUE(deposit_key_at_is(deposits, 1U, REWARD_ADDRESS2));

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address2);
}

TEST(cardano_builder_add_direct_deposit, canAccumulateUpToTheMaximumAmount)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address, UINT64_MAX - 1U, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address, 1U, &error_message), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(get_deposit(get_direct_deposits(&state), reward_address), UINT64_MAX);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_direct_deposit, returnsErrorIfAccumulatedAmountOverflows)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address, UINT64_MAX, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_direct_deposit(&state, reward_address, 1U, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(error_message, "Direct deposit amount overflows.");
  EXPECT_EQ(cardano_direct_deposit_map_get_length(get_direct_deposits(&state)), 1U);
  EXPECT_EQ(get_deposit(get_direct_deposits(&state), reward_address), UINT64_MAX);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_direct_deposit, leavesTheTransactionUnchangedWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_direct_deposit(&state, reward_address, 1000U, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(get_deposit(get_direct_deposits(&state), reward_address), 1000U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(get_direct_deposits(&state), nullptr);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_direct_deposit, keepsThePreviousAmountsWhenAllocationFailsWhileAccumulating)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address  = new_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t* reward_address2 = new_reward_address(REWARD_ADDRESS2);
  const char*               error_message   = NULL;

  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address, 1000U, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_direct_deposit(&state, reward_address2, 50U, &error_message), CARDANO_SUCCESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_direct_deposit(&state, reward_address2, 25U, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_direct_deposit_map_t* deposits = get_direct_deposits(&state);

    EXPECT_EQ(cardano_direct_deposit_map_get_length(deposits), 2U);
    EXPECT_EQ(get_deposit(deposits, reward_address), 1000U);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(get_deposit(deposits, reward_address2), 75U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(get_deposit(deposits, reward_address2), 50U);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address2);
}

TEST(cardano_builder_add_direct_deposit_ex, canAddDirectDeposit)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_direct_deposit_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 1000U, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_direct_deposit_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 234U, &error_message), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error_message, nullptr);
  EXPECT_EQ(cardano_direct_deposit_map_get_length(get_direct_deposits(&state)), 1U);
  EXPECT_EQ(get_deposit(get_direct_deposits(&state), reward_address), 1234U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_direct_deposit_ex, returnsErrorIfRewardAddressIsNullOrEmpty)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act & Assert
  EXPECT_EQ(cardano_builder_add_direct_deposit_ex(&state, nullptr, 0U, 1000U, &error_message), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL or empty.");

  error_message = NULL;

  EXPECT_EQ(cardano_builder_add_direct_deposit_ex(&state, REWARD_ADDRESS, 0U, 1000U, &error_message), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL or empty.");
  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_direct_deposit_ex, returnsErrorIfRewardAddressIsInvalid)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_direct_deposit_ex(&state, INVALID_REWARD_ADDRESS, strlen(INVALID_REWARD_ADDRESS), 1000U, &error_message);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(error_message, "Failed to parse reward address.");
  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_direct_deposit_ex, returnsErrorIfAmountIsZero)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_direct_deposit_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 0U, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(error_message, "Direct deposit amount must be greater than zero.");
  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_direct_deposit_ex, leavesTheTransactionUnchangedWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_direct_deposit_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 1000U, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_direct_deposit_map_get_length(get_direct_deposits(&state)), 1U);
    }
    else
    {
      EXPECT_NE(result, CARDANO_SUCCESS);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(get_direct_deposits(&state), nullptr);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_account_balance_interval, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t*           reward_address = new_reward_address(REWARD_ADDRESS);
  cardano_account_balance_interval_t* interval       = new_exact_interval(5000000U);
  const char*                         error_message  = NULL;

  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);

  // Act
  const cardano_error_t result = cardano_builder_add_account_balance_interval(&state, reward_address, interval, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(error_message, nullptr);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(get_account_balance_intervals(&state)), 1U);
  EXPECT_TRUE(interval_key_at_is(get_account_balance_intervals(&state), 0U, REWARD_ADDRESS));
  EXPECT_EQ(get_interval_at(get_account_balance_intervals(&state), 0U), interval);
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);
  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_account_balance_interval, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_account_balance_interval(&state, nullptr, interval, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL.");
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_account_balance_interval, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_account_balance_interval(&state, reward_address, nullptr, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Account balance interval is NULL.");
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_account_balance_interval, replacesTheIntervalOfTheSameRewardAccount)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t*           reward_address  = new_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t*           reward_address2 = new_reward_address(REWARD_ADDRESS2);
  cardano_account_balance_interval_t* first           = new_exact_interval(1U);
  cardano_account_balance_interval_t* second          = new_exact_interval(2U);
  cardano_account_balance_interval_t* replacement     = new_exact_interval(3U);
  const char*                         error_message   = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_account_balance_interval(&state, reward_address, first, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_account_balance_interval(&state, reward_address2, second, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_account_balance_interval(&state, reward_address, replacement, &error_message), CARDANO_SUCCESS);

  // Assert
  cardano_account_balance_intervals_map_t* intervals = get_account_balance_intervals(&state);

  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 2U);
  EXPECT_TRUE(interval_key_at_is(intervals, 0U, REWARD_ADDRESS));
  EXPECT_TRUE(interval_key_at_is(intervals, 1U, REWARD_ADDRESS2));
  EXPECT_EQ(get_interval_at(intervals, 0U), replacement);
  EXPECT_EQ(get_interval_at(intervals, 1U), second);
  EXPECT_EQ(cardano_account_balance_interval_refcount(first), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address2);
  cardano_account_balance_interval_unref(&first);
  cardano_account_balance_interval_unref(&second);
  cardano_account_balance_interval_unref(&replacement);
}

TEST(cardano_builder_add_account_balance_interval, keepsThePreviousIntervalsWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t*           reward_address  = new_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t*           reward_address2 = new_reward_address(REWARD_ADDRESS2);
  cardano_account_balance_interval_t* first           = new_exact_interval(1U);
  cardano_account_balance_interval_t* second          = new_exact_interval(2U);
  cardano_account_balance_interval_t* replacement     = new_exact_interval(3U);
  const char*                         error_message   = NULL;

  EXPECT_EQ(cardano_builder_add_account_balance_interval(&state, reward_address, first, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_account_balance_interval(&state, reward_address2, second, &error_message), CARDANO_SUCCESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_account_balance_interval(&state, reward_address2, replacement, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_account_balance_intervals_map_t* intervals = get_account_balance_intervals(&state);

    EXPECT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 2U);
    EXPECT_EQ(get_interval_at(intervals, 0U), first);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(get_interval_at(intervals, 1U), replacement);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(get_interval_at(intervals, 1U), second);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address2);
  cardano_account_balance_interval_unref(&first);
  cardano_account_balance_interval_unref(&second);
  cardano_account_balance_interval_unref(&replacement);
}

TEST(cardano_builder_add_account_balance_interval_ex, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  cardano_account_balance_interval_t* replacement   = new_exact_interval(3U);
  const char*                         error_message = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), replacement, &error_message), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error_message, nullptr);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(get_account_balance_intervals(&state)), 1U);
  EXPECT_TRUE(interval_key_at_is(get_account_balance_intervals(&state), 0U, REWARD_ADDRESS));
  EXPECT_EQ(get_interval_at(get_account_balance_intervals(&state), 0U), replacement);
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_interval_unref(&replacement);
}

TEST(cardano_builder_add_account_balance_interval_ex, returnsErrorIfRewardAddressIsNullOrEmpty)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act & Assert
  EXPECT_EQ(cardano_builder_add_account_balance_interval_ex(&state, nullptr, 0U, interval, &error_message), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL or empty.");

  error_message = NULL;

  EXPECT_EQ(cardano_builder_add_account_balance_interval_ex(&state, REWARD_ADDRESS, 0U, interval, &error_message), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL or empty.");
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_account_balance_interval_ex, returnsErrorIfRewardAddressIsInvalid)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_account_balance_interval_ex(&state, INVALID_REWARD_ADDRESS, strlen(INVALID_REWARD_ADDRESS), interval, &error_message);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(error_message, "Failed to parse reward address.");
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_account_balance_interval_ex, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Account balance interval is NULL.");
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_account_balance_interval_ex, leavesTheTransactionUnchangedWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(get_interval_at(get_account_balance_intervals(&state), 0U), interval);
    }
    else
    {
      EXPECT_NE(result, CARDANO_SUCCESS);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(get_account_balance_intervals(&state), nullptr);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_starting_account_balance_interval, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t*           reward_address = new_reward_address(REWARD_ADDRESS);
  cardano_account_balance_interval_t* interval       = new_exact_interval(5000000U);
  const char*                         error_message  = NULL;

  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Act
  const cardano_error_t result = cardano_builder_add_starting_account_balance_interval(&state, reward_address, interval, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(error_message, nullptr);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(get_starting_account_balance_intervals(&state)), 1U);
  EXPECT_TRUE(interval_key_at_is(get_starting_account_balance_intervals(&state), 0U, REWARD_ADDRESS));
  EXPECT_EQ(get_interval_at(get_starting_account_balance_intervals(&state), 0U), interval);
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);
  EXPECT_EQ(get_direct_deposits(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_starting_account_balance_interval, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_starting_account_balance_interval(&state, nullptr, interval, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL.");
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_starting_account_balance_interval, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_reward_address(REWARD_ADDRESS);
  const char*               error_message  = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_starting_account_balance_interval(&state, reward_address, nullptr, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Starting account balance interval is NULL.");
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_builder_add_starting_account_balance_interval, replacesTheIntervalOfTheSameRewardAccount)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t*           reward_address  = new_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t*           reward_address2 = new_reward_address(REWARD_ADDRESS2);
  cardano_account_balance_interval_t* first           = new_exact_interval(1U);
  cardano_account_balance_interval_t* second          = new_exact_interval(2U);
  cardano_account_balance_interval_t* replacement     = new_exact_interval(3U);
  const char*                         error_message   = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval(&state, reward_address, first, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval(&state, reward_address2, second, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval(&state, reward_address, replacement, &error_message), CARDANO_SUCCESS);

  // Assert
  cardano_account_balance_intervals_map_t* intervals = get_starting_account_balance_intervals(&state);

  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 2U);
  EXPECT_TRUE(interval_key_at_is(intervals, 0U, REWARD_ADDRESS));
  EXPECT_TRUE(interval_key_at_is(intervals, 1U, REWARD_ADDRESS2));
  EXPECT_EQ(get_interval_at(intervals, 0U), replacement);
  EXPECT_EQ(get_interval_at(intervals, 1U), second);
  EXPECT_EQ(cardano_account_balance_interval_refcount(first), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address2);
  cardano_account_balance_interval_unref(&first);
  cardano_account_balance_interval_unref(&second);
  cardano_account_balance_interval_unref(&replacement);
}

TEST(cardano_builder_add_starting_account_balance_interval, keepsThePreviousIntervalsWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t*           reward_address  = new_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t*           reward_address2 = new_reward_address(REWARD_ADDRESS2);
  cardano_account_balance_interval_t* first           = new_exact_interval(1U);
  cardano_account_balance_interval_t* second          = new_exact_interval(2U);
  cardano_account_balance_interval_t* replacement     = new_exact_interval(3U);
  const char*                         error_message   = NULL;

  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval(&state, reward_address, first, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval(&state, reward_address2, second, &error_message), CARDANO_SUCCESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_starting_account_balance_interval(&state, reward_address2, replacement, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_account_balance_intervals_map_t* intervals = get_starting_account_balance_intervals(&state);

    EXPECT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 2U);
    EXPECT_EQ(get_interval_at(intervals, 0U), first);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(get_interval_at(intervals, 1U), replacement);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(get_interval_at(intervals, 1U), second);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address2);
  cardano_account_balance_interval_unref(&first);
  cardano_account_balance_interval_unref(&second);
  cardano_account_balance_interval_unref(&replacement);
}

TEST(cardano_builder_add_starting_account_balance_interval_ex, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  cardano_account_balance_interval_t* replacement   = new_exact_interval(3U);
  const char*                         error_message = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), replacement, &error_message), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error_message, nullptr);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(get_starting_account_balance_intervals(&state)), 1U);
  EXPECT_TRUE(interval_key_at_is(get_starting_account_balance_intervals(&state), 0U, REWARD_ADDRESS));
  EXPECT_EQ(get_interval_at(get_starting_account_balance_intervals(&state), 0U), replacement);
  EXPECT_EQ(get_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_interval_unref(&replacement);
}

TEST(cardano_builder_add_starting_account_balance_interval_ex, returnsErrorIfRewardAddressIsNullOrEmpty)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act & Assert
  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval_ex(&state, nullptr, 0U, interval, &error_message), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL or empty.");

  error_message = NULL;

  EXPECT_EQ(cardano_builder_add_starting_account_balance_interval_ex(&state, REWARD_ADDRESS, 0U, interval, &error_message), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Reward address is NULL or empty.");
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_starting_account_balance_interval_ex, returnsErrorIfRewardAddressIsInvalid)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_starting_account_balance_interval_ex(&state, INVALID_REWARD_ADDRESS, strlen(INVALID_REWARD_ADDRESS), interval, &error_message);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(error_message, "Failed to parse reward address.");
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_builder_add_starting_account_balance_interval_ex, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_starting_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Starting account balance interval is NULL.");
  EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_starting_account_balance_interval_ex, leavesTheTransactionUnchangedWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval      = new_exact_interval(5000000U);
  const char*                         error_message = NULL;

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_starting_account_balance_interval_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(get_interval_at(get_starting_account_balance_intervals(&state), 0U), interval);
    }
    else
    {
      EXPECT_NE(result, CARDANO_SUCCESS);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(get_starting_account_balance_intervals(&state), nullptr);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}
