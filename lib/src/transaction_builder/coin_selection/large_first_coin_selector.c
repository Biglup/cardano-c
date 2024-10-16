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

#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/object.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/typedefs.h>

#include "../../allocators.h"
#include "../../string_safe.h"

#include <assert.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a large_first_coin_selector object.
 *
 * This function is responsible for properly deallocating a large_first_coin_selector object (`cardano_large_first_coin_selector_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the large_first_coin_selector object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_large_first_coin_selector_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the large_first_coin_selector
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_large_first_coin_selector_deallocate(void* object)
{
  assert(object != NULL);

  _cardano_free(object);
}

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

  if (lhs < rhs)
  {
    *result = false;
    return CARDANO_SUCCESS;
  }

  cardano_multi_asset_t* lhs_multi_asset = cardano_value_get_multi_asset(lhs);
  cardano_multi_asset_t* rhs_multi_asset = cardano_value_get_multi_asset(rhs);

  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);

  if ((rhs_multi_asset != NULL) && (lhs_multi_asset == NULL))
  {
    *result = false;
    return CARDANO_SUCCESS;
  }

  if (rhs != NULL)
  {
    cardano_asset_id_list_t* asset_ids = NULL;

    result = cardano_multi_asset_get_keys(&asset_ids);
    cardano_asset_id_list_unref(&asset_ids);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    const size_t assets_count = cardano_asset_id_list_get_length(asset_ids);

    for (size_t i = 0U; i < assets_count; ++i)
    {
      cardano_asset_id_t* id = NULL;

      result = cardano_asset_id_list_get(asset_ids, i, &id);

      cardano_asset_id_unref(&id);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      int64_t lhs_multi_asset_amount = 0;
      int64_t rhs_multi_asset_amount = 0;

      result = cardano_multi_asset_get_with_id(lhs_multi_asset, id, &lhs_multi_asset_amount);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_multi_asset_get_with_id(rhs_multi_asset, id, &rhs_multi_asset_amount);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (lhs_multi_asset_amount < rhs_multi_asset_amount)
      {
        *result = false;
        return CARDANO_SUCCESS;
      }
    }
  }

  *result = true;

  return CARDANO_SUCCESS;
}

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

      return error;
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

        return error;
      }

      result = cardano_utxo_list_add(*selection, utxo);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utxo_list_unref(remaining_utxo);
        cardano_utxo_list_unref(selection);
        cardano_value_unref(&accumulated_value);

        return error;
      }

      result = cardano_utxo_list_remove(*remaining_utxo, utxo);
    }

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(remaining_utxo);
      cardano_utxo_list_unref(selection);
      cardano_value_unref(&accumulated_value);

      return error;
    }
  }

  // To be continued...

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

  impl.select  = NULL;
  impl.context = NULL;

  return cardano_coin_selector_new(impl, cardano_coin_selector);
}