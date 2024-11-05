/**
 * \file bip32_public_key.c
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

#include <cardano/crypto/bip32_public_key.h>

#include <cardano/buffer.h>
#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "bip32_key_derivation.h"

#include <assert.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t BIP32_ED25519_PUBLIC_KEY_LENGTH = 64;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a BIP32 hierarchical deterministic public key.
 *
 * This structure encapsulates a BIP32 public key, part of the hierarchical deterministic (HD) wallet
 * architecture. HD wallets allow for the generation of a family of public keys from a single master
 * seed.
 */
typedef struct cardano_bip32_public_key_t
{
    cardano_object_t  base;
    cardano_buffer_t* key_material;
} cardano_bip32_public_key_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a BIP32 public key object.
 *
 * This function is responsible for properly deallocating a BIP32 public key object (`cardano_bip32_public_key_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the BIP32 public key object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_bip32_public_key_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the BIP32 public key
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_bip32_public_key_deallocate(void* object)
{
  assert(object != NULL);

  cardano_bip32_public_key_t* public_key = (cardano_bip32_public_key_t*)object;

  if (public_key->key_material != NULL)
  {
    cardano_buffer_unref(&public_key->key_material);
  }

  _cardano_free(public_key);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_bip32_public_key_from_bytes(
  const byte_t*                data,
  const size_t                 data_length,
  cardano_bip32_public_key_t** public_key)
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

  if (data_length != BIP32_ED25519_PUBLIC_KEY_LENGTH)
  {
    *public_key = NULL;
    return CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE;
  }

  cardano_bip32_public_key_t* bip32_public_key = (cardano_bip32_public_key_t*)_cardano_malloc(sizeof(cardano_bip32_public_key_t));

  if (bip32_public_key == NULL)
  {
    *public_key = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  bip32_public_key->base.ref_count     = 1;
  bip32_public_key->base.deallocator   = cardano_bip32_public_key_deallocate;
  bip32_public_key->base.last_error[0] = '\0';
  bip32_public_key->key_material       = cardano_buffer_new_from(data, data_length);

  if (bip32_public_key->key_material == NULL)
  {
    _cardano_free(bip32_public_key);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *public_key = bip32_public_key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bip32_public_key_from_hex(
  const char*                  hex,
  const size_t                 hex_length,
  cardano_bip32_public_key_t** public_key)
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

  if (hex_length != (BIP32_ED25519_PUBLIC_KEY_LENGTH * 2U))
  {
    *public_key = NULL;
    return CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE;
  }

  cardano_bip32_public_key_t* bip32_public_key = (cardano_bip32_public_key_t*)_cardano_malloc(sizeof(cardano_bip32_public_key_t));

  if (bip32_public_key == NULL)
  {
    *public_key = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  bip32_public_key->base.ref_count     = 1;
  bip32_public_key->base.deallocator   = cardano_bip32_public_key_deallocate;
  bip32_public_key->base.last_error[0] = '\0';
  bip32_public_key->key_material       = cardano_buffer_from_hex(hex, hex_length);

  if (bip32_public_key->key_material == NULL)
  {
    _cardano_free(bip32_public_key);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *public_key = bip32_public_key;

  return CARDANO_SUCCESS;
}

void
cardano_bip32_public_key_unref(cardano_bip32_public_key_t** bip32_public_key)
{
  if ((bip32_public_key == NULL) || (*bip32_public_key == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*bip32_public_key)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *bip32_public_key = NULL;
    return;
  }
}

void
cardano_bip32_public_key_ref(cardano_bip32_public_key_t* bip32_public_key)
{
  if (bip32_public_key == NULL)
  {
    return;
  }

  cardano_object_ref(&bip32_public_key->base);
}

size_t
cardano_bip32_public_key_refcount(const cardano_bip32_public_key_t* bip32_public_key)
{
  if (bip32_public_key == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&bip32_public_key->base);
}

cardano_error_t
cardano_bip32_public_key_derive(
  const cardano_bip32_public_key_t* bip32_public_key,
  const uint32_t*                   indices,
  const size_t                      indices_count,
  cardano_bip32_public_key_t**      derived_bip32_public_key)
{
  if (derived_bip32_public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bip32_public_key == NULL)
  {
    *derived_bip32_public_key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (indices == NULL)
  {
    *derived_bip32_public_key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (indices_count == 0U)
  {
    *derived_bip32_public_key = NULL;

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_bip32_public_key_t* working_key      = NULL;
  cardano_error_t             clone_key_result = cardano_bip32_public_key_from_bytes(
    cardano_buffer_get_data(bip32_public_key->key_material),
    BIP32_ED25519_PUBLIC_KEY_LENGTH,
    &working_key);

  if (clone_key_result != CARDANO_SUCCESS)
  {
    *derived_bip32_public_key = NULL;

    return clone_key_result;
  }

  for (size_t i = 0; i < indices_count; ++i)
  {
    if (_cardano_crypto_is_hardened_derivation(indices[i]))
    {
      *derived_bip32_public_key = NULL;
      cardano_bip32_public_key_unref(&working_key);

      return CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX;
    }

    const byte_t*   key_data         = cardano_buffer_get_data(working_key->key_material);
    byte_t          derived_key[64U] = { 0 };
    cardano_error_t error            = _cardano_crypto_derive_public(key_data, (int32_t)indices[i], &derived_key[0], 64U);

    assert(error == CARDANO_SUCCESS);
    CARDANO_UNUSED(error);

    cardano_bip32_public_key_t* new_key = NULL;

    error = cardano_bip32_public_key_from_bytes(&derived_key[0], BIP32_ED25519_PUBLIC_KEY_LENGTH, &new_key);

    if (error != CARDANO_SUCCESS)
    {
      *derived_bip32_public_key = NULL;
      cardano_bip32_public_key_unref(&working_key);

      return error;
    }

    cardano_bip32_public_key_unref(&working_key);
    working_key = new_key;
  }

  *derived_bip32_public_key = working_key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bip32_public_key_to_ed25519_key(
  const cardano_bip32_public_key_t* public_key,
  cardano_ed25519_public_key_t**    ed25519_public_key)
{
  static const size_t ed25519_public_key_length = 32;

  if (ed25519_public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (public_key == NULL)
  {
    *ed25519_public_key = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const byte_t* data = cardano_buffer_get_data(public_key->key_material);

  return cardano_ed25519_public_key_from_bytes(data, ed25519_public_key_length, ed25519_public_key);
}

const byte_t*
cardano_bip32_public_key_get_data(const cardano_bip32_public_key_t* bip32_public_key)
{
  if (bip32_public_key == NULL)
  {
    return NULL;
  }

  return cardano_buffer_get_data(bip32_public_key->key_material);
}

size_t
cardano_bip32_public_key_get_bytes_size(const cardano_bip32_public_key_t* bip32_public_key)
{
  if (bip32_public_key == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_size(bip32_public_key->key_material);
}

cardano_error_t
cardano_bip32_public_key_to_bytes(
  const cardano_bip32_public_key_t* public_key,
  byte_t*                           out_key_bytes,
  const size_t                      out_key_length)
{
  if (public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_copy_bytes(public_key->key_material, out_key_bytes, out_key_length);
}

size_t
cardano_bip32_public_key_get_hex_size(const cardano_bip32_public_key_t* bip32_public_key)
{
  if (bip32_public_key == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_hex_size(bip32_public_key->key_material);
}

cardano_error_t
cardano_bip32_public_key_to_hex(
  const cardano_bip32_public_key_t* bip32_public_key,
  char*                             hex,
  const size_t                      hex_length)
{
  if (bip32_public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_to_hex(bip32_public_key->key_material, hex, hex_length);
}

cardano_error_t
cardano_bip32_public_key_to_hash(
  const cardano_bip32_public_key_t* bip32_public_key,
  cardano_blake2b_hash_t**          hash)
{
  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bip32_public_key == NULL)
  {
    *hash = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const byte_t* data = cardano_buffer_get_data(bip32_public_key->key_material);

  return cardano_blake2b_compute_hash(data, cardano_buffer_get_size(bip32_public_key->key_material), CARDANO_BLAKE2B_HASH_SIZE_224, hash);
}