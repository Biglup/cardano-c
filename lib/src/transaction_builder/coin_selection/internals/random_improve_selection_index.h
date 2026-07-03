/**
 * \file random_improve_selection_index.h
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_SELECTION_INDEX_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_SELECTION_INDEX_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/asset_id.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction_body/value.h>
#include <cardano/typedefs.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief A growable list of pool entry indices, used as a selection bucket.
 *
 * Buckets use lazy deletion: entries that have already been selected by another requirement are
 * skipped (and removed) when encountered during a pick.
 */
typedef struct selection_index_bucket_t
{
    size_t* items;
    size_t  size;
    size_t  capacity;
} selection_index_bucket_t;

/**
 * \brief A selection requirement tracked by the index: one per required asset, plus one for
 * lovelace (with a NULL asset id), which is placed last.
 *
 * The three buckets hold candidate pool entries by priority tier (port of the reference
 * implementation's SelectSingleton / SelectPairWith / SelectAnyWith filters): entries whose
 * total asset count (lovelace included) is exactly one, exactly two, and any, respectively.
 */
typedef struct selection_index_processor_t
{
    cardano_asset_id_t*      asset_id;
    uint64_t                 minimum;
    bool                     active;
    uint64_t                 selected_total;
    selection_index_bucket_t buckets[3];
} selection_index_processor_t;

/**
 * \brief A single pick, recorded so that it can be undone by the improvement step.
 */
typedef struct selection_index_pick_t
{
    size_t pool_index;
    size_t processor_index;
    size_t tier;
} selection_index_pick_t;

/**
 * \brief A per-selection index over the available UTXO pool, providing constant-time selected
 * quantity queries and cheap randomized picks by priority tier.
 *
 * The index caches, for every pool entry, its total asset count and its quantity of every
 * requirement's asset, so that the selection loop never re-derives asset maps from values.
 * Picks record their origin so that the most recent pick can be undone.
 */
typedef struct random_improve_selection_index_t
{
    cardano_utxo_t**             pool;
    size_t*                      pool_asset_counts;
    bool*                        pool_selected;
    int64_t*                     pool_quantities;
    size_t                       pool_size;
    size_t                       pre_selected_count;
    selection_index_processor_t* processors;
    size_t                       processor_count;
    selection_index_pick_t*      picks;
    size_t                       pick_count;
    size_t                       pick_capacity;
} random_improve_selection_index_t;

/* DECLARATIONS **************************************************************/

/**
 * \brief Initializes the selection index for a target and a UTXO pool.
 *
 * Builds one processor per asset required by the target (plus a final one for lovelace),
 * caches every pool entry's asset count and per-requirement quantities, seeds the priority
 * buckets, marks the pre-selected entries as selected and initializes the running totals
 * with their contents.
 *
 * \param[out] index             The index to initialize.
 * \param[in]  pre_selected_utxo The pre-selected UTXOs, or NULL.
 * \param[in]  available_utxo    The available UTXO pool.
 * \param[in]  target            The target value the selection must cover.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code. On failure the index
 *         is safe to pass to \ref _cardano_random_improve_index_free.
 */
cardano_error_t
_cardano_random_improve_index_init(
  random_improve_selection_index_t* index,
  cardano_utxo_list_t*              pre_selected_utxo,
  cardano_utxo_list_t*              available_utxo,
  cardano_value_t*                  target);

/**
 * \brief Frees all buffers held by the index and releases its UTXO references.
 *
 * \param[in,out] index The index to free.
 */
void
_cardano_random_improve_index_free(random_improve_selection_index_t* index);

/**
 * \brief Picks a random unselected pool entry for the given processor, preferring entries with
 * fewer assets (priority tiers: singleton, pair, any).
 *
 * On success the entry is marked as selected, every processor's running total is updated, and
 * the pick is recorded so that it can be undone.
 *
 * \param[in,out] index           The selection index.
 * \param[in,out] rng_state       The random number generator state.
 * \param[in]     processor_index The processor to pick for.
 *
 * \return true if an entry was picked, false if no unselected entry holds the processor's asset.
 */
bool
_cardano_random_improve_index_pick(
  random_improve_selection_index_t* index,
  uint64_t*                         rng_state,
  size_t                            processor_index);

/**
 * \brief Undoes the most recent pick: unmarks the entry, restores the running totals and
 * returns the entry to the bucket it was picked from.
 *
 * \param[in,out] index The selection index.
 */
void
_cardano_random_improve_index_undo_last_pick(random_improve_selection_index_t* index);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_SELECTION_INDEX_H
