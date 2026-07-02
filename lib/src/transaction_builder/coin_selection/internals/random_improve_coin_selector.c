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

#include <cardano/error.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>

#include "../../../allocators.h"
#include "../../../string_safe.h"
#include "./random_improve_change_state.h"
#include "./random_improve_helpers.h"
#include "./random_improve_utxo_utils.h"

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

/* SELECTION PHASE ***********************************************************/

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
  const uint64_t current = _cardano_random_improve_selected_quantity(selection, processor->asset_id);

  if (current < processor->minimum)
  {
    return _cardano_random_improve_select_random_with_priority(rng_state, selection, leftover, processor->asset_id);
  }

  const uint64_t target = processor->minimum * 2U;

  if (!_cardano_random_improve_select_random_with_priority(rng_state, selection, leftover, processor->asset_id))
  {
    return false;
  }

  const uint64_t updated = _cardano_random_improve_selected_quantity(selection, processor->asset_id);

  if (_cardano_random_improve_distance(target, updated) < _cardano_random_improve_distance(target, current))
  {
    return true;
  }

  _cardano_random_improve_undo_last_selection(selection, leftover);

  return false;
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
  cardano_value_t*              target,
  random_improve_asset_table_t* required_assets,
  selection_processor_t*        processors,
  size_t*                       processor_count)
{
  size_t count = 0U;

  for (size_t a = 0U; a < required_assets->size; ++a)
  {
    const int64_t required = _cardano_random_improve_get_asset_quantity(target, required_assets->ids[a]);

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
    if (_cardano_random_improve_selected_quantity(selection, processors[p].asset_id) < processors[p].minimum)
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
  random_improve_asset_table_t required_assets = { NULL, 0U, 0U };

  cardano_error_t result = _cardano_random_improve_asset_table_add_value_assets(&required_assets, target);

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
    if (!_cardano_random_improve_select_random_with_priority(rng_state, selection, leftover, NULL))
    {
      result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  _cardano_free(processors);
  _cardano_random_improve_asset_table_free(&required_assets);

  return result;
}

/* CHANGE GENERATION *********************************************************/

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
distribute_user_specified_excess(random_improve_change_state_t* state, const size_t asset_index, const int64_t excess)
{
  uint64_t* parts = (uint64_t*)_cardano_malloc(state->map_count * sizeof(uint64_t));

  if (parts == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = _cardano_random_improve_partition((uint64_t)excess, state->scratch, state->map_count, parts);

  if (result == CARDANO_SUCCESS)
  {
    _cardano_random_improve_store_distribution(state, asset_index, parts);
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
  random_improve_change_state_t* state,
  const size_t                   asset_index,
  const int64_t                  excess,
  const int64_t                  selected_total,
  const size_t                   input_count)
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

    _cardano_random_improve_store_distribution(state, asset_index, parts);
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
  random_improve_change_state_t*     state,
  cardano_utxo_list_t*               selection,
  cardano_value_t*                   target,
  cardano_transaction_output_list_t* outputs_to_cover)
{
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t a = 0U; ((a < state->table.size) && (result == CARDANO_SUCCESS)) && state->excess_valid; ++a)
  {
    cardano_asset_id_t* asset_id = state->table.ids[a];

    int64_t      selected_total = 0;
    const size_t input_count    = _cardano_random_improve_collect_input_quantities(state, selection, asset_id, &selected_total);

    const int64_t required = _cardano_random_improve_get_asset_quantity(target, asset_id);
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

    const uint64_t user_weight_total = (state->output_count > 0U) ? _cardano_random_improve_collect_user_weights(state, outputs_to_cover, asset_id) : 0U;

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

  random_improve_change_state_t state;

  cardano_error_t result = _cardano_random_improve_change_state_init(&state, selection, target, outputs_to_cover);

  if (result == CARDANO_SUCCESS)
  {
    result = distribute_asset_excesses(&state, selection, target, outputs_to_cover);
  }

  if ((result == CARDANO_SUCCESS) && !state.excess_valid)
  {
    result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
  }

  if ((result == CARDANO_SUCCESS) && _cardano_random_improve_change_is_exact_match(&state))
  {
    result = cardano_transaction_output_list_new(change_outputs);

    _cardano_random_improve_change_state_free(&state);

    return result;
  }

  uint64_t ada_required = 0U;
  size_t   first_map    = 0U;

  const uint64_t ada_available = (state.excess_coin > 0) ? (uint64_t)state.excess_coin : 0U;

  if (result == CARDANO_SUCCESS)
  {
    const uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);

    _cardano_random_improve_order_maps_by_asset_count(&state);

    result = _cardano_random_improve_compute_min_ada_requirements(&state, change_address, ada_per_utxo_byte, &ada_required);
  }

  if (result == CARDANO_SUCCESS)
  {
    _cardano_random_improve_drop_underfunded_empty_maps(&state, ada_available, &ada_required, &first_map);

    if (ada_available < ada_required)
    {
      result = CARDANO_ERROR_BALANCE_INSUFFICIENT;
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = _cardano_random_improve_emit_change_outputs(&state, first_map, ada_available, ada_required, change_address, change_outputs);
  }

  if (result == CARDANO_SUCCESS)
  {
    *change_count = state.map_count - first_map;
  }

  if ((result != CARDANO_SUCCESS) && (*change_outputs != NULL))
  {
    cardano_transaction_output_list_unref(change_outputs);
  }

  _cardano_random_improve_change_state_free(&state);

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
      if ((change_count >= desired_map_count) || !_cardano_random_improve_select_random_with_priority(rng_state, selection, leftover, NULL))
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
      if (!_cardano_random_improve_select_random_with_priority(rng_state, selection, leftover, NULL))
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

  result = _cardano_random_improve_move_pre_selected(pre_selected_utxo, *selection, *remaining_utxo);

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
