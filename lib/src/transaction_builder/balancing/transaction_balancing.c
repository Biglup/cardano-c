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

    if (input_to_redeemer_map != NULL)
    {
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
 * \brief Adds a change output to a transaction body.
 *
 * This function calculates the necessary padding and adds a change output to the provided transaction body.
 * It ensures that the change value meets the minimum ADA requirement for a UTXO, as specified by the protocol parameters.
 *
 * \param[in,out] body           The transaction body to which the change output will be added.
 * \param[in]     protocol_params The protocol parameters for computing minimum UTXO requirements.
 * \param[in]     change_value   The value of the change to be added.
 * \param[in]     change_address The address to which the change will be sent.
 * \param[out]    change_padding A pointer to a uint64_t where the computed padding value will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the change output was successfully added, or an error code if any issue was encountered.
 */
static cardano_error_t
add_change_output(
  cardano_transaction_body_t*    body,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t*               change_value,
  cardano_address_t*             change_address,
  uint64_t*                      change_padding)
{
  cardano_transaction_output_t* change_output = NULL;
  cardano_error_t               result        = cardano_transaction_output_new(change_address, 0, &change_output);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_transaction_output_set_value(change_output, change_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_output_unref(&change_output);

    return result;
  }

  const uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);
  uint64_t       min_utxo_value    = 0U;
  result                           = cardano_compute_min_ada_required(change_output, ada_per_utxo_byte, &min_utxo_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_output_unref(&change_output);

    return result;
  }

  const uint64_t change_coin = cardano_value_get_coin(change_value);

  if (change_coin < min_utxo_value)
  {
    *change_padding += min_utxo_value - change_coin;

    cardano_transaction_output_unref(&change_output);

    return CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  result                                     = cardano_transaction_output_list_add(outputs, change_output);

  cardano_transaction_output_unref(&change_output);
  cardano_transaction_output_list_unref(&outputs);

  return result;
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
 * \param[in] signature_count The number of VK witnesses to include.
 * \param[in] min_fee_coefficient The coefficient used to compute fees based on transaction size.
 *
 * \return The computed cost for including the VK witnesses.
 */
static int64_t
compute_vk_witnesses_cost(const size_t signature_count, const uint64_t min_fee_coefficient)
{
  // Tag 3 bytes + length of the list + 101 bytes for each structure with 2 fields (signature, public key)
  const size_t vk_witness_set_size = 3U + cbor_array_header_size(signature_count) + (101U * signature_count);
  return (int64_t)vk_witness_set_size * (int64_t)min_fee_coefficient;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_balance_transaction(
  cardano_transaction_t*           unbalanced_tx,
  const size_t                     foreign_signature_count,
  cardano_protocol_parameters_t*   protocol_params,
  cardano_utxo_list_t*             reference_inputs,
  cardano_utxo_list_t*             pre_selected_utxo,
  cardano_input_to_redeemer_map_t* input_to_redeemer_map,
  cardano_utxo_list_t*             available_utxo,
  cardano_coin_selector_t*         coin_selector,
  cardano_address_t*               change_address,
  cardano_utxo_list_t*             available_collateral_utxo,
  cardano_address_t*               collateral_change_address,
  cardano_tx_evaluator_t*          evaluator)
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
    return result;
  }

  cardano_transaction_output_list_t* original_outputs       = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_t* shallow_cloned_outputs = shallow_clone_outputs(original_outputs);
  cardano_transaction_output_list_unref(&original_outputs);

  uint64_t fee            = cardano_transaction_body_get_fee(body);
  uint64_t change_padding = 0U;

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

    cardano_value_t* implicit_value = NULL;

    result = cardano_value_new(
      ((int64_t)implicit_coin.withdrawals + (int64_t)implicit_coin.reclaim_deposits) -
        ((int64_t)implicit_coin.deposits + (int64_t)fee + (int64_t)change_padding),
      mint,
      &implicit_value);

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

    cardano_utxo_list_t* selection      = NULL;
    cardano_utxo_list_t* remaining_utxo = NULL;

    result = cardano_coin_selector_select(
      coin_selector,
      pre_selected_utxo,
      available_utxo,
      required_input_value,
      &selection,
      &remaining_utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_value_unref(&required_input_value);

      return result;
    }

    result = set_transaction_inputs(body, selection, input_to_redeemer_map);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_value_unref(&required_input_value);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      return result;
    }

    cardano_value_t*                 selected_input_value = NULL;
    cardano_transaction_input_set_t* inputs               = cardano_transaction_body_get_inputs(body);

    cardano_transaction_input_set_unref(&inputs);

    result = coalesce_all_inputs(inputs, selection, &selected_input_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_value_unref(&required_input_value);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      return result;
    }

    cardano_value_t* change_value = NULL;
    result                        = cardano_value_subtract(selected_input_value, required_input_value, &change_value);

    cardano_value_unref(&selected_input_value);
    cardano_value_unref(&required_input_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      return CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }

    result = cardano_value_add_coin(change_value, (int64_t)change_padding);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_list_unref(&shallow_cloned_outputs);
      cardano_value_unref(&change_value);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      return result;
    }

    if (!cardano_value_is_zero(change_value))
    {
      result = add_change_output(body, protocol_params, change_value, change_address, &change_padding);

      if (result == CARDANO_ERROR_BALANCE_INSUFFICIENT)
      {
        cardano_value_unref(&change_value);
        cardano_utxo_list_unref(&selection);
        cardano_utxo_list_unref(&remaining_utxo);

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

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        cardano_value_unref(&change_value);
        cardano_utxo_list_unref(&selection);
        cardano_utxo_list_unref(&remaining_utxo);
        return result;
      }
    }

    cardano_value_unref(&change_value);

    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(unbalanced_tx);
    cardano_witness_set_unref(&witnesses);

    cardano_redeemer_list_t* current_redeemers = cardano_witness_set_get_redeemers(witnesses);
    cardano_redeemer_list_unref(&current_redeemers);

    const bool has_plutus_scripts = cardano_redeemer_list_get_length(current_redeemers) > 0U;

    if (has_plutus_scripts)
    {
      cardano_redeemer_list_t* redeemers = NULL;
      result                             = cardano_tx_evaluator_evaluate(evaluator, unbalanced_tx, selection, &redeemers);

      if (result != CARDANO_SUCCESS)
      {
        cardano_transaction_output_list_unref(&shallow_cloned_outputs);
        cardano_utxo_list_unref(&selection);
        cardano_utxo_list_unref(&remaining_utxo);
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

    result = cardano_compute_transaction_fee(unbalanced_tx, reference_inputs, protocol_params, &computed_fee);

    const uint64_t signer_count      = foreign_signature_count + cardano_blake2b_hash_set_get_length(unique_signers);
    const int64_t  vk_witnesses_cost = compute_vk_witnesses_cost(signer_count, cardano_protocol_parameters_get_min_fee_a(protocol_params));

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

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  cardano_implicit_coin_t implicit_coin = { 0 };

  cardano_error_t result = cardano_compute_implicit_coin(tx, protocol_params, &implicit_coin);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const uint64_t fee                 = cardano_transaction_body_get_fee(body);
  const int64_t  implicit_coin_value = ((int64_t)implicit_coin.withdrawals + (int64_t)implicit_coin.reclaim_deposits) - (int64_t)implicit_coin.deposits;

  cardano_value_t* implicit_value = NULL;

  result = cardano_value_new(implicit_coin_value - (int64_t)fee, mint, &implicit_value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_value_t*                   total_output_value = NULL;
  cardano_value_t*                   total_input_value  = NULL;
  cardano_value_t*                   diff_value         = NULL;
  cardano_value_t*                   net_value          = NULL;
  cardano_transaction_output_list_t* outputs            = cardano_transaction_body_get_outputs(body);
  cardano_transaction_input_set_t*   inputs             = cardano_transaction_body_get_inputs(body);

  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_input_set_unref(&inputs);

  result = coalesce_all_outputs(outputs, &total_output_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_value);

    return result;
  }

  result = coalesce_all_inputs(inputs, resolved_inputs, &total_input_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_value);
    cardano_value_unref(&total_output_value);

    return result;
  }

  result = cardano_value_subtract(total_output_value, total_input_value, &diff_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_value);
    cardano_value_unref(&total_output_value);
    cardano_value_unref(&total_input_value);

    return result;
  }

  result = cardano_value_subtract(diff_value, implicit_value, &net_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&implicit_value);
    cardano_value_unref(&total_output_value);
    cardano_value_unref(&total_input_value);
    cardano_value_unref(&diff_value);

    return result;
  }

  *is_balanced = cardano_value_is_zero(net_value);

  cardano_value_unref(&implicit_value);
  cardano_value_unref(&total_output_value);
  cardano_value_unref(&total_input_value);
  cardano_value_unref(&diff_value);
  cardano_value_unref(&net_value);

  return CARDANO_SUCCESS;
}