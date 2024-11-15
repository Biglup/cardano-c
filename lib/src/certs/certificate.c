/**
 * \file certificate.c
 *
 * \author angel.castillo
 * \date   Jul 31, 2024
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

#include <cardano/certs/auth_committee_hot_cert.h>
#include <cardano/certs/cert_type.h>
#include <cardano/certs/certificate.h>
#include <cardano/certs/genesis_key_delegation_cert.h>
#include <cardano/certs/mir_cert.h>
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
#include <cardano/error.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t
{
    cardano_object_t                                   base;
    cardano_cert_type_t                                type;
    cardano_auth_committee_hot_cert_t*                 auth_committee_hot_cert;
    cardano_genesis_key_delegation_cert_t*             genesis_key_delegation_cert;
    cardano_mir_cert_t*                                mir_cert;
    cardano_pool_registration_cert_t*                  pool_registration_cert;
    cardano_pool_retirement_cert_t*                    pool_retirement_cert;
    cardano_register_drep_cert_t*                      register_drep_cert;
    cardano_registration_cert_t*                       registration_cert;
    cardano_resign_committee_cold_cert_t*              resign_committee_cold_cert;
    cardano_stake_delegation_cert_t*                   stake_delegation_cert;
    cardano_stake_deregistration_cert_t*               stake_deregistration_cert;
    cardano_stake_registration_cert_t*                 stake_registration_cert;
    cardano_stake_registration_delegation_cert_t*      stake_registration_delegation_cert;
    cardano_stake_vote_delegation_cert_t*              stake_vote_delegation_cert;
    cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation_cert;
    cardano_unregister_drep_cert_t*                    unregister_drep_cert;
    cardano_unregistration_cert_t*                     unregistration_cert;
    cardano_update_drep_cert_t*                        update_drep_cert;
    cardano_vote_delegation_cert_t*                    vote_delegation_cert;
    cardano_vote_registration_delegation_cert_t*       vote_registration_delegation_cert;
} cardano_certificate_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a certificate object.
 *
 * This function is responsible for properly deallocating a certificate object (`cardano_certificate_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the certificate object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_certificate_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the certificate
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_certificate_deallocate(void* object)
{
  assert(object != NULL);

  cardano_certificate_t* data = (cardano_certificate_t*)object;

  cardano_auth_committee_hot_cert_unref(&data->auth_committee_hot_cert);
  cardano_genesis_key_delegation_cert_unref(&data->genesis_key_delegation_cert);
  cardano_mir_cert_unref(&data->mir_cert);
  cardano_pool_registration_cert_unref(&data->pool_registration_cert);
  cardano_pool_retirement_cert_unref(&data->pool_retirement_cert);
  cardano_register_drep_cert_unref(&data->register_drep_cert);
  cardano_registration_cert_unref(&data->registration_cert);
  cardano_resign_committee_cold_cert_unref(&data->resign_committee_cold_cert);
  cardano_stake_delegation_cert_unref(&data->stake_delegation_cert);
  cardano_stake_deregistration_cert_unref(&data->stake_deregistration_cert);
  cardano_stake_registration_cert_unref(&data->stake_registration_cert);
  cardano_stake_registration_delegation_cert_unref(&data->stake_registration_delegation_cert);
  cardano_stake_vote_delegation_cert_unref(&data->stake_vote_delegation_cert);
  cardano_stake_vote_registration_delegation_cert_unref(&data->stake_vote_registration_delegation_cert);
  cardano_unregister_drep_cert_unref(&data->unregister_drep_cert);
  cardano_unregistration_cert_unref(&data->unregistration_cert);
  cardano_update_drep_cert_unref(&data->update_drep_cert);
  cardano_vote_delegation_cert_unref(&data->vote_delegation_cert);
  cardano_vote_registration_delegation_cert_unref(&data->vote_registration_delegation_cert);

  _cardano_free(data);
}

/**
 * \brief Creates a new certificate object.
 *
 * \return A pointer to the newly created cardano_certificate_t object, or `NULL` if the operation failed.
 */
static cardano_certificate_t*
cardano_certificate_new(void)
{
  cardano_certificate_t* data = _cardano_malloc(sizeof(cardano_certificate_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count                          = 1;
  data->base.last_error[0]                      = '\0';
  data->base.deallocator                        = cardano_certificate_deallocate;
  data->auth_committee_hot_cert                 = NULL;
  data->genesis_key_delegation_cert             = NULL;
  data->mir_cert                                = NULL;
  data->pool_registration_cert                  = NULL;
  data->pool_retirement_cert                    = NULL;
  data->register_drep_cert                      = NULL;
  data->registration_cert                       = NULL;
  data->resign_committee_cold_cert              = NULL;
  data->stake_delegation_cert                   = NULL;
  data->stake_deregistration_cert               = NULL;
  data->stake_registration_cert                 = NULL;
  data->stake_registration_delegation_cert      = NULL;
  data->stake_vote_delegation_cert              = NULL;
  data->stake_vote_registration_delegation_cert = NULL;
  data->unregister_drep_cert                    = NULL;
  data->unregistration_cert                     = NULL;
  data->update_drep_cert                        = NULL;
  data->vote_delegation_cert                    = NULL;
  data->vote_registration_delegation_cert       = NULL;
  data->type                                    = CARDANO_CERT_TYPE_STAKE_REGISTRATION;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_certificate_new_auth_committee_hot(
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert,
  cardano_certificate_t**            certificate)
{
  if (auth_committee_hot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_auth_committee_hot_cert_ref(auth_committee_hot_cert);

  data->auth_committee_hot_cert = auth_committee_hot_cert;
  data->type                    = CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_genesis_key_delegation(
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation,
  cardano_certificate_t**                certificate)
{
  if (genesis_key_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_genesis_key_delegation_cert_ref(genesis_key_delegation);

  data->genesis_key_delegation_cert = genesis_key_delegation;
  data->type                        = CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_mir(
  cardano_mir_cert_t*     mir,
  cardano_certificate_t** certificate)
{
  if (mir == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_mir_cert_ref(mir);

  data->mir_cert = mir;
  data->type     = CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_pool_registration(
  cardano_pool_registration_cert_t* pool_registration,
  cardano_certificate_t**           certificate)
{
  if (pool_registration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_pool_registration_cert_ref(pool_registration);

  data->pool_registration_cert = pool_registration;
  data->type                   = CARDANO_CERT_TYPE_POOL_REGISTRATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_pool_retirement(
  cardano_pool_retirement_cert_t* pool_retirement,
  cardano_certificate_t**         certificate)
{
  if (pool_retirement == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_pool_retirement_cert_ref(pool_retirement);

  data->pool_retirement_cert = pool_retirement;
  data->type                 = CARDANO_CERT_TYPE_POOL_RETIREMENT;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_register_drep(
  cardano_register_drep_cert_t* register_drep,
  cardano_certificate_t**       certificate)
{
  if (register_drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_register_drep_cert_ref(register_drep);

  data->register_drep_cert = register_drep;
  data->type               = CARDANO_CERT_TYPE_DREP_REGISTRATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_registration(
  cardano_registration_cert_t* registration,
  cardano_certificate_t**      certificate)
{
  if (registration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_registration_cert_ref(registration);

  data->registration_cert = registration;
  data->type              = CARDANO_CERT_TYPE_REGISTRATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_resign_committee_cold(
  cardano_resign_committee_cold_cert_t* resign_committee_cold,
  cardano_certificate_t**               certificate)
{
  if (resign_committee_cold == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_resign_committee_cold_cert_ref(resign_committee_cold);

  data->resign_committee_cold_cert = resign_committee_cold;
  data->type                       = CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_stake_delegation(
  cardano_stake_delegation_cert_t* stake_delegation,
  cardano_certificate_t**          certificate)
{
  if (stake_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_stake_delegation_cert_ref(stake_delegation);

  data->stake_delegation_cert = stake_delegation;
  data->type                  = CARDANO_CERT_TYPE_STAKE_DELEGATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_stake_deregistration(
  cardano_stake_deregistration_cert_t* stake_deregistration,
  cardano_certificate_t**              certificate)
{
  if (stake_deregistration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_stake_deregistration_cert_ref(stake_deregistration);

  data->stake_deregistration_cert = stake_deregistration;
  data->type                      = CARDANO_CERT_TYPE_STAKE_DEREGISTRATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_stake_registration(
  cardano_stake_registration_cert_t* stake_registration,
  cardano_certificate_t**            certificate)
{
  if (stake_registration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_stake_registration_cert_ref(stake_registration);

  data->stake_registration_cert = stake_registration;
  data->type                    = CARDANO_CERT_TYPE_STAKE_REGISTRATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_stake_registration_delegation(
  cardano_stake_registration_delegation_cert_t* stake_registration_delegation,
  cardano_certificate_t**                       certificate)
{
  if (stake_registration_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_stake_registration_delegation_cert_ref(stake_registration_delegation);

  data->stake_registration_delegation_cert = stake_registration_delegation;
  data->type                               = CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_stake_vote_delegation(
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation,
  cardano_certificate_t**               certificate)
{
  if (stake_vote_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_stake_vote_delegation_cert_ref(stake_vote_delegation);

  data->stake_vote_delegation_cert = stake_vote_delegation;
  data->type                       = CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_stake_vote_registration_delegation(
  cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation,
  cardano_certificate_t**                            certificate)
{
  if (stake_vote_registration_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_stake_vote_registration_delegation_cert_ref(stake_vote_registration_delegation);

  data->stake_vote_registration_delegation_cert = stake_vote_registration_delegation;
  data->type                                    = CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_unregister_drep(
  cardano_unregister_drep_cert_t* unregister_drep,
  cardano_certificate_t**         certificate)
{
  if (unregister_drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_unregister_drep_cert_ref(unregister_drep);

  data->unregister_drep_cert = unregister_drep;
  data->type                 = CARDANO_CERT_TYPE_DREP_UNREGISTRATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_unregistration(
  cardano_unregistration_cert_t* unregistration,
  cardano_certificate_t**        certificate)
{
  if (unregistration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_unregistration_cert_ref(unregistration);

  data->unregistration_cert = unregistration;
  data->type                = CARDANO_CERT_TYPE_UNREGISTRATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_update_drep(
  cardano_update_drep_cert_t* update_drep,
  cardano_certificate_t**     certificate)
{
  if (update_drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_update_drep_cert_ref(update_drep);

  data->update_drep_cert = update_drep;
  data->type             = CARDANO_CERT_TYPE_UPDATE_DREP;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_vote_delegation(
  cardano_vote_delegation_cert_t* vote_delegation,
  cardano_certificate_t**         certificate)
{
  if (vote_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_vote_delegation_cert_ref(vote_delegation);

  data->vote_delegation_cert = vote_delegation;
  data->type                 = CARDANO_CERT_TYPE_VOTE_DELEGATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_new_vote_registration_delegation(
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation,
  cardano_certificate_t**                      certificate)
{
  if (vote_registration_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_t* data = cardano_certificate_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_vote_registration_delegation_cert_ref(vote_registration_delegation);

  data->vote_registration_delegation_cert = vote_registration_delegation;
  data->type                              = CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION;

  *certificate = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_from_cbor(cardano_cbor_reader_t* reader, cardano_certificate_t** certificate)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_reader_t* reader_clone = NULL;

  cardano_error_t result = cardano_cbor_reader_clone(reader, &reader_clone);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  static const char* validator_name = "certificate";

  int64_t array_size = 0;
  result             = cardano_cbor_reader_read_start_array(reader_clone, &array_size);

  CARDANO_UNUSED(array_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader_clone);
    return result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "type",
    reader_clone,
    &type,
    CARDANO_CERT_TYPE_STAKE_REGISTRATION,
    CARDANO_CERT_TYPE_UPDATE_DREP);

  cardano_cbor_reader_unref(&reader_clone);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  switch (type)
  {
    case CARDANO_CERT_TYPE_REGISTRATION:
    {
      cardano_registration_cert_t* registration_cert = NULL;

      result = cardano_registration_cert_from_cbor(reader, &registration_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_registration(registration_cert, certificate);

      cardano_registration_cert_unref(&registration_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_UNREGISTRATION:
    {
      cardano_unregistration_cert_t* unregistration_cert = NULL;

      result = cardano_unregistration_cert_from_cbor(reader, &unregistration_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_unregistration(unregistration_cert, certificate);

      cardano_unregistration_cert_unref(&unregistration_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT:
    {
      cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

      result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_auth_committee_hot(auth_committee_hot_cert, certificate);

      cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION:
    {
      cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

      result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_genesis_key_delegation(genesis_key_delegation_cert, certificate);

      cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS:
    {
      cardano_mir_cert_t* mir_cert = NULL;

      result = cardano_mir_cert_from_cbor(reader, &mir_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_mir(mir_cert, certificate);

      cardano_mir_cert_unref(&mir_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_POOL_REGISTRATION:
    {
      cardano_pool_registration_cert_t* pool_registration_cert = NULL;

      result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_pool_registration(pool_registration_cert, certificate);

      cardano_pool_registration_cert_unref(&pool_registration_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_POOL_RETIREMENT:
    {
      cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;

      result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_pool_retirement(pool_retirement_cert, certificate);

      cardano_pool_retirement_cert_unref(&pool_retirement_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_DREP_REGISTRATION:
    {
      cardano_register_drep_cert_t* register_drep_cert = NULL;

      result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_register_drep(register_drep_cert, certificate);

      cardano_register_drep_cert_unref(&register_drep_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
    {
      cardano_stake_registration_cert_t* stake_registration_cert = NULL;

      result = cardano_stake_registration_cert_from_cbor(reader, &stake_registration_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_stake_registration(stake_registration_cert, certificate);

      cardano_stake_registration_cert_unref(&stake_registration_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
    {
      cardano_resign_committee_cold_cert_t* resign_committee_cold_cert = NULL;

      result = cardano_resign_committee_cold_cert_from_cbor(reader, &resign_committee_cold_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_resign_committee_cold(resign_committee_cold_cert, certificate);

      cardano_resign_committee_cold_cert_unref(&resign_committee_cold_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_STAKE_DELEGATION:
    {
      cardano_stake_delegation_cert_t* stake_delegation_cert = NULL;

      result = cardano_stake_delegation_cert_from_cbor(reader, &stake_delegation_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_stake_delegation(stake_delegation_cert, certificate);

      cardano_stake_delegation_cert_unref(&stake_delegation_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
    {
      cardano_stake_deregistration_cert_t* stake_deregistration_cert = NULL;

      result = cardano_stake_deregistration_cert_from_cbor(reader, &stake_deregistration_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_stake_deregistration(stake_deregistration_cert, certificate);

      cardano_stake_deregistration_cert_unref(&stake_deregistration_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
    {
      cardano_stake_registration_delegation_cert_t* stake_registration_delegation_cert = NULL;

      result = cardano_stake_registration_delegation_cert_from_cbor(reader, &stake_registration_delegation_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_stake_registration_delegation(stake_registration_delegation_cert, certificate);

      cardano_stake_registration_delegation_cert_unref(&stake_registration_delegation_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
    {
      cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;

      result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_stake_vote_delegation(stake_vote_delegation_cert, certificate);

      cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation_cert = NULL;

      result = cardano_stake_vote_registration_delegation_cert_from_cbor(reader, &stake_vote_registration_delegation_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_stake_vote_registration_delegation(stake_vote_registration_delegation_cert, certificate);

      cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration_delegation_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
    {
      cardano_unregister_drep_cert_t* unregister_drep_cert = NULL;

      result = cardano_unregister_drep_cert_from_cbor(reader, &unregister_drep_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_unregister_drep(unregister_drep_cert, certificate);

      cardano_unregister_drep_cert_unref(&unregister_drep_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_UPDATE_DREP:
    {
      cardano_update_drep_cert_t* update_drep_cert = NULL;

      result = cardano_update_drep_cert_from_cbor(reader, &update_drep_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_update_drep(update_drep_cert, certificate);

      cardano_update_drep_cert_unref(&update_drep_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_VOTE_DELEGATION:
    {
      cardano_vote_delegation_cert_t* vote_delegation_cert = NULL;

      result = cardano_vote_delegation_cert_from_cbor(reader, &vote_delegation_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_vote_delegation(vote_delegation_cert, certificate);

      cardano_vote_delegation_cert_unref(&vote_delegation_cert);
      return result;
    }
    case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert = NULL;

      result = cardano_vote_registration_delegation_cert_from_cbor(reader, &vote_registration_delegation_cert);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_certificate_new_vote_registration_delegation(vote_registration_delegation_cert, certificate);

      cardano_vote_registration_delegation_cert_unref(&vote_registration_delegation_cert);
      return result;
    }

    default:
    {
      return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
    }
  }
}

cardano_error_t
cardano_certificate_to_cbor(
  const cardano_certificate_t* certificate,
  cardano_cbor_writer_t*       writer)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  switch (certificate->type)
  {
    case CARDANO_CERT_TYPE_REGISTRATION:
    {
      result = cardano_registration_cert_to_cbor(certificate->registration_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_UNREGISTRATION:
    {
      result = cardano_unregistration_cert_to_cbor(certificate->unregistration_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT:
    {
      result = cardano_auth_committee_hot_cert_to_cbor(certificate->auth_committee_hot_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION:
    {
      result = cardano_genesis_key_delegation_cert_to_cbor(certificate->genesis_key_delegation_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS:
    {
      result = cardano_mir_cert_to_cbor(certificate->mir_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_POOL_REGISTRATION:
    {
      result = cardano_pool_registration_cert_to_cbor(certificate->pool_registration_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_POOL_RETIREMENT:
    {
      result = cardano_pool_retirement_cert_to_cbor(certificate->pool_retirement_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_DREP_REGISTRATION:
    {
      result = cardano_register_drep_cert_to_cbor(certificate->register_drep_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
    {
      result = cardano_stake_registration_cert_to_cbor(certificate->stake_registration_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
    {
      result = cardano_resign_committee_cold_cert_to_cbor(certificate->resign_committee_cold_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DELEGATION:
    {
      result = cardano_stake_delegation_cert_to_cbor(certificate->stake_delegation_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
    {
      result = cardano_stake_deregistration_cert_to_cbor(certificate->stake_deregistration_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
    {
      result = cardano_stake_registration_delegation_cert_to_cbor(certificate->stake_registration_delegation_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
    {
      result = cardano_stake_vote_delegation_cert_to_cbor(certificate->stake_vote_delegation_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
    {
      result = cardano_stake_vote_registration_delegation_cert_to_cbor(certificate->stake_vote_registration_delegation_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
    {
      result = cardano_unregister_drep_cert_to_cbor(certificate->unregister_drep_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_UPDATE_DREP:
    {
      result = cardano_update_drep_cert_to_cbor(certificate->update_drep_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_VOTE_DELEGATION:
    {
      result = cardano_vote_delegation_cert_to_cbor(certificate->vote_delegation_cert, writer);
      break;
    }
    case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
    {
      result = cardano_vote_registration_delegation_cert_to_cbor(certificate->vote_registration_delegation_cert, writer);
      break;
    }

    default:
    {
      result = CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
    }
  }

  return result;
}

cardano_error_t
cardano_cert_get_type(const cardano_certificate_t* certificate, cardano_cert_type_t* type)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = certificate->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_auth_committee_hot(
  cardano_certificate_t*              certificate,
  cardano_auth_committee_hot_cert_t** auth_committee_hot_cert)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (auth_committee_hot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_auth_committee_hot_cert_ref(certificate->auth_committee_hot_cert);

  *auth_committee_hot_cert = certificate->auth_committee_hot_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_genesis_key_delegation(
  cardano_certificate_t*                  certificate,
  cardano_genesis_key_delegation_cert_t** genesis_key_delegation)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (genesis_key_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_genesis_key_delegation_cert_ref(certificate->genesis_key_delegation_cert);

  *genesis_key_delegation = certificate->genesis_key_delegation_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_mir(
  cardano_certificate_t* certificate,
  cardano_mir_cert_t**   mir)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_mir_cert_ref(certificate->mir_cert);

  *mir = certificate->mir_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_pool_registration(
  cardano_certificate_t*             certificate,
  cardano_pool_registration_cert_t** pool_registration)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pool_registration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_POOL_REGISTRATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_pool_registration_cert_ref(certificate->pool_registration_cert);

  *pool_registration = certificate->pool_registration_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_pool_retirement(
  cardano_certificate_t*           certificate,
  cardano_pool_retirement_cert_t** pool_retirement)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pool_retirement == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_POOL_RETIREMENT)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_pool_retirement_cert_ref(certificate->pool_retirement_cert);

  *pool_retirement = certificate->pool_retirement_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_register_drep(
  cardano_certificate_t*         certificate,
  cardano_register_drep_cert_t** register_drep)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (register_drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_DREP_REGISTRATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_register_drep_cert_ref(certificate->register_drep_cert);

  *register_drep = certificate->register_drep_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_registration(
  cardano_certificate_t*        certificate,
  cardano_registration_cert_t** registration)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (registration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_REGISTRATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_registration_cert_ref(certificate->registration_cert);

  *registration = certificate->registration_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_resign_committee_cold(
  cardano_certificate_t*                 certificate,
  cardano_resign_committee_cold_cert_t** resign_committee_cold)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resign_committee_cold == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_resign_committee_cold_cert_ref(certificate->resign_committee_cold_cert);

  *resign_committee_cold = certificate->resign_committee_cold_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_stake_delegation(
  cardano_certificate_t*            certificate,
  cardano_stake_delegation_cert_t** stake_delegation)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_STAKE_DELEGATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_stake_delegation_cert_ref(certificate->stake_delegation_cert);

  *stake_delegation = certificate->stake_delegation_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_stake_deregistration(
  cardano_certificate_t*                certificate,
  cardano_stake_deregistration_cert_t** stake_deregistration)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_deregistration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_STAKE_DEREGISTRATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_stake_deregistration_cert_ref(certificate->stake_deregistration_cert);

  *stake_deregistration = certificate->stake_deregistration_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_stake_registration(
  cardano_certificate_t*              certificate,
  cardano_stake_registration_cert_t** stake_registration)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_registration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_STAKE_REGISTRATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_stake_registration_cert_ref(certificate->stake_registration_cert);

  *stake_registration = certificate->stake_registration_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_stake_registration_delegation(
  cardano_certificate_t*                         certificate,
  cardano_stake_registration_delegation_cert_t** stake_registration_delegation)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_registration_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_stake_registration_delegation_cert_ref(certificate->stake_registration_delegation_cert);

  *stake_registration_delegation = certificate->stake_registration_delegation_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_stake_vote_delegation(
  cardano_certificate_t*                 certificate,
  cardano_stake_vote_delegation_cert_t** stake_vote_delegation)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_vote_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_stake_vote_delegation_cert_ref(certificate->stake_vote_delegation_cert);

  *stake_vote_delegation = certificate->stake_vote_delegation_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_stake_vote_registration_delegation(
  cardano_certificate_t*                              certificate,
  cardano_stake_vote_registration_delegation_cert_t** stake_vote_registration_delegation)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_vote_registration_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_stake_vote_registration_delegation_cert_ref(certificate->stake_vote_registration_delegation_cert);

  *stake_vote_registration_delegation = certificate->stake_vote_registration_delegation_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_unregister_drep(
  cardano_certificate_t*           certificate,
  cardano_unregister_drep_cert_t** unregister_drep)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (unregister_drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_DREP_UNREGISTRATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_unregister_drep_cert_ref(certificate->unregister_drep_cert);

  *unregister_drep = certificate->unregister_drep_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_unregistration(
  cardano_certificate_t*          certificate,
  cardano_unregistration_cert_t** unregistration)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (unregistration == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_UNREGISTRATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_unregistration_cert_ref(certificate->unregistration_cert);

  *unregistration = certificate->unregistration_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_update_drep(
  cardano_certificate_t*       certificate,
  cardano_update_drep_cert_t** update_drep)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_UPDATE_DREP)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_update_drep_cert_ref(certificate->update_drep_cert);

  *update_drep = certificate->update_drep_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_vote_delegation(
  cardano_certificate_t*           certificate,
  cardano_vote_delegation_cert_t** vote_delegation)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vote_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_VOTE_DELEGATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_vote_delegation_cert_ref(certificate->vote_delegation_cert);

  *vote_delegation = certificate->vote_delegation_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_to_vote_registration_delegation(
  cardano_certificate_t*                        certificate,
  cardano_vote_registration_delegation_cert_t** vote_registration_delegation)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vote_registration_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate->type != CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_vote_registration_delegation_cert_ref(certificate->vote_registration_delegation_cert);

  *vote_registration_delegation = certificate->vote_registration_delegation_cert;

  return CARDANO_SUCCESS;
}

void
cardano_certificate_unref(cardano_certificate_t** certificate)
{
  if ((certificate == NULL) || (*certificate == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*certificate)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *certificate = NULL;
    return;
  }
}

void
cardano_certificate_ref(cardano_certificate_t* certificate)
{
  if (certificate == NULL)
  {
    return;
  }

  cardano_object_ref(&certificate->base);
}

size_t
cardano_certificate_refcount(const cardano_certificate_t* certificate)
{
  if (certificate == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&certificate->base);
}

void
cardano_certificate_set_last_error(cardano_certificate_t* certificate, const char* message)
{
  cardano_object_set_last_error(&certificate->base, message);
}

const char*
cardano_certificate_get_last_error(const cardano_certificate_t* certificate)
{
  return cardano_object_get_last_error(&certificate->base);
}
