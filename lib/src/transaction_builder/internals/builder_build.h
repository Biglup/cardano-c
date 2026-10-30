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
 * This function verifies that a change address and an available UTXO list were configured, that the
 * transaction carries every top level guard its sub transactions require, and that a collateral
 * change address and collateral UTXOs were configured when the transaction carries script data or a
 * redeemer exists anywhere in the batch, including one carried only by a sub transaction. It then
 * sets a placeholder script data hash for fee calculation when the transaction carries script data,
 * balances the transaction using the configured protocol parameters, coin selector and transaction
 * evaluator, checks that every script purpose of the balanced body whose script is a Plutus script
 * has a redeemer, recomputes the script data hash from the final redeemers, datums and cost models,
 * and returns the finalized transaction with a new reference.
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
 * \return \ref CARDANO_SUCCESS if the transaction was balanced and finalized,
 *         \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if the transaction lacks a guard required by one of its
 *         sub transactions, \ref CARDANO_ERROR_MISSING_REDEEMER if a script purpose of the transaction
 *         needs a Plutus script but has no redeemer, or an appropriate error code indicating the failure
 *         reason.
 */
cardano_error_t
cardano_builder_build(
  cardano_builder_state_t* state,
  cardano_transaction_t**  transaction,
  const char**             error_message);

/**
 * \brief Sets a placeholder script data hash in a transaction that carries script data.
 *
 * The placeholder has the size of the real hash, so the transaction can be sized and evaluated
 * before its redeemers carry their final execution units. A transaction without script data is
 * left unchanged.
 *
 * \param[in,out] tx A pointer to the \ref cardano_transaction_t that receives the placeholder. This
 *                   parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the placeholder was set or the transaction carries no script data,
 *         or an appropriate error code indicating the failure reason.
 */
cardano_error_t
cardano_builder_set_dummy_script_data_hash(cardano_transaction_t* tx);

/**
 * \brief Computes and sets the script data hash of a transaction that carries script data.
 *
 * The hash covers the redeemers and datums of the transaction witness set and the cost models of
 * the Plutus languages of the scripts the transaction needs. Like the ledger, a script counts only
 * when the body needs it, through a spent input, the mint, a withdrawal, a certificate, a vote, a
 * proposal guardrail or a guard, and its language is found among the scripts provided to the batch:
 * the witness sets of the transaction and of its sub transactions, and the reference scripts of the
 * reference inputs, the spent inputs and the UTXOs the sub transactions spend or reference. A
 * provided script that the body does not need adds no language view. A transaction without script
 * data is left unchanged.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t holding the protocol parameters and
 *                  the resolved UTXOs of the transaction. This parameter must not be NULL.
 * \param[in,out] tx A pointer to the \ref cardano_transaction_t whose script data hash is set. This
 *                   parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the hash was set or the transaction carries no script data,
 *         \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input of the transaction is not among the UTXOs of
 *         \p state, or an appropriate error code indicating the failure reason.
 */
cardano_error_t
cardano_builder_update_script_data_hash(cardano_builder_state_t* state, cardano_transaction_t* tx);

/**
 * \brief Checks that every script purpose of a transaction whose script is a Plutus script has a redeemer.
 *
 * The scripts the body needs are collected from the spent inputs, the mint, the withdrawals, the
 * certificates, the voters, the proposal guardrails and the guards, and each one is looked up among
 * the scripts provided to the batch: the witness sets of the transaction and of its sub transactions,
 * and the reference scripts of the reference inputs, the spent inputs and the UTXOs the sub
 * transactions spend or reference. Every purpose whose script resolves to a Plutus script must have a
 * redeemer with the tag and index the ledger gives that purpose. A purpose whose script is a native
 * script, or a script that nothing provides, needs no redeemer.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t holding the resolved UTXOs of the
 *                  transaction. This parameter must not be NULL.
 * \param[in,out] tx A pointer to the \ref cardano_transaction_t to check. Its last error describes
 *                   the purpose that has no redeemer. This parameter must not be NULL.
 * \param[in] inputs A pointer to the sorted \ref cardano_transaction_input_set_t the transaction
 *                   spends, which the indices of the spend redeemers point into.
 * \param[out] error_message A pointer that receives a string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. The string is either
 *                           static or owned by \p tx. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if every such purpose has a redeemer,
 *         \ref CARDANO_ERROR_MISSING_REDEEMER if one of them has none, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_check_redeemers(
  cardano_builder_state_t*         state,
  cardano_transaction_t*           tx,
  cardano_transaction_input_set_t* inputs,
  const char**                     error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_BUILD_H
