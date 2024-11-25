/**
 * \file base_address.c
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
#include <cardano/address/base_address.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/encoding/bech32.h>
#include <cardano/error.h>

#include "../allocators.h"
#include "./internals/addr_common.h"
#include "./internals/base_addr_pack.h"

#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t ADDRESS_HEADER_SIZE = 1;

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_base_address_from_credentials(
  const cardano_network_id_t network_id,
  cardano_credential_t*      payment,
  cardano_credential_t*      stake,
  cardano_base_address_t**   base_address)
{
  if (payment == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (stake == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (base_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* address = _cardano_malloc(sizeof(cardano_address_t));

  if (address == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  address->base.ref_count     = 1;
  address->base.last_error[0] = '\0';
  address->base.deallocator   = _cardano_address_deallocate;

  const cardano_error_t result = _cardano_get_base_address_type(payment, stake, &address->type);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(address);
    return result;
  }

  address->network_id = _cardano_malloc(sizeof(cardano_network_id_t));

  if (address->network_id == NULL)
  {
    _cardano_free(address);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *address->network_id        = network_id;
  address->payment_credential = payment;
  address->stake_credential   = stake;
  address->byron_content      = NULL;
  address->stake_pointer      = NULL;
  address->address_data_size  = ADDRESS_HEADER_SIZE + (2U * (size_t)CARDANO_BLAKE2B_HASH_SIZE_224);

  _cardano_pack_base_address(address, address->address_data, sizeof(address->address_data));
  _cardano_to_bech32_addr(
    address->address_data,
    address->address_data_size,
    network_id,
    address->type,
    address->address_str,
    1024);

  cardano_credential_ref(payment);
  cardano_credential_ref(stake);

  *base_address = _cardano_from_address_to_base(address);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_base_address_from_address(
  const cardano_address_t* address,
  cardano_base_address_t** base_address)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (base_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (
    (address->type != CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY) &&
    (address->type != CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT) &&
    (address->type != CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT) &&
    (address->type != CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY))
  {
    return CARDANO_ERROR_INVALID_ADDRESS_TYPE;
  }

  cardano_address_t*    address_copy = NULL;
  const cardano_error_t copy_result  = cardano_address_from_bytes(address->address_data, address->address_data_size, &address_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    return copy_result;
  }

  *base_address = _cardano_from_address_to_base(address_copy);

  return CARDANO_SUCCESS;
}

cardano_address_t*
cardano_base_address_to_address(const cardano_base_address_t* base_address)
{
  if (base_address == NULL)
  {
    return NULL;
  }

  const cardano_address_t* address = _cardano_from_base_to_address_const(base_address);

  cardano_address_t*    address_copy = NULL;
  const cardano_error_t copy_result  = cardano_address_from_bytes(address->address_data, address->address_data_size, &address_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return address_copy;
}

cardano_credential_t*
cardano_base_address_get_payment_credential(
  cardano_base_address_t* base_address)
{
  if (base_address == NULL)
  {
    return NULL;
  }

  cardano_address_t* address = _cardano_from_base_to_address(base_address);

  cardano_credential_ref(address->payment_credential);

  return address->payment_credential;
}

cardano_credential_t*
cardano_base_address_get_stake_credential(
  cardano_base_address_t* base_address)
{
  if (base_address == NULL)
  {
    return NULL;
  }

  cardano_address_t* address = _cardano_from_base_to_address(base_address);

  cardano_credential_ref(address->stake_credential);

  return address->stake_credential;
}

cardano_error_t
cardano_base_address_from_bytes(
  const byte_t*            data,
  const size_t             size,
  cardano_base_address_t** address)
{
  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < (ADDRESS_HEADER_SIZE + (2U * (size_t)CARDANO_BLAKE2B_HASH_SIZE_224)))
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return _cardano_unpack_base_address(data, size, address);
}

size_t
cardano_base_address_get_bytes_size(const cardano_base_address_t* address)
{
  return cardano_address_get_bytes_size(_cardano_from_base_to_address_const(address));
}

const byte_t*
cardano_base_address_get_bytes(const cardano_base_address_t* address)
{
  return cardano_address_get_bytes(_cardano_from_base_to_address_const(address));
}

cardano_error_t
cardano_base_address_to_bytes(
  const cardano_base_address_t* address,
  byte_t*                       data,
  const size_t                  size)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < (ADDRESS_HEADER_SIZE + (2U * (size_t)CARDANO_BLAKE2B_HASH_SIZE_224)))
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  return cardano_address_to_bytes(_cardano_from_base_to_address_const(address), data, size);
}

cardano_error_t
cardano_base_address_from_bech32(
  const char*              data,
  const size_t             size,
  cardano_base_address_t** address)
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

  size_t       hrp_size  = 0;
  const size_t data_size = cardano_encoding_bech32_get_decoded_length(data, size, &hrp_size);

  if (data_size < (ADDRESS_HEADER_SIZE + (2U * (size_t)CARDANO_BLAKE2B_HASH_SIZE_224)))
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  char*   hrp          = _cardano_malloc(hrp_size);
  byte_t* decoded_data = _cardano_malloc(data_size);

  const cardano_error_t decode_result = cardano_encoding_bech32_decode(data, size, hrp, hrp_size, decoded_data, data_size);

  if (decode_result != CARDANO_SUCCESS)
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return decode_result;
  }

  if (!_cardano_is_valid_payment_address_prefix(hrp, hrp_size))
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  const cardano_error_t unpack_result = _cardano_unpack_base_address(decoded_data, data_size, address);

  _cardano_free(hrp);
  _cardano_free(decoded_data);

  return unpack_result;
}

size_t
cardano_base_address_get_bech32_size(const cardano_base_address_t* address)
{
  return cardano_address_get_string_size(_cardano_from_base_to_address_const(address));
}

cardano_error_t
cardano_base_address_to_bech32(
  const cardano_base_address_t* address,
  char*                         data,
  const size_t                  size)
{
  return cardano_address_to_string(_cardano_from_base_to_address_const(address), data, size);
}

const char*
cardano_base_address_get_string(const cardano_base_address_t* address)
{
  return cardano_address_get_string(_cardano_from_base_to_address_const(address));
}

cardano_error_t
cardano_base_address_get_network_id(const cardano_base_address_t* address, cardano_network_id_t* network_id)
{
  return cardano_address_get_network_id(_cardano_from_base_to_address_const(address), network_id);
}

void
cardano_base_address_unref(cardano_base_address_t** address)
{
  cardano_address_unref(_cardano_from_base_pointer_to_address_pointer(address));
}

void
cardano_base_address_ref(cardano_base_address_t* address)
{
  cardano_address_ref(_cardano_from_base_to_address(address));
}

size_t
cardano_base_address_refcount(const cardano_base_address_t* address)
{
  return cardano_address_refcount(_cardano_from_base_to_address_const(address));
}

void
cardano_base_address_set_last_error(cardano_base_address_t* address, const char* message)
{
  cardano_address_set_last_error(_cardano_from_base_to_address(address), message);
}

const char*
cardano_base_address_get_last_error(const cardano_base_address_t* address)
{
  return cardano_address_get_last_error(_cardano_from_base_to_address_const(address));
}