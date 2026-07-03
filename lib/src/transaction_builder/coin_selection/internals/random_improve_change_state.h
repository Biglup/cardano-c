/**
 * \file random_improve_change_state.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_CHANGE_STATE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_CHANGE_STATE_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/assets/asset_id.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_body/value.h>
#include <cardano/typedefs.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Working table of every distinct asset involved in a selection.
 */
typedef struct random_improve_asset_table_t
{
    cardano_asset_id_t** ids;
    size_t               size;
    size_t               capacity;
} random_improve_asset_table_t;

/**
 * \brief Working state for the change generation phase of the random improve selector.
 *
 * The `quantities` buffer is an asset-major matrix of `table.size` rows by `map_count` columns:
 * entry `(a, m)` holds the quantity of asset `a` assigned to change map `m`. The `map_order`
 * buffer holds a permutation of the change map indices, sorted by ascending asset count so that
 * empty maps come first. The `scratch` buffer is shared working space for per-asset collection.
 */
typedef struct random_improve_change_state_t
{
    random_improve_asset_table_t table;
    size_t                       map_count;
    size_t                       output_count;
    size_t                       selection_size;
    uint64_t*                    quantities;
    uint64_t*                    scratch;
    uint64_t*                    out_weights;
    size_t*                      map_order;
    uint64_t*                    min_adas;
    int64_t                      excess_coin;
    bool                         excess_valid;
} random_improve_change_state_t;

/* DECLARATIONS **************************************************************/

/**
 * \brief Releases the asset ids held by the table and frees its storage.
 *
 * \param[in,out] table The table to free. Reset to an empty table.
 */
void
_cardano_random_improve_asset_table_free(random_improve_asset_table_t* table);

/**
 * \brief Adds every non-lovelace asset of the given value to the table, skipping duplicates.
 *
 * The table takes a reference on every added asset id. The storage grows geometrically as needed.
 *
 * \param[in,out] table The table to add to.
 * \param[in]     value The value whose assets to add.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
cardano_error_t
_cardano_random_improve_asset_table_add_value_assets(random_improve_asset_table_t* table, cardano_value_t* value);

/**
 * \brief Initializes the change state: allocates the working buffers, collects the involved
 * assets, gathers the output coin weights and computes the excess coin of the selection over
 * the target.
 *
 * \param[out] state            The state to initialize.
 * \param[in]  selection        The selected UTXOs.
 * \param[in]  target           The target value.
 * \param[in]  outputs_to_cover The user-specified outputs, or NULL (in which case a single
 *                              change map with a unit weight is used).
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code. On failure the state
 *         is safe to pass to \ref _cardano_random_improve_change_state_free.
 */
cardano_error_t
_cardano_random_improve_change_state_init(
  random_improve_change_state_t*     state,
  cardano_utxo_list_t*               selection,
  cardano_value_t*                   target,
  cardano_transaction_output_list_t* outputs_to_cover);

/**
 * \brief Frees all buffers held by the change state.
 *
 * \param[in,out] state The state to free.
 */
void
_cardano_random_improve_change_state_free(random_improve_change_state_t* state);

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
size_t
_cardano_random_improve_collect_input_quantities(
  random_improve_change_state_t* state,
  cardano_utxo_list_t*           selection,
  cardano_asset_id_t*            asset_id,
  int64_t*                       selected_total);

/**
 * \brief Collects the per-output quantities of the given asset (the user-specified weights)
 * into the state's scratch buffer.
 *
 * \param[in,out] state            The change state (its scratch buffer receives the weights).
 * \param[in]     outputs_to_cover The user-specified outputs.
 * \param[in]     asset_id         The asset to collect.
 *
 * \return The total user-specified quantity of the asset. A value of zero means the asset is
 *         not user-specified.
 */
uint64_t
_cardano_random_improve_collect_user_weights(
  random_improve_change_state_t*     state,
  cardano_transaction_output_list_t* outputs_to_cover,
  cardano_asset_id_t*                asset_id);

/**
 * \brief Writes the distributed parts of an asset into the state's quantity matrix.
 *
 * \param[in,out] state       The change state.
 * \param[in]     asset_index The row (asset) to write.
 * \param[in]     parts       The quantities to write, one per change map.
 */
void
_cardano_random_improve_store_distribution(
  random_improve_change_state_t* state,
  size_t                         asset_index,
  const uint64_t*                parts);

/**
 * \brief Splits change maps whose assets would exceed the maximum serialized size of a
 * transaction output value.
 *
 * This is a port of `splitBundlesWithExcessiveAssetCounts` from the cardano-coin-selection
 * library: every oversized map has its assets recursively equipartitioned into halves, and each
 * half becomes a change map of its own. The output coin weight of a split map is distributed
 * evenly among its parts. As a result, the number of change maps can exceed the number of
 * user-specified outputs.
 *
 * \param[in,out] state          The change state; its buffers are reallocated when maps split.
 * \param[in]     max_value_size The protocol maximum serialized size of an output value, in
 *                               bytes. A value of zero means "no limit" (no splitting occurs).
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
cardano_error_t
_cardano_random_improve_split_oversized_maps(
  random_improve_change_state_t* state,
  uint64_t                       max_value_size);

/**
 * \brief Orders the change maps by ascending asset count (stable), so empty maps come first.
 *
 * This ordering allows the coin assignment step to drop empty maps from the front of the list
 * when the available ada is insufficient (port of the AssetCount ordering used by
 * assignCoinsToChangeMaps in the reference implementation).
 *
 * \param[in,out] state The change state; its `map_order` buffer is rearranged.
 */
void
_cardano_random_improve_order_maps_by_asset_count(random_improve_change_state_t* state);

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
cardano_error_t
_cardano_random_improve_compute_min_ada_requirements(
  random_improve_change_state_t* state,
  cardano_address_t*             change_address,
  uint64_t                       ada_per_utxo_byte,
  uint64_t*                      ada_required);

/**
 * \brief Indicates whether the selection matches the target exactly (no change is needed).
 *
 * \param[in] state The change state.
 *
 * \return true if the excess coin is zero and no asset quantities were distributed.
 */
bool
_cardano_random_improve_change_is_exact_match(random_improve_change_state_t* state);

/**
 * \brief Drops empty change maps from the front of the ordered list while the available ada is
 * insufficient to fund the min-ADA requirement of every remaining map.
 *
 * At least one map is always kept: the change must carry the full excess, since the local
 * balance invariant of the coin selector contract does not allow leftover ada to be discarded.
 *
 * \param[in]     state         The change state.
 * \param[in]     ada_available The ada available for the change outputs.
 * \param[in,out] ada_required  The total min-ADA requirement; reduced as maps are dropped.
 * \param[out]    first_map     The index (in map order) of the first map that is kept.
 */
void
_cardano_random_improve_drop_underfunded_empty_maps(
  random_improve_change_state_t* state,
  uint64_t                       ada_available,
  uint64_t*                      ada_required,
  size_t*                        first_map);

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
cardano_error_t
_cardano_random_improve_emit_change_outputs(
  random_improve_change_state_t*      state,
  size_t                              first_map,
  uint64_t                            ada_available,
  uint64_t                            ada_required,
  cardano_address_t*                  change_address,
  cardano_transaction_output_list_t** change_outputs);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_CHANGE_STATE_H
