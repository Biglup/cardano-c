/**
 * \file base58.c
 *
 * \author angel.castillo
 * \date   Apr 06, 2024
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

#include <cardano/encoding/base58.h>

#include "../allocators.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const char BASE58_ALPHABET[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Finds the index of a character in the Base58 alphabet.
 *
 * \param c The character to find.
 * \param alphabet The Base58 alphabet.
 *
 * \return The index of the character in the alphabet, or -1 if the character is not found.
 */
static int64_t
char_index(const char c, const char* alphabet)
{
  for (size_t i = 0; alphabet[i] != '\0'; ++i)
  {
    if (alphabet[i] == c)
    {
      return (int64_t)i;
    }
  }

  return -1;
}

/* DEFINITIONS ***************************************************************/

size_t
cardano_encoding_base58_get_encoded_length(const byte_t* data, const size_t data_length)
{
  if (data == NULL)
  {
    return 0;
  }

  if (data_length == 0U)
  {
    return 1;
  }

  size_t leading_zeros = 0;

  for (size_t i = 0; (i < data_length) && (data[i] == 0U); ++i)
  {
    ++leading_zeros;
  }

  const double log_ratio       = log(256) / log(58);
  const size_t non_zero_length = data_length - leading_zeros;

  return (size_t)ceil((double)non_zero_length * log_ratio) + leading_zeros + 1U;
}

cardano_error_t
cardano_encoding_base58_encode(
  const byte_t* data,
  const size_t  data_length,
  char*         output,
  const size_t  output_length)
{
  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_length == 0U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t encoded_length = cardano_encoding_base58_get_encoded_length(data, data_length);

  if (output_length < encoded_length)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  uint8_t* temp = (uint8_t*)_cardano_malloc(encoded_length);

  if (!temp)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  CARDANO_UNUSED(memset(temp, 0, encoded_length));

  for (size_t i = 0; i < data_length; i++)
  {
    int64_t carry = data[i];

    for (size_t j = encoded_length - 2U; j < encoded_length; j--)
    {
      carry   += 256 * (int64_t)temp[j];
      temp[j] = carry % 58;
      carry   /= 58;
    }
  }

  size_t output_index = 0;

  for (size_t i = 0; (i < encoded_length) && (temp[i] == 0U); i++)
  {
    output[output_index] = '1';
    ++output_index;
  }

  for (size_t i = 0; i < encoded_length; i++)
  {
    if (temp[i] != 0U)
    {
      output[output_index] = BASE58_ALPHABET[temp[i]];
      ++output_index;
    }
  }

  output[output_index] = '\0';

  _cardano_free(temp);

  return CARDANO_SUCCESS;
}

size_t
cardano_encoding_base58_get_decoded_length(const char* data, const size_t data_length)
{
  if (data == NULL)
  {
    return 0;
  }

  if (data_length == 0U)
  {
    return 0;
  }

  double leading_ones = 0.0;

  for (size_t i = 0; (i < data_length) && (data[i] == '1'); ++i)
  {
    ++leading_ones;
  }

  const double log_base256_of_58       = log(58) / log(256);
  const size_t non_zero_decoded_length = (size_t)floor(((double)data_length - leading_ones) * log_base256_of_58);
  const size_t decoded_length          = (size_t)leading_ones + non_zero_decoded_length;

  return decoded_length;
}

cardano_error_t
cardano_encoding_base58_decode(
  const char* input,
  size_t      input_length,
  byte_t*     data,
  size_t      data_length)
{
  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_length == 0U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t decoded_length = cardano_encoding_base58_get_decoded_length(input, input_length);

  if (data_length < decoded_length)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  CARDANO_UNUSED(memset(data, 0, data_length));

  for (size_t i = 0; i < input_length; ++i)
  {
    int64_t value = char_index(input[i], BASE58_ALPHABET);

    if (value == -1)
    {
      return CARDANO_ERROR_DECODING;
    }

    int64_t carry = value;

    for (int64_t j = (int64_t)decoded_length - 1; j >= 0; --j)
    {
      carry   += (int64_t)58 * (int64_t)data[j];
      data[j] = carry % (int64_t)256U;
      carry   /= (int64_t)256U;
    }
  }

  return CARDANO_SUCCESS;
}
