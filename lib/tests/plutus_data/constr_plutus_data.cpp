/**
 * \file constr_plutus_data.cpp
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
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_data_kind.h>
#include <cardano/plutus_data/plutus_list.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CONSTR_PLUTUS_DATA_CBOR = "d8799f0102030405ff";

/* UNIT TESTS ****************************************************************/

TEST(cardano_constr_plutus_data_new, canCreateConstrPlutusData)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(constr_plutus_data, testing::Not((cardano_constr_plutus_data_t*)nullptr));

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_new, returnsErrorIfConstrPlutusDataIsNull)
{
  // Act
  cardano_error_t error = cardano_constr_plutus_data_new(0, (cardano_plutus_list_t*)"", nullptr);
  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_constr_plutus_data_new, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_constr_plutus_data_new(0, nullptr, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(constr_plutus_data, (cardano_constr_plutus_data_t*)nullptr);
}

TEST(cardano_constr_plutus_data_new, returnsErrorIfMemoryAllocationFails)
{
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(constr_plutus_data, (cardano_constr_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_to_cbor, canSerializeAnEmptyConstrPlutusData)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;
  cardano_cbor_writer_t*        writer             = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_to_cbor(constr_plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 7);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d87980");

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_constr_plutus_data_to_cbor, canSerializeAnSimpleConstrPlutusData)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_writer_t*        writer             = cardano_cbor_writer_new();
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_to_cbor(constr_plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CONSTR_PLUTUS_DATA_CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CONSTR_PLUTUS_DATA_CBOR);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_constr_plutus_data_to_cbor, canSerializeAnConstrPlutusData)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_writer_t*        writer             = cardano_cbor_writer_new();
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  error = cardano_constr_plutus_data_new(150, list, &constr_plutus_data);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_to_cbor(constr_plutus_data, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("d8668218969f0102030405ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d8668218969f0102030405ff");

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_constr_plutus_data_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_constr_plutus_data_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_constr_plutus_data_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_to_cbor(constr_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_to_cbor, canDeserializeAndReserializeCborTag0)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d8009f0102030405ff", strlen("d8009f0102030405ff"));
  cardano_cbor_writer_t*        writer             = cardano_cbor_writer_new();

  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_to_cbor(constr_plutus_data, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_constr_plutus_data_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d9055f9f0102030405ff", strlen("d9055f9f0102030405ff"));
  cardano_cbor_writer_t*        writer             = cardano_cbor_writer_new();

  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_to_cbor(constr_plutus_data, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("d9055f9f0102030405ff") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9055f9f0102030405ff");

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_constr_plutus_data_from_cbor, canDeserializeConstrPlutusData)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(CONSTR_PLUTUS_DATA_CBOR, strlen(CONSTR_PLUTUS_DATA_CBOR));

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(constr_plutus_data, testing::Not((cardano_constr_plutus_data_t*)nullptr));

  uint64_t alternative = 9;

  error = cardano_constr_plutus_data_get_alternative(constr_plutus_data, &alternative);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(alternative, 0);

  cardano_plutus_list_t* list = nullptr;

  error = cardano_constr_plutus_data_get_data(constr_plutus_data, &list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < cardano_plutus_list_get_length(list); ++i)
  {
    cardano_plutus_data_t* elem = nullptr;
    EXPECT_EQ(cardano_plutus_list_get(list, i, &elem), CARDANO_SUCCESS);

    cardano_plutus_data_kind_t kind;
    int64_t                    value;

    EXPECT_EQ(cardano_plutus_data_get_kind(elem, &kind), CARDANO_SUCCESS);
    EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

    EXPECT_EQ(cardano_plutus_data_to_integer(elem, &value), CARDANO_SUCCESS);
    EXPECT_EQ(value, i + 1);

    cardano_plutus_data_unref(&elem);
  }

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_from_cbor, canDeserializeConstrPlutusDataGeneralFormTag)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d86682009f0102030405ff", strlen("d86682009f0102030405ff"));

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(constr_plutus_data, testing::Not((cardano_constr_plutus_data_t*)nullptr));

  uint64_t alternative = 9;

  error = cardano_constr_plutus_data_get_alternative(constr_plutus_data, &alternative);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(alternative, 0);

  cardano_plutus_list_t* list = nullptr;

  error = cardano_constr_plutus_data_get_data(constr_plutus_data, &list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < cardano_plutus_list_get_length(list); ++i)
  {
    cardano_plutus_data_t* elem = nullptr;
    EXPECT_EQ(cardano_plutus_list_get(list, i, &elem), CARDANO_SUCCESS);

    cardano_plutus_data_kind_t kind;
    int64_t                    value;

    EXPECT_EQ(cardano_plutus_data_get_kind(elem, &kind), CARDANO_SUCCESS);
    EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

    EXPECT_EQ(cardano_plutus_data_to_integer(elem, &value), CARDANO_SUCCESS);
    EXPECT_EQ(value, i + 1);

    cardano_plutus_data_unref(&elem);
  }

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_from_cbor, canDeserializeConstrPlutusDataGeneralFormTagIndefArray)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d8669f009f0102030405ffff", strlen("d8669f009f0102030405ffff"));

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(constr_plutus_data, testing::Not((cardano_constr_plutus_data_t*)nullptr));

  uint64_t alternative = 9;

  error = cardano_constr_plutus_data_get_alternative(constr_plutus_data, &alternative);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(alternative, 0);

  cardano_plutus_list_t* list = nullptr;

  error = cardano_constr_plutus_data_get_data(constr_plutus_data, &list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < cardano_plutus_list_get_length(list); ++i)
  {
    cardano_plutus_data_t* elem = nullptr;
    EXPECT_EQ(cardano_plutus_list_get(list, i, &elem), CARDANO_SUCCESS);

    cardano_plutus_data_kind_t kind;
    int64_t                    value;

    EXPECT_EQ(cardano_plutus_data_get_kind(elem, &kind), CARDANO_SUCCESS);
    EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

    EXPECT_EQ(cardano_plutus_data_to_integer(elem, &value), CARDANO_SUCCESS);
    EXPECT_EQ(value, i + 1);

    cardano_plutus_data_unref(&elem);
  }

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_cbor_reader_unref(&reader);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfConstrPlutusDataGeneralFormTagDoesntHaveArray)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d866ff009f0102030405ff", strlen("d866ff009f0102030405ff"));

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constr_plutus_data, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfConstrPlutusDataGeneralFormTagDoesntAlternative)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d8669ff59f0102030405ffff", strlen("d8669ff59f0102030405ffff"));

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(constr_plutus_data, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfConstrPlutusDataGeneralFormTagDoesntHaveDataList)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("d8669f0000ff", strlen("d8669f0000ff"));

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(constr_plutus_data, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfConstrPlutusDataIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CONSTR_PLUTUS_DATA_CBOR, strlen(CONSTR_PLUTUS_DATA_CBOR));

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(nullptr, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(CONSTR_PLUTUS_DATA_CBOR, strlen(CONSTR_PLUTUS_DATA_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(constr_plutus_data, (cardano_constr_plutus_data_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfNotATag)
{
  // Arrange
  cardano_constr_plutus_data_t* constr = nullptr;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfInvalidPlutusDataElements)
{
  // Arrange
  cardano_constr_plutus_data_t* constr = nullptr;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex("d87901", 6);

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_constr_plutus_data_t* constr = nullptr;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex("d8799f0102030405", 16);

  // Act
  cardano_error_t error = cardano_constr_plutus_data_from_cbor(reader, &constr);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected end of buffer.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_constr_plutus_data_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  cardano_plutus_list_unref(&list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_constr_plutus_data_ref(constr_plutus_data);

  // Assert
  EXPECT_THAT(constr_plutus_data, testing::Not((cardano_constr_plutus_data_t*)nullptr));
  EXPECT_EQ(cardano_constr_plutus_data_refcount(constr_plutus_data), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_constr_plutus_data_unref(&constr_plutus_data);
}

TEST(cardano_constr_plutus_data_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_constr_plutus_data_ref(nullptr);
}

TEST(cardano_constr_plutus_data_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;

  // Act
  cardano_constr_plutus_data_unref(&constr_plutus_data);
}

TEST(cardano_constr_plutus_data_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_constr_plutus_data_unref((cardano_constr_plutus_data_t**)nullptr);
}

TEST(cardano_constr_plutus_data_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_list_unref(&list);

  // Act
  cardano_constr_plutus_data_ref(constr_plutus_data);
  size_t ref_count = cardano_constr_plutus_data_refcount(constr_plutus_data);

  cardano_constr_plutus_data_unref(&constr_plutus_data);
  size_t updated_ref_count = cardano_constr_plutus_data_refcount(constr_plutus_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
}

TEST(cardano_constr_plutus_data_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  cardano_plutus_list_unref(&list);

  // Act
  cardano_constr_plutus_data_ref(constr_plutus_data);
  size_t ref_count = cardano_constr_plutus_data_refcount(constr_plutus_data);

  cardano_constr_plutus_data_unref(&constr_plutus_data);
  size_t updated_ref_count = cardano_constr_plutus_data_refcount(constr_plutus_data);

  cardano_constr_plutus_data_unref(&constr_plutus_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(constr_plutus_data, (cardano_constr_plutus_data_t*)nullptr);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
}

TEST(cardano_constr_plutus_data_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_constr_plutus_data_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_constr_plutus_data_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_constr_plutus_data_set_last_error(constr_plutus_data, message);

  // Assert
  EXPECT_STREQ(cardano_constr_plutus_data_get_last_error(constr_plutus_data), "Object is NULL.");
}

TEST(cardano_constr_plutus_data_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  cardano_plutus_list_unref(&list);

  const char* message = nullptr;

  // Act
  cardano_constr_plutus_data_set_last_error(constr_plutus_data, message);

  // Assert
  EXPECT_STREQ(cardano_constr_plutus_data_get_last_error(constr_plutus_data), "");

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
}

TEST(cardano_constr_plutus_data_get_data, returnsErrorIfConstrPlutusDataIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  // Act
  cardano_error_t error = cardano_constr_plutus_data_get_data(constr_plutus_data, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(list, (cardano_plutus_list_t*)nullptr);
}

TEST(cardano_constr_plutus_data_get_data, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_get_data(constr_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_set_data, returnsErrorIfConstrPlutusDataIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  // Act
  cardano_error_t error = cardano_constr_plutus_data_set_data(constr_plutus_data, list);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_constr_plutus_data_set_data, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_set_data(constr_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_set_data, canSetList)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;
  cardano_plutus_list_t*        new_list           = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&new_list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_set_data(constr_plutus_data, new_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_list_t* actual_list = nullptr;

  error = cardano_constr_plutus_data_get_data(constr_plutus_data, &actual_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(actual_list, new_list);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
  cardano_plutus_list_unref(&new_list);
  cardano_plutus_list_unref(&actual_list);
}

TEST(cardano_constr_plutus_data_get_alternative, returnsErrorIfConstrPlutusDataIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  uint64_t                      alternative        = 0;

  // Act
  cardano_error_t error = cardano_constr_plutus_data_get_alternative(constr_plutus_data, &alternative);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_constr_plutus_data_get_alternative, returnsErrorIfAlternativeIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_constr_plutus_data_get_alternative(constr_plutus_data, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_data_set_alternative, returnsErrorIfConstrPlutusDataIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  uint64_t                      alternative        = 0;

  // Act
  cardano_error_t error = cardano_constr_plutus_data_set_alternative(constr_plutus_data, alternative);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_constr_plutus_data_set_alternative, canSetAlternative)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t alternative = 150;

  // Act
  error = cardano_constr_plutus_data_set_alternative(constr_plutus_data, alternative);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t actual_alternative = 0;

  error = cardano_constr_plutus_data_get_alternative(constr_plutus_data, &actual_alternative);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(actual_alternative, alternative);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_equals, returnsTrueIfConstrPlutusDataAreEqual)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data_1 = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data_2 = nullptr;
  cardano_plutus_list_t*        list_1               = nullptr;
  cardano_plutus_list_t*        list_2               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list_1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&list_2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list_1, data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list_2, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  error = cardano_constr_plutus_data_new(0, list_1, &constr_plutus_data_1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list_2, &constr_plutus_data_2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_constr_plutus_equals(constr_plutus_data_1, constr_plutus_data_2);

  // Assert
  EXPECT_TRUE(are_equal);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data_1);
  cardano_constr_plutus_data_unref(&constr_plutus_data_2);
  cardano_plutus_list_unref(&list_1);
  cardano_plutus_list_unref(&list_2);
}

TEST(cardano_constr_plutus_equals, returnsFalseIfConstrPlutusDataAreNotEqual)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data_1 = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data_2 = nullptr;
  cardano_plutus_list_t*        list_1               = nullptr;
  cardano_plutus_list_t*        list_2               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list_1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&list_2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list_1, data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list_2, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  error = cardano_constr_plutus_data_new(0, list_1, &constr_plutus_data_1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(1, list_2, &constr_plutus_data_2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_constr_plutus_equals(constr_plutus_data_1, constr_plutus_data_2);

  // Assert
  EXPECT_FALSE(are_equal);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data_1);
  cardano_constr_plutus_data_unref(&constr_plutus_data_2);
  cardano_plutus_list_unref(&list_1);
  cardano_plutus_list_unref(&list_2);
}

TEST(cardano_constr_plutus_equals, returnsFalseIfConstrPlutusDataAreDifferentLength)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data_1 = nullptr;
  cardano_constr_plutus_data_t* constr_plutus_data_2 = nullptr;
  cardano_plutus_list_t*        list_1               = nullptr;
  cardano_plutus_list_t*        list_2               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list_1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_list_new(&list_2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list_1, data), CARDANO_SUCCESS);

    if (i < 4)
    {
      EXPECT_EQ(cardano_plutus_list_add(list_2, data), CARDANO_SUCCESS);
    }

    cardano_plutus_data_unref(&data);
  }

  error = cardano_constr_plutus_data_new(0, list_1, &constr_plutus_data_1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_constr_plutus_data_new(0, list_2, &constr_plutus_data_2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_constr_plutus_equals(constr_plutus_data_1, constr_plutus_data_2);

  // Assert
  EXPECT_FALSE(are_equal);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data_1);
  cardano_constr_plutus_data_unref(&constr_plutus_data_2);
  cardano_plutus_list_unref(&list_1);
  cardano_plutus_list_unref(&list_2);
}

TEST(cardano_constr_plutus_equals, returnsTrueIfSamePointer)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_constr_plutus_equals(constr_plutus_data, constr_plutus_data);

  // Assert
  EXPECT_TRUE(are_equal);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}

TEST(cardano_constr_plutus_equals, returnsFalseIfOnePointerIsNull)
{
  // Arrange
  cardano_constr_plutus_data_t* constr_plutus_data = nullptr;
  cardano_plutus_list_t*        list               = nullptr;

  cardano_error_t error = cardano_plutus_list_new(&list);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_plutus_data_t* data = nullptr;

    EXPECT_EQ(cardano_plutus_data_new_integer(i + 1, &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_list_add(list, data), CARDANO_SUCCESS);

    cardano_plutus_data_unref(&data);
  }

  error = cardano_constr_plutus_data_new(0, list, &constr_plutus_data);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_constr_plutus_equals(constr_plutus_data, nullptr);

  // Assert
  EXPECT_FALSE(are_equal);

  // Cleanup
  cardano_constr_plutus_data_unref(&constr_plutus_data);
  cardano_plutus_list_unref(&list);
}