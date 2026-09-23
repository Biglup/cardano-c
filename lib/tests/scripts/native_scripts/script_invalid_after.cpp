/**
 * \file script_invalid_after.cpp
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
#include <cardano/json/json_writer.h>
#include <cardano/scripts/native_scripts/native_script_list.h>
#include <cardano/scripts/native_scripts/script_invalid_after.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PUBKEY_SCRIPT =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* AFTER_SCRIPT =
  "{\n"
  "  \"type\": \"before\",\n"
  "  \"slot\": 3000\n"
  "}";

static const char* AFTER_SCRIPT2 =
  "{\n"
  "  \"type\": \"before\",\n"
  "  \"slot\": 4000\n"
  "}";

static const char* NON_MINIMAL_INVALID_AFTER_CBOR = "82051b0000000000000007";
static const char* MINIMAL_INVALID_AFTER_CBOR     = "820507";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Serializes a script_invalid_after and returns its CBOR as a hex string.
 *
 * @param script The script_invalid_after to serialize.
 *
 * @return The CBOR hex string. The caller must release it with free.
 */
static char*
script_invalid_after_to_cbor_hex(const cardano_script_invalid_after_t* script)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_script_invalid_after_to_cbor(script, writer), CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return hex;
}

/**
 * Reads a script_invalid_after from a CBOR hex string.
 *
 * @param cbor_hex The CBOR hex string.
 *
 * @return The script_invalid_after object.
 */
static cardano_script_invalid_after_t*
script_invalid_after_from_cbor_hex(const char* cbor_hex)
{
  cardano_cbor_reader_t*          reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_script_invalid_after_t* script = NULL;

  EXPECT_EQ(cardano_script_invalid_after_from_cbor(reader, &script), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return script;
}

/**
 * Releases a script_invalid_after and a hex string returned by script_invalid_after_to_cbor_hex.
 *
 * @param script The script_invalid_after to release.
 * @param hex The hex string to release.
 */
static void
script_invalid_after_unref_and_free(cardano_script_invalid_after_t** script, char* hex)
{
  cardano_script_invalid_after_unref(script);
  free(hex);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_script_invalid_after_new, returnsErrorIfInvalidAfterIsNull)
{
  EXPECT_EQ(cardano_script_invalid_after_new(0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_after_new, returnsErrorIfMemoryInvalidAfterocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_script_invalid_after_t* invalid_after = NULL;
  EXPECT_EQ(cardano_script_invalid_after_new(0, &invalid_after), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_invalid_after_from_cbor, returnsErrorIfReaderIsNull)
{
  cardano_script_invalid_after_t* invalid_after = NULL;

  EXPECT_EQ(cardano_script_invalid_after_from_cbor(nullptr, &invalid_after), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_after_from_cbor, returnsErrorIfInvalidAfterIsNull)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  EXPECT_EQ(cardano_script_invalid_after_from_cbor(reader, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_invalid_after_from_cbor, returnsErrorIfInvalidCborNoArray)
{
  cardano_script_invalid_after_t* invalid_after = NULL;
  cardano_cbor_reader_t*          reader        = cardano_cbor_reader_from_hex("fe01", strlen("fe01"));

  EXPECT_EQ(cardano_script_invalid_after_from_cbor(reader, &invalid_after), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_invalid_after_from_cbor, returnsErrorIfInvalidCborNoInt)
{
  cardano_script_invalid_after_t* invalid_after = NULL;
  cardano_cbor_reader_t*          reader        = cardano_cbor_reader_from_hex("82fe", strlen("82fe"));

  EXPECT_EQ(cardano_script_invalid_after_from_cbor(reader, &invalid_after), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_invalid_after_to_cbor, returnsErrorIfInvalidAfterIsNull)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_script_invalid_after_to_cbor(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_invalid_after_to_cbor, returnsErrorIfWriterIsNull)
{
  cardano_script_invalid_after_t* invalid_after = (cardano_script_invalid_after_t*)"";

  EXPECT_EQ(cardano_script_invalid_after_to_cbor(invalid_after, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_after_from_json, returnsErrorIfJsonIsNull)
{
  cardano_script_invalid_after_t* invalid_after = NULL;

  EXPECT_EQ(cardano_script_invalid_after_from_json(nullptr, 0, &invalid_after), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_after_from_json, returnsErrorIfInvalidAfterIsNull)
{
  EXPECT_EQ(cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_after_from_json, returnsErrorIfMemoryInvalidAfterocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_script_invalid_after_t* invalid_after = NULL;

  // Act
  EXPECT_EQ(cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after), CARDANO_ERROR_INVALID_JSON);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_invalid_after_from_json, returnsErrorIfJsonStringIsInvalid)
{
  cardano_script_invalid_after_t* invalid_after = NULL;

  EXPECT_EQ(cardano_script_invalid_after_from_json("{\"key\": \"value\"}", strlen("{\"key\": \"value\"}"), &invalid_after), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_invalid_after_from_json, returnsErrorIfJsonStringIsInvalid2)
{
  cardano_script_invalid_after_t* invalid_after = NULL;

  EXPECT_EQ(cardano_script_invalid_after_from_json("}", strlen("}"), &invalid_after), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_invalid_after_from_json, returnsErrorIfSizeIsZero)
{
  cardano_script_invalid_after_t* invalid_after = NULL;

  EXPECT_EQ(cardano_script_invalid_after_from_json("{\"type\": \"value\"}", 0, &invalid_after), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_invalid_after_equals, returnsFalseIfInvalidAfterIsNull)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after = NULL;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_after_equals(nullptr, invalid_after);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after);
}

TEST(cardano_script_invalid_after_equals, returnsFalseIfInvalidAfterIsNull2)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after = NULL;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_after_equals(invalid_after, nullptr);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after);
}

TEST(cardano_script_invalid_after_equals, returnsTrueIfBothAreTheSame)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after1 = NULL;
  cardano_script_invalid_after_t* invalid_after2 = NULL;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_after_equals(invalid_after1, invalid_after2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after1);
  cardano_script_invalid_after_unref(&invalid_after2);
}

TEST(cardano_script_invalid_after_equals, returnsFalseIfBothAreDifferent)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after1 = NULL;
  cardano_script_invalid_after_t* invalid_after2 = NULL;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_invalid_after_from_json(AFTER_SCRIPT2, strlen(AFTER_SCRIPT2), &invalid_after2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_after_equals(invalid_after1, invalid_after2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after1);
  cardano_script_invalid_after_unref(&invalid_after2);
}

TEST(cardano_script_invalid_after_equals, returnsFalseIfOneIsNull)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after1 = NULL;
  cardano_script_invalid_after_t* invalid_after2 = NULL;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_after_equals(invalid_after1, invalid_after2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after1);
}

TEST(cardano_script_invalid_after_equals, returnsTrueIfBothAreNull)
{
  // Act
  bool result = cardano_script_invalid_after_equals(nullptr, nullptr);

  // Assert
  ASSERT_TRUE(result);
}

TEST(cardano_script_invalid_after_equals, returnsFalseIfNotTheSameType)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after = NULL;
  cardano_script_pubkey_t*        pubkey        = NULL;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_after_equals(invalid_after, (cardano_script_invalid_after_t*)pubkey);
  ASSERT_FALSE(result);

  result = cardano_script_invalid_after_equals((cardano_script_invalid_after_t*)pubkey, invalid_after);
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after);
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_invalid_after_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_script_invalid_after_t* script_all = nullptr;
  cardano_error_t                 error      = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_invalid_after_ref(script_all);

  // Assert
  EXPECT_THAT(script_all, testing::Not((cardano_script_invalid_after_t*)nullptr));
  EXPECT_EQ(cardano_script_invalid_after_refcount(script_all), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_script_invalid_after_unref(&script_all);
  cardano_script_invalid_after_unref(&script_all);
}

TEST(cardano_script_invalid_after_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_invalid_after_ref(nullptr);
}

TEST(cardano_script_invalid_after_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_script_invalid_after_t* script_all = nullptr;

  // Act
  cardano_script_invalid_after_unref(&script_all);
}

TEST(cardano_script_invalid_after_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_invalid_after_unref((cardano_script_invalid_after_t**)nullptr);
}

TEST(cardano_script_invalid_after_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_script_invalid_after_t* script_all = nullptr;
  cardano_error_t                 error      = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_invalid_after_ref(script_all);
  size_t ref_count = cardano_script_invalid_after_refcount(script_all);

  cardano_script_invalid_after_unref(&script_all);
  size_t updated_ref_count = cardano_script_invalid_after_refcount(script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_script_invalid_after_unref(&script_all);
}

TEST(cardano_script_invalid_after_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_script_invalid_after_t* script_all = nullptr;
  cardano_error_t                 error      = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_invalid_after_ref(script_all);
  size_t ref_count = cardano_script_invalid_after_refcount(script_all);

  cardano_script_invalid_after_unref(&script_all);
  size_t updated_ref_count = cardano_script_invalid_after_refcount(script_all);

  cardano_script_invalid_after_unref(&script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(script_all, (cardano_script_invalid_after_t*)nullptr);

  // Cleanup
  cardano_script_invalid_after_unref(&script_all);
}

TEST(cardano_script_invalid_after_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_script_invalid_after_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_script_invalid_after_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_script_invalid_after_t* script_all = nullptr;
  const char*                     message    = "This is a test message";

  // Act
  cardano_script_invalid_after_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_invalid_after_get_last_error(script_all), "Object is NULL.");
}

TEST(cardano_script_invalid_after_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_script_invalid_after_t* script_all = nullptr;
  cardano_error_t                 error      = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script_all);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_script_invalid_after_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_invalid_after_get_last_error(script_all), "");

  // Cleanup
  cardano_script_invalid_after_unref(&script_all);
}

TEST(cardano_script_invalid_after_get_slot, returnsZeroIfInvalidAfterIsNull)
{
  // Act
  uint64_t slot = 0;

  EXPECT_EQ(cardano_script_invalid_after_get_slot(nullptr, &slot), CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(slot, 0);
}

TEST(cardano_script_invalid_after_get_slot, returnsErrorIfSlotIsNull)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after = nullptr;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_script_invalid_after_get_slot(invalid_after, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after);
}

TEST(cardano_script_invalid_after_get_slot, returnsSlot)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after = nullptr;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t slot = 0;
  EXPECT_EQ(cardano_script_invalid_after_get_slot(invalid_after, &slot), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(slot, 3000);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after);
}

TEST(cardano_script_invalid_after_set_slot, returnsErrorIfInvalidAfterIsNull)
{
  // Act
  EXPECT_EQ(cardano_script_invalid_after_set_slot(nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_after_set_slot, setsSlot)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after = nullptr;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_script_invalid_after_set_slot(invalid_after, 4000), CARDANO_SUCCESS);

  // Assert
  uint64_t slot = 0;
  EXPECT_EQ(cardano_script_invalid_after_get_slot(invalid_after, &slot), CARDANO_SUCCESS);
  EXPECT_EQ(slot, 4000);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after);
}

TEST(cardano_script_invalid_after_to_cip116_json, canSerializeInvalidAfter)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after = nullptr;

  cardano_error_t error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  ASSERT_THAT(writer, testing::Not((cardano_json_writer_t*)nullptr));

  // Act
  error = cardano_script_invalid_after_to_cip116_json(invalid_after, writer);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  char   buffer[256] = { 0 };
  size_t size        = sizeof(buffer);

  error = cardano_json_writer_encode(writer, buffer, size);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char* expected_json =
    "{\n"
    "  \"tag\": \"timelock_expiry\",\n"
    "  \"slot\": \"3000\"\n"
    "}";

  EXPECT_STREQ((const char*)buffer, expected_json);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_script_invalid_after_to_cip116_json, returnsErrorIfInvalidAfterIsNull)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  ASSERT_THAT(writer, testing::Not((cardano_json_writer_t*)nullptr));

  // Act
  cardano_error_t error = cardano_script_invalid_after_to_cip116_json(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&writer);
}

TEST(cardano_script_invalid_after_from_cbor, preservesTheOriginalCborEncoding)
{
  // Arrange
  cardano_script_invalid_after_t* script = script_invalid_after_from_cbor_hex(NON_MINIMAL_INVALID_AFTER_CBOR);

  // Act
  char* hex = script_invalid_after_to_cbor_hex(script);

  // Assert
  EXPECT_STREQ(hex, NON_MINIMAL_INVALID_AFTER_CBOR);

  // Cleanup
  script_invalid_after_unref_and_free(&script, hex);
}

TEST(cardano_script_invalid_after_set_slot, discardsTheCachedCbor)
{
  // Arrange
  cardano_script_invalid_after_t* script = script_invalid_after_from_cbor_hex(NON_MINIMAL_INVALID_AFTER_CBOR);

  // Act
  EXPECT_EQ(cardano_script_invalid_after_set_slot(script, 9U), CARDANO_SUCCESS);

  char* hex = script_invalid_after_to_cbor_hex(script);

  // Assert
  EXPECT_STREQ(hex, "820509");

  // Cleanup
  script_invalid_after_unref_and_free(&script, hex);
}

TEST(cardano_script_invalid_after_clear_cbor_cache, encodesTheScriptFromItsFields)
{
  // Arrange
  cardano_script_invalid_after_t* script = script_invalid_after_from_cbor_hex(NON_MINIMAL_INVALID_AFTER_CBOR);

  // Act
  cardano_script_invalid_after_clear_cbor_cache(script);
  char* hex = script_invalid_after_to_cbor_hex(script);

  cardano_script_invalid_after_clear_cbor_cache(script);
  char* hex_after_second_clear = script_invalid_after_to_cbor_hex(script);

  // Assert
  EXPECT_STREQ(hex, MINIMAL_INVALID_AFTER_CBOR);
  EXPECT_STREQ(hex_after_second_clear, MINIMAL_INVALID_AFTER_CBOR);

  // Cleanup
  free(hex_after_second_clear);
  script_invalid_after_unref_and_free(&script, hex);
}

TEST(cardano_script_invalid_after_clear_cbor_cache, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_invalid_after_clear_cbor_cache(nullptr);
}

TEST(cardano_script_invalid_after_from_cbor, returnsErrorIfMemoryAllocationFailsWhileCachingTheCbor)
{
  // Arrange
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(NON_MINIMAL_INVALID_AFTER_CBOR, strlen(NON_MINIMAL_INVALID_AFTER_CBOR));
  bool                   succeeded = false;

  // Act
  for (int i = 0; (i < 500) && !succeeded; ++i)
  {
    cardano_cbor_reader_t* reader_copy = NULL;
    ASSERT_EQ(cardano_cbor_reader_clone(reader, &reader_copy), CARDANO_SUCCESS);

    cardano_script_invalid_after_t* script = NULL;

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_script_invalid_after_from_cbor(reader_copy, &script);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      char* hex = script_invalid_after_to_cbor_hex(script);

      EXPECT_STREQ(hex, NON_MINIMAL_INVALID_AFTER_CBOR);

      free(hex);
    }
    else
    {
      EXPECT_EQ(script, nullptr);
    }

    cardano_script_invalid_after_unref(&script);
    cardano_cbor_reader_unref(&reader_copy);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}
