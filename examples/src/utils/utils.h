/**
 * \file utils.h
 *
 * \author luisd.bianchi
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UTILS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UTILS_H

/* INCLUDES ******************************************************************/

#include "cardano/common/drep.h"
#include "cardano/key_handlers/secure_key_handler.h"
#include "cardano/key_handlers/software_secure_key_handler.h"
#include "console.h"
#include <cardano/error.h>
#include <cardano/providers/provider.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Safe version of memcpy that copies up to buffer size into the destination buffer.
 *
 * \param dest Destination buffer.
 * \param dest_size Size of the destination buffer.
 * \param src Source buffer.
 * \param src_size Number of bytes to copy.
 */
void
cardano_utils_safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size);

/**
 * \brief Safe version of strlen that limits the maximum number of characters to inspect.
 *
 * \param str The string to measure.
 * \param max_length Maximum number of characters to inspect.
 *
 * \return Length of the string or \c max_length if the string exceeds max_length.
 */
size_t
cardano_utils_safe_strlen(const char* str, size_t max_length);

/**
 * \brief Sets a NULL terminated message as the inner error message of a provider implementation object.
 *
 * \param provider_impl Provider implementation object.
 * \param message Error message to set.
 */
void
cardano_utils_set_error_message(cardano_provider_impl_t* provider_impl, const char* message);

/**
 * \brief Retrieves the current timestamp in seconds.
 *
 * This function returns the current time as the number of seconds since the Unix epoch (January 1, 1970).
 *
 * \return The current time as a \c uint64_t representing the Unix timestamp in seconds.
 */
uint64_t
cardano_utils_get_time(void);

/**
 * \brief Calculates the elapsed time in seconds since a given start time.
 *
 * This function computes the elapsed time in seconds between a specified start time and the current time.
 *
 * \param[in] start A \c uint64_t representing the start time as a Unix timestamp (in seconds).
 *
 * \return The elapsed time in seconds as a \c uint64_t. If the current time is less than the start time (unlikely under normal circumstances),
 *         the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t start_time = cardano_utils_get_time();
 * // Perform some operations...
 * uint64_t elapsed_time = cardano_utils_get_elapsed_time_since(start_time);
 * printf("Elapsed time: %llu seconds\n", elapsed_time);
 * \endcode
 */
uint64_t
cardano_utils_get_elapsed_time_since(uint64_t start);

/**
 * \brief Suspends the execution of the current thread for a specified number of milliseconds.
 *
 * \param milliseconds Number of milliseconds to sleep.
 */
void
cardano_utils_sleep(uint64_t milliseconds);

// Address creation functions

/**
 * \brief Creates a Cardano address object from derivation paths.
 *
 * This function generates a \ref cardano_address_t object using the provided derivation paths, which specify the
 * account, payment, and stake key indices. It leverages a secure key handler to derive the address based on
 * hierarchical deterministic (HD) paths.
 *
 * \param[in] key_handler A pointer to the initialized \ref cardano_secure_key_handler_t, which manages the secure
 *                        key derivation and cryptographic operations. This parameter must not be NULL.
 * \param[in] account_path The derivation path for the account level (e.g., purpose'/coin_type'/account').
 *                         This specifies the base path for generating child keys.
 * \param[in] payment_index The index in the derivation path for the payment key, which defines the address's payment credential.
 * \param[in] stake_key_index The index for the stake key in the derivation path, defining the address's stake credential.
 *
 * \return A pointer to the newly created \ref cardano_address_t object representing the derived address.
 *         Returns NULL if the address creation fails due to an invalid path, key handler issue, or internal error.
 *
 * \note This function will exit the program if an error occurs during the address derivation process.
 */
cardano_address_t* create_address_from_derivation_paths(
  cardano_secure_key_handler_t*     key_handler,
  cardano_account_derivation_path_t account_path,
  uint32_t                          payment_index,
  uint32_t                          stake_key_index);

/**
 * \brief Creates a Cardano reward address object from a string representation.
 *
 * This function creates a \ref cardano_reward_address_t object from the provided reward address string. The address string
 * must be a valid Bech32-encoded Cardano reward address. The function validates the format of the address and, if valid,
 * initializes the corresponding reward address object.
 *
 * \param[in] address_str A pointer to the reward address string in Bech32 format. This parameter must not be NULL.
 * \param[in] address_str_length The length of the reward address string in bytes.
 *
 * \return A pointer to the newly created \ref cardano_reward_address_t object if the address is valid. Returns NULL if the
 * address creation fails due to an invalid format or any internal error.
 *
 * \note This function will exit the program if an error occurs during the reward address creation process.
 */
cardano_reward_address_t* create_reward_address(const char* address_str, size_t address_str_length);

/**
 * \brief Creates a Cardano DRep object from a string representation.
 *
 * This function creates a \ref cardano_drep_t object from the provided DRep ID string. The string
 * must be a valid Bech32-encoded Cardano DRep ID (either CIP-105 or CIP-129). The function validates the format of the ID and, if valid,
 * initializes the corresponding cardano_drep_t object.
 *
 * \param[in] key_handler A pointer to the initialized \ref cardano_secure_key_handler_t.
 * \param[in] account_path The derivation path for the account level.
 *
 * \return A pointer to the newly created \ref cardano_drep_t object if the ID is valid. Returns NULL if the
 * address creation fails due to an invalid format or any internal error.
 *
 * \note This function will exit the program if an error occurs during the DRep creation process.
 */
cardano_drep_t* create_drep_from_derivation_path(cardano_secure_key_handler_t* key_handler, cardano_account_derivation_path_t account_path);

// Key handler functions

/**
 * \brief Creates a secure key handler for managing cryptographic keys.
 *
 * This function initializes a \ref cardano_secure_key_handler_t object from serialized data. The handler will manage
 * cryptographic key operations, using a passphrase retrieval function to handle encrypted keys securely.
 *
 * \param[in] serialized_data A pointer to the serialized key data. This data must be non-null and contain valid serialized key information.
 * \param[in] length The length of the serialized data in bytes.
 * \param[in] get_passphrase A callback function of type \ref cardano_get_passphrase_func_t that will be used to retrieve the passphrase
 *                           for decrypting the key data if needed. This parameter must not be NULL.
 *
 * \return A pointer to the initialized \ref cardano_secure_key_handler_t if successful, or NULL if initialization fails due to
 *         invalid data, memory allocation issues, or passphrase retrieval errors.
 *
 * \note This function will exit the program if an error occurs during the secure key handler creation process.
 */
cardano_secure_key_handler_t* create_secure_key_handler(const char* serialized_data, size_t length, cardano_get_passphrase_func_t get_passphras);

// Provider functions

/**
 * \brief Creates a provider for interacting with the Cardano network.
 *
 * This function initializes a \ref cardano_provider_t object configured with the specified network magic and API key,
 * which enables the provider to connect and interact with the designated Cardano network.
 *
 * \param[in] network_magic The unique identifier for the Cardano network (e.g., 1 for mainnet).
 * \param[in] api_key A pointer to a null-terminated string containing the API key required for network access. This parameter must not be NULL.
 *
 * \return A pointer to the newly created \ref cardano_provider_t object if initialization succeeds. Returns NULL if creation fails due to
 *         invalid parameters or other internal errors.
 *
 * \note This function will exit the program if an error occurs during the secure key handler creation process.
 */
cardano_provider_t* create_provider(uint32_t network_magic, const char* api_key);

/**
 * \brief Retrieves the list of unspent UTXOs for a specified Cardano address.
 *
 * This function fetches the unspent UTXOs associated with the given Cardano address by using the specified
 * provider, allowing access to the address's available balance and outputs.
 *
 * \param[in] provider A pointer to the initialized \ref cardano_provider_t object used for network communication.
 *                     This parameter must not be NULL.
 * \param[in] address  A pointer to the \ref cardano_address_t object representing the Cardano address for which
 *                     unspent UTXOs are requested. This parameter must not be NULL.
 *
 * \return A pointer to a newly created \ref cardano_utxo_list_t object containing the unspent UTXOs for the specified address.
 *         Returns NULL if retrieval fails due to network errors, invalid parameters, or if no unspent UTXOs are found.
 *
 * \note The caller is responsible for freeing the \ref cardano_utxo_list_t object using `cardano_utxo_list_unref` when it is no longer needed.
 * \note This function will exit the program if an error occurs during the UTXO retrieval process.
 */
cardano_utxo_list_t* get_unspent_utxos(cardano_provider_t* provider, cardano_address_t* address);

/**
 * \brief Retrieves the current Cardano protocol parameters.
 *
 * This function fetches the active protocol parameters from the Cardano blockchain via the provided network provider.
 * The parameters contain important configuration data such as transaction fees, minimum UTXO values, and other
 * protocol-specific settings required for transaction construction and validation.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_t object used to communicate with the Cardano network.
 *                     This parameter must not be NULL.
 *
 * \return A pointer to a newly created \ref cardano_protocol_parameters_t object containing the latest protocol parameters.
 *         Returns NULL if the retrieval fails due to network errors or an invalid provider.
 *
 * \note The caller is responsible for freeing the \ref cardano_protocol_parameters_t object using `cardano_protocol_parameters_unref`
 *       when it is no longer needed.
 * \note This function will exit the program if an error occurs during the protocol parameters retrieval process.
 */
cardano_protocol_parameters_t* get_protocol_parameters(cardano_provider_t* provider);

/**
 * \brief Signs a transaction with a specified signing key.
 *
 * This function applies a digital signature to the given transaction using the private key associated with the provided
 * derivation path. The signing process is performed securely through the \ref cardano_secure_key_handler_t, which
 * manages key access and security.
 *
 * \param[in] key_handler A pointer to an initialized \ref cardano_secure_key_handler_t instance that provides access
 *                        to the private signing key. This parameter must not be NULL.
 * \param[in] signer_derivation_path The derivation path for the signer within the key handler, identifying which key to
 *                                   use for signing. This is required to locate the exact private key.
 * \param[in,out] transaction A pointer to the \ref cardano_transaction_t object representing the transaction to be signed.
 *                            This parameter must not be NULL.
 *
 * \return None.
 *
 * \note This function will exit the program if an error occurs during the secure key handler creation process.
 * \note The caller is responsible for ensuring that the transaction object is properly initialized and memory-managed.
 */
void sign_transaction(cardano_secure_key_handler_t* key_handler, cardano_derivation_path_t signer_derivation_path, cardano_transaction_t* transaction);

/**
 * \brief Signs a transaction with a specified number of signing keys.
 *
 * This function applies a digital signature to the given transaction using the private keys associated with the provided
 * derivation paths. The signing process is performed securely through the \ref cardano_secure_key_handler_t, which
 * manages key access and security.
 *
 * \param[in] key_handler A pointer to an initialized \ref cardano_secure_key_handler_t instance that provides access
 *                        to the private signing key. This parameter must not be NULL.
 * \param[in] signer_derivation_path The derivation paths for the signer within the key handler, identifying which keys to
 *                                   use for signing. This is required to locate the exact private key.
 * \param[in] signer_derivation_path_count The number of signer derivation paths provided in the array.
 * \param[in,out] transaction A pointer to the \ref cardano_transaction_t object representing the transaction to be signed.
 *                            This parameter must not be NULL.
 *
 * \return None.
 *
 * \note This function will exit the program if an error occurs during the secure key handler creation process.
 * \note The caller is responsible for ensuring that the transaction object is properly initialized and memory-managed.
 */
void
sign_transaction_with_keys(
  cardano_secure_key_handler_t*    key_handler,
  const cardano_derivation_path_t* signer_derivation_path,
  size_t                           signer_derivation_path_count,
  cardano_transaction_t*           transaction);

/**
 * \brief Submits a transaction to the Cardano network.
 *
 * This function sends a fully constructed and signed \ref cardano_transaction_t to the Cardano network through
 * the specified \ref cardano_provider_t. The function includes a timeout, which defines the maximum time allowed
 * for the transaction to be processed before reporting a failure.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_t instance to facilitate network communication.
 *                     This parameter must not be NULL.
 * \param[in] timeout_ms The maximum time, in milliseconds, to wait for the transaction submission to complete. If the
 *                       timeout elapses, the transaction submission will fail.
 * \param[in] transaction A pointer to the \ref cardano_transaction_t object representing the transaction to be submitted.
 *                        This parameter must not be NULL.
 *
 * \return None.
 *
 * \note Ensure that the transaction is fully signed and ready for submission before calling this function.
 *       The caller is responsible for initializing and managing the memory of the transaction and provider objects.
 * \note This function does not return an error code; however, any errors during submission will result in program exit.
 */
void submit_transaction(cardano_provider_t* provider, uint64_t timeout_ms, cardano_transaction_t* transaction);

// Utils functions

/**
 * \brief Prints a Cardano Blake2b hash with an accompanying message.
 *
 * This function outputs a formatted message followed by the hexadecimal representation of the provided
 * \ref cardano_blake2b_hash_t. This is useful for debugging and logging purposes, where a message and
 * the associated hash value need to be displayed together.
 *
 * \param[in] message A pointer to a null-terminated string that provides context for the printed hash.
 *                    This parameter must not be NULL.
 * \param[in] hash A pointer to a \ref cardano_blake2b_hash_t object containing the hash to print.
 *                 This parameter must not be NULL.
 *
 * \return None.
 *
 * \note Ensure that the hash is initialized and valid before calling this function.
 * \note This function will exit the program if the hash is NULL or invalid.
 */
void print_hash(const char* message, cardano_blake2b_hash_t* hash);

/**
 * \brief Resolves a UTXO input from a transaction ID and index.
 *
 * This function retrieves a specific UTXO (Unspent Transaction Output) from a transaction by its ID and output index,
 * using the given \ref cardano_provider_t instance to query the blockchain. The resolved UTXO can then be used as an input
 * in transaction construction.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_t instance that provides access to the blockchain data.
 * \param[in] tx_id A pointer to the transaction ID as a byte array. This parameter must not be NULL.
 * \param[in] tx_id_size The size of the transaction ID byte array.
 * \param[in] index The index of the desired UTXO in the transaction’s output list.
 *
 * \return A pointer to the resolved \ref cardano_utxo_t object representing the specified transaction output.
 *         Returns NULL if the UTXO cannot be resolved or an error occurs.
 */
cardano_utxo_t*
cardano_resolve_input(cardano_provider_t* provider, const char* tx_id, size_t tx_id_size, uint32_t index);

/**
 * \brief Creates a Plutus V2 script object from a hexadecimal string representation.
 *
 * This function creates a \ref cardano_script_t object representing a Plutus V2 script from the provided hexadecimal string.
 * The hexadecimal string should encode a valid Plutus V2 script in CBOR format, which is then parsed and loaded into a
 * Cardano script object.
 *
 * \param[in] script_hex A pointer to a NULL-terminated string containing the hexadecimal representation of the Plutus V2 script.
 *                       This parameter must not be NULL.
 *
 * \return A pointer to the created \ref cardano_script_t object if the script is successfully parsed. Returns NULL if the
 *         script creation fails due to an invalid format or internal error.
 */
cardano_script_t*
create_plutus_v2_script_from_hex(const char* script_hex);

/**
 * \brief Creates a Plutus V3 script object from a hexadecimal string representation.
 *
 * This function creates a \ref cardano_script_t object representing a Plutus V3 script from the provided hexadecimal string.
 * The hexadecimal string should encode a valid Plutus V3 script in CBOR format, which is then parsed and loaded into a
 * Cardano script object.
 *
 * \param[in] script_hex A pointer to a NULL-terminated string containing the hexadecimal representation of the Plutus V3 script.
 *                       This parameter must not be NULL.
 *
 * \return A pointer to the created \ref cardano_script_t object if the script is successfully parsed. Returns NULL if the
 *         script creation fails due to an invalid format or internal error.
 */
cardano_script_t*
create_plutus_v3_script_from_hex(const char* script_hex);

/**
 * \brief Creates a Native script object from a JSON string representation.
 *
 * This function creates a \ref cardano_script_t object representing a Native script from the provided JSON string.
 *
 * \param[in] json A pointer to a NULL-terminated string containing the JSON representation of the Native script.
 *                 his parameter must not be NULL.
 *
 * \return A pointer to the created \ref cardano_script_t object if the script is successfully parsed. Returns NULL if the
 *         script creation fails due to an invalid format or internal error.
 */
cardano_script_t*
create_native_script_from_json(const char* json);

/**
 * \brief Creates a Cardano asset name from a string.
 * \param name The asset name as a string.
 *
 * \return A pointer to the created \ref cardano_asset_name_t object if the asset name is successfully created.
 */
cardano_asset_name_t*
create_asset_name_from_string(const char* name);

/**
 * \brief Generates a Cardano script address from a given script.
 *
 * This function creates a \ref cardano_address_t object representing the address associated with a provided
 * \ref cardano_script_t.
 *
 * \param[in] script A pointer to a \ref cardano_script_t object representing the script for which the address is generated.
 *                   This parameter must not be NULL.
 *
 * \return A pointer to the created \ref cardano_address_t object if the address is successfully generated.
 *         Returns NULL if the address creation fails due to an invalid or unsupported script format.
 */
cardano_address_t* get_script_address(cardano_script_t* script);

/**
 * \brief Generates a Cardano script stake address from a given script.
 *
 * This function creates a \ref cardano_address_t object representing the address associated with a provided
 * \ref cardano_script_t.
 *
 * \param[in] script A pointer to a \ref cardano_script_t object representing the script for which the address is generated.
 *                   This parameter must not be NULL.
 *
 * \return A pointer to the created \ref cardano_address_t object if the address is successfully generated.
 *         Returns NULL if the address creation fails due to an invalid or unsupported script format.
 */
cardano_reward_address_t* get_script_stake_address(cardano_script_t* script);

/**
 * \brief Derives the DRep ID from a given script.
 *
 * \param script The script from which to derive the DRep ID.
 *
 * \return A pointer to the \ref cardano_drep_t object representing the script DRep ID.
 */
cardano_drep_t* get_script_drep(cardano_script_t* script);

/**
 * \brief Creates a Cardano datum from an zero initialized integer value.
 *
 * \return A pointer to the created \ref cardano_datum_t object if the datum is successfully created.
 *         Returns NULL if memory allocation fails or an internal error occurs.
 */
cardano_datum_t* create_void_datum();

/**
 * \brief Retrieves the Cardano burn address.
 *
 * This function returns the predefined burn address, `addr_test1wza7ec20249sqg87yu2aqkqp735qa02q6yd93u28gzul93gvc4wuw`.
 *
 * Tokens sent to this address cannot be spent or recovered, effectively removing them from circulation.
 *
 * The burn address is configured to always fail script validation, as demonstrated in the `locked_script.json` file:
 *
 * \code{.json}
 * {
 *   "type": "all",
 *   "scripts":
 *   [
 *     {
 *       "type": "before",
 *       "slot": 0
 *     }
 *   ]
 * }
 * \endcode
 *
 * This script validates only when the current slot is before 0, ensuring the funds are permanently locked.
 *
 * \return A pointer to the \ref cardano_address_t object representing the burn address.
 *
 * \note The caller is responsible for releasing the \ref cardano_address_t object when it is no longer needed by using \ref cardano_address_unref.
 */
cardano_address_t* get_brun_address();

/**
 * \brief Creates a transaction output with a reference script.
 *
 * This function initializes a new \ref cardano_transaction_output_t object for the specified `address` with the given `amount` in lovelace. Additionally, it associates a reference `script`, which can be used to validate this output without requiring the script to be fully included in every transaction.
 *
 * \param[in] address A pointer to a \ref cardano_address_t object representing the recipient address for the transaction output. This parameter must not be NULL.
 * \param[in] amount The amount of lovelace to be assigned to the transaction output.
 * \param[in] script A pointer to a \ref cardano_script_t object representing the reference script for the output. This parameter can be NULL if no reference script is required.
 *
 * \return A pointer to the newly created \ref cardano_transaction_output_t object if the output is successfully created; otherwise, NULL if memory allocation fails or if the parameters are invalid.
 *
 * \note The caller is responsible for managing the lifecycle of the created \ref cardano_transaction_output_t, releasing it when it is no longer needed by calling \ref cardano_transaction_output_unref.
 */
cardano_transaction_output_t*
create_output_with_ref_script(cardano_address_t* address, uint32_t amount, cardano_script_t* script);

/**
 * \brief Creates a void plutus data.
 *
 * This function initializes a \ref cardano_plutus_data_t object with zero initialized 32-bit integer value.
 *
 * \return A pointer to the created \ref cardano_plutus_data_t object if the datum is successfully created.
 *         Returns NULL if memory allocation fails or an internal error occurs.
 */
cardano_plutus_data_t* create_void_plutus_data();

/**
 * \brief Creates a Cardano transaction input from a UTXO.
 *
 * \param tx_id The transaction ID of the UTXO.
 * \param index The index of the UTXO in the transaction's output list.
 * \param output The output of the UTXO.
 *
 * \return A pointer to the created \ref cardano_transaction_input_t object if the input is successfully created.
 */
cardano_utxo_t*
create_utxo(cardano_blake2b_hash_t* tx_id, uint32_t index, cardano_transaction_output_t* output);

/**
 * \brief Gets a utxo from the given transaction at the given index.
 *
 * \param transaction The transaction from which to get the utxo.
 * \param index The index of the utxo in the transaction's output list.
 * \return A pointer to the \ref cardano_utxo_t object representing the utxo at the specified index.
 */
cardano_utxo_t*
get_utxo_at_index(cardano_transaction_t* transaction, uint32_t index);

/**
 * \brief Creates a Cardano DRep voter object from a DRep ID.
 *
 * This function creates a \ref cardano_voter_t object representing a DRep voter from the provided DRep ID.
 *
 * \param drep_id The DRep ID as a string.
 *
 * \return A pointer to the created \ref cardano_voter_t object if the DRep voter is successfully created.
 */
cardano_voter_t*
create_drep_voter(const char* drep_id);

/**
 * \brief Creates a Cardano governance action ID object.
 *
 * This function creates a \ref cardano_governance_action_id_t object representing a governance action ID from the provided hexadecimal string and index.
 *
 * \param gov_id_hex The hexadecimal string representing the governance action ID.
 * \param index The index of the governance action.
 *
 * \return A pointer to the created \ref cardano_governance_action_id_t object if the governance action ID is successfully created.
 */
cardano_governance_action_id_t*
create_governance_id(const char* gov_id_hex, uint64_t index);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UTILS_H
