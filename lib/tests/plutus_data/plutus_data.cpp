/**
 * \file plutus_data.cpp
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

#include <cardano/buffer.h>
#include <cardano/plutus_data/plutus_data.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PLUTUS_DATA_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";

/* UNIT TESTS ****************************************************************/

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorWhenMemoryAllocationFailes)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_constr, canCreateAConstrPlutusData)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_constr, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_constr(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_constr, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  cardano_error_t error = cardano_plutus_data_new_constr(constr_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_new_constr, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_map, canCreateNewMap)
{
  // Arrange
  cardano_plutus_data_t* plutus_data     = nullptr;
  cardano_plutus_map_t*  map_plutus_data = nullptr;

  EXPECT_EQ(cardano_plutus_map_new(&map_plutus_data), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_data_new_map(map_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_map_unref(&map_plutus_data);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_map, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_map(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_map, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_plutus_map_t* map_plutus_data = nullptr;
  EXPECT_EQ(cardano_plutus_map_new(&map_plutus_data), CARDANO_SUCCESS);

  cardano_error_t error = cardano_plutus_data_new_map(map_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // cleanup
  cardano_plutus_map_unref(&map_plutus_data);
}

TEST(cardano_plutus_data_new_map, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_plutus_data_t* plutus_data     = nullptr;
  cardano_plutus_map_t*  map_plutus_data = nullptr;
  EXPECT_EQ(cardano_plutus_map_new(&map_plutus_data), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_plutus_data_new_map(map_plutus_data, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_plutus_map_unref(&map_plutus_data);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_list, canCreateANewList)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_data_new_list(list, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_list, returnErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_list(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_list, returnErrorIfSecondArgIsNull)
{
  // Act
  cardano_plutus_list_t* list = nullptr;
  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  cardano_error_t error = cardano_plutus_data_new_list(list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // cleanup
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_new_list, returnErrorIfMemoryAllocationFails)
{
  // Act
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;
  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_plutus_data_new_list(list, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_plutus_list_unref(&list);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_int, canCreateAnIntegerPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorIfPlutusDataIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_int, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes, canCreateABytesPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const uint8_t          bytes[]     = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  const uint8_t bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfBytesIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const uint8_t          bytes[]     = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(nullptr, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  const uint8_t bytes[] = { 0x85, 0x01, 0x02, 0x03, 0x04, 0x05 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes, returnsErrorIfMemoryEventuallyAllocationFails)
{
  // Arrange
  const uint8_t bytes[] = { 0x85, 0x01, 0x02, 0x03, 0x04, 0x05 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes_from_hex, canCreateABytesPlutusDataFromHex)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const char*            hex         = "850102030405";

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  const char* hex = "850102030405";

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfHexIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(nullptr, 0, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  const char* hex = "850102030405";

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_bytes_from_hex, returnsErrorIfMemoryAllocationEventuallyFails)
{
  // Arrange
  const char* hex = "850102030405";

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_bytes_from_hex(hex, strlen(hex), &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeAnIntegerPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, canDecodeNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("24", strlen("24"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), -5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, canDecodeBigPositiveInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 72057594037927936);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryInt)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("24", strlen("24"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryInt2)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("24", strlen("24"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfInvalidBigPositiveInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c2490001000000000000", strlen("c2490001000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigPositiveInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigPositiveInteger2)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c249000100000000000000", strlen("c249000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDecodeBigNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), -72057594037927936);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfInvalidBigNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c3490001000000000000", strlen("c3490001000000000000"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigNegativeInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsBigNegativeInteger2)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("c349000100000000000000", strlen("c349000100000000000000"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeABytesPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("450102030405", strlen("450102030405"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_buffer_t* buffer = NULL;
  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_SUCCESS);

  const uint8_t expected_bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  EXPECT_EQ(cardano_buffer_get_size(buffer), sizeof(expected_bytes));

  const byte_t* bytes = cardano_buffer_get_data(buffer);
  for (size_t i = 0; i < sizeof(expected_bytes); ++i)
  {
    EXPECT_EQ(bytes[i], expected_bytes[i]);
  }

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryBytes)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("450102030405", strlen("450102030405"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeAListPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("9f0102030405ff", strlen("9f0102030405ff"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_plutus_list_t* list = NULL;

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&plutus_data);

  const size_t length = cardano_plutus_list_get_length(list);

  EXPECT_EQ(length, 5);

  cardano_plutus_data_t* elem1 = NULL;
  cardano_plutus_data_t* elem2 = NULL;
  cardano_plutus_data_t* elem3 = NULL;
  cardano_plutus_data_t* elem4 = NULL;
  cardano_plutus_data_t* elem5 = NULL;

  EXPECT_EQ(cardano_plutus_list_get(list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 3, &elem4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 4, &elem5), CARDANO_SUCCESS);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(elem1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 2);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem3, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 3);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem4, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 4);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem5, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_list_unref(&list);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
  cardano_plutus_data_unref(&elem3);
  cardano_plutus_data_unref(&elem4);
  cardano_plutus_data_unref(&elem5);
}

TEST(cardano_plutus_data_from_cbor, canDeserializeAMapPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("a3010402050306", strlen("a3010402050306"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_plutus_map_t* map = NULL;

  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&plutus_data);

  const size_t length = cardano_plutus_map_get_length(map);

  EXPECT_EQ(length, 3);

  cardano_plutus_list_t* keys = NULL;
  cardano_plutus_data_t* key1 = NULL;
  cardano_plutus_data_t* key2 = NULL;
  cardano_plutus_data_t* key3 = NULL;

  cardano_plutus_data_t* value1 = NULL;
  cardano_plutus_data_t* value2 = NULL;
  cardano_plutus_data_t* value3 = NULL;

  EXPECT_EQ(cardano_plutus_map_get_keys(map, &keys), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_list_get(keys, 0, &key1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(keys, 1, &key2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(keys, 2, &key3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_get(map, key1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get(map, key2, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get(map, key3, &value3), CARDANO_SUCCESS);

  cardano_bigint_t* key_value = NULL;
  cardano_bigint_t* value     = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(key1, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 1);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_plutus_data_to_integer(key2, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 2);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_plutus_data_to_integer(key3, &key_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(key_value), 3);
  cardano_bigint_unref(&key_value);

  EXPECT_EQ(cardano_plutus_data_to_integer(value1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 4);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(value2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 5);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(value3, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(value), 6);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_map_unref(&map);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_list_unref(&keys);
  cardano_plutus_data_unref(&key1);
  cardano_plutus_data_unref(&key2);
  cardano_plutus_data_unref(&key3);
  cardano_plutus_data_unref(&value1);
  cardano_plutus_data_unref(&value2);
  cardano_plutus_data_unref(&value3);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfMemoryMap)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("a3010402050306", strlen("a3010402050306"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, canDecodeConstructorPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("d8799f0102ff", strlen("d8799f0102ff"));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));

  cardano_constr_plutus_data_t* constr_plutus_data = NULL;

  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&plutus_data);

  uint64_t               alternative = 0;
  cardano_plutus_list_t* list        = NULL;

  EXPECT_EQ(cardano_constr_plutus_data_get_alternative(constr_plutus_data, &alternative), CARDANO_SUCCESS);
  EXPECT_EQ(alternative, 0);

  EXPECT_EQ(cardano_constr_plutus_data_get_data(constr_plutus_data, &list), CARDANO_SUCCESS);

  const size_t length = cardano_plutus_list_get_length(list);

  EXPECT_EQ(length, 2);

  cardano_plutus_data_t* elem1 = NULL;
  cardano_plutus_data_t* elem2 = NULL;

  EXPECT_EQ(cardano_plutus_list_get(list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(list, 1, &elem2), CARDANO_SUCCESS);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(elem1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 2);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
}

TEST(cardano_plutus_data_from_cbor, returnsErrorIfEventualMemoryAllocationFailsConstrData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("d8799f0102ff", strlen("d8799f0102ff"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_to_cbor, canEncodeConstPlutusDataToCbor)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;
  cardano_plutus_data_t*        elem1              = nullptr;
  cardano_plutus_data_t*        elem2              = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(1, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &elem2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(list, elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(list, elem2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("d8799f0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d8799f0102ff");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, canEncodeMapToCbor)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_map_t*  map         = nullptr;
  cardano_plutus_data_t* key1        = nullptr;
  cardano_plutus_data_t* key2        = nullptr;
  cardano_plutus_data_t* key3        = nullptr;
  cardano_plutus_data_t* value1      = nullptr;
  cardano_plutus_data_t* value2      = nullptr;
  cardano_plutus_data_t* value3      = nullptr;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(1, &key1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &key2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(3, &key3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(4, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(5, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(6, &value3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_new(&map), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_map_insert(map, key1, value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_insert(map, key2, value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_insert(map, key3, value3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_map(map, &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("a3010402050306") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a3010402050306");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_map_unref(&map);
  cardano_plutus_data_unref(&key1);
  cardano_plutus_data_unref(&key2);
  cardano_plutus_data_unref(&key3);
  cardano_plutus_data_unref(&value1);
  cardano_plutus_data_unref(&value2);
  cardano_plutus_data_unref(&value3);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, canEncodeSmallByteArray)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  const uint8_t bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  EXPECT_EQ(cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("450102030405") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "450102030405");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, canEncodeBigByteArray)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  /* clang-format off */
  const uint8_t bytes[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0xaa, 0xaa
  };
  /* clang-format on */

  EXPECT_EQ(cardano_plutus_data_new_bytes(bytes, sizeof(bytes), &plutus_data), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 539);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "5f58400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070858400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070858400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070858400102030405060708010203040506070801020304050607080102030405060708010203040506070801020304050607080102030405060708010203040506070842aaaaff");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_data_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_data_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_data_to_cbor(plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("9f0102ff", strlen("9f0102ff"));
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_to_cbor(plutus_data, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("9f0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "9f0102ff");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfPlutusListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PLUTUS_DATA_CBOR, strlen(PLUTUS_DATA_CBOR));

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(nullptr, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(PLUTUS_DATA_CBOR, strlen(PLUTUS_DATA_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfInvalidPlutusDataElements)
{
  // Arrange
  cardano_plutus_data_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Invalid CBOR data item type for plutus data.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_plutus_data_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected end of buffer.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_data_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_ref(plutus_data);

  // Assert
  EXPECT_THAT(plutus_data, testing::Not((cardano_plutus_data_t*)nullptr));
  EXPECT_EQ(cardano_plutus_data_refcount(plutus_data), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_data_ref(nullptr);
}

TEST(cardano_plutus_data_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_data_unref((cardano_plutus_data_t**)nullptr);
}

TEST(cardano_plutus_data_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_ref(plutus_data);
  size_t ref_count = cardano_plutus_data_refcount(plutus_data);

  cardano_plutus_data_unref(&plutus_data);
  size_t updated_ref_count = cardano_plutus_data_refcount(plutus_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_ref(plutus_data);
  size_t ref_count = cardano_plutus_data_refcount(plutus_data);

  cardano_plutus_data_unref(&plutus_data);
  size_t updated_ref_count = cardano_plutus_data_refcount(plutus_data);

  cardano_plutus_data_unref(&plutus_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(plutus_data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_data_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_data_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_plutus_data_set_last_error(plutus_data, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_data_get_last_error(plutus_data), "Object is NULL.");
}

TEST(cardano_plutus_data_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_data_set_last_error(plutus_data, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_data_get_last_error(plutus_data), "");

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_get_kind, returnsTheKindOfPlutusData)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;

  EXPECT_EQ(cardano_plutus_data_get_kind(plutus_data, &kind), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_get_kind, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;

  EXPECT_EQ(cardano_plutus_data_get_kind(plutus_data, &kind), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_get_kind, returnsErrorIfKindIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;
  EXPECT_EQ(cardano_plutus_data_get_kind(plutus_data, nullptr), CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_CONSTR);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_integer, returnsTheIntegerValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_bigint_to_int(value), 1);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_bigint_unref(&value);
}

TEST(cardano_plutus_data_to_integer, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_integer, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, nullptr), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_integer, returnsErrorIfPlutusDataIsNotAnInteger)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_t* value = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(plutus_data, &value), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_bytes, returnsTheBytesValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_buffer_get_size(buffer), 4);
  EXPECT_EQ(memcmp(cardano_buffer_get_data(buffer), "test", 4), 0);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_plutus_data_to_bytes, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, nullptr), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_bytes, returnsErrorIfPlutusDataIsNotAByteArray)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* buffer = NULL;

  EXPECT_EQ(cardano_plutus_data_to_bounded_bytes(plutus_data, &buffer), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_constr, returnsTheConstrValue)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list, &constr_plutus_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data), CARDANO_SUCCESS);
  cardano_constr_plutus_data_unref(&constr_plutus_data);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(constr_plutus_data, testing::Not((cardano_constr_plutus_data_t*)nullptr));

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_to_constr, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_constr_plutus_data_t* constr_plutus_data = NULL;

  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
}

TEST(cardano_plutus_data_to_constr, returnsErrorIfConstrPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_integer_from_int(1, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, nullptr), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_constr, returnsErrorIfPlutusDataIsNotAConstr)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_constr_plutus_data_t* constr_plutus_data = NULL;

  EXPECT_EQ(cardano_plutus_data_to_constr(plutus_data, &constr_plutus_data), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_map, returnsTheMapValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_map_t*  map         = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map, &plutus_data);
  cardano_plutus_map_unref(&map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(map, testing::Not((cardano_plutus_map_t*)nullptr));

  // Cleanup
  cardano_plutus_map_unref(&map);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_map, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_map_t* map = NULL;

  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_map_unref(&map);
}

TEST(cardano_plutus_data_to_map, returnsErrorIfMapIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_map_t*  map         = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, nullptr), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_map_unref(&map);
}

TEST(cardano_plutus_data_to_map, returnsErrorIfPlutusDataIsNotAMap)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_map_t* map = NULL;

  EXPECT_EQ(cardano_plutus_data_to_map(plutus_data, &map), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_list, returnsTheListValue)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list, &plutus_data);
  cardano_plutus_list_unref(&list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_plutus_list_t*)nullptr));

  // Cleanup
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_to_list, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;

  // Act
  cardano_plutus_list_t* list = NULL;

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_to_list, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_plutus_list_t* list        = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, nullptr), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_plutus_data_to_list, returnsErrorIfPlutusDataIsNotAList)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_error_t        error       = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_list_t* list = NULL;

  EXPECT_EQ(cardano_plutus_data_to_list(plutus_data, &list), CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothPlutusDataAreEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_integer_from_int(1, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsFalseIfPlutusDataAreDifferent)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_integer_from_int(2, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsFalseIfPlutusDataAreDifferentTypes)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsFalseIfPlutusDataAreNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);
}

TEST(cardano_plutus_data_equals, returnsFalseIfOnePlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
}

TEST(cardano_plutus_data_equals, returnsFalseIfBothPlutusDataAreNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), false);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothAreConstrPlutusDataAndEqual)
{
  // Arrange
  cardano_plutus_data_t*        plutus_data1        = nullptr;
  cardano_plutus_data_t*        plutus_data2        = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data1 = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data2 = nullptr;
  cardano_plutus_list_t*        list1               = nullptr;
  cardano_plutus_list_t*        list2               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_new(&list2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_constr_plutus_data_new(0, list1, &constr_plutus_data1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_constr_plutus_data_new(0, list2, &constr_plutus_data2), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data1, &plutus_data1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_constr(constr_plutus_data2, &plutus_data2), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data1);
  cardano_constr_plutus_data_unref(&constr_plutus_data2);
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
  cardano_plutus_list_unref(&list1);
  cardano_plutus_list_unref(&list2);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothArePlutusMapAndEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;
  cardano_plutus_map_t*  map1         = nullptr;
  cardano_plutus_map_t*  map2         = nullptr;

  cardano_error_t error = cardano_plutus_map_new(&map1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_map_new(&map2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_map(map2, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_map_unref(&map1);
  cardano_plutus_map_unref(&map2);
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothArePlutusListAndEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;
  cardano_plutus_list_t* list1        = nullptr;
  cardano_plutus_list_t* list2        = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&list2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list1, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_list(list2, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_list_unref(&list1);
  cardano_plutus_list_unref(&list2);
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_equals, returnsTrueIfBothAreBytesAndEqual)
{
  // Arrange
  cardano_plutus_data_t* plutus_data1 = nullptr;
  cardano_plutus_data_t* plutus_data2 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_bytes((const uint8_t*)"test", 4, &plutus_data2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_plutus_data_equals(plutus_data1, plutus_data2), true);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data1);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_plutus_data_new_integer, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer(nullptr, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer, returnsErrorIfIntegerIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer((const cardano_bigint_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_bigint_t* integer = NULL;
  EXPECT_EQ(cardano_bigint_from_int(1, &integer), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer(integer, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_uint, returnsErrorIfPlutusDataIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_uint, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_uint, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(data, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_new_integer_from_uint, canReturnUint)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_uint(0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bigint_t* integer = nullptr;
  error                     = cardano_plutus_data_to_integer(data, &integer);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_unsigned_int(integer), 0);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfPlutusDataIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("0", 1, 10, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfStringIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string(nullptr, 0, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfEmptyString)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("", 0, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsErrorIfInvalidString)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("a", 1, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_CONVERSION_ERROR);

  // Cleanup
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_new_integer_from_string, returnsPlutusDataWithCorrectNumber)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_data_new_integer_from_string("123", 3, 10, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bigint_t* integer = nullptr;
  error                     = cardano_plutus_data_to_integer(data, &integer);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(integer), 123);

  // Cleanup
  cardano_bigint_unref(&integer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfMemoryAllocationFailsWhileReadingUint)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("00", strlen("00"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_from_cbor(reader, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_plutus_data_unref(&data);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_from_cbor, returnErrorIfMemoryAllocationFailsWhileReadingUint2)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("00", strlen("00"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_from_cbor(reader, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_plutus_data_unref(&data);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_data_to_cbor, canSerializeMaxUint64AsUnsignedInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_uint(UINT64_MAX, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "1bffffffffffffffff");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeSmallUint64AsUnsignedInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_uint(1U, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "01");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeMinInt64AsInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_int(INT64_MIN, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "3b7fffffffffffffff");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeSmallIntAsInt)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_int(-1, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "20");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_plutus_data_to_cbor, canSerializeBigInteger)
{
  // Arrange
  cardano_plutus_data_t* data  = nullptr;
  cardano_error_t        error = cardano_plutus_data_new_integer_from_string("340199290171201906221318119490500689920", strlen("340199290171201906221318119490500689920"), 10, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  error = cardano_plutus_data_to_cbor(data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex  = (char*)malloc(cbor_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, "c250fff00000000000000000000000000000");

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&data);
}