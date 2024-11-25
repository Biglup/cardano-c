/**
 * \file bip39.c
 *
 * \author angel.castillo
 * \date   Nov 22, 2024
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

#include <cardano/bip39.h>

#include "bip39_wordlist_en.h"

#include "../string_safe.h"
#include <assert.h>
#include <sodium.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t BITS_PER_WORD = 11U;

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_bip39_entropy_to_mnemonic_words(
  const byte_t* entropy,
  const size_t  entropy_size,
  const char**  words,
  size_t*       word_count)
{
  if (!entropy || !words || !word_count)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((entropy_size != 16U) && (entropy_size != 20U) && (entropy_size != 24U) && (entropy_size != 28U) && (entropy_size != 32U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  size_t checksum_bits = (entropy_size * 8U) / 32U;
  size_t total_bits    = (entropy_size * 8U) + checksum_bits;
  *word_count          = total_bits / BITS_PER_WORD;

  byte_t entropy_with_checksum[33] = { 0 };
  cardano_safe_memcpy(entropy_with_checksum, sizeof(entropy_with_checksum), entropy, entropy_size);

  byte_t hash[32] = { 0 };
  CARDANO_UNUSED(crypto_hash_sha256(hash, entropy, entropy_size));

  if (checksum_bits > 0U)
  {
    entropy_with_checksum[entropy_size] = hash[0];
  }

  byte_t bitstream[33] = { 0 };
  cardano_safe_memcpy(bitstream, sizeof(bitstream), entropy_with_checksum, entropy_size + 1U);

  for (size_t i = 0U; i < *word_count; ++i)
  {
    uint16_t index = 0U;

    for (size_t j = 0U; j < BITS_PER_WORD; ++j)
    {
      size_t bit_index   = (i * BITS_PER_WORD) + j;
      size_t byte_index  = bit_index / 8U;
      size_t bit_in_byte = 7U - (bit_index % 8U);

      if ((bitstream[byte_index] & (1U << bit_in_byte)) > 0U)
      {
        index |= (uint16_t)(1U << (BITS_PER_WORD - 1U - j));
      }
    }

    assert(index < 2048U);
    words[i] = BIP39_WORDLIST_EN[index];
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bip39_mnemonic_words_to_entropy(
  const char** words,
  const size_t word_count,
  byte_t*      entropy,
  const size_t entropy_buf_size,
  size_t*      entropy_size)
{
  if (!words || !entropy || !entropy_size)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((word_count != 12U) && (word_count != 15U) && (word_count != 18U) && (word_count != 21U) && (word_count != 24U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  size_t checksum_bits = word_count / 3U;
  size_t entropy_bits  = (word_count * BITS_PER_WORD) - checksum_bits;
  size_t entropy_bytes = entropy_bits / 8U;

  if (entropy_buf_size < entropy_bytes)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  size_t total_bits    = word_count * BITS_PER_WORD;
  size_t bitstram_size = (total_bits + 7U) / 8U;
  byte_t bitstream[34] = { 0 };

  CARDANO_UNUSED(memset(bitstream, 0U, bitstram_size));

  for (size_t i = 0U; i < word_count; i++)
  {
    int32_t index = -1;
    for (int32_t j = 0; j < 2048; ++j)
    {
      if (strcmp(words[i], BIP39_WORDLIST_EN[j]) == 0)
      {
        index = j;
        break;
      }
    }
    if (index == -1)
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }

    for (size_t k = 0U; k < BITS_PER_WORD; ++k)
    {
      size_t shift_amount = 10U - k;
      size_t mask         = ((size_t)1) << shift_amount;

      if (((uint32_t)index & (uint32_t)mask) != 0U)
      {
        size_t bit_index   = (i * BITS_PER_WORD) + k;
        size_t byte_index  = bit_index / 8U;
        byte_t bit_in_byte = (byte_t)(7U - (bit_index % 8U));

        bitstream[byte_index] |= (1U << bit_in_byte);
      }
    }
  }

  cardano_safe_memcpy(entropy, entropy_buf_size, bitstream, entropy_bytes);

  *entropy_size = entropy_bytes;

  byte_t hash[32];
  crypto_hash_sha256(hash, entropy, entropy_bytes);

  byte_t calculated_checksum = hash[0] >> (8U - checksum_bits);

  byte_t extracted_checksum = 0U;
  size_t checksum_start_bit = entropy_bits;

  for (size_t i = 0U; i < checksum_bits; i++)
  {
    size_t bit_index   = checksum_start_bit + i;
    size_t byte_index  = bit_index / 8U;
    size_t bit_in_byte = 7U - (bit_index % 8U);

    if ((bitstream[byte_index] & (1U << bit_in_byte)) > 0U)
    {
      extracted_checksum |= (byte_t)(1U << (checksum_bits - 1U - i));
    }
  }

  if (calculated_checksum != extracted_checksum)
  {
    return CARDANO_ERROR_INVALID_CHECKSUM;
  }

  return CARDANO_SUCCESS;
}