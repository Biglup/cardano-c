/**
 * \file cardano.h
 *
 * \author angel.castillo
 * \date   Mar 13, 2024
 *
 * \section LICENSE
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_H
#define BIGLUP_LABS_INCLUDE_CARDANO_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/address/address_type.h>
#include <cardano/address/base_address.h>
#include <cardano/address/byron_address.h>
#include <cardano/address/byron_address_attributes.h>
#include <cardano/address/byron_address_type.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/address/reward_address.h>
#include <cardano/address/stake_pointer.h>
#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_id_list.h>
#include <cardano/assets/asset_id_map.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/asset_name_list.h>
#include <cardano/assets/asset_name_map.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/assets/policy_id_list.h>
#include <cardano/auxiliary_data/auxiliary_data.h>
#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/metadatum_kind.h>
#include <cardano/auxiliary_data/metadatum_label_list.h>
#include <cardano/auxiliary_data/metadatum_list.h>
#include <cardano/auxiliary_data/metadatum_map.h>
#include <cardano/auxiliary_data/plutus_v1_script_list.h>
#include <cardano/auxiliary_data/plutus_v2_script_list.h>
#include <cardano/auxiliary_data/plutus_v3_script_list.h>
#include <cardano/auxiliary_data/transaction_metadata.h>
#include <cardano/bip39.h>
#include <cardano/buffer.h>
#include <cardano/cbor/cbor_major_type.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_reader_state.h>
#include <cardano/cbor/cbor_simple_value.h>
#include <cardano/cbor/cbor_tag.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/auth_committee_hot_cert.h>
#include <cardano/certs/cert_type.h>
#include <cardano/certs/certificate.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/certs/genesis_key_delegation_cert.h>
#include <cardano/certs/mir_cert.h>
#include <cardano/certs/mir_cert_pot_type.h>
#include <cardano/certs/mir_cert_type.h>
#include <cardano/certs/mir_to_pot_cert.h>
#include <cardano/certs/mir_to_stake_creds_cert.h>
#include <cardano/certs/pool_registration_cert.h>
#include <cardano/certs/pool_retirement_cert.h>
#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/resign_committee_cold_cert.h>
#include <cardano/certs/stake_delegation_cert.h>
#include <cardano/certs/stake_deregistration_cert.h>
#include <cardano/certs/stake_registration_cert.h>
#include <cardano/certs/stake_registration_delegation_cert.h>
#include <cardano/certs/stake_vote_delegation_cert.h>
#include <cardano/certs/stake_vote_registration_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/certs/vote_delegation_cert.h>
#include <cardano/certs/vote_registration_delegation_cert.h>
#include <cardano/common/anchor.h>
#include <cardano/common/bigint.h>
#include <cardano/common/byte_order.h>
#include <cardano/common/credential.h>
#include <cardano/common/credential_type.h>
#include <cardano/common/datum.h>
#include <cardano/common/datum_type.h>
#include <cardano/common/drep.h>
#include <cardano/common/drep_type.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/governance_key_type.h>
#include <cardano/common/network_id.h>
#include <cardano/common/protocol_version.h>
#include <cardano/common/reward_address_list.h>
#include <cardano/common/unit_interval.h>
#include <cardano/common/utxo.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/bip32_private_key.h>
#include <cardano/crypto/bip32_public_key.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_set.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/crypto/crc32.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/crypto/emip3.h>
#include <cardano/crypto/pbkdf2.h>
#include <cardano/encoding/base58.h>
#include <cardano/encoding/bech32.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/json/json_context.h>
#include <cardano/json/json_format.h>
#include <cardano/json/json_object.h>
#include <cardano/json/json_object_type.h>
#include <cardano/json/json_writer.h>
#include <cardano/key_handlers/account_derivation_path.h>
#include <cardano/key_handlers/cip_1852_constants.h>
#include <cardano/key_handlers/derivation_path.h>
#include <cardano/key_handlers/secure_key_handler.h>
#include <cardano/key_handlers/secure_key_handler_impl.h>
#include <cardano/key_handlers/secure_key_handler_type.h>
#include <cardano/key_handlers/software_secure_key_handler.h>
#include <cardano/object.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_data_kind.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include <cardano/pool_params/ipv4.h>
#include <cardano/pool_params/ipv6.h>
#include <cardano/pool_params/multi_host_name_relay.h>
#include <cardano/pool_params/pool_metadata.h>
#include <cardano/pool_params/pool_owners.h>
#include <cardano/pool_params/pool_params.h>
#include <cardano/pool_params/relay.h>
#include <cardano/pool_params/relay_type.h>
#include <cardano/pool_params/relays.h>
#include <cardano/pool_params/single_host_addr_relay.h>
#include <cardano/pool_params/single_host_name_relay.h>
#include <cardano/proposal_procedures/committee.h>
#include <cardano/proposal_procedures/committee_members_map.h>
#include <cardano/proposal_procedures/constitution.h>
#include <cardano/proposal_procedures/credential_set.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/proposal_procedures/info_action.h>
#include <cardano/proposal_procedures/new_constitution_action.h>
#include <cardano/proposal_procedures/no_confidence_action.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>
#include <cardano/proposal_procedures/update_committee_action.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/protocol_params/drep_voting_thresholds.h>
#include <cardano/protocol_params/ex_unit_prices.h>
#include <cardano/protocol_params/pool_voting_thresholds.h>
#include <cardano/protocol_params/proposed_param_updates.h>
#include <cardano/protocol_params/protocol_param_update.h>
#include <cardano/protocol_params/update.h>
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/native_scripts/native_script_list.h>
#include <cardano/scripts/native_scripts/native_script_type.h>
#include <cardano/scripts/native_scripts/script_all.h>
#include <cardano/scripts/native_scripts/script_any.h>
#include <cardano/scripts/native_scripts/script_invalid_after.h>
#include <cardano/scripts/native_scripts/script_invalid_before.h>
#include <cardano/scripts/native_scripts/script_n_of_k.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <cardano/scripts/plutus_scripts/plutus_language_version.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/scripts/script.h>
#include <cardano/scripts/script_language.h>
#include <cardano/time.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_body/value.h>
#include <cardano/transaction_builder/balancing/implicit_coin.h>
#include <cardano/transaction_builder/balancing/input_to_redeemer_map.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/coin_selector_impl.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/transaction_builder/evaluation/provider_tx_evaluator.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator_impl.h>
#include <cardano/transaction_builder/fee.h>
#include <cardano/transaction_builder/script_data_hash.h>
#include <cardano/transaction_builder/transaction_builder.h>
#include <cardano/typedefs.h>
#include <cardano/voting_procedures/governance_action_id_list.h>
#include <cardano/voting_procedures/vote.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>
#include <cardano/voting_procedures/voter_type.h>
#include <cardano/voting_procedures/voting_procedure.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/bootstrap_witness.h>
#include <cardano/witness_set/bootstrap_witness_set.h>
#include <cardano/witness_set/native_script_set.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/plutus_v1_script_set.h>
#include <cardano/witness_set/plutus_v2_script_set.h>
#include <cardano/witness_set/plutus_v3_script_set.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/redeemer_tag.h>
#include <cardano/witness_set/vkey_witness.h>
#include <cardano/witness_set/vkey_witness_set.h>
#include <cardano/witness_set/witness_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Retrieves the version of the Cardano C library.
 *
 * This function returns a string representing the version of the Cardano C library. The string
 * is statically allocated and should not be freed by the caller. The version string follows the
 * Semantic Versioning (SemVer) format, which consists of three segments: MAJOR.MINOR.PATCH
 * (e.g., "1.0.3").
 *
 * \return A pointer to a statically allocated string containing the library's version. This string
 * is located in read-only memory and must not be modified or freed by the caller.
 */
CARDANO_EXPORT const char* cardano_get_lib_version(void);

/**
 * \brief Securely wipes the contents of a buffer from memory.
 *
 * The `cardano_memzero` function is used to securely erase the sensitive data stored in the provided \p buffer.
 * This ensures that the data is no longer recoverable from memory after it has been used.
 *
 * After use, sensitive data should be overwritten. However, traditional approaches like `memset()` or hand-written
 * memory clearing routines can be stripped away by optimizing compilers or linkers, potentially leaving sensitive
 * information exposed.
 *
 * The `cardano_memzero` function guarantees that the memory is cleared, even in the presence of compiler optimizations.
 * It is especially important to call this function before freeing memory that contains sensitive information, such as
 * cryptographic keys or decrypted data, to prevent the data from remaining in memory.
 *
 * \param[in] buffer A pointer to the buffer whose contents should be securely erased.
 * \param[in] size The size of the buffer in bytes.
 *
 * \example
 * \code
 * // Example usage after processing sensitive data
 * char sensitive_data[64] = ...; // Buffer containing sensitive data
 *
 * // Wipe the buffer contents securely
 * cardano_memzero(sensitive_data, 64);
 * \endcode
 */
CARDANO_EXPORT void cardano_memzero(void* buffer, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_H
