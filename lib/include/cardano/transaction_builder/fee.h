/**
 * \file fee.h
 *
 * \author angel.castillo
 * \date   Oct 13, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_FEE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_FEE_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction/transaction.h>
#include <cardano/witness_set/redeemer.h>

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Computes the minimum transaction fee.
 *
 * This function computes the minimum required transaction fee for the provided transaction, based on
 * the resolved inputs and protocol parameters. The calculated fee considers factors such as the transaction
 * size, execution units consumed by plutus scripts and size of included reference scripts.
 *
 * Reference scripts are always priced, whatever their language and whether or not the transaction has redeemers: the
 * ledger charges for every reference script found on the outputs behind the reference inputs and the spend inputs of a
 * transaction, even if the script never runs. \p resolved_ref_inputs must therefore resolve both kinds of inputs.
 *
 * A transaction that carries sub transactions pays the fee of the whole batch. The size of the sub transactions is part
 * of the size of the transaction, and the execution units of the redeemers of every sub transaction are added to the ones
 * of the transaction. The reference scripts of the sub transactions are included if the resolved UTXOs of their reference
 * inputs and spend inputs are given in \p resolved_ref_inputs. Including them is deliberate and may exceed the current
 * ledger minimum, which does not charge for the reference scripts of sub transactions yet.
 *
 * \param[in] transaction The pointer to the \ref cardano_transaction_t object representing the transaction.
 * \param[in] resolved_ref_inputs A pointer to the \ref cardano_utxo_list_t containing every resolved UTXO whose reference script is priced: the ones behind
 *                                the reference inputs and the spend inputs of the transaction and of its sub transactions. Every entry is priced, so a UTXO
 *                                that a body both references and spends must be listed once, and a UTXO used by several bodies once per body.
 * \param[in] protocol_params The pointer to the \ref cardano_protocol_parameters_t structure that contains protocol-related parameters for fee calculation.
 * \param[out] fee A pointer to a uint64_t that will hold the computed transaction fee upon success.
 *
 * \return \ref CARDANO_SUCCESS if the fee was successfully computed, or an appropriate error code indicating failure.
 *
 * \note The transaction size and the execution unit requirements for Plutus scripts influence the final fee.
 *       Ensure that the resolved UTXOs match the reference inputs and the spend inputs specified in the transaction to compute accurate fees.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;
 * cardano_utxo_list_t* resolved_ref_inputs = ...;
 * cardano_protocol_parameters_t* protocol_params = ...;
 * uint64_t computed_fee = 0;
 *
 * cardano_error_t result = cardano_compute_transaction_fee(
 *   tx,
 *   resolved_ref_inputs,
 *   protocol_params,
 *   &computed_fee);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully computed the fee
 *   // computed_fee contains the result
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_compute_transaction_fee(
  cardano_transaction_t*         transaction,
  cardano_utxo_list_t*           resolved_ref_inputs,
  cardano_protocol_parameters_t* protocol_params,
  uint64_t*                      fee);

/**
 * \brief Computes the minimum ADA required for a transaction output.
 *
 * This function calculates the minimum amount of ADA (in lovelace) that must be present in a UTXO, based on its size
 * and the `coins_per_utxo_byte` parameter from the protocol. The size of the UTXO is determined by the provided
 * transaction output object, and the protocol parameter `coins_per_utxo_byte` specifies the cost per byte for UTXOs.
 *
 * \param[in] output The pointer to the \ref cardano_transaction_output_t object representing the transaction output for which the minimum ADA is being calculated.
 * \param[in] coins_per_utxo_byte The protocol parameter specifying the cost in lovelace per byte of the UTXO.
 * \param[out] lovelace_required A pointer to a uint64_t that will hold the calculated minimum amount of ADA (in lovelace) required for the UTXO.
 *
 * \return \ref CARDANO_SUCCESS if the minimum ADA was successfully computed, or an appropriate error code indicating failure.
 *
 * \note The minimum ADA ensures that the UTXO is valid and meets protocol requirements. It accounts for factors such as
 *       the size of the UTXO, including assets, scripts, and other elements within the output.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...;
 * uint64_t coins_per_utxo_byte = 4310;  // Example value from protocol parameters
 * uint64_t min_ada = 0;
 *
 * cardano_error_t result = cardano_compute_min_ada_required(output, coins_per_utxo_byte, &min_ada);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully computed the minimum ADA required for the UTXO
 *   // min_ada now holds the result
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_compute_min_ada_required(
  cardano_transaction_output_t* output,
  uint64_t                      coins_per_utxo_byte,
  uint64_t*                     lovelace_required);

/**
 * \brief Computes the minimum fee required for a transaction with Plutus scripts.
 *
 * This function calculates the minimum fee required for a transaction that contains Plutus scripts. The fee is based on:
 * - The execution units required by the scripts.
 * - The prices of execution units.
 * - The size of the reference scripts and the cost per reference script byte.
 *
 * The execution units are the total of the whole batch: the redeemers of the witness set of the transaction plus the
 * redeemers of the witness set of every sub transaction it carries, since the transaction that carries the sub
 * transactions pays for their script execution. A transaction without sub transactions only accounts for its own redeemers.
 *
 * Reference scripts are always priced, whatever their language: when the batch has no redeemers the execution unit part of
 * the fee is zero and the computed fee is the reference script fee alone. The reference scripts of the sub transactions
 * resolved in \p resolved_reference_inputs are included. This is deliberate and may exceed the current ledger minimum, which
 * does not charge for the reference scripts of sub transactions yet.
 *
 * \param[in] tx The pointer to the \ref cardano_transaction_t object representing the transaction for which the script fee is being calculated.
 * \param[in] prices The pointer to the \ref cardano_ex_unit_prices_t object containing the prices for execution units (memory and steps).
 * \param[in] resolved_reference_inputs A pointer to the \ref cardano_utxo_list_t object holding every resolved UTXO whose reference script is priced: the ones
 *                                      behind the reference inputs and the spend inputs of the transaction and of its sub transactions. Every entry
 *                                      is priced, so a UTXO that a body both references and spends must be listed once, and a UTXO used by several
 *                                      bodies once per body.
 * \param[in] coins_per_ref_script_byte The pointer to the \ref cardano_unit_interval_t object representing the cost per byte of reference scripts.
 * \param[out] min_fee A pointer to a uint64_t where the calculated minimum fee for the transaction will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the minimum script fee was successfully computed, or an appropriate error code indicating failure.
 *
 * \note The calculated fee ensures that the transaction covers the costs for all the Plutus scripts included, based on their execution requirements, and the size of every reference
 *       script reachable from its inputs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;  // The transaction object
 * cardano_ex_unit_prices_t* prices = ...;  // Prices for execution units
 * cardano_utxo_list_t* resolved_reference_inputs = ...;  // List of resolved UTXOs
 * cardano_unit_interval_t* coins_per_ref_script_byte = ...;  // Cost per byte for reference scripts
 * uint64_t min_fee = 0;
 *
 * cardano_error_t result = cardano_compute_min_script_fee(tx, prices, resolved_ref_inputs, coins_per_ref_script_byte, &min_fee);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully computed the minimum fee for the transaction
 *   // min_fee now holds the calculated value
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_compute_min_script_fee(
  cardano_transaction_t*    tx,
  cardano_ex_unit_prices_t* prices,
  cardano_utxo_list_t*      resolved_reference_inputs,
  cardano_unit_interval_t*  coins_per_ref_script_byte,
  uint64_t*                 min_fee);

/**
 * \brief Computes the minimum fee required for a transaction without considering script costs.
 *
 * This function calculates the minimum transaction fee based on the serialized size of the transaction
 * and the protocol parameters for fee calculation. The fee is computed as:
 *
 * \f$ \text{min_fee} = \text{min_fee_constant} + (\text{min_fee_coefficient} \times \text{tx_size}) \f$
 *
 * where `tx_size` is the size of the transaction in bytes.
 *
 * \param[in] tx A pointer to the \ref cardano_transaction_t object representing the transaction.
 * \param[in] min_fee_constant The constant fee factor (A) from the protocol parameters.
 * \param[in] min_fee_coefficient The fee-per-byte coefficient (B) from the protocol parameters.
 * \param[out] min_fee A pointer to a uint64_t where the computed minimum fee will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the minimum fee was successfully computed, or an appropriate error code indicating failure.
 *
 * \note This function only computes the base fee without considering any costs related to scripts or smart contract execution.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* tx = ...;  // The transaction to calculate the fee for
 * uint64_t min_fee_constant = 155381;  // Example value from protocol parameters
 * uint64_t min_fee_coefficient = 44;   // Example value from protocol parameters
 * uint64_t min_fee = 0;
 *
 * cardano_error_t result = cardano_compute_min_fee_without_scripts(tx, min_fee_constant, min_fee_coefficient, &min_fee);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully computed the minimum fee
 *   // min_fee now holds the calculated fee
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_compute_min_fee_without_scripts(
  cardano_transaction_t* tx,
  uint64_t               min_fee_constant,
  uint64_t               min_fee_coefficient,
  uint64_t*              min_fee);

/**
 * \brief Computes the script reference fee for transaction inputs.
 *
 * This function calculates the fee component that is contributed by reference scripts on the inputs of a transaction.
 * The fee is computed based on the size of the reference scripts and the protocol parameter `coins_per_ref_script_byte`.
 * The total size is priced in tiers of 25600 bytes, and every tier costs 1.2 times as much per byte as the previous one.
 *
 * The ledger charges for every reference script found on the outputs behind the reference inputs and the spend inputs of a
 * transaction, whatever the language of the script (native scripts included) and whether or not the script runs. The size
 * of each script is the one reported by \ref cardano_get_serialized_script_size, which documents how that size compares
 * to the one the ledger prices.
 *
 * The total size is not distinct: every entry of the list is counted, so a reference script that appears in several entries
 * is priced once per entry. The ledger takes the inputs of a body as a set, so a UTXO that a body both references and
 * spends must be listed once. Counting every entry matches how the ledger measures the reference script size of a batch, where
 * a UTXO referenced by the transaction and by one of its sub transactions counts twice. For the sub transactions the ledger only
 * applies that measure to the reference script size limit of the transaction and does not charge for their reference scripts
 * yet. Including them in the list is deliberate and may exceed the current ledger minimum.
 *
 * \param[in] resolved_reference_inputs A pointer to the \ref cardano_utxo_list_t object holding every resolved UTXO whose reference script is priced: the
 *                            ones behind the reference inputs and the spend inputs of the transaction. Entries without a reference script add nothing.
 * \param[in] coins_per_ref_script_byte The fee cost per byte of reference script data, as defined by the protocol parameters.
 * \param[out] script_ref_fee A pointer to a uint64_t where the computed reference script fee will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the reference script fee was successfully computed, or an appropriate error code indicating failure.
 *
 * \note This function helps determine the additional cost of including reference scripts in a transaction's inputs based on the
 *       protocol's fee model.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* resolved_ref_inputs = ...;  // Resolved UTXO inputs, potentially containing reference scripts
 * cardano_unit_interval_t* coins_per_ref_script_byte = ...;  // Protocol parameter for fee per byte of reference script
 * uint64_t script_ref_fee = 0;
 *
 * cardano_error_t result = cardano_compute_script_ref_fee(resolved_ref_inputs, coins_per_ref_script_byte, &script_ref_fee);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully computed the script reference fee
 *   // script_ref_fee now holds the calculated fee value
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_compute_script_ref_fee(
  cardano_utxo_list_t*     resolved_reference_inputs,
  cardano_unit_interval_t* coins_per_ref_script_byte,
  uint64_t*                script_ref_fee);

/**
 * \brief Computes the total execution units (exUnits) from a list of redeemers.
 *
 * This function aggregates the total execution units consumed by all redeemers in the provided \ref cardano_redeemer_list_t.
 * Execution units are a critical metric used in Plutus script evaluation, where both memory and CPU units are factored.
 *
 * \param[in] redeemers A pointer to the \ref cardano_redeemer_list_t object representing the list of redeemers.
 * \param[out] total A pointer to a \ref cardano_ex_units_t object where the total execution units will be stored.
 *                   This will contain the sum of the memory and CPU units from all redeemers.
 *
 * \return \ref CARDANO_SUCCESS if the total execution units were successfully computed, or an appropriate error code
 *         indicating failure.
 *
 * \note The `total` object must be managed by the caller. After use, it should be properly de-referenced
 *       with \ref cardano_ex_units_unref to avoid memory leaks.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemers = ...;  // Redeemer list for the transaction
 * cardano_ex_units_t* total_ex_units = NULL;
 *
 * cardano_error_t result = cardano_get_total_ex_units_in_redeemers(redeemers, &total_ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully computed the total execution units for the redeemers
 *   // total_ex_units now holds the summed execution units
 * }
 *
 * // Clean up after use
 * cardano_ex_units_unref(total_ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_get_total_ex_units_in_redeemers(cardano_redeemer_list_t* redeemers, cardano_ex_units_t** total);

/**
 * \brief Computes the serialized size of a given amount of lovelace.
 *
 * This function calculates the size in bytes required to serialize a given amount of lovelace.
 *
 * \param[in] lovelace The amount of lovelace to calculate the serialized size for.
 * \param[out] size_in_bytes A pointer to a \c size_t where the size of the serialized lovelace in bytes will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the size was successfully computed, or an appropriate error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t lovelace = 1000000;  // Example amount of 1 ADA
 * size_t size_in_bytes = 0;
 *
 * cardano_error_t result = cardano_get_serialized_coin_size(lovelace, &size_in_bytes);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The size_in_bytes now holds the serialized size of the lovelace amount
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_get_serialized_coin_size(uint64_t lovelace, size_t* size_in_bytes);

/**
 * \brief Computes the serialized size of a given transaction output.
 *
 * This function calculates the size in bytes required to serialize a given transaction output.
 *
 * \param[in] output A pointer to the \ref cardano_transaction_output_t object for which the serialized size is to be calculated.
 * \param[out] size_in_bytes A pointer to a \c size_t where the size of the serialized output in bytes will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the size was successfully computed, or an appropriate error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = { ... };  // Assume this is initialized
 * size_t size_in_bytes = 0;
 *
 * cardano_error_t result = cardano_get_serialized_output_size(output, &size_in_bytes);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The size_in_bytes now holds the serialized size of the output
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_get_serialized_output_size(cardano_transaction_output_t* output, size_t* size_in_bytes);

/**
 * \brief Computes the serialized size of a given script.
 *
 * This function calculates the size in bytes required to serialize a given script. The size is the one of the two element
 * array that holds the language tag followed by the script, as a script reference carries it: the CBOR of the native
 * script, or the byte string with the bytes of the Plutus script. The script is encoded again from its fields, it is not
 * measured from the bytes it was stored with on chain.
 *
 * The ledger prices the size of the script alone: the bytes inside the byte string for a Plutus script, and the bytes the
 * script was stored with on chain for a native script. The size computed here is a few bytes larger than that for a Plutus
 * script and for a native script in the encoding this library produces, so a fee computed from it covers what the ledger
 * charges for those scripts. A native script that was stored on chain with a longer, non minimal encoding (for example a
 * slot written as an 8 byte integer) can be measured smaller than the ledger measures it, because this library does not keep the
 * original bytes of a native script and encodes its integers with the minimal length. A fee computed from the size of such
 * a script can be lower than the minimum the ledger requires for it.
 *
 * \param[in] script A pointer to the \ref cardano_script_t object for which the serialized size is to be calculated.
 * \param[out] size_in_bytes A pointer to a \c size_t where the size of the serialized script in bytes will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the size was successfully computed, or an appropriate error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_t* script = { ... };  // Assume this is initialized
 * size_t size_in_bytes = 0;
 *
 * cardano_error_t result = cardano_get_serialized_script_size(script, &size_in_bytes);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The size_in_bytes now holds the serialized size of the script
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_get_serialized_script_size(cardano_script_t* script, size_t* size_in_bytes);

/**
 * \brief Computes the serialized size of a given transaction.
 *
 * This function calculates the size in bytes required to serialize a given transaction.
 *
 * \param[in] transaction A pointer to the \ref cardano_transaction_t object for which the serialized size is to be calculated.
 * \param[out] size_in_bytes A pointer to a \c size_t where the size of the serialized transaction in bytes will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the size was successfully computed, or an appropriate error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* transaction = { ... };  // Assume this is initialized
 * size_t size_in_bytes = 0;
 *
 * cardano_error_t result = cardano_get_serialized_transaction_size(transaction, &size_in_bytes);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The size_in_bytes now holds the serialized size of the transaction
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_get_serialized_transaction_size(cardano_transaction_t* transaction, size_t* size_in_bytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_FEE_H