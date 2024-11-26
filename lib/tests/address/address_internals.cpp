/**
 * \file address_internals.cpp
 *
 * \author angel.castillo
 * \date   Apr 15, 2024
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

#include "../../src/allocators.h"
#include "../allocators_helpers.h"
#include "cip19_test_vectors.h"

#include <cardano/address/address.h>
#include <cardano/address/address_type.h>

extern "C" {
#include "../../src/address/internals/addr_common.h"
#include "../../src/address/internals/base_addr_pack.h"
#include "../../src/address/internals/byron_addr_pack.h"
#include "../../src/address/internals/pointer_addr_pack.h"
}

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(_cardano_get_payment_credential_type, returnsErrorWhenGivenInvalidCredentialType)
{
  // Arrange
  const cardano_address_type_t type = CARDANO_ADDRESS_TYPE_BYRON;
  cardano_credential_type_t    cred;

  // Act
  EXPECT_EQ(_cardano_get_payment_credential_type(type, &cred), CARDANO_ERROR_INVALID_ADDRESS_TYPE);
  EXPECT_EQ(_cardano_get_stake_credential_type(type, &cred), CARDANO_ERROR_INVALID_ADDRESS_TYPE);
}

TEST(_cardano_is_valid_payment_address_prefix, returnsFalseWehnGivenNullPointer)
{
  // Act
  EXPECT_FALSE(_cardano_is_valid_payment_address_prefix(NULL, 0));
  EXPECT_FALSE(_cardano_is_valid_stake_address_prefix(NULL, 0));
}

TEST(_cardano_unpack_base_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_base_address_t* address = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_four_malloc, realloc, free);

  // Act
  EXPECT_EQ(_cardano_unpack_base_address(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes), &address), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cardano_byron_address_extract_cbor_data, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_writer_t* writer    = cardano_cbor_writer_new();
  byte_t*                data      = NULL;
  size_t                 data_size = 0;

  // Arrange

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  EXPECT_EQ(_cardano_byron_address_extract_cbor_data(writer, &data, &data_size), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Clean up
  cardano_cbor_writer_unref(&writer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cardano_byron_address_finalize_writer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_writer_t* writer    = cardano_cbor_writer_new();
  byte_t*                data      = NULL;
  size_t                 data_size = 0;

  // Arrange

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  EXPECT_EQ(_cardano_byron_address_finalize_writer(writer, &data, &data_size), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Clean up
  cardano_cbor_writer_unref(&writer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cardano_byron_address_verify_cbor_structure, returnsErrorIfNotStartingWithArray)
{
  // clang-format off
  static const byte_t worng_data[] = {
    0x00, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x3c, 0x42, 0xcc, 0xdd, 0xa0, 0x00, 0x1a, 0x90, 0x26, 0xda, 0x5b
  };
  // clang-format on

  uint32_t               crc_calculated       = 0;
  uint64_t               crc_expected         = 0;
  cardano_buffer_t*      address_data_encoded = NULL;
  cardano_cbor_reader_t* reader               = cardano_cbor_reader_new(worng_data, sizeof(worng_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_verify_cbor_structure(reader, &crc_calculated, &crc_expected, &address_data_encoded);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_verify_cbor_structure, returnsErrorIfMissingTag)
{
  // clang-format off
  static const byte_t worng_data[] = {
    0x82, 0x00, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x3c, 0x42, 0xcc, 0xdd, 0xa0, 0x00, 0x1a, 0x90, 0x26, 0xda, 0x5b
  };
  // clang-format on

  uint32_t               crc_calculated       = 0;
  uint64_t               crc_expected         = 0;
  cardano_buffer_t*      address_data_encoded = NULL;
  cardano_cbor_reader_t* reader               = cardano_cbor_reader_new(worng_data, sizeof(worng_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_verify_cbor_structure(reader, &crc_calculated, &crc_expected, &address_data_encoded);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_verify_cbor_structure, returnsErrorIfMissingUint)
{
  // clang-format off
  static const byte_t worng_data[] = {
    0x82, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x00, 0x00, 0xcc, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };
  // clang-format on

  uint32_t               crc_calculated       = 0;
  uint64_t               crc_expected         = 0;
  cardano_buffer_t*      address_data_encoded = NULL;
  cardano_cbor_reader_t* reader               = cardano_cbor_reader_new(worng_data, sizeof(worng_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_verify_cbor_structure(reader, &crc_calculated, &crc_expected, &address_data_encoded);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_verify_cbor_structure, returnsErrorIfCrcMissMatch)
{
  // clang-format off
  static const byte_t worng_data[] = {
    0x82, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x3c, 0x42, 0xcc, 0xdd, 0xa0, 0x00, 0x1a, 0x90, 0x26, 0xda, 0xFF
  };
  // clang-format on

  uint32_t               crc_calculated       = 0;
  uint64_t               crc_expected         = 0;
  cardano_buffer_t*      address_data_encoded = NULL;
  cardano_cbor_reader_t* reader               = cardano_cbor_reader_new(worng_data, sizeof(worng_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_verify_cbor_structure(reader, &crc_calculated, &crc_expected, &address_data_encoded);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_CHECKSUM_MISMATCH);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_unpack_inner_cbor_content, returnsErrorIfChecksumMissmatch)
{
  // Arrange
  cardano_error_t result = _cardano_byron_address_unpack_inner_cbor_content((cardano_buffer_t*)"", 1, 2, (cardano_cbor_reader_t**)"");

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_CHECKSUM_MISMATCH);
}

TEST(_cardano_byron_address_process_derivation_path, returnsErrorIfMissingFirstByteString)
{
  // clang-format off
  static const byte_t wrong_data[] = {
    0x00, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x3c, 0x42, 0xcc, 0xdd, 0xa0, 0x08
  };
  // clang-format on

  cardano_byron_address_attributes_t attributes = { 0 };
  cardano_cbor_reader_t*             reader     = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_process_derivation_path(reader, &attributes);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_process_derivation_path, returnsErrorIfMissingInnerByteString)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd, 0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34,
    0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe, 0x3c, 0x42, 0xcc, 0xdd
  };

  // clang-format on

  cardano_byron_address_attributes_t attributes = { 0 };
  cardano_cbor_reader_t*             reader     = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_process_derivation_path(reader, &attributes);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_process_magic, returnsErrorIfMissingFirstByteString)
{
  // clang-format off
  static const byte_t wrong_data[] = {
    0x00, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd,
    0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34, 0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe,
    0x3c, 0x42, 0xcc, 0xdd, 0xa0, 0x08
  };
  // clang-format on

  cardano_byron_address_attributes_t attributes = { 0 };
  cardano_cbor_reader_t*             reader     = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_process_magic(reader, &attributes);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_process_magic, returnsErrorIfMissingInnerInt)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd, 0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34,
    0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe, 0x3c, 0x42, 0xcc, 0xdd
  };

  // clang-format on

  cardano_byron_address_attributes_t attributes = { 0 };
  cardano_cbor_reader_t*             reader     = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Arrange
  cardano_error_t result = _cardano_byron_address_process_magic(reader, &attributes);

  // Act
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_extract_address_components, returnsErrorIfMissingStartArray)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x58, 0x1c, 0xba, 0x97, 0x0a, 0xd3, 0x66, 0x54, 0xd8, 0xdd, 0x8f, 0x74, 0x27, 0x4b, 0x73, 0x34,
    0x52, 0xdd, 0xea, 0xb9, 0xa6, 0x2a, 0x39, 0x77, 0x46, 0xbe, 0x3c, 0x42, 0xcc, 0xdd
  };

  // clang-format on

  cardano_byron_address_t* result = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Act
  EXPECT_EQ(_cardano_byron_address_extract_address_components(reader, &result), CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_extract_address_components, returnsErrorIfInnerByteString)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x83, 0x00, 0x1c, 0x9c, 0x70, 0x85, 0x38, 0xa7, 0x63, 0xff, 0x27, 0x16, 0x99, 0x87, 0xa4, 0x89,
    0xe3, 0x50, 0x57, 0xef, 0x3c, 0xd3, 0x77, 0x8c, 0x05, 0xe9, 0x6f, 0x7b, 0xa9, 0x45, 0x0e, 0xa2,
    0x01, 0x58, 0x1e, 0x58, 0x1c, 0x9c, 0x17, 0x2f, 0x7e, 0x44, 0x66, 0x89, 0x25, 0x6e, 0x1a, 0x30,
    0x26, 0x0f, 0x35, 0x10, 0xd5, 0x58, 0xd9, 0x9d, 0x0c, 0x39, 0x1f, 0x2b, 0xa8, 0x9c, 0xb6, 0x97,
    0x70, 0x24, 0x51, 0xa4, 0x17, 0x0c, 0xb1, 0x70, 0x00
  };

  // clang-format on

  cardano_byron_address_t* result = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Act
  EXPECT_EQ(_cardano_byron_address_extract_address_components(reader, &result), CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_extract_address_components, returnsErrorIfInnerMap)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x83, 0x58, 0x1c, 0x9c, 0x70, 0x85, 0x38, 0xa7, 0x63, 0xff, 0x27, 0x16, 0x99, 0x87, 0xa4, 0x89,
    0xe3, 0x50, 0x57, 0xef, 0x3c, 0xd3, 0x77, 0x8c, 0x05, 0xe9, 0x6f, 0x7b, 0xa9, 0x45, 0x0e, 0xFF,
    0x01, 0x58, 0x1e, 0x58, 0x1c, 0x9c, 0x17, 0x2f, 0x7e, 0x44, 0x66, 0x89, 0x25, 0x6e, 0x1a, 0x30,
    0x26, 0x0f, 0x35, 0x10, 0xd5, 0x58, 0xd9, 0x9d, 0x0c, 0x39, 0x1f, 0x2b, 0xa8, 0x9c, 0xb6, 0x97,
    0x70, 0x24, 0x51, 0xa4, 0x17, 0x0c, 0xb1, 0x70, 0x00
  };

  // clang-format on

  cardano_byron_address_t* result = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Act
  EXPECT_EQ(_cardano_byron_address_extract_address_components(reader, &result), CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_extract_address_components, returnErrorIfInvalidMapIndex)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x83, 0x58, 0x1c, 0x9c, 0x70, 0x85, 0x38, 0xa7, 0x63, 0xff, 0x27, 0x16, 0x99, 0x87, 0xa4, 0x89,
    0xe3, 0x50, 0x57, 0xef, 0x3c, 0xd3, 0x77, 0x8c, 0x05, 0xe9, 0x6f, 0x7b, 0xa9, 0x45, 0x0e, 0xa2,
    0xff, 0x58, 0x1e, 0x58, 0x1c, 0x9c, 0x17, 0x2f, 0x7e, 0x44, 0x66, 0x89, 0x25, 0x6e, 0x1a, 0x30,
    0x26, 0x0f, 0x35, 0x10, 0xd5, 0x58, 0xd9, 0x9d, 0x0c, 0x39, 0x1f, 0x2b, 0xa8, 0x9c, 0xb6, 0x97,
    0x70, 0x24, 0x51, 0xa4, 0x17, 0x0c, 0xb1, 0x70, 0x00
  };

  // clang-format on

  cardano_byron_address_t* result = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Act
  EXPECT_EQ(_cardano_byron_address_extract_address_components(reader, &result), CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_extract_address_components, returnErrorIfInvalidMapIndexVal)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x83, 0x58, 0x1c, 0x9c, 0x70, 0x85, 0x38, 0xa7, 0x63, 0xff, 0x27, 0x16, 0x99, 0x87, 0xa4, 0x89,
    0xe3, 0x50, 0x57, 0xef, 0x3c, 0xd3, 0x77, 0x8c, 0x05, 0xe9, 0x6f, 0x7b, 0xa9, 0x45, 0x0e, 0xa2,
    0x09, 0x58, 0x1e, 0x58, 0x1c, 0x9c, 0x17, 0x2f, 0x7e, 0x44, 0x66, 0x89, 0x25, 0x6e, 0x1a, 0x30,
    0x26, 0x0f, 0x35, 0x10, 0xd5, 0x58, 0xd9, 0x9d, 0x0c, 0x39, 0x1f, 0x2b, 0xa8, 0x9c, 0xb6, 0x97,
    0x70, 0x24, 0x51, 0xa4, 0x17, 0x0c, 0xb1, 0x70, 0x00
  };

  // clang-format on

  cardano_byron_address_t* result = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Act
  EXPECT_EQ(_cardano_byron_address_extract_address_components(reader, &result), CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_byron_address_extract_address_components, returnErrorIfInvalidIntAtEnd)
{
  // clang-format off
  const byte_t wrong_data[] = {
    0x83, 0x58, 0x1C, 0x9C, 0x70, 0x85, 0x38, 0xA7, 0x63, 0xFF, 0x27, 0x16, 0x99, 0x87,
    0xA4, 0x89, 0xE3, 0x50, 0x57, 0xEF, 0x3C, 0xD3, 0x77, 0x8C, 0x05, 0xE9, 0x6F, 0x7B,
    0xA9, 0x45, 0x0E, 0xA2, 0x01, 0x58, 0x1E, 0x58, 0x1C, 0x9C, 0x17, 0x22, 0xF7, 0xE4,
    0x46, 0x68, 0x92, 0x56, 0xE1, 0xA3, 0x02, 0x60, 0xF3, 0x51, 0x0D, 0x55, 0x8D, 0x99,
    0xD0, 0xC3, 0x91, 0xF2, 0xBA, 0x89, 0xCB, 0x69, 0x77, 0x02, 0x45, 0x1A, 0x41, 0x70,
    0xCB, 0x17, 0xff
  };

  // clang-format on

  cardano_byron_address_t* result = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_new(wrong_data, sizeof(wrong_data));

  // Act
  EXPECT_EQ(_cardano_byron_address_extract_address_components(reader, &result), CARDANO_ERROR_DECODING);

  // Clean up
  cardano_cbor_reader_unref(&reader);
}