/**
 * \file auth_committee_hot_cert.c
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
#include <cardano/error.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 3;

/* STRUCTURES ****************************************************************/

/**
 * \brief This certificate registers the Hot and Cold credentials of a committee member.
 */
typedef struct cardano_auth_committee_hot_cert_t
{
    cardano_object_t      base;
    cardano_credential_t* committee_cold_cred;
    cardano_credential_t* committee_hot_cred;
} cardano_auth_committee_hot_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a auth_committee_hot_cert object.
 *
 * This function is responsible for properly deallocating a auth_committee_hot_cert object (`cardano_auth_committee_hot_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the auth_committee_hot_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_auth_committee_hot_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the auth_committee_hot_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_auth_committee_hot_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_auth_committee_hot_cert_t* data = (cardano_auth_committee_hot_cert_t*)object;

  cardano_credential_unref(&data->committee_cold_cred);
  cardano_credential_unref(&data->committee_hot_cred);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_auth_committee_hot_cert_new(
  cardano_credential_t*               committee_cold_cred,
  cardano_credential_t*               committee_hot_cred,
  cardano_auth_committee_hot_cert_t** auth_committee_hot_cert)
{
  if (committee_cold_cred == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_hot_cred == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (auth_committee_hot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_auth_committee_hot_cert_t* data = _cardano_malloc(sizeof(cardano_auth_committee_hot_cert_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_auth_committee_hot_cert_deallocate;

  cardano_credential_ref(committee_cold_cred);
  data->committee_cold_cred = committee_cold_cred;

  cardano_credential_ref(committee_hot_cred);
  data->committee_hot_cred = committee_hot_cred;

  *auth_committee_hot_cert = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_auth_committee_hot_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_auth_committee_hot_cert_t** auth_committee_hot_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (auth_committee_hot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "auth_committee_hot_cert";

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
    CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT,
    (enum_to_string_callback_t)((void*)&cardano_cert_type_to_string),
    &type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_credential_t* committee_cold_cred = NULL;
  cardano_credential_t* committee_hot_cred  = NULL;

  cardano_error_t read_cred = cardano_credential_from_cbor(reader, &committee_cold_cred);

  if (read_cred != CARDANO_SUCCESS)
  {
    return read_cred;
  }

  read_cred = cardano_credential_from_cbor(reader, &committee_hot_cred);

  if (read_cred != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&committee_cold_cred);

    return read_cred;
  }

  cardano_error_t new_result = cardano_auth_committee_hot_cert_new(committee_cold_cred, committee_hot_cred, auth_committee_hot_cert);

  cardano_credential_unref(&committee_cold_cred);
  cardano_credential_unref(&committee_hot_cred);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_auth_committee_hot_cert_to_cbor(
  const cardano_auth_committee_hot_cert_t* auth_committee_hot_cert,
  cardano_cbor_writer_t*                   writer)
{
  if (auth_committee_hot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_credential_to_cbor(auth_committee_hot_cert->committee_cold_cred, writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_credential_to_cbor(auth_committee_hot_cert->committee_hot_cred, writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_auth_committee_hot_cert_set_cold_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t*              credential)
{
  if (auth_committee_hot == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(credential);
  cardano_credential_unref(&auth_committee_hot->committee_cold_cred);
  auth_committee_hot->committee_cold_cred = credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_auth_committee_hot_cert_get_cold_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t**             credential)
{
  if (auth_committee_hot == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(auth_committee_hot->committee_cold_cred);
  *credential = auth_committee_hot->committee_cold_cred;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_auth_committee_hot_cert_set_hot_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t*              credential)
{
  if (auth_committee_hot == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(credential);
  cardano_credential_unref(&auth_committee_hot->committee_hot_cred);
  auth_committee_hot->committee_hot_cred = credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_auth_committee_hot_cert_get_hot_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t**             credential)
{
  if (auth_committee_hot == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(auth_committee_hot->committee_hot_cred);
  *credential = auth_committee_hot->committee_hot_cred;

  return CARDANO_SUCCESS;
}

void
cardano_auth_committee_hot_cert_unref(cardano_auth_committee_hot_cert_t** auth_committee_hot_cert)
{
  if ((auth_committee_hot_cert == NULL) || (*auth_committee_hot_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*auth_committee_hot_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *auth_committee_hot_cert = NULL;
    return;
  }
}

void
cardano_auth_committee_hot_cert_ref(cardano_auth_committee_hot_cert_t* auth_committee_hot_cert)
{
  if (auth_committee_hot_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&auth_committee_hot_cert->base);
}

size_t
cardano_auth_committee_hot_cert_refcount(const cardano_auth_committee_hot_cert_t* auth_committee_hot_cert)
{
  if (auth_committee_hot_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&auth_committee_hot_cert->base);
}

void
cardano_auth_committee_hot_cert_set_last_error(cardano_auth_committee_hot_cert_t* auth_committee_hot_cert, const char* message)
{
  cardano_object_set_last_error(&auth_committee_hot_cert->base, message);
}

const char*
cardano_auth_committee_hot_cert_get_last_error(const cardano_auth_committee_hot_cert_t* auth_committee_hot_cert)
{
  return cardano_object_get_last_error(&auth_committee_hot_cert->base);
}
