/**
 * \file redeemer_list.cpp
 *
 * \author angel.castillo
 * \date   Sep 21, 2024
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

#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR           = "a482000082d8799f0102030405ff821821182c82010182d8799f0102030405ff821821182c82030382d8799f0102030405ff821821182c82040482d8799f0102030405ff821821182c";
static const char* CBOR2          = "a482000182d8799f0102030405ff821821182c82000082d8799f0102030405ff821821182c82000182d8799f0102030405ff821821182c82000382d8799f0102030405ff821821182c82000482d8799f0102030405ff821821182c";
static const char* CBOR_LEGACY    = "84840000d8799f0102030405ff821821182c840101d8799f0102030405ff821821182c840303d8799f0102030405ff821821182c840404d8799f0102030405ff821821182c";
static const char* REDEEMER1_CBOR = "840000d8799f0102030405ff821821182c";
static const char* REDEEMER2_CBOR = "840404d8799f0102030405ff821821182c";
static const char* REDEEMER3_CBOR = "840303d8799f0102030405ff821821182c";
static const char* REDEEMER4_CBOR = "840101d8799f0102030405ff821821182c";
static const char* REDEEMER5_CBOR = "840000d8799f0102030405ff821821182c";
static const char* REDEEMER6_CBOR = "840004d8799f0102030405ff821821182c";
static const char* REDEEMER7_CBOR = "840003d8799f0102030405ff821821182c";
static const char* REDEEMER8_CBOR = "840001d8799f0102030405ff821821182c";
static const char* REDEEMER9_CBOR = "840001d8799f0102030405ff821821182c";

/**
 * Creates a new default instance of the redeemer.
 * @return A new instance of the redeemer.
 */
static cardano_redeemer_t*
new_default_redeemer(const char* cbor)
{
  cardano_redeemer_t*    redeemer = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_redeemer_from_cbor(reader, &redeemer);

  cardano_redeemer_clear_cbor_cache(redeemer);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_redeemer_unref(&redeemer);
    return nullptr;
  }

  return redeemer;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_redeemer_list_new, canCreateRedeemerSet)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;

  // Act
  cardano_error_t error = cardano_redeemer_list_new(&redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(redeemer_list, testing::Not((cardano_redeemer_list_t*)nullptr));

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_new, returnsErrorIfRedeemerSetIsNull)
{
  // Act
  cardano_error_t error = cardano_redeemer_list_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_list_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_redeemer_list_t* redeemer_list = nullptr;

  // Act
  cardano_error_t error = cardano_redeemer_list_new(&redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(redeemer_list, (cardano_redeemer_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_redeemer_list_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_redeemer_list_t* redeemer_list = nullptr;

  // Act
  cardano_error_t error = cardano_redeemer_list_new(&redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(redeemer_list, (cardano_redeemer_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_redeemer_list_to_cbor, canSerializeAnEmptyRedeemerSet)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_redeemer_list_new(&redeemer_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_redeemer_list_to_cbor(redeemer_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_redeemer_list_to_cbor, canSerializeRedeemerSet)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* redeemers[] = { REDEEMER1_CBOR, REDEEMER2_CBOR, REDEEMER3_CBOR, REDEEMER4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_redeemer_t* redeemer = new_default_redeemer(redeemers[i]);

    EXPECT_EQ(cardano_redeemer_list_add(redeemer_list, redeemer), CARDANO_SUCCESS);

    cardano_redeemer_unref(&redeemer);
  }

  // Act
  error = cardano_redeemer_list_to_cbor(redeemer_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_redeemer_list_to_cbor, canSerializeRedeemerSetSorted)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* redeemers[] = { REDEEMER1_CBOR, REDEEMER2_CBOR, REDEEMER3_CBOR, REDEEMER4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_redeemer_t* redeemer = new_default_redeemer(redeemers[i]);

    EXPECT_EQ(cardano_redeemer_list_add(redeemer_list, redeemer), CARDANO_SUCCESS);

    cardano_redeemer_unref(&redeemer);
  }

  // Act
  error = cardano_redeemer_list_to_cbor(redeemer_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_redeemer_list_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_redeemer_list_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_redeemer_list_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;

  cardano_error_t error = cardano_redeemer_list_new(&redeemer_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_redeemer_list_to_cbor(redeemer_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &redeemer_list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_redeemer_list_to_cbor(redeemer_list, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_redeemer_list_to_cbor, canDeserializeAndReserializeLegacy)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(CBOR_LEGACY, strlen(CBOR_LEGACY));
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &redeemer_list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_list_clear_cbor_cache(redeemer_list);

  error = cardano_redeemer_list_to_cbor(redeemer_list, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_redeemer_list_to_cbor, canDeserializeAndReserializeLegacyCache)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(CBOR_LEGACY, strlen(CBOR_LEGACY));
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &redeemer_list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_redeemer_list_to_cbor(redeemer_list, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_LEGACY) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_LEGACY);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_redeemer_list_from_cbor, canDeserializeRedeemerList)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(redeemer_list, testing::Not((cardano_redeemer_list_t*)nullptr));

  const size_t length = cardano_redeemer_list_get_length(redeemer_list);

  EXPECT_EQ(length, 4);

  cardano_redeemer_t* elem1 = NULL;
  cardano_redeemer_t* elem2 = NULL;
  cardano_redeemer_t* elem3 = NULL;
  cardano_redeemer_t* elem4 = NULL;

  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 3, &elem4), CARDANO_SUCCESS);

  const char* redeemers[] = { REDEEMER1_CBOR, REDEEMER4_CBOR, REDEEMER3_CBOR, REDEEMER2_CBOR };

  cardano_redeemer_t* redeemers_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_redeemer_to_cbor(redeemers_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(redeemers[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, redeemers[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_reader_unref(&reader);

  cardano_redeemer_unref(&elem1);
  cardano_redeemer_unref(&elem2);
  cardano_redeemer_unref(&elem3);
  cardano_redeemer_unref(&elem4);
}

TEST(cardano_redeemer_list_from_cbor, canDeserializeRedeemerList2)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(CBOR2, strlen(CBOR2));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(redeemer_list, testing::Not((cardano_redeemer_list_t*)nullptr));

  const size_t length = cardano_redeemer_list_get_length(redeemer_list);

  EXPECT_EQ(length, 4);

  cardano_redeemer_t* elem1 = NULL;
  cardano_redeemer_t* elem2 = NULL;
  cardano_redeemer_t* elem3 = NULL;
  cardano_redeemer_t* elem4 = NULL;

  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 3, &elem4), CARDANO_SUCCESS);

  const char* redeemers[] = { REDEEMER5_CBOR, REDEEMER8_CBOR, REDEEMER9_CBOR, REDEEMER7_CBOR, REDEEMER6_CBOR };

  cardano_redeemer_t* redeemers_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_redeemer_to_cbor(redeemers_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(redeemers[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, redeemers[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_cbor_reader_unref(&reader);

  cardano_redeemer_unref(&elem1);
  cardano_redeemer_unref(&elem2);
  cardano_redeemer_unref(&elem3);
  cardano_redeemer_unref(&elem4);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfRedeemerSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(nullptr, &redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(redeemer_list, (cardano_redeemer_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidRedeemer)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "818404040404";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// a182000082d8799f0102030405ff821821182c
TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidRedeemerKeyArray)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a181008200821821182c";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidRedeemerValueArray)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a18200008100";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidRedeemerTag)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a182ef008200821821182c";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidRedeemerIndex)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a18200ef8200821821182c";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidRedeemerData)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a182000082ef821821182c";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidRedeemerExCosts)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a1820000820000";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidMap)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "00";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidArray)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a10000";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_from_cbor, returnErrorIfInvalidArray2)
{
  // Arrange
  cardano_redeemer_list_t* list   = nullptr;
  const char*              cbor   = "a182000000";
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_redeemer_list_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_list_ref(redeemer_list);

  // Assert
  EXPECT_THAT(redeemer_list, testing::Not((cardano_redeemer_list_t*)nullptr));
  EXPECT_EQ(cardano_redeemer_list_refcount(redeemer_list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_redeemer_list_ref(nullptr);
}

TEST(cardano_redeemer_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;

  // Act
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_redeemer_list_unref((cardano_redeemer_list_t**)nullptr);
}

TEST(cardano_redeemer_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_list_ref(redeemer_list);
  size_t ref_count = cardano_redeemer_list_refcount(redeemer_list);

  cardano_redeemer_list_unref(&redeemer_list);
  size_t updated_ref_count = cardano_redeemer_list_refcount(redeemer_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_list_ref(redeemer_list);
  size_t ref_count = cardano_redeemer_list_refcount(redeemer_list);

  cardano_redeemer_list_unref(&redeemer_list);
  size_t updated_ref_count = cardano_redeemer_list_refcount(redeemer_list);

  cardano_redeemer_list_unref(&redeemer_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(redeemer_list, (cardano_redeemer_list_t*)nullptr);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_redeemer_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_redeemer_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_redeemer_list_set_last_error(redeemer_list, message);

  // Assert
  EXPECT_STREQ(cardano_redeemer_list_get_last_error(redeemer_list), "Object is NULL.");
}

TEST(cardano_redeemer_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_redeemer_list_set_last_error(redeemer_list, message);

  // Assert
  EXPECT_STREQ(cardano_redeemer_list_get_last_error(redeemer_list), "");

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_get_length, returnsZeroIfRedeemerSetIsNull)
{
  // Act
  size_t length = cardano_redeemer_list_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_redeemer_list_get_length, returnsZeroIfRedeemerSetIsEmpty)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_redeemer_list_get_length(redeemer_list);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_get, returnsErrorIfRedeemerSetIsNull)
{
  // Arrange
  cardano_redeemer_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_redeemer_list_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_list_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_redeemer_list_get(redeemer_list, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_redeemer_t* data = nullptr;
  error                    = cardano_redeemer_list_get(redeemer_list, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_add, returnsErrorIfRedeemerSetIsNull)
{
  // Arrange
  cardano_redeemer_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_redeemer_list_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_list_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_redeemer_list_add(redeemer_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_clear_cbor_cache, doesNothingIfRedeemerSetIsNull)
{
  // Act
  cardano_redeemer_list_clear_cbor_cache(nullptr);
}

TEST(cardano_redeemer_list_set_ex_units, returnsErrorIfRedeemerSetIsNull)
{
  // Act
  cardano_error_t error = cardano_redeemer_list_set_ex_units(nullptr, CARDANO_REDEEMER_TAG_SPEND, 0, 0, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_list_set_ex_units, returnsErrorIfElementWithTagAndIndexNotFound)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_redeemer_list_set_ex_units(redeemer_list, CARDANO_REDEEMER_TAG_SPEND, 0, 0, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_set_ex_units, canSetTheExecutionUnits)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* redeemers[] = { REDEEMER1_CBOR, REDEEMER2_CBOR, REDEEMER3_CBOR, REDEEMER4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_redeemer_t* redeemer = new_default_redeemer(redeemers[i]);

    EXPECT_EQ(cardano_redeemer_list_add(redeemer_list, redeemer), CARDANO_SUCCESS);

    cardano_redeemer_unref(&redeemer);
  }

  // Act
  error = cardano_redeemer_list_set_ex_units(redeemer_list, CARDANO_REDEEMER_TAG_SPEND, 0, 1, 2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  cardano_redeemer_t* elem1 = NULL;

  EXPECT_EQ(cardano_redeemer_list_get(redeemer_list, 0, &elem1), CARDANO_SUCCESS);

  cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(elem1);

  uint64_t cpu    = cardano_ex_units_get_cpu_steps(ex_units);
  uint64_t memory = cardano_ex_units_get_memory(ex_units);

  EXPECT_EQ(cpu, 2);
  EXPECT_EQ(memory, 1);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_redeemer_unref(&elem1);
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_redeemer_list_clone, returnsErrorIfRedeemerSetIsNull)
{
  // Act
  cardano_redeemer_list_t* cloned = nullptr;
  cardano_error_t          error  = cardano_redeemer_list_clone(nullptr, &cloned);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cloned, (cardano_redeemer_list_t*)nullptr);
}

TEST(cardano_redeemer_list_clone, returnsErrorIfClonedIsNull)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_redeemer_list_clone(redeemer_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
}

TEST(cardano_redeemer_list_clone, canCloneRedeemerSet)
{
  // Arrange
  cardano_redeemer_list_t* redeemer_list = nullptr;
  cardano_error_t          error         = cardano_redeemer_list_new(&redeemer_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* redeemers[] = { REDEEMER1_CBOR, REDEEMER2_CBOR, REDEEMER3_CBOR, REDEEMER4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_redeemer_t* redeemer = new_default_redeemer(redeemers[i]);

    EXPECT_EQ(cardano_redeemer_list_add(redeemer_list, redeemer), CARDANO_SUCCESS);

    cardano_redeemer_unref(&redeemer);
  }

  // Act
  cardano_redeemer_list_t* cloned = nullptr;
  error                           = cardano_redeemer_list_clone(redeemer_list, &cloned);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(cloned, testing::Not((cardano_redeemer_list_t*)nullptr));

  const size_t length = cardano_redeemer_list_get_length(cloned);

  EXPECT_EQ(length, 4);

  cardano_redeemer_t* elem1 = NULL;
  cardano_redeemer_t* elem2 = NULL;
  cardano_redeemer_t* elem3 = NULL;
  cardano_redeemer_t* elem4 = NULL;

  EXPECT_EQ(cardano_redeemer_list_get(cloned, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(cloned, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(cloned, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(cloned, 3, &elem4), CARDANO_SUCCESS);

  const char* redeemers2[] = { REDEEMER1_CBOR, REDEEMER4_CBOR, REDEEMER3_CBOR, REDEEMER2_CBOR };

  cardano_redeemer_t* redeemers_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_redeemer_to_cbor(redeemers_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(redeemers2[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, redeemers2[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_redeemer_list_unref(&cloned);
  cardano_redeemer_unref(&elem1);
  cardano_redeemer_unref(&elem2);
  cardano_redeemer_unref(&elem3);
  cardano_redeemer_unref(&elem4);
}