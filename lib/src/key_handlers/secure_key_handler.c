/**
 * \file secure_key_handler.c
 *
 * \author angel.castillo
 * \date   Oct 06, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/key_handlers/secure_key_handler.h>
#include <cardano/key_handlers/secure_key_handler_impl.h>
#include <cardano/object.h>

#include "../allocators.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Opaque type definition for the secure key handler.
 *
 * `cardano_secure_key_handler_t` is an opaque structure representing a secure key handler. This handler abstracts the management of
 * cryptographic key operations for both BIP32 (HD) and Ed25519 key types, ensuring that sensitive key material is managed securely.
 * The internal details of this structure are hidden from users of the API, who interact with it through
 * the provided API functions.
 */
typedef struct cardano_secure_key_handler_t
{
    cardano_object_t                  base;
    cardano_secure_key_handler_impl_t impl;
} cardano_secure_key_handler_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a secure_key_handler object.
 *
 * This function is responsible for properly deallocating a secure_key_handler object (`cardano_secure_key_handler_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the secure_key_handler object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_secure_key_handler_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the secure_key_handler
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_secure_key_handler_deallocate(void* object)
{
  assert(object != NULL);

  cardano_secure_key_handler_t* secure_key_handler = (cardano_secure_key_handler_t*)object;

  cardano_object_unref(&secure_key_handler->impl.context);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_secure_key_handler_new(
  cardano_secure_key_handler_impl_t impl,
  cardano_secure_key_handler_t**    secure_key_handler)
{
  if (secure_key_handler == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *secure_key_handler = _cardano_malloc(sizeof(cardano_secure_key_handler_t));

  if (*secure_key_handler == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*secure_key_handler)->base.deallocator   = cardano_secure_key_handler_deallocate;
  (*secure_key_handler)->base.ref_count     = 1;
  (*secure_key_handler)->base.last_error[0] = '\0';

  (*secure_key_handler)->impl = impl;

  // Make sure secure_key_handler name is null terminated
  (*secure_key_handler)->impl.name[sizeof((*secure_key_handler)->impl.name) - 1U] = '\0';
  (*secure_key_handler)->impl.error_message[0]                                    = '\0';

  return CARDANO_SUCCESS;
}

const char*
cardano_secure_key_handler_get_name(const cardano_secure_key_handler_t* secure_key_handler)
{
  if (secure_key_handler == NULL)
  {
    return "";
  }

  return secure_key_handler->impl.name;
}

cardano_error_t
cardano_secure_key_handler_bip32_sign_transaction(
  cardano_secure_key_handler_t*    secure_key_handler,
  cardano_transaction_t*           tx,
  const cardano_derivation_path_t* derivation_paths,
  size_t                           num_paths,
  cardano_vkey_witness_set_t**     vkey_witness_set)
{
  if ((secure_key_handler == NULL) || (tx == NULL) || (derivation_paths == NULL) || (vkey_witness_set == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (secure_key_handler->impl.bip32_sign_transaction == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = secure_key_handler->impl.bip32_sign_transaction(&secure_key_handler->impl, tx, derivation_paths, num_paths, vkey_witness_set);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_set_last_error(secure_key_handler, secure_key_handler->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_secure_key_handler_bip32_get_extended_account_public_key(
  cardano_secure_key_handler_t*           secure_key_handler,
  const cardano_account_derivation_path_t derivation_path,
  cardano_bip32_public_key_t**            bip32_public_key)
{
  if ((secure_key_handler == NULL) || (bip32_public_key == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (secure_key_handler->impl.bip32_get_extended_account_public_key == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = secure_key_handler->impl.bip32_get_extended_account_public_key(&secure_key_handler->impl, derivation_path, bip32_public_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_set_last_error(secure_key_handler, secure_key_handler->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_secure_key_handler_ed25519_sign_transaction(
  cardano_secure_key_handler_t* secure_key_handler,
  cardano_transaction_t*        tx,
  cardano_vkey_witness_set_t**  vkey_witness_set)
{
  if ((secure_key_handler == NULL) || (tx == NULL) || (vkey_witness_set == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (secure_key_handler->impl.ed25519_sign_transaction == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = secure_key_handler->impl.ed25519_sign_transaction(&secure_key_handler->impl, tx, vkey_witness_set);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_set_last_error(secure_key_handler, secure_key_handler->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_secure_key_handler_ed25519_get_public_key(
  cardano_secure_key_handler_t*  secure_key_handler,
  cardano_ed25519_public_key_t** public_key)
{
  if ((secure_key_handler == NULL) || (public_key == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (secure_key_handler->impl.ed25519_get_public_key == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = secure_key_handler->impl.ed25519_get_public_key(&secure_key_handler->impl, public_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_set_last_error(secure_key_handler, secure_key_handler->impl.error_message);
  }

  return result;
}

CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_secure_key_handler_serialize(
  cardano_secure_key_handler_t* secure_key_handler,
  cardano_buffer_t**            serialized_data)
{
  if ((secure_key_handler == NULL) || (serialized_data == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (secure_key_handler->impl.serialize == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = secure_key_handler->impl.serialize(&secure_key_handler->impl, serialized_data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_set_last_error(secure_key_handler, secure_key_handler->impl.error_message);
  }

  return result;
}

void
cardano_secure_key_handler_unref(cardano_secure_key_handler_t** secure_key_handler)
{
  if ((secure_key_handler == NULL) || (*secure_key_handler == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*secure_key_handler)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *secure_key_handler = NULL;
    return;
  }
}

void
cardano_secure_key_handler_ref(cardano_secure_key_handler_t* secure_key_handler)
{
  if (secure_key_handler == NULL)
  {
    return;
  }

  cardano_object_ref(&secure_key_handler->base);
}

size_t
cardano_secure_key_handler_refcount(const cardano_secure_key_handler_t* secure_key_handler)
{
  if (secure_key_handler == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&secure_key_handler->base);
}

void
cardano_secure_key_handler_set_last_error(cardano_secure_key_handler_t* secure_key_handler, const char* message)
{
  cardano_object_set_last_error(&secure_key_handler->base, message);
}

const char*
cardano_secure_key_handler_get_last_error(const cardano_secure_key_handler_t* secure_key_handler)
{
  return cardano_object_get_last_error(&secure_key_handler->base);
}