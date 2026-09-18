/**
 * \file builder_sub_build.h
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_SUB_BUILD_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_SUB_BUILD_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/transaction/sub_transaction.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Produces the sub transaction described by the builder state.
 *
 * A sub transaction is an intent: it is deliberately unbalanced, pays no fee and posts no
 * collateral, so this function neither selects inputs nor balances. It projects the transaction
 * under construction onto a new \ref cardano_sub_transaction_t member by member. The inputs are the
 * UTXOs explicitly added to the state, and the outputs, mint, certificates, withdrawals, validity
 * interval, network id, auxiliary data hash, script data hash, reference inputs, voting procedures,
 * proposal procedures, treasury value, donation, guards, required top level guards, direct deposits
 * and account balance intervals are taken from the body of the transaction held by the state. The
 * witness set and the auxiliary data of that transaction are shared with the sub transaction. Fields
 * that only exist on a top level transaction, such as the fee and the collateral, are left out.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                  construction. This parameter must not be NULL.
 * \param[out] sub_transaction A pointer to a pointer that receives the newly created
 *                             \ref cardano_sub_transaction_t. The caller must release it with
 *                             \ref cardano_sub_transaction_unref. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the sub transaction was created, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_build_sub_transaction(
  cardano_builder_state_t*    state,
  cardano_sub_transaction_t** sub_transaction,
  const char**                error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_SUB_BUILD_H
