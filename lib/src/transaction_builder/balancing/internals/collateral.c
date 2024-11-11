/**
 * \file collateral.c
 *
 * \author angel.castillo
 * \date   Nov 14, 2024
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

#include "collateral.h"

#include <cardano/common/utxo.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>

#include <cardano/transaction_builder/fee.h>
#include <math.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
_cardano_coalesce_all_utxos(cardano_utxo_list_t* utxos, cardano_value_t** total_output_value)
{
  const size_t num_utxos = cardano_utxo_list_get_length(utxos);

  cardano_value_t* total_value = cardano_value_new_zero();

  if (total_value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0U; i < num_utxos; ++i)
  {
    cardano_utxo_t* utxo   = NULL;
    cardano_error_t result = cardano_utxo_list_get(utxos, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&total_value);
      return result;
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

  *total_output_value = total_value;

  return CARDANO_SUCCESS;
}

cardano_transaction_input_set_t*
_cardano_utxo_list_to_input_set(cardano_utxo_list_t* utxos)
{
  cardano_transaction_input_set_t* inputs = NULL;
  cardano_error_t                  result = cardano_transaction_input_set_new(&inputs);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  const size_t num_utxos = cardano_utxo_list_get_length(utxos);

  for (size_t i = 0U; i < num_utxos; ++i)
  {
    cardano_utxo_t* utxo = NULL;
    result               = cardano_utxo_list_get(utxos, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_input_set_unref(&inputs);

      return NULL;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    result = cardano_transaction_input_set_add(inputs, input);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_input_set_unref(&inputs);

      return NULL;
    }
  }

  return inputs;
}

uint64_t
_cardano_calculate_collateral_amount(uint64_t fee, uint64_t collateral_percentage)
{
  if ((fee == 0U) || (collateral_percentage == 0U))
  {
    return 5000000U;
  }

  return (uint64_t)ceil(((double)fee * (double)collateral_percentage) / 100.0);
}

cardano_error_t
_cardano_create_collateral_change_output(
  cardano_value_t*               change_value,
  cardano_address_t*             change_address,
  cardano_protocol_parameters_t* protocol_params,
  uint64_t*                      change_padding,
  cardano_transaction_output_t** change_output)
{
  cardano_error_t result;
  *change_output = NULL;

  if (!cardano_value_is_zero(change_value))
  {
    result = cardano_transaction_output_new(change_address, 0, change_output);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_output_set_value(*change_output, change_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_unref(change_output);

      return result;
    }

    const uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);
    uint64_t       min_utxo_value    = 0U;
    result                           = cardano_compute_min_ada_required(*change_output, ada_per_utxo_byte, &min_utxo_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_output_unref(change_output);

      return result;
    }

    const uint64_t change_coin = cardano_value_get_coin(change_value);

    if (change_coin < min_utxo_value)
    {
      *change_padding += min_utxo_value - change_coin;
      cardano_transaction_output_unref(change_output);
      *change_output = NULL;

      return CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_update_transaction_body_collateral(
  cardano_transaction_body_t*   body,
  const uint64_t                collateral_amount,
  cardano_transaction_output_t* change_output,
  cardano_utxo_list_t*          selection)
{
  cardano_error_t result;

  result = cardano_transaction_body_set_total_collateral(body, &collateral_amount);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (change_output != NULL)
  {
    result = cardano_transaction_body_set_collateral_return(body, change_output);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  cardano_transaction_input_set_t* collateral_inputs = _cardano_utxo_list_to_input_set(selection);

  result = cardano_transaction_body_set_collateral(body, collateral_inputs);
  cardano_transaction_input_set_unref(&collateral_inputs);

  return result;
}

cardano_error_t
_cardano_set_collateral_output(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol_params,
  cardano_utxo_list_t*           available_collateral_outputs,
  cardano_address_t*             change_address)
{
  if ((tx == NULL) || (protocol_params == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t collateral_count = cardano_utxo_list_get_length(available_collateral_outputs);

  if (collateral_count == 0U)
  {
    return CARDANO_SUCCESS;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  const uint64_t fee                   = cardano_transaction_body_get_fee(body);
  const uint64_t collateral_percentage = cardano_protocol_parameters_get_collateral_percentage(protocol_params);
  uint64_t       collateral_amount     = _cardano_calculate_collateral_amount(fee, collateral_percentage);
  uint64_t       change_padding        = 0U;
  bool           is_balanced           = false;

  cardano_coin_selector_t* coin_selector = NULL;
  cardano_error_t          result        = cardano_large_first_coin_selector_new(&coin_selector);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  while (!is_balanced)
  {
    cardano_value_t*     collateral_value = cardano_value_new_from_coin((int64_t)collateral_amount + (int64_t)change_padding);
    cardano_utxo_list_t* selection        = NULL;
    cardano_utxo_list_t* remaining_utxo   = NULL;

    result = cardano_coin_selector_select(
      coin_selector,
      NULL,
      available_collateral_outputs,
      collateral_value,
      &selection,
      &remaining_utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&collateral_value);
      cardano_coin_selector_unref(&coin_selector);
      return result;
    }

    cardano_value_t* selected_input_value = NULL;
    result                                = _cardano_coalesce_all_utxos(selection, &selected_input_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&collateral_value);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);
      cardano_coin_selector_unref(&coin_selector);
      return result;
    }

    cardano_value_t* change_value = NULL;
    result                        = cardano_value_subtract(selected_input_value, collateral_value, &change_value);
    cardano_value_unref(&selected_input_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&collateral_value);
      cardano_value_unref(&change_value);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);
      cardano_coin_selector_unref(&coin_selector);

      return result;
    }

    cardano_transaction_output_t* change_output = NULL;
    result                                      = _cardano_create_collateral_change_output(change_value, change_address, protocol_params, &change_padding, &change_output);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&collateral_value);
      cardano_value_unref(&change_value);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);

      if (result == CARDANO_ERROR_BALANCE_INSUFFICIENT)
      {
        continue;
      }

      cardano_coin_selector_unref(&coin_selector);
      return result;
    }

    result = _cardano_update_transaction_body_collateral(body, collateral_amount, change_output, selection);

    if (result != CARDANO_SUCCESS)
    {
      if (change_output != NULL)
      {
        cardano_transaction_output_unref(&change_output);
      }

      cardano_value_unref(&collateral_value);
      cardano_value_unref(&change_value);
      cardano_utxo_list_unref(&selection);
      cardano_utxo_list_unref(&remaining_utxo);
      cardano_coin_selector_unref(&coin_selector);

      return result;
    }

    if (change_output != NULL)
    {
      cardano_transaction_output_unref(&change_output);
    }

    cardano_value_unref(&collateral_value);
    cardano_value_unref(&change_value);
    cardano_utxo_list_unref(&selection);
    cardano_utxo_list_unref(&remaining_utxo);

    is_balanced = true;
  }

  cardano_coin_selector_unref(&coin_selector);

  return CARDANO_SUCCESS;
}