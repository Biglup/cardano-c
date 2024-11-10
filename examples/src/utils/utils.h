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
 * @param milliseconds Number of milliseconds to sleep.
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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UTILS_H
