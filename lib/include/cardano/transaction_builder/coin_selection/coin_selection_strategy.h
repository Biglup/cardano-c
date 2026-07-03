/**
 * \file coin_selection_strategy.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTION_STRATEGY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTION_STRATEGY_H

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTION_STRATEGY_H
