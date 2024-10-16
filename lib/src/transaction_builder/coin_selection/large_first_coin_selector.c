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
#include <cardano/object.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>

#include "../../string_safe.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Retrieves the amount of a specific asset from a Cardano value.
 *
 * This function retrieves the amount of a specific asset identified by `asset_id` from a `cardano_value_t` object.
 *
 * \param[in] value A pointer to the \ref cardano_value_t object representing the value containing multiple assets.
 * \param[in] asset_id A pointer to the \ref cardano_asset_id_t object identifying the specific asset to look up.
 * \param[out] amount A pointer to an int64_t where the asset amount will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the asset amount was successfully retrieved, or an appropriate error code indicating failure.
 */

static cardano_error_t
get_asset_amount(
  cardano_value_t*    value,
  cardano_asset_id_t* asset_id)
{
  if (!value || !asset_id)
  {
    return 0;
  }

  int64_t                amount      = 0;
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  if (multi_asset != NULL)
  {
    cardano_error_t result = cardano_multi_asset_get_with_id(multi_asset, asset_id, &amount);

    if (result != CARDANO_SUCCESS)
    {
      return 0;
    }
  }

  return amount;
}

/**
 * \brief Compares two UTXOs based on ADA amounts, with a secondary comparison based on the number of additional assets.
 *
 * This function compares two UTXOs first by the number of assets (other than ADA) they contain, and if equal, by their ADA value.
 * It ensures proper cleanup of referenced objects after comparison.
 *
 * \param[in] lhs A pointer to the first \ref cardano_utxo_t to be compared.
 * \param[in] rhs A pointer to the second \ref cardano_utxo_t to be compared.
 * \param[in] context Unused parameter, passed for compatibility with callback signature.
 *
 * \return A negative value if `lhs` is considered less than `rhs`, zero if they are equal, or a positive value if `lhs` is greater than `rhs`.
 */
static int32_t
compare_utxos_for_ada(cardano_utxo_t* lhs, cardano_utxo_t* rhs, void* context)
{
  CARDANO_UNUSED(context);

  cardano_transaction_output_t* lhs_output = cardano_utxo_get_output(lhs);
  cardano_transaction_output_t* rhs_output = cardano_utxo_get_output(rhs);

  cardano_value_t* lhs_value = cardano_transaction_output_get_value(lhs_output);
  cardano_value_t* rhs_value = cardano_transaction_output_get_value(rhs_output);

  uint64_t lhs_asset_count = cardano_value_get_asset_count(lhs_value);
  uint64_t rhs_asset_count = cardano_value_get_asset_count(rhs_value);

  if (lhs_asset_count != rhs_asset_count)
  {
    int32_t result = (lhs_asset_count < rhs_asset_count) ? (int32_t)-1 : (int32_t)1;

    cardano_value_unref(&lhs_value);
    cardano_value_unref(&rhs_value);
    cardano_transaction_output_unref(&lhs_output);
    cardano_transaction_output_unref(&rhs_output);

    return result;
  }

  uint64_t lhs_ada = cardano_value_get_coin(lhs_value);
  uint64_t rhs_ada = cardano_value_get_coin(rhs_value);

  int32_t result = (lhs_ada < rhs_ada) ? (int32_t)1 : ((lhs_ada > rhs_ada) ? (int32_t)-1 : (int32_t)0);

  cardano_value_unref(&lhs_value);
  cardano_value_unref(&rhs_value);
  cardano_transaction_output_unref(&lhs_output);
  cardano_transaction_output_unref(&rhs_output);

  return result;
}

/**
 * \brief Selects UTXOs to meet a required ADA amount.
 *
 * This function sorts the available UTXOs by their ADA amounts and selects UTXOs from the list until the required ADA
 * amount is accumulated. Selected UTXOs are added to the `selected_utxos` list and removed from the `available_utxos` list.
 *
 * \param[in] required_ada The minimum ADA amount required.
 * \param[in] available_utxos A list of available UTXOs to select from.
 * \param[out] selected_utxos A list to which selected UTXOs will be added.
 *
 * \return \ref CARDANO_SUCCESS if the required ADA amount was met, or \ref CARDANO_ERROR_BALANCE_INSUFFICIENT if
 *         there were not enough UTXOs to meet the required amount. Other errors may be returned for internal failures.
 */
static cardano_error_t
select_utxos_for_ada(
  const uint64_t       required_ada,
  cardano_utxo_list_t* available_utxos,
  cardano_utxo_list_t* selected_utxos)
{
  uint64_t accumulated_ada = 0;

  cardano_utxo_list_sort(available_utxos, compare_utxos_for_ada, NULL);

  size_t utxo_count = cardano_utxo_list_get_length(available_utxos);

  for (size_t i = 0U; (i < utxo_count) && (accumulated_ada < required_ada); ++i)
  {
    cardano_utxo_t* utxo   = NULL;
    cardano_error_t result = cardano_utxo_list_get(available_utxos, i, &utxo);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_transaction_output_t* output     = cardano_utxo_get_output(utxo);
    cardano_value_t*              utxo_value = cardano_transaction_output_get_value(output);

    uint64_t utxo_ada = cardano_value_get_coin(utxo_value);

    if (utxo_ada > 0U)
    {
      result = cardano_utxo_list_add(selected_utxos, utxo);

      if (result != CARDANO_SUCCESS)
      {
        cardano_value_unref(&utxo_value);
        cardano_transaction_output_unref(&output);
        cardano_utxo_unref(&utxo);

        return result;
      }

      accumulated_ada += utxo_ada;

      cardano_utxo_list_t* removed = cardano_utxo_list_erase(available_utxos, (int64_t)i, 1);
      cardano_utxo_list_unref(&removed);

      --i;
      --utxo_count;
    }

    cardano_value_unref(&utxo_value);
    cardano_transaction_output_unref(&output);
    cardano_utxo_unref(&utxo);
  }

  if (accumulated_ada < required_ada)
  {
    return CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Compares two cardano_value_t objects to check if lhs is greater than or equal to rhs.
 *
 * This function compares the ADA and multi-asset values in the lhs and rhs `cardano_value_t` objects.
 * It sets the result to `true` if lhs >= rhs for all assets (including ADA), and `false` otherwise.
 *
 * \param[in] lhs The left-hand side value to compare.
 * \param[in] rhs The right-hand side value to compare.
 * \param[out] result Pointer to a boolean that will be set to true if lhs >= rhs, otherwise false.
 *
 * \return \ref CARDANO_SUCCESS if the comparison was successful, or an appropriate error code indicating failure.
 *
 * \note This function only compares ADA and multi-asset amounts between the two values. If rhs has any asset that is
 * greater than the same asset in lhs, the result will be set to `false`.
 */
static cardano_error_t
value_greater_than_or_equal(
  cardano_value_t* lhs,
  cardano_value_t* rhs,
  bool*            result)
{
  if ((lhs == NULL) || (rhs == NULL) || (result == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  uint64_t lhs_coin = cardano_value_get_coin(lhs);
  uint64_t rhs_coin = cardano_value_get_coin(rhs);

  if (lhs_coin < rhs_coin)
  {
    *result = false;
    return CARDANO_SUCCESS;
  }

  cardano_asset_id_map_t* lhs_asset_id_map = cardano_value_as_assets_map(lhs);
  cardano_asset_id_map_t* rhs_asset_id_map = cardano_value_as_assets_map(rhs);

  if ((lhs_asset_id_map != NULL) && (rhs_asset_id_map == NULL))
  {
    cardano_asset_id_map_unref(&lhs_asset_id_map);
    cardano_asset_id_map_unref(&rhs_asset_id_map);

    *result = false;

    return CARDANO_SUCCESS;
  }

  cardano_error_t op_result = CARDANO_SUCCESS;

  if (rhs != NULL)
  {
    cardano_asset_id_list_t* asset_ids = NULL;

    op_result = cardano_asset_id_map_get_keys(rhs_asset_id_map, &asset_ids);
    cardano_asset_id_list_unref(&asset_ids);

    if (op_result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&lhs_asset_id_map);
      cardano_asset_id_map_unref(&rhs_asset_id_map);

      return op_result;
    }

    const size_t assets_count = cardano_asset_id_list_get_length(asset_ids);

    for (size_t i = 0U; i < assets_count; ++i)
    {
      cardano_asset_id_t* id = NULL;

      op_result = cardano_asset_id_list_get(asset_ids, i, &id);

      cardano_asset_id_unref(&id);

      if (op_result != CARDANO_SUCCESS)
      {
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        return op_result;
      }

      int64_t lhs_multi_asset_amount = 0;
      int64_t rhs_multi_asset_amount = 0;

      op_result = cardano_asset_id_map_get(lhs_asset_id_map, id, &lhs_multi_asset_amount);

      if (op_result != CARDANO_SUCCESS)
      {
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        return op_result;
      }

      op_result = cardano_asset_id_map_get(rhs_asset_id_map, id, &rhs_multi_asset_amount);

      if (op_result != CARDANO_SUCCESS)
      {
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        return op_result;
      }

      if (lhs_multi_asset_amount < rhs_multi_asset_amount)
      {
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        *result = false;
        return CARDANO_SUCCESS;
      }
    }
  }

  cardano_asset_id_map_unref(&lhs_asset_id_map);
  cardano_asset_id_map_unref(&rhs_asset_id_map);

  *result = true;

  return CARDANO_SUCCESS;
}

/**
 * \brief Compares two UTXOs by a specific asset amount in descending order.
 *
 * This function compares the specified asset amounts in the values of two UTXOs. It sorts in descending order,
 * meaning the UTXO with the greater amount of the asset comes first.
 *
 * \param[in] lhs The left-hand side UTXO to compare.
 * \param[in] rhs The right-hand side UTXO to compare.
 * \param[in] context A pointer to the specific \ref cardano_asset_id_t representing the asset to compare.
 *
 * \return A negative value if lhs has more of the asset than rhs, a positive value if rhs has more of the asset than lhs,
 *         or 0 if they are equal in terms of the asset amount.
 */
static int32_t
compare_utxos_by_asset_desc(cardano_utxo_t* lhs, cardano_utxo_t* rhs, void* context)
{
  cardano_asset_id_t* asset_id = (cardano_asset_id_t*)context;

  cardano_transaction_output_t* lhs_output = cardano_utxo_get_output(lhs);
  cardano_transaction_output_t* rhs_output = cardano_utxo_get_output(rhs);

  cardano_value_t* lhs_value = cardano_transaction_output_get_value(lhs_output);
  cardano_value_t* rhs_value = cardano_transaction_output_get_value(rhs_output);

  uint64_t lhs_amount = get_asset_amount(lhs_value, asset_id);
  uint64_t rhs_amount = get_asset_amount(rhs_value, asset_id);

  int32_t result = 0;

  if (lhs_amount < rhs_amount)
  {
    result = 1;
  }
  else if (lhs_amount > rhs_amount)
  {
    result = -1;
  }
  else
  {
    result = 0;
  }

  cardano_value_unref(&lhs_value);
  cardano_value_unref(&rhs_value);
  cardano_transaction_output_unref(&lhs_output);
  cardano_transaction_output_unref(&rhs_output);

  return result;
}

/**
 * \brief Checks if the pre-selected UTXOs satisfy the target value.
 *
 * This function iterates over the provided list of pre-selected UTXOs and accumulates their value.
 * It checks whether the accumulated value satisfies the target value.
 *
 * \param[in] pre_selected_utxo A list of pre-selected UTXOs.
 * \param[in] target_value The target value to satisfy with the pre-selected UTXOs.
 * \param[out] accumulated_value A pointer to a \ref cardano_value_t object where the accumulated value will be stored.
 * \param[out] satisfies_target A boolean flag that will be set to true if the accumulated value satisfies the target value,
 *                              false otherwise.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were processed successfully, or an appropriate error code.
 */
static cardano_error_t
check_preselected_utxos(
  cardano_utxo_list_t* pre_selected_utxo,
  cardano_value_t*     target_value,
  cardano_value_t**    accumulated_value,
  bool*                satisfies_target)
{
  if ((pre_selected_utxo == NULL) || (target_value == NULL) || (accumulated_value == NULL) || (satisfies_target == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_value_new(0, NULL, accumulated_value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  size_t utxo_count = cardano_utxo_list_get_length(pre_selected_utxo);

  for (size_t i = 0U; i < utxo_count; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(pre_selected_utxo, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(accumulated_value);

      return result;
    }

    cardano_transaction_output_t* output     = cardano_utxo_get_output(utxo);
    cardano_value_t*              utxo_value = cardano_transaction_output_get_value(output);

    cardano_transaction_output_unref(&output);
    cardano_value_unref(&utxo_value);

    if (utxo_value == NULL)
    {
      cardano_value_unref(accumulated_value);

      return result;
    }

    cardano_value_t* new_accumulated_value = NULL;

    result = cardano_value_add(*accumulated_value, utxo_value, &new_accumulated_value);

    cardano_value_unref(accumulated_value);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    *accumulated_value = new_accumulated_value;
  }

  result = value_greater_than_or_equal(*accumulated_value, target_value, satisfies_target);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(accumulated_value);

    return result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Selects UTXOs containing the specified asset to satisfy the required amount.
 *
 * This function iterates over the available UTXOs and selects those that contain the specified asset,
 * accumulating their value until the required amount is met or exceeded.
 *
 * \param[in] asset_req The asset ID for which UTXOs are being selected.
 * \param[in] required_amount The amount of the asset required.
 * \param[in] available_utxos A list of available UTXOs to select from.
 * \param[out] selected_utxos A list where the selected UTXOs will be added.
 * \param[out] accumulated_value A pointer to a \ref cardano_value_t object that will accumulate the selected asset's value.
 *
 * \return \ref CARDANO_SUCCESS if UTXOs were successfully selected, or an appropriate error code indicating failure.
 *
 * \note The caller is responsible for managing the memory of the `selected_utxos` and `accumulated_value` lists.
 */
static cardano_error_t
select_utxos_for_asset(
  cardano_asset_id_t*  asset_req,
  int64_t              required_amount,
  cardano_utxo_list_t* available_utxos,
  cardano_utxo_list_t* selected_utxos,
  cardano_value_t**    accumulated_value)
{
  if ((asset_req == NULL) || (available_utxos == NULL) || (selected_utxos == NULL) || (accumulated_value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(*accumulated_value);

  if (assets == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t         accumulated_amount = 0;
  cardano_error_t result             = cardano_asset_id_map_get(assets, asset_req, &accumulated_amount);

  if (result != CARDANO_SUCCESS)
  {
    cardano_asset_id_map_unref(&assets);

    return result;
  }

  if (accumulated_amount >= required_amount)
  {
    cardano_asset_id_map_unref(&assets);

    return CARDANO_SUCCESS;
  }

  cardano_utxo_list_sort(available_utxos, compare_utxos_by_asset_desc, (void*)asset_req);

  size_t utxo_count = cardano_utxo_list_get_length(available_utxos);

  for (size_t i = 0U; (i < utxo_count) && (accumulated_amount < required_amount); ++i)
  {
    cardano_utxo_t* utxo = NULL;
    result               = cardano_utxo_list_get(available_utxos, i, &utxo);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&assets);

      return result;
    }

    cardano_transaction_output_t* output     = cardano_utxo_get_output(utxo);
    cardano_value_t*              utxo_value = cardano_transaction_output_get_value(output);

    int64_t utxo_asset_amount = (int64_t)get_asset_amount(utxo_value, asset_req);

    if (utxo_asset_amount > 0)
    {
      result = cardano_utxo_list_add(selected_utxos, utxo);

      if (result != CARDANO_SUCCESS)
      {
        cardano_asset_id_map_unref(&assets);
        cardano_value_unref(&utxo_value);
        cardano_transaction_output_unref(&output);
        cardano_utxo_unref(&utxo);

        return result;
      }

      accumulated_amount += utxo_asset_amount;

      cardano_value_t* new_accumulated_value = NULL;
      result                                 = cardano_value_add(*accumulated_value, utxo_value, &new_accumulated_value);

      if (result != CARDANO_SUCCESS)
      {
        cardano_asset_id_map_unref(&assets);
        cardano_value_unref(&utxo_value);
        cardano_transaction_output_unref(&output);
        cardano_utxo_unref(&utxo);

        return result;
      }

      cardano_value_unref(accumulated_value);
      *accumulated_value = new_accumulated_value;

      cardano_utxo_list_t* removed = cardano_utxo_list_erase(available_utxos, (int64_t)i, 1);
      cardano_utxo_list_unref(&removed);

      --i;
      --utxo_count;
    }

    cardano_value_unref(&utxo_value);
    cardano_transaction_output_unref(&output);
    cardano_utxo_unref(&utxo);
  }

  if (accumulated_amount < required_amount)
  {
    cardano_asset_id_map_unref(&assets);

    return CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  cardano_asset_id_map_unref(&assets);

  return CARDANO_SUCCESS;
}

/**
 * \brief Selects UTXOs from the available list and pre-selected UTXOs to meet the target value.
 *
 * This function selects UTXOs from both the pre-selected UTXO list and available UTXOs to meet a specified target value.
 * The selected UTXOs are stored in the `selection` list, and any remaining UTXOs are stored in the `remaining_utxo` list.
 *
 * \param[in] coin_selector A pointer to the coin selector implementation object.
 * \param[in] pre_selected_utxo A list of pre-selected UTXOs that must be included in the final selection.
 * \param[in] available_utxo A list of available UTXOs to select from.
 * \param[in] target A pointer to a \ref cardano_value_t object that defines the target amount of ADA and assets.
 * \param[out] selection A pointer to the list of selected UTXOs that meet the target value.
 * \param[out] remaining_utxo A pointer to the list of UTXOs that were not selected and remain available for future transactions.
 *
 * \return \ref CARDANO_SUCCESS if UTXOs were successfully selected, or an appropriate error code indicating failure.
 */
static cardano_error_t
select(
  cardano_coin_selector_impl_t* coin_selector,
  cardano_utxo_list_t*          pre_selected_utxo,
  cardano_utxo_list_t*          available_utxo,
  cardano_value_t*              target,
  cardano_utxo_list_t**         selection,
  cardano_utxo_list_t**         remaining_utxo)
{
  if ((coin_selector == NULL) || (available_utxo == NULL) || (target == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_utxo_list_new(selection);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *remaining_utxo = cardano_utxo_list_clone(available_utxo);

  if (remaining_utxo == NULL)
  {
    cardano_utxo_list_unref(selection);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_value_t* accumulated_value = NULL;

  result = cardano_value_new(0, NULL, &accumulated_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(remaining_utxo);
    cardano_utxo_list_unref(selection);

    return result;
  }

  bool preselected_utxo_satisfies_target = false;

  if (pre_selected_utxo != NULL)
  {
    result = check_preselected_utxos(pre_selected_utxo, target, &accumulated_value, &preselected_utxo_satisfies_target);

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
      cardano_utxo_list_unref(selection);
      cardano_utxo_list_unref(remaining_utxo);
      cardano_value_unref(&accumulated_value);

      return result;
    }

    result = cardano_asset_id_map_get(assets, asset_id, &asset_amount);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(selection);
      cardano_utxo_list_unref(remaining_utxo);
      cardano_value_unref(&accumulated_value);

      return result;
    }

    result = select_utxos_for_asset(asset_id, asset_amount, *remaining_utxo, *selection, &accumulated_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(selection);
      cardano_utxo_list_unref(remaining_utxo);
      cardano_value_unref(&accumulated_value);

      return result;
    }
  }

  uint64_t required_ada = cardano_value_get_coin(target);

  result = select_utxos_for_ada(required_ada, *remaining_utxo, *selection);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(selection);
    cardano_utxo_list_unref(remaining_utxo);
    cardano_value_unref(&accumulated_value);

    return result;
  }

  cardano_value_unref(&accumulated_value);

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