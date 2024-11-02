/**
 * \file provider_tx_evaluator.h
 *
 * \author angel.castillo
 * \date   Nov 05, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_TX_EVALUATOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_TX_EVALUATOR_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/providers/provider.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Creates a transaction evaluator from a provider.
 *
 * This function initializes a new transaction evaluator (`tx_evaluator`) using the specified blockchain data provider, allowing
 * transactions to be evaluated based on the provider’s configuration and resources.
 *
 * \param[in] provider A pointer to the \ref cardano_provider_t instance that will be used to configure the transaction evaluator.
 * \param[out] tx_evaluator A double pointer to a \ref cardano_tx_evaluator_t instance, which will be populated with
 *                          the newly created transaction evaluator.
 *
 * \return \ref CARDANO_SUCCESS if the transaction evaluator was successfully created, or an appropriate error code indicating failure.
 *
 * \note The caller is responsible for releasing the allocated memory of `tx_evaluator` by calling the appropriate release function.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_evaluator_t* tx_evaluator = NULL;
 * cardano_error_t result = cardano_tx_evaluator_from_provider(provider, &tx_evaluator);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The transaction evaluator was successfully created and is ready for use
 * }
 *
 * // Release the evaluator when it is no longer needed
 * cardano_tx_evaluator_unref(&tx_evaluator);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_tx_evaluator_from_provider(
  cardano_provider_t*      provider,
  cardano_tx_evaluator_t** tx_evaluator);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_TX_EVALUATOR_H