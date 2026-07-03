/**
 * \file random_improve_selection_index.c
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

#include "./random_improve_selection_index.h"

#include "../../../allocators.h"
#include "./random_improve_change_state.h"
#include "./random_improve_helpers.h"
#include "./random_improve_utxo_utils.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Appends a pool entry index to a bucket, growing its storage as needed.
 *
 * \param[in,out] bucket The bucket to append to.
 * \param[in]     entry  The pool entry index to append.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
bucket_append(selection_index_bucket_t* bucket, const size_t entry)
{
  if (bucket->size == bucket->capacity)
  {
    const size_t new_capacity = (bucket->capacity == 0U) ? 8U : (bucket->capacity * 2U);
    size_t*      new_items    = (size_t*)_cardano_malloc(new_capacity * sizeof(size_t));

    if (new_items == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    for (size_t i = 0U; i < bucket->size; ++i)
    {
      new_items[i] = bucket->items[i];
    }

    _cardano_free(bucket->items);

    bucket->items    = new_items;
    bucket->capacity = new_capacity;
  }

  bucket->items[bucket->size] = entry;
  ++bucket->size;

  return CARDANO_SUCCESS;
}

/**
 * \brief Records a pick on the undo stack, growing its storage as needed.
 *
 * \param[in,out] index           The selection index.
 * \param[in]     pool_index      The picked pool entry.
 * \param[in]     processor_index The processor the pick was made for.
 * \param[in]     tier            The bucket tier the entry was picked from.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
record_pick(
  random_improve_selection_index_t* index,
  const size_t                      pool_index,
  const size_t                      processor_index,
  const size_t                      tier)
{
  if (index->pick_count == index->pick_capacity)
  {
    const size_t            new_capacity = (index->pick_capacity == 0U) ? 8U : (index->pick_capacity * 2U);
    selection_index_pick_t* new_picks    = (selection_index_pick_t*)_cardano_malloc(new_capacity * sizeof(selection_index_pick_t));

    if (new_picks == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    for (size_t i = 0U; i < index->pick_count; ++i)
    {
      new_picks[i] = index->picks[i];
    }

    _cardano_free(index->picks);

    index->picks         = new_picks;
    index->pick_capacity = new_capacity;
  }

  index->picks[index->pick_count].pool_index      = pool_index;
  index->picks[index->pick_count].processor_index = processor_index;
  index->picks[index->pick_count].tier            = tier;

  ++index->pick_count;

  return CARDANO_SUCCESS;
}

/**
 * \brief Returns the cached quantity of a processor's asset held by a pool entry.
 *
 * \param[in] index           The selection index.
 * \param[in] pool_index      The pool entry.
 * \param[in] processor_index The processor whose asset quantity to read.
 *
 * \return The cached quantity.
 */
static int64_t
pool_quantity(
  const random_improve_selection_index_t* index,
  const size_t                            pool_index,
  const size_t                            processor_index)
{
  return index->pool_quantities[(pool_index * index->processor_count) + processor_index];
}

/**
 * \brief Applies a pick or an undo to every processor's running total.
 *
 * \param[in,out] index      The selection index.
 * \param[in]     pool_index The pool entry being selected or deselected.
 * \param[in]     selected   True when selecting the entry, false when deselecting it.
 */
static void
apply_to_totals(random_improve_selection_index_t* index, const size_t pool_index, const bool selected)
{
  for (size_t p = 0U; p < index->processor_count; ++p)
  {
    const int64_t quantity = pool_quantity(index, pool_index, p);

    if (quantity > 0)
    {
      if (selected)
      {
        index->processors[p].selected_total += (uint64_t)quantity;
      }
      else
      {
        index->processors[p].selected_total -= (uint64_t)quantity;
      }
    }
  }
}

/**
 * \brief Caches the asset count and per-processor quantities of a pool entry, and seeds the
 * buckets of every processor whose asset the entry holds.
 *
 * \param[in,out] index      The selection index.
 * \param[in]     pool_index The pool entry to register.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
register_pool_entry(random_improve_selection_index_t* index, const size_t pool_index)
{
  cardano_utxo_t*  utxo  = index->pool[pool_index];
  cardano_value_t* value = _cardano_random_improve_borrow_utxo_value(utxo);

  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(value);

  if (assets == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const size_t asset_count = cardano_asset_id_map_get_length(assets);

  cardano_asset_id_map_unref(&assets);

  index->pool_asset_counts[pool_index] = asset_count;

  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t p = 0U; (p < index->processor_count) && (result == CARDANO_SUCCESS); ++p)
  {
    int64_t quantity = 0;

    if (index->processors[p].asset_id == NULL)
    {
      quantity = cardano_value_get_coin(value);
    }
    else
    {
      quantity = _cardano_random_improve_get_asset_quantity(value, index->processors[p].asset_id);
    }

    index->pool_quantities[(pool_index * index->processor_count) + p] = quantity;

    if (quantity > 0)
    {
      size_t tier = 2U;

      if (asset_count == 1U)
      {
        tier = 0U;
      }
      else if (asset_count == 2U)
      {
        tier = 1U;
      }
      else
      {
        tier = 2U;
      }

      result = bucket_append(&index->processors[p].buckets[tier], pool_index);

      if ((result == CARDANO_SUCCESS) && (tier != 2U))
      {
        // Every entry is also a candidate of the lowest-priority tier, so that picks can fall
        // back to it when the preferred tiers are exhausted.
        result = bucket_append(&index->processors[p].buckets[2], pool_index);
      }
    }
  }

  return result;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
_cardano_random_improve_index_init(
  random_improve_selection_index_t* index,
  cardano_utxo_list_t*              pre_selected_utxo,
  cardano_utxo_list_t*              available_utxo,
  cardano_value_t*                  target)
{
  CARDANO_UNUSED(memset(index, 0, sizeof(*index)));

  const size_t pre_count       = (pre_selected_utxo != NULL) ? cardano_utxo_list_get_length(pre_selected_utxo) : 0U;
  const size_t available_count = cardano_utxo_list_get_length(available_utxo);

  index->pool_size          = pre_count + available_count;
  index->pre_selected_count = pre_count;

  // Build the processors: one per asset required by the target, plus lovelace last.
  random_improve_asset_table_t required_assets = { NULL, 0U, 0U };

  cardano_error_t result = _cardano_random_improve_asset_table_add_value_assets(&required_assets, target);

  if (result == CARDANO_SUCCESS)
  {
    index->processors = (selection_index_processor_t*)_cardano_malloc((required_assets.size + 1U) * sizeof(selection_index_processor_t));

    if (index->processors == NULL)
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    else
    {
      CARDANO_UNUSED(memset(index->processors, 0, (required_assets.size + 1U) * sizeof(selection_index_processor_t)));

      for (size_t a = 0U; a < required_assets.size; ++a)
      {
        const int64_t required = _cardano_random_improve_get_asset_quantity(target, required_assets.ids[a]);

        if (required > 0)
        {
          cardano_asset_id_ref(required_assets.ids[a]);

          index->processors[index->processor_count].asset_id = required_assets.ids[a];
          index->processors[index->processor_count].minimum  = (uint64_t)required;
          index->processors[index->processor_count].active   = true;

          ++index->processor_count;
        }
      }

      const int64_t target_coin = cardano_value_get_coin(target);

      index->processors[index->processor_count].asset_id = NULL;
      index->processors[index->processor_count].minimum  = (target_coin > 0) ? (uint64_t)target_coin : 0U;
      index->processors[index->processor_count].active   = true;

      ++index->processor_count;
    }
  }

  _cardano_random_improve_asset_table_free(&required_assets);

  if (result == CARDANO_SUCCESS)
  {
    const size_t pool_size = (index->pool_size > 0U) ? index->pool_size : 1U;

    index->pool              = (cardano_utxo_t**)_cardano_malloc(pool_size * sizeof(cardano_utxo_t*));
    index->pool_asset_counts = (size_t*)_cardano_malloc(pool_size * sizeof(size_t));
    index->pool_selected     = (bool*)_cardano_malloc(pool_size * sizeof(bool));
    index->pool_quantities   = (int64_t*)_cardano_malloc(pool_size * index->processor_count * sizeof(int64_t));

    if (index->pool != NULL)
    {
      CARDANO_UNUSED(memset(index->pool, 0, pool_size * sizeof(cardano_utxo_t*)));
    }

    if (index->pool_selected != NULL)
    {
      CARDANO_UNUSED(memset(index->pool_selected, 0, pool_size * sizeof(bool)));
    }

    if ((index->pool == NULL) || (index->pool_asset_counts == NULL) || (index->pool_selected == NULL) || (index->pool_quantities == NULL))
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  for (size_t i = 0U; (i < index->pool_size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_utxo_t*      utxo   = NULL;
    cardano_utxo_list_t* source = (i < pre_count) ? pre_selected_utxo : available_utxo;
    const size_t         offset = (i < pre_count) ? i : (i - pre_count);

    result = cardano_utxo_list_get(source, offset, &utxo);

    if (result == CARDANO_SUCCESS)
    {
      index->pool[i] = utxo;

      result = register_pool_entry(index, i);
    }
  }

  // Mark the pre-selected entries as selected and seed the running totals with their contents.
  for (size_t i = 0U; (i < pre_count) && (result == CARDANO_SUCCESS); ++i)
  {
    index->pool_selected[i] = true;

    apply_to_totals(index, i, true);
  }

  return result;
}

void
_cardano_random_improve_index_free(random_improve_selection_index_t* index)
{
  if (index->pool != NULL)
  {
    for (size_t i = 0U; i < index->pool_size; ++i)
    {
      cardano_utxo_unref(&index->pool[i]);
    }
  }

  for (size_t p = 0U; p < index->processor_count; ++p)
  {
    cardano_asset_id_unref(&index->processors[p].asset_id);

    for (size_t t = 0U; t < 3U; ++t)
    {
      _cardano_free(index->processors[p].buckets[t].items);
    }
  }

  _cardano_free(index->pool);
  _cardano_free(index->pool_asset_counts);
  _cardano_free(index->pool_selected);
  _cardano_free(index->pool_quantities);
  _cardano_free(index->processors);
  _cardano_free(index->picks);

  CARDANO_UNUSED(memset(index, 0, sizeof(*index)));
}

bool
_cardano_random_improve_index_pick(
  random_improve_selection_index_t* index,
  uint64_t*                         rng_state,
  const size_t                      processor_index)
{
  selection_index_processor_t* processor = &index->processors[processor_index];

  for (size_t tier = 0U; tier < 3U; ++tier)
  {
    selection_index_bucket_t* bucket = &processor->buckets[tier];

    while (bucket->size > 0U)
    {
      const size_t slot  = _cardano_random_improve_rng_below(rng_state, bucket->size);
      const size_t entry = bucket->items[slot];

      // Lazy deletion: remove the slot regardless. If the entry turns out to be selectable it
      // is recorded on the pick stack (and restored on undo); if it was already selected by
      // another requirement it is simply dropped from this bucket.
      bucket->items[slot] = bucket->items[bucket->size - 1U];
      --bucket->size;

      if (!index->pool_selected[entry])
      {
        if (record_pick(index, entry, processor_index, tier) != CARDANO_SUCCESS)
        {
          return false;
        }

        index->pool_selected[entry] = true;

        apply_to_totals(index, entry, true);

        return true;
      }
    }
  }

  return false;
}

void
_cardano_random_improve_index_undo_last_pick(random_improve_selection_index_t* index)
{
  if (index->pick_count == 0U)
  {
    return;
  }

  --index->pick_count;

  const selection_index_pick_t pick = index->picks[index->pick_count];

  index->pool_selected[pick.pool_index] = false;

  apply_to_totals(index, pick.pool_index, false);

  // Return the entry to the bucket it was picked from so it remains a candidate.
  CARDANO_UNUSED(bucket_append(&index->processors[pick.processor_index].buckets[pick.tier], pick.pool_index));
}
