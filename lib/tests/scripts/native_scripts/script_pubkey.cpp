/**
 * \file script_pubkey.cpp
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
#include <cardano/scripts/native_scripts/script_invalid_after.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PUBKEY_SCRIPT =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* PUBKEY_SCRIPT2 =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"666e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* PUBKEY_SCRIPT_SHORT_HASH =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"666e394a544f242081e41d1965137b1bb412ac230d40ed5407821c\"\n"
  "}";

static const char* AFTER_SCRIPT =
  "{\n"
  "  \"type\": \"after\",\n"
  "  \"slot\": 3000\n"
  "}";

/* UNIT TESTS ****************************************************************/

TEST(cardano_script_pubkey_new, returnsErrorIfPubKeyIsNull)
{
  // Act
  cardano_error_t error = cardano_script_pubkey_new((cardano_blake2b_hash_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_new, returnsErrorIfKeyHashIsNull)
{
  // Act
  cardano_error_t error = cardano_script_pubkey_new(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_from_cbor, returnsErrorIfReaderIsNull)
{
  cardano_script_pubkey_t* pubkey = NULL;

  EXPECT_EQ(cardano_script_pubkey_from_cbor(nullptr, &pubkey), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_from_cbor, returnsErrorIfPubKeyIsNull)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  EXPECT_EQ(cardano_script_pubkey_from_cbor(reader, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_pubkey_from_cbor, returnsErrorIfInvalidCborNoArray)
{
  cardano_script_pubkey_t* pubkey = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("fe01", strlen("fe01"));

  EXPECT_EQ(cardano_script_pubkey_from_cbor(reader, &pubkey), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_pubkey_from_cbor, returnsErrorIfInvalidCborNoInt)
{
  cardano_script_pubkey_t* pubkey = NULL;
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex("82fe", strlen("82fe"));

  EXPECT_EQ(cardano_script_pubkey_from_cbor(reader, &pubkey), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_pubkey_to_cbor, returnsErrorIfPubKeyIsNull)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_script_pubkey_to_cbor(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_pubkey_to_cbor, returnsErrorIfWriterIsNull)
{
  cardano_script_pubkey_t* pubkey = (cardano_script_pubkey_t*)"";

  EXPECT_EQ(cardano_script_pubkey_to_cbor(pubkey, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_from_json, returnsErrorIfJsonIsNull)
{
  cardano_script_pubkey_t* pubkey = NULL;

  EXPECT_EQ(cardano_script_pubkey_from_json(nullptr, 0, &pubkey), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_from_json, returnsErrorIfPubKeyIsNull)
{
  EXPECT_EQ(cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_from_json, returnsErrorIfMemoryPubKeyocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_script_pubkey_t* pubkey = NULL;

  // Act
  EXPECT_EQ(cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_pubkey_from_json, returnsErrorIfJsonStringIsInvalid)
{
  cardano_script_pubkey_t* pubkey = NULL;

  EXPECT_EQ(cardano_script_pubkey_from_json("{\"key\": \"value\"}", strlen("{\"key\": \"value\"}"), &pubkey), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_pubkey_from_json, returnsErrorIfJsonStringIsInvalid2)
{
  cardano_script_pubkey_t* pubkey = NULL;

  EXPECT_EQ(cardano_script_pubkey_from_json("}", strlen("}"), &pubkey), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_pubkey_from_json, returnsErrorIfTypeIsInvalid)
{
  cardano_script_pubkey_t* pubkey = NULL;

  EXPECT_EQ(cardano_script_pubkey_from_json("{\"type\": \"value\",\"slot\": 0 }", strlen("{\"type\": \"value\",\"slot\": 0 }"), &pubkey), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_pubkey_from_json, returnsErrorIfSizeIsZero)
{
  cardano_script_pubkey_t* pubkey = NULL;

  EXPECT_EQ(cardano_script_pubkey_from_json("{\"type\": \"value\"}", 0, &pubkey), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_pubkey_equals, returnsFalseIfPubKeyIsNull)
{
  // Arrange
  cardano_script_pubkey_t* pubkey = NULL;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_pubkey_equals(nullptr, pubkey);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_pubkey_equals, returnsFalseIfPubKeyIsNull2)
{
  // Arrange
  cardano_script_pubkey_t* pubkey = NULL;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_pubkey_equals(pubkey, nullptr);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_pubkey_equals, returnsTrueIfBothAreTheSame)
{
  // Arrange
  cardano_script_pubkey_t* pubkey1 = NULL;
  cardano_script_pubkey_t* pubkey2 = NULL;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_pubkey_equals(pubkey1, pubkey2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey1);
  cardano_script_pubkey_unref(&pubkey2);
}

TEST(cardano_script_pubkey_equals, returnsFalseIfBothAreDifferent)
{
  // Arrange
  cardano_script_pubkey_t* pubkey1 = NULL;
  cardano_script_pubkey_t* pubkey2 = NULL;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT2, strlen(PUBKEY_SCRIPT2), &pubkey2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_pubkey_equals(pubkey1, pubkey2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey1);
  cardano_script_pubkey_unref(&pubkey2);
}

TEST(cardano_script_pubkey_equals, returnsFalseIfHashSizeIsDifferent)
{
  // Arrange
  cardano_script_pubkey_t* pubkey1 = NULL;
  cardano_script_pubkey_t* pubkey2 = NULL;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT2, strlen(PUBKEY_SCRIPT2), &pubkey1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT_SHORT_HASH, strlen(PUBKEY_SCRIPT_SHORT_HASH), &pubkey2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_pubkey_equals(pubkey1, pubkey2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey1);
  cardano_script_pubkey_unref(&pubkey2);
}

TEST(cardano_script_pubkey_equals, returnsFalseIfOneIsNull)
{
  // Arrange
  cardano_script_pubkey_t* pubkey1 = NULL;
  cardano_script_pubkey_t* pubkey2 = NULL;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_pubkey_equals(pubkey1, pubkey2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey1);
}

TEST(cardano_script_pubkey_equals, returnsTrueIfBothAreNull)
{
  // Act
  bool result = cardano_script_pubkey_equals(nullptr, nullptr);

  // Assert
  ASSERT_TRUE(result);
}

TEST(cardano_script_pubkey_equals, returnsFalseIfNotTheSameType)
{
  // Arrange
  cardano_script_pubkey_t*        pubkey = NULL;
  cardano_script_invalid_after_t* after  = NULL;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_pubkey_equals(pubkey, (cardano_script_pubkey_t*)after);
  ASSERT_FALSE(result);

  result = cardano_script_pubkey_equals((cardano_script_pubkey_t*)after, pubkey);
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey);
  cardano_script_invalid_after_unref(&after);
}

TEST(cardano_script_pubkey_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_script_pubkey_t* script_all = nullptr;
  cardano_error_t          error      = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_pubkey_ref(script_all);

  // Assert
  EXPECT_THAT(script_all, testing::Not((cardano_script_pubkey_t*)nullptr));
  EXPECT_EQ(cardano_script_pubkey_refcount(script_all), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_script_pubkey_unref(&script_all);
  cardano_script_pubkey_unref(&script_all);
}

TEST(cardano_script_pubkey_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_pubkey_ref(nullptr);
}

TEST(cardano_script_pubkey_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_script_pubkey_t* script_all = nullptr;

  // Act
  cardano_script_pubkey_unref(&script_all);
}

TEST(cardano_script_pubkey_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_pubkey_unref((cardano_script_pubkey_t**)nullptr);
}

TEST(cardano_script_pubkey_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_script_pubkey_t* script_all = nullptr;
  cardano_error_t          error      = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_pubkey_ref(script_all);
  size_t ref_count = cardano_script_pubkey_refcount(script_all);

  cardano_script_pubkey_unref(&script_all);
  size_t updated_ref_count = cardano_script_pubkey_refcount(script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_script_pubkey_unref(&script_all);
}

TEST(cardano_script_pubkey_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_script_pubkey_t* script_all = nullptr;
  cardano_error_t          error      = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script_all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_pubkey_ref(script_all);
  size_t ref_count = cardano_script_pubkey_refcount(script_all);

  cardano_script_pubkey_unref(&script_all);
  size_t updated_ref_count = cardano_script_pubkey_refcount(script_all);

  cardano_script_pubkey_unref(&script_all);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(script_all, (cardano_script_pubkey_t*)nullptr);

  // Cleanup
  cardano_script_pubkey_unref(&script_all);
}

TEST(cardano_script_pubkey_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_script_pubkey_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_script_pubkey_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_script_pubkey_t* script_all = nullptr;
  const char*              message    = "This is a test message";

  // Act
  cardano_script_pubkey_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_pubkey_get_last_error(script_all), "Object is NULL.");
}

TEST(cardano_script_pubkey_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_script_pubkey_t* script_all = nullptr;
  cardano_error_t          error      = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script_all);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_script_pubkey_set_last_error(script_all, message);

  // Assert
  EXPECT_STREQ(cardano_script_pubkey_get_last_error(script_all), "");

  // Cleanup
  cardano_script_pubkey_unref(&script_all);
}

TEST(cardano_script_pubkey_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_script_pubkey_t* pubkey = NULL;

  // Act
  cardano_error_t error = cardano_script_pubkey_new((cardano_blake2b_hash_t*)"", &pubkey);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}
