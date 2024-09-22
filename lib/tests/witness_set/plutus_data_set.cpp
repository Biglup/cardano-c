/**
 * \file plutus_data_set.cpp
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
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

#include <cardano/witness_set/plutus_data_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR              = "d90102849f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff";
static const char* CBOR_WITHOUT_TAG  = "849f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff9f01029f0102030405ff9f0102030405ff05ff";
static const char* PLUTUS_DATA1_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";
static const char* PLUTUS_DATA2_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";
static const char* PLUTUS_DATA3_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";
static const char* PLUTUS_DATA4_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";

/**
 * Creates a new default instance of the plutus_data.
 * @return A new instance of the plutus_data.
 */
static cardano_plutus_data_t*
new_default_plutus_data(const char* cbor)
{
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_plutus_data_unref(&plutus_data);
    return nullptr;
  }

  return plutus_data;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_plutus_data_set_new, canCreatePlutusDataSet)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_set_new(&plutus_data_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data_set, testing::Not((cardano_plutus_data_set_t*)nullptr));

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_new, returnsErrorIfPlutusDataSetIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_set_t* plutus_data_set = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_set_new(&plutus_data_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data_set, (cardano_plutus_data_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_set_t* plutus_data_set = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_set_new(&plutus_data_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data_set, (cardano_plutus_data_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_set_to_cbor, canSerializeAnEmptyPlutusDataSet)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_set_new(&plutus_data_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_data_set_to_cbor(plutus_data_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_set_to_cbor, canSerializePlutusDataSet)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* plutus_datas[] = { PLUTUS_DATA1_CBOR, PLUTUS_DATA2_CBOR, PLUTUS_DATA3_CBOR, PLUTUS_DATA4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_plutus_data_t* plutus_data = new_default_plutus_data(plutus_datas[i]);

    EXPECT_EQ(cardano_plutus_data_set_add(plutus_data_set, plutus_data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&plutus_data);
  }

  // Act
  error = cardano_plutus_data_set_to_cbor(plutus_data_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_set_to_cbor, canSerializePlutusDataSetSorted)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* plutus_datas[] = { PLUTUS_DATA1_CBOR, PLUTUS_DATA2_CBOR, PLUTUS_DATA3_CBOR, PLUTUS_DATA4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_plutus_data_t* plutus_data = new_default_plutus_data(plutus_datas[i]);

    EXPECT_EQ(cardano_plutus_data_set_add(plutus_data_set, plutus_data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&plutus_data);
  }

  // Act
  error = cardano_plutus_data_set_to_cbor(plutus_data_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_data_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;

  cardano_error_t error = cardano_plutus_data_set_new(&plutus_data_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_data_set_to_cbor(plutus_data_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &plutus_data_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_set_clear_cbor_cache(plutus_data_set);

  error = cardano_plutus_data_set_to_cbor(plutus_data_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_set_to_cbor, canDeserializeAndReserializeCborFromCache)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &plutus_data_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_set_to_cbor(plutus_data_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_set_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &plutus_data_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_set_clear_cbor_cache(plutus_data_set);

  error = cardano_plutus_data_set_to_cbor(plutus_data_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_set_to_cbor, canDeserializeAndReserializeCborWithoutTagFromCache)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &plutus_data_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_set_to_cbor(plutus_data_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_WITHOUT_TAG) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITHOUT_TAG);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_set_from_cbor, canDeserializePlutusDataSet)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &plutus_data_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data_set, testing::Not((cardano_plutus_data_set_t*)nullptr));

  const size_t length = cardano_plutus_data_set_get_length(plutus_data_set);

  EXPECT_EQ(length, 4);

  cardano_plutus_data_t* elem1 = NULL;
  cardano_plutus_data_t* elem2 = NULL;
  cardano_plutus_data_t* elem3 = NULL;
  cardano_plutus_data_t* elem4 = NULL;

  EXPECT_EQ(cardano_plutus_data_set_get(plutus_data_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_get(plutus_data_set, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_get(plutus_data_set, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_get(plutus_data_set, 3, &elem4), CARDANO_SUCCESS);

  const char* plutus_datas[] = { PLUTUS_DATA1_CBOR, PLUTUS_DATA2_CBOR, PLUTUS_DATA3_CBOR, PLUTUS_DATA4_CBOR };

  cardano_plutus_data_t* plutus_datas_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_plutus_data_to_cbor(plutus_datas_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(plutus_datas[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, plutus_datas[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_cbor_reader_unref(&reader);

  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
  cardano_plutus_data_unref(&elem3);
  cardano_plutus_data_unref(&elem4);
}

TEST(cardano_plutus_data_set_from_cbor, returnErrorIfPlutusDataSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(nullptr, &plutus_data_set);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &plutus_data_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data_set, (cardano_plutus_data_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_plutus_data_set_t* list   = nullptr;
  cardano_cbor_reader_t*     reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_set_from_cbor, returnErrorIfInvalidElements)
{
  // Arrange
  cardano_plutus_data_set_t* list   = nullptr;
  cardano_cbor_reader_t*     reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_set_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_plutus_data_set_t* list   = nullptr;
  cardano_cbor_reader_t*     reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_plutus_data_set_t* list   = nullptr;
  cardano_cbor_reader_t*     reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_plutus_data_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_set_ref(plutus_data_set);

  // Assert
  EXPECT_THAT(plutus_data_set, testing::Not((cardano_plutus_data_set_t*)nullptr));
  EXPECT_EQ(cardano_plutus_data_set_refcount(plutus_data_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_data_set_unref(&plutus_data_set);
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_data_set_ref(nullptr);
}

TEST(cardano_plutus_data_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;

  // Act
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_data_set_unref((cardano_plutus_data_set_t**)nullptr);
}

TEST(cardano_plutus_data_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_set_ref(plutus_data_set);
  size_t ref_count = cardano_plutus_data_set_refcount(plutus_data_set);

  cardano_plutus_data_set_unref(&plutus_data_set);
  size_t updated_ref_count = cardano_plutus_data_set_refcount(plutus_data_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_set_ref(plutus_data_set);
  size_t ref_count = cardano_plutus_data_set_refcount(plutus_data_set);

  cardano_plutus_data_set_unref(&plutus_data_set);
  size_t updated_ref_count = cardano_plutus_data_set_refcount(plutus_data_set);

  cardano_plutus_data_set_unref(&plutus_data_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(plutus_data_set, (cardano_plutus_data_set_t*)nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_data_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_data_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  const char*                message         = "This is a test message";

  // Act
  cardano_plutus_data_set_set_last_error(plutus_data_set, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_data_set_get_last_error(plutus_data_set), "Object is NULL.");
}

TEST(cardano_plutus_data_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_data_set_set_last_error(plutus_data_set, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_data_set_get_last_error(plutus_data_set), "");

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_get_length, returnsZeroIfPlutusDataSetIsNull)
{
  // Act
  size_t length = cardano_plutus_data_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_plutus_data_set_get_length, returnsZeroIfPlutusDataSetIsEmpty)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_plutus_data_set_get_length(plutus_data_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_get, returnsErrorIfPlutusDataSetIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_data_set_get(plutus_data_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_t* data = nullptr;
  error                       = cardano_plutus_data_set_get(plutus_data_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_add, returnsErrorIfPlutusDataSetIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_data_set_add(plutus_data_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_clear_cbor_cache, doesNothingIfPlutusDataSetIsNull)
{
  // Act
  cardano_plutus_data_set_clear_cbor_cache(nullptr);
}

TEST(cardano_plutus_data_set_set_use_tag, canSetUseTag)
{
  // Arrange
  cardano_plutus_data_set_t* plutus_data_set = nullptr;
  cardano_error_t            error           = cardano_plutus_data_set_new(&plutus_data_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_set_set_use_tag(plutus_data_set, true), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_get_use_tag(plutus_data_set), true);

  EXPECT_EQ(cardano_plutus_data_set_set_use_tag(plutus_data_set, false), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_get_use_tag(plutus_data_set), false);

  // Cleanup
  cardano_plutus_data_set_unref(&plutus_data_set);
}

TEST(cardano_plutus_data_set_set_use_tag, returnsErrorIfGivenNull)
{
  // Act
  EXPECT_EQ(cardano_plutus_data_set_set_use_tag(nullptr, true), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_get_set_use_tag, returnsFalseIfGivenNull)
{
  // Act
  EXPECT_EQ(cardano_plutus_data_set_get_use_tag(nullptr), false);
}
