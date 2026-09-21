/**
 * \file sub_transaction_builder.c
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

#include <cardano/transaction_builder/sub_transaction_builder.h>

#include <cardano/object.h>

#include "../allocators.h"
#include "./internals/builder_certs.h"
#include "./internals/builder_config.h"
#include "./internals/builder_entities.h"
#include "./internals/builder_inputs.h"
#include "./internals/builder_mint.h"
#include "./internals/builder_outputs.h"
#include "./internals/builder_proposals.h"
#include "./internals/builder_state.h"
#include "./internals/builder_sub_build.h"
#include "./internals/builder_votes.h"
#include "./internals/builder_withdrawals.h"
#include "./internals/builder_witnesses.h"

#include <assert.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Type definition for the Cardano Sub Transaction builder.
 *
 * The `cardano_sub_tx_builder_t` type represents an instance of a sub transaction builder for creating
 * CIP-118 sub transactions. It provides methods for adding inputs, outputs and the other elements a sub
 * transaction can carry.
 *
 * A sub transaction is an intent, so the builder never balances it: the result spends exactly the inputs
 * and produces exactly the outputs that were added, and it carries no fee and no collateral.
 */
typedef struct cardano_sub_tx_builder_t
{
    cardano_object_t        base;
    cardano_error_t         last_error;
    cardano_builder_state_t state;
} cardano_sub_tx_builder_t;

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Deallocates a sub_tx_builder object.
 *
 * This function is responsible for properly deallocating a sub_tx_builder object (`cardano_sub_tx_builder_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the sub_tx_builder object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_sub_tx_builder_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the sub_tx_builder
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_sub_tx_builder_deallocate(void* object)
{
  assert(object != NULL);

  cardano_sub_tx_builder_t* builder = (cardano_sub_tx_builder_t*)object;

  cardano_builder_state_release(&builder->state);

  _cardano_free(builder);
}

/**
 * \brief Records the outcome of an internal builder operation in the builder.
 *
 * This function stores the error code of a failed internal operation as the builder deferred error
 * and, when the operation reported an error message, copies it into the builder last error message.
 * A successful outcome leaves the builder untouched.
 *
 * \param[in,out] builder A pointer to the \ref cardano_sub_tx_builder_t that receives the outcome. This
 *                        parameter must not be NULL.
 * \param[in] result The error code returned by the internal operation.
 * \param[in] error_message The error message reported by the internal operation, or NULL when the
 *                          operation did not report one.
 */
static void
track_builder_result(
  cardano_sub_tx_builder_t* builder,
  const cardano_error_t     result,
  const char*               error_message)
{
  if (result != CARDANO_SUCCESS)
  {
    if (error_message != NULL)
    {
      cardano_sub_tx_builder_set_last_error(builder, error_message);
    }

    builder->last_error = result;
  }
}

/* DEFINITIONS ****************************************************************/

cardano_sub_tx_builder_t*
cardano_sub_tx_builder_new(
  cardano_protocol_parameters_t* params,
  const cardano_slot_config_t*   slot_config)
{
  if ((params == NULL) || (slot_config == NULL))
  {
    return NULL;
  }

  cardano_sub_tx_builder_t* builder = _cardano_malloc(sizeof(cardano_sub_tx_builder_t));

  if (builder == NULL)
  {
    return NULL;
  }

  builder->base.ref_count     = 1;
  builder->base.last_error[0] = '\0';
  builder->base.deallocator   = cardano_sub_tx_builder_deallocate;

  builder->last_error = CARDANO_SUCCESS;

  const cardano_error_t result = cardano_builder_state_init(&builder->state, params, slot_config);

  if (result != CARDANO_SUCCESS)
  {
    cardano_sub_tx_builder_unref(&builder);
    return NULL;
  }

  return builder;
}

void
cardano_sub_tx_builder_set_network_id(
  cardano_sub_tx_builder_t*  builder,
  const cardano_network_id_t network_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_network_id(&builder->state, network_id, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_donation(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            donation)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_donation(&builder->state, donation, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_after(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            slot)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_after(&builder->state, slot, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_after_ex(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            unix_time)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_after_ex(&builder->state, unix_time, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_before(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            slot)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_before(&builder->state, slot, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_before_ex(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            unix_time)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_before_ex(&builder->state, unix_time, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_input(
  cardano_sub_tx_builder_t* builder,
  cardano_utxo_t*           utxo)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_input(&builder->state, utxo, NULL, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_reference_input(
  cardano_sub_tx_builder_t* builder,
  cardano_utxo_t*           utxo)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_reference_input(&builder->state, utxo, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_output(
  cardano_sub_tx_builder_t*     builder,
  cardano_transaction_output_t* output)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const cardano_error_t result = cardano_builder_add_output(&builder->state, output);

  track_builder_result(builder, result, NULL);
}

void
cardano_sub_tx_builder_send_lovelace(
  cardano_sub_tx_builder_t* builder,
  cardano_address_t*        address,
  const uint64_t            amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_lovelace(&builder->state, address, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_send_lovelace_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               address,
  size_t                    address_size,
  uint64_t                  amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_lovelace_ex(&builder->state, address, address_size, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_send_value(
  cardano_sub_tx_builder_t* builder,
  cardano_address_t*        address,
  cardano_value_t*          value)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_value(&builder->state, address, value, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_send_value_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               address,
  size_t                    address_size,
  cardano_value_t*          value)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_value_ex(&builder->state, address, address_size, value, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_metadata(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            tag,
  cardano_metadatum_t*      metadata)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_metadata(&builder->state, tag, metadata, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_metadata_ex(
  cardano_sub_tx_builder_t* builder,
  uint64_t                  tag,
  const char*               metadata_json,
  const size_t              json_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_metadata_ex(&builder->state, tag, metadata_json, json_size, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token(
  cardano_sub_tx_builder_t* builder,
  cardano_blake2b_hash_t*   policy_id,
  cardano_asset_name_t*     name,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token(&builder->state, policy_id, name, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               policy_id_hex,
  size_t                    policy_id_size,
  const char*               name_hex,
  size_t                    name_size,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_ex(&builder->state, policy_id_hex, policy_id_size, name_hex, name_size, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token_with_id(
  cardano_sub_tx_builder_t* builder,
  cardano_asset_id_t*       asset_id,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_with_id(&builder->state, asset_id, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token_with_id_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               asset_id_hex,
  size_t                    hex_size,
  int64_t                   amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_with_id_ex(&builder->state, asset_id_hex, hex_size, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_script(
  cardano_sub_tx_builder_t* builder,
  cardano_script_t*         script)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_sub_transaction_script(&builder->state, script, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_signer(
  cardano_sub_tx_builder_t* builder,
  cardano_blake2b_hash_t*   pub_key_hash)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_signer(&builder->state, pub_key_hash, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_signer_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               pub_key_hash,
  size_t                    hash_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_signer_ex(&builder->state, pub_key_hash, hash_size, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_guard(
  cardano_sub_tx_builder_t* builder,
  cardano_credential_t*     guard)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_guard(&builder->state, guard, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_guard_ex(
  cardano_sub_tx_builder_t*       builder,
  const char*                     hash_hex,
  size_t                          hash_hex_size,
  const cardano_credential_type_t type)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_guard_ex(&builder->state, hash_hex, hash_hex_size, type, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_require_top_level_guard(
  cardano_sub_tx_builder_t* builder,
  cardano_credential_t*     credential,
  cardano_plutus_data_t*    datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_require_top_level_guard(&builder->state, credential, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_require_top_level_guard_ex(
  cardano_sub_tx_builder_t*       builder,
  const char*                     hash_hex,
  size_t                          hash_hex_size,
  const cardano_credential_type_t type,
  cardano_plutus_data_t*          datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_require_top_level_guard_ex(&builder->state, hash_hex, hash_hex_size, type, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_direct_deposit(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* reward_address,
  const uint64_t            amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_direct_deposit(&builder->state, reward_address, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_direct_deposit_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  const uint64_t            amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_direct_deposit_ex(&builder->state, reward_address, address_size, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_account_balance_interval(
  cardano_sub_tx_builder_t*           builder,
  cardano_reward_address_t*           reward_address,
  cardano_account_balance_interval_t* interval)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_account_balance_interval(&builder->state, reward_address, interval, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_account_balance_interval_ex(
  cardano_sub_tx_builder_t*           builder,
  const char*                         reward_address,
  size_t                              address_size,
  cardano_account_balance_interval_t* interval)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_account_balance_interval_ex(&builder->state, reward_address, address_size, interval, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_withdraw_rewards(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_withdraw_rewards(&builder->state, address, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_withdraw_rewards_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_withdraw_rewards_ex(&builder->state, reward_address, address_size, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_register_reward_address(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_reward_address(&builder->state, address, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_register_reward_address_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_reward_address_ex(&builder->state, reward_address, address_size, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_deregister_reward_address(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_reward_address(&builder->state, address, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_deregister_reward_address_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_reward_address_ex(&builder->state, reward_address, address_size, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_delegate_stake(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address,
  cardano_blake2b_hash_t*   pool_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_stake(&builder->state, address, pool_id, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_delegate_stake_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  const char*               pool_id,
  size_t                    pool_id_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_stake_ex(&builder->state, reward_address, address_size, pool_id, pool_id_size, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_delegate_voting_power(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address,
  cardano_drep_t*           drep)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_voting_power(&builder->state, address, drep, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_delegate_voting_power_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  const char*               drep_id,
  size_t                    drep_id_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_voting_power_ex(&builder->state, reward_address, address_size, drep_id, drep_id_size, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_register_drep(
  cardano_sub_tx_builder_t* builder,
  cardano_drep_t*           drep,
  cardano_anchor_t*         anchor)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_drep(&builder->state, drep, anchor, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_register_drep_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               drep_id,
  const size_t              drep_id_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_drep_ex(&builder->state, drep_id, drep_id_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_update_drep(
  cardano_sub_tx_builder_t* builder,
  cardano_drep_t*           drep,
  cardano_anchor_t*         anchor)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_update_drep(&builder->state, drep, anchor, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_update_drep_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               drep_id,
  const size_t              drep_id_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_update_drep_ex(&builder->state, drep_id, drep_id_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_deregister_drep(
  cardano_sub_tx_builder_t* builder,
  cardano_drep_t*           drep)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_drep(&builder->state, drep, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_deregister_drep_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               drep_id,
  size_t                    drep_id_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_drep_ex(&builder->state, drep_id, drep_id_size, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_vote(
  cardano_sub_tx_builder_t*       builder,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_vote(&builder->state, voter, action_id, vote, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_certificate(
  cardano_sub_tx_builder_t* builder,
  cardano_certificate_t*    certificate)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_certificate(&builder->state, certificate, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_hardfork(
  cardano_sub_tx_builder_t*       builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_protocol_version_t*     version,
  cardano_governance_action_id_t* governance_action_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_hardfork(&builder->state, reward_address, anchor, version, governance_action_id, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_hardfork_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  const size_t              reward_address_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size,
  const char*               gov_action_id,
  const size_t              gov_action_id_size,
  const uint64_t            minor_protocol_version,
  const uint64_t            major_protocol_version)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_hardfork_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, minor_protocol_version, major_protocol_version, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_no_confidence(
  cardano_sub_tx_builder_t*       builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_no_confidence(&builder->state, reward_address, anchor, governance_action_id, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_no_confidence_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  const size_t              reward_address_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size,
  const char*               gov_action_id,
  const size_t              gov_action_id_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_no_confidence_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_update_committee(
  cardano_sub_tx_builder_t*        builder,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  cardano_unit_interval_t*         new_quorum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_update_committee(&builder->state, reward_address, anchor, governance_action_id, members_to_be_removed, members_to_be_added, new_quorum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_update_committee_ex(
  cardano_sub_tx_builder_t*        builder,
  const char*                      reward_address,
  const size_t                     reward_address_size,
  const char*                      metadata_url,
  const size_t                     metadata_url_size,
  const char*                      metadata_hash_hex,
  const size_t                     metadata_hash_hex_size,
  const char*                      gov_action_id,
  const size_t                     gov_action_id_size,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  const double                     new_quorum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_update_committee_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, members_to_be_removed, members_to_be_added, new_quorum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_new_constitution(
  cardano_sub_tx_builder_t*       builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  cardano_constitution_t*         constitution)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_new_constitution(&builder->state, reward_address, anchor, governance_action_id, constitution, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_new_constitution_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  const size_t              reward_address_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size,
  const char*               gov_action_id,
  const size_t              gov_action_id_size,
  cardano_constitution_t*   constitution)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_new_constitution_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, constitution, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_info(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_info(&builder->state, reward_address, anchor, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_propose_info_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  const size_t              reward_address_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_info_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &error_message);

  track_builder_result(builder, result, error_message);
}

cardano_error_t
cardano_sub_tx_builder_build(
  cardano_sub_tx_builder_t*   builder,
  cardano_sub_transaction_t** sub_transaction)
{
  if (builder == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (builder->last_error != CARDANO_SUCCESS)
  {
    return builder->last_error;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_build_sub_transaction(&builder->state, sub_transaction, &error_message);

  if (result != CARDANO_SUCCESS)
  {
    track_builder_result(builder, result, error_message);

    return result;
  }

  builder->last_error = CARDANO_ERROR_ILLEGAL_STATE;

  return CARDANO_SUCCESS;
}

void
cardano_sub_tx_builder_unref(cardano_sub_tx_builder_t** sub_tx_builder)
{
  if ((sub_tx_builder == NULL) || (*sub_tx_builder == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*sub_tx_builder)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *sub_tx_builder = NULL;
    return;
  }
}

void
cardano_sub_tx_builder_ref(cardano_sub_tx_builder_t* sub_tx_builder)
{
  if (sub_tx_builder == NULL)
  {
    return;
  }

  cardano_object_ref(&sub_tx_builder->base);
}

size_t
cardano_sub_tx_builder_refcount(const cardano_sub_tx_builder_t* sub_tx_builder)
{
  if (sub_tx_builder == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&sub_tx_builder->base);
}

void
cardano_sub_tx_builder_set_last_error(cardano_sub_tx_builder_t* sub_tx_builder, const char* message)
{
  cardano_object_set_last_error(&sub_tx_builder->base, message);
}

const char*
cardano_sub_tx_builder_get_last_error(const cardano_sub_tx_builder_t* sub_tx_builder)
{
  return cardano_object_get_last_error(&sub_tx_builder->base);
}
