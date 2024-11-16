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
#include <cardano/providers/provider.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Type definition for the Cardano Transaction Builder.
 *
 * The `cardano_tx_builder_t` type represents an instance of a transaction builder for creating
 * and managing Cardano transactions. It provides methods for adding inputs and outputs, setting
 * fees, and ensuring that the transaction is balanced according to protocol parameters.
 *
 * A transaction builder simplifies the creation of complex transactions by allowing the incremental
 * addition of transaction elements, handling necessary computations such as fee calculations and
 * change outputs, and enforcing protocol compliance.
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
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t that serves as the redeemer for
 *                     script-locked withdrawals, if applicable.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;           // Initialized transaction builder
 * cardano_reward_address_t* reward_address = ...;    // Initialized reward address
 * cardano_plutus_data_t* redeemer = ...;            // Optional redeemer data
 *
 * cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, redeemer);
 * \endcode
 *
 * \note The transaction builder will use the configured \ref cardano_provider_t instance to retrieve the available rewards for the
 *       specified reward account. Errors related to reward withdrawal will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_withdraw_rewards(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
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
 * cardano_tx_builder_withdraw_rewards_ex(tx_builder, reward_addr, address_size, redeemer);
 * \endcode
 *
 * \note The transaction builder will use the configured \ref cardano_provider_t instance to retrieve the available rewards for
 *       the specified reward address. Errors related to reward withdrawal will be deferred until `cardano_tx_builder_build` is called.
 *       Ensure the reward address and redeemer remain valid until the transaction is built.
 */
CARDANO_EXPORT void cardano_tx_builder_withdraw_rewards_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
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
 * \param[in] reward_address A pointer to a hexadecimal string representing the reward address.
 * \param[in] address_size The size of the `reward_address` string.
 * \param[in] redeemer An optional pointer to \ref cardano_plutus_data_t, used if the registration requires
 *                     redeemer data for script-locked transactions.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;       // Initialized transaction builder
 * const char* reward_address_hex = "abcdef..."; // Reward address in hexadecimal format
 * size_t address_size = strlen(reward_address_hex);
 * cardano_plutus_data_t* redeemer = ...;        // Optional redeemer data for script-locked registration
 *
 * cardano_tx_builder_register_reward_address_ex(tx_builder, reward_address_hex, address_size, redeemer);
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
 * \param[in] reward_address A hex-encoded string representing the staking reward address for delegation.
 * \param[in] address_size The size of the `reward_address` string.
 * \param[in] pool_id A hex-encoded string representing the pool ID to which stake is delegated.
 * \param[in] pool_id_size The size of the `pool_id` string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t for providing redeemer data if required for script-locked accounts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* reward_address = "stake1u9...";  // Hex-encoded reward address for staking
 * size_t reward_address_size = strlen(reward_address);
 * const char* pool_id = "pool1xy...";  // Hex-encoded pool ID for delegation
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
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] reward_address A string representing the staking reward address for the account delegating voting power.
 * \param[in] address_size The length of the `reward_address` string.
 * \param[in] drep_id A string representing the ID of the decentralized representative (DRep) receiving the voting power.
 * \param[in] drep_id_size The length of the `drep_id` string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t for providing redeemer data, useful for script-locked accounts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;  // Initialized transaction builder
 * const char* reward_address = "stake1u9...";  // Reward address delegating voting power
 * size_t address_size = strlen(reward_address);
 * const char* drep_id = "drep123...";  // DRep ID receiving delegated voting power
 * size_t drep_id_size = strlen(drep_id);
 * cardano_plutus_data_t* redeemer = ...;  // Optional redeemer data
 *
 * cardano_tx_builder_delegate_voting_power_ex(tx_builder, reward_address, address_size, drep_id, drep_id_size, redeemer);
 * \endcode
 *
 * \note Errors related to this delegation will be deferred until `cardano_tx_builder_build` is called.
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
 * \brief Registers a decentralized representative DRep by ID in the transaction.
 *
 * This function registers a DRep in the transaction using a specified DRep ID, allowing the inclusion of an optional
 * governance anchor and redeemer data for script DReps.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep_id A pointer to a character array containing the DRep ID as a hex-encoded string.
 * \param[in] drep_id_size The size of the `drep_id` string.
 * \param[in] anchor An optional pointer to a \ref cardano_anchor_t instance providing governance anchor details.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required for
 *                     script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;    // Initialized transaction builder
 * const char* drep_id = "abc123...";         // Hex-encoded DRep ID
 * size_t drep_id_size = strlen(drep_id);
 * cardano_anchor_t* anchor = ...;            // Optional anchor
 * cardano_plutus_data_t* redeemer = ...;     // Optional redeemer data
 *
 * cardano_tx_builder_register_drep_ex(tx_builder, drep_id, drep_id_size, anchor, redeemer);
 * \endcode
 *
 * \note Errors related to this registration will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_register_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_anchor_t*      anchor,
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
 * This function updates a DRep in the transaction using a specified DRep ID in hexadecimal format, optionally providing
 * a governance anchor and redeemer data for validation purposes associated with script-locked DReps.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep_id A pointer to a character array representing the hexadecimal DRep ID.
 * \param[in] drep_id_size The size of the DRep ID string.
 * \param[in] anchor An optional pointer to a \ref cardano_anchor_t instance, providing governance anchor details associated with the DRep.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required for
 *                     script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;    // Initialized transaction builder
 * const char* drep_id = "abcdef123456";      // DRep ID in hexadecimal
 * size_t drep_id_size = strlen(drep_id);
 * cardano_anchor_t* anchor = ...;            // Optional anchor
 * cardano_plutus_data_t* redeemer = ...;     // Optional redeemer data
 *
 * cardano_tx_builder_update_drep_ex(tx_builder, drep_id, drep_id_size, anchor, redeemer);
 * \endcode
 *
 * \note Errors associated with updating the DRep will be deferred until `cardano_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_tx_builder_update_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_anchor_t*      anchor,
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
 * This function deregisters a DRep, identified by its ID.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance managing the transaction.
 * \param[in] drep_id A pointer to a character array containing the ID of the DRep to deregister.
 * \param[in] drep_id_size The size of the DRep ID string.
 * \param[in] redeemer An optional pointer to a \ref cardano_plutus_data_t instance for providing redeemer data, which may be required for
 *                     script-locked DReps.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_builder_t* tx_builder = ...;   // Initialized transaction builder
 * const char* drep_id = "drep_id_example";  // DRep ID to deregister
 * size_t drep_id_size = strlen(drep_id);
 * cardano_plutus_data_t* redeemer = ...;    // Optional redeemer data
 *
 * cardano_tx_builder_deregister_drep_ex(tx_builder, drep_id, drep_id_size, redeemer);
 * \endcode
 *
 * \note Errors related to deregistering the DRep will be deferred until `cardano_tx_builder_build` is called.
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
