/**
 * \file implicit_coin.c
 *
 * \author angel.castillo
 * \date   Nov 02, 2024
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

#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/stake_registration_delegation_cert.h>
#include <cardano/certs/stake_vote_registration_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/vote_registration_delegation_cert.h>
#include <cardano/transaction_builder/balancing/implicit_coin.h>
#include <string.h>

#include "../../string_safe.h"

/* STATIC FUNCTIONS **********************************************************/

static cardano_error_t
compute_withdrawals(
  cardano_transaction_body_t* body,
  cardano_implicit_coin_t*    implicit_coin)
{
  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
  cardano_withdrawal_map_unref(&withdrawals);

  const size_t size = cardano_withdrawal_map_get_length(withdrawals);

  for (size_t i = 0U; i < size; ++i)
  {
    uint64_t amount = 0;

    cardano_error_t result = cardano_withdrawal_map_get_value_at(withdrawals, i, &amount);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    implicit_coin->withdrawals += amount;
  }

  return CARDANO_SUCCESS;
}

static cardano_error_t
compute_shelley_deposits(
  cardano_transaction_body_t* body,
  const uint64_t              pool_deposit,
  const uint64_t              stake_deposit,
  cardano_implicit_coin_t*    implicit_coin)
{
  cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certificates);

  const size_t size = cardano_certificate_set_get_length(certificates);

  // Remark: For the case of deregistration (CARDANO_CERT_TYPE_STAKE_DEREGISTRATION and CARDANO_CERT_TYPE_POOL_RETIREMENT) the code here is not entirely correct
  // as we are assuming the current protocol parameters for the deposits where the same as the ones used when the certificates where issued.
  // This is going to work for now, but to properly implement this we need a way to know when the certificate we are undoing was originally issued
  // and get the protocol parameters for that epoch. However, these parameters in particular have never changed in mainnet and these certificates will be deprecated,
  // so this is probably good for now.
  for (size_t i = 0U; i < size; ++i)
  {
    cardano_certificate_t* certificate = NULL;

    cardano_error_t result = cardano_certificate_set_get(certificates, i, &certificate);
    cardano_certificate_unref(&certificate);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_cert_type_t type;

    result = cardano_cert_get_type(certificate, &type);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    switch (type)
    {
      case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
      {
        implicit_coin->deposits += stake_deposit;
        break;
      }
      case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
      {
        implicit_coin->reclaim_deposits += stake_deposit;
        break;
      }
      case CARDANO_CERT_TYPE_POOL_REGISTRATION:
      {
        implicit_coin->deposits += pool_deposit;
        break;
      }
      case CARDANO_CERT_TYPE_POOL_RETIREMENT:
      {
        implicit_coin->reclaim_deposits += pool_deposit;

        break;
      }
      default:
      {
        continue;
      }
    }
  }

  return CARDANO_SUCCESS;
}

static cardano_error_t
compute_conway_deposits(
  cardano_transaction_body_t* body,
  cardano_implicit_coin_t*    implicit_coin)
{
  cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certificates);

  cardano_proposal_procedure_set_t* proposal_procedures = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&proposal_procedures);

  const size_t size = cardano_certificate_set_get_length(certificates);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_certificate_t* certificate = NULL;

    cardano_error_t result = cardano_certificate_set_get(certificates, i, &certificate);
    cardano_certificate_unref(&certificate);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_cert_type_t type;

    result = cardano_cert_get_type(certificate, &type);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    switch (type)
    {
      case CARDANO_CERT_TYPE_REGISTRATION:
      {
        cardano_registration_cert_t* registration = NULL;
        result                                    = cardano_certificate_to_registration(certificate, &registration);
        cardano_registration_cert_unref(&registration);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        implicit_coin->deposits += cardano_registration_cert_get_deposit(registration);

        break;
      }
      case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
      {
        cardano_stake_registration_delegation_cert_t* stake_registration = NULL;
        result                                                           = cardano_certificate_to_stake_registration_delegation(certificate, &stake_registration);
        cardano_stake_registration_delegation_cert_unref(&stake_registration);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        implicit_coin->deposits += cardano_stake_registration_delegation_cert_get_deposit(stake_registration);

        break;
      }
      case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
      {
        cardano_vote_registration_delegation_cert_t* vote_registration = NULL;
        result                                                         = cardano_certificate_to_vote_registration_delegation(certificate, &vote_registration);
        cardano_vote_registration_delegation_cert_unref(&vote_registration);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        implicit_coin->deposits += cardano_vote_registration_delegation_cert_get_deposit(vote_registration);

        break;
      }
      case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
      {
        cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration = NULL;
        result                                                                     = cardano_certificate_to_stake_vote_registration_delegation(certificate, &stake_vote_registration);
        cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        implicit_coin->deposits += cardano_stake_vote_registration_delegation_cert_get_deposit(stake_vote_registration);

        break;
      }
      case CARDANO_CERT_TYPE_UNREGISTRATION:
      {
        cardano_unregistration_cert_t* unregistration = NULL;
        result                                        = cardano_certificate_to_unregistration(certificate, &unregistration);
        cardano_unregistration_cert_unref(&unregistration);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        implicit_coin->reclaim_deposits += cardano_unregistration_cert_get_deposit(unregistration);

        break;
      }
      case CARDANO_CERT_TYPE_DREP_REGISTRATION:
      {
        cardano_register_drep_cert_t* register_drep = NULL;
        result                                      = cardano_certificate_to_register_drep(certificate, &register_drep);
        cardano_register_drep_cert_unref(&register_drep);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        implicit_coin->deposits += cardano_register_drep_cert_get_deposit(register_drep);

        break;
      }
      case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
      {
        cardano_unregister_drep_cert_t* unregister_drep = NULL;
        result                                          = cardano_certificate_to_unregister_drep(certificate, &unregister_drep);
        cardano_unregister_drep_cert_unref(&unregister_drep);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        implicit_coin->reclaim_deposits += cardano_unregister_drep_cert_get_deposit(unregister_drep);

        break;
      }
      default:
      {
        continue;
      }
    }
  }

  const size_t proposal_size = cardano_proposal_procedure_set_get_length(proposal_procedures);

  for (size_t i = 0U; i < proposal_size; ++i)
  {
    cardano_proposal_procedure_t* proposal_procedure = NULL;

    cardano_error_t result = cardano_proposal_procedure_set_get(proposal_procedures, i, &proposal_procedure);
    cardano_proposal_procedure_unref(&proposal_procedure);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    implicit_coin->deposits += cardano_proposal_procedure_get_deposit(proposal_procedure);
  }

  return CARDANO_SUCCESS;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_compute_implicit_coin(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol_params,
  cardano_implicit_coin_t*       implicit_coin)
{
  if (tx == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (implicit_coin == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  const uint64_t pool_deposit  = cardano_protocol_parameters_get_pool_deposit(protocol_params);
  const uint64_t stake_deposit = cardano_protocol_parameters_get_key_deposit(protocol_params);

  cardano_error_t result = compute_withdrawals(body, implicit_coin);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = compute_shelley_deposits(body, pool_deposit, stake_deposit, implicit_coin);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = compute_conway_deposits(body, implicit_coin);

  return result;
}