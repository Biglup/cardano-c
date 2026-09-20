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

#include <cardano/common/credential.h>
#include <cardano/common/guard_set.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/scripts/plutus_scripts/plutus_language_version.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/script_data_hash.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "../balancing/internals/collateral.h"

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

/**
 * \brief Checks whether a guard set holds a credential.
 *
 * Two credentials are the same guard only when both their type and their hash match.
 *
 * \param[in] guards A pointer to the \ref cardano_guard_set_t to search, or NULL when the transaction
 *                   carries no guards.
 * \param[in] credential A pointer to the \ref cardano_credential_t to look up.
 *
 * \return true if the guard set holds the credential, false otherwise.
 */
static bool
has_guard(const cardano_guard_set_t* guards, const cardano_credential_t* credential)
{
  const size_t length = cardano_guard_set_get_length(guards);
  bool         found  = false;

  for (size_t i = 0U; (i < length) && !found; ++i)
  {
    cardano_credential_t* guard = NULL;

    const cardano_error_t result = cardano_guard_set_get(guards, i, &guard);
    cardano_credential_unref(&guard);

    found = (result == CARDANO_SUCCESS) && cardano_credential_equals(guard, credential);
  }

  return found;
}

/**
 * \brief Checks that a guard set holds every top level guard a sub transaction requires.
 *
 * \param[in] guards A pointer to the \ref cardano_guard_set_t of the top level transaction, or NULL
 *                   when it carries no guards.
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t whose required top level
 *                            guards are checked.
 * \param[out] error_message A pointer that receives a static string describing the failure when a
 *                           required guard is missing.
 *
 * \return \ref CARDANO_SUCCESS if every required guard is present or the sub transaction requires
 *         none, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if a required guard is missing, or an appropriate
 *         error code indicating the failure reason.
 */
static cardano_error_t
check_sub_transaction_required_guards(
  const cardano_guard_set_t* guards,
  cardano_sub_transaction_t* sub_transaction,
  const char**               error_message)
{
  cardano_sub_transaction_body_t* sub_body = cardano_sub_transaction_get_body(sub_transaction);
  cardano_sub_transaction_body_unref(&sub_body);

  cardano_required_guards_map_t* required_guards = cardano_sub_transaction_body_get_required_top_level_guards(sub_body);
  cardano_required_guards_map_unref(&required_guards);

  const size_t length = cardano_required_guards_map_get_length(required_guards);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_credential_t* credential = NULL;

    const cardano_error_t result = cardano_required_guards_map_get_key_at(required_guards, i, &credential);
    cardano_credential_unref(&credential);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (!has_guard(guards, credential))
    {
      *error_message = "A sub transaction requires a top level guard that the transaction does not carry. You must add it with `cardano_tx_builder_add_guard` before calling `build`.";

      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Checks that the transaction carries every top level guard its sub transactions require.
 *
 * A sub transaction can list the guards it requires from the transaction that carries it, and the
 * batch is only valid when each of them is among the guards of that transaction. Guards are never
 * added on behalf of the batcher, since a key hash guard is also a required signer. Transactions
 * without sub transactions always pass.
 *
 * \param[in] tx A pointer to the \ref cardano_transaction_t to check.
 * \param[out] error_message A pointer that receives a static string describing the failure when a
 *                           required guard is missing.
 *
 * \return \ref CARDANO_SUCCESS if no required guard is missing, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the transaction lacks a guard required by one of its sub transactions, or an appropriate
 *         error code indicating the failure reason.
 */
static cardano_error_t
check_required_top_level_guards(cardano_transaction_t* tx, const char** error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);
  cardano_guard_set_unref(&guards);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  const size_t length = cardano_sub_transaction_set_get_length(sub_transactions);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_sub_transaction_t* sub_transaction = NULL;

    cardano_error_t result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_transaction);
    cardano_sub_transaction_unref(&sub_transaction);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = check_sub_transaction_required_guards(guards, sub_transaction, error_message);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Callback that checks whether a UTXO belongs to a transaction input.
 *
 * \param[in] item A pointer to the \ref cardano_utxo_t to evaluate.
 * \param[in] context A pointer to the \ref cardano_transaction_input_t to match against.
 *
 * \return true if the UTXO belongs to the transaction input, false otherwise.
 */
static bool
find_utxo(cardano_utxo_t* item, const void* context)
{
  const cardano_transaction_input_t* input      = (const cardano_transaction_input_t*)context;
  cardano_transaction_input_t*       utxo_input = cardano_utxo_get_input(item);

  bool found = cardano_transaction_input_equals(utxo_input, input);

  cardano_transaction_input_unref(&utxo_input);

  return found;
}

/**
 * \brief Adds to a list the UTXOs of another list whose input the list does not hold yet.
 *
 * \param[in,out] list A pointer to the \ref cardano_utxo_list_t that receives the UTXOs.
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t with the UTXOs to add.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were added, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
add_utxos_with_new_inputs(cardano_utxo_list_t* list, cardano_utxo_list_t* utxos)
{
  const size_t length = cardano_utxo_list_get_length(utxos);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    cardano_error_t result = cardano_utxo_list_get(utxos, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    cardano_utxo_t* listed_utxo = cardano_utxo_list_find(list, find_utxo, input);
    cardano_utxo_unref(&listed_utxo);

    if (listed_utxo == NULL)
    {
      result = cardano_utxo_list_add(list, utxo);
    }

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Creates the list of available UTXOs the transaction is balanced with.
 *
 * The balancer takes the resolved UTXOs of the sub transactions from the available UTXOs, so the UTXOs
 * the sub transactions spend and reference are added to the ones available for input selection. The
 * batcher can also be a party of the batch, so a UTXO that is already available is not listed again.
 * The list of the state is never modified.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                  construction.
 * \param[out] utxos On success, this will point to the list to balance the transaction with, which is
 *                   the list of available UTXOs of the state itself when the transaction carries no sub
 *                   transactions. The caller is responsible for releasing it with
 *                   \ref cardano_utxo_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the list was created, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
get_balancing_utxos(cardano_builder_state_t* state, cardano_utxo_list_t** utxos)
{
  const size_t sub_transaction_utxo_count = cardano_utxo_list_get_length(state->sub_transaction_inputs) +
    cardano_utxo_list_get_length(state->sub_transaction_reference_inputs);

  if (sub_transaction_utxo_count == 0U)
  {
    cardano_utxo_list_ref(state->available_utxos);
    *utxos = state->available_utxos;

    return CARDANO_SUCCESS;
  }

  cardano_utxo_list_t* balancing_utxos = cardano_utxo_list_clone(state->available_utxos);

  if (balancing_utxos == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = add_utxos_with_new_inputs(balancing_utxos, state->sub_transaction_inputs);

  if (result == CARDANO_SUCCESS)
  {
    result = add_utxos_with_new_inputs(balancing_utxos, state->sub_transaction_reference_inputs);
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&balancing_utxos);

    return result;
  }

  *utxos = balancing_utxos;

  return CARDANO_SUCCESS;
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

  cardano_error_t result = check_required_top_level_guards(state->transaction, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  bool is_collateral_required = false;

  result = _cardano_is_collateral_required(state->transaction, &is_collateral_required);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (cardano_transaction_has_script_data(state->transaction) || is_collateral_required)
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

  result = set_dummy_script_data_hash(tx);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_utxo_list_t* balancing_utxos = NULL;

  result = get_balancing_utxos(state, &balancing_utxos);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to gather the UTXOs of the sub transactions.";

    return result;
  }

  result = cardano_balance_transaction(
    tx,
    state->additional_signature_count,
    state->params,
    state->reference_inputs,
    state->pre_selected_inputs,
    state->input_to_redeemer_map,
    balancing_utxos,
    state->coin_selector,
    state->change_address,
    state->collateral_utxos,
    state->collateral_address,
    state->tx_evaluator,
    state->deferred_redeemers);

  cardano_utxo_list_unref(&balancing_utxos);

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
