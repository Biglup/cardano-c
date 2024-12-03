/**
 * \file transaction_builder.h
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BUILDER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BUILDER_H

/* INCLUDES ******************************************************************/

#include <cardano/common/drep.h>
#include <cardano/error.h>
#include <cardano/proposal_procedures/constitution.h>
#include <cardano/providers/provider.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief High-Level Transaction Builder for Cardano Blockchain.
 *
 * The `cardano_tx_builder_t` type serves as a comprehensive interface for constructing
 * Cardano transactions programmatically encapsulating the complexities of transaction assembly,
 * balancing, and validation.
 *
 * **Key Features:**
 * - **Modular Design**: Incrementally add inputs, outputs, certificates, metadata, and scripts,
 *   enabling flexible transaction construction.
 * - **Automatic Fee Calculation and Balancing**: Automatically calculates fees and ensures the
 *   transaction is balanced according to Cardano's protocol parameters.
 * - **Support for Advanced Constructs**: Facilitates multi-asset transactions, Plutus smart
 *   contracts, token minting/burning, and governance actions.
 * - **Extensibility**: Allows for custom coin selection strategies.
 */
typedef struct cardano_tx_builder_t cardano_tx_builder_t;

/**
 * \brief Creates a new transaction builder instance.
 *
 * This function initializes a new transaction builder (`cardano_tx_builder_t`) using the specified
 * protocol parameters and provider. The builder enables incremental construction of a transaction
 * while ensuring it adheres to protocol rules and balances according to the specified parameters.
 *
 * \param[in] params A pointer to the \ref cardano_protocol_parameters_t structure containing the
 *                   protocol parameters to configure the transaction builder.
 * \param[in] provider A pointer to the \ref cardano_provider_t instance, which supplies the builder
 *                     with necessary external data and services during transaction construction.
 *
 * \returns A pointer to a newly created \ref cardano_tx_builder_t instance configured with the given
 *          protocol parameters and provider, or `NULL` if memory allocation fails or if `params` or
 *          `provider` is invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...;  // Initialize protocol parameters
 * cardano_provider_t* provider = ...;  // Initialize provider instance
 * cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(protocol_params, provider);
 *
 * if (tx_builder != NULL)
 * {
 *   // Proceed with building the transaction
 * }
 *
 * cardano_tx_builder_unref(&tx_builder);
 * \endcode
 *
 * \note The caller is responsible for freeing the `cardano_tx_builder_t` instance when it is no
 *       longer needed by using the appropriate cleanup function.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_tx_builder_t* cardano_tx_builder_new(
  cardano_protocol_parameters_t* params,
  cardano_provider_t*            provider);

/**
 * \brief Sets the coin selector for the transaction builder.
 *
 * This function assigns a specific coin selection strategy to the transaction builder. The coin
 * selector determines how UTXOs are selected to cover the transaction's required value. Any errors
 * related to coin selection configuration or validation are deferred and will be reported only when
 * \ref cardano_tx_builder_build is called.
 *
 * If this function is not called, the default coin selector (large first coin selector) will be used.
 *
 * \param[in] builder      A pointer to the \ref cardano_tx_builder_t instance where the coin selector
 *                         is to be set.
 * \param[in] coin_selector A pointer to the \ref cardano_coin_selector_t structure that specifies the
 *                          coin selection strategy.
 *
 * \note Errors arising from coin selector configuration or compatibility will be deferred until the
 *       transaction is finalized with \ref cardano_tx_builder_build. This deferred error handling
 *       allows for smoother configuration and incremental transaction building.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_coin_selector_t* coin_selector = ...;  // Initialized coin selector
 * cardano_tx_builder_set_coin_selector(tx_builder, coin_selector);
 *
 * ...
 *
 * cardano_error_t result = cardano_tx_builder_build(tx_builder);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   const char* error_msg = cardano_tx_builder_get_last_error(tx_builder);
 *   printf("Failed to build transaction: %s\n", error_msg);
 * }
 *
 *  cardano_tx_builder_unref(&tx_builder);
 * \endcode
 *
 * \sa cardano_tx_builder_build
 */
CARDANO_EXPORT void cardano_tx_builder_set_coin_selector(
  cardano_tx_builder_t*    builder,
  cardano_coin_selector_t* coin_selector);

/**
 * \brief Sets the transaction evaluator for a transaction builder.
 *
 * This function assigns a transaction evaluator to the specified transaction builder. The evaluator will be used
 * to compute the execution units required for evaluating transactions. It enables users to incorporate custom
 * evaluation logic during transaction creation.
 *
 * If this function is not called, the default evaluator (provider tx evaluator) will be used.
 *
 * Note: This function defers error reporting until the transaction is built using \ref cardano_tx_builder_build.
 * If an issue arises during evaluator setup, it will be reported at that stage.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance to which the evaluator will be set.
 * \param[in] tx_evaluator A pointer to the \ref cardano_tx_evaluator_t instance that will serve as the transaction evaluator.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_tx_evaluator_t* tx_evaluator = ...;  // Initialized transaction evaluator
 * cardano_tx_builder_set_tx_evaluator(tx_builder, tx_evaluator);
 *
 * ...
 *
 * cardano_error_t result = cardano_tx_builder_build(tx_builder);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   const char* error_msg = cardano_tx_builder_get_last_error(tx_builder);
 *   printf("Failed to build transaction: %s\n", error_msg);
 * }
 *
 * cardano_tx_builder_unref(&tx_builder);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_set_tx_evaluator(
  cardano_tx_builder_t*   builder,
  cardano_tx_evaluator_t* tx_evaluator);

/**
 * \brief Sets the network ID for the transaction builder.
 *
 * This function configures the transaction builder (`cardano_tx_builder_t`) with a specific network ID.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance for which the network ID is being set.
 * \param[in] network_id The network ID to assign to the transaction builder. This ID specifies the target network
 *                       (e.g., mainnet, testnet) for which the transaction is intended.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialize or obtain a transaction builder
 * cardano_network_id_t network_id = CARDANO_MAINNET;  // Set the network ID to mainnet
 * cardano_tx_builder_set_network_id(tx_builder, network_id);
 * \endcode
 *
 * \note This function only sets the network ID within the builder and does not perform any validation until
 *       \ref cardano_tx_builder_build is called, which will report any related errors at that time.
 */
CARDANO_EXPORT void cardano_tx_builder_set_network_id(cardano_tx_builder_t* builder, cardano_network_id_t network_id);

/**
 * \brief Sets the change address for the transaction builder.
 *
 * This function specifies the address to which any remaining balance (after covering transaction outputs
 * and fees) will be sent. The address is stored within the transaction builder (`cardano_tx_builder_t`)
 * and applied during the final transaction construction.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance for which the change address is being set.
 * \param[in] change_address A pointer to the \ref cardano_address_t object representing the change address.
 *                           Ownership of this address is not transferred; it must remain valid for the duration
 *                           of the transaction builder's usage.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_address_t* change_addr = ...;    // The change address to be set
 * cardano_tx_builder_set_change_address(tx_builder, change_addr);
 * \endcode
 *
 * \note This function only assigns the change address to the builder; any validation related to the address
 *       will be deferred until \ref cardano_tx_builder_build is called, at which point related errors may be reported.
 */
CARDANO_EXPORT void cardano_tx_builder_set_change_address(cardano_tx_builder_t* builder, cardano_address_t* change_address);

/**
 * \brief Sets the change address for the transaction builder using a raw address string.
 *
 * This function allows specifying the change address directly as a raw string.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance for which the change address is being set.
 * \param[in] change_address A pointer to a character array containing the raw change address in encoded form.
 * \param[in] address_size The size of the change address array.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* raw_change_address = "addr1q...";  // Raw encoded change address
 * size_t address_size = strlen(raw_change_address);
 * cardano_tx_builder_set_change_address_ex(tx_builder, raw_change_address, address_size);
 * \endcode
 *
 * \note This function only assigns the change address to the builder. Any errors related to the validity of
 *       the address are deferred until \ref cardano_tx_builder_build is called, where issues may be reported.
 */
CARDANO_EXPORT void cardano_tx_builder_set_change_address_ex(cardano_tx_builder_t* builder, const char* change_address, size_t address_size);

/**
 * \brief Sets the collateral change address for the transaction builder.
 *
 * This function assigns a collateral change address to the transaction builder (`cardano_tx_builder_t`),
 * specifying where any remaining balance from collateral inputs will be sent.
 * Collateral is used in transactions that contain scripts, acting as a safeguard in case script validation fails.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance for which the collateral change address is being set.
 * \param[in] collateral_change_address A pointer to the \ref cardano_address_t object representing the collateral change address.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_address_t* collateral_address = ...;  // Prepared collateral change address
 * cardano_tx_builder_set_collateral_change_address(tx_builder, collateral_address);
 * \endcode
 *
 * \note This function only sets the collateral change address; any issues or validation errors with the address are deferred until
 *       \ref cardano_tx_builder_build is called, at which point an error may be reported if necessary.
 */
CARDANO_EXPORT void cardano_tx_builder_set_collateral_change_address(cardano_tx_builder_t* builder, cardano_address_t* collateral_change_address);

/**
 * \brief Sets the collateral change address for the transaction builder using a raw address string.
 *
 * This function allows setting the collateral change address as a raw byte string in the transaction builder
 * (`cardano_tx_builder_t`). The collateral change address specifies where any remaining balance from collateral inputs
 * will be sent, used as a safeguard in script-validated transactions.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance for which the collateral change address is being set.
 * \param[in] collateral_change_address A constant pointer to the raw byte string representing the collateral change address.
 * \param[in] address_size The size (in bytes) of the `collateral_change_address` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* collateral_address_str = "...";  // Raw collateral change address as a string
 * size_t address_len = strlen(collateral_address_str);
 * cardano_tx_builder_set_collateral_change_address_ex(tx_builder, collateral_address_str, address_len);
 * \endcode
 *
 * \note This function only assigns the provided address string to the builder; any validation errors
 *       related to the address will be deferred until \ref cardano_tx_builder_build is called.
 */
CARDANO_EXPORT void cardano_tx_builder_set_collateral_change_address_ex(cardano_tx_builder_t* builder, const char* collateral_change_address, size_t address_size);

/**
 * \brief Sets the minimum transaction fee for the transaction builder.
 *
 * This function allows setting a minimum transaction fee for a transaction being constructed with the
 * specified transaction builder (`cardano_tx_builder_t`).
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance where the minimum fee will be set.
 * \param[in] minimum_fee The minimum fee amount, specified in lovelace.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * uint64_t min_fee = 1000000;  // Minimum fee in lovelace
 * cardano_tx_builder_set_minimum_fee(tx_builder, min_fee);
 * \endcode
 *
 * \note Setting this fee does not validate its sufficiency for the transaction; validation will occur when
 *       \ref cardano_tx_builder_build is called, where an error will be reported if the fee is inadequate.
 */
CARDANO_EXPORT void cardano_tx_builder_set_minimum_fee(cardano_tx_builder_t* builder, uint64_t minimum_fee);

/**
 * \brief Sets the available UTXOs for coin selection in transaction balancing.
 *
 * This function assigns a list of available UTXOs (`cardano_utxo_list_t`) to the transaction builder.
 * These UTXOs will be utilized during the coin selection process to meet the transaction's balance requirements.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance that will use the specified UTXOs.
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t containing the UTXOs available for selection.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_utxo_list_t* utxos = ...;        // List of available UTXOs
 * cardano_tx_builder_set_utxos(tx_builder, utxos);
 * \endcode
 *
 * \note This function only sets the UTXOs for potential selection; the actual coin selection will occur
 *       when \ref cardano_tx_builder_build is called, allowing the transaction builder to balance the transaction.
 */
CARDANO_EXPORT void cardano_tx_builder_set_utxos(cardano_tx_builder_t* builder, cardano_utxo_list_t* utxos);

/**
 * \brief Sets the UTXO list for collateral when scripts are included in the transaction.
 *
 * This function assigns a specific list of UTXOs (`cardano_utxo_list_t`) for use as collateral if the transaction
 * involves script execution. If this collateral UTXO list is not set, the transaction builder will default to using
 * the general UTXO list provided via \ref cardano_tx_builder_set_utxos.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance that will utilize the specified UTXOs for collateral.
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t containing the UTXOs designated for collateral.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_utxo_list_t* collateral_utxos = ...;  // UTXOs designated for collateral
 * cardano_tx_builder_set_collateral_utxos(tx_builder, collateral_utxos);
 * \endcode
 *
 * \note Only necessary if the transaction contains scripts that require collateral. Without this setting,
 *       the transaction builder defaults to the general UTXO set.
 */
CARDANO_EXPORT void cardano_tx_builder_set_collateral_utxos(cardano_tx_builder_t* builder, cardano_utxo_list_t* utxos);

/**
 * \brief Sets the expiration slot for a transaction, beyond which it is no longer valid.
 *
 * This function specifies the slot number after which the transaction will be considered invalid. Setting this
 * value helps enforce transaction timeouts, allowing the transaction to expire if not included in a block before
 * the specified slot.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance being configured.
 * \param[in] slot The slot number after which the transaction will be marked as invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * uint64_t expiration_slot = 5000000;  // Slot after which the transaction becomes invalid
 * cardano_tx_builder_set_invalid_after(tx_builder, expiration_slot);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_set_invalid_after(cardano_tx_builder_t* builder, uint64_t slot);

/**
 * \brief Sets the expiration time for a transaction based on Unix time (in seconds).
 *
 * This function configures the transaction's validity based on a specified Unix timestamp, marking the transaction
 * as invalid if it is not included in a block before this time. The Unix timestamp is expressed in seconds since
 * the epoch (January 1, 1970, UTC). This setting is useful for transactions that need to expire at a specific
 * real-world time.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance being configured.
 * \param[in] unix_time The Unix timestamp (in seconds) at which the transaction expires.
 *
 * Usage Example:
 * \code{.c}
 * #include <time.h>
 *
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * time_t current_time = time(NULL);         // Get current Unix time in seconds
 * uint64_t expiration_time = current_time + 2 * 60 * 60;  // Set expiration to 2 hours from now
 *
 * cardano_tx_builder_set_invalid_after_ex(tx_builder, expiration_time);
 * \endcode
 *
 * \note If any errors occur while setting the expiration, they will not be immediately reported. Instead, they will
 *       be deferred and can be checked when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_set_invalid_after_ex(cardano_tx_builder_t* builder, uint64_t unix_time);

/**
 * \brief Sets the minimum valid slot for a transaction.
 *
 * This function configures the earliest slot at which the transaction will be considered valid. Transactions
 * created with this setting will not be valid for inclusion in a block until the specified slot.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance being configured.
 * \param[in] slot The minimum slot number at which the transaction is valid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * uint64_t min_valid_slot = 123456;         // Example slot number
 * cardano_tx_builder_set_invalid_before(tx_builder, min_valid_slot);
 * \endcode
 *
 * \note Any errors arising from setting this parameter will be reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_set_invalid_before(cardano_tx_builder_t* builder, uint64_t slot);

/**
 * \brief Sets the minimum valid time (Unix timestamp) for a transaction.
 *
 * This function configures the earliest time, in Unix timestamp format (seconds since Epoch),
 * at which the transaction will be considered valid. Transactions created with this setting will
 * not be valid for inclusion in a block until the specified timestamp.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance being configured.
 * \param[in] unix_time The minimum Unix timestamp (in seconds) at which the transaction is valid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * uint64_t min_valid_time = 7200 + time(NULL);  // Set to 2 hours from now
 * cardano_tx_builder_set_invalid_before_ex(tx_builder, min_valid_time);
 * \endcode
 *
 * \note Any errors resulting from setting this parameter will be reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_set_invalid_before_ex(cardano_tx_builder_t* builder, uint64_t unix_time);

/**
 * \brief Adds a reference input to the transaction.
 *
 * This function appends a specified UTXO as a reference input to the transaction being built.
 * Reference inputs are utilized by Plutus scripts to access additional data without consuming the UTXO in the transaction.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance in which to add the reference input.
 * \param[in] utxo A pointer to the \ref cardano_utxo_t structure representing the UTXO to be used as a reference input.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_utxo_t* reference_utxo = ...;    // UTXO to add as a reference input
 * cardano_tx_builder_add_reference_input(tx_builder, reference_utxo);
 * \endcode
 *
 * \note Any errors resulting from adding this input will be reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_add_reference_input(cardano_tx_builder_t* builder, cardano_utxo_t* utxo);

/**
 * \brief Sends a specified amount of lovelace to a given address.
 *
 * This function adds a transaction output to the builder, targeting a specific address and setting the amount of lovelace to be sent.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance to which the output will be added.
 * \param[in] address A pointer to the \ref cardano_address_t structure representing the recipient address.
 * \param[in] amount The amount of lovelace to send to the specified address.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;            // Initialized transaction builder
 * cardano_address_t* recipient_address = ...;         // Recipient address
 * uint64_t lovelace_amount = 1000000;                // Amount of lovelace to send (e.g., 1 ADA)
 *
 * cardano_tx_builder_send_lovelace(tx_builder, recipient_address, lovelace_amount);
 * \endcode
 *
 * \note Any errors related to sending lovelace, such as balance requirements or transaction limits, will be deferred and reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_send_lovelace(
  cardano_tx_builder_t* builder,
  cardano_address_t*    address,
  uint64_t              amount);

/**
 * \brief Sends a specified amount of lovelace to a given address in string format.
 *
 * This function adds a transaction output to the builder, targeting a specific address in string format and setting the amount of lovelace to be sent.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance to which the output will be added.
 * \param[in] address A pointer to a character array containing the recipient address in string format.
 * \param[in] address_size The size of the address string.
 * \param[in] amount The amount of lovelace to send to the specified address.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;         // Initialized transaction builder
 * const char* recipient_address = "addr1...";      // Recipient address in string format
 * size_t address_size = strlen(recipient_address); // Size of the address string
 * uint64_t lovelace_amount = 1000000;             // Amount of lovelace to send (e.g., 1 ADA)
 *
 * cardano_tx_builder_send_lovelace_ex(tx_builder, recipient_address, address_size, lovelace_amount);
 * \endcode
 *
 * \note Errors, such as insufficient balance or invalid address, will be deferred and reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_send_lovelace_ex(
  cardano_tx_builder_t* builder,
  const char*           address,
  size_t                address_size,
  uint64_t              amount);

/**
 * \brief Sends a specified value (which may include ADA and other assets) to a given address.
 *
 * This function adds a transaction output to the builder, specifying an address and a complex value that can include multiple assets in addition to ADA.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance to which the output will be added.
 * \param[in] address A pointer to a \ref cardano_address_t object representing the recipient address.
 * \param[in] value A pointer to a \ref cardano_value_t object containing the ADA and any additional assets to send.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_address_t* recipient_address = ...;  // Address to send to
 * cardano_value_t* transfer_value = ...;  // Value to transfer (e.g., ADA and assets)
 *
 * cardano_tx_builder_send_value(tx_builder, recipient_address, transfer_value);
 * \endcode
 *
 * \note Errors, such as insufficient balance or invalid assets, will be deferred and reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_send_value(
  cardano_tx_builder_t* builder,
  cardano_address_t*    address,
  cardano_value_t*      value);

/**
 * \brief Sends a specified value (which may include ADA and other assets) to a given address
 * provided as raw string.
 *
 * This function adds a transaction output to the builder, specifying an address in
 * character array format along with a complex value that may include multiple assets.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance to which the output will be added.
 * \param[in] address A character array containing the recipient's address in bech32 or hex format.
 * \param[in] address_size The length of the address character array.
 * \param[in] value A pointer to a \ref cardano_value_t object containing the ADA and any additional assets to send.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* recipient_address = "addr1...";  // Bech32 address
 * size_t address_size = strlen(recipient_address);
 * cardano_value_t* transfer_value = ...;  // Value to transfer (e.g., ADA and assets)
 *
 * cardano_tx_builder_send_value_ex(tx_builder, recipient_address, address_size, transfer_value);
 * \endcode
 *
 * \note Errors, such as insufficient balance or invalid assets, will be deferred and reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_send_value_ex(
  cardano_tx_builder_t* builder,
  const char*           address,
  size_t                address_size,
  cardano_value_t*      value);

/**
 * \brief Locks a specified amount of lovelace at a script address.
 *
 * This function adds a transaction output to the builder that sends lovelace to a script address.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance to which the locked output will be added.
 * \param[in] script_address A pointer to the \ref cardano_address_t representing the script address where the lovelace will be locked.
 * \param[in] amount The amount of lovelace to be locked at the script address.
 * \param[in] datum A pointer to the \ref cardano_datum_t object containing the data to lock with the transaction.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;         // Initialized transaction builder
 * cardano_address_t* script_addr = ...;           // Script address
 * uint64_t lovelace_amount = 5000000;             // Amount of lovelace to lock
 * cardano_datum_t* datum = ...;                   // Datum associated with the script
 *
 * cardano_tx_builder_lock_lovelace(tx_builder, script_addr, lovelace_amount, datum);
 * \endcode
 *
 * \note Errors such as an invalid amount or address will be reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_lock_lovelace(
  cardano_tx_builder_t* builder,
  cardano_address_t*    script_address,
  uint64_t              amount,
  cardano_datum_t*      datum);

/**
 * \brief Locks a specified amount of lovelace at a script address using an address in raw byte format.
 *
 * This function adds a transaction output to the builder, sending lovelace to a script address.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance where the locked output will be added.
 * \param[in] script_address A pointer to a raw byte array containing the script address where the lovelace will be locked.
 * \param[in] script_address_size The size, in bytes, of the script address.
 * \param[in] amount The amount of lovelace to be locked at the specified script address.
 * \param[in] datum A pointer to the \ref cardano_datum_t object containing the datum for the script output.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;           // Initialized transaction builder
 * const char* script_addr_bytes = ...;              // Raw bytes of script address
 * size_t script_addr_size = ...;                    // Size of the address in bytes
 * uint64_t lovelace_amount = 5000000;               // Amount of lovelace to lock
 * cardano_datum_t* datum = ...;                     // Datum associated with the script
 *
 * cardano_tx_builder_lock_lovelace_ex(tx_builder, script_addr_bytes, script_addr_size, lovelace_amount, datum);
 * \endcode
 *
 * \note Errors such as an invalid amount or address will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_lock_lovelace_ex(
  cardano_tx_builder_t* builder,
  const char*           script_address,
  size_t                script_address_size,
  uint64_t              amount,
  cardano_datum_t*      datum);

/**
 * \brief Locks a specified value (including ADA and multi-asset tokens) at a script address.
 *
 * This function adds a transaction output to the builder, sending a specified value to a script address.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance where the locked output will be added.
 * \param[in] script_address A pointer to the \ref cardano_address_t object representing the script address where the value will be locked.
 * \param[in] value A pointer to the \ref cardano_value_t object representing the amount and type of value (ADA or tokens) to be locked at the script address.
 * \param[in] datum A pointer to the \ref cardano_datum_t object containing the datum for the script output.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;       // Initialized transaction builder
 * cardano_address_t* script_addr = ...;         // Script address to lock value at
 * cardano_value_t* value = ...;                 // Value including ADA and/or tokens
 * cardano_datum_t* datum = ...;                 // Datum associated with the script
 *
 * cardano_tx_builder_lock_value(tx_builder, script_addr, value, datum);
 * \endcode
 *
 * \note Errors such as invalid address or value will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_lock_value(
  cardano_tx_builder_t* builder,
  cardano_address_t*    script_address,
  cardano_value_t*      value,
  cardano_datum_t*      datum);

/**
 * \brief Locks a specified value (including ADA and multi-asset tokens) at a script address using a raw address string.
 *
 * This function adds a transaction output to the builder, sending a specified value to a script address represented by a raw address string.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance where the locked output will be added.
 * \param[in] script_address A pointer to a character array containing the raw script address string where the value will be locked.
 * \param[in] script_address_size The size of the `script_address` string in bytes.
 * \param[in] value A pointer to the \ref cardano_value_t object representing the amount and type of value (ADA or tokens) to be locked at the script address.
 * \param[in] datum A pointer to the \ref cardano_datum_t object containing the datum for the script output.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;       // Initialized transaction builder
 * const char* script_addr_str = "addr_test1..."; // Raw address string for the script address
 * size_t script_addr_size = strlen(script_addr_str);
 * cardano_value_t* value = ...;                 // Value including ADA and/or tokens
 * cardano_datum_t* datum = ...;                 // Datum associated with the script (optional)
 *
 * cardano_tx_builder_lock_value_ex(tx_builder, script_addr_str, script_addr_size, value, datum);
 * \endcode
 *
 * \note Errors such as invalid address or value will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_lock_value_ex(
  cardano_tx_builder_t* builder,
  const char*           script_address,
  size_t                script_address_size,
  cardano_value_t*      value,
  cardano_datum_t*      datum);

/**
 * \brief Adds an input to the transaction.
 *
 * This function appends a specified UTXO as an input to the transaction being built. Optionally,
 * it allows attaching a redeemer and datum if the input is associated with a Plutus script.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance in which to add the input.
 * \param[in] utxo A pointer to the \ref cardano_utxo_t structure representing the UTXO to be used as an input.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t structure representing the redeemer for the Plutus script (optional; can be NULL).
 * \param[in] datum A pointer to the \ref cardano_plutus_data_t structure representing the datum for the Plutus script (optional; can be NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_utxo_t* input_utxo = ...;        // UTXO to add as an input
 * cardano_plutus_data_t* redeemer = ...;   // Optional redeemer for the input
 * cardano_plutus_data_t* datum = ...;      // Optional datum for the input
 *
 * cardano_tx_builder_add_input(tx_builder, input_utxo, redeemer, datum);
 * \endcode
 *
 * \note Any errors resulting from adding this input will be deferred and reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_add_input(
  cardano_tx_builder_t*  builder,
  cardano_utxo_t*        utxo,
  cardano_plutus_data_t* redeemer,
  cardano_plutus_data_t* datum);

/**
 * \brief Adds an output to the transaction.
 *
 * This function appends a specified transaction output to the transaction being built.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance to which the output will be added.
 * \param[in] output A pointer to the \ref cardano_transaction_output_t structure representing the transaction output to be added.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;            // Initialized transaction builder
 * cardano_transaction_output_t* tx_output = ...;     // Transaction output to add
 *
 * cardano_tx_builder_add_output(tx_builder, tx_output);
 * \endcode
 *
 * \note Any errors related to adding this output, such as exceeding balance requirements, will be deferred and reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_add_output(
  cardano_tx_builder_t*         builder,
  cardano_transaction_output_t* output);

/**
 * \brief Sets metadata for a transaction builder with a specified tag.
 *
 * This function attaches metadata to a transaction being built, associating it with a specific metadata tag. Metadata can include additional information for the transaction, often used to encode arbitrary data on-chain.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance where the metadata will be added.
 * \param[in] tag A unique identifier for the metadata, typically a 64-bit unsigned integer.
 * \param[in] metadata A pointer to the \ref cardano_metadatum_t object containing the metadata to be added.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;       // Initialized transaction builder
 * uint64_t metadata_tag = 12345;                // Example metadata tag
 * cardano_metadatum_t* metadata = ...;          // Initialized metadata object
 *
 * cardano_tx_builder_set_metadata(tx_builder, metadata_tag, metadata);
 * \endcode
 *
 * \note This function only stores a reference to the metadata. Errors, such as invalid metadata format, will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_set_metadata(
  cardano_tx_builder_t* builder,
  uint64_t              tag,
  cardano_metadatum_t*  metadata);

/**
 * \brief Sets metadata for a transaction builder from a JSON string with a specified tag.
 *
 * This function allows adding metadata to the transaction by parsing a JSON string representation of the metadata. The metadata is associated with a specified tag, allowing for organized and accessible on-chain data.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance where the metadata will be added.
 * \param[in] tag A unique identifier for the metadata, typically a 64-bit unsigned integer.
 * \param[in] metadata_json A pointer to a JSON string representing the metadata content.
 * \param[in] json_size The length of the `metadata_json` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * uint64_t metadata_tag = 12345;            // Example metadata tag
 * const char* metadata_json = "{ \"name\": \"some_name\" }";  // Metadata JSON as string
 * size_t json_size = strlen(metadata_json);
 *
 * cardano_tx_builder_set_metadata_ex(tx_builder, metadata_tag, metadata_json, json_size);
 * \endcode
 *
 * \note This function will parse the JSON string to convert it into metadata. Errors, such as invalid JSON format, will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_set_metadata_ex(
  cardano_tx_builder_t* builder,
  uint64_t              tag,
  const char*           metadata_json,
  size_t                json_size);

/**
 * \brief Adds a token minting operation to the transaction builder.
 *
 * This function allows the user to specify a token minting operation within a transaction, including
 * the policy ID, asset name, amount, and an optional redeemer for Plutus scripts.
 * This enables minting (positive amount) or burning (negative amount) of tokens.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance for building the transaction.
 * \param[in] policy_id A pointer to a \ref cardano_blake2b_hash_t representing the unique minting policy ID.
 * \param[in] name A pointer to a \ref cardano_asset_name_t defining the name of the asset being minted or burned.
 * \param[in] amount The number of tokens to mint or burn; positive values indicate minting, and negative values indicate burning.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t structure, representing a redeemer if a Plutus script is used. Can be `NULL` if no redeemer is required.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;    // Initialized transaction builder
 * cardano_blake2b_hash_t* policy_id = ...;   // Policy ID for the token
 * cardano_asset_name_t* asset_name = ...;    // Asset name for the token
 * int64_t mint_amount = 100;                 // Amount to mint
 * cardano_plutus_data_t* redeemer = ...;     // Optional redeemer for Plutus scripts
 *
 * cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, mint_amount, redeemer);
 * \endcode
 *
 * \note Errors related to this operation will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_mint_token(
  cardano_tx_builder_t*   builder,
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   name,
  int64_t                 amount,
  cardano_plutus_data_t*  redeemer);

/**
 * \brief Adds a token minting operation to the transaction builder using policy ID and asset name in hexadecimal format.
 *
 * This function allows the transaction builder to mint or burn tokens associated with a specific policy ID and asset name.
 * The minting or burning amount and an optional redeemer for Plutus scripts are specified.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for transaction construction.
 * \param[in] policy_id_hex A string in hexadecimal format representing the policy ID of the token to mint or burn.
 * \param[in] policy_id_size The size of the `policy_id_hex` string.
 * \param[in] name_hex A string in hexadecimal format representing the asset name within the policy ID.
 * \param[in] name_size The size of the `name_hex` string.
 * \param[in] amount The amount of tokens to mint (positive) or burn (negative).
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t structure, representing a redeemer if a Plutus script is involved. Set to `NULL` if not required.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;     // Initialized transaction builder
 * const char* policy_id_hex = "abcdef123456"; // Example policy ID in hexadecimal format
 * size_t policy_id_size = strlen(policy_id_hex); // Size of policy ID
 * const char* name_hex = "74657374";          // Asset name in hexadecimal ("test" in hex)
 * size_t name_size = strlen(name_hex);        // Size of asset name
 * int64_t mint_amount = 100;                  // Amount to mint (positive for minting, negative for burning)
 * cardano_plutus_data_t* redeemer = ...;      // Optional redeemer for Plutus script
 *
 * cardano_tx_builder_mint_token_ex(tx_builder, policy_id_hex, policy_id_size, name_hex, name_size, mint_amount, redeemer);
 * \endcode
 *
 * \note Any errors related to minting policies or Plutus script requirements will only be reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_mint_token_ex(
  cardano_tx_builder_t*  builder,
  const char*            policy_id_hex,
  size_t                 policy_id_size,
  const char*            name_hex,
  size_t                 name_size,
  int64_t                amount,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Adds a token minting operation to the transaction builder using a predefined asset ID.
 *
 * This function allows minting or burning of tokens within a transaction by specifying an asset ID, along with the amount and an optional redeemer for Plutus scripts.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used to build the transaction.
 * \param[in] asset_id A pointer to a \ref cardano_asset_id_t structure representing the asset to mint or burn.
 * \param[in] amount The amount of tokens to mint (positive) or burn (negative).
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t structure, representing a redeemer if a Plutus script is involved. Set to `NULL` if not required.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * cardano_asset_id_t* asset_id = ...;       // Predefined asset ID for the token
 * int64_t mint_amount = 50;                 // Amount to mint (positive for minting, negative for burning)
 * cardano_plutus_data_t* redeemer = ...;    // Optional Plutus script redeemer
 *
 * cardano_tx_builder_mint_token_with_id(tx_builder, asset_id, mint_amount, redeemer);
 * \endcode
 *
 * \note Any errors related to this operation are deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_mint_token_with_id(
  cardano_tx_builder_t*  builder,
  cardano_asset_id_t*    asset_id,
  int64_t                amount,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Adds a token minting operation to the transaction builder using an asset ID in hexadecimal format.
 *
 * This function enables minting or burning tokens for a specific asset identified by its unique `asset_id` in hexadecimal format.
 * The minting or burning amount, along with an optional redeemer for Plutus scripts, is specified.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for transaction construction.
 * \param[in] asset_id_hex A string in hexadecimal format representing the unique asset ID.
 * \param[in] hex_size The size of the `asset_id_hex` string.
 * \param[in] amount The amount of tokens to mint (positive) or burn (negative).
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t structure, representing a redeemer if a Plutus script is involved. Set to `NULL` if not required.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;       // Initialized transaction builder
 * const char* asset_id_hex = "abcdef1234567890"; // Example asset ID in hexadecimal format
 * size_t hex_size = strlen(asset_id_hex);        // Size of asset ID
 * int64_t mint_amount = 100;                     // Amount to mint (positive for minting, negative for burning)
 * cardano_plutus_data_t* redeemer = ...;         // Optional redeemer for Plutus script
 *
 * cardano_tx_builder_mint_token_with_id_ex(tx_builder, asset_id_hex, hex_size, mint_amount, redeemer);
 * \endcode
 *
 * \note Any errors will only be reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_mint_token_with_id_ex(
  cardano_tx_builder_t*  builder,
  const char*            asset_id_hex,
  size_t                 hex_size,
  int64_t                amount,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Pads the transaction signer count for fee calculation purposes.
 *
 * This function pads the expected signature count in the transaction builder. It allows the
 * transaction to calculate the fee considering additional signers.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for transaction construction.
 * \param[in] count The number of expected signers to pad, affecting fee calculation.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * size_t extra_signers = 2;                // Additional signers to pad for fee calculation
 *
 * cardano_tx_builder_pad_signer_count(tx_builder, extra_signers);
 * \endcode
 *
 * \note This function influences only the fee estimation and does not modify the actual required signers for
 *       the transaction. Errors related to setting the signer count are deferred and will only be reported
 *       when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_pad_signer_count(cardano_tx_builder_t* builder, size_t count);

/**
 * \brief Adds a signer to the transaction being built.
 *
 * This function registers a specific signer for the transaction by providing their public key hash.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for transaction construction.
 * \param[in] pub_key_hash A pointer to the \ref cardano_blake2b_hash_t structure representing the public
 *                         key hash of the signer to be added.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_blake2b_hash_t* signer_hash = ...;  // Public key hash for the signer
 *
 * cardano_tx_builder_add_signer(tx_builder, signer_hash);
 * \endcode
 *
 * \note Errors related to adding signers are deferred and will only be reported when `cardano_tx_builder_build`
 *       is called.
 */
CARDANO_EXPORT void cardano_tx_builder_add_signer(
  cardano_tx_builder_t*   builder,
  cardano_blake2b_hash_t* pub_key_hash);

/**
 * \brief Adds a signer to the transaction by specifying their public key hash in hexadecimal format.
 *
 * This function registers a signer by accepting their public key hash as a hexadecimal string.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for transaction construction.
 * \param[in] pub_key_hash A string representing the public key hash in hexadecimal format.
 * \param[in] hash_size The size of the `pub_key_hash` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* pub_key_hash = "aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899";
 * size_t hash_size = strlen(pub_key_hash);
 *
 * cardano_tx_builder_add_signer_ex(tx_builder, pub_key_hash, hash_size);
 * \endcode
 *
 * \note Errors associated with adding a signer are deferred and will only be reported when `cardano_tx_builder_build`
 *       is called.
 */
CARDANO_EXPORT void cardano_tx_builder_add_signer_ex(
  cardano_tx_builder_t* builder,
  const char*           pub_key_hash,
  size_t                hash_size);

/**
 * \brief Adds a datum to the transaction builder for use in script-locked UTXOs.
 *
 * This function registers a Plutus datum with the transaction builder, which can then be referenced by outputs
 * that are locked by Plutus scripts.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for constructing the transaction.
 * \param[in] datum A pointer to a \ref cardano_plutus_data_t object representing the datum to be included.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_plutus_data_t* datum = ...;      // Initialized Plutus datum
 *
 * cardano_tx_builder_add_datum(tx_builder, datum);
 * \endcode
 *
 * \note Errors related to adding a datum are deferred and will be reported when `cardano_tx_builder_build` is called.
 *       The caller must ensure that the `datum` remains valid until the transaction is built.
 */
CARDANO_EXPORT void cardano_tx_builder_add_datum(
  cardano_tx_builder_t*  builder,
  cardano_plutus_data_t* datum);

/**
 * \brief Withdraws rewards from a specified reward account in the transaction builder.
 *
 * This function instructs the transaction builder to withdraw the available rewards for the specified
 * reward account. It uses the associated \ref cardano_provider_t to fetch the available reward balance
 * for the given account address and adds the withdrawal to the transaction.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for constructing the transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the reward account address
 *                    from which rewards should be withdrawn.
 * \param[in] amount  The amount of rewards to withdraw from the account. It must be the full available reward balance.
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t that serves as the redeemer for
 *                     script-locked withdrawals, if applicable.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;           // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...;    // Initialized reward address
 * cardano_plutus_data_t* redeemer = ...;            // Optional redeemer data
 *
 * cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, 2532145, redeemer);
 * \endcode
 *
 * \note Errors related to reward withdrawal will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_withdraw_rewards(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  int64_t                   amount,
  cardano_plutus_data_t*    redeemer);

/**
 * \brief Withdraws rewards from a specified reward account using a string address in the transaction builder.
 *
 * This function instructs the transaction builder to withdraw available rewards from the specified reward account,
 * using a string format for the reward address. It uses the associated \ref cardano_provider_t to fetch the available
 * reward balance for the given address and includes the withdrawal in the transaction.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for constructing the transaction.
 * \param[in] reward_address A string representing the reward account address from which rewards are to be withdrawn.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] amount  The amount of rewards to withdraw from the account. It must be the full available reward balance.
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t that acts as the redeemer for
 *                     script-locked withdrawals, if applicable.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;          // Initialized transaction builder
 * const char* reward_addr = "stake1u9...";         // Reward address in string format
 * size_t address_size = strlen(reward_addr);       // Length of the reward address string
 * cardano_plutus_data_t* redeemer = ...;           // Optional redeemer data
 *
 * cardano_tx_builder_withdraw_rewards_ex(tx_builder, reward_addr, address_size, 966584122, redeemer);
 * \endcode
 *
 * \note Errors related to reward withdrawal will be deferred until `cardano_tx_builder_build` is called.
 *       Ensure the reward address and redeemer remain valid until the transaction is built.
 */
CARDANO_EXPORT void cardano_tx_builder_withdraw_rewards_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  int64_t                amount,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Registers a staking reward address.
 *
 * This function adds a staking reward address to be registered in the transaction, allowing the specified
 * reward account to start receiving staking rewards.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance used for constructing the transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the reward account to be registered.
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t, which provides the redeemer data if the
 *                     registration is script-locked.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;          // Initialized transaction builder
 * cardano_reward_address_t* reward_addr = ...;     // Reward address to register
 * cardano_plutus_data_t* redeemer = ...;           // Optional redeemer data for script-locked registration
 *
 * cardano_tx_builder_register_reward_address(tx_builder, reward_addr, redeemer);
 * \endcode
 *
 * \note Any errors related to reward address registration will be reported when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_register_reward_address(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer);

/**
 * \brief Registers a staking reward address using a hexadecimal address string.
 *
 * This function registers a staking reward address in the transaction builder, allowing
 * the specified reward account, provided as a hexadecimal string, to receive staking rewards.
 * The transaction builder leverages the \ref cardano_provider_t to validate this process.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction construction.
 * \param[in] reward_address A pointer to a bech32 string representing the reward address.
 * \param[in] address_size The size of the `reward_address` string.
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t, used if the registration requires
 *                     redeemer data for script-locked transactions.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;       // Initialized transaction builder
 * const char* reward_address = "stake1uxxx...";
 * size_t address_size = strlen(reward_address);
 * cardano_plutus_data_t* redeemer = ...;        // Optional redeemer data for script-locked registration
 *
 * cardano_tx_builder_register_reward_address_ex(tx_builder, reward_address, address_size, redeemer);
 * \endcode
 *
 * \note Errors related to the reward address registration will be reported only when `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_register_reward_address_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Deregisters a staking reward address.
 *
 * This function deregisters the reward address, effectively preventing it from receiving future staking rewards. a \ref cardano_plutus_data_t redeemer can be provided for
 * script-locked accounts.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t structure representing the reward address to deregister.
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t for providing redeemer data if required for deregistration.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...; // Reward address to be deregistered
 * cardano_plutus_data_t* redeemer = ...;  // Optional redeemer data for script-locked deregistration
 *
 * cardano_tx_builder_deregister_reward_address(tx_builder, reward_address, redeemer);
 * \endcode
 *
 * \note Errors related to the deregistration process are deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_deregister_reward_address(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer);

/**
 * \brief Deregisters a staking reward address.
 *
 * This function deregisters the reward address, preventing future staking rewards from being sent to
 * the specified reward account. It accepts the reward address in a raw string format along with the
 * length of the address. Optionally, a \ref cardano_plutus_data_t redeemer can be provided for
 * script-locked accounts.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] reward_address A pointer to a raw string containing the reward address to be deregistered.
 * \param[in] address_size The length of the reward address string.
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t for providing redeemer data if required for deregistration.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* reward_address = "stake1uxxx...";  // Reward address in raw string format
 * size_t address_size = strlen(reward_address);
 * cardano_plutus_data_t* redeemer = ...;  // Optional redeemer data for script-locked deregistration
 *
 * cardano_tx_builder_deregister_reward_address_ex(tx_builder, reward_address, address_size, redeemer);
 * \endcode
 *
 * \note Errors related to the deregistration process are deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_deregister_reward_address_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Delegates stake from a specified staking reward address to a staking pool.
 *
 * This function delegates stake from a specified staking reward address to a staking pool,
 * identified by its unique pool ID.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the staking reward address to delegate.
 * \param[in] pool_id A pointer to the \ref cardano_blake2b_hash_t representing the hash ID of the pool to delegate to.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t for providing redeemer data if required for script-locked accounts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...;  // Reward address for staking
 * cardano_blake2b_hash_t* pool_id = ...;  // Pool ID to delegate stake to
 * cardano_plutus_data_t* redeemer = ...;  // Optional redeemer for script-locked delegation
 *
 * cardano_tx_builder_delegate_stake(tx_builder, reward_address, pool_id, redeemer);
 * \endcode
 *
 * \note Any errors associated with this delegation action will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_delegate_stake(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_blake2b_hash_t*   pool_id,
  cardano_plutus_data_t*    redeemer);

/**
 * \brief Delegates stake from a specified staking reward address to a staking pool.
 *
 * This function allows for delegating stake from a specified staking reward address to a staking pool,
 * using hex-encoded strings for the reward address and pool ID. The delegation request will be added to
 * the transaction managed by the builder instance.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] reward_address A bech32 string representing the staking reward address for delegation.
 * \param[in] address_size The size of the `reward_address` string.
 * \param[in] pool_id A bech32 string representing the pool ID to which stake is delegated.
 * \param[in] pool_id_size The size of the `pool_id` string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t for providing redeemer data if required for script-locked accounts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* reward_address = "stake1u9...";
 * size_t reward_address_size = strlen(reward_address);
 * const char* pool_id = "pool1xy...";
 * size_t pool_id_size = strlen(pool_id);
 * cardano_plutus_data_t* redeemer = ...;  // Optional redeemer for script-locked delegation
 *
 * cardano_tx_builder_delegate_stake_ex(tx_builder, reward_address, reward_address_size, pool_id, pool_id_size, redeemer);
 * \endcode
 *
 * \note Errors related to this action will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_delegate_stake_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  const char*            pool_id,
  size_t                 pool_id_size,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Adds a voting power delegation to a DRep for governance purposes.
 *
 * This function enables delegating voting power from a specified reward address to DRep.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the staking reward address that is delegating voting power.
 * \param[in] drep A pointer to the \ref cardano_drep_t structure that identifies the decentralized representative receiving the delegated voting power.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t for providing redeemer data, useful for script-locked accounts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...;  // Reward address delegating voting power
 * cardano_drep_t* drep = ...;  // DRep receiving the delegated voting power
 * cardano_plutus_data_t* redeemer = ...;  // Optional redeemer data
 *
 * cardano_tx_builder_delegate_voting_power(tx_builder, reward_address, drep, redeemer);
 * \endcode
 *
 * \note Errors associated with this delegation will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_delegate_voting_power(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_drep_t*           drep,
  cardano_plutus_data_t*    redeemer);

/**
 * \brief Delegates voting power to a DRep using raw data.
 *
 * This function enables the delegation of voting power from a specified reward address to a DRep using string-based identifiers.
 * The DRep ID can be provided in either CIP-105 or CIP-129 format.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] reward_address A string representing the staking reward address for the account delegating voting power.
 * \param[in] address_size The length of the `reward_address` string.
 * \param[in] drep_id A string representing the ID of the decentralized representative (DRep) receiving the voting power.
 *                     The DRep ID can be in CIP-105 or CIP-129 format.
 * \param[in] drep_id_size The length of the `drep_id` string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t for providing redeemer data, useful for script-locked accounts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* reward_address = "stake1u9...";  // Reward address delegating voting power
 * size_t address_size = strlen(reward_address);
 * const char* drep_id = "drep1q...";  // DRep ID in CIP-129 format
 * size_t drep_id_size = strlen(drep_id);
 * cardano_plutus_data_t* redeemer = ...;  // Optional redeemer data
 *
 * // Alternatively, using CIP-105 format
 * // const char* drep_id = "drep...";  // DRep ID in CIP-105 format
 * // size_t drep_id_size = strlen(drep_id);
 *
 * cardano_tx_builder_delegate_voting_power_ex(tx_builder, reward_address, address_size, drep_id, drep_id_size, redeemer);
 * \endcode
 *
 * \note The DRep ID must conform to either CIP-105 or CIP-129 format.
 *       Errors related to this delegation will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_delegate_voting_power_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Registers a DRep in the transaction.
 *
 * This function registers a DRep with an optional anchor and redeemer in the transaction being constructed.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep A pointer to the \ref cardano_drep_t instance representing the decentralized representative to be registered.
 * \param[in] anchor An optional pointer to a \ref cardano_anchor_t instance providing an anchor for governance (e.g., delegation details).
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required for
 *                     script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_drep_t* drep = ...;              // DRep to register
 * cardano_anchor_t* anchor = ...;          // Optional governance anchor
 * cardano_plutus_data_t* redeemer = ...;   // Optional redeemer data
 *
 * cardano_tx_builder_register_drep(tx_builder, drep, anchor, redeemer);
 * \endcode
 *
 * \note Errors related to this registration will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_register_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Registers a DRep by ID in the transaction.
 *
 * This function registers a DRep in the transaction using a specified DRep ID. The DRep ID must be provided in either
 * CIP-105 or CIP-129 bech32 format. An optional redeemer data can be included for script-locked DReps.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep_id A pointer to a character array containing the DRep ID in bech32 format (either CIP-105 or CIP-129).
 * \param[in] drep_id_size The size of the `drep_id` string.
 * \param[in] metadata_url The URL pointing to the DRep metadata file.
 * \param[in] metadata_url_size The size of the `metadata_url` string.
 * \param[in] metadata_hash_hex The hash of the DRep metadata file in hexadecimal format.
 * \param[in] metadata_hash_hex_size The size of the `metadata_hash_hex` string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required for
 *                     script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * // Using CIP-105 format (bech32-encoded DRep ID with specific prefix)
 * cardano_tx_builder_t* tx_builder = ...;    // Initialized transaction builder
 * const char* drep_id = "drep1q...";
 * size_t drep_id_size = strlen(drep_id);
 * const char* metadata_url = "https://example.com/drep_metadata.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abcdef123456...";  // Hex-encoded hash of metadata file
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 * cardano_plutus_data_t* redeemer = ...;     // Optional redeemer data
 *
 * cardano_tx_builder_register_drep_ex(
 *   tx_builder,
 *   drep_id,
 *   drep_id_size,
 *   metadata_url,
 *   metadata_url_size,
 *   metadata_hash_hex,
 *   metadata_hash_hex_size,
 *   redeemer);
 * \endcode
 *
 * \note The `drep_id` must be in bech32 format as specified by either CIP-105 or CIP-129, which differ in their internal binary encoding.
 *       Ensure that the correct format is used according to the DRep's identification scheme.
 *       Errors related to this registration will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_register_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  const char*            metadata_url,
  size_t                 metadata_url_size,
  const char*            metadata_hash_hex,
  size_t                 metadata_hash_hex_size,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Updates an existing DRep in the transaction.
 *
 * This function allows updating a DRep in the transaction with a specified DRep object, optionally providing a
 * governance anchor and redeemer data for script DReps.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep A pointer to the \ref cardano_drep_t instance representing the DRep to be updated.
 * \param[in] anchor An optional pointer to a \ref cardano_anchor_t instance, providing governance anchor details associated with the DRep.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required for
 *                     script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;    // Initialized transaction builder
 * cardano_drep_t* drep = ...;                // DRep object to update
 * cardano_anchor_t* anchor = ...;            // Optional anchor
 * cardano_plutus_data_t* redeemer = ...;     // Optional redeemer data
 *
 * cardano_tx_builder_update_drep(tx_builder, drep, anchor, redeemer);
 * \endcode
 *
 * \note Any errors related to updating the DRep will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_update_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Updates an existing DRep in the transaction by ID.
 *
 * This function updates a DRep in the transaction using a specified DRep ID. The DRep ID must be provided in bech32 format,
 * conforming to either CIP-105 or CIP-129 standards. Optionally, you can provide a redeemer for script-locked DReps.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep_id A pointer to a character array containing the DRep ID in bech32 format (either CIP-105 or CIP-129).
 * \param[in] drep_id_size The size of the `drep_id` string.
 * \param[in] metadata_url The URL pointing to the DRep metadata file.
 * \param[in] metadata_url_size The size of the `metadata_url` string.
 * \param[in] metadata_hash_hex The hash of the DRep metadata file in hexadecimal format.
 * \param[in] metadata_hash_hex_size The size of the `metadata_hash_hex` string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required
 *                     for script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * // Using CIP-105 format (bech32-encoded DRep ID with specific prefix)
 * cardano_tx_builder_t* tx_builder = ...;    // Initialized transaction builder
 * const char* drep_id = "drep1q...";
 * size_t drep_id_size = strlen(drep_id);
 * const char* metadata_url = "https://example.com/drep_metadata.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abcdef123456...";  // Hex-encoded hash of metadata file
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 * cardano_plutus_data_t* redeemer = ...;     // Optional redeemer data
 *
 * cardano_tx_builder_update_drep_ex(
 *   tx_builder,
 *   drep_id,
 *   drep_id_size,
 *   metadata_url,
 *   metadata_url_size,
 *   metadata_hash_hex,
 *   metadata_hash_hex_size,
 *   redeemer);
 * \endcode
 *
 * \note The `drep_id` must be in bech32 format as specified by either CIP-105 or CIP-129, which differ in their internal binary encoding.
 *       Ensure that the correct format is used according to the DRep's identification scheme.
 *       Errors associated with updating the DRep will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_update_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  const char*            metadata_url,
  size_t                 metadata_url_size,
  const char*            metadata_hash_hex,
  size_t                 metadata_hash_hex_size,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Deregisters an existing DRep.
 *
 * This function registers a DRep. A redeemer can be specified for additional validation if
 * the DRep is a script.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep A pointer to the \ref cardano_drep_t instance representing the DRep to deregister.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required for
 *                     script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;    // Initialized transaction builder
 * cardano_drep_t* drep = ...;                // DRep to deregister
 * cardano_plutus_data_t* redeemer = ...;     // Optional redeemer data
 *
 * cardano_tx_builder_deregister_drep(tx_builder, drep, redeemer);
 * \endcode
 *
 * \note Errors related to deregistering the DRep will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_deregister_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Deregisters an existing DRep.
 *
 * This function deregisters a DRep identified by its ID in bech32 format. The DRep ID must conform to either
 * CIP-105 or CIP-129 standards.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep_id A pointer to a character array containing the DRep ID in bech32 format (either CIP-105 or CIP-129).
 * \param[in] drep_id_size The size of the `drep_id` string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data,
 *                     which may be required for script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * // Using CIP-105 format (bech32-encoded DRep ID)
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * const char* drep_id = "drep1q...";        // DRep ID in CIP-105 format
 * size_t drep_id_size = strlen(drep_id);
 * cardano_plutus_data_t* redeemer = ...;    // Optional redeemer data
 *
 * cardano_tx_builder_deregister_drep_ex(tx_builder, drep_id, drep_id_size, redeemer);
 * \endcode
 *
 * \note The `drep_id` must be in bech32 format as specified by either CIP-105 or CIP-129, which differ in their internal binary encoding.
 *       Ensure that the correct format is used according to the DRep's identification scheme.
 *       Errors related to deregistering the DRep will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_deregister_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Registers a vote for a specified governance action within the transaction.
 *
 * This function allows a voter to submit their vote for a given governance action. The vote, represented by
 * `cardano_voting_procedure_t`, and an optional redeemer for script voters.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] voter A pointer to a \ref cardano_voter_t structure representing the voter participating in the governance action.
 * \param[in] action_id A pointer to a \ref cardano_governance_action_id_t identifying the governance action being voted on.
 * \param[in] vote A pointer to a \ref cardano_voting_procedure_t defining the voting procedure and choice.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, required if the voting action is locked by a script.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * cardano_voter_t* voter = ...;             // Voter information
 * cardano_governance_action_id_t* action_id = ...; // Governance action ID
 * cardano_voting_procedure_t* vote = ...;   // Voting procedure with choice
 * cardano_plutus_data_t* redeemer = ...;    // Optional redeemer data
 *
 * cardano_tx_builder_vote(tx_builder, voter, action_id, vote, redeemer);
 * \endcode
 *
 * \note Errors related to the voting operation will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_vote(
  cardano_tx_builder_t*           builder,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_plutus_data_t*          redeemer);

/**
 * \brief Adds a certificate to the transaction.
 *
 * This function adds a specified certificate to the transaction being constructed. Certificates are used to perform
 * various actions on the blockchain, such as staking, delegating, or registering/deregistering entities.
 * An optional redeemer may be provided if the certificate requires validation by a script.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] certificate A pointer to a \ref cardano_certificate_t structure representing the certificate to add.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance containing redeemer data, required if the certificate is locked by a script.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;        // Initialized transaction builder
 * cardano_certificate_t* certificate = ...;      // Certificate to add
 * cardano_plutus_data_t* redeemer = ...;         // Optional redeemer data
 *
 * cardano_tx_builder_add_certificate(tx_builder, certificate, redeemer);
 * \endcode
 *
 * \note Errors related to the certificate addition will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_add_certificate(
  cardano_tx_builder_t*  builder,
  cardano_certificate_t* certificate,
  cardano_plutus_data_t* redeemer);

/**
 * \brief Adds a script to the transaction builder.
 *
 * This function allows the addition of a script (`cardano_script_t`) to the transaction builder. Scripts can be used for defining
 * specific conditions under which transaction outputs can be unlocked.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction details.
 * \param[in] script A pointer to the \ref cardano_script_t structure representing the script to be added to the transaction.
 *
 * \note Errors related to adding an invalid or incompatible script will be deferred and reported only when `cardano_tx_builder_build` is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * cardano_script_t* script = ...;          // A script for the transaction
 *
 * cardano_tx_builder_add_script(tx_builder, script);
 *
 * // Additional transaction setup...
 *
 * cardano_error_t result = cardano_tx_builder_build(tx_builder, &transaction);
 * if (result != CARDANO_SUCCESS)
 * {
 *   const char* error_msg = cardano_tx_builder_get_last_error(tx_builder);
 *   printf("Failed to build transaction with script: %s\n", error_msg);
 * }
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_add_script(
  cardano_tx_builder_t* builder,
  cardano_script_t*     script);

/**
 * \brief Proposes a parameter change within a Cardano transaction.
 *
 * This function adds a parameter change proposal to the transaction being built. The proposal specifies
 * updates to protocol parameters, a governance action ID, and other metadata associated with the proposal.
 *
 * \param[in, out] builder A pointer to the \ref cardano_tx_builder_t instance used to build the transaction.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t where the deposit will be refunded.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing the anchor for the proposal. This parameter must not be NULL.
 * \param[in] protocol_param_update A pointer to a \ref cardano_protocol_param_update_t object containing the protocol parameter updates.
 *                                  This parameter must not be NULL.
 * \param[in] governance_action_id A pointer to a \ref cardano_governance_action_id_t object referencing the most recent enacted action of the same type.
 *                                 This parameter can be NULL if no proposals of this type have been enacted on the network.
 * \param[in] policy_hash A pointer to a \ref cardano_blake2b_hash_t object representing the guardrails script hash (also known as the governance action policy script).
 *                        This parameter is required for certain types of proposals to ensure compliance with governance guardrails.
 *
 * \note The `governance_action_id` is used to reference the most recent enacted governance action of the same type. If no such
 *       action has been enacted on the network, this parameter can be NULL. The `policy_hash` represents the hash of the guardrails script,
 *       which imposes additional constraints on governance actions such as protocol parameter updates.
 *
 * \note Any errors encountered during the addition of the parameter change proposal will be deferred until
 *       \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = cardano_tx_builder_new(...); // Assume builder is initialized
 * cardano_reward_address_t* reward_address = ...; // Assume initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Assume initialized
 * cardano_protocol_param_update_t* param_update = cardano_protocol_param_update_new(...); // Assume initialized
 * cardano_governance_action_id_t* action_id = cardano_governance_action_id_new(...); // Optionally initialized
 * cardano_blake2b_hash_t* policy_hash = cardano_blake2b_hash_new(...); // Assume initialized
 *
 * cardano_tx_builder_propose_parameter_change(builder, reward_address, anchor, param_update, action_id, policy_hash);
 *
 * // Cleanup when done
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_protocol_param_update_unref(&param_update);
 * if (action_id)
 * {
 *   cardano_governance_action_id_unref(&action_id);
 * }
 * cardano_blake2b_hash_unref(&policy_hash);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_parameter_change(
  cardano_tx_builder_t*            builder,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_blake2b_hash_t*          policy_hash);

/**
 * \brief Proposes a parameter change within a Cardano transaction (extended version).
 *
 * This function allows the inclusion of a parameter change proposal into the transaction being built, using string representations for key parameters
 * such as reward addresses, metadata, governance action IDs, and policy hashes.
 *
 * \param[in, out] builder A pointer to the \ref cardano_tx_builder_t instance used to build the transaction.
 * \param[in] reward_address A string representation of the reward address where the deposit will be refunded.
 * \param[in] reward_address_size The size (in bytes) of the `reward_address` string.
 * \param[in] metadata_url A string containing the URL pointing to additional metadata for the proposal. This can be NULL if not applicable.
 * \param[in] metadata_url_size The size (in bytes) of the `metadata_url` string.
 * \param[in] metadata_hash_hex A hexadecimal string representing the hash of the metadata contents.
 * \param[in] metadata_hash_hex_size The size (in bytes) of the `metadata_hash_hex` string.
 * \param[in] gov_action_id A CIP-0129 bech32 string representing the governance action ID that references the most recent enacted action of the same type.
 * \param[in] gov_action_id_size The size (in bytes) of the `gov_action_id` string.
 * \param[in] policy_hash_hash_hex A hexadecimal string representing the guardrails script hash (also known as the governance action policy script).
 * \param[in] policy_hash_hash_hex_size The size (in bytes) of the `policy_hash_hash_hex` string.
 * \param[in] protocol_param_update A pointer to a \ref cardano_protocol_param_update_t object containing the protocol parameter updates.
 *                                  This parameter must not be NULL.
 *
 * \note The `gov_action_id` references the most recent enacted governance action of the same type. If no such action has been enacted,
 *       this parameter can be NULL. The `policy_hash_hash_hex` represents the hash of the guardrails script, which imposes additional constraints
 *       on certain governance actions such as protocol parameter updates.
 *
 * \note Errors encountered during the addition of the parameter change proposal will be deferred until \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = cardano_tx_builder_new(...); // Assume builder is initialized
 * const char* reward_address = "stake1u9hwrj..."; // Reward address in Bech32
 * const char* metadata_url = "https://example.com/metadata.json"; // Metadata URL
 * const char* metadata_hash = "d2a4f89b..."; // Metadata hash in hexadecimal
 * const char* gov_action_id = "gov_action1..."; // Governance action ID in CIP-129 format
 * const char* policy_hash = "f4a6b3e..."; // Policy hash in hexadecimal
 * cardano_protocol_param_update_t* param_update = cardano_protocol_param_update_new(...); // Assume initialized
 *
 * cardano_tx_builder_propose_parameter_change_ex(builder,
 *                                                reward_address, strlen(reward_address),
 *                                                metadata_url, strlen(metadata_url),
 *                                                metadata_hash, strlen(metadata_hash),
 *                                                gov_action_id, strlen(gov_action_id),
 *                                                policy_hash, strlen(policy_hash),
 *                                                param_update);
 *
 * // Cleanup when done
 * cardano_protocol_param_update_unref(&param_update);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_parameter_change_ex(
  cardano_tx_builder_t*            builder,
  const char*                      reward_address,
  size_t                           reward_address_size,
  const char*                      metadata_url,
  size_t                           metadata_url_size,
  const char*                      metadata_hash_hex,
  size_t                           metadata_hash_hex_size,
  const char*                      gov_action_id,
  size_t                           gov_action_id_size,
  const char*                      policy_hash_hash_hex,
  size_t                           policy_hash_hash_hex_size,
  cardano_protocol_param_update_t* protocol_param_update);

/**
 * \brief Proposes a hard fork within a Cardano transaction.
 *
 * This function adds a proposal to initiate a hard fork in the Cardano network to the transaction being built. The hard fork proposal
 * specifies the target protocol version and other required parameters. An optional governance action ID can be provided to reference
 * the most recent enacted hard fork action of the same type.
 *
 * \param[in, out] builder A pointer to the \ref cardano_tx_builder_t instance used to build the transaction.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object where the deposit will be refunded if the proposal is accepted.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object containing additional metadata related to the proposal.
 * \param[in] version A pointer to a \ref cardano_protocol_version_t object specifying the target protocol version for the hard fork.
 * \param[in] governance_action_id An optional pointer to a \ref cardano_governance_action_id_t object referencing the most recent
 *                                 enacted governance action of the same type. This can be NULL if no such action has been enacted.
 *
 * \note The `governance_action_id` parameter is required to reference the last enacted hard fork action. If no hard fork proposals
 *       have been enacted yet, this parameter can be NULL.
 *
 * \note Errors encountered during the addition of the hard fork proposal will be deferred until \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = cardano_tx_builder_new(...); // Assume builder is initialized
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Assume initialized
 * cardano_protocol_version_t* version = cardano_protocol_version_new(8, 0); // Example protocol version
 * cardano_governance_action_id_t* gov_action_id = cardano_governance_action_id_new(...); // Optional, assume initialized
 *
 * cardano_tx_builder_propose_hardfork(builder, reward_address, anchor, version, gov_action_id);
 *
 * // Cleanup resources when done
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_protocol_version_unref(&version);
 * cardano_governance_action_id_unref(&gov_action_id);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_hardfork(
  cardano_tx_builder_t*           builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_protocol_version_t*     version,
  cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Proposes a hard fork within a Cardano transaction (extended version).
 *
 * This function adds a proposal to initiate a hard fork in the Cardano network to the transaction being built. The hard fork proposal
 * specifies the target protocol version, metadata details, and other required parameters. The proposal includes a governance action ID
 * to reference the most recent enacted action of the same type.
 *
 * \param[in, out] builder A pointer to the \ref cardano_tx_builder_t instance used to build the transaction.
 * \param[in] reward_address A pointer to the reward address string where the deposit will be refunded if the proposal is accepted.
 *                           This must be a valid Bech32-encoded reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the URL string containing metadata about the hard fork proposal.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hexadecimal string representing the hash of the metadata file.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id A CIP-0129 bech32 string representing the governance action ID that references the most recent enacted action of the same type.
 * \param[in] gov_action_id_size The size (in bytes) of the `gov_action_id` string.
 * \param[in] minor_protocol_version The minor protocol version for the proposed hard fork.
 * \param[in] major_protocol_version The major protocol version for the proposed hard fork.
 *
 * \note The `gov_action_id` parameters reference the most recent enacted governance action of the same type.
 *
 * \note Errors encountered during the addition of the hard fork proposal will be deferred until \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = cardano_tx_builder_new(...); // Assume builder is initialized
 * const char* reward_address = "stake1u..."; // Example reward address
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com/metadata.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash = "abcdef1234567890..."; // Example metadata hash
 * size_t metadata_hash_size = strlen(metadata_hash);
 * const char* gov_action_id = "gov_action1..."; // Example governance action ID
 * size_t gov_action_id_size = strlen(gov_action_id);
 * uint64_t minor_version = 1; // Example minor protocol version
 * uint64_t major_version = 8; // Example major protocol version
 *
 * cardano_tx_builder_propose_hardfork_ex(
 *   builder,
 *   reward_address,
 *   reward_address_size,
 *   metadata_url,
 *   metadata_url_size,
 *   metadata_hash,
 *   metadata_hash_size,
 *   gov_action_id,
 *   gov_action_id_size,
 *   minor_version,
 *   major_version
 * );
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_hardfork_ex(
  cardano_tx_builder_t* builder,
  const char*           reward_address,
  size_t                reward_address_size,
  const char*           metadata_url,
  size_t                metadata_url_size,
  const char*           metadata_hash_hex,
  size_t                metadata_hash_hex_size,
  const char*           gov_action_id,
  size_t                gov_action_id_size,
  uint64_t              minor_protocol_version,
  uint64_t              major_protocol_version);

/**
 * \brief Proposes treasury withdrawals as a governance action within the Cardano network.
 *
 * This function prepares a proposal to withdraw funds from the treasury.
 *
 * \param[in,out] builder       A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address A pointer to a \ref cardano_reward_address_t object representing the address that will receive the deposit refund.
 * \param[in]     anchor        A pointer to an initialized \ref cardano_anchor_t object containing metadata about the proposal.
 * \param[in]     withdrawals   A pointer to a \ref cardano_withdrawal_map_t object defining the requested treasury withdrawals.
 * \param[in]     policy_hash   A pointer to a \ref cardano_blake2b_hash_t object representing the guardrails script hash.
 *
 * \note The `policy_hash` parameter represents the hash of the guardrails script, a Plutus script that imposes constraints
 *       on treasury withdrawals and other governance actions. If no policy constraints apply, this parameter can be NULL.
 *
 * \note Any errors encountered during the addition of the treasury withdrawals proposal will be deferred until
 *       \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;  // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...; // Reward address for deposit refund
 * cardano_anchor_t* anchor = cardano_anchor_new("metadata_url", metadata_hash);  // Metadata for the proposal
 * cardano_withdrawal_map_t* withdrawals = ...; // Treasury withdrawals
 * cardano_blake2b_hash_t* policy_hash = ...;   // Guardrails script hash
 *
 * cardano_tx_builder_propose_treasury_withdrawals(builder, reward_address, anchor, withdrawals, policy_hash);
 * \endcode
 *
 * \note Ensure the `withdrawals` object is populated with valid treasury withdrawal requests before calling this function.
 */
CARDANO_EXPORT void cardano_tx_builder_propose_treasury_withdrawals(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor,
  cardano_withdrawal_map_t* withdrawals,
  cardano_blake2b_hash_t*   policy_hash);

/**
 * \brief Proposes treasury withdrawals as a governance action within the Cardano network, using string-based parameters.
 *
 * This function prepares a proposal for withdrawing funds from the treasury.
 *
 * \param[in,out] builder                 A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address          A pointer to the reward address string for receiving the deposit refund. Must be a valid Bech32 address.
 * \param[in]     reward_address_size     The size of the reward address string.
 * \param[in]     metadata_url            A pointer to the metadata URL string providing additional proposal context.
 * \param[in]     metadata_url_size       The size of the metadata URL string.
 * \param[in]     metadata_hash_hex       A pointer to the hexadecimal string representation of the metadata hash.
 * \param[in]     metadata_hash_hex_size  The size of the metadata hash string.
 * \param[in]     policy_hash_hash_hex    A pointer to the hexadecimal string representation of the guardrails script hash.
 * \param[in]     policy_hash_hash_hex_size The size of the guardrails script hash string.
 * \param[in]     withdrawals             A pointer to a \ref cardano_withdrawal_map_t object defining the requested treasury withdrawals.
 *
 * \note The `policy_hash_hash_hex` parameter represents the guardrails script hash. This Plutus script imposes additional constraints
 *       on treasury withdrawals and other governance actions. If no policy constraints apply, this parameter can be NULL.
 *
 * \note Any errors encountered during the addition of the treasury withdrawals proposal will be deferred until
 *       \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;  // Initialized transaction builder
 * const char* reward_address = "stake1u9v8lkxklxyz123";  // Example reward address
 * const char* metadata_url = "https://example.com/metadata";  // Metadata URL
 * const char* metadata_hash = "abc123";  // Metadata hash in hex
 * const char* policy_hash = "def456";    // Guardrails script hash in hex
 * cardano_withdrawal_map_t* withdrawals = ...; // Treasury withdrawals
 *
 * cardano_tx_builder_propose_treasury_withdrawals_ex(
 *   builder, reward_address, strlen(reward_address),
 *   metadata_url, strlen(metadata_url), metadata_hash, strlen(metadata_hash),
 *   policy_hash, strlen(policy_hash), withdrawals);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_treasury_withdrawals_ex(
  cardano_tx_builder_t*     builder,
  const char*               reward_address,
  size_t                    reward_address_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size,
  const char*               policy_hash_hash_hex,
  size_t                    policy_hash_hash_hex_size,
  cardano_withdrawal_map_t* withdrawals);

/**
 * \brief Proposes a no-confidence governance action within the Cardano network.
 *
 * This function prepares a proposal for a no-confidence action against the current constitutional committee.
 * If enacted, it signals the community's lack of confidence in the committee, potentially leading to its reconstitution.
 *
 * \param[in,out] builder                 A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address          A pointer to a \ref cardano_reward_address_t object. This address will receive the deposit refund after the governance action completes.
 * \param[in]     anchor                  A pointer to a \ref cardano_anchor_t object providing additional context or metadata for the governance action.
 * \param[in]     governance_action_id    An optional pointer to a \ref cardano_governance_action_id_t object. This represents the unique identifier
 *                                         for the most recently enacted no-confidence action of the same type. This parameter can be NULL if no previous actions of this type exist.
 *
 * \note The `governance_action_id` parameter ensures that the proposal references the latest enacted governance action of the same type, as required by the Cardano protocol.
 *
 * \note Any errors encountered during the addition of the no-confidence proposal will be deferred until
 *       \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;                // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...;     // Example reward address
 * cardano_anchor_t* anchor = ...;                     // Example anchor object
 * cardano_governance_action_id_t* action_id = ...;    // Example governance action ID (optional)
 *
 * cardano_tx_builder_propose_no_confidence(builder, reward_address, anchor, action_id);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_no_confidence(
  cardano_tx_builder_t*           builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Proposes a no-confidence governance action using extended parameters.
 *
 * This function prepares a proposal for a no-confidence action against the current constitutional committee within the Cardano network.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address         A pointer to a string containing the Bech32-encoded reward address. This address will receive the deposit refund
 *                                        after the governance action completes.
 * \param[in] reward_address_size    The size of the reward address string in bytes.
 * \param[in] metadata_url           A pointer to a string containing the metadata URL for additional context regarding the proposal.
 * \param[in] metadata_url_size      The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex      A pointer to a string containing the hexadecimal representation of the metadata hash.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id A CIP-0129 bech32 string representing the governance action ID that references the most recent enacted action of the same type.
 * \param[in] gov_action_id_size The size (in bytes) of the `gov_action_id` string.
 *
 * \note The `gov_action_id` ensure that the proposal references the latest enacted governance action of the same type,
 *       as required by the Cardano protocol. These can be omitted (set to NULL or 0) if no prior actions of this type exist.
 *
 * \note Any errors encountered during the addition of the no-confidence proposal will be deferred until
 *       \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;                // Initialized transaction builder
 * const char* reward_address = "stake1u9...";         // Example reward address
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com";   // Example metadata URL
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "a1b2c3...";        // Example metadata hash (hex-encoded)
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 * const char* gov_action_id = "gov_action1...";      // Example governance action ID (hex-encoded)
 * size_t gov_action_id_size = strlen(gov_action_id);
 *
 * cardano_tx_builder_propose_no_confidence_ex(builder, reward_address, reward_address_size,
 *                                             metadata_url, metadata_url_size, metadata_hash_hex,
 *                                             metadata_hash_hex_size, gov_action_id,
 *                                             gov_action_id_size);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_no_confidence_ex(
  cardano_tx_builder_t* builder,
  const char*           reward_address,
  size_t                reward_address_size,
  const char*           metadata_url,
  size_t                metadata_url_size,
  const char*           metadata_hash_hex,
  size_t                metadata_hash_hex_size,
  const char*           gov_action_id,
  size_t                gov_action_id_size);

/**
 * \brief Proposes an update to the Cardano constitutional committee.
 *
 * This function prepares a governance action to update the Cardano constitutional committee. The proposal specifies:
 * - Committee members to be added.
 * - Committee members to be removed.
 * - The new quorum threshold for the committee.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address         A pointer to a \ref cardano_reward_address_t object representing the reward address that will receive the deposit refund.
 * \param[in]     anchor                 A pointer to a \ref cardano_anchor_t object representing the anchor for this governance action.
 * \param[in]     governance_action_id   An optional pointer to a \ref cardano_governance_action_id_t object representing the most recently enacted governance action
 *                                        of the same type. Can be NULL if no such prior action exists.
 * \param[in]     members_to_be_removed  A pointer to a \ref cardano_credential_set_t object specifying the committee members to be removed.
 * \param[in]     members_to_be_added    A pointer to a \ref cardano_committee_members_map_t object specifying the committee members to be added.
 * \param[in]     new_quorum             A pointer to a \ref cardano_unit_interval_t object specifying the new quorum threshold for the committee.
 *
 * \note If a prior governance action of the same type exists, the `governance_action_id` parameter must reference it.
 *
 * \note Any errors encountered during the addition of the update to the Cardano constitutional committee proposal will be deferred until
 *       \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;                // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...;     // Reward address to receive refund
 * cardano_anchor_t* anchor = ...;                     // Governance action anchor
 * cardano_governance_action_id_t* gov_action_id = ...;// Most recently enacted action ID
 * cardano_credential_set_t* members_to_remove = ...;  // Members to be removed
 * cardano_committee_members_map_t* members_to_add = ...; // Members to be added
 * cardano_unit_interval_t* new_quorum = ...;          // New quorum threshold
 *
 * cardano_tx_builder_propose_update_committee(builder, reward_address, anchor,
 *                                             gov_action_id, members_to_remove,
 *                                             members_to_add, new_quorum);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_update_committee(
  cardano_tx_builder_t*            builder,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  cardano_unit_interval_t*         new_quorum);

/**
 * \brief Proposes an update to the Cardano constitutional committee with extended parameters.
 *
 * This function creates a governance action to update the Cardano constitutional committee.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address         A pointer to a string containing the reward address (in Bech32 format) that will receive the deposit refund.
 * \param[in]     reward_address_size    The size of the `reward_address` string.
 * \param[in]     metadata_url           A pointer to a string containing the URL for the governance action metadata.
 * \param[in]     metadata_url_size      The size of the `metadata_url` string.
 * \param[in]     metadata_hash_hex      A pointer to a string containing the hex-encoded hash of the metadata file.
 * \param[in]     metadata_hash_hex_size The size of the `metadata_hash_hex` string.
 * \param[in]     gov_action_id          A CIP-0129 bech32 string representing the governance action ID that references the most recent enacted action of the same type.
 * \param[in]     gov_action_id_size     The size (in bytes) of the `gov_action_id` string.
 * \param[in]     members_to_be_removed  A pointer to a \ref cardano_credential_set_t object specifying the committee members to be removed.
 * \param[in]     members_to_be_added    A pointer to a \ref cardano_committee_members_map_t object specifying the committee members to be added.
 * \param[in]     new_quorum             The new quorum threshold for the committee.
 *
 * \note Any errors encountered during the addition of the update to the Cardano constitutional committee proposal will be deferred until
 *       \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;                // Initialized transaction builder
 * const char* reward_address = "addr1...";            // Bech32 reward address
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com/proposal.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abc123...";        // Hex-encoded hash of metadata file
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 * const char* gov_action_id = "gov_action1...";      // Optional governance action ID
 * size_t gov_action_id_size = strlen(gov_action_id);
 * cardano_credential_set_t* members_to_remove = ...;  // Members to be removed
 * cardano_committee_members_map_t* members_to_add = ...; // Members to be added
 *
 * cardano_tx_builder_propose_update_committee_ex(builder,
 *                                                reward_address, reward_address_size,
 *                                                metadata_url, metadata_url_size,
 *                                                metadata_hash_hex, metadata_hash_hex_size,
 *                                                gov_action1, gov_action1_size,
 *                                                members_to_remove,
 *                                                members_to_add,
 *                                                0.3);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_update_committee_ex(
  cardano_tx_builder_t*            builder,
  const char*                      reward_address,
  size_t                           reward_address_size,
  const char*                      metadata_url,
  size_t                           metadata_url_size,
  const char*                      metadata_hash_hex,
  size_t                           metadata_hash_hex_size,
  const char*                      gov_action_id,
  size_t                           gov_action_id_size,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  double                           new_quorum);

/**
 * \brief Proposes a new constitution for the Cardano network.
 *
 * This function creates a governance action to propose a new constitution.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address         A pointer to a \ref cardano_reward_address_t object representing the address that will receive the deposit refund.
 * \param[in]     anchor                 A pointer to a \ref cardano_anchor_t object containing the anchor metadata for the proposal.
 * \param[in]     governance_action_id   An optional pointer to a \ref cardano_governance_action_id_t object representing the governance action ID of the most
 *                                        recently enacted action of the same type. This parameter can be NULL if no prior action exists.
 * \param[in]     constitution           A pointer to a \ref cardano_constitution_t object containing the new constitution's details.
 *
 * \note The `governance_action_id` parameter ensures compliance with the protocol's requirement of referencing the most recently enacted action of the same type.
 *       If no such prior action exists, this parameter can be omitted (set to NULL).
 *
 * \note Any errors encountered during the addition of the new constitution proposal will be deferred until \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;                  // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...;       // Reward address to refund deposit
 * cardano_anchor_t* anchor = ...;                       // Anchor metadata
 * cardano_governance_action_id_t* gov_action_id = ...;  // Optional governance action ID
 * cardano_constitution_t* constitution = ...;           // Constitution details
 *
 * cardano_tx_builder_propose_new_constitution(builder, reward_address, anchor, gov_action_id, constitution);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_new_constitution(
  cardano_tx_builder_t*           builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  cardano_constitution_t*         constitution);

/**
 * \brief Proposes a new constitution for the Cardano network.
 *
 * This function allows creating a governance action to propose a new constitution while providing additional metadata
 * (e.g., URL and hash).
 *
 * \param[in,out] builder              A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address       A pointer to the reward address as a string. The address will receive the deposit refund.
 * \param[in]     reward_address_size  The size of the reward address string in bytes.
 * \param[in]     metadata_url         A pointer to a string containing the URL for additional proposal metadata.
 * \param[in]     metadata_url_size    The size of the metadata URL string in bytes.
 * \param[in]     metadata_hash_hex    A pointer to a hexadecimal string representing the hash of the metadata file.
 * \param[in]     metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in]     gov_action_id          A CIP-0129 bech32 string representing the governance action ID that references the most recent enacted action of the same type.
 * \param[in]     gov_action_id_size     The size (in bytes) of the `gov_action_id` string.
 * \param[in]     constitution         A pointer to a \ref cardano_constitution_t object containing the new constitution's details.
 *
 * \note Any errors encountered during the addition of the new constitution proposal will be deferred until \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;                 // Initialized transaction builder
 * const char* reward_address = "addr1...";             // Reward address string
 * size_t reward_address_size = strlen(reward_address); // Size of the reward address string
 * const char* metadata_url = "https://example.com";    // Metadata URL string
 * size_t metadata_url_size = strlen(metadata_url);     // Size of the metadata URL string
 * const char* metadata_hash_hex = "abc123...";         // Metadata hash in hexadecimal
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex); // Size of the metadata hash string
 * const char* gov_action_id = "gov_action1...";       // Governance action ID in hexadecimal
 * size_t gov_action_id_size = strlen(gov_action_id); // Size of the governance action ID string
 * cardano_constitution_t* constitution = ...;          // Constitution details
 *
 * cardano_tx_builder_propose_new_constitution_ex(
 *   builder, reward_address, reward_address_size,
 *   metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size,
 *   gov_action_id, gov_action_id_size, constitution);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_new_constitution_ex(
  cardano_tx_builder_t*   builder,
  const char*             reward_address,
  size_t                  reward_address_size,
  const char*             metadata_url,
  size_t                  metadata_url_size,
  const char*             metadata_hash_hex,
  size_t                  metadata_hash_hex_size,
  const char*             gov_action_id,
  size_t                  gov_action_id_size,
  cardano_constitution_t* constitution);

/**
 * \brief Proposes an informational governance action for the Cardano network.
 *
 * This function creates a governance action proposal to share information with the network.
 *
 * \param[in,out] builder        A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address A pointer to the reward address where the deposit refund will be sent if the proposal is enacted or discarded.
 * \param[in]     anchor         A pointer to a \ref cardano_anchor_t object containing metadata such as URLs and hashes to link to external information.
 *
 * \note Any errors encountered during the addition of the informational proposal will be deferred until \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;            // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...; // Reward address for deposit refund
 * cardano_anchor_t* anchor = ...;                 // Anchor metadata for the proposal
 *
 * cardano_tx_builder_propose_info(builder, reward_address, anchor);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_info(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor);

/**
 * \brief Proposes an informational governance action for the Cardano network.
 *
 * This function creates a governance action proposal to share information with the network.
 *
 * \param[in,out] builder              A pointer to an initialized \ref cardano_tx_builder_t object used to build the transaction.
 * \param[in]     reward_address       A pointer to the reward address in string format where the deposit refund will be sent if the proposal is enacted or discarded.
 * \param[in]     reward_address_size  The size of the reward address string in bytes.
 * \param[in]     metadata_url         A pointer to a string containing the URL for the proposal metadata. This parameter is optional and can be NULL if no URL is provided.
 * \param[in]     metadata_url_size    The size of the metadata URL string in bytes. Set to 0 if no URL is provided.
 * \param[in]     metadata_hash_hex    A pointer to a string containing the hexadecimal hash of the metadata. This parameter is optional and can be NULL if no hash is provided.
 * \param[in]     metadata_hash_hex_size The size of the metadata hash string in bytes. Set to 0 if no hash is provided.
 *
 * \note Any errors encountered during the addition of the informational proposal will be deferred until \ref cardano_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* builder = ...;   // Initialized transaction builder
 * const char* reward_address = "addr1..."; // Reward address as a string
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com/proposal-metadata";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abcdef123456...";
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 *
 * cardano_tx_builder_propose_info_ex(
 *     builder, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size);
 * \endcode
 */
CARDANO_EXPORT void cardano_tx_builder_propose_info_ex(
  cardano_tx_builder_t* builder,
  const char*           reward_address,
  size_t                reward_address_size,
  const char*           metadata_url,
  size_t                metadata_url_size,
  const char*           metadata_hash_hex,
  size_t                metadata_hash_hex_size);

/**
 * \brief Builds the transaction from the current state of the transaction builder.
 *
 * This function finalizes the transaction by aggregating all previously added inputs, outputs, certificates, and other data.
 * If any required data is missing or incorrect, this function will report the errors encountered during the build process.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance that manages the transaction details.
 * \param[out] transaction A pointer to a \ref cardano_transaction_t pointer where the created transaction will be stored upon success.
 *
 * \returns \ref CARDANO_SUCCESS if the transaction was successfully built, or an appropriate error code if issues were detected.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* transaction = NULL;
 * cardano_tx_builder_t* tx_builder   = ...; // Initialized transaction builder
 *
 * cardano_tx_builder_withdraw_rewards_ex(tx_builder, "stake...", size, NULL);                      // Withdraw rewards
 * cardano_tx_builder_send_lovelace_ex(tx_builder, "add...", size, 50000);                          // Add inputs, outputs, and other transaction details
 * cardano_tx_builder_set_invalid_after(tx_builder, 1000);                                          // Set the transaction validity
 * cardano_tx_builder_mint_token_ex(tx_builder, "00ff...", policy_size, "ff09...", name_size, 100); // Mint tokens
 *
 * cardano_error_t result = cardano_tx_builder_build(tx_builder, &transaction);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   const char* error_msg = cardano_tx_builder_get_last_error(tx_builder);
 *   printf("Failed to build transaction: %s\n", error_msg);
 * }
 * else
 * {
 *   // Transaction successfully built; proceed with processing
 *   // ...
 *   cardano_transaction_unref(&transaction);
 * }
 *
 * cardano_tx_builder_unref(&tx_builder);
 * \endcode
 *
 * \note All errors related to missing or invalid data will be reported upon calling this function.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_tx_builder_build(
  cardano_tx_builder_t*   builder,
  cardano_transaction_t** transaction);

/**
 * \brief Decrements the reference count of a cardano_tx_builder_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_tx_builder_t object
 * by decreasing its reference count. When the reference count reaches zero, the transaction_builder is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] transaction_builder A pointer to the pointer of the transaction_builder object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* transaction_builder = cardano_tx_builder_new(major, minor);
 *
 * // Perform operations with the transaction_builder...
 *
 * cardano_tx_builder_unref(&transaction_builder);
 * // At this point, transaction_builder is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_tx_builder_unref, the pointer to the \ref cardano_tx_builder_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_tx_builder_unref(cardano_tx_builder_t** transaction_builder);

/**
 * \brief Increases the reference count of the cardano_tx_builder_t object.
 *
 * This function is used to manually increment the reference count of an cardano_tx_builder_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_tx_builder_unref.
 *
 * \param transaction_builder A pointer to the cardano_tx_builder_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_builder is a previously created transaction_builder object
 *
 * cardano_tx_builder_ref(transaction_builder);
 *
 * // Now transaction_builder can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_tx_builder_ref there is a corresponding
 * call to \ref cardano_tx_builder_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_tx_builder_ref(cardano_tx_builder_t* transaction_builder);

/**
 * \brief Retrieves the current reference count of the cardano_tx_builder_t object.
 *
 * This function returns the number of active references to an cardano_tx_builder_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_tx_builder_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param transaction_builder A pointer to the cardano_tx_builder_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_tx_builder_t object. If the object
 * is properly managed (i.e., every \ref cardano_tx_builder_ref call is matched with a
 * \ref cardano_tx_builder_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_builder is a previously created transaction_builder object
 *
 * size_t ref_count = cardano_tx_builder_refcount(transaction_builder);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_tx_builder_refcount(const cardano_tx_builder_t* transaction_builder);

/**
 * \brief Sets the last error message for a given cardano_tx_builder_t object.
 *
 * Records an error message in the transaction_builder's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] transaction_builder A pointer to the \ref cardano_tx_builder_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the transaction_builder's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_tx_builder_set_last_error(
  cardano_tx_builder_t* transaction_builder,
  const char*           message);

/**
 * \brief Retrieves the last error message recorded for a specific transaction_builder.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_tx_builder_set_last_error for the given
 * transaction_builder. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] transaction_builder A pointer to the \ref cardano_tx_builder_t instance whose last error
 *                   message is to be retrieved. If the transaction_builder is NULL, the function
 *                   returns a generic error message indicating the null transaction_builder.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified transaction_builder. If the transaction_builder is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_tx_builder_set_last_error for the same transaction_builder, or until
 *       the transaction_builder is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_tx_builder_get_last_error(
  const cardano_tx_builder_t* transaction_builder);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BUILDER_H
