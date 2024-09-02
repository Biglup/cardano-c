/**
 * \file constitution.cpp
 *
 * \author angel.castillo
 * \date   Aug 31, 2024
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/common/anchor.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/proposal_procedures/constitution.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                  = "82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_WITH_SCRIPT_HASH = "82827668747470733a2f2f7777772e736f6d6575726c2e696f5820000000000000000000000000000000000000000000000000000000000000000058200000000000000000000000000000000000000000000000000000000000000000";
static const char* ANCHOR_CBOR           = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* DATA_HASH             = "0000000000000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the constitution.
 * @return A new instance of the constitution.
 */
static cardano_constitution_t*
new_default_constitution()
{
  cardano_constitution_t* constitution = NULL;
  cardano_cbor_reader_t*  reader       = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t         result       = cardano_constitution_from_cbor(reader, &constitution);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return constitution;
};

/**
 * Creates a new default instance of the hash.
 * @return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
new_default_hash(const char* hash)
{
  cardano_blake2b_hash_t* hash_instance = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(hash, strlen(hash), &hash_instance);

  EXPECT_THAT(error, CARDANO_SUCCESS);

  return hash_instance;
}

/**
 * Creates a new default instance of the anchor.
 * @return A new instance of the anchor.
 */
static cardano_anchor_t*
new_default_anchor(const char* cbor)
{
  cardano_anchor_t* anchor = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_anchor_from_cbor(reader, &anchor);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return anchor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_constitution_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  EXPECT_NE(constitution, nullptr);

  // Act
  cardano_constitution_ref(constitution);

  // Assert
  EXPECT_THAT(constitution, testing::Not((cardano_constitution_t*)nullptr));
  EXPECT_EQ(cardano_constitution_refcount(constitution), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_constitution_unref(&constitution);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_constitution_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_constitution_ref(nullptr);
}

TEST(cardano_constitution_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_constitution_t* constitution = nullptr;

  // Act
  cardano_constitution_unref(&constitution);
}

TEST(cardano_constitution_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_constitution_unref((cardano_constitution_t**)nullptr);
}

TEST(cardano_constitution_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  EXPECT_NE(constitution, nullptr);

  // Act
  cardano_constitution_ref(constitution);
  size_t ref_count = cardano_constitution_refcount(constitution);

  cardano_constitution_unref(&constitution);
  size_t updated_ref_count = cardano_constitution_refcount(constitution);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_constitution_unref(&constitution);
}

TEST(cardano_constitution_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  EXPECT_NE(constitution, nullptr);

  // Act
  cardano_constitution_ref(constitution);
  size_t ref_count = cardano_constitution_refcount(constitution);

  cardano_constitution_unref(&constitution);
  size_t updated_ref_count = cardano_constitution_refcount(constitution);

  cardano_constitution_unref(&constitution);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(constitution, (cardano_constitution_t*)nullptr);

  // Cleanup
  cardano_constitution_unref(&constitution);
}

TEST(cardano_constitution_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_constitution_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_constitution_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_constitution_t* constitution = nullptr;
  const char*             message      = "This is a test message";

  // Act
  cardano_constitution_set_last_error(constitution, message);

  // Assert
  EXPECT_STREQ(cardano_constitution_get_last_error(constitution), "Object is NULL.");
}

TEST(cardano_constitution_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  EXPECT_NE(constitution, nullptr);

  const char* message = nullptr;

  // Act
  cardano_constitution_set_last_error(constitution, message);

  // Assert
  EXPECT_STREQ(cardano_constitution_get_last_error(constitution), "");

  // Cleanup
  cardano_constitution_unref(&constitution);
}

TEST(cardano_constitution_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_constitution_t* constitution = NULL;

  // Act
  cardano_error_t result = cardano_constitution_from_cbor(nullptr, &constitution);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_constitution_from_cbor, returnsErrorIfCommitteeIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_constitution_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constitution_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*  writer = cardano_cbor_writer_new();
  cardano_constitution_t* cert   = new_default_constitution();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_constitution_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_constitution_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_constitution_to_cbor, returnsErrorIfCommitteeIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_constitution_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_constitution_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_constitution_to_cbor((cardano_constitution_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Committee specific tests

TEST(cardano_constitution_new, canCreateNewInstance)
{
  // Act
  cardano_anchor_t* anchor = new_default_anchor(ANCHOR_CBOR);

  cardano_constitution_t* constitution = NULL;

  cardano_error_t result = cardano_constitution_new(anchor, nullptr, &constitution);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(constitution, nullptr);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_constitution_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_constitution_t* constitution = NULL;

  cardano_error_t result = cardano_constitution_new(nullptr, nullptr, &constitution);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_constitution_new, returnsErrorIfCommitteeIsNull)
{
  // Act
  cardano_anchor_t* anchor = new_default_anchor(ANCHOR_CBOR);

  cardano_error_t result = cardano_constitution_new(anchor, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_constitution_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_anchor_t* anchor = new_default_anchor(ANCHOR_CBOR);

  cardano_constitution_t* constitution = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_constitution_new(anchor, nullptr, &constitution);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_constitution_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_constitution_t* constitution = NULL;
  cardano_cbor_reader_t*  reader       = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_constitution_from_cbor(reader, &constitution);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constitution_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_constitution_t* constitution = NULL;
  cardano_cbor_reader_t*  reader       = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_constitution_from_cbor(reader, &constitution);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constitution_from_cbor, returnsErrorIfInvalidAnchor)
{
  // Arrange
  cardano_cbor_reader_t*  reader       = cardano_cbor_reader_from_hex("82ef", strlen("82ef"));
  cardano_constitution_t* constitution = NULL;

  // Act
  cardano_error_t result = cardano_constitution_from_cbor(reader, &constitution);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constitution_from_cbor, returnsErrorIfInvalidScriptHash)
{
  // Arrange
  cardano_cbor_reader_t*  reader       = cardano_cbor_reader_from_hex("82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000ef", strlen("82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6"));
  cardano_constitution_t* constitution = NULL;

  // Act
  cardano_error_t result = cardano_constitution_from_cbor(reader, &constitution);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_constitution_set_anchor, canCreateWithHash)
{
  // Arrange
  cardano_anchor_t*       anchor      = new_default_anchor(ANCHOR_CBOR);
  cardano_blake2b_hash_t* script_hash = new_default_hash(DATA_HASH);

  // Act
  cardano_constitution_t* constitution = NULL;
  cardano_error_t         result       = cardano_constitution_new(anchor, script_hash, &constitution);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(constitution, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_constitution_to_cbor(constitution, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITH_SCRIPT_HASH);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&script_hash);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_constitution_set_anchor, canSetAnchor)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  cardano_anchor_t*       anchor       = new_default_anchor(ANCHOR_CBOR);

  // Act
  cardano_error_t result = cardano_constitution_set_anchor(constitution, anchor);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_constitution_set_anchor, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = new_default_anchor(ANCHOR_CBOR);

  // Act
  cardano_error_t result = cardano_constitution_set_anchor(nullptr, anchor);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_constitution_set_anchor, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();

  // Act
  cardano_error_t result = cardano_constitution_set_anchor(constitution, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_constitution_unref(&constitution);
}

TEST(cardano_constitution_get_anchor, canGetAnchor)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  cardano_anchor_t*       anchor       = new_default_anchor(ANCHOR_CBOR);

  EXPECT_EQ(cardano_constitution_set_anchor(constitution, anchor), CARDANO_SUCCESS);

  // Act
  cardano_anchor_t* anchor_out = cardano_constitution_get_anchor(constitution);

  // Assert
  EXPECT_NE(anchor_out, nullptr);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_anchor_unref(&anchor);
  cardano_anchor_unref(&anchor_out);
}

TEST(cardano_constitution_get_anchor, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_anchor_t* anchor = cardano_constitution_get_anchor(nullptr);

  // Assert
  EXPECT_EQ(anchor, nullptr);
}

TEST(cardano_constitution_set_script_hash, canSetScriptHash)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  cardano_blake2b_hash_t* script_hash  = new_default_hash(DATA_HASH);

  // Act
  cardano_error_t result = cardano_constitution_set_script_hash(constitution, script_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_blake2b_hash_unref(&script_hash);
}

TEST(cardano_constitution_set_script_hash, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* script_hash = new_default_hash(DATA_HASH);

  // Act
  cardano_error_t result = cardano_constitution_set_script_hash(nullptr, script_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&script_hash);
}

TEST(cardano_constitution_set_script_hash, canBeSetToNull)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();

  // Act
  cardano_error_t result = cardano_constitution_set_script_hash(constitution, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_constitution_unref(&constitution);
}

TEST(cardano_constitution_get_script_hash, canGetScriptHash)
{
  // Arrange
  cardano_constitution_t* constitution = new_default_constitution();
  cardano_blake2b_hash_t* script_hash  = new_default_hash(DATA_HASH);

  EXPECT_EQ(cardano_constitution_set_script_hash(constitution, script_hash), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* script_hash_out = cardano_constitution_get_script_hash(constitution);

  // Assert
  EXPECT_NE(script_hash_out, nullptr);

  // Cleanup
  cardano_constitution_unref(&constitution);
  cardano_blake2b_hash_unref(&script_hash);
  cardano_blake2b_hash_unref(&script_hash_out);
}

TEST(cardano_constitution_get_script_hash, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_blake2b_hash_t* script_hash = cardano_constitution_get_script_hash(nullptr);

  // Assert
  EXPECT_EQ(script_hash, nullptr);
}
