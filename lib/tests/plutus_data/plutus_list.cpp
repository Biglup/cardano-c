/**
 * \file plutus_list.cpp
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

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PLUTUS_LIST_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";

/* UNIT TESTS ****************************************************************/

TEST(cardano_plutus_list_new, canCreatePlutusList)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_list_new(&plutus_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_list, testing::Not((cardano_plutus_list_t*)nullptr));

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_new, returnsErrorIfPlutusListIsNull)
{
  // Act
  cardano_error_t error = cardano_plutus_list_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_list_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_plutus_list_t* plutus_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_list_new(&plutus_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_list, (cardano_plutus_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_list_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_plutus_list_t* plutus_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_list_new(&plutus_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_list, (cardano_plutus_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_plutus_list_to_cbor, canSerializeAnEmptyPlutusList)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_list_new(&plutus_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_list_to_cbor(plutus_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "80");

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_list_to_cbor, canSerializeAnSimplePlutusList)
{
  // Arrange
  cardano_plutus_list_t* plutus_list      = nullptr;
  cardano_cbor_writer_t* writer           = cardano_cbor_writer_new();
  const char*            simple_list_cbor = "9f0102030405ff";

  cardano_error_t error = cardano_plutus_list_new(&plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer_from_int(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(plutus_list, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  // Act
  error = cardano_plutus_list_to_cbor(plutus_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(simple_list_cbor) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, simple_list_cbor);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_list_to_cbor, canSerializeAnNestedPlutusList)
{
  // Arrange
  cardano_plutus_list_t* inner_list       = nullptr;
  cardano_cbor_writer_t* writer           = cardano_cbor_writer_new();
  const char*            nested_list_cbor = "9f01029f0102030405ff9f0102030405ff05ff";

  cardano_error_t error = cardano_plutus_list_new(&inner_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer_from_int(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(inner_list, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  cardano_plutus_data_t* inner_data = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_list(inner_list, &inner_data), CARDANO_SUCCESS);
  cardano_plutus_list_unref(&inner_list);

  cardano_plutus_list_t* outer = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&outer), CARDANO_SUCCESS);

  cardano_plutus_data_t* elem1 = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(1, &elem1), CARDANO_SUCCESS);
  cardano_plutus_data_t* elem2 = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &elem2), CARDANO_SUCCESS);
  cardano_plutus_data_t* elem3 = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(5, &elem3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_plutus_list_add(outer, elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(outer, elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(outer, inner_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(outer, inner_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_add(outer, elem3), CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_list_to_cbor(outer, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(nested_list_cbor) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, nested_list_cbor);

  // Cleanup
  cardano_plutus_list_unref(&outer);
  cardano_plutus_data_unref(&inner_data);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
  cardano_plutus_data_unref(&elem3);

  free(actual_cbor);
}

TEST(cardano_plutus_list_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_plutus_list_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_list_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&plutus_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_list_to_cbor(plutus_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("9f0102ff", strlen("9f0102ff"));
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_list_from_cbor(reader, &plutus_list);
  cardano_plutus_list_clear_cbor_cache(plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_to_cbor(plutus_list, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("9f0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "9f0102ff");

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_plutus_list_from_cbor, canDeserializePlutusList)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(PLUTUS_LIST_CBOR, strlen(PLUTUS_LIST_CBOR));

  // Act
  cardano_error_t error = cardano_plutus_list_from_cbor(reader, &plutus_list);
  cardano_plutus_list_clear_cbor_cache(plutus_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(plutus_list, testing::Not((cardano_plutus_list_t*)nullptr));

  const size_t length = cardano_plutus_list_get_length(plutus_list);

  EXPECT_EQ(length, 5);

  cardano_plutus_data_t* elem1 = NULL;
  cardano_plutus_data_t* elem2 = NULL;
  cardano_plutus_data_t* elem3 = NULL;
  cardano_plutus_data_t* elem4 = NULL;
  cardano_plutus_data_t* elem5 = NULL;

  EXPECT_EQ(cardano_plutus_list_get(plutus_list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(plutus_list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(plutus_list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(plutus_list, 3, &elem4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get(plutus_list, 4, &elem5), CARDANO_SUCCESS);

  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;

  EXPECT_EQ(cardano_plutus_data_get_kind(elem1, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

  EXPECT_EQ(cardano_plutus_data_get_kind(elem2, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

  EXPECT_EQ(cardano_plutus_data_get_kind(elem3, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_LIST);

  EXPECT_EQ(cardano_plutus_data_get_kind(elem4, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_LIST);

  EXPECT_EQ(cardano_plutus_data_get_kind(elem5, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(elem1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_plutus_data_to_integer(elem2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 2);
  cardano_bigint_unref(&value);

  cardano_plutus_list_t* plutus_list2 = nullptr;
  EXPECT_EQ(cardano_plutus_data_to_list(elem3, &plutus_list2), CARDANO_SUCCESS);

  for (size_t i = 0; i < cardano_plutus_list_get_length(plutus_list2); ++i)
  {
    cardano_plutus_data_t* elem = nullptr;
    EXPECT_EQ(cardano_plutus_list_get(plutus_list2, i, &elem), CARDANO_SUCCESS);

    EXPECT_EQ(cardano_plutus_data_get_kind(elem, &kind), CARDANO_SUCCESS);
    EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

    EXPECT_EQ(cardano_plutus_data_to_integer(elem, &value), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_bigint_to_int(value), i + 1);
    cardano_bigint_unref(&value);

    cardano_plutus_data_unref(&elem);
  }

  cardano_plutus_list_t* plutus_list3 = nullptr;
  EXPECT_EQ(cardano_plutus_data_to_list(elem4, &plutus_list3), CARDANO_SUCCESS);

  for (size_t i = 0; i < cardano_plutus_list_get_length(plutus_list3); ++i)
  {
    cardano_plutus_data_t* elem = nullptr;
    EXPECT_EQ(cardano_plutus_list_get(plutus_list3, i, &elem), CARDANO_SUCCESS);

    EXPECT_EQ(cardano_plutus_data_get_kind(elem, &kind), CARDANO_SUCCESS);
    EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

    EXPECT_EQ(cardano_plutus_data_to_integer(elem, &value), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_bigint_to_int(value), i + 1);
    cardano_bigint_unref(&value);

    cardano_plutus_data_unref(&elem);
  }

  EXPECT_EQ(cardano_plutus_data_to_integer(elem5, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
  cardano_cbor_reader_unref(&reader);

  cardano_plutus_data_unref(&elem1);
  cardano_plutus_data_unref(&elem2);
  cardano_plutus_data_unref(&elem3);
  cardano_plutus_data_unref(&elem4);
  cardano_plutus_data_unref(&elem5);

  cardano_plutus_list_unref(&plutus_list2);
  cardano_plutus_list_unref(&plutus_list3);
}

TEST(cardano_plutus_list_from_cbor, returnErrorIfPlutusListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PLUTUS_LIST_CBOR, strlen(PLUTUS_LIST_CBOR));

  // Act
  cardano_error_t error = cardano_plutus_list_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_list_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_list_from_cbor(nullptr, &plutus_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_list_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(PLUTUS_LIST_CBOR, strlen(PLUTUS_LIST_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_list_from_cbor(reader, &plutus_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(plutus_list, (cardano_plutus_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_list_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_plutus_list_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_plutus_list_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_list_ref(plutus_list);

  // Assert
  EXPECT_THAT(plutus_list, testing::Not((cardano_plutus_list_t*)nullptr));
  EXPECT_EQ(cardano_plutus_list_refcount(plutus_list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_list_unref(&plutus_list);
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_list_ref(nullptr);
}

TEST(cardano_plutus_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;

  // Act
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_list_unref((cardano_plutus_list_t**)nullptr);
}

TEST(cardano_plutus_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_list_ref(plutus_list);
  size_t ref_count = cardano_plutus_list_refcount(plutus_list);

  cardano_plutus_list_unref(&plutus_list);
  size_t updated_ref_count = cardano_plutus_list_refcount(plutus_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_list_ref(plutus_list);
  size_t ref_count = cardano_plutus_list_refcount(plutus_list);

  cardano_plutus_list_unref(&plutus_list);
  size_t updated_ref_count = cardano_plutus_list_refcount(plutus_list);

  cardano_plutus_list_unref(&plutus_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(plutus_list, (cardano_plutus_list_t*)nullptr);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_plutus_list_set_last_error(plutus_list, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_list_get_last_error(plutus_list), "Object is NULL.");
}

TEST(cardano_plutus_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_list_set_last_error(plutus_list, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_list_get_last_error(plutus_list), "");

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_get_length, returnsZeroIfPlutusListIsNull)
{
  // Act
  size_t length = cardano_plutus_list_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_plutus_list_get_length, returnsZeroIfPlutusListIsEmpty)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_plutus_list_get_length(plutus_list);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_get, returnsErrorIfPlutusListIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_list_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_list_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_list_get(plutus_list, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_t* data = nullptr;
  error                       = cardano_plutus_list_get(plutus_list, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_add, returnsErrorIfPlutusListIsNull)
{
  // Arrange
  cardano_plutus_data_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_plutus_list_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_list_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_list_add(plutus_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_equals, returnsFalseIfEitherPlutusListIsNull)
{
  // Act
  bool equals = cardano_plutus_list_equals((cardano_plutus_list_t*)"", nullptr);

  // Assert
  EXPECT_FALSE(equals);
}

TEST(cardano_plutus_list_equals, returnsTrueIfBothPlutusListsAreNull)
{
  // Act
  bool equals = cardano_plutus_list_equals(nullptr, nullptr);

  // Assert
  EXPECT_TRUE(equals);
}

TEST(cardano_plutus_list_equals, returnsFalseIfOnePlutusListIsNull)
{
  // Arrange
  cardano_plutus_list_t* plutus_list = nullptr;
  cardano_error_t        error       = cardano_plutus_list_new(&plutus_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_plutus_list_equals(plutus_list, nullptr);

  // Assert
  EXPECT_FALSE(equals);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list);
}

TEST(cardano_plutus_list_equals, returnsFalseIfPlutusListsHaveDifferentLengths)
{
  // Arrange
  cardano_plutus_list_t* plutus_list1 = nullptr;
  cardano_plutus_list_t* plutus_list2 = nullptr;

  cardano_plutus_data_t* data1 = nullptr;

  cardano_error_t error = cardano_plutus_data_new_integer_from_int(1, &data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&plutus_list1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&plutus_list2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_add(plutus_list1, data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_plutus_list_equals(plutus_list1, plutus_list2);

  // Assert
  EXPECT_FALSE(equals);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list1);
  cardano_plutus_list_unref(&plutus_list2);
  cardano_plutus_data_unref(&data1);
}

TEST(cardano_plutus_list_equals, returnsFalseIfPlutusListsHaveDifferentElements)
{
  // Arrange
  cardano_plutus_list_t* plutus_list1 = nullptr;
  cardano_plutus_list_t* plutus_list2 = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&plutus_list1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&plutus_list2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* data1 = nullptr;
  cardano_plutus_data_t* data2 = nullptr;

  error = cardano_plutus_data_new_integer_from_int(1, &data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_integer_from_int(2, &data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_add(plutus_list1, data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_add(plutus_list2, data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_plutus_list_equals(plutus_list1, plutus_list2);

  // Assert
  EXPECT_FALSE(equals);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list1);
  cardano_plutus_list_unref(&plutus_list2);
  cardano_plutus_data_unref(&data1);
  cardano_plutus_data_unref(&data2);
}

TEST(cardano_plutus_list_equals, returnsTrueIfPlutusListsAreEqual)
{
  // Arrange
  cardano_plutus_list_t* plutus_list1 = nullptr;
  cardano_plutus_list_t* plutus_list2 = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&plutus_list1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&plutus_list2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* data1 = nullptr;
  cardano_plutus_data_t* data2 = nullptr;

  error = cardano_plutus_data_new_integer_from_int(1, &data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_data_new_integer_from_int(1, &data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_add(plutus_list1, data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_add(plutus_list2, data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_plutus_list_equals(plutus_list1, plutus_list2);

  // Assert
  EXPECT_TRUE(equals);

  // Cleanup
  cardano_plutus_list_unref(&plutus_list1);
  cardano_plutus_list_unref(&plutus_list2);
  cardano_plutus_data_unref(&data1);
  cardano_plutus_data_unref(&data2);
}
