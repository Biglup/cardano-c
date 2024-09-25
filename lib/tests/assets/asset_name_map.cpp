/**
 * \file asset_name_map.cpp
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

#include <cardano/assets/asset_name_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR              = "a349736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303";
static const char* ASSET_NAME_CBOR_1 = "49736b7977616c6b6571";
static const char* ASSET_NAME_CBOR_2 = "49736b7977616c6b6572";
static const char* ASSET_NAME_HEX_1  = "736b7977616c6b6571";
static const char* ASSET_NAME_HEX_2  = "736b7977616c6b6572";
static const char* ASSET_NAME_HEX_3  = "736b7977616c6b6573";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the governance action id.
 * @return A new instance of the governance action id.
 */
static cardano_asset_name_t*
new_default_asset_name(const char* name)
{
  cardano_asset_name_t*  asset_name = NULL;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(name, strlen(name));
  cardano_error_t        result     = cardano_asset_name_from_cbor(reader, &asset_name);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return asset_name;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_asset_name_map_new, canCreateAssetMaps)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_new(&asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(asset_name_map, testing::Not((cardano_asset_name_map_t*)nullptr));

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_new, returnsErrorIfAssetMapsIsNull)
{
  // Act
  cardano_error_t error = cardano_asset_name_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_asset_name_map_t* asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_new(&asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_name_map, (cardano_asset_name_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_asset_name_map_t* asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_new(&asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_name_map, (cardano_asset_name_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_map_to_cbor, canSerializeAnEmptyAssetMaps)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  cardano_error_t error = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_to_cbor(asset_name_map, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_asset_name_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_asset_name_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_asset_name_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_to_cbor(asset_name_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  cardano_error_t error = cardano_asset_name_map_from_cbor(reader, &asset_name_map);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_to_cbor(asset_name_map, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_asset_name_map_from_cbor, returnErrorIfAssetMapsIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_asset_name_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_from_cbor(nullptr, &asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_asset_name_map_from_cbor(reader, &asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(asset_name_map, (cardano_asset_name_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_map_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_asset_name_map_t* list   = nullptr;
  cardano_cbor_reader_t*    reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_asset_name_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_name_map_ref(asset_name_map);

  // Assert
  EXPECT_THAT(asset_name_map, testing::Not((cardano_asset_name_map_t*)nullptr));
  EXPECT_EQ(cardano_asset_name_map_refcount(asset_name_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_name_map_ref(nullptr);
}

TEST(cardano_asset_name_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;

  // Act
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_asset_name_map_unref((cardano_asset_name_map_t**)nullptr);
}

TEST(cardano_asset_name_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_name_map_ref(asset_name_map);
  size_t ref_count = cardano_asset_name_map_refcount(asset_name_map);

  cardano_asset_name_map_unref(&asset_name_map);
  size_t valued_ref_count = cardano_asset_name_map_refcount(asset_name_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(valued_ref_count, 1);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_name_map_ref(asset_name_map);
  size_t ref_count = cardano_asset_name_map_refcount(asset_name_map);

  cardano_asset_name_map_unref(&asset_name_map);
  size_t valued_ref_count = cardano_asset_name_map_refcount(asset_name_map);

  cardano_asset_name_map_unref(&asset_name_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(valued_ref_count, 1);
  EXPECT_EQ(asset_name_map, (cardano_asset_name_map_t*)nullptr);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_asset_name_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_asset_name_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  const char*               message        = "This is a test message";

  // Act
  cardano_asset_name_map_set_last_error(asset_name_map, message);

  // Assert
  EXPECT_STREQ(cardano_asset_name_map_get_last_error(asset_name_map), "Object is NULL.");
}

TEST(cardano_asset_name_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_asset_name_map_set_last_error(asset_name_map, message);

  // Assert
  EXPECT_STREQ(cardano_asset_name_map_get_last_error(asset_name_map), "");

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_from_cbor, returnErrorIfInvalidMap)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("a100", 4);

  // Act
  cardano_error_t error = cardano_asset_name_map_from_cbor(reader, &asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_map_from_cbor, returnErrorIfInvalidAssetName)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("a3ef736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303", strlen("a349736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303"));

  // Act
  cardano_error_t error = cardano_asset_name_map_from_cbor(reader, &asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_map_from_cbor, returnErrorIfInvalidValue)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("a349736b7977616c6b6571ef49736b7977616c6b65720249736b7977616c6b657303", strlen("a349736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303"));

  // Act
  cardano_error_t error = cardano_asset_name_map_from_cbor(reader, &asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_asset_name_map_get_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_asset_name_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_asset_name_map_get_size, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name, 0);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_asset_name_map_get_length(asset_name_map);

  // Assert
  EXPECT_EQ(size, 1);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_map_insert, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t error = cardano_asset_name_map_insert(nullptr, (cardano_asset_name_t*)"", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_insert, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_insert(asset_name_map, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_insert, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_asset_name_map_insert(asset_name_map, asset_name, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_map_insert, keepsElementsSortedByAssetName)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = NULL;
  cardano_asset_name_t* asset_name2 = NULL;
  cardano_asset_name_t* asset_name3 = NULL;

  error = cardano_asset_name_from_hex(ASSET_NAME_HEX_1, strlen(ASSET_NAME_HEX_1), &asset_name1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_from_hex(ASSET_NAME_HEX_2, strlen(ASSET_NAME_HEX_2), &asset_name2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_from_hex(ASSET_NAME_HEX_3, strlen(ASSET_NAME_HEX_3), &asset_name3);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_insert(asset_name_map, asset_name3, 0);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name1, 2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t size = cardano_asset_name_map_get_length(asset_name_map);

  EXPECT_EQ(size, 3);

  cardano_asset_name_t* asset_name1_out = NULL;
  cardano_asset_name_t* asset_name2_out = NULL;
  cardano_asset_name_t* asset_name3_out = NULL;

  EXPECT_EQ(cardano_asset_name_map_get_key_at(asset_name_map, 0, &asset_name1_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_key_at(asset_name_map, 1, &asset_name2_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_key_at(asset_name_map, 2, &asset_name3_out), CARDANO_SUCCESS);

  EXPECT_STREQ(cardano_asset_name_get_string(asset_name1), cardano_asset_name_get_string(asset_name1_out));
  EXPECT_STREQ(cardano_asset_name_get_string(asset_name2), cardano_asset_name_get_string(asset_name2_out));
  EXPECT_STREQ(cardano_asset_name_get_string(asset_name3), cardano_asset_name_get_string(asset_name3_out));

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_asset_name_unref(&asset_name3);
  cardano_asset_name_unref(&asset_name1_out);
  cardano_asset_name_unref(&asset_name2_out);
  cardano_asset_name_unref(&asset_name3_out);
}

TEST(cardano_asset_name_map_insert, canOverrideValueIfAlreadyPresent)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name, 0);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_insert(asset_name_map, asset_name, 1);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  int64_t value = 0;
  error         = cardano_asset_name_map_get(asset_name_map, asset_name, &value);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 1);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_map_get, returnsErrorIfObjectIsNull)
{
  // Arrange
  int64_t               value      = 0;
  cardano_asset_name_t* asset_name = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_get(nullptr, asset_name, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get, returnsErrorIfElementIsNull)
{
  // Act
  cardano_error_t error = cardano_asset_name_map_get((cardano_asset_name_map_t*)"", (cardano_asset_name_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  int64_t value = 0;

  // Act
  error = cardano_asset_name_map_get(asset_name_map, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);
  int64_t               value      = 0;

  // Act
  error = cardano_asset_name_map_get(asset_name_map, asset_name, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_map_get, returnsTheElement)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);
  int64_t               value      = 0;

  error = cardano_asset_name_map_insert(asset_name_map, asset_name, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t value_out = 0;
  error             = cardano_asset_name_map_get(asset_name_map, asset_name, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_map_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  int64_t value1 = 0;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  int64_t value2 = 1;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name1, value1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name2, value2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t value_out = 0;
  error             = cardano_asset_name_map_get(asset_name_map, asset_name1, &value_out);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value1, value_out);

  int64_t value_out2 = 0;
  error              = cardano_asset_name_map_get(asset_name_map, asset_name2, &value_out2);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value1, value_out);
  EXPECT_EQ(value2, value_out2);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_asset_name_map_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_asset_name_t* asset_name = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_get_key_at(nullptr, 0, &asset_name);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_asset_name_map_get_key_at((cardano_asset_name_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = nullptr;

  // Act
  error = cardano_asset_name_map_get_key_at(asset_name_map, 0, &asset_name);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  int64_t value = 0;

  // Act
  cardano_error_t error = cardano_asset_name_map_get_value_at(nullptr, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_asset_name_map_get_value_at((cardano_asset_name_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  int64_t value = 0;

  // Act
  error = cardano_asset_name_map_get_value_at(asset_name_map, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  int64_t value = 0;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t value_out = 0;
  error             = cardano_asset_name_map_get_value_at(asset_name_map, 0, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_asset_name_map_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_asset_name_t* asset_name = nullptr;
  int64_t               value      = 0;

  // Act
  cardano_error_t error = cardano_asset_name_map_get_key_value_at(nullptr, 0, &asset_name, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_key_value_at, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  int64_t value = 0;

  // Act
  cardano_error_t error = cardano_asset_name_map_get_key_value_at((cardano_asset_name_map_t*)"", 0, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_key_value_at, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_asset_name_t* asset_name = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_get_key_value_at((cardano_asset_name_map_t*)"", 0, &asset_name, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = nullptr;
  int64_t               value      = 0;

  // Act
  error = cardano_asset_name_map_get_key_value_at(asset_name_map, 0, &asset_name, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_asset_name_map_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  int64_t value = 0;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_name_t* asset_name_out = nullptr;
  int64_t               value_out      = 0;
  error                                = cardano_asset_name_map_get_key_value_at(asset_name_map, 0, &asset_name_out, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(asset_name, asset_name_out);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_unref(&asset_name);
  cardano_asset_name_unref(&asset_name_out);
}

TEST(cardano_asset_name_map_get_keys, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_asset_name_list_t* asset_names = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_get_keys(nullptr, &asset_names);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_keys, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_asset_name_map_get_keys((cardano_asset_name_map_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_get_keys, returnsTheListOfKeys)
{
  // Arrange
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_error_t           error          = cardano_asset_name_map_new(&asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name1, 0);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(asset_name_map, asset_name2, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_name_list_t* asset_names = nullptr;
  error                                  = cardano_asset_name_map_get_keys(asset_name_map, &asset_names);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_list_get_length(asset_names), 2);

  cardano_asset_name_t* asset_name1_out = nullptr;
  cardano_asset_name_t* asset_name2_out = nullptr;

  error = cardano_asset_name_list_get(asset_names, 0, &asset_name1_out);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_list_get(asset_names, 1, &asset_name2_out);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cardano_asset_name_get_string(asset_name1), cardano_asset_name_get_string(asset_name1_out));
  EXPECT_STREQ(cardano_asset_name_get_string(asset_name2), cardano_asset_name_get_string(asset_name2_out));

  // Cleanup
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_list_unref(&asset_names);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_asset_name_unref(&asset_name1_out);
  cardano_asset_name_unref(&asset_name2_out);
}

TEST(cardano_asset_name_map_add, returnsErrorIfLhsIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_add, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_add((cardano_asset_name_map_t*)"", rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_add, returnsErrorIfOutIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_add, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_map_add, canAddTwoEmptyMaps)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 0);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
}

TEST(cardano_asset_name_map_add, canAddTwoMaps)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 2);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, 1);
  EXPECT_EQ(value2, 1);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_asset_name_map_add, canAddTwoMapsAndAddsPositiveValuesForSameAssetName)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 2);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, 2);
  EXPECT_EQ(value2, 1);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_asset_name_map_add, canAddTwoMapsAndAddsNegativeValuesForSameAssetName)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name1, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name2, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 2);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, -2);
  EXPECT_EQ(value2, -1);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_asset_name_map_add, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_add(lhs_asset_name_map, rhs_asset_name_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
}

TEST(cardano_asset_name_map_subtract, returnsErrorIfLhsIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_subtract, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_subtract((cardano_asset_name_map_t*)"", rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_subtract, returnsErrorIfOutIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_asset_name_map_subtract, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_asset_name_map_subtract, canSubtractTwoEmptyMaps)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 0);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
}

TEST(cardano_asset_name_map_subtract, canSubtractTwoMaps)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 2);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, 1);
  EXPECT_EQ(value2, -1);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_asset_name_map_subtract, canSubtractTwoMapsAndSubtractsPositiveValuesForSameAssetName)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 1);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value2, -1);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_asset_name_map_subtract, canSubtractTwoMapsAndSubtractsNegativeValuesForSameAssetName)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map    = nullptr;
  cardano_asset_name_map_t* result_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name1, 4);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name2, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, &result_asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_map_get_length(result_asset_name_map), 2);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_get(result_asset_name_map, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, -3);
  EXPECT_EQ(value2, 1);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_map_unref(&result_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_asset_name_map_subtract, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_asset_name_map_subtract(lhs_asset_name_map, rhs_asset_name_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
}

TEST(cardano_asset_name_map_equals, returnsErrorIfLhsIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = (cardano_asset_name_map_t*)"";

  // Act
  bool result = cardano_asset_name_map_equals(lhs_asset_name_map, rhs_asset_name_map);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_asset_name_map_equals, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = (cardano_asset_name_map_t*)"";
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  // Act
  bool result = cardano_asset_name_map_equals(lhs_asset_name_map, rhs_asset_name_map);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_asset_name_map_equals, returnsTrueIfBothAreNull)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  // Act
  bool result = cardano_asset_name_map_equals(lhs_asset_name_map, rhs_asset_name_map);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_asset_name_map_equals, returnsTrueIfBothAreEmpty)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_asset_name_map_equals(lhs_asset_name_map, rhs_asset_name_map);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
}

TEST(cardano_asset_name_map_equals, returnsFalseIfLengthsAreDifferent)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_asset_name_map_equals(lhs_asset_name_map, rhs_asset_name_map);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
}

TEST(cardano_asset_name_map_equals, returnsFalseIfValuesAreDifferent)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name1, 2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_asset_name_map_equals(lhs_asset_name_map, rhs_asset_name_map);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
}

TEST(cardano_asset_name_map_equals, returnsTrueIfMapsAreEqual)
{
  // Arrange
  cardano_asset_name_map_t* lhs_asset_name_map = nullptr;
  cardano_asset_name_map_t* rhs_asset_name_map = nullptr;

  cardano_error_t error = cardano_asset_name_map_new(&lhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_new(&rhs_asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_asset_name_map_insert(lhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_asset_name_map_insert(rhs_asset_name_map, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_asset_name_map_equals(lhs_asset_name_map, rhs_asset_name_map);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_asset_name_map_unref(&lhs_asset_name_map);
  cardano_asset_name_map_unref(&rhs_asset_name_map);
  cardano_asset_name_unref(&asset_name1);
}
