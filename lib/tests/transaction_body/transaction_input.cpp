/**
 * \file transaction_input.cpp
 *
 * \author angel.castillo
 * \date   Sep 15, 2024
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
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/transaction_body/transaction_input.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR               = "8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102005";
static const char* TX_ID_HASH         = "0102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020";
static const char* TX_ID_HASH_2       = "ff02030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020";
static const char* TX_INVALID_ID_HASH = "0102030405060708090a0b0c0d0e0f0e0d0c0b0a0908070605040302010010";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the transaction_input.
 * @return A new instance of the transaction_input.
 */
static cardano_transaction_input_t*
new_default_transaction_input()
{
  cardano_transaction_input_t* transaction_input = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t              result            = cardano_transaction_input_from_cbor(reader, &transaction_input);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction_input;
};

/**
 * Creates a new default instance of the blake2b_hash.
 *
 * @return A new instance of the blake2b_hash.
 */
static cardano_blake2b_hash_t*
new_default_hash()
{
  cardano_blake2b_hash_t* hash   = NULL;
  cardano_error_t         result = cardano_blake2b_hash_from_hex(TX_ID_HASH, strlen(TX_ID_HASH), &hash);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return hash;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_transaction_input_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();
  EXPECT_NE(transaction_input, nullptr);

  // Act
  cardano_transaction_input_ref(transaction_input);

  // Assert
  EXPECT_THAT(transaction_input, testing::Not((cardano_transaction_input_t*)nullptr));
  EXPECT_EQ(cardano_transaction_input_refcount(transaction_input), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_transaction_input_unref(&transaction_input);
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_input_ref(nullptr);
}

TEST(cardano_transaction_input_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = nullptr;

  // Act
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_input_unref((cardano_transaction_input_t**)nullptr);
}

TEST(cardano_transaction_input_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();
  EXPECT_NE(transaction_input, nullptr);

  // Act
  cardano_transaction_input_ref(transaction_input);
  size_t ref_count = cardano_transaction_input_refcount(transaction_input);

  cardano_transaction_input_unref(&transaction_input);
  size_t updated_ref_count = cardano_transaction_input_refcount(transaction_input);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();
  EXPECT_NE(transaction_input, nullptr);

  // Act
  cardano_transaction_input_ref(transaction_input);
  size_t ref_count = cardano_transaction_input_refcount(transaction_input);

  cardano_transaction_input_unref(&transaction_input);
  size_t updated_ref_count = cardano_transaction_input_refcount(transaction_input);

  cardano_transaction_input_unref(&transaction_input);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(transaction_input, (cardano_transaction_input_t*)nullptr);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_transaction_input_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_transaction_input_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = nullptr;
  const char*                  message           = "This is a test message";

  // Act
  cardano_transaction_input_set_last_error(transaction_input, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_input_get_last_error(transaction_input), "Object is NULL.");
}

TEST(cardano_transaction_input_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();
  EXPECT_NE(transaction_input, nullptr);

  const char* message = nullptr;

  // Act
  cardano_transaction_input_set_last_error(transaction_input, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_input_get_last_error(transaction_input), "");

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = NULL;

  // Act
  cardano_error_t result = cardano_transaction_input_from_cbor(nullptr, &transaction_input);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_from_cbor, returnsErrorIfInputIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_transaction_input_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*       writer = cardano_cbor_writer_new();
  cardano_transaction_input_t* cert   = new_default_transaction_input();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_transaction_input_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_transaction_input_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_input_to_cbor, returnsErrorIfInputIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_transaction_input_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_transaction_input_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_transaction_input_to_cbor((cardano_transaction_input_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Input specific tests

TEST(cardano_transaction_input_new, canCreateNewInstance)
{
  // Act
  cardano_blake2b_hash_t* hash = new_default_hash();

  cardano_transaction_input_t* transaction_input = NULL;

  cardano_error_t result = cardano_transaction_input_new(hash, 0, &transaction_input);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(transaction_input, nullptr);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_transaction_input_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_transaction_input_t* transaction_input = NULL;

  cardano_error_t result = cardano_transaction_input_new(nullptr, 0, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_new, returnsErrorIfInputIsNull)
{
  // Act

  cardano_error_t result = cardano_transaction_input_new((cardano_blake2b_hash_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_transaction_input_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t* hash = new_default_hash();

  // Act
  cardano_transaction_input_t* transaction_input = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_transaction_input_new(hash, 0, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_input_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_transaction_input_from_cbor(reader, &transaction_input);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_transaction_input_from_cbor(reader, &transaction_input);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_from_cbor, returnsErrorIfInvalidUintAsIndex)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020ef", strlen("8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020ef"));
  cardano_transaction_input_t* transaction_input = NULL;

  // Act
  cardano_error_t result = cardano_transaction_input_from_cbor(reader, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_from_cbor, returnsErrorIfInvalidFirstHash)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("8200ef1c00000000000000000000000000000000000000000000000000000000", strlen("8200581c00000000000000000000000000000000000000000000000000000000"));
  cardano_transaction_input_t* transaction_input = NULL;

  // Act
  cardano_error_t result = cardano_transaction_input_from_cbor(reader, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_transaction_input_set_id, canSetHash)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();
  cardano_blake2b_hash_t*      hash              = new_default_hash();

  // Act
  cardano_error_t result = cardano_transaction_input_set_id(transaction_input, hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_transaction_input_set_id, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = new_default_hash();

  // Act
  cardano_error_t result = cardano_transaction_input_set_id(nullptr, hash);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_transaction_input_set_id, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();

  // Act
  cardano_error_t result = cardano_transaction_input_set_id(transaction_input, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_get_id, canGetHash)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();
  cardano_blake2b_hash_t*      hash              = new_default_hash();

  EXPECT_EQ(cardano_transaction_input_set_id(transaction_input, hash), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* hash2 = cardano_transaction_input_get_id(transaction_input);

  // Assert
  EXPECT_NE(hash2, nullptr);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_transaction_input_get_id, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = cardano_transaction_input_get_id(nullptr);

  // Assert
  EXPECT_EQ(hash, nullptr);
}

// Setter and Getters

TEST(cardano_transaction_input_get_index, canGetIndex)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();

  // Act
  uint64_t index = cardano_transaction_input_get_index(transaction_input);

  // Assert
  EXPECT_EQ(index, 5);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_get_index, returnsErrorIfObjectIsNull)
{
  // Act
  uint64_t index = cardano_transaction_input_get_index(nullptr);

  // Assert
  EXPECT_EQ(index, 0);
}

TEST(cardano_transaction_input_set_index, canSetIndex)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = new_default_transaction_input();

  // Act
  cardano_error_t result = cardano_transaction_input_set_index(transaction_input, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_set_index, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_transaction_input_set_index(nullptr, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_equals, canCompare)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  // Act
  bool result = cardano_transaction_input_equals(transaction_input1, transaction_input2);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
  cardano_transaction_input_unref(&transaction_input2);
}

TEST(cardano_transaction_input_equals, returnsFalseIfObjectsAreDifferent)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  EXPECT_EQ(cardano_transaction_input_set_index(transaction_input2, 1), CARDANO_SUCCESS);

  // Act
  bool result = cardano_transaction_input_equals(transaction_input1, transaction_input2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
  cardano_transaction_input_unref(&transaction_input2);
}

TEST(cardano_transaction_input_equals, returnsFalseIfOneObjectIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = nullptr;

  // Act
  bool result = cardano_transaction_input_equals(transaction_input1, transaction_input2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
}

TEST(cardano_transaction_input_equals, returnsFalseIfOneObjectIsNull2)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = nullptr;
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  // Act
  bool result = cardano_transaction_input_equals(transaction_input1, transaction_input2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input2);
}

TEST(cardano_transaction_input_equals, returnsTrueIfBothObjectsAreNull)
{
  // Act
  bool result = cardano_transaction_input_equals(nullptr, nullptr);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_transaction_input_from_hex, canCreateFromHex)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = NULL;

  // Act
  cardano_error_t result = cardano_transaction_input_from_hex(TX_ID_HASH, strlen(TX_ID_HASH), 0, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(transaction_input, nullptr);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input);
}

TEST(cardano_transaction_input_from_hex, returnsErrorIfHexIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = NULL;

  // Act
  cardano_error_t result = cardano_transaction_input_from_hex(nullptr, 0, 0, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_from_hex, returnsErrorIfHexIsInvalid)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = NULL;

  // Act
  cardano_error_t result = cardano_transaction_input_from_hex(TX_INVALID_ID_HASH, strlen(TX_INVALID_ID_HASH), 1, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
}

TEST(cardano_transaction_input_from_hex, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_transaction_input_from_hex(TX_ID_HASH, strlen(TX_ID_HASH), 0, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_new, returnErrorIfInvalidHashSize)
{
  // Arrange
  cardano_transaction_input_t* transaction_input = NULL;
  cardano_blake2b_hash_t*      hash              = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(TX_INVALID_ID_HASH, strlen(TX_INVALID_ID_HASH), &hash);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_transaction_input_new(hash, 1, &transaction_input);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_transaction_input_compare, canCompare)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  // Act
  int result = cardano_transaction_input_compare(transaction_input1, transaction_input2);

  // Assert
  EXPECT_EQ(result, 0);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
  cardano_transaction_input_unref(&transaction_input2);
}

TEST(cardano_transaction_input_compare, returnsErrorIfFirstObjectIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = nullptr;
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  // Act
  int result = cardano_transaction_input_compare(transaction_input1, transaction_input2);

  // Assert
  EXPECT_EQ(result, -1);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input2);
}

TEST(cardano_transaction_input_compare, returnsErrorIfSecondObjectIsNull)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = nullptr;

  // Act
  int result = cardano_transaction_input_compare(transaction_input1, transaction_input2);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
}

TEST(cardano_transaction_input_compare, returnsErrorIfBothObjectsAreNull)
{
  // Act
  int result = cardano_transaction_input_compare(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, 0);
}

TEST(cardano_transaction_input_compare, returnsErrorIfObjectsAreDifferent)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  EXPECT_EQ(cardano_transaction_input_set_index(transaction_input2, 1), CARDANO_SUCCESS);

  // Act
  int result = cardano_transaction_input_compare(transaction_input1, transaction_input2);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
  cardano_transaction_input_unref(&transaction_input2);
}

TEST(cardano_transaction_input_compare, returnsErrorIfObjectsAreDifferent2)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  EXPECT_EQ(cardano_transaction_input_set_index(transaction_input2, 1), CARDANO_SUCCESS);

  // Act
  int result = cardano_transaction_input_compare(transaction_input2, transaction_input1);

  // Assert
  EXPECT_EQ(result, -1);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
  cardano_transaction_input_unref(&transaction_input2);
}

TEST(cardano_transaction_input_compare, returnsErrorIfHashesAreDifferent)
{
  // Arrange
  cardano_transaction_input_t* transaction_input1 = new_default_transaction_input();
  cardano_transaction_input_t* transaction_input2 = new_default_transaction_input();

  cardano_blake2b_hash_t* hash = NULL;

  cardano_error_t error = cardano_blake2b_hash_from_hex(TX_ID_HASH_2, strlen(TX_ID_HASH_2), &hash);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_transaction_input_set_id(transaction_input2, hash), CARDANO_SUCCESS);

  // Act
  int result = cardano_transaction_input_compare(transaction_input1, transaction_input2);

  EXPECT_NE(result, 0);

  // Cleanup
  cardano_transaction_input_unref(&transaction_input1);
  cardano_transaction_input_unref(&transaction_input2);
  cardano_blake2b_hash_unref(&hash);
}
