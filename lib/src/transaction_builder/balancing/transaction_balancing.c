/**
 * \file transaction_balancing.c
 *
 * \author angel.castillo
 * \date   Nov 01, 2024
 *
 * Copyright 2024 Biglup Labs
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

#include "internals/collateral.h"
#include "internals/unique_signers.h"
#include <cardano/transaction_builder/balancing/implicit_coin.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/fee.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Callback function to find a specific UTXO based on the provided context.
 *
 * This function checks if the provided UTXO matches the criteria specified in `context`.
 *
 * \param[in] item The UTXO to evaluate.
 * \param[in] context A pointer to the search criteria, typically a structure or identifier.
 *
 * \return `true` if the UTXO matches the search criteria; otherwise, `false`.
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
 * \brief Coalesces all input values in a transaction input set into a single total value.
 *
 * This function iterates over each input in the provided transaction input set, looks up the resolved value
 * in the corresponding UTXO list, and accumulates it into a single total input value.
 *
 * \param[in]  inputs            The set of transaction inputs to coalesce.
 * \param[in]  resolved_inputs   The UTXO list containing resolved values for each input.
 * \param[out] total_input_value A pointer to store the cumulative value of all inputs.
 *
 * \return \ref CARDANO_SUCCESS if the total input value was successfully calculated, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `total_input_value` when it is no longer needed.
 */
static cardano_error_t
coalesce_all_inputs(
  cardano_transaction_input_set_t* inputs,
  cardano_utxo_list_t*             resolved_inputs,
  cardano_value_t**                total_input_value)
{
  const size_t num_inputs = cardano_transaction_input_set_get_length(inputs);

  cardano_value_t* total_value = cardano_value_new_zero();

  if (total_value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0U; i < num_inputs; ++i)
  {
    cardano_transaction_input_t* input  = NULL;
    cardano_error_t              result = cardano_transaction_input_set_get(inputs, i, &input);
    cardano_transaction_input_unref(&input);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&total_value);

      return result;
    }

    cardano_utxo_t* utxo = cardano_utxo_list_find(resolved_inputs, find_utxo, (void*)input);
    cardano_utxo_unref(&utxo);

    if (utxo == NULL)
    {
      cardano_value_unref(&total_value);

      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_value_t*              value  = cardano_transaction_output_get_value(output);

    cardano_transaction_output_unref(&output);
    cardano_value_unref(&value);

    cardano_value_t* tmp_value = NULL;

    result = cardano_value_add(total_value, value, &tmp_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&tmp_value);
      cardano_value_unref(&total_value);

      return result;
    }

    cardano_value_unref(&total_value);
    total_value = tmp_value;
  }

  *total_input_value = total_value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Coalesces all output values in a transaction output list into a single total value.
 *
 * This function iterates over each transaction output in the provided list, extracts its value,
 * and accumulates it into a single total output value.
 *
 * \param[in]  outputs            The list of transaction outputs to coalesce.
 * \param[out] total_output_value  A pointer to store the cumulative value of all outputs.
 *
 * \return \ref CARDANO_SUCCESS if the total output value was successfully calculated, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `total_output_value` when it is no longer needed.
 */
static cardano_error_t
coalesce_all_outputs(cardano_transaction_output_list_t* outputs, cardano_value_t** total_output_value)
{
  const size_t num_outputs = cardano_transaction_output_list_get_length(outputs);

  cardano_value_t* total_value = cardano_value_new_zero();

  if (total_value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0U; i < num_outputs; ++i)
  {
    cardano_transaction_output_t* output = NULL;
    cardano_error_t               result = cardano_transaction_output_list_get(outputs, i, &output);
    cardano_transaction_output_unref(&output);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&total_value);

      return result;
    }

    cardano_value_t* value = cardano_transaction_output_get_value(output);
    cardano_value_unref(&value);

    cardano_value_t* tmp_value = NULL;

    result = cardano_value_add(total_value, value, &tmp_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&tmp_value);
      cardano_value_unref(&total_value);

      return result;
    }

    cardano_value_unref(&total_value);
    total_value = tmp_value;
  }

  *total_output_value = total_value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Sums the lovelace amounts of a direct deposit map.
 *
 * \param[in]  direct_deposits The direct deposit map of the body, or NULL when the body has none.
 * \param[out] total           A pointer to store the sum of all direct deposit amounts.
 *
 * \return \ref CARDANO_SUCCESS if the total was computed, or an appropriate error code.
 */
static cardano_error_t
sum_direct_deposits(cardano_direct_deposit_map_t* direct_deposits, uint64_t* total)
{
  const size_t num_deposits = cardano_direct_deposit_map_get_length(direct_deposits);

  *total = 0U;

  for (size_t i = 0U; i < num_deposits; ++i)
  {
    uint64_t amount = 0U;

    cardano_error_t result = cardano_direct_deposit_map_get_value_at(direct_deposits, i, &amount);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    *total += amount;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Adds every asset of one policy to a value.
 *
 * \param[in,out] value     The value that receives the assets.
 * \param[in]     policy_id The policy id of the assets.
 * \param[in]     assets    The asset name map holding the quantity of each asset.
 *
 * \return \ref CARDANO_SUCCESS if every asset was added, or an appropriate error code.
 */
static cardano_error_t
add_policy_assets(cardano_value_t* value, cardano_blake2b_hash_t* policy_id, cardano_asset_name_map_t* assets)
{
  const size_t asset_count = cardano_asset_name_map_get_length(assets);

  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < asset_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_asset_name_t* asset_name = NULL;
    int64_t               quantity   = 0;

    result = cardano_asset_name_map_get_key_value_at(assets, i, &asset_name, &quantity);
    cardano_asset_name_unref(&asset_name);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_value_add_asset(value, policy_id, asset_name, quantity);
    }
  }

  return result;
}

/**
 * \brief Creates a value with the same coin and assets as another value but backed by its own multi asset object.
 *
 * Value arithmetic shares the multi asset of the non-empty operand when the other operand carries no assets, and
 * merging two multi assets shares the asset name maps of the policies that only one of them holds. The outcome of a
 * chain of additions and subtractions can therefore still reference the maps of one of its inputs. This function
 * rebuilds the value asset by asset so that it can be modified without affecting any of those inputs.
 *
 * \param[in]  value A pointer to the value to copy.
 * \param[out] copy  A pointer to store the independent copy of the value.
 *
 * \return \ref CARDANO_SUCCESS if the copy was created, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `copy` when it is no longer needed.
 */
static cardano_error_t
copy_value(cardano_value_t* value, cardano_value_t** copy)
{
  cardano_error_t result = cardano_value_new(cardano_value_get_coin(value), NULL, copy);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  cardano_policy_id_list_t* policies = NULL;

  result = cardano_multi_asset_get_keys(multi_asset, &policies);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(copy);

    return result;
  }

  const size_t policy_count = cardano_policy_id_list_get_length(policies);

  for (size_t i = 0U; (i < policy_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_blake2b_hash_t*   policy_id = NULL;
    cardano_asset_name_map_t* assets    = NULL;

    result = cardano_policy_id_list_get(policies, i, &policy_id);
    cardano_blake2b_hash_unref(&policy_id);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_multi_asset_get_assets(multi_asset, policy_id, &assets);
      cardano_asset_name_map_unref(&assets);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = add_policy_assets(*copy, policy_id, assets);
    }
  }

  cardano_policy_id_list_unref(&policies);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(copy);
  }

  return result;
}

/**
 * \brief Computes the imbalance of a body as the value it consumes minus the value it produces.
 *
 * The consumed value is the sum of the resolved inputs plus the implicit consumed coin and the mint field (where
 * minted assets are positive and burned assets negative). The produced value is the sum of the outputs plus the
 * implicit produced coin.
 *
 * \param[in]  inputs          The set of inputs of the body.
 * \param[in]  resolved_inputs The UTXO list containing resolved values for each input.
 * \param[in]  outputs         The list of outputs of the body.
 * \param[in]  mint            The mint field of the body, or NULL when the body has none.
 * \param[in]  consumed_coin   The implicit consumed coin: the withdrawals plus the deposit refunds.
 * \param[in]  produced_coin   The implicit produced coin: the fee, the deposits, the donation and the direct deposits.
 * \param[out] imbalance       A pointer to store the resulting imbalance value. The value owns its multi asset, so it
 *                             never aliases the mint field or a resolved input and can be modified freely.
 *
 * \return \ref CARDANO_SUCCESS if the imbalance was computed, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `imbalance` when it is no longer needed.
 */
static cardano_error_t
compute_imbalance(
  cardano_transaction_input_set_t*   inputs,
  cardano_utxo_list_t*               resolved_inputs,
  cardano_transaction_output_list_t* outputs,
  cardano_multi_asset_t*             mint,
  const int64_t                      consumed_coin,
  const int64_t                      produced_coin,
  cardano_value_t**                  imbalance)
{
  cardano_value_t* implicit_consumed = NULL;
  cardano_value_t* implicit_produced = NULL;

  cardano_error_t result = cardano_value_new(consumed_coin, mint, &implicit_consumed);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_value_new(produced_coin, NULL, &implicit_produced);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_consumed);

    return result;
  }

  cardano_value_t* total_input_value  = NULL;
  cardano_value_t* total_output_value = NULL;
  cardano_value_t* consumed_value     = NULL;
  cardano_value_t* diff_value         = NULL;

  result = coalesce_all_inputs(inputs, resolved_inputs, &total_input_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_consumed);
    cardano_value_unref(&implicit_produced);

    return result;
  }

  result = coalesce_all_outputs(outputs, &total_output_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_consumed);
    cardano_value_unref(&implicit_produced);
    cardano_value_unref(&total_input_value);

    return result;
  }

  result = cardano_value_add(total_input_value, implicit_consumed, &consumed_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_consumed);
    cardano_value_unref(&implicit_produced);
    cardano_value_unref(&total_input_value);
    cardano_value_unref(&total_output_value);

    return result;
  }

  result = cardano_value_subtract(consumed_value, total_output_value, &diff_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_consumed);
    cardano_value_unref(&implicit_produced);
    cardano_value_unref(&total_input_value);
    cardano_value_unref(&total_output_value);
    cardano_value_unref(&consumed_value);

    return result;
  }

  cardano_value_t* imbalance_value = NULL;

  result = cardano_value_subtract(diff_value, implicit_produced, &imbalance_value);

  cardano_value_unref(&implicit_consumed);
  cardano_value_unref(&implicit_produced);
  cardano_value_unref(&total_input_value);
  cardano_value_unref(&total_output_value);
  cardano_value_unref(&consumed_value);
  cardano_value_unref(&diff_value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = copy_value(imbalance_value, imbalance);

  cardano_value_unref(&imbalance_value);

  return result;
}

/**
 * \brief Adds the imbalance of a sub transaction to a running total.
 *
 * \param[in]     sub_tx          The sub transaction whose imbalance is added.
 * \param[in]     resolved_inputs The UTXO list containing resolved values for each input of the sub transaction.
 * \param[in]     protocol_params The protocol parameters supplying the deposit amounts.
 * \param[in,out] total           The running total. On success it is replaced by a new value holding the sum; on
 *                                failure it is left untouched.
 *
 * \return \ref CARDANO_SUCCESS if the imbalance was added, or an appropriate error code.
 */
static cardano_error_t
add_sub_transaction_imbalance(
  cardano_sub_transaction_t*     sub_tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              total)
{
  cardano_value_t* sub_imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol_params, &sub_imbalance);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_value_t* tmp_value = NULL;

  result = cardano_value_add(*total, sub_imbalance, &tmp_value);

  cardano_value_unref(&sub_imbalance);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&tmp_value);

    return result;
  }

  cardano_value_unref(total);
  *total = tmp_value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Adds the imbalance of every sub transaction of a set to a running total.
 *
 * \param[in]     sub_transactions The sub transactions whose imbalances are added, or NULL when the body has none.
 * \param[in]     resolved_inputs  The UTXO list containing resolved values for each input of the sub transactions.
 * \param[in]     protocol_params  The protocol parameters supplying the deposit amounts.
 * \param[in,out] total            The running total. On success it holds the sum; on failure the caller must still
 *                                 release it.
 *
 * \return \ref CARDANO_SUCCESS if every imbalance was added, or an appropriate error code.
 */
static cardano_error_t
add_sub_transactions_imbalance(
  cardano_sub_transaction_set_t* sub_transactions,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              total)
{
  const size_t num_sub_transactions = cardano_sub_transaction_set_get_length(sub_transactions);

  for (size_t i = 0U; i < num_sub_transactions; ++i)
  {
    cardano_sub_transaction_t* sub_tx = NULL;

    cardano_error_t result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_tx);
    cardano_sub_transaction_unref(&sub_tx);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = add_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol_params, total);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Checks whether a set of transaction inputs holds a transaction input.
 *
 * \param[in]  inputs    The transaction inputs to search, or NULL when there are none.
 * \param[in]  input     The transaction input to look up.
 * \param[out] has_input Set to true if the set holds the input.
 *
 * \return \ref CARDANO_SUCCESS if the inputs were searched, or an appropriate error code.
 */
static cardano_error_t
has_transaction_input(
  cardano_transaction_input_set_t*   inputs,
  const cardano_transaction_input_t* input,
  bool*                              has_input)
{
  const size_t num_inputs = cardano_transaction_input_set_get_length(inputs);

  bool found = false;

  for (size_t i = 0U; (i < num_inputs) && !found; ++i)
  {
    cardano_transaction_input_t* current_input = NULL;

    cardano_error_t result = cardano_transaction_input_set_get(inputs, i, &current_input);
    cardano_transaction_input_unref(&current_input);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    found = cardano_transaction_input_equals(current_input, input);
  }

  *has_input = found;

  return CARDANO_SUCCESS;
}

/**
 * \brief Checks whether a sub transaction uses a transaction input.
 *
 * A sub transaction uses the inputs it spends and, if \p include_references is set, the inputs it references.
 *
 * \param[in]  sub_tx             The sub transaction whose inputs are searched.
 * \param[in]  input              The transaction input to look up.
 * \param[in]  include_references Whether the reference inputs of the sub transaction are searched too.
 * \param[out] is_used            Set to true if the sub transaction uses the input.
 *
 * \return \ref CARDANO_SUCCESS if the inputs were searched, or an appropriate error code.
 */
static cardano_error_t
is_used_by_sub_transaction(
  cardano_sub_transaction_t*         sub_tx,
  const cardano_transaction_input_t* input,
  const bool                         include_references,
  bool*                              is_used)
{
  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  const cardano_error_t result = has_transaction_input(inputs, input, is_used);

  if ((result != CARDANO_SUCCESS) || *is_used || !include_references)
  {
    return result;
  }

  cardano_transaction_input_set_t* reference_inputs = cardano_sub_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_unref(&reference_inputs);

  return has_transaction_input(reference_inputs, input, is_used);
}

/**
 * \brief Checks whether any sub transaction of a set uses a transaction input.
 *
 * A sub transaction uses the inputs it spends and, if \p include_references is set, the inputs it references.
 *
 * \param[in]  sub_transactions   The sub transactions whose inputs are searched.
 * \param[in]  input              The transaction input to look up.
 * \param[in]  include_references Whether the reference inputs of the sub transactions are searched too.
 * \param[out] is_used            Set to true if one of the sub transactions uses the input.
 *
 * \return \ref CARDANO_SUCCESS if the sub transactions were searched, or an appropriate error code.
 */
static cardano_error_t
is_used_by_sub_transactions(
  cardano_sub_transaction_set_t*     sub_transactions,
  const cardano_transaction_input_t* input,
  const bool                         include_references,
  bool*                              is_used)
{
  const size_t num_sub_transactions = cardano_sub_transaction_set_get_length(sub_transactions);

  bool found = false;

  for (size_t i = 0U; (i < num_sub_transactions) && !found; ++i)
  {
    cardano_sub_transaction_t* sub_tx = NULL;

    cardano_error_t result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_tx);
    cardano_sub_transaction_unref(&sub_tx);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = is_used_by_sub_transaction(sub_tx, input, include_references, &found);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  *is_used = found;

  return CARDANO_SUCCESS;
}

/**
 * \brief Checks whether any UTXO of a list is spent by a sub transaction of a set.
 *
 * \param[in]  utxos            The UTXOs to look up, or NULL when there are none.
 * \param[in]  sub_transactions The sub transactions whose spend inputs are searched, or NULL when the body has none.
 * \param[out] has_spent_utxo   Set to true if one of the sub transactions spends one of the UTXOs.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were searched, or an appropriate error code.
 */
static cardano_error_t
has_utxo_spent_by_sub_transactions(
  cardano_utxo_list_t*           utxos,
  cardano_sub_transaction_set_t* sub_transactions,
  bool*                          has_spent_utxo)
{
  const size_t num_utxos = cardano_utxo_list_get_length(utxos);

  bool found = false;

  for (size_t i = 0U; (i < num_utxos) && !found; ++i)
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

    result = is_used_by_sub_transactions(sub_transactions, input, false, &found);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  *has_spent_utxo = found;

  return CARDANO_SUCCESS;
}

/**
 * \brief Creates the list of UTXOs that coin selection may spend at the top level of a batch.
 *
 * The available UTXOs also hold the resolved UTXOs of the sub transactions, which belong to the parties of the batch
 * and to the holders of the scripts they use, so every available UTXO that a sub transaction spends or references is
 * left out. The order of the remaining UTXOs is preserved.
 *
 * \param[in]  available_utxo   The list of available UTXOs.
 * \param[in]  sub_transactions The sub transactions carried by the transaction, or NULL when it has none.
 * \param[out] selectable_utxo  A pointer to store the selectable UTXOs, which is \p available_utxo itself when the
 *                              transaction carries no sub transactions.
 *
 * \return \ref CARDANO_SUCCESS if the list was created, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `selectable_utxo` when it is no longer needed.
 */
static cardano_error_t
exclude_sub_transaction_inputs(
  cardano_utxo_list_t*           available_utxo,
  cardano_sub_transaction_set_t* sub_transactions,
  cardano_utxo_list_t**          selectable_utxo)
{
  if ((available_utxo == NULL) || (cardano_sub_transaction_set_get_length(sub_transactions) == 0U))
  {
    cardano_utxo_list_ref(available_utxo);
    *selectable_utxo = available_utxo;

    return CARDANO_SUCCESS;
  }

  cardano_utxo_list_t* utxos  = NULL;
  cardano_error_t      result = cardano_utxo_list_new(&utxos);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t num_utxos = cardano_utxo_list_get_length(available_utxo);

  for (size_t i = 0U; i < num_utxos; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(available_utxo, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(&utxos);

      return result;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    bool is_used = false;

    result = is_used_by_sub_transactions(sub_transactions, input, true, &is_used);

    if ((result == CARDANO_SUCCESS) && !is_used)
    {
      result = cardano_utxo_list_add(utxos, utxo);
    }

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(&utxos);

      return result;
    }
  }

  *selectable_utxo = utxos;

  return CARDANO_SUCCESS;
}

/**
 * \brief Checks whether a list of resolved UTXOs carries a PlutusV1, PlutusV2 or PlutusV3 reference script.
 *
 * \param[in]  utxos             The resolved UTXOs to inspect, or NULL when there are none.
 * \param[out] has_legacy_script Set to true if one of the reference scripts is a PlutusV1, PlutusV2 or PlutusV3 script.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were inspected, or an appropriate error code.
 */
static cardano_error_t
has_legacy_reference_script(cardano_utxo_list_t* utxos, bool* has_legacy_script)
{
  const size_t num_utxos = cardano_utxo_list_get_length(utxos);

  bool found = false;

  for (size_t i = 0U; (i < num_utxos) && !found; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    cardano_error_t result = cardano_utxo_list_get(utxos, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_transaction_output_unref(&output);

    cardano_script_t* script = cardano_transaction_output_get_script_ref(output);
    cardano_script_unref(&script);

    if (script == NULL)
    {
      continue;
    }

    cardano_script_language_t language = CARDANO_SCRIPT_LANGUAGE_NATIVE;

    result = cardano_script_get_language(script, &language);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    found = (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1) ||
      (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2) ||
      (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3);
  }

  *has_legacy_script = found;

  return CARDANO_SUCCESS;
}

/**
 * \brief Checks whether the ledger validates a transaction in legacy mode.
 *
 * A top level transaction that uses a PlutusV1, PlutusV2 or PlutusV3 script must also conserve value by itself, with
 * its sub transactions removed. The scripts of the top level witness set, the reference scripts of the resolved top
 * level reference inputs and the reference scripts of the pre selected inputs are inspected.
 *
 * \param[in]  tx                The top level transaction.
 * \param[in]  reference_inputs  The resolved reference inputs of the transaction, or NULL when it has none.
 * \param[in]  pre_selected_utxo The UTXOs that must be included in the transaction inputs, or NULL when there are none.
 * \param[out] is_legacy_mode    Set to true if the transaction uses a PlutusV1, PlutusV2 or PlutusV3 script.
 *
 * \return \ref CARDANO_SUCCESS if the transaction was inspected, or an appropriate error code.
 */
static cardano_error_t
is_legacy_mode_transaction(
  cardano_transaction_t* tx,
  cardano_utxo_list_t*   reference_inputs,
  cardano_utxo_list_t*   pre_selected_utxo,
  bool*                  is_legacy_mode)
{
  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witnesses);

  cardano_plutus_v1_script_set_t* plutus_v1_scripts = cardano_witness_set_get_plutus_v1_scripts(witnesses);
  cardano_plutus_v2_script_set_t* plutus_v2_scripts = cardano_witness_set_get_plutus_v2_scripts(witnesses);
  cardano_plutus_v3_script_set_t* plutus_v3_scripts = cardano_witness_set_get_plutus_v3_scripts(witnesses);

  cardano_plutus_v1_script_set_unref(&plutus_v1_scripts);
  cardano_plutus_v2_script_set_unref(&plutus_v2_scripts);
  cardano_plutus_v3_script_set_unref(&plutus_v3_scripts);

  *is_legacy_mode = (cardano_plutus_v1_script_set_get_length(plutus_v1_scripts) > 0U) ||
    (cardano_plutus_v2_script_set_get_length(plutus_v2_scripts) > 0U) ||
    (cardano_plutus_v3_script_set_get_length(plutus_v3_scripts) > 0U);

  if (*is_legacy_mode)
  {
    return CARDANO_SUCCESS;
  }

  const cardano_error_t result = has_legacy_reference_script(reference_inputs, is_legacy_mode);

  if ((result != CARDANO_SUCCESS) || *is_legacy_mode)
  {
    return result;
  }

  return has_legacy_reference_script(pre_selected_utxo, is_legacy_mode);
}

/**
 * \brief Adds to a list the UTXOs of another list whose input the list does not hold yet.
 *
 * \param[in,out] list  The list that receives the UTXOs.
 * \param[in]     utxos The UTXOs to add, or NULL when there are none.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were added, or an appropriate error code.
 */
static cardano_error_t
add_utxos_with_new_inputs(cardano_utxo_list_t* list, cardano_utxo_list_t* utxos)
{
  const size_t num_utxos = cardano_utxo_list_get_length(utxos);

  for (size_t i = 0U; i < num_utxos; ++i)
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

    cardano_utxo_t* listed_utxo = cardano_utxo_list_find(list, find_utxo, (void*)input);
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
 * \brief Looks up the resolved UTXO of a transaction input among the lists of UTXOs given to the balancer.
 *
 * The available UTXOs are searched first, then the resolved reference inputs of the top level transaction and then the
 * pre selected UTXOs, since a UTXO may be shared between the top level transaction and its sub transactions.
 *
 * \param[in] input             The transaction input to resolve.
 * \param[in] available_utxo    The list of available UTXOs, or NULL when there are none.
 * \param[in] reference_inputs  The resolved reference inputs of the top level transaction, or NULL when it has none.
 * \param[in] pre_selected_utxo The UTXOs that must be included in the transaction inputs, or NULL when there are none.
 *
 * \return A new reference to the resolved UTXO, or NULL if none of the lists resolves the input.
 *
 * \note The caller is responsible for releasing the returned UTXO with \ref cardano_utxo_unref.
 */
static cardano_utxo_t*
find_resolved_utxo(
  const cardano_transaction_input_t* input,
  cardano_utxo_list_t*               available_utxo,
  cardano_utxo_list_t*               reference_inputs,
  cardano_utxo_list_t*               pre_selected_utxo)
{
  cardano_utxo_t* utxo = cardano_utxo_list_find(available_utxo, find_utxo, input);

  if (utxo == NULL)
  {
    utxo = cardano_utxo_list_find(reference_inputs, find_utxo, input);
  }

  if (utxo == NULL)
  {
    utxo = cardano_utxo_list_find(pre_selected_utxo, find_utxo, input);
  }

  return utxo;
}

/**
 * \brief Adds to a list the resolved UTXOs of a set of transaction inputs whose input the list does not hold yet.
 *
 * \param[in,out] list              The list that receives the UTXOs.
 * \param[in]     inputs            The transaction inputs to resolve, or NULL when there are none.
 * \param[in]     available_utxo    The list of available UTXOs, or NULL when there are none.
 * \param[in]     reference_inputs  The resolved reference inputs of the top level transaction, or NULL when it has none.
 * \param[in]     pre_selected_utxo The UTXOs that must be included in the transaction inputs, or NULL when there are none.
 * \param[in]     skip_unresolved   Whether an input that is not resolved is skipped instead of reported as an error.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were added, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input is not
 *         resolved and \p skip_unresolved is not set, or an appropriate error code.
 */
static cardano_error_t
add_resolved_inputs(
  cardano_utxo_list_t*             list,
  cardano_transaction_input_set_t* inputs,
  cardano_utxo_list_t*             available_utxo,
  cardano_utxo_list_t*             reference_inputs,
  cardano_utxo_list_t*             pre_selected_utxo,
  const bool                       skip_unresolved)
{
  const size_t num_inputs = cardano_transaction_input_set_get_length(inputs);

  for (size_t i = 0U; i < num_inputs; ++i)
  {
    cardano_transaction_input_t* input = NULL;

    cardano_error_t result = cardano_transaction_input_set_get(inputs, i, &input);
    cardano_transaction_input_unref(&input);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_utxo_t* listed_utxo = cardano_utxo_list_find(list, find_utxo, input);
    cardano_utxo_unref(&listed_utxo);

    if (listed_utxo != NULL)
    {
      continue;
    }

    cardano_utxo_t* utxo = find_resolved_utxo(input, available_utxo, reference_inputs, pre_selected_utxo);
    cardano_utxo_unref(&utxo);

    if ((utxo == NULL) && !skip_unresolved)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    if (utxo != NULL)
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
 * \brief Creates the list of resolved UTXOs spent by the sub transactions of a set.
 *
 * The value of these UTXOs takes part in the value conservation of the whole batch, so every spend input of every sub
 * transaction must be resolved.
 *
 * \param[in]  sub_transactions  The sub transactions carried by the transaction, or NULL when it has none.
 * \param[in]  available_utxo    The list of available UTXOs, or NULL when there are none.
 * \param[in]  reference_inputs  The resolved reference inputs of the top level transaction, or NULL when it has none.
 * \param[in]  pre_selected_utxo The UTXOs that must be included in the transaction inputs, or NULL when there are none.
 * \param[out] spent_utxos       A pointer to store the list.
 *
 * \return \ref CARDANO_SUCCESS if the list was created, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if a sub transaction spends
 *         an input that is not resolved, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `spent_utxos` when it is no longer needed.
 */
static cardano_error_t
get_sub_transaction_spent_utxos(
  cardano_sub_transaction_set_t* sub_transactions,
  cardano_utxo_list_t*           available_utxo,
  cardano_utxo_list_t*           reference_inputs,
  cardano_utxo_list_t*           pre_selected_utxo,
  cardano_utxo_list_t**          spent_utxos)
{
  cardano_utxo_list_t* spent_list = NULL;

  cardano_error_t result = cardano_utxo_list_new(&spent_list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t num_sub_transactions = cardano_sub_transaction_set_get_length(sub_transactions);

  for (size_t i = 0U; i < num_sub_transactions; ++i)
  {
    cardano_sub_transaction_t* sub_tx = NULL;

    result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_tx);
    cardano_sub_transaction_unref(&sub_tx);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(&spent_list);

      return result;
    }

    cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
    cardano_sub_transaction_body_unref(&body);

    cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(body);
    cardano_transaction_input_set_unref(&inputs);

    result = add_resolved_inputs(spent_list, inputs, available_utxo, reference_inputs, pre_selected_utxo, false);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(&spent_list);

      return result;
    }
  }

  *spent_utxos = spent_list;

  return CARDANO_SUCCESS;
}

/**
 * \brief Appends the resolved UTXOs of a sub transaction whose reference scripts the fee includes to a running list.
 *
 * The reference inputs and the spend inputs of the sub transaction are taken as a set, so a UTXO that the sub
 * transaction both references and spends is appended once. Reference inputs that are not resolved are skipped, since
 * they are only needed to price their reference scripts.
 *
 * \param[in]     sub_tx            The sub transaction whose resolved UTXOs are appended.
 * \param[in]     available_utxo    The list of available UTXOs, or NULL when there are none.
 * \param[in]     reference_inputs  The resolved reference inputs of the top level transaction, or NULL when it has none.
 * \param[in]     pre_selected_utxo The UTXOs that must be included in the transaction inputs, or NULL when there are none.
 * \param[in,out] priced_inputs     The running list. On success it is replaced by a new list that also holds the UTXOs
 *                                  of the sub transaction; on failure, or when the sub transaction has no resolved
 *                                  UTXOs, it is left untouched.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were appended, or an appropriate error code.
 */
static cardano_error_t
add_sub_transaction_priced_inputs(
  cardano_sub_transaction_t* sub_tx,
  cardano_utxo_list_t*       available_utxo,
  cardano_utxo_list_t*       reference_inputs,
  cardano_utxo_list_t*       pre_selected_utxo,
  cardano_utxo_list_t**      priced_inputs)
{
  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  cardano_transaction_input_set_t* body_reference_inputs = cardano_sub_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_t* body_inputs           = cardano_sub_transaction_body_get_inputs(body);

  cardano_transaction_input_set_unref(&body_reference_inputs);
  cardano_transaction_input_set_unref(&body_inputs);

  cardano_utxo_list_t* body_utxos = NULL;

  cardano_error_t result = cardano_utxo_list_new(&body_utxos);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = add_resolved_inputs(body_utxos, body_reference_inputs, available_utxo, reference_inputs, pre_selected_utxo, true);

  if (result == CARDANO_SUCCESS)
  {
    result = add_resolved_inputs(body_utxos, body_inputs, available_utxo, reference_inputs, pre_selected_utxo, false);
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&body_utxos);

    return result;
  }

  if (cardano_utxo_list_get_length(body_utxos) == 0U)
  {
    cardano_utxo_list_unref(&body_utxos);

    return CARDANO_SUCCESS;
  }

  cardano_utxo_list_t* updated_inputs = cardano_utxo_list_concat(*priced_inputs, body_utxos);

  cardano_utxo_list_unref(&body_utxos);

  if (updated_inputs == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_utxo_list_unref(priced_inputs);
  *priced_inputs = updated_inputs;

  return CARDANO_SUCCESS;
}

/**
 * \brief Creates the list of resolved UTXOs of the sub transactions whose reference scripts the fee includes.
 *
 * The top level transaction pays the fee of the whole batch, so the reference scripts reachable from its sub
 * transactions are priced too: the ones of their resolved reference inputs and the ones that sit on the UTXOs they
 * spend. The UTXOs are taken from the body of each sub transaction, as a set, so a UTXO that a sub transaction both
 * references and spends is listed once, while a UTXO used by several sub transactions is listed, and priced, once per
 * sub transaction. Including them is deliberate and may exceed the current ledger minimum, which does not charge for the
 * reference scripts of sub transactions yet. This list must only be used to price reference scripts, the scripts of the
 * sub transactions take no part in the evaluation or in the validation mode of the top level transaction.
 *
 * \param[in]  sub_transactions  The sub transactions carried by the transaction, or NULL when it has none.
 * \param[in]  available_utxo    The list of available UTXOs, or NULL when there are none.
 * \param[in]  reference_inputs  The resolved reference inputs of the top level transaction, or NULL when it has none.
 * \param[in]  pre_selected_utxo The UTXOs that must be included in the transaction inputs, or NULL when there are none.
 * \param[out] priced_inputs     A pointer to store the list.
 *
 * \return \ref CARDANO_SUCCESS if the list was created, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `priced_inputs` when it is no longer needed.
 */
static cardano_error_t
get_sub_transaction_priced_inputs(
  cardano_sub_transaction_set_t* sub_transactions,
  cardano_utxo_list_t*           available_utxo,
  cardano_utxo_list_t*           reference_inputs,
  cardano_utxo_list_t*           pre_selected_utxo,
  cardano_utxo_list_t**          priced_inputs)
{
  cardano_utxo_list_t* priced_list = NULL;

  cardano_error_t result = cardano_utxo_list_new(&priced_list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t num_sub_transactions = cardano_sub_transaction_set_get_length(sub_transactions);

  for (size_t i = 0U; i < num_sub_transactions; ++i)
  {
    cardano_sub_transaction_t* sub_tx = NULL;

    result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_tx);
    cardano_sub_transaction_unref(&sub_tx);

    if (result == CARDANO_SUCCESS)
    {
      result = add_sub_transaction_priced_inputs(sub_tx, available_utxo, reference_inputs, pre_selected_utxo, &priced_list);
    }

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(&priced_list);

      return result;
    }
  }

  *priced_inputs = priced_list;

  return CARDANO_SUCCESS;
}

/**
 * \brief Creates the list of resolved UTXOs whose reference scripts the fee of a balancing iteration includes.
 *
 * The ledger charges for every reference script found on the outputs behind the reference inputs and the spend
 * inputs of a transaction, whatever its language and whether or not it runs. The inputs are taken as a set, so a UTXO
 * that is both referenced and spent is priced once, while the same script sitting on two different UTXOs is priced
 * twice. The spend inputs change with every coin selection, so the list is created again on every iteration. The
 * priced UTXOs of the sub transactions are appended as they are.
 *
 * \param[in]  reference_inputs              The resolved reference inputs of the top level transaction.
 * \param[in]  pre_selected_utxo             The UTXOs that must be included in the transaction inputs, or NULL when
 *                                           there are none.
 * \param[in]  selection                     The UTXOs spent by the transaction on this iteration.
 * \param[in]  sub_transaction_priced_inputs The priced UTXOs of the sub transactions.
 * \param[out] priced_inputs                 A pointer to store the list.
 *
 * \return \ref CARDANO_SUCCESS if the list was created, or an appropriate error code.
 *
 * \note The caller is responsible for freeing `priced_inputs` when it is no longer needed.
 */
static cardano_error_t
get_priced_inputs(
  cardano_utxo_list_t*  reference_inputs,
  cardano_utxo_list_t*  pre_selected_utxo,
  cardano_utxo_list_t*  selection,
  cardano_utxo_list_t*  sub_transaction_priced_inputs,
  cardano_utxo_list_t** priced_inputs)
{
  if (reference_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_utxo_list_t* top_level_inputs = NULL;

  cardano_error_t result = cardano_utxo_list_new(&top_level_inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = add_utxos_with_new_inputs(top_level_inputs, reference_inputs);

  if (result == CARDANO_SUCCESS)
  {
    result = add_utxos_with_new_inputs(top_level_inputs, pre_selected_utxo);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = add_utxos_with_new_inputs(top_level_inputs, selection);
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&top_level_inputs);

    return result;
  }

  if (cardano_utxo_list_get_length(sub_transaction_priced_inputs) == 0U)
  {
    *priced_inputs = top_level_inputs;

    return CARDANO_SUCCESS;
  }

  *priced_inputs = cardano_utxo_list_concat(top_level_inputs, sub_transaction_priced_inputs);

  cardano_utxo_list_unref(&top_level_inputs);

  if (*priced_inputs == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Sets the transaction inputs for a transaction body.
 *
 * This function takes a list of selected UTXOs and sets them as inputs in the provided transaction body.
 *
 * \param[in,out] body       The transaction body to which the inputs will be added.
 * \param[in]     selection  The list of selected UTXOs to be set as transaction inputs.
 * \param[in]     input_to_redeemer_map A map of input to redeemer values.
 *
 * \return \ref CARDANO_SUCCESS if the inputs were successfully set, or an appropriate error code indicating failure.
 */
static cardano_error_t
set_transaction_inputs(
  cardano_transaction_body_t*      body,
  cardano_utxo_list_t*             selection,
  cardano_input_to_redeemer_map_t* input_to_redeemer_map)
{
  cardano_transaction_input_set_t* inputs = NULL;
  cardano_error_t                  result = cardano_transaction_input_set_new(&inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t num_inputs = cardano_utxo_list_get_length(selection);

  for (size_t i = 0U; i < num_inputs; ++i)
  {
    cardano_utxo_t* utxo = NULL;
    result               = cardano_utxo_list_get(selection, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_input_set_unref(&inputs);

      return result;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    result = cardano_transaction_input_set_add(inputs, input);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_input_set_unref(&inputs);

      return result;
    }
  }

  if (input_to_redeemer_map != NULL)
  {
    for (size_t i = 0U; i < num_inputs; ++i)
    {
      cardano_transaction_input_t* input = NULL;

      result = cardano_transaction_input_set_get(inputs, i, &input);
      cardano_transaction_input_unref(&input);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_input_set_unref(&inputs);
        return result;
      }

      result = cardano_input_to_redeemer_map_update_redeemer_index(input_to_redeemer_map, input, i);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_input_set_unref(&inputs);

        return result;
      }
    }
  }

  result = cardano_transaction_body_set_inputs(body, inputs);

  cardano_transaction_input_set_unref(&inputs);

  return result;
}

/**
 * \brief Verifies and appends the change outputs produced by the coin selector to a transaction body.
 *
 * This function verifies that every change output returned by the coin selector meets the minimum ADA
 * requirement specified by the protocol parameters, and appends them to the transaction body's output list.
 *
 * \param[in,out] body            The transaction body to which the change outputs will be added.
 * \param[in]     protocol_params The protocol parameters for computing minimum UTXO requirements.
 * \param[in]     change_outputs  The list of change outputs produced by the coin selector.
 *
 * \return \ref CARDANO_SUCCESS if the change outputs were successfully added, \ref CARDANO_ERROR_BALANCE_INSUFFICIENT
 *         if a change output is below the min-ADA requirement, or an error code if any issue was encountered.
 */
static cardano_error_t
add_change_outputs(
  cardano_transaction_body_t*        body,
  cardano_protocol_parameters_t*     protocol_params,
  cardano_transaction_output_list_t* change_outputs)
{
  const uint64_t ada_per_utxo_byte  = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);
  const size_t   num_change_outputs = cardano_transaction_output_list_get_length(change_outputs);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  for (size_t i = 0U; i < num_change_outputs; ++i)
  {
    cardano_transaction_output_t* change_output = NULL;
    cardano_error_t               result        = cardano_transaction_output_list_get(change_outputs, i, &change_output);
    cardano_transaction_output_unref(&change_output);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    uint64_t min_utxo_value = 0U;
    result                  = cardano_compute_min_ada_required(change_output, ada_per_utxo_byte, &min_utxo_value);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_value_t* change_value = cardano_transaction_output_get_value(change_output);
    cardano_value_unref(&change_value);

    if (cardano_value_get_coin(change_value) < (int64_t)min_utxo_value)
    {
      return CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }

    result = cardano_transaction_output_list_add(outputs, change_output);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Creates a shallow copy of a transaction output list.
 *
 * This function creates a shallow clone of the given `cardano_transaction_output_list_t`.
 * Each output in the list is referenced by the new list, but no deep copy of the individual
 * outputs is made. The reference count of each output is incremented to manage shared ownership.
 *
 * \param[in] original The original transaction output list to clone.
 *
 * \return A new `cardano_transaction_output_list_t` that references the same outputs as `original`.
 *         Returns NULL if memory allocation fails or if `original` is NULL.
 */
static cardano_transaction_output_list_t*
shallow_clone_outputs(cardano_transaction_output_list_t* original)
{
  cardano_transaction_output_list_t* new_outputs = NULL;
  cardano_error_t                    result      = cardano_transaction_output_list_new(&new_outputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_output_list_unref(&new_outputs);

    return NULL;
  }

  const size_t num_outputs = cardano_transaction_output_list_get_length(original);

  for (size_t i = 0U; i < num_outputs; ++i)
  {
    cardano_transaction_output_t* output = NULL;
    result                               = cardano_transaction_output_list_get(original, i, &output);
    cardano_transaction_output_unref(&output);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&new_outputs);

      return NULL;
    }

    result = cardano_transaction_output_list_add(new_outputs, output);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&new_outputs);

      return NULL;
    }
  }

  return new_outputs;
}

/**
 * \brief Computes the CBOR array header size for a given element count.
 *
 * This function determines the number of bytes required for encoding the header of a CBOR array,
 * based on the element count, following CBOR's size encoding conventions.
 *
 * \param[in] element_count The number of elements in the array.
 *
 * \return The size, in bytes, required to encode the CBOR array header for the specified element count.
 */
static size_t
cbor_array_header_size(const size_t element_count)
{
  if (element_count <= 23U)
  {
    // 1 byte: 0x80 | element_count
    return 1U;
  }
  else if (element_count <= 255U)
  {
    // 2 bytes: 0x98 followed by 1 byte for element count
    return 2U;
  }
  else if (element_count <= 65535U)
  {
    // 3 bytes: 0x99 followed by 2 bytes for element count
    return 3U;
  }
  else
  {
    // 5 bytes: 0x9a followed by 4 bytes for element count
    return 5U;
  }
}

/**
 * \brief Computes the cost for VK (verification key) witnesses in a transaction.
 *
 * This function calculates the cost required to include a specified number of VK witnesses in a transaction, based on the
 * minimum fee coefficient and an estimated size for each witness structure.
 *
 * The VK witness set takes 3 bytes for its tag, the header of the list and 101 bytes for each witness, a structure
 * with 2 fields (signature, public key). The witness set stores it under a map key of 1 byte. That key is only part of
 * the unsigned transaction when its witness set already holds VK witnesses, so it is added to the estimate when
 * signing is what introduces the entry. Any other entry of the witness set, such as native scripts or datums, keeps
 * its own key and does not change the estimate.
 *
 * \param[in] witness_set The witness set of the transaction as it is before the VK witnesses are added.
 * \param[in] signature_count The number of VK witnesses to include.
 * \param[in] min_fee_coefficient The coefficient used to compute fees based on transaction size.
 *
 * \return The computed cost for including the VK witnesses.
 */
static int64_t
compute_vk_witnesses_cost(
  cardano_witness_set_t* witness_set,
  const size_t           signature_count,
  const uint64_t         min_fee_coefficient)
{
  cardano_vkey_witness_set_t* vkey_witnesses = cardano_witness_set_get_vkeys(witness_set);
  cardano_vkey_witness_set_unref(&vkey_witnesses);

  size_t vk_witness_set_size = 3U + cbor_array_header_size(signature_count) + (101U * signature_count);

  if ((signature_count > 0U) && (cardano_vkey_witness_set_get_length(vkey_witnesses) == 0U))
  {
    vk_witness_set_size += 1U;
  }

  return (int64_t)vk_witness_set_size * (int64_t)min_fee_coefficient;
}

/**
 * \brief Runs the balancing loop of a transaction until the fee converges and the whole batch is balanced.
 *
 * On every iteration the value that coin selection must cover is the value of the outputs minus the implicit value of
 * the batch: the withdrawals, the deposit refunds, the minted assets and the net imbalance of the sub transactions,
 * minus the deposits, the fee, the donation and the direct deposits. A surplus of the sub transactions therefore lowers
 * the target or ends up as change, while a deficit is funded by the selected inputs.
 *
 * \param[in,out] unbalanced_tx                   The transaction to balance.
 * \param[in]     foreign_signature_count         The number of expected extra signatures, not specified in the transaction.
 * \param[in]     protocol_params                 The protocol parameters.
 * \param[in]     reference_inputs                The resolved reference inputs of the transaction.
 * \param[in]     sub_transaction_priced_inputs   The resolved UTXOs of the sub transactions whose reference scripts the
 *                                                fee includes.
 * \param[in]     pre_selected_utxo               The UTXOs that must be included in the transaction inputs.
 * \param[in]     sub_transaction_spent_utxos     The resolved UTXOs spent by the sub transactions, empty when the
 *                                                transaction carries none.
 * \param[in]     sub_transactions_imbalance      The net imbalance of the sub transactions, zero when the transaction
 *                                                carries none.
 * \param[in]     input_to_redeemer_map           The map of inputs to redeemers.
 * \param[in]     available_utxo                  The UTXOs coin selection may spend at the top level.
 * \param[in]     coin_selector                   The coin selector.
 * \param[in]     change_address                  The address that receives the change.
 * \param[in]     available_collateral_utxo       The UTXOs available as collateral.
 * \param[in]     collateral_change_address       The address that receives the collateral change.
 * \param[in]     evaluator                       The transaction evaluator.
 * \param[in]     deferred_redeemers              The deferred redeemers to resolve on every iteration, or NULL.
 *
 * \return \ref CARDANO_SUCCESS if the transaction was balanced, or an appropriate error code.
 */
static cardano_error_t
balance_transaction(
  cardano_transaction_t*            unbalanced_tx,
  const size_t                      foreign_signature_count,
  cardano_protocol_parameters_t*    protocol_params,
  cardano_utxo_list_t*              reference_inputs,
  cardano_utxo_list_t*              sub_transaction_priced_inputs,
  cardano_utxo_list_t*              pre_selected_utxo,
  cardano_utxo_list_t*              sub_transaction_spent_utxos,
  cardano_value_t*                  sub_transactions_imbalance,
  cardano_input_to_redeemer_map_t*  input_to_redeemer_map,
  cardano_utxo_list_t*              available_utxo,
  cardano_coin_selector_t*          coin_selector,
  cardano_address_t*                change_address,
  cardano_utxo_list_t*              available_collateral_utxo,
  cardano_address_t*                collateral_change_address,
  cardano_tx_evaluator_t*           evaluator,
  cardano_deferred_redeemer_list_t* deferred_redeemers)
{
  cardano_error_t result      = CARDANO_SUCCESS;
  bool            is_balanced = false;

  cardano_transaction_body_t* body = cardano_transaction_get_body(unbalanced_tx);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  cardano_implicit_coin_t implicit_coin = { 0 };
  result                                = cardano_compute_implicit_coin(unbalanced_tx, protocol_params, &implicit_coin);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_set_last_error(
      unbalanced_tx,
      "Failed to compute implicit coin for transaction balancing.");

    return result;
  }

  cardano_direct_deposit_map_t* direct_deposits = cardano_transaction_body_get_direct_deposits(body);
  cardano_direct_deposit_map_unref(&direct_deposits);

  uint64_t direct_deposit_total = 0U;

  result = sum_direct_deposits(direct_deposits, &direct_deposit_total);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_set_last_error(
      unbalanced_tx,
      "Failed to compute direct deposits for transaction balancing.");

    return result;
  }

  cardano_transaction_output_list_t* original_outputs       = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_t* shallow_cloned_outputs = shallow_clone_outputs(original_outputs);
  cardano_transaction_output_list_unref(&original_outputs);

  uint64_t fee      = cardano_transaction_body_get_fee(body);
  uint64_t donation = 0;

  const uint64_t* donationPtr = cardano_transaction_body_get_donation(body);
  if (donationPtr != NULL)
  {
    donation = *donationPtr;
  }

  while (!is_balanced)
  {
    cardano_transaction_output_list_t* outputs            = cardano_transaction_body_get_outputs(body);
    cardano_value_t*                   total_output_value = NULL;
    result                                                = coalesce_all_outputs(outputs, &total_output_value);

    cardano_transaction_output_list_unref(&outputs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      return result;
    }

    result = _cardano_set_collateral_output(
      unbalanced_tx,
      protocol_params,
      available_collateral_utxo,
      collateral_change_address);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_value_unref(&total_output_value);

      return result;
    }

    cardano_value_t* top_level_implicit_value = NULL;

    result = cardano_value_new(
      ((int64_t)implicit_coin.withdrawals + (int64_t)implicit_coin.reclaim_deposits) -
        ((int64_t)implicit_coin.deposits + (int64_t)fee + (int64_t)donation + (int64_t)direct_deposit_total),
      mint,
      &top_level_implicit_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_value_unref(&total_output_value);

      return result;
    }

    cardano_value_t* implicit_value = NULL;

    result = cardano_value_add(top_level_implicit_value, sub_transactions_imbalance, &implicit_value);

    cardano_value_unref(&top_level_implicit_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_value_unref(&total_output_value);

      return result;
    }

    cardano_value_t* required_input_value = NULL;
    result                                = cardano_value_subtract(total_output_value, implicit_value, &required_input_value);

    cardano_value_unref(&total_output_value);
    cardano_value_unref(&implicit_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);

      return result;
    }

    cardano_utxo_list_t*               selection      = NULL;
    cardano_utxo_list_t*               remaining_utxo = NULL;
    cardano_transaction_output_list_t* change_outputs = NULL;

    cardano_coin_selection_request_t request = { 0 };

    request.pre_selected_utxo = pre_selected_utxo;
    request.available_utxo    = available_utxo;
    request.target            = required_input_value;
    request.outputs_to_cover  = shallow_cloned_outputs;
    request.change_address    = change_address;
    request.protocol_params   = protocol_params;

    result = cardano_coin_selector_select(
      coin_selector,
      &request,
      &selection,
      &remaining_utxo,
      &change_outputs);

    cardano_value_unref(&required_input_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);

      cardano_transaction_set_last_error(
        unbalanced_tx,
        cardano_coin_selector_get_last_error(coin_selector));

      return result;
    }

    result = set_transaction_inputs(body, selection, input_to_redeemer_map);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_transaction_output_list_unref(&change_outputs);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      return result;
    }

    result = add_change_outputs(body, protocol_params, change_outputs);

    cardano_transaction_output_list_unref(&change_outputs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      if (result == CARDANO_ERROR_BALANCE_INSUFFICIENT)
      {
        cardano_transaction_set_last_error(
          unbalanced_tx,
          "Coin selector returned a change output below the min-ADA requirement.");
      }

      return result;
    }

    // Deferred redeemers are resolved once the canonical input order and the change outputs are
    // final, and before evaluation, so that payloads are priced within the same iteration.
    result = cardano_deferred_redeemer_list_resolve(deferred_redeemers, unbalanced_tx, selection);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      return result;
    }

    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(unbalanced_tx);
    cardano_witness_set_unref(&witnesses);

    cardano_redeemer_list_t* current_redeemers = cardano_witness_set_get_redeemers(witnesses);
    cardano_redeemer_list_unref(&current_redeemers);

    const bool has_plutus_scripts = cardano_redeemer_list_get_length(current_redeemers) > 0U;

    if (has_plutus_scripts)
    {
      if (evaluator == NULL)
      {
        cardano_transaction_set_last_error(
          unbalanced_tx,
          "Transaction evaluator not set. Transaction evaluator is required to balance transactions with Plutus scripts.");

        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        cardano_utxo_list_unref(&selection);
        cardano_utxo_list_unref(&remaining_utxo);

        return CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE;
      }

      cardano_redeemer_list_t* redeemers = NULL;

      cardano_utxo_list_t* eval_utxos = selection;

      if ((reference_inputs != NULL) && (cardano_utxo_list_get_length(reference_inputs) > 0U))
      {
        eval_utxos = cardano_utxo_list_concat(selection, reference_inputs);

        if (eval_utxos == NULL)
        {
          cardano_transaction_output_list_unref(&shallow_cloned_outputs);
          cardano_utxo_list_unref(&selection);
          cardano_utxo_list_unref(&remaining_utxo);

          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
      }

      result = cardano_tx_evaluator_evaluate(evaluator, unbalanced_tx, eval_utxos, &redeemers);

      if (eval_utxos != selection)
      {
        cardano_utxo_list_unref(&eval_utxos);
      }

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        cardano_utxo_list_unref(&selection);
        cardano_utxo_list_unref(&remaining_utxo);

        cardano_transaction_set_last_error(
          unbalanced_tx,
          cardano_tx_evaluator_get_last_error(evaluator));

        return result;
      }

      cardano_redeemer_list_t* tx_redeemers = cardano_witness_set_get_redeemers(witnesses);
      cardano_redeemer_list_unref(&tx_redeemers);

      const size_t redeemers_count = cardano_redeemer_list_get_length(redeemers);

      for (size_t i = 0U; i < redeemers_count; ++i)
      {
        cardano_redeemer_t* redeemer = NULL;
        result                       = cardano_redeemer_list_get(redeemers, i, &redeemer);
        cardano_redeemer_unref(&redeemer);

        if (result != CARDANO_SUCCESS)
        {
          cardano_transaction_output_list_unref(&shallow_cloned_outputs);
          cardano_utxo_list_unref(&selection);
          cardano_utxo_list_unref(&remaining_utxo);
          cardano_redeemer_list_unref(&redeemers);

          return result;
        }

        const cardano_redeemer_tag_t tag   = cardano_redeemer_get_tag(redeemer);
        const uint64_t               index = cardano_redeemer_get_index(redeemer);

        cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(redeemer);
        cardano_ex_units_unref(&ex_units);

        const uint64_t mem   = cardano_ex_units_get_memory(ex_units);
        const uint64_t steps = cardano_ex_units_get_cpu_steps(ex_units);

        result = cardano_redeemer_list_set_ex_units(tx_redeemers, tag, index, mem, steps);

        if (result != CARDANO_SUCCESS)
        {
          cardano_transaction_output_list_unref(&shallow_cloned_outputs);
          cardano_utxo_list_unref(&selection);
          cardano_utxo_list_unref(&remaining_utxo);
          cardano_redeemer_list_unref(&redeemers);

          return result;
        }
      }

      cardano_redeemer_list_unref(&redeemers);
    }

    uint64_t             computed_fee    = 0;
    cardano_utxo_list_t* resolved_inputs = selection;
    cardano_utxo_list_ref(selection);

    if (pre_selected_utxo != NULL)
    {
      cardano_utxo_list_unref(&resolved_inputs);
      resolved_inputs = cardano_utxo_list_concat(pre_selected_utxo, selection);
    }

    if (available_collateral_utxo != NULL)
    {
      cardano_utxo_list_t* resolved_with_collateral = cardano_utxo_list_concat(available_collateral_utxo, resolved_inputs);

      cardano_utxo_list_unref(&resolved_inputs);
      resolved_inputs = resolved_with_collateral;
    }

    cardano_blake2b_hash_set_t* unique_signers = NULL;

    result = _cardano_get_unique_signers(unbalanced_tx, resolved_inputs, &unique_signers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_utxo_list_unref(&resolved_inputs);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      return result;
    }

    cardano_utxo_list_t* priced_inputs = NULL;

    result = get_priced_inputs(reference_inputs, pre_selected_utxo, selection, sub_transaction_priced_inputs, &priced_inputs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_utxo_list_unref(&resolved_inputs);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);
      cardano_blake2b_hash_set_unref(&unique_signers);

      return result;
    }

    result = cardano_compute_transaction_fee(unbalanced_tx, priced_inputs, protocol_params, &computed_fee);

    cardano_utxo_list_unref(&priced_inputs);

    const uint64_t signer_count      = foreign_signature_count + cardano_blake2b_hash_set_get_length(unique_signers);
    const int64_t  vk_witnesses_cost = compute_vk_witnesses_cost(witnesses, signer_count, cardano_protocol_parameters_get_min_fee_a(protocol_params));

    computed_fee += (uint64_t)vk_witnesses_cost;

    cardano_utxo_list_unref(&selection);
    cardano_utxo_list_unref(&remaining_utxo);
    cardano_blake2b_hash_set_unref(&unique_signers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_utxo_list_unref(&resolved_inputs);

      return result;
    }

    if (computed_fee > fee)
    {
      fee    = computed_fee;
      result = cardano_transaction_body_set_fee(body, fee);

      cardano_utxo_list_unref(&resolved_inputs);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        return result;
      }

      cardano_transaction_output_list_t* tmp = shallow_clone_outputs(shallow_cloned_outputs);
      result                                 = cardano_transaction_body_set_outputs(body, tmp);
      cardano_transaction_output_list_unref(&tmp);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);

        return result;
      }

      cardano_transaction_input_set_t* empty_inputs = NULL;
      result                                        = cardano_transaction_input_set_new(&empty_inputs);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        return result;
      }

      result = cardano_transaction_body_set_inputs(body, empty_inputs);
      cardano_transaction_input_set_unref(&empty_inputs);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        return result;
      }

      result = cardano_transaction_body_set_collateral(body, NULL);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        return result;
      }

      result = cardano_transaction_body_set_collateral_return(body, NULL);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        return result;
      }

      result = cardano_transaction_body_set_total_collateral(body, NULL);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        return result;
      }

      continue;
    }

    if (cardano_utxo_list_get_length(sub_transaction_spent_utxos) > 0U)
    {
      cardano_utxo_list_t* resolved_with_sub_transactions = cardano_utxo_list_concat(resolved_inputs, sub_transaction_spent_utxos);

      cardano_utxo_list_unref(&resolved_inputs);
      resolved_inputs = resolved_with_sub_transactions;

      if (resolved_inputs == NULL)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);

        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }
    }

    result = cardano_is_transaction_balanced(unbalanced_tx, resolved_inputs, protocol_params, &is_balanced);
    cardano_utxo_list_unref(&resolved_inputs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      return result;
    }

    if (!is_balanced)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      return CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  cardano_transaction_output_list_unref(&shallow_cloned_outputs);

  return CARDANO_SUCCESS;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_balance_transaction(
  cardano_transaction_t*            unbalanced_tx,
  const size_t                      foreign_signature_count,
  cardano_protocol_parameters_t*    protocol_params,
  cardano_utxo_list_t*              reference_inputs,
  cardano_utxo_list_t*              pre_selected_utxo,
  cardano_input_to_redeemer_map_t*  input_to_redeemer_map,
  cardano_utxo_list_t*              available_utxo,
  cardano_coin_selector_t*          coin_selector,
  cardano_address_t*                change_address,
  cardano_utxo_list_t*              available_collateral_utxo,
  cardano_address_t*                collateral_change_address,
  cardano_tx_evaluator_t*           evaluator,
  cardano_deferred_redeemer_list_t* deferred_redeemers)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(unbalanced_tx);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  bool has_spent_utxo = false;

  cardano_error_t result = has_utxo_spent_by_sub_transactions(pre_selected_utxo, sub_transactions, &has_spent_utxo);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (has_spent_utxo)
  {
    cardano_transaction_set_last_error(
      unbalanced_tx,
      "A pre selected input is already spent by a sub transaction. The inputs of the top level transaction must be disjoint from the inputs spent by its sub transactions.");

    return CARDANO_ERROR_DUPLICATED_KEY;
  }

  cardano_utxo_list_t* sub_transaction_spent_utxos = NULL;

  result = get_sub_transaction_spent_utxos(
    sub_transactions,
    available_utxo,
    reference_inputs,
    pre_selected_utxo,
    &sub_transaction_spent_utxos);

  if (result != CARDANO_SUCCESS)
  {
    if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
    {
      cardano_transaction_set_last_error(
        unbalanced_tx,
        "A sub transaction spends an input that is not among the UTXOs given to the balancer. The resolved UTXOs of the sub transactions must be included in the available UTXOs.");
    }

    return result;
  }

  cardano_value_t* sub_transactions_imbalance = cardano_value_new_zero();

  if (sub_transactions_imbalance == NULL)
  {
    cardano_utxo_list_unref(&sub_transaction_spent_utxos);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = add_sub_transactions_imbalance(
    sub_transactions,
    sub_transaction_spent_utxos,
    protocol_params,
    &sub_transactions_imbalance);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&sub_transaction_spent_utxos);
    cardano_value_unref(&sub_transactions_imbalance);

    cardano_transaction_set_last_error(
      unbalanced_tx,
      "Failed to compute the imbalance of the sub transactions for transaction balancing.");

    return result;
  }

  if (!cardano_value_is_zero(sub_transactions_imbalance))
  {
    bool is_legacy_mode = false;

    result = is_legacy_mode_transaction(unbalanced_tx, reference_inputs, pre_selected_utxo, &is_legacy_mode);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(&sub_transaction_spent_utxos);
      cardano_value_unref(&sub_transactions_imbalance);

      return result;
    }

    if (is_legacy_mode)
    {
      cardano_utxo_list_unref(&sub_transaction_spent_utxos);
      cardano_value_unref(&sub_transactions_imbalance);

      cardano_transaction_set_last_error(
        unbalanced_tx,
        "The top level transaction uses PlutusV1, PlutusV2 or PlutusV3 scripts, so the sub transactions must balance between themselves. Add a balancing sub transaction, top level change can not absorb their imbalance.");

      return CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS;
    }
  }

  cardano_utxo_list_t* selectable_utxo = NULL;

  result = exclude_sub_transaction_inputs(available_utxo, sub_transactions, &selectable_utxo);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&sub_transaction_spent_utxos);
    cardano_value_unref(&sub_transactions_imbalance);

    return result;
  }

  cardano_utxo_list_t* sub_transaction_priced_inputs = NULL;

  result = get_sub_transaction_priced_inputs(
    sub_transactions,
    available_utxo,
    reference_inputs,
    pre_selected_utxo,
    &sub_transaction_priced_inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&sub_transaction_spent_utxos);
    cardano_value_unref(&sub_transactions_imbalance);
    cardano_utxo_list_unref(&selectable_utxo);

    return result;
  }

  result = balance_transaction(
    unbalanced_tx,
    foreign_signature_count,
    protocol_params,
    reference_inputs,
    sub_transaction_priced_inputs,
    pre_selected_utxo,
    sub_transaction_spent_utxos,
    sub_transactions_imbalance,
    input_to_redeemer_map,
    selectable_utxo,
    coin_selector,
    change_address,
    available_collateral_utxo,
    collateral_change_address,
    evaluator,
    deferred_redeemers);

  cardano_utxo_list_unref(&sub_transaction_spent_utxos);
  cardano_value_unref(&sub_transactions_imbalance);
  cardano_utxo_list_unref(&selectable_utxo);
  cardano_utxo_list_unref(&sub_transaction_priced_inputs);

  return result;
}

cardano_error_t
cardano_is_transaction_balanced(
  cardano_transaction_t*         tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  bool*                          is_balanced)
{
  if (tx == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (is_balanced == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resolved_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *is_balanced = false;

  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol_params, &imbalance);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *is_balanced = cardano_value_is_zero(imbalance);

  cardano_value_unref(&imbalance);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_compute_transaction_imbalance(
  cardano_transaction_t*         tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              imbalance)
{
  if (imbalance == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *imbalance = NULL;

  if (tx == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resolved_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_implicit_coin_t implicit_coin = { 0 };

  cardano_error_t result = cardano_compute_implicit_coin(tx, protocol_params, &implicit_coin);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_direct_deposit_map_t* direct_deposits = cardano_transaction_body_get_direct_deposits(body);
  cardano_direct_deposit_map_unref(&direct_deposits);

  uint64_t direct_deposit_total = 0U;

  result = sum_direct_deposits(direct_deposits, &direct_deposit_total);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_input_set_t*   inputs  = cardano_transaction_body_get_inputs(body);

  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_input_set_unref(&inputs);

  const uint64_t* donation_ptr  = cardano_transaction_body_get_donation(body);
  const uint64_t  donation      = (donation_ptr != NULL) ? *donation_ptr : 0U;
  const uint64_t  fee           = cardano_transaction_body_get_fee(body);
  const int64_t   consumed_coin = (int64_t)implicit_coin.withdrawals + (int64_t)implicit_coin.reclaim_deposits;
  const int64_t   produced_coin = (int64_t)fee + (int64_t)implicit_coin.deposits + (int64_t)donation + (int64_t)direct_deposit_total;

  return compute_imbalance(inputs, resolved_inputs, outputs, mint, consumed_coin, produced_coin, imbalance);
}

cardano_error_t
cardano_compute_sub_transaction_imbalance(
  cardano_sub_transaction_t*     sub_tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              imbalance)
{
  if (imbalance == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *imbalance = NULL;

  if (sub_tx == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resolved_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  cardano_implicit_coin_t implicit_coin = { 0 };

  cardano_error_t result = cardano_compute_sub_transaction_implicit_coin(sub_tx, protocol_params, &implicit_coin);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_direct_deposit_map_t* direct_deposits = cardano_sub_transaction_body_get_direct_deposits(body);
  cardano_direct_deposit_map_unref(&direct_deposits);

  uint64_t direct_deposit_total = 0U;

  result = sum_direct_deposits(direct_deposits, &direct_deposit_total);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_multi_asset_t* mint = cardano_sub_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(body);
  cardano_transaction_input_set_t*   inputs  = cardano_sub_transaction_body_get_inputs(body);

  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_input_set_unref(&inputs);

  const uint64_t* donation_ptr  = cardano_sub_transaction_body_get_donation(body);
  const uint64_t  donation      = (donation_ptr != NULL) ? *donation_ptr : 0U;
  const int64_t   consumed_coin = (int64_t)implicit_coin.withdrawals + (int64_t)implicit_coin.reclaim_deposits;
  const int64_t   produced_coin = (int64_t)implicit_coin.deposits + (int64_t)donation + (int64_t)direct_deposit_total;

  return compute_imbalance(inputs, resolved_inputs, outputs, mint, consumed_coin, produced_coin, imbalance);
}

cardano_error_t
cardano_compute_transaction_batch_imbalance(
  cardano_transaction_t*         tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              imbalance)
{
  if (imbalance == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *imbalance = NULL;

  if (tx == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resolved_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_value_t* total = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol_params, &total);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  result = add_sub_transactions_imbalance(sub_transactions, resolved_inputs, protocol_params, &total);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&total);

    return result;
  }

  *imbalance = total;

  return CARDANO_SUCCESS;
}
