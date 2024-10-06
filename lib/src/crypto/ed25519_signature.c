/**
 * \file ed25519_signature.c
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

#include <cardano/crypto/ed25519_signature.h>

#include <cardano/buffer.h>
#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"

#include <assert.h>
#include <sodium.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an Ed25519 signature.
 *
 * This structure encapsulates an Ed25519 signature, which is a digital signature algorithm
 * used for cryptographic verification in the Cardano blockchain ecosystem.
 */
typedef struct cardano_ed25519_signature_t
{
    cardano_object_t  base;
    cardano_buffer_t* buffer;
} cardano_ed25519_signature_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a Ed25519 signature object.
 *
 * This function is responsible for properly deallocating a Ed25519 signature object (`cardano_ed25519_signature_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the Ed25519 signature object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_ed25519_signature_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the Ed25519 signature
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_ed25519_signature_deallocate(void* object)
{
  assert(object != NULL);

  cardano_ed25519_signature_t* signature = (cardano_ed25519_signature_t*)object;

  if (signature->buffer != NULL)
  {
    cardano_buffer_unref(&signature->buffer);
  }

  _cardano_free(signature);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_ed25519_signature_from_bytes(
  const byte_t*                 data,
  const size_t                  data_length,
  cardano_ed25519_signature_t** signature)
{
  if (signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *signature = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_length != crypto_sign_BYTES)
  {
    *signature = NULL;
    return CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE;
  }

  cardano_ed25519_signature_t* ed25519_signature = (cardano_ed25519_signature_t*)_cardano_malloc(sizeof(cardano_ed25519_signature_t));

  if (ed25519_signature == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  ed25519_signature->base.ref_count     = 1;
  ed25519_signature->base.deallocator   = cardano_ed25519_signature_deallocate;
  ed25519_signature->base.last_error[0] = '\0';
  ed25519_signature->buffer             = cardano_buffer_new_from(data, data_length);

  if (ed25519_signature->buffer == NULL)
  {
    _cardano_free(ed25519_signature);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *signature = ed25519_signature;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ed25519_signature_from_hex(
  const char*                   hex,
  const size_t                  hex_length,
  cardano_ed25519_signature_t** signature)
{
  if (signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *signature = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex_length != (crypto_sign_BYTES * 2))
  {
    *signature = NULL;

    return CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE;
  }

  cardano_ed25519_signature_t* ed25519_signature = (cardano_ed25519_signature_t*)_cardano_malloc(sizeof(cardano_ed25519_signature_t));

  if (ed25519_signature == NULL)
  {
    *signature = NULL;

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  ed25519_signature->base.ref_count     = 1;
  ed25519_signature->base.deallocator   = cardano_ed25519_signature_deallocate;
  ed25519_signature->base.last_error[0] = '\0';
  ed25519_signature->buffer             = cardano_buffer_from_hex(hex, hex_length);

  if (ed25519_signature->buffer == NULL)
  {
    *signature = NULL;
    _cardano_free(ed25519_signature);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *signature = ed25519_signature;

  return CARDANO_SUCCESS;
}

void
cardano_ed25519_signature_unref(cardano_ed25519_signature_t** ed25519_signature)
{
  if ((ed25519_signature == NULL) || (*ed25519_signature == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*ed25519_signature)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *ed25519_signature = NULL;
    return;
  }
}

void
cardano_ed25519_signature_ref(cardano_ed25519_signature_t* ed25519_signature)
{
  if (ed25519_signature == NULL)
  {
    return;
  }

  cardano_object_ref(&ed25519_signature->base);
}

size_t
cardano_ed25519_signature_refcount(const cardano_ed25519_signature_t* ed25519_signature)
{
  if (ed25519_signature == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&ed25519_signature->base);
}

const byte_t*
cardano_ed25519_signature_get_data(const cardano_ed25519_signature_t* ed25519_signature)
{
  if (ed25519_signature == NULL)
  {
    return NULL;
  }

  return cardano_buffer_get_data(ed25519_signature->buffer);
}

size_t
cardano_ed25519_signature_get_bytes_size(const cardano_ed25519_signature_t* ed25519_signature)
{
  if (ed25519_signature == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_size(ed25519_signature->buffer);
}

cardano_error_t
cardano_ed25519_signature_to_bytes(
  const cardano_ed25519_signature_t* ed25519_signature,
  byte_t*                            signature,
  const size_t                       signature_length)
{
  if (ed25519_signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_copy_bytes(ed25519_signature->buffer, signature, signature_length);
}

size_t
cardano_ed25519_signature_get_hex_size(const cardano_ed25519_signature_t* ed25519_signature)
{
  if (ed25519_signature == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_hex_size(ed25519_signature->buffer);
}

cardano_error_t
cardano_ed25519_signature_to_hex(
  const cardano_ed25519_signature_t* ed25519_signature,
  char*                              hex,
  const size_t                       hex_length)
{
  if (ed25519_signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_to_hex(ed25519_signature->buffer, hex, hex_length);
}