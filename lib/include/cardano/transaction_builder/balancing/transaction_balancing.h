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
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>

#include <cardano/export.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Balances a Cardano transaction by adding necessary inputs and calculating change.
 *
 * This function balances an unbalanced transaction (`unbalanced_tx`) by:
 * - Adding additional inputs if the transaction does not meet the required balance.
 * - Computing the cost of script execution.
 * - Calculating the change output to ensure the transaction has the correct total ADA and assets.
 *
 * \param[in, out] unbalanced_tx       A pointer to the transaction that needs balancing.
 * \param[in]      signature_count     The number of expected signatures, used to calculate witness size and fees.
 * \param[in]      protocol_params     A pointer to the protocol parameters required for fee calculation and balancing.
 * \param[in]      pre_selected_utxo   A list of UTXOs that must be included in the transaction inputs.
 * \param[in]      available_utxo      A list of available UTXOs to select from, if additional inputs are needed.
 * \param[in]      coin_selector       A pointer to the coin selector used for choosing appropriate UTXOs.
 * \param[in]      change_address      The address where any remaining balance (change) will be sent.
 * \param[in]      evaluator           A transaction evaluator instance for determining the execution cost of scripts.
 *
 * \return \ref CARDANO_SUCCESS if the transaction was balanced successfully, or an appropriate error code indicating the type of failure.
 *
 * \note This function assumes that the `unbalanced_tx` is a valid but incomplete transaction, missing necessary inputs to meet the target balance.
 *       After calling this function, the `unbalanced_tx` will be updated with additional inputs and, if necessary, a change output.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;              // Unbalanced transaction
 * size_t signature_count = 2;                   // Example number of signatures
 * cardano_protocol_parameters_t* params = ...;  // Protocol parameters
 * cardano_utxo_list_t* preselected = ...;       // Pre-selected UTXOs
 * cardano_utxo_list_t* available = ...;         // Available UTXOs
 * cardano_coin_selector_t* selector = ...;      // Coin selector instance
 * cardano_address_t* change_addr = ...;         // Change address
 * cardano_tx_evaluator_t* eval = ...;           // Evaluator instance
 *
 * cardano_error_t result = cardano_balance_transaction(tx, signature_count, params, preselected, available, selector, change_addr, eval);
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
  cardano_transaction_t*         unbalanced_tx,
  size_t                         signature_count,
  cardano_protocol_parameters_t* protocol_params,
  cardano_utxo_list_t*           pre_selected_utxo,
  cardano_utxo_list_t*           available_utxo,
  cardano_coin_selector_t*       coin_selector,
  cardano_address_t*             change_address,
  cardano_tx_evaluator_t*        evaluator);

/**
 * \brief Checks whether a Cardano transaction is balanced.
 *
 * This function verifies if the specified transaction (`tx`) meets the balance requirements as per Cardano protocol rules.
 * It considers the total inputs, outputs, fees, and execution costs to determine if the transaction is balanced.
 *
 * \param[in]  tx               A pointer to the transaction to be checked.
 * \param[in]  resolved_inputs  A list of UTXOs that have been selected and are expected to cover the transaction's outputs and fees.
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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BALANCING_H