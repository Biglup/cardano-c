/**
 * \file stake_vote_delegation_cert.c
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
#include <cardano/certs/stake_vote_delegation_cert.h>
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
 * \brief This certificate is used when an individual wants to delegate their voting
 * rights to any other DRep and simultaneously wants to delegate their stake to a
 * specific stake pool.
 */
typedef struct cardano_stake_vote_delegation_cert_t
{
    cardano_object_t        base;
    cardano_credential_t*   credential;
    cardano_blake2b_hash_t* pool_key_hash;
    cardano_drep_t*         drep;
} cardano_stake_vote_delegation_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a stake_vote_delegation_cert object.
 *
 * This function is responsible for properly deallocating a stake_vote_delegation_cert object (`cardano_stake_vote_delegation_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the stake_vote_delegation_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_stake_vote_delegation_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the stake_vote_delegation_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_stake_vote_delegation_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_stake_vote_delegation_cert_t* data = (cardano_stake_vote_delegation_cert_t*)object;

  cardano_credential_unref(&data->credential);
  cardano_blake2b_hash_unref(&data->pool_key_hash);
  cardano_drep_unref(&data->drep);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_stake_vote_delegation_cert_new(
  cardano_credential_t*                  credential,
  cardano_blake2b_hash_t*                pool_key_hash,
  cardano_drep_t*                        drep,
  cardano_stake_vote_delegation_cert_t** stake_vote_delegation)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pool_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_vote_delegation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_stake_vote_delegation_cert_t* data = _cardano_malloc(sizeof(cardano_stake_vote_delegation_cert_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_stake_vote_delegation_cert_deallocate;

  cardano_credential_ref(credential);
  data->credential = credential;

  cardano_blake2b_hash_ref(pool_key_hash);
  data->pool_key_hash = pool_key_hash;

  cardano_drep_ref(drep);
  data->drep = drep;

  *stake_vote_delegation = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_stake_vote_delegation_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_stake_vote_delegation_cert_t** stake_vote_delegation_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake_vote_delegation_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "stake_vote_delegation_cert";

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
    CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION,
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

  cardano_blake2b_hash_t* hash = NULL;

  cardano_error_t read_hash_result = cardano_blake2b_hash_from_cbor(reader, &hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);

    return read_hash_result;
  }

  cardano_drep_t* drep = NULL;

  cardano_error_t read_drep_result = cardano_drep_from_cbor(reader, &drep);

  if (read_drep_result != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);
    cardano_blake2b_hash_unref(&hash);

    return read_drep_result;
  }

  cardano_error_t new_result = cardano_stake_vote_delegation_cert_new(credential, hash, drep, stake_vote_delegation_cert);

  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);
  cardano_drep_unref(&drep);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_stake_vote_delegation_cert_to_cbor(
  const cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert,
  cardano_cbor_writer_t*                      writer)
{
  if (stake_vote_delegation_cert == NULL)
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

  cardano_error_t write_type_result = cardano_cbor_writer_write_uint(writer, CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION);

  if (write_type_result != CARDANO_SUCCESS)
  {
    return write_type_result;
  }

  cardano_error_t write_credential_result = cardano_credential_to_cbor(stake_vote_delegation_cert->credential, writer);

  if (write_credential_result != CARDANO_SUCCESS)
  {
    return write_credential_result;
  }

  cardano_error_t write_hash_result = cardano_blake2b_hash_to_cbor(stake_vote_delegation_cert->pool_key_hash, writer);

  if (write_hash_result != CARDANO_SUCCESS)
  {
    return write_hash_result;
  }

  cardano_error_t write_drep_result = cardano_drep_to_cbor(stake_vote_delegation_cert->drep, writer);

  if (write_drep_result != CARDANO_SUCCESS)
  {
    return write_drep_result;
  }

  return CARDANO_SUCCESS;
}

cardano_credential_t*
cardano_stake_vote_delegation_cert_get_credential(cardano_stake_vote_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_credential_ref(certificate->credential);

  return certificate->credential;
}

cardano_error_t
cardano_stake_vote_delegation_cert_set_credential(cardano_stake_vote_delegation_cert_t* certificate, cardano_credential_t* credential)
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

cardano_blake2b_hash_t*
cardano_stake_vote_delegation_cert_get_pool_key_hash(cardano_stake_vote_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(certificate->pool_key_hash);

  return certificate->pool_key_hash;
}

cardano_error_t
cardano_stake_vote_delegation_cert_set_pool_key_hash(cardano_stake_vote_delegation_cert_t* certificate, cardano_blake2b_hash_t* hash)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(hash);
  cardano_blake2b_hash_unref(&certificate->pool_key_hash);
  certificate->pool_key_hash = hash;

  return CARDANO_SUCCESS;
}

cardano_drep_t*
cardano_stake_vote_delegation_cert_get_drep(cardano_stake_vote_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_drep_ref(certificate->drep);

  return certificate->drep;
}

cardano_error_t
cardano_stake_vote_delegation_cert_set_drep(
  cardano_stake_vote_delegation_cert_t* certificate,
  cardano_drep_t*                       drep)
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

void
cardano_stake_vote_delegation_cert_unref(cardano_stake_vote_delegation_cert_t** stake_vote_delegation_cert)
{
  if ((stake_vote_delegation_cert == NULL) || (*stake_vote_delegation_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*stake_vote_delegation_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *stake_vote_delegation_cert = NULL;
    return;
  }
}

void
cardano_stake_vote_delegation_cert_ref(cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert)
{
  if (stake_vote_delegation_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&stake_vote_delegation_cert->base);
}

size_t
cardano_stake_vote_delegation_cert_refcount(const cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert)
{
  if (stake_vote_delegation_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&stake_vote_delegation_cert->base);
}

void
cardano_stake_vote_delegation_cert_set_last_error(cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert, const char* message)
{
  cardano_object_set_last_error(&stake_vote_delegation_cert->base, message);
}

const char*
cardano_stake_vote_delegation_cert_get_last_error(const cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert)
{
  return cardano_object_get_last_error(&stake_vote_delegation_cert->base);
}
