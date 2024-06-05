/**
 * \file governance_action_id.cpp
 *
 * \author angel.castillo
 * \date   Apr 14, 2024
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

#include <cardano/error.h>

#include <cardano/common/governance_action_id.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* KEY_HASH_HEX              = "0000000000000000000000000000000000000000000000000000000000000000";
static const char* KEY_HASH_HEX_2            = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
static const char* INVALID_KEY_HASH_HEX      = "000000000000000000000000000000000000000000000000";
static const char* GOVERNANCE_ACTION_ID_CBOR = "825820000000000000000000000000000000000000000000000000000000000000000003";

/* UNIT TESTS ****************************************************************/

TEST(cardano_governance_action_id_to_cbor, canSerializeGovernanceActionId)
{
  // Arrange
  cardano_cbor_writer_t*          writer               = cardano_cbor_writer_new();
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_error_t                 error                = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_to_cbor(governance_action_id, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex      = (char*)malloc(cbor_hex_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, GOVERNANCE_ACTION_ID_CBOR);

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_error_t                 error                = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_to_cbor(governance_action_id, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_to_cbor, returnsErrorIfGovernanceActionIdIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_governance_action_id_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_governance_action_id_from_cbor, canDeserializeGovernanceActionId)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(governance_action_id, testing::Not((cardano_governance_action_id_t*)nullptr));

  cardano_blake2b_hash_t* hash = cardano_governance_action_id_get_hash(governance_action_id);
  const char*             hex  = cardano_governance_action_id_get_hash_hex(governance_action_id);

  EXPECT_STREQ(hex, KEY_HASH_HEX);

  uint64_t index = 0;
  error          = cardano_governance_action_id_get_index(governance_action_id, &index);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(index, 3);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_governance_action_id_from_cbor, returnErrorIfInvalidArraySize)
{
  // Arrange
  const char*            invalid_cbor = "8100581c00000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  const char* error_msg = cardano_cbor_reader_get_last_error(reader);
  EXPECT_STREQ(error_msg, "There was an error decoding 'governance_action_id', expected a 'Major Type: Byte String' (2) of 2 element(s) but got a 'Major Type: Byte String' (2) of 1 element(s).");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_governance_action_id_from_cbor, returnErrorIfInvalidGovernanceActionIdIndex)
{
  // Arrange
  const char*            invalid_cbor = "8258200000000000000000000000000000000000000000000000000000000000000000ff";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  const char* error_msg = cardano_cbor_reader_get_last_error(reader);
  EXPECT_STREQ(error_msg, "Unexpected break byte.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_governance_action_id_from_cbor, returnErrorIfInvalidByteStringSize)
{
  // Arrange
  const char*            invalid_cbor = "8200581b0000000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_governance_action_id_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_cbor(nullptr, &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_from_cbor, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_governance_action_id_t* governance_action_id = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_from_cbor, returnsErrorIfGovernanceActionIdIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));

  // Act
  cardano_error_t error = cardano_governance_action_id_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_governance_action_id_new, canCreateGovernanceActionId)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_new(
    hash,
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(governance_action_id, testing::Not((cardano_governance_action_id_t*)nullptr));

  cardano_blake2b_hash_t* hash2       = cardano_governance_action_id_get_hash(governance_action_id);
  const byte_t*           hash2_bytes = cardano_blake2b_hash_get_data(hash2);
  const byte_t*           hash3_bytes = cardano_governance_action_id_get_hash_bytes(governance_action_id);
  const char*             hex         = cardano_governance_action_id_get_hash_hex(governance_action_id);

  EXPECT_EQ(memcmp(hash2_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_EQ(memcmp(hash3_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, KEY_HASH_HEX);
  EXPECT_EQ(cardano_governance_action_id_get_hash_bytes_size(governance_action_id), cardano_blake2b_hash_get_bytes_size(hash));
  EXPECT_EQ(cardano_governance_action_id_get_hash_hex_size(governance_action_id), cardano_blake2b_hash_get_hex_size(hash));

  uint64_t index = 0;
  error          = cardano_governance_action_id_get_index(governance_action_id, &index);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(index, 0);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_governance_action_id_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_governance_action_id_new(
    hash,
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_new, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_new(
    hash,
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_governance_action_id_from_hash_hex, canCreateGovernanceActionId)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(governance_action_id, testing::Not((cardano_governance_action_id_t*)nullptr));

  cardano_blake2b_hash_t* hash2 = cardano_governance_action_id_get_hash(governance_action_id);
  const char*             hex   = cardano_governance_action_id_get_hash_hex(governance_action_id);

  EXPECT_STREQ(hex, KEY_HASH_HEX);

  uint64_t index = 0;
  error          = cardano_governance_action_id_get_index(governance_action_id, &index);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(index, 3);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_governance_action_id_from_hash_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_from_hash_hex, returnsErrorIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_from_hash_hex, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
}

TEST(cardano_governance_action_id_from_hash_hex, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_hex(
    nullptr,
    0,
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);
}

TEST(cardano_governance_action_id_from_hash_hex, returnsErrorIfGovernanceActionIdIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_new, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_new(
    nullptr,
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);
}

TEST(cardano_governance_action_id_new, returnsErrorIfGovernanceActionIdIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_new(
    hash,
    0,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_governance_action_id_from_hash_bytes, canCreateGovernanceActionId)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(governance_action_id, testing::Not((cardano_governance_action_id_t*)nullptr));

  cardano_blake2b_hash_t* hash2       = cardano_governance_action_id_get_hash(governance_action_id);
  const byte_t*           hash2_bytes = cardano_blake2b_hash_get_data(hash2);
  const byte_t*           hash3_bytes = cardano_governance_action_id_get_hash_bytes(governance_action_id);
  const char*             hex         = cardano_governance_action_id_get_hash_hex(governance_action_id);

  EXPECT_EQ(memcmp(hash2_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_EQ(memcmp(hash3_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, KEY_HASH_HEX);

  uint64_t index = 0;
  error          = cardano_governance_action_id_get_index(governance_action_id, &index);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(index, 0);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_governance_action_id_from_hash_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_governance_action_id_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_from_hash_bytes, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  error = cardano_governance_action_id_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_governance_action_id_from_hash_bytes, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_governance_action_id_from_hash_bytes, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_bytes(
    nullptr,
    0,
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);
}

TEST(cardano_governance_action_id_from_hash_bytes, returnsErrorIfGovernanceActionIdIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_from_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    0,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_governance_action_id_from_hash_bytes, returnsErrorIfHashIsInvalid)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_from_hash_bytes(
    nullptr,
    0,
    0,
    &governance_action_id);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);
}

TEST(cardano_governance_action_id_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_error_t                 error                = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_ref(governance_action_id);

  // Assert
  EXPECT_THAT(governance_action_id, testing::Not((cardano_governance_action_id_t*)nullptr));
  EXPECT_EQ(cardano_governance_action_id_refcount(governance_action_id), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_governance_action_id_ref(nullptr);
}

TEST(cardano_governance_action_id_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;

  // Act
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_governance_action_id_unref((cardano_governance_action_id_t**)nullptr);
}

TEST(cardano_governance_action_id_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_error_t                 error                = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_ref(governance_action_id);
  size_t ref_count = cardano_governance_action_id_refcount(governance_action_id);

  cardano_governance_action_id_unref(&governance_action_id);
  size_t updated_ref_count = cardano_governance_action_id_refcount(governance_action_id);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_error_t                 error                = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_ref(governance_action_id);
  size_t ref_count = cardano_governance_action_id_refcount(governance_action_id);

  cardano_governance_action_id_unref(&governance_action_id);
  size_t updated_ref_count = cardano_governance_action_id_refcount(governance_action_id);

  cardano_governance_action_id_unref(&governance_action_id);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(governance_action_id, (cardano_governance_action_id_t*)nullptr);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_governance_action_id_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_governance_action_id_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  const char*                     message              = "This is a test message";

  // Act
  cardano_governance_action_id_set_last_error(governance_action_id, message);

  // Assert
  EXPECT_STREQ(cardano_governance_action_id_get_last_error(governance_action_id), "Object is NULL.");
}

TEST(cardano_governance_action_id_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_error_t                 error                = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_governance_action_id_set_last_error(governance_action_id, message);

  // Assert
  EXPECT_STREQ(cardano_governance_action_id_get_last_error(governance_action_id), "");

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_get_hash, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_governance_action_id_get_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_governance_action_id_get_hash_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  const byte_t* hash = cardano_governance_action_id_get_hash_bytes(nullptr);

  // Assert
  EXPECT_EQ(hash, (const byte_t*)nullptr);
}

TEST(cardano_governance_action_id_get_hash_hex, returnsNullIfGivenANullPtr)
{
  // Act
  const char* hash = cardano_governance_action_id_get_hash_hex(nullptr);

  // Assert
  EXPECT_EQ(hash, (const char*)nullptr);
}

TEST(cardano_governance_action_id_get_index, returnsErrorIfGivenANullPtr)
{
  // Arrange
  uint64_t index = 0;

  // Act
  cardano_error_t error = cardano_governance_action_id_get_index(nullptr, &index);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_get_index, returnsErrorIfIndexIsNull)
{
  // Act
  cardano_error_t error = cardano_governance_action_id_get_index((cardano_governance_action_id_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_set_index, returnsErrorIfGivenANullPtr)
{
  // Act
  cardano_error_t error = cardano_governance_action_id_set_index(nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_set_index, canSetIndex)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_error_t                 error                = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_set_index(governance_action_id, 9);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t index = 0;
  error          = cardano_governance_action_id_get_index(governance_action_id, &index);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(index, 9);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_governance_action_id_set_hash, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_error_t error = cardano_governance_action_id_set_hash(nullptr, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_set_hash, returnsErrorIfHashIsNull)
{
  // Act
  cardano_error_t error = cardano_governance_action_id_set_hash((cardano_governance_action_id_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_governance_action_id_set_hash, canSetHash)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    KEY_HASH_HEX_2,
    strlen(KEY_HASH_HEX_2),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_set_hash(governance_action_id, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash2 = cardano_governance_action_id_get_hash(governance_action_id);
  const char*             hex   = cardano_governance_action_id_get_hash_hex(governance_action_id);

  EXPECT_EQ(memcmp(cardano_blake2b_hash_get_data(hash2), cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, KEY_HASH_HEX_2);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_governance_action_id_set_hash, returnErrorIfWorngHashSize)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = nullptr;
  cardano_blake2b_hash_t*         hash                 = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_KEY_HASH_HEX,
    strlen(INVALID_KEY_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_governance_action_id_from_hash_hex(
    KEY_HASH_HEX,
    strlen(KEY_HASH_HEX),
    3,
    &governance_action_id);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_governance_action_id_set_hash(governance_action_id, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_governance_action_id_get_hash_hex_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_governance_action_id_get_hash_hex_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_governance_action_id_get_hash_bytes_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_governance_action_id_get_hash_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}