/**
 * \file addr_common.c
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

#include <cardano/encoding/bech32.h>

#include "../../allocators.h"
#include "addr_common.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const char* BECH32_PREFIX_MAINNET       = "addr";
static const char* BECH32_PREFIX_TESTNET       = "addr_test";
static const char* BECH32_PREFIX_STAKE_MAINNET = "stake";
static const char* BECH32_PREFIX_STAKE_TESTNET = "stake_test";

static const size_t BECH32_PREFIX_LENGTH               = 4;
static const size_t BECH32_PREFIX_STAKE_LENGTH         = 5;
static const size_t BECH32_PREFIX_TESTNET_LENGTH       = 9;
static const size_t BECH32_PREFIX_STAKE_TESTNET_LENGTH = 10;

/* IMPLEMENTATION ************************************************************/

const char*
_cardano_get_bech32_prefix(const cardano_address_type_t type, const cardano_network_id_t network_id, size_t* size)
{
  const char* prefix = NULL;

  switch (type)
  {
    case CARDANO_ADDRESS_TYPE_REWARD_KEY:
    case CARDANO_ADDRESS_TYPE_REWARD_SCRIPT:
      if (network_id == CARDANO_NETWORK_ID_MAIN_NET)
      {
        prefix = BECH32_PREFIX_STAKE_MAINNET;
        *size  = BECH32_PREFIX_STAKE_LENGTH;
      }
      else
      {
        prefix = BECH32_PREFIX_STAKE_TESTNET;
        *size  = BECH32_PREFIX_STAKE_TESTNET_LENGTH;
      }
      break;
    case CARDANO_ADDRESS_TYPE_POINTER_KEY:
    case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BYRON:
    default:
      if (network_id == CARDANO_NETWORK_ID_MAIN_NET)
      {
        prefix = BECH32_PREFIX_MAINNET;
        *size  = BECH32_PREFIX_LENGTH;
      }
      else
      {
        prefix = BECH32_PREFIX_TESTNET;
        *size  = BECH32_PREFIX_TESTNET_LENGTH;
      }
      break;
  }

  return prefix;
}

bool
_cardano_has_valid_bech32_prefix(const char* address, size_t length)
{
  assert(address != NULL);

  return _cardano_is_valid_payment_address_prefix(address, length) ||
    _cardano_is_valid_stake_address_prefix(address, length);
}

cardano_address_t*
_cardano_from_base_to_address(cardano_base_address_t* base_address)
{
  return (cardano_address_t*)((void*)base_address);
}

cardano_address_t**
_cardano_from_base_pointer_to_address_pointer(cardano_base_address_t** base_address)
{
  return (cardano_address_t**)((void**)base_address);
}

const cardano_address_t*
_cardano_from_base_to_address_const(const cardano_base_address_t* base_address)
{
  return (const cardano_address_t*)((const void*)base_address);
}

cardano_base_address_t*
_cardano_from_address_to_base(cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  assert(
    (address->type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY) ||
    (address->type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT) ||
    (address->type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY) ||
    (address->type == CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT));

  return (cardano_base_address_t*)((void*)address);
}

cardano_address_t*
_cardano_from_enterprise_to_address(cardano_enterprise_address_t* enterprise_address)
{
  return (cardano_address_t*)((void*)enterprise_address);
}

cardano_address_t**
_cardano_from_enterprise_pointer_to_address_pointer(cardano_enterprise_address_t** enterprise_address)
{
  return (cardano_address_t**)((void**)enterprise_address);
}

const cardano_address_t*
_cardano_from_enterprise_to_address_const(const cardano_enterprise_address_t* enterprise_address)
{
  return (const cardano_address_t*)((const void*)enterprise_address);
}

cardano_enterprise_address_t*
_cardano_from_address_to_enterprise(cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  assert(
    (address->type == CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY) ||
    (address->type == CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT));

  return (cardano_enterprise_address_t*)((void*)address);
}

cardano_address_t*
_cardano_from_pointer_to_address(cardano_pointer_address_t* pointer_address)
{
  return (cardano_address_t*)((void*)pointer_address);
}

cardano_address_t**
_cardano_from_pointer_pointer_to_address_pointer(cardano_pointer_address_t** pointer_address)
{
  return (cardano_address_t**)((void**)pointer_address);
}

const cardano_address_t*
_cardano_from_pointer_to_address_const(const cardano_pointer_address_t* pointer_address)
{
  return (const cardano_address_t*)((const void*)pointer_address);
}

cardano_pointer_address_t*
_cardano_from_address_to_pointer(cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  assert(
    (address->type == CARDANO_ADDRESS_TYPE_POINTER_KEY) ||
    (address->type == CARDANO_ADDRESS_TYPE_POINTER_SCRIPT));

  return (cardano_pointer_address_t*)((void*)address);
}

cardano_address_t*
_cardano_from_reward_to_address(cardano_reward_address_t* reward_address)
{
  return (cardano_address_t*)((void*)reward_address);
}

cardano_address_t**
_cardano_from_reward_pointer_to_address_pointer(cardano_reward_address_t** reward_address)
{
  return (cardano_address_t**)((void**)reward_address);
}

const cardano_address_t*
_cardano_from_reward_to_address_const(const cardano_reward_address_t* reward_address)
{
  return (const cardano_address_t*)((const void*)reward_address);
}

cardano_reward_address_t*
_cardano_from_address_to_reward(cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  assert(
    (address->type == CARDANO_ADDRESS_TYPE_REWARD_KEY) ||
    (address->type == CARDANO_ADDRESS_TYPE_REWARD_SCRIPT));

  return (cardano_reward_address_t*)((void*)address);
}

cardano_address_t*
_cardano_from_byron_to_address(cardano_byron_address_t* byron_address)
{
  return (cardano_address_t*)((void*)byron_address);
}

cardano_address_t**
_cardano_from_byron_pointer_to_address_pointer(cardano_byron_address_t** byron_address)
{
  return (cardano_address_t**)((void**)byron_address);
}

const cardano_address_t*
_cardano_from_byron_to_address_const(const cardano_byron_address_t* byron_address)
{
  return (const cardano_address_t*)((const void*)byron_address);
}

cardano_byron_address_t*
_cardano_from_address_to_byron(cardano_address_t* address)
{
  if (address == NULL)
  {
    return NULL;
  }

  assert(address->type == CARDANO_ADDRESS_TYPE_BYRON);

  return (cardano_byron_address_t*)((void*)address);
}

cardano_error_t
_cardano_get_payment_credential_type(const cardano_address_type_t type, cardano_credential_type_t* credential_type)
{
  assert(credential_type != NULL);

  switch (type)
  {
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_POINTER_KEY:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
    case CARDANO_ADDRESS_TYPE_REWARD_KEY:
      *credential_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
      break;
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_REWARD_SCRIPT:
      *credential_type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
      break;
    case CARDANO_ADDRESS_TYPE_BYRON:
    default:
      return CARDANO_ERROR_INVALID_ADDRESS_TYPE;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_get_stake_credential_type(cardano_address_type_t type, cardano_credential_type_t* credential_type)
{
  assert(credential_type != NULL);

  switch (type)
  {
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
      *credential_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
      break;
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
      *credential_type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
      break;
    case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_REWARD_SCRIPT:
    case CARDANO_ADDRESS_TYPE_POINTER_KEY:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
    case CARDANO_ADDRESS_TYPE_REWARD_KEY:
    case CARDANO_ADDRESS_TYPE_BYRON:
    default:
      return CARDANO_ERROR_INVALID_ADDRESS_TYPE;
  }

  return CARDANO_SUCCESS;
}

bool
_cardano_is_valid_payment_address_prefix(const char* address, size_t length)
{
  if (address == NULL)
  {
    return false;
  }

  return ((length >= BECH32_PREFIX_LENGTH) && (strncmp(address, BECH32_PREFIX_MAINNET, BECH32_PREFIX_LENGTH) == 0)) ||
    ((length >= BECH32_PREFIX_TESTNET_LENGTH) && (strncmp(address, BECH32_PREFIX_TESTNET, BECH32_PREFIX_TESTNET_LENGTH) == 0));
}

bool
_cardano_is_valid_stake_address_prefix(const char* address, size_t length)
{
  if (address == NULL)
  {
    return false;
  }

  return ((length >= BECH32_PREFIX_STAKE_LENGTH) && (strncmp(address, BECH32_PREFIX_STAKE_MAINNET, BECH32_PREFIX_STAKE_LENGTH) == 0)) ||
    ((length >= BECH32_PREFIX_STAKE_TESTNET_LENGTH) && (strncmp(address, BECH32_PREFIX_STAKE_TESTNET, BECH32_PREFIX_STAKE_TESTNET_LENGTH) == 0));
}

void
_cardano_to_bech32_addr(
  const byte_t*          data,
  size_t                 data_size,
  cardano_network_id_t   network_id,
  cardano_address_type_t type,
  char*                  address,
  size_t                 address_size)
{
  assert(data != NULL);
  assert(address != NULL);

  size_t      hrp_size = 0U;
  const char* hrp      = _cardano_get_bech32_prefix(type, network_id, &hrp_size);
  size_t      size     = cardano_encoding_bech32_get_encoded_length(hrp, hrp_size, data, data_size);

  assert(size <= address_size);
  CARDANO_UNUSED(size);

  cardano_error_t result = cardano_encoding_bech32_encode(hrp, hrp_size, data, data_size, address, address_size);
  CARDANO_UNUSED(result);
}

void
_cardano_address_deallocate(void* object)
{
  assert(object != NULL);

  cardano_address_t* address = (cardano_address_t*)object;

  if (address->network_id != NULL)
  {
    _cardano_free(address->network_id);
    address->network_id = NULL;
  }

  if (address->stake_pointer != NULL)
  {
    _cardano_free(address->stake_pointer);
    address->stake_pointer = NULL;
  }

  if (address->payment_credential != NULL)
  {
    cardano_credential_unref(&address->payment_credential);
  }

  if (address->stake_credential != NULL)
  {
    cardano_credential_unref(&address->stake_credential);
  }

  if (address->byron_content != NULL)
  {
    _cardano_free(address->byron_content);
    address->byron_content = NULL;
  }

  _cardano_free(address);
}
