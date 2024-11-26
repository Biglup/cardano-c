/**
 * \file bip32_private_key.c
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

#include <cardano/crypto/bip32_private_key.h>

#include <cardano/buffer.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/crypto/pbkdf2.h>
#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"
#include "bip32_key_derivation.h"

#include <assert.h>
#include <sodium.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t BIP32_ED25519_PRIVATE_KEY_LENGTH = 96;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a BIP32 hierarchical deterministic private key.
 *
 * This structure encapsulates a BIP32 private key, part of the hierarchical deterministic (HD) wallet
 * architecture. HD wallets allow for the generation of a family of private keys from a single master
 * seed.
 */
typedef struct cardano_bip32_private_key_t
{
    cardano_object_t  base;
    cardano_buffer_t* key_material;
} cardano_bip32_private_key_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a BIP32 private key object.
 *
 * This function is responsible for properly deallocating a BIP32 private key object (`cardano_bip32_private_key_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the BIP32 private key object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_bip32_private_key_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the BIP32 private key
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_bip32_private_key_deallocate(void* object)
{
  assert(object != NULL);

  cardano_bip32_private_key_t* private_key = (cardano_bip32_private_key_t*)object;

  if (private_key->key_material != NULL)
  {
    sodium_memzero(cardano_buffer_get_data(private_key->key_material), cardano_buffer_get_size(private_key->key_material));
    cardano_buffer_unref(&private_key->key_material);
  }

  _cardano_free(private_key);
}

/**
 * \brief clamp the scalar by:
 *
 *  1. Clearing the 3 lower bits.
 *  2. Clearing the three highest bits.
 *  3. Setting the second-highest bit.
 *
 * \param[out] scalar The clamped scalar.
 */
static void
clamp_scalar(byte_t* scalar)
{
  assert(scalar != NULL);

  scalar[0]  &= 0b11111000u;
  scalar[31] &= 0b00011111u;
  scalar[31] |= 0b01000000u;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_bip32_private_key_from_bytes(
  const byte_t*                 key_bytes,
  const size_t                  key_length,
  cardano_bip32_private_key_t** private_key)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key_bytes == NULL)
  {
    *private_key = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key_length != BIP32_ED25519_PRIVATE_KEY_LENGTH)
  {
    *private_key = NULL;
    return CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE;
  }

  cardano_bip32_private_key_t* bip32_private_key = (cardano_bip32_private_key_t*)_cardano_malloc(sizeof(cardano_bip32_private_key_t));

  if (bip32_private_key == NULL)
  {
    *private_key = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  bip32_private_key->base.ref_count     = 1;
  bip32_private_key->base.deallocator   = cardano_bip32_private_key_deallocate;
  bip32_private_key->base.last_error[0] = '\0';
  bip32_private_key->key_material       = cardano_buffer_new_from(key_bytes, key_length);

  if (bip32_private_key->key_material == NULL)
  {
    _cardano_free(bip32_private_key);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *private_key = bip32_private_key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bip32_private_key_from_bip39_entropy(
  const byte_t*                 password,
  const size_t                  password_length,
  const byte_t*                 entropy,
  const size_t                  entropy_length,
  cardano_bip32_private_key_t** key)
{
  static const size_t pbkdf2_iterations = 4096;
  static const size_t pbkdf2_key_size   = 96;

  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((password == NULL) && (password_length > 0U))
  {
    *key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (entropy == NULL)
  {
    *key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (entropy_length == 0U)
  {
    *key = NULL;

    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  byte_t pbkdf2_key[96] = { 0 };

  cardano_error_t result = cardano_crypto_pbkdf2_hmac_sha512(
    password,
    password_length,
    entropy,
    entropy_length,
    (uint32_t)pbkdf2_iterations,
    &pbkdf2_key[0],
    pbkdf2_key_size);

  if (result != CARDANO_SUCCESS)
  {
    *key = NULL;

    return result;
  }

  clamp_scalar(&pbkdf2_key[0]);

  return cardano_bip32_private_key_from_bytes(&pbkdf2_key[0], BIP32_ED25519_PRIVATE_KEY_LENGTH, key);
}

cardano_error_t
cardano_bip32_private_key_from_hex(
  const char*                   hex,
  const size_t                  hex_length,
  cardano_bip32_private_key_t** private_key)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *private_key = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex_length != (BIP32_ED25519_PRIVATE_KEY_LENGTH * 2U))
  {
    *private_key = NULL;
    return CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE;
  }

  cardano_bip32_private_key_t* bip32_private_key = (cardano_bip32_private_key_t*)_cardano_malloc(sizeof(cardano_bip32_private_key_t));

  if (bip32_private_key == NULL)
  {
    *private_key = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  bip32_private_key->base.ref_count     = 1;
  bip32_private_key->base.deallocator   = cardano_bip32_private_key_deallocate;
  bip32_private_key->base.last_error[0] = '\0';
  bip32_private_key->key_material       = cardano_buffer_from_hex(hex, hex_length);

  if (bip32_private_key->key_material == NULL)
  {
    _cardano_free(bip32_private_key);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *private_key = bip32_private_key;

  return CARDANO_SUCCESS;
}

void
cardano_bip32_private_key_unref(cardano_bip32_private_key_t** bip32_private_key)
{
  if ((bip32_private_key == NULL) || (*bip32_private_key == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*bip32_private_key)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *bip32_private_key = NULL;
    return;
  }
}

void
cardano_bip32_private_key_ref(cardano_bip32_private_key_t* bip32_private_key)
{
  if (bip32_private_key == NULL)
  {
    return;
  }

  cardano_object_ref(&bip32_private_key->base);
}

size_t
cardano_bip32_private_key_refcount(const cardano_bip32_private_key_t* bip32_private_key)
{
  if (bip32_private_key == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&bip32_private_key->base);
}

cardano_error_t
cardano_bip32_private_key_derive(
  const cardano_bip32_private_key_t* private_key,
  const uint32_t*                    indices,
  size_t                             indices_count,
  cardano_bip32_private_key_t**      derived_private_key)
{
  if (derived_private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (private_key == NULL)
  {
    *derived_private_key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (indices == NULL)
  {
    *derived_private_key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (indices_count == 0U)
  {
    *derived_private_key = NULL;

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_bip32_private_key_t* working_key      = NULL;
  cardano_error_t              clone_key_result = cardano_bip32_private_key_from_bytes(
    cardano_buffer_get_data(private_key->key_material),
    BIP32_ED25519_PRIVATE_KEY_LENGTH,
    &working_key);

  if (clone_key_result != CARDANO_SUCCESS)
  {
    *derived_private_key = NULL;

    return clone_key_result;
  }

  for (size_t i = 0; i < indices_count; ++i)
  {
    const byte_t*   key_data         = cardano_buffer_get_data(working_key->key_material);
    byte_t          derived_key[96U] = { 0 };
    cardano_error_t error            = _cardano_crypto_derive_private(key_data, (int32_t)indices[i], &derived_key[0], sizeof(derived_key));

    assert(error == CARDANO_SUCCESS);
    CARDANO_UNUSED(error);

    cardano_bip32_private_key_t* new_key = NULL;

    error = cardano_bip32_private_key_from_bytes(&derived_key[0], BIP32_ED25519_PRIVATE_KEY_LENGTH, &new_key);

    if (error != CARDANO_SUCCESS)
    {
      *derived_private_key = NULL;
      cardano_bip32_private_key_unref(&working_key);

      return error;
    }

    cardano_bip32_private_key_unref(&working_key);
    working_key = new_key;
  }

  *derived_private_key = working_key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bip32_private_key_to_ed25519_key(
  const cardano_bip32_private_key_t* private_key,
  cardano_ed25519_private_key_t**    ed25519_private_key)
{
  static const size_t extended_ed25519_private_key_length = 64;

  if (ed25519_private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (private_key == NULL)
  {
    *ed25519_private_key = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const byte_t* data = cardano_buffer_get_data(private_key->key_material);

  return cardano_ed25519_private_key_from_extended_bytes(
    data,
    extended_ed25519_private_key_length,
    ed25519_private_key);
}

cardano_error_t
cardano_bip32_private_key_get_public_key(
  const cardano_bip32_private_key_t* private_key,
  cardano_bip32_public_key_t**       ed25519_public_key)
{
  static const size_t chain_code_index                = 64;
  static const size_t chain_code_size                 = 32;
  static const size_t bip32_ed25519_public_key_length = 64;

  if (ed25519_public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (private_key == NULL)
  {
    *ed25519_public_key = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const byte_t* extended_scalar = cardano_buffer_get_data(private_key->key_material);
  byte_t        public_key[32]  = { 0 };

  if (crypto_scalarmult_ed25519_base_noclamp(&public_key[0], extended_scalar) != 0)
  {
    *ed25519_public_key = NULL;
    return CARDANO_ERROR_GENERIC;
  }

  byte_t bip32_public_key[64] = { 0 };

  cardano_safe_memcpy(&bip32_public_key[0], sizeof(bip32_public_key), &public_key[0], 32);
  cardano_safe_memcpy(&bip32_public_key[32], sizeof(bip32_public_key) - 32U, &extended_scalar[chain_code_index], chain_code_size);

  return cardano_bip32_public_key_from_bytes(
    &bip32_public_key[0],
    bip32_ed25519_public_key_length,
    ed25519_public_key);
}

const byte_t*
cardano_bip32_private_key_get_data(const cardano_bip32_private_key_t* bip32_private_key)
{
  if (bip32_private_key == NULL)
  {
    return NULL;
  }

  return cardano_buffer_get_data(bip32_private_key->key_material);
}

size_t
cardano_bip32_private_key_get_bytes_size(const cardano_bip32_private_key_t* bip32_private_key)
{
  if (bip32_private_key == NULL)
  {
    return 0U;
  }

  return cardano_buffer_get_size(bip32_private_key->key_material);
}

cardano_error_t
cardano_bip32_private_key_to_bytes(
  const cardano_bip32_private_key_t* private_key,
  byte_t*                            out_key_bytes,
  const size_t                       out_key_length)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_copy_bytes(private_key->key_material, out_key_bytes, out_key_length);
}

size_t
cardano_bip32_private_key_get_hex_size(const cardano_bip32_private_key_t* bip32_private_key)
{
  if (bip32_private_key == NULL)
  {
    return 0U;
  }

  return cardano_buffer_get_hex_size(bip32_private_key->key_material);
}

cardano_error_t
cardano_bip32_private_key_to_hex(
  const cardano_bip32_private_key_t* bip32_private_key,
  char*                              hex,
  const size_t                       hex_length)
{
  if (bip32_private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_to_hex(bip32_private_key->key_material, hex, hex_length);
}

uint32_t
cardano_bip32_harden(uint32_t index)
{
  return index | 0x80000000;
}