/**
 * \file builder_proposals.h
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_PROPOSALS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_PROPOSALS_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/common/anchor.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/protocol_version.h>
#include <cardano/common/unit_interval.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/proposal_procedures/committee_members_map.h>
#include <cardano/proposal_procedures/constitution.h>
#include <cardano/proposal_procedures/credential_set.h>
#include <cardano/protocol_params/protocol_param_update.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Adds a protocol parameter change proposal to the transaction.
 *
 * This function creates a parameter change governance action from \p protocol_param_update,
 * \p governance_action_id and \p policy_hash, wraps it in a proposal procedure funded with the
 * governance action deposit from the state protocol parameters and adds it to the proposal
 * procedures of the transaction body, creating the set when it is missing. It also adds an empty
 * proposing redeemer for the proposal to the witness set so the constitution script can validate
 * the proposal.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t that receives the deposit
 *                           refund. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the proposal metadata. This
 *                   parameter must not be NULL.
 * \param[in] protocol_param_update A pointer to the \ref cardano_protocol_param_update_t with the
 *                                  parameter changes being proposed. This parameter must not be NULL.
 * \param[in] governance_action_id A pointer to the \ref cardano_governance_action_id_t of the most
 *                                 recently enacted action of this kind, or NULL when there is none.
 * \param[in] policy_hash A pointer to the \ref cardano_blake2b_hash_t with the constitution script
 *                        hash. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_parameter_change(
  cardano_builder_state_t*         state,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_blake2b_hash_t*          policy_hash,
  const char**                     error_message);

/**
 * \brief Adds a protocol parameter change proposal given its parts as strings.
 *
 * This function parses the reward address, metadata anchor, governance action id and policy hash
 * from their string representations and delegates to
 * \ref cardano_builder_propose_parameter_change.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata URL of the anchor.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hex string with the metadata hash of the anchor.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id A pointer to the bech32 string with the governance action id.
 * \param[in] gov_action_id_size The size of the governance action id string in bytes.
 * \param[in] policy_hash_hash_hex A pointer to the hex string with the constitution script hash.
 * \param[in] policy_hash_hash_hex_size The size of the constitution script hash string in bytes.
 * \param[in] protocol_param_update A pointer to the \ref cardano_protocol_param_update_t with the
 *                                  parameter changes being proposed. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_parameter_change_ex(
  cardano_builder_state_t*         state,
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
  cardano_protocol_param_update_t* protocol_param_update,
  const char**                     error_message);

/**
 * \brief Adds a hard fork initiation proposal to the transaction.
 *
 * This function creates a hard fork initiation governance action for \p version, wraps it in a
 * proposal procedure funded with the governance action deposit from the state protocol parameters
 * and adds it to the proposal procedures of the transaction body, creating the set when it is
 * missing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t that receives the deposit
 *                           refund. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the proposal metadata. This
 *                   parameter must not be NULL.
 * \param[in] version A pointer to the \ref cardano_protocol_version_t being proposed. This parameter
 *                    must not be NULL.
 * \param[in] governance_action_id A pointer to the \ref cardano_governance_action_id_t of the most
 *                                 recently enacted action of this kind, or NULL when there is none.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_hardfork(
  cardano_builder_state_t*        state,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_protocol_version_t*     version,
  cardano_governance_action_id_t* governance_action_id,
  const char**                    error_message);

/**
 * \brief Adds a hard fork initiation proposal given its parts as strings.
 *
 * This function parses the reward address, metadata anchor and governance action id from their
 * string representations, builds the protocol version from its components and delegates to
 * \ref cardano_builder_propose_hardfork.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata URL of the anchor.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hex string with the metadata hash of the anchor.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id A pointer to the bech32 string with the governance action id.
 * \param[in] gov_action_id_size The size of the governance action id string in bytes.
 * \param[in] minor_protocol_version The minor component of the proposed protocol version.
 * \param[in] major_protocol_version The major component of the proposed protocol version.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_hardfork_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   reward_address_size,
  const char*              metadata_url,
  size_t                   metadata_url_size,
  const char*              metadata_hash_hex,
  size_t                   metadata_hash_hex_size,
  const char*              gov_action_id,
  size_t                   gov_action_id_size,
  uint64_t                 minor_protocol_version,
  uint64_t                 major_protocol_version,
  const char**             error_message);

/**
 * \brief Adds a treasury withdrawals proposal to the transaction.
 *
 * This function creates a treasury withdrawals governance action from \p withdrawals and
 * \p policy_hash, wraps it in a proposal procedure funded with the governance action deposit from
 * the state protocol parameters and adds it to the proposal procedures of the transaction body,
 * creating the set when it is missing. It also adds an empty proposing redeemer for the proposal
 * to the witness set so the constitution script can validate the proposal.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t that receives the deposit
 *                           refund. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the proposal metadata. This
 *                   parameter must not be NULL.
 * \param[in] withdrawals A pointer to the \ref cardano_withdrawal_map_t with the reward addresses
 *                        and amounts to withdraw from the treasury. This parameter must not be NULL.
 * \param[in] policy_hash A pointer to the \ref cardano_blake2b_hash_t with the constitution script
 *                        hash. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_treasury_withdrawals(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor,
  cardano_withdrawal_map_t* withdrawals,
  cardano_blake2b_hash_t*   policy_hash,
  const char**              error_message);

/**
 * \brief Adds a treasury withdrawals proposal given its parts as strings.
 *
 * This function parses the reward address, metadata anchor and policy hash from their string
 * representations and delegates to \ref cardano_builder_propose_treasury_withdrawals.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata URL of the anchor.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hex string with the metadata hash of the anchor.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] policy_hash_hash_hex A pointer to the hex string with the constitution script hash.
 * \param[in] policy_hash_hash_hex_size The size of the constitution script hash string in bytes.
 * \param[in] withdrawals A pointer to the \ref cardano_withdrawal_map_t with the reward addresses
 *                        and amounts to withdraw from the treasury. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_treasury_withdrawals_ex(
  cardano_builder_state_t*  state,
  const char*               reward_address,
  size_t                    reward_address_size,
  const char*               metadata_url,
  size_t                    metadata_url_size,
  const char*               metadata_hash_hex,
  size_t                    metadata_hash_hex_size,
  const char*               policy_hash_hash_hex,
  size_t                    policy_hash_hash_hex_size,
  cardano_withdrawal_map_t* withdrawals,
  const char**              error_message);

/**
 * \brief Adds a no confidence proposal to the transaction.
 *
 * This function creates a no confidence governance action, wraps it in a proposal procedure funded
 * with the governance action deposit from the state protocol parameters and adds it to the proposal
 * procedures of the transaction body, creating the set when it is missing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t that receives the deposit
 *                           refund. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the proposal metadata. This
 *                   parameter must not be NULL.
 * \param[in] governance_action_id A pointer to the \ref cardano_governance_action_id_t of the most
 *                                 recently enacted action of this kind, or NULL when there is none.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_no_confidence(
  cardano_builder_state_t*        state,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  const char**                    error_message);

/**
 * \brief Adds a no confidence proposal given its parts as strings.
 *
 * This function parses the reward address, metadata anchor and governance action id from their
 * string representations and delegates to \ref cardano_builder_propose_no_confidence.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata URL of the anchor.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hex string with the metadata hash of the anchor.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id A pointer to the bech32 string with the governance action id.
 * \param[in] gov_action_id_size The size of the governance action id string in bytes.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_no_confidence_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   reward_address_size,
  const char*              metadata_url,
  size_t                   metadata_url_size,
  const char*              metadata_hash_hex,
  size_t                   metadata_hash_hex_size,
  const char*              gov_action_id,
  size_t                   gov_action_id_size,
  const char**             error_message);

/**
 * \brief Adds an update committee proposal to the transaction.
 *
 * This function creates an update committee governance action from the members to remove, the
 * members to add and the new quorum, wraps it in a proposal procedure funded with the governance
 * action deposit from the state protocol parameters and adds it to the proposal procedures of the
 * transaction body, creating the set when it is missing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t that receives the deposit
 *                           refund. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the proposal metadata. This
 *                   parameter must not be NULL.
 * \param[in] governance_action_id A pointer to the \ref cardano_governance_action_id_t of the most
 *                                 recently enacted action of this kind, or NULL when there is none.
 * \param[in] members_to_be_removed A pointer to the \ref cardano_credential_set_t with the cold
 *                                  credentials of the committee members to remove. This parameter
 *                                  must not be NULL.
 * \param[in] members_to_be_added A pointer to the \ref cardano_committee_members_map_t with the new
 *                                committee members and their terms. This parameter must not be NULL.
 * \param[in] new_quorum A pointer to the \ref cardano_unit_interval_t with the new committee quorum
 *                       threshold. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_update_committee(
  cardano_builder_state_t*         state,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  cardano_unit_interval_t*         new_quorum,
  const char**                     error_message);

/**
 * \brief Adds an update committee proposal given its parts as strings.
 *
 * This function parses the reward address, metadata anchor and governance action id from their
 * string representations, builds the quorum unit interval from a double and delegates to
 * \ref cardano_builder_propose_update_committee.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata URL of the anchor.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hex string with the metadata hash of the anchor.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id A pointer to the bech32 string with the governance action id.
 * \param[in] gov_action_id_size The size of the governance action id string in bytes.
 * \param[in] members_to_be_removed A pointer to the \ref cardano_credential_set_t with the cold
 *                                  credentials of the committee members to remove. This parameter
 *                                  must not be NULL.
 * \param[in] members_to_be_added A pointer to the \ref cardano_committee_members_map_t with the new
 *                                committee members and their terms. This parameter must not be NULL.
 * \param[in] new_quorum The new committee quorum threshold as a value between 0 and 1.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_update_committee_ex(
  cardano_builder_state_t*         state,
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
  double                           new_quorum,
  const char**                     error_message);

/**
 * \brief Adds a new constitution proposal to the transaction.
 *
 * This function creates a new constitution governance action from \p constitution, wraps it in a
 * proposal procedure funded with the governance action deposit from the state protocol parameters
 * and adds it to the proposal procedures of the transaction body, creating the set when it is
 * missing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t that receives the deposit
 *                           refund. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the proposal metadata. This
 *                   parameter must not be NULL.
 * \param[in] governance_action_id A pointer to the \ref cardano_governance_action_id_t of the most
 *                                 recently enacted action of this kind, or NULL when there is none.
 * \param[in] constitution A pointer to the \ref cardano_constitution_t being proposed. This
 *                         parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_new_constitution(
  cardano_builder_state_t*        state,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  cardano_constitution_t*         constitution,
  const char**                    error_message);

/**
 * \brief Adds a new constitution proposal given its parts as strings.
 *
 * This function parses the reward address, metadata anchor and governance action id from their
 * string representations and delegates to \ref cardano_builder_propose_new_constitution.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata URL of the anchor.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hex string with the metadata hash of the anchor.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] gov_action_id A pointer to the bech32 string with the governance action id.
 * \param[in] gov_action_id_size The size of the governance action id string in bytes.
 * \param[in] constitution A pointer to the \ref cardano_constitution_t being proposed. This
 *                         parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_new_constitution_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   reward_address_size,
  const char*              metadata_url,
  size_t                   metadata_url_size,
  const char*              metadata_hash_hex,
  size_t                   metadata_hash_hex_size,
  const char*              gov_action_id,
  size_t                   gov_action_id_size,
  cardano_constitution_t*  constitution,
  const char**             error_message);

/**
 * \brief Adds an info proposal to the transaction.
 *
 * This function creates an info governance action, wraps it in a proposal procedure funded with
 * the governance action deposit from the state protocol parameters and adds it to the proposal
 * procedures of the transaction body, creating the set when it is missing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t that receives the deposit
 *                           refund. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the proposal metadata. This
 *                   parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_info(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor,
  const char**              error_message);

/**
 * \brief Adds an info proposal given its parts as strings.
 *
 * This function parses the reward address and metadata anchor from their string representations
 * and delegates to \ref cardano_builder_propose_info.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] reward_address_size The size of the reward address string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata URL of the anchor.
 * \param[in] metadata_url_size The size of the metadata URL string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hex string with the metadata hash of the anchor.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_propose_info_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   reward_address_size,
  const char*              metadata_url,
  size_t                   metadata_url_size,
  const char*              metadata_hash_hex,
  size_t                   metadata_hash_hex_size,
  const char**             error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_PROPOSALS_H
