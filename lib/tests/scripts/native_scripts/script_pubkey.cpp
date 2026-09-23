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
  "  \"type\": \"before\",\n"
  "  \"slot\": 3000\n"
  "}";

static const char* NON_MINIMAL_PUBKEY_CBOR = "820059001c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* MINIMAL_PUBKEY_CBOR     = "8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* OTHER_KEY_HASH          = "b275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Serializes a script_pubkey and returns its CBOR as a hex string.
 *
 * @param script The script_pubkey to serialize.
 *
 * @return The CBOR hex string. The caller must release it with free.
 */
static char*
script_pubkey_to_cbor_hex(const cardano_script_pubkey_t* script)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_script_pubkey_to_cbor(script, writer), CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return hex;
}

/**
 * Reads a script_pubkey from a CBOR hex string.
 *
 * @param cbor_hex The CBOR hex string.
 *
 * @return The script_pubkey object.
 */
static cardano_script_pubkey_t*
script_pubkey_from_cbor_hex(const char* cbor_hex)
{
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_script_pubkey_t* script = NULL;

  EXPECT_EQ(cardano_script_pubkey_from_cbor(reader, &script), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return script;
}

/**
 * Releases a script_pubkey and a hex string returned by script_pubkey_to_cbor_hex.
 *
 * @param script The script_pubkey to release.
 * @param hex The hex string to release.
 */
static void
script_pubkey_unref_and_free(cardano_script_pubkey_t** script, char* hex)
{
  cardano_script_pubkey_unref(script);
  free(hex);
}

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
  EXPECT_EQ(cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey), CARDANO_ERROR_INVALID_JSON);

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

TEST(cardano_script_pubkey_get_key_hash, returnsErrorIfPubKeyIsNull)
{
  // Act
  cardano_error_t error = cardano_script_pubkey_get_key_hash(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_get_key_hash, returnsErrorIfKeyHashIsNull)
{
  // Arrange
  cardano_script_pubkey_t* pubkey = nullptr;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_pubkey_get_key_hash(pubkey, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_pubkey_get_key_hash, returnsTheKey)
{
  // Arrange
  cardano_script_pubkey_t* pubkey   = nullptr;
  cardano_blake2b_hash_t*  key_hash = nullptr;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_pubkey_get_key_hash(pubkey, &key_hash);

  // key_hash convert to hex
  const size_t hex_size = cardano_blake2b_hash_get_hex_size(key_hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(key_hash, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37");

  // Cleanup
  cardano_script_pubkey_unref(&pubkey);
  free(hex);
  cardano_blake2b_hash_unref(&key_hash);
}

TEST(cardano_script_pubkey_to_cip116_json, canSerializeScriptPubkey)
{
  // Arrange
  cardano_script_pubkey_t* pubkey = NULL;
  cardano_json_writer_t*   json   = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_error_t          error  = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_to_cip116_json(pubkey, json);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t json_size = cardano_json_writer_get_encoded_size(json);
  char*  json_str  = (char*)malloc(json_size);

  ASSERT_EQ(cardano_json_writer_encode(json, json_str, json_size), CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, "{\"tag\":\"pubkey\",\"pubkey\":\"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"}");

  // Cleanup
  free(json_str);
  cardano_json_writer_unref(&json);
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_pubkey_to_cip116_json, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error = cardano_script_pubkey_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_script_pubkey_set_key_hash, returnsErrorIfPubKeyIsNull)
{
  // Act
  cardano_error_t error = cardano_script_pubkey_set_key_hash(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_pubkey_set_key_hash, returnsErrorIfKeyHashIsNull)
{
  // Arrange
  cardano_script_pubkey_t* pubkey = nullptr;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_pubkey_set_key_hash(pubkey, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_script_pubkey_set_key_hash, setsTheKeyHash)
{
  // Arrange
  cardano_script_pubkey_t* pubkey    = nullptr;
  cardano_blake2b_hash_t*  new_hash  = nullptr;
  cardano_blake2b_hash_t*  retrieved = nullptr;

  cardano_error_t error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_from_hex("666e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37", strlen("666e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37"), &new_hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_pubkey_set_key_hash(pubkey, new_hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_pubkey_get_key_hash(pubkey, &retrieved);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // retrieved convert to hex
  const size_t hex_size = cardano_blake2b_hash_get_hex_size(retrieved);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(retrieved, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, "666e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37");

  // Cleanup
  free(hex);
  cardano_script_pubkey_unref(&pubkey);
  cardano_blake2b_hash_unref(&new_hash);
  cardano_blake2b_hash_unref(&retrieved);
}

TEST(cardano_script_pubkey_from_cbor, preservesTheOriginalCborEncoding)
{
  // Arrange
  cardano_script_pubkey_t* script = script_pubkey_from_cbor_hex(NON_MINIMAL_PUBKEY_CBOR);

  // Act
  char* hex = script_pubkey_to_cbor_hex(script);

  // Assert
  EXPECT_STREQ(hex, NON_MINIMAL_PUBKEY_CBOR);

  // Cleanup
  script_pubkey_unref_and_free(&script, hex);
}

TEST(cardano_script_pubkey_set_key_hash, discardsTheCachedCbor)
{
  // Arrange
  cardano_script_pubkey_t* script = script_pubkey_from_cbor_hex(NON_MINIMAL_PUBKEY_CBOR);

  // Act
  cardano_blake2b_hash_t* key_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(OTHER_KEY_HASH, strlen(OTHER_KEY_HASH), &key_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_script_pubkey_set_key_hash(script, key_hash), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&key_hash);

  char* hex = script_pubkey_to_cbor_hex(script);

  // Assert
  EXPECT_STREQ(hex, "8200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538");

  // Cleanup
  script_pubkey_unref_and_free(&script, hex);
}

TEST(cardano_script_pubkey_clear_cbor_cache, encodesTheScriptFromItsFields)
{
  // Arrange
  cardano_script_pubkey_t* script = script_pubkey_from_cbor_hex(NON_MINIMAL_PUBKEY_CBOR);

  // Act
  cardano_script_pubkey_clear_cbor_cache(script);
  char* hex = script_pubkey_to_cbor_hex(script);

  cardano_script_pubkey_clear_cbor_cache(script);
  char* hex_after_second_clear = script_pubkey_to_cbor_hex(script);

  // Assert
  EXPECT_STREQ(hex, MINIMAL_PUBKEY_CBOR);
  EXPECT_STREQ(hex_after_second_clear, MINIMAL_PUBKEY_CBOR);

  // Cleanup
  free(hex_after_second_clear);
  script_pubkey_unref_and_free(&script, hex);
}

TEST(cardano_script_pubkey_clear_cbor_cache, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_pubkey_clear_cbor_cache(nullptr);
}

TEST(cardano_script_pubkey_from_cbor, returnsErrorIfMemoryAllocationFailsWhileCachingTheCbor)
{
  // Arrange
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(NON_MINIMAL_PUBKEY_CBOR, strlen(NON_MINIMAL_PUBKEY_CBOR));
  bool                   succeeded = false;

  // Act
  for (int i = 0; (i < 500) && !succeeded; ++i)
  {
    cardano_cbor_reader_t* reader_copy = NULL;
    ASSERT_EQ(cardano_cbor_reader_clone(reader, &reader_copy), CARDANO_SUCCESS);

    cardano_script_pubkey_t* script = NULL;

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_script_pubkey_from_cbor(reader_copy, &script);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      char* hex = script_pubkey_to_cbor_hex(script);

      EXPECT_STREQ(hex, NON_MINIMAL_PUBKEY_CBOR);

      free(hex);
    }
    else
    {
      EXPECT_EQ(script, nullptr);
    }

    cardano_script_pubkey_unref(&script);
    cardano_cbor_reader_unref(&reader_copy);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}
