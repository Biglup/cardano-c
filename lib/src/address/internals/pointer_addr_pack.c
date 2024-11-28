/**
 * \file pointer_addr_pack.c
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

#include <cardano/address/stake_pointer.h>
#include <cardano/crypto/blake2b_hash_size.h>

#include "../../allocators.h"
#include "addr_common.h"
#include "pointer_addr_pack.h"

#include "../../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t ADDRESS_HEADER_SIZE = 1;

/* IMPLEMENTATION ************************************************************/

size_t
_cardano_pointer_address_variable_length_encode(uint32_t value, byte_t* buffer, size_t buffer_size)
{
  assert(buffer != NULL);
  assert(buffer_size >= 10U);

  size_t   output_size    = 0;
  uint32_t remaining      = value;
  byte_t   tmp_buffer[10] = { 0 };

  tmp_buffer[output_size] = ((uint8_t)(remaining & 127U));
  ++output_size;

  while (remaining > 127U)
  {
    if (output_size >= buffer_size)
    {
      return 0;
    }

    remaining               >>= 7U;
    tmp_buffer[output_size] = ((uint8_t)(remaining & 127U)) | 128U;

    ++output_size;
  }

  for (size_t i = 0; i < output_size; ++i)
  {
    buffer[i] = tmp_buffer[output_size - i - 1U];
  }

  return output_size;
}

int64_t
_cardano_pointer_address_variable_length_decode(const byte_t* array, size_t total_bytes, size_t size, size_t* bytes_read)
{
  assert(array != NULL);
  assert(bytes_read != NULL);

  uint32_t value = 0;
  *bytes_read    = 0;
  byte_t byte    = 0;

  do
  {
    if (((*bytes_read) + total_bytes) >= size)
    {
      return -1;
    }

    byte  = array[*bytes_read];
    value = (value << 7U) | (byte & 127U);

    ++(*bytes_read);
  }
  while ((byte & 128U) > 0U);

  return value;
}

cardano_error_t
_cardano_unpack_pointer_address(const byte_t* data, const size_t size, cardano_pointer_address_t** address)
{
  assert(data != NULL);
  assert(address != NULL);

  if (size < (size_t)(((size_t)ADDRESS_HEADER_SIZE + (size_t)CARDANO_BLAKE2B_HASH_SIZE_224 + 1U)))
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  cardano_address_type_t type       = (cardano_address_type_t)(data[0] >> 4);
  cardano_network_id_t   network_id = (cardano_network_id_t)(uint8_t)((uint8_t)data[0] & 0x0FU);

  cardano_credential_type_t payment_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

  cardano_error_t credential_type_result = _cardano_get_payment_credential_type(type, &payment_type);

  assert(credential_type_result == CARDANO_SUCCESS);
  CARDANO_UNUSED(credential_type_result);

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

  cardano_stake_pointer_t stake_pointer = { 0 };

  size_t bytes_read  = 0;
  size_t total_bytes = ADDRESS_HEADER_SIZE + (int64_t)CARDANO_BLAKE2B_HASH_SIZE_224;

  const int64_t slot = _cardano_pointer_address_variable_length_decode(&data[total_bytes], total_bytes, size, &bytes_read);

  if (slot < 0)
  {
    cardano_credential_unref(&payment_credential);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  stake_pointer.slot = (size_t)slot;

  total_bytes += bytes_read;
  bytes_read  = 0;

  const int64_t tx_index = _cardano_pointer_address_variable_length_decode(&data[total_bytes], total_bytes, size, &bytes_read);

  if (tx_index < 0)
  {
    cardano_credential_unref(&payment_credential);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  stake_pointer.tx_index = (size_t)tx_index;

  total_bytes += bytes_read;
  bytes_read  = 0;

  const int64_t cert_index = _cardano_pointer_address_variable_length_decode(&data[total_bytes], total_bytes, size, &bytes_read);

  if (cert_index < 0)
  {
    cardano_credential_unref(&payment_credential);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  stake_pointer.cert_index = (size_t)cert_index;

  cardano_error_t pointer_address_result = cardano_pointer_address_from_credentials(
    network_id,
    payment_credential,
    stake_pointer,
    address);

  cardano_credential_unref(&payment_credential);

  return pointer_address_result;
}

int64_t
_cardano_pack_pointer_address(const cardano_address_t* address, byte_t* data, const size_t size)
{
  assert(address != NULL);
  assert(data != NULL);

  data[0] = ((byte_t)address->type << 4U) | (byte_t)(*address->network_id);

  cardano_safe_memcpy(
    &data[ADDRESS_HEADER_SIZE],
    size - (size_t)ADDRESS_HEADER_SIZE,
    cardano_credential_get_hash_bytes(address->payment_credential),
    CARDANO_BLAKE2B_HASH_SIZE_224);

  int64_t total_bytes   = (int64_t)(ADDRESS_HEADER_SIZE + (int64_t)CARDANO_BLAKE2B_HASH_SIZE_224);
  size_t  bytes_written = 0;
  byte_t  buffer[10]    = { 0 };

  bytes_written = _cardano_pointer_address_variable_length_encode((uint32_t)address->stake_pointer->slot, buffer, 10);

  cardano_safe_memcpy(
    &data[total_bytes],
    size - (size_t)total_bytes,
    buffer,
    bytes_written);

  total_bytes += (int64_t)bytes_written;

  bytes_written = _cardano_pointer_address_variable_length_encode((uint32_t)address->stake_pointer->tx_index, buffer, 10);

  cardano_safe_memcpy(
    &data[total_bytes],
    size - (size_t)total_bytes,
    buffer,
    bytes_written);

  total_bytes += (int64_t)bytes_written;

  bytes_written = _cardano_pointer_address_variable_length_encode((uint32_t)address->stake_pointer->cert_index, buffer, 10);

  cardano_safe_memcpy(
    &data[total_bytes],
    size - (size_t)total_bytes,
    buffer,
    bytes_written);

  return (int64_t)total_bytes + (int64_t)bytes_written;
}
