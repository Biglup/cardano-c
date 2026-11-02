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

/**
 * \brief Converts Base58 digits into a caller-owned buffer and reports their exact byte count.
 *
 * \param input The validated non-null input pointer.
 * \param input_length The number of input characters.
 * \param data The validated non-null destination pointer.
 * \param data_length The nonzero destination capacity.
 * \param decoded_length Receives the exact byte count on success.
 *
 * \return Success, a decoding error, or an insufficient-buffer error.
 */
static cardano_error_t
base58_decode_into(const char* input, const size_t input_length, byte_t* data, const size_t data_length, size_t* decoded_length)
{
  size_t leading_ones = 0U;

  while ((leading_ones < input_length) && (input[leading_ones] == '1'))
  {
    ++leading_ones;
  }

  CARDANO_UNUSED(memset(data, 0, data_length));

  for (size_t i = leading_ones; i < input_length; ++i)
  {
    const int64_t value = char_index(input[i], BASE58_ALPHABET);

    if (value < 0)
    {
      return CARDANO_ERROR_DECODING;
    }

    uint32_t carry = (uint32_t)value;

    for (size_t j = data_length; j > 0U; --j)
    {
      carry        += 58U * (uint32_t)data[j - 1U];
      data[j - 1U] = (byte_t)(carry % 256U);
      carry        /= 256U;
    }

    if (carry != 0U)
    {
      return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
    }
  }

  size_t first_nonzero = 0U;

  while ((first_nonzero < data_length) && (data[first_nonzero] == 0U))
  {
    ++first_nonzero;
  }

  if (leading_ones > first_nonzero)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  const size_t magnitude_length = data_length - first_nonzero;
  *decoded_length               = leading_ones + magnitude_length;

  CARDANO_UNUSED(memmove(&data[leading_ones], &data[first_nonzero], magnitude_length));
  CARDANO_UNUSED(memset(data, 0, leading_ones));
  CARDANO_UNUSED(memset(&data[*decoded_length], 0, data_length - *decoded_length));

  return CARDANO_SUCCESS;
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

  const size_t non_zero_length = data_length - leading_zeros;

  if (data_length > ((SIZE_MAX - 1U) / 2U))
  {
    return 0U;
  }

  // 1.38 exceeds log(256) / log(58); round the integer estimate upward.
  const size_t digits = ((non_zero_length / 100U) * 138U) + ((((non_zero_length % 100U) * 138U) + 99U) / 100U);

  return digits + leading_zeros + 1U;
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

  if ((encoded_length == 0U) || (output_length < encoded_length))
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

  size_t output_index = 0U;

  while ((output_index < data_length) && (data[output_index] == 0U))
  {
    output[output_index] = '1';
    ++output_index;
  }

  size_t first_digit = 0U;

  while ((first_digit < (encoded_length - 1U)) && (temp[first_digit] == 0U))
  {
    ++first_digit;
  }

  for (size_t i = first_digit; i < (encoded_length - 1U); ++i)
  {
    output[output_index] = BASE58_ALPHABET[temp[i]];
    ++output_index;
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

  byte_t* decoded = _cardano_malloc(data_length);

  if (decoded == NULL)
  {
    return 0U;
  }

  size_t                decoded_length = 0U;
  const cardano_error_t result         = base58_decode_into(data, data_length, decoded, data_length, &decoded_length);

  _cardano_free(decoded);

  return (result == CARDANO_SUCCESS) ? decoded_length : 0U;
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

  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0U; i < input_length; ++i)
  {
    if (char_index(input[i], BASE58_ALPHABET) < 0)
    {
      return CARDANO_ERROR_DECODING;
    }
  }

  if (data_length == 0U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  size_t decoded_length = 0U;

  return base58_decode_into(input, input_length, data, data_length, &decoded_length);
}
