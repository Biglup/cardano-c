/**
 * \file bootstrap_witness.c
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
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
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/witness_set/bootstrap_witness.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 4;

/* STRUCTURES ****************************************************************/

/**
 * \brief The bootstrap witness proves that the transaction has the authority to spend
 * the value from the associated Byron-era input UTxOs.
 *
 * Cardano has transitioned away from this type of witness from Shelley and later eras, BootstrapWitnesses
 * are currently deprecated.
 */
typedef struct cardano_bootstrap_witness_t
{
    cardano_object_t              base;
    cardano_ed25519_public_key_t* vkey;
    cardano_ed25519_signature_t*  signature;
    cardano_buffer_t*             chain_code;
    cardano_buffer_t*             attributes;
} cardano_bootstrap_witness_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a bootstrap_witness object.
 *
 * This function is responsible for properly deallocating a bootstrap_witness object (`cardano_bootstrap_witness_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the bootstrap_witness object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_bootstrap_witness_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the bootstrap_witness
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_bootstrap_witness_deallocate(void* object)
{
  assert(object != NULL);

  cardano_bootstrap_witness_t* data = (cardano_bootstrap_witness_t*)object;

  cardano_ed25519_public_key_unref(&data->vkey);
  cardano_ed25519_signature_unref(&data->signature);
  cardano_buffer_unref(&data->chain_code);
  cardano_buffer_unref(&data->attributes);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_bootstrap_witness_new(
  cardano_ed25519_public_key_t* vkey,
  cardano_ed25519_signature_t*  signature,
  cardano_buffer_t*             chain_code,
  cardano_buffer_t*             attributes,
  cardano_bootstrap_witness_t** bootstrap_witness)
{
  if (vkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (chain_code == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (attributes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bootstrap_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bootstrap_witness_t* data = _cardano_malloc(sizeof(cardano_bootstrap_witness_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_bootstrap_witness_deallocate;

  cardano_ed25519_public_key_ref(vkey);
  data->vkey = vkey;

  cardano_ed25519_signature_ref(signature);
  data->signature = signature;

  cardano_buffer_ref(chain_code);
  data->chain_code = chain_code;

  cardano_buffer_ref(attributes);
  data->attributes = attributes;

  *bootstrap_witness = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bootstrap_witness_from_cbor(cardano_cbor_reader_t* reader, cardano_bootstrap_witness_t** bootstrap_witness)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bootstrap_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "bootstrap_witness";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  cardano_buffer_t* vkey_bytes = NULL;

  cardano_error_t read_vkey_result = cardano_cbor_reader_read_bytestring(reader, &vkey_bytes);

  if (read_vkey_result != CARDANO_SUCCESS)
  {
    return read_vkey_result;
  }

  cardano_ed25519_public_key_t* vkey = NULL;

  cardano_error_t new_vkey_result = cardano_ed25519_public_key_from_bytes(cardano_buffer_get_data(vkey_bytes), cardano_buffer_get_size(vkey_bytes), &vkey);
  cardano_buffer_unref(&vkey_bytes);

  if (new_vkey_result != CARDANO_SUCCESS)
  {
    return new_vkey_result;
  }

  cardano_buffer_t* signature_bytes = NULL;

  cardano_error_t read_signature_result = cardano_cbor_reader_read_bytestring(reader, &signature_bytes);

  if (read_signature_result != CARDANO_SUCCESS)
  {
    cardano_ed25519_public_key_unref(&vkey);

    return read_signature_result;
  }

  cardano_ed25519_signature_t* signature = NULL;

  cardano_error_t new_signature_result = cardano_ed25519_signature_from_bytes(cardano_buffer_get_data(signature_bytes), cardano_buffer_get_size(signature_bytes), &signature);

  cardano_buffer_unref(&signature_bytes);

  if (new_signature_result != CARDANO_SUCCESS)
  {
    cardano_ed25519_public_key_unref(&vkey);

    return new_signature_result;
  }

  cardano_buffer_t* chain_code = NULL;

  cardano_error_t read_chain_code_result = cardano_cbor_reader_read_bytestring(reader, &chain_code);

  if (read_chain_code_result != CARDANO_SUCCESS)
  {
    cardano_ed25519_public_key_unref(&vkey);
    cardano_ed25519_signature_unref(&signature);

    return read_chain_code_result;
  }

  cardano_buffer_t* attributes = NULL;

  cardano_error_t read_attributes_result = cardano_cbor_reader_read_bytestring(reader, &attributes);

  if (read_attributes_result != CARDANO_SUCCESS)
  {
    cardano_ed25519_public_key_unref(&vkey);
    cardano_ed25519_signature_unref(&signature);
    cardano_buffer_unref(&chain_code);

    return read_attributes_result;
  }

  cardano_error_t new_result = cardano_bootstrap_witness_new(vkey, signature, chain_code, attributes, bootstrap_witness);

  cardano_ed25519_public_key_unref(&vkey);
  cardano_ed25519_signature_unref(&signature);
  cardano_buffer_unref(&chain_code);
  cardano_buffer_unref(&attributes);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_bootstrap_witness_to_cbor(
  const cardano_bootstrap_witness_t* bootstrap_witness,
  cardano_cbor_writer_t*             writer)
{
  if (bootstrap_witness == NULL)
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

  cardano_error_t write_vkey_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_ed25519_public_key_get_data(bootstrap_witness->vkey),
    cardano_ed25519_public_key_get_bytes_size(bootstrap_witness->vkey));

  if (write_vkey_result != CARDANO_SUCCESS)
  {
    return write_vkey_result;
  }

  cardano_error_t write_signature_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_ed25519_signature_get_data(bootstrap_witness->signature),
    cardano_ed25519_signature_get_bytes_size(bootstrap_witness->signature));

  if (write_signature_result != CARDANO_SUCCESS)
  {
    return write_signature_result;
  }

  cardano_error_t write_chain_code_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_buffer_get_data(bootstrap_witness->chain_code),
    cardano_buffer_get_size(bootstrap_witness->chain_code));

  if (write_chain_code_result != CARDANO_SUCCESS)
  {
    return write_chain_code_result;
  }

  cardano_error_t write_attributes_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_buffer_get_data(bootstrap_witness->attributes),
    cardano_buffer_get_size(bootstrap_witness->attributes));

  if (write_attributes_result != CARDANO_SUCCESS)
  {
    return write_attributes_result;
  }

  return CARDANO_SUCCESS;
}

cardano_ed25519_public_key_t*
cardano_bootstrap_witness_get_vkey(
  cardano_bootstrap_witness_t* bootstrap_witness)
{
  if (bootstrap_witness == NULL)
  {
    return NULL;
  }

  cardano_ed25519_public_key_ref(bootstrap_witness->vkey);

  return bootstrap_witness->vkey;
}

cardano_error_t
cardano_bootstrap_witness_set_vkey(
  cardano_bootstrap_witness_t*  bootstrap_witness,
  cardano_ed25519_public_key_t* vkey)
{
  if (bootstrap_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ed25519_public_key_ref(vkey);
  cardano_ed25519_public_key_unref(&bootstrap_witness->vkey);

  bootstrap_witness->vkey = vkey;

  return CARDANO_SUCCESS;
}

cardano_ed25519_signature_t*
cardano_bootstrap_witness_get_signature(
  cardano_bootstrap_witness_t* bootstrap_witness)
{
  if (bootstrap_witness == NULL)
  {
    return NULL;
  }

  cardano_ed25519_signature_ref(bootstrap_witness->signature);

  return bootstrap_witness->signature;
}

cardano_error_t
cardano_bootstrap_witness_set_signature(
  cardano_bootstrap_witness_t* bootstrap_witness,
  cardano_ed25519_signature_t* signature)
{
  if (bootstrap_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ed25519_signature_ref(signature);
  cardano_ed25519_signature_unref(&bootstrap_witness->signature);

  bootstrap_witness->signature = signature;

  return CARDANO_SUCCESS;
}

cardano_buffer_t*
cardano_bootstrap_witness_get_chain_code(
  cardano_bootstrap_witness_t* bootstrap_witness)
{
  if (bootstrap_witness == NULL)
  {
    return NULL;
  }

  cardano_buffer_ref(bootstrap_witness->chain_code);

  return bootstrap_witness->chain_code;
}

cardano_error_t
cardano_bootstrap_witness_set_chain_code(
  cardano_bootstrap_witness_t* bootstrap_witness,
  cardano_buffer_t*            chan_code)
{
  if (bootstrap_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (chan_code == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_ref(chan_code);
  cardano_buffer_unref(&bootstrap_witness->chain_code);

  bootstrap_witness->chain_code = chan_code;

  return CARDANO_SUCCESS;
}

cardano_buffer_t*
cardano_bootstrap_witness_get_attributes(
  cardano_bootstrap_witness_t* bootstrap_witness)
{
  if (bootstrap_witness == NULL)
  {
    return NULL;
  }

  cardano_buffer_ref(bootstrap_witness->attributes);

  return bootstrap_witness->attributes;
}

cardano_error_t
cardano_bootstrap_witness_set_attributes(
  cardano_bootstrap_witness_t* bootstrap_witness,
  cardano_buffer_t*            attributes)
{
  if (bootstrap_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (attributes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_ref(attributes);
  cardano_buffer_unref(&bootstrap_witness->attributes);

  bootstrap_witness->attributes = attributes;

  return CARDANO_SUCCESS;
}

void
cardano_bootstrap_witness_unref(cardano_bootstrap_witness_t** bootstrap_witness)
{
  if ((bootstrap_witness == NULL) || (*bootstrap_witness == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*bootstrap_witness)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *bootstrap_witness = NULL;
    return;
  }
}

void
cardano_bootstrap_witness_ref(cardano_bootstrap_witness_t* bootstrap_witness)
{
  if (bootstrap_witness == NULL)
  {
    return;
  }

  cardano_object_ref(&bootstrap_witness->base);
}

size_t
cardano_bootstrap_witness_refcount(const cardano_bootstrap_witness_t* bootstrap_witness)
{
  if (bootstrap_witness == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&bootstrap_witness->base);
}

void
cardano_bootstrap_witness_set_last_error(cardano_bootstrap_witness_t* bootstrap_witness, const char* message)
{
  cardano_object_set_last_error(&bootstrap_witness->base, message);
}

const char*
cardano_bootstrap_witness_get_last_error(const cardano_bootstrap_witness_t* bootstrap_witness)
{
  return cardano_object_get_last_error(&bootstrap_witness->base);
}
