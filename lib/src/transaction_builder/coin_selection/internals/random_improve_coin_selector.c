/**
 * \file random_improve_coin_selector.c
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
#include <cardano/assets/multi_asset.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>
#include <cardano/transaction_builder/fee.h>

#include "../../../allocators.h"
#include "../../../string_safe.h"
#include "./random_improve_helpers.h"

#include <sodium.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Context for the random improve coin selector, holding the RNG state.
 */
typedef struct random_improve_context_t
{
    cardano_object_t base;
    uint64_t         seed;
} random_improve_context_t;

/**
 * \brief A requirement processed by the round-robin selection loop.
 *
 * The lovelace requirement is represented with a NULL asset id.
 */
typedef struct selection_processor_t
{
    cardano_asset_id_t* asset_id;
    uint64_t            minimum;
    bool                active;
} selection_processor_t;

/**
 * \brief Working table of every asset involved in a selection (target and selected inputs).
 */
typedef struct asset_table_t
{
    cardano_asset_id_t** ids;
    size_t               size;
    size_t               capacity;
} asset_table_t;

/* STATIC FUNCTIONS **********************************************************/

static uint64_t
rng_next(uint64_t* rng_state)
{
  *rng_state += 0x9e3779b97f4a7c15ULL;

  uint64_t z = *rng_state;
  z          = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  z          = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;

  return z ^ (z >> 31U);
}

static size_t
rng_below(uint64_t* rng_state, const size_t bound)
{
  return (size_t)(rng_next(rng_state) % (uint64_t)bound);
}

/**
 * \brief Returns the quantity of the given asset in the given value (0 if absent).
 */
static int64_t
value_get_asset_quantity(cardano_value_t* value, cardano_asset_id_t* asset_id)
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

/**
 * \brief Returns the value of the output of the given UTXO as a borrowed reference.
 */
static cardano_value_t*
utxo_borrow_value(cardano_utxo_t* utxo)
{
  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_value_t*              value  = cardano_transaction_output_get_value(output);

  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return value;
}

static bool
asset_id_same(cardano_asset_id_t* lhs, cardano_asset_id_t* rhs)
{
  const char* lhs_hex = cardano_asset_id_get_hex(lhs);
  const char* rhs_hex = cardano_asset_id_get_hex(rhs);

  if ((lhs_hex == NULL) || (rhs_hex == NULL))
  {
    return false;
  }

  return strcmp(lhs_hex, rhs_hex) == 0;
}

/**
 * \brief Counts the assets of a UTXO, counting lovelace as an asset when the coin is non-zero.
 */
static size_t
utxo_asset_count(cardano_utxo_t* utxo)
{
  cardano_value_t* value = utxo_borrow_value(utxo);

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
      const size_t entries = cardano_asset_id_map_get_length(assets);
      count                = entries;

      cardano_asset_id_map_unref(&assets);
    }
  }

  return count;
}

/**
 * \brief Indicates whether the given UTXO contains the given asset (NULL asset id = lovelace).
 */
static bool
utxo_has_asset(cardano_utxo_t* utxo, cardano_asset_id_t* asset_id)
{
  cardano_value_t* value = utxo_borrow_value(utxo);

  if (asset_id == NULL)
  {
    return cardano_value_get_coin(value) > 0;
  }

  return value_get_asset_quantity(value, asset_id) > 0;
}

/**
 * \brief Selects a random UTXO containing the given asset from the leftover list and moves it
 * into the selection, giving priority to UTXOs with fewer assets.
 *
 * Priority tiers (port of SelectSingleton / SelectPairWith / SelectAnyWith): UTXOs whose total
 * asset count (lovelace included) is 1, then 2, then any. A NULL asset id selects by lovelace.
 *
 * \return true if a UTXO was moved, false if no UTXO in the leftover list contains the asset.
 */
static bool
select_random_with_priority(
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
      const size_t index = candidates[rng_below(rng_state, candidate_count)];

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

/**
 * \brief Returns the total selected quantity of the given asset (NULL asset id = lovelace).
 */
static uint64_t
selected_quantity(cardano_utxo_list_t* selection, cardano_asset_id_t* asset_id)
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

    cardano_value_t* value = utxo_borrow_value(utxo);

    if (asset_id == NULL)
    {
      const int64_t coin = cardano_value_get_coin(value);
      total              += (coin > 0) ? (uint64_t)coin : 0U;
    }
    else
    {
      const int64_t quantity = value_get_asset_quantity(value, asset_id);
      total                  += (quantity > 0) ? (uint64_t)quantity : 0U;
    }
  }

  return total;
}

static uint64_t
distance_u64(const uint64_t a, const uint64_t b)
{
  uint64_t result = b - a;

  if (a > b)
  {
    result = a - b;
  }

  return result;
}

/**
 * \brief Runs a single selection step for the given processor (port of runSelectionStep).
 *
 * While the selected quantity is below the minimum, any successful selection is accepted.
 * Once at or above the minimum, a selection is accepted only if it moves the selected quantity
 * strictly closer to twice the minimum; otherwise it is undone.
 *
 * \return true if the step improved the selection, false if the processor is exhausted.
 */
static bool
run_selection_step(
  uint64_t*              rng_state,
  cardano_utxo_list_t*   selection,
  cardano_utxo_list_t*   leftover,
  selection_processor_t* processor)
{
  const uint64_t current = selected_quantity(selection, processor->asset_id);

  if (current < processor->minimum)
  {
    return select_random_with_priority(rng_state, selection, leftover, processor->asset_id);
  }

  const uint64_t target = processor->minimum * 2U;

  if (!select_random_with_priority(rng_state, selection, leftover, processor->asset_id))
  {
    return false;
  }

  const uint64_t updated = selected_quantity(selection, processor->asset_id);

  if (distance_u64(target, updated) < distance_u64(target, current))
  {
    return true;
  }

  const size_t last = cardano_utxo_list_get_length(selection) - 1U;

  cardano_utxo_t* utxo = NULL;

  if (cardano_utxo_list_get(selection, last, &utxo) == CARDANO_SUCCESS)
  {
    cardano_utxo_unref(&utxo);

    if (cardano_utxo_list_add(leftover, utxo) == CARDANO_SUCCESS)
    {
      cardano_utxo_list_t* removed = cardano_utxo_list_erase(selection, (int64_t)last, 1);
      cardano_utxo_list_unref(&removed);
    }
  }

  return false;
}

/* ASSET TABLE ***************************************************************/

static void
asset_table_free(asset_table_t* table)
{
  for (size_t i = 0U; i < table->size; ++i)
  {
    cardano_asset_id_unref(&table->ids[i]);
  }

  _cardano_free(table->ids);

  table->ids      = NULL;
  table->size     = 0U;
  table->capacity = 0U;
}

static cardano_error_t
asset_table_add(asset_table_t* table, cardano_asset_id_t* asset_id)
{
  for (size_t i = 0U; i < table->size; ++i)
  {
    if (asset_id_same(table->ids[i], asset_id))
    {
      return CARDANO_SUCCESS;
    }
  }

  if (table->size == table->capacity)
  {
    const size_t         new_capacity = (table->capacity == 0U) ? 8U : (table->capacity * 2U);
    cardano_asset_id_t** new_ids      = (cardano_asset_id_t**)_cardano_malloc(new_capacity * sizeof(cardano_asset_id_t*));

    if (new_ids == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    if (table->ids != NULL)
    {
      cardano_safe_memcpy(new_ids, new_capacity * sizeof(cardano_asset_id_t*), table->ids, table->size * sizeof(cardano_asset_id_t*));
      _cardano_free(table->ids);
    }

    table->ids      = new_ids;
    table->capacity = new_capacity;
  }

  cardano_asset_id_ref(asset_id);
  table->ids[table->size] = asset_id;
  ++table->size;

  return CARDANO_SUCCESS;
}

/**
 * \brief Adds every non-lovelace asset of the given value to the table.
 */
static cardano_error_t
asset_table_add_value_assets(asset_table_t* table, cardano_value_t* value)
{
  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(value);

  if (assets == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  const size_t size = cardano_asset_id_map_get_length(assets);

  for (size_t i = 0U; (i < size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_asset_id_t* asset_id = NULL;

    result = cardano_asset_id_map_get_key_at(assets, i, &asset_id);

    if (result == CARDANO_SUCCESS)
    {
      if (!cardano_asset_id_is_lovelace(asset_id))
      {
        result = asset_table_add(table, asset_id);
      }

      cardano_asset_id_unref(&asset_id);
    }
  }

  cardano_asset_id_map_unref(&assets);

  return result;
}

/* CHANGE GENERATION *********************************************************/

/**
 * \brief Computes the minimum required ada for a change output holding the given quantities.
 */
static cardano_error_t
min_ada_for_change_map(
  asset_table_t*     table,
  const uint64_t*    quantities,
  const size_t       map_count,
  const size_t       map_index,
  cardano_address_t* change_address,
  const uint64_t     ada_per_utxo_byte,
  uint64_t*          min_ada)
{
  cardano_value_t* value = cardano_value_new_zero();

  if (value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_value_set_coin(value, 1);

  for (size_t a = 0U; (a < table->size) && (result == CARDANO_SUCCESS); ++a)
  {
    const uint64_t quantity = quantities[(a * map_count) + map_index];

    if (quantity > 0U)
    {
      result = cardano_value_add_asset_with_id(value, table->ids[a], (int64_t)quantity);
    }
  }

  cardano_transaction_output_t* output = NULL;

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_output_new(change_address, 0, &output);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_output_set_value(output, value);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_compute_min_ada_required(output, ada_per_utxo_byte, min_ada);
  }

  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return result;
}

/**
 * \brief Attempts to construct change bundles for the current selection (port of makeChange).
 *
 * On success, the resulting change outputs are stored in `change_outputs`. If the selected ada
 * is insufficient to make every non-empty change bundle min-ADA compliant, the function returns
 * \ref CARDANO_ERROR_BALANCE_INSUFFICIENT so that the caller can select additional ada and retry.
 *
 * \param[out] change_count_out The number of change outputs generated (may be fewer than the
 *                              requested map count when ada was tight and empty maps were dropped).
 */
static cardano_error_t
make_change(
  cardano_utxo_list_t*                selection,
  cardano_value_t*                    target,
  cardano_transaction_output_list_t*  outputs_to_cover,
  cardano_address_t*                  change_address,
  cardano_protocol_parameters_t*      protocol_params,
  cardano_transaction_output_list_t** change_outputs,
  size_t*                             change_count_out)
{
  const size_t output_count = (outputs_to_cover != NULL) ? cardano_transaction_output_list_get_length(outputs_to_cover) : 0U;
  const size_t map_count    = (output_count > 0U) ? output_count : 1U;

  *change_outputs   = NULL;
  *change_count_out = 0U;

  const size_t selection_size = cardano_utxo_list_get_length(selection);

  asset_table_t table = { NULL, 0U, 0U };

  cardano_error_t result = asset_table_add_value_assets(&table, target);

  for (size_t i = 0U; (i < selection_size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(selection, i, &utxo);

    if (result == CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);
      result = asset_table_add_value_assets(&table, utxo_borrow_value(utxo));
    }
  }

  if (result != CARDANO_SUCCESS)
  {
    asset_table_free(&table);

    return result;
  }

  const size_t asset_count = table.size;

  uint64_t* quantities   = NULL;
  uint64_t* scratch      = NULL;
  uint64_t* out_weights  = NULL;
  size_t*   map_order    = NULL;
  int64_t   excess_coin  = 0;
  bool      excess_valid = true;

  if (asset_count > 0U)
  {
    quantities = (uint64_t*)_cardano_malloc(asset_count * map_count * sizeof(uint64_t));
    scratch    = (uint64_t*)_cardano_malloc(((selection_size > map_count) ? selection_size : map_count) * sizeof(uint64_t));

    if ((quantities == NULL) || (scratch == NULL))
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    else
    {
      CARDANO_UNUSED(memset(quantities, 0, asset_count * map_count * sizeof(uint64_t)));
    }
  }

  out_weights = (uint64_t*)_cardano_malloc(map_count * sizeof(uint64_t));
  map_order   = (size_t*)_cardano_malloc(map_count * sizeof(size_t));

  if ((out_weights == NULL) || (map_order == NULL))
  {
    result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  int64_t selected_coin = 0;

  for (size_t i = 0U; (i < selection_size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(selection, i, &utxo);

    if (result == CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);
      selected_coin += cardano_value_get_coin(utxo_borrow_value(utxo));
    }
  }

  excess_coin = selected_coin - cardano_value_get_coin(target);

  if (excess_coin < 0)
  {
    excess_valid = false;
  }

  for (size_t m = 0U; (m < map_count) && (result == CARDANO_SUCCESS); ++m)
  {
    map_order[m]   = m;
    out_weights[m] = 1U;

    if (output_count > 0U)
    {
      cardano_transaction_output_t* output = NULL;

      result = cardano_transaction_output_list_get(outputs_to_cover, m, &output);

      if (result == CARDANO_SUCCESS)
      {
        cardano_value_t* value = cardano_transaction_output_get_value(output);

        const int64_t coin = cardano_value_get_coin(value);

        if (coin > 0)
        {
          out_weights[m] = (uint64_t)coin;
        }
        else
        {
          out_weights[m] = 0U;
        }

        cardano_value_unref(&value);
        cardano_transaction_output_unref(&output);
      }
    }
  }

  // Distribute the excess of each asset across the change maps.
  for (size_t a = 0U; ((a < asset_count) && (result == CARDANO_SUCCESS)) && excess_valid; ++a)
  {
    cardano_asset_id_t* asset_id = table.ids[a];

    int64_t selected_total = 0;

    size_t input_quantity_count = 0U;

    for (size_t i = 0U; i < selection_size; ++i)
    {
      cardano_utxo_t* utxo = NULL;

      if (cardano_utxo_list_get(selection, i, &utxo) != CARDANO_SUCCESS)
      {
        continue;
      }

      cardano_utxo_unref(&utxo);

      const int64_t quantity = value_get_asset_quantity(utxo_borrow_value(utxo), asset_id);

      if (quantity > 0)
      {
        selected_total                += quantity;
        scratch[input_quantity_count] = (uint64_t)quantity;
        ++input_quantity_count;
      }
    }

    const int64_t required = value_get_asset_quantity(target, asset_id);
    const int64_t excess   = selected_total - required;

    if (excess < 0)
    {
      excess_valid = false;
      continue;
    }

    if (excess == 0)
    {
      continue;
    }

    // Determine whether this asset is user-specified (present in the outputs to cover).
    uint64_t user_weight_total = 0U;

    if (output_count > 0U)
    {
      for (size_t m = 0U; m < output_count; ++m)
      {
        cardano_transaction_output_t* output = NULL;

        if (cardano_transaction_output_list_get(outputs_to_cover, m, &output) != CARDANO_SUCCESS)
        {
          continue;
        }

        cardano_transaction_output_unref(&output);

        cardano_value_t* value = cardano_transaction_output_get_value(output);
        cardano_value_unref(&value);

        const int64_t quantity = value_get_asset_quantity(value, asset_id);

        scratch[m]        = (quantity > 0) ? (uint64_t)quantity : 0U;
        user_weight_total += scratch[m];
      }
    }

    if (user_weight_total > 0U)
    {
      // User-specified asset: partition the excess proportionally to the output quantities.
      uint64_t* parts = (uint64_t*)_cardano_malloc(map_count * sizeof(uint64_t));

      if (parts == NULL)
      {
        result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        continue;
      }

      result = _cardano_random_improve_partition((uint64_t)excess, scratch, map_count, parts);

      if (result == CARDANO_SUCCESS)
      {
        for (size_t m = 0U; m < map_count; ++m)
        {
          quantities[(a * map_count) + m] = parts[m];
        }
      }

      _cardano_free(parts);
    }
    else
    {
      // Non-user-specified asset: preserve the granularity of the selected inputs, then
      // reconcile mints (excess > selected) and burns (excess < selected).
      uint64_t* parts = (uint64_t*)_cardano_malloc(map_count * sizeof(uint64_t));

      if (parts == NULL)
      {
        result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        continue;
      }

      if (input_quantity_count == 0U)
      {
        CARDANO_UNUSED(memset(parts, 0, map_count * sizeof(uint64_t)));
      }
      else
      {
        result = _cardano_random_improve_pad_coalesce(scratch, input_quantity_count, map_count, parts);
      }

      if (result == CARDANO_SUCCESS)
      {
        if (excess < selected_total)
        {
          const int64_t burn_amount = selected_total - excess;

          _cardano_random_improve_reduce_quantities((uint64_t)burn_amount, parts, map_count);
        }

        if (excess > selected_total)
        {
          const int64_t mint_amount = excess - selected_total;

          parts[map_count - 1U] += (uint64_t)mint_amount;
        }

        for (size_t m = 0U; m < map_count; ++m)
        {
          quantities[(a * map_count) + m] = parts[m];
        }
      }

      _cardano_free(parts);
    }
  }

  if ((result == CARDANO_SUCCESS) && !excess_valid)
  {
    result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  // Order the maps by asset count ascending (stable), so empty maps come first.
  if (result == CARDANO_SUCCESS)
  {
    for (size_t i = 1U; i < map_count; ++i)
    {
      const size_t key = map_order[i];

      size_t key_assets = 0U;

      for (size_t a = 0U; a < asset_count; ++a)
      {
        key_assets += (quantities[(a * map_count) + key] > 0U) ? 1U : 0U;
      }

      size_t j        = i;
      bool   in_place = false;

      while ((j > 0U) && !in_place)
      {
        size_t prev_assets = 0U;

        for (size_t a = 0U; a < asset_count; ++a)
        {
          prev_assets += (quantities[(a * map_count) + map_order[j - 1U]] > 0U) ? 1U : 0U;
        }

        if (prev_assets <= key_assets)
        {
          in_place = true;
        }
        else
        {
          map_order[j] = map_order[j - 1U];
          --j;
        }
      }

      map_order[j] = key;
    }
  }

  // Assign coins: drop empty maps from the front while the available ada is insufficient.
  size_t    first_map    = 0U;
  uint64_t* min_adas     = NULL;
  uint64_t  ada_required = 0U;

  if (result == CARDANO_SUCCESS)
  {
    min_adas = (uint64_t*)_cardano_malloc(map_count * sizeof(uint64_t));

    if (min_adas == NULL)
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  const uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);
  const uint64_t ada_available     = (excess_coin > 0) ? (uint64_t)excess_coin : 0U;

  if (result == CARDANO_SUCCESS)
  {
    for (size_t m = 0U; (m < map_count) && (result == CARDANO_SUCCESS); ++m)
    {
      result = min_ada_for_change_map(&table, quantities, map_count, map_order[m], change_address, ada_per_utxo_byte, &min_adas[m]);

      if (result == CARDANO_SUCCESS)
      {
        ada_required += min_adas[m];
      }
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    bool exact_match = (excess_coin == 0);

    for (size_t a = 0U; (a < asset_count) && exact_match; ++a)
    {
      for (size_t m = 0U; (m < map_count) && exact_match; ++m)
      {
        exact_match = (quantities[(a * map_count) + m] == 0U);
      }
    }

    if (exact_match)
    {
      result = cardano_transaction_output_list_new(change_outputs);

      asset_table_free(&table);
      _cardano_free(quantities);
      _cardano_free(scratch);
      _cardano_free(out_weights);
      _cardano_free(map_order);
      _cardano_free(min_adas);

      return result;
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    while (ada_available < ada_required)
    {
      bool front_is_empty = (first_map < (map_count - 1U));

      for (size_t a = 0U; (a < asset_count) && front_is_empty; ++a)
      {
        front_is_empty = (quantities[(a * map_count) + map_order[first_map]] == 0U);
      }

      if (!front_is_empty)
      {
        break;
      }

      ada_required -= min_adas[first_map];
      ++first_map;
    }

    if (ada_available < ada_required)
    {
      result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  // Distribute the remaining ada proportionally to the original output coins.
  if (result == CARDANO_SUCCESS)
  {
    const size_t   remaining_maps = map_count - first_map;
    const uint64_t ada_remaining  = ada_available - ada_required;

    uint64_t* weights = (uint64_t*)_cardano_malloc(remaining_maps * sizeof(uint64_t));
    uint64_t* shares  = (uint64_t*)_cardano_malloc(remaining_maps * sizeof(uint64_t));

    if ((weights == NULL) || (shares == NULL))
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    else
    {
      uint64_t weight_total = 0U;

      for (size_t m = 0U; m < remaining_maps; ++m)
      {
        weights[m]   = out_weights[map_order[first_map + m]];
        weight_total += weights[m];
      }

      if (weight_total == 0U)
      {
        CARDANO_UNUSED(memset(shares, 0, remaining_maps * sizeof(uint64_t)));
        shares[remaining_maps - 1U] = ada_remaining;
      }
      else
      {
        result = _cardano_random_improve_partition(ada_remaining, weights, remaining_maps, shares);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_transaction_output_list_new(change_outputs);
      }

      for (size_t m = 0U; (m < remaining_maps) && (result == CARDANO_SUCCESS); ++m)
      {
        const size_t map_index = map_order[first_map + m];

        cardano_value_t* value = cardano_value_new_zero();

        if (value == NULL)
        {
          result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
          break;
        }

        const uint64_t bundle_coin = min_adas[m + first_map] + shares[m];

        result = cardano_value_set_coin(value, (int64_t)bundle_coin);

        for (size_t a = 0U; (a < asset_count) && (result == CARDANO_SUCCESS); ++a)
        {
          const uint64_t quantity = quantities[(a * map_count) + map_index];

          if (quantity > 0U)
          {
            result = cardano_value_add_asset_with_id(value, table.ids[a], (int64_t)quantity);
          }
        }

        cardano_transaction_output_t* output = NULL;

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_transaction_output_new(change_address, 0, &output);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_transaction_output_set_value(output, value);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_transaction_output_list_add(*change_outputs, output);
        }

        cardano_transaction_output_unref(&output);
        cardano_value_unref(&value);
      }

      if (result == CARDANO_SUCCESS)
      {
        *change_count_out = remaining_maps;
      }
    }

    _cardano_free(weights);
    _cardano_free(shares);
  }

  if ((result != CARDANO_SUCCESS) && (*change_outputs != NULL))
  {
    cardano_transaction_output_list_unref(change_outputs);
  }

  asset_table_free(&table);
  _cardano_free(quantities);
  _cardano_free(scratch);
  _cardano_free(out_weights);
  _cardano_free(map_order);
  _cardano_free(min_adas);

  return result;
}

/* SELECT ********************************************************************/

/**
 * \brief Selects UTXOs using the Round-Robin Random-Improve algorithm and produces
 * min-ADA compliant change outputs that mimic the shape of the user-specified outputs.
 *
 * \param[in] coin_selector A pointer to the coin selector implementation object.
 * \param[in] request The selection request. The request's outputs_to_cover hint is used to size the change outputs.
 * \param[out] selection A pointer to the list of selected UTXOs that meet the target value.
 * \param[out] remaining_utxo A pointer to the list of UTXOs that were not selected.
 * \param[out] change_outputs A pointer to the list of change outputs produced by the selection.
 *
 * \return \ref CARDANO_SUCCESS if UTXOs were successfully selected, or an appropriate error code.
 */
static cardano_error_t
random_improve_select(
  cardano_coin_selector_impl_t*           coin_selector,
  const cardano_coin_selection_request_t* request,
  cardano_utxo_list_t**                   selection,
  cardano_utxo_list_t**                   remaining_utxo,
  cardano_transaction_output_list_t**     change_outputs)
{
  if ((coin_selector == NULL) || (request == NULL) || (selection == NULL) || (remaining_utxo == NULL) || (change_outputs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_utxo_list_t*               pre_selected_utxo = request->pre_selected_utxo;
  cardano_utxo_list_t*               available_utxo    = request->available_utxo;
  cardano_value_t*                   target            = request->target;
  cardano_transaction_output_list_t* outputs_to_cover  = request->outputs_to_cover;
  cardano_address_t*                 change_address    = request->change_address;
  cardano_protocol_parameters_t*     protocol_params   = request->protocol_params;

  if ((available_utxo == NULL) || (target == NULL) || (change_address == NULL) || (protocol_params == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  random_improve_context_t* context = (random_improve_context_t*)((void*)coin_selector->context);

  if (context == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  // The RNG is re-seeded on every call so that repeated invocations from the balancing loop are
  // reproducible: each fee-convergence iteration sees the same random choices for the same inputs,
  // allowing the selection to evolve progressively as the target grows instead of being reshuffled.
  uint64_t rng_state = context->seed;

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

  const size_t pre_selected_count = (pre_selected_utxo != NULL) ? cardano_utxo_list_get_length(pre_selected_utxo) : 0U;

  for (size_t i = 0U; (i < pre_selected_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(pre_selected_utxo, i, &utxo);

    if (result == CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);

      result = cardano_utxo_list_add(*selection, utxo);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_utxo_list_remove(*remaining_utxo, utxo);
      }
    }
  }

  // Build the list of requirements (one processor per required asset, plus lovelace last).
  asset_table_t required_assets = { NULL, 0U, 0U };

  if (result == CARDANO_SUCCESS)
  {
    result = asset_table_add_value_assets(&required_assets, target);
  }

  selection_processor_t* processors = NULL;

  if (result == CARDANO_SUCCESS)
  {
    processors = (selection_processor_t*)_cardano_malloc((required_assets.size + 1U) * sizeof(selection_processor_t));

    if (processors == NULL)
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    size_t processor_count = 0U;

    for (size_t a = 0U; a < required_assets.size; ++a)
    {
      const int64_t required = value_get_asset_quantity(target, required_assets.ids[a]);

      if (required > 0)
      {
        processors[processor_count].asset_id = required_assets.ids[a];
        processors[processor_count].minimum  = (uint64_t)required;
        processors[processor_count].active   = true;
        ++processor_count;
      }
    }

    const int64_t target_coin = cardano_value_get_coin(target);

    processors[processor_count].asset_id = NULL;
    processors[processor_count].minimum  = (target_coin > 0) ? (uint64_t)target_coin : 0U;
    processors[processor_count].active   = true;
    ++processor_count;

    // Round-robin processing: keep cycling while at least one processor makes progress.
    bool progress = true;

    while (progress)
    {
      progress = false;

      for (size_t p = 0U; p < processor_count; ++p)
      {
        if (!processors[p].active)
        {
          continue;
        }

        if (run_selection_step(&rng_state, *selection, *remaining_utxo, &processors[p]))
        {
          progress = true;
        }
        else
        {
          processors[p].active = false;
        }
      }
    }

    // Coverage check: every requirement must now be met.
    for (size_t p = 0U; (p < processor_count) && (result == CARDANO_SUCCESS); ++p)
    {
      if (selected_quantity(*selection, processors[p].asset_id) < processors[p].minimum)
      {
        result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
      }
    }

    // A selection must never be empty.
    if ((result == CARDANO_SUCCESS) && (cardano_utxo_list_get_length(*selection) == 0U))
    {
      if (!select_random_with_priority(&rng_state, *selection, *remaining_utxo, NULL))
      {
        result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
      }
    }
  }

  const size_t output_count      = (outputs_to_cover != NULL) ? cardano_transaction_output_list_get_length(outputs_to_cover) : 0U;
  const size_t desired_map_count = (output_count > 0U) ? output_count : 1U;

  // Change generation: retry with one additional ada UTXO while change cannot be constructed,
  // or while the number of change outputs is fewer than desired.
  bool change_done = false;

  while ((result == CARDANO_SUCCESS) && !change_done)
  {
    size_t change_count = 0U;

    const cardano_error_t change_result = make_change(
      *selection,
      target,
      outputs_to_cover,
      change_address,
      protocol_params,
      change_outputs,
      &change_count);

    if (change_result == CARDANO_SUCCESS)
    {
      if ((change_count >= desired_map_count) || !select_random_with_priority(&rng_state, *selection, *remaining_utxo, NULL))
      {
        change_done = true;
      }
      else
      {
        cardano_transaction_output_list_unref(change_outputs);
      }
    }
    else if (change_result == CARDANO_ERROR_BALANCE_INSUFFICIENT)
    {
      if (!select_random_with_priority(&rng_state, *selection, *remaining_utxo, NULL))
      {
        result = change_result;
      }
    }
    else
    {
      result = change_result;
    }
  }

  _cardano_free(processors);
  asset_table_free(&required_assets);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(selection);
    cardano_utxo_list_unref(remaining_utxo);

    if (*change_outputs != NULL)
    {
      cardano_transaction_output_list_unref(change_outputs);
    }

    cardano_coin_selector_impl_t* impl = coin_selector;

    cardano_safe_memcpy(
      impl->error_message,
      1024U,
      "Random improve selection failed: insufficient balance to cover the target and min-ADA compliant change.",
      cardano_safe_strlen("Random improve selection failed: insufficient balance to cover the target and min-ADA compliant change.", 1023U));
  }

  return result;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_random_improve_coin_selector_new_with_seed(
  const uint64_t            seed,
  cardano_coin_selector_t** coin_selector)
{
  if (coin_selector == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  random_improve_context_t* context = (random_improve_context_t*)_cardano_malloc(sizeof(random_improve_context_t));

  if (context == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  context->base.ref_count     = 1;
  context->base.last_error[0] = '\0';
  context->base.deallocator   = _cardano_free;
  context->seed               = seed;

  static const char* selector_name = "Random improve coin selector";

  cardano_coin_selector_impl_t impl = { 0 };

  cardano_safe_memcpy(impl.name, 256U, selector_name, cardano_safe_strlen(selector_name, 256U));

  impl.select = random_improve_select;
  // cppcheck-suppress misra-c2012-11.3; Reason: The context embeds cardano_object_t as its first member.
  impl.context = (cardano_object_t*)((void*)context);

  cardano_error_t result = cardano_coin_selector_new(impl, coin_selector);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(context);

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_random_improve_coin_selector_new(cardano_coin_selector_t** coin_selector)
{
  uint64_t seed = 0U;

  randombytes_buf(&seed, sizeof(seed));

  return cardano_random_improve_coin_selector_new_with_seed(seed, coin_selector);
}
