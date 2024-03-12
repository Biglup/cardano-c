/**
 * \file pbkdf2.c
 *
 * \author angel.castillo
 * \date   Mar 02, 2024
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

#include <cardano/cryptography/pbkdf2.h>

#include <assert.h>
#include <sodium.h>
#include <string.h>

#include "../config.h"
#include "../endian.h"

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_cryptography_pbkdf2_hmac_sha512(
  const byte_t*  password,
  const size_t   password_length,
  const byte_t*  salt,
  const size_t   salt_length,
  const uint32_t iterations,
  byte_t*        derived_key,
  const size_t   derived_key_length)
{
  if ((password == NULL) || (salt == NULL) || (derived_key == NULL))
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if ((password_length == 0U) || (salt_length == 0U) || (derived_key_length == 0U))
  {
    return CARDANO_INSUFFICIENT_BUFFER_SIZE;
  }

  crypto_auth_hmacsha512_state initial_hmac_state   = { 0 };
  crypto_auth_hmacsha512_state loop_hHmac_state     = { 0 };
  byte_t                       iteration_vector[4U] = { 0 };
  byte_t                       temp_digest[64U]     = { 0 };
  byte_t                       block_digest[64U]    = { 0 };

  crypto_auth_hmacsha512_init(&initial_hmac_state, password, password_length);
  crypto_auth_hmacsha512_update(&initial_hmac_state, salt, salt_length);

  for (size_t i = 0; (i * crypto_auth_hmacsha512_BYTES) < derived_key_length; ++i)
  {
    size_t current_block_length = 0;

    cardano_error_t result = cardano_write_uint32_be((uint32_t)(i + 1U), iteration_vector, sizeof(iteration_vector), 0);

    (void)result;
    assert(result == CARDANO_SUCCESS);

    void* initial_state_copy_result = memcpy(&loop_hHmac_state, &initial_hmac_state, sizeof(crypto_auth_hmacsha512_state));
    // Satisfies misra-c2012-17.7. memcpy doesn't return any useful feedback, so it's safe to ignore.
    (void)initial_state_copy_result;

    crypto_auth_hmacsha512_update(&loop_hHmac_state, iteration_vector, sizeof(iteration_vector));
    crypto_auth_hmacsha512_final(&loop_hHmac_state, temp_digest);

    void* tmp_copy_result = memcpy(block_digest, temp_digest, crypto_auth_hmacsha512_BYTES);
    // Satisfies misra-c2012-17.7. memcpy doesn't return any useful feedback, so it's safe to ignore.
    (void)tmp_copy_result;

    for (size_t j = 2; j <= iterations; ++j)
    {
      crypto_auth_hmacsha512_init(&loop_hHmac_state, password, password_length);
      crypto_auth_hmacsha512_update(&loop_hHmac_state, temp_digest, crypto_auth_hmacsha512_BYTES);
      crypto_auth_hmacsha512_final(&loop_hHmac_state, temp_digest);

      for (size_t k = 0; k < crypto_auth_hmacsha512_BYTES; ++k)
      {
        block_digest[k] ^= temp_digest[k];
      }
    }

    current_block_length = derived_key_length - (i * crypto_auth_hmacsha512_BYTES);

    if (current_block_length > crypto_auth_hmacsha512_BYTES)
    {
      current_block_length = crypto_auth_hmacsha512_BYTES;
    }

    void* block_digest_copy_result = memcpy(&derived_key[i * crypto_auth_hmacsha512_BYTES], block_digest, current_block_length);
    // Satisfies misra-c2012-17.7. memcpy doesn't return any useful feedback, so it's safe to ignore.
    (void)block_digest_copy_result;
  }

  sodium_memzero((void*)&initial_hmac_state, sizeof(initial_hmac_state));
  sodium_memzero((void*)&loop_hHmac_state, sizeof(loop_hHmac_state));
  sodium_memzero((void*)iteration_vector, sizeof(iteration_vector));
  sodium_memzero((void*)temp_digest, sizeof(temp_digest));
  sodium_memzero((void*)block_digest, sizeof(block_digest));

  return CARDANO_SUCCESS;
}
