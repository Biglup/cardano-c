/**
 * \file bip32_key_derivation.c
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#include "bip32_key_derivation.h"

#include <assert.h>
#include <sodium.h>
#include <string.h>

#include "../string_safe.h"
#include "arithmetic.h"

/* DEFINITIONS ****************************************************************/

bool
_cardano_crypto_is_hardened_derivation(const uint32_t index)
{
  return index >= 0x80000000u;
}

cardano_error_t
_cardano_crypto_derive_hardened(
  const int32_t index,
  const byte_t* scalar,
  const byte_t* iv,
  const byte_t* chain_code,
  byte_t*       z_mac,
  byte_t*       cc_mac)
{
  byte_t data[1 + 64 + 4] = { 0 };

  cardano_safe_memcpy(&data[1], sizeof(data) - 1U, scalar, 32);
  cardano_safe_memcpy(&data[1 + 32], sizeof(data) - (1U + 32U), iv, 32);

  data[1 + 64]     = (byte_t)((uint32_t)index & 0xFFu);
  data[1 + 64 + 1] = (byte_t)(((uint32_t)index >> 8) & 0xFFu);
  data[1 + 64 + 2] = (byte_t)(((uint32_t)index >> 16) & 0xFFu);
  data[1 + 64 + 3] = (byte_t)(((uint32_t)index >> 24) & 0xFFu);

  data[0] = 0x00;

  if (crypto_auth_hmacsha512(z_mac, data, sizeof(data), chain_code) == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  data[0] = 0x01;

  if (crypto_auth_hmacsha512(cc_mac, data, sizeof(data), chain_code) == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_crypto_derive_soft(
  const int32_t index,
  const byte_t* scalar,
  const byte_t* chain_code,
  byte_t*       z_mac,
  byte_t*       cc_mac)
{
  byte_t data[1 + 32 + 4] = { 0 };
  byte_t vk[32]           = { 0 };

  if (crypto_scalarmult_ed25519_base_noclamp(vk, scalar) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  cardano_safe_memcpy(&data[1], sizeof(data) - 1U, vk, 32);

  data[1 + 32]     = (byte_t)((uint32_t)index & 0xFFu);
  data[1 + 32 + 1] = (byte_t)(((uint32_t)index >> 8) & 0xFFu);
  data[1 + 32 + 2] = (byte_t)(((uint32_t)index >> 16) & 0xFFu);
  data[1 + 32 + 3] = (byte_t)(((uint32_t)index >> 24) & 0xFFu);

  data[0] = 0x02;

  if (crypto_auth_hmacsha512(z_mac, data, sizeof(data), chain_code) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  data[0] = 0x03;

  if (crypto_auth_hmacsha512(cc_mac, data, sizeof(data), chain_code) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_crypto_point_of_trunc28_mul8(const byte_t* sk, byte_t* out)
{
  byte_t zero[32]   = { 0 };
  byte_t scalar[32] = { 0 };

  _cardano_crypto_add28_mul8(zero, sk, scalar);

  if (crypto_scalarmult_ed25519_base_noclamp(out, scalar) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_crypto_derive_private(const byte_t* key, const int32_t index, byte_t* out, const size_t out_size)
{
  assert(out_size >= 96U);

  const byte_t* kl = key;
  const byte_t* kr = &key[32];
  const byte_t* cc = &key[64];

  byte_t z_mac[64]  = { 0 };
  byte_t cc_mac[64] = { 0 };

  if (_cardano_crypto_is_hardened_derivation(index))
  {
    cardano_error_t error = _cardano_crypto_derive_hardened(index, kl, kr, cc, z_mac, cc_mac);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }
  else
  {
    cardano_error_t error = _cardano_crypto_derive_soft(index, kl, cc, z_mac, cc_mac);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }

  byte_t zl[32] = { 0 };
  byte_t zr[32] = { 0 };

  cardano_safe_memcpy(zl, sizeof(zl), &z_mac, 32);
  cardano_safe_memcpy(zr, sizeof(zr), &z_mac[32], 32);

  byte_t left[32]  = { 0 };
  byte_t right[32] = { 0 };

  _cardano_crypto_add28_mul8(kl, zl, left);
  _cardano_crypto_add256bits(kr, zr, right);

  cardano_safe_memcpy(out, out_size, left, 32);
  cardano_safe_memcpy(&out[32], out_size - 32U, right, 32);
  cardano_safe_memcpy(&out[64], out_size - 64U, &cc_mac[32], 32);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_crypto_derive_public(const byte_t* key, const int32_t index, byte_t* out, const size_t out_size)
{
  assert(out_size >= 64U);

  const byte_t* pk = key;
  const byte_t* cc = &key[32];

  byte_t data[1 + 32 + 4] = { 0 };
  byte_t z[64]            = { 0 };
  byte_t c[64]            = { 0 };
  byte_t zl[32]           = { 0 };
  byte_t p[32]            = { 0 };

  CARDANO_UNUSED(memset(data, 0, sizeof(data)));

  cardano_safe_memcpy(&data[1], sizeof(data) - 1U, pk, 32);

  data[0] = 0x02;

  data[1 + 32]     = (byte_t)((uint32_t)index & 0xFFu);
  data[1 + 32 + 1] = (byte_t)(((uint32_t)index >> 8) & 0xFFu);
  data[1 + 32 + 2] = (byte_t)(((uint32_t)index >> 16) & 0xFFu);
  data[1 + 32 + 3] = (byte_t)(((uint32_t)index >> 24) & 0xFFu);

  if (crypto_auth_hmacsha512(z, data, sizeof(data), cc) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  data[0] = 0x03;

  if (crypto_auth_hmacsha512(c, data, sizeof(data), cc) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  byte_t* chain_code = &c[32];

  cardano_safe_memcpy(zl, sizeof(zl), z, 32);

  cardano_error_t error = _cardano_crypto_point_of_trunc28_mul8(zl, p);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  if (crypto_core_ed25519_add(out, p, pk) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  cardano_safe_memcpy(&out[32], out_size - 32U, chain_code, 32);

  return CARDANO_SUCCESS;
}
