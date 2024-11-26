/**
 * \file vkey_witness.c
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
#include <cardano/witness_set/vkey_witness.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief vkey Witness (Verification Key Witness) is a component of a transaction that
 * provides cryptographic proof that proves that the creator of the transaction
 * has access to the private keys controlling the UTxOs being spent.
 */
typedef struct cardano_vkey_witness_t
{
    cardano_object_t              base;
    cardano_ed25519_public_key_t* vkey;
    cardano_ed25519_signature_t*  signature;
} cardano_vkey_witness_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a vkey_witness object.
 *
 * This function is responsible for properly deallocating a vkey_witness object (`cardano_vkey_witness_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the vkey_witness object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_vkey_witness_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the vkey_witness
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_vkey_witness_deallocate(void* object)
{
  assert(object != NULL);

  cardano_vkey_witness_t* data = (cardano_vkey_witness_t*)object;

  cardano_ed25519_public_key_unref(&data->vkey);
  cardano_ed25519_signature_unref(&data->signature);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_vkey_witness_new(
  cardano_ed25519_public_key_t* vkey,
  cardano_ed25519_signature_t*  signature,
  cardano_vkey_witness_t**      vkey_witness)
{
  if (vkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vkey_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_vkey_witness_t* data = _cardano_malloc(sizeof(cardano_vkey_witness_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_vkey_witness_deallocate;

  cardano_ed25519_public_key_ref(vkey);
  data->vkey = vkey;

  cardano_ed25519_signature_ref(signature);
  data->signature = signature;

  *vkey_witness = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_vkey_witness_from_cbor(cardano_cbor_reader_t* reader, cardano_vkey_witness_t** vkey_witness)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vkey_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "vkey_witness";

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

  cardano_error_t new_result = cardano_vkey_witness_new(vkey, signature, vkey_witness);

  cardano_ed25519_public_key_unref(&vkey);
  cardano_ed25519_signature_unref(&signature);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_vkey_witness_to_cbor(
  const cardano_vkey_witness_t* vkey_witness,
  cardano_cbor_writer_t*        writer)
{
  if (vkey_witness == NULL)
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
    cardano_ed25519_public_key_get_data(vkey_witness->vkey),
    cardano_ed25519_public_key_get_bytes_size(vkey_witness->vkey));

  if (write_vkey_result != CARDANO_SUCCESS)
  {
    return write_vkey_result;
  }

  cardano_error_t write_signature_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_ed25519_signature_get_data(vkey_witness->signature),
    cardano_ed25519_signature_get_bytes_size(vkey_witness->signature));

  if (write_signature_result != CARDANO_SUCCESS)
  {
    return write_signature_result;
  }

  return CARDANO_SUCCESS;
}

bool
cardano_vkey_witness_has_public_key(const cardano_vkey_witness_t* vkey_witness, const cardano_ed25519_public_key_t* vkey)
{
  if ((vkey_witness == NULL) || (vkey == NULL))
  {
    return false;
  }

  const byte_t* key_bytes  = cardano_ed25519_public_key_get_data(vkey);
  const size_t  key_size   = cardano_ed25519_public_key_get_bytes_size(vkey);
  const byte_t* vkey_bytes = cardano_ed25519_public_key_get_data(vkey_witness->vkey);
  const size_t  vkey_size  = cardano_ed25519_public_key_get_bytes_size(vkey_witness->vkey);

  return (key_size == vkey_size) && (memcmp(key_bytes, vkey_bytes, key_size) == 0);
}

cardano_ed25519_public_key_t*
cardano_vkey_witness_get_vkey(
  cardano_vkey_witness_t* vkey_witness)
{
  if (vkey_witness == NULL)
  {
    return NULL;
  }

  cardano_ed25519_public_key_ref(vkey_witness->vkey);

  return vkey_witness->vkey;
}

cardano_error_t
cardano_vkey_witness_set_vkey(
  cardano_vkey_witness_t*       vkey_witness,
  cardano_ed25519_public_key_t* vkey)
{
  if (vkey_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ed25519_public_key_ref(vkey);
  cardano_ed25519_public_key_unref(&vkey_witness->vkey);

  vkey_witness->vkey = vkey;

  return CARDANO_SUCCESS;
}

cardano_ed25519_signature_t*
cardano_vkey_witness_get_signature(
  cardano_vkey_witness_t* vkey_witness)
{
  if (vkey_witness == NULL)
  {
    return NULL;
  }

  cardano_ed25519_signature_ref(vkey_witness->signature);

  return vkey_witness->signature;
}

cardano_error_t
cardano_vkey_witness_set_signature(
  cardano_vkey_witness_t*      vkey_witness,
  cardano_ed25519_signature_t* signature)
{
  if (vkey_witness == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ed25519_signature_ref(signature);
  cardano_ed25519_signature_unref(&vkey_witness->signature);

  vkey_witness->signature = signature;

  return CARDANO_SUCCESS;
}

void
cardano_vkey_witness_unref(cardano_vkey_witness_t** vkey_witness)
{
  if ((vkey_witness == NULL) || (*vkey_witness == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*vkey_witness)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *vkey_witness = NULL;
    return;
  }
}

void
cardano_vkey_witness_ref(cardano_vkey_witness_t* vkey_witness)
{
  if (vkey_witness == NULL)
  {
    return;
  }

  cardano_object_ref(&vkey_witness->base);
}

size_t
cardano_vkey_witness_refcount(const cardano_vkey_witness_t* vkey_witness)
{
  if (vkey_witness == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&vkey_witness->base);
}

void
cardano_vkey_witness_set_last_error(cardano_vkey_witness_t* vkey_witness, const char* message)
{
  cardano_object_set_last_error(&vkey_witness->base, message);
}

const char*
cardano_vkey_witness_get_last_error(const cardano_vkey_witness_t* vkey_witness)
{
  return cardano_object_get_last_error(&vkey_witness->base);
}
