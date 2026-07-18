/**
 * \file builder_withdrawals.c
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
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

#include "builder_withdrawals.h"

#include <cardano/common/withdrawal_map.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/witness_set/witness_set.h>

#include "../../allocators.h"
#include "../../string_safe.h"
#include "builder_redeemers.h"

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Computes a deterministic, sortable hash ID for a credential.
 *
 * This function creates a unique identifier by concatenating the credential's type
 * and its raw hash bytes.
 *
 * \param[in] credential A pointer to the `cardano_credential_t` object.
 * \param[out] id On success, this will be populated with a pointer to a newly allocated
 * `cardano_blake2b_hash_t` object representing the unique ID.
 *
 * \return `CARDANO_SUCCESS` if the ID was computed successfully, or an appropriate
 * error code on failure.
 *
 * \note The caller is responsible for managing the lifecycle of the returned `id` object
 * and must release it by calling `cardano_blake2b_hash_unref`.
 */
static cardano_error_t
compute_credential_sortable_id(cardano_credential_t* credential, cardano_blake2b_hash_t** id)
{
  if ((credential == NULL) || (id == NULL))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_credential_type_t type;
  cardano_error_t           result = cardano_credential_get_type(credential, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_t* hash       = cardano_credential_get_hash(credential);
  const byte_t*           hash_bytes = cardano_blake2b_hash_get_data(hash);
  size_t                  hash_size  = cardano_blake2b_hash_get_bytes_size(hash);

  const size_t total_size = sizeof(type) + hash_size;
  byte_t*      id_bytes   = (byte_t*)_cardano_malloc(total_size);

  if (id_bytes == NULL)
  {
    cardano_blake2b_hash_unref(&hash);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  size_t cursor = 0;

  cardano_safe_memcpy(&id_bytes[cursor], total_size, &type, sizeof(type));
  cursor += sizeof(type);

  cardano_safe_memcpy(&id_bytes[cursor], total_size - cursor, hash_bytes, hash_size);

  cardano_blake2b_hash_unref(&hash);

  result = cardano_blake2b_hash_from_bytes(id_bytes, total_size, id);

  _cardano_free(id_bytes);

  return result;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_withdraw_rewards(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  const int64_t             amount,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message)
{
  if (address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount < 0)
  {
    *error_message = "Amount must be greater than zero.";

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);

  if (withdrawals == NULL)
  {
    cardano_error_t result = cardano_withdrawal_map_new(&withdrawals);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create withdrawal map.";

      return result;
    }

    result = cardano_transaction_body_set_withdrawals(body, withdrawals);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set withdrawal map.";
      cardano_withdrawal_map_unref(&withdrawals);

      return result;
    }
  }

  cardano_withdrawal_map_unref(&withdrawals);

  cardano_error_t result = cardano_withdrawal_map_insert(withdrawals, address, amount);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to insert withdrawal.";

    return result;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  cardano_blake2b_hash_t* hash = NULL;

  result = compute_credential_sortable_id(credential, &hash);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to compute credential sortable ID.";

    return result;
  }

  if (redeemer != NULL)
  {
    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
    cardano_witness_set_unref(&witnesses);

    result = cardano_builder_add_redeemer(witnesses, hash, redeemer, CARDANO_REDEEMER_TAG_REWARD, state, error_message);

    cardano_blake2b_hash_unref(&hash);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to add redeemer.";

      return result;
    }
  }
  else
  {
    cardano_credential_type_t cred_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
    result                              = cardano_credential_get_type(credential, &cred_type);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&hash);
      *error_message = "Failed to add withdrawal.";

      return result;
    }

    if (cred_type != CARDANO_CREDENTIAL_TYPE_KEY_HASH)
    {
      result = cardano_blake2b_hash_to_redeemer_map_insert(state->withdrawals_to_redeemer_map, hash, NULL);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&hash);
        *error_message = "Failed to add withdrawal.";

        return result;
      }
    }

    cardano_blake2b_hash_unref(&hash);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_withdraw_rewards_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  const int64_t            amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((reward_address == NULL) || (address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_t* addr   = NULL;
  cardano_error_t           result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse reward address.";

    return result;
  }

  result = cardano_builder_withdraw_rewards(state, addr, amount, redeemer, error_message);
  cardano_reward_address_unref(&addr);

  return result;
}

cardano_error_t
cardano_builder_withdraw_rewards_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_reward_address_t*      address,
  const int64_t                  amount,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  const char**                   error_message)
{
  if ((address == NULL) || (callback == NULL))
  {
    *error_message = "Reward address and callback are required";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* placeholder = NULL;

  cardano_error_t result = cardano_builder_create_placeholder_plutus_data(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the placeholder redeemer.";

    return result;
  }

  result = cardano_builder_withdraw_rewards(state, address, amount, placeholder, error_message);

  cardano_plutus_data_unref(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  cardano_blake2b_hash_t* hash = NULL;

  result = compute_credential_sortable_id(credential, &hash);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to compute the withdrawal sortable id.";

    return result;
  }

  result = cardano_builder_register_deferred_from_map(state, state->withdrawals_to_redeemer_map, hash, callback, user_context, error_message);

  cardano_blake2b_hash_unref(&hash);

  return result;
}
