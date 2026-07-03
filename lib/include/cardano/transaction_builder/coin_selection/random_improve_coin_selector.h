/**
 * \file random_improve_coin_selector.h
 *
 * \author angel.castillo
 * \date   Jul 02, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_COIN_SELECTOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_COIN_SELECTOR_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>

#include <cardano/transaction_builder/coin_selection/coin_selector.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Indicates how much of each asset the random improve selector attempts to select,
 * relative to the minimum amount necessary to cover the target.
 *
 * The optimal strategy attempts to select around twice the minimum possible amount of each
 * asset, making it possible to generate change outputs that are roughly the same sizes and
 * shapes as the user-specified outputs. This is the recommended default: it helps a wallet's
 * UTXO distribution evolve over time to resemble the distribution of the payments it makes.
 *
 * The minimal strategy selects just enough of each asset to meet the minimum amount, producing
 * selections with fewer inputs (and therefore smaller, cheaper transactions) at the cost of
 * change outputs that are much smaller than the user-specified outputs.
 *
 * \note Because the selector upholds an exact local balance and acquires additional ada
 * whenever change construction requires it, a selection that fails with the optimal strategy
 * will also fail with the minimal strategy; no automatic fallback between strategies is
 * performed.
 */
typedef enum cardano_selection_strategy_t
{
  /**
   * \brief Selects just enough of each asset to meet the minimum amount.
   */
  CARDANO_SELECTION_STRATEGY_MINIMAL = 0,

  /**
   * \brief Attempts to select around twice the minimum amount of each asset.
   */
  CARDANO_SELECTION_STRATEGY_OPTIMAL = 1
} cardano_selection_strategy_t;

/**
 * \brief Creates a new Round-Robin Random-Improve coin selector.
 *
 * This selector is a port of the multi-asset coin selection algorithm used by cardano-wallet
 * (Round-Robin Random-Improve, a generalization of CIP-2 Random-Improve). It works in two phases:
 *
 * 1. Selection: one sub-selector per required asset (plus one for ada, which runs last) is processed
 *    in round-robin order. On each turn a sub-selector picks a single UTXO at random, preferring UTXOs
 *    that contain only its asset, then UTXOs that pair its asset with one other, then any UTXO containing
 *    the asset. While a sub-selector's minimum is not yet covered, any selection is accepted; once covered,
 *    additional selections are accepted only while they move the selected quantity closer to twice the
 *    minimum. This headroom is what allows change outputs to resemble the user's payments.
 *
 * 2. Change generation: one change output is created per user-specified output (see the `outputs_to_cover`
 *    parameter of \ref cardano_coin_selector_select). The excess of each user-specified asset is partitioned
 *    across the change outputs in proportion to the user's output quantities; assets that were selected
 *    incidentally are distributed in a way that preserves the granularity of the selected inputs. Every
 *    change output is min-ADA compliant; if the selected ada is insufficient to build the change outputs,
 *    additional ada-only UTXOs are selected at random until change can be constructed.
 *
 * Compared to largest-first selection, this algorithm keeps the wallet's UTXO distribution healthy over
 * time (improving transaction parallelism and privacy) at the cost of potentially selecting more inputs.
 *
 * \param[out] coin_selector A pointer to store the address of the newly created coin selector object.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *
 * \note The selector is seeded from a cryptographically secure entropy source. Use
 *       \ref cardano_random_improve_coin_selector_new_with_seed for deterministic behavior.
 *
 * Usage Example:
 * \code{.c}
 * cardano_coin_selector_t* coin_selector = NULL;
 * cardano_error_t result = cardano_random_improve_coin_selector_new(&coin_selector);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   cardano_tx_builder_set_coin_selector(tx_builder, coin_selector);
 * }
 *
 * cardano_coin_selector_unref(&coin_selector);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_random_improve_coin_selector_new(cardano_coin_selector_t** coin_selector);

/**
 * \brief Creates a new Round-Robin Random-Improve coin selector with a deterministic seed.
 *
 * Behaves exactly like \ref cardano_random_improve_coin_selector_new, but the internal random number
 * generator is seeded with the given value, making the selection fully deterministic and reproducible.
 * This is primarily useful for testing.
 *
 * \param[in]  seed          The seed for the internal random number generator.
 * \param[out] coin_selector A pointer to store the address of the newly created coin selector object.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *
 * Usage Example:
 * \code{.c}
 * cardano_coin_selector_t* coin_selector = NULL;
 * cardano_error_t result = cardano_random_improve_coin_selector_new_with_seed(42, &coin_selector);
 *
 * // ... use the selector; selections are reproducible for the same seed and inputs ...
 *
 * cardano_coin_selector_unref(&coin_selector);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_random_improve_coin_selector_new_with_seed(
  uint64_t                  seed,
  cardano_coin_selector_t** coin_selector);

/**
 * \brief Creates a new Round-Robin Random-Improve coin selector with a deterministic seed and
 * an explicit selection strategy.
 *
 * Behaves like \ref cardano_random_improve_coin_selector_new_with_seed, but additionally allows
 * the selection strategy to be chosen. See \ref cardano_selection_strategy_t.
 *
 * \param[in]  seed          The seed for the internal random number generator.
 * \param[in]  strategy      The selection strategy to use.
 * \param[out] coin_selector A pointer to store the address of the newly created coin selector object.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *
 * Usage Example:
 * \code{.c}
 * cardano_coin_selector_t* coin_selector = NULL;
 * cardano_error_t result = cardano_random_improve_coin_selector_new_with_options(
 *   42, CARDANO_SELECTION_STRATEGY_MINIMAL, &coin_selector);
 *
 * // ... the selector now selects just enough of each asset to cover the target ...
 *
 * cardano_coin_selector_unref(&coin_selector);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_random_improve_coin_selector_new_with_options(
  uint64_t                     seed,
  cardano_selection_strategy_t strategy,
  cardano_coin_selector_t**    coin_selector);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_COIN_SELECTOR_H
