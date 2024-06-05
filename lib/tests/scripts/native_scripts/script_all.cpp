/**
 * \file script_all.cpp
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/scripts/native_scripts/native_script.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <cardano/scripts/native_scripts/native_script_list.h>
#include <cardano/scripts/native_scripts/script_all.h>
#include <cardano/scripts/native_scripts/script_n_of_k.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PUBKEY_SCRIPT =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* ALL_SCRIPT =
  "{\n"
  "  \"type\": \"all\",\n"
  "  \"scripts\":\n"
  "  [\n"
  "    {\n"
  "      \"type\": \"after\",\n"
  "      \"slot\": 3000\n"
  "    },\n"
  "    {\n"
  "      \"type\": \"sig\",\n"
  "      \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "    },\n"
  "    {\n"
  "      \"type\": \"before\",\n"
  "      \"slot\": 4000\n"
  "    }\n"
  "  ]\n"
  "}";

static const char* ALL_SCRIPT2 =
  "{\n"
  "  \"type\": \"all\",\n"
  "  \"scripts\":\n"
  "  [\n"
  "    {\n"
  "      \"type\": \"sig\",\n"
  "      \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "    },\n"
  "    {\n"
  "      \"type\": \"before\",\n"
  "      \"slot\": 4000\n"
  "    }\n"
  "  ]\n"
  "}";

/* UNIT TESTS ****************************************************************/

TEST(cardano_script_all_new, returnsErrorIfScriptIsNull)
{
  cardano_script_all_t* all = NULL;
  EXPECT_EQ(cardano_script_all_new(nullptr, &all), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_new, returnsErrorIfAllIsNull)
{
  EXPECT_EQ(cardano_script_all_new((cardano_native_script_list_t*)"", nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_native_script_list_t* list = (cardano_native_script_list_t*)"";

  // Act
  cardano_script_all_t* all = NULL;
  EXPECT_EQ(cardano_script_all_new(list, &all), CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_all_from_cbor, returnsErrorIfReaderIsNull)
{
  cardano_script_all_t* all = NULL;

  EXPECT_EQ(cardano_script_all_from_cbor(nullptr, &all), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_from_cbor, returnsErrorIfAllIsNull)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  EXPECT_EQ(cardano_script_all_from_cbor(reader, nullptr), CARDANO_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_all_from_cbor, returnsErrorIfInvalidCborNoArray)
{
  cardano_script_all_t*  all    = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("fe01", strlen("fe01"));

  EXPECT_EQ(cardano_script_all_from_cbor(reader, &all), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_all_from_cbor, returnsErrorIfInvalidCborNoInt)
{
  cardano_script_all_t*  all    = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82fe", strlen("82fe"));

  EXPECT_EQ(cardano_script_all_from_cbor(reader, &all), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_all_to_cbor, returnsErrorIfAllIsNull)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_script_all_to_cbor(nullptr, writer), CARDANO_POINTER_IS_NULL);

  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_all_to_cbor, returnsErrorIfWriterIsNull)
{
  cardano_script_all_t* all = (cardano_script_all_t*)"";

  EXPECT_EQ(cardano_script_all_to_cbor(all, nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_from_json, returnsErrorIfJsonIsNull)
{
  cardano_script_all_t* all = NULL;

  EXPECT_EQ(cardano_script_all_from_json(nullptr, 0, &all), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_from_json, returnsErrorIfAllIsNull)
{
  EXPECT_EQ(cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), nullptr), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_from_json, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_script_all_t* all = NULL;

  // Act
  EXPECT_EQ(cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all), CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_all_from_json, returnsErrorIfJsonStringIsInvalid)
{
  cardano_script_all_t* all = NULL;

  EXPECT_EQ(cardano_script_all_from_json("{\"key\": \"value\"}", strlen("{\"key\": \"value\"}"), &all), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_all_from_json, returnsErrorIfJsonStringIsInvalid2)
{
  cardano_script_all_t* all = NULL;

  EXPECT_EQ(cardano_script_all_from_json("}", strlen("}"), &all), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_all_from_json, returnsErrorIfTypeIsInvalid)
{
  cardano_script_all_t* all = NULL;

  EXPECT_EQ(cardano_script_all_from_json("{\"type\": \"value\"}", strlen("{\"type\": \"value\"}"), &all), CARDANO_INVALID_NATIVE_SCRIPT_TYPE);
}

TEST(cardano_script_all_get_length, returnsTheLengthOfTheAllScript)
{
  // Arrange
  cardano_script_all_t* all = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_script_all_get_length(all);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(length, 3);

  // Cleanup
  cardano_script_all_unref(&all);
}

TEST(cardano_script_all_get_length, returnsErrorIfNull)
{
  // Act
  size_t length = cardano_script_all_get_length(nullptr);

  // Assert
  ASSERT_EQ(length, 0);
}

TEST(cardano_script_all_get_scripts, returnsTheScriptsOfTheAllScript)
{
  // Arrange
  cardano_script_all_t* all = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_list_t* scripts = NULL;

  ASSERT_EQ(cardano_script_all_get_scripts(all, &scripts), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(scripts, nullptr);

  // Cleanup
  cardano_script_all_unref(&all);
  cardano_native_script_list_unref(&scripts);
}

TEST(cardano_script_all_get_scripts, returnsErrorIfAllIsNull)
{
  // Act
  cardano_native_script_list_t* scripts = NULL;

  ASSERT_EQ(cardano_script_all_get_scripts(nullptr, &scripts), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_get_scripts, returnsErrorIfScriptsIsNull)
{
  // Arrange
  cardano_script_all_t* all = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_script_all_get_scripts(all, nullptr), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_all_unref(&all);
}

TEST(cardano_script_all_set_scripts, returnsErrorIfAllIsNull)
{
  // Arrange
  cardano_native_script_list_t* scripts = (cardano_native_script_list_t*)"";

  // Act
  ASSERT_EQ(cardano_script_all_set_scripts(nullptr, scripts), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_script_all_set_scripts, returnsErrorIfScriptsIsNull)
{
  // Arrange
  cardano_script_all_t* all = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_script_all_set_scripts(all, nullptr), CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_all_unref(&all);
}

TEST(cardano_script_all_set_scripts, canSetNewList)
{
  // Arrange
  cardano_script_all_t*         all   = NULL;
  cardano_native_script_list_t* list  = NULL;
  cardano_native_script_list_t* list2 = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_from_json(ALL_SCRIPT2, strlen(ALL_SCRIPT2), &list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_script_all_set_scripts(all, list), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_script_all_get_scripts(all, &list2), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_native_script_list_equals(list, list), true);

  // Cleanup
  cardano_script_all_unref(&all);
  cardano_native_script_list_unref(&list);
  cardano_native_script_list_unref(&list);
}

TEST(cardano_script_all_equals, returnsFalseIfAllIsNull)
{
  // Arrange
  cardano_script_all_t* all = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_all_equals(nullptr, all);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_all_unref(&all);
}

TEST(cardano_script_all_equals, returnsFalseIfAllIsNull2)
{
  // Arrange
  cardano_script_all_t* all = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_all_equals(all, nullptr);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_all_unref(&all);
}

TEST(cardano_script_all_equals, returnsTrueIfBothAreTheSame)
{
  // Arrange
  cardano_script_all_t* all1 = NULL;
  cardano_script_all_t* all2 = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_all_equals(all1, all2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_script_all_unref(&all1);
  cardano_script_all_unref(&all2);
}

TEST(cardano_script_all_equals, returnsFalseIfBothAreDifferent)
{
  // Arrange
  cardano_script_all_t* all1 = NULL;
  cardano_script_all_t* all2 = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_all_from_json(ALL_SCRIPT2, strlen(ALL_SCRIPT2), &all2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_all_equals(all1, all2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_all_unref(&all1);
  cardano_script_all_unref(&all2);
}

TEST(cardano_script_all_equals, returnsFalseIfOneIsNull)
{
  // Arrange
  cardano_script_all_t* all1 = NULL;
  cardano_script_all_t* all2 = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_all_equals(all1, all2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_all_unref(&all1);
}

TEST(cardano_script_all_equals, returnsTrueIfBothAreNull)
{
  // Act
  bool result = cardano_script_all_equals(nullptr, nullptr);

  // Assert
  ASSERT_TRUE(result);
}

TEST(cardano_script_all_equals, returnsFalseIfNotTheSameType)
{
  // Arrange
  cardano_script_all_t*    all    = NULL;
  cardano_script_pubkey_t* pubkey = NULL;

  cardano_error_t error = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &all);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_all_equals(all, (cardano_script_all_t*)pubkey);
  ASSERT_FALSE(result);

  result = cardano_script_all_equals((cardano_script_all_t*)pubkey, all);
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_all_unref(&all);
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_all_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_script_all_t* script_all = nullptr;
  cardano_error_t       error      = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_all_ref(script_all);

  // Assert
  EXPECT_THAT(script_all, testing::Not((cardano_script_all_t*)nullptr));
  EXPECT_EQ(cardano_script_all_refcount(script_all), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_script_all_unref(&script_all);
  cardano_script_all_unref(&script_all);
}

TEST(cardano_script_all_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_all_ref(nullptr);
}

TEST(cardano_script_all_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_script_all_t* script_all = nullptr;

  // Act
  cardano_script_all_unref(&script_all);
}

TEST(cardano_script_all_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_all_unref((cardano_script_all_t**)nullptr);
}

TEST(cardano_script_all_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_script_all_t* script_all = nullptr;
  cardano_error_t       error      = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_all_ref(script_all);
  size_t ref_count = cardano_script_all_refcount(script_all);

  cardano_script_all_unref(&script_all);
  size_t updated_ref_count = cardano_script_all_refcount(script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_script_all_unref(&script_all);
}

TEST(cardano_script_all_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_script_all_t* script_all = nullptr;
  cardano_error_t       error      = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_all_ref(script_all);
  size_t ref_count = cardano_script_all_refcount(script_all);

  cardano_script_all_unref(&script_all);
  size_t updated_ref_count = cardano_script_all_refcount(script_all);

  cardano_script_all_unref(&script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(script_all, (cardano_script_all_t*)nullptr);

  // Cleanup
  cardano_script_all_unref(&script_all);
}

TEST(cardano_script_all_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_script_all_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_script_all_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_script_all_t* script_all = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_script_all_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_all_get_last_error(script_all), "Object is NULL.");
}

TEST(cardano_script_all_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_script_all_t* script_all = nullptr;
  cardano_error_t       error      = cardano_script_all_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &script_all);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_script_all_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_all_get_last_error(script_all), "");

  // Cleanup
  cardano_script_all_unref(&script_all);
}
