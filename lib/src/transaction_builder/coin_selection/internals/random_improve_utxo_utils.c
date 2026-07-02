/**
 * \file random_improve_utxo_utils.c
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
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

#include <cardano/assets/asset_id_map.h>
#include <cardano/assets/multi_asset.h>

#include "./random_improve_utxo_utils.h"

#include "../../../allocators.h"
#include "./random_improve_helpers.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Counts the assets of a UTXO, counting lovelace as an asset when the coin is non-zero.
 *
 * This mirrors the asset counting convention of the reference implementation's UTxO index, where
 * a pure-ada UTXO has one asset, and a UTXO holding ada plus one token has two.
 *
 * \param[in] utxo The UTXO to inspect.
 *
 * \return The number of distinct assets in the UTXO, including lovelace.
 */
static size_t
utxo_asset_count(cardano_utxo_t* utxo)
{
  cardano_value_t* value = _cardano_random_improve_borrow_utxo_value(utxo);

  size_t count = 0U;

  if (cardano_value_get_coin(value) != 0)
  {
    count = 1U;
  }

  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  if (multi_asset != NULL)
  {
    cardano_asset_id_map_t* assets = cardano_value_as_assets_map(value);

    if (assets != NULL)
    {
      count = cardano_asset_id_map_get_length(assets);

      cardano_asset_id_map_unref(&assets);
    }
  }

  return count;
}

/**
 * \brief Indicates whether the given UTXO contains the given asset (NULL asset id = lovelace).
 *
 * \param[in] utxo     The UTXO to inspect.
 * \param[in] asset_id The asset to look for, or NULL for lovelace.
 *
 * \return true if the UTXO contains a positive quantity of the asset, false otherwise.
 */
static bool
utxo_has_asset(cardano_utxo_t* utxo, cardano_asset_id_t* asset_id)
{
  cardano_value_t* value = _cardano_random_improve_borrow_utxo_value(utxo);

  if (asset_id == NULL)
  {
    return cardano_value_get_coin(value) > 0;
  }

  return _cardano_random_improve_get_asset_quantity(value, asset_id) > 0;
}

/* DEFINITIONS ****************************************************************/

int64_t
_cardano_random_improve_get_asset_quantity(cardano_value_t* value, cardano_asset_id_t* asset_id)
{
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  if (multi_asset == NULL)
  {
    return 0;
  }

  int64_t quantity = 0;

  if (cardano_multi_asset_get_with_id(multi_asset, asset_id, &quantity) != CARDANO_SUCCESS)
  {
    return 0;
  }

  return quantity;
}

cardano_value_t*
_cardano_random_improve_borrow_utxo_value(cardano_utxo_t* utxo)
{
  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_value_t*              value  = cardano_transaction_output_get_value(output);

  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return value;
}

bool
_cardano_random_improve_asset_id_equals(cardano_asset_id_t* lhs, cardano_asset_id_t* rhs)
{
  const char* lhs_hex = cardano_asset_id_get_hex(lhs);
  const char* rhs_hex = cardano_asset_id_get_hex(rhs);

  if ((lhs_hex == NULL) || (rhs_hex == NULL))
  {
    return false;
  }

  return strcmp(lhs_hex, rhs_hex) == 0;
}

uint64_t
_cardano_random_improve_selected_quantity(cardano_utxo_list_t* selection, cardano_asset_id_t* asset_id)
{
  uint64_t total = 0U;

  const size_t size = cardano_utxo_list_get_length(selection);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    if (cardano_utxo_list_get(selection, i, &utxo) != CARDANO_SUCCESS)
    {
      continue;
    }

    cardano_utxo_unref(&utxo);

    cardano_value_t* value = _cardano_random_improve_borrow_utxo_value(utxo);

    int64_t quantity = 0;

    if (asset_id == NULL)
    {
      quantity = cardano_value_get_coin(value);
    }
    else
    {
      quantity = _cardano_random_improve_get_asset_quantity(value, asset_id);
    }

    if (quantity > 0)
    {
      total += (uint64_t)quantity;
    }
  }

  return total;
}

bool
_cardano_random_improve_select_random_with_priority(
  uint64_t*            rng_state,
  cardano_utxo_list_t* selection,
  cardano_utxo_list_t* leftover,
  cardano_asset_id_t*  asset_id)
{
  const size_t leftover_size = cardano_utxo_list_get_length(leftover);

  if (leftover_size == 0U)
  {
    return false;
  }

  size_t* candidates = (size_t*)_cardano_malloc(leftover_size * sizeof(size_t));

  if (candidates == NULL)
  {
    return false;
  }

  bool moved = false;

  for (size_t max_assets = 1U; (max_assets <= 3U) && !moved; ++max_assets)
  {
    size_t candidate_count = 0U;

    for (size_t i = 0U; i < leftover_size; ++i)
    {
      cardano_utxo_t* utxo = NULL;

      if (cardano_utxo_list_get(leftover, i, &utxo) != CARDANO_SUCCESS)
      {
        continue;
      }

      cardano_utxo_unref(&utxo);

      if (!utxo_has_asset(utxo, asset_id))
      {
        continue;
      }

      const size_t asset_count = utxo_asset_count(utxo);

      if ((max_assets < 3U) && (asset_count != max_assets))
      {
        continue;
      }

      candidates[candidate_count] = i;
      ++candidate_count;
    }

    if (candidate_count > 0U)
    {
      const size_t index = candidates[_cardano_random_improve_rng_below(rng_state, candidate_count)];

      cardano_utxo_t* utxo = NULL;

      if (cardano_utxo_list_get(leftover, index, &utxo) == CARDANO_SUCCESS)
      {
        if (cardano_utxo_list_add(selection, utxo) == CARDANO_SUCCESS)
        {
          cardano_utxo_list_t* removed = cardano_utxo_list_erase(leftover, (int64_t)index, 1);
          cardano_utxo_list_unref(&removed);

          moved = true;
        }

        cardano_utxo_unref(&utxo);
      }
    }
  }

  _cardano_free(candidates);

  return moved;
}

void
_cardano_random_improve_undo_last_selection(cardano_utxo_list_t* selection, cardano_utxo_list_t* leftover)
{
  const size_t size = cardano_utxo_list_get_length(selection);

  if (size == 0U)
  {
    return;
  }

  cardano_utxo_t* utxo = NULL;

  if (cardano_utxo_list_get(selection, size - 1U, &utxo) == CARDANO_SUCCESS)
  {
    cardano_utxo_unref(&utxo);

    if (cardano_utxo_list_add(leftover, utxo) == CARDANO_SUCCESS)
    {
      const size_t last_index = size - 1U;

      cardano_utxo_list_t* removed = cardano_utxo_list_erase(selection, (int64_t)last_index, 1);
      cardano_utxo_list_unref(&removed);
    }
  }
}

cardano_error_t
_cardano_random_improve_move_pre_selected(
  cardano_utxo_list_t* pre_selected_utxo,
  cardano_utxo_list_t* selection,
  cardano_utxo_list_t* leftover)
{
  cardano_error_t result = CARDANO_SUCCESS;

  const size_t size = (pre_selected_utxo != NULL) ? cardano_utxo_list_get_length(pre_selected_utxo) : 0U;

  for (size_t i = 0U; (i < size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(pre_selected_utxo, i, &utxo);

    if (result == CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);

      result = cardano_utxo_list_add(selection, utxo);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_utxo_list_remove(leftover, utxo);
      }
    }
  }

  return result;
}
