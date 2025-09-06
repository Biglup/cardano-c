/**
 * \file metadatum_list.cpp
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

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* METADATUM_LIST_CBOR = "9f01029f0102030405ff9f0102030405ff05ff";

/* UNIT TESTS ****************************************************************/

TEST(cardano_metadatum_list_new, canCreateMetadatumList)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum_list, testing::Not((cardano_metadatum_list_t*)nullptr));

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_new, returnsErrorIfMetadatumListIsNull)
{
  // Act
  cardano_error_t error = cardano_metadatum_list_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_list_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_metadatum_list_t* metadatum_list = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum_list, (cardano_metadatum_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_list_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_metadatum_list_t* metadatum_list = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum_list, (cardano_metadatum_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_metadatum_list_to_cbor, canSerializeAnEmptyMetadatumList)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_list_to_cbor(metadatum_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "80");

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_list_to_cbor, canSerializeAnSimpleMetadatumList)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list   = nullptr;
  cardano_cbor_writer_t*    writer           = cardano_cbor_writer_new();
  const char*               simple_list_cbor = "9f0102030405ff";

  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_metadatum_t* data = nullptr;

    EXPECT_EQ(cardano_metadatum_new_integer_from_int(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_metadatum_list_add(metadatum_list, data), CARDANO_SUCCESS);

    cardano_metadatum_unref(&data);
  }

  // Act
  error = cardano_metadatum_list_to_cbor(metadatum_list, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(simple_list_cbor) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, simple_list_cbor);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_list_to_cbor, canSerializeAnNestedMetadatumList)
{
  // Arrange
  cardano_metadatum_list_t* inner_list       = nullptr;
  cardano_cbor_writer_t*    writer           = cardano_cbor_writer_new();
  const char*               nested_list_cbor = "9f01029f0102030405ff9f0102030405ff05ff";

  cardano_error_t error = cardano_metadatum_list_new(&inner_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_metadatum_t* data = nullptr;

    EXPECT_EQ(cardano_metadatum_new_integer_from_int(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_metadatum_list_add(inner_list, data), CARDANO_SUCCESS);

    cardano_metadatum_unref(&data);
  }

  cardano_metadatum_t* inner_data = nullptr;
  EXPECT_EQ(cardano_metadatum_new_list(inner_list, &inner_data), CARDANO_SUCCESS);
  cardano_metadatum_list_unref(&inner_list);

  cardano_metadatum_list_t* outer = nullptr;

  EXPECT_EQ(cardano_metadatum_list_new(&outer), CARDANO_SUCCESS);

  cardano_metadatum_t* elem1 = nullptr;
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(1, &elem1), CARDANO_SUCCESS);
  cardano_metadatum_t* elem2 = nullptr;
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(2, &elem2), CARDANO_SUCCESS);
  cardano_metadatum_t* elem3 = nullptr;
  EXPECT_EQ(cardano_metadatum_new_integer_from_int(5, &elem3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_metadatum_list_add(outer, elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_add(outer, elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_add(outer, inner_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_add(outer, inner_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_add(outer, elem3), CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_list_to_cbor(outer, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(nested_list_cbor) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, nested_list_cbor);

  // Cleanup
  cardano_metadatum_list_unref(&outer);
  cardano_metadatum_unref(&inner_data);
  cardano_cbor_writer_unref(&writer);
  cardano_metadatum_unref(&elem1);
  cardano_metadatum_unref(&elem2);
  cardano_metadatum_unref(&elem3);

  free(actual_cbor);
}

TEST(cardano_metadatum_list_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_metadatum_list_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_metadatum_list_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;

  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_list_to_cbor(metadatum_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("9f0102ff", strlen("9f0102ff"));
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  cardano_error_t error = cardano_metadatum_list_from_cbor(reader, &metadatum_list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_to_cbor(metadatum_list, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("9f0102ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "9f0102ff");

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_metadatum_list_from_cbor, canDeserializeMetadatumList)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(METADATUM_LIST_CBOR, strlen(METADATUM_LIST_CBOR));

  // Act
  cardano_error_t error = cardano_metadatum_list_from_cbor(reader, &metadatum_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(metadatum_list, testing::Not((cardano_metadatum_list_t*)nullptr));

  const size_t length = cardano_metadatum_list_get_length(metadatum_list);

  EXPECT_EQ(length, 5);

  cardano_metadatum_t* elem1 = NULL;
  cardano_metadatum_t* elem2 = NULL;
  cardano_metadatum_t* elem3 = NULL;
  cardano_metadatum_t* elem4 = NULL;
  cardano_metadatum_t* elem5 = NULL;

  EXPECT_EQ(cardano_metadatum_list_get(metadatum_list, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(metadatum_list, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(metadatum_list, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(metadatum_list, 3, &elem4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_list_get(metadatum_list, 4, &elem5), CARDANO_SUCCESS);

  cardano_metadatum_kind_t kind = CARDANO_METADATUM_KIND_INTEGER;

  EXPECT_EQ(cardano_metadatum_get_kind(elem1, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_METADATUM_KIND_INTEGER);

  EXPECT_EQ(cardano_metadatum_get_kind(elem2, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_METADATUM_KIND_INTEGER);

  EXPECT_EQ(cardano_metadatum_get_kind(elem3, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_METADATUM_KIND_LIST);

  EXPECT_EQ(cardano_metadatum_get_kind(elem4, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_METADATUM_KIND_LIST);

  EXPECT_EQ(cardano_metadatum_get_kind(elem5, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_METADATUM_KIND_INTEGER);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_metadatum_to_integer(elem1, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 1);
  cardano_bigint_unref(&value);

  EXPECT_EQ(cardano_metadatum_to_integer(elem2, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 2);
  cardano_bigint_unref(&value);

  cardano_metadatum_list_t* metadatum_list2 = nullptr;
  EXPECT_EQ(cardano_metadatum_to_list(elem3, &metadatum_list2), CARDANO_SUCCESS);

  for (size_t i = 0; i < cardano_metadatum_list_get_length(metadatum_list2); ++i)
  {
    cardano_metadatum_t* elem = nullptr;
    EXPECT_EQ(cardano_metadatum_list_get(metadatum_list2, i, &elem), CARDANO_SUCCESS);

    EXPECT_EQ(cardano_metadatum_get_kind(elem, &kind), CARDANO_SUCCESS);
    EXPECT_EQ(kind, CARDANO_METADATUM_KIND_INTEGER);

    EXPECT_EQ(cardano_metadatum_to_integer(elem, &value), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_bigint_to_int(value), i + 1);
    cardano_bigint_unref(&value);

    cardano_metadatum_unref(&elem);
  }

  cardano_metadatum_list_t* metadatum_list3 = nullptr;
  EXPECT_EQ(cardano_metadatum_to_list(elem4, &metadatum_list3), CARDANO_SUCCESS);

  for (size_t i = 0; i < cardano_metadatum_list_get_length(metadatum_list3); ++i)
  {
    cardano_metadatum_t* elem = nullptr;
    EXPECT_EQ(cardano_metadatum_list_get(metadatum_list3, i, &elem), CARDANO_SUCCESS);

    EXPECT_EQ(cardano_metadatum_get_kind(elem, &kind), CARDANO_SUCCESS);
    EXPECT_EQ(kind, CARDANO_METADATUM_KIND_INTEGER);

    EXPECT_EQ(cardano_metadatum_to_integer(elem, &value), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_bigint_to_int(value), i + 1);
    cardano_bigint_unref(&value);

    cardano_metadatum_unref(&elem);
  }

  EXPECT_EQ(cardano_metadatum_to_integer(elem5, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), 5);
  cardano_bigint_unref(&value);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
  cardano_cbor_reader_unref(&reader);

  cardano_metadatum_unref(&elem1);
  cardano_metadatum_unref(&elem2);
  cardano_metadatum_unref(&elem3);
  cardano_metadatum_unref(&elem4);
  cardano_metadatum_unref(&elem5);

  cardano_metadatum_list_unref(&metadatum_list2);
  cardano_metadatum_list_unref(&metadatum_list3);
}

TEST(cardano_metadatum_list_from_cbor, returnErrorIfMetadatumListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(METADATUM_LIST_CBOR, strlen(METADATUM_LIST_CBOR));

  // Act
  cardano_error_t error = cardano_metadatum_list_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_list_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_list_from_cbor(nullptr, &metadatum_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_list_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(METADATUM_LIST_CBOR, strlen(METADATUM_LIST_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_metadatum_list_from_cbor(reader, &metadatum_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(metadatum_list, (cardano_metadatum_list_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_list_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_metadatum_list_t* list   = nullptr;
  cardano_cbor_reader_t*    reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_metadatum_list_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_list_from_cbor, returnErrorIfInvalidMetadatumDataElements)
{
  // Arrange
  cardano_metadatum_list_t* list   = nullptr;
  cardano_cbor_reader_t*    reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_metadatum_list_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Invalid CBOR data item type for metadatum.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_list_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_metadatum_list_t* list   = nullptr;
  cardano_cbor_reader_t*    reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_metadatum_list_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected end of buffer.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_metadatum_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_list_ref(metadatum_list);

  // Assert
  EXPECT_THAT(metadatum_list, testing::Not((cardano_metadatum_list_t*)nullptr));
  EXPECT_EQ(cardano_metadatum_list_refcount(metadatum_list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_metadatum_list_unref(&metadatum_list);
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_list_ref(nullptr);
}

TEST(cardano_metadatum_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;

  // Act
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_metadatum_list_unref((cardano_metadatum_list_t**)nullptr);
}

TEST(cardano_metadatum_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_list_ref(metadatum_list);
  size_t ref_count = cardano_metadatum_list_refcount(metadatum_list);

  cardano_metadatum_list_unref(&metadatum_list);
  size_t updated_ref_count = cardano_metadatum_list_refcount(metadatum_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_list_ref(metadatum_list);
  size_t ref_count = cardano_metadatum_list_refcount(metadatum_list);

  cardano_metadatum_list_unref(&metadatum_list);
  size_t updated_ref_count = cardano_metadatum_list_refcount(metadatum_list);

  cardano_metadatum_list_unref(&metadatum_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(metadatum_list, (cardano_metadatum_list_t*)nullptr);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_metadatum_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_metadatum_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  const char*               message        = "This is a test message";

  // Act
  cardano_metadatum_list_set_last_error(metadatum_list, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_list_get_last_error(metadatum_list), "Object is NULL.");
}

TEST(cardano_metadatum_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_metadatum_list_set_last_error(metadatum_list, message);

  // Assert
  EXPECT_STREQ(cardano_metadatum_list_get_last_error(metadatum_list), "");

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_get_length, returnsZeroIfMetadatumListIsNull)
{
  // Act
  size_t length = cardano_metadatum_list_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_metadatum_list_get_length, returnsZeroIfMetadatumListIsEmpty)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_metadatum_list_get_length(metadatum_list);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_get, returnsErrorIfMetadatumListIsNull)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_list_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_list_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_list_get(metadatum_list, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_t* data = nullptr;
  error                     = cardano_metadatum_list_get(metadatum_list, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_add, returnsErrorIfMetadatumListIsNull)
{
  // Arrange
  cardano_metadatum_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_metadatum_list_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_metadatum_list_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_metadatum_list_add(metadatum_list, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_equals, returnsFalseIfEitherMetadatumListIsNull)
{
  // Act
  bool equals = cardano_metadatum_list_equals((cardano_metadatum_list_t*)"", nullptr);

  // Assert
  EXPECT_FALSE(equals);
}

TEST(cardano_metadatum_list_equals, returnsTrueIfBothMetadatumListsAreNull)
{
  // Act
  bool equals = cardano_metadatum_list_equals(nullptr, nullptr);

  // Assert
  EXPECT_TRUE(equals);
}

TEST(cardano_metadatum_list_equals, returnsFalseIfOneMetadatumListIsNull)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list = nullptr;
  cardano_error_t           error          = cardano_metadatum_list_new(&metadatum_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_metadatum_list_equals(metadatum_list, nullptr);

  // Assert
  EXPECT_FALSE(equals);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list);
}

TEST(cardano_metadatum_list_equals, returnsFalseIfMetadatumListsHaveDifferentLengths)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list1 = nullptr;
  cardano_metadatum_list_t* metadatum_list2 = nullptr;

  cardano_metadatum_t* data1 = nullptr;

  cardano_error_t error = cardano_metadatum_new_integer_from_int(1, &data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_new(&metadatum_list1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_new(&metadatum_list2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_add(metadatum_list1, data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_metadatum_list_equals(metadatum_list1, metadatum_list2);

  // Assert
  EXPECT_FALSE(equals);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list1);
  cardano_metadatum_list_unref(&metadatum_list2);
  cardano_metadatum_unref(&data1);
}

TEST(cardano_metadatum_list_equals, returnsFalseIfMetadatumListsHaveDifferentElements)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list1 = nullptr;
  cardano_metadatum_list_t* metadatum_list2 = nullptr;

  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_new(&metadatum_list2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* data1 = nullptr;
  cardano_metadatum_t* data2 = nullptr;

  error = cardano_metadatum_new_integer_from_int(1, &data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_integer_from_int(2, &data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_add(metadatum_list1, data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_add(metadatum_list2, data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_metadatum_list_equals(metadatum_list1, metadatum_list2);

  // Assert
  EXPECT_FALSE(equals);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list1);
  cardano_metadatum_list_unref(&metadatum_list2);
  cardano_metadatum_unref(&data1);
  cardano_metadatum_unref(&data2);
}

TEST(cardano_metadatum_list_equals, returnsTrueIfMetadatumListsAreEqual)
{
  // Arrange
  cardano_metadatum_list_t* metadatum_list1 = nullptr;
  cardano_metadatum_list_t* metadatum_list2 = nullptr;

  cardano_error_t error = cardano_metadatum_list_new(&metadatum_list1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_new(&metadatum_list2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* data1 = nullptr;
  cardano_metadatum_t* data2 = nullptr;

  error = cardano_metadatum_new_integer_from_int(1, &data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_new_integer_from_int(1, &data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_add(metadatum_list1, data1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_list_add(metadatum_list2, data2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool equals = cardano_metadatum_list_equals(metadatum_list1, metadatum_list2);

  // Assert
  EXPECT_TRUE(equals);

  // Cleanup
  cardano_metadatum_list_unref(&metadatum_list1);
  cardano_metadatum_list_unref(&metadatum_list2);
  cardano_metadatum_unref(&data1);
  cardano_metadatum_unref(&data2);
}

TEST(cardano_metadatum_list_to_cip116_json, returnErrorIfNullPointer)
{
  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  EXPECT_EQ(cardano_metadatum_list_to_cip116_json(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_metadatum_list_to_cip116_json((cardano_metadatum_list_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_json_writer_unref(&writer);
}
