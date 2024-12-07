/**
 * \file json_writer.cpp
 *
 * \author angel.castillo
 * \date   Dec 07, 2024
 *
 * \section LICENSE
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

#pragma warning(disable : 4566)

#include <cardano/json/json_writer.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* STATIC FUNCTIONS **********************************************************/

static void
test_create_empty_objects(const cardano_json_format_t format, const char* expected_string)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  EXPECT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, expected_string);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

static void
test_create_empty_arrays(const cardano_json_format_t format, const char* expected_string)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  // Act
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "a", 1);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, expected_string);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

static void
test_create_empty_arrays_of_empty_objects(const cardano_json_format_t format, const char* expected_string)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  // Act
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "a", 1);
  cardano_json_writer_write_start_array(writer);

  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_end_object(writer);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_end_object(writer);

  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, expected_string);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

static void
test_create_empty_arrays_of_nested_empty_objects(const cardano_json_format_t format, const char* expected_string)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  // Act
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "a", 1);
  cardano_json_writer_write_start_array(writer);

  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "b", 1);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_end_object(writer);

  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "c", 1);
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "d", 1);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_end_object(writer);

  cardano_json_writer_write_end_object(writer);

  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, expected_string);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

static void
test_primitives(const cardano_json_format_t format, const char* expected_string)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  // Act
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "a", 1);
  cardano_json_writer_write_uint(writer, UINT_MAX);

  cardano_json_writer_write_property_name(writer, "b", 1);
  cardano_json_writer_write_signed_int(writer, INT_MAX);

  cardano_json_writer_write_property_name(writer, "c", 1);
  cardano_json_writer_write_signed_int(writer, -INT_MAX);

  cardano_json_writer_write_property_name(writer, "d", 1);
  cardano_json_writer_write_double(writer, MAXFLOAT);

  cardano_json_writer_write_property_name(writer, "e", 1);
  cardano_json_writer_write_double(writer, -MAXFLOAT);

  cardano_json_writer_write_property_name(writer, "f", 1);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);

  cardano_json_writer_write_property_name(writer, "g", 1);
  cardano_json_writer_write_bool(writer, true);

  cardano_json_writer_write_property_name(writer, "h", 1);
  cardano_json_writer_write_bool(writer, false);

  cardano_json_writer_write_property_name(writer, "i", 1);
  cardano_json_writer_write_null(writer);

  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, expected_string);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

static void
test_array_of_primitives(const cardano_json_format_t format, const char* expected_string)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  // Act
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "a", 1);
  cardano_json_writer_write_start_array(writer);

  cardano_json_writer_write_uint(writer, UINT_MAX);
  cardano_json_writer_write_signed_int(writer, INT_MAX);
  cardano_json_writer_write_signed_int(writer, -INT_MAX);
  cardano_json_writer_write_double(writer, MAXFLOAT);
  cardano_json_writer_write_double(writer, -MAXFLOAT);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);
  cardano_json_writer_write_bool(writer, true);
  cardano_json_writer_write_bool(writer, false);
  cardano_json_writer_write_null(writer);

  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, expected_string);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_json_writer, canCreateEmptyObjects)
{
  test_create_empty_objects(CARDANO_JSON_FORMAT_PRETTY, "{}");
  test_create_empty_objects(CARDANO_JSON_FORMAT_COMPACT, "{}");
}

TEST(cardano_json_writer, canCreateEmptyArrays)
{
  test_create_empty_arrays(CARDANO_JSON_FORMAT_PRETTY, "{\n  \"a\": []\n}");
  test_create_empty_arrays(CARDANO_JSON_FORMAT_COMPACT, "{\"a\":[]}");
}

TEST(cardano_json_writer, canCreateEmptyArraysOfEmptyObjects)
{
  test_create_empty_arrays_of_empty_objects(CARDANO_JSON_FORMAT_PRETTY, "{\n  \"a\": [\n    {},\n    {}\n  ]\n}");
  test_create_empty_arrays_of_empty_objects(CARDANO_JSON_FORMAT_COMPACT, "{\"a\":[{},{}]}");
}

TEST(cardano_json_writer, canCreateEmptyArraysOfEmptyNestedObjects)
{
  test_create_empty_arrays_of_nested_empty_objects(
    CARDANO_JSON_FORMAT_PRETTY,
    R"({
  "a": [
    {
      "b": []
    },
    {
      "c": {
        "d": []
      }
    }
  ]
})");

  test_create_empty_arrays_of_nested_empty_objects(CARDANO_JSON_FORMAT_COMPACT, R"({"a":[{"b":[]},{"c":{"d":[]}}]})");
}

TEST(cardano_json_writer, canCreatePrimitives)
{
  test_primitives(
    CARDANO_JSON_FORMAT_PRETTY,
    R"({
  "a": 4294967295,
  "b": 2147483647,
  "c": -2147483647,
  "d": 3.4028234663852886e+38,
  "e": -3.4028234663852886e+38,
  "f": "Hello, World!",
  "g": true,
  "h": false,
  "i": null
})");

  test_primitives(
    CARDANO_JSON_FORMAT_COMPACT,
    R"({"a":4294967295,"b":2147483647,"c":-2147483647,"d":3.4028234663852886e+38,"e":-3.4028234663852886e+38,"f":"Hello, World!","g":true,"h":false,"i":null})");
}

TEST(cardano_json_writer, canCreateArrayOfPrimitives)
{
  test_array_of_primitives(
    CARDANO_JSON_FORMAT_PRETTY,
    R"({
  "a": [
    4294967295,
    2147483647,
    -2147483647,
    3.4028234663852886e+38,
    -3.4028234663852886e+38,
    "Hello, World!",
    true,
    false,
    null
  ]
})");

  test_array_of_primitives(
    CARDANO_JSON_FORMAT_COMPACT,
    R"({"a":[4294967295,2147483647,-2147483647,3.4028234663852886e+38,-3.4028234663852886e+38,"Hello, World!",true,false,null]})");
}

TEST(cardano_json_writer_write_property_name, writesAPropertyName)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "name", 4);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"name\": \"Hello, World!\"\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer, returnsErrorIfMaxNestedLevelReached)
{
  // Arrange
  char                   buffer[128] = { 0 };
  cardano_json_writer_t* writer      = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "name", 4);

  for (size_t i = 0; i < 1024; i++)
  {
    cardano_json_writer_write_start_array(writer);
  }

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_property_name, returnsErrorIfCalledInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_property_name(writer, "name", 4);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_property_name, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, nullptr, 0);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Don't crash
  cardano_json_writer_write_property_name(nullptr, nullptr, 0);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_property_name, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_property_name(writer, "name", 4);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_bool, writesABooleanValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "bool", 4);
  cardano_json_writer_write_bool(writer, true);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"bool\": true\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_bool, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "bool", 4);
  cardano_json_writer_write_bool(nullptr, true);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_bool(nullptr, true);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_bool, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_bool(writer, true);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  EXPECT_EQ(cardano_json_writer_reset(writer), CARDANO_SUCCESS);

  cardano_json_writer_write_bool(writer, true);
  result = cardano_json_writer_encode(writer, buffer, 128);

  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_null, writesANullValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "null", 4);
  cardano_json_writer_write_null(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"null\": null\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_null, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "null", 4);
  cardano_json_writer_write_null(nullptr);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_null(nullptr);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_null, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_null(writer);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  EXPECT_EQ(cardano_json_writer_reset(writer), CARDANO_SUCCESS);

  cardano_json_writer_write_bool(writer, true);
  result = cardano_json_writer_encode(writer, buffer, 128);

  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_bigint, writesABigintValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_bigint_t*      bigint = NULL;

  EXPECT_EQ(cardano_bigint_from_string("123456789123456789", strlen("123456789123456789"), 10, &bigint), CARDANO_SUCCESS);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "bigNumber", 9);
  cardano_json_writer_write_bigint(writer, bigint);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"bigNumber\": \"123456789123456789\"\n}");

  // Cleanup
  cardano_bigint_unref(&bigint);
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_bigint, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "bigNumber", 9);
  cardano_json_writer_write_bigint(writer, nullptr);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Don't crash
  cardano_json_writer_write_bigint(nullptr, nullptr);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_bigint, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char              buffer[128] = { 0 };
  cardano_bigint_t* bigint      = NULL;

  EXPECT_EQ(cardano_bigint_from_string("123456789123456789", strlen("123456789123456789"), 10, &bigint), CARDANO_SUCCESS);

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_bigint(writer, bigint);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_bigint_unref(&bigint);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_bigint, canWriteBigintArray)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_bigint_t*      bigint = NULL;

  EXPECT_EQ(cardano_bigint_from_string("123456789123456789", strlen("123456789123456789"), 10, &bigint), CARDANO_SUCCESS);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "bigNumbers", 10);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_bigint(writer, bigint);
  cardano_json_writer_write_bigint(writer, bigint);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"bigNumbers\": [\n    \"123456789123456789\",\n    \"123456789123456789\"\n  ]\n}");

  // Cleanup
  cardano_bigint_unref(&bigint);
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_bigint, retursErrorIfMemoryAllocationFails)
{
  char buffer[128] = { 0 };
  // Arrange

  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  cardano_bigint_t* bigint = NULL;
  EXPECT_EQ(cardano_bigint_from_string("123456789123456789", strlen("123456789123456789"), 10, &bigint), CARDANO_SUCCESS);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "bigNumber", 9);
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  cardano_json_writer_write_bigint(writer, bigint);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);
  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_bigint_unref(&bigint);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_start_array, writesAnArrayStart)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "array", 5);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"array\": []\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_start_array, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "array", 5);
  cardano_json_writer_write_start_array(nullptr);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_start_array(nullptr);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_start_array, canWriteArrayOfArrays)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "array", 5);
  cardano_json_writer_write_start_array(writer);

  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(writer);

  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"array\": [\n    [],\n    []\n  ]\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_start_array, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_array(writer);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_end_array, writesAnArrayEnd)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "array", 5);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"array\": []\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_end_array, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "array", 5);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_end_array(nullptr);
  cardano_json_writer_write_end_object(writer);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_end_array(nullptr);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_end_array, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_end_array(writer);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_start_object, writesAnObjectStart)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_start_object, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(nullptr);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Don't crash
  cardano_json_writer_write_start_object(nullptr);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_start_object, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_start_object(writer);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_end_object, writesAnObjectEnd)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_end_object, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_end_object(nullptr);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_end_object(nullptr);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_end_object, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_end_object(writer);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_raw_value, writesARawValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  const char* raw_value = R"({"raw": "value"})";
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "raw", 3);
  cardano_json_writer_write_raw_value(writer, raw_value, strlen(raw_value));
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"raw\": {\"raw\": \"value\"}\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_raw_value, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "raw", 3);
  cardano_json_writer_write_raw_value(writer, nullptr, 0);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  EXPECT_EQ(cardano_json_writer_reset(writer), CARDANO_SUCCESS);

  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "raw", 3);
  cardano_json_writer_write_raw_value(writer, "false", 0);

  result = cardano_json_writer_encode(writer, buffer, 128);

  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Don't crash
  cardano_json_writer_write_raw_value(nullptr, nullptr, 0);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_raw_value, canWriteArrayOfRawValues)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  const char* raw_value = R"({"raw": "value"})";
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "raw", 3);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_raw_value(writer, raw_value, strlen(raw_value));
  cardano_json_writer_write_raw_value(writer, raw_value, strlen(raw_value));
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"raw\": [\n    {\"raw\": \"value\"},\n    {\"raw\": \"value\"}\n  ]\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_raw_value, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_raw_value(writer, "true", 4);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_uint, writesAnUnsignedIntegerValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "uint", 4);
  cardano_json_writer_write_uint(writer, UINT_MAX);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"uint\": 4294967295\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_uint, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "uint", 4);
  cardano_json_writer_write_uint(nullptr, UINT_MAX);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_uint(nullptr, UINT_MAX);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_uint, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_uint(writer, UINT_MAX);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_uint, canWriteArrayOfUints)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "uints", 5);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_uint(writer, UINT_MAX);
  cardano_json_writer_write_uint(writer, UINT_MAX);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"uints\": [\n    4294967295,\n    4294967295\n  ]\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_signed_int, writesASignedIntegerValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "int", 3);
  cardano_json_writer_write_signed_int(writer, INT_MAX);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"int\": 2147483647\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_signed_int, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "int", 3);
  cardano_json_writer_write_signed_int(nullptr, INT_MAX);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_signed_int(nullptr, INT_MAX);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_signed_int, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_signed_int(writer, INT_MAX);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_signed_int, canWriteArrayOfSignedInts)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "ints", 4);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_signed_int(writer, INT_MAX);
  cardano_json_writer_write_signed_int(writer, INT_MAX);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"ints\": [\n    2147483647,\n    2147483647\n  ]\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_double, writesADoubleValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "double", 6);
  cardano_json_writer_write_double(writer, MAXFLOAT);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  // We need to compare the string representation of the float since the precision may vary.
  char expected[128];
  snprintf(expected, 128, "{\n  \"double\": %.17g\n}", MAXFLOAT);

  EXPECT_STREQ(encoded, expected);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_double, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "double", 6);
  cardano_json_writer_write_double(nullptr, MAXFLOAT);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_double(nullptr, MAXFLOAT);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_double, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_double(writer, MAXFLOAT);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_double, canWriteArrayOfDoubles)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "doubles", 7);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_double(writer, MAXFLOAT);
  cardano_json_writer_write_double(writer, MAXFLOAT);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  // We need to compare the string representation of the float since the precision may vary.
  char expected[128];
  snprintf(expected, 128, "{\n  \"doubles\": [\n    %.17g,\n    %.17g\n  ]\n}", MAXFLOAT, MAXFLOAT);

  EXPECT_STREQ(encoded, expected);

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_string, writesAStringValue)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "string", 6);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"string\": \"Hello, World!\"\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_string, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "string", 6);
  cardano_json_writer_write_string(writer, nullptr, 13);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Don't crash
  cardano_json_writer_write_string(nullptr, "Hello, World!", 13);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_string, returnsErrorIfUsedInWrongContext)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_write_string, canWriteArrayOfStrings)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "strings", 7);
  cardano_json_writer_write_start_array(writer);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"strings\": [\n    \"Hello, World!\",\n    \"Hello, World!\"\n  ]\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_string, writeEscapedCharacters)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "string", 6);
  cardano_json_writer_write_string(writer, "Hello, \"World\"!", 15);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "{\n  \"string\": \"Hello, \\\"World\\\"!\"\n}");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_get_context, returnsNullIfGivenNull)
{
  // Act
  cardano_json_writer_t* writer  = nullptr;
  cardano_json_context_t context = cardano_json_writer_get_context(writer);

  // Assert
  EXPECT_EQ(context, CARDANO_JSON_CONTEXT_ROOT);
}

TEST(cardano_json_writer_get_context, returnsTheCurrentContext)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_context_t context = cardano_json_writer_get_context(writer);

  // Assert
  EXPECT_EQ(context, CARDANO_JSON_CONTEXT_OBJECT);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_get_encoded_size, returnsZeroIfGivenNull)
{
  // Act
  size_t size = cardano_json_writer_get_encoded_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_json_writer_encode, returnsErrorIfGivenNull)
{
  // Arrange
  char buffer[128] = { 0 };

  // Act
  cardano_error_t result = cardano_json_writer_encode(nullptr, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_json_writer_encode, returnsErrorIfGivenNullBuffer)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_error_t result = cardano_json_writer_encode(writer, nullptr, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_encode, returnsErrorIfGivenZeroSize)
{
  // Arrange
  cardano_json_writer_t* writer      = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  char                   buffer[128] = { 0 };

  // Act
  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_encode_in_buffer, encodesInBuffer)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_buffer_t*      buffer = NULL;

  // Act
  cardano_json_writer_write_start_object(writer);
  cardano_json_writer_write_property_name(writer, "string", 6);
  cardano_json_writer_write_string(writer, "Hello, World!", 13);
  cardano_json_writer_write_end_object(writer);

  cardano_error_t result = cardano_json_writer_encode_in_buffer(writer, &buffer);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));
  EXPECT_EQ(strncmp((const char*)cardano_buffer_get_data(buffer), "{\n  \"string\": \"Hello, World!\"\n}", cardano_buffer_get_size(buffer)), 0);

  // Cleanup
  cardano_json_writer_unref(&writer);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_json_writer_encode_in_buffer, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_buffer_t* buffer = NULL;

  // Act
  cardano_error_t result = cardano_json_writer_encode_in_buffer(nullptr, &buffer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_json_writer_encode_in_buffer, returnsErrorIfGivenNullBuffer)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_error_t result = cardano_json_writer_encode_in_buffer(writer, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_encode_in_buffer, returnsErrorIfGivenNullBufferAndNullWriter)
{
  // Act
  cardano_error_t result = cardano_json_writer_encode_in_buffer(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_json_writer_encode_in_buffer, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_buffer_t* buffer = NULL;
  cardano_error_t   result = cardano_json_writer_encode_in_buffer(writer, &buffer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(buffer, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_reset, resetsTheWriter)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_start_object(writer);
  EXPECT_EQ(cardano_json_writer_reset(writer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_json_writer_get_context(writer), CARDANO_JSON_CONTEXT_ROOT);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_set_last_error, setsTheLastError)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_set_last_error(writer, "This is an error message");

  // Assert
  EXPECT_STREQ(cardano_json_writer_get_last_error(writer), "This is an error message");

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_reset, returnsErrorIfGivenNull)
{
  // Act
  cardano_error_t result = cardano_json_writer_reset(nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_json_writer_new, createsANewObjectWithRefCountOne)
{
  // Arrange
  cardano_json_writer_t* writer = nullptr;

  // Act
  writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Assert
  EXPECT_THAT(writer, testing::Not((cardano_json_writer_t*)nullptr));
  EXPECT_EQ(cardano_json_writer_refcount(writer), 1);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_json_writer_t* writer = nullptr;

  // Act
  writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_json_writer_ref(writer);

  // Assert
  EXPECT_THAT(writer, testing::Not((cardano_json_writer_t*)nullptr));
  EXPECT_EQ(cardano_json_writer_refcount(writer), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_json_writer_unref(&writer);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_new, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_json_writer_t* writer = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Assert
  EXPECT_EQ(writer, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_json_writer_new, returnsNullIfMemoryAllocationEventuallyFails)
{
  // Arrange
  cardano_json_writer_t* writer = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Assert
  EXPECT_EQ(writer, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_json_writer_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_json_writer_ref(nullptr);
}

TEST(cardano_json_writer_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_json_writer_t* writer = nullptr;

  // Act
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_json_writer_unref((cardano_json_writer_t**)nullptr);
}

TEST(cardano_json_writer_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  ;

  // Act
  cardano_json_writer_ref(writer);
  size_t ref_count = cardano_json_writer_refcount(writer);

  cardano_json_writer_unref(&writer);
  size_t updated_ref_count = cardano_json_writer_refcount(writer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_ref(writer);
  size_t ref_count = cardano_json_writer_refcount(writer);

  cardano_json_writer_unref(&writer);
  size_t updated_ref_count = cardano_json_writer_refcount(writer);

  cardano_json_writer_unref(&writer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(writer, (cardano_json_writer_t*)nullptr);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_json_writer_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}
