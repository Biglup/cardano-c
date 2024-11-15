/**
 * \file ed25519_public_key.c
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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

#include <cardano/crypto/ed25519_public_key.h>

#include <cardano/buffer.h>
#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"

#include <assert.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <sodium.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an Ed25519 public key.
 *
 * This structure is designed to hold an Ed25519 public key, which is part of an asymmetric key pair used in
 * digital signatures.
 */
typedef struct cardano_ed25519_public_key_t
{
    cardano_object_t  base;
    cardano_buffer_t* key_material;
} cardano_ed25519_public_key_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a Ed25519 public key object.
 *
 * This function is responsible for properly deallocating a Ed25519 public key object (`cardano_ed25519_public_key_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the Ed25519 public key object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_ed25519_public_key_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the Ed25519 public key
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_ed25519_public_key_deallocate(void* object)
{
  assert(object != NULL);

  cardano_ed25519_public_key_t* public_key = (cardano_ed25519_public_key_t*)object;

  if (public_key->key_material != NULL)
  {
    cardano_buffer_unref(&public_key->key_material);
  }

  _cardano_free(public_key);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_ed25519_public_key_from_bytes(
  const byte_t*                  data,
  const size_t                   data_length,
  cardano_ed25519_public_key_t** public_key)
{
  if (public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *public_key = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_length != crypto_sign_PUBLICKEYBYTES)
  {
    *public_key = NULL;
    return CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE;
  }

  cardano_ed25519_public_key_t* ed25519_public_key = (cardano_ed25519_public_key_t*)_cardano_malloc(sizeof(cardano_ed25519_public_key_t));

  if (ed25519_public_key == NULL)
  {
    *public_key = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  ed25519_public_key->base.ref_count     = 1;
  ed25519_public_key->base.deallocator   = cardano_ed25519_public_key_deallocate;
  ed25519_public_key->base.last_error[0] = '\0';
  ed25519_public_key->key_material       = cardano_buffer_new_from(data, data_length);

  if (ed25519_public_key->key_material == NULL)
  {
    *public_key = NULL;
    _cardano_free(ed25519_public_key);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *public_key = ed25519_public_key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ed25519_public_key_from_hex(
  const char*                    hex,
  const size_t                   hex_length,
  cardano_ed25519_public_key_t** public_key)
{
  if (public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *public_key = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex_length != (crypto_sign_PUBLICKEYBYTES * 2))
  {
    *public_key = NULL;
    return CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE;
  }

  cardano_ed25519_public_key_t* ed25519_public_key = (cardano_ed25519_public_key_t*)_cardano_malloc(sizeof(cardano_ed25519_public_key_t));

  if (ed25519_public_key == NULL)
  {
    *public_key = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  ed25519_public_key->base.ref_count     = 1;
  ed25519_public_key->base.deallocator   = cardano_ed25519_public_key_deallocate;
  ed25519_public_key->base.last_error[0] = '\0';
  ed25519_public_key->key_material       = cardano_buffer_from_hex(hex, hex_length);

  if (ed25519_public_key->key_material == NULL)
  {
    *public_key = NULL;
    _cardano_free(ed25519_public_key);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *public_key = ed25519_public_key;

  return CARDANO_SUCCESS;
}

void
cardano_ed25519_public_key_unref(cardano_ed25519_public_key_t** ed25519_public_key)
{
  if ((ed25519_public_key == NULL) || (*ed25519_public_key == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*ed25519_public_key)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *ed25519_public_key = NULL;
    return;
  }
}

void
cardano_ed25519_public_key_ref(cardano_ed25519_public_key_t* ed25519_public_key)
{
  if (ed25519_public_key == NULL)
  {
    return;
  }

  cardano_object_ref(&ed25519_public_key->base);
}

size_t
cardano_ed25519_public_key_refcount(const cardano_ed25519_public_key_t* ed25519_public_key)
{
  if (ed25519_public_key == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&ed25519_public_key->base);
}

bool
cardano_ed25519_public_verify(
  const cardano_ed25519_public_key_t* public_key,
  const cardano_ed25519_signature_t*  signature,
  const byte_t*                       message,
  const size_t                        message_len)
{
  if ((public_key == NULL) || (signature == NULL))
  {
    return false;
  }

  assert(cardano_ed25519_signature_get_bytes_size(signature) == crypto_sign_BYTES);

  if (sodium_init() == -1)
  {
    return false;
  }

  int verify_result = crypto_sign_verify_detached(
    cardano_ed25519_signature_get_data(signature),
    message,
    message_len,
    cardano_buffer_get_data(public_key->key_material));

  return verify_result == 0;
}

const byte_t*
cardano_ed25519_public_key_get_data(const cardano_ed25519_public_key_t* ed25519_public_key)
{
  if (ed25519_public_key == NULL)
  {
    return NULL;
  }

  return cardano_buffer_get_data(ed25519_public_key->key_material);
}

size_t
cardano_ed25519_public_key_get_bytes_size(const cardano_ed25519_public_key_t* ed25519_public_key)
{
  if (ed25519_public_key == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_size(ed25519_public_key->key_material);
}

cardano_error_t
cardano_ed25519_public_key_to_bytes(
  const cardano_ed25519_public_key_t* public_key,
  byte_t*                             out_key_bytes,
  const size_t                        out_key_length)
{
  if (public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_copy_bytes(public_key->key_material, out_key_bytes, out_key_length);
}

size_t
cardano_ed25519_public_key_get_hex_size(const cardano_ed25519_public_key_t* ed25519_public_key)
{
  if (ed25519_public_key == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_hex_size(ed25519_public_key->key_material);
}

cardano_error_t
cardano_ed25519_public_key_to_hex(
  const cardano_ed25519_public_key_t* ed25519_public_key,
  char*                               hex,
  const size_t                        hex_length)
{
  if (ed25519_public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_to_hex(ed25519_public_key->key_material, hex, hex_length);
}

cardano_error_t
cardano_ed25519_public_key_to_hash(
  const cardano_ed25519_public_key_t* public_key,
  cardano_blake2b_hash_t**            hash)
{
  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (public_key == NULL)
  {
    *hash = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const byte_t* data = cardano_buffer_get_data(public_key->key_material);

  return cardano_blake2b_compute_hash(data, cardano_buffer_get_size(public_key->key_material), CARDANO_BLAKE2B_HASH_SIZE_224, hash);
}