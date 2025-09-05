/**
 * \file metadatum_map.cpp
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

#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/metadatum_list.h>
#include <cardano/auxiliary_data/metadatum_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* METADATUM_MAP_CBOR = "a10102";

/* UNIT TESTS ****************************************************************/

TEST(cardano_metadatum_map_new, canCreateMetadatumMap)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum_map, testing::Not((cardano_metadatum_map_t*)nullptr));

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_new, returnsErrorIfMetadatumMapIsNull)
{
  // Act
  cardano_error_t error = cardano_metadatum_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_map_t* metadatum_map = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum_map, (cardano_metadatum_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_map_t* metadatum_map = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum_map, (cardano_metadatum_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_map_to_cbor, canSerializeAnEmptyMetadatumMap)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_map_to_cbor(metadatum_map, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_map_to_cbor, canSerializeAnSimpleMetadatumMap)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map    = nullptr;
  cardano_cbor_writer_t*   writer           = cardano_cbor_writer_new();
  const char*              simple_list_cbor = "a10102";

  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  error = cardano_metadatum_map_to_cbor(metadatum_map, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(simple_list_cbor) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, simple_list_cbor);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_map_to_cbor, canSerializeIndefiniteMap)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("bf0102ff", strlen("bf0102ff"));
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &metadatum_map);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_map_to_cbor(metadatum_map, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("bf0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "bf0102ff");

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_map_to_cbor, canFindElementInMapInteger)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  cardano_metadatum_t* found = nullptr;

  // Act
  error = cardano_metadatum_map_get(metadatum_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bigint_t* result = NULL;

  EXPECT_EQ(cardano_metadatum_to_integer(found, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_bigint_to_int(result), 2);
  cardano_bigint_unref(&result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&found);
}

TEST(cardano_metadatum_map_to_cbor, canFindElementInMapBytes)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  const uint8_t key_bytes[] = { 0x01, 0x02, 0x03, 0x04 };
  const uint8_t val_bytes[] = { 0x05, 0x06, 0x07, 0x08 };

  EXPECT_EQ(cardano_metadatum_new_bytes(key_bytes, sizeof(key_bytes), &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_bytes(val_bytes, sizeof(val_bytes), &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  cardano_metadatum_t* found = nullptr;

  // Act
  error = cardano_metadatum_map_get(metadatum_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* buffer = nullptr;

  EXPECT_EQ(cardano_metadatum_to_bounded_bytes(found, &buffer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_buffer_get_size(buffer), sizeof(val_bytes));

  const uint8_t* data = cardano_buffer_get_data(buffer);
  for (size_t i = 0; i < sizeof(val_bytes); ++i)
  {
    EXPECT_EQ(data[i], val_bytes[i]);
  }

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&found);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_metadatum_map_to_cbor, canFindElementInMapList)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;

  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  cardano_metadatum_list_t* list = nullptr;

  EXPECT_EQ(cardano_metadatum_list_new(&list), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_new_list(list, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  cardano_metadatum_t* found = nullptr;

  // Act
  error = cardano_metadatum_map_get(metadatum_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // the key is a list, the val is an int
  cardano_bigint_t* result = NULL;

  EXPECT_EQ(cardano_metadatum_to_integer(found, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_bigint_to_int(result), 1);

  cardano_bigint_unref(&result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_unref(&found);
  cardano_metadatum_list_unref(&list);
}

TEST(cardano_metadatum_map_to_cbor, canFindElementInMapWhenKeyIsAMap)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;

  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  cardano_metadatum_map_t* inner_map = nullptr;

  EXPECT_EQ(cardano_metadatum_map_new(&inner_map), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_new_map(inner_map, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  cardano_metadatum_t* found = nullptr;

  // Act
  error = cardano_metadatum_map_get(metadatum_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // the key is a map, the val is an int
  cardano_bigint_t* result = NULL;

  EXPECT_EQ(cardano_metadatum_to_integer(found, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_bigint_to_int(result), 1);
  cardano_bigint_unref(&result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_unref(&found);
  cardano_metadatum_map_unref(&inner_map);
}

TEST(cardano_metadatum_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_metadatum_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_metadatum_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;

  cardano_error_t error = cardano_metadatum_map_new(&metadatum_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_map_to_cbor(metadatum_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("a10102", strlen("a10102"));
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &metadatum_map);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_map_to_cbor(metadatum_map, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("a10102") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a10102");

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_map_from_cbor, canDeserializeMetadatumMap)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(METADATUM_MAP_CBOR, strlen(METADATUM_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &metadatum_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum_map, testing::Not((cardano_metadatum_map_t*)nullptr));

  const size_t length = cardano_metadatum_map_get_length(metadatum_map);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfMetadatumMapIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(METADATUM_MAP_CBOR, strlen(METADATUM_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(nullptr, &metadatum_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(METADATUM_MAP_CBOR, strlen(METADATUM_MAP_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &metadatum_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum_map, (cardano_metadatum_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_metadatum_map_t* list   = nullptr;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfInvalidMetadatumDataElementUnexpectedBreak)
{
  // Arrange
  cardano_metadatum_map_t* list   = nullptr;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("a1ff", 4);

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected break byte.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfInvalidMetadatumDataElementKey)
{
  // Arrange
  cardano_metadatum_map_t* list   = nullptr;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("a1f5f5", 4);

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Invalid CBOR data item type for metadatum.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfInvalidMetadatumDataElementValue)
{
  // Arrange
  cardano_metadatum_map_t* list   = nullptr;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("a101f5", 6);

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Invalid CBOR data item type for metadatum.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_from_cbor, returnErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(METADATUM_MAP_CBOR, strlen(METADATUM_MAP_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_fourteen_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_map_from_cbor(reader, &metadatum_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum_map, (cardano_metadatum_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_map_ref(metadatum_map);

  // Assert
  EXPECT_THAT(metadatum_map, testing::Not((cardano_metadatum_map_t*)nullptr));
  EXPECT_EQ(cardano_metadatum_map_refcount(metadatum_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_map_ref(nullptr);
}

TEST(cardano_metadatum_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;

  // Act
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_map_unref((cardano_metadatum_map_t**)nullptr);
}

TEST(cardano_metadatum_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_map_ref(metadatum_map);
  size_t ref_count = cardano_metadatum_map_refcount(metadatum_map);

  cardano_metadatum_map_unref(&metadatum_map);
  size_t updated_ref_count = cardano_metadatum_map_refcount(metadatum_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_map_ref(metadatum_map);
  size_t ref_count = cardano_metadatum_map_refcount(metadatum_map);

  cardano_metadatum_map_unref(&metadatum_map);
  size_t updated_ref_count = cardano_metadatum_map_refcount(metadatum_map);

  cardano_metadatum_map_unref(&metadatum_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(metadatum_map, (cardano_metadatum_map_t*)nullptr);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_metadatum_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_metadatum_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_metadatum_map_set_last_error(metadatum_map, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_map_get_last_error(metadatum_map), "Object is NULL.");
}

TEST(cardano_metadatum_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_metadatum_map_set_last_error(metadatum_map, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_map_get_last_error(metadatum_map), "");

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_length, returnsZeroIfMetadatumMapIsNull)
{
  // Act
  size_t length = cardano_metadatum_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_metadatum_map_get_length, returnsZeroIfMetadatumMapIsEmpty)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_metadatum_map_get_length(metadatum_map);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get, returnsErrorIfMetadatumMapIsNull)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_map_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_map_get(metadatum_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get, returnsErrorIfKeyNotFound)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_metadatum_t* find = nullptr;
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(3, &find), CARDANO_SUCCESS);

  error = cardano_metadatum_map_get(metadatum_map, find, &data);

  cardano_metadatum_unref(&find);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  error = cardano_metadatum_map_get(metadatum_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_insert, returnsErrorIfMetadatumMapIsNull)
{
  // Arrange
  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_metadatum_map_insert(nullptr, key, val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);
}

TEST(cardano_metadatum_map_insert, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_map_insert(metadatum_map, nullptr, val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&val);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_insert, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_map_insert(metadatum_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_unref(&key);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_insert, returnsErrorIfMemoryAllocationFailes)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_metadatum_map_insert(metadatum_map, key, val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_keys, returnsErrorIfMetadatumMapIsNull)
{
  // Arrange
  cardano_metadatum_list_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_get_keys(nullptr, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_map_get_keys, returnsErrorIfKeysIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_map_get_keys(metadatum_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_keys, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_list_t* keys = nullptr;

  // Act
  error = cardano_metadatum_map_get_keys(metadatum_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(keys, (cardano_metadatum_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_keys, returnsEmptyListIfMetadatumMapIsEmpty)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_list_t* keys = nullptr;

  // Act
  error = cardano_metadatum_map_get_keys(metadatum_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(keys, testing::Not((cardano_metadatum_list_t*)nullptr));

  size_t length = cardano_metadatum_list_get_length(keys);

  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_metadatum_list_unref(&keys);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_keys, returnsListOfKeys)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  cardano_metadatum_list_t* keys = nullptr;

  // Act
  error = cardano_metadatum_map_get_keys(metadatum_map, &keys);

  cardano_metadatum_t* value = nullptr;
  EXPECT_EQ(cardano_metadatum_list_get(keys, 0, &value), CARDANO_SUCCESS);

  cardano_bigint_t* result = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(value, &result), CARDANO_SUCCESS);

  cardano_metadatum_unref(&value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(result), 1);
  EXPECT_THAT(keys, testing::Not((cardano_metadatum_list_t*)nullptr));
  cardano_bigint_unref(&result);

  size_t length = cardano_metadatum_list_get_length(keys);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_metadatum_list_unref(&keys);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_values, returnsErrorIfMetadatumMapIsNull)
{
  // Arrange
  cardano_metadatum_list_t* values = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_get_values(nullptr, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_map_get_values, returnsErrorIfValuesIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_map_get_values(metadatum_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_values, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_list_t* values = nullptr;

  // Act
  error = cardano_metadatum_map_get_values(metadatum_map, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(values, (cardano_metadatum_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_values, returnsEmptyListIfMetadatumMapIsEmpty)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_list_t* values = nullptr;

  // Act
  error = cardano_metadatum_map_get_values(metadatum_map, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(values, testing::Not((cardano_metadatum_list_t*)nullptr));

  size_t length = cardano_metadatum_list_get_length(values);

  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_metadatum_list_unref(&values);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_values, returnsListOfValues)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  cardano_metadatum_list_t* values = nullptr;

  // Act
  error = cardano_metadatum_map_get_values(metadatum_map, &values);

  cardano_metadatum_t* value = nullptr;
  EXPECT_EQ(cardano_metadatum_list_get(values, 0, &value), CARDANO_SUCCESS);

  cardano_bigint_t* result = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(value, &result), CARDANO_SUCCESS);

  cardano_metadatum_unref(&value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(result), 2);
  EXPECT_THAT(values, testing::Not((cardano_metadatum_list_t*)nullptr));

  cardano_bigint_unref(&result);

  size_t length = cardano_metadatum_list_get_length(values);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_metadatum_list_unref(&values);
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_equals, returnsTrueIfBothMetadatumMapsAreNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_metadatum_map_t* other         = nullptr;

  // Act
  bool result = cardano_metadatum_map_equals(metadatum_map, other);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_metadatum_map_equals, returnsFalseIfOtherIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_map_t* other = nullptr;

  // Act
  bool result = cardano_metadatum_map_equals(metadatum_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_equals, returnsFalseIfMetadatumMapIsEmptyAndOtherIsNotEmpty)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_map_t* other = nullptr;
  error                          = cardano_metadatum_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  bool result = cardano_metadatum_map_equals(metadatum_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_map_unref(&other);
}

TEST(cardano_metadatum_map_equals, returnsFalseIfMetadatumMapIsNotEmptyAndOtherIsEmpty)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_map_t* other = nullptr;
  error                          = cardano_metadatum_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  bool result = cardano_metadatum_map_equals(metadatum_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_map_unref(&other);
}

TEST(cardano_metadatum_map_equals, returnsFalseIfMetadatumMapHasDifferentKeysThanOther)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_map_t* other = nullptr;
  error                          = cardano_metadatum_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  key = nullptr;
  val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(3, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(4, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  bool result = cardano_metadatum_map_equals(metadatum_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_map_unref(&other);
}

TEST(cardano_metadatum_map_equals, returnsFalseIfMetadatumMapHasDifferentValuesThanOther)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_map_t* other = nullptr;
  error                          = cardano_metadatum_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  key = nullptr;
  val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(3, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  bool result = cardano_metadatum_map_equals(metadatum_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_map_unref(&other);
}

TEST(cardano_metadatum_map_equals, returnsTrueIfMetadatumMapsAreEqual)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_map_t* other = nullptr;
  error                          = cardano_metadatum_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  bool result = cardano_metadatum_map_equals(metadatum_map, other);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
  cardano_metadatum_map_unref(&other);
}

TEST(cardano_metadatum_map_get_at, returnsErrorIfMetadatumMapIsNull)
{
  // Arrange
  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_map_get_at(nullptr, 0, &key, &val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_map_get_at, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_t* val = nullptr;
  error                    = cardano_metadatum_map_get_at(metadatum_map, 0, nullptr, &val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_at, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_t* key = nullptr;
  error                    = cardano_metadatum_map_get_at(metadatum_map, 0, &key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  cardano_metadatum_t* out_key = nullptr;
  cardano_metadatum_t* out_val = nullptr;
  error                        = cardano_metadatum_map_get_at(metadatum_map, 1, &out_key, &out_val);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(out_key, (cardano_metadatum_t*)nullptr);
  EXPECT_EQ(out_val, (cardano_metadatum_t*)nullptr);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_get_at, returnsKeyAndValueAtGivenIndex)
{
  // Arrange
  cardano_metadatum_map_t* metadatum_map = nullptr;
  cardano_error_t          error         = cardano_metadatum_map_new(&metadatum_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* key = nullptr;
  cardano_metadatum_t* val = nullptr;

  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_map_insert(metadatum_map, key, val), CARDANO_SUCCESS);

  cardano_metadatum_unref(&key);
  cardano_metadatum_unref(&val);

  // Act
  cardano_metadatum_t* out_key = nullptr;
  cardano_metadatum_t* out_val = nullptr;
  error                        = cardano_metadatum_map_get_at(metadatum_map, 0, &out_key, &out_val);

  cardano_bigint_t* key_result = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(out_key, &key_result), CARDANO_SUCCESS);

  cardano_bigint_t* val_result = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(out_val, &val_result), CARDANO_SUCCESS);

  cardano_metadatum_unref(&out_key);
  cardano_metadatum_unref(&out_val);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(key_result), 1);
  EXPECT_EQ(cardano_bigint_to_int(val_result), 2);

  cardano_bigint_unref(&key_result);
  cardano_bigint_unref(&val_result);

  // Cleanup
  cardano_metadatum_map_unref(&metadatum_map);
}

TEST(cardano_metadatum_map_to_cip116_json, returnErrorIfNullPointer)
{
  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  EXPECT_EQ(cardano_metadatum_map_to_cip116_json(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_metadatum_map_to_cip116_json((cardano_metadatum_map_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_json_writer_unref(&writer);
}
