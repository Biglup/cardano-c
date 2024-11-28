/**
 * \file base_addr_pack.c
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

#include <cardano/crypto/blake2b_hash_size.h>

#include "addr_common.h"
#include "base_addr_pack.h"

#include "../../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t ADDRESS_HEADER_SIZE = 1;

/* IMPLEMENTATION ************************************************************/

cardano_error_t
_cardano_get_base_address_type(
  const cardano_credential_t* payment_credential,
  const cardano_credential_t* stake_credential,
  cardano_address_type_t*     type)
{
  assert(payment_credential != NULL);
  assert(stake_credential != NULL);
  assert(type != NULL);

  cardano_credential_type_t payment_type;
  cardano_credential_type_t stake_type;

  cardano_error_t get_type_result = cardano_credential_get_type(payment_credential, &payment_type);

  assert(get_type_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(get_type_result);

  get_type_result = cardano_credential_get_type(stake_credential, &stake_type);

  assert(get_type_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(get_type_result);

  if ((payment_type == CARDANO_CREDENTIAL_TYPE_KEY_HASH) && (stake_type == CARDANO_CREDENTIAL_TYPE_KEY_HASH))
  {
    *type = CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY;
  }

  if ((payment_type == CARDANO_CREDENTIAL_TYPE_KEY_HASH) && (stake_type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH))
  {
    *type = CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT;
  }

  if ((payment_type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH) && (stake_type == CARDANO_CREDENTIAL_TYPE_KEY_HASH))
  {
    *type = CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY;
  }

  if ((payment_type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH) && (stake_type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH))
  {
    *type = CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_unpack_base_address(const byte_t* data, const size_t size, cardano_base_address_t** address)
{
  assert(data != NULL);
  assert(address != NULL);

  if (size < (size_t)ADDRESS_HEADER_SIZE + (2U * (size_t)CARDANO_BLAKE2B_HASH_SIZE_224))
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  cardano_address_type_t type       = (cardano_address_type_t)(data[0] >> 4);
  cardano_network_id_t   network_id = (cardano_network_id_t)(uint8_t)((uint8_t)data[0] & 0x0FU);

  cardano_credential_type_t payment_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  cardano_credential_type_t stake_type   = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

  cardano_error_t credential_type_result = _cardano_get_payment_credential_type(type, &payment_type);

  assert(credential_type_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(credential_type_result);

  credential_type_result = _cardano_get_stake_credential_type(type, &stake_type);

  assert(credential_type_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(credential_type_result);

  cardano_credential_t* payment_credential = NULL;
  cardano_credential_t* stake_credential   = NULL;

  cardano_error_t credential_result = cardano_credential_from_hash_bytes(
    &data[ADDRESS_HEADER_SIZE],
    CARDANO_BLAKE2B_HASH_SIZE_224,
    payment_type,
    &payment_credential);

  if (credential_result != CARDANO_SUCCESS)
  {
    return credential_result;
  }

  credential_result = cardano_credential_from_hash_bytes(
    &data[(size_t)CARDANO_BLAKE2B_HASH_SIZE_224 + ADDRESS_HEADER_SIZE],
    CARDANO_BLAKE2B_HASH_SIZE_224,
    stake_type,
    &stake_credential);

  if (credential_result != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&payment_credential);
    return credential_result;
  }

  cardano_error_t base_address_result = cardano_base_address_from_credentials(
    network_id,
    payment_credential,
    stake_credential,
    address);

  cardano_credential_unref(&payment_credential);
  cardano_credential_unref(&stake_credential);

  return base_address_result;
}

void
_cardano_pack_base_address(const cardano_address_t* address, byte_t* data, const size_t size)
{
  assert(size >= (size_t)ADDRESS_HEADER_SIZE + (2U * (size_t)CARDANO_BLAKE2B_HASH_SIZE_224));
  assert(address != NULL);
  assert(data != NULL);

  CARDANO_UNUSED(memset(data, 0, ADDRESS_HEADER_SIZE + (2U * (size_t)CARDANO_BLAKE2B_HASH_SIZE_224)));

  cardano_blake2b_hash_t* payment_cred_hash = cardano_credential_get_hash(address->payment_credential);
  cardano_blake2b_hash_t* stake_cred_hash   = cardano_credential_get_hash(address->stake_credential);

  data[0] = ((byte_t)address->type << 4U) | (byte_t)(*address->network_id);

  cardano_safe_memcpy(
    &data[ADDRESS_HEADER_SIZE],
    size - ADDRESS_HEADER_SIZE,
    cardano_blake2b_hash_get_data(payment_cred_hash),
    CARDANO_BLAKE2B_HASH_SIZE_224);

  cardano_safe_memcpy(
    &data[(size_t)CARDANO_BLAKE2B_HASH_SIZE_224 + ADDRESS_HEADER_SIZE],
    size - ((size_t)CARDANO_BLAKE2B_HASH_SIZE_224 + ADDRESS_HEADER_SIZE),
    cardano_blake2b_hash_get_data(stake_cred_hash),
    CARDANO_BLAKE2B_HASH_SIZE_224);

  cardano_blake2b_hash_unref(&payment_cred_hash);
  cardano_blake2b_hash_unref(&stake_cred_hash);
}
