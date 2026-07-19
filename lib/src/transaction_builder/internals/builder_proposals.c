/**
 * \file builder_proposals.c
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

/* INCLUDES ******************************************************************/

#include "builder_proposals.h"

#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/proposal_procedures/info_action.h>
#include <cardano/proposal_procedures/new_constitution_action.h>
#include <cardano/proposal_procedures/no_confidence_action.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>
#include <cardano/proposal_procedures/update_committee_action.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/witness_set.h>

#include "../../string_safe.h"

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Gets the proposal procedures set from the transaction body. If there are no proposal
 * procedures set, it creates a new list.
 *
 * \param body The transaction body.
 *
 * \return The proposal procedures set, or NULL when it could not be created or attached.
 */
static cardano_proposal_procedure_set_t*
get_proposal_procedure_set(cardano_transaction_body_t* body)
{
  cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);

  if (proposals == NULL)
  {
    cardano_error_t result = cardano_proposal_procedure_set_new(&proposals);

    if (result != CARDANO_SUCCESS)
    {
      return NULL;
    }

    result = cardano_transaction_body_set_proposal_procedure(body, proposals);

    if (result != CARDANO_SUCCESS)
    {
      cardano_proposal_procedure_set_unref(&proposals);

      return NULL;
    }
  }

  cardano_proposal_procedure_set_unref(&proposals);

  return proposals;
}

/**
 * \brief Creates an empty plutus data object.
 *
 * \return The empty plutus data object.
 */
static cardano_plutus_data_t*
create_empty_plutus_data(void)
{
  static const char* VOID_DATA = "d87980";

  cardano_plutus_data_t* plutus_data = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(VOID_DATA, cardano_safe_strlen(VOID_DATA, 6));

  cardano_error_t result = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_cbor_reader_unref(&reader);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return plutus_data;
}

/**
 * \brief Adds an empty proposing redeemer to the witness set.
 *
 * \param witness_set The witness set.
 * \param proposal_index The index of the proposal that requires the redeemer.
 *
 * \return The result of the operation.
 */
static cardano_error_t
add_proposing_redeemer(
  cardano_witness_set_t* witness_set,
  const size_t           proposal_index)
{
  cardano_redeemer_list_t* redeemers         = cardano_witness_set_get_redeemers(witness_set);
  cardano_plutus_data_t*   empty_plutus_data = create_empty_plutus_data();
  cardano_redeemer_t*      rdmer             = NULL;
  cardano_ex_units_t*      ex_units          = NULL;

  cardano_error_t result = cardano_ex_units_new(0, 0, &ex_units);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_data_unref(&empty_plutus_data);
    cardano_redeemer_list_unref(&redeemers);

    return result;
  }

  result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_PROPOSING, proposal_index, empty_plutus_data, ex_units, &rdmer);

  cardano_plutus_data_unref(&empty_plutus_data);
  cardano_ex_units_unref(&ex_units);

  if (result != CARDANO_SUCCESS)
  {
    cardano_redeemer_list_unref(&redeemers);

    return result;
  }

  if (redeemers == NULL)
  {
    result = cardano_redeemer_list_new(&redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_unref(&rdmer);

      return result;
    }

    result = cardano_witness_set_set_redeemers(witness_set, redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_unref(&rdmer);
      cardano_redeemer_list_unref(&redeemers);

      return result;
    }
  }

  cardano_redeemer_list_unref(&redeemers);

  result = cardano_redeemer_list_add(redeemers, rdmer);
  cardano_redeemer_unref(&rdmer);

  return result;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_propose_parameter_change(
  cardano_builder_state_t*         state,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_blake2b_hash_t*          policy_hash,
  const char**                     error_message)
{
  if (policy_hash == NULL)
  {
    *error_message = "Policy hash is NULL. You must provide the constitution script hash.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_parameter_change_action_t* action = NULL;
  cardano_error_t                    result = cardano_parameter_change_action_new(protocol_param_update, governance_action_id, policy_hash, &action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create parameter change action.";

    return result;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(state->params);

  result = cardano_proposal_procedure_new_parameter_change_action(deposit, reward_address, anchor, action, &proposal);

  cardano_parameter_change_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create proposal procedure.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(body);

  if (proposals == NULL)
  {
    cardano_proposal_procedure_unref(&proposal);
    *error_message = "Failed to add proposal procedure.";

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposal procedure.";

    return result;
  }

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witnesses);
  const size_t proposal_index = cardano_proposal_procedure_set_get_length(proposals) - 1U;

  result = add_proposing_redeemer(witnesses, proposal_index);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposing redeemer.";
  }

  return result;
}

cardano_error_t
cardano_builder_propose_parameter_change_ex(
  cardano_builder_state_t*         state,
  const char*                      reward_address,
  const size_t                     reward_address_size,
  const char*                      metadata_url,
  const size_t                     metadata_url_size,
  const char*                      metadata_hash_hex,
  const size_t                     metadata_hash_hex_size,
  const char*                      gov_action_id,
  const size_t                     gov_action_id_size,
  const char*                      policy_hash_hash_hex,
  const size_t                     policy_hash_hash_hex_size,
  cardano_protocol_param_update_t* protocol_param_update,
  const char**                     error_message)
{
  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    *error_message = "Metadata URL is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    *error_message = "Metadata hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    *error_message = "Governance action ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((policy_hash_hash_hex == NULL) || (policy_hash_hash_hex_size == 0U))
  {
    *error_message = "Policy hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    *error_message = "Failed to parse governance action ID.";

    return result;
  }

  cardano_blake2b_hash_t* policy_hash = NULL;

  result = cardano_blake2b_hash_from_hex(policy_hash_hash_hex, policy_hash_hash_hex_size, &policy_hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_governance_action_id_unref(&id);
    *error_message = "Failed to parse policy hash.";

    return result;
  }

  result = cardano_builder_propose_parameter_change(state, addr, anchor, protocol_param_update, id, policy_hash, error_message);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);
  cardano_blake2b_hash_unref(&policy_hash);

  return result;
}

cardano_error_t
cardano_builder_propose_hardfork(
  cardano_builder_state_t*        state,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_protocol_version_t*     version,
  cardano_governance_action_id_t* governance_action_id,
  const char**                    error_message)
{
  cardano_hard_fork_initiation_action_t* action = NULL;
  cardano_error_t                        result = cardano_hard_fork_initiation_action_new(version, governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create hard fork action.";

    return result;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(state->params);

  result = cardano_proposal_procedure_new_hard_fork_initiation_action(deposit, reward_address, anchor, action, &proposal);

  cardano_hard_fork_initiation_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create proposal procedure.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposal procedure.";

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_propose_hardfork_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  const size_t             reward_address_size,
  const char*              metadata_url,
  const size_t             metadata_url_size,
  const char*              metadata_hash_hex,
  const size_t             metadata_hash_hex_size,
  const char*              gov_action_id,
  const size_t             gov_action_id_size,
  const uint64_t           minor_protocol_version,
  const uint64_t           major_protocol_version,
  const char**             error_message)
{
  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    *error_message = "Metadata URL is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    *error_message = "Metadata hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    *error_message = "Governance action ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    *error_message = "Failed to parse governance action ID.";

    return result;
  }

  cardano_protocol_version_t* version = NULL;
  result                              = cardano_protocol_version_new(minor_protocol_version, major_protocol_version, &version);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_governance_action_id_unref(&id);
    *error_message = "Failed to create protocol version.";

    return result;
  }

  result = cardano_builder_propose_hardfork(state, addr, anchor, version, id, error_message);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);
  cardano_protocol_version_unref(&version);

  return result;
}

cardano_error_t
cardano_builder_propose_treasury_withdrawals(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor,
  cardano_withdrawal_map_t* withdrawals,
  cardano_blake2b_hash_t*   policy_hash,
  const char**              error_message)
{
  if (policy_hash == NULL)
  {
    *error_message = "Policy hash is NULL. You must provide the constitution script hash.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_treasury_withdrawals_action_t* action = NULL;
  cardano_error_t                        result = cardano_treasury_withdrawals_action_new(withdrawals, policy_hash, &action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create treasury withdrawal action.";

    return result;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(state->params);

  result = cardano_proposal_procedure_new_treasury_withdrawals_action(deposit, reward_address, anchor, action, &proposal);

  cardano_treasury_withdrawals_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create proposal procedure.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(body);

  if (proposals == NULL)
  {
    cardano_proposal_procedure_unref(&proposal);
    *error_message = "Failed to add proposal procedure.";

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposal procedure.";

    return result;
  }

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witnesses);
  const size_t proposal_index = cardano_proposal_procedure_set_get_length(proposals) - 1U;

  result = add_proposing_redeemer(witnesses, proposal_index);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposing redeemer.";
  }

  return result;
}

cardano_error_t
cardano_builder_propose_treasury_withdrawals_ex(
  cardano_builder_state_t*  state,
  const char*               reward_address,
  const size_t              reward_address_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size,
  const char*               policy_hash_hash_hex,
  const size_t              policy_hash_hash_hex_size,
  cardano_withdrawal_map_t* withdrawals,
  const char**              error_message)
{
  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    *error_message = "Metadata URL is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    *error_message = "Metadata hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((policy_hash_hash_hex == NULL) || (policy_hash_hash_hex_size == 0U))
  {
    *error_message = "Policy hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  cardano_blake2b_hash_t* policy_hash = NULL;
  result                              = cardano_blake2b_hash_from_hex(policy_hash_hash_hex, policy_hash_hash_hex_size, &policy_hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    *error_message = "Failed to parse policy hash.";

    return result;
  }

  result = cardano_builder_propose_treasury_withdrawals(state, addr, anchor, withdrawals, policy_hash, error_message);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_blake2b_hash_unref(&policy_hash);

  return result;
}

cardano_error_t
cardano_builder_propose_no_confidence(
  cardano_builder_state_t*        state,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  const char**                    error_message)
{
  cardano_no_confidence_action_t* action = NULL;
  cardano_error_t                 result = cardano_no_confidence_action_new(governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create no confidence action.";

    return result;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(state->params);

  result = cardano_proposal_procedure_new_no_confidence_action(deposit, reward_address, anchor, action, &proposal);
  cardano_no_confidence_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create proposal procedure.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposal procedure.";

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_propose_no_confidence_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  const size_t             reward_address_size,
  const char*              metadata_url,
  const size_t             metadata_url_size,
  const char*              metadata_hash_hex,
  const size_t             metadata_hash_hex_size,
  const char*              gov_action_id,
  const size_t             gov_action_id_size,
  const char**             error_message)
{
  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    *error_message = "Metadata URL is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    *error_message = "Metadata hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    *error_message = "Governance action ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    *error_message = "Failed to parse governance action ID.";

    return result;
  }

  result = cardano_builder_propose_no_confidence(state, addr, anchor, id, error_message);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);

  return result;
}

cardano_error_t
cardano_builder_propose_update_committee(
  cardano_builder_state_t*         state,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  cardano_unit_interval_t*         new_quorum,
  const char**                     error_message)
{
  cardano_update_committee_action_t* action = NULL;
  cardano_error_t                    result = cardano_update_committee_action_new(members_to_be_removed, members_to_be_added, new_quorum, governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create update committee action.";

    return result;
  }

  cardano_proposal_procedure_t* proposal;

  const uint64_t deposit = cardano_protocol_parameters_get_governance_action_deposit(state->params);

  result = cardano_proposal_procedure_new_update_committee_action(deposit, reward_address, anchor, action, &proposal);

  cardano_update_committee_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create proposal procedure.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposal procedure.";

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_propose_update_committee_ex(
  cardano_builder_state_t*         state,
  const char*                      reward_address,
  const size_t                     reward_address_size,
  const char*                      metadata_url,
  const size_t                     metadata_url_size,
  const char*                      metadata_hash_hex,
  const size_t                     metadata_hash_hex_size,
  const char*                      gov_action_id,
  const size_t                     gov_action_id_size,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  const double                     new_quorum,
  const char**                     error_message)
{
  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    *error_message = "Metadata URL is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    *error_message = "Metadata hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    *error_message = "Governance action ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    *error_message = "Failed to parse governance action ID.";

    return result;
  }

  cardano_unit_interval_t* quorum = NULL;
  result                          = cardano_unit_interval_from_double(new_quorum, &quorum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_governance_action_id_unref(&id);
    *error_message = "Failed to create unit interval.";

    return result;
  }

  result = cardano_builder_propose_update_committee(state, addr, anchor, id, members_to_be_removed, members_to_be_added, quorum, error_message);

  cardano_unit_interval_unref(&quorum);
  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);

  return result;
}

cardano_error_t
cardano_builder_propose_new_constitution(
  cardano_builder_state_t*        state,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  cardano_constitution_t*         constitution,
  const char**                    error_message)
{
  cardano_new_constitution_action_t* action = NULL;

  cardano_error_t result = cardano_new_constitution_action_new(constitution, governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create new constitution action.";

    return result;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(state->params);

  result = cardano_proposal_procedure_new_constitution_action(deposit, reward_address, anchor, action, &proposal);

  cardano_new_constitution_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create proposal procedure.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposal procedure.";

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_propose_new_constitution_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  const size_t             reward_address_size,
  const char*              metadata_url,
  const size_t             metadata_url_size,
  const char*              metadata_hash_hex,
  const size_t             metadata_hash_hex_size,
  const char*              gov_action_id,
  const size_t             gov_action_id_size,
  cardano_constitution_t*  constitution,
  const char**             error_message)
{
  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    *error_message = "Metadata URL is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    *error_message = "Metadata hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    *error_message = "Governance action ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;

  result = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    *error_message = "Failed to parse governance action ID.";

    return result;
  }

  result = cardano_builder_propose_new_constitution(state, addr, anchor, id, constitution, error_message);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);

  return result;
}

cardano_error_t
cardano_builder_propose_info(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor,
  const char**              error_message)
{
  cardano_info_action_t* action = NULL;
  cardano_error_t        result = cardano_info_action_new(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create info action.";

    return result;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(state->params);

  result = cardano_proposal_procedure_new_info_action(deposit, reward_address, anchor, action, &proposal);

  cardano_info_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create proposal procedure.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);

  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add proposal procedure.";

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_propose_info_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  const size_t             reward_address_size,
  const char*              metadata_url,
  const size_t             metadata_url_size,
  const char*              metadata_hash_hex,
  const size_t             metadata_hash_hex_size,
  const char**             error_message)
{
  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    *error_message = "Metadata URL is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    *error_message = "Metadata hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;

  result = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  result = cardano_builder_propose_info(state, addr, anchor, error_message);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);

  return result;
}
