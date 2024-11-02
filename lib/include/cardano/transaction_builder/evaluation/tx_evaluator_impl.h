/**
 * \file tx_evaluator_impl.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TX_EVALUATOR_IMPL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TX_EVALUATOR_IMPL_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/transaction/transaction.h>
#include <cardano/witness_set/redeemer_list.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FORWARD DECLARATIONS *******************************************************/

/**
 * \brief Opaque structure for a transaction evaluator implementation.
 *
 * The `cardano_tx_evaluator_impl_t` structure represents an opaque implementation of
 * a transaction evaluator within the Cardano framework. This implementation is intended
 * to encapsulate the logic needed to evaluate transactions.
 */
typedef struct cardano_tx_evaluator_impl_t cardano_tx_evaluator_impl_t;

/* CALLBACKS *****************************************************************/

/**
 * \brief Function pointer type for evaluating a transaction's execution units.
 *
 * Evaluates the execution units required by the transaction, considering any additional UTXOs and redeemers.
 *
 * \param[in] tx_evaluator_impl Pointer to the tx evaluator implementation instance.
 * \param[in] tx               Transaction to evaluate.
 * \param[in] additional_utxos Additional UTXOs required for evaluation (optional).
 * \param[in] redeemers        Redeemers to be evaluated with the transaction.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_tx_evaluate_func_t)(
  cardano_tx_evaluator_impl_t* tx_evaluator_impl,
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         additional_utxos,
  cardano_redeemer_list_t**    redeemers);

/* STRUCTURES ****************************************************************/

/**
 * \brief Opaque structure for a transaction evaluator implementation.
 *
 * The `cardano_tx_evaluator_impl_t` structure is an internal, opaque structure
 * used to encapsulate the implementation details of a transaction evaluator in the
 * Cardano framework.
 */
typedef struct cardano_tx_evaluator_impl_t
{
    /**
     * \brief Name of the tx evaluator implementation.
     */
    char name[256];

    /**
     * \brief Error message buffer for tx evaluator specific error messages.
     */
    char error_message[1024];

    /**
     * \brief Opaque pointer to the implementation-specific context.
     *
     * This pointer holds the state or context required by the tx evaluator implementation.
     * Users should not access or modify this directly.
     */
    cardano_object_t* context;

    /**
     * \brief Function to evaluate a transaction.
     *
     * \see cardano_tx_evaluate_func_t
     */
    cardano_tx_evaluate_func_t evaluate;

} cardano_tx_evaluator_impl_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TX_EVALUATOR_IMPL_H