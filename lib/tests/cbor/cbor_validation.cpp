/**
 * \file cbor_validation.cpp
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
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

#include <cardano/cbor/cbor_reader.h>

extern "C" {
#include "../../src/cbor/cbor_validation.h"
}

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_validate_array_of_n_elements, returnValidIfValidCborArray)
{
  // Arrange
  const byte_t           cbor_array[] = { 0x83, 0x01, 0x02, 0x03 };
  const size_t           n_elements   = 3;
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_new(cbor_array, sizeof(cbor_array));

  // Act
  const cardano_error_t result = cardano_cbor_validate_array_of_n_elements("field_name", reader, n_elements);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_array_of_n_elements, returnErrorIfInvalidCborArray)
{
  // Arrange
  const byte_t           cbor_array[] = { 0x83, 0x01, 0x02, 0x03 };
  const size_t           n_elements   = 4;
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_new(cbor_array, sizeof(cbor_array));

  // Act
  const cardano_error_t result = cardano_cbor_validate_array_of_n_elements("field_name", reader, n_elements);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_array_of_n_elements, returnErrorIfNotAnArray)
{
  // Arrange
  const byte_t           cbor_array[] = { 0x01, 0x02, 0x03 };
  const size_t           n_elements   = 3;
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_new(cbor_array, sizeof(cbor_array));

  // Act
  const cardano_error_t result = cardano_cbor_validate_array_of_n_elements("field_name", reader, n_elements);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_array_of_n_elements, returnErrorIfReaderIsNull)
{
  // Arrange
  const size_t n_elements = 3;

  // Act
  const cardano_error_t result = cardano_cbor_validate_array_of_n_elements("field_name", NULL, n_elements);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_validate_uint_in_range, returnValidIfValidUintInRange)
{
  // Arrange
  const byte_t           cbor_uint[] = { 0x01 };
  const uint32_t         min         = 0;
  const uint32_t         max         = 4;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_new(cbor_uint, sizeof(cbor_uint));
  uint64_t               type;

  // Act
  const cardano_error_t result = cardano_cbor_validate_uint_in_range("field_name", "type_name", reader, &type, min, max);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(type, 1);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_uint_in_range, returnErrorIfNotAnUint)
{
  // Arrange
  const byte_t           cbor_uint[] = { 0x83 };
  const uint32_t         min         = 0;
  const uint32_t         max         = 4;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_new(cbor_uint, sizeof(cbor_uint));
  uint64_t               type;

  // Act
  const cardano_error_t result = cardano_cbor_validate_uint_in_range("field_name", "type_name", reader, &type, min, max);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_uint_in_range, returnErrorIfUintOutOfRange)
{
  // Arrange
  const byte_t           cbor_uint[] = { 0x05 };
  const uint32_t         min         = 0;
  const uint32_t         max         = 4;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_new(cbor_uint, sizeof(cbor_uint));
  uint64_t               type;

  // Act
  const cardano_error_t result = cardano_cbor_validate_uint_in_range("field_name", "type_name", reader, &type, min, max);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_uint_in_range, returnErrorIfReaderIsNull)
{
  // Arrange
  const uint32_t min = 0;
  const uint32_t max = 4;
  uint64_t       type;

  // Act
  const cardano_error_t result = cardano_cbor_validate_uint_in_range("field_name", "type_name", NULL, &type, min, max);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_validate_byte_string_of_size, returnValidIfValidByteString)
{
  // Arrange
  const byte_t           cbor_byte_string[] = { 0x43, 0x01, 0x02, 0x03 };
  const uint32_t         size               = 3;
  cardano_cbor_reader_t* reader             = cardano_cbor_reader_new(cbor_byte_string, sizeof(cbor_byte_string));
  cardano_buffer_t*      byte_string;

  // Act
  const cardano_error_t result = cardano_cbor_validate_byte_string_of_size("field_name", reader, &byte_string, size);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(cardano_buffer_get_size(byte_string), size);

  // Compare the byte string contents
  byte_t* data = cardano_buffer_get_data(byte_string);

  for (uint32_t i = 0; i < size; i++)
  {
    ASSERT_EQ(data[i], cbor_byte_string[i + 1]);
  }

  // Cleanup
  cardano_buffer_unref(&byte_string);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_byte_string_of_size, returnErrorIfNotAByteString)
{
  // Arrange
  const byte_t           cbor_byte_string[] = { 0x03, 0x01, 0x02, 0x03 };
  const uint32_t         size               = 3;
  cardano_cbor_reader_t* reader             = cardano_cbor_reader_new(cbor_byte_string, sizeof(cbor_byte_string));
  cardano_buffer_t*      byte_string;

  // Act
  const cardano_error_t result = cardano_cbor_validate_byte_string_of_size("field_name", reader, &byte_string, size);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_byte_string_of_size, returnErrorIfByteStringSizeMismatch)
{
  // Arrange
  const byte_t           cbor_byte_string[] = { 0x43, 0x01, 0x02, 0x03 };
  const uint32_t         size               = 4;
  cardano_cbor_reader_t* reader             = cardano_cbor_reader_new(cbor_byte_string, sizeof(cbor_byte_string));
  cardano_buffer_t*      byte_string;

  // Act
  const cardano_error_t result = cardano_cbor_validate_byte_string_of_size("field_name", reader, &byte_string, size);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_byte_string_of_size, returnErrorIfByteStringMalformed)
{
  // Arrange
  const byte_t           cbor_byte_string[] = { 0x43, 0x01, 0x02 };
  const uint32_t         size               = 3;
  cardano_cbor_reader_t* reader             = cardano_cbor_reader_new(cbor_byte_string, sizeof(cbor_byte_string));
  cardano_buffer_t*      byte_string;

  // Act
  const cardano_error_t result = cardano_cbor_validate_byte_string_of_size("field_name", reader, &byte_string, size);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_byte_string_of_size, returnErrorIfReaderIsNull)
{
  // Arrange
  const uint32_t    size = 3;
  cardano_buffer_t* byte_string;

  // Act
  const cardano_error_t result = cardano_cbor_validate_byte_string_of_size("field_name", NULL, &byte_string, size);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_validate_end_array, returnValidIfEndOfArray)
{
  // Arrange
  const byte_t           cbor_array[] = { 0x80 };
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_new(cbor_array, sizeof(cbor_array));

  // Act
  int64_t               n_elements        = 0;
  const cardano_error_t read_array_result = cardano_cbor_reader_read_start_array(reader, &n_elements);

  // Assert
  ASSERT_EQ(n_elements, 0); // Ensure the array is empty
  ASSERT_EQ(read_array_result, CARDANO_SUCCESS);

  const cardano_error_t result = cardano_cbor_validate_end_array("field_name", reader);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_end_array, returnErrorIfNotEndOfArray)
{
  // Arrange
  const byte_t           cbor_array[] = { 0x83, 0x01, 0x02, 0x03, 0x04 };
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_new(cbor_array, sizeof(cbor_array));

  // Act
  const cardano_error_t result = cardano_cbor_validate_end_array("field_name", reader);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_end_array, returnErrorIfNotAnArray)
{
  // Arrange
  const byte_t           cbor_array[] = { 0x01, 0x02, 0x03 };
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_new(cbor_array, sizeof(cbor_array));

  // Act
  const cardano_error_t result = cardano_cbor_validate_end_array("field_name", reader);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_validate_end_array, returnErrorIfReaderIsNull)
{
  // Act
  const cardano_error_t result = cardano_cbor_validate_end_array("field_name", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}