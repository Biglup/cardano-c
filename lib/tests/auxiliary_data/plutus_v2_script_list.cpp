/**
 * \file plutus_v2_script_list.cpp
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#include <cardano/auxiliary_data/plutus_v2_script_list.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                   = "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011";
static const char* PLUTUS_V2_SCRIPT1_CBOR = "4e4d01000033222220051200120011";
static const char* PLUTUS_V2_SCRIPT2_CBOR = "4e4d01000133222220051200120011";
static const char* PLUTUS_V2_SCRIPT3_CBOR = "4e4d01000233222220051200120011";
static const char* PLUTUS_V2_SCRIPT4_CBOR = "4e4d01000333222220051200120011";

/**
 * Creates a new default instance of the plutus_v2_script.
 * @return A new instance of the plutus_v2_script.
 */
static cardano_plutus_v2_script_t*
new_default_plutus_v2_script(const char* cbor)
{
  cardano_plutus_v2_script_t* plutus_v2_script = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_plutus_v2_script_from_cbor(reader, &plutus_v2_script);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_plutus_v2_script_unref(&plutus_v2_script);
    return nullptr;
  }

  return plutus_v2_script;
}

/**
 * Encodes the JSON writer content into a string.
 * @param writer The JSON writer to encode.
 * @return A newly allocated string containing the encoded JSON. The caller is responsible for freeing the memory.
 */
static char*
encode_json(cardano_json_writer_t* writer)
{
  const size_t json_size = cardano_json_writer_get_encoded_size(writer);
  char*        json_str  = static_cast<char*>(malloc(json_size ? json_size : 1));

  if (json_size != 0)
  {
    EXPECT_EQ(cardano_json_writer_encode(writer, json_str, json_size), CARDANO_SUCCESS);
  }
  else
  {
    json_str[0] = '\0';
  }

  return json_str;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_plutus_v2_script_list_new, canCreateTransactionOutputList)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_v2_script_list, testing::Not((cardano_plutus_v2_script_list_t*)nullptr));

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_new, returnsErrorIfTransactionOutputListIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_list_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_v2_script_list, (cardano_plutus_v2_script_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_v2_script_list_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_v2_script_list, (cardano_plutus_v2_script_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_v2_script_list_to_cbor, canSerializeAnEmptyTransactionOutputList)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_v2_script_list_to_cbor(plutus_v2_script_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "80");

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_v2_script_list_to_cbor, canSerializeTransactionOutputList)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* plutus_v2_scripts[] = { PLUTUS_V2_SCRIPT1_CBOR, PLUTUS_V2_SCRIPT2_CBOR, PLUTUS_V2_SCRIPT3_CBOR, PLUTUS_V2_SCRIPT4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_plutus_v2_script_t* plutus_v2_script = new_default_plutus_v2_script(plutus_v2_scripts[i]);

    cardano_error_t result = cardano_plutus_v2_script_list_add(plutus_v2_script_list, plutus_v2_script);
    EXPECT_EQ(result, CARDANO_SUCCESS);

    cardano_plutus_v2_script_unref(&plutus_v2_script);
  }

  // Act
  error = cardano_plutus_v2_script_list_to_cbor(plutus_v2_script_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_v2_script_list_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_v2_script_list_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;

  cardano_error_t error = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_v2_script_list_to_cbor(plutus_v2_script_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(reader, &plutus_v2_script_list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_v2_script_list_to_cbor(plutus_v2_script_list, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_v2_script_list_from_cbor, canDeserializeTransactionOutputList)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(reader, &plutus_v2_script_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_v2_script_list, testing::Not((cardano_plutus_v2_script_list_t*)nullptr));

  const size_t length = cardano_plutus_v2_script_list_get_length(plutus_v2_script_list);

  EXPECT_EQ(length, 4);

  cardano_plutus_v2_script_t* elem1 = NULL;
  cardano_plutus_v2_script_t* elem2 = NULL;
  cardano_plutus_v2_script_t* elem3 = NULL;
  cardano_plutus_v2_script_t* elem4 = NULL;

  EXPECT_EQ(cardano_plutus_v2_script_list_get(plutus_v2_script_list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v2_script_list_get(plutus_v2_script_list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v2_script_list_get(plutus_v2_script_list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v2_script_list_get(plutus_v2_script_list, 3, &elem4), CARDANO_SUCCESS);

  const char* plutus_v2_scripts[] = { PLUTUS_V2_SCRIPT1_CBOR, PLUTUS_V2_SCRIPT2_CBOR, PLUTUS_V2_SCRIPT3_CBOR, PLUTUS_V2_SCRIPT4_CBOR };

  cardano_plutus_v2_script_t* plutus_v2_scripts_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_plutus_v2_script_to_cbor(plutus_v2_scripts_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(plutus_v2_scripts[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, plutus_v2_scripts[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
  cardano_cbor_reader_unref(&reader);

  cardano_plutus_v2_script_unref(&elem1);
  cardano_plutus_v2_script_unref(&elem2);
  cardano_plutus_v2_script_unref(&elem3);
  cardano_plutus_v2_script_unref(&elem4);
}

TEST(cardano_plutus_v2_script_list_from_cbor, returnErrorIfTransactionOutputListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_v2_script_list_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(nullptr, &plutus_v2_script_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_list_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(reader, &plutus_v2_script_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_v2_script_list, (cardano_plutus_v2_script_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_v2_script_list_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_plutus_v2_script_list_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_v2_script_list_from_cbor, returnErrorIfInvalidElements)
{
  // Arrange
  cardano_plutus_v2_script_list_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_v2_script_list_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_plutus_v2_script_list_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_v2_script_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_list_ref(plutus_v2_script_list);

  // Assert
  EXPECT_THAT(plutus_v2_script_list, testing::Not((cardano_plutus_v2_script_list_t*)nullptr));
  EXPECT_EQ(cardano_plutus_v2_script_list_refcount(plutus_v2_script_list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_v2_script_list_ref(nullptr);
}

TEST(cardano_plutus_v2_script_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;

  // Act
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_v2_script_list_unref((cardano_plutus_v2_script_list_t**)nullptr);
}

TEST(cardano_plutus_v2_script_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_list_ref(plutus_v2_script_list);
  size_t ref_count = cardano_plutus_v2_script_list_refcount(plutus_v2_script_list);

  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
  size_t updated_ref_count = cardano_plutus_v2_script_list_refcount(plutus_v2_script_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_list_ref(plutus_v2_script_list);
  size_t ref_count = cardano_plutus_v2_script_list_refcount(plutus_v2_script_list);

  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
  size_t updated_ref_count = cardano_plutus_v2_script_list_refcount(plutus_v2_script_list);

  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(plutus_v2_script_list, (cardano_plutus_v2_script_list_t*)nullptr);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_v2_script_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_v2_script_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  const char*                      message               = "This is a test message";

  // Act
  cardano_plutus_v2_script_list_set_last_error(plutus_v2_script_list, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_v2_script_list_get_last_error(plutus_v2_script_list), "Object is NULL.");
}

TEST(cardano_plutus_v2_script_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_v2_script_list_set_last_error(plutus_v2_script_list, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_v2_script_list_get_last_error(plutus_v2_script_list), "");

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_get_length, returnsZeroIfTransactionOutputListIsNull)
{
  // Act
  size_t length = cardano_plutus_v2_script_list_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_plutus_v2_script_list_get_length, returnsZeroIfTransactionOutputListIsEmpty)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_plutus_v2_script_list_get_length(plutus_v2_script_list);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_get, returnsErrorIfTransactionOutputListIsNull)
{
  // Arrange
  cardano_plutus_v2_script_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_list_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_v2_script_list_get(plutus_v2_script_list, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_t* data = nullptr;
  error                            = cardano_plutus_v2_script_list_get(plutus_v2_script_list, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_add, returnsErrorIfTransactionOutputListIsNull)
{
  // Arrange
  cardano_plutus_v2_script_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_v2_script_list_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_list_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_plutus_v2_script_list_t* plutus_v2_script_list = nullptr;
  cardano_error_t                  error                 = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_v2_script_list_add(plutus_v2_script_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
}

TEST(cardano_plutus_v2_script_list_to_cip116_json, returnsErrorIfGivenNullList)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  const cardano_error_t err = cardano_plutus_v2_script_list_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(err, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_plutus_v2_script_list_to_cip116_json, returnsErrorIfGivenNullWriter)
{
  // Arrange
  cardano_plutus_v2_script_list_t* list = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_list_new(&list), CARDANO_SUCCESS);

  // Act
  const cardano_error_t err = cardano_plutus_v2_script_list_to_cip116_json(list, nullptr);

  // Assert
  EXPECT_EQ(err, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&list);
}

TEST(cardano_plutus_v2_script_list_to_cip116_json, canSerializeEmptyList)
{
  // Arrange
  cardano_json_writer_t*           json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_plutus_v2_script_list_t* list = nullptr;

  ASSERT_EQ(cardano_plutus_v2_script_list_new(&list), CARDANO_SUCCESS);

  // Act
  const cardano_error_t err = cardano_plutus_v2_script_list_to_cip116_json(list, json);

  // Assert
  EXPECT_EQ(err, CARDANO_SUCCESS);

  char* json_str = encode_json(json);
  EXPECT_STREQ(json_str, "[]");

  // Cleanup
  free(json_str);
  cardano_plutus_v2_script_list_unref(&list);
  cardano_json_writer_unref(&json);
}

TEST(cardano_plutus_v2_script_list_to_cip116_json, canSerializeSingleScript)
{
  // Arrange
  cardano_json_writer_t*           json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_plutus_v2_script_list_t* list = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_list_new(&list), CARDANO_SUCCESS);

  const byte_t                BYTES1[] = { 0xDE, 0xAD, 0xBE, 0xEF };
  cardano_plutus_v2_script_t* s1       = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_new_bytes(BYTES1, sizeof(BYTES1), &s1), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_v2_script_list_add(list, s1), CARDANO_SUCCESS);
  cardano_plutus_v2_script_unref(&s1); // list holds its own ref

  // Act
  const cardano_error_t err = cardano_plutus_v2_script_list_to_cip116_json(list, json);

  // Assert
  EXPECT_EQ(err, CARDANO_SUCCESS);

  char* json_str = encode_json(json);
  EXPECT_STREQ(json_str, R"([{"language":"plutus_v2","bytes":"deadbeef"}])");

  // Cleanup
  free(json_str);
  cardano_plutus_v2_script_list_unref(&list);
  cardano_json_writer_unref(&json);
}

TEST(cardano_plutus_v2_script_list_to_cip116_json, canSerializeMultipleScripts)
{
  // Arrange
  cardano_json_writer_t*           json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_plutus_v2_script_list_t* list = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_list_new(&list), CARDANO_SUCCESS);

  const byte_t BYTES1[] = { 0x00, 0x01 };
  const byte_t BYTES2[] = { 0xAA, 0xBB, 0xCC };

  cardano_plutus_v2_script_t* s1 = nullptr;
  cardano_plutus_v2_script_t* s2 = nullptr;

  ASSERT_EQ(cardano_plutus_v2_script_new_bytes(BYTES1, sizeof(BYTES1), &s1), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_v2_script_new_bytes(BYTES2, sizeof(BYTES2), &s2), CARDANO_SUCCESS);

  ASSERT_EQ(cardano_plutus_v2_script_list_add(list, s1), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_v2_script_list_add(list, s2), CARDANO_SUCCESS);

  // Release local refs; the list retains them
  cardano_plutus_v2_script_unref(&s1);
  cardano_plutus_v2_script_unref(&s2);

  // Act
  const cardano_error_t err = cardano_plutus_v2_script_list_to_cip116_json(list, json);

  // Assert
  EXPECT_EQ(err, CARDANO_SUCCESS);

  char* json_str = encode_json(json);
  EXPECT_STREQ(json_str, R"([{"language":"plutus_v2","bytes":"0001"},{"language":"plutus_v2","bytes":"aabbcc"}])");

  // Cleanup
  free(json_str);
  cardano_plutus_v2_script_list_unref(&list);
  cardano_json_writer_unref(&json);
}

TEST(cardano_plutus_v2_script_list_to_cip116_json, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_json_writer_t*           json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_plutus_v2_script_list_t* list = nullptr;
  ASSERT_EQ(cardano_plutus_v2_script_list_new(&list), CARDANO_SUCCESS);

  const byte_t BYTES1[] = { 0x00, 0x01 };
  const byte_t BYTES2[] = { 0xAA, 0xBB, 0xCC };

  cardano_plutus_v2_script_t* s1 = nullptr;
  cardano_plutus_v2_script_t* s2 = nullptr;

  ASSERT_EQ(cardano_plutus_v2_script_new_bytes(BYTES1, sizeof(BYTES1), &s1), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_v2_script_new_bytes(BYTES2, sizeof(BYTES2), &s2), CARDANO_SUCCESS);

  ASSERT_EQ(cardano_plutus_v2_script_list_add(list, s1), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_v2_script_list_add(list, s2), CARDANO_SUCCESS);

  // Release local refs; the list retains them
  cardano_plutus_v2_script_unref(&s1);
  cardano_plutus_v2_script_unref(&s2);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  const cardano_error_t err = cardano_plutus_v2_script_list_to_cip116_json(list, json);

  // Assert
  EXPECT_EQ(err, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_plutus_v2_script_list_unref(&list);
  cardano_json_writer_unref(&json);
  cardano_set_allocators(malloc, realloc, free);
  reset_allocators_run_count();
}