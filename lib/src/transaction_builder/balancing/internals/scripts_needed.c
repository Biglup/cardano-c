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
 * \brief Adds the hash of a credential to a set if the credential is a script hash.
 *
 * \param[in,out] set The set that receives the script hash.
 * \param[in] credential The credential to inspect.
 *
 * \return \ref CARDANO_SUCCESS if the credential was inspected, or an appropriate error code.
 */
static cardano_error_t
add_script_credential(cardano_blake2b_hash_set_t* set, cardano_credential_t* credential)
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

  result = add_unique_hash(set, hash);

  cardano_blake2b_hash_unref(&hash);

  return result;
}

/**
 * \brief Adds the payment script hash of every spent input locked by a script.
 *
 * \param[in,out] set The set that receives the script hashes.
 * \param[in] inputs The spent inputs, or NULL when there are none.
 * \param[in] resolved_inputs The resolved UTXOs of the spent inputs.
 *
 * \return \ref CARDANO_SUCCESS if the inputs were inspected, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input is not
 *         resolved, or an appropriate error code.
 */
static cardano_error_t
add_input_scripts(
  cardano_blake2b_hash_set_t*      set,
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

    cardano_blake2b_hash_t* script_hash = _cardano_get_payment_script_hash(address);

    if (script_hash == NULL)
    {
      continue;
    }

    result = add_unique_hash(set, script_hash);

    cardano_blake2b_hash_unref(&script_hash);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Adds the policy id of every entry of the mint field.
 *
 * \param[in,out] set The set that receives the script hashes.
 * \param[in] mint The mint field, or NULL when the body mints nothing.
 *
 * \return \ref CARDANO_SUCCESS if the mint field was inspected, or an appropriate error code.
 */
static cardano_error_t
add_mint_scripts(cardano_blake2b_hash_set_t* set, cardano_multi_asset_t* mint)
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
      result = add_unique_hash(set, policy_id);
    }

    cardano_blake2b_hash_unref(&policy_id);
  }

  cardano_policy_id_list_unref(&policies);

  return result;
}

/**
 * \brief Adds the credential of every withdrawal whose reward address is controlled by a script.
 *
 * \param[in,out] set The set that receives the script hashes.
 * \param[in] withdrawals The withdrawals, or NULL when the body withdraws nothing.
 *
 * \return \ref CARDANO_SUCCESS if the withdrawals were inspected, or an appropriate error code.
 */
static cardano_error_t
add_withdrawal_scripts(cardano_blake2b_hash_set_t* set, cardano_withdrawal_map_t* withdrawals)
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

    cardano_credential_t* credential = cardano_reward_address_get_credential(reward_address);

    if (credential == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    result = add_script_credential(set, credential);

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
 * \brief Adds the script credential that must authorize each certificate.
 *
 * \param[in,out] set The set that receives the script hashes.
 * \param[in] certificates The certificates, or NULL when the body carries none.
 *
 * \return \ref CARDANO_SUCCESS if the certificates were inspected, or an appropriate error code.
 */
static cardano_error_t
add_certificate_scripts(cardano_blake2b_hash_set_t* set, cardano_certificate_set_t* certificates)
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
      result = add_script_credential(set, credential);
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
 * \brief Adds the credential of every voter that is a script.
 *
 * \param[in,out] set The set that receives the script hashes.
 * \param[in] procedures The voting procedures, or NULL when the body casts no vote.
 *
 * \return \ref CARDANO_SUCCESS if the voters were inspected, or an appropriate error code.
 */
static cardano_error_t
add_voter_scripts(cardano_blake2b_hash_set_t* set, cardano_voting_procedures_t* procedures)
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

    result = cardano_voter_list_get(voters, i, &voter);

    if (result == CARDANO_SUCCESS)
    {
      cardano_credential_t* credential = cardano_voter_get_credential(voter);

      result = (credential == NULL) ? CARDANO_ERROR_POINTER_IS_NULL : add_script_credential(set, credential);

      cardano_credential_unref(&credential);
    }

    cardano_voter_unref(&voter);
  }

  cardano_voter_list_unref(&voters);

  return result;
}

/**
 * \brief Adds every guard that is a script hash.
 *
 * \param[in,out] set The set that receives the script hashes.
 * \param[in] guards The guards, or NULL when the body has none.
 *
 * \return \ref CARDANO_SUCCESS if the guards were inspected, or an appropriate error code.
 */
static cardano_error_t
add_guard_scripts(cardano_blake2b_hash_set_t* set, cardano_guard_set_t* guards)
{
  const size_t size = cardano_guard_set_get_length(guards);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_credential_t* credential = NULL;
    cardano_error_t       result     = cardano_guard_set_get(guards, i, &credential);

    if (result == CARDANO_SUCCESS)
    {
      result = add_script_credential(set, credential);
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
 * \brief Adds the guardrails script hash of every proposal procedure whose governance action carries one.
 *
 * \param[in,out] set The set that receives the script hashes.
 * \param[in] proposals The proposal procedures, or NULL when the body proposes nothing.
 *
 * \return \ref CARDANO_SUCCESS if the proposals were inspected, or an appropriate error code.
 */
static cardano_error_t
add_proposal_scripts(cardano_blake2b_hash_set_t* set, cardano_proposal_procedure_set_t* proposals)
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
      result = add_unique_hash(set, policy_hash);
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
 * \brief Records the language of a provided script if the script is needed.
 *
 * \param[in] needed_scripts The needed script hashes.
 * \param[in,out] matched_scripts The needed script hashes matched so far.
 * \param[in] hash The hash of the provided script.
 * \param[in] language The language of the provided script.
 * \param[in,out] languages The languages of the needed scripts matched so far.
 *
 * \return \ref CARDANO_SUCCESS if the script was recorded or is not needed, or an appropriate error code.
 */
static cardano_error_t
record_provided_script(
  cardano_blake2b_hash_set_t* needed_scripts,
  cardano_blake2b_hash_set_t* matched_scripts,
  cardano_blake2b_hash_t*     hash,
  cardano_script_language_t   language,
  plutus_languages_t*         languages)
{
  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!_cardano_blake2b_hash_set_has(needed_scripts, hash))
  {
    return CARDANO_SUCCESS;
  }

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      languages->has_plutus_v1 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      languages->has_plutus_v2 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      languages->has_plutus_v3 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V4:
    {
      languages->has_plutus_v4 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
    default:
    {
      break;
    }
  }

  return add_unique_hash(matched_scripts, hash);
}

/**
 * \brief Records the languages of the needed scripts among the scripts of a witness set.
 *
 * \param[in] witnesses The witness set that provides the scripts.
 * \param[in] needed_scripts The needed script hashes.
 * \param[in,out] matched_scripts The needed script hashes matched so far.
 * \param[in,out] languages The languages of the needed scripts matched so far.
 *
 * \return \ref CARDANO_SUCCESS if the scripts were inspected, or an appropriate error code.
 */
static cardano_error_t
record_witness_set_scripts(
  cardano_witness_set_t*      witnesses,
  cardano_blake2b_hash_set_t* needed_scripts,
  cardano_blake2b_hash_set_t* matched_scripts,
  plutus_languages_t*         languages)
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

      result = record_provided_script(needed_scripts, matched_scripts, hash, CARDANO_SCRIPT_LANGUAGE_NATIVE, languages);

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

      result = record_provided_script(needed_scripts, matched_scripts, hash, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1, languages);

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

      result = record_provided_script(needed_scripts, matched_scripts, hash, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2, languages);

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

      result = record_provided_script(needed_scripts, matched_scripts, hash, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3, languages);

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v3_script_unref(&script);
  }

  return result;
}

/**
 * \brief Records the languages of the needed scripts among the reference scripts of a list of resolved UTXOs.
 *
 * \param[in] utxos The resolved UTXOs whose reference scripts are provided, or NULL when there are none.
 * \param[in] needed_scripts The needed script hashes.
 * \param[in,out] matched_scripts The needed script hashes matched so far.
 * \param[in,out] languages The languages of the needed scripts matched so far.
 *
 * \return \ref CARDANO_SUCCESS if the reference scripts were inspected, or an appropriate error code.
 */
static cardano_error_t
record_reference_scripts(
  cardano_utxo_list_t*        utxos,
  cardano_blake2b_hash_set_t* needed_scripts,
  cardano_blake2b_hash_set_t* matched_scripts,
  plutus_languages_t*         languages)
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

    result = record_provided_script(needed_scripts, matched_scripts, hash, language, languages);

    cardano_blake2b_hash_unref(&hash);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/* IMPLEMENTATION ************************************************************/

cardano_blake2b_hash_t*
_cardano_get_payment_script_hash(cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  cardano_address_type_t type;

  cardano_error_t result = cardano_address_get_type(address, &type);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_credential_t* credential;

  switch (type)
  {
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
    {
      cardano_base_address_t* base_address = cardano_address_to_base_address(address);

      if (base_address == NULL)
      {
        return NULL;
      }

      credential = cardano_base_address_get_payment_credential(base_address);
      cardano_base_address_unref(&base_address);
      break;
    }
    case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
    {
      cardano_pointer_address_t* pointer_address = cardano_address_to_pointer_address(address);

      if (pointer_address == NULL)
      {
        return NULL;
      }

      credential = cardano_pointer_address_get_payment_credential(pointer_address);
      cardano_pointer_address_unref(&pointer_address);
      break;
    }
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
    {
      cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);

      if (enterprise_address == NULL)
      {
        return NULL;
      }

      credential = cardano_enterprise_address_get_payment_credential(enterprise_address);
      cardano_enterprise_address_unref(&enterprise_address);
      break;
    }
    case CARDANO_ADDRESS_TYPE_BYRON:
    case CARDANO_ADDRESS_TYPE_REWARD_KEY:
    case CARDANO_ADDRESS_TYPE_REWARD_SCRIPT:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
    case CARDANO_ADDRESS_TYPE_POINTER_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
    default:
    {
      credential = NULL;
    }
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

  if (resolved_inputs != NULL)
  {
    cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);
    cardano_transaction_input_set_unref(&inputs);

    result = add_input_scripts(needed, inputs, resolved_inputs);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
    cardano_multi_asset_unref(&mint);

    result = add_mint_scripts(needed, mint);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
    cardano_withdrawal_map_unref(&withdrawals);

    result = add_withdrawal_scripts(needed, withdrawals);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);
    cardano_certificate_set_unref(&certificates);

    result = add_certificate_scripts(needed, certificates);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_voting_procedures_t* procedures = cardano_transaction_body_get_voting_procedures(body);
    cardano_voting_procedures_unref(&procedures);

    result = add_voter_scripts(needed, procedures);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);
    cardano_proposal_procedure_set_unref(&proposals);

    result = add_proposal_scripts(needed, proposals);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);
    cardano_guard_set_unref(&guards);

    result = add_guard_scripts(needed, guards);
  }

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

  plutus_languages_t languages = { false, false, false, false };

  cardano_blake2b_hash_set_t* matched_scripts = NULL;
  cardano_error_t             result          = cardano_blake2b_hash_set_new(&matched_scripts);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witnesses);

  result = record_witness_set_scripts(witnesses, needed_scripts, matched_scripts, &languages);

  if (result == CARDANO_SUCCESS)
  {
    result = record_reference_scripts(reference_inputs, needed_scripts, matched_scripts, &languages);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = record_reference_scripts(pre_selected_utxo, needed_scripts, matched_scripts, &languages);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = record_reference_scripts(selection, needed_scripts, matched_scripts, &languages);
  }

  bool is_missing = false;

  const size_t needed_count = cardano_blake2b_hash_set_get_length(needed_scripts);

  for (size_t i = 0U; (i < needed_count) && (result == CARDANO_SUCCESS) && !is_missing; ++i)
  {
    cardano_blake2b_hash_t* hash = NULL;

    result = cardano_blake2b_hash_set_get(needed_scripts, i, &hash);

    if (result == CARDANO_SUCCESS)
    {
      is_missing = !_cardano_blake2b_hash_set_has(matched_scripts, hash);
    }

    cardano_blake2b_hash_unref(&hash);
  }

  cardano_blake2b_hash_set_unref(&matched_scripts);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *has_plutus_v1       = languages.has_plutus_v1;
  *has_plutus_v2       = languages.has_plutus_v2;
  *has_plutus_v3       = languages.has_plutus_v3;
  *has_plutus_v4       = languages.has_plutus_v4;
  *has_missing_scripts = is_missing;

  return CARDANO_SUCCESS;
}
