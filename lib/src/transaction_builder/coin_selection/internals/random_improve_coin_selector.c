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
 * \brief Context for the random improve coin selector, holding the RNG seed.
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
 * \brief Working table of every distinct asset involved in a selection.
 */
typedef struct asset_table_t
{
    cardano_asset_id_t** ids;
    size_t               size;
    size_t               capacity;
} asset_table_t;

/**
 * \brief Working state for the change generation phase (port of makeChange).
 *
 * The `quantities` buffer is an asset-major matrix of `asset_count` rows by `map_count` columns:
 * entry `(a, m)` holds the quantity of asset `a` assigned to change map `m`.
 */
typedef struct change_state_t
{
    asset_table_t table;
    size_t        map_count;
    size_t        output_count;
    size_t        selection_size;
    uint64_t*     quantities;
    uint64_t*     scratch;
    uint64_t*     out_weights;
    size_t*       map_order;
    uint64_t*     min_adas;
    int64_t       excess_coin;
    bool          excess_valid;
} change_state_t;

/* UTXO AND VALUE UTILITIES **************************************************/

/**
 * \brief Returns the quantity of the given asset in the given value (0 if absent).
 *
 * \param[in] value    The value to inspect.
 * \param[in] asset_id The asset to look up.
 *
 * \return The quantity of the asset, or 0 if the value does not contain the asset.
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
 *
 * \param[in] utxo The UTXO whose output value to borrow.
 *
 * \return A borrowed pointer to the value; valid while the UTXO is alive.
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

/**
 * \brief Indicates whether two asset ids identify the same asset.
 *
 * \param[in] lhs The first asset id.
 * \param[in] rhs The second asset id.
 *
 * \return true if both ids refer to the same policy and asset name, false otherwise.
 */
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
  cardano_value_t* value = utxo_borrow_value(utxo);

  if (asset_id == NULL)
  {
    return cardano_value_get_coin(value) > 0;
  }

  return value_get_asset_quantity(value, asset_id) > 0;
}

/**
 * \brief Returns the total selected quantity of the given asset (NULL asset id = lovelace).
 *
 * \param[in] selection The list of selected UTXOs.
 * \param[in] asset_id  The asset to total, or NULL for lovelace.
 *
 * \return The total quantity of the asset across all selected UTXOs.
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

    int64_t quantity = 0;

    if (asset_id == NULL)
    {
      quantity = cardano_value_get_coin(value);
    }
    else
    {
      quantity = value_get_asset_quantity(value, asset_id);
    }

    if (quantity > 0)
    {
      total += (uint64_t)quantity;
    }
  }

  return total;
}

/* RANDOM SELECTION **********************************************************/

/**
 * \brief Selects a random UTXO containing the given asset from the leftover list and moves it
 * into the selection, giving priority to UTXOs with fewer assets.
 *
 * Priority tiers (port of SelectSingleton / SelectPairWith / SelectAnyWith): UTXOs whose total
 * asset count (lovelace included) is 1, then 2, then any. A NULL asset id selects by lovelace.
 *
 * \param[in,out] rng_state The random number generator state.
 * \param[in,out] selection The selection list to which the chosen UTXO is added.
 * \param[in,out] leftover  The pool from which the UTXO is removed.
 * \param[in]     asset_id  The asset the UTXO must contain, or NULL for lovelace.
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

/**
 * \brief Moves the most recently selected UTXO back into the leftover pool.
 *
 * Used by the improvement step to undo a selection that did not move the selected quantity
 * closer to its target.
 *
 * \param[in,out] selection The selection list from which the last UTXO is removed.
 * \param[in,out] leftover  The pool to which the UTXO is returned.
 */
static void
undo_last_selection(cardano_utxo_list_t* selection, cardano_utxo_list_t* leftover)
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

/**
 * \brief Runs a single selection step for the given processor (port of runSelectionStep).
 *
 * While the selected quantity is below the minimum, any successful selection is accepted.
 * Once at or above the minimum, a selection is accepted only if it moves the selected quantity
 * strictly closer to twice the minimum; otherwise it is undone.
 *
 * \param[in,out] rng_state The random number generator state.
 * \param[in,out] selection The current selection.
 * \param[in,out] leftover  The pool of unselected UTXOs.
 * \param[in]     processor The requirement being processed.
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

  if (_cardano_random_improve_distance(target, updated) < _cardano_random_improve_distance(target, current))
  {
    return true;
  }

  undo_last_selection(selection, leftover);

  return false;
}

/* ASSET TABLE ***************************************************************/

/**
 * \brief Releases the asset ids held by the table and frees its storage.
 *
 * \param[in,out] table The table to free. Reset to an empty table.
 */
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

/**
 * \brief Adds an asset id to the table if it is not already present.
 *
 * The table takes a reference on the added asset id. The storage grows geometrically as needed.
 *
 * \param[in,out] table    The table to add to.
 * \param[in]     asset_id The asset id to add.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
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
 *
 * \param[in,out] table The table to add to.
 * \param[in]     value The value whose assets to add.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
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

/* SELECTION PHASE ***********************************************************/

/**
 * \brief Moves every pre-selected UTXO into the selection and out of the leftover pool.
 *
 * \param[in]     pre_selected_utxo The pre-selected UTXOs, or NULL.
 * \param[in,out] selection         The selection list to add to.
 * \param[in,out] leftover          The pool from which the UTXOs are removed.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
move_pre_selected_into_selection(
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

/**
 * \brief Builds the round-robin processors for the given target: one per required asset,
 * plus one for lovelace, which is placed last.
 *
 * Running the lovelace processor last increases the probability that it can terminate without
 * selecting an additional UTXO, since every asset selection also brings in ada.
 *
 * \param[in]  target          The target value to cover.
 * \param[in]  required_assets The table of assets present in the target.
 * \param[out] processors      The resulting processor array, sized `required_assets->size + 1`.
 * \param[out] processor_count The number of processors written.
 */
static void
build_selection_processors(
  cardano_value_t*       target,
  asset_table_t*         required_assets,
  selection_processor_t* processors,
  size_t*                processor_count)
{
  size_t count = 0U;

  for (size_t a = 0U; a < required_assets->size; ++a)
  {
    const int64_t required = value_get_asset_quantity(target, required_assets->ids[a]);

    if (required > 0)
    {
      processors[count].asset_id = required_assets->ids[a];
      processors[count].minimum  = (uint64_t)required;
      processors[count].active   = true;
      ++count;
    }
  }

  const int64_t target_coin = cardano_value_get_coin(target);

  processors[count].asset_id = NULL;
  processors[count].minimum  = (target_coin > 0) ? (uint64_t)target_coin : 0U;
  processors[count].active   = true;
  ++count;

  *processor_count = count;
}

/**
 * \brief Runs the processors round-robin until none of them can improve the selection.
 *
 * Each active processor performs one selection step per cycle. A processor that fails to improve
 * the selection is deactivated; the loop terminates when a full cycle makes no progress.
 *
 * \param[in,out] rng_state       The random number generator state.
 * \param[in,out] selection       The current selection.
 * \param[in,out] leftover        The pool of unselected UTXOs.
 * \param[in,out] processors      The processors to run.
 * \param[in]     processor_count The number of processors.
 */
static void
run_round_robin(
  uint64_t*              rng_state,
  cardano_utxo_list_t*   selection,
  cardano_utxo_list_t*   leftover,
  selection_processor_t* processors,
  const size_t           processor_count)
{
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

      if (run_selection_step(rng_state, selection, leftover, &processors[p]))
      {
        progress = true;
      }
      else
      {
        processors[p].active = false;
      }
    }
  }
}

/**
 * \brief Indicates whether the selection covers the minimum quantity of every processor.
 *
 * \param[in] selection       The current selection.
 * \param[in] processors      The processors holding the minimum requirements.
 * \param[in] processor_count The number of processors.
 *
 * \return true if every minimum is covered, false otherwise.
 */
static bool
all_minimums_covered(
  cardano_utxo_list_t*   selection,
  selection_processor_t* processors,
  const size_t           processor_count)
{
  for (size_t p = 0U; p < processor_count; ++p)
  {
    if (selected_quantity(selection, processors[p].asset_id) < processors[p].minimum)
    {
      return false;
    }
  }

  return true;
}

/**
 * \brief Runs the selection phase: covers the target via round-robin random-improve selection.
 *
 * \param[in,out] rng_state The random number generator state.
 * \param[in]     target    The target value to cover.
 * \param[in,out] selection The selection list, already seeded with any pre-selected UTXOs.
 * \param[in,out] leftover  The pool of unselected UTXOs.
 *
 * \return \ref CARDANO_SUCCESS if the target is covered and the selection is not empty, or
 *         \ref CARDANO_ERROR_BALANCE_INSUFFICIENT otherwise.
 */
static cardano_error_t
run_selection_phase(
  uint64_t*            rng_state,
  cardano_value_t*     target,
  cardano_utxo_list_t* selection,
  cardano_utxo_list_t* leftover)
{
  asset_table_t required_assets = { NULL, 0U, 0U };

  cardano_error_t result = asset_table_add_value_assets(&required_assets, target);

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

    build_selection_processors(target, &required_assets, processors, &processor_count);
    run_round_robin(rng_state, selection, leftover, processors, processor_count);

    if (!all_minimums_covered(selection, processors, processor_count))
    {
      result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  if ((result == CARDANO_SUCCESS) && (cardano_utxo_list_get_length(selection) == 0U))
  {
    if (!select_random_with_priority(rng_state, selection, leftover, NULL))
    {
      result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  _cardano_free(processors);
  asset_table_free(&required_assets);

  return result;
}

/* CHANGE GENERATION *********************************************************/

/**
 * \brief Frees all buffers held by the change state.
 *
 * \param[in,out] state The state to free.
 */
static void
change_state_free(change_state_t* state)
{
  asset_table_free(&state->table);

  _cardano_free(state->quantities);
  _cardano_free(state->scratch);
  _cardano_free(state->out_weights);
  _cardano_free(state->map_order);
  _cardano_free(state->min_adas);

  state->quantities  = NULL;
  state->scratch     = NULL;
  state->out_weights = NULL;
  state->map_order   = NULL;
  state->min_adas    = NULL;
}

/**
 * \brief Collects the original output coins to be used as weights for the ada distribution.
 *
 * When no outputs were provided, a single change map with a unit weight is used.
 *
 * \param[in,out] state            The change state to fill.
 * \param[in]     outputs_to_cover The user-specified outputs, or NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
collect_output_coin_weights(change_state_t* state, cardano_transaction_output_list_t* outputs_to_cover)
{
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t m = 0U; (m < state->map_count) && (result == CARDANO_SUCCESS); ++m)
  {
    state->map_order[m]   = m;
    state->out_weights[m] = 1U;

    if (state->output_count > 0U)
    {
      cardano_transaction_output_t* output = NULL;

      result = cardano_transaction_output_list_get(outputs_to_cover, m, &output);

      if (result == CARDANO_SUCCESS)
      {
        cardano_value_t* value = cardano_transaction_output_get_value(output);

        const int64_t coin = cardano_value_get_coin(value);

        if (coin > 0)
        {
          state->out_weights[m] = (uint64_t)coin;
        }
        else
        {
          state->out_weights[m] = 0U;
        }

        cardano_value_unref(&value);
        cardano_transaction_output_unref(&output);
      }
    }
  }

  return result;
}

/**
 * \brief Initializes the change state: allocates the working buffers, collects the involved
 * assets and computes the excess coin of the selection over the target.
 *
 * \param[out] state            The state to initialize.
 * \param[in]  selection        The selected UTXOs.
 * \param[in]  target           The target value.
 * \param[in]  outputs_to_cover The user-specified outputs, or NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code. On failure the state
 *         is left in a state that is safe to pass to \ref change_state_free.
 */
static cardano_error_t
change_state_init(
  change_state_t*                    state,
  cardano_utxo_list_t*               selection,
  cardano_value_t*                   target,
  cardano_transaction_output_list_t* outputs_to_cover)
{
  CARDANO_UNUSED(memset(state, 0, sizeof(*state)));

  state->output_count   = (outputs_to_cover != NULL) ? cardano_transaction_output_list_get_length(outputs_to_cover) : 0U;
  state->map_count      = (state->output_count > 0U) ? state->output_count : 1U;
  state->selection_size = cardano_utxo_list_get_length(selection);
  state->excess_valid   = true;

  cardano_error_t result = asset_table_add_value_assets(&state->table, target);

  for (size_t i = 0U; (i < state->selection_size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(selection, i, &utxo);

    if (result == CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);
      result = asset_table_add_value_assets(&state->table, utxo_borrow_value(utxo));
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    const size_t scratch_size = (state->selection_size > state->map_count) ? state->selection_size : state->map_count;

    if (state->table.size > 0U)
    {
      state->quantities = (uint64_t*)_cardano_malloc(state->table.size * state->map_count * sizeof(uint64_t));
    }

    state->scratch     = (uint64_t*)_cardano_malloc(scratch_size * sizeof(uint64_t));
    state->out_weights = (uint64_t*)_cardano_malloc(state->map_count * sizeof(uint64_t));
    state->map_order   = (size_t*)_cardano_malloc(state->map_count * sizeof(size_t));
    state->min_adas    = (uint64_t*)_cardano_malloc(state->map_count * sizeof(uint64_t));

    const bool quantities_missing = (state->table.size > 0U) && (state->quantities == NULL);

    if (quantities_missing || (state->scratch == NULL) || (state->out_weights == NULL) || (state->map_order == NULL) || (state->min_adas == NULL))
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    else if (state->quantities != NULL)
    {
      CARDANO_UNUSED(memset(state->quantities, 0, state->table.size * state->map_count * sizeof(uint64_t)));
    }
    else
    {
      /* No assets involved in this selection; only ada change will be generated. */
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    int64_t selected_coin = 0;

    for (size_t i = 0U; (i < state->selection_size) && (result == CARDANO_SUCCESS); ++i)
    {
      cardano_utxo_t* utxo = NULL;

      result = cardano_utxo_list_get(selection, i, &utxo);

      if (result == CARDANO_SUCCESS)
      {
        cardano_utxo_unref(&utxo);
        selected_coin += cardano_value_get_coin(utxo_borrow_value(utxo));
      }
    }

    state->excess_coin = selected_coin - cardano_value_get_coin(target);

    if (state->excess_coin < 0)
    {
      state->excess_valid = false;
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = collect_output_coin_weights(state, outputs_to_cover);
  }

  return result;
}

/**
 * \brief Collects the per-input quantities of the given asset into the state's scratch buffer.
 *
 * \param[in,out] state          The change state (its scratch buffer receives the quantities).
 * \param[in]     selection      The selected UTXOs.
 * \param[in]     asset_id       The asset to collect.
 * \param[out]    selected_total The total selected quantity of the asset.
 *
 * \return The number of inputs holding a positive quantity of the asset.
 */
static size_t
collect_input_asset_quantities(
  change_state_t*      state,
  cardano_utxo_list_t* selection,
  cardano_asset_id_t*  asset_id,
  int64_t*             selected_total)
{
  size_t count = 0U;

  *selected_total = 0;

  for (size_t i = 0U; i < state->selection_size; ++i)
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
      *selected_total       += quantity;
      state->scratch[count] = (uint64_t)quantity;
      ++count;
    }
  }

  return count;
}

/**
 * \brief Collects the per-output quantities of the given asset (the user-specified weights)
 * into the state's scratch buffer.
 *
 * \param[in,out] state            The change state (its scratch buffer receives the weights).
 * \param[in]     outputs_to_cover The user-specified outputs, or NULL.
 * \param[in]     asset_id         The asset to collect.
 *
 * \return The total user-specified quantity of the asset. A value of zero means the asset is
 *         not user-specified.
 */
static uint64_t
collect_user_specified_weights(
  change_state_t*                    state,
  cardano_transaction_output_list_t* outputs_to_cover,
  cardano_asset_id_t*                asset_id)
{
  uint64_t total = 0U;

  for (size_t m = 0U; m < state->output_count; ++m)
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

    state->scratch[m] = (quantity > 0) ? (uint64_t)quantity : 0U;
    total             += state->scratch[m];
  }

  return total;
}

/**
 * \brief Writes the distributed parts of an asset into the state's quantity matrix.
 *
 * \param[in,out] state       The change state.
 * \param[in]     asset_index The row (asset) to write.
 * \param[in]     parts       The quantities to write, one per change map.
 */
static void
store_asset_distribution(change_state_t* state, const size_t asset_index, const uint64_t* parts)
{
  for (size_t m = 0U; m < state->map_count; ++m)
  {
    state->quantities[(asset_index * state->map_count) + m] = parts[m];
  }
}

/**
 * \brief Distributes the excess of a user-specified asset proportionally to the quantities
 * of that asset in the user-specified outputs (port of makeChangeForUserSpecifiedAsset).
 *
 * The per-output weights are expected in the state's scratch buffer.
 *
 * \param[in,out] state       The change state.
 * \param[in]     asset_index The row (asset) being distributed.
 * \param[in]     excess      The excess quantity to distribute.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
distribute_user_specified_excess(change_state_t* state, const size_t asset_index, const int64_t excess)
{
  uint64_t* parts = (uint64_t*)_cardano_malloc(state->map_count * sizeof(uint64_t));

  if (parts == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = _cardano_random_improve_partition((uint64_t)excess, state->scratch, state->map_count, parts);

  if (result == CARDANO_SUCCESS)
  {
    store_asset_distribution(state, asset_index, parts);
  }

  _cardano_free(parts);

  return result;
}

/**
 * \brief Distributes the excess of a non-user-specified asset, preserving the granularity of the
 * selected inputs (port of makeChangeForNonUserSpecifiedAsset).
 *
 * The per-input quantities are expected in the state's scratch buffer, and are pad-coalesced to
 * the number of change maps. Mints (excess greater than the selected total) are added to the
 * largest map; burns (excess smaller than the selected total) are removed from the smallest maps.
 *
 * \param[in,out] state          The change state.
 * \param[in]     asset_index    The row (asset) being distributed.
 * \param[in]     excess         The excess quantity to distribute.
 * \param[in]     selected_total The total selected quantity of the asset.
 * \param[in]     input_count    The number of per-input quantities in the scratch buffer.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
distribute_non_user_specified_excess(
  change_state_t* state,
  const size_t    asset_index,
  const int64_t   excess,
  const int64_t   selected_total,
  const size_t    input_count)
{
  uint64_t* parts = (uint64_t*)_cardano_malloc(state->map_count * sizeof(uint64_t));

  if (parts == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (input_count == 0U)
  {
    CARDANO_UNUSED(memset(parts, 0, state->map_count * sizeof(uint64_t)));
  }
  else
  {
    result = _cardano_random_improve_pad_coalesce(state->scratch, input_count, state->map_count, parts);
  }

  if (result == CARDANO_SUCCESS)
  {
    if (excess < selected_total)
    {
      const int64_t burn_amount = selected_total - excess;

      _cardano_random_improve_reduce_quantities((uint64_t)burn_amount, parts, state->map_count);
    }

    if (excess > selected_total)
    {
      const int64_t mint_amount = excess - selected_total;

      parts[state->map_count - 1U] += (uint64_t)mint_amount;
    }

    store_asset_distribution(state, asset_index, parts);
  }

  _cardano_free(parts);

  return result;
}

/**
 * \brief Distributes the excess of every involved asset across the change maps.
 *
 * Sets the state's `excess_valid` flag to false if the selection does not cover the target
 * for some asset.
 *
 * \param[in,out] state            The change state.
 * \param[in]     selection        The selected UTXOs.
 * \param[in]     target           The target value.
 * \param[in]     outputs_to_cover The user-specified outputs, or NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
distribute_asset_excesses(
  change_state_t*                    state,
  cardano_utxo_list_t*               selection,
  cardano_value_t*                   target,
  cardano_transaction_output_list_t* outputs_to_cover)
{
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t a = 0U; ((a < state->table.size) && (result == CARDANO_SUCCESS)) && state->excess_valid; ++a)
  {
    cardano_asset_id_t* asset_id = state->table.ids[a];

    int64_t      selected_total = 0;
    const size_t input_count    = collect_input_asset_quantities(state, selection, asset_id, &selected_total);

    const int64_t required = value_get_asset_quantity(target, asset_id);
    const int64_t excess   = selected_total - required;

    if (excess < 0)
    {
      state->excess_valid = false;
      continue;
    }

    if (excess == 0)
    {
      continue;
    }

    const uint64_t user_weight_total = (state->output_count > 0U) ? collect_user_specified_weights(state, outputs_to_cover, asset_id) : 0U;

    if (user_weight_total > 0U)
    {
      result = distribute_user_specified_excess(state, a, excess);
    }
    else
    {
      result = distribute_non_user_specified_excess(state, a, excess, selected_total, input_count);
    }
  }

  return result;
}

/**
 * \brief Returns the number of distinct assets assigned to the given change map.
 *
 * \param[in] state     The change state.
 * \param[in] map_index The change map to inspect.
 *
 * \return The number of assets with a positive quantity in the map.
 */
static size_t
map_asset_count(change_state_t* state, const size_t map_index)
{
  size_t count = 0U;

  for (size_t a = 0U; a < state->table.size; ++a)
  {
    if (state->quantities[(a * state->map_count) + map_index] > 0U)
    {
      ++count;
    }
  }

  return count;
}

/**
 * \brief Orders the change maps by ascending asset count (stable), so empty maps come first.
 *
 * This ordering allows the coin assignment step to drop empty maps from the front of the list
 * when the available ada is insufficient (port of the AssetCount ordering used by
 * assignCoinsToChangeMaps).
 *
 * \param[in,out] state The change state; its `map_order` buffer is rearranged.
 */
static void
order_maps_by_asset_count(change_state_t* state)
{
  for (size_t i = 1U; i < state->map_count; ++i)
  {
    const size_t key        = state->map_order[i];
    const size_t key_assets = map_asset_count(state, key);

    size_t j        = i;
    bool   in_place = false;

    while ((j > 0U) && !in_place)
    {
      if (map_asset_count(state, state->map_order[j - 1U]) <= key_assets)
      {
        in_place = true;
      }
      else
      {
        state->map_order[j] = state->map_order[j - 1U];
        --j;
      }
    }

    state->map_order[j] = key;
  }
}

/**
 * \brief Computes the minimum required ada for a change output holding the given map's assets.
 *
 * \param[in]  state             The change state.
 * \param[in]  map_index         The change map to measure.
 * \param[in]  change_address    The address the change output would be sent to.
 * \param[in]  ada_per_utxo_byte The protocol parameter driving the min-ADA computation.
 * \param[out] min_ada           The minimum required ada quantity.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
min_ada_for_change_map(
  change_state_t*    state,
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

  for (size_t a = 0U; (a < state->table.size) && (result == CARDANO_SUCCESS); ++a)
  {
    const uint64_t quantity = state->quantities[(a * state->map_count) + map_index];

    if (quantity > 0U)
    {
      result = cardano_value_add_asset_with_id(value, state->table.ids[a], (int64_t)quantity);
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
 * \brief Computes the min-ADA requirement of every change map, in map order.
 *
 * \param[in,out] state             The change state; its `min_adas` buffer is filled.
 * \param[in]     change_address    The address the change outputs would be sent to.
 * \param[in]     ada_per_utxo_byte The protocol parameter driving the min-ADA computation.
 * \param[out]    ada_required      The total min-ADA requirement across all maps.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
compute_min_ada_requirements(
  change_state_t*    state,
  cardano_address_t* change_address,
  const uint64_t     ada_per_utxo_byte,
  uint64_t*          ada_required)
{
  cardano_error_t result = CARDANO_SUCCESS;

  *ada_required = 0U;

  for (size_t m = 0U; (m < state->map_count) && (result == CARDANO_SUCCESS); ++m)
  {
    result = min_ada_for_change_map(state, state->map_order[m], change_address, ada_per_utxo_byte, &state->min_adas[m]);

    if (result == CARDANO_SUCCESS)
    {
      *ada_required += state->min_adas[m];
    }
  }

  return result;
}

/**
 * \brief Indicates whether the selection matches the target exactly (no change is needed).
 *
 * \param[in] state The change state.
 *
 * \return true if the excess coin is zero and no asset quantities were distributed.
 */
static bool
change_is_exact_match(change_state_t* state)
{
  if (state->excess_coin != 0)
  {
    return false;
  }

  for (size_t a = 0U; a < state->table.size; ++a)
  {
    for (size_t m = 0U; m < state->map_count; ++m)
    {
      if (state->quantities[(a * state->map_count) + m] > 0U)
      {
        return false;
      }
    }
  }

  return true;
}

/**
 * \brief Drops empty change maps from the front of the ordered list while the available ada is
 * insufficient to fund the min-ADA requirement of every remaining map.
 *
 * At least one map is always kept (port of the drop step of assignCoinsToChangeMaps, with the
 * deviation that the last map is never dropped: the change must carry the full excess, since
 * the local balance invariant does not allow "burning" leftover ada).
 *
 * \param[in]     state         The change state.
 * \param[in]     ada_available The ada available for the change outputs.
 * \param[in,out] ada_required  The total min-ADA requirement; reduced as maps are dropped.
 * \param[out]    first_map     The index (in map order) of the first map that is kept.
 */
static void
drop_underfunded_empty_maps(
  change_state_t* state,
  const uint64_t  ada_available,
  uint64_t*       ada_required,
  size_t*         first_map)
{
  *first_map = 0U;

  bool done = false;

  while ((ada_available < *ada_required) && !done)
  {
    const bool can_drop = (*first_map < (state->map_count - 1U)) && (map_asset_count(state, state->map_order[*first_map]) == 0U);

    if (can_drop)
    {
      *ada_required -= state->min_adas[*first_map];
      ++(*first_map);
    }
    else
    {
      done = true;
    }
  }
}

/**
 * \brief Builds the value of a single change output from its map quantities and coin.
 *
 * \param[in]  state     The change state.
 * \param[in]  map_index The change map to materialize.
 * \param[in]  coin      The ada quantity to assign to the output.
 * \param[out] value     The resulting value.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
build_change_bundle_value(
  change_state_t*   state,
  const size_t      map_index,
  const uint64_t    coin,
  cardano_value_t** value)
{
  *value = cardano_value_new_zero();

  if (*value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_value_set_coin(*value, (int64_t)coin);

  for (size_t a = 0U; (a < state->table.size) && (result == CARDANO_SUCCESS); ++a)
  {
    const uint64_t quantity = state->quantities[(a * state->map_count) + map_index];

    if (quantity > 0U)
    {
      result = cardano_value_add_asset_with_id(*value, state->table.ids[a], (int64_t)quantity);
    }
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(value);
  }

  return result;
}

/**
 * \brief Emits the final change outputs: assigns each kept map its min-ADA plus a share of the
 * remaining ada, proportional to the original output coins (port of assignCoinsToChangeMaps).
 *
 * \param[in]  state          The change state.
 * \param[in]  first_map      The index (in map order) of the first map that was kept.
 * \param[in]  ada_available  The ada available for the change outputs.
 * \param[in]  ada_required   The total min-ADA requirement of the kept maps.
 * \param[in]  change_address The address the change outputs are sent to.
 * \param[out] change_outputs The resulting list of change outputs.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
emit_change_outputs(
  change_state_t*                     state,
  const size_t                        first_map,
  const uint64_t                      ada_available,
  const uint64_t                      ada_required,
  cardano_address_t*                  change_address,
  cardano_transaction_output_list_t** change_outputs)
{
  const size_t   remaining_maps = state->map_count - first_map;
  const uint64_t ada_remaining  = ada_available - ada_required;

  uint64_t* weights = (uint64_t*)_cardano_malloc(remaining_maps * sizeof(uint64_t));
  uint64_t* shares  = (uint64_t*)_cardano_malloc(remaining_maps * sizeof(uint64_t));

  cardano_error_t result = CARDANO_SUCCESS;

  if ((weights == NULL) || (shares == NULL))
  {
    result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }
  else
  {
    uint64_t weight_total = 0U;

    for (size_t m = 0U; m < remaining_maps; ++m)
    {
      weights[m]   = state->out_weights[state->map_order[first_map + m]];
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
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_output_list_new(change_outputs);
  }

  for (size_t m = 0U; (m < remaining_maps) && (result == CARDANO_SUCCESS); ++m)
  {
    const size_t   map_index   = state->map_order[first_map + m];
    const uint64_t bundle_coin = state->min_adas[m + first_map] + shares[m];

    cardano_value_t* value = NULL;

    result = build_change_bundle_value(state, map_index, bundle_coin, &value);

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

  _cardano_free(weights);
  _cardano_free(shares);

  return result;
}

/**
 * \brief Attempts to construct change outputs for the current selection (port of makeChange).
 *
 * On success, the resulting change outputs are stored in `change_outputs`. If the selected ada
 * is insufficient to make every non-empty change bundle min-ADA compliant, the function returns
 * \ref CARDANO_ERROR_BALANCE_INSUFFICIENT so that the caller can select additional ada and retry.
 *
 * \param[in]  selection        The selected UTXOs.
 * \param[in]  target           The target value.
 * \param[in]  outputs_to_cover The user-specified outputs used as a shape hint, or NULL.
 * \param[in]  change_address   The address the change outputs are sent to.
 * \param[in]  protocol_params  The protocol parameters, used for min-ADA compliance.
 * \param[out] change_outputs   The resulting list of change outputs.
 * \param[out] change_count     The number of change outputs generated (may be fewer than the
 *                              number of change maps when ada was tight and empty maps were dropped).
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
make_change(
  cardano_utxo_list_t*                selection,
  cardano_value_t*                    target,
  cardano_transaction_output_list_t*  outputs_to_cover,
  cardano_address_t*                  change_address,
  cardano_protocol_parameters_t*      protocol_params,
  cardano_transaction_output_list_t** change_outputs,
  size_t*                             change_count)
{
  *change_outputs = NULL;
  *change_count   = 0U;

  change_state_t state;

  cardano_error_t result = change_state_init(&state, selection, target, outputs_to_cover);

  if (result == CARDANO_SUCCESS)
  {
    result = distribute_asset_excesses(&state, selection, target, outputs_to_cover);
  }

  if ((result == CARDANO_SUCCESS) && !state.excess_valid)
  {
    result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  if ((result == CARDANO_SUCCESS) && change_is_exact_match(&state))
  {
    result = cardano_transaction_output_list_new(change_outputs);

    change_state_free(&state);

    return result;
  }

  uint64_t ada_required = 0U;
  size_t   first_map    = 0U;

  const uint64_t ada_available = (state.excess_coin > 0) ? (uint64_t)state.excess_coin : 0U;

  if (result == CARDANO_SUCCESS)
  {
    const uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);

    order_maps_by_asset_count(&state);

    result = compute_min_ada_requirements(&state, change_address, ada_per_utxo_byte, &ada_required);
  }

  if (result == CARDANO_SUCCESS)
  {
    drop_underfunded_empty_maps(&state, ada_available, &ada_required, &first_map);

    if (ada_available < ada_required)
    {
      result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = emit_change_outputs(&state, first_map, ada_available, ada_required, change_address, change_outputs);
  }

  if (result == CARDANO_SUCCESS)
  {
    *change_count = state.map_count - first_map;
  }

  if ((result != CARDANO_SUCCESS) && (*change_outputs != NULL))
  {
    cardano_transaction_output_list_unref(change_outputs);
  }

  change_state_free(&state);

  return result;
}

/**
 * \brief Generates change outputs, selecting one additional ada UTXO and retrying while change
 * cannot be constructed, or while the number of change outputs is fewer than desired
 * (port of makeChangeRepeatedly).
 *
 * \param[in,out] rng_state        The random number generator state.
 * \param[in,out] selection        The selected UTXOs; may grow as additional ada is selected.
 * \param[in,out] leftover         The pool of unselected UTXOs.
 * \param[in]     target           The target value.
 * \param[in]     outputs_to_cover The user-specified outputs used as a shape hint, or NULL.
 * \param[in]     change_address   The address the change outputs are sent to.
 * \param[in]     protocol_params  The protocol parameters, used for min-ADA compliance.
 * \param[out]    change_outputs   The resulting list of change outputs.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
generate_change_with_retries(
  uint64_t*                           rng_state,
  cardano_utxo_list_t*                selection,
  cardano_utxo_list_t*                leftover,
  cardano_value_t*                    target,
  cardano_transaction_output_list_t*  outputs_to_cover,
  cardano_address_t*                  change_address,
  cardano_protocol_parameters_t*      protocol_params,
  cardano_transaction_output_list_t** change_outputs)
{
  const size_t output_count      = (outputs_to_cover != NULL) ? cardano_transaction_output_list_get_length(outputs_to_cover) : 0U;
  const size_t desired_map_count = (output_count > 0U) ? output_count : 1U;

  cardano_error_t result = CARDANO_SUCCESS;
  bool            done   = false;

  while ((result == CARDANO_SUCCESS) && !done)
  {
    size_t change_count = 0U;

    const cardano_error_t change_result = make_change(
      selection,
      target,
      outputs_to_cover,
      change_address,
      protocol_params,
      change_outputs,
      &change_count);

    if (change_result == CARDANO_SUCCESS)
    {
      if ((change_count >= desired_map_count) || !select_random_with_priority(rng_state, selection, leftover, NULL))
      {
        done = true;
      }
      else
      {
        cardano_transaction_output_list_unref(change_outputs);
      }
    }
    else if (change_result == CARDANO_ERROR_BALANCE_INSUFFICIENT)
    {
      if (!select_random_with_priority(rng_state, selection, leftover, NULL))
      {
        result = change_result;
      }
    }
    else
    {
      result = change_result;
    }
  }

  return result;
}

/* SELECT ********************************************************************/

/**
 * \brief Records the standard insufficient-balance error message on the implementation.
 *
 * \param[in,out] coin_selector The coin selector implementation.
 */
static void
set_insufficient_balance_error(cardano_coin_selector_impl_t* coin_selector)
{
  static const char* message = "Random improve selection failed: insufficient balance to cover the target and min-ADA compliant change.";

  cardano_safe_memcpy(coin_selector->error_message, 1024U, message, cardano_safe_strlen(message, 1023U));
}

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

  if ((request->available_utxo == NULL) || (request->target == NULL) || (request->change_address == NULL) || (request->protocol_params == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  // cppcheck-suppress misra-c2012-11.3; Reason: The context is always created as a random_improve_context_t.
  random_improve_context_t* context = (random_improve_context_t*)((void*)coin_selector->context);

  if (context == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  // The RNG is re-seeded on every call so that repeated invocations from the balancing loop are
  // reproducible: each fee-convergence iteration sees the same random choices for the same inputs,
  // allowing the selection to evolve progressively as the target grows instead of being reshuffled.
  uint64_t rng_state = context->seed;

  cardano_utxo_list_t*               pre_selected_utxo = request->pre_selected_utxo;
  cardano_utxo_list_t*               available_utxo    = request->available_utxo;
  cardano_value_t*                   target            = request->target;
  cardano_transaction_output_list_t* outputs_to_cover  = request->outputs_to_cover;
  cardano_address_t*                 change_address    = request->change_address;
  cardano_protocol_parameters_t*     protocol_params   = request->protocol_params;

  cardano_error_t result = cardano_utxo_list_new(selection);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *change_outputs = NULL;
  *remaining_utxo = cardano_utxo_list_clone(available_utxo);

  if (*remaining_utxo == NULL)
  {
    cardano_utxo_list_unref(selection);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = move_pre_selected_into_selection(pre_selected_utxo, *selection, *remaining_utxo);

  if (result == CARDANO_SUCCESS)
  {
    result = run_selection_phase(&rng_state, target, *selection, *remaining_utxo);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = generate_change_with_retries(
      &rng_state,
      *selection,
      *remaining_utxo,
      target,
      outputs_to_cover,
      change_address,
      protocol_params,
      change_outputs);
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(selection);
    cardano_utxo_list_unref(remaining_utxo);

    if (*change_outputs != NULL)
    {
      cardano_transaction_output_list_unref(change_outputs);
    }

    set_insufficient_balance_error(coin_selector);
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
