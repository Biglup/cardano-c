/**
 * \file change_builder.c
 *
 * \author angel.castillo
 * \date   Jul 02, 2026
 *
 * Copyright 2026 Biglup Labs
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
#include <cardano/transaction_builder/fee.h>

#include "../../../allocators.h"

#include "./change_builder.h"
#include "./large_first_helpers.h"
#include "./value_splitting.h"

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Coalesces the values of all UTXOs in a list into a single total value.
 *
 * \param[in]  utxos       The list of UTXOs to coalesce.
 * \param[out] total_value A pointer to store the cumulative value of all UTXOs.
 *
 * \return \ref CARDANO_SUCCESS if the total value was successfully calculated, or an appropriate error code.
 */
static cardano_error_t
sum_utxo_values(cardano_utxo_list_t* utxos, cardano_value_t** total_value)
{
  const size_t num_utxos = cardano_utxo_list_get_length(utxos);

  cardano_value_t* total = cardano_value_new_zero();

  if (total == NULL)
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
      cardano_value_unref(&total);

      return result;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_value_t*              value  = cardano_transaction_output_get_value(output);

    cardano_transaction_output_unref(&output);
    cardano_value_unref(&value);

    cardano_value_t* tmp_value = NULL;

    result = cardano_value_add(total, value, &tmp_value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&tmp_value);
      cardano_value_unref(&total);

      return result;
    }

    cardano_value_unref(&total);
    total = tmp_value;
  }

  *total_value = total;

  return CARDANO_SUCCESS;
}

/**
 * \brief Checks that a value has no negative components (neither lovelace nor any asset).
 *
 * \param[in]  value           The value to inspect.
 * \param[out] is_non_negative Set to true if every component of the value is greater than or equal to zero.
 *
 * \return \ref CARDANO_SUCCESS if the check completed, or an appropriate error code.
 */
static cardano_error_t
value_is_non_negative(cardano_value_t* value, bool* is_non_negative)
{
  *is_non_negative = true;

  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(value);

  if (assets == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const size_t asset_count = cardano_asset_id_map_get_length(assets);

  for (size_t i = 0U; i < asset_count; ++i)
  {
    cardano_asset_id_t* asset_id = NULL;
    cardano_error_t     result   = cardano_asset_id_map_get_key_at(assets, i, &asset_id);
    cardano_asset_id_unref(&asset_id);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&assets);

      return result;
    }

    int64_t amount = 0;
    result         = cardano_asset_id_map_get(assets, asset_id, &amount);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&assets);

      return result;
    }

    if (amount < 0)
    {
      *is_non_negative = false;
      break;
    }
  }

  cardano_asset_id_map_unref(&assets);

  return CARDANO_SUCCESS;
}

/**
 * \brief Moves the UTXO with the largest lovelace amount from `remaining_utxo` into `selection`.
 *
 * \param[in,out] selection      The selection list to which the UTXO will be added.
 * \param[in,out] remaining_utxo The pool from which the UTXO will be removed.
 * \param[out]    moved_value    The value of the moved UTXO.
 *
 * \return \ref CARDANO_SUCCESS if a UTXO carrying lovelace was moved, \ref CARDANO_ERROR_BALANCE_INSUFFICIENT
 *         if the pool is exhausted or holds no more lovelace, or an appropriate error code.
 */
static cardano_error_t
move_largest_lovelace_utxo(
  cardano_utxo_list_t* selection,
  cardano_utxo_list_t* remaining_utxo,
  cardano_value_t**    moved_value)
{
  if (cardano_utxo_list_get_length(remaining_utxo) == 0U)
  {
    return CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  cardano_asset_id_t* lovelace = NULL;
  cardano_error_t     result   = cardano_asset_id_new_lovelace(&lovelace);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_utxo_list_sort(remaining_utxo, _cardano_large_fist_compare_utxos, (void*)lovelace);
  cardano_asset_id_unref(&lovelace);

  cardano_utxo_t* utxo = NULL;
  result               = cardano_utxo_list_get(remaining_utxo, 0U, &utxo);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_value_t*              value  = cardano_transaction_output_get_value(output);

  cardano_transaction_output_unref(&output);

  if (cardano_value_get_coin(value) == 0)
  {
    cardano_value_unref(&value);
    cardano_utxo_unref(&utxo);

    return CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  result = cardano_utxo_list_add(selection, utxo);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&value);
    cardano_utxo_unref(&utxo);

    return result;
  }

  cardano_utxo_list_t* removed = cardano_utxo_list_erase(remaining_utxo, 0, 1);
  cardano_utxo_list_unref(&removed);
  cardano_utxo_unref(&utxo);

  *moved_value = value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Computes the minimum required ada for a change output holding the given value's assets.
 *
 * \param[in]  value             The asset-only value the output would hold.
 * \param[in]  change_address    The address the output would be sent to.
 * \param[in]  ada_per_utxo_byte The protocol parameter driving the min-ADA computation.
 * \param[out] min_ada           The minimum required ada quantity.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
min_ada_for_value(
  cardano_value_t*   value,
  cardano_address_t* change_address,
  const uint64_t     ada_per_utxo_byte,
  uint64_t*          min_ada)
{
  cardano_transaction_output_t* output = NULL;

  cardano_error_t result = cardano_transaction_output_new(change_address, 1, &output);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_output_set_value(output, value);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_compute_min_ada_required(output, ada_per_utxo_byte, min_ada);
  }

  cardano_transaction_output_unref(&output);

  return result;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
_cardano_coin_selector_build_change(
  cardano_value_t*                    target,
  cardano_address_t*                  change_address,
  cardano_protocol_parameters_t*      protocol_params,
  cardano_utxo_list_t*                selection,
  cardano_utxo_list_t*                remaining_utxo,
  cardano_transaction_output_list_t** change_outputs)
{
  if ((target == NULL) || (change_address == NULL) || (protocol_params == NULL) || (selection == NULL) || (remaining_utxo == NULL) || (change_outputs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_transaction_output_list_new(change_outputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_value_t* selected_value = NULL;
  result                          = sum_utxo_values(selection, &selected_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_output_list_unref(change_outputs);

    return result;
  }

  cardano_value_t* change_value = NULL;
  result                        = cardano_value_subtract(selected_value, target, &change_value);

  cardano_value_unref(&selected_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_output_list_unref(change_outputs);

    return result;
  }

  bool change_is_non_negative = false;
  result                      = value_is_non_negative(change_value, &change_is_non_negative);

  if ((result == CARDANO_SUCCESS) && !change_is_non_negative)
  {
    result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&change_value);
    cardano_transaction_output_list_unref(change_outputs);

    return result;
  }

  if (cardano_value_is_zero(change_value))
  {
    cardano_value_unref(&change_value);

    return CARDANO_SUCCESS;
  }

  const uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);
  const uint64_t max_value_size    = cardano_protocol_parameters_get_max_value_size(protocol_params);

  cardano_value_part_list_t parts = { NULL, 0U, 0U };

  uint64_t* min_adas      = NULL;
  uint64_t  total_min_ada = 0U;
  bool      fundable      = false;

  // Split the change assets into as many outputs as the maximum output value size requires, then
  // keep consuming additional UTXOs until the change carries enough ada to make every resulting
  // output min-ADA compliant. Consuming a UTXO can add assets to the change, so the split is
  // recomputed on every iteration.
  while ((result == CARDANO_SUCCESS) && !fundable)
  {
    _cardano_coin_selection_value_parts_free(&parts);
    _cardano_free(min_adas);
    min_adas = NULL;

    result = _cardano_coin_selection_split_value_assets(change_value, max_value_size, &parts);

    if (result == CARDANO_SUCCESS)
    {
      min_adas = (uint64_t*)_cardano_malloc(parts.size * sizeof(uint64_t));

      if (min_adas == NULL)
      {
        result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }
    }

    if (result == CARDANO_SUCCESS)
    {
      total_min_ada = 0U;

      for (size_t i = 0U; (i < parts.size) && (result == CARDANO_SUCCESS); ++i)
      {
        result = min_ada_for_value(parts.items[i], change_address, ada_per_utxo_byte, &min_adas[i]);

        if (result == CARDANO_SUCCESS)
        {
          total_min_ada += min_adas[i];
        }
      }
    }

    if (result == CARDANO_SUCCESS)
    {
      if (cardano_value_get_coin(change_value) >= (int64_t)total_min_ada)
      {
        fundable = true;
      }
      else
      {
        cardano_value_t* moved_value = NULL;

        result = move_largest_lovelace_utxo(selection, remaining_utxo, &moved_value);

        if (result == CARDANO_SUCCESS)
        {
          cardano_value_t* new_change_value = NULL;

          result = cardano_value_add(change_value, moved_value, &new_change_value);

          cardano_value_unref(&moved_value);

          if (result == CARDANO_SUCCESS)
          {
            cardano_value_unref(&change_value);
            change_value = new_change_value;
          }
        }
      }
    }
  }

  // Assign the ada: every part receives its min-ADA requirement, and the last part additionally
  // receives the remainder, so that the parts sum exactly to the change value.
  const int64_t change_coin = cardano_value_get_coin(change_value);

  for (size_t i = 0U; (i < parts.size) && (result == CARDANO_SUCCESS); ++i)
  {
    uint64_t part_coin = min_adas[i];

    if (i == (parts.size - 1U))
    {
      part_coin += (uint64_t)change_coin - total_min_ada;
    }

    result = cardano_value_set_coin(parts.items[i], (int64_t)part_coin);

    cardano_transaction_output_t* change_output = NULL;

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_transaction_output_new(change_address, 0, &change_output);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_transaction_output_set_value(change_output, parts.items[i]);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_transaction_output_list_add(*change_outputs, change_output);
    }

    cardano_transaction_output_unref(&change_output);
  }

  _cardano_coin_selection_value_parts_free(&parts);
  _cardano_free(min_adas);
  cardano_value_unref(&change_value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_output_list_unref(change_outputs);

    return result;
  }

  return CARDANO_SUCCESS;
}
