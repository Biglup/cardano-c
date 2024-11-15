/**
 * \file collateral.h
 *
 * \author angel.castillo
 * \date   Nov 14, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COLLATERAL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COLLATERAL_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction/transaction.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Coalesces all output values in a UTXO list into a single total value.
 *
 * This function iterates over each UTXO in the provided list, extracts its value,
 * and accumulates it into a single total output value.
 *
 * \param[in]  utxos               The list of UTXO to coalesce.
 * \param[out] total_output_value  A pointer to store the cumulative value of all UTXO.
 *
 * \return \ref CARDANO_SUCCESS if the total UTXO value was successfully calculated, or an appropriate error code.
 */
cardano_error_t
_cardano_coalesce_all_utxos(cardano_utxo_list_t* utxos, cardano_value_t** total_output_value);

/**
 * \brief Converts a list of UTXOs to a set of transaction inputs.
 *
 * This function takes a list of UTXOs and converts it into a \ref cardano_transaction_input_set_t.
 *
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t containing the UTXOs to be converted.
 *
 * \return A pointer to a \ref cardano_transaction_input_set_t containing the transaction inputs created
 *         from the UTXO list. Returns NULL if the conversion fails.
 */
cardano_transaction_input_set_t*
_cardano_utxo_list_to_input_set(cardano_utxo_list_t* utxos);

/**
 * \brief Calculates the collateral amount required for a transaction based on the transaction fee and collateral percentage.
 *
 * This function calculates the collateral amount by applying a specified percentage to the transaction fee. The collateral
 * is used in transactions involving Plutus scripts to ensure sufficient funds are available to cover potential transaction costs.
 *
 * \param[in] fee The transaction fee in lovelace.
 * \param[in] collateral_percentage The collateral percentage as an integer. For example, a 150% collateral would be passed as 150.
 *
 * \return The calculated collateral amount in lovelace.
 */
uint64_t
_cardano_calculate_collateral_amount(uint64_t fee, uint64_t collateral_percentage);

/**
 * \brief Creates a collateral change output for a transaction.
 *
 * \param[in]  change_value       The change amount in \ref cardano_value_t format.
 * \param[in]  change_address     A pointer to the \ref cardano_address_t representing the change address.
 * \param[in]  protocol_params    A pointer to the \ref cardano_protocol_parameters_t containing relevant protocol parameters.
 * \param[out] change_padding     If the change output cant be created, the missing value to reach the minimum UTXO value will be set here.
 * \param[out] change_output      On success, this will point to the created \ref cardano_transaction_output_t.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the collateral change
 *         output was created successfully, or an appropriate error code indicating the failure reason.
 */
cardano_error_t
_cardano_create_collateral_change_output(
  cardano_value_t*               change_value,
  cardano_address_t*             change_address,
  cardano_protocol_parameters_t* protocol_params,
  uint64_t*                      change_padding,
  cardano_transaction_output_t** change_output);

/**
 * \brief Updates the transaction body with collateral inputs and collateral change output.
 *
 * This function updates a transaction body by setting the collateral amount and adding the specified
 * collateral change output. It also includes a selection of collateral inputs that will satisfy the collateral requirement.
 *
 * \param[in,out] body              A pointer to the \ref cardano_transaction_body_t to be updated.
 * \param[in]     collateral_amount The amount required as collateral.
 * \param[in]     change_output     A pointer to the \ref cardano_transaction_output_t representing the collateral change output.
 * \param[in]     selection         A list of selected UTXOs to be used as collateral inputs.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the transaction body was
 *         successfully updated with the collateral details, or an appropriate error code indicating the failure reason.
 */
cardano_error_t
_cardano_update_transaction_body_collateral(
  cardano_transaction_body_t*   body,
  uint64_t                      collateral_amount,
  cardano_transaction_output_t* change_output,
  cardano_utxo_list_t*          selection);

/**
 * \brief Sets the collateral output in a Cardano transaction.
 *
 * This function selects collateral outputs from a list of available collateral UTXOs to meet the required collateral amount
 * for the transaction. If the total collateral exceeds the required amount, a change collateral output is created and added to the transaction.
 *
 * \param[in,out] tx                           A pointer to the \ref cardano_transaction_t where the collateral output will be set.
 * \param[in]     protocol_params              A pointer to the \ref cardano_protocol_parameters_t containing protocol parameters for collateral calculations.
 * \param[in]     available_collateral_outputs A list of available UTXOs that can be used for collateral.
 * \param[in]     change_address               A pointer to the \ref cardano_address_t where any collateral change will be sent.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the collateral output was
 *         successfully set, or an appropriate error code indicating the failure reason.
 */
cardano_error_t
_cardano_set_collateral_output(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol_params,
  cardano_utxo_list_t*           available_collateral_outputs,
  cardano_address_t*             change_address);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COLLATERAL_H