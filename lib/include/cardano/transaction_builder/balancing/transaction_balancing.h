/**
 * \file transaction_balancing.h
 *
 * \author angel.castillo
 * \date   Nov 01, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BALANCING_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BALANCING_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/providers/provider.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/value.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>
#include <cardano/transaction_builder/balancing/input_to_redeemer_map.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>

#include <cardano/export.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Balances a Cardano transaction by adding necessary inputs, calculating change, and adjusting collateral.
 *
 * This function balances an unbalanced transaction (`unbalanced_tx`) by:
 * - Adding additional inputs if the transaction does not meet the required balance.
 * - Computing the cost of script execution.
 * - Calculating the change output to ensure the transaction has the correct total ADA and assets.
 * - Adding collateral inputs if a redeemer exists in the transaction or in any of the sub transactions it carries.
 *
 * When the transaction carries CIP-118 sub transactions the ledger checks value conservation over the whole batch, so
 * the net imbalance of the sub transactions (see \ref cardano_compute_sub_transaction_imbalance) is part of the value
 * that coin selection must cover: a net deficit is funded by additional top level inputs and a net surplus ends up in
 * the change outputs, and the result is verified with the whole batch check of \ref cardano_is_transaction_balanced.
 *
 * The resolved UTXOs of the sub transactions, the ones they spend and optionally the ones they reference, are given in
 * \p available_utxo, next to the UTXOs the top level transaction may spend. The role of each UTXO is taken from the
 * bodies of the sub transactions: a UTXO that a sub transaction spends or references is never selected as a top level
 * input. An input of a sub transaction is looked up in \p available_utxo first, then in \p reference_inputs and then in
 * \p pre_selected_utxo, since a UTXO may be shared between the top level transaction and a sub transaction. Every input
 * spent by a sub transaction must be resolved, while the reference inputs of the sub transactions that are not resolved
 * are skipped.
 *
 * A top level transaction that uses a PlutusV1, PlutusV2 or PlutusV3 script (in its witness set, as a reference script
 * of a resolved reference input or as a reference script of a pre selected input) must also conserve value by itself,
 * with the sub transactions removed. In that case the sub transactions must balance between themselves, otherwise the
 * function fails with \ref CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS and the fix is to add a balancing sub transaction,
 * since top level change can not absorb the imbalance.
 *
 * The fee always includes the reference scripts the ledger charges for, whatever their language and whether or not the
 * transaction has redeemers: every reference script found on the UTXOs of \p reference_inputs and on the UTXOs the
 * transaction spends, pre selected or coin selected. A UTXO that is both referenced and spent is priced once, while the same
 * script sitting on two different UTXOs is priced twice. The size of each script is the one reported by
 * \ref cardano_get_serialized_script_size, which documents how that size compares to the one the ledger prices.
 *
 * The top level transaction also pays the fee and posts the collateral of the whole batch. The fee covers the size of the
 * sub transactions, the execution units of the redeemers of every sub transaction and the reference scripts of the sub
 * transactions: the ones on the UTXOs each sub transaction spends and the ones on its resolved reference inputs. A UTXO that
 * a sub transaction both references and spends is priced once, while a UTXO referenced by several sub transactions is priced
 * once per sub transaction, as the ledger counts it when it measures the reference script size of a batch. Including them is
 * deliberate and may exceed the current ledger minimum, which does not charge for the reference scripts of sub transactions
 * yet. Collateral is added when a redeemer exists in the transaction or in any of its sub transactions, and it is sized from
 * the fee of the top level transaction. The scripts of the sub transactions are not evaluated, the execution units their
 * redeemers declare are taken as final.
 *
 * \param[in, out] unbalanced_tx              A pointer to the transaction that needs balancing.
 * \param[in]      foreign_signature_count    The number of expected extra signatures, not specified in the transaction.
 * \param[in]      protocol_params            A pointer to the protocol parameters required for fee calculation and balancing.
 * \param[in]      reference_inputs           A list of resolved reference inputs of the top level transaction that have already been included in the
 *                                            transaction. The reference scripts they carry are priced in the fee. An input of a sub transaction that
 *                                            is not found in \p available_utxo is also looked up in this list.
 * \param[in]      pre_selected_utxo          A list of UTXOs that must be included in the transaction inputs. They must be disjoint from the inputs
 *                                            spent by the sub transactions the transaction carries. An overlap is rejected. The reference scripts
 *                                            they carry are priced in the fee. A reference input of a sub transaction that is not found in
 *                                            \p available_utxo or in \p reference_inputs is also looked up in this list.
 * \param[in]      input_to_redeemer_map      A map of inputs to redeemers. This map associates specific references of inputs to redeemers in the witness set. Balancing the transaction can add
 *                                            additional inputs and this can make inputs change positions in the input set. Redeemers must be updated to point to the correct input.
 *                                            If you provide redeemers for any pre-selected input, you must specify this association in this map.
 * \param[in]      available_utxo             A list of available UTXOs to select from, if additional inputs are needed. The reference scripts
 *                                            carried by the ones that end up selected are priced in the fee. When the transaction carries sub
 *                                            transactions, the UTXOs they spend must be resolvable from this list or from \p reference_inputs, and
 *                                            supplying them in this list is the recommended way. This list may also hold the UTXOs they reference.
 *                                            The UTXOs a sub transaction spends or references are never selected, the value of the spent ones takes
 *                                            part in the value conservation of the batch and the reference scripts they carry are priced in the fee.
 *                                            The scripts of the UTXOs used only by sub transactions take no part in script evaluation or in the
 *                                            validation mode of the top level transaction.
 * \param[in]      coin_selector              A pointer to the coin selector used for choosing appropriate UTXOs.
 * \param[in]      change_address             The address where any remaining balance (change) will be sent.
 * \param[in]      available_collateral_utxo  A list of available UTXOs to select from as collateral if a redeemer exists in the transaction or in any of its sub transactions.
 * \param[in]      collateral_change_address  The address where any remaining collateral change will be sent, if applicable.
 * \param[in]      evaluator                  A transaction evaluator instance for determining the execution cost of scripts.
 * \param[in]      deferred_redeemers         An optional list of deferred redeemers to resolve on every balancing iteration, once the canonical
 *                                            input order, change outputs and fee of the draft transaction are known. Each entry's callback produces
 *                                            the redeemer payload from the balanced draft. Can be NULL if the transaction has no deferred redeemers.
 *
 * \return \ref CARDANO_SUCCESS if the transaction was balanced successfully, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input spent
 *         by a sub transaction is not resolved, \ref CARDANO_ERROR_DUPLICATED_KEY if a pre selected UTXO is also spent by a sub transaction,
 *         \ref CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS if the top level transaction uses a PlutusV1, PlutusV2 or PlutusV3 script and
 *         its sub transactions do not balance between themselves, or an appropriate error code indicating the type of failure.
 *
 * \note This function assumes that the `unbalanced_tx` is a valid but incomplete transaction, missing necessary inputs to meet the target balance.
 *       After calling this function, the `unbalanced_tx` will be updated with additional inputs, collateral, and, if necessary, a change output.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;                               // Unbalanced transaction
 * size_t foreign_signature_count = 2;                            // Example number of signatures
 * cardano_protocol_parameters_t* params = ...;                   // Protocol parameters
 * cardano_utxo_list_t* ref_inputs = ...;                         // Resolved reference inputs
 * cardano_utxo_list_t* preselected = ...;                        // Pre-selected UTXOs
 * cardano_input_to_redeemer_map_t* input_to_redeemer_map = ...;  // Input to redeemer map
 * cardano_utxo_list_t* available = ...;                          // Available UTXOs, including the UTXOs of the sub transactions
 * cardano_coin_selector_t* selector = ...;                       // Coin selector instance
 * cardano_address_t* change_addr = ...;                          // Change address
 * cardano_utxo_list_t* collateral_utxo = ...;                    // Available collateral UTXOs
 * cardano_address_t* collateral_change_addr = ...;               // Collateral change address
 * cardano_tx_evaluator_t* eval = ...;                            // Evaluator instance
 *
 * cardano_error_t result = cardano_balance_transaction(tx, foreign_signature_count, params, ref_inputs, preselected, input_to_redeemer_map, available, selector, change_addr, collateral_utxo, collateral_change_addr, eval, NULL);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Transaction was balanced successfully
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_balance_transaction(
  cardano_transaction_t*            unbalanced_tx,
  size_t                            foreign_signature_count,
  cardano_protocol_parameters_t*    protocol_params,
  cardano_utxo_list_t*              reference_inputs,
  cardano_utxo_list_t*              pre_selected_utxo,
  cardano_input_to_redeemer_map_t*  input_to_redeemer_map,
  cardano_utxo_list_t*              available_utxo,
  cardano_coin_selector_t*          coin_selector,
  cardano_address_t*                change_address,
  cardano_utxo_list_t*              available_collateral_utxo,
  cardano_address_t*                collateral_change_address,
  cardano_tx_evaluator_t*           evaluator,
  cardano_deferred_redeemer_list_t* deferred_redeemers);

/**
 * \brief Checks whether a Cardano transaction is balanced.
 *
 * This function verifies if the specified transaction (`tx`) meets the balance requirements as per Cardano protocol rules.
 * It considers the total inputs, outputs, fees, and execution costs to determine if the transaction is balanced.
 *
 * Value conservation covers the whole CIP-118 batch: when the transaction carries sub transactions, the value they
 * consume and produce is accounted together with the top level body, so the transaction is balanced when its
 * \ref cardano_compute_transaction_batch_imbalance is zero. A transaction without sub transactions is balanced when its
 * own body is.
 *
 * \param[in]  tx               A pointer to the transaction to be checked.
 * \param[in]  resolved_inputs  A list of UTXOs that have been selected and are expected to cover the transaction's outputs and fees.
 *                              It must resolve the inputs of the transaction and of every sub transaction it carries.
 * \param[in]  protocol_params  Protocol parameters needed for fee calculation, including min-fee coefficients and other constraints.
 * \param[out] is_balanced      A pointer to a boolean that will hold the result. Set to `true` if the transaction is balanced, or `false` otherwise.
 *
 * \return \ref CARDANO_SUCCESS if the balance check was performed successfully, or an appropriate error code indicating the type of failure.
 *
 * \note This function provides a quick way to verify that a transaction includes enough inputs to cover its outputs and all associated fees.
 *       It does not modify the transaction but provides a binary check on its balance state.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;                // Transaction to check
 * cardano_utxo_list_t* resolved_inputs = ...;     // List of resolved input UTXOs
 * cardano_protocol_parameters_t* params = ...;    // Protocol parameters
 * bool is_balanced = false;
 *
 * cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, params, &is_balanced);
 *
 * if (result == CARDANO_SUCCESS && is_balanced)
 * {
 *   // Transaction is balanced
 * }
 * else
 * {
 *   // Transaction is not balanced, or an error occurred
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_is_transaction_balanced(
  cardano_transaction_t*         tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  bool*                          is_balanced);

/**
 * \brief Computes the imbalance of a Cardano transaction.
 *
 * The imbalance is the value the transaction consumes minus the value it produces. Consumed value is the sum of the
 * resolved input values, the reward withdrawals, the deposit refunds and the minted assets; produced value is the sum
 * of the outputs, the fee, the deposits, the treasury donation, the direct deposits and the burned assets.
 *
 * The imbalance covers the top level body only and ignores the sub transactions the transaction carries (body key 23).
 * The figure the ledger checks for a CIP-118 batch is \ref cardano_compute_transaction_batch_imbalance, which is also
 * what \ref cardano_is_transaction_balanced uses.
 *
 * For a transaction without sub transactions, a zero imbalance means the transaction is balanced. A positive coin or
 * asset amount means the transaction consumes more than it produces (a surplus that a change output or another
 * transaction in a CIP-118 batch must absorb), while a negative amount means it produces more than it consumes (a
 * deficit that other transactions in the batch must fund).
 *
 * The returned value is independent of the transaction and of the resolved inputs: it owns its multi asset, so it may
 * be modified freely (for example with \ref cardano_value_add_asset) without altering the mint field of the body or
 * the value of any resolved UTXO.
 *
 * \param[in]  tx               A pointer to the transaction whose imbalance is computed.
 * \param[in]  resolved_inputs  A list of UTXOs resolving every input of the transaction.
 * \param[in]  protocol_params  Protocol parameters supplying the deposit amounts of the Shelley era certificates.
 * \param[out] imbalance        On success, a pointer to a newly created \ref cardano_value_t holding the imbalance.
 *                              The caller must release it with \ref cardano_value_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the imbalance was computed, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input has no
 *         resolved UTXO, or an appropriate error code indicating the type of failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;                // Transaction to inspect
 * cardano_utxo_list_t* resolved_inputs = ...;     // List of resolved input UTXOs
 * cardano_protocol_parameters_t* params = ...;    // Protocol parameters
 * cardano_value_t* imbalance = NULL;
 *
 * cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, params, &imbalance);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (cardano_value_get_coin(imbalance) > 0)
 *   {
 *     // The transaction consumes more lovelace than it produces
 *   }
 *
 *   cardano_value_unref(&imbalance);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_compute_transaction_imbalance(
  cardano_transaction_t*         tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              imbalance);

/**
 * \brief Computes the imbalance of a CIP-118 sub transaction.
 *
 * A sub transaction is an intent: it is deliberately unbalanced and its imbalance is what a batcher matches against the
 * other transactions of the batch. The imbalance is the value the sub transaction consumes minus the value it produces,
 * with the same terms and the same sign convention as \ref cardano_compute_transaction_imbalance, except that a sub
 * transaction body carries no fee.
 *
 * The returned value is independent of the sub transaction and of the resolved inputs: it owns its multi asset, so a
 * batcher may modify it freely (for example with \ref cardano_value_add_asset) without altering the mint field of the
 * sub transaction body or the value of any resolved UTXO.
 *
 * \param[in]  sub_tx           A pointer to the sub transaction whose imbalance is computed.
 * \param[in]  resolved_inputs  A list of UTXOs resolving every input of the sub transaction.
 * \param[in]  protocol_params  Protocol parameters supplying the deposit amounts of the Shelley era certificates.
 * \param[out] imbalance        On success, a pointer to a newly created \ref cardano_value_t holding the imbalance.
 *                              The caller must release it with \ref cardano_value_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the imbalance was computed, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input has no
 *         resolved UTXO, or an appropriate error code indicating the type of failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_tx = ...;        // Sub transaction to inspect
 * cardano_utxo_list_t* resolved_inputs = ...;     // List of resolved input UTXOs
 * cardano_protocol_parameters_t* params = ...;    // Protocol parameters
 * cardano_value_t* imbalance = NULL;
 *
 * cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Match the imbalance against the rest of the batch
 *   cardano_value_unref(&imbalance);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_compute_sub_transaction_imbalance(
  cardano_sub_transaction_t*     sub_tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              imbalance);

/**
 * \brief Computes the imbalance of a whole CIP-118 batch.
 *
 * The ledger checks value conservation over the whole batch: the value consumed and produced by the top level
 * transaction body and by every sub transaction it carries is summed in a single check. This function returns the
 * imbalance of the top level body (see \ref cardano_compute_transaction_imbalance) plus the imbalance of each sub
 * transaction (see \ref cardano_compute_sub_transaction_imbalance), coin and multi assets included, with the same sign
 * convention: a positive amount is value the batch still has left over, a negative amount is value the batch still
 * needs, and a zero imbalance means the batch is balanced.
 *
 * For a transaction that carries no sub transactions the result equals \ref cardano_compute_transaction_imbalance.
 *
 * The returned value is independent of the transaction, of its sub transactions and of the resolved inputs: it may be
 * modified freely (for example with \ref cardano_value_add_asset) without altering any of them.
 *
 * \param[in]  tx               A pointer to the top level transaction whose batch imbalance is computed.
 * \param[in]  resolved_inputs  A list of UTXOs resolving every input of the top level transaction body and of every sub
 *                              transaction it carries.
 * \param[in]  protocol_params  Protocol parameters supplying the deposit amounts of the Shelley era certificates.
 * \param[out] imbalance        On success, a pointer to a newly created \ref cardano_value_t holding the imbalance.
 *                              The caller must release it with \ref cardano_value_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the imbalance was computed, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input of the
 *         top level body or of a sub transaction has no resolved UTXO, or an appropriate error code indicating the
 *         type of failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;                // Top level transaction carrying the sub transactions
 * cardano_utxo_list_t* resolved_inputs = ...;     // Resolved input UTXOs of the top level body and the sub transactions
 * cardano_protocol_parameters_t* params = ...;    // Protocol parameters
 * cardano_value_t* imbalance = NULL;
 *
 * cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, params, &imbalance);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (cardano_value_get_coin(imbalance) < 0)
 *   {
 *     // The batch still needs lovelace that the top level transaction must fund
 *   }
 *
 *   cardano_value_unref(&imbalance);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_compute_transaction_batch_imbalance(
  cardano_transaction_t*         tx,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  cardano_value_t**              imbalance);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BALANCING_H