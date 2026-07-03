/**
 * \file random_improve_change_state.c
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
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_builder/fee.h>

#include "./random_improve_change_state.h"

#include "../../../allocators.h"
#include "../../../string_safe.h"
#include "./random_improve_helpers.h"
#include "./random_improve_utxo_utils.h"
#include "./value_splitting.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

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
asset_table_add(random_improve_asset_table_t* table, cardano_asset_id_t* asset_id)
{
  for (size_t i = 0U; i < table->size; ++i)
  {
    if (_cardano_random_improve_asset_id_equals(table->ids[i], asset_id))
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
collect_output_coin_weights(random_improve_change_state_t* state, cardano_transaction_output_list_t* outputs_to_cover)
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
 * \brief Allocates the working buffers of the change state.
 *
 * \param[in,out] state The change state; its size fields must already be set.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
allocate_state_buffers(random_improve_change_state_t* state)
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
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if (state->quantities != NULL)
  {
    CARDANO_UNUSED(memset(state->quantities, 0, state->table.size * state->map_count * sizeof(uint64_t)));
  }

  return CARDANO_SUCCESS;
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
map_asset_count(random_improve_change_state_t* state, const size_t map_index)
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
  random_improve_change_state_t* state,
  const size_t                   map_index,
  cardano_address_t*             change_address,
  const uint64_t                 ada_per_utxo_byte,
  uint64_t*                      min_ada)
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
  random_improve_change_state_t* state,
  const size_t                   map_index,
  const uint64_t                 coin,
  cardano_value_t**              value)
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

/* DEFINITIONS ****************************************************************/

void
_cardano_random_improve_asset_table_free(random_improve_asset_table_t* table)
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

cardano_error_t
_cardano_random_improve_asset_table_add_value_assets(random_improve_asset_table_t* table, cardano_value_t* value)
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

cardano_error_t
_cardano_random_improve_change_state_init(
  random_improve_change_state_t*     state,
  cardano_utxo_list_t*               selection,
  cardano_value_t*                   target,
  cardano_transaction_output_list_t* outputs_to_cover)
{
  CARDANO_UNUSED(memset(state, 0, sizeof(*state)));

  state->output_count   = (outputs_to_cover != NULL) ? cardano_transaction_output_list_get_length(outputs_to_cover) : 0U;
  state->map_count      = (state->output_count > 0U) ? state->output_count : 1U;
  state->selection_size = cardano_utxo_list_get_length(selection);
  state->excess_valid   = true;

  cardano_error_t result = _cardano_random_improve_asset_table_add_value_assets(&state->table, target);

  for (size_t i = 0U; (i < state->selection_size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    result = cardano_utxo_list_get(selection, i, &utxo);

    if (result == CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);
      result = _cardano_random_improve_asset_table_add_value_assets(&state->table, _cardano_random_improve_borrow_utxo_value(utxo));
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = allocate_state_buffers(state);
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
        selected_coin += cardano_value_get_coin(_cardano_random_improve_borrow_utxo_value(utxo));
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

void
_cardano_random_improve_change_state_free(random_improve_change_state_t* state)
{
  _cardano_random_improve_asset_table_free(&state->table);

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

size_t
_cardano_random_improve_collect_input_quantities(
  random_improve_change_state_t* state,
  cardano_utxo_list_t*           selection,
  cardano_asset_id_t*            asset_id,
  int64_t*                       selected_total)
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

    const int64_t quantity = _cardano_random_improve_get_asset_quantity(_cardano_random_improve_borrow_utxo_value(utxo), asset_id);

    if (quantity > 0)
    {
      *selected_total       += quantity;
      state->scratch[count] = (uint64_t)quantity;
      ++count;
    }
  }

  return count;
}

uint64_t
_cardano_random_improve_collect_user_weights(
  random_improve_change_state_t*     state,
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

    const int64_t quantity = _cardano_random_improve_get_asset_quantity(value, asset_id);

    state->scratch[m] = (quantity > 0) ? (uint64_t)quantity : 0U;
    total             += state->scratch[m];
  }

  return total;
}

void
_cardano_random_improve_store_distribution(
  random_improve_change_state_t* state,
  const size_t                   asset_index,
  const uint64_t*                parts)
{
  for (size_t m = 0U; m < state->map_count; ++m)
  {
    state->quantities[(asset_index * state->map_count) + m] = parts[m];
  }
}

/**
 * \brief Builds an asset-only value from the quantities of a single change map.
 *
 * \param[in]  state     The change state.
 * \param[in]  map_index The change map to materialize.
 * \param[out] value     The resulting asset-only value (zero coin).
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
build_map_asset_value(random_improve_change_state_t* state, const size_t map_index, cardano_value_t** value)
{
  *value = cardano_value_new_zero();

  if (*value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

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
 * \brief Writes the assets of a part value into a column of the given quantity matrix.
 *
 * \param[in]     state        The change state (for the asset table).
 * \param[in]     part         The asset-only value holding the part's assets.
 * \param[in,out] quantities   The destination matrix, `table.size` rows by `column_count` columns.
 * \param[in]     column_count The number of columns of the destination matrix.
 * \param[in]     column       The destination column.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
write_part_to_column(
  random_improve_change_state_t* state,
  cardano_value_t*               part,
  uint64_t*                      quantities,
  const size_t                   column_count,
  const size_t                   column)
{
  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(part);

  if (assets == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  const size_t asset_count = cardano_asset_id_map_get_length(assets);

  for (size_t i = 0U; (i < asset_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_asset_id_t* asset_id = NULL;

    result = cardano_asset_id_map_get_key_at(assets, i, &asset_id);

    if (result == CARDANO_SUCCESS)
    {
      if (!cardano_asset_id_is_lovelace(asset_id))
      {
        int64_t quantity = 0;

        result = cardano_asset_id_map_get(assets, asset_id, &quantity);

        for (size_t a = 0U; (a < state->table.size) && (result == CARDANO_SUCCESS); ++a)
        {
          if (_cardano_random_improve_asset_id_equals(state->table.ids[a], asset_id))
          {
            quantities[(a * column_count) + column] = (quantity > 0) ? (uint64_t)quantity : 0U;
          }
        }
      }

      cardano_asset_id_unref(&asset_id);
    }
  }

  cardano_asset_id_map_unref(&assets);

  return result;
}

cardano_error_t
_cardano_random_improve_split_oversized_maps(
  random_improve_change_state_t* state,
  const uint64_t                 max_value_size)
{
  if ((max_value_size == 0U) || (state->table.size == 0U))
  {
    return CARDANO_SUCCESS;
  }

  const size_t original_map_count = state->map_count;

  cardano_value_part_list_t* map_parts = (cardano_value_part_list_t*)_cardano_malloc(original_map_count * sizeof(cardano_value_part_list_t));

  if (map_parts == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  CARDANO_UNUSED(memset(map_parts, 0, original_map_count * sizeof(cardano_value_part_list_t)));

  cardano_error_t result    = CARDANO_SUCCESS;
  size_t          new_count = 0U;

  for (size_t m = 0U; (m < original_map_count) && (result == CARDANO_SUCCESS); ++m)
  {
    cardano_value_t* map_value = NULL;

    result = build_map_asset_value(state, m, &map_value);

    if (result == CARDANO_SUCCESS)
    {
      result = _cardano_coin_selection_split_value_assets(map_value, max_value_size, &map_parts[m]);

      cardano_value_unref(&map_value);
    }

    if (result == CARDANO_SUCCESS)
    {
      new_count += map_parts[m].size;
    }
  }

  if ((result == CARDANO_SUCCESS) && (new_count > original_map_count))
  {
    uint64_t* new_quantities  = (uint64_t*)_cardano_malloc(state->table.size * new_count * sizeof(uint64_t));
    uint64_t* new_out_weights = (uint64_t*)_cardano_malloc(new_count * sizeof(uint64_t));
    size_t*   new_map_order   = (size_t*)_cardano_malloc(new_count * sizeof(size_t));
    uint64_t* new_min_adas    = (uint64_t*)_cardano_malloc(new_count * sizeof(uint64_t));

    if ((new_quantities == NULL) || (new_out_weights == NULL) || (new_map_order == NULL) || (new_min_adas == NULL))
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    else
    {
      CARDANO_UNUSED(memset(new_quantities, 0, state->table.size * new_count * sizeof(uint64_t)));

      size_t column = 0U;

      for (size_t m = 0U; (m < original_map_count) && (result == CARDANO_SUCCESS); ++m)
      {
        const size_t   parts  = map_parts[m].size;
        const uint64_t weight = state->out_weights[m];

        for (size_t p = 0U; (p < parts) && (result == CARDANO_SUCCESS); ++p)
        {
          result = write_part_to_column(state, map_parts[m].items[p], new_quantities, new_count, column);

          if (result == CARDANO_SUCCESS)
          {
            uint64_t part_weight = weight / (uint64_t)parts;

            if ((uint64_t)p < (weight % (uint64_t)parts))
            {
              part_weight += 1U;
            }

            new_out_weights[column] = part_weight;
            new_map_order[column]   = column;

            ++column;
          }
        }
      }

      if (result == CARDANO_SUCCESS)
      {
        _cardano_free(state->quantities);
        _cardano_free(state->out_weights);
        _cardano_free(state->map_order);
        _cardano_free(state->min_adas);

        state->quantities  = new_quantities;
        state->out_weights = new_out_weights;
        state->map_order   = new_map_order;
        state->min_adas    = new_min_adas;
        state->map_count   = new_count;
      }
    }

    if (result != CARDANO_SUCCESS)
    {
      _cardano_free(new_quantities);
      _cardano_free(new_out_weights);
      _cardano_free(new_map_order);
      _cardano_free(new_min_adas);
    }
  }

  for (size_t m = 0U; m < original_map_count; ++m)
  {
    _cardano_coin_selection_value_parts_free(&map_parts[m]);
  }

  _cardano_free(map_parts);

  return result;
}

void
_cardano_random_improve_order_maps_by_asset_count(random_improve_change_state_t* state)
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

cardano_error_t
_cardano_random_improve_compute_min_ada_requirements(
  random_improve_change_state_t* state,
  cardano_address_t*             change_address,
  const uint64_t                 ada_per_utxo_byte,
  uint64_t*                      ada_required)
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

bool
_cardano_random_improve_change_is_exact_match(random_improve_change_state_t* state)
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

void
_cardano_random_improve_drop_underfunded_empty_maps(
  random_improve_change_state_t* state,
  const uint64_t                 ada_available,
  uint64_t*                      ada_required,
  size_t*                        first_map)
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

cardano_error_t
_cardano_random_improve_emit_change_outputs(
  random_improve_change_state_t*      state,
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
