/**
 * \file provider.h
 *
 * \author angel.castillo
 * \date   Sep 27, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>

#include <cardano/providers/provider_impl.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Opaque structure representing a Cardano blockchain data provider instance.
 *
 * This structure serves as a handle to a Cardano provider, encapsulating the necessary context and
 * state required to interact with the Cardano blockchain.
 */
typedef struct cardano_provider_t cardano_provider_t;

/**
 * \brief Creates a new `cardano_provider_t` object using the provided implementation.
 *
 * This function initializes a new \ref cardano_provider_t object by wrapping the given
 * \ref cardano_provider_impl_t implementation. The newly created provider object manages
 * the lifecycle of the underlying implementation and provides an interface for interacting
 * with the Cardano blockchain functionalities.
 *
 * \param[in]  impl     The provider implementation containing function pointers and context.
 * \param[out] provider A pointer to store the address of the newly created provider object.
 *                      This should be a valid pointer to a \ref cardano_provider_t* variable.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *
 * Usage Example:
 * \code{.c}
 * // Assume 'impl' is a valid cardano_provider_impl_t initialized elsewhere.
 * cardano_provider_t* provider = NULL;
 * cardano_error_t result = cardano_provider_new(impl, &provider);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the provider
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * // When done with the provider
 * cardano_provider_unref(&provider);
 * \endcode
 *
 * \note After successfully creating a \ref cardano_provider_t object, you are responsible for
 *       managing its lifecycle. Ensure that you call \ref cardano_provider_unref when the
 *       provider is no longer needed to release resources and prevent memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_provider_new(cardano_provider_impl_t impl, cardano_provider_t** provider);

/**
 * \brief Retrieves the name of the provider implementation.
 *
 * This function returns a constant string representing the name of the provider implementation.
 * The name can be used for logging, debugging, or informational purposes to identify which
 * provider implementation is being used.
 *
 * \param[in] provider Pointer to the \ref cardano_provider_t object.
 *
 * \returns A constant character pointer to the provider's name string.
 *          The returned string is owned by the provider and must not be modified or freed by the caller.
 *          If the \p provider is NULL or invalid, the function may return NULL.
 *
 * Usage Example:
 * \code{.c}
 * const char* provider_name = cardano_provider_get_name(provider);
 *
 * if (provider_name)
 * {
 *   printf("Using provider: %s\n", provider_name);
 * }
 * else
 * {
 *   printf("Failed to retrieve provider name.\n");
 * }
 * \endcode
 *
 * \note The returned string remains valid as long as the \ref cardano_provider_t object is valid.
 *       Do not attempt to modify or free the returned string.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_provider_get_name(const cardano_provider_t* provider);

/**
 * \brief Retrieves the network magic associated with a provider.
 *
 * This function returns the network magic value for the specified Cardano provider (`cardano_provider_t`).
 *
 * \param[in] provider A constant pointer to the \ref cardano_provider_t instance from which to retrieve the network magic.
 *
 * \returns The network magic value (\ref cardano_network_magic_t) associated with the provider.
 *
 * Usage Example:
 * \code{.c}
 * cardano_provider_t* provider = ...;  // Initialized provider
 * cardano_network_magic_t network_magic = cardano_provider_get_network_magic(provider);
 *
 * printf("Network magic: %u\n", network_magic);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_network_magic_t
cardano_provider_get_network_magic(const cardano_provider_t* provider);

/**
 * \brief Retrieves the current protocol parameters from the Cardano blockchain.
 *
 * This function obtains the protocol parameters using the specified provider instance.
 * The protocol parameters include important configuration details of the Cardano network,
 * such as parameters to compute fees, maximum block sizes, and other protocol-related settings.
 *
 * \param[in]  provider   Pointer to the \ref cardano_provider_t instance.
 *                        Must not be `NULL`.
 * \param[out] parameters Pointer to a location where the function will store a pointer
 *                        to the retrieved \ref cardano_protocol_parameters_t structure.
 *                        This parameter must not be `NULL`. On success, the caller is
 *                        responsible for freeing the returned \ref cardano_protocol_parameters_t
 *                        object using the \ref cardano_protocol_parameters_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - An appropriate error code on failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* parameters = NULL;
 * cardano_error_t result = cardano_provider_get_parameters(provider, &parameters);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the parameters
 *   // ...
 *   // When done, free the parameters
 *   cardano_protocol_parameters_unref(parameters);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_provider_get_parameters(cardano_provider_t* provider, cardano_protocol_parameters_t** parameters);

/**
 * \brief Retrieves unspent transaction outputs (UTXOs) for a given address.
 *
 * This function obtains a list of unspent transaction outputs (UTXOs) associated with the specified
 * address using the provided Cardano provider instance. UTXOs represent funds that can be used as
 * inputs in new transactions. Retrieving UTXOs is essential for constructing transactions and
 * managing balances.
 *
 * \param[in]  provider  Pointer to the \ref cardano_provider_t instance.
 *                       Must not be `NULL`.
 * \param[in]  address   Pointer to a \ref cardano_address_t representing the address for which
 *                       to retrieve unspent outputs. Must not be `NULL`.
 * \param[out] utxo_list Pointer to a location where the function will store a pointer to the
 *                       retrieved \ref cardano_utxo_list_t structure. Must not be `NULL`. On success,
 *                       the caller is responsible for freeing the returned \ref cardano_utxo_list_t
 *                       object using the \ref cardano_utxo_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - An appropriate error code on failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = NULL;
 * cardano_error_t result = cardano_provider_get_unspent_outputs(provider, address, &utxo_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Iterate over the UTXOs
 *   size_t utxo_count = cardano_utxo_list_length(utxo_list);
 *
 *   for (size_t i = 0U; i < utxo_count; ++i)
 *   {
 *     // Process the UTXOs
 *     // ...
 *   }
 *   // When done, free the UTXO list
 *   cardano_utxo_list_unref(utxo_list);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_provider_get_unspent_outputs(
  cardano_provider_t*   provider,
  cardano_address_t*    address,
  cardano_utxo_list_t** utxo_list);

/**
 * \brief Retrieves the staking rewards for a given address.
 *
 * This function obtains the current available staking rewards associated with the specified address.
 * It uses the provided Cardano provider instance to access blockchain data and retrieve
 * the reward balance for the address. Staking rewards are accumulated for addresses that
 * delegate their stake to a stake pool.
 *
 * \param[in]  provider Pointer to the \ref cardano_provider_t instance.
 *                      Must not be `NULL`.
 * \param[in]  address  Pointer to a \ref cardano_address_t representing the address
 *                      for which to retrieve staking rewards. Must not be `NULL`.
 * \param[out] rewards  Pointer to a `uint64_t` variable where the function will store
 *                      the amount of rewards (in Lovelace). Must not be `NULL`.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 *
 * Example:
 * \code{.c}
 * uint64_t rewards = 0;
 *
 * cardano_error_t result = cardano_provider_get_rewards_available(provider, address, &rewards)
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the rewards value
 *   printf("Rewards: %ul Lovelace\n", rewards);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 *
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_provider_get_rewards_available(cardano_provider_t* provider, cardano_reward_address_t* address, uint64_t* rewards);

/**
 * \brief Retrieves unspent transaction outputs (UTXOs) for a given address that contain a specific asset.
 *
 * This function obtains a list of unspent transaction outputs (UTXOs) associated with the specified
 * address that contain the specified asset. It uses the provided Cardano provider instance to access
 * the blockchain data. UTXOs represent funds that can be used as inputs in new transactions.
 *
 * \param[in]  provider  Pointer to the \ref cardano_provider_t instance.
 *                       Must not be `NULL`.
 * \param[in]  address   Pointer to a \ref cardano_address_t representing the address for which
 *                       to retrieve unspent outputs. Must not be `NULL`.
 * \param[in]  asset_id  Pointer to a \ref cardano_asset_id_t representing the asset identifier
 *                       used to filter the UTXOs. Must not be `NULL`.
 * \param[out] utxo_list Pointer to a location where the function will store a pointer to the
 *                       retrieved \ref cardano_utxo_list_t structure containing UTXOs that include
 *                       the specified asset. Must not be `NULL`. On success, the caller is responsible
 *                       for freeing the returned \ref cardano_utxo_list_t object using the \ref cardano_utxo_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - Other error codes for provider-specific failures.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = NULL;
 * cardano_error_t result = cardano_provider_get_unspent_outputs_with_asset(provider, address, asset_id, &utxo_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Iterate over the UTXOs
 *   size_t utxo_count = cardano_utxo_list_length(utxo_list);
 *
 *   for (size_t i = 0U; i < utxo_count; ++i)
 *   {
 *     // Process the UTXO
 *     // ...
 *   }
 *
 *   // When done, free the UTXO list
 *   cardano_utxo_list_unref(utxo_list);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_provider_get_unspent_outputs_with_asset(
  cardano_provider_t*   provider,
  cardano_address_t*    address,
  cardano_asset_id_t*   asset_id,
  cardano_utxo_list_t** utxo_list);

/**
 * \brief Retrieves an unspent transaction output (UTXO) containing a specific Non-Fungible Token (NFT).
 *
 * This function obtains a UTXO that contains the specified NFT (Non-Fungible Token) identified by the given asset ID.
 * It uses the provided Cardano provider instance to access the blockchain data.
 *
 * \param[in]  provider Pointer to the \ref cardano_provider_t instance.
 *                      Must not be `NULL`.
 * \param[in]  asset_id Pointer to a \ref cardano_asset_id_t representing the asset identifier of the NFT.
 *                      Must not be `NULL`.
 * \param[out] utxo     Pointer to a location where the function will store a pointer to the retrieved
 *                      \ref cardano_utxo_t structure containing the NFT. Must not be `NULL`.
 *                      On success, the caller is responsible for freeing the returned \ref cardano_utxo_t
 *                      object using the \ref cardano_utxo_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - Other error codes for provider-specific failures.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo = NULL;
 * cardano_error_t result = cardano_provider_get_unspent_output_by_nft(provider, asset_id, &utxo);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the UTXO containing the NFT
 *   // ...
 *   // When done, free the UTXO
 *   cardano_utxo_unref(utxo);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_provider_get_unspent_output_by_nft(
  cardano_provider_t* provider,
  cardano_asset_id_t* asset_id,
  cardano_utxo_t**    utxo);

/**
 * \brief Resolves unspent transaction outputs (UTXOs) for given transaction inputs.
 *
 * This function obtains the unspent transaction outputs (UTXOs) corresponding to the specified
 * set of transaction inputs. It uses the provided Cardano provider instance to access the blockchain
 * data.
 *
 * \param[in]  provider  Pointer to the \ref cardano_provider_t instance.
 *                       Must not be `NULL`.
 * \param[in]  tx_ins    Pointer to a \ref cardano_transaction_input_set_t containing the set of
 *                       transaction inputs to resolve. Must not be `NULL`.
 * \param[out] utxo_list Pointer to a location where the function will store a pointer to the
 *                       retrieved \ref cardano_utxo_list_t structure containing the resolved UTXOs.
 *                       Must not be `NULL`. On success, the caller is responsible for freeing the
 *                       returned \ref cardano_utxo_list_t object using the \ref cardano_utxo_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = NULL;
 * cardano_error_t result = cardano_provider_resolve_unspent_outputs(provider, tx_ins, &utxo_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Iterate over the resolved UTXOs
 *   size_t utxo_count = cardano_utxo_list_length(utxo_list);
 *
 *   for (size_t i = 0; i < utxo_count; ++i)
 *   {
 *     // Process the UTXO
 *     // ...
 *   }
 *
 *   // When done, free the UTXO list
 *   cardano_utxo_list_unref(utxo_list);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_EXPORT cardano_error_t cardano_provider_resolve_unspent_outputs(
  cardano_provider_t*              provider,
  cardano_transaction_input_set_t* tx_ins,
  cardano_utxo_list_t**            utxo_list);

/**
 * \brief Resolves a Plutus datum from its hash.
 *
 * This function retrieves the Plutus datum associated with the given datum hash.
 * It uses the provided Cardano provider instance to access the blockchain data.
 *
 * \param[in]  provider    Pointer to the \ref cardano_provider_t instance.
 *                         Must not be `NULL`.
 * \param[in]  datum_hash  Pointer to a \ref cardano_blake2b_hash_t representing the hash
 *                         of the datum to resolve. Must not be `NULL`.
 * \param[out] datum       Pointer to a location where the function will store a pointer
 *                         to the retrieved \ref cardano_plutus_data_t structure containing
 *                         the datum. Must not be `NULL`. On success, the caller is responsible
 *                         for freeing the returned \ref cardano_plutus_data_t object using
 *                         the \ref cardano_plutus_data_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* datum = NULL;
 * cardano_error_t result = cardano_provider_resolve_datum(provider, datum_hash, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved datum
 *   // ...
 *   // When done, free the datum
 *   cardano_plutus_data_unref(datum);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_EXPORT cardano_error_t
cardano_provider_resolve_datum(cardano_provider_t* provider, cardano_blake2b_hash_t* datum_hash, cardano_plutus_data_t** datum);

/**
 * \brief Confirms the inclusion of a transaction in the blockchain within a specified timeout period.
 *
 * This function waits for the specified transaction to be confirmed (i.e., included in a block)
 * within a given timeout period. It uses the provided Cardano provider instance to monitor
 * the blockchain for the transaction's confirmation.
 *
 * \param[in] provider    Pointer to the \ref cardano_provider_t instance.
 *                        Must not be `NULL`.
 * \param[in] tx_id       Pointer to a \ref cardano_blake2b_hash_t representing the transaction ID
 *                        to confirm. Must not be `NULL`.
 * \param[in] timeout_ms  Timeout in milliseconds to wait for the transaction confirmation.
 *                        If the transaction is not confirmed within this period, the function will return
 *                        an appropriate error code.
 *  \param[out] confirmed  Pointer to a boolean variable where the function will store the confirmation status.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *
 * Usage Example:
 * \code{.c}
 * bool confirmed = false;
 * cardano_error_t result = cardano_provider_confirm_transaction(provider, tx_id, 60000, &confirmed); // Wait up to 60 seconds
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Check if the transaction is confirmed
 *   // Proceed with dependent operations
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_EXPORT cardano_error_t
cardano_provider_confirm_transaction(
  cardano_provider_t*     provider,
  cardano_blake2b_hash_t* tx_id,
  uint64_t                timeout_ms,
  bool*                   confirmed);

/**
 * \brief Submits a transaction to the Cardano blockchain network.
 *
 * This function submits the given transaction to the Cardano network for processing.
 * Upon successful submission, it returns the transaction ID (hash) of the submitted transaction.
 * The transaction ID can be used to track the transaction's status and confirmation.
 *
 * \param[in]  provider Pointer to the \ref cardano_provider_t instance.
 *                      Must not be `NULL`.
 * \param[in]  tx       Pointer to the \ref cardano_transaction_t representing the transaction to submit.
 *                      Must not be `NULL`.
 * \param[out] tx_id    Pointer to a location where the function will store a pointer to the
 *                      newly allocated \ref cardano_blake2b_hash_t representing the transaction ID.
 *                      Must not be `NULL`. On success, the caller is responsible for freeing the
 *                      returned \ref cardano_blake2b_hash_t object using the appropriate deallocation function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* tx_id = NULL;
 * cardano_error_t result = cardano_provider_submit_transaction(provider, tx, &tx_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Transaction submitted successfully
 *   // Use tx_id to track the transaction
 *   // ...
 *   // When done, free the transaction ID
 *   cardano_blake2b_hash_unref(tx_id);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_EXPORT cardano_error_t
cardano_provider_submit_transaction(
  cardano_provider_t*      provider,
  cardano_transaction_t*   tx,
  cardano_blake2b_hash_t** tx_id);

/**
 * \brief Evaluates a transaction to estimate the required execution units for Plutus scripts.
 *
 * This function evaluates the given transaction to determine the execution units (e.g., memory and CPU usage)
 * required by any Plutus scripts involved in the transaction. The evaluation considers any additional UTXOs
 * and redeemers provided.
 *
 * \param[in]  provider         Pointer to the \ref cardano_provider_t instance.
 *                              Must not be `NULL`.
 * \param[in]  tx               Pointer to the \ref cardano_transaction_t representing the transaction to evaluate.
 *                              Must not be `NULL`.
 * \param[in]  additional_utxos Pointer to a \ref cardano_utxo_list_t containing additional UTXOs required
 *                              for evaluation. Can be `NULL` if not needed.
 * \param[out] redeemers        Pointer to a location where the function will store a pointer to the
 *                              \ref cardano_redeemer_list_t containing updated redeemers with estimated
 *                              execution units. Must not be `NULL`. On success, the caller is responsible
 *                              for freeing the returned \ref cardano_redeemer_list_t object using the
 *                              \ref cardano_redeemer_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemers = NULL;
 *
 * cardano_error_t result = cardano_provider_evaluate_transaction(provider, tx, additional_utxos, &redeemers);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the updated redeemers with estimated execution units
 *   // ...
 *   // When done, free the redeemers list
 *   cardano_redeemer_list_unref(redeemers);
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_provider_unref(&provider);
 * \endcode
 */
CARDANO_EXPORT cardano_error_t
cardano_provider_evaluate_transaction(
  cardano_provider_t*       provider,
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      additional_utxos,
  cardano_redeemer_list_t** redeemers);

/**
 * \brief Decrements the reference count of a cardano_provider_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_provider_t object
 * by decreasing its reference count. When the reference count reaches zero, the provider is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] provider A pointer to the pointer of the provider object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_provider_t* provider = cardano_provider_new(major, minor);
 *
 * // Perform operations with the provider...
 *
 * cardano_provider_unref(&provider);
 * // At this point, provider is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_provider_unref, the pointer to the \ref cardano_provider_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_provider_unref(cardano_provider_t** provider);

/**
 * \brief Increases the reference count of the cardano_provider_t object.
 *
 * This function is used to manually increment the reference count of an cardano_provider_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_provider_unref.
 *
 * \param provider A pointer to the cardano_provider_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming provider is a previously created provider object
 *
 * cardano_provider_ref(provider);
 *
 * // Now provider can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_provider_ref there is a corresponding
 * call to \ref cardano_provider_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_provider_ref(cardano_provider_t* provider);

/**
 * \brief Retrieves the current reference count of the cardano_provider_t object.
 *
 * This function returns the number of active references to an cardano_provider_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_provider_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param provider A pointer to the cardano_provider_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_provider_t object. If the object
 * is properly managed (i.e., every \ref cardano_provider_ref call is matched with a
 * \ref cardano_provider_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming provider is a previously created provider object
 *
 * size_t ref_count = cardano_provider_refcount(provider);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_provider_refcount(const cardano_provider_t* provider);

/**
 * \brief Sets the last error message for a given cardano_provider_t object.
 *
 * Records an error message in the provider's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] provider A pointer to the \ref cardano_provider_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the provider's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_provider_set_last_error(
  cardano_provider_t* provider,
  const char*         message);

/**
 * \brief Retrieves the last error message recorded for a specific provider.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_provider_set_last_error for the given
 * provider. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] provider A pointer to the \ref cardano_provider_t instance whose last error
 *                   message is to be retrieved. If the provider is NULL, the function
 *                   returns a generic error message indicating the null provider.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified provider. If the provider is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_provider_set_last_error for the same provider, or until
 *       the provider is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_provider_get_last_error(const cardano_provider_t* provider);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_H