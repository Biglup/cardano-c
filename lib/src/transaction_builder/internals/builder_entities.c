/**
 * \file builder_entities.c
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#include "builder_entities.h"

#include <cardano/transaction_body/account_balance_intervals_map.h>
#include <cardano/transaction_body/direct_deposit_map.h>
#include <cardano/transaction_body/transaction_body.h>

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Compares two reward addresses based on their serialized bytes.
 *
 * \param[in] lhs Pointer to the first reward address object.
 * \param[in] rhs Pointer to the second reward address object.
 *
 * \return true if the addresses are equal, false otherwise.
 */
static bool
reward_address_equals(const cardano_reward_address_t* lhs, const cardano_reward_address_t* rhs)
{
  const size_t lhs_size = cardano_reward_address_get_bytes_size(lhs);
  const size_t rhs_size = cardano_reward_address_get_bytes_size(rhs);

  if (lhs_size != rhs_size)
  {
    return false;
  }

  const byte_t* lhs_bytes = cardano_reward_address_get_bytes(lhs);
  const byte_t* rhs_bytes = cardano_reward_address_get_bytes(rhs);

  return memcmp(lhs_bytes, rhs_bytes, lhs_size) == 0;
}

/**
 * \brief Parses a reward address from its bech32 representation.
 *
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[out] address On success, this will point to a newly created \ref cardano_reward_address_t.
 *                     The caller is responsible for releasing it with \ref cardano_reward_address_unref.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the reward address was parsed, or an appropriate error code
 *         indicating the failure reason.
 */
static cardano_error_t
parse_reward_address(
  const char*                reward_address,
  const size_t               address_size,
  cardano_reward_address_t** address,
  const char**               error_message)
{
  if ((reward_address == NULL) || (address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const cardano_error_t result = cardano_reward_address_from_bech32(reward_address, address_size, address);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse reward address.";
  }

  return result;
}

/**
 * \brief Copies a direct deposit map setting the amount deposited to one reward account.
 *
 * Direct deposit maps reject duplicated keys and entries can not be updated in place, so this
 * function builds a new map with every entry of \p current in its original position. The entry of
 * \p reward_address gets \p amount, and it is appended at the end when the account is not present.
 *
 * \param[in] current A pointer to the \ref cardano_direct_deposit_map_t to copy, or NULL when there
 *                    are no direct deposits yet.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t of the entry to set.
 * \param[in] amount The amount of lovelace stored in the entry.
 * \param[out] updated On success, this will point to the newly created map. The caller is responsible
 *                     for releasing it with \ref cardano_direct_deposit_map_unref.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the map was created, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
copy_with_direct_deposit(
  const cardano_direct_deposit_map_t* current,
  cardano_reward_address_t*           reward_address,
  const uint64_t                      amount,
  cardano_direct_deposit_map_t**      updated,
  const char**                        error_message)
{
  cardano_direct_deposit_map_t* deposits = NULL;
  cardano_error_t               result   = cardano_direct_deposit_map_new(&deposits);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create direct deposit map.";

    return result;
  }

  const size_t length   = cardano_direct_deposit_map_get_length(current);
  bool         replaced = false;

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_reward_address_t* key   = NULL;
    uint64_t                  value = 0U;

    result = cardano_direct_deposit_map_get_key_value_at(current, i, &key, &value);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to read direct deposit.";
      cardano_direct_deposit_map_unref(&deposits);

      return result;
    }

    if (reward_address_equals(key, reward_address))
    {
      value    = amount;
      replaced = true;
    }

    result = cardano_direct_deposit_map_insert(deposits, key, value);
    cardano_reward_address_unref(&key);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to insert direct deposit.";
      cardano_direct_deposit_map_unref(&deposits);

      return result;
    }
  }

  if (!replaced)
  {
    result = cardano_direct_deposit_map_insert(deposits, reward_address, amount);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to insert direct deposit.";
      cardano_direct_deposit_map_unref(&deposits);

      return result;
    }
  }

  *updated = deposits;

  return CARDANO_SUCCESS;
}

/**
 * \brief Copies an account balance intervals map setting the interval of one reward account.
 *
 * Account balance intervals maps reject duplicated keys and entries can not be updated in place, so
 * this function builds a new map with every entry of \p current in its original position. The entry
 * of \p reward_address gets \p interval, and it is appended at the end when the account is not
 * present.
 *
 * \param[in] current A pointer to the \ref cardano_account_balance_intervals_map_t to copy, or NULL
 *                    when there are no intervals yet.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t of the entry to set.
 * \param[in] interval A pointer to the \ref cardano_account_balance_interval_t stored in the entry.
 * \param[out] updated On success, this will point to the newly created map. The caller is responsible
 *                     for releasing it with \ref cardano_account_balance_intervals_map_unref.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the map was created, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
copy_with_interval(
  const cardano_account_balance_intervals_map_t* current,
  cardano_reward_address_t*                      reward_address,
  cardano_account_balance_interval_t*            interval,
  cardano_account_balance_intervals_map_t**      updated,
  const char**                                   error_message)
{
  cardano_account_balance_intervals_map_t* intervals = NULL;
  cardano_error_t                          result    = cardano_account_balance_intervals_map_new(&intervals);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create account balance intervals map.";

    return result;
  }

  const size_t length   = cardano_account_balance_intervals_map_get_length(current);
  bool         replaced = false;

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_reward_address_t*           key   = NULL;
    cardano_account_balance_interval_t* value = NULL;

    result = cardano_account_balance_intervals_map_get_key_value_at(current, i, &key, &value);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to read account balance interval.";
      cardano_account_balance_intervals_map_unref(&intervals);

      return result;
    }

    if (reward_address_equals(key, reward_address))
    {
      result   = cardano_account_balance_intervals_map_insert(intervals, key, interval);
      replaced = true;
    }
    else
    {
      result = cardano_account_balance_intervals_map_insert(intervals, key, value);
    }

    cardano_reward_address_unref(&key);
    cardano_account_balance_interval_unref(&value);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to insert account balance interval.";
      cardano_account_balance_intervals_map_unref(&intervals);

      return result;
    }
  }

  if (!replaced)
  {
    result = cardano_account_balance_intervals_map_insert(intervals, reward_address, interval);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to insert account balance interval.";
      cardano_account_balance_intervals_map_unref(&intervals);

      return result;
    }
  }

  *updated = intervals;

  return CARDANO_SUCCESS;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_add_direct_deposit(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* reward_address,
  const uint64_t            amount,
  const char**              error_message)
{
  if (reward_address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount == 0U)
  {
    *error_message = "Direct deposit amount must be greater than zero.";

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_direct_deposit_map_t* current = cardano_transaction_body_get_direct_deposits(body);
  cardano_direct_deposit_map_unref(&current);

  uint64_t deposited = 0U;
  uint64_t total     = amount;

  if (cardano_direct_deposit_map_get(current, reward_address, &deposited) == CARDANO_SUCCESS)
  {
    if (amount > (UINT64_MAX - deposited))
    {
      *error_message = "Direct deposit amount overflows.";

      return CARDANO_ERROR_INTEGER_OVERFLOW;
    }

    total += deposited;
  }

  cardano_direct_deposit_map_t* updated = NULL;
  cardano_error_t               result  = copy_with_direct_deposit(current, reward_address, total, &updated, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_transaction_body_set_direct_deposits(body, updated);
  cardano_direct_deposit_map_unref(&updated);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set direct deposit map.";
  }

  return result;
}

cardano_error_t
cardano_builder_add_direct_deposit_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  const uint64_t           amount,
  const char**             error_message)
{
  cardano_reward_address_t* address = NULL;
  cardano_error_t           result  = parse_reward_address(reward_address, address_size, &address, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_builder_add_direct_deposit(state, address, amount, error_message);
  cardano_reward_address_unref(&address);

  return result;
}

cardano_error_t
cardano_builder_add_account_balance_interval(
  cardano_builder_state_t*            state,
  cardano_reward_address_t*           reward_address,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message)
{
  if (reward_address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (interval == NULL)
  {
    *error_message = "Account balance interval is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_account_balance_intervals_map_t* current = cardano_transaction_body_get_account_balance_intervals(body);
  cardano_account_balance_intervals_map_unref(&current);

  cardano_account_balance_intervals_map_t* updated = NULL;
  cardano_error_t                          result  = copy_with_interval(current, reward_address, interval, &updated, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_transaction_body_set_account_balance_intervals(body, updated);
  cardano_account_balance_intervals_map_unref(&updated);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set account balance intervals map.";
  }

  return result;
}

cardano_error_t
cardano_builder_add_account_balance_interval_ex(
  cardano_builder_state_t*            state,
  const char*                         reward_address,
  size_t                              address_size,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message)
{
  cardano_reward_address_t* address = NULL;
  cardano_error_t           result  = parse_reward_address(reward_address, address_size, &address, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_builder_add_account_balance_interval(state, address, interval, error_message);
  cardano_reward_address_unref(&address);

  return result;
}

cardano_error_t
cardano_builder_add_starting_account_balance_interval(
  cardano_builder_state_t*            state,
  cardano_reward_address_t*           reward_address,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message)
{
  if (reward_address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (interval == NULL)
  {
    *error_message = "Starting account balance interval is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_account_balance_intervals_map_t* current = cardano_transaction_body_get_starting_account_balance_intervals(body);
  cardano_account_balance_intervals_map_unref(&current);

  cardano_account_balance_intervals_map_t* updated = NULL;
  cardano_error_t                          result  = copy_with_interval(current, reward_address, interval, &updated, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_transaction_body_set_starting_account_balance_intervals(body, updated);
  cardano_account_balance_intervals_map_unref(&updated);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set starting account balance intervals map.";
  }

  return result;
}

cardano_error_t
cardano_builder_add_starting_account_balance_interval_ex(
  cardano_builder_state_t*            state,
  const char*                         reward_address,
  size_t                              address_size,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message)
{
  cardano_reward_address_t* address = NULL;
  cardano_error_t           result  = parse_reward_address(reward_address, address_size, &address, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_builder_add_starting_account_balance_interval(state, address, interval, error_message);
  cardano_reward_address_unref(&address);

  return result;
}
