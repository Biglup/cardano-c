/**
 * \file scripts_needed.c
 *
 * \author angel.castillo
 * \date   Sep 23, 2026
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

#include "scripts_needed.h"
#include "unique_signers.h"

#include <cardano/address/base_address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/certs/auth_committee_hot_cert.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/utxo.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>
#include <cardano/scripts/script.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>

#include "../../../common/ledger_order.h"

/* STRUCTURES ****************************************************************/

/**
 * \brief The Plutus languages of the needed scripts found among the provided scripts.
 */
typedef struct plutus_languages_t
{
    bool has_plutus_v1;
    bool has_plutus_v2;
    bool has_plutus_v3;
    bool has_plutus_v4;
} plutus_languages_t;

/**
 * \brief The outcome of looking the needed script hashes up among the scripts a transaction is provided with.
 */
typedef struct script_lookup_t
{
    cardano_blake2b_hash_set_t* needed_scripts;
    cardano_blake2b_hash_set_t* matched_scripts;
    cardano_blake2b_hash_set_t* plutus_scripts;
    plutus_languages_t          languages;
} script_lookup_t;

/**
 * \brief Receives a script purpose of a transaction body.
 *
 * \param[in,out] context The context the visit was started with.
 * \param[in] tag The redeemer tag of the purpose.
 * \param[in] index The redeemer index of the purpose.
 * \param[in] script_hash The hash of the script the purpose needs.
 *
 * \return \ref CARDANO_SUCCESS to continue the visit, or an appropriate error code that ends it.
 */
typedef cardano_error_t (*script_purpose_callback_t)(
  void*                   context,
  cardano_redeemer_tag_t  tag,
  uint64_t                index,
  cardano_blake2b_hash_t* script_hash);

/**
 * \brief A callback that receives every script purpose of a transaction body, and its context.
 */
typedef struct script_purpose_visitor_t
{
    script_purpose_callback_t callback;
    void*                     context;
} script_purpose_visitor_t;

/**
 * \brief The state of the search for a Plutus script purpose that has no redeemer.
 */
typedef struct missing_redeemer_search_t
{
    cardano_blake2b_hash_set_t* plutus_scripts;
    cardano_redeemer_list_t*    redeemers;
    cardano_redeemer_tag_t      tag;
    uint64_t                    index;
    cardano_blake2b_hash_t*     script_hash;
} missing_redeemer_search_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Callback function to find the UTXO of a transaction input.
 *
 * \param[in] item The UTXO to evaluate.
 * \param[in] context The transaction input to look for.
 *
 * \return `true` if the input of the UTXO is the given transaction input; otherwise, `false`.
 */
static bool
find_utxo(cardano_utxo_t* item, const void* context)
{
  const cardano_transaction_input_t* input      = (const cardano_transaction_input_t*)context;
  cardano_transaction_input_t*       utxo_input = cardano_utxo_get_input(item);

  bool found = cardano_transaction_input_equals(utxo_input, input);

  cardano_transaction_input_unref(&utxo_input);

  return found;
}

/**
 * \brief Adds a hash to a set if the set does not hold it yet.
 *
 * \param[in,out] set The set that receives the hash.
 * \param[in] hash The hash to add.
 *
 * \return \ref CARDANO_SUCCESS if the set holds the hash, or an appropriate error code.
 */
static cardano_error_t
add_unique_hash(cardano_blake2b_hash_set_t* set, cardano_blake2b_hash_t* hash)
{
  if (_cardano_blake2b_hash_set_has(set, hash))
  {
    return CARDANO_SUCCESS;
  }

  return cardano_blake2b_hash_set_add(set, hash);
}

/**
 * \brief Script purpose callback that adds the needed script hash to a set.
 *
 * \param[in,out] context The \ref cardano_blake2b_hash_set_t that receives the script hash.
 * \param[in] tag Unused.
 * \param[in] index Unused.
 * \param[in] script_hash The hash of the needed script.
 *
 * \return \ref CARDANO_SUCCESS if the set holds the script hash, or an appropriate error code.
 */
static cardano_error_t
collect_script_hash(
  void*                   context,
  cardano_redeemer_tag_t  tag,
  uint64_t                index,
  cardano_blake2b_hash_t* script_hash)
{
  CARDANO_UNUSED(tag);
  CARDANO_UNUSED(index);

  return add_unique_hash((cardano_blake2b_hash_set_t*)context, script_hash);
}

/**
 * \brief Checks whether a redeemer list holds a redeemer with the given tag and index.
 *
 * \param[in] redeemers The redeemers to search, or NULL when there are none.
 * \param[in] tag The redeemer tag to look for.
 * \param[in] index The redeemer index to look for.
 *
 * \return true if the list holds a redeemer with that tag and index, false otherwise.
 */
static bool
has_redeemer(const cardano_redeemer_list_t* redeemers, const cardano_redeemer_tag_t tag, const uint64_t index)
{
  const size_t length = cardano_redeemer_list_get_length(redeemers);
  bool         found  = false;

  for (size_t i = 0U; (i < length) && !found; ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    const cardano_error_t result = cardano_redeemer_list_get(redeemers, i, &redeemer);
    cardano_redeemer_unref(&redeemer);

    found = (result == CARDANO_SUCCESS) && (cardano_redeemer_get_tag(redeemer) == tag) && (cardano_redeemer_get_index(redeemer) == index);
  }

  return found;
}

/**
 * \brief Script purpose callback that records the first purpose whose Plutus script has no redeemer.
 *
 * \param[in,out] context The \ref missing_redeemer_search_t that holds the Plutus script hashes and the redeemers,
 *                        and receives the purpose.
 * \param[in] tag The redeemer tag of the purpose.
 * \param[in] index The redeemer index of the purpose.
 * \param[in] script_hash The hash of the script the purpose needs.
 *
 * \return \ref CARDANO_SUCCESS, since the purpose is only recorded.
 */
static cardano_error_t
find_purpose_without_redeemer(
  void*                   context,
  cardano_redeemer_tag_t  tag,
  uint64_t                index,
  cardano_blake2b_hash_t* script_hash)
{
  missing_redeemer_search_t* search = (missing_redeemer_search_t*)context;

  if ((search->script_hash != NULL) || !_cardano_blake2b_hash_set_has(search->plutus_scripts, script_hash) || has_redeemer(search->redeemers, tag, index))
  {
    return CARDANO_SUCCESS;
  }

  cardano_blake2b_hash_ref(script_hash);

  search->tag         = tag;
  search->index       = index;
  search->script_hash = script_hash;

  return CARDANO_SUCCESS;
}

/**
 * \brief Visits the purpose of a credential if the credential is a script hash.
 *
 * \param[in] visitor The visitor that receives the purpose.
 * \param[in] tag The redeemer tag of the purpose.
 * \param[in] index The redeemer index of the purpose.
 * \param[in] credential The credential to inspect.
 *
 * \return \ref CARDANO_SUCCESS if the credential was inspected, or an appropriate error code.
 */
static cardano_error_t
visit_script_credential(
  const script_purpose_visitor_t* visitor,
  const cardano_redeemer_tag_t    tag,
  const uint64_t                  index,
  cardano_credential_t*           credential)
{
  cardano_credential_type_t type   = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  cardano_error_t           result = cardano_credential_get_type(credential, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (type != CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH)
  {
    return CARDANO_SUCCESS;
  }

  cardano_blake2b_hash_t* hash = cardano_credential_get_hash(credential);

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = visitor->callback(visitor->context, tag, index, hash);

  cardano_blake2b_hash_unref(&hash);

  return result;
}

/**
 * \brief Visits the spend purpose of every spent input locked by a script.
 *
 * The index of each purpose is the position of its input in the sorted input set.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] inputs The spent inputs, or NULL when there are none.
 * \param[in] resolved_inputs The resolved UTXOs of the spent inputs.
 *
 * \return \ref CARDANO_SUCCESS if the inputs were inspected, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input is not
 *         resolved, or an appropriate error code.
 */
static cardano_error_t
visit_input_purposes(
  const script_purpose_visitor_t*  visitor,
  cardano_transaction_input_set_t* inputs,
  cardano_utxo_list_t*             resolved_inputs)
{
  const size_t size = cardano_transaction_input_set_get_length(inputs);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_transaction_input_t* input  = NULL;
    cardano_error_t              result = cardano_transaction_input_set_get(inputs, i, &input);
    cardano_transaction_input_unref(&input);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_utxo_t* utxo = cardano_utxo_list_find(resolved_inputs, find_utxo, (const void*)input);
    cardano_utxo_unref(&utxo);

    if (utxo == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_transaction_output_unref(&output);

    cardano_address_t* address = cardano_transaction_output_get_address(output);
    cardano_address_unref(&address);

    if (!_cardano_has_payment_script(address))
    {
      continue;
    }

    cardano_blake2b_hash_t* script_hash = _cardano_get_payment_script_hash(address);

    if (script_hash == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    result = visitor->callback(visitor->context, CARDANO_REDEEMER_TAG_SPEND, (uint64_t)i, script_hash);

    cardano_blake2b_hash_unref(&script_hash);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Visits the mint purpose of every policy id of the mint field.
 *
 * The index of each purpose is the position of its policy in the mint field.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] mint The mint field, or NULL when the body mints nothing.
 *
 * \return \ref CARDANO_SUCCESS if the mint field was inspected, or an appropriate error code.
 */
static cardano_error_t
visit_mint_purposes(const script_purpose_visitor_t* visitor, cardano_multi_asset_t* mint)
{
  if (mint == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_policy_id_list_t* policies = NULL;
  cardano_error_t           result   = cardano_multi_asset_get_keys(mint, &policies);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t size = cardano_policy_id_list_get_length(policies);

  for (size_t i = 0U; (i < size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_blake2b_hash_t* policy_id = NULL;

    result = cardano_policy_id_list_get(policies, i, &policy_id);

    if (result == CARDANO_SUCCESS)
    {
      result = visitor->callback(visitor->context, CARDANO_REDEEMER_TAG_MINT, (uint64_t)i, policy_id);
    }

    cardano_blake2b_hash_unref(&policy_id);
  }

  cardano_policy_id_list_unref(&policies);

  return result;
}

/**
 * \brief Visits the reward purpose of every withdrawal whose reward address is controlled by a script.
 *
 * The index of each purpose is the position of its reward address among the withdrawals in ledger order.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] withdrawals The withdrawals, or NULL when the body withdraws nothing.
 *
 * \return \ref CARDANO_SUCCESS if the withdrawals were inspected, or an appropriate error code.
 */
static cardano_error_t
visit_withdrawal_purposes(const script_purpose_visitor_t* visitor, cardano_withdrawal_map_t* withdrawals)
{
  const size_t size = cardano_withdrawal_map_get_length(withdrawals);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_reward_address_t* reward_address = NULL;
    cardano_error_t           result         = cardano_withdrawal_map_get_key_at(withdrawals, i, &reward_address);
    cardano_reward_address_unref(&reward_address);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    uint64_t index = 0U;

    result = _cardano_ledger_order_withdrawal_index(withdrawals, reward_address, &index);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_credential_t* credential = cardano_reward_address_get_credential(reward_address);

    if (credential == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    result = visit_script_credential(visitor, CARDANO_REDEEMER_TAG_REWARD, index, credential);

    cardano_credential_unref(&credential);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the credential that must authorize a certificate.
 *
 * \param[in] certificate The certificate to inspect.
 * \param[in] type The type of the certificate.
 * \param[out] credential On success, a new reference to the credential, or NULL if the certificate needs none.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was inspected, or an appropriate error code.
 */
static cardano_error_t
get_certificate_witness_credential(
  cardano_certificate_t* certificate,
  cardano_cert_type_t    type,
  cardano_credential_t** credential)
{
  if (type != CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT)
  {
    return _cardano_get_certificate_credential(certificate, type, credential);
  }

  cardano_auth_committee_hot_cert_t* auth_committee = NULL;
  cardano_error_t                    result         = cardano_certificate_to_auth_committee_hot(certificate, &auth_committee);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_auth_committee_hot_cert_get_cold_cred(auth_committee, credential);

  cardano_auth_committee_hot_cert_unref(&auth_committee);

  return result;
}

/**
 * \brief Visits the certifying purpose of every certificate a script must authorize.
 *
 * The index of each purpose is the position of its certificate in the certificate list.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] certificates The certificates, or NULL when the body carries none.
 *
 * \return \ref CARDANO_SUCCESS if the certificates were inspected, or an appropriate error code.
 */
static cardano_error_t
visit_certificate_purposes(const script_purpose_visitor_t* visitor, cardano_certificate_set_t* certificates)
{
  const size_t size = cardano_certificate_set_get_length(certificates);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_certificate_t* certificate = NULL;
    cardano_error_t        result      = cardano_certificate_set_get(certificates, i, &certificate);
    cardano_certificate_unref(&certificate);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_cert_type_t type = CARDANO_CERT_TYPE_STAKE_REGISTRATION;

    result = cardano_cert_get_type(certificate, &type);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_credential_t* credential = NULL;

    result = get_certificate_witness_credential(certificate, type, &credential);

    if ((result == CARDANO_SUCCESS) && (credential != NULL))
    {
      result = visit_script_credential(visitor, CARDANO_REDEEMER_TAG_CERTIFYING, (uint64_t)i, credential);
    }

    cardano_credential_unref(&credential);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Visits the voting purpose of every voter that is a script.
 *
 * The index of each purpose is the position of its voter among the voters in ledger order.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] procedures The voting procedures, or NULL when the body casts no vote.
 *
 * \return \ref CARDANO_SUCCESS if the voters were inspected, or an appropriate error code.
 */
static cardano_error_t
visit_voter_purposes(const script_purpose_visitor_t* visitor, cardano_voting_procedures_t* procedures)
{
  if (procedures == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_voter_list_t* voters = NULL;
  cardano_error_t       result = cardano_voting_procedures_get_voters(procedures, &voters);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t size = cardano_voter_list_get_length(voters);

  for (size_t i = 0U; (i < size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_voter_t* voter = NULL;
    uint64_t         index = 0U;

    result = cardano_voter_list_get(voters, i, &voter);

    if (result == CARDANO_SUCCESS)
    {
      result = _cardano_ledger_order_voter_index(voters, voter, &index);
    }

    if (result == CARDANO_SUCCESS)
    {
      cardano_credential_t* credential = cardano_voter_get_credential(voter);

      result = (credential == NULL) ? CARDANO_ERROR_POINTER_IS_NULL : visit_script_credential(visitor, CARDANO_REDEEMER_TAG_VOTING, index, credential);

      cardano_credential_unref(&credential);
    }

    cardano_voter_unref(&voter);
  }

  cardano_voter_list_unref(&voters);

  return result;
}

/**
 * \brief Visits the guarding purpose of every guard that is a script hash.
 *
 * The index of each purpose is the position of its guard in the guard set, counting key hash guards.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] guards The guards, or NULL when the body has none.
 *
 * \return \ref CARDANO_SUCCESS if the guards were inspected, or an appropriate error code.
 */
static cardano_error_t
visit_guard_purposes(const script_purpose_visitor_t* visitor, cardano_guard_set_t* guards)
{
  const size_t size = cardano_guard_set_get_length(guards);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_credential_t* credential = NULL;
    cardano_error_t       result     = cardano_guard_set_get(guards, i, &credential);

    if (result == CARDANO_SUCCESS)
    {
      result = visit_script_credential(visitor, CARDANO_REDEEMER_TAG_GUARDING, (uint64_t)i, credential);
    }

    cardano_credential_unref(&credential);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the guardrails script hash of a proposal procedure.
 *
 * \param[in] proposal The proposal procedure to inspect.
 * \param[out] policy_hash On success, a new reference to the guardrails script hash of the proposal, or NULL if its
 *                         governance action carries none.
 *
 * \return \ref CARDANO_SUCCESS if the proposal was inspected, or an appropriate error code.
 */
static cardano_error_t
get_proposal_policy_hash(cardano_proposal_procedure_t* proposal, cardano_blake2b_hash_t** policy_hash)
{
  *policy_hash = NULL;

  cardano_governance_action_type_t type   = CARDANO_GOVERNANCE_ACTION_TYPE_INFO;
  cardano_error_t                  result = cardano_proposal_procedure_get_action_type(proposal, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE:
    {
      cardano_parameter_change_action_t* action = NULL;

      result = cardano_proposal_procedure_to_parameter_change_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        *policy_hash = cardano_parameter_change_action_get_policy_hash(action);
      }

      cardano_parameter_change_action_unref(&action);
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS:
    {
      cardano_treasury_withdrawals_action_t* action = NULL;

      result = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        *policy_hash = cardano_treasury_withdrawals_action_get_policy_hash(action);
      }

      cardano_treasury_withdrawals_action_unref(&action);
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION:
    case CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE:
    case CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE:
    case CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION:
    case CARDANO_GOVERNANCE_ACTION_TYPE_INFO:
    default:
    {
      break;
    }
  }

  return result;
}

/**
 * \brief Visits the proposing purpose of every proposal procedure whose governance action carries a guardrails script.
 *
 * The index of each purpose is the position of its proposal in the proposal list.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] proposals The proposal procedures, or NULL when the body proposes nothing.
 *
 * \return \ref CARDANO_SUCCESS if the proposals were inspected, or an appropriate error code.
 */
static cardano_error_t
visit_proposal_purposes(const script_purpose_visitor_t* visitor, cardano_proposal_procedure_set_t* proposals)
{
  const size_t size = cardano_proposal_procedure_set_get_length(proposals);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_proposal_procedure_t* proposal = NULL;
    cardano_error_t               result   = cardano_proposal_procedure_set_get(proposals, i, &proposal);
    cardano_proposal_procedure_unref(&proposal);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_blake2b_hash_t* policy_hash = NULL;

    result = get_proposal_policy_hash(proposal, &policy_hash);

    if ((result == CARDANO_SUCCESS) && (policy_hash != NULL))
    {
      result = visitor->callback(visitor->context, CARDANO_REDEEMER_TAG_PROPOSING, (uint64_t)i, policy_hash);
    }

    cardano_blake2b_hash_unref(&policy_hash);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Visits every script purpose of a transaction body.
 *
 * The purposes are visited in this order: the spent inputs, the mint field, the withdrawals, the certificates, the
 * voters, the proposals and the guards.
 *
 * \param[in] visitor The visitor that receives the purposes.
 * \param[in] body The transaction body whose purposes are visited.
 * \param[in] inputs The inputs spent by the body, or NULL to visit no spend purpose.
 * \param[in] resolved_inputs The resolved UTXOs of \p inputs.
 *
 * \return \ref CARDANO_SUCCESS if every purpose was visited, or the first error code returned while visiting them.
 */
static cardano_error_t
visit_script_purposes(
  const script_purpose_visitor_t*  visitor,
  cardano_transaction_body_t*      body,
  cardano_transaction_input_set_t* inputs,
  cardano_utxo_list_t*             resolved_inputs)
{
  cardano_error_t result = visit_input_purposes(visitor, inputs, resolved_inputs);

  if (result == CARDANO_SUCCESS)
  {
    cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
    cardano_multi_asset_unref(&mint);

    result = visit_mint_purposes(visitor, mint);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
    cardano_withdrawal_map_unref(&withdrawals);

    result = visit_withdrawal_purposes(visitor, withdrawals);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);
    cardano_certificate_set_unref(&certificates);

    result = visit_certificate_purposes(visitor, certificates);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_voting_procedures_t* procedures = cardano_transaction_body_get_voting_procedures(body);
    cardano_voting_procedures_unref(&procedures);

    result = visit_voter_purposes(visitor, procedures);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);
    cardano_proposal_procedure_set_unref(&proposals);

    result = visit_proposal_purposes(visitor, proposals);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);
    cardano_guard_set_unref(&guards);

    result = visit_guard_purposes(visitor, guards);
  }

  return result;
}

/**
 * \brief Records the language of a provided script if the script is needed.
 *
 * \param[in,out] lookup The lookup that holds the needed script hashes and receives the match.
 * \param[in] hash The hash of the provided script.
 * \param[in] language The language of the provided script.
 *
 * \return \ref CARDANO_SUCCESS if the script was recorded or is not needed, or an appropriate error code.
 */
static cardano_error_t
record_provided_script(
  script_lookup_t*          lookup,
  cardano_blake2b_hash_t*   hash,
  cardano_script_language_t language)
{
  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!_cardano_blake2b_hash_set_has(lookup->needed_scripts, hash))
  {
    return CARDANO_SUCCESS;
  }

  bool is_plutus = true;

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      lookup->languages.has_plutus_v1 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      lookup->languages.has_plutus_v2 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      lookup->languages.has_plutus_v3 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V4:
    {
      lookup->languages.has_plutus_v4 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
    default:
    {
      is_plutus = false;
      break;
    }
  }

  cardano_error_t result = add_unique_hash(lookup->matched_scripts, hash);

  if ((result == CARDANO_SUCCESS) && is_plutus)
  {
    result = add_unique_hash(lookup->plutus_scripts, hash);
  }

  return result;
}

/**
 * \brief Records the languages of the needed scripts among the scripts of a witness set.
 *
 * \param[in] witnesses The witness set that provides the scripts.
 * \param[in,out] lookup The lookup that holds the needed script hashes and receives the matches.
 *
 * \return \ref CARDANO_SUCCESS if the scripts were inspected, or an appropriate error code.
 */
static cardano_error_t
record_witness_set_scripts(cardano_witness_set_t* witnesses, script_lookup_t* lookup)
{
  cardano_error_t result = CARDANO_SUCCESS;

  cardano_native_script_set_t* native_scripts = cardano_witness_set_get_native_scripts(witnesses);
  cardano_native_script_set_unref(&native_scripts);

  for (size_t i = 0U; (i < cardano_native_script_set_get_length(native_scripts)) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_native_script_t* script = NULL;

    result = cardano_native_script_set_get(native_scripts, i, &script);

    if (result == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(script);

      result = record_provided_script(lookup, hash, CARDANO_SCRIPT_LANGUAGE_NATIVE);

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_native_script_unref(&script);
  }

  cardano_plutus_v1_script_set_t* plutus_v1_scripts = cardano_witness_set_get_plutus_v1_scripts(witnesses);
  cardano_plutus_v1_script_set_unref(&plutus_v1_scripts);

  for (size_t i = 0U; (i < cardano_plutus_v1_script_set_get_length(plutus_v1_scripts)) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_v1_script_t* script = NULL;

    result = cardano_plutus_v1_script_set_get(plutus_v1_scripts, i, &script);

    if (result == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v1_script_get_hash(script);

      result = record_provided_script(lookup, hash, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1);

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v1_script_unref(&script);
  }

  cardano_plutus_v2_script_set_t* plutus_v2_scripts = cardano_witness_set_get_plutus_v2_scripts(witnesses);
  cardano_plutus_v2_script_set_unref(&plutus_v2_scripts);

  for (size_t i = 0U; (i < cardano_plutus_v2_script_set_get_length(plutus_v2_scripts)) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_v2_script_t* script = NULL;

    result = cardano_plutus_v2_script_set_get(plutus_v2_scripts, i, &script);

    if (result == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v2_script_get_hash(script);

      result = record_provided_script(lookup, hash, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2);

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v2_script_unref(&script);
  }

  cardano_plutus_v3_script_set_t* plutus_v3_scripts = cardano_witness_set_get_plutus_v3_scripts(witnesses);
  cardano_plutus_v3_script_set_unref(&plutus_v3_scripts);

  for (size_t i = 0U; (i < cardano_plutus_v3_script_set_get_length(plutus_v3_scripts)) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_v3_script_t* script = NULL;

    result = cardano_plutus_v3_script_set_get(plutus_v3_scripts, i, &script);

    if (result == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v3_script_get_hash(script);

      result = record_provided_script(lookup, hash, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3);

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v3_script_unref(&script);
  }

  cardano_plutus_v4_script_set_t* plutus_v4_scripts = cardano_witness_set_get_plutus_v4_scripts(witnesses);
  cardano_plutus_v4_script_set_unref(&plutus_v4_scripts);

  for (size_t i = 0U; (i < cardano_plutus_v4_script_set_get_length(plutus_v4_scripts)) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_v4_script_t* script = NULL;

    result = cardano_plutus_v4_script_set_get(plutus_v4_scripts, i, &script);

    if (result == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v4_script_get_hash(script);

      result = record_provided_script(lookup, hash, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V4);

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v4_script_unref(&script);
  }

  return result;
}

/**
 * \brief Records the languages of the needed scripts among the reference scripts of a list of resolved UTXOs.
 *
 * \param[in] utxos The resolved UTXOs whose reference scripts are provided, or NULL when there are none.
 * \param[in,out] lookup The lookup that holds the needed script hashes and receives the matches.
 *
 * \return \ref CARDANO_SUCCESS if the reference scripts were inspected, or an appropriate error code.
 */
static cardano_error_t
record_reference_scripts(cardano_utxo_list_t* utxos, script_lookup_t* lookup)
{
  const size_t size = cardano_utxo_list_get_length(utxos);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_utxo_t* utxo   = NULL;
    cardano_error_t result = cardano_utxo_list_get(utxos, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_transaction_output_unref(&output);

    cardano_script_t* script = cardano_transaction_output_get_script_ref(output);
    cardano_script_unref(&script);

    if (script == NULL)
    {
      continue;
    }

    cardano_script_language_t language = CARDANO_SCRIPT_LANGUAGE_NATIVE;

    result = cardano_script_get_language(script, &language);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);

    result = record_provided_script(lookup, hash, language);

    cardano_blake2b_hash_unref(&hash);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Records the languages of the needed scripts among the scripts of the witness sets of the sub transactions.
 *
 * \param[in] tx The transaction whose sub transactions provide the scripts.
 * \param[in,out] lookup The lookup that holds the needed script hashes and receives the matches.
 *
 * \return \ref CARDANO_SUCCESS if the scripts were inspected, or an appropriate error code.
 */
static cardano_error_t
record_sub_transaction_scripts(cardano_transaction_t* tx, script_lookup_t* lookup)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < cardano_sub_transaction_set_get_length(sub_transactions)) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_sub_transaction_t* sub_tx = NULL;

    result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_tx);
    cardano_sub_transaction_unref(&sub_tx);

    if (result == CARDANO_SUCCESS)
    {
      cardano_witness_set_t* witnesses = cardano_sub_transaction_get_witness_set(sub_tx);
      cardano_witness_set_unref(&witnesses);

      result = record_witness_set_scripts(witnesses, lookup);
    }
  }

  return result;
}

/**
 * \brief Looks the needed script hashes up among the scripts a transaction is provided with.
 *
 * The provided scripts are pooled over the whole batch, as the ledger does for a top level transaction: the scripts of
 * its witness set and of the witness sets of its sub transactions, and the reference scripts of the resolved UTXOs.
 *
 * \param[in] tx The transaction whose witness sets provide scripts.
 * \param[in] needed_scripts The needed script hashes.
 * \param[in] reference_inputs The resolved reference inputs whose reference scripts are provided, or NULL.
 * \param[in] pre_selected_utxo The pre selected UTXOs whose reference scripts are provided, or NULL.
 * \param[in] selection The UTXOs chosen by coin selection whose reference scripts are provided, or NULL.
 * \param[in] sub_transaction_utxos The resolved UTXOs of the sub transactions whose reference scripts are provided, or NULL.
 * \param[out] lookup On success, the needed script hashes that were matched and the ones matched by a Plutus script,
 *                    which the caller must release with \ref cardano_blake2b_hash_set_unref, and their languages.
 *
 * \return \ref CARDANO_SUCCESS if the provided scripts were inspected, or an appropriate error code.
 */
static cardano_error_t
lookup_provided_scripts(
  cardano_transaction_t*      tx,
  cardano_blake2b_hash_set_t* needed_scripts,
  cardano_utxo_list_t*        reference_inputs,
  cardano_utxo_list_t*        pre_selected_utxo,
  cardano_utxo_list_t*        selection,
  cardano_utxo_list_t*        sub_transaction_utxos,
  script_lookup_t*            lookup)
{
  const plutus_languages_t no_languages = { false, false, false, false };

  lookup->needed_scripts  = needed_scripts;
  lookup->matched_scripts = NULL;
  lookup->plutus_scripts  = NULL;
  lookup->languages       = no_languages;

  cardano_error_t result = cardano_blake2b_hash_set_new(&lookup->matched_scripts);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_blake2b_hash_set_new(&lookup->plutus_scripts);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx);
    cardano_witness_set_unref(&witnesses);

    result = record_witness_set_scripts(witnesses, lookup);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = record_sub_transaction_scripts(tx, lookup);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = record_reference_scripts(reference_inputs, lookup);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = record_reference_scripts(pre_selected_utxo, lookup);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = record_reference_scripts(selection, lookup);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = record_reference_scripts(sub_transaction_utxos, lookup);
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_set_unref(&lookup->matched_scripts);
    cardano_blake2b_hash_set_unref(&lookup->plutus_scripts);
  }

  return result;
}

/* IMPLEMENTATION ************************************************************/

bool
_cardano_has_payment_script(const cardano_address_t* address)
{
  cardano_address_type_t type = CARDANO_ADDRESS_TYPE_BYRON;

  if ((address == NULL) || (cardano_address_get_type(address, &type) != CARDANO_SUCCESS))
  {
    return false;
  }

  return (type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY) || (type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT) || (type == CARDANO_ADDRESS_TYPE_POINTER_SCRIPT) || (type == CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT);
}

cardano_blake2b_hash_t*
_cardano_get_payment_script_hash(cardano_address_t* address)
{
  if (!_cardano_has_payment_script(address))
  {
    return NULL;
  }

  cardano_address_type_t type       = CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT;
  cardano_credential_t*  credential = NULL;

  CARDANO_UNUSED(cardano_address_get_type(address, &type));

  if ((type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY) || (type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT))
  {
    cardano_base_address_t* base_address = cardano_address_to_base_address(address);

    credential = cardano_base_address_get_payment_credential(base_address);
    cardano_base_address_unref(&base_address);
  }
  else if (type == CARDANO_ADDRESS_TYPE_POINTER_SCRIPT)
  {
    cardano_pointer_address_t* pointer_address = cardano_address_to_pointer_address(address);

    credential = cardano_pointer_address_get_payment_credential(pointer_address);
    cardano_pointer_address_unref(&pointer_address);
  }
  else
  {
    cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);

    credential = cardano_enterprise_address_get_payment_credential(enterprise_address);
    cardano_enterprise_address_unref(&enterprise_address);
  }

  cardano_blake2b_hash_t* script_hash = cardano_credential_get_hash(credential);

  cardano_credential_unref(&credential);

  return script_hash;
}

cardano_error_t
_cardano_get_script_hashes_needed(
  cardano_transaction_body_t*  body,
  cardano_utxo_list_t*         resolved_inputs,
  cardano_blake2b_hash_set_t** script_hashes)
{
  if ((body == NULL) || (script_hashes == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_set_t* needed = NULL;
  cardano_error_t             result = cardano_blake2b_hash_set_new(&needed);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_input_set_t* inputs = NULL;

  if (resolved_inputs != NULL)
  {
    inputs = cardano_transaction_body_get_inputs(body);
    cardano_transaction_input_set_unref(&inputs);
  }

  const script_purpose_visitor_t visitor = { collect_script_hash, (void*)needed };

  result = visit_script_purposes(&visitor, body, inputs, resolved_inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_set_unref(&needed);

    return result;
  }

  *script_hashes = needed;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_get_plutus_languages_used(
  cardano_transaction_t*      tx,
  cardano_blake2b_hash_set_t* needed_scripts,
  cardano_utxo_list_t*        reference_inputs,
  cardano_utxo_list_t*        pre_selected_utxo,
  cardano_utxo_list_t*        selection,
  cardano_utxo_list_t*        sub_transaction_utxos,
  bool*                       has_plutus_v1,
  bool*                       has_plutus_v2,
  bool*                       has_plutus_v3,
  bool*                       has_plutus_v4,
  bool*                       has_missing_scripts)
{
  if ((tx == NULL) || (needed_scripts == NULL) || (has_plutus_v1 == NULL) || (has_plutus_v2 == NULL) || (has_plutus_v3 == NULL) || (has_plutus_v4 == NULL) || (has_missing_scripts == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  script_lookup_t lookup = { NULL, NULL, NULL, { false, false, false, false } };

  cardano_error_t result = lookup_provided_scripts(tx, needed_scripts, reference_inputs, pre_selected_utxo, selection, sub_transaction_utxos, &lookup);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  bool is_missing = false;

  const size_t needed_count = cardano_blake2b_hash_set_get_length(needed_scripts);

  for (size_t i = 0U; (i < needed_count) && (result == CARDANO_SUCCESS) && !is_missing; ++i)
  {
    cardano_blake2b_hash_t* hash = NULL;

    result = cardano_blake2b_hash_set_get(needed_scripts, i, &hash);

    if (result == CARDANO_SUCCESS)
    {
      is_missing = !_cardano_blake2b_hash_set_has(lookup.matched_scripts, hash);
    }

    cardano_blake2b_hash_unref(&hash);
  }

  cardano_blake2b_hash_set_unref(&lookup.matched_scripts);
  cardano_blake2b_hash_set_unref(&lookup.plutus_scripts);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *has_plutus_v1       = lookup.languages.has_plutus_v1;
  *has_plutus_v2       = lookup.languages.has_plutus_v2;
  *has_plutus_v3       = lookup.languages.has_plutus_v3;
  *has_plutus_v4       = lookup.languages.has_plutus_v4;
  *has_missing_scripts = is_missing;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_get_plutus_scripts_provided(
  cardano_transaction_t*       tx,
  cardano_blake2b_hash_set_t*  script_hashes,
  cardano_utxo_list_t*         reference_inputs,
  cardano_utxo_list_t*         pre_selected_utxo,
  cardano_utxo_list_t*         selection,
  cardano_utxo_list_t*         sub_transaction_utxos,
  cardano_blake2b_hash_set_t** plutus_scripts)
{
  if ((tx == NULL) || (script_hashes == NULL) || (plutus_scripts == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  script_lookup_t lookup = { NULL, NULL, NULL, { false, false, false, false } };

  const cardano_error_t result = lookup_provided_scripts(tx, script_hashes, reference_inputs, pre_selected_utxo, selection, sub_transaction_utxos, &lookup);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_set_unref(&lookup.matched_scripts);

  *plutus_scripts = lookup.plutus_scripts;

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_find_missing_redeemer(
  cardano_transaction_body_t*      body,
  cardano_transaction_input_set_t* inputs,
  cardano_utxo_list_t*             resolved_inputs,
  cardano_blake2b_hash_set_t*      plutus_scripts,
  cardano_redeemer_list_t*         redeemers,
  cardano_redeemer_tag_t*          tag,
  uint64_t*                        index,
  cardano_blake2b_hash_t**         script_hash)
{
  if ((body == NULL) || (plutus_scripts == NULL) || (tag == NULL) || (index == NULL) || (script_hash == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  missing_redeemer_search_t search = { plutus_scripts, redeemers, CARDANO_REDEEMER_TAG_SPEND, 0U, NULL };

  const script_purpose_visitor_t visitor = { find_purpose_without_redeemer, (void*)&search };

  const cardano_error_t result = visit_script_purposes(&visitor, body, inputs, resolved_inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&search.script_hash);

    return result;
  }

  *tag         = search.tag;
  *index       = search.index;
  *script_hash = search.script_hash;

  return CARDANO_SUCCESS;
}
