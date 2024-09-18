/**
 * \file transaction_output_list.cpp
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                     = "84a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a02820058200000000000000000000000000000000000000000000000000000000000000000a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ffa2005826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51b8cd9819561bddc6614011a02faf080";
static const char* TRANSACTION_OUTPUT1_CBOR = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* TRANSACTION_OUTPUT2_CBOR = "83583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a58200000000000000000000000000000000000000000000000000000000000000000";
static const char* TRANSACTION_OUTPUT3_CBOR = "a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff";
static const char* TRANSACTION_OUTPUT4_CBOR = "825826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51bbcd9819561bddc66141a02faf080";

/**
 * Creates a new default instance of the transaction_output.
 * @return A new instance of the transaction_output.
 */
static cardano_transaction_output_t*
new_default_transaction_output(const char* cbor)
{
  cardano_transaction_output_t* transaction_output = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_transaction_output_from_cbor(reader, &transaction_output);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_transaction_output_unref(&transaction_output);
    return nullptr;
  }

  return transaction_output;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_transaction_output_list_new, canCreateTransactionOutputList)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_output_list_new(&transaction_output_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(transaction_output_list, testing::Not((cardano_transaction_output_list_t*)nullptr));

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_new, returnsErrorIfTransactionOutputListIsNull)
{
  // Act
  cardano_error_t error = cardano_transaction_output_list_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_list_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_output_list_t* transaction_output_list = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_output_list_new(&transaction_output_list);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_output_list, (cardano_transaction_output_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_output_list_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_transaction_output_list_t* transaction_output_list = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_output_list_new(&transaction_output_list);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_output_list, (cardano_transaction_output_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_output_list_to_cbor, canSerializeAnEmptyTransactionOutputList)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_cbor_writer_t*             writer                  = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_output_list_new(&transaction_output_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_output_list_to_cbor(transaction_output_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "80");

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_output_list_to_cbor, canSerializeTransactionOutputList)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_cbor_writer_t*             writer                  = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_output_list_new(&transaction_output_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* transaction_outputs[] = { TRANSACTION_OUTPUT1_CBOR, TRANSACTION_OUTPUT2_CBOR, TRANSACTION_OUTPUT3_CBOR, TRANSACTION_OUTPUT4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_transaction_output_t* transaction_output = new_default_transaction_output(transaction_outputs[i]);

    cardano_error_t result = cardano_transaction_output_list_add(transaction_output_list, transaction_output);
    EXPECT_EQ(result, CARDANO_SUCCESS);

    cardano_transaction_output_unref(&transaction_output);
  }

  // Act
  error = cardano_transaction_output_list_to_cbor(transaction_output_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_output_list_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_transaction_output_list_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_transaction_output_list_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;

  cardano_error_t error = cardano_transaction_output_list_new(&transaction_output_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_output_list_to_cbor(transaction_output_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*             writer                  = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_output_list_from_cbor(reader, &transaction_output_list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_output_list_to_cbor(transaction_output_list, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_output_list_from_cbor, canDeserializeTransactionOutputList)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_transaction_output_list_from_cbor(reader, &transaction_output_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(transaction_output_list, testing::Not((cardano_transaction_output_list_t*)nullptr));

  const size_t length = cardano_transaction_output_list_get_length(transaction_output_list);

  EXPECT_EQ(length, 4);

  cardano_transaction_output_t* elem1 = NULL;
  cardano_transaction_output_t* elem2 = NULL;
  cardano_transaction_output_t* elem3 = NULL;
  cardano_transaction_output_t* elem4 = NULL;

  EXPECT_EQ(cardano_transaction_output_list_get(transaction_output_list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_get(transaction_output_list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_get(transaction_output_list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_get(transaction_output_list, 3, &elem4), CARDANO_SUCCESS);

  const char* transaction_outputs[] = { TRANSACTION_OUTPUT1_CBOR, "a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a02820058200000000000000000000000000000000000000000000000000000000000000000", TRANSACTION_OUTPUT3_CBOR, "a2005826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51b8cd9819561bddc6614011a02faf080" };

  cardano_transaction_output_t* transaction_outputs_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_transaction_output_to_cbor(transaction_outputs_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(transaction_outputs[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, transaction_outputs[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
  cardano_cbor_reader_unref(&reader);

  cardano_transaction_output_unref(&elem1);
  cardano_transaction_output_unref(&elem2);
  cardano_transaction_output_unref(&elem3);
  cardano_transaction_output_unref(&elem4);
}

TEST(cardano_transaction_output_list_from_cbor, returnErrorIfTransactionOutputListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_transaction_output_list_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_list_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_output_list_from_cbor(nullptr, &transaction_output_list);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_list_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_transaction_output_list_from_cbor(reader, &transaction_output_list);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_output_list, (cardano_transaction_output_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_list_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_transaction_output_list_t* list   = nullptr;
  cardano_cbor_reader_t*             reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_transaction_output_list_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_list_from_cbor, returnErrorIfInvalidElements)
{
  // Arrange
  cardano_transaction_output_list_t* list   = nullptr;
  cardano_cbor_reader_t*             reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_transaction_output_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_list_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_transaction_output_list_t* list   = nullptr;
  cardano_cbor_reader_t*             reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_transaction_output_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_output_list_ref(transaction_output_list);

  // Assert
  EXPECT_THAT(transaction_output_list, testing::Not((cardano_transaction_output_list_t*)nullptr));
  EXPECT_EQ(cardano_transaction_output_list_refcount(transaction_output_list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_transaction_output_list_unref(&transaction_output_list);
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_output_list_ref(nullptr);
}

TEST(cardano_transaction_output_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;

  // Act
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_output_list_unref((cardano_transaction_output_list_t**)nullptr);
}

TEST(cardano_transaction_output_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_output_list_ref(transaction_output_list);
  size_t ref_count = cardano_transaction_output_list_refcount(transaction_output_list);

  cardano_transaction_output_list_unref(&transaction_output_list);
  size_t updated_ref_count = cardano_transaction_output_list_refcount(transaction_output_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_output_list_ref(transaction_output_list);
  size_t ref_count = cardano_transaction_output_list_refcount(transaction_output_list);

  cardano_transaction_output_list_unref(&transaction_output_list);
  size_t updated_ref_count = cardano_transaction_output_list_refcount(transaction_output_list);

  cardano_transaction_output_list_unref(&transaction_output_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(transaction_output_list, (cardano_transaction_output_list_t*)nullptr);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_transaction_output_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_transaction_output_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  const char*                        message                 = "This is a test message";

  // Act
  cardano_transaction_output_list_set_last_error(transaction_output_list, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_output_list_get_last_error(transaction_output_list), "Object is NULL.");
}

TEST(cardano_transaction_output_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_transaction_output_list_set_last_error(transaction_output_list, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_output_list_get_last_error(transaction_output_list), "");

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_get_length, returnsZeroIfTransactionOutputListIsNull)
{
  // Act
  size_t length = cardano_transaction_output_list_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_transaction_output_list_get_length, returnsZeroIfTransactionOutputListIsEmpty)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_transaction_output_list_get_length(transaction_output_list);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_get, returnsErrorIfTransactionOutputListIsNull)
{
  // Arrange
  cardano_transaction_output_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_output_list_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_list_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_output_list_get(transaction_output_list, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_output_t* data = nullptr;
  error                              = cardano_transaction_output_list_get(transaction_output_list, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}

TEST(cardano_transaction_output_list_add, returnsErrorIfTransactionOutputListIsNull)
{
  // Arrange
  cardano_transaction_output_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_output_list_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_list_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_transaction_output_list_t* transaction_output_list = nullptr;
  cardano_error_t                    error                   = cardano_transaction_output_list_new(&transaction_output_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_output_list_add(transaction_output_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_output_list_unref(&transaction_output_list);
}
