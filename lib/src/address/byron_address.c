/**
 * \file byron_address.c
 *
 * \author angel.castillo
 * \date   Apr 15, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include <cardano/address/address.h>
#include <cardano/address/byron_address.h>
#include <cardano/encoding/base58.h>
#include <cardano/error.h>

#include "../allocators.h"
#include "../string_safe.h"
#include "./internals/addr_common.h"
#include "./internals/byron_addr_pack.h"

#include <assert.h>
#include <string.h>

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_byron_address_from_credentials(
  cardano_blake2b_hash_t*                  root,
  const cardano_byron_address_attributes_t attributes,
  const cardano_byron_address_type_t       type,
  cardano_byron_address_t**                byron_address)
{
  if (root == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (byron_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* address = _cardano_malloc(sizeof(cardano_address_t));

  if (address == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  address->byron_content = _cardano_malloc(sizeof(cardano_byron_address_content_t));

  if (address->byron_content == NULL)
  {
    _cardano_free(address);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  address->base.ref_count            = 1;
  address->base.last_error[0]        = '\0';
  address->base.deallocator          = _cardano_address_deallocate;
  address->type                      = CARDANO_ADDRESS_TYPE_BYRON;
  address->network_id                = NULL;
  address->payment_credential        = NULL;
  address->stake_credential          = NULL;
  address->stake_pointer             = NULL;
  address->address_data_size         = 0;
  address->byron_content->attributes = attributes;
  address->byron_content->type       = type;

  cardano_safe_memcpy(
    address->byron_content->root,
    sizeof(address->byron_content->root),
    cardano_blake2b_hash_get_data(root),
    sizeof(address->byron_content->root));

  const cardano_error_t packing_result = _cardano_pack_byron_address(address, address->address_data, sizeof(address->address_data), &address->address_data_size);

  if (packing_result != CARDANO_SUCCESS)
  {
    _cardano_address_deallocate(address);
    return packing_result;
  }

  const cardano_error_t encoding_result = cardano_encoding_base58_encode(address->address_data, address->address_data_size, address->address_str, sizeof(address->address_str));

  if (encoding_result != CARDANO_SUCCESS)
  {
    _cardano_address_deallocate(&address);
    return encoding_result;
  }

  *byron_address = _cardano_from_address_to_byron(address);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_byron_address_from_address(
  const cardano_address_t*  address,
  cardano_byron_address_t** byron_address)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (byron_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (address->type != CARDANO_ADDRESS_TYPE_BYRON)
  {
    return CARDANO_ERROR_INVALID_ADDRESS_TYPE;
  }

  cardano_address_t*    address_copy = NULL;
  const cardano_error_t copy_result  = cardano_address_from_bytes(address->address_data, address->address_data_size, &address_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    return copy_result;
  }

  *byron_address = _cardano_from_address_to_byron(address_copy);

  return CARDANO_SUCCESS;
}

cardano_address_t*
cardano_byron_address_to_address(const cardano_byron_address_t* byron_address)
{
  if (byron_address == NULL)
  {
    return NULL;
  }

  const cardano_address_t* address = _cardano_from_byron_to_address_const(byron_address);

  cardano_address_t*    address_copy = NULL;
  const cardano_error_t copy_result  = cardano_address_from_bytes(address->address_data, address->address_data_size, &address_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return address_copy;
}

cardano_error_t
cardano_byron_address_get_attributes(
  const cardano_byron_address_t*      address,
  cardano_byron_address_attributes_t* attributes)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const cardano_address_t* addr = _cardano_from_byron_to_address_const(address);

  assert(addr->type == CARDANO_ADDRESS_TYPE_BYRON);
  assert(addr->byron_content != NULL);

  *attributes = addr->byron_content->attributes;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_byron_address_get_type(
  const cardano_byron_address_t* address,
  cardano_byron_address_type_t*  type)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const cardano_address_t* addr = _cardano_from_byron_to_address_const(address);

  assert(addr->type == CARDANO_ADDRESS_TYPE_BYRON);
  assert(addr->byron_content != NULL);

  *type = addr->byron_content->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_byron_address_get_root(
  const cardano_byron_address_t* address,
  cardano_blake2b_hash_t**       root)
{
  if (address == NULL)
  {
    *root = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const cardano_address_t* addr = _cardano_from_byron_to_address_const(address);

  assert(addr->type == CARDANO_ADDRESS_TYPE_BYRON);
  assert(addr->byron_content != NULL);

  const cardano_error_t error = cardano_blake2b_hash_from_bytes(addr->byron_content->root, sizeof(addr->byron_content->root), root);

  if (error != CARDANO_SUCCESS)
  {
    *root = NULL;
    return error;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_byron_address_from_bytes(
  const byte_t*             data,
  size_t                    size,
  cardano_byron_address_t** address)
{
  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return _cardano_unpack_byron_address(data, size, address);
}

size_t
cardano_byron_address_get_bytes_size(const cardano_byron_address_t* address)
{
  return cardano_address_get_bytes_size(_cardano_from_byron_to_address_const(address));
}

const byte_t*
cardano_byron_address_get_bytes(const cardano_byron_address_t* address)
{
  return cardano_address_get_bytes(_cardano_from_byron_to_address_const(address));
}

cardano_error_t
cardano_byron_address_to_bytes(
  const cardano_byron_address_t* address,
  byte_t*                        data,
  size_t                         size)
{
  return cardano_address_to_bytes(_cardano_from_byron_to_address_const(address), data, size);
}

cardano_error_t
cardano_byron_address_from_base58(
  const char*               data,
  size_t                    size,
  cardano_byron_address_t** address)
{
  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size == 0U)
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t data_size   = cardano_encoding_base58_get_decoded_length(data, size);
  byte_t*      data_buffer = _cardano_malloc(data_size);

  if (data_buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const cardano_error_t unpack_result = cardano_encoding_base58_decode(data, size, data_buffer, data_size);

  if (unpack_result != CARDANO_SUCCESS)
  {
    _cardano_free(data_buffer);
    return unpack_result;
  }

  const cardano_error_t result = cardano_byron_address_from_bytes(data_buffer, data_size, address);

  _cardano_free(data_buffer);

  return result;
}

size_t
cardano_byron_address_get_base58_size(const cardano_byron_address_t* address)
{
  return cardano_address_get_string_size(_cardano_from_byron_to_address_const(address));
}

cardano_error_t
cardano_byron_address_to_base58(
  const cardano_byron_address_t* address,
  char*                          data,
  size_t                         size)
{
  const char*  base58      = cardano_byron_address_get_string(address);
  const size_t base58_size = cardano_byron_address_get_base58_size(address);

  if (size < base58_size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(data, size, base58, base58_size);

  return CARDANO_SUCCESS;
}

const char*
cardano_byron_address_get_string(const cardano_byron_address_t* address)
{
  return cardano_address_get_string(_cardano_from_byron_to_address_const(address));
}

cardano_error_t
cardano_byron_address_get_network_id(const cardano_byron_address_t* address, cardano_network_id_t* network_id)
{
  return cardano_address_get_network_id(_cardano_from_byron_to_address_const(address), network_id);
}

void
cardano_byron_address_unref(cardano_byron_address_t** address)
{
  cardano_address_unref(_cardano_from_byron_pointer_to_address_pointer(address));
}

void
cardano_byron_address_ref(cardano_byron_address_t* address)
{
  cardano_address_ref(_cardano_from_byron_to_address(address));
}

size_t
cardano_byron_address_refcount(const cardano_byron_address_t* address)
{
  return cardano_address_refcount(_cardano_from_byron_to_address_const(address));
}

void
cardano_byron_address_set_last_error(cardano_byron_address_t* address, const char* message)
{
  cardano_address_set_last_error(_cardano_from_byron_to_address(address), message);
}

const char*
cardano_byron_address_get_last_error(const cardano_byron_address_t* address)
{
  return cardano_address_get_last_error(_cardano_from_byron_to_address_const(address));
}
