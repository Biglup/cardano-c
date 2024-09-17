/**
 * \file transaction_input_set.cpp
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

#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                    = "d90102848258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001021058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001022058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102305";
static const char* CBOR_WITHOUT_TAG        = "848258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001020058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001021058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a090807060504030201001022058258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102305";
static const char* TRANSACTION_INPUT1_CBOR = "8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102005";
static const char* TRANSACTION_INPUT2_CBOR = "8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102105";
static const char* TRANSACTION_INPUT3_CBOR = "8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102205";
static const char* TRANSACTION_INPUT4_CBOR = "8258200102030405060708090a0b0c0d0e0f0e0d0c0b0a09080706050403020100102305";

/**
 * Creates a new default instance of the transaction_input.
 * @return A new instance of the transaction_input.
 */
static cardano_transaction_input_t*
new_default_transaction_input(const char* cbor)
{
  cardano_transaction_input_t* transaction_input = nullptr;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_transaction_input_from_cbor(reader, &transaction_input);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_transaction_input_unref(&transaction_input);
    return nullptr;
  }

  return transaction_input;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_transaction_input_set_new, canCreateHashSet)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_input_set_new(&transaction_input_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(transaction_input_set, testing::Not((cardano_transaction_input_set_t*)nullptr));

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_new, returnsErrorIfHashSetIsNull)
{
  // Act
  cardano_error_t error = cardano_transaction_input_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_input_set_t* transaction_input_set = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_input_set_new(&transaction_input_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_input_set, (cardano_transaction_input_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_input_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_transaction_input_set_t* transaction_input_set = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_input_set_new(&transaction_input_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_input_set, (cardano_transaction_input_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_input_set_to_cbor, canSerializeAnEmptyHashSet)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_input_set_new(&transaction_input_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_input_set_to_cbor(transaction_input_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_input_set_to_cbor, canSerializeHashSet)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_input_set_new(&transaction_input_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* transaction_inputs[] = { TRANSACTION_INPUT1_CBOR, TRANSACTION_INPUT2_CBOR, TRANSACTION_INPUT3_CBOR, TRANSACTION_INPUT4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_transaction_input_t* transaction_input = new_default_transaction_input(transaction_inputs[i]);

    EXPECT_EQ(cardano_transaction_input_set_add(transaction_input_set, transaction_input), CARDANO_SUCCESS);

    cardano_transaction_input_unref(&transaction_input);
  }

  // Act
  error = cardano_transaction_input_set_to_cbor(transaction_input_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_input_set_to_cbor, canSerializeHashSetSorted)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_input_set_new(&transaction_input_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* transaction_inputs[] = { TRANSACTION_INPUT1_CBOR, TRANSACTION_INPUT2_CBOR, TRANSACTION_INPUT3_CBOR, TRANSACTION_INPUT4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_transaction_input_t* transaction_input = new_default_transaction_input(transaction_inputs[i]);

    EXPECT_EQ(cardano_transaction_input_set_add(transaction_input_set, transaction_input), CARDANO_SUCCESS);

    cardano_transaction_input_unref(&transaction_input);
  }

  // Act
  error = cardano_transaction_input_set_to_cbor(transaction_input_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_input_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_transaction_input_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_transaction_input_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;

  cardano_error_t error = cardano_transaction_input_set_new(&transaction_input_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_input_set_to_cbor(transaction_input_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &transaction_input_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_input_set_to_cbor(transaction_input_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_input_set_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &transaction_input_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_input_set_to_cbor(transaction_input_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_input_set_from_cbor, canDeserializeHashSet)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &transaction_input_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(transaction_input_set, testing::Not((cardano_transaction_input_set_t*)nullptr));

  const size_t length = cardano_transaction_input_set_get_length(transaction_input_set);

  EXPECT_EQ(length, 4);

  cardano_transaction_input_t* elem1 = NULL;
  cardano_transaction_input_t* elem2 = NULL;
  cardano_transaction_input_t* elem3 = NULL;
  cardano_transaction_input_t* elem4 = NULL;

  EXPECT_EQ(cardano_transaction_input_set_get(transaction_input_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_get(transaction_input_set, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_get(transaction_input_set, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_get(transaction_input_set, 3, &elem4), CARDANO_SUCCESS);

  const char* transaction_inputs[] = { TRANSACTION_INPUT1_CBOR, TRANSACTION_INPUT2_CBOR, TRANSACTION_INPUT3_CBOR, TRANSACTION_INPUT4_CBOR };

  cardano_transaction_input_t* transaction_inputs_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_transaction_input_to_cbor(transaction_inputs_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(transaction_inputs[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, transaction_inputs[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
  cardano_cbor_reader_unref(&reader);

  cardano_transaction_input_unref(&elem1);
  cardano_transaction_input_unref(&elem2);
  cardano_transaction_input_unref(&elem3);
  cardano_transaction_input_unref(&elem4);
}

TEST(cardano_transaction_input_set_from_cbor, returnErrorIfHashSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(nullptr, &transaction_input_set);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &transaction_input_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_input_set, (cardano_transaction_input_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_transaction_input_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_set_from_cbor, returnErrorIfInvalidRelayElements)
{
  // Arrange
  cardano_transaction_input_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_set_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_transaction_input_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_transaction_input_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_transaction_input_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_input_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_input_set_ref(transaction_input_set);

  // Assert
  EXPECT_THAT(transaction_input_set, testing::Not((cardano_transaction_input_set_t*)nullptr));
  EXPECT_EQ(cardano_transaction_input_set_refcount(transaction_input_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_transaction_input_set_unref(&transaction_input_set);
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_input_set_ref(nullptr);
}

TEST(cardano_transaction_input_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;

  // Act
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_input_set_unref((cardano_transaction_input_set_t**)nullptr);
}

TEST(cardano_transaction_input_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_input_set_ref(transaction_input_set);
  size_t ref_count = cardano_transaction_input_set_refcount(transaction_input_set);

  cardano_transaction_input_set_unref(&transaction_input_set);
  size_t updated_ref_count = cardano_transaction_input_set_refcount(transaction_input_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_input_set_ref(transaction_input_set);
  size_t ref_count = cardano_transaction_input_set_refcount(transaction_input_set);

  cardano_transaction_input_set_unref(&transaction_input_set);
  size_t updated_ref_count = cardano_transaction_input_set_refcount(transaction_input_set);

  cardano_transaction_input_set_unref(&transaction_input_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(transaction_input_set, (cardano_transaction_input_set_t*)nullptr);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_transaction_input_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_transaction_input_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  const char*                      message               = "This is a test message";

  // Act
  cardano_transaction_input_set_set_last_error(transaction_input_set, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_input_set_get_last_error(transaction_input_set), "Object is NULL.");
}

TEST(cardano_transaction_input_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_transaction_input_set_set_last_error(transaction_input_set, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_input_set_get_last_error(transaction_input_set), "");

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_get_length, returnsZeroIfHashSetIsNull)
{
  // Act
  size_t length = cardano_transaction_input_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_transaction_input_set_get_length, returnsZeroIfHashSetIsEmpty)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_transaction_input_set_get_length(transaction_input_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_get, returnsErrorIfHashSetIsNull)
{
  // Arrange
  cardano_transaction_input_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_input_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_input_set_get(transaction_input_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_input_t* data = nullptr;
  error                             = cardano_transaction_input_set_get(transaction_input_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}

TEST(cardano_transaction_input_set_add, returnsErrorIfHashSetIsNull)
{
  // Arrange
  cardano_transaction_input_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_input_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_input_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_transaction_input_set_t* transaction_input_set = nullptr;
  cardano_error_t                  error                 = cardano_transaction_input_set_new(&transaction_input_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_input_set_add(transaction_input_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_input_set_unref(&transaction_input_set);
}
