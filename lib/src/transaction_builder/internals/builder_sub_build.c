/**
 * \file builder_sub_build.c
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

#include "builder_sub_build.h"
#include "builder_build.h"

#include <cardano/common/ex_units.h>
#include <cardano/common/protocol_version.h>
#include <cardano/common/utxo.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Creates the input set that spends a list of resolved UTXOs.
 *
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t with the UTXOs to spend.
 * \param[out] inputs On success, this will point to a newly created \ref cardano_transaction_input_set_t
 *                    with the input of every UTXO in \p utxos. The caller is responsible for releasing
 *                    it with \ref cardano_transaction_input_set_unref.
 *
 * \return \ref CARDANO_SUCCESS if the input set was created, or an appropriate error code indicating
 *         the failure reason.
 */
static cardano_error_t
create_input_set(cardano_utxo_list_t* utxos, cardano_transaction_input_set_t** inputs)
{
  cardano_transaction_input_set_t* input_set = NULL;
  cardano_error_t                  result    = cardano_transaction_input_set_new(&input_set);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t length = cardano_utxo_list_get_length(utxos);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(utxos, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_input_set_unref(&input_set);

      return result;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    result = cardano_transaction_input_set_add(input_set, input);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_input_set_unref(&input_set);

      return result;
    }
  }

  *inputs = input_set;

  return CARDANO_SUCCESS;
}

/**
 * \brief Copies the optional scalar fields of a transaction body into a sub transaction body.
 *
 * This function copies the validity interval, the network id, the treasury value and the donation.
 * A field that is absent in \p body is left absent in \p sub_body.
 *
 * \param[in] body A pointer to the \ref cardano_transaction_body_t to copy from.
 * \param[in,out] sub_body A pointer to the \ref cardano_sub_transaction_body_t to copy into.
 *
 * \return \ref CARDANO_SUCCESS if every field was copied, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
copy_scalar_fields(cardano_transaction_body_t* body, cardano_sub_transaction_body_t* sub_body)
{
  cardano_error_t result = cardano_sub_transaction_body_set_invalid_before(sub_body, cardano_transaction_body_get_invalid_before(body));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_sub_transaction_body_set_invalid_after(sub_body, cardano_transaction_body_get_invalid_after(body));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_sub_transaction_body_set_network_id(sub_body, cardano_transaction_body_get_network_id(body));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_sub_transaction_body_set_treasury_value(sub_body, cardano_transaction_body_get_treasury_value(body));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_sub_transaction_body_set_donation(sub_body, cardano_transaction_body_get_donation(body));
}

/**
 * \brief Copies the optional object fields of a transaction body into a sub transaction body.
 *
 * This function copies the mint, certificates, withdrawals, auxiliary data hash, script data hash,
 * reference inputs, voting procedures, proposal procedures, guards, required top level guards, direct
 * deposits and account balance intervals. The objects are shared between both bodies, and a field
 * that is absent in \p body is left absent in \p sub_body.
 *
 * \param[in] body A pointer to the \ref cardano_transaction_body_t to copy from.
 * \param[in,out] sub_body A pointer to the \ref cardano_sub_transaction_body_t to copy into.
 *
 * \return \ref CARDANO_SUCCESS if every field was copied, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
copy_object_fields(cardano_transaction_body_t* body, cardano_sub_transaction_body_t* sub_body)
{
  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  cardano_error_t result = cardano_sub_transaction_body_set_mint(sub_body, mint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certificates);

  result = cardano_sub_transaction_body_set_certificates(sub_body, certificates);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
  cardano_withdrawal_map_unref(&withdrawals);

  result = cardano_sub_transaction_body_set_withdrawals(sub_body, withdrawals);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_t* aux_data_hash = cardano_transaction_body_get_aux_data_hash(body);
  cardano_blake2b_hash_unref(&aux_data_hash);

  result = cardano_sub_transaction_body_set_aux_data_hash(sub_body, aux_data_hash);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_t* script_data_hash = cardano_transaction_body_get_script_data_hash(body);
  cardano_blake2b_hash_unref(&script_data_hash);

  result = cardano_sub_transaction_body_set_script_data_hash(sub_body, script_data_hash);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_input_set_t* reference_inputs = cardano_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_unref(&reference_inputs);

  result = cardano_sub_transaction_body_set_reference_inputs(sub_body, reference_inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_voting_procedures_t* voting_procedures = cardano_transaction_body_get_voting_procedures(body);
  cardano_voting_procedures_unref(&voting_procedures);

  result = cardano_sub_transaction_body_set_voting_procedures(sub_body, voting_procedures);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_proposal_procedure_set_t* proposal_procedures = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&proposal_procedures);

  result = cardano_sub_transaction_body_set_proposal_procedures(sub_body, proposal_procedures);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);
  cardano_guard_set_unref(&guards);

  result = cardano_sub_transaction_body_set_guards(sub_body, guards);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_required_guards_map_t* required_top_level_guards = cardano_transaction_body_get_required_top_level_guards(body);
  cardano_required_guards_map_unref(&required_top_level_guards);

  result = cardano_sub_transaction_body_set_required_top_level_guards(sub_body, required_top_level_guards);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_direct_deposit_map_t* direct_deposits = cardano_transaction_body_get_direct_deposits(body);
  cardano_direct_deposit_map_unref(&direct_deposits);

  result = cardano_sub_transaction_body_set_direct_deposits(sub_body, direct_deposits);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_account_balance_intervals_map_t* account_balance_intervals = cardano_transaction_body_get_account_balance_intervals(body);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals);

  return cardano_sub_transaction_body_set_account_balance_intervals(sub_body, account_balance_intervals);
}

/**
 * \brief Creates a top level transaction that carries a sub transaction and nothing else.
 *
 * The sub transaction is the single sub transaction of the carrier, so its scripts are evaluated
 * at position 0 of the batch. The script context of a sub transaction carries its position in the
 * batch, so a script whose path or cost depends on that position may need different execution
 * units in the final batch.
 *
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t to carry.
 * \param[out] transaction On success, this will point to the newly created \ref cardano_transaction_t.
 *                         The caller is responsible for releasing it with \ref cardano_transaction_unref.
 *
 * \return \ref CARDANO_SUCCESS if the transaction was created, or an appropriate error code indicating
 *         the failure reason.
 */
static cardano_error_t
create_carrier_transaction(cardano_sub_transaction_t* sub_transaction, cardano_transaction_t** transaction)
{
  cardano_transaction_input_set_t*   inputs           = NULL;
  cardano_transaction_output_list_t* outputs          = NULL;
  cardano_transaction_body_t*        body             = NULL;
  cardano_sub_transaction_set_t*     sub_transactions = NULL;
  cardano_witness_set_t*             witness_set      = NULL;
  cardano_error_t                    result           = cardano_transaction_input_set_new(&inputs);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_output_list_new(&outputs);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_body_new(inputs, outputs, 0U, NULL, &body);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_sub_transaction_set_new(&sub_transactions);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_sub_transaction_set_add(sub_transactions, sub_transaction);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_body_set_sub_transactions(body, sub_transactions);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_witness_set_new(&witness_set);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_new(body, witness_set, NULL, transaction);
  }

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_body_unref(&body);
  cardano_sub_transaction_set_unref(&sub_transactions);
  cardano_witness_set_unref(&witness_set);

  return result;
}

/**
 * \brief Writes evaluated execution units into the matching redeemers of a redeemer list.
 *
 * \param[in,out] redeemers A pointer to the \ref cardano_redeemer_list_t whose redeemers are updated.
 * \param[in] evaluated A pointer to the \ref cardano_redeemer_list_t holding the evaluated redeemers.
 *
 * \return \ref CARDANO_SUCCESS if every redeemer was updated, or an appropriate error code indicating
 *         the failure reason.
 */
static cardano_error_t
apply_execution_units(cardano_redeemer_list_t* redeemers, cardano_redeemer_list_t* evaluated)
{
  cardano_error_t result = CARDANO_SUCCESS;
  const size_t    length = cardano_redeemer_list_get_length(evaluated);

  for (size_t i = 0U; (i < length) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    result = cardano_redeemer_list_get(evaluated, i, &redeemer);
    cardano_redeemer_unref(&redeemer);

    if (result == CARDANO_SUCCESS)
    {
      cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(redeemer);
      cardano_ex_units_unref(&ex_units);

      result = cardano_redeemer_list_set_ex_units(
        redeemers,
        cardano_redeemer_get_tag(redeemer),
        cardano_redeemer_get_index(redeemer),
        cardano_ex_units_get_memory(ex_units),
        cardano_ex_units_get_cpu_steps(ex_units));
    }
  }

  return result;
}

/**
 * \brief Computes the execution units of the redeemers of a sub transaction.
 *
 * The sub transaction is evaluated with the native evaluator configured from the cost models and the
 * protocol version of the state, resolving its inputs and reference inputs with the UTXOs added to
 * the state. The computed execution units are written into the redeemers of the witness set the sub
 * transaction shares with the state. A sub transaction without redeemers is left unchanged.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the sub transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t to evaluate.
 * \param[out] error_message A pointer that receives a string describing the failure when the function
 *                           does not return \ref CARDANO_SUCCESS. The string is static or owned by the
 *                           transaction held by the state. \c *error_message must be NULL on entry,
 *                           since the generic failure message is only set when no message was set.
 *
 * \return \ref CARDANO_SUCCESS if the redeemers were evaluated, or an appropriate error code indicating
 *         the failure reason.
 */
static cardano_error_t
evaluate_sub_transaction(
  cardano_builder_state_t*   state,
  cardano_sub_transaction_t* sub_transaction,
  const char**               error_message)
{
  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  if (cardano_redeemer_list_get_length(redeemers) == 0U)
  {
    return CARDANO_SUCCESS;
  }

  cardano_costmdls_t*         cost_models = cardano_protocol_parameters_get_cost_models(state->params);
  cardano_protocol_version_t* version     = cardano_protocol_parameters_get_protocol_version(state->params);
  cardano_transaction_t*      carrier     = NULL;
  cardano_utxo_list_t*        utxos       = NULL;
  cardano_tx_evaluator_t*     evaluator   = NULL;
  cardano_redeemer_list_t*    evaluated   = NULL;
  cardano_error_t             result      = cardano_tx_evaluator_new_native_sub_transaction(&state->slot_config, cost_models, cardano_protocol_version_get_major(version), 0U, &evaluator);

  if (result == CARDANO_SUCCESS)
  {
    result = create_carrier_transaction(sub_transaction, &carrier);
  }

  if (result == CARDANO_SUCCESS)
  {
    utxos  = cardano_utxo_list_concat(state->pre_selected_inputs, state->reference_inputs);
    result = (utxos != NULL) ? CARDANO_SUCCESS : CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_tx_evaluator_evaluate(evaluator, carrier, utxos, &evaluated);

    const char* evaluator_error = cardano_tx_evaluator_get_last_error(evaluator);

    if ((result != CARDANO_SUCCESS) && (evaluator_error[0] != '\0'))
    {
      cardano_transaction_set_last_error(state->transaction, evaluator_error);
      *error_message = cardano_transaction_get_last_error(state->transaction);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = apply_execution_units(redeemers, evaluated);
  }

  if ((result != CARDANO_SUCCESS) && (*error_message == NULL))
  {
    *error_message = "Failed to evaluate the redeemers of the sub transaction.";
  }

  cardano_redeemer_list_unref(&evaluated);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&carrier);
  cardano_protocol_version_unref(&version);
  cardano_costmdls_unref(&cost_models);

  return result;
}

/**
 * \brief Evaluates the redeemers of a built sub transaction and sets its script data hash.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the sub transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in,out] sub_transaction A pointer to the \ref cardano_sub_transaction_t to finalize.
 * \param[out] error_message A pointer that receives a string describing the failure when the function
 *                           does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the sub transaction was finalized, or an appropriate error code
 *         indicating the failure reason.
 */
static cardano_error_t
finalize_script_data(
  cardano_builder_state_t*   state,
  cardano_sub_transaction_t* sub_transaction,
  const char**               error_message)
{
  cardano_error_t result = evaluate_sub_transaction(state, sub_transaction, error_message);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_builder_update_script_data_hash(state, state->transaction);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to compute the script data hash of the sub transaction.";
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
    cardano_transaction_body_unref(&body);

    cardano_blake2b_hash_t* script_data_hash = cardano_transaction_body_get_script_data_hash(body);
    cardano_blake2b_hash_unref(&script_data_hash);

    cardano_sub_transaction_body_t* sub_body = cardano_sub_transaction_get_body(sub_transaction);
    cardano_sub_transaction_body_unref(&sub_body);

    result = cardano_sub_transaction_body_set_script_data_hash(sub_body, script_data_hash);
    cardano_sub_transaction_clear_cbor_cache(sub_transaction);
  }

  return result;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_build_sub_transaction(
  cardano_builder_state_t*    state,
  cardano_sub_transaction_t** sub_transaction,
  const char**                error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_error_t result = cardano_builder_set_dummy_script_data_hash(state->transaction);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set the script data hash of the sub transaction.";

    return result;
  }

  cardano_transaction_input_set_t* inputs = NULL;

  result = create_input_set(state->pre_selected_inputs, &inputs);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the sub transaction inputs.";

    return result;
  }

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_sub_transaction_body_t* sub_body = NULL;

  result = cardano_sub_transaction_body_new(inputs, outputs, NULL, &sub_body);
  cardano_transaction_input_set_unref(&inputs);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the sub transaction body.";

    return result;
  }

  result = copy_scalar_fields(body, sub_body);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to copy the sub transaction body fields.";
    cardano_sub_transaction_body_unref(&sub_body);

    return result;
  }

  result = copy_object_fields(body, sub_body);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to copy the sub transaction body fields.";
    cardano_sub_transaction_body_unref(&sub_body);

    return result;
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witness_set);

  cardano_auxiliary_data_t* auxiliary_data = cardano_transaction_get_auxiliary_data(state->transaction);
  cardano_auxiliary_data_unref(&auxiliary_data);

  result = cardano_sub_transaction_new(sub_body, witness_set, auxiliary_data, sub_transaction);
  cardano_sub_transaction_body_unref(&sub_body);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the sub transaction.";

    return result;
  }

  result = finalize_script_data(state, *sub_transaction, error_message);

  if (result != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_unref(sub_transaction);
  }

  return result;
}
