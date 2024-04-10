/**
 * \file base58.cpp
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
#include <gmock/gmock.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

/* STATIC FUNCTIONS **********************************************************/

static bool
encodes_correctly(const char* encoded, const uint8_t* data, size_t data_size)
{
  size_t encoded_size = cardano_encoding_base58_get_encoded_length(data, data_size);
  char*  result       = (char*)malloc(encoded_size);

  cardano_error_t encode_result = cardano_encoding_base58_encode(data, data_size, result, encoded_size);

  if (encode_result != CARDANO_SUCCESS)
  {
    free(result);
    return false;
  }

  EXPECT_EQ(strlen(result), strlen(encoded));
  EXPECT_STREQ(result, encoded);

  bool is_equal = strcmp(result, encoded) == 0;

  free(result);

  return is_equal;
}

static bool
decodes_correctly(const char* encoded, const uint8_t* data, size_t data_size)
{
  size_t   decoded_size = cardano_encoding_base58_get_decoded_length(encoded, strlen(encoded));
  uint8_t* result       = (uint8_t*)malloc(decoded_size);

  cardano_error_t decode_result = cardano_encoding_base58_decode(encoded, strlen(encoded), result, decoded_size);

  if (decode_result != CARDANO_SUCCESS)
  {
    free(result);
    return false;
  }

  EXPECT_EQ(decoded_size, data_size);
  EXPECT_EQ(memcmp(result, data, data_size), 0);

  const bool is_equal = memcmp(result, data, data_size) == 0;

  free(result);

  return is_equal;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_encoding_base58_encode, canDecodeBase58Strings)
{
  // Arrange

  // clang-format off
  const uint8_t byron_mainnet_yoroi[] = {
    0x82, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x3c, 0x42, 0xcc, 0xdd, 0xa0, 0x00, 0x1a, 0x90, 0x26, 0xda, 0x5b
  };

  const uint8_t byron_testnet_daedalus[] = {
    0x82, 0xd8, 0x18, 0x58, 0x49, 0x83, 0x58, 0x1c, 0x9c, 0x70, 0x85, 0x38, 0xa7, 0x63, 0xff, 0x27,
    0x16, 0x99, 0x87, 0xa4, 0x89, 0xe3, 0x50, 0x57, 0xef, 0x3c, 0xd3, 0x77, 0x8c, 0x05, 0xe9, 0x6f,
    0x7b, 0xa9, 0x45, 0x0e, 0xa2, 0x01, 0x58, 0x1e, 0x58, 0x1c, 0x9c, 0x17, 0x22, 0xf7, 0xe4, 0x46,
    0x68, 0x92, 0x56, 0xe1, 0xa3, 0x02, 0x60, 0xf3, 0x51, 0x0d, 0x55, 0x8d, 0x99, 0xd0, 0xc3, 0x91,
    0xf2, 0xba, 0x89, 0xcb, 0x69, 0x77, 0x02, 0x45, 0x1a, 0x41, 0x70, 0xcb, 0x17, 0x00, 0x1a, 0x69,
    0x79, 0x12, 0x6c
  };

  const uint8_t b58_high[] = {
    0xff, 0x5a, 0x1f, 0xc5, 0xdd, 0x9e, 0x6f, 0x03, 0x81, 0x9f, 0xca, 0x94, 0xa2, 0xd8, 0x96, 0x69,
    0x46, 0x96, 0x67, 0xf9, 0xa0, 0xc0, 0xd6, 0x8d, 0xec
  };

  const uint8_t leading_zero[] = {
    0x00, 0x5a, 0x1f, 0xc5, 0xdd, 0x9e, 0x6f, 0x03, 0x81, 0x9f, 0xca, 0x94, 0xa2, 0xd8, 0x96, 0x69,
    0x46, 0x96, 0x67, 0xf9, 0xa0, 0x74, 0x65, 0x59, 0x46
  };

  // clang-format on

  const size_t byron_mainnet_yoroi_size    = sizeof(byron_mainnet_yoroi);
  const size_t byron_testnet_daedalus_size = sizeof(byron_testnet_daedalus);
  const size_t b58_high_size               = sizeof(b58_high);
  const size_t leading_zero_size           = sizeof(leading_zero);

  // Act & Assert
  EXPECT_EQ(encodes_correctly("Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi", byron_mainnet_yoroi, byron_mainnet_yoroi_size), true);
  EXPECT_EQ(encodes_correctly("37btjrVyb4KEB2STADSsj3MYSAdj52X5FrFWpw2r7Wmj2GDzXjFRsHWuZqrw7zSkwopv8Ci3VWeg6bisU9dgJxW5hb2MZYeduNKbQJrqz3zVBsu9nT", byron_testnet_daedalus, byron_testnet_daedalus_size), true);
  EXPECT_EQ(encodes_correctly("2mkQLxaN3Y4CwN5E9rdMWNgsXX7VS6UnfeT", b58_high, b58_high_size), true);
  EXPECT_EQ(encodes_correctly("19DXstMaV43WpYg4ceREiiTv2UntmoiA9j", leading_zero, leading_zero_size), true);
}

TEST(cardano_encoding_base58_decode, canEncodeDataInBase58Strings)
{
  // Arrange

  // clang-format off
  const uint8_t byron_mainnet_yoroi[] = {
    0x82, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x3c, 0x42, 0xcc, 0xdd, 0xa0, 0x00, 0x1a, 0x90, 0x26, 0xda, 0x5b
  };

  const uint8_t byron_testnet_daedalus[] = {
    0x82, 0xd8, 0x18, 0x58, 0x49, 0x83, 0x58, 0x1c, 0x9c, 0x70, 0x85, 0x38, 0xa7, 0x63, 0xff, 0x27,
    0x16, 0x99, 0x87, 0xa4, 0x89, 0xe3, 0x50, 0x57, 0xef, 0x3c, 0xd3, 0x77, 0x8c, 0x05, 0xe9, 0x6f,
    0x7b, 0xa9, 0x45, 0x0e, 0xa2, 0x01, 0x58, 0x1e, 0x58, 0x1c, 0x9c, 0x17, 0x22, 0xf7, 0xe4, 0x46,
    0x68, 0x92, 0x56, 0xe1, 0xa3, 0x02, 0x60, 0xf3, 0x51, 0x0d, 0x55, 0x8d, 0x99, 0xd0, 0xc3, 0x91,
    0xf2, 0xba, 0x89, 0xcb, 0x69, 0x77, 0x02, 0x45, 0x1a, 0x41, 0x70, 0xcb, 0x17, 0x00, 0x1a, 0x69,
    0x79, 0x12, 0x6c
  };

  const uint8_t b58_high[] = {
    0xff, 0x5a, 0x1f, 0xc5, 0xdd, 0x9e, 0x6f, 0x03, 0x81, 0x9f, 0xca, 0x94, 0xa2, 0xd8, 0x96, 0x69,
    0x46, 0x96, 0x67, 0xf9, 0xa0, 0xc0, 0xd6, 0x8d, 0xec
  };

  const uint8_t leading_zero[] = {
    0x00, 0x5a, 0x1f, 0xc5, 0xdd, 0x9e, 0x6f, 0x03, 0x81, 0x9f, 0xca, 0x94, 0xa2, 0xd8, 0x96, 0x69,
    0x46, 0x96, 0x67, 0xf9, 0xa0, 0x74, 0x65, 0x59, 0x46
  };

  // clang-format on

  const size_t byron_mainnet_yoroi_size    = sizeof(byron_mainnet_yoroi);
  const size_t byron_testnet_daedalus_size = sizeof(byron_testnet_daedalus);
  const size_t b58_high_size               = sizeof(b58_high);
  const size_t leading_zero_size           = sizeof(leading_zero);

  // Act & Assert
  EXPECT_EQ(decodes_correctly("Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi", byron_mainnet_yoroi, byron_mainnet_yoroi_size), true);
  EXPECT_EQ(decodes_correctly("37btjrVyb4KEB2STADSsj3MYSAdj52X5FrFWpw2r7Wmj2GDzXjFRsHWuZqrw7zSkwopv8Ci3VWeg6bisU9dgJxW5hb2MZYeduNKbQJrqz3zVBsu9nT", byron_testnet_daedalus, byron_testnet_daedalus_size), true);
  EXPECT_EQ(decodes_correctly("2mkQLxaN3Y4CwN5E9rdMWNgsXX7VS6UnfeT", b58_high, b58_high_size), true);
  EXPECT_EQ(decodes_correctly("19DXstMaV43WpYg4ceREiiTv2UntmoiA9j", leading_zero, leading_zero_size), true);
}

TEST(cardano_encoding_base58_get_encoded_length, returnZeroIfGivenNullPtr)
{
  // Arrange
  const byte_t* data        = NULL;
  const size_t  data_length = 0;

  // Act
  size_t encoded_length = cardano_encoding_base58_get_encoded_length(data, data_length);

  // Assert
  EXPECT_EQ(encoded_length, 0);
}

TEST(cardano_encoding_base58_get_encoded_length, returnEmptyStringIfGivenEmptyData)
{
  // Arrange
  const byte_t* data        = (byte_t*)"";
  const size_t  data_length = 0;

  // Act
  size_t encoded_length = cardano_encoding_base58_get_encoded_length(data, data_length);

  // Assert
  EXPECT_EQ(encoded_length, 1);
}

TEST(cardano_encoding_base58_get_decoded_length, returnZeroIfGivenNullPtr)
{
  // Arrange
  const char*  data        = NULL;
  const size_t data_length = 0;

  // Act
  size_t decoded_length = cardano_encoding_base58_get_decoded_length(data, data_length);

  // Assert
  EXPECT_EQ(decoded_length, 0);
}

TEST(cardano_encoding_base58_get_decoded_length, returnZeroIfGivenEmptyString)
{
  // Arrange
  const char*  data        = "";
  const size_t data_length = 0;

  // Act
  size_t decoded_length = cardano_encoding_base58_get_decoded_length(data, data_length);

  // Assert
  EXPECT_EQ(decoded_length, 0);
}

TEST(cardano_encoding_base58_encode, returnPointerIsNullIfGivenNullPtr)
{
  // Arrange
  const byte_t* data          = NULL;
  const size_t  data_length   = 0;
  char*         output        = NULL;
  const size_t  output_length = 0;

  // Act
  cardano_error_t result = cardano_encoding_base58_encode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_encoding_base58_encode, returnInsufficientBufferSizeIfGivenEmptyData)
{
  // Arrange
  const byte_t* data          = (byte_t*)"";
  const size_t  data_length   = 0;
  char*         output        = (char*)malloc(1);
  const size_t  output_length = 1;

  // Act
  cardano_error_t result = cardano_encoding_base58_encode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  free(output);
}

TEST(cardano_encoding_base58_encode, returnPointerIsNullIfGivenNullOutput)
{
  // Arrange
  const byte_t* data          = (byte_t*)"";
  const size_t  data_length   = 1;
  char*         output        = NULL;
  const size_t  output_length = 0;

  // Act
  cardano_error_t result = cardano_encoding_base58_encode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_encoding_base58_encode, returnInsufficientBufferSizeIfGivenEmptyOutput)
{
  // Arrange
  const byte_t* data          = (byte_t*)"";
  const size_t  data_length   = 0;
  char*         output        = (char*)malloc(1);
  const size_t  output_length = 0;

  // Act
  cardano_error_t result = cardano_encoding_base58_encode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  free(output);
}

TEST(cardano_encoding_base58_encode, returnInsufficientBufferSizeIfGivenSmallOutputLength)
{
  // Arrange
  const byte_t* data          = (byte_t*)"Hello, World!";
  const size_t  data_length   = strlen((const char*)data);
  char*         output        = (char*)malloc(1);
  const size_t  output_length = 1;

  // Act
  cardano_error_t result = cardano_encoding_base58_encode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  free(output);
}

TEST(cardano_encoding_base58_encode, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  const byte_t* data          = (byte_t*)"Hello, World!";
  const size_t  data_length   = strlen((const char*)data);
  byte_t        output_data[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_base58_encode(data, data_length, (char*)output_data, 100);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_base58_decode, returnPointerIsNullIfGivenNullPtr)
{
  // Arrange
  const char*  data          = NULL;
  const size_t data_length   = 0;
  uint8_t*     output        = (uint8_t*)"";
  const size_t output_length = 1;

  // Act
  cardano_error_t result = cardano_encoding_base58_decode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_encoding_base58_decode, returnInsufficientBufferSizeIfGivenEmptyData)
{
  // Arrange
  const char*  data          = "";
  const size_t data_length   = 0;
  uint8_t*     output        = (uint8_t*)malloc(1);
  const size_t output_length = 0;

  // Act
  cardano_error_t result = cardano_encoding_base58_decode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  free(output);
}

TEST(cardano_encoding_base58_decode, returnPointerIsNullIfGivenNullOutput)
{
  // Arrange
  const char*  data          = "";
  const size_t data_length   = 1;
  uint8_t*     output        = NULL;
  const size_t output_length = 0;

  // Act
  cardano_error_t result = cardano_encoding_base58_decode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_encoding_base58_decode, returnInsufficientBufferSizeIfGivenEmptyOutput)
{
  // Arrange
  const char*  data          = "";
  const size_t data_length   = 0;
  uint8_t*     output        = (uint8_t*)malloc(1);
  const size_t output_length = 0;

  // Act
  cardano_error_t result = cardano_encoding_base58_decode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  free(output);
}

TEST(cardano_encoding_base58_decode, returnInsufficientBufferSizeIfGivenSmallOutputLength)
{
  // Arrange
  const char*  data          = "Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi";
  const size_t data_length   = strlen(data);
  uint8_t*     output        = (uint8_t*)malloc(1);
  const size_t output_length = 1;

  // Act
  cardano_error_t result = cardano_encoding_base58_decode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  free(output);
}

TEST(cardano_encoding_base58_decode, returnEncodingErrorIfGivenAWrongCharacter)
{
  // Arrange
  const char*  data          = "Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi!";
  const size_t data_length   = strlen(data);
  uint8_t*     output        = (uint8_t*)malloc(100);
  const size_t output_length = 100;

  // Act
  cardano_error_t result = cardano_encoding_base58_decode(data, data_length, output, output_length);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  free(output);
}