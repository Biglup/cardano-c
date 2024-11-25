/**
 * \file emip3.c
 *
 * \author angel.castillo
 * \date   Oct 08, 2024
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

#include <cardano/crypto/emip3.h>

#include <cardano/crypto/pbkdf2.h>

#include "../string_safe.h"

#include <sodium.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const uint64_t SALT_LENGTH       = 32U;
static const uint64_t NONCE_LENGTH      = 12U;
static const uint64_t TAG_LENGTH        = crypto_aead_chacha20poly1305_IETF_ABYTES;
static const uint64_t KEY_LENGTH        = 32U;
static const uint64_t PBKDF2_ITERATIONS = 19162U;

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_crypto_emip3_encrypt(
  const byte_t*      data,
  const size_t       data_length,
  const byte_t*      passphrase,
  const size_t       passphrase_length,
  cardano_buffer_t** encrypted_data)
{
  if ((data == NULL) || ((passphrase_length > 0U) && (passphrase == NULL)) || (encrypted_data == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  const size_t required_length = SALT_LENGTH + NONCE_LENGTH + TAG_LENGTH + data_length;
  *encrypted_data              = cardano_buffer_new(required_length);

  if (*encrypted_data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_buffer_set_size(*encrypted_data, required_length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(encrypted_data);

    return result;
  }

  byte_t* output     = cardano_buffer_get_data(*encrypted_data);
  byte_t* salt       = output;
  byte_t* nonce      = &output[SALT_LENGTH];
  byte_t* tag        = &nonce[NONCE_LENGTH];
  byte_t* ciphertext = &tag[TAG_LENGTH];

  randombytes_buf(salt, SALT_LENGTH);

  byte_t key[32U] = { 0 };

  result = cardano_crypto_pbkdf2_hmac_sha512(
    passphrase, passphrase_length, salt, SALT_LENGTH, (uint32_t)PBKDF2_ITERATIONS, key, KEY_LENGTH);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(encrypted_data);
    sodium_memzero(key, KEY_LENGTH);

    return result;
  }

  randombytes_buf(nonce, NONCE_LENGTH);

  unsigned char tag_output[16] = { 0 };

  if (crypto_aead_chacha20poly1305_ietf_encrypt_detached(ciphertext, tag_output, NULL, data, data_length, NULL, 0, NULL, nonce, key) != 0)
  {
    cardano_buffer_unref(encrypted_data);
    sodium_memzero(key, KEY_LENGTH);

    return CARDANO_ERROR_GENERIC;
  }

  cardano_safe_memcpy(tag, TAG_LENGTH, tag_output, TAG_LENGTH);

  sodium_memzero(key, KEY_LENGTH);

  return result;
}

cardano_error_t
cardano_crypto_emip3_decrypt(
  const byte_t*      encrypted_data,
  const size_t       encrypted_data_length,
  const byte_t*      passphrase,
  const size_t       passphrase_length,
  cardano_buffer_t** data)
{
  if (encrypted_data_length < (SALT_LENGTH + NONCE_LENGTH + TAG_LENGTH))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  const size_t ciphertext_len = encrypted_data_length - (SALT_LENGTH + NONCE_LENGTH + TAG_LENGTH);

  *data = cardano_buffer_new(ciphertext_len);

  if (*data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_buffer_set_size(*data, ciphertext_len);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(data);

    return result;
  }

  const byte_t* salt       = encrypted_data;
  const byte_t* nonce      = &encrypted_data[SALT_LENGTH];
  const byte_t* tag        = &nonce[NONCE_LENGTH];
  const byte_t* ciphertext = &tag[TAG_LENGTH];

  byte_t key[32U] = { 0 };

  result = cardano_crypto_pbkdf2_hmac_sha512(
    passphrase, passphrase_length, salt, SALT_LENGTH, (uint32_t)PBKDF2_ITERATIONS, key, KEY_LENGTH);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(data);

    return result;
  }

  if (crypto_aead_chacha20poly1305_ietf_decrypt_detached(cardano_buffer_get_data(*data), NULL, ciphertext, ciphertext_len, tag, NULL, 0, nonce, key) != 0)
  {
    cardano_buffer_unref(data);
    sodium_memzero(key, KEY_LENGTH);

    return CARDANO_ERROR_GENERIC;
  }

  sodium_memzero(key, KEY_LENGTH);

  return CARDANO_SUCCESS;
}