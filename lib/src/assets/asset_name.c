/**
 * \file asset_name.c
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

#include <cardano/assets/asset_name.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <sodium/core.h>
#include <sodium/utils.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an asset name.
 */
typedef struct cardano_asset_name_t
{
    cardano_object_t base;
    byte_t           data[32];
    uint64_t         size;
    char             hex[65];
    uint64_t         hex_size;
} cardano_asset_name_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a asset name object.
 *
 * This function is responsible for properly deallocating a asset name object (`cardano_asset_name_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the asset_name object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_asset_name_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the asset_name
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_asset_name_deallocate(void* object)
{
  assert(object != NULL);
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
  assert(hex_buffer != NULL);
  assert(hex_buffer_size >= (data_size * 2U));

  if ((data_size == 0U) || (data == NULL))
  {
    return CARDANO_SUCCESS;
  }

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
cardano_asset_name_from_bytes(
  const byte_t*          data,
  size_t                 size,
  cardano_asset_name_t** asset_name)
{
  if ((size > 0U) && (data == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size > 32U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (asset_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_name_t* new_asset_name = _cardano_malloc(sizeof(cardano_asset_name_t));

  if (new_asset_name == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  new_asset_name->base.deallocator   = cardano_asset_name_deallocate;
  new_asset_name->base.last_error[0] = '\0';
  new_asset_name->base.ref_count     = 1;

  CARDANO_UNUSED(memset(new_asset_name->data, 0, 32));
  cardano_safe_memcpy(new_asset_name->data, 32, data, size);
  new_asset_name->size = size;

  CARDANO_UNUSED(memset(new_asset_name->hex, 0, 65));
  cardano_error_t to_hex_result = to_hex_string(data, size, new_asset_name->hex, 65);

  if (to_hex_result != CARDANO_SUCCESS)
  {
    _cardano_free(new_asset_name);

    return to_hex_result;
  }

  new_asset_name->hex_size = size * 2U;
  *asset_name              = new_asset_name;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_asset_name_from_hex(
  const char*            hex,
  size_t                 size,
  cardano_asset_name_t** asset_name)
{
  if ((size > 0U) && (hex == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (((size % 2U) != 0U) || (size > 64U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  byte_t data[32] = { 0 };

  int init_result = sodium_init();

  if (init_result == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  if (sodium_hex2bin(data, 32, hex, size, NULL, NULL, NULL) < 0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  return cardano_asset_name_from_bytes(data, size / 2U, asset_name);
}

cardano_error_t
cardano_asset_name_from_string(
  const char*            string,
  size_t                 size,
  cardano_asset_name_t** asset_name)
{
  if (string == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_asset_name_from_bytes((const byte_t*)string, size, asset_name);
}

cardano_error_t
cardano_asset_name_from_cbor(cardano_cbor_reader_t* reader, cardano_asset_name_t** asset_name)
{
  if (asset_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *asset_name = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t* buffer          = NULL;
  cardano_error_t   validate_result = cardano_cbor_reader_read_bytestring(reader, &buffer);

  if (validate_result != CARDANO_SUCCESS)
  {
    *asset_name = NULL;
    return validate_result;
  }

  cardano_error_t from_bytes_result = cardano_asset_name_from_bytes(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), asset_name);
  cardano_buffer_unref(&buffer);

  return from_bytes_result;
}

cardano_error_t
cardano_asset_name_to_cbor(const cardano_asset_name_t* asset_name, cardano_cbor_writer_t* writer)
{
  if (asset_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_result = cardano_cbor_writer_write_bytestring(writer, asset_name->data, asset_name->size);

  return write_result;
}

const char*
cardano_asset_name_get_string(const cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return NULL;
  }

  return (const char*)(asset_name)->data;
}

size_t
cardano_asset_name_get_string_size(const cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return 0;
  }

  return asset_name->size + 1U;
}

const byte_t*
cardano_asset_name_get_bytes(const cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return NULL;
  }

  return asset_name->data;
}

size_t
cardano_asset_name_get_bytes_size(const cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return 0;
  }

  return asset_name->size;
}

const char*
cardano_asset_name_get_hex(const cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return NULL;
  }

  return asset_name->hex;
}

size_t
cardano_asset_name_get_hex_size(const cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return 0;
  }

  return asset_name->hex_size + 1U;
}

void
cardano_asset_name_unref(cardano_asset_name_t** asset_name)
{
  if ((asset_name == NULL) || (*asset_name == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*asset_name)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *asset_name = NULL;
    return;
  }
}

void
cardano_asset_name_ref(cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return;
  }

  cardano_object_ref(&asset_name->base);
}

size_t
cardano_asset_name_refcount(const cardano_asset_name_t* asset_name)
{
  if (asset_name == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&asset_name->base);
}

void
cardano_asset_name_set_last_error(cardano_asset_name_t* asset_name, const char* message)
{
  cardano_object_set_last_error(&asset_name->base, message);
}

const char*
cardano_asset_name_get_last_error(const cardano_asset_name_t* asset_name)
{
  return cardano_object_get_last_error(&asset_name->base);
}