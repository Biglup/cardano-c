/**
 * \file genesis_key_delegation_cert.c
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
#include <cardano/certs/genesis_key_delegation_cert.h>
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
 * \brief This certificate is used to delegate from a Genesis key to a set of keys. This was primarily used in the early
 * phases of the Cardano network's existence during the transition from the Byron to the Shelley era.
 */
typedef struct cardano_genesis_key_delegation_cert_t
{
    cardano_object_t        base;
    cardano_blake2b_hash_t* genesis_hash;
    cardano_blake2b_hash_t* genesis_delegate_hash;
    cardano_blake2b_hash_t* vrf_key_hash;
} cardano_genesis_key_delegation_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a genesis_key_delegation_cert object.
 *
 * This function is responsible for properly deallocating a genesis_key_delegation_cert object (`cardano_genesis_key_delegation_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the genesis_key_delegation_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_genesis_key_delegation_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the genesis_key_delegation_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_genesis_key_delegation_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_genesis_key_delegation_cert_t* data = (cardano_genesis_key_delegation_cert_t*)object;

  cardano_blake2b_hash_unref(&data->genesis_hash);
  cardano_blake2b_hash_unref(&data->genesis_delegate_hash);
  cardano_blake2b_hash_unref(&data->vrf_key_hash);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_genesis_key_delegation_cert_new(
  cardano_blake2b_hash_t*                 genesis_hash,
  cardano_blake2b_hash_t*                 genesis_delegate_hash,
  cardano_blake2b_hash_t*                 vrf_key_hash,
  cardano_genesis_key_delegation_cert_t** cert)
{
  if (genesis_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (genesis_delegate_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vrf_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_genesis_key_delegation_cert_t* data = _cardano_malloc(sizeof(cardano_genesis_key_delegation_cert_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_genesis_key_delegation_cert_deallocate;

  cardano_blake2b_hash_ref(genesis_hash);
  data->genesis_hash = genesis_hash;

  cardano_blake2b_hash_ref(genesis_delegate_hash);
  data->genesis_delegate_hash = genesis_delegate_hash;

  cardano_blake2b_hash_ref(vrf_key_hash);
  data->vrf_key_hash = vrf_key_hash;

  *cert = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_genesis_key_delegation_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_genesis_key_delegation_cert_t** genesis_key_delegation_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (genesis_key_delegation_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "genesis_key_delegation_cert";

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
    CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION,
    (enum_to_string_callback_t)((void*)&cardano_cert_type_to_string),
    &type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_blake2b_hash_t* genesis_hash          = NULL;
  cardano_blake2b_hash_t* genesis_delegate_hash = NULL;
  cardano_blake2b_hash_t* vrf_key_hash          = NULL;

  cardano_error_t read_hash = cardano_blake2b_hash_from_cbor(reader, &genesis_hash);

  if (read_hash != CARDANO_SUCCESS)
  {
    return read_hash;
  }

  read_hash = cardano_blake2b_hash_from_cbor(reader, &genesis_delegate_hash);

  if (read_hash != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&genesis_hash);

    return read_hash;
  }

  read_hash = cardano_blake2b_hash_from_cbor(reader, &vrf_key_hash);

  if (read_hash != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&genesis_hash);
    cardano_blake2b_hash_unref(&genesis_delegate_hash);

    return read_hash;
  }

  cardano_error_t new_result = cardano_genesis_key_delegation_cert_new(genesis_hash, genesis_delegate_hash, vrf_key_hash, genesis_key_delegation_cert);

  cardano_blake2b_hash_unref(&genesis_hash);
  cardano_blake2b_hash_unref(&genesis_delegate_hash);
  cardano_blake2b_hash_unref(&vrf_key_hash);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_genesis_key_delegation_cert_to_cbor(
  const cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert,
  cardano_cbor_writer_t*                       writer)
{
  if (genesis_key_delegation_cert == NULL)
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

  cardano_error_t write_type_result = cardano_cbor_writer_write_uint(writer, CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION);

  if (write_type_result != CARDANO_SUCCESS)
  {
    return write_type_result;
  }

  cardano_error_t write_hash_result = cardano_blake2b_hash_to_cbor(genesis_key_delegation_cert->genesis_hash, writer);

  if (write_hash_result != CARDANO_SUCCESS)
  {
    return write_hash_result;
  }

  write_hash_result = cardano_blake2b_hash_to_cbor(genesis_key_delegation_cert->genesis_delegate_hash, writer);

  if (write_hash_result != CARDANO_SUCCESS)
  {
    return write_hash_result;
  }

  write_hash_result = cardano_blake2b_hash_to_cbor(genesis_key_delegation_cert->vrf_key_hash, writer);

  if (write_hash_result != CARDANO_SUCCESS)
  {
    return write_hash_result;
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_genesis_key_delegation_cert_get_genesis_hash(cardano_genesis_key_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(certificate->genesis_hash);

  return certificate->genesis_hash;
}

cardano_error_t
cardano_genesis_key_delegation_cert_set_genesis_hash(
  cardano_genesis_key_delegation_cert_t* certificate,
  cardano_blake2b_hash_t*                hash)
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
  cardano_blake2b_hash_unref(&certificate->genesis_hash);
  certificate->genesis_hash = (cardano_blake2b_hash_t*)hash;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_genesis_key_delegation_cert_get_genesis_delegate_hash(
  cardano_genesis_key_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(certificate->genesis_delegate_hash);

  return certificate->genesis_delegate_hash;
}

cardano_error_t
cardano_genesis_key_delegation_cert_set_genesis_delegate_hash(
  cardano_genesis_key_delegation_cert_t* certificate,
  cardano_blake2b_hash_t*                hash)
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
  cardano_blake2b_hash_unref(&certificate->genesis_delegate_hash);
  certificate->genesis_delegate_hash = (cardano_blake2b_hash_t*)hash;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_genesis_key_delegation_cert_get_vrf_key_hash(cardano_genesis_key_delegation_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(certificate->vrf_key_hash);

  return certificate->vrf_key_hash;
}

cardano_error_t
cardano_genesis_key_delegation_cert_set_vrf_key_hash(
  cardano_genesis_key_delegation_cert_t* certificate,
  cardano_blake2b_hash_t*                hash)
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
  cardano_blake2b_hash_unref(&certificate->vrf_key_hash);
  certificate->vrf_key_hash = (cardano_blake2b_hash_t*)hash;

  return CARDANO_SUCCESS;
}

void
cardano_genesis_key_delegation_cert_unref(cardano_genesis_key_delegation_cert_t** genesis_key_delegation_cert)
{
  if ((genesis_key_delegation_cert == NULL) || (*genesis_key_delegation_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*genesis_key_delegation_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *genesis_key_delegation_cert = NULL;
    return;
  }
}

void
cardano_genesis_key_delegation_cert_ref(cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert)
{
  if (genesis_key_delegation_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&genesis_key_delegation_cert->base);
}

size_t
cardano_genesis_key_delegation_cert_refcount(const cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert)
{
  if (genesis_key_delegation_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&genesis_key_delegation_cert->base);
}

void
cardano_genesis_key_delegation_cert_set_last_error(cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert, const char* message)
{
  cardano_object_set_last_error(&genesis_key_delegation_cert->base, message);
}

const char*
cardano_genesis_key_delegation_cert_get_last_error(const cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert)
{
  return cardano_object_get_last_error(&genesis_key_delegation_cert->base);
}
