/**
 * \file native_script.cpp
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
#include <cardano/scripts/native_scripts/script_any.h>
#include <cardano/scripts/native_scripts/script_invalid_after.h>
#include <cardano/scripts/native_scripts/script_invalid_before.h>
#include <cardano/scripts/native_scripts/script_n_of_k.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

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

static const char* ALL_SCRIPT_2 =
  "{\n"
  "  \"type\": \"all\",\n"
  "  \"scripts\":\n"
  "  [\n"
  "    {\n"
  "      \"type\": \"after\",\n"
  "      \"slot\": 2000\n"
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

static const char* ALL_SCRIPT_3 =
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

TEST(cardano_native_script_list_new, createsANewInstanceOfNativeScriptList)
{
  cardano_native_script_list_t* list = NULL;

  EXPECT_EQ(cardano_native_script_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_native_script_list_get_length(list), 0);

  cardano_native_script_list_unref(&list);
}

TEST(cardano_native_script_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_native_script_list_new(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_native_script_list_t* list = NULL;

  EXPECT_EQ(cardano_native_script_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_native_script_list_t* list = NULL;

  EXPECT_EQ(cardano_native_script_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_list_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_native_script_list_from_cbor(nullptr, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_from_cbor, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  // Act
  cardano_error_t result = cardano_native_script_list_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_list_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_native_script_list_t* list   = NULL;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_native_script_list_from_cbor(reader, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_list_from_cbor, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_native_script_list_t* list   = NULL;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_native_script_list_from_cbor(reader, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_list_from_cbor, returnsErrorIfMemoryAllocationFails3)
{
  // Arrange
  cardano_native_script_list_t* list   = NULL;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex("82041a02625a0a", strlen("82041a02625a0a"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_native_script_list_from_cbor(reader, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_list_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_native_script_list_from_cbor(nullptr, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_from_cbor, returnErrorIfListIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  // Act
  cardano_error_t result = cardano_native_script_list_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_list_from_json, returnsErrorIfJsonIsNull)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_native_script_list_from_json(nullptr, 0, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_from_json, returnsErrorIfListIsNull)
{
  // Act
  cardano_error_t result = cardano_native_script_list_from_json("[]", 2, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_from_json, returnsErrorIfJsonIsZeroLength)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_native_script_list_from_json("", 0, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_list_from_json, returnsErrorIfJsonIsInvalid)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_native_script_list_from_json("[", 1, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_list_from_json, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_native_script_list_from_json("[]", 2, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_list_from_json, returnErrorIfScriptsIsNotArray)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_native_script_list_from_json("{ \"scripts\": 1 }", 2, &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_list_from_json, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &list);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_list_to_cbor, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_native_script_list_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_native_script_list_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_native_script_list_to_cbor(list, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_to_cbor, canDecodeIndefiniteListCbor)
{
  // Arrange
  cardano_native_script_list_t* list   = NULL;
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex("9f8205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0ff", strlen("9f8205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0ff"));

  cardano_error_t result = cardano_native_script_list_from_cbor(reader, &list);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_native_script_list_to_cbor(list, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_native_script_list_unref(&list);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_list_to_cbor, returnErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_native_script_list_to_cbor((cardano_native_script_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_native_script_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_native_script_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_native_script_list_get(nullptr, 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_native_script_list_get((cardano_native_script_list_t*)"", 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  cardano_error_t error = cardano_native_script_list_from_json("{ \"scripts\": [] }", strlen("{ \"scripts\": [] }"), &list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_t* script = NULL;
  error                           = cardano_native_script_list_get(list, 0, &script);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_native_script_list_unref(&list);
}

TEST(cardano_native_script_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_native_script_list_t* list = NULL;

  cardano_error_t error = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_t* script = NULL;
  error                           = cardano_native_script_list_get(list, 0, &script);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_native_script_type_t type;
  EXPECT_EQ(cardano_native_script_get_type(script, &type), CARDANO_SUCCESS);

  ASSERT_EQ(type, CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER);

  // Cleanup
  cardano_native_script_list_unref(&list);
  cardano_native_script_unref(&script);
}

TEST(cardano_native_script_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_native_script_list_t* native_script_list = nullptr;
  cardano_error_t               error              = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &native_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_list_ref(native_script_list);

  // Assert
  EXPECT_THAT(native_script_list, testing::Not((cardano_native_script_list_t*)nullptr));
  EXPECT_EQ(cardano_native_script_list_refcount(native_script_list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_native_script_list_unref(&native_script_list);
  cardano_native_script_list_unref(&native_script_list);
}

TEST(cardano_native_script_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_native_script_list_ref(nullptr);
}

TEST(cardano_native_script_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_native_script_list_t* native_script_list = nullptr;

  // Act
  cardano_native_script_list_unref(&native_script_list);
}

TEST(cardano_native_script_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_native_script_list_unref((cardano_native_script_list_t**)nullptr);
}

TEST(cardano_native_script_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_native_script_list_t* native_script_list = nullptr;
  cardano_error_t               error              = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &native_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_list_ref(native_script_list);
  size_t ref_count = cardano_native_script_list_refcount(native_script_list);

  cardano_native_script_list_unref(&native_script_list);
  size_t updated_ref_count = cardano_native_script_list_refcount(native_script_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_native_script_list_unref(&native_script_list);
}

TEST(cardano_native_script_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_native_script_list_t* native_script_list = nullptr;
  cardano_error_t               error              = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &native_script_list);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_list_ref(native_script_list);
  size_t ref_count = cardano_native_script_list_refcount(native_script_list);

  cardano_native_script_list_unref(&native_script_list);
  size_t updated_ref_count = cardano_native_script_list_refcount(native_script_list);

  cardano_native_script_list_unref(&native_script_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(native_script_list, (cardano_native_script_list_t*)nullptr);

  // Cleanup
  cardano_native_script_list_unref(&native_script_list);
}

TEST(cardano_native_script_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_native_script_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_native_script_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_native_script_list_t* native_script_list = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_native_script_list_set_last_error(native_script_list, message);

  // Assert
  EXPECT_STREQ(cardano_native_script_list_get_last_error(native_script_list), "Object is NULL.");
}

TEST(cardano_native_script_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_native_script_list_t* native_script_list = nullptr;
  cardano_error_t               error              = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &native_script_list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_native_script_list_set_last_error(native_script_list, message);

  // Assert
  EXPECT_STREQ(cardano_native_script_list_get_last_error(native_script_list), "");

  // Cleanup
  cardano_native_script_list_unref(&native_script_list);
}

TEST(cardano_native_script_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_native_script_t* script = nullptr;

  // Act
  cardano_error_t result = cardano_native_script_list_add(nullptr, script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_native_script_list_add((cardano_native_script_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_list_equals, returnsFalseIfListsAreDifferent)
{
  // Arrange
  cardano_native_script_list_t* list1 = nullptr;
  cardano_native_script_list_t* list2 = nullptr;

  cardano_error_t error = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &list1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_from_json(ALL_SCRIPT_2, strlen(ALL_SCRIPT_2), &list2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_list_equals(list1, list2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_list_unref(&list1);
  cardano_native_script_list_unref(&list2);
}

TEST(cardano_native_script_list_equals, returnsFalseIfListsAreDifferentSize)
{
  // Arrange
  cardano_native_script_list_t* list1 = nullptr;
  cardano_native_script_list_t* list2 = nullptr;

  cardano_error_t error = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &list1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_from_json(ALL_SCRIPT_3, strlen(ALL_SCRIPT_3), &list2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_list_equals(list1, list2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_list_unref(&list1);
  cardano_native_script_list_unref(&list2);
}

TEST(cardano_native_script_list_equals, returnsTrueIfListsAreEqual)
{
  // Arrange
  cardano_native_script_list_t* list1 = nullptr;
  cardano_native_script_list_t* list2 = nullptr;

  cardano_error_t error = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &list1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &list2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_list_equals(list1, list2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_list_unref(&list1);
  cardano_native_script_list_unref(&list2);
}

TEST(cardano_native_script_list_equals, returnsFalseIfOneListIsNull)
{
  // Arrange
  cardano_native_script_list_t* list1 = nullptr;
  cardano_native_script_list_t* list2 = nullptr;

  cardano_error_t error = cardano_native_script_list_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &list1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_list_equals(list1, list2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_list_unref(&list1);
}

TEST(cardano_native_script_list_equals, returnsTrueIfBothListsAreNull)
{
  // Act
  bool result = cardano_native_script_list_equals(nullptr, nullptr);

  // Assert
  ASSERT_TRUE(result);
}
