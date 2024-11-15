/**
 * \file unique_signers.c
 *
 * \author angel.castillo
 * \date   Nov 07, 2024
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

/* INCLUDES ******************************************************************/

#include "unique_signers.h"

#include <cardano/address/base_address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/certs/auth_committee_hot_cert.h>
#include <cardano/certs/pool_registration_cert.h>
#include <cardano/certs/pool_retirement_cert.h>
#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/resign_committee_cold_cert.h>
#include <cardano/certs/stake_delegation_cert.h>
#include <cardano/certs/stake_deregistration_cert.h>
#include <cardano/certs/stake_registration_delegation_cert.h>
#include <cardano/certs/stake_vote_delegation_cert.h>
#include <cardano/certs/stake_vote_registration_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/certs/vote_delegation_cert.h>
#include <cardano/certs/vote_registration_delegation_cert.h>
#include <cardano/common/utxo.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Callback function to find a specific UTXO based on the provided context.
 *
 * This function checks if the provided UTXO matches the criteria specified in `context`.
 *
 * \param[in] item The UTXO to evaluate.
 * \param[in] context A pointer to the search criteria, typically a structure or identifier.
 *
 * \return `true` if the UTXO matches the search criteria; otherwise, `false`.
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

/* IMPLEMENTATION ************************************************************/

bool
_cardano_blake2b_hash_set_has(cardano_blake2b_hash_set_t* set, const cardano_blake2b_hash_t* hash)
{
  if ((set == NULL) || (hash == NULL))
  {
    return false;
  }

  const size_t size = cardano_blake2b_hash_set_get_length(set);

  if (size == 0U)
  {
    return false;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_blake2b_hash_t* current = NULL;

    cardano_error_t result = cardano_blake2b_hash_set_get(set, i, &current);
    cardano_blake2b_hash_unref(&current);

    if (result != CARDANO_SUCCESS)
    {
      return false;
    }

    if (cardano_blake2b_hash_equals(current, hash))
    {
      return true;
    }
  }

  return false;
}

cardano_error_t
_cardano_add_required_signers(cardano_blake2b_hash_set_t* unique_signers, cardano_blake2b_hash_set_t* required_signers)
{
  if (unique_signers == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t size = cardano_blake2b_hash_set_get_length(required_signers);

  if (size == 0U)
  {
    return CARDANO_SUCCESS;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_blake2b_hash_t* current = NULL;
    cardano_error_t         result  = cardano_blake2b_hash_set_get(required_signers, i, &current);

    cardano_blake2b_hash_unref(&current);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (!_cardano_blake2b_hash_set_has(unique_signers, current))
    {
      result = cardano_blake2b_hash_set_add(unique_signers, current);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }
    }
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
_cardano_get_payment_pub_key_hash(cardano_address_t* address)
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
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
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
    case CARDANO_ADDRESS_TYPE_POINTER_KEY:
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
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
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
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
    default:
    {
      credential = NULL;
    }
  }

  cardano_blake2b_hash_t* pub_key_hash = cardano_credential_get_hash(credential);

  cardano_credential_unref(&credential);

  return pub_key_hash;
}

cardano_error_t
_cardano_add_input_signers(
  cardano_blake2b_hash_set_t*      unique_signers,
  cardano_transaction_input_set_t* set,
  cardano_utxo_list_t*             resolved_inputs)
{
  if ((unique_signers == NULL) || (set == NULL) || (resolved_inputs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t size = cardano_transaction_input_set_get_length(set);

  if (size == 0U)
  {
    return CARDANO_SUCCESS;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_transaction_input_t* input  = NULL;
    cardano_error_t              result = cardano_transaction_input_set_get(set, i, &input);
    cardano_transaction_input_unref(&input);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_utxo_t* utxo = cardano_utxo_list_find(resolved_inputs, find_utxo, (void*)input);
    cardano_utxo_unref(&utxo);

    if (utxo == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_transaction_output_unref(&output);

    cardano_address_t* address = cardano_transaction_output_get_address(output);
    cardano_address_unref(&address);

    cardano_blake2b_hash_t* pub_key_hash = _cardano_get_payment_pub_key_hash(address);

    if (pub_key_hash == NULL)
    {
      continue;
    }

    if (!_cardano_blake2b_hash_set_has(unique_signers, pub_key_hash))
    {
      result = cardano_blake2b_hash_set_add(unique_signers, pub_key_hash);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&pub_key_hash);
        return result;
      }
    }

    cardano_blake2b_hash_unref(&pub_key_hash);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_add_withdrawals(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_withdrawal_map_t*   withdrawals)
{
  if (unique_signers == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (withdrawals == NULL)
  {
    return CARDANO_SUCCESS;
  }

  const size_t size = cardano_withdrawal_map_get_length(withdrawals);

  if (size == 0U)
  {
    return CARDANO_SUCCESS;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_reward_address_t* reward_address = NULL;
    cardano_error_t           result         = cardano_withdrawal_map_get_key_at(withdrawals, i, &reward_address);
    cardano_reward_address_unref(&reward_address);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_credential_t*   credential   = cardano_reward_address_get_credential(reward_address);
    cardano_blake2b_hash_t* pub_key_hash = cardano_credential_get_hash(credential);

    cardano_credential_unref(&credential);

    cardano_credential_type_t type;

    result = cardano_credential_get_type(credential, &type);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&pub_key_hash);

      return result;
    }

    if (type != CARDANO_CREDENTIAL_TYPE_KEY_HASH)
    {
      cardano_blake2b_hash_unref(&pub_key_hash);

      continue;
    }

    if (pub_key_hash == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    if (!_cardano_blake2b_hash_set_has(unique_signers, pub_key_hash))
    {
      result = cardano_blake2b_hash_set_add(unique_signers, pub_key_hash);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&pub_key_hash);

        return result;
      }
    }

    cardano_blake2b_hash_unref(&pub_key_hash);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_process_credential(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_credential_t*       credential)
{
  if (unique_signers == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t           result;
  cardano_credential_type_t cred_type;
  result = cardano_credential_get_type(credential, &cred_type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (cred_type != CARDANO_CREDENTIAL_TYPE_KEY_HASH)
  {
    return CARDANO_SUCCESS;
  }

  cardano_blake2b_hash_t* pub_key_hash = cardano_credential_get_hash(credential);

  if (pub_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!_cardano_blake2b_hash_set_has(unique_signers, pub_key_hash))
  {
    result = cardano_blake2b_hash_set_add(unique_signers, pub_key_hash);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&pub_key_hash);
      return result;
    }
  }

  cardano_blake2b_hash_unref(&pub_key_hash);
  return CARDANO_SUCCESS;
}

cardano_error_t
_process_pool_registration(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate)
{
  cardano_error_t                   result;
  cardano_pool_registration_cert_t* registration = NULL;

  result = cardano_certificate_to_pool_registration(certificate, &registration);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_pool_params_t* params = NULL;
  result                        = cardano_pool_registration_cert_get_params(registration, &params);

  if (result != CARDANO_SUCCESS)
  {
    cardano_pool_registration_cert_unref(&registration);

    return result;
  }

  cardano_pool_owners_t* owners = NULL;
  result                        = cardano_pool_params_get_owners(params, &owners);

  if (result != CARDANO_SUCCESS)
  {
    cardano_pool_params_unref(&params);
    cardano_pool_registration_cert_unref(&registration);

    return result;
  }

  const size_t owners_size = cardano_pool_owners_get_length(owners);

  for (size_t j = 0U; j < owners_size; ++j)
  {
    cardano_blake2b_hash_t* pub_key_hash = NULL;
    result                               = cardano_pool_owners_get(owners, j, &pub_key_hash);

    if (result != CARDANO_SUCCESS)
    {
      cardano_pool_owners_unref(&owners);
      cardano_pool_params_unref(&params);
      cardano_pool_registration_cert_unref(&registration);

      return result;
    }

    if (!_cardano_blake2b_hash_set_has(unique_signers, pub_key_hash))
    {
      result = cardano_blake2b_hash_set_add(unique_signers, pub_key_hash);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&pub_key_hash);
        cardano_pool_owners_unref(&owners);
        cardano_pool_params_unref(&params);
        cardano_pool_registration_cert_unref(&registration);

        return result;
      }
    }

    cardano_blake2b_hash_unref(&pub_key_hash);
  }

  cardano_pool_owners_unref(&owners);
  cardano_pool_params_unref(&params);
  cardano_pool_registration_cert_unref(&registration);

  return CARDANO_SUCCESS;
}

cardano_error_t
_process_pool_retirement(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate)
{
  cardano_error_t                 result;
  cardano_pool_retirement_cert_t* retirement = NULL;

  result = cardano_certificate_to_pool_retirement(certificate, &retirement);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_t* pub_key_hash = cardano_pool_retirement_cert_get_pool_key_hash(retirement);

  if (pub_key_hash == NULL)
  {
    cardano_pool_retirement_cert_unref(&retirement);

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!_cardano_blake2b_hash_set_has(unique_signers, pub_key_hash))
  {
    result = cardano_blake2b_hash_set_add(unique_signers, pub_key_hash);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&pub_key_hash);
      cardano_pool_retirement_cert_unref(&retirement);

      return result;
    }
  }

  cardano_blake2b_hash_unref(&pub_key_hash);
  cardano_pool_retirement_cert_unref(&retirement);

  return CARDANO_SUCCESS;
}

cardano_error_t
_process_auth_committee_hot(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate)
{
  cardano_error_t                    result;
  cardano_auth_committee_hot_cert_t* auth_committee = NULL;

  result = cardano_certificate_to_auth_committee_hot(certificate, &auth_committee);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_credential_t* credential = NULL;
  result                           = cardano_auth_committee_hot_cert_get_cold_cred(auth_committee, &credential);

  if (result != CARDANO_SUCCESS)
  {
    cardano_auth_committee_hot_cert_unref(&auth_committee);

    return result;
  }

  result = _process_credential(unique_signers, credential);
  cardano_credential_unref(&credential);
  cardano_auth_committee_hot_cert_unref(&auth_committee);

  return result;
}

cardano_error_t
_process_certificate_with_credential(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate,
  cardano_cert_type_t         type)
{
  cardano_error_t       result;
  cardano_credential_t* credential = NULL;

  switch (type)
  {
    case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
    {
      cardano_stake_deregistration_cert_t* deregistration = NULL;
      result                                              = cardano_certificate_to_stake_deregistration(certificate, &deregistration);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_stake_deregistration_cert_get_credential(deregistration);
      cardano_stake_deregistration_cert_unref(&deregistration);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DELEGATION:
    {
      cardano_stake_delegation_cert_t* delegation = NULL;
      result                                      = cardano_certificate_to_stake_delegation(certificate, &delegation);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_stake_delegation_cert_get_credential(delegation);
      cardano_stake_delegation_cert_unref(&delegation);

      break;
    }
    case CARDANO_CERT_TYPE_REGISTRATION:
    {
      cardano_registration_cert_t* registration = NULL;
      result                                    = cardano_certificate_to_registration(certificate, &registration);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_registration_cert_get_stake_credential(registration);
      cardano_registration_cert_unref(&registration);

      break;
    }
    case CARDANO_CERT_TYPE_UNREGISTRATION:
    {
      cardano_unregistration_cert_t* unregistration = NULL;
      result                                        = cardano_certificate_to_unregistration(certificate, &unregistration);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_unregistration_cert_get_credential(unregistration);
      cardano_unregistration_cert_unref(&unregistration);

      break;
    }
    case CARDANO_CERT_TYPE_VOTE_DELEGATION:
    {
      cardano_vote_delegation_cert_t* vote_delegation = NULL;
      result                                          = cardano_certificate_to_vote_delegation(certificate, &vote_delegation);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_vote_delegation_cert_get_credential(vote_delegation);
      cardano_vote_delegation_cert_unref(&vote_delegation);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
    {
      cardano_stake_vote_delegation_cert_t* stake_vote_delegation = NULL;
      result                                                      = cardano_certificate_to_stake_vote_delegation(certificate, &stake_vote_delegation);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_stake_vote_delegation_cert_get_credential(stake_vote_delegation);
      cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
    {
      cardano_stake_registration_delegation_cert_t* stake_registration_delegation = NULL;
      result                                                                      = cardano_certificate_to_stake_registration_delegation(certificate, &stake_registration_delegation);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_stake_registration_delegation_cert_get_credential(stake_registration_delegation);
      cardano_stake_registration_delegation_cert_unref(&stake_registration_delegation);

      break;
    }
    case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_vote_registration_delegation_cert_t* vote_registration_delegation = NULL;
      result                                                                    = cardano_certificate_to_vote_registration_delegation(certificate, &vote_registration_delegation);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_vote_registration_delegation_cert_get_credential(vote_registration_delegation);
      cardano_vote_registration_delegation_cert_unref(&vote_registration_delegation);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation = NULL;
      result                                                                                = cardano_certificate_to_stake_vote_registration_delegation(certificate, &stake_vote_registration_delegation);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_stake_vote_registration_delegation_cert_get_credential(stake_vote_registration_delegation);
      cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration_delegation);

      break;
    }
    case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
    {
      cardano_resign_committee_cold_cert_t* resign_committee = NULL;
      result                                                 = cardano_certificate_to_resign_committee_cold(certificate, &resign_committee);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_resign_committee_cold_cert_get_credential(resign_committee);
      cardano_resign_committee_cold_cert_unref(&resign_committee);

      break;
    }
    case CARDANO_CERT_TYPE_DREP_REGISTRATION:
    {
      cardano_register_drep_cert_t* register_drep = NULL;
      result                                      = cardano_certificate_to_register_drep(certificate, &register_drep);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_register_drep_cert_get_credential(register_drep);
      cardano_register_drep_cert_unref(&register_drep);

      break;
    }
    case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
    {
      cardano_unregister_drep_cert_t* unregister_drep = NULL;
      result                                          = cardano_certificate_to_unregister_drep(certificate, &unregister_drep);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_unregister_drep_cert_get_credential(unregister_drep);
      cardano_unregister_drep_cert_unref(&unregister_drep);

      break;
    }
    case CARDANO_CERT_TYPE_UPDATE_DREP:
    {
      cardano_update_drep_cert_t* update_drep = NULL;
      result                                  = cardano_certificate_to_update_drep(certificate, &update_drep);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      credential = cardano_update_drep_cert_get_credential(update_drep);
      cardano_update_drep_cert_unref(&update_drep);

      break;
    }
    default:
      return CARDANO_SUCCESS;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = _process_credential(unique_signers, credential);
  cardano_credential_unref(&credential);

  return result;
}

cardano_error_t
_cardano_add_certificates_pub_key_hashes(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_set_t*  certificates)
{
  if (unique_signers == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificates == NULL)
  {
    return CARDANO_SUCCESS;
  }

  const size_t size = cardano_certificate_set_get_length(certificates);

  if (size == 0U)
  {
    return CARDANO_SUCCESS;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_certificate_t* certificate = NULL;
    cardano_error_t        result      = cardano_certificate_set_get(certificates, i, &certificate);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_cert_type_t type;
    result = cardano_cert_get_type(certificate, &type);

    if (result != CARDANO_SUCCESS)
    {
      cardano_certificate_unref(&certificate);

      return result;
    }

    switch (type)
    {
      case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
      case CARDANO_CERT_TYPE_STAKE_DELEGATION:
      case CARDANO_CERT_TYPE_REGISTRATION:
      case CARDANO_CERT_TYPE_UNREGISTRATION:
      case CARDANO_CERT_TYPE_VOTE_DELEGATION:
      case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
      case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
      case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
      case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
      case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
      case CARDANO_CERT_TYPE_DREP_REGISTRATION:
      case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
      case CARDANO_CERT_TYPE_UPDATE_DREP:
      {
        result = _process_certificate_with_credential(unique_signers, certificate, type);

        if (result != CARDANO_SUCCESS)
        {
          cardano_certificate_unref(&certificate);

          return result;
        }

        break;
      }
      case CARDANO_CERT_TYPE_POOL_REGISTRATION:
      {
        result = _process_pool_registration(unique_signers, certificate);

        if (result != CARDANO_SUCCESS)
        {
          cardano_certificate_unref(&certificate);

          return result;
        }

        break;
      }
      case CARDANO_CERT_TYPE_POOL_RETIREMENT:
      {
        result = _process_pool_retirement(unique_signers, certificate);

        if (result != CARDANO_SUCCESS)
        {
          cardano_certificate_unref(&certificate);

          return result;
        }

        break;
      }
      case CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT:
      {
        result = _process_auth_committee_hot(unique_signers, certificate);

        if (result != CARDANO_SUCCESS)
        {
          cardano_certificate_unref(&certificate);

          return result;
        }

        break;
      }
      default:
        break;
    }
    cardano_certificate_unref(&certificate);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_voting_procedures_pub_key_hashes(
  cardano_blake2b_hash_set_t*  unique_signers,
  cardano_voting_procedures_t* procedures)
{
  if (unique_signers == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (procedures == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_voter_list_t* voters = NULL;
  cardano_error_t       result = cardano_voting_procedures_get_voters(procedures, &voters);

  if (result != CARDANO_SUCCESS)
  {
    cardano_voter_list_unref(&voters);

    return result;
  }

  const size_t size = cardano_voter_list_get_length(voters);

  if (size == 0U)
  {
    cardano_voter_list_unref(&voters);
    return CARDANO_SUCCESS;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_voter_t* voter = NULL;
    result                 = cardano_voter_list_get(voters, i, &voter);

    if (result != CARDANO_SUCCESS)
    {
      cardano_voter_unref(&voter);
      cardano_voter_list_unref(&voters);

      return result;
    }

    cardano_credential_t* credential = cardano_voter_get_credential(voter);

    cardano_credential_type_t cred_type;
    cardano_credential_unref(&credential);

    result = cardano_credential_get_type(credential, &cred_type);

    if (result != CARDANO_SUCCESS)
    {
      cardano_voter_unref(&voter);
      cardano_voter_list_unref(&voters);

      return result;
    }

    if (cred_type != CARDANO_CREDENTIAL_TYPE_KEY_HASH)
    {
      cardano_voter_unref(&voter);
      cardano_voter_list_unref(&voters);

      continue;
    }

    cardano_blake2b_hash_t* pub_key_hash = cardano_credential_get_hash(credential);

    if (pub_key_hash == NULL)
    {
      cardano_voter_unref(&voter);
      cardano_voter_list_unref(&voters);

      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    if (!_cardano_blake2b_hash_set_has(unique_signers, pub_key_hash))
    {
      result = cardano_blake2b_hash_set_add(unique_signers, pub_key_hash);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&pub_key_hash);
        cardano_voter_unref(&voter);
        cardano_voter_list_unref(&voters);

        return result;
      }
    }

    cardano_blake2b_hash_unref(&pub_key_hash);
    cardano_voter_unref(&voter);
  }

  cardano_voter_list_unref(&voters);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_get_unique_signers(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  cardano_blake2b_hash_set_t** unique_signers)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (unique_signers == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t*      body              = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs            = cardano_transaction_body_get_inputs(body);
  cardano_transaction_input_set_t* collateral_inputs = cardano_transaction_body_get_collateral(body);

  cardano_transaction_body_unref(&body);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_set_unref(&collateral_inputs);

  if (inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_blake2b_hash_set_new(unique_signers);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_set_t* required_signers = cardano_transaction_body_get_required_signers(body);
  cardano_blake2b_hash_set_unref(&required_signers);

  result = _cardano_add_required_signers(*unique_signers, required_signers);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_set_unref(unique_signers);

    return result;
  }

  result = _cardano_add_input_signers(*unique_signers, inputs, resolved_inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_set_unref(unique_signers);

    return result;
  }

  if (collateral_inputs != NULL)
  {
    result = _cardano_add_input_signers(*unique_signers, collateral_inputs, resolved_inputs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_set_unref(unique_signers);

      return result;
    }
  }

  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
  cardano_withdrawal_map_unref(&withdrawals);

  result = _cardano_add_withdrawals(*unique_signers, withdrawals);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_set_unref(unique_signers);

    return result;
  }

  cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certificates);

  result = _cardano_add_certificates_pub_key_hashes(*unique_signers, certificates);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_set_unref(unique_signers);

    return result;
  }

  cardano_voting_procedures_t* procedures = cardano_transaction_body_get_voting_procedures(body);
  cardano_voting_procedures_unref(&procedures);

  result = _cardano_voting_procedures_pub_key_hashes(*unique_signers, procedures);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_set_unref(unique_signers);

    return result;
  }

  return CARDANO_SUCCESS;
}