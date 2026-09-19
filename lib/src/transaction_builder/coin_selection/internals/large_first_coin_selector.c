/**
 * \file large_first_coin_selector.c
 *
 * \author angel.castillo
 * \date   Oct 16, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_id_map.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>

#include "../../../string_safe.h"
#include "./change_builder.h"
#include "./large_first_helpers.h"

#include <assert.h>
#include <string.h>

/* STATIC FUNCTIONS ************************************************************/

/**
 * \brief Moves the UTXO with the largest lovelace amount from the available UTXOs into the selection.
 *
 * A transaction must spend at least one input, so a selection must never be empty. This function is the fall back
 * for a target that requires nothing from the available UTXOs, which happens when the target is already met or
 * exceeded by implicit value (its lovelace and every one of its assets are zero or negative).
 *
 * \param[in,out] remaining_utxo The available UTXOs from which the UTXO is removed.
 * \param[in,out] selection The selection to which the UTXO is added.
 *
 * \return \ref CARDANO_SUCCESS if a UTXO was selected, \ref CARDANO_ERROR_BALANCE_INSUFFICIENT if no UTXO holding
 *         lovelace is available, or an appropriate error code indicating failure.
 */
static cardano_error_t
select_largest_lovelace_utxo(cardano_utxo_list_t* remaining_utxo, cardano_utxo_list_t* selection)
{
  cardano_asset_id_t* lovelace = NULL;
  cardano_error_t     result   = cardano_asset_id_new_lovelace(&lovelace);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_value_t* accumulated_value = cardano_value_new_zero();

  result = _cardano_large_fist_select_utxos(lovelace, 1, remaining_utxo, selection, &accumulated_value);

  cardano_value_unref(&accumulated_value);
  cardano_asset_id_unref(&lovelace);

  return result;
}

/**
 * \brief Selects UTXOs from the available list and pre-selected UTXOs to meet the target value.
 *
 * This function selects UTXOs from both the pre-selected UTXO list and available UTXOs to meet a specified target value.
 * The selected UTXOs are stored in the `selection` list, and any remaining UTXOs are stored in the `remaining_utxo` list.
 * Any excess value is returned as min-ADA compliant change outputs in the `change_outputs` list, upholding the local
 * balance invariant: sum(selection) = target + sum(change_outputs).
 *
 * The selection is never empty: when neither the pre-selected UTXOs nor the target require an input, the available UTXO
 * with the largest lovelace amount is selected and its value is returned in the change outputs.
 *
 * \param[in] coin_selector A pointer to the coin selector implementation object.
 * \param[in] request The selection request. The request's outputs_to_cover hint is unused by this selector.
 * \param[out] selection A pointer to the list of selected UTXOs that meet the target value.
 * \param[out] remaining_utxo A pointer to the list of UTXOs that were not selected and remain available for future transactions.
 * \param[out] change_outputs A pointer to the list of change outputs produced by the selection.
 *
 * \return \ref CARDANO_SUCCESS if the target is covered and the selection is not empty,
 *         \ref CARDANO_ERROR_BALANCE_INSUFFICIENT if the available UTXOs can not cover the target or there is no UTXO
 *         to select, or an appropriate error code indicating failure.
 */
static cardano_error_t
select(
  cardano_coin_selector_impl_t*           coin_selector,
  const cardano_coin_selection_request_t* request,
  cardano_utxo_list_t**                   selection,
  cardano_utxo_list_t**                   remaining_utxo,
  cardano_transaction_output_list_t**     change_outputs)
{
  assert(coin_selector != NULL);
  assert(request != NULL);

  cardano_utxo_list_t*           pre_selected_utxo = request->pre_selected_utxo;
  cardano_utxo_list_t*           available_utxo    = request->available_utxo;
  cardano_value_t*               target            = request->target;
  cardano_address_t*             change_address    = request->change_address;
  cardano_protocol_parameters_t* protocol_params   = request->protocol_params;

  assert(available_utxo != NULL);
  assert(target != NULL);
  assert(change_address != NULL);
  assert(protocol_params != NULL);

  CARDANO_UNUSED(coin_selector);

  cardano_error_t result = cardano_utxo_list_new(selection);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *remaining_utxo = cardano_utxo_list_clone(available_utxo);

  if (*remaining_utxo == NULL)
  {
    cardano_utxo_list_unref(selection);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_value_t* accumulated_value = NULL;

  bool preselected_utxo_satisfies_target = false;

  if (pre_selected_utxo != NULL)
  {
    result = _cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &preselected_utxo_satisfies_target);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(remaining_utxo);
      cardano_utxo_list_unref(selection);
      cardano_value_unref(&accumulated_value);

      return result;
    }

    const size_t pre_selected_count = cardano_utxo_list_get_length(pre_selected_utxo);

    for (size_t i = 0U; i < pre_selected_count; ++i)
    {
      cardano_utxo_t* utxo = NULL;

      result = cardano_utxo_list_get(pre_selected_utxo, i, &utxo);
      cardano_utxo_unref(&utxo);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utxo_list_unref(remaining_utxo);
        cardano_utxo_list_unref(selection);
        cardano_value_unref(&accumulated_value);

        return result;
      }

      result = cardano_utxo_list_add(*selection, utxo);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utxo_list_unref(remaining_utxo);
        cardano_utxo_list_unref(selection);
        cardano_value_unref(&accumulated_value);

        return result;
      }

      result = cardano_utxo_list_remove(*remaining_utxo, utxo);
    }

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(remaining_utxo);
      cardano_utxo_list_unref(selection);
      cardano_value_unref(&accumulated_value);

      return result;
    }
  }

  cardano_asset_id_map_t* assets      = cardano_value_as_assets_map(target);
  size_t                  asset_count = cardano_asset_id_map_get_length(assets);

  for (size_t i = 0U; i < asset_count; ++i)
  {
    cardano_asset_id_t* asset_id     = NULL;
    int64_t             asset_amount = 0;

    result = cardano_asset_id_map_get_key_at(assets, i, &asset_id);
    cardano_asset_id_unref(&asset_id);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&assets);
      cardano_utxo_list_unref(selection);
      cardano_utxo_list_unref(remaining_utxo);
      cardano_value_unref(&accumulated_value);

      return result;
    }

    result = cardano_asset_id_map_get(assets, asset_id, &asset_amount);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&assets);
      cardano_utxo_list_unref(selection);
      cardano_utxo_list_unref(remaining_utxo);
      cardano_value_unref(&accumulated_value);

      return result;
    }

    result = _cardano_large_fist_select_utxos(asset_id, asset_amount, *remaining_utxo, *selection, &accumulated_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&assets);
      cardano_utxo_list_unref(selection);
      cardano_utxo_list_unref(remaining_utxo);
      cardano_value_unref(&accumulated_value);

      return result;
    }
  }

  cardano_asset_id_map_unref(&assets);
  cardano_value_unref(&accumulated_value);

  if (cardano_utxo_list_get_length(*selection) == 0U)
  {
    result = select_largest_lovelace_utxo(*remaining_utxo, *selection);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(selection);
      cardano_utxo_list_unref(remaining_utxo);

      return result;
    }
  }

  result = _cardano_coin_selector_build_change(target, change_address, protocol_params, *selection, *remaining_utxo, change_outputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(selection);
    cardano_utxo_list_unref(remaining_utxo);

    return result;
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_large_first_coin_selector_new(cardano_coin_selector_t** cardano_coin_selector)
{
  static const char*           handler_name = "Large first coin selector";
  cardano_coin_selector_impl_t impl         = { 0 };

  cardano_safe_memcpy(impl.name, 256U, handler_name, cardano_safe_strlen(handler_name, 256U));

  if (cardano_coin_selector == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  impl.select  = select;
  impl.context = NULL;

  return cardano_coin_selector_new(impl, cardano_coin_selector);
}