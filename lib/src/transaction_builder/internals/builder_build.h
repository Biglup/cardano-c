/**
 * \file builder_build.h
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_BUILD_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_BUILD_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/transaction/transaction.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Balances and finalizes the transaction under construction.
 *
 * This function verifies that a change address and an available UTXO list were configured, and that
 * a collateral change address and collateral UTXOs were configured when the transaction interacts
 * with Plutus validators. It then sets a placeholder script data hash for fee calculation when the
 * transaction carries script data, balances the transaction using the configured protocol
 * parameters, coin selector and transaction evaluator, recomputes the script data hash from the
 * final redeemers, datums and cost models, and returns the finalized transaction with a new
 * reference.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[out] transaction A pointer to a pointer that receives the finalized
 *                         \ref cardano_transaction_t. The caller must release it with
 *                         \ref cardano_transaction_unref. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS, or is left untouched
 *                           when the failing operation does not report a message. The string is
 *                           either static or owned by the transaction held by the state. It is left
 *                           untouched on success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the transaction was balanced and finalized, or an appropriate
 *         error code indicating the failure reason.
 */
cardano_error_t
cardano_builder_build(
  cardano_builder_state_t* state,
  cardano_transaction_t**  transaction,
  const char**             error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_BUILD_H
