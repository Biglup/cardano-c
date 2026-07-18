/**
 * \file builder_state.c
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

#include "builder_state.h"

#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/witness_set/witness_set.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Creates and initializes a new, empty transaction.
 *
 * This function allocates and initializes a new instance of a \ref cardano_transaction_t object.
 *
 * \return A pointer to the newly created \ref cardano_transaction_t object if successful. Returns
 * \c NULL if memory allocation fails.
 */
static cardano_transaction_t*
transaction_new(void)
{
  cardano_transaction_body_t*        body        = NULL;
  cardano_transaction_input_set_t*   inputs      = NULL;
  cardano_transaction_output_list_t* outputs     = NULL;
  cardano_witness_set_t*             witnesses   = NULL;
  cardano_transaction_t*             transaction = NULL;

  cardano_error_t result = cardano_transaction_input_set_new(&inputs);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  result = cardano_transaction_output_list_new(&outputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    return NULL;
  }

  result = cardano_transaction_body_new(inputs, outputs, 0U, NULL, &body);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    cardano_transaction_output_list_unref(&outputs);
    return NULL;
  }

  result = cardano_witness_set_new(&witnesses);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    cardano_transaction_output_list_unref(&outputs);
    cardano_transaction_body_unref(&body);
    return NULL;
  }

  result = cardano_transaction_new(body, witnesses, NULL, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    cardano_transaction_output_list_unref(&outputs);
    cardano_transaction_body_unref(&body);
    cardano_witness_set_unref(&witnesses);

    return NULL;
  }

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&witnesses);

  return transaction;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_state_init(
  cardano_builder_state_t*       state,
  cardano_protocol_parameters_t* params,
  const cardano_slot_config_t*   slot_config)
{
  if ((state == NULL) || (params == NULL) || (slot_config == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  state->transaction                 = NULL;
  state->params                      = NULL;
  state->slot_config                 = *slot_config;
  state->coin_selector               = NULL;
  state->tx_evaluator                = NULL;
  state->change_address              = NULL;
  state->collateral_address          = NULL;
  state->available_utxos             = NULL;
  state->collateral_utxos            = NULL;
  state->pre_selected_inputs         = NULL;
  state->reference_inputs            = NULL;
  state->input_to_redeemer_map       = NULL;
  state->withdrawals_to_redeemer_map = NULL;
  state->mints_to_redeemer_map       = NULL;
  state->votes_to_redeemer_map       = NULL;
  state->deferred_redeemers          = NULL;
  state->tx_evaluator                = NULL;

  cardano_protocol_parameters_ref(params);
  state->params = params;

  cardano_error_t result = cardano_random_improve_coin_selector_new(&state->coin_selector);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  state->transaction = transaction_new();

  if (state->transaction == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  state->change_address             = NULL;
  state->collateral_address         = NULL;
  state->available_utxos            = NULL;
  state->collateral_utxos           = NULL;
  state->pre_selected_inputs        = NULL;
  state->reference_inputs           = NULL;
  state->has_plutus_v1              = false;
  state->has_plutus_v2              = false;
  state->has_plutus_v3              = false;
  state->additional_signature_count = 0U;

  result = cardano_utxo_list_new(&state->pre_selected_inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_utxo_list_new(&state->reference_inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_input_to_redeemer_map_new(&state->input_to_redeemer_map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_blake2b_hash_to_redeemer_map_new(&state->withdrawals_to_redeemer_map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_blake2b_hash_to_redeemer_map_new(&state->mints_to_redeemer_map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_blake2b_hash_to_redeemer_map_new(&state->votes_to_redeemer_map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_deferred_redeemer_list_new(&state->deferred_redeemers);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_costmdls_t*         cost_models    = cardano_protocol_parameters_get_cost_models(state->params);
  cardano_protocol_version_t* version        = cardano_protocol_parameters_get_protocol_version(state->params);
  const uint64_t              protocol_major = cardano_protocol_version_get_major(version);

  result = cardano_tx_evaluator_new_native(&state->slot_config, cost_models, protocol_major, &state->tx_evaluator);

  cardano_costmdls_unref(&cost_models);
  cardano_protocol_version_unref(&version);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

void
cardano_builder_state_release(cardano_builder_state_t* state)
{
  if (state == NULL)
  {
    return;
  }

  cardano_transaction_unref(&state->transaction);
  cardano_protocol_parameters_unref(&state->params);
  cardano_coin_selector_unref(&state->coin_selector);
  cardano_tx_evaluator_unref(&state->tx_evaluator);
  cardano_address_unref(&state->change_address);
  cardano_address_unref(&state->collateral_address);
  cardano_utxo_list_unref(&state->available_utxos);
  cardano_utxo_list_unref(&state->collateral_utxos);
  cardano_utxo_list_unref(&state->pre_selected_inputs);
  cardano_utxo_list_unref(&state->reference_inputs);
  cardano_input_to_redeemer_map_unref(&state->input_to_redeemer_map);
  cardano_blake2b_hash_to_redeemer_map_unref(&state->withdrawals_to_redeemer_map);
  cardano_blake2b_hash_to_redeemer_map_unref(&state->mints_to_redeemer_map);
  cardano_blake2b_hash_to_redeemer_map_unref(&state->votes_to_redeemer_map);
  cardano_deferred_redeemer_list_unref(&state->deferred_redeemers);

  state->transaction                 = NULL;
  state->params                      = NULL;
  state->coin_selector               = NULL;
  state->tx_evaluator                = NULL;
  state->change_address              = NULL;
  state->collateral_address          = NULL;
  state->available_utxos             = NULL;
  state->collateral_utxos            = NULL;
  state->pre_selected_inputs         = NULL;
  state->reference_inputs            = NULL;
  state->input_to_redeemer_map       = NULL;
  state->withdrawals_to_redeemer_map = NULL;
  state->mints_to_redeemer_map       = NULL;
  state->votes_to_redeemer_map       = NULL;
  state->deferred_redeemers          = NULL;
}
