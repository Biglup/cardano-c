/**
 * \file bls_key.cpp
 *
 * \author angel.castillo
 * \date   Sep 05, 2026
 *
 * Copyright 2026 Biglup Labs
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

#include <cardano/error.h>

#include <cardano/pool_params/bls_key.h>

#include "../allocators_helpers.h"
#include "../json_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PUBLIC_KEY_HEX       = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f";
static const char* POSSESSION_PROOF_HEX = "606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f";
static const char* CBOR                 = "825860000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f5830606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f";
static const char* CBOR_ONE_ELEMENT     = "815860000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f";
static const char* CBOR_SHORT_KEY       = "82582f0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f5830606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f";
static const char* CBOR_SHORT_PROOF     = "825860000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f5820606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f";
static const char* CBOR_KEY_NOT_BYTES   = "820105";

static const size_t PUBLIC_KEY_SIZE       = 96;
static const size_t POSSESSION_PROOF_SIZE = 48;

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the BLS key.
 * @return A new instance of the BLS key.
 */
static cardano_bls_key_t*
new_default_bls_key()
{
  cardano_bls_key_t* bls_key = NULL;
  cardano_error_t    error   = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX), POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX), &bls_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  return bls_key;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_bls_key_new, canCreateBlsKey)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;
  byte_t             public_key[96];
  byte_t             possession_proof[48];

  for (size_t i = 0; i < sizeof(public_key); ++i)
  {
    public_key[i] = (byte_t)i;
  }

  for (size_t i = 0; i < sizeof(possession_proof); ++i)
  {
    possession_proof[i] = (byte_t)(sizeof(public_key) + i);
  }

  // Act
  cardano_error_t error = cardano_bls_key_new(public_key, sizeof(public_key), possession_proof, sizeof(possession_proof), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(bls_key, testing::Not((cardano_bls_key_t*)nullptr));
  EXPECT_EQ(cardano_bls_key_get_public_key_size(bls_key), PUBLIC_KEY_SIZE);
  EXPECT_EQ(memcmp(cardano_bls_key_get_public_key_bytes(bls_key), public_key, sizeof(public_key)), 0);
  EXPECT_EQ(cardano_bls_key_get_possession_proof_size(bls_key), POSSESSION_PROOF_SIZE);
  EXPECT_EQ(memcmp(cardano_bls_key_get_possession_proof_bytes(bls_key), possession_proof, sizeof(possession_proof)), 0);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_new, returnsErrorIfPublicKeyIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key              = nullptr;
  byte_t             possession_proof[48] = { 0 };

  // Act
  cardano_error_t error = cardano_bls_key_new(nullptr, PUBLIC_KEY_SIZE, possession_proof, sizeof(possession_proof), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new, returnsErrorIfPossessionProofIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key        = nullptr;
  byte_t             public_key[96] = { 0 };

  // Act
  cardano_error_t error = cardano_bls_key_new(public_key, sizeof(public_key), nullptr, POSSESSION_PROOF_SIZE, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new, returnsErrorIfBlsKeyIsNull)
{
  // Arrange
  byte_t public_key[96]       = { 0 };
  byte_t possession_proof[48] = { 0 };

  // Act
  cardano_error_t error = cardano_bls_key_new(public_key, sizeof(public_key), possession_proof, sizeof(possession_proof), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bls_key_new, returnsErrorIfPublicKeySizeIsInvalid)
{
  // Arrange
  cardano_bls_key_t* bls_key              = nullptr;
  byte_t             public_key[96]       = { 0 };
  byte_t             possession_proof[48] = { 0 };

  // Act
  cardano_error_t error = cardano_bls_key_new(public_key, sizeof(public_key) - 1, possession_proof, sizeof(possession_proof), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new, returnsErrorIfPossessionProofSizeIsInvalid)
{
  // Arrange
  cardano_bls_key_t* bls_key              = nullptr;
  byte_t             public_key[96]       = { 0 };
  byte_t             possession_proof[48] = { 0 };

  // Act
  cardano_error_t error = cardano_bls_key_new(public_key, sizeof(public_key), possession_proof, sizeof(possession_proof) + 1, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_bls_key_t* bls_key              = nullptr;
  byte_t             public_key[96]       = { 0 };
  byte_t             possession_proof[48] = { 0 };

  // Act
  cardano_error_t error = cardano_bls_key_new(public_key, sizeof(public_key), possession_proof, sizeof(possession_proof), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bls_key_new_from_hex, canCreateBlsKey)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX), POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(bls_key, testing::Not((cardano_bls_key_t*)nullptr));
  EXPECT_EQ(cardano_bls_key_get_public_key_size(bls_key), PUBLIC_KEY_SIZE);
  EXPECT_EQ(cardano_bls_key_get_public_key_bytes(bls_key)[0], 0x00);
  EXPECT_EQ(cardano_bls_key_get_public_key_bytes(bls_key)[95], 0x5f);
  EXPECT_EQ(cardano_bls_key_get_possession_proof_size(bls_key), POSSESSION_PROOF_SIZE);
  EXPECT_EQ(cardano_bls_key_get_possession_proof_bytes(bls_key)[0], 0x60);
  EXPECT_EQ(cardano_bls_key_get_possession_proof_bytes(bls_key)[47], 0x8f);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfPublicKeyHexIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(nullptr, strlen(PUBLIC_KEY_HEX), POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfPossessionProofHexIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX), nullptr, strlen(POSSESSION_PROOF_HEX), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfBlsKeyIsNull)
{
  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX), POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfPublicKeyHexHasWrongLength)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX) - 2, POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfPossessionProofHexHasWrongLength)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX), POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX) - 2, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfPublicKeyHexIsInvalid)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  std::string invalid_hex(PUBLIC_KEY_HEX);
  invalid_hex[0] = 'z';

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(invalid_hex.c_str(), invalid_hex.size(), POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfPossessionProofHexIsInvalid)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  std::string invalid_hex(POSSESSION_PROOF_HEX);
  invalid_hex[0] = 'z';

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX), invalid_hex.c_str(), invalid_hex.size(), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_new_from_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_error_t error = cardano_bls_key_new_from_hex(PUBLIC_KEY_HEX, strlen(PUBLIC_KEY_HEX), POSSESSION_PROOF_HEX, strlen(POSSESSION_PROOF_HEX), &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bls_key_to_cbor, canSerializeBlsKey)
{
  // Arrange
  cardano_bls_key_t*     bls_key = new_default_bls_key();
  cardano_cbor_writer_t* writer  = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_bls_key_to_cbor(bls_key, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_bls_key_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_bls_key_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_bls_key_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key = new_default_bls_key();

  // Act
  cardano_error_t error = cardano_bls_key_to_cbor(bls_key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_from_cbor, canDeserializeBlsKey)
{
  // Arrange
  cardano_bls_key_t*     bls_key = nullptr;
  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(bls_key, testing::Not((cardano_bls_key_t*)nullptr));
  EXPECT_EQ(cardano_bls_key_get_public_key_size(bls_key), PUBLIC_KEY_SIZE);
  EXPECT_EQ(cardano_bls_key_get_public_key_bytes(bls_key)[95], 0x5f);
  EXPECT_EQ(cardano_bls_key_get_possession_proof_size(bls_key), POSSESSION_PROOF_SIZE);
  EXPECT_EQ(cardano_bls_key_get_possession_proof_bytes(bls_key)[47], 0x8f);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  error = cardano_bls_key_to_cbor(bls_key, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfBlsKeyIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(nullptr, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_bls_key_t*     bls_key = nullptr;
  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfArrayHasWrongLength)
{
  // Arrange
  cardano_bls_key_t*     bls_key = nullptr;
  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex(CBOR_ONE_ELEMENT, strlen(CBOR_ONE_ELEMENT));

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfPublicKeyIsNotAByteString)
{
  // Arrange
  cardano_bls_key_t*     bls_key = nullptr;
  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex(CBOR_KEY_NOT_BYTES, strlen(CBOR_KEY_NOT_BYTES));

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfPublicKeyHasWrongSize)
{
  // Arrange
  cardano_bls_key_t*     bls_key = nullptr;
  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex(CBOR_SHORT_KEY, strlen(CBOR_SHORT_KEY));

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'bls_key', expected a 'Reader State: Byte String' (3) of 96 element(s) but got a 'Reader State: Byte String' (3) of 47 element(s).");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfPossessionProofHasWrongSize)
{
  // Arrange
  cardano_bls_key_t*     bls_key = nullptr;
  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex(CBOR_SHORT_PROOF, strlen(CBOR_SHORT_PROOF));

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, &bls_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bls_key_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_bls_key_t*     bls_key = nullptr;
  cardano_cbor_reader_t* reader  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bls_key_from_cbor(reader, &bls_key);

  // Assert
  EXPECT_NE(error, CARDANO_SUCCESS);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bls_key_to_cip116_json, canConvertToCip116Json)
{
  // Arrange
  cardano_bls_key_t*     bls_key = new_default_bls_key();
  cardano_json_writer_t* json    = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_bls_key_to_cip116_json(bls_key, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"public_key":"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f","possession_proof":"606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f"})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_bls_key_unref(&bls_key);
  free(json_str);
}

TEST(cardano_bls_key_to_cip116_json, returnsErrorIfBlsKeyIsNull)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error = cardano_bls_key_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_bls_key_to_cip116_json, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key = new_default_bls_key();

  // Act
  cardano_error_t error = cardano_bls_key_to_cip116_json(bls_key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_get_public_key_size, returnsZeroWhenObjectIsNull)
{
  // Act
  size_t size = cardano_bls_key_get_public_key_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_bls_key_get_public_key_bytes, returnsNullWhenObjectIsNull)
{
  // Act
  const byte_t* bytes = cardano_bls_key_get_public_key_bytes(nullptr);

  // Assert
  EXPECT_EQ(bytes, (byte_t*)nullptr);
}

TEST(cardano_bls_key_get_possession_proof_size, returnsZeroWhenObjectIsNull)
{
  // Act
  size_t size = cardano_bls_key_get_possession_proof_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_bls_key_get_possession_proof_bytes, returnsNullWhenObjectIsNull)
{
  // Act
  const byte_t* bytes = cardano_bls_key_get_possession_proof_bytes(nullptr);

  // Assert
  EXPECT_EQ(bytes, (byte_t*)nullptr);
}

TEST(cardano_bls_key_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_bls_key_t* bls_key = new_default_bls_key();

  // Act
  cardano_bls_key_ref(bls_key);

  // Assert
  EXPECT_THAT(bls_key, testing::Not((cardano_bls_key_t*)nullptr));
  EXPECT_EQ(cardano_bls_key_refcount(bls_key), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_bls_key_unref(&bls_key);
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bls_key_ref(nullptr);
}

TEST(cardano_bls_key_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;

  // Act
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bls_key_unref((cardano_bls_key_t**)nullptr);
}

TEST(cardano_bls_key_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_bls_key_t* bls_key = new_default_bls_key();

  // Act
  cardano_bls_key_ref(bls_key);
  size_t ref_count = cardano_bls_key_refcount(bls_key);

  cardano_bls_key_unref(&bls_key);
  size_t updated_ref_count = cardano_bls_key_refcount(bls_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_bls_key_t* bls_key = new_default_bls_key();

  // Act
  cardano_bls_key_ref(bls_key);
  size_t ref_count = cardano_bls_key_refcount(bls_key);

  cardano_bls_key_unref(&bls_key);
  size_t updated_ref_count = cardano_bls_key_refcount(bls_key);

  cardano_bls_key_unref(&bls_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(bls_key, (cardano_bls_key_t*)nullptr);

  // Cleanup
  cardano_bls_key_unref(&bls_key);
}

TEST(cardano_bls_key_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_bls_key_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_bls_key_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key = nullptr;
  const char*        message = "This is a test message";

  // Act
  cardano_bls_key_set_last_error(bls_key, message);

  // Assert
  EXPECT_STREQ(cardano_bls_key_get_last_error(bls_key), "Object is NULL.");
}

TEST(cardano_bls_key_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_bls_key_t* bls_key = new_default_bls_key();
  const char*        message = nullptr;

  // Act
  cardano_bls_key_set_last_error(bls_key, message);

  // Assert
  EXPECT_STREQ(cardano_bls_key_get_last_error(bls_key), "");

  // Cleanup
  cardano_bls_key_unref(&bls_key);
}
