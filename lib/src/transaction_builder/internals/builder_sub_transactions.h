/**
 * \file builder_sub_transactions.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_SUB_TRANSACTIONS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_SUB_TRANSACTIONS_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/transaction_input.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Adds a finished sub transaction to the transaction.
 *
 * A sub transaction is built and signed by an independent party, so it is treated as opaque: this
 * function adds it to the sub transaction set of the transaction body, creating the set when it is
 * missing, and never modifies it. The set holds a reference to the very same object, which keeps
 * its original bytes and therefore its id and signatures.
 *
 * The inputs spent by a batch must be disjoint and the ids of its sub transactions unique, so the
 * sub transaction is rejected when its id is already in the set, or when one of its spend inputs is
 * spent by a sub transaction added before or by an input explicitly added to the transaction.
 *
 * \p resolved_utxos must resolve every spend input of the sub transaction, since the value they hold
 * takes part in the value conservation of the whole batch and the reference scripts they carry are
 * priced in the fee. It may also resolve its reference inputs, which carry the reference scripts the
 * fee accounts for. The resolved spend inputs are appended to the state sub transaction inputs and
 * the resolved reference inputs to the state sub transaction reference inputs. Reference inputs that
 * are not resolved are skipped, and any other UTXO of the list is ignored.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t to add. The transaction
 *                            holds a reference to it. This parameter must not be NULL.
 * \param[in] resolved_utxos A pointer to the \ref cardano_utxo_list_t with the UTXOs behind the inputs
 *                           of the sub transaction. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the sub transaction was added, \ref CARDANO_ERROR_DUPLICATED_KEY if
 *         its id is already in the transaction or one of its spend inputs is already spent,
 *         \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if one of its spend inputs is not resolved by
 *         \p resolved_utxos, or an appropriate error code indicating the failure reason. The state is
 *         left unchanged on failure.
 */
cardano_error_t
cardano_builder_add_sub_transaction(
  cardano_builder_state_t*   state,
  cardano_sub_transaction_t* sub_transaction,
  cardano_utxo_list_t*       resolved_utxos,
  const char**               error_message);

/**
 * \brief Checks whether an input is spent by a sub transaction of the transaction.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                  construction. This parameter must not be NULL.
 * \param[in] input A pointer to the \ref cardano_transaction_input_t to look up.
 *
 * \return true if one of the sub transactions added to the transaction spends the input, false
 *         otherwise.
 */
bool
cardano_builder_is_input_spent_by_sub_transaction(
  const cardano_builder_state_t*     state,
  const cardano_transaction_input_t* input);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_SUB_TRANSACTIONS_H
