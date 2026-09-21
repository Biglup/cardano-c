/**
 * \file sub_transaction_builder.h
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_BUILDER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_BUILDER_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/address/reward_address.h>
#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_name.h>
#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/certs/certificate.h>
#include <cardano/common/anchor.h>
#include <cardano/common/credential.h>
#include <cardano/common/drep.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/network_id.h>
#include <cardano/common/protocol_version.h>
#include <cardano/common/unit_interval.h>
#include <cardano/common/utxo.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/proposal_procedures/committee_members_map.h>
#include <cardano/proposal_procedures/constitution.h>
#include <cardano/proposal_procedures/credential_set.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/scripts/script.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/account_balance_interval.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/value.h>
#include <cardano/typedefs.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voting_procedure.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief High-Level Sub Transaction Builder for Cardano Blockchain.
 *
 * The `cardano_sub_tx_builder_t` type is the interface a party of a CIP-118 batch uses to construct its
 * sub transaction programmatically. A sub transaction is an intent: independent parties each build and sign
 * one, and a batcher aggregates the finished sub transactions into a top level transaction that pays the fee
 * and posts the collateral for the whole batch. The ledger checks value conservation over the whole batch, not
 * over each sub transaction.
 *
 * **Key Features:**
 * - **Modular Design**: Incrementally add inputs, outputs, minting operations, metadata, native scripts,
 *   guards, direct deposits, account balance intervals, withdrawals, certificates, votes and governance
 *   proposals.
 * - **Never Balances**: The builder does not select inputs, does not add change outputs and does not compute
 *   fees. The sub transaction carries exactly the inputs and outputs that were added, and its imbalance (see
 *   \ref cardano_compute_sub_transaction_imbalance) is the intent the batcher matches against the rest of
 *   the batch.
 * - **Sub Transaction Fields Only**: Fees, collateral and starting account balance intervals only exist on a
 *   top level transaction, so this builder has no functions for them.
 * - **Native Scripts and Key Witnesses**: PlutusV1, PlutusV2 and PlutusV3 scripts can not run inside a sub
 *   transaction and PlutusV4 scripts are not supported by this builder yet, so no function of this builder
 *   takes a redeemer and Plutus scripts are rejected. For the same reason the governance proposals that are
 *   validated by the guardrails script, which is a Plutus script, have no functions on this builder: parameter
 *   change and treasury withdrawals proposals belong in a top level transaction.
 */
typedef struct cardano_sub_tx_builder_t cardano_sub_tx_builder_t;

/**
 * \brief Creates a new sub transaction builder instance.
 *
 * This function initializes a new sub transaction builder (`cardano_sub_tx_builder_t`)
 * using the supplied protocol parameters and slot configuration. The builder
 * enables incremental construction of a CIP-118 sub transaction.
 *
 * The slot configuration provides time/slot conversion rules needed for
 * unix time to slot conversions.
 *
 * \param[in] params
 *   A pointer to a \ref cardano_protocol_parameters_t structure containing the
 *   protocol parameters used to configure the sub transaction builder.
 *   Must not be `NULL`.
 *
 * \param[in] slot_config
 *   A pointer to the \ref cardano_slot_config_t structure that defines the
 *   slot/time translation rules for the current network. Must not be `NULL`. Constants for every public network can be found at `cardano/slot_config.h`:
 *   \ref CARDANO_MAINNET_SLOT_CONFIG
 *   \ref CARDANO_PREVIEW_SLOT_CONFIG
 *   \ref CARDANO_PREPROD_SLOT_CONFIG
 *
 * \returns
 *   A pointer to a newly created \ref cardano_sub_tx_builder_t instance configured
 *   with the given protocol parameters and slot configuration, or `NULL` if
 *   memory allocation fails or if any argument is invalid.
 *
 * \par Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...;            // Initialized protocol parameters
 * cardano_slot_config_t slot_config = CARDANO_PREVIEW_SLOT_CONFIG; // Constants for slot config for every public network can be found at cardano/slot_config.h
 *
 * cardano_sub_tx_builder_t* sub_tx_builder =
 *     cardano_sub_tx_builder_new(protocol_params, &slot_config);
 *
 * if (sub_tx_builder != NULL)
 * {
 *   // Proceed with constructing the sub transaction
 * }
 *
 * cardano_sub_tx_builder_unref(&sub_tx_builder);
 * \endcode
 *
 * \note
 * The caller is responsible for releasing the `cardano_sub_tx_builder_t` instance
 * using \ref cardano_sub_tx_builder_unref once it is no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_sub_tx_builder_t* cardano_sub_tx_builder_new(
  cardano_protocol_parameters_t* params,
  const cardano_slot_config_t*   slot_config);

/**
 * \brief Sets the network ID for the sub transaction builder.
 *
 * This function configures the sub transaction builder (`cardano_sub_tx_builder_t`) with a specific network ID.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance for which the network ID is being set.
 * \param[in] network_id The network ID to assign to the sub transaction. This ID specifies the target network
 *                       (e.g., mainnet, testnet) for which the sub transaction is intended.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_network_id_t network_id = CARDANO_NETWORK_ID_MAIN_NET;  // Set the network ID to mainnet
 * cardano_sub_tx_builder_set_network_id(sub_tx_builder, network_id);
 * \endcode
 *
 * \note This function only sets the network ID within the builder and does not perform any validation until
 *       \ref cardano_sub_tx_builder_build is called, which will report any related errors at that time.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_network_id(cardano_sub_tx_builder_t* builder, cardano_network_id_t network_id);

/**
 * \brief Sets the donation amount for the sub transaction builder.
 *
 * This function sets the donation amount. A donation contributes a specific amount to the treasury and is
 * value produced by the sub transaction.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance where the donation will be set.
 * \param[in] donation The donation amount, specified in lovelace.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...; // Initialized sub transaction builder
 * uint64_t donation = 1000000;                   // 1 ADA donation in lovelace
 *
 * cardano_sub_tx_builder_set_donation(sub_tx_builder, donation);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_donation(cardano_sub_tx_builder_t* builder, uint64_t donation);

/**
 * \brief Sets the expiration slot for a sub transaction, beyond which it is no longer valid.
 *
 * This function specifies the slot number after which the sub transaction will be considered invalid. Setting this
 * value lets an intent expire if no batcher includes it in a block before the specified slot.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance being configured.
 * \param[in] slot The slot number after which the sub transaction will be marked as invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * uint64_t expiration_slot = 5000000;  // Slot after which the sub transaction becomes invalid
 * cardano_sub_tx_builder_set_invalid_after(sub_tx_builder, expiration_slot);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_invalid_after(cardano_sub_tx_builder_t* builder, uint64_t slot);

/**
 * \brief Sets the expiration time for a sub transaction based on Unix time (in seconds).
 *
 * This function configures the validity of the sub transaction based on a specified Unix timestamp, marking it
 * as invalid if it is not included in a block before this time. The Unix timestamp is expressed in seconds since
 * the epoch (January 1, 1970, UTC). This setting is useful for intents that need to expire at a specific
 * real-world time.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance being configured.
 * \param[in] unix_time The Unix timestamp (in seconds) at which the sub transaction expires.
 *
 * Usage Example:
 * \code{.c}
 * #include <time.h>
 *
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;   // Initialized sub transaction builder
 * time_t current_time = time(NULL);                 // Get current Unix time in seconds
 * uint64_t expiration_time = current_time + 2 * 60 * 60;  // Set expiration to 2 hours from now
 *
 * cardano_sub_tx_builder_set_invalid_after_ex(sub_tx_builder, expiration_time);
 * \endcode
 *
 * \note If any errors occur while setting the expiration, they will not be immediately reported. Instead, they will
 *       be deferred and can be checked when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_invalid_after_ex(cardano_sub_tx_builder_t* builder, uint64_t unix_time);

/**
 * \brief Sets the minimum valid slot for a sub transaction.
 *
 * This function configures the earliest slot at which the sub transaction will be considered valid. Sub transactions
 * created with this setting will not be valid for inclusion in a block until the specified slot.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance being configured.
 * \param[in] slot The minimum slot number at which the sub transaction is valid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;   // Initialized sub transaction builder
 * uint64_t min_valid_slot = 123456;                 // Example slot number
 * cardano_sub_tx_builder_set_invalid_before(sub_tx_builder, min_valid_slot);
 * \endcode
 *
 * \note Any errors arising from setting this parameter will be reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_invalid_before(cardano_sub_tx_builder_t* builder, uint64_t slot);

/**
 * \brief Sets the minimum valid time (Unix timestamp) for a sub transaction.
 *
 * This function configures the earliest time, in Unix timestamp format (seconds since Epoch),
 * at which the sub transaction will be considered valid. Sub transactions created with this setting will
 * not be valid for inclusion in a block until the specified timestamp.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance being configured.
 * \param[in] unix_time The minimum Unix timestamp (in seconds) at which the sub transaction is valid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;   // Initialized sub transaction builder
 * uint64_t min_valid_time = 7200 + time(NULL);      // Set to 2 hours from now
 * cardano_sub_tx_builder_set_invalid_before_ex(sub_tx_builder, min_valid_time);
 * \endcode
 *
 * \note Any errors resulting from setting this parameter will be reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_invalid_before_ex(cardano_sub_tx_builder_t* builder, uint64_t unix_time);

/**
 * \brief Adds an input to the sub transaction.
 *
 * This function appends a specified UTXO as an input to the sub transaction being built. The builder never selects
 * inputs by itself, so the sub transaction spends exactly the UTXOs added through this function. Only inputs locked
 * by a key or by a native script can be spent, since this builder only supports native scripts and key witnesses.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance in which to add the input.
 * \param[in] utxo A pointer to the \ref cardano_utxo_t structure representing the resolved UTXO to be used as an input.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_utxo_t* input_utxo = ...;                // UTXO to add as an input
 *
 * cardano_sub_tx_builder_add_input(sub_tx_builder, input_utxo);
 * \endcode
 *
 * \note Any errors resulting from adding this input will be deferred and reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_input(
  cardano_sub_tx_builder_t* builder,
  cardano_utxo_t*           utxo);

/**
 * \brief Adds a reference input to the sub transaction.
 *
 * This function appends a specified UTXO as a reference input to the sub transaction being built.
 * Reference inputs give access to the data and the reference script of a UTXO without consuming it.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance in which to add the reference input.
 * \param[in] utxo A pointer to the \ref cardano_utxo_t structure representing the UTXO to be used as a reference input.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_utxo_t* reference_utxo = ...;            // UTXO to add as a reference input
 * cardano_sub_tx_builder_add_reference_input(sub_tx_builder, reference_utxo);
 * \endcode
 *
 * \note Any errors resulting from adding this input will be reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_reference_input(cardano_sub_tx_builder_t* builder, cardano_utxo_t* utxo);

/**
 * \brief Adds an output to the sub transaction.
 *
 * This function appends a specified transaction output to the sub transaction being built.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance to which the output will be added.
 * \param[in] output A pointer to the \ref cardano_transaction_output_t structure representing the transaction output to be added.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;    // Initialized sub transaction builder
 * cardano_transaction_output_t* tx_output = ...;     // Transaction output to add
 *
 * cardano_sub_tx_builder_add_output(sub_tx_builder, tx_output);
 * \endcode
 *
 * \note Any errors related to adding this output will be deferred and reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_output(
  cardano_sub_tx_builder_t*     builder,
  cardano_transaction_output_t* output);

/**
 * \brief Sends a specified amount of lovelace to a given address.
 *
 * This function adds a transaction output to the builder, targeting a specific address and setting the amount of lovelace to be sent.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance to which the output will be added.
 * \param[in] address A pointer to the \ref cardano_address_t structure representing the recipient address.
 * \param[in] amount The amount of lovelace to send to the specified address.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;    // Initialized sub transaction builder
 * cardano_address_t* recipient_address = ...;        // Recipient address
 * uint64_t lovelace_amount = 1000000;                // Amount of lovelace to send (e.g., 1 ADA)
 *
 * cardano_sub_tx_builder_send_lovelace(sub_tx_builder, recipient_address, lovelace_amount);
 * \endcode
 *
 * \note Any errors related to sending lovelace will be deferred and reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_send_lovelace(
  cardano_sub_tx_builder_t* builder,
  cardano_address_t*        address,
  uint64_t                  amount);

/**
 * \brief Sends a specified amount of lovelace to a given address in string format.
 *
 * This function adds a transaction output to the builder, targeting a specific address in string format and setting the amount of lovelace to be sent.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance to which the output will be added.
 * \param[in] address A pointer to a character array containing the recipient address in string format.
 * \param[in] address_size The size of the address string.
 * \param[in] amount The amount of lovelace to send to the specified address.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* recipient_address = "addr1...";      // Recipient address in string format
 * size_t address_size = strlen(recipient_address); // Size of the address string
 * uint64_t lovelace_amount = 1000000;              // Amount of lovelace to send (e.g., 1 ADA)
 *
 * cardano_sub_tx_builder_send_lovelace_ex(sub_tx_builder, recipient_address, address_size, lovelace_amount);
 * \endcode
 *
 * \note Errors, such as an invalid address, will be deferred and reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_send_lovelace_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               address,
  size_t                    address_size,
  uint64_t                  amount);

/**
 * \brief Sends a specified value (which may include ADA and other assets) to a given address.
 *
 * This function adds a transaction output to the builder, specifying an address and a complex value that can include multiple assets in addition to ADA.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance to which the output will be added.
 * \param[in] address A pointer to a \ref cardano_address_t object representing the recipient address.
 * \param[in] value A pointer to a \ref cardano_value_t object containing the ADA and any additional assets to send.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_address_t* recipient_address = ...;      // Address to send to
 * cardano_value_t* transfer_value = ...;           // Value to transfer (e.g., ADA and assets)
 *
 * cardano_sub_tx_builder_send_value(sub_tx_builder, recipient_address, transfer_value);
 * \endcode
 *
 * \note Errors, such as invalid assets, will be deferred and reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_send_value(
  cardano_sub_tx_builder_t* builder,
  cardano_address_t*        address,
  cardano_value_t*          value);

/**
 * \brief Sends a specified value (which may include ADA and other assets) to a given address
 * provided as raw string.
 *
 * This function adds a transaction output to the builder, specifying an address in
 * character array format along with a complex value that may include multiple assets.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance to which the output will be added.
 * \param[in] address A character array containing the recipient's address in bech32 or hex format.
 * \param[in] address_size The length of the address character array.
 * \param[in] value A pointer to a \ref cardano_value_t object containing the ADA and any additional assets to send.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* recipient_address = "addr1...";      // Bech32 address
 * size_t address_size = strlen(recipient_address);
 * cardano_value_t* transfer_value = ...;           // Value to transfer (e.g., ADA and assets)
 *
 * cardano_sub_tx_builder_send_value_ex(sub_tx_builder, recipient_address, address_size, transfer_value);
 * \endcode
 *
 * \note Errors, such as an invalid address or invalid assets, will be deferred and reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_send_value_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               address,
  size_t                    address_size,
  cardano_value_t*          value);

/**
 * \brief Sets metadata for a sub transaction builder with a specified tag.
 *
 * This function attaches metadata to the sub transaction being built, associating it with a specific metadata tag. Metadata can include additional information for the sub transaction, often used to encode arbitrary data on-chain.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance where the metadata will be added.
 * \param[in] tag A unique identifier for the metadata, typically a 64-bit unsigned integer.
 * \param[in] metadata A pointer to the \ref cardano_metadatum_t object containing the metadata to be added.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * uint64_t metadata_tag = 12345;                   // Example metadata tag
 * cardano_metadatum_t* metadata = ...;             // Initialized metadata object
 *
 * cardano_sub_tx_builder_set_metadata(sub_tx_builder, metadata_tag, metadata);
 * \endcode
 *
 * \note This function only stores a reference to the metadata. Errors, such as invalid metadata format, will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_metadata(
  cardano_sub_tx_builder_t* builder,
  uint64_t                  tag,
  cardano_metadatum_t*      metadata);

/**
 * \brief Sets metadata for a sub transaction builder from a JSON string with a specified tag.
 *
 * This function allows adding metadata to the sub transaction by parsing a JSON string representation of the metadata. The metadata is associated with a specified tag, allowing for organized and accessible on-chain data.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance where the metadata will be added.
 * \param[in] tag A unique identifier for the metadata, typically a 64-bit unsigned integer.
 * \param[in] metadata_json A pointer to a JSON string representing the metadata content.
 * \param[in] json_size The length of the `metadata_json` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * uint64_t metadata_tag = 12345;                   // Example metadata tag
 * const char* metadata_json = "{ \"name\": \"some_name\" }";  // Metadata JSON as string
 * size_t json_size = strlen(metadata_json);
 *
 * cardano_sub_tx_builder_set_metadata_ex(sub_tx_builder, metadata_tag, metadata_json, json_size);
 * \endcode
 *
 * \note This function will parse the JSON string to convert it into metadata. Errors, such as invalid JSON format, will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_metadata_ex(
  cardano_sub_tx_builder_t* builder,
  uint64_t                  tag,
  const char*               metadata_json,
  size_t                    json_size);

/**
 * \brief Adds a token minting operation to the sub transaction builder.
 *
 * This function allows the user to specify a token minting operation within a sub transaction, including
 * the policy ID, asset name and amount. This enables minting (positive amount) or burning (negative amount) of
 * tokens. The minting policy must be a native script, since this builder does not support Plutus scripts.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance for building the sub transaction.
 * \param[in] policy_id A pointer to a \ref cardano_blake2b_hash_t representing the unique minting policy ID.
 * \param[in] name A pointer to a \ref cardano_asset_name_t defining the name of the asset being minted or burned.
 * \param[in] amount The number of tokens to mint or burn; positive values indicate minting, and negative values indicate burning.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_blake2b_hash_t* policy_id = ...;         // Policy ID for the token
 * cardano_asset_name_t* asset_name = ...;          // Asset name for the token
 * int64_t mint_amount = 100;                       // Amount to mint
 *
 * cardano_sub_tx_builder_mint_token(sub_tx_builder, policy_id, asset_name, mint_amount);
 * \endcode
 *
 * \note Errors related to this operation will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_mint_token(
  cardano_sub_tx_builder_t* builder,
  cardano_blake2b_hash_t*   policy_id,
  cardano_asset_name_t*     name,
  int64_t                   amount);

/**
 * \brief Adds a token minting operation to the sub transaction builder using policy ID and asset name in hexadecimal format.
 *
 * This function allows the sub transaction builder to mint or burn tokens associated with a specific policy ID and asset name.
 * The minting policy must be a native script, since this builder does not support Plutus scripts.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] policy_id_hex A string in hexadecimal format representing the policy ID of the token to mint or burn.
 * \param[in] policy_id_size The size of the `policy_id_hex` string.
 * \param[in] name_hex A string in hexadecimal format representing the asset name within the policy ID.
 * \param[in] name_size The size of the `name_hex` string.
 * \param[in] amount The amount of tokens to mint (positive) or burn (negative).
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* policy_id_hex = "abcdef123456";      // Example policy ID in hexadecimal format
 * size_t policy_id_size = strlen(policy_id_hex);   // Size of policy ID
 * const char* name_hex = "74657374";               // Asset name in hexadecimal ("test" in hex)
 * size_t name_size = strlen(name_hex);             // Size of asset name
 * int64_t mint_amount = 100;                       // Amount to mint (positive for minting, negative for burning)
 *
 * cardano_sub_tx_builder_mint_token_ex(sub_tx_builder, policy_id_hex, policy_id_size, name_hex, name_size, mint_amount);
 * \endcode
 *
 * \note Any errors related to this operation will only be reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_mint_token_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               policy_id_hex,
  size_t                    policy_id_size,
  const char*               name_hex,
  size_t                    name_size,
  int64_t                   amount);

/**
 * \brief Adds a token minting operation to the sub transaction builder using a predefined asset ID.
 *
 * This function allows minting or burning of tokens within a sub transaction by specifying an asset ID along with the amount.
 * The minting policy must be a native script, since this builder does not support Plutus scripts.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used to build the sub transaction.
 * \param[in] asset_id A pointer to a \ref cardano_asset_id_t structure representing the asset to mint or burn.
 * \param[in] amount The amount of tokens to mint (positive) or burn (negative).
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_asset_id_t* asset_id = ...;              // Predefined asset ID for the token
 * int64_t mint_amount = 50;                        // Amount to mint (positive for minting, negative for burning)
 *
 * cardano_sub_tx_builder_mint_token_with_id(sub_tx_builder, asset_id, mint_amount);
 * \endcode
 *
 * \note Any errors related to this operation are deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_mint_token_with_id(
  cardano_sub_tx_builder_t* builder,
  cardano_asset_id_t*       asset_id,
  int64_t                   amount);

/**
 * \brief Adds a token minting operation to the sub transaction builder using an asset ID in hexadecimal format.
 *
 * This function enables minting or burning tokens for a specific asset identified by its unique `asset_id` in hexadecimal format.
 * The minting policy must be a native script, since this builder does not support Plutus scripts.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] asset_id_hex A string in hexadecimal format representing the unique asset ID.
 * \param[in] hex_size The size of the `asset_id_hex` string.
 * \param[in] amount The amount of tokens to mint (positive) or burn (negative).
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* asset_id_hex = "abcdef1234567890";   // Example asset ID in hexadecimal format
 * size_t hex_size = strlen(asset_id_hex);          // Size of asset ID
 * int64_t mint_amount = 100;                       // Amount to mint (positive for minting, negative for burning)
 *
 * cardano_sub_tx_builder_mint_token_with_id_ex(sub_tx_builder, asset_id_hex, hex_size, mint_amount);
 * \endcode
 *
 * \note Any errors will only be reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_mint_token_with_id_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               asset_id_hex,
  size_t                    hex_size,
  int64_t                   amount);

/**
 * \brief Adds a script to the sub transaction builder.
 *
 * This function allows the addition of a native script (`cardano_script_t`) to the witness set of the sub transaction.
 * Native scripts define the conditions under which outputs can be unlocked and tokens can be minted. PlutusV1,
 * PlutusV2 and PlutusV3 scripts can not run inside a sub transaction and PlutusV4 scripts are not supported by this
 * builder yet, so adding a Plutus script of any language version is reported as an error.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction details.
 * \param[in] script A pointer to the \ref cardano_script_t structure representing the native script to be added.
 *
 * \note Errors related to adding an invalid or incompatible script will be deferred and reported only when `cardano_sub_tx_builder_build` is called.
 *       A Plutus script is reported as \ref CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_script_t* script = ...;                  // A native script for the sub transaction
 *
 * cardano_sub_tx_builder_add_script(sub_tx_builder, script);
 *
 * // Additional sub transaction setup...
 *
 * cardano_error_t result = cardano_sub_tx_builder_build(sub_tx_builder, &sub_transaction);
 * if (result != CARDANO_SUCCESS)
 * {
 *   const char* error_msg = cardano_sub_tx_builder_get_last_error(sub_tx_builder);
 *   printf("Failed to build sub transaction with script: %s\n", error_msg);
 * }
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_script(
  cardano_sub_tx_builder_t* builder,
  cardano_script_t*         script);

/**
 * \brief Adds a signer to the sub transaction being built.
 *
 * This function registers a specific signer for the sub transaction by providing their public key hash, so the sub
 * transaction must carry a witness for that key. A required signer is a key hash guard: this function produces the
 * same sub transaction as `cardano_sub_tx_builder_add_guard` with a key hash credential of the same hash. Adding a
 * signer that is already present leaves the sub transaction unchanged.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] pub_key_hash A pointer to the \ref cardano_blake2b_hash_t structure representing the public
 *                         key hash of the signer to be added.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_blake2b_hash_t* signer_hash = ...;       // Public key hash for the signer
 *
 * cardano_sub_tx_builder_add_signer(sub_tx_builder, signer_hash);
 * \endcode
 *
 * \note Errors related to adding signers are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_signer(
  cardano_sub_tx_builder_t* builder,
  cardano_blake2b_hash_t*   pub_key_hash);

/**
 * \brief Adds a signer to the sub transaction by specifying their public key hash in hexadecimal format.
 *
 * This function registers a signer by accepting their public key hash as a hexadecimal string. See
 * `cardano_sub_tx_builder_add_signer` for how a signer relates to a key hash guard.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] pub_key_hash A string representing the public key hash in hexadecimal format.
 * \param[in] hash_size The size of the `pub_key_hash` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* pub_key_hash = "966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
 * size_t hash_size = strlen(pub_key_hash);
 *
 * cardano_sub_tx_builder_add_signer_ex(sub_tx_builder, pub_key_hash, hash_size);
 * \endcode
 *
 * \note Errors associated with adding a signer are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_signer_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               pub_key_hash,
  size_t                    hash_size);

/**
 * \brief Adds a guard to the sub transaction being built.
 *
 * This function registers a guard credential for the sub transaction. Guards generalize required signers: a key hash
 * guard is exactly a required signer, so the sub transaction must carry a witness for that key, while a script hash
 * guard requires the ledger to run the script as a validity condition. Adding a guard that is already present leaves
 * the sub transaction unchanged.
 *
 * The builder does not resolve or attach the script behind a script hash guard. Provide the script with
 * `cardano_sub_tx_builder_add_script` or through a reference input.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] guard A pointer to the \ref cardano_credential_t structure representing the guard to be added.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_credential_t* guard = ...;               // Key hash or script hash credential
 *
 * cardano_sub_tx_builder_add_guard(sub_tx_builder, guard);
 * \endcode
 *
 * \note Errors related to adding guards are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_guard(
  cardano_sub_tx_builder_t* builder,
  cardano_credential_t*     guard);

/**
 * \brief Adds a guard to the sub transaction by specifying the credential hash in hexadecimal format.
 *
 * This function registers a guard by accepting the hash of its credential as a hexadecimal string together with
 * the credential type. See `cardano_sub_tx_builder_add_guard` for the difference between key hash and script hash
 * guards.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] hash_hex A string representing the credential hash in hexadecimal format.
 * \param[in] hash_hex_size The size of the `hash_hex` string.
 * \param[in] type The type of the credential, either \ref CARDANO_CREDENTIAL_TYPE_KEY_HASH or
 *                 \ref CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* key_hash = "966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
 * size_t hash_size = strlen(key_hash);
 *
 * cardano_sub_tx_builder_add_guard_ex(sub_tx_builder, key_hash, hash_size, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
 * \endcode
 *
 * \note Errors associated with adding a guard are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_guard_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               hash_hex,
  size_t                    hash_hex_size,
  cardano_credential_type_t type);

/**
 * \brief Requires a guard from the top level transaction that carries the sub transaction.
 *
 * A sub transaction can constrain the batch it takes part in: every credential it requires must be listed among the
 * guards of the top level transaction, otherwise the whole batch is invalid. Each required guard may carry a datum
 * that is handed to the guard script of the top level transaction, which lets the sub transaction pass its own
 * terms to that script. Requiring a credential that is already present replaces its datum.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] credential A pointer to the \ref cardano_credential_t the top level transaction must be guarded by.
 * \param[in] datum A pointer to the \ref cardano_plutus_data_t handed to the guard (optional; can be NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_credential_t* batcher_guard = ...;       // Credential the top level transaction must be guarded by
 * cardano_plutus_data_t* terms = ...;              // Optional datum for the guard
 *
 * cardano_sub_tx_builder_require_top_level_guard(sub_tx_builder, batcher_guard, terms);
 * \endcode
 *
 * \note Errors related to requiring a top level guard are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_require_top_level_guard(
  cardano_sub_tx_builder_t* builder,
  cardano_credential_t*     credential,
  cardano_plutus_data_t*    datum);

/**
 * \brief Requires a guard from the top level transaction by specifying the credential hash in hexadecimal format.
 *
 * This function behaves like `cardano_sub_tx_builder_require_top_level_guard` but accepts the hash of the credential
 * as a hexadecimal string together with the credential type.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for sub transaction construction.
 * \param[in] hash_hex A string representing the credential hash in hexadecimal format.
 * \param[in] hash_hex_size The size of the `hash_hex` string.
 * \param[in] type The type of the credential, either \ref CARDANO_CREDENTIAL_TYPE_KEY_HASH or
 *                 \ref CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH.
 * \param[in] datum A pointer to the \ref cardano_plutus_data_t handed to the guard (optional; can be NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* script_hash = "966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
 * size_t hash_size = strlen(script_hash);
 *
 * cardano_sub_tx_builder_require_top_level_guard_ex(sub_tx_builder, script_hash, hash_size, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, NULL);
 * \endcode
 *
 * \note Errors related to requiring a top level guard are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_require_top_level_guard_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               hash_hex,
  size_t                    hash_hex_size,
  cardano_credential_type_t type,
  cardano_plutus_data_t*    datum);

/**
 * \brief Adds a direct deposit to the sub transaction being built.
 *
 * A direct deposit pays lovelace straight into a reward account without creating a UTxO. The deposited amount is
 * value produced by the sub transaction, so it is part of the imbalance the batch has to cover. Adding a direct
 * deposit for a reward account that already has one accumulates both amounts.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for constructing the sub transaction.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t representing the reward account that
 *                           receives the deposit.
 * \param[in] amount The amount of lovelace to deposit. It must be greater than zero.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;   // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;   // Initialized reward address
 *
 * cardano_sub_tx_builder_add_direct_deposit(sub_tx_builder, reward_address, 2000000);
 * \endcode
 *
 * \note Errors related to adding a direct deposit, including an accumulated amount that does not fit in 64 bits,
 *       are deferred and will only be reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_direct_deposit(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* reward_address,
  uint64_t                  amount);

/**
 * \brief Adds a direct deposit to the sub transaction using a string reward address.
 *
 * This function behaves like `cardano_sub_tx_builder_add_direct_deposit` but accepts the reward account as a Bech32
 * string.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for constructing the sub transaction.
 * \param[in] reward_address A string representing the reward account that receives the deposit.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] amount The amount of lovelace to deposit. It must be greater than zero.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_addr = "stake1u9...";         // Reward address in string format
 * size_t address_size = strlen(reward_addr);       // Length of the reward address string
 *
 * cardano_sub_tx_builder_add_direct_deposit_ex(sub_tx_builder, reward_addr, address_size, 2000000);
 * \endcode
 *
 * \note Errors related to adding a direct deposit are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_direct_deposit_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  uint64_t                  amount);

/**
 * \brief Adds an account balance interval to the sub transaction being built.
 *
 * An account balance interval makes the sub transaction valid only while the balance of the reward account sits
 * inside the interval at the point the sub transaction is applied. An interval is either bounded (inclusive lower
 * bound, exclusive upper bound, either of which may be absent) or an exact balance. Adding an interval for a
 * reward account that already has one replaces the previous interval.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for constructing the sub transaction.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t representing the reward account the
 *                           interval constrains.
 * \param[in] interval A pointer to the \ref cardano_account_balance_interval_t the account balance must sit in.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;   // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;   // Initialized reward address
 * cardano_account_balance_interval_t* interval = NULL;
 * const uint64_t lower_bound = 1000000;
 *
 * if (cardano_account_balance_interval_new(&lower_bound, NULL, &interval) == CARDANO_SUCCESS)
 * {
 *   cardano_sub_tx_builder_add_account_balance_interval(sub_tx_builder, reward_address, interval);
 *   cardano_account_balance_interval_unref(&interval);
 * }
 * \endcode
 *
 * \note Errors related to adding an interval are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_account_balance_interval(
  cardano_sub_tx_builder_t*           builder,
  cardano_reward_address_t*           reward_address,
  cardano_account_balance_interval_t* interval);

/**
 * \brief Adds an account balance interval to the sub transaction using a string reward address.
 *
 * This function behaves like `cardano_sub_tx_builder_add_account_balance_interval` but accepts the reward account
 * as a Bech32 string.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for constructing the sub transaction.
 * \param[in] reward_address A string representing the reward account the interval constrains.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] interval A pointer to the \ref cardano_account_balance_interval_t the account balance must sit in.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;           // Initialized sub transaction builder
 * cardano_account_balance_interval_t* interval = ...;       // Initialized interval
 * const char* reward_addr = "stake1u9...";                  // Reward address in string format
 * size_t address_size = strlen(reward_addr);                // Length of the reward address string
 *
 * cardano_sub_tx_builder_add_account_balance_interval_ex(sub_tx_builder, reward_addr, address_size, interval);
 * \endcode
 *
 * \note Errors related to adding an interval are deferred and will only be reported when
 *       `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_account_balance_interval_ex(
  cardano_sub_tx_builder_t*           builder,
  const char*                         reward_address,
  size_t                              address_size,
  cardano_account_balance_interval_t* interval);

/**
 * \brief Withdraws rewards from a specified reward account in the sub transaction builder.
 *
 * This function adds a withdrawal from the specified reward account to the sub transaction. The withdrawn amount
 * is value consumed by the sub transaction, so it is part of the imbalance the batch has to match.
 *
 * A sub transaction can withdraw part of the balance of an account: the ledger only requires that the withdrawals
 * of an account, added over the top level transaction and every sub transaction of the batch, do not exceed the
 * balance the account had before the batch. The rule that a withdrawal must drain the account exactly only
 * applies to the withdrawals of a top level transaction that uses a PlutusV1, PlutusV2 or PlutusV3 script.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for constructing the sub transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the reward account address
 *                    from which rewards should be withdrawn. The account must be controlled by a key or by a
 *                    native script.
 * \param[in] amount  The amount of rewards to withdraw from the account, in lovelace.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;   // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;   // Initialized reward address
 *
 * cardano_sub_tx_builder_withdraw_rewards(sub_tx_builder, reward_address, 2532145);
 * \endcode
 *
 * \note Errors related to reward withdrawal will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_withdraw_rewards(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address,
  int64_t                   amount);

/**
 * \brief Withdraws rewards from a specified reward account using a string address in the sub transaction builder.
 *
 * This function behaves like `cardano_sub_tx_builder_withdraw_rewards` but accepts the reward account as a Bech32
 * string.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for constructing the sub transaction.
 * \param[in] reward_address A string representing the reward account address from which rewards are to be withdrawn.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] amount  The amount of rewards to withdraw from the account, in lovelace.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_addr = "stake1u9...";         // Reward address in string format
 * size_t address_size = strlen(reward_addr);       // Length of the reward address string
 *
 * cardano_sub_tx_builder_withdraw_rewards_ex(sub_tx_builder, reward_addr, address_size, 966584122);
 * \endcode
 *
 * \note Errors related to reward withdrawal will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_withdraw_rewards_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  int64_t                   amount);

/**
 * \brief Registers a staking reward address.
 *
 * This function adds a staking reward address to be registered in the sub transaction, allowing the specified
 * reward account to start receiving staking rewards. The key deposit of the registration is value produced by the
 * sub transaction, so it is part of the imbalance the batch has to cover.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance used for constructing the sub transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the reward account to be registered.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_reward_address_t* reward_addr = ...;     // Reward address to register
 *
 * cardano_sub_tx_builder_register_reward_address(sub_tx_builder, reward_addr);
 * \endcode
 *
 * \note Any errors related to reward address registration will be reported when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_register_reward_address(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address);

/**
 * \brief Registers a staking reward address using a Bech32 address string.
 *
 * This function behaves like `cardano_sub_tx_builder_register_reward_address` but accepts the reward account as a
 * Bech32 string.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction construction.
 * \param[in] reward_address A pointer to a bech32 string representing the reward address.
 * \param[in] address_size The size of the `reward_address` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_address = "stake1uxxx...";
 * size_t address_size = strlen(reward_address);
 *
 * cardano_sub_tx_builder_register_reward_address_ex(sub_tx_builder, reward_address, address_size);
 * \endcode
 *
 * \note Errors related to the reward address registration will be reported only when `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_register_reward_address_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size);

/**
 * \brief Deregisters a staking reward address.
 *
 * This function deregisters the reward address, effectively preventing it from receiving future staking rewards.
 * The key deposit the deregistration gives back is value consumed by the sub transaction, so it is part of the
 * imbalance the batch has to match.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t structure representing the reward address to deregister.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;  // Reward address to be deregistered
 *
 * cardano_sub_tx_builder_deregister_reward_address(sub_tx_builder, reward_address);
 * \endcode
 *
 * \note Errors related to the deregistration process are deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_deregister_reward_address(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address);

/**
 * \brief Deregisters a staking reward address using a Bech32 address string.
 *
 * This function behaves like `cardano_sub_tx_builder_deregister_reward_address` but accepts the reward account as a
 * Bech32 string.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] reward_address A pointer to a bech32 string containing the reward address to be deregistered.
 * \param[in] address_size The length of the reward address string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_address = "stake1uxxx...";    // Reward address in string format
 * size_t address_size = strlen(reward_address);
 *
 * cardano_sub_tx_builder_deregister_reward_address_ex(sub_tx_builder, reward_address, address_size);
 * \endcode
 *
 * \note Errors related to the deregistration process are deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_deregister_reward_address_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size);

/**
 * \brief Delegates stake from a specified staking reward address to a staking pool.
 *
 * This function delegates stake from a specified staking reward address to a staking pool,
 * identified by its unique pool ID.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the staking reward address to delegate.
 * \param[in] pool_id A pointer to the \ref cardano_blake2b_hash_t representing the hash ID of the pool to delegate to.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;  // Reward address for staking
 * cardano_blake2b_hash_t* pool_id = ...;           // Pool ID to delegate stake to
 *
 * cardano_sub_tx_builder_delegate_stake(sub_tx_builder, reward_address, pool_id);
 * \endcode
 *
 * \note Any errors associated with this delegation action will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_delegate_stake(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address,
  cardano_blake2b_hash_t*   pool_id);

/**
 * \brief Delegates stake from a specified staking reward address to a staking pool using string identifiers.
 *
 * This function behaves like `cardano_sub_tx_builder_delegate_stake` but accepts the reward address and the pool ID
 * as Bech32 strings.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] reward_address A bech32 string representing the staking reward address for delegation.
 * \param[in] address_size The size of the `reward_address` string.
 * \param[in] pool_id A bech32 string representing the pool ID to which stake is delegated.
 * \param[in] pool_id_size The size of the `pool_id` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_address = "stake1u9...";
 * size_t reward_address_size = strlen(reward_address);
 * const char* pool_id = "pool1xy...";
 * size_t pool_id_size = strlen(pool_id);
 *
 * cardano_sub_tx_builder_delegate_stake_ex(sub_tx_builder, reward_address, reward_address_size, pool_id, pool_id_size);
 * \endcode
 *
 * \note Errors related to this action will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_delegate_stake_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  const char*               pool_id,
  size_t                    pool_id_size);

/**
 * \brief Adds a voting power delegation to a DRep for governance purposes.
 *
 * This function enables delegating voting power from a specified reward address to a DRep.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] address A pointer to the \ref cardano_reward_address_t representing the staking reward address that is delegating voting power.
 * \param[in] drep A pointer to the \ref cardano_drep_t structure that identifies the decentralized representative receiving the delegated voting power.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;  // Reward address delegating voting power
 * cardano_drep_t* drep = ...;                      // DRep receiving the delegated voting power
 *
 * cardano_sub_tx_builder_delegate_voting_power(sub_tx_builder, reward_address, drep);
 * \endcode
 *
 * \note Errors associated with this delegation will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_delegate_voting_power(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* address,
  cardano_drep_t*           drep);

/**
 * \brief Delegates voting power to a DRep using string identifiers.
 *
 * This function behaves like `cardano_sub_tx_builder_delegate_voting_power` but accepts the reward address and the
 * DRep ID as strings. The DRep ID can be provided in either CIP-105 or CIP-129 format.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] reward_address A string representing the staking reward address for the account delegating voting power.
 * \param[in] address_size The length of the `reward_address` string.
 * \param[in] drep_id A string representing the ID of the decentralized representative (DRep) receiving the voting power.
 *                     The DRep ID can be in CIP-105 or CIP-129 format.
 * \param[in] drep_id_size The length of the `drep_id` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_address = "stake1u9...";      // Reward address delegating voting power
 * size_t address_size = strlen(reward_address);
 * const char* drep_id = "drep1q...";               // DRep ID in CIP-129 format
 * size_t drep_id_size = strlen(drep_id);
 *
 * cardano_sub_tx_builder_delegate_voting_power_ex(sub_tx_builder, reward_address, address_size, drep_id, drep_id_size);
 * \endcode
 *
 * \note The DRep ID must conform to either CIP-105 or CIP-129 format.
 *       Errors related to this delegation will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_delegate_voting_power_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  const char*               drep_id,
  size_t                    drep_id_size);

/**
 * \brief Registers a DRep in the sub transaction.
 *
 * This function registers a DRep with an optional anchor in the sub transaction being constructed. The DRep deposit
 * of the registration is value produced by the sub transaction, so it is part of the imbalance the batch has to
 * cover.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] drep A pointer to the \ref cardano_drep_t instance representing the decentralized representative to be registered.
 * \param[in] anchor An optional pointer to a \ref cardano_anchor_t instance pointing to the metadata of the DRep.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_drep_t* drep = ...;                      // DRep to register
 * cardano_anchor_t* anchor = ...;                  // Optional governance anchor
 *
 * cardano_sub_tx_builder_register_drep(sub_tx_builder, drep, anchor);
 * \endcode
 *
 * \note Errors related to this registration will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_register_drep(
  cardano_sub_tx_builder_t* builder,
  cardano_drep_t*           drep,
  cardano_anchor_t*         anchor);

/**
 * \brief Registers a DRep by ID in the sub transaction.
 *
 * This function behaves like `cardano_sub_tx_builder_register_drep` but accepts the DRep ID and the anchor as
 * strings. The DRep ID must be provided in either CIP-105 or CIP-129 bech32 format.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] drep_id A pointer to a character array containing the DRep ID in bech32 format (either CIP-105 or CIP-129).
 * \param[in] drep_id_size The size of the `drep_id` string.
 * \param[in] metadata_url The URL pointing to the DRep metadata file.
 * \param[in] metadata_url_size The size of the `metadata_url` string.
 * \param[in] metadata_hash_hex The hash of the DRep metadata file in hexadecimal format.
 * \param[in] metadata_hash_hex_size The size of the `metadata_hash_hex` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* drep_id = "drep1q...";
 * size_t drep_id_size = strlen(drep_id);
 * const char* metadata_url = "https://example.com/drep_metadata.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abcdef123456...";  // Hex-encoded hash of metadata file
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 *
 * cardano_sub_tx_builder_register_drep_ex(
 *   sub_tx_builder,
 *   drep_id,
 *   drep_id_size,
 *   metadata_url,
 *   metadata_url_size,
 *   metadata_hash_hex,
 *   metadata_hash_hex_size);
 * \endcode
 *
 * \note The `drep_id` must be in bech32 format as specified by either CIP-105 or CIP-129, which differ in their internal binary encoding.
 *       Errors related to this registration will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_register_drep_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               drep_id,
  size_t                    drep_id_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size);

/**
 * \brief Updates an existing DRep in the sub transaction.
 *
 * This function allows updating a DRep in the sub transaction with a specified DRep object, optionally providing a
 * new anchor.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] drep A pointer to the \ref cardano_drep_t instance representing the DRep to be updated.
 * \param[in] anchor An optional pointer to a \ref cardano_anchor_t instance pointing to the metadata of the DRep.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_drep_t* drep = ...;                      // DRep object to update
 * cardano_anchor_t* anchor = ...;                  // Optional anchor
 *
 * cardano_sub_tx_builder_update_drep(sub_tx_builder, drep, anchor);
 * \endcode
 *
 * \note Any errors related to updating the DRep will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_update_drep(
  cardano_sub_tx_builder_t* builder,
  cardano_drep_t*           drep,
  cardano_anchor_t*         anchor);

/**
 * \brief Updates an existing DRep in the sub transaction by ID.
 *
 * This function behaves like `cardano_sub_tx_builder_update_drep` but accepts the DRep ID and the anchor as strings.
 * The DRep ID must be provided in either CIP-105 or CIP-129 bech32 format.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] drep_id A pointer to a character array containing the DRep ID in bech32 format (either CIP-105 or CIP-129).
 * \param[in] drep_id_size The size of the `drep_id` string.
 * \param[in] metadata_url The URL pointing to the DRep metadata file.
 * \param[in] metadata_url_size The size of the `metadata_url` string.
 * \param[in] metadata_hash_hex The hash of the DRep metadata file in hexadecimal format.
 * \param[in] metadata_hash_hex_size The size of the `metadata_hash_hex` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* drep_id = "drep1q...";
 * size_t drep_id_size = strlen(drep_id);
 * const char* metadata_url = "https://example.com/drep_metadata.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abcdef123456...";  // Hex-encoded hash of metadata file
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 *
 * cardano_sub_tx_builder_update_drep_ex(
 *   sub_tx_builder,
 *   drep_id,
 *   drep_id_size,
 *   metadata_url,
 *   metadata_url_size,
 *   metadata_hash_hex,
 *   metadata_hash_hex_size);
 * \endcode
 *
 * \note The `drep_id` must be in bech32 format as specified by either CIP-105 or CIP-129, which differ in their internal binary encoding.
 *       Errors associated with updating the DRep will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_update_drep_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               drep_id,
  size_t                    drep_id_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size);

/**
 * \brief Deregisters an existing DRep.
 *
 * This function deregisters a DRep. The DRep deposit the deregistration gives back is value consumed by the sub
 * transaction, so it is part of the imbalance the batch has to match.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] drep A pointer to the \ref cardano_drep_t instance representing the DRep to deregister.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_drep_t* drep = ...;                      // DRep to deregister
 *
 * cardano_sub_tx_builder_deregister_drep(sub_tx_builder, drep);
 * \endcode
 *
 * \note Errors related to deregistering the DRep will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_deregister_drep(
  cardano_sub_tx_builder_t* builder,
  cardano_drep_t*           drep);

/**
 * \brief Deregisters an existing DRep by ID.
 *
 * This function behaves like `cardano_sub_tx_builder_deregister_drep` but accepts the DRep ID as a string. The DRep
 * ID must conform to either CIP-105 or CIP-129 standards.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] drep_id A pointer to a character array containing the DRep ID in bech32 format (either CIP-105 or CIP-129).
 * \param[in] drep_id_size The size of the `drep_id` string.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* drep_id = "drep1q...";               // DRep ID in CIP-105 format
 * size_t drep_id_size = strlen(drep_id);
 *
 * cardano_sub_tx_builder_deregister_drep_ex(sub_tx_builder, drep_id, drep_id_size);
 * \endcode
 *
 * \note The `drep_id` must be in bech32 format as specified by either CIP-105 or CIP-129, which differ in their internal binary encoding.
 *       Errors related to deregistering the DRep will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_deregister_drep_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               drep_id,
  size_t                    drep_id_size);

/**
 * \brief Registers a vote for a specified governance action within the sub transaction.
 *
 * This function allows a voter to submit their vote for a given governance action. The voter must be identified by
 * a key or by a native script, since this builder only supports native scripts and key witnesses.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] voter A pointer to a \ref cardano_voter_t structure representing the voter participating in the governance action.
 * \param[in] action_id A pointer to a \ref cardano_governance_action_id_t identifying the governance action being voted on.
 * \param[in] vote A pointer to a \ref cardano_voting_procedure_t defining the voting procedure and choice.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;       // Initialized sub transaction builder
 * cardano_voter_t* voter = ...;                         // Voter information
 * cardano_governance_action_id_t* action_id = ...;      // Governance action ID
 * cardano_voting_procedure_t* vote = ...;               // Voting procedure with choice
 *
 * cardano_sub_tx_builder_vote(sub_tx_builder, voter, action_id, vote);
 * \endcode
 *
 * \note Errors related to the voting operation will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_vote(
  cardano_sub_tx_builder_t*       builder,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote);

/**
 * \brief Adds a certificate to the sub transaction.
 *
 * This function adds a specified certificate to the sub transaction being constructed. Certificates are used to perform
 * various actions on the blockchain, such as staking, delegating, or registering/deregistering entities. The
 * credential the certificate acts on must be a key or a native script, since this builder only supports native
 * scripts and key witnesses.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance managing the sub transaction.
 * \param[in] certificate A pointer to a \ref cardano_certificate_t structure representing the certificate to add.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_certificate_t* certificate = ...;        // Certificate to add
 *
 * cardano_sub_tx_builder_add_certificate(sub_tx_builder, certificate);
 * \endcode
 *
 * \note Errors related to the certificate addition will be deferred until `cardano_sub_tx_builder_build` is called.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_add_certificate(
  cardano_sub_tx_builder_t* builder,
  cardano_certificate_t*    certificate);

/**
 * \brief Proposes a hard fork within a sub transaction.
 *
 * This function adds a proposal to initiate a hard fork in the Cardano network to the sub transaction being built. The hard fork proposal
 * specifies the target protocol version and other required parameters. An optional governance action ID can be provided to reference
 * the most recent enacted hard fork action of the same type. The governance action deposit of the proposal is value produced by the
 * sub transaction, so it is part of the imbalance the batch has to cover.
 *
 * \param[in, out] builder A pointer to the \ref cardano_sub_tx_builder_t instance used to build the sub transaction.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object where the deposit will be refunded.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object containing additional metadata related to the proposal.
 * \param[in] version A pointer to a \ref cardano_protocol_version_t object specifying the target protocol version for the hard fork.
 * \param[in] governance_action_id An optional pointer to a \ref cardano_governance_action_id_t object referencing the most recent
 *                                 enacted governance action of the same type. This can be NULL if no such action has been enacted.
 *
 * \note Errors encountered during the addition of the hard fork proposal will be deferred until \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;         // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;         // Reward address for deposit refund
 * cardano_anchor_t* anchor = ...;                         // Anchor metadata for the proposal
 * cardano_protocol_version_t* version = ...;              // Target protocol version
 * cardano_governance_action_id_t* gov_action_id = ...;    // Optional governance action ID
 *
 * cardano_sub_tx_builder_propose_hardfork(sub_tx_builder, reward_address, anchor, version, gov_action_id);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_hardfork(
  cardano_sub_tx_builder_t*       builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_protocol_version_t*     version,
  cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Proposes a hard fork within a sub transaction (extended version).
 *
 * This function behaves like `cardano_sub_tx_builder_propose_hardfork` but accepts the reward address, the anchor and the
 * governance action ID as strings, and the target protocol version as numbers.
 *
 * \param[in, out] builder A pointer to the \ref cardano_sub_tx_builder_t instance used to build the sub transaction.
 * \param[in] reward_address A pointer to the reward address string where the deposit will be refunded.
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
 * \note Errors encountered during the addition of the hard fork proposal will be deferred until \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_address = "stake1u...";
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com/metadata.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash = "abcdef1234567890...";
 * size_t metadata_hash_size = strlen(metadata_hash);
 * const char* gov_action_id = "gov_action1...";
 * size_t gov_action_id_size = strlen(gov_action_id);
 *
 * cardano_sub_tx_builder_propose_hardfork_ex(
 *   sub_tx_builder,
 *   reward_address,
 *   reward_address_size,
 *   metadata_url,
 *   metadata_url_size,
 *   metadata_hash,
 *   metadata_hash_size,
 *   gov_action_id,
 *   gov_action_id_size,
 *   0,
 *   12);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_hardfork_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    reward_address_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size,
  const char*               gov_action_id,
  size_t                    gov_action_id_size,
  uint64_t                  minor_protocol_version,
  uint64_t                  major_protocol_version);

/**
 * \brief Proposes a no-confidence governance action within a sub transaction.
 *
 * This function prepares a proposal for a no-confidence action against the current constitutional committee.
 * If enacted, it signals the community's lack of confidence in the committee, potentially leading to its reconstitution.
 * The governance action deposit of the proposal is value produced by the sub transaction, so it is part of the imbalance
 * the batch has to cover.
 *
 * \param[in,out] builder                 A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
 * \param[in]     reward_address          A pointer to a \ref cardano_reward_address_t object. This address will receive the deposit refund after the governance action completes.
 * \param[in]     anchor                  A pointer to a \ref cardano_anchor_t object providing additional context or metadata for the governance action.
 * \param[in]     governance_action_id    An optional pointer to a \ref cardano_governance_action_id_t object. This represents the unique identifier
 *                                         for the most recently enacted action of the same type. This parameter can be NULL if no previous actions of this type exist.
 *
 * \note Any errors encountered during the addition of the no-confidence proposal will be deferred until
 *       \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;      // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;      // Reward address for deposit refund
 * cardano_anchor_t* anchor = ...;                      // Anchor metadata for the proposal
 * cardano_governance_action_id_t* action_id = ...;     // Optional governance action ID
 *
 * cardano_sub_tx_builder_propose_no_confidence(sub_tx_builder, reward_address, anchor, action_id);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_no_confidence(
  cardano_sub_tx_builder_t*       builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Proposes a no-confidence governance action using extended parameters.
 *
 * This function behaves like `cardano_sub_tx_builder_propose_no_confidence` but accepts the reward address, the anchor
 * and the governance action ID as strings.
 *
 * \param[in,out] builder            A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
 * \param[in] reward_address         A pointer to a string containing the Bech32-encoded reward address. This address will receive the deposit refund
 *                                   after the governance action completes.
 * \param[in] reward_address_size    The size of the reward address string in bytes.
 * \param[in] metadata_url           A pointer to a string containing the metadata URL for additional context regarding the proposal.
 * \param[in] metadata_url_size      The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex      A pointer to a string containing the hexadecimal representation of the metadata hash.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id          A CIP-0129 bech32 string representing the governance action ID that references the most recent enacted action of the same type.
 * \param[in] gov_action_id_size     The size (in bytes) of the `gov_action_id` string.
 *
 * \note Any errors encountered during the addition of the no-confidence proposal will be deferred until
 *       \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;      // Initialized sub transaction builder
 * const char* reward_address = "stake1u9...";
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "a1b2c3...";
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 * const char* gov_action_id = "gov_action1...";
 * size_t gov_action_id_size = strlen(gov_action_id);
 *
 * cardano_sub_tx_builder_propose_no_confidence_ex(sub_tx_builder, reward_address, reward_address_size,
 *                                                 metadata_url, metadata_url_size, metadata_hash_hex,
 *                                                 metadata_hash_hex_size, gov_action_id,
 *                                                 gov_action_id_size);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_no_confidence_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    reward_address_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size,
  const char*               gov_action_id,
  size_t                    gov_action_id_size);

/**
 * \brief Proposes an update to the Cardano constitutional committee.
 *
 * This function prepares a governance action to update the Cardano constitutional committee. The proposal specifies:
 * - Committee members to be added.
 * - Committee members to be removed.
 * - The new quorum threshold for the committee.
 *
 * The governance action deposit of the proposal is value produced by the sub transaction, so it is part of the imbalance
 * the batch has to cover.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
 * \param[in]     reward_address         A pointer to a \ref cardano_reward_address_t object representing the reward address that will receive the deposit refund.
 * \param[in]     anchor                 A pointer to a \ref cardano_anchor_t object representing the anchor for this governance action.
 * \param[in]     governance_action_id   An optional pointer to a \ref cardano_governance_action_id_t object representing the most recently enacted governance action
 *                                        of the same type. Can be NULL if no such prior action exists.
 * \param[in]     members_to_be_removed  A pointer to a \ref cardano_credential_set_t object specifying the committee members to be removed.
 * \param[in]     members_to_be_added    A pointer to a \ref cardano_committee_members_map_t object specifying the committee members to be added.
 * \param[in]     new_quorum             A pointer to a \ref cardano_unit_interval_t object specifying the new quorum threshold for the committee.
 *
 * \note Any errors encountered during the addition of the update committee proposal will be deferred until
 *       \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;         // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;         // Reward address to receive refund
 * cardano_anchor_t* anchor = ...;                         // Governance action anchor
 * cardano_governance_action_id_t* gov_action_id = ...;    // Most recently enacted action ID
 * cardano_credential_set_t* members_to_remove = ...;      // Members to be removed
 * cardano_committee_members_map_t* members_to_add = ...;  // Members to be added
 * cardano_unit_interval_t* new_quorum = ...;              // New quorum threshold
 *
 * cardano_sub_tx_builder_propose_update_committee(sub_tx_builder, reward_address, anchor,
 *                                                 gov_action_id, members_to_remove,
 *                                                 members_to_add, new_quorum);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_update_committee(
  cardano_sub_tx_builder_t*        builder,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  cardano_unit_interval_t*         new_quorum);

/**
 * \brief Proposes an update to the Cardano constitutional committee with extended parameters.
 *
 * This function behaves like `cardano_sub_tx_builder_propose_update_committee` but accepts the reward address, the anchor
 * and the governance action ID as strings, and the new quorum threshold as a number.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
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
 * \note Any errors encountered during the addition of the update committee proposal will be deferred until
 *       \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;         // Initialized sub transaction builder
 * const char* reward_address = "stake1u9...";
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com/proposal.json";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abc123...";
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 * const char* gov_action_id = "gov_action1...";
 * size_t gov_action_id_size = strlen(gov_action_id);
 * cardano_credential_set_t* members_to_remove = ...;      // Members to be removed
 * cardano_committee_members_map_t* members_to_add = ...;  // Members to be added
 *
 * cardano_sub_tx_builder_propose_update_committee_ex(sub_tx_builder,
 *                                                    reward_address, reward_address_size,
 *                                                    metadata_url, metadata_url_size,
 *                                                    metadata_hash_hex, metadata_hash_hex_size,
 *                                                    gov_action_id, gov_action_id_size,
 *                                                    members_to_remove,
 *                                                    members_to_add,
 *                                                    0.3);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_update_committee_ex(
  cardano_sub_tx_builder_t*        builder,
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
 * This function creates a governance action to propose a new constitution. The governance action deposit of the
 * proposal is value produced by the sub transaction, so it is part of the imbalance the batch has to cover.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
 * \param[in]     reward_address         A pointer to a \ref cardano_reward_address_t object representing the address that will receive the deposit refund.
 * \param[in]     anchor                 A pointer to a \ref cardano_anchor_t object containing the anchor metadata for the proposal.
 * \param[in]     governance_action_id   An optional pointer to a \ref cardano_governance_action_id_t object representing the governance action ID of the most
 *                                        recently enacted action of the same type. This parameter can be NULL if no prior action exists.
 * \param[in]     constitution           A pointer to a \ref cardano_constitution_t object containing the new constitution's details.
 *
 * \note Any errors encountered during the addition of the new constitution proposal will be deferred until \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;       // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;       // Reward address to refund deposit
 * cardano_anchor_t* anchor = ...;                       // Anchor metadata
 * cardano_governance_action_id_t* gov_action_id = ...;  // Optional governance action ID
 * cardano_constitution_t* constitution = ...;           // Constitution details
 *
 * cardano_sub_tx_builder_propose_new_constitution(sub_tx_builder, reward_address, anchor, gov_action_id, constitution);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_new_constitution(
  cardano_sub_tx_builder_t*       builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  cardano_constitution_t*         constitution);

/**
 * \brief Proposes a new constitution for the Cardano network using extended parameters.
 *
 * This function behaves like `cardano_sub_tx_builder_propose_new_constitution` but accepts the reward address, the
 * anchor and the governance action ID as strings.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
 * \param[in]     reward_address         A pointer to the reward address as a string. The address will receive the deposit refund.
 * \param[in]     reward_address_size    The size of the reward address string in bytes.
 * \param[in]     metadata_url           A pointer to a string containing the URL for additional proposal metadata.
 * \param[in]     metadata_url_size      The size of the metadata URL string in bytes.
 * \param[in]     metadata_hash_hex      A pointer to a hexadecimal string representing the hash of the metadata file.
 * \param[in]     metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in]     gov_action_id          A CIP-0129 bech32 string representing the governance action ID that references the most recent enacted action of the same type.
 * \param[in]     gov_action_id_size     The size (in bytes) of the `gov_action_id` string.
 * \param[in]     constitution           A pointer to a \ref cardano_constitution_t object containing the new constitution's details.
 *
 * \note Any errors encountered during the addition of the new constitution proposal will be deferred until \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;      // Initialized sub transaction builder
 * const char* reward_address = "stake1u9...";
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abc123...";
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 * const char* gov_action_id = "gov_action1...";
 * size_t gov_action_id_size = strlen(gov_action_id);
 * cardano_constitution_t* constitution = ...;          // Constitution details
 *
 * cardano_sub_tx_builder_propose_new_constitution_ex(
 *   sub_tx_builder, reward_address, reward_address_size,
 *   metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size,
 *   gov_action_id, gov_action_id_size, constitution);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_new_constitution_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    reward_address_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size,
  const char*               gov_action_id,
  size_t                    gov_action_id_size,
  cardano_constitution_t*   constitution);

/**
 * \brief Proposes an informational governance action for the Cardano network.
 *
 * This function creates a governance action proposal to share information with the network. The governance action
 * deposit of the proposal is value produced by the sub transaction, so it is part of the imbalance the batch has to
 * cover.
 *
 * \param[in,out] builder        A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
 * \param[in]     reward_address A pointer to the reward address where the deposit refund will be sent if the proposal is enacted or discarded.
 * \param[in]     anchor         A pointer to a \ref cardano_anchor_t object containing metadata such as URLs and hashes to link to external information.
 *
 * \note Any errors encountered during the addition of the informational proposal will be deferred until \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * cardano_reward_address_t* reward_address = ...;  // Reward address for deposit refund
 * cardano_anchor_t* anchor = ...;                  // Anchor metadata for the proposal
 *
 * cardano_sub_tx_builder_propose_info(sub_tx_builder, reward_address, anchor);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_info(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor);

/**
 * \brief Proposes an informational governance action for the Cardano network using extended parameters.
 *
 * This function behaves like `cardano_sub_tx_builder_propose_info` but accepts the reward address and the anchor as
 * strings.
 *
 * \param[in,out] builder                A pointer to an initialized \ref cardano_sub_tx_builder_t object used to build the sub transaction.
 * \param[in]     reward_address         A pointer to the reward address in string format where the deposit refund will be sent if the proposal is enacted or discarded.
 * \param[in]     reward_address_size    The size of the reward address string in bytes.
 * \param[in]     metadata_url           A pointer to a string containing the URL for the proposal metadata.
 * \param[in]     metadata_url_size      The size of the metadata URL string in bytes.
 * \param[in]     metadata_hash_hex      A pointer to a string containing the hexadecimal hash of the metadata.
 * \param[in]     metadata_hash_hex_size The size of the metadata hash string in bytes.
 *
 * \note Any errors encountered during the addition of the informational proposal will be deferred until \ref cardano_sub_tx_builder_build is called.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_tx_builder = ...;  // Initialized sub transaction builder
 * const char* reward_address = "stake1u9...";
 * size_t reward_address_size = strlen(reward_address);
 * const char* metadata_url = "https://example.com/proposal-metadata";
 * size_t metadata_url_size = strlen(metadata_url);
 * const char* metadata_hash_hex = "abcdef123456...";
 * size_t metadata_hash_hex_size = strlen(metadata_hash_hex);
 *
 * cardano_sub_tx_builder_propose_info_ex(
 *   sub_tx_builder, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_tx_builder_propose_info_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    reward_address_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size);

/**
 * \brief Builds the sub transaction from the current state of the sub transaction builder.
 *
 * This function finalizes the sub transaction by aggregating all previously added inputs, outputs, minting
 * operations and other data. The sub transaction is not balanced: it spends exactly the inputs and produces exactly
 * the outputs that were added, and the difference is the intent a batcher matches against the rest of the batch.
 * If any of the previous operations failed, this function reports the first error that was recorded.
 *
 * \param[in] builder A pointer to the \ref cardano_sub_tx_builder_t instance that manages the sub transaction details.
 * \param[out] sub_transaction A pointer to a \ref cardano_sub_transaction_t pointer where the created sub transaction
 *                             will be stored upon success.
 *
 * \returns \ref CARDANO_SUCCESS if the sub transaction was successfully built, or an appropriate error code if issues were detected.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = NULL;
 * cardano_sub_tx_builder_t* sub_tx_builder   = ...; // Initialized sub transaction builder
 *
 * cardano_sub_tx_builder_add_input(sub_tx_builder, utxo);                       // Offer the value of a UTXO
 * cardano_sub_tx_builder_send_lovelace_ex(sub_tx_builder, "add...", size, 50000); // Ask for an output in return
 * cardano_sub_tx_builder_set_invalid_after(sub_tx_builder, 1000);               // Set the sub transaction validity
 *
 * cardano_error_t result = cardano_sub_tx_builder_build(sub_tx_builder, &sub_transaction);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   const char* error_msg = cardano_sub_tx_builder_get_last_error(sub_tx_builder);
 *   printf("Failed to build sub transaction: %s\n", error_msg);
 * }
 * else
 * {
 *   // Sub transaction successfully built; sign it and hand it to a batcher
 *   // ...
 *   cardano_sub_transaction_unref(&sub_transaction);
 * }
 *
 * cardano_sub_tx_builder_unref(&sub_tx_builder);
 * \endcode
 *
 * \note All errors related to missing or invalid data will be reported upon calling this function. A builder can
 *       build a single sub transaction, further calls report \ref CARDANO_ERROR_ILLEGAL_STATE.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_tx_builder_build(
  cardano_sub_tx_builder_t*   builder,
  cardano_sub_transaction_t** sub_transaction);

/**
 * \brief Decrements the reference count of a cardano_sub_tx_builder_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_sub_tx_builder_t object
 * by decreasing its reference count. When the reference count reaches zero, the sub_transaction_builder is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] sub_transaction_builder A pointer to the pointer of the sub_transaction_builder object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_tx_builder_t* sub_transaction_builder = cardano_sub_tx_builder_new(params, &slot_config);
 *
 * // Perform operations with the sub_transaction_builder...
 *
 * cardano_sub_tx_builder_unref(&sub_transaction_builder);
 * // At this point, sub_transaction_builder is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_sub_tx_builder_unref, the pointer to the \ref cardano_sub_tx_builder_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_unref(cardano_sub_tx_builder_t** sub_transaction_builder);

/**
 * \brief Increases the reference count of the cardano_sub_tx_builder_t object.
 *
 * This function is used to manually increment the reference count of an cardano_sub_tx_builder_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_sub_tx_builder_unref.
 *
 * \param sub_transaction_builder A pointer to the cardano_sub_tx_builder_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction_builder is a previously created sub_transaction_builder object
 *
 * cardano_sub_tx_builder_ref(sub_transaction_builder);
 *
 * // Now sub_transaction_builder can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_sub_tx_builder_ref there is a corresponding
 * call to \ref cardano_sub_tx_builder_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_ref(cardano_sub_tx_builder_t* sub_transaction_builder);

/**
 * \brief Retrieves the current reference count of the cardano_sub_tx_builder_t object.
 *
 * This function returns the number of active references to an cardano_sub_tx_builder_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_sub_tx_builder_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param sub_transaction_builder A pointer to the cardano_sub_tx_builder_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_sub_tx_builder_t object. If the object
 * is properly managed (i.e., every \ref cardano_sub_tx_builder_ref call is matched with a
 * \ref cardano_sub_tx_builder_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction_builder is a previously created sub_transaction_builder object
 *
 * size_t ref_count = cardano_sub_tx_builder_refcount(sub_transaction_builder);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_sub_tx_builder_refcount(const cardano_sub_tx_builder_t* sub_transaction_builder);

/**
 * \brief Sets the last error message for a given cardano_sub_tx_builder_t object.
 *
 * Records an error message in the sub_transaction_builder's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] sub_transaction_builder A pointer to the \ref cardano_sub_tx_builder_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the sub_transaction_builder's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_sub_tx_builder_set_last_error(
  cardano_sub_tx_builder_t* sub_transaction_builder,
  const char*               message);

/**
 * \brief Retrieves the last error message recorded for a specific sub_transaction_builder.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_sub_tx_builder_set_last_error for the given
 * sub_transaction_builder. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] sub_transaction_builder A pointer to the \ref cardano_sub_tx_builder_t instance whose last error
 *                   message is to be retrieved. If the sub_transaction_builder is NULL, the function
 *                   returns a generic error message indicating the null sub_transaction_builder.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified sub_transaction_builder. If the sub_transaction_builder is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_sub_tx_builder_set_last_error for the same sub_transaction_builder, or until
 *       the sub_transaction_builder is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_sub_tx_builder_get_last_error(
  const cardano_sub_tx_builder_t* sub_transaction_builder);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_BUILDER_H
