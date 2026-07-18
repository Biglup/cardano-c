/**
 * \file builder_certs.c
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

#include "builder_certs.h"

#include <cardano/certs/certificate_set.h>
#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/stake_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/certs/vote_delegation_cert.h>
#include <cardano/common/ex_units.h>
#include <cardano/encoding/bech32.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "builder_redeemers.h"

#include <string.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_add_certificate(
  cardano_builder_state_t* state,
  cardano_certificate_t*   certificate,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if (certificate == NULL)
  {
    *error_message = "Certificate is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);

  if (certs == NULL)
  {
    cardano_error_t result = cardano_certificate_set_new(&certs);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create certificate set.";

      return result;
    }

    result = cardano_transaction_body_set_certificates(body, certs);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set certificate set.";
      cardano_certificate_set_unref(&certs);

      return result;
    }
  }

  cardano_certificate_set_unref(&certs);

  cardano_error_t result = cardano_certificate_set_add(certs, certificate);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add certificate.";

    return result;
  }

  if (redeemer != NULL)
  {
    cardano_redeemer_t* rdmer = NULL;

    cardano_ex_units_t* ex_units = NULL;
    result                       = cardano_ex_units_new(0, 0, &ex_units);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create execution units.";

      return result;
    }

    const size_t certs_size = cardano_certificate_set_get_length(certs);

    if (certs_size == 0U)
    {
      cardano_redeemer_unref(&rdmer);
      *error_message = "Unexpected empty certificate set.";

      return CARDANO_ERROR_INVALID_ARGUMENT;
    }

    const size_t cert_index = cardano_certificate_set_get_length(certs) - 1U;

    result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_CERTIFYING, cert_index, redeemer, ex_units, &rdmer);
    cardano_ex_units_unref(&ex_units);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create redeemer.";

      return result;
    }

    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
    cardano_witness_set_unref(&witnesses);

    cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);

    if (redeemers == NULL)
    {
      result = cardano_redeemer_list_new(&redeemers);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_unref(&rdmer);
        *error_message = "Failed to create redeemer list.";

        return result;
      }

      result = cardano_witness_set_set_redeemers(witnesses, redeemers);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_unref(&rdmer);
        *error_message = "Failed to set redeemer list.";
        cardano_redeemer_list_unref(&redeemers);

        return result;
      }
    }

    cardano_redeemer_list_unref(&redeemers);

    result = cardano_redeemer_list_add(redeemers, rdmer);
    cardano_redeemer_unref(&rdmer);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to add redeemer.";

      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_register_reward_address(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message)
{
  if (address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  const uint64_t deposit = cardano_protocol_parameters_get_key_deposit(state->params);

  cardano_certificate_t*       cert         = NULL;
  cardano_registration_cert_t* registration = NULL;
  cardano_error_t              result       = cardano_registration_cert_new(credential, deposit, &registration);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create registration certificate.";

    return result;
  }

  result = cardano_certificate_new_registration(registration, &cert);
  cardano_registration_cert_unref(&registration);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create certificate.";

    return result;
  }

  result = cardano_builder_add_certificate(state, cert, redeemer, error_message);
  cardano_certificate_unref(&cert);

  return result;
}

cardano_error_t
cardano_builder_register_reward_address_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((reward_address == NULL) || (address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_t* addr   = NULL;
  cardano_error_t           result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse reward address.";

    return result;
  }

  result = cardano_builder_register_reward_address(state, addr, redeemer, error_message);
  cardano_reward_address_unref(&addr);

  return result;
}

cardano_error_t
cardano_builder_deregister_reward_address(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message)
{
  if (address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  const uint64_t deposit = cardano_protocol_parameters_get_key_deposit(state->params);

  cardano_certificate_t*         cert           = NULL;
  cardano_unregistration_cert_t* unregistration = NULL;

  cardano_error_t result = cardano_unregistration_cert_new(credential, deposit, &unregistration);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create unregistration certificate.";

    return result;
  }

  result = cardano_certificate_new_unregistration(unregistration, &cert);
  cardano_unregistration_cert_unref(&unregistration);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create certificate.";

    return result;
  }

  result = cardano_builder_add_certificate(state, cert, redeemer, error_message);
  cardano_certificate_unref(&cert);

  return result;
}

cardano_error_t
cardano_builder_deregister_reward_address_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((reward_address == NULL) || (address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_t* addr = NULL;

  cardano_error_t result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse reward address.";

    return result;
  }

  result = cardano_builder_deregister_reward_address(state, addr, redeemer, error_message);
  cardano_reward_address_unref(&addr);

  return result;
}

cardano_error_t
cardano_builder_delegate_stake(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_blake2b_hash_t*   pool_id,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message)
{
  if (address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pool_id == NULL)
  {
    *error_message = "Pool ID is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  cardano_certificate_t*           cert       = NULL;
  cardano_stake_delegation_cert_t* delegation = NULL;

  cardano_error_t result = cardano_stake_delegation_cert_new(credential, pool_id, &delegation);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create delegation certificate.";

    return result;
  }

  result = cardano_certificate_new_stake_delegation(delegation, &cert);
  cardano_stake_delegation_cert_unref(&delegation);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create certificate.";

    return result;
  }

  result = cardano_builder_add_certificate(state, cert, redeemer, error_message);
  cardano_certificate_unref(&cert);

  return result;
}

cardano_error_t
cardano_builder_delegate_stake_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  const char*              pool_id,
  size_t                   pool_id_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((reward_address == NULL) || (address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((pool_id == NULL) || (pool_id_size == 0U))
  {
    *error_message = "Pool ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t pool_id_hex[28] = { 0 };
  char   pool_hrp[5]     = { 0 };

  cardano_error_t result = cardano_encoding_bech32_decode(pool_id, pool_id_size, pool_hrp, sizeof(pool_hrp), pool_id_hex, sizeof(pool_id_hex));

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to decode pool ID.";

    return result;
  }

  if (strncmp(pool_hrp, "pool", 4) != 0)
  {
    *error_message = "Invalid pool ID.";

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_blake2b_hash_t* hash = NULL;
  result                       = cardano_blake2b_hash_from_bytes(pool_id_hex, sizeof(pool_id_hex), &hash);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse pool ID.";

    return result;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&hash);
    *error_message = "Failed to parse reward address.";

    return result;
  }

  result = cardano_builder_delegate_stake(state, addr, hash, redeemer, error_message);
  cardano_reward_address_unref(&addr);
  cardano_blake2b_hash_unref(&hash);

  return result;
}

cardano_error_t
cardano_builder_delegate_voting_power(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_drep_t*           drep,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message)
{
  if (address == NULL)
  {
    *error_message = "Reward address is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (drep == NULL)
  {
    *error_message = "DREP is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  cardano_certificate_t*          cert       = NULL;
  cardano_vote_delegation_cert_t* delegation = NULL;

  cardano_error_t result = cardano_vote_delegation_cert_new(credential, drep, &delegation);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create delegation certificate.";

    return result;
  }

  result = cardano_certificate_new_vote_delegation(delegation, &cert);
  cardano_vote_delegation_cert_unref(&delegation);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create certificate.";

    return result;
  }

  result = cardano_builder_add_certificate(state, cert, redeemer, error_message);
  cardano_certificate_unref(&cert);

  return result;
}

cardano_error_t
cardano_builder_delegate_voting_power_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  const char*              drep_id,
  size_t                   drep_id_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((reward_address == NULL) || (address_size == 0U))
  {
    *error_message = "Reward address is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    *error_message = "DREP ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_t* addr   = NULL;
  cardano_error_t           result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse reward address.";

    return result;
  }

  cardano_drep_t* drep = NULL;
  result               = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse DREP ID.";
    cardano_reward_address_unref(&addr);

    return result;
  }

  result = cardano_builder_delegate_voting_power(state, addr, drep, redeemer, error_message);

  cardano_reward_address_unref(&addr);
  cardano_drep_unref(&drep);

  return result;
}

cardano_error_t
cardano_builder_register_drep(
  cardano_builder_state_t* state,
  cardano_drep_t*          drep,
  cardano_anchor_t*        anchor,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if (drep == NULL)
  {
    *error_message = "DRep is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t*        cert          = NULL;
  cardano_register_drep_cert_t* register_drep = NULL;
  cardano_credential_t*         credential    = NULL;

  cardano_error_t result = cardano_drep_get_credential(drep, &credential);
  cardano_credential_unref(&credential);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to get credential.";

    return result;
  }

  const uint64_t deposit = cardano_protocol_parameters_get_drep_deposit(state->params);
  result                 = cardano_register_drep_cert_new(credential, deposit, anchor, &register_drep);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create DRep registration certificate.";

    return result;
  }

  result = cardano_certificate_new_register_drep(register_drep, &cert);
  cardano_register_drep_cert_unref(&register_drep);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create certificate.";

    return result;
  }

  result = cardano_builder_add_certificate(state, cert, redeemer, error_message);
  cardano_certificate_unref(&cert);

  return result;
}

cardano_error_t
cardano_builder_register_drep_ex(
  cardano_builder_state_t* state,
  const char*              drep_id,
  const size_t             drep_id_size,
  const char*              metadata_url,
  const size_t             metadata_url_size,
  const char*              metadata_hash_hex,
  const size_t             metadata_hash_hex_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    *error_message = "DRep ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_drep_t* drep = NULL;
  result               = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse DRep ID.";

    return result;
  }

  result = cardano_builder_register_drep(state, drep, anchor, redeemer, error_message);

  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);

  return result;
}

cardano_error_t
cardano_builder_update_drep(
  cardano_builder_state_t* state,
  cardano_drep_t*          drep,
  cardano_anchor_t*        anchor,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if (drep == NULL)
  {
    *error_message = "DRep is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t*      cert        = NULL;
  cardano_update_drep_cert_t* update_drep = NULL;
  cardano_credential_t*       credential  = NULL;

  cardano_error_t result = cardano_drep_get_credential(drep, &credential);
  cardano_credential_unref(&credential);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to get credential.";

    return result;
  }

  result = cardano_update_drep_cert_new(credential, anchor, &update_drep);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create DRep update certificate.";

    return result;
  }

  result = cardano_certificate_new_update_drep(update_drep, &cert);
  cardano_update_drep_cert_unref(&update_drep);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create certificate.";

    return result;
  }

  result = cardano_builder_add_certificate(state, cert, redeemer, error_message);
  cardano_certificate_unref(&cert);

  return result;
}

cardano_error_t
cardano_builder_update_drep_ex(
  cardano_builder_state_t* state,
  const char*              drep_id,
  const size_t             drep_id_size,
  const char*              metadata_url,
  const size_t             metadata_url_size,
  const char*              metadata_hash_hex,
  const size_t             metadata_hash_hex_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    *error_message = "DRep ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create anchor.";

    return result;
  }

  cardano_drep_t* drep = NULL;
  result               = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    *error_message = "Failed to parse DRep ID.";

    return result;
  }

  result = cardano_builder_update_drep(state, drep, anchor, redeemer, error_message);

  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);

  return result;
}

cardano_error_t
cardano_builder_deregister_drep(
  cardano_builder_state_t* state,
  cardano_drep_t*          drep,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if (drep == NULL)
  {
    *error_message = "DRep is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t*          cert           = NULL;
  cardano_unregister_drep_cert_t* deregistration = NULL;
  cardano_credential_t*           credential     = NULL;

  cardano_error_t result = cardano_drep_get_credential(drep, &credential);
  cardano_credential_unref(&credential);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to get credential.";

    return result;
  }

  const uint64_t deposit = cardano_protocol_parameters_get_drep_deposit(state->params);
  result                 = cardano_unregister_drep_cert_new(credential, deposit, &deregistration);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create DRep deregistration certificate.";

    return result;
  }

  result = cardano_certificate_new_unregister_drep(deregistration, &cert);
  cardano_unregister_drep_cert_unref(&deregistration);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create certificate.";

    return result;
  }

  result = cardano_builder_add_certificate(state, cert, redeemer, error_message);
  cardano_certificate_unref(&cert);

  return result;
}

cardano_error_t
cardano_builder_deregister_drep_ex(
  cardano_builder_state_t* state,
  const char*              drep_id,
  size_t                   drep_id_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    *error_message = "DRep ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_drep_t* drep   = NULL;
  cardano_error_t result = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse DRep ID.";

    return result;
  }

  result = cardano_builder_deregister_drep(state, drep, redeemer, error_message);
  cardano_drep_unref(&drep);

  return result;
}

cardano_error_t
cardano_builder_add_certificate_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_certificate_t*         certificate,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  const char**                   error_message)
{
  if ((certificate == NULL) || (callback == NULL))
  {
    *error_message = "Certificate and callback are required";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* placeholder = NULL;

  cardano_error_t result = cardano_builder_create_placeholder_plutus_data(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the placeholder redeemer.";

    return result;
  }

  result = cardano_builder_add_certificate(state, certificate, placeholder, error_message);

  cardano_plutus_data_unref(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  const uint64_t cert_index = (uint64_t)(cardano_certificate_set_get_length(certs) - 1U);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);
  cardano_redeemer_list_unref(&redeemers);

  result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  const size_t redeemer_count = cardano_redeemer_list_get_length(redeemers);

  for (size_t i = 0U; (i < redeemer_count) && (result == CARDANO_ERROR_ELEMENT_NOT_FOUND); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    const cardano_error_t get_result = cardano_redeemer_list_get(redeemers, i, &redeemer);

    if (get_result != CARDANO_SUCCESS)
    {
      result = get_result;
      break;
    }

    const bool matches = (cardano_redeemer_get_tag(redeemer) == CARDANO_REDEEMER_TAG_CERTIFYING) &&
      (cardano_redeemer_get_index(redeemer) == cert_index);

    if (matches)
    {
      result = cardano_deferred_redeemer_list_add(state->deferred_redeemers, redeemer, callback, user_context);
    }

    cardano_redeemer_unref(&redeemer);
  }

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to register the deferred redeemer.";
  }

  return result;
}
