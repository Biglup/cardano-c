/**
 * \file bech32.c
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

#include "../allocators.h"
#include "../string_safe.h"
#include <cardano/encoding/bech32.h>

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t BECH32_CHECKSUM_LENGTH = 6;
static const char   BECH32_SEPARATOR       = '1';
static const size_t NULL_TERMINATOR_LENGTH = 1;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Finds the index of a character in a string.
 *
 * \param[in] data The data to search.
 * \param[in] length The length of the data.
 * \param[in] separator The character to find.
 *
 * \return The index of the separator character in the data, or -1 if the separator is not found.
 */
static int32_t
get_index_of(const char* data, const size_t length, const char separator)
{
  assert(data != NULL);
  assert(length > 0U);

  int32_t index = -1;

  for (size_t i = length - 1U; i > 0U; --i)
  {
    if (data[i] == separator)
    {
      index = (int32_t)i;
      break;
    }
  }

  return index;
}

/**
 * \brief Calculate the number of 5-bit groups needed to represent the data.
 * This is data_length_8bit * 8 (bits) / 5 (bits per group), rounded up to the nearest whole number.
 *
 * \param[in] data_length_8bit The length of the data in 8-bit groups.
 *
 * \return The length of the data in 5-bit groups.
 */
static size_t
convert_8bit_to_5bit_length(const size_t data_length_8bit)
{
  return ((data_length_8bit * 8U) + 4U) / 5U;
}

/**
 * \brief Calculate the number of bytes needed to represent the data in 5-bit groups.
 * This is data_length_5bit * 5 (bits) / 8 (bits per byte), rounded down to the nearest whole number.
 *
 * \param[in] data_length_5bit The length of the data in 5-bit groups.
 *
 * \return The number of bytes needed to represent the data in 5-bit groups.
 */
static size_t
convert_5bit_to_8bit_length(const size_t data_length_5bit)
{
  return (data_length_5bit * 5U) / 8U;
}

/**
 * \brief Converts a string to lowercase.
 *
 * \param[out] dest Destination string where the lowercase string will be stored.
 * \param[in] src Source string to be converted to lowercase.
 * \param[in] length Length of the source string.
 */
static void
to_lower_string(char* dest, const char* src, const size_t length)
{
  assert(dest != NULL);

  for (size_t i = 0; i < length; ++i)
  {
    dest[i] = (char)tolower((int)src[i]);
  }

  dest[length] = '\0';
}

/**
 * \brief Converts a string to uppercase.
 *
 * \param[out] dest Destination string where the uppercase string will be stored.
 * \param[in] src Source string to be converted to uppercase.
 * \param[in] length Length of the source string.
 */
static void
to_upper_string(char* dest, const char* src, const size_t length)
{
  assert(dest != NULL);

  for (size_t i = 0; i < length; ++i)
  {
    dest[i] = (char)toupper((int)src[i]);
  }

  dest[length] = '\0';
}

/**
 * \brief PolyMod takes a byte slice and returns the 32-bit BCH checksum.
 *
 * Note that the input bytes to poly_mod need to be squashed to 5-bits tall
 * before being used in this function.  And this function will not error,
 * but instead return an unstable checksum, if you give it full-height bytes.
 *
 * \param[in] values The values to get the checksum from.
 * \param[in] length The length of the values array.
 *
 * \return The checksum.
 */
static uint32_t
poly_mod(const byte_t* values, const size_t length)
{
  assert(values != NULL);

  static const uint32_t GENERATOR[] = { 0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3 };

  uint32_t checksum = 1;

  for (size_t i = 0; i < length; ++i)
  {
    byte_t top = (byte_t)(checksum >> 25);

    checksum = ((checksum & 0x1ffffffU) << 5U) ^ values[i];

    for (size_t j = 0; j < 5U; ++j)
    {
      if (((top >> j) & 1U) == 1U)
      {
        checksum ^= GENERATOR[j];
      }
    }
  }

  return checksum;
}

/**
 * \brief Checks if a string contains mixed case letters and formats it to lowercase.
 *
 * \param[out] dest Destination string where the lowercase address will be stored.
 * \param[in] address The address to be verified.
 * \param[in] length Length of the address string.
 *
 * \return 0 if successful, 1 if the address contains mixed case.
 */
static cardano_error_t
check_and_format(char* dest, const char* address, const size_t length)
{
  assert(dest != NULL);
  assert(address != NULL);

  char* low_addr = (char*)_cardano_malloc(length + NULL_TERMINATOR_LENGTH);

  if (low_addr == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  char* high_addr = (char*)_cardano_malloc(length + NULL_TERMINATOR_LENGTH);

  if (high_addr == NULL)
  {
    _cardano_free(low_addr);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  to_lower_string(low_addr, address, length);
  to_upper_string(high_addr, address, length);

  if ((strncmp(address, low_addr, length) != 0) && (strncmp(address, high_addr, length) != 0))
  {
    _cardano_free(low_addr);
    _cardano_free(high_addr);

    return CARDANO_ERROR_DECODING;
  }

  cardano_safe_memcpy(dest, length, low_addr, length);
  dest[length] = '\0';

  _cardano_free(low_addr);
  _cardano_free(high_addr);

  return CARDANO_SUCCESS;
}

/**
 * \brief Expands the human readable part into 5bit-bytes for later processing.
 *
 * \param[in] input The human readable part to be expanded.
 * \param[in] input_length The length of the input string.
 * \param[out] output A pointer to the pointer of the output array, where the result will be stored.
 * \param[out] output_length A pointer to a size_t variable, where the length of the output array will be stored.
 */
static void
hrp_expand(const char* input, const size_t input_length, byte_t** output, size_t* output_length)
{
  assert(input != NULL);
  assert(output != NULL);
  assert(output_length != NULL);

  *output_length = (input_length * 2U) + 1U;
  *output        = (byte_t*)_cardano_malloc(*output_length * sizeof(byte_t));

  if (*output == NULL)
  {
    return;
  }

  CARDANO_UNUSED(memset(*output, 0, *output_length));

  for (size_t i = 0; i < input_length; ++i)
  {
    const byte_t khar = (byte_t)input[i];
    (*output)[i]      = khar >> 5;
  }

  for (size_t i = 0; i < input_length; ++i)
  {
    const byte_t khar                = (byte_t)input[i];
    (*output)[i + input_length + 1U] = khar & 0x1FU;
  }
}

/**
 * \brief Verifies the checksum of the address.
 *
 * \param[in] hrp The human readable part of the address.
 * \param[in] hrp_length The length of the hrp string.
 * \param[in] data The data part of the address.
 * \param[in] data_length The length of the data array.
 *
 * \return \c true if the checksum is valid; otherwise; \c false.
 */
static bool
verify_checksum(const char* hrp, const size_t hrp_length, const byte_t* data, const size_t data_length)
{
  size_t  expanded_hrp_length = 0;
  byte_t* expanded_hrp        = NULL;

  assert(hrp != NULL);
  assert(data != NULL);

  hrp_expand(hrp, hrp_length, &expanded_hrp, &expanded_hrp_length);

  if (expanded_hrp == NULL)
  {
    return false;
  }

  const size_t values_length = expanded_hrp_length + data_length;
  byte_t*      values        = (byte_t*)_cardano_malloc(values_length);

  if (values == NULL)
  {
    _cardano_free(expanded_hrp);
    return false;
  }

  cardano_safe_memcpy(values, values_length, expanded_hrp, expanded_hrp_length);
  cardano_safe_memcpy(&values[expanded_hrp_length], values_length - expanded_hrp_length, data, data_length);

  const byte_t checksum = poly_mod(values, values_length);

  _cardano_free(expanded_hrp);
  _cardano_free(values);

  return checksum == 1U;
}

/**
 * Creates an address checksum.
 *
 * \param[in] hrp The human readable part of the address.
 * \param[in] hrp_length The length of the hrp string.
 * \param[in] data The data part of the address.
 * \param[in] data_length The length of the data array.
 * \param[out] checksum Output parameter for the checksum. The caller is responsible for allocating memory for 6 bytes.
 */
static cardano_error_t
create_checksum(const char* hrp, size_t hrp_length, const byte_t* data, const size_t data_length, byte_t* checksum)
{
  size_t  expanded_hrp_length = 0;
  byte_t* expanded_hrp        = NULL;

  assert(hrp != NULL);
  assert(checksum != NULL);

  hrp_expand(hrp, hrp_length, &expanded_hrp, &expanded_hrp_length);

  if (expanded_hrp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const size_t values_length = expanded_hrp_length + data_length + BECH32_CHECKSUM_LENGTH;
  byte_t*      values        = (byte_t*)_cardano_malloc(values_length * sizeof(byte_t));

  if (values == NULL)
  {
    _cardano_free(expanded_hrp);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(values, values_length, expanded_hrp, expanded_hrp_length);
  cardano_safe_memcpy(&values[expanded_hrp_length], values_length - expanded_hrp_length, data, data_length);

  CARDANO_UNUSED(memset(&values[expanded_hrp_length + data_length], 0, BECH32_CHECKSUM_LENGTH));

  const uint32_t poly_checksum = poly_mod(values, values_length) ^ 1U;

  for (size_t i = 0; i < BECH32_CHECKSUM_LENGTH; i++)
  {
    checksum[i] = (poly_checksum >> (5U * (5U - i))) & 0x1fU;
  }

  _cardano_free(expanded_hrp);
  _cardano_free(values);

  return CARDANO_SUCCESS;
}

/**
 * Converts a string to an array of squashed bytes.
 *
 * \param[in] input The string to be squashed.
 * \param[in] input_length The length of the input string.
 * \param[out] output A pointer to the pointer of the output array where the squashed bytes will be stored.
 * \param[out] output_length A pointer to a size_t variable where the length of the output array will be stored.
 *
 * \return 0 if successful, non-zero if the string contains an invalid character.
 */
static cardano_error_t
string_to_squashed_bytes(const char* input, const size_t input_length, byte_t** output, size_t* output_length)
{
  assert(input != NULL);
  assert(output != NULL);
  assert(output_length != NULL);

  // clang-format off

  static const int16_t
  ICHARSET[] =
  {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
    1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
    1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
  };

  // clang-format on

  byte_t* tmp = (byte_t*)_cardano_malloc(input_length * sizeof(byte_t));

  if (tmp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0; i < input_length; ++i)
  {
    unsigned char khar   = input[i];
    short         buffer = ICHARSET[khar];

    if (buffer == -1)
    {
      _cardano_free(tmp);
      *output = NULL;

      return CARDANO_ERROR_DECODING;
    }

    tmp[i] = (byte_t)buffer;
  }

  *output_length = input_length;
  *output        = tmp;

  return CARDANO_SUCCESS;
}

/**
 * Decodes a bech32 string into a squashed byte array.
 *
 * \param[in] address The bech32 address to be decoded.
 * \param[in] address_length Length of the bech32 address.
 * \param[out] hrp Output buffer for the human-readable part.
 * \param[in] hrp_length Length of the buffer allocated for hrp.
 * \param[out] data Output buffer for the decoded data.
 * \param[out] data_length Pointer to store the length of the decoded data.
 *
 * \return 0 if successful, non-zero if the bech32 string is invalid.
 */
static cardano_error_t
decode_squashed(const char* address, const size_t address_length, char* hrp, const size_t hrp_length, byte_t** data, size_t* data_length)
{
  assert(address != NULL);
  assert(hrp != NULL);
  assert(data != NULL);
  assert(*data == NULL);
  assert(data_length != NULL);

  char* formatted_address = _cardano_malloc(address_length + NULL_TERMINATOR_LENGTH);

  if (formatted_address == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if (check_and_format(formatted_address, address, address_length) != CARDANO_SUCCESS)
  {
    _cardano_free(formatted_address);
    return CARDANO_ERROR_DECODING;
  }

  int32_t split_loc = get_index_of(formatted_address, address_length, BECH32_SEPARATOR);

  if (split_loc == -1)
  {
    _cardano_free(formatted_address);
    return CARDANO_ERROR_DECODING;
  }

  if ((size_t)split_loc >= hrp_length)
  {
    _cardano_free(formatted_address);
    return CARDANO_ERROR_DECODING;
  }

  cardano_safe_memcpy(hrp, hrp_length, formatted_address, split_loc);

  hrp[split_loc]          = '\0';
  byte_t* squashed        = NULL;
  size_t  squashed_length = 0;

  if (string_to_squashed_bytes(&formatted_address[split_loc + 1], address_length - (size_t)split_loc - 1U, &squashed, &squashed_length) != CARDANO_SUCCESS)
  {
    _cardano_free(formatted_address);
    return CARDANO_ERROR_DECODING;
  }

  if (!verify_checksum(hrp, cardano_safe_strlen(hrp, split_loc), squashed, squashed_length))
  {
    _cardano_free(formatted_address);
    _cardano_free(squashed);

    return CARDANO_ERROR_DECODING;
  }

  *data_length = squashed_length - BECH32_CHECKSUM_LENGTH;
  *data        = _cardano_malloc(*data_length);

  if (*data == NULL)
  {
    _cardano_free(formatted_address);
    _cardano_free(squashed);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(*data, *data_length, squashed, *data_length);

  _cardano_free(formatted_address);
  _cardano_free(squashed);

  return CARDANO_SUCCESS;
}

/**
 * Converts a squashed byte array to a string.
 *
 * \param[in] input The squashed byte array to be converted.
 * \param[in] input_length The length of the squashed byte array.
 * \param[out] output Pointer to a char* that will point to the newly created string.
 * \param[out] output_length Pointer to a size_t that will store the length of the output string.
 *
 * \return 0 on success, non-zero error code on failure.
 */
static cardano_error_t
squashed_bytes_to_string(const byte_t* input, const size_t input_length, char** output, size_t* output_length)
{
  assert(input != NULL);
  assert(output != NULL);
  assert(*output == NULL);

  static const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

  *output = _cardano_malloc(input_length + NULL_TERMINATOR_LENGTH);

  if (*output == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0; i < input_length; ++i)
  {
    assert((input[i] & 0xE0U) == 0U);

    (*output)[i] = CHARSET[input[i]];
  }

  (*output)[input_length] = '\0';
  *output_length          = input_length;

  return CARDANO_SUCCESS;
}

/**
 * Encodes a squashed byte array into a bech32 string.
 *
 * \param[in] hrp The human-readable part.
 * \param[in] hrp_length Length of the human-readable part.
 * \param[in] data The squashed data.
 * \param[in] data_length Length of the squashed data.
 * \param[out] output Pointer to the char* that will hold the bech32 encoded string.
 * \param[in] output_length Length of the output buffer.
 *
 * \return 0 on success, non-zero error code on failure. Caller is responsible for freeing the output string.
 */
static cardano_error_t
encode_squashed(
  const char*   hrp,
  const size_t  hrp_length,
  const byte_t* data,
  const size_t  data_length,
  char*         output,
  const size_t  output_length)
{
  assert(hrp != NULL);
  assert(output != NULL);

  byte_t* checksum = (byte_t*)_cardano_malloc(BECH32_CHECKSUM_LENGTH);

  if (checksum == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = create_checksum(hrp, hrp_length, data, data_length, checksum);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(checksum);
    return result;
  }

  size_t  combined_length = data_length + BECH32_CHECKSUM_LENGTH;
  byte_t* combined        = _cardano_malloc(combined_length);

  if (combined == NULL)
  {
    _cardano_free(checksum);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(combined, combined_length, data, data_length);
  cardano_safe_memcpy(&combined[data_length], combined_length - data_length, checksum, BECH32_CHECKSUM_LENGTH);

  char*  encoded        = NULL;
  size_t encoded_length = 0U;
  result                = squashed_bytes_to_string(combined, combined_length, &encoded, &encoded_length);

  _cardano_free(combined);
  _cardano_free(checksum);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  size_t required_output_length = hrp_length + NULL_TERMINATOR_LENGTH + cardano_safe_strlen(encoded, encoded_length);

  if (output_length < required_output_length)
  {
    _cardano_free(encoded);
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  int32_t format_result = snprintf(output, output_length, "%s1%s", hrp, encoded);

  _cardano_free(encoded);

  if (format_result < 0)
  {
    return CARDANO_ERROR_ENCODING;
  }

  return CARDANO_SUCCESS;
}

/**
 * Converts full-width (8-bit) bytes into "squashed" 5-bit bytes, and vice versa.
 *
 * \param[in] input The data to be squashed.
 * \param[in] input_length The length of the input data.
 * \param[in] input_width The width of the input bytes.
 * \param[in] output_width The width of the output bytes.
 * \param[out] output A pointer to a buffer that will hold the squashed data. This buffer should be allocated by the caller.
 * \param[out] output_length The length of the output data (number of bytes).
 *
 * \return CARDANO_SUCCESS on success, an error code on failure.
 */
static cardano_error_t
byte_squasher(const byte_t* input, const size_t input_length, const size_t input_width, const size_t output_width, byte_t** output, size_t* output_length)
{
  assert(output != NULL);
  assert(*output == NULL);
  assert(output_length != NULL);

  if ((input == NULL) || (input_length == 0U))
  {
    *output_length = 0;
    return CARDANO_SUCCESS;
  }

  size_t         bit_stash        = 0;
  uint32_t       accumulator      = 0;
  const uint32_t max_output_value = ((uint32_t)1U << (uint32_t)output_width) - 1U;

  const size_t estimated_output_size = ((input_length * input_width) + (output_width - 1U)) / output_width;
  *output                            = _cardano_malloc(estimated_output_size);

  if (*output == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  size_t current_output_length = 0;

  for (size_t i = 0; i < input_length; ++i)
  {
    assert((input[i] >> input_width) == 0U);

    accumulator = (accumulator << input_width) | input[i];
    bit_stash   += input_width;

    while (bit_stash >= output_width)
    {
      bit_stash                        -= output_width;
      (*output)[current_output_length] = (accumulator >> bit_stash) & max_output_value;
      ++current_output_length;
    }
  }

  if (input_width == 8U)
  {
    if (bit_stash != 0U)
    {
      (*output)[current_output_length] = (accumulator << (output_width - bit_stash)) & max_output_value;
      ++current_output_length;
    }
  }
  else
  {
    if ((bit_stash >= input_width) || (((accumulator << (output_width - bit_stash)) & max_output_value) != 0U))
    {
      _cardano_free(*output);
      *output = NULL;

      return CARDANO_ERROR_ENCODING;
    }
  }

  *output_length = current_output_length;

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ***************************************************************/

size_t
cardano_encoding_bech32_get_encoded_length(
  const char*   hrp,
  const size_t  hrp_length,
  const byte_t* data,
  const size_t  data_length)
{
  static const size_t BECH32_SEPARATOR_LENGTH = 1;

  CARDANO_UNUSED(hrp);
  CARDANO_UNUSED(data);

  return hrp_length +
    BECH32_SEPARATOR_LENGTH +
    convert_8bit_to_5bit_length(data_length) +
    BECH32_CHECKSUM_LENGTH +
    NULL_TERMINATOR_LENGTH;
}

cardano_error_t
cardano_encoding_bech32_encode(
  const char*   hrp,
  const size_t  hrp_length,
  const byte_t* data,
  const size_t  data_length,
  char*         output,
  const size_t  output_length)
{
  if (hrp == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((data == NULL) && (data_length > 0U))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (output_length == 0U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  byte_t* squashed_data   = NULL;
  size_t  squashed_length = 0;

  cardano_error_t result = byte_squasher(data, data_length, 8, 5, &squashed_data, &squashed_length);

  if (result != CARDANO_SUCCESS)
  {
    if (squashed_data != NULL)
    {
      _cardano_free(squashed_data);
    }

    return result;
  }

  result = encode_squashed(hrp, hrp_length, squashed_data, squashed_length, output, output_length);

  _cardano_free(squashed_data);

  return result;
}

size_t
cardano_encoding_bech32_get_decoded_length(
  const char*  data,
  const size_t data_length,
  size_t*      hrp_length)
{
  if (data == NULL)
  {
    return 0;
  }

  if (data_length == 0U)
  {
    return 0;
  }

  if (hrp_length == NULL)
  {
    return 0;
  }

  *hrp_length = 0;

  int32_t separator_index = get_index_of(data, data_length, BECH32_SEPARATOR);

  if (separator_index == -1)
  {
    return 0;
  }

  *hrp_length = separator_index;

  if (data_length < (*hrp_length + NULL_TERMINATOR_LENGTH + BECH32_CHECKSUM_LENGTH))
  {
    return 0;
  }

  size_t data_part_length = data_length - *hrp_length - NULL_TERMINATOR_LENGTH - BECH32_CHECKSUM_LENGTH;

  size_t decoded_data_length = convert_5bit_to_8bit_length(data_part_length);

  *hrp_length += NULL_TERMINATOR_LENGTH;

  return decoded_data_length;
}

cardano_error_t
cardano_encoding_bech32_decode(
  const char*  input,
  const size_t input_length,
  char*        hrp,
  const size_t hrp_length,
  byte_t*      data,
  const size_t data_length)
{
  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hrp == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t* squashed_data   = NULL;
  size_t  squashed_length = 0;

  cardano_error_t result = decode_squashed(input, input_length, hrp, hrp_length, &squashed_data, &squashed_length);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  byte_t* output_data   = NULL;
  size_t  output_length = 0;

  result = byte_squasher((byte_t*)squashed_data, squashed_length, 5, 8, &output_data, &output_length);

  _cardano_free(squashed_data);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (output_length > data_length)
  {
    _cardano_free(output_data);
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  cardano_safe_memcpy(data, data_length, output_data, output_length);

  _cardano_free(output_data);

  return result;
}