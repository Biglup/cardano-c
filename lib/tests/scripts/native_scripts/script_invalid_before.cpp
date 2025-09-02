/**
 * \file script_invalid_before.cpp
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
#include <cardano/scripts/native_scripts/script_invalid_before.h>
#include <cardano/scripts/native_scripts/script_n_of_k.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PUBKEY_SCRIPT =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* BEFORE_SCRIPT =
  "{\n"
  "  \"type\": \"after\",\n"
  "  \"slot\": 3000\n"
  "}";

static const char* BEFORE_SCRIPT2 =
  "{\n"
  "  \"type\": \"after\",\n"
  "  \"slot\": 4000\n"
  "}";

/* UNIT TESTS ****************************************************************/

TEST(cardano_script_invalid_before_new, returnsErrorIfInvalidAfterIsNull)
{
  EXPECT_EQ(cardano_script_invalid_before_new(0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_before_new, returnsErrorIfMemoryInvalidAfterocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_script_invalid_before_t* invalid_before = NULL;
  EXPECT_EQ(cardano_script_invalid_before_new(0, &invalid_before), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_invalid_before_from_cbor, returnsErrorIfReaderIsNull)
{
  cardano_script_invalid_before_t* invalid_before = NULL;

  EXPECT_EQ(cardano_script_invalid_before_from_cbor(nullptr, &invalid_before), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_before_from_cbor, returnsErrorIfInvalidAfterIsNull)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  EXPECT_EQ(cardano_script_invalid_before_from_cbor(reader, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_invalid_before_from_cbor, returnsErrorIfInvalidCborNoArray)
{
  cardano_script_invalid_before_t* invalid_before = NULL;
  cardano_cbor_reader_t*           reader         = cardano_cbor_reader_from_hex("fe01", strlen("fe01"));

  EXPECT_EQ(cardano_script_invalid_before_from_cbor(reader, &invalid_before), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_invalid_before_from_cbor, returnsErrorIfInvalidCborNoInt)
{
  cardano_script_invalid_before_t* invalid_before = NULL;
  cardano_cbor_reader_t*           reader         = cardano_cbor_reader_from_hex("82fe", strlen("82fe"));

  EXPECT_EQ(cardano_script_invalid_before_from_cbor(reader, &invalid_before), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_invalid_before_to_cbor, returnsErrorIfInvalidAfterIsNull)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_script_invalid_before_to_cbor(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_invalid_before_to_cbor, returnsErrorIfWriterIsNull)
{
  cardano_script_invalid_before_t* invalid_before = (cardano_script_invalid_before_t*)"";

  EXPECT_EQ(cardano_script_invalid_before_to_cbor(invalid_before, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_before_from_json, returnsErrorIfJsonIsNull)
{
  cardano_script_invalid_before_t* invalid_before = NULL;

  EXPECT_EQ(cardano_script_invalid_before_from_json(nullptr, 0, &invalid_before), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_before_from_json, returnsErrorIfInvalidAfterIsNull)
{
  EXPECT_EQ(cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_before_from_json, returnsErrorIfMemoryInvalidAfterocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_script_invalid_before_t* invalid_before = NULL;

  // Act
  EXPECT_EQ(cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before), CARDANO_ERROR_INVALID_JSON);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_invalid_before_from_json, returnsErrorIfJsonStringIsInvalid)
{
  cardano_script_invalid_before_t* invalid_before = NULL;

  EXPECT_EQ(cardano_script_invalid_before_from_json("{\"key\": \"value\"}", strlen("{\"key\": \"value\"}"), &invalid_before), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_invalid_before_from_json, returnsErrorIfJsonStringIsInvalid2)
{
  cardano_script_invalid_before_t* invalid_before = NULL;

  EXPECT_EQ(cardano_script_invalid_before_from_json("}", strlen("}"), &invalid_before), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_invalid_before_from_json, returnsErrorIfSizeIsZero)
{
  cardano_script_invalid_before_t* invalid_before = NULL;

  EXPECT_EQ(cardano_script_invalid_before_from_json("{\"type\": \"value\"}", 0, &invalid_before), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_invalid_before_equals, returnsFalseIfInvalidAfterIsNull)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before = NULL;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_before_equals(nullptr, invalid_before);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before);
}

TEST(cardano_script_invalid_before_equals, returnsFalseIfInvalidAfterIsNull2)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before = NULL;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_before_equals(invalid_before, nullptr);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before);
}

TEST(cardano_script_invalid_before_equals, returnsTrueIfBothAreTheSame)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before1 = NULL;
  cardano_script_invalid_before_t* invalid_before2 = NULL;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_before_equals(invalid_before1, invalid_before2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before1);
  cardano_script_invalid_before_unref(&invalid_before2);
}

TEST(cardano_script_invalid_before_equals, returnsFalseIfBothAreDifferent)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before1 = NULL;
  cardano_script_invalid_before_t* invalid_before2 = NULL;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT2, strlen(BEFORE_SCRIPT2), &invalid_before2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_before_equals(invalid_before1, invalid_before2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before1);
  cardano_script_invalid_before_unref(&invalid_before2);
}

TEST(cardano_script_invalid_before_equals, returnsFalseIfOneIsNull)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before1 = NULL;
  cardano_script_invalid_before_t* invalid_before2 = NULL;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_before_equals(invalid_before1, invalid_before2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before1);
}

TEST(cardano_script_invalid_before_equals, returnsTrueIfBothAreNull)
{
  // Act
  bool result = cardano_script_invalid_before_equals(nullptr, nullptr);

  // Assert
  ASSERT_TRUE(result);
}

TEST(cardano_script_invalid_before_equals, returnsFalseIfNotTheSameType)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before = NULL;
  cardano_script_pubkey_t*         pubkey         = NULL;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_invalid_before_equals(invalid_before, (cardano_script_invalid_before_t*)pubkey);
  ASSERT_FALSE(result);

  result = cardano_script_invalid_before_equals((cardano_script_invalid_before_t*)pubkey, invalid_before);
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before);
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_invalid_before_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_script_invalid_before_t* script_all = nullptr;
  cardano_error_t                  error      = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_invalid_before_ref(script_all);

  // Assert
  EXPECT_THAT(script_all, testing::Not((cardano_script_invalid_before_t*)nullptr));
  EXPECT_EQ(cardano_script_invalid_before_refcount(script_all), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_script_invalid_before_unref(&script_all);
  cardano_script_invalid_before_unref(&script_all);
}

TEST(cardano_script_invalid_before_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_invalid_before_ref(nullptr);
}

TEST(cardano_script_invalid_before_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_script_invalid_before_t* script_all = nullptr;

  // Act
  cardano_script_invalid_before_unref(&script_all);
}

TEST(cardano_script_invalid_before_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_invalid_before_unref((cardano_script_invalid_before_t**)nullptr);
}

TEST(cardano_script_invalid_before_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_script_invalid_before_t* script_all = nullptr;
  cardano_error_t                  error      = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_invalid_before_ref(script_all);
  size_t ref_count = cardano_script_invalid_before_refcount(script_all);

  cardano_script_invalid_before_unref(&script_all);
  size_t updated_ref_count = cardano_script_invalid_before_refcount(script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_script_invalid_before_unref(&script_all);
}

TEST(cardano_script_invalid_before_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_script_invalid_before_t* script_all = nullptr;
  cardano_error_t                  error      = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_invalid_before_ref(script_all);
  size_t ref_count = cardano_script_invalid_before_refcount(script_all);

  cardano_script_invalid_before_unref(&script_all);
  size_t updated_ref_count = cardano_script_invalid_before_refcount(script_all);

  cardano_script_invalid_before_unref(&script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(script_all, (cardano_script_invalid_before_t*)nullptr);

  // Cleanup
  cardano_script_invalid_before_unref(&script_all);
}

TEST(cardano_script_invalid_before_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_script_invalid_before_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_script_invalid_before_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_script_invalid_before_t* script_all = nullptr;
  const char*                      message    = "This is a test message";

  // Act
  cardano_script_invalid_before_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_invalid_before_get_last_error(script_all), "Object is NULL.");
}

TEST(cardano_script_invalid_before_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_script_invalid_before_t* script_all = nullptr;
  cardano_error_t                  error      = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script_all);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_script_invalid_before_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_invalid_before_get_last_error(script_all), "");

  // Cleanup
  cardano_script_invalid_before_unref(&script_all);
}

TEST(cardano_script_invalid_before_get_slot, returnsZeroIfInvalidAfterIsNull)
{
  // Act
  uint64_t slot = 0;

  EXPECT_EQ(cardano_script_invalid_before_get_slot(nullptr, &slot), CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(slot, 0);
}

TEST(cardano_script_invalid_before_get_slot, returnsErrorIfSlotIsNull)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before = nullptr;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_script_invalid_before_get_slot(invalid_before, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before);
}

TEST(cardano_script_invalid_before_get_slot, returnsSlot)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before = nullptr;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t slot = 0;
  EXPECT_EQ(cardano_script_invalid_before_get_slot(invalid_before, &slot), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(slot, 3000);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before);
}

TEST(cardano_script_invalid_before_set_slot, returnsErrorIfInvalidAfterIsNull)
{
  // Act
  EXPECT_EQ(cardano_script_invalid_before_set_slot(nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_invalid_before_set_slot, setsSlot)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before = nullptr;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_script_invalid_before_set_slot(invalid_before, 4000), CARDANO_SUCCESS);

  // Assert
  uint64_t slot = 0;
  EXPECT_EQ(cardano_script_invalid_before_get_slot(invalid_before, &slot), CARDANO_SUCCESS);
  EXPECT_EQ(slot, 4000);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before);
}

TEST(cardano_script_invalid_before_to_cip116_json, canSerializeInvalidAfter)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before = nullptr;

  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  ASSERT_THAT(writer, testing::Not((cardano_json_writer_t*)nullptr));

  // Act
  error = cardano_script_invalid_before_to_cip116_json(invalid_before, writer);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  char   buffer[256] = { 0 };
  size_t size        = sizeof(buffer);

  error = cardano_json_writer_encode(writer, buffer, size);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char* expected_json =
    "{\n"
    "  \"tag\": \"timelock_start\",\n"
    "  \"slot\": \"3000\"\n"
    "}";

  EXPECT_STREQ((const char*)buffer, expected_json);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before);
  cardano_json_writer_unref(&writer);
}

TEST(cardano_script_invalid_before_to_cip116_json, returnsErrorIfInvalidAfterIsNull)
{
  // Arrange
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);
  ASSERT_THAT(writer, testing::Not((cardano_json_writer_t*)nullptr));

  // Act
  cardano_error_t error = cardano_script_invalid_before_to_cip116_json(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&writer);
}
