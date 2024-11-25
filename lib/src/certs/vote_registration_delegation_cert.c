/**
 * \file vote_registration_delegation_cert.c
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

#include <cardano/certs/cert_type.h>
#include <cardano/certs/vote_registration_delegation_cert.h>
#include <cardano/common/credential.h>
#include <cardano/common/drep.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 4;

/* STRUCTURES ****************************************************************/

/**
 * \brief This certificate Register the stake key and delegate with a single certificate to a DRep.
 */
typedef struct cardano_vote_registration_delegation_cert_t
{
    cardano_object_t      base;
    cardano_credential_t* credential;
    cardano_drep_t*       drep;
    uint64_t              deposit;
} cardano_vote_registration_delegation_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a vote_registration_delegation_cert object.
 *
 * This function is responsible for properly deallocating a vote_registration_delegation_cert object (`cardano_vote_registration_delegation_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the vote_registration_delegation_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_vote_registration_delegation_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the vote_registration_delegation_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_vote_registration_delegation_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_vote_registration_delegation_cert_t* data = (cardano_vote_registration_delegation_cert_t*)object;

  cardano_credential_unref(&data->credential);
  cardano_drep_unref(&data->drep);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_vote_registration_delegation_cert_new(
  cardano_credential_t*                         credential,
  const uint64_t                                deposit,
  cardano_drep_t*                               drep,
  cardano_vote_registration_delegation_cert_t** vote_registration_delegation)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vote_registration_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_vote_registration_delegation_cert_t* data = _cardano_malloc(sizeof(cardano_vote_registration_delegation_cert_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_vote_registration_delegation_cert_deallocate;

  cardano_credential_ref(credential);
  data->credential = credential;

  cardano_drep_ref(drep);
  data->drep = drep;

  data->deposit = deposit;

  *vote_registration_delegation = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_vote_registration_delegation_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_vote_registration_delegation_cert_t** vote_registration_delegation_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vote_registration_delegation_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "vote_registration_delegation_cert";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION,
    (enum_to_string_callback_t)((void*)&cardano_cert_type_to_string),
    &type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_credential_t* credential = NULL;

  cardano_error_t read_credential_result = cardano_credential_from_cbor(reader, &credential);

  if (read_credential_result != CARDANO_SUCCESS)
  {
    return read_credential_result;
  }

  cardano_drep_t* drep = NULL;

  cardano_error_t read_drep_result = cardano_drep_from_cbor(reader, &drep);

  if (read_drep_result != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);

    return read_drep_result;
  }

  uint64_t deposit = 0U;

  cardano_error_t read_deposit_result = cardano_cbor_reader_read_uint(reader, &deposit);

  if (read_deposit_result != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);
    cardano_drep_unref(&drep);

    return read_deposit_result;
  }

  cardano_error_t new_result = cardano_vote_registration_delegation_cert_new(credential, deposit, drep, vote_registration_delegation_cert);

  cardano_credential_unref(&credential);
  cardano_drep_unref(&drep);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_vote_registration_delegation_cert_to_cbor(
  const cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert,
  cardano_cbor_writer_t*                             writer)
{
  if (vote_registration_delegation_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_array_result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (write_array_result != CARDANO_SUCCESS)
  {
    return write_array_result;
  }

  cardano_error_t write_type_result = cardano_cbor_writer_write_uint(writer, CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION);

  if (write_type_result != CARDANO_SUCCESS)
  {
    return write_type_result;
  }

  cardano_error_t write_credential_result = cardano_credential_to_cbor(vote_registration_delegation_cert->credential, writer);

  if (write_credential_result != CARDANO_SUCCESS)
  {
    return write_credential_result;
  }

  cardano_error_t write_drep_result = cardano_drep_to_cbor(vote_registration_delegation_cert->drep, writer);

  if (write_drep_result != CARDANO_SUCCESS)
  {
    return write_drep_result;
  }

  cardano_error_t write_deposit_result = cardano_cbor_writer_write_uint(writer, vote_registration_delegation_cert->deposit);

  if (write_deposit_result != CARDANO_SUCCESS)
  {
    return write_deposit_result;
  }

  return CARDANO_SUCCESS;
}

cardano_credential_t*
cardano_vote_registration_delegation_cert_get_credential(cardano_vote_registration_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_credential_ref(certificate->credential);

  return certificate->credential;
}

cardano_error_t
cardano_vote_registration_delegation_cert_set_credential(cardano_vote_registration_delegation_cert_t* certificate, cardano_credential_t* credential)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(credential);
  cardano_credential_unref(&certificate->credential);
  certificate->credential = credential;

  return CARDANO_SUCCESS;
}

cardano_drep_t*
cardano_vote_registration_delegation_cert_get_drep(cardano_vote_registration_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_drep_ref(certificate->drep);

  return certificate->drep;
}

cardano_error_t
cardano_vote_registration_delegation_cert_set_drep(
  cardano_vote_registration_delegation_cert_t* certificate,
  cardano_drep_t*                              drep)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_drep_ref(drep);
  cardano_drep_unref(&certificate->drep);
  certificate->drep = drep;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_vote_registration_delegation_cert_get_deposit(
  const cardano_vote_registration_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return 0;
  }

  return certificate->deposit;
}

cardano_error_t
cardano_vote_registration_delegation_cert_set_deposit(
  cardano_vote_registration_delegation_cert_t* certificate,
  const uint64_t                               deposit)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  certificate->deposit = deposit;

  return CARDANO_SUCCESS;
}

void
cardano_vote_registration_delegation_cert_unref(cardano_vote_registration_delegation_cert_t** vote_registration_delegation_cert)
{
  if ((vote_registration_delegation_cert == NULL) || (*vote_registration_delegation_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*vote_registration_delegation_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *vote_registration_delegation_cert = NULL;
    return;
  }
}

void
cardano_vote_registration_delegation_cert_ref(cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert)
{
  if (vote_registration_delegation_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&vote_registration_delegation_cert->base);
}

size_t
cardano_vote_registration_delegation_cert_refcount(const cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert)
{
  if (vote_registration_delegation_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&vote_registration_delegation_cert->base);
}

void
cardano_vote_registration_delegation_cert_set_last_error(cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert, const char* message)
{
  cardano_object_set_last_error(&vote_registration_delegation_cert->base, message);
}

const char*
cardano_vote_registration_delegation_cert_get_last_error(const cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert)
{
  return cardano_object_get_last_error(&vote_registration_delegation_cert->base);
}
