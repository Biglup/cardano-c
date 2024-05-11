/**
 * \file plutus_map.cpp
 *
 * \author angel.castillo
 * \date   May 14, 2024
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

#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/plutus_data/constr_plutus_data.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PLUTUS_MAP_CBOR = "a10102";

/* UNIT TESTS ****************************************************************/

TEST(cardano_plutus_map_new, canCreatePlutusMap)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_map, testing::Not((cardano_plutus_map_t*)nullptr));

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_new, returnsErrorIfPlutusMapIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_map_t* plutus_map = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_map, (cardano_plutus_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_map_t* plutus_map = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_map, (cardano_plutus_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_map_to_cbor, canSerializeAnEmptyPlutusMap)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_map_to_cbor(plutus_map, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_map_to_cbor, canSerializeAnSimplePlutusMap)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map       = nullptr;
  cardano_cbor_writer_t* writer           = cardano_cbor_writer_new();
  const char*            simple_list_cbor = "a10102";

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  // Act
  error = cardano_plutus_map_to_cbor(plutus_map, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(simple_list_cbor) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, simple_list_cbor);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_map_to_cbor, canSerializeIndefiniteMap)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex("bf0102ff", strlen("bf0102ff"));
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &plutus_map);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_map_to_cbor(plutus_map, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("bf0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "bf0102ff");

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_map_to_cbor, canFindElementInMapInteger)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_data_t* found = nullptr;

  // Act
  error = cardano_plutus_map_get(plutus_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  int64_t result = 0;

  EXPECT_EQ(cardano_plutus_data_to_integer(found, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 2);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&found);
}

TEST(cardano_plutus_map_to_cbor, canFindElementInMapBytes)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  const uint8_t key_bytes[] = { 0x01, 0x02, 0x03, 0x04 };
  const uint8_t val_bytes[] = { 0x05, 0x06, 0x07, 0x08 };

  EXPECT_EQ(cardano_plutus_data_new_bytes(key_bytes, sizeof(key_bytes), &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_bytes(val_bytes, sizeof(val_bytes), &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_data_t* found = nullptr;

  // Act
  error = cardano_plutus_map_get(plutus_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* buffer = nullptr;

  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(found, &buffer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_buffer_get_size(buffer), sizeof(val_bytes));

  const uint8_t* data = cardano_buffer_get_data(buffer);
  for (size_t i = 0; i < sizeof(val_bytes); ++i)
  {
    EXPECT_EQ(data[i], val_bytes[i]);
  }

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&found);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_plutus_map_to_cbor, canFindElementInMapList)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  cardano_plutus_list_t* list = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_list(list, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(1, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_data_t* found = nullptr;

  // Act
  error = cardano_plutus_map_get(plutus_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // the key is a list, the val is an int
  int64_t result = 0;

  EXPECT_EQ(cardano_plutus_data_to_integer(found, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_data_unref(&found);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_map_to_cbor, canFindElementInMapWhenKeyIsAMap)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  cardano_plutus_map_t* inner_map = nullptr;

  EXPECT_EQ(cardano_plutus_map_new(&inner_map), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_map(inner_map, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(1, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_data_t* found = nullptr;

  // Act
  error = cardano_plutus_map_get(plutus_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // the key is a map, the val is an int
  int64_t result = 0;

  EXPECT_EQ(cardano_plutus_data_to_integer(found, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_data_unref(&found);
  cardano_plutus_map_unref(&inner_map);
}

TEST(cardano_plutus_map_to_cbor, canFindElementInMapWhenKeyIsAConstr)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d8009f0102030405ff", strlen("d8009f0102030405ff"));

  error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(1, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_data_t* found = nullptr;

  // Act
  error = cardano_plutus_map_get(plutus_map, key, &found);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // the key is a constr, the val is an int
  int64_t result = 0;

  EXPECT_EQ(cardano_plutus_data_to_integer(found, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_data_unref(&found);
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&plutus_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_map_to_cbor(plutus_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex("a10102", strlen("a10102"));
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &plutus_map);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_map_to_cbor(plutus_map, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("a10102") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a10102");

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_map_from_cbor, canDeserializePlutusMap)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(PLUTUS_MAP_CBOR, strlen(PLUTUS_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &plutus_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_map, testing::Not((cardano_plutus_map_t*)nullptr));

  const size_t length = cardano_plutus_map_get_length(plutus_map);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfPlutusMapIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PLUTUS_MAP_CBOR, strlen(PLUTUS_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(nullptr, &plutus_map);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(PLUTUS_MAP_CBOR, strlen(PLUTUS_MAP_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &plutus_map);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_map, (cardano_plutus_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_plutus_map_t*  list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfInvalidPlutusDataElementUnexpectedBreak)
{
  // Arrange
  cardano_plutus_map_t*  list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("a1ff", 4);

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected break byte.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfInvalidPlutusDataElementKey)
{
  // Arrange
  cardano_plutus_map_t*  list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("a1f5f5", 4);

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Invalid CBOR data item type for plutus data.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfInvalidPlutusDataElementValue)
{
  // Arrange
  cardano_plutus_map_t*  list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("a101f5", 6);

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Invalid CBOR data item type for plutus data.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_from_cbor, returnErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_map_t*  plutus_map = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(PLUTUS_MAP_CBOR, strlen(PLUTUS_MAP_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_fourteen_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_map_from_cbor(reader, &plutus_map);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_map, (cardano_plutus_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_map_ref(plutus_map);

  // Assert
  EXPECT_THAT(plutus_map, testing::Not((cardano_plutus_map_t*)nullptr));
  EXPECT_EQ(cardano_plutus_map_refcount(plutus_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_map_ref(nullptr);
}

TEST(cardano_plutus_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;

  // Act
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_map_unref((cardano_plutus_map_t**)nullptr);
}

TEST(cardano_plutus_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_map_ref(plutus_map);
  size_t ref_count = cardano_plutus_map_refcount(plutus_map);

  cardano_plutus_map_unref(&plutus_map);
  size_t updated_ref_count = cardano_plutus_map_refcount(plutus_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_map_ref(plutus_map);
  size_t ref_count = cardano_plutus_map_refcount(plutus_map);

  cardano_plutus_map_unref(&plutus_map);
  size_t updated_ref_count = cardano_plutus_map_refcount(plutus_map);

  cardano_plutus_map_unref(&plutus_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(plutus_map, (cardano_plutus_map_t*)nullptr);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_plutus_map_set_last_error(plutus_map, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_map_get_last_error(plutus_map), "Object is NULL.");
}

TEST(cardano_plutus_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_map_set_last_error(plutus_map, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_map_get_last_error(plutus_map), "");

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_length, returnsZeroIfPlutusMapIsNull)
{
  // Act
  size_t length = cardano_plutus_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_plutus_map_get_length, returnsZeroIfPlutusMapIsEmpty)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_plutus_map_get_length(plutus_map);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get, returnsErrorIfPlutusMapIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_map_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_map_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_map_get(plutus_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get, returnsErrorIfKeyNotFound)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_plutus_data_t* find = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer(3, &find), CARDANO_SUCCESS);

  error = cardano_plutus_map_get(plutus_map, find, &data);

  cardano_plutus_data_unref(&find);

  // Assert
  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  // Act
  error = cardano_plutus_map_get(plutus_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_insert, returnsErrorIfPlutusMapIsNull)
{
  // Arrange
  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_map_insert(nullptr, key, val);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);
}

TEST(cardano_plutus_map_insert, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_map_insert(plutus_map, nullptr, val);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&val);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_insert, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_map_insert(plutus_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&key);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_insert, returnsErrorIfMemoryAllocationFailes)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_plutus_map_insert(plutus_map, key, val);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_keys, returnsErrorIfPlutusMapIsNull)
{
  // Arrange
  cardano_plutus_list_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_map_get_keys(nullptr, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_map_get_keys, returnsErrorIfKeysIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_map_get_keys(plutus_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_keys, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_list_t* keys = nullptr;

  // Act
  error = cardano_plutus_map_get_keys(plutus_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(keys, (cardano_plutus_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_keys, returnsEmptyListIfPlutusMapIsEmpty)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_list_t* keys = nullptr;

  // Act
  error = cardano_plutus_map_get_keys(plutus_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(keys, testing::Not((cardano_plutus_list_t*)nullptr));

  size_t length = cardano_plutus_list_get_length(keys);

  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_plutus_list_unref(&keys);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_keys, returnsListOfKeys)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_list_t* keys = nullptr;

  // Act
  error = cardano_plutus_map_get_keys(plutus_map, &keys);

  cardano_plutus_data_t* value = nullptr;
  EXPECT_EQ(cardano_plutus_list_get(keys, 0, &value), CARDANO_SUCCESS);

  int64_t result = 0;
  EXPECT_EQ(cardano_plutus_data_to_integer(value, &result), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result, 1);
  EXPECT_THAT(keys, testing::Not((cardano_plutus_list_t*)nullptr));

  size_t length = cardano_plutus_list_get_length(keys);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_plutus_list_unref(&keys);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_values, returnsErrorIfPlutusMapIsNull)
{
  // Arrange
  cardano_plutus_list_t* values = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_map_get_values(nullptr, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_map_get_values, returnsErrorIfValuesIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_map_get_values(plutus_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_values, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_list_t* values = nullptr;

  // Act
  error = cardano_plutus_map_get_values(plutus_map, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(values, (cardano_plutus_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_values, returnsEmptyListIfPlutusMapIsEmpty)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_list_t* values = nullptr;

  // Act
  error = cardano_plutus_map_get_values(plutus_map, &values);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(values, testing::Not((cardano_plutus_list_t*)nullptr));

  size_t length = cardano_plutus_list_get_length(values);

  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_plutus_list_unref(&values);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_get_values, returnsListOfValues)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  cardano_plutus_list_t* values = nullptr;

  // Act
  error = cardano_plutus_map_get_values(plutus_map, &values);

  cardano_plutus_data_t* value = nullptr;
  EXPECT_EQ(cardano_plutus_list_get(values, 0, &value), CARDANO_SUCCESS);

  int64_t result = 0;
  EXPECT_EQ(cardano_plutus_data_to_integer(value, &result), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(result, 2);
  EXPECT_THAT(values, testing::Not((cardano_plutus_list_t*)nullptr));

  size_t length = cardano_plutus_list_get_length(values);

  EXPECT_EQ(length, 1);

  // Cleanup
  cardano_plutus_list_unref(&values);
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_equals, returnsTrueIfBothPlutusMapsAreNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_plutus_map_t* other      = nullptr;

  // Act
  bool result = cardano_plutus_map_equals(plutus_map, other);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_plutus_map_equals, returnsFalseIfOtherIsNull)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_map_t* other = nullptr;

  // Act
  bool result = cardano_plutus_map_equals(plutus_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
}

TEST(cardano_plutus_map_equals, returnsFalseIfPlutusMapIsEmptyAndOtherIsNotEmpty)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_map_t* other = nullptr;
  error                       = cardano_plutus_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  // Act
  bool result = cardano_plutus_map_equals(plutus_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_map_unref(&other);
}

TEST(cardano_plutus_map_equals, returnsFalseIfPlutusMapIsNotEmptyAndOtherIsEmpty)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_map_t* other = nullptr;
  error                       = cardano_plutus_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  // Act
  bool result = cardano_plutus_map_equals(plutus_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_map_unref(&other);
}

TEST(cardano_plutus_map_equals, returnsFalseIfPlutusMapHasDifferentKeysThanOther)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_map_t* other = nullptr;
  error                       = cardano_plutus_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  key = nullptr;
  val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(3, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(4, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  // Act
  bool result = cardano_plutus_map_equals(plutus_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_map_unref(&other);
}

TEST(cardano_plutus_map_equals, returnsFalseIfPlutusMapHasDifferentValuesThanOther)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_map_t* other = nullptr;
  error                       = cardano_plutus_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  key = nullptr;
  val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(3, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  // Act
  bool result = cardano_plutus_map_equals(plutus_map, other);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_map_unref(&other);
}

TEST(cardano_plutus_map_equals, returnsTrueIfPlutusMapsAreEqual)
{
  // Arrange
  cardano_plutus_map_t* plutus_map = nullptr;
  cardano_error_t       error      = cardano_plutus_map_new(&plutus_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_map_t* other = nullptr;
  error                       = cardano_plutus_map_new(&other);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  cardano_plutus_data_t* val = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer(1, &key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer(2, &val), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(plutus_map, key, val), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_insert(other, key, val), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&val);

  // Act
  bool result = cardano_plutus_map_equals(plutus_map, other);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_plutus_map_unref(&plutus_map);
  cardano_plutus_map_unref(&other);
}