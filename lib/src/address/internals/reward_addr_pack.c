/**
 * \file reward_addr_pack.c
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
#include "reward_addr_pack.h"

#include "../../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t ADDRESS_HEADER_SIZE = 1;

/* IMPLEMENTATION ************************************************************/

cardano_error_t
_cardano_unpack_reward_address(const byte_t* data, size_t size, cardano_reward_address_t** address)
{
  assert(data != NULL);
  assert(address != NULL);

  if (size < (ADDRESS_HEADER_SIZE + (size_t)CARDANO_BLAKE2B_HASH_SIZE_224))
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  cardano_address_type_t type       = (cardano_address_type_t)(data[0] >> 4);
  cardano_network_id_t   network_id = (cardano_network_id_t)(uint8_t)((uint8_t)data[0] & 0x0FU);

  cardano_credential_type_t payment_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

  cardano_error_t credential_type_result = _cardano_get_payment_credential_type(type, &payment_type);

  if (credential_type_result != CARDANO_SUCCESS)
  {
    return credential_type_result;
  }

  cardano_credential_t* payment_credential = NULL;

  cardano_error_t credential_result = cardano_credential_from_hash_bytes(
    &data[ADDRESS_HEADER_SIZE],
    CARDANO_BLAKE2B_HASH_SIZE_224,
    payment_type,
    &payment_credential);

  if (credential_result != CARDANO_SUCCESS)
  {
    return credential_result;
  }

  cardano_error_t reward_address_result = cardano_reward_address_from_credentials(
    network_id,
    payment_credential,
    address);

  cardano_credential_unref(&payment_credential);

  return reward_address_result;
}

void
_cardano_pack_reward_address(const cardano_address_t* address, byte_t* data, const size_t size)
{
  assert(address != NULL);
  assert(data != NULL);

  CARDANO_UNUSED(memset(data, 0, ADDRESS_HEADER_SIZE + (size_t)CARDANO_BLAKE2B_HASH_SIZE_224));

  cardano_blake2b_hash_t* payment_cred_hash = cardano_credential_get_hash(address->payment_credential);

  data[0] = ((byte_t)address->type << 4U) | (byte_t)(*address->network_id);

  cardano_safe_memcpy(
    &data[ADDRESS_HEADER_SIZE],
    size - ADDRESS_HEADER_SIZE,
    cardano_blake2b_hash_get_data(payment_cred_hash),
    CARDANO_BLAKE2B_HASH_SIZE_224);

  cardano_blake2b_hash_unref(&payment_cred_hash);
}
