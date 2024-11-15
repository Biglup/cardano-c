/**
 * \file address.c
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
#include <cardano/address/address_type.h>
#include <cardano/address/base_address.h>
#include <cardano/address/byron_address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/address/reward_address.h>
#include <cardano/encoding/base58.h>
#include <cardano/encoding/bech32.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"
#include "./internals/addr_common.h"

#include <string.h>

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_address_from_bytes(const byte_t* data, const size_t size, cardano_address_t** address)
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

  const cardano_address_type_t type = (cardano_address_type_t)(data[0] >> 4);

  switch (type)
  {
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
    {
      cardano_base_address_t* base_address = NULL;
      const cardano_error_t   result       = cardano_base_address_from_bytes(data, size, &base_address);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *address = _cardano_from_base_to_address(base_address);
      return CARDANO_SUCCESS;
    }
    case CARDANO_ADDRESS_TYPE_BYRON:
    {
      cardano_byron_address_t* byron_address = NULL;
      const cardano_error_t    result        = cardano_byron_address_from_bytes(data, size, &byron_address);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *address = _cardano_from_byron_to_address(byron_address);
      return CARDANO_SUCCESS;
    }
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
    {
      cardano_enterprise_address_t* enterprise_address = NULL;
      const cardano_error_t         result             = cardano_enterprise_address_from_bytes(data, size, &enterprise_address);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *address = _cardano_from_enterprise_to_address(enterprise_address);
      return CARDANO_SUCCESS;
    }
    case CARDANO_ADDRESS_TYPE_REWARD_KEY:
    case CARDANO_ADDRESS_TYPE_REWARD_SCRIPT:
    {
      cardano_reward_address_t* reward_address = NULL;
      const cardano_error_t     result         = cardano_reward_address_from_bytes(data, size, &reward_address);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *address = _cardano_from_reward_to_address(reward_address);
      return CARDANO_SUCCESS;
    }
    case CARDANO_ADDRESS_TYPE_POINTER_KEY:
    case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
    {
      cardano_pointer_address_t* pointer_address = NULL;
      const cardano_error_t      result          = cardano_pointer_address_from_bytes(data, size, &pointer_address);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      *address = _cardano_from_pointer_to_address(pointer_address);
      return CARDANO_SUCCESS;
    }
    default:
    {
      return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
    }
  }
}

size_t
cardano_address_get_bytes_size(const cardano_address_t* address)
{
  if (address == NULL)
  {
    return 0;
  }

  return address->address_data_size;
}

cardano_error_t
cardano_address_to_bytes(const cardano_address_t* address, byte_t* data, const size_t size)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < cardano_address_get_bytes_size(address))
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(data, size, address->address_data, address->address_data_size);

  return CARDANO_SUCCESS;
}

const byte_t*
cardano_address_get_bytes(const cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  return address->address_data;
}

cardano_error_t
cardano_address_from_string(const char* data, const size_t size, cardano_address_t** address)
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

  if (_cardano_has_valid_bech32_prefix(data, size))
  {
    size_t       hrp_size    = 0;
    const size_t data_length = cardano_encoding_bech32_get_decoded_length(data, size, &hrp_size);
    char*        hrp         = (char*)_cardano_malloc(hrp_size);

    if ((hrp_size == 0U) || (hrp == NULL))
    {
      _cardano_free(hrp);

      return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
    }

    byte_t* decoded_data = (byte_t*)_cardano_malloc(data_length);

    if ((data_length == 0U) || (decoded_data == NULL))
    {
      _cardano_free(hrp);
      _cardano_free(decoded_data);

      return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
    }

    cardano_error_t result = cardano_encoding_bech32_decode(data, size, hrp, hrp_size, decoded_data, data_length);

    if (result != CARDANO_SUCCESS)
    {
      _cardano_free(hrp);
      _cardano_free(decoded_data);

      return result;
    }

    result = cardano_address_from_bytes(decoded_data, data_length, address);

    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return result;
  }

  const size_t base58_data_length = cardano_encoding_base58_get_decoded_length(data, size);
  byte_t*      decoded_data       = (byte_t*)_cardano_malloc(base58_data_length);

  if (decoded_data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_encoding_base58_decode(data, size, decoded_data, base58_data_length);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(decoded_data);

    return result;
  }

  result = cardano_address_from_bytes(decoded_data, base58_data_length, address);

  _cardano_free(decoded_data);

  return result;
}

size_t
cardano_address_get_string_size(const cardano_address_t* address)
{
  if (address == NULL)
  {
    return 0U;
  }

  return cardano_safe_strlen(address->address_str, 1024) + 1U;
}

cardano_error_t
cardano_address_to_string(const cardano_address_t* address, char* data, const size_t size)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size == 0U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  const size_t src_size = cardano_address_get_string_size(address);

  if (size < cardano_address_get_string_size(address))
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(data, size, address->address_str, src_size);

  return CARDANO_SUCCESS;
}

const char*
cardano_address_get_string(const cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  return address->address_str;
}

bool
cardano_address_is_valid_bech32(const char* data, const size_t size)
{
  size_t       hrp_size    = 0;
  const size_t data_length = cardano_encoding_bech32_get_decoded_length(data, size, &hrp_size);
  char*        hrp         = (char*)_cardano_malloc(hrp_size);

  if ((hrp_size == 0U) || (hrp == NULL))
  {
    _cardano_free(hrp);

    return false;
  }

  byte_t* decoded_data = (byte_t*)_cardano_malloc(data_length);

  if ((data_length == 0U) || (decoded_data == NULL))
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return false;
  }

  cardano_error_t result = cardano_encoding_bech32_decode(data, size, hrp, hrp_size, decoded_data, data_length);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return false;
  }

  cardano_address_t* address = NULL;
  result                     = cardano_address_from_bytes(decoded_data, data_length, &address);

  _cardano_free(hrp);
  _cardano_free(decoded_data);
  cardano_address_unref(&address);

  return result == CARDANO_SUCCESS;
}

bool
cardano_address_is_valid_byron(const char* data, const size_t size)
{
  const size_t data_length  = cardano_encoding_base58_get_decoded_length(data, size);
  byte_t*      decoded_data = (byte_t*)_cardano_malloc(data_length);

  if (decoded_data == NULL)
  {
    return false;
  }

  cardano_error_t result = cardano_encoding_base58_decode(data, size, decoded_data, data_length);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(decoded_data);

    return false;
  }

  cardano_address_t* address = NULL;
  result                     = cardano_address_from_bytes(decoded_data, data_length, &address);

  cardano_address_unref(&address);
  _cardano_free(decoded_data);

  return result == CARDANO_SUCCESS;
}

bool
cardano_address_is_valid(const char* data, const size_t size)
{
  if (data == NULL)
  {
    return false;
  }

  if (size == 0U)
  {
    return false;
  }

  if (_cardano_has_valid_bech32_prefix(data, size))
  {
    return cardano_address_is_valid_bech32(data, size);
  }

  return cardano_address_is_valid_byron(data, size);
}

cardano_error_t
cardano_address_get_type(const cardano_address_t* address, cardano_address_type_t* type)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = address->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_address_get_network_id(const cardano_address_t* address, cardano_network_id_t* network_id)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (network_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (address->type == CARDANO_ADDRESS_TYPE_BYRON)
  {
    if (address->byron_content == NULL)
    {
      return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
    }

    if (address->byron_content->attributes.magic == -1)
    {
      *network_id = CARDANO_NETWORK_ID_MAIN_NET;
      return CARDANO_SUCCESS;
    }

    *network_id = CARDANO_NETWORK_ID_TEST_NET;
    return CARDANO_SUCCESS;
  }

  if (address->network_id == NULL)
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  *network_id = *address->network_id;

  return CARDANO_SUCCESS;
}

cardano_byron_address_t*
cardano_address_to_byron_address(const cardano_address_t* address)
{
  cardano_byron_address_t* byron_address = NULL;
  const cardano_error_t    result        = cardano_byron_address_from_address(address, &byron_address);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return byron_address;
}

cardano_reward_address_t*
cardano_address_to_reward_address(const cardano_address_t* address)
{
  cardano_reward_address_t* reward_address = NULL;
  const cardano_error_t     result         = cardano_reward_address_from_address(address, &reward_address);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return reward_address;
}

cardano_pointer_address_t*
cardano_address_to_pointer_address(const cardano_address_t* address)
{
  cardano_pointer_address_t* pointer_address = NULL;
  const cardano_error_t      result          = cardano_pointer_address_from_address(address, &pointer_address);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return pointer_address;
}

cardano_enterprise_address_t*
cardano_address_to_enterprise_address(const cardano_address_t* address)
{
  cardano_enterprise_address_t* enterprise_address = NULL;
  const cardano_error_t         result             = cardano_enterprise_address_from_address(address, &enterprise_address);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return enterprise_address;
}

cardano_base_address_t*
cardano_address_to_base_address(const cardano_address_t* address)
{
  cardano_base_address_t* base_address = NULL;
  const cardano_error_t   result       = cardano_base_address_from_address(address, &base_address);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return base_address;
}

bool
cardano_address_equals(const cardano_address_t* lhs, const cardano_address_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (lhs->type != rhs->type)
  {
    return false;
  }

  if (lhs->address_data_size != rhs->address_data_size)
  {
    return false;
  }

  return memcmp(lhs->address_data, rhs->address_data, lhs->address_data_size) == 0;
}

void
cardano_address_unref(cardano_address_t** address)
{
  if ((address == NULL) || (*address == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*address)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *address = NULL;
    return;
  }
}

void
cardano_address_ref(cardano_address_t* address)
{
  if (address == NULL)
  {
    return;
  }

  cardano_object_ref(&address->base);
}

size_t
cardano_address_refcount(const cardano_address_t* address)
{
  if (address == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&address->base);
}

void
cardano_address_set_last_error(cardano_address_t* address, const char* message)
{
  cardano_object_set_last_error(&address->base, message);
}

const char*
cardano_address_get_last_error(const cardano_address_t* address)
{
  return cardano_object_get_last_error(&address->base);
}
