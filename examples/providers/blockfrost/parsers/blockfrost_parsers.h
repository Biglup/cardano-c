/**
 * \file blockfrost_parsers.h
 *
 * \author angel.castillo
 * \date   Sep 30, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_PARSERS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_PARSERS_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/providers/provider_impl.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Parses the Blockfrost protocol parameters from a JSON response.
 *
 * This function takes a JSON response string from the Blockfrost API, parses it,
 * and sets the Cardano protocol parameters accordingly. The function validates
 * the JSON and converts it into the appropriate data structures.
 *
 * \param[in] provider     A pointer to the cardano provider implementation used for handling internal state and error reporting.
 * \param[in] json         The raw JSON string containing the protocol parameters returned by the Blockfrost API.
 * \param[in] size         The size of the JSON string.
 * \param[out] parameters  A pointer to the location where the parsed protocol parameters will be stored.
 *
 * \return CARDANO_SUCCESS on success, or an appropriate error code on failure.
 *
 * \note The caller is responsible for freeing the memory allocated for `parameters`.
 */
cardano_error_t
cardano_blockfrost_parse_protocol_parameters(
  cardano_provider_impl_t*        provider,
  const char*                     json,
  size_t                          size,
  cardano_protocol_parameters_t** parameters);

/**
 * \brief Parses the unspent transaction outputs (UTXOs) from a Blockfrost API JSON response.
 *
 * This function parses the JSON data returned by the Blockfrost API, representing unspent transaction outputs (UTXOs),
 * and converts it into a \ref cardano_utxo_list_t object.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object that provides context for the parsing operation.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to a character array containing the JSON data returned by the Blockfrost API. This string must not be NULL.
 * \param[in] size The size of the \p json string in bytes.
 * \param[out] utxo_list On successful parsing, this will point to a newly created \ref cardano_utxo_list_t object containing the list of UTXOs parsed from the JSON.
 *                       The caller is responsible for managing the lifecycle of this object and must call \ref cardano_utxo_list_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the UTXOs were successfully parsed,
 *         or an appropriate error code if an error occurred (e.g., if the JSON format is invalid or if the UTXO data is missing).
 */
cardano_error_t
cardano_blockfrost_parse_unspent_outputs(
  cardano_provider_impl_t* provider,
  const char*              json,
  size_t                   size,
  cardano_utxo_list_t**    utxo_list);

/**
 * \brief Parses unspent transaction outputs (UTXOs) from a Blockfrost API response for a given transaction hash.
 *
 * This function parses the JSON response from the Blockfrost API to extract the UTXOs associated with the specified transaction hash. The results
 * are stored in a UTXO list.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for parsing the UTXOs.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to a JSON string representing the Blockfrost API response containing the UTXOs. This parameter must not be NULL.
 * \param[in] size The size of the JSON string in bytes.
 * \param[in] tx_hash A pointer to a null-terminated string representing the transaction hash for which to retrieve UTXOs. This parameter must not be NULL.
 * \param[in] tx_hash_len The length of the \p tx_hash string.
 * \param[out] utxo_list A pointer to a pointer of \ref cardano_utxo_list_t that will be populated with the UTXOs associated with the given transaction hash.
 *                       The caller must manage the lifecycle of this list and free it when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the UTXOs were successfully parsed,
 *         or an appropriate error code if an error occurred (e.g., invalid JSON format, failure to parse the UTXOs, or invalid transaction hash).
 */
cardano_error_t
cardano_blockfrost_parse_tx_unspent_outputs(
  cardano_provider_impl_t* provider,
  const char*              json,
  size_t                   size,
  const char*              tx_hash,
  size_t                   tx_hash_len,
  cardano_utxo_list_t**    utxo_list);

/**
 * \brief Retrieves a script from the Blockfrost API using a script hash.
 *
 * This function fetches a Cardano script from the Blockfrost API by querying with a provided script hash. The script is returned as a \ref cardano_script_t object.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that contains the necessary context for querying the Blockfrost API.
 *                          This parameter must not be NULL.
 * \param[in] script_hash A pointer to a character array representing the script hash, which is used to identify the script on the blockchain.
 *                        This string must be null-terminated.
 * \param[in] script_hash_len The length of the \p script_hash string.
 * \param[out] script On successful execution, this will point to a newly created \ref cardano_script_t object representing the script retrieved from the Blockfrost API.
 *                    The caller is responsible for managing the lifecycle of this object and must call \ref cardano_script_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script was successfully retrieved,
 *         or an appropriate error code if an error occurred (e.g., if the API request failed or the script hash was invalid).
 */
cardano_error_t
cardano_blockfrost_get_script(
  cardano_provider_impl_t* provider_impl,
  const char*              script_hash,
  size_t                   script_hash_len,
  cardano_script_t**       script);

/**
 * \brief Parses rewards data from a Blockfrost API JSON response.
 *
 * This function parses the JSON data returned by the Blockfrost API, which contains staking rewards information, and retrieves the total rewards amount.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object that provides context for the parsing operation.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to a character array containing the JSON data returned by the Blockfrost API. This string must not be NULL.
 * \param[in] size The size of the \p json string in bytes.
 * \param[out] rewards On successful parsing, this will contain the total rewards amount as a 64-bit unsigned integer. The caller must ensure this pointer is valid.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the rewards were successfully parsed,
 *         or an appropriate error code if an error occurred (e.g., if the JSON format is invalid or if the rewards data is missing).
 */
cardano_error_t
cardano_blockfrost_parse_rewards(
  cardano_provider_impl_t* provider,
  const char*              json,
  size_t                   size,
  uint64_t*                rewards);

/**
 * \brief Parses a datum from a Blockfrost API JSON response.
 *
 * This function parses the JSON representation of a Plutus datum returned by the Blockfrost API and constructs a corresponding
 * \ref cardano_plutus_data_t object.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object that provides necessary context for parsing.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to a JSON-encoded string representing the datum. This parameter must not be NULL.
 * \param[in] size The size of the JSON string.
 * \param[out] datum A pointer to a pointer of \ref cardano_plutus_data_t that will be populated with the parsed datum. The caller
 *                   is responsible for managing the lifecycle of the returned datum and must release it using \ref cardano_plutus_data_unref
 *                   when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the datum was successfully parsed,
 *         or an appropriate error code if an error occurred (e.g., invalid JSON format or failure to parse the datum).
 */
cardano_error_t
cardano_blockfrost_parse_datum(
  cardano_provider_impl_t* provider,
  const char*              json,
  size_t                   size,
  cardano_plutus_data_t**  datum);

/**
 * \brief Serializes transaction evaluation parameters and UTXOs to a JSON string.
 *
 * This function serializes the provided transaction evaluation parameters, including the transaction itself and associated UTXO list, into a JSON string.
 *
 * \param[in] transaction A pointer to an initialized \ref cardano_transaction_t object representing the transaction to be evaluated.
 *                        This parameter must not be NULL.
 * \param[in] utxos A pointer to an initialized \ref cardano_utxo_list_t object representing the UTXOs associated with the transaction.
 *                  This parameter must not be NULL.
 * \param[out] json_main_obj_str On success, this will point to the allocated JSON string that contains the serialized data.
 *                               The caller is responsible for freeing this string using `free()` when no longer needed.
 * \param[out] json_main_obj_size On success, this will be set to the size of the allocated JSON string, including the null terminator.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *
 * \note The caller must ensure to free the memory allocated for the JSON string when it is no longer needed.
 */
cardano_error_t
cardano_evaluate_params_to_json(
  cardano_transaction_t* transaction,
  cardano_utxo_list_t*   utxos,
  char**                 json_main_obj_str,
  size_t*                json_main_obj_size);

/**
 * \brief Parses a transaction evaluation response from Blockfrost and updates redeemer execution units.
 *
 * This function parses the JSON response returned by Blockfrost after evaluating a transaction and extracts the updated redeemer execution units.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object representing the Blockfrost provider.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to the JSON string containing the Blockfrost response for the transaction evaluation. This must be a valid JSON string.
 * \param[in] size The size of the JSON string, in bytes.
 * \param[in] original_redeemers A pointer to an initialized \ref cardano_redeemer_list_t object representing the original redeemers.
 *                               This parameter must not be NULL.
 * \param[out] redeemers On success, this will point to a newly created \ref cardano_redeemer_list_t object containing the updated redeemers
 *                       with the applied execution units from the Blockfrost response. The caller is responsible for managing the lifecycle of
 *                       this object, including calling \ref cardano_redeemer_list_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the redeemers were successfully updated,
 *         or an appropriate error code such as \ref CARDANO_POINTER_IS_NULL if any of the input pointers are NULL, or \ref CARDANO_ERROR_JSON_PARSE
 *         if the JSON parsing fails.
 *
 * \note The caller must manage the memory of the \p redeemers object, ensuring it is properly released when no longer needed.
 */
cardano_error_t
cardano_blockfrost_parse_tx_eval_response(
  cardano_provider_impl_t*  provider,
  const char*               json,
  size_t                    size,
  cardano_redeemer_list_t*  original_redeemers,
  cardano_redeemer_list_t** redeemers);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_PARSERS_H
