/**
 * \file large_first_coin_selector.h
 *
 * \author angel.castillo
 * \date   Oct 16, 2024
 *
 * Copyright 2024 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_LARGE_FIRST_COIN_SELECTOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_LARGE_FIRST_COIN_SELECTOR_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Creates a new coin selector using the "large first" strategy.
 *
 * The `cardano_large_first_coin_selector_new` function initializes a new coin selector object that implements
 * the "large first" strategy. In this strategy, UTXOs (Unspent Transaction Outputs) with larger amounts of assets
 * are selected first to satisfy the target amount. This strategy can be more efficient for reducing the number
 * of UTXOs involved in transactions, but may result in lower UTXO fragmentation.
 *
 * \param[out] coin_selector A pointer to the coin selector object that will be created and returned by this function.
 *
 * \return \ref CARDANO_SUCCESS if the coin selector was successfully created, or an appropriate error code indicating failure.
 *
 * \note The caller is responsible for managing the memory of the created `coin_selector` object, and must ensure
 *       it is properly freed when no longer needed using the appropriate deallocation function.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_large_first_coin_selector_new(
  cardano_coin_selector_t** coin_selector);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_LARGE_FIRST_COIN_SELECTOR_H