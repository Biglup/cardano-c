/**
 * \file sub_transaction_set.cpp
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
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

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* SUB_WITH_AUX_DATA_CBOR    = "83a300d90102800180031864a0a1016474657374";
static const char* SUB_WITHOUT_AUX_DATA_CBOR = "83a200d90102800180a0f6";
static const char* SUB_DUPLICATE_ID_CBOR     = "83a200d90102800180a0a1016474657374";

static const char* SUB_WITH_AUX_DATA_ID_HEX    = "8cbebc5de7b583095090a501bd7cd092fcebbfdb87a48adb03e8c00a8960a0ad";
static const char* SUB_WITHOUT_AUX_DATA_ID_HEX = "da4603f4488d798af667794ab542f130a4bd1c20c7ed950c4648aaa95d0be4f4";
static const char* UNKNOWN_ID_HEX              = "0000000000000000000000000000000000000000000000000000000000000000";

static const char* SET_TAGGED_CBOR    = "d901028283a300d90102800180031864a0a101647465737483a200d90102800180a0f6";
static const char* SET_BARE_CBOR      = "8283a300d90102800180031864a0a101647465737483a200d90102800180a0f6";
static const char* SET_REVERSED_CBOR  = "d901028283a200d90102800180a0f683a300d90102800180031864a0a1016474657374";
static const char* SET_DUPLICATE_CBOR = "d901028283a200d90102800180a0f683a200d90102800180a0a1016474657374";
static const char* SET_EMPTY_TAGGED   = "d9010280";
static const char* SET_EMPTY_BARE     = "80";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the sub_transaction.
 * @return A new instance of the sub_transaction.
 */
static cardano_sub_transaction_t*
new_default_sub_transaction(const char* cbor)
{
  cardano_sub_transaction_t* sub_transaction = nullptr;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_unref(&sub_transaction);
    return nullptr;
  }

  return sub_transaction;
}

/**
 * Decodes the given CBOR hex string into a sub transaction set.
 * @return A new instance of the sub transaction set.
 */
static cardano_sub_transaction_set_t*
new_default_sub_transaction_set(const char* cbor)
{
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_set_unref(&sub_transaction_set);
    return nullptr;
  }

  return sub_transaction_set;
}

/**
 * Creates a blake2b hash from the given hex string.
 * @return A new instance of the blake2b hash.
 */
static cardano_blake2b_hash_t*
new_default_hash(const char* hex)
{
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(hex, strlen(hex), &hash);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  return hash;
}

/**
 * Encodes the given sub transaction set to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_sub_transaction_set(cardano_sub_transaction_set_t* sub_transaction_set)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_sub_transaction_set_to_cbor(sub_transaction_set, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/**
 * Checks that the element at the given index of the set has the given id.
 */
static void
expect_id_at(cardano_sub_transaction_set_t* sub_transaction_set, const size_t index, const char* id_hex)
{
  cardano_sub_transaction_t* element = nullptr;
  EXPECT_EQ(cardano_sub_transaction_set_get(sub_transaction_set, index, &element), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* id          = cardano_sub_transaction_get_id(element);
  cardano_blake2b_hash_t* expected_id = new_default_hash(id_hex);

  EXPECT_TRUE(cardano_blake2b_hash_equals(id, expected_id));

  cardano_blake2b_hash_unref(&expected_id);
  cardano_blake2b_hash_unref(&id);
  cardano_sub_transaction_unref(&element);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_sub_transaction_set_new, canCreateSubTransactionSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  // Act
  cardano_error_t error = cardano_sub_transaction_set_new(&sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_new, returnsErrorIfSubTransactionSetIsNull)
{
  // Act
  cardano_error_t error = cardano_sub_transaction_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  // Act
  cardano_error_t error = cardano_sub_transaction_set_new(&sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_sub_transaction_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  // Act
  cardano_error_t error = cardano_sub_transaction_set_new(&sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_sub_transaction_set_from_cbor, canDeserializeTaggedSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(SET_TAGGED_CBOR, strlen(SET_TAGGED_CBOR));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));
  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transaction_set), 2);
  EXPECT_TRUE(cardano_sub_transaction_set_is_tagged(sub_transaction_set));

  expect_id_at(sub_transaction_set, 0, SUB_WITH_AUX_DATA_ID_HEX);
  expect_id_at(sub_transaction_set, 1, SUB_WITHOUT_AUX_DATA_ID_HEX);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, canDeserializeBareArray)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(SET_BARE_CBOR, strlen(SET_BARE_CBOR));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transaction_set), 2);
  EXPECT_FALSE(cardano_sub_transaction_set_is_tagged(sub_transaction_set));

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfEmptySet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(SET_EMPTY_BARE, strlen(SET_EMPTY_BARE));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'sub_transaction_set', the set must not be empty.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfEmptyTaggedSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(SET_EMPTY_TAGGED, strlen(SET_EMPTY_TAGGED));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfDuplicatedId)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(SET_DUPLICATE_CBOR, strlen(SET_DUPLICATE_CBOR));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'sub_transaction_set', the set must not contain duplicated elements.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfInvalidElement)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex("8182a200d90102800180a0", 22);

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfInvalidTag)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex("d901008183a200d90102800180a0f6", 30);

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfSubTransactionSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SET_TAGGED_CBOR, strlen(SET_TAGGED_CBOR));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(nullptr, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(SET_TAGGED_CBOR, strlen(SET_TAGGED_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_set_to_cbor, canRoundTripTaggedSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_TAGGED_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  // Act
  char* actual_cbor = encode_sub_transaction_set(sub_transaction_set);

  // Assert
  EXPECT_STREQ(actual_cbor, SET_TAGGED_CBOR);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  free(actual_cbor);
}

TEST(cardano_sub_transaction_set_to_cbor, canRoundTripBareArray)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_BARE_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  // Act
  char* actual_cbor = encode_sub_transaction_set(sub_transaction_set);

  // Assert
  EXPECT_STREQ(actual_cbor, SET_BARE_CBOR);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  free(actual_cbor);
}

TEST(cardano_sub_transaction_set_to_cbor, preservesSubTransactionOrder)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_REVERSED_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  expect_id_at(sub_transaction_set, 0, SUB_WITHOUT_AUX_DATA_ID_HEX);
  expect_id_at(sub_transaction_set, 1, SUB_WITH_AUX_DATA_ID_HEX);

  // Act
  char* actual_cbor = encode_sub_transaction_set(sub_transaction_set);

  // Assert
  EXPECT_STREQ(actual_cbor, SET_REVERSED_CBOR);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  free(actual_cbor);
}

TEST(cardano_sub_transaction_set_to_cbor, canSerializeSetBuiltWithAdd)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  cardano_error_t error = cardano_sub_transaction_set_new(&sub_transaction_set);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char* sub_transactions[] = { SUB_WITH_AUX_DATA_CBOR, SUB_WITHOUT_AUX_DATA_CBOR };

  for (size_t i = 0; i < 2; ++i)
  {
    cardano_sub_transaction_t* element = new_default_sub_transaction(sub_transactions[i]);

    EXPECT_EQ(cardano_sub_transaction_set_add(sub_transaction_set, element), CARDANO_SUCCESS);

    cardano_sub_transaction_unref(&element);
  }

  // Act
  char* actual_cbor = encode_sub_transaction_set(sub_transaction_set);

  // Assert
  EXPECT_STREQ(actual_cbor, SET_TAGGED_CBOR);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  free(actual_cbor);
}

TEST(cardano_sub_transaction_set_to_cbor, canSerializeAnEmptySubTransactionSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  cardano_error_t error = cardano_sub_transaction_set_new(&sub_transaction_set);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  char* actual_cbor = encode_sub_transaction_set(sub_transaction_set);

  // Assert
  EXPECT_STREQ(actual_cbor, SET_EMPTY_TAGGED);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  free(actual_cbor);
}

TEST(cardano_sub_transaction_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_sub_transaction_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_sub_transaction_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  cardano_error_t error = cardano_sub_transaction_set_new(&sub_transaction_set);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_sub_transaction_set_to_cbor(sub_transaction_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_get_length, returnsZeroIfSubTransactionSetIsNull)
{
  // Act
  size_t length = cardano_sub_transaction_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_sub_transaction_set_get_length, returnsZeroIfSubTransactionSetIsEmpty)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_sub_transaction_set_get_length(sub_transaction_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_get, returnsErrorIfSubTransactionSetIsNull)
{
  // Arrange
  cardano_sub_transaction_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_sub_transaction_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_sub_transaction_set_get(sub_transaction_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_sub_transaction_t* data = nullptr;
  error                           = cardano_sub_transaction_set_get(sub_transaction_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_add, returnsErrorIfSubTransactionSetIsNull)
{
  // Arrange
  cardano_sub_transaction_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_sub_transaction_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_sub_transaction_set_add(sub_transaction_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_add, returnsErrorIfElementIsDuplicated)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_sub_transaction_t* element = new_default_sub_transaction(SUB_WITHOUT_AUX_DATA_CBOR);

  // Act
  EXPECT_EQ(cardano_sub_transaction_set_add(sub_transaction_set, element), CARDANO_SUCCESS);
  error = cardano_sub_transaction_set_add(sub_transaction_set, element);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transaction_set), 1);

  // Cleanup
  cardano_sub_transaction_unref(&element);
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_add, returnsErrorIfIdIsDuplicated)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_sub_transaction_t* element   = new_default_sub_transaction(SUB_WITHOUT_AUX_DATA_CBOR);
  cardano_sub_transaction_t* duplicate = new_default_sub_transaction(SUB_DUPLICATE_ID_CBOR);

  // Act
  EXPECT_EQ(cardano_sub_transaction_set_add(sub_transaction_set, element), CARDANO_SUCCESS);
  error = cardano_sub_transaction_set_add(sub_transaction_set, duplicate);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transaction_set), 1);

  // Cleanup
  cardano_sub_transaction_unref(&element);
  cardano_sub_transaction_unref(&duplicate);
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_add, preservesInsertionOrder)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* sub_transactions[] = { SUB_WITHOUT_AUX_DATA_CBOR, SUB_WITH_AUX_DATA_CBOR };

  for (size_t i = 0; i < 2; ++i)
  {
    cardano_sub_transaction_t* element = new_default_sub_transaction(sub_transactions[i]);

    EXPECT_EQ(cardano_sub_transaction_set_add(sub_transaction_set, element), CARDANO_SUCCESS);

    cardano_sub_transaction_unref(&element);
  }

  // Act & Assert
  expect_id_at(sub_transaction_set, 0, SUB_WITHOUT_AUX_DATA_ID_HEX);
  expect_id_at(sub_transaction_set, 1, SUB_WITH_AUX_DATA_ID_HEX);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_find_by_id, canFindSubTransactionById)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_TAGGED_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  cardano_blake2b_hash_t* id = new_default_hash(SUB_WITHOUT_AUX_DATA_ID_HEX);

  // Act
  cardano_sub_transaction_t* element = nullptr;
  cardano_error_t            error   = cardano_sub_transaction_set_find_by_id(sub_transaction_set, id, &element);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(element, testing::Not((cardano_sub_transaction_t*)nullptr));

  cardano_blake2b_hash_t* found_id = cardano_sub_transaction_get_id(element);
  EXPECT_TRUE(cardano_blake2b_hash_equals(found_id, id));

  // Cleanup
  cardano_blake2b_hash_unref(&found_id);
  cardano_blake2b_hash_unref(&id);
  cardano_sub_transaction_unref(&element);
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_find_by_id, returnsErrorIfIdIsNotInTheSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_TAGGED_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  cardano_blake2b_hash_t* id = new_default_hash(UNKNOWN_ID_HEX);

  // Act
  cardano_sub_transaction_t* element = nullptr;
  cardano_error_t            error   = cardano_sub_transaction_set_find_by_id(sub_transaction_set, id, &element);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(element, (cardano_sub_transaction_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&id);
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_find_by_id, returnsErrorIfSubTransactionSetIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* id = new_default_hash(SUB_WITHOUT_AUX_DATA_ID_HEX);

  // Act
  cardano_sub_transaction_t* element = nullptr;
  cardano_error_t            error   = cardano_sub_transaction_set_find_by_id(nullptr, id, &element);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&id);
}

TEST(cardano_sub_transaction_set_find_by_id, returnsErrorIfIdIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_sub_transaction_t* element = nullptr;
  error                              = cardano_sub_transaction_set_find_by_id(sub_transaction_set, nullptr, &element);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_find_by_id, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* id = new_default_hash(SUB_WITHOUT_AUX_DATA_ID_HEX);

  // Act
  error = cardano_sub_transaction_set_find_by_id(sub_transaction_set, id, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&id);
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_is_tagged, returnsFalseIfSubTransactionSetIsNull)
{
  // Act
  bool is_tagged = cardano_sub_transaction_set_is_tagged(nullptr);

  // Assert
  EXPECT_FALSE(is_tagged);
}

TEST(cardano_sub_transaction_set_is_tagged, returnsTrueIfFromCborReadTaggedSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_TAGGED_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  // Act
  bool is_tagged = cardano_sub_transaction_set_is_tagged(sub_transaction_set);

  // Assert
  EXPECT_TRUE(is_tagged);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_is_tagged, returnsFalseIfFromCborReadUntaggedSet)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_BARE_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  // Act
  bool is_tagged = cardano_sub_transaction_set_is_tagged(sub_transaction_set);

  // Assert
  EXPECT_FALSE(is_tagged);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_set_use_tag, canEncodeBareArrayWhenDisabled)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_TAGGED_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_set_use_tag(sub_transaction_set, false);

  char* actual_cbor = encode_sub_transaction_set(sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_FALSE(cardano_sub_transaction_set_is_tagged(sub_transaction_set));
  EXPECT_STREQ(actual_cbor, SET_BARE_CBOR);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  free(actual_cbor);
}

TEST(cardano_sub_transaction_set_set_use_tag, canEncodeTaggedSetWhenEnabled)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = new_default_sub_transaction_set(SET_BARE_CBOR);
  ASSERT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));

  // Act
  cardano_error_t error = cardano_sub_transaction_set_set_use_tag(sub_transaction_set, true);

  char* actual_cbor = encode_sub_transaction_set(sub_transaction_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_sub_transaction_set_is_tagged(sub_transaction_set));
  EXPECT_STREQ(actual_cbor, SET_TAGGED_CBOR);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  free(actual_cbor);
}

TEST(cardano_sub_transaction_set_set_use_tag, returnsErrorIfSubTransactionSetIsNull)
{
  // Act
  cardano_error_t error = cardano_sub_transaction_set_set_use_tag(nullptr, true);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_sub_transaction_set_ref(sub_transaction_set);

  // Assert
  EXPECT_THAT(sub_transaction_set, testing::Not((cardano_sub_transaction_set_t*)nullptr));
  EXPECT_EQ(cardano_sub_transaction_set_refcount(sub_transaction_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_sub_transaction_set_unref(&sub_transaction_set);
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_transaction_set_ref(nullptr);
}

TEST(cardano_sub_transaction_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;

  // Act
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_transaction_set_unref((cardano_sub_transaction_set_t**)nullptr);
}

TEST(cardano_sub_transaction_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_sub_transaction_set_ref(sub_transaction_set);
  size_t ref_count = cardano_sub_transaction_set_refcount(sub_transaction_set);

  cardano_sub_transaction_set_unref(&sub_transaction_set);
  size_t updated_ref_count = cardano_sub_transaction_set_refcount(sub_transaction_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_sub_transaction_set_ref(sub_transaction_set);
  size_t ref_count = cardano_sub_transaction_set_refcount(sub_transaction_set);

  cardano_sub_transaction_set_unref(&sub_transaction_set);
  size_t updated_ref_count = cardano_sub_transaction_set_refcount(sub_transaction_set);

  cardano_sub_transaction_set_unref(&sub_transaction_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(sub_transaction_set, (cardano_sub_transaction_set_t*)nullptr);

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}

TEST(cardano_sub_transaction_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_sub_transaction_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_sub_transaction_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  const char*                    message             = "This is a test message";

  // Act
  cardano_sub_transaction_set_set_last_error(sub_transaction_set, message);

  // Assert
  EXPECT_STREQ(cardano_sub_transaction_set_get_last_error(sub_transaction_set), "Object is NULL.");
}

TEST(cardano_sub_transaction_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_sub_transaction_set_t* sub_transaction_set = nullptr;
  cardano_error_t                error               = cardano_sub_transaction_set_new(&sub_transaction_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_sub_transaction_set_set_last_error(sub_transaction_set, message);

  // Assert
  EXPECT_STREQ(cardano_sub_transaction_set_get_last_error(sub_transaction_set), "");

  // Cleanup
  cardano_sub_transaction_set_unref(&sub_transaction_set);
}
