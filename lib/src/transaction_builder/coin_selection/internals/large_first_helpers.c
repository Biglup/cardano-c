/**
 * \file large_first_utils.c
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

#include "./large_first_helpers.h"

#include <string.h>

/* DEFINITIONS ****************************************************************/

int64_t
_cardano_large_fist_get_amount(cardano_value_t* value, cardano_asset_id_t* asset_id)
{
  if (!value || !asset_id)
  {
    return 0;
  }

  if (cardano_asset_id_is_lovelace(asset_id))
  {
    return (int64_t)cardano_value_get_coin(value);
  }

  int64_t amount = 0;

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

cardano_error_t
_cardano_large_fist_value_gte(
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

  if ((lhs_asset_id_map == NULL) || (rhs_asset_id_map == NULL))
  {
    cardano_asset_id_map_unref(&lhs_asset_id_map);
    cardano_asset_id_map_unref(&rhs_asset_id_map);

    *result = false;

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t op_result;

  if (rhs != NULL)
  {
    cardano_asset_id_list_t* asset_ids = NULL;

    op_result = cardano_asset_id_map_get_keys(rhs_asset_id_map, &asset_ids);

    if (op_result != CARDANO_SUCCESS)
    {
      cardano_asset_id_list_unref(&asset_ids);
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
        cardano_asset_id_list_unref(&asset_ids);
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        return op_result;
      }

      int64_t lhs_multi_asset_amount = 0;
      int64_t rhs_multi_asset_amount = 0;

      op_result = cardano_asset_id_map_get(lhs_asset_id_map, id, &lhs_multi_asset_amount);

      if (op_result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
      {
        cardano_asset_id_list_unref(&asset_ids);
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        *result = false;

        return CARDANO_SUCCESS;
      }

      if (op_result != CARDANO_SUCCESS)
      {
        cardano_asset_id_list_unref(&asset_ids);
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        return op_result;
      }

      op_result = cardano_asset_id_map_get(rhs_asset_id_map, id, &rhs_multi_asset_amount);

      if (op_result != CARDANO_SUCCESS)
      {
        cardano_asset_id_list_unref(&asset_ids);
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        return op_result;
      }

      if (lhs_multi_asset_amount < rhs_multi_asset_amount)
      {
        cardano_asset_id_list_unref(&asset_ids);
        cardano_asset_id_map_unref(&lhs_asset_id_map);
        cardano_asset_id_map_unref(&rhs_asset_id_map);

        *result = false;
        return CARDANO_SUCCESS;
      }
    }

    cardano_asset_id_list_unref(&asset_ids);
  }

  cardano_asset_id_map_unref(&lhs_asset_id_map);
  cardano_asset_id_map_unref(&rhs_asset_id_map);

  *result = true;

  return CARDANO_SUCCESS;
}

int32_t
_cardano_large_fist_compare_utxos(cardano_utxo_t* lhs, cardano_utxo_t* rhs, void* context)
{
  cardano_asset_id_t* asset_id = (cardano_asset_id_t*)context;

  cardano_transaction_output_t* lhs_output = cardano_utxo_get_output(lhs);
  cardano_transaction_output_t* rhs_output = cardano_utxo_get_output(rhs);

  cardano_value_t* lhs_value = cardano_transaction_output_get_value(lhs_output);
  cardano_value_t* rhs_value = cardano_transaction_output_get_value(rhs_output);

  int64_t lhs_amount = _cardano_large_fist_get_amount(lhs_value, asset_id);
  int64_t rhs_amount = _cardano_large_fist_get_amount(rhs_value, asset_id);

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

cardano_error_t
_cardano_large_fist_check_preselected(
  cardano_utxo_list_t* pre_selected_utxo,
  cardano_value_t*     target_value,
  cardano_value_t**    accumulated_value,
  bool*                satisfies_target)
{
  if ((pre_selected_utxo == NULL) || (target_value == NULL) || (satisfies_target == NULL) || (accumulated_value == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (*accumulated_value == NULL)
  {
    result = cardano_value_new(0, NULL, accumulated_value);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
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

  result = _cardano_large_fist_value_gte(*accumulated_value, target_value, satisfies_target);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(accumulated_value);

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_large_fist_select_utxos(
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

  cardano_error_t result = CARDANO_SUCCESS;

  if (*accumulated_value == NULL)
  {
    result = cardano_value_new(0, NULL, accumulated_value);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(*accumulated_value);

  if (assets == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t accumulated_amount = 0;
  result                     = cardano_asset_id_map_get(assets, asset_req, &accumulated_amount);

  if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_ELEMENT_NOT_FOUND))
  {
    cardano_asset_id_map_unref(&assets);

    return result;
  }

  if (accumulated_amount >= required_amount)
  {
    cardano_asset_id_map_unref(&assets);

    return CARDANO_SUCCESS;
  }

  cardano_utxo_list_sort(available_utxos, _cardano_large_fist_compare_utxos, (void*)asset_req);

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

    int64_t utxo_asset_amount = _cardano_large_fist_get_amount(utxo_value, asset_req);

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
