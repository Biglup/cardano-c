/**
 * \file asset_id.c
 *
 * \author angel.castillo
 * \date   Sept 12, 2024
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

#include <cardano/assets/asset_id.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <sodium/core.h>
#include <sodium/utils.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an asset identifier in the Cardano blockchain.
 *
 * The `cardano_asset_id_t` structure encapsulates the unique identification of a native asset on the Cardano blockchain.
 * Each asset is uniquely identified by a combination of its policy ID and asset name.
 */
typedef struct cardano_asset_id_t
{
    cardano_object_t        base;
    cardano_blake2b_hash_t* policy_id;
    cardano_asset_name_t*   asset_name;
    bool                    is_lovelace;
    byte_t                  data[60];
    size_t                  size;
    char                    hex[121];
    size_t                  hex_size;
} cardano_asset_id_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a asset id object.
 *
 * This function is responsible for properly deallocating a asset id object (`cardano_asset_id_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the asset_id object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_asset_id_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the asset_id
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_asset_id_deallocate(void* object)
{
  assert(object != NULL);

  cardano_asset_id_t* asset_id = (cardano_asset_id_t*)object;

  cardano_blake2b_hash_unref(&asset_id->policy_id);
  cardano_asset_name_unref(&asset_id->asset_name);

  _cardano_free(object);
}

/**
 * \brief Converts a byte array to a hexadecimal string.
 *
 * @param data The data to be encoded in hexadecimal.
 * @param data_size The size of the data to be encoded.
 * @param hex_buffer The buffer where the hexadecimal string will be stored.
 * @param hex_buffer_size The size of the buffer where the hexadecimal string will be stored.
 *
 * @return A `cardano_error_t` value indicating the result of the operation.
 */
static cardano_error_t
to_hex_string(const byte_t* data, const size_t data_size, char* hex_buffer, const size_t hex_buffer_size)
{
  assert(data != NULL);
  assert(hex_buffer != NULL);
  assert(hex_buffer_size >= (data_size * 2U));
  assert(data_size > 0U);

  int init_result = sodium_init();

  if (init_result == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  if (sodium_bin2hex(hex_buffer, hex_buffer_size, data, data_size) < 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_asset_id_new(cardano_blake2b_hash_t* policy_id, cardano_asset_name_t* asset_name, cardano_asset_id_t** asset_id)
{
  if (policy_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cardano_blake2b_hash_get_bytes_size(policy_id) != 28U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_asset_id_t* new_asset_id = _cardano_malloc(sizeof(cardano_asset_id_t));

  if (new_asset_id == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  new_asset_id->base.deallocator   = cardano_asset_id_deallocate;
  new_asset_id->base.last_error[0] = '\0';
  new_asset_id->base.ref_count     = 1;

  cardano_blake2b_hash_ref(policy_id);
  cardano_asset_name_ref(asset_name);

  new_asset_id->policy_id   = policy_id;
  new_asset_id->asset_name  = asset_name;
  new_asset_id->is_lovelace = false;
  new_asset_id->size        = 0;
  new_asset_id->hex_size    = 0;

  CARDANO_UNUSED(memset(new_asset_id->data, 0, 60));
  cardano_safe_memcpy(new_asset_id->data, 28, cardano_blake2b_hash_get_data(policy_id), 28);
  cardano_safe_memcpy(&new_asset_id->data[28], 32U, cardano_asset_name_get_bytes(asset_name), cardano_asset_name_get_bytes_size(asset_name));
  new_asset_id->size = cardano_blake2b_hash_get_bytes_size(policy_id) + cardano_asset_name_get_bytes_size(asset_name);

  CARDANO_UNUSED(memset(new_asset_id->hex, 0, 121));

  cardano_error_t to_hex_result = to_hex_string(new_asset_id->data, new_asset_id->size, new_asset_id->hex, 121);

  if (to_hex_result != CARDANO_SUCCESS)
  {
    cardano_asset_id_unref(&new_asset_id);

    return to_hex_result;
  }

  new_asset_id->hex_size = cardano_safe_strlen(new_asset_id->hex, 121);

  *asset_id = new_asset_id;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_asset_id_new_lovelace(cardano_asset_id_t** asset_id)
{
  if (asset_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_id_t* new_asset_id = _cardano_malloc(sizeof(cardano_asset_id_t));

  if (new_asset_id == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  new_asset_id->base.deallocator   = cardano_asset_id_deallocate;
  new_asset_id->base.last_error[0] = '\0';
  new_asset_id->base.ref_count     = 1;
  new_asset_id->policy_id          = NULL;
  new_asset_id->asset_name         = NULL;
  new_asset_id->is_lovelace        = true;

  CARDANO_UNUSED(memset(new_asset_id->data, 0, 60));
  new_asset_id->size = 0;

  CARDANO_UNUSED(memset(new_asset_id->hex, 0, 121));
  new_asset_id->hex_size = 0;

  *asset_id = new_asset_id;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_asset_id_from_bytes(
  const byte_t*        data,
  size_t               size,
  cardano_asset_id_t** asset_id)
{
  if ((size > 0U) && (data == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < 28U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if (asset_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_t* policy_id  = NULL;
  cardano_asset_name_t*   asset_name = NULL;

  cardano_error_t policy_id_result = cardano_blake2b_hash_from_bytes(data, 28U, &policy_id);

  if (policy_id_result != CARDANO_SUCCESS)
  {
    return policy_id_result;
  }

  cardano_error_t asset_name_result = cardano_asset_name_from_bytes(&data[28], size - 28U, &asset_name);

  if (asset_name_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&policy_id);

    return asset_name_result;
  }

  cardano_error_t new_asset_id_result = cardano_asset_id_new(policy_id, asset_name, asset_id);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);

  if (new_asset_id_result != CARDANO_SUCCESS)
  {
    return new_asset_id_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_asset_id_from_hex(
  const char*          hex_string,
  size_t               size,
  cardano_asset_id_t** asset_id)
{
  if ((size > 0U) && (hex_string == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < 56U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if ((size % 2U) != 0U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (asset_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t data[60] = { 0 };

  int init_result = sodium_init();

  if (init_result == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  if (sodium_hex2bin(data, 60, hex_string, size, NULL, NULL, NULL) < 0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  return cardano_asset_id_from_bytes(data, size / 2U, asset_id);
}

const byte_t*
cardano_asset_id_get_bytes(const cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return NULL;
  }

  return asset_id->data;
}

size_t
cardano_asset_id_get_bytes_size(const cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return 0;
  }

  return asset_id->size;
}

const char*
cardano_asset_id_get_hex(const cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return NULL;
  }

  return asset_id->hex;
}

size_t
cardano_asset_id_get_hex_size(const cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return 0;
  }

  return asset_id->hex_size + 1U;
}

bool
cardano_asset_id_is_lovelace(const cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return false;
  }

  return asset_id->is_lovelace;
}

cardano_blake2b_hash_t*
cardano_asset_id_get_policy_id(cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(asset_id->policy_id);
  return asset_id->policy_id;
}

cardano_asset_name_t*
cardano_asset_id_get_asset_name(cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return NULL;
  }

  cardano_asset_name_ref(asset_id->asset_name);
  return asset_id->asset_name;
}

void
cardano_asset_id_unref(cardano_asset_id_t** asset_id)
{
  if ((asset_id == NULL) || (*asset_id == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*asset_id)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *asset_id = NULL;
    return;
  }
}

void
cardano_asset_id_ref(cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return;
  }

  cardano_object_ref(&asset_id->base);
}

size_t
cardano_asset_id_refcount(const cardano_asset_id_t* asset_id)
{
  if (asset_id == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&asset_id->base);
}

void
cardano_asset_id_set_last_error(cardano_asset_id_t* asset_id, const char* message)
{
  cardano_object_set_last_error(&asset_id->base, message);
}

const char*
cardano_asset_id_get_last_error(const cardano_asset_id_t* asset_id)
{
  return cardano_object_get_last_error(&asset_id->base);
}