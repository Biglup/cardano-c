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

extern "C" {
#include "../src/json/internals/json_writer_common.h"
#include "../src/json/internals/utf8.h"
}

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
  cardano_json_writer_write_double(writer, FLT_MAX);

  cardano_json_writer_write_property_name(writer, "e", 1);
  cardano_json_writer_write_double(writer, -FLT_MAX);

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
test_primitives_as_string(const cardano_json_format_t format, const char* expected_string)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  // Act
  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "a", 1);
  cardano_json_writer_write_uint_as_string(writer, UINT_MAX);

  cardano_json_writer_write_property_name(writer, "b", 1);
  cardano_json_writer_write_signed_int_as_string(writer, INT_MAX);

  cardano_json_writer_write_property_name(writer, "c", 1);
  cardano_json_writer_write_signed_int_as_string(writer, -INT_MAX);

  cardano_json_writer_write_property_name(writer, "d", 1);
  cardano_json_writer_write_double_as_string(writer, FLT_MAX);

  cardano_json_writer_write_property_name(writer, "e", 1);
  cardano_json_writer_write_double_as_string(writer, -FLT_MAX);

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
  cardano_json_writer_write_double(writer, FLT_MAX);
  cardano_json_writer_write_double(writer, -FLT_MAX);
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

TEST(cardano_json_writer, canCreateNumericPrimitivesAsStrings)
{
  test_primitives_as_string(
    CARDANO_JSON_FORMAT_PRETTY,
    R"({
  "a": "4294967295",
  "b": "2147483647",
  "c": "-2147483647",
  "d": "3.4028234663852886e+38",
  "e": "-3.4028234663852886e+38"
})");

  test_primitives_as_string(
    CARDANO_JSON_FORMAT_COMPACT,
    R"({"a":"4294967295","b":"2147483647","c":"-2147483647","d":"3.4028234663852886e+38","e":"-3.4028234663852886e+38"})");
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
  cardano_json_writer_write_start_object(writer);
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

TEST(cardano_json_writer_write_uint, canWriteSingleValueAtRoot)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_uint(writer, UINT_MAX);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "4294967295");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_int, canWriteSingleValueAtRoot)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_signed_int(writer, INT_MAX);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "2147483647");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_double, canWriteSingleValueAtRoot)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_double(writer, FLT_MAX);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "3.4028234663852886e+38");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_string, canWriteSingleValueAtRoot)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_string(writer, "Hello, World!", 13);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "\"Hello, World!\"");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_bool, canWriteSingleValueAtRoot)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_bool(writer, true);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "true");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_null, canWriteSingleValueAtRoot)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_write_null(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "null");

  // Cleanup
  cardano_json_writer_unref(&writer);
  free(encoded);
}

TEST(cardano_json_writer_write_bigint, canWriteSingleValueAtRoot)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  cardano_bigint_t*      bigint = NULL;

  EXPECT_EQ(cardano_bigint_from_string("123456789123456789", strlen("123456789123456789"), 10, &bigint), CARDANO_SUCCESS);

  // Act
  cardano_json_writer_write_bigint(writer, bigint);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  EXPECT_STREQ(encoded, "\"123456789123456789\"");

  // Cleanup
  cardano_bigint_unref(&bigint);
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
  cardano_json_writer_write_start_object(writer);
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
  cardano_json_writer_write_double(writer, FLT_MAX);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  // We need to compare the string representation of the float since the precision may vary.
  char expected[128];
  snprintf(expected, 128, "{\n  \"double\": %.17g\n}", FLT_MAX);

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
  cardano_json_writer_write_double(nullptr, FLT_MAX);

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, 128);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ENCODING);

  // Don't crash
  cardano_json_writer_write_double(nullptr, FLT_MAX);

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
  cardano_json_writer_write_double(writer, FLT_MAX);

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
  cardano_json_writer_write_double(writer, FLT_MAX);
  cardano_json_writer_write_double(writer, FLT_MAX);
  cardano_json_writer_write_end_array(writer);
  cardano_json_writer_write_end_object(writer);

  // Assert
  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  // We need to compare the string representation of the float since the precision may vary.
  char expected[128];
  snprintf(expected, 128, "{\n  \"doubles\": [\n    %.17g,\n    %.17g\n  ]\n}", FLT_MAX, FLT_MAX);

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

TEST(cardano_json_writer_write_object, canWriteObject)
{
  // Act
  const char*  json      = "{\"data\":[{\"MainId\":1111,\"firstName\":\"Sherlock\",\"lastName\":\"Homes\",\"categories\":[{\"CategoryID\":-1.3,\"CategoryName\":\"Example\"}]},{\"MainId\":122,\"firstName\":\"James\",\"lastName\":\"Watson\",\"categories\":[{\"CategoryID\":2,\"CategoryName\":\"Example2\"}]}],\"messages\":[],\"success\":true}";
  const size_t json_size = strlen(json);

  cardano_json_object_t* object = cardano_json_object_parse(json, json_size);

  EXPECT_NE(object, nullptr);

  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_json_writer_write_object(writer, object);

  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);
  char*        encoded      = (char*)malloc(encoded_size);

  ASSERT_EQ(cardano_json_writer_encode(writer, encoded, encoded_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(encoded, json);

  // Cleanup
  cardano_json_writer_unref(&writer);
  cardano_json_object_unref(&object);
  free(encoded);
}

TEST(cardano_json_writer_set_message_if_error, setsTheMessageIfError)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_set_message_if_error(writer, CARDANO_ERROR_POINTER_IS_NULL, "This is an error message");

  // Assert
  EXPECT_STREQ(cardano_json_writer_get_last_error(writer), "This is an error message");

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_json_writer_set_message_if_error, doesNothingIfAlreadyInError)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  // Act
  cardano_json_writer_set_message_if_error(writer, CARDANO_ERROR_POINTER_IS_NULL, "This is an error message");
  cardano_json_writer_set_message_if_error(writer, CARDANO_ERROR_POINTER_IS_NULL, "This is another error message");

  // Assert
  EXPECT_STREQ(cardano_json_writer_get_last_error(writer), "This is an error message");

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_utf8_sequence_length, canComputeCorrectByteLenght)
{
  // Act
  size_t length = cardano_utf8_sequence_length(0x00);
  EXPECT_EQ(length, 1);

  length = cardano_utf8_sequence_length(0x7F);
  EXPECT_EQ(length, 1);

  length = cardano_utf8_sequence_length(0xC2);
  EXPECT_EQ(length, 2);

  length = cardano_utf8_sequence_length(0xDF);
  EXPECT_EQ(length, 2);

  length = cardano_utf8_sequence_length(0xE0);
  EXPECT_EQ(length, 3);

  length = cardano_utf8_sequence_length(0xEF);
  EXPECT_EQ(length, 3);

  length = cardano_utf8_sequence_length(0xF0);
  EXPECT_EQ(length, 4);

  length = cardano_utf8_sequence_length(0xF4);
  EXPECT_EQ(length, 4);

  length = cardano_utf8_sequence_length(0xFF);
  EXPECT_EQ(length, 0);
}

TEST(cardano_parse_hex_digit, canConverHexNibbleToInt)
{
  // Act
  int32_t digit = cardano_parse_hex_digit('0');
  EXPECT_EQ(digit, 0);

  digit = cardano_parse_hex_digit('1');
  EXPECT_EQ(digit, 1);

  digit = cardano_parse_hex_digit('2');
  EXPECT_EQ(digit, 2);

  digit = cardano_parse_hex_digit('3');
  EXPECT_EQ(digit, 3);

  digit = cardano_parse_hex_digit('4');
  EXPECT_EQ(digit, 4);

  digit = cardano_parse_hex_digit('5');
  EXPECT_EQ(digit, 5);

  digit = cardano_parse_hex_digit('6');
  EXPECT_EQ(digit, 6);

  digit = cardano_parse_hex_digit('7');
  EXPECT_EQ(digit, 7);

  digit = cardano_parse_hex_digit('8');
  EXPECT_EQ(digit, 8);

  digit = cardano_parse_hex_digit('9');
  EXPECT_EQ(digit, 9);

  digit = cardano_parse_hex_digit('A');
  EXPECT_EQ(digit, 10);

  digit = cardano_parse_hex_digit('B');
  EXPECT_EQ(digit, 11);

  digit = cardano_parse_hex_digit('C');
  EXPECT_EQ(digit, 12);

  digit = cardano_parse_hex_digit('D');
  EXPECT_EQ(digit, 13);

  digit = cardano_parse_hex_digit('E');
  EXPECT_EQ(digit, 14);

  digit = cardano_parse_hex_digit('F');
  EXPECT_EQ(digit, 15);

  digit = cardano_parse_hex_digit('a');
  EXPECT_EQ(digit, 10);

  digit = cardano_parse_hex_digit('b');
  EXPECT_EQ(digit, 11);

  digit = cardano_parse_hex_digit('c');
  EXPECT_EQ(digit, 12);

  digit = cardano_parse_hex_digit('d');
  EXPECT_EQ(digit, 13);

  digit = cardano_parse_hex_digit('e');
  EXPECT_EQ(digit, 14);

  digit = cardano_parse_hex_digit('f');
  EXPECT_EQ(digit, 15);

  digit = cardano_parse_hex_digit('G');
  EXPECT_EQ(digit, -1);

  digit = cardano_parse_hex_digit('g');
  EXPECT_EQ(digit, -1);
}

TEST(cardano_encode_utf8, canEncodeUtf8)
{
  // Arrange
  char buffer[4] = { 0 };

  // Act & Assert
  size_t length = cardano_encode_utf8(0x00, buffer);
  EXPECT_EQ(length, 1);
  EXPECT_EQ((unsigned char)buffer[0], 0x00);

  length = cardano_encode_utf8(0x7F, buffer);
  EXPECT_EQ(length, 1);
  EXPECT_EQ((unsigned char)buffer[0], 0x7F);

  length = cardano_encode_utf8(0x80, buffer);
  EXPECT_EQ(length, 2);
  EXPECT_EQ((unsigned char)buffer[0], 0xC2);
  EXPECT_EQ((unsigned char)buffer[1], 0x80);

  length = cardano_encode_utf8(0x7FF, buffer);
  EXPECT_EQ(length, 2);
  EXPECT_EQ((unsigned char)buffer[0], 0xDF);
  EXPECT_EQ((unsigned char)buffer[1], 0xBF);

  length = cardano_encode_utf8(0x800, buffer);
  EXPECT_EQ(length, 3);
  EXPECT_EQ((unsigned char)buffer[0], 0xE0);
  EXPECT_EQ((unsigned char)buffer[1], 0xA0);
  EXPECT_EQ((unsigned char)buffer[2], 0x80);

  length = cardano_encode_utf8(0xFFFF, buffer);
  EXPECT_EQ(length, 3);
  EXPECT_EQ((unsigned char)buffer[0], 0xEF);
  EXPECT_EQ((unsigned char)buffer[1], 0xBF);
  EXPECT_EQ((unsigned char)buffer[2], 0xBF);

  length = cardano_encode_utf8(0x10000, buffer);
  EXPECT_EQ(length, 4);
  EXPECT_EQ((unsigned char)buffer[0], 0xF0);
  EXPECT_EQ((unsigned char)buffer[1], 0x90);
  EXPECT_EQ((unsigned char)buffer[2], 0x80);
  EXPECT_EQ((unsigned char)buffer[3], 0x80);

  length = cardano_encode_utf8(0x10FFFF, buffer);
  EXPECT_EQ(length, 4);
  EXPECT_EQ((unsigned char)buffer[0], 0xF4);
  EXPECT_EQ((unsigned char)buffer[1], 0x8F);
  EXPECT_EQ((unsigned char)buffer[2], 0xBF);
  EXPECT_EQ((unsigned char)buffer[3], 0xBF);

  // Invalid code points
  length = cardano_encode_utf8(0x110000, buffer);
  EXPECT_EQ(length, 0);

  length = cardano_encode_utf8(-1, buffer);
  EXPECT_EQ(length, 1);
}

TEST(cardano_encode_utf8, fourByteSequence)
{
  char   utf8_buf[4] = { 0 };
  size_t length      = cardano_encode_utf8(0x1F98B, utf8_buf); // U+1F98B = 🦋

  EXPECT_EQ(length, 4);
  EXPECT_EQ((byte_t)utf8_buf[0], 0xF0);
  EXPECT_EQ((byte_t)utf8_buf[1], 0x9F);
  EXPECT_EQ((byte_t)utf8_buf[2], 0xA6);
  EXPECT_EQ((byte_t)utf8_buf[3], 0x8B);
}

TEST(cardano_decode_unicode_sequence, BasicUnicodeEscapes)
{
  char        output[4] = { 0 };
  const char* input     = "\\u0041"; // 'A'
  size_t      len       = cardano_decode_unicode_sequence(input, strlen(input), output);

  EXPECT_EQ(len, 1);
  EXPECT_EQ((unsigned char)output[0], 'A');
  EXPECT_EQ(output[1], '\0'); // Ensure null termination
}

TEST(cardano_decode_unicode_sequence, surrogatePairs)
{
  char        output[4] = { 0 };
  const char* input     = "\\uD83D\\uDE00"; // 😀
  size_t      len       = cardano_decode_unicode_sequence(input, strlen(input), output);

  EXPECT_EQ(len, 4);
  EXPECT_EQ((unsigned char)output[0], 0xF0);
  EXPECT_EQ((unsigned char)output[1], 0x9F);
  EXPECT_EQ((unsigned char)output[2], 0x98);
  EXPECT_EQ((unsigned char)output[3], 0x80);
}

TEST(cardano_decode_unicode_sequence, surrogatePairs2)
{
  char        output[4] = { 0 };
  const char* input     = "\\uD83E\\uDD8B"; // 🦋
  size_t      len       = cardano_decode_unicode_sequence(input, strlen(input), output);

  EXPECT_EQ(len, 4);
  EXPECT_EQ((unsigned char)output[0], 0xf0);
  EXPECT_EQ((unsigned char)output[1], 0x9f);
  EXPECT_EQ((unsigned char)output[2], 0xa6);
  EXPECT_EQ((unsigned char)output[3], 0x8b);
}

TEST(cardano_decode_unicode_sequence, InvalidUnicodeEscape)
{
  char        output[4] = { 0 };
  const char* input     = "\\uZZZZ";
  size_t      len       = cardano_decode_unicode_sequence(input, strlen(input), output);

  EXPECT_EQ(len, 0);
}

TEST(cardano_decode_unicode_sequence, IncompleteEscape)
{
  char        output[4] = { 0 };
  const char* input     = "\\u00";
  size_t      len       = cardano_decode_unicode_sequence(input, strlen(input), output);

  EXPECT_EQ(len, 0);
}

TEST(cardano_decode_unicode_sequence, InvalidSurrogatePair)
{
  char        output[4] = { 0 };
  const char* input     = "\\uD83D\\u1234";
  size_t      len       = cardano_decode_unicode_sequence(input, strlen(input), output);

  EXPECT_EQ(len, 0);
}

TEST(cardano_decode_unicode_sequence, BoundaryCases)
{
  char output[4] = { 0 };

  const char* input1 = "\\u0000";
  size_t      len1   = cardano_decode_unicode_sequence(input1, strlen(input1), output);
  EXPECT_EQ(len1, 1);
  EXPECT_EQ((unsigned char)output[0], 0x00);

  const char* input2 = "\\uDBFF\\uDFFF";
  size_t      len2   = cardano_decode_unicode_sequence(input2, strlen(input2), output);
  EXPECT_EQ(len2, 4);
  EXPECT_EQ((unsigned char)output[0], 0xF4);
  EXPECT_EQ((unsigned char)output[1], 0x8F);
  EXPECT_EQ((unsigned char)output[2], 0xBF);
  EXPECT_EQ((unsigned char)output[3], 0xBF);
}