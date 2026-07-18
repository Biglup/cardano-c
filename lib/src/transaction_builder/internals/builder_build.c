/**
 * \file builder_build.c
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

#include "builder_build.h"

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/scripts/plutus_scripts/plutus_language_version.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/script_data_hash.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include <assert.h>

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Sets a dummy script data hash in the transaction.
 *
 * This function assigns a placeholder script data hash to the specified transaction for fee calculation
 * purposes when the transaction includes script data.
 *
 * \param[in,out] tx A pointer to a \ref cardano_transaction_t object representing the transaction to which
 *                   the dummy script data hash will be set. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the dummy
 *         script data hash was successfully set, or an appropriate error code indicating the failure reason.
 */
static cardano_error_t
set_dummy_script_data_hash(cardano_transaction_t* tx)
{
  if (!cardano_transaction_has_script_data(tx))
  {
    return CARDANO_SUCCESS;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  static byte_t           dummy_hash[32] = { 0 };
  cardano_blake2b_hash_t* hash           = NULL;

  cardano_error_t new_hash_result = cardano_blake2b_hash_from_bytes(dummy_hash, sizeof(dummy_hash), &hash);

  if (new_hash_result != CARDANO_SUCCESS)
  {
    return new_hash_result;
  }

  new_hash_result = cardano_transaction_body_set_script_data_hash(body, hash);
  cardano_blake2b_hash_unref(&hash);

  if (new_hash_result != CARDANO_SUCCESS)
  {
    return new_hash_result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the cost model for a specified Plutus language version.
 *
 * This function searches for the cost model matching the specified Plutus language version within a
 * provided set of source cost models and, if found, copies it into a destination cost model set.
 *
 * \param[in] source_models A pointer to the \ref cardano_costmdls_t object containing the source cost models.
 * \param[out] dest_models A pointer to the \ref cardano_costmdls_t object where the matching cost model
 *                         for the specified version will be copied. This parameter must not be NULL.
 * \param[in] version The Plutus language version of the cost model to retrieve.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the cost model
 *         was successfully found and copied, or an appropriate error code if an internal error occurs.
 */
static cardano_error_t
get_cost_model_for_version(
  cardano_costmdls_t*                     source_models,
  cardano_costmdls_t*                     dest_models,
  const cardano_plutus_language_version_t version)
{
  assert(source_models != NULL);
  assert(dest_models != NULL);

  cardano_cost_model_t* cost_model = NULL;

  cardano_error_t result = cardano_costmdls_get(source_models, version, &cost_model);
  cardano_cost_model_unref(&cost_model);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_costmdls_insert(dest_models, cost_model);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the cost models flagged by the builder state.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t instance from which the cost models will be retrieved.
 * \param[out] costmdls A pointer to an initialized \ref cardano_costmdls_t object where the cost models will be stored.
 *                      This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the cost models
 *         were successfully retrieved and stored, or an appropriate error code if an error occurs during retrieval.
 */
static cardano_error_t
get_cost_models(cardano_builder_state_t* state, cardano_costmdls_t* costmdls)
{
  cardano_costmdls_t* pparams_costmdls = cardano_protocol_parameters_get_cost_models(state->params);
  cardano_costmdls_unref(&pparams_costmdls);

  cardano_error_t result = CARDANO_SUCCESS;

  if (state->has_plutus_v1)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (state->has_plutus_v2)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V2);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (state->has_plutus_v3)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V3);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

/**
 * \brief Computes and updates the script data hash in a transaction.
 *
 * This function calculates the script data hash based on the redeemers, datums, and cost models
 * associated with the transaction in the \ref cardano_builder_state_t instance, then updates this
 * hash in the provided \ref cardano_transaction_t.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t instance containing the necessary
 *                  transaction data, including redeemers, datums, and cost models.
 * \param[in,out] tx A pointer to an initialized \ref cardano_transaction_t object. This transaction
 *                   will be updated with the computed script data hash.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script data hash was successfully computed and updated in the transaction, or an
 *         appropriate error code if the computation or update fails.
 */
static cardano_error_t
update_script_data_hash(cardano_builder_state_t* state, cardano_transaction_t* tx)
{
  if (!cardano_transaction_has_script_data(tx))
  {
    return CARDANO_SUCCESS;
  }

  cardano_costmdls_t* costmdls = NULL;

  cardano_error_t result = cardano_costmdls_new(&costmdls);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = get_cost_models(state, costmdls);

  if (result != CARDANO_SUCCESS)
  {
    cardano_costmdls_unref(&costmdls);
    return result;
  }

  cardano_blake2b_hash_t* hash = NULL;

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witness_set);
  cardano_plutus_data_set_unref(&datums);

  result = cardano_compute_script_data_hash(costmdls, redeemers, datums, &hash);
  cardano_costmdls_unref(&costmdls);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&hash);

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  result = cardano_transaction_body_set_script_data_hash(body, hash);
  cardano_blake2b_hash_unref(&hash);

  return result;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_build(
  cardano_builder_state_t* state,
  cardano_transaction_t**  transaction,
  const char**             error_message)
{
  if (state->change_address == NULL)
  {
    *error_message = "You must set a change address before calling `build`.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (state->available_utxos == NULL)
  {
    *error_message = "You must set the available UTXOs for input selection before calling `build`.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cardano_transaction_has_script_data(state->transaction))
  {
    if (state->collateral_address == NULL)
    {
      *error_message = "This transaction interacts with plutus validators. You must set a collateral change address before calling `build`.";

      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    if (state->collateral_utxos == NULL)
    {
      *error_message = "This transaction interacts with plutus validators. You must set the collateral UTXOs before calling `build`.";

      return CARDANO_ERROR_POINTER_IS_NULL;
    }
  }

  cardano_transaction_t* tx = state->transaction;

  cardano_error_t result = set_dummy_script_data_hash(tx);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_balance_transaction(
    tx,
    state->additional_signature_count,
    state->params,
    state->reference_inputs,
    state->pre_selected_inputs,
    state->input_to_redeemer_map,
    state->available_utxos,
    state->coin_selector,
    state->change_address,
    state->collateral_utxos,
    state->collateral_address,
    state->tx_evaluator,
    state->deferred_redeemers);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = cardano_transaction_get_last_error(tx);

    return result;
  }

  result = update_script_data_hash(state, tx);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_ref(tx);
  *transaction = tx;

  return CARDANO_SUCCESS;
}
