/**
 * \file script_require_guard.cpp
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
 *
 * Copyright 2026 Biglup Labs
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
#include <cardano/scripts/native_scripts/script_require_guard.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* KEY_HASH_CREDENTIAL_CBOR    = "82068200581c00112233445566778899aabbccddeeff00112233445566778899aabb";
static const char* SCRIPT_HASH_CREDENTIAL_CBOR = "82068201581caabbccddeeff00112233445566778899aabbccddeeff001122334455";
static const char* NESTED_ALL_CBOR             = "8201828200581c00112233445566778899aabbccddeeff00112233445566778899aabb82068200581c00112233445566778899aabbccddeeff00112233445566778899aabb";
static const char* NESTED_ANY_CBOR             = "82028182068201581caabbccddeeff00112233445566778899aabbccddeeff001122334455";
static const char* NESTED_N_OF_K_CBOR          = "8303018182068200581c00112233445566778899aabbccddeeff00112233445566778899aabb";

static const char* REQUIRE_GUARD_SCRIPT =
  "{\n"
  "  \"type\": \"guard\",\n"
  "  \"keyHash\": \"00112233445566778899aabbccddeeff00112233445566778899aabb\"\n"
  "}";

static const char* REQUIRE_GUARD_SCRIPT2 =
  "{\n"
  "  \"type\": \"guard\",\n"
  "  \"scriptHash\": \"aabbccddeeff00112233445566778899aabbccddeeff001122334455\"\n"
  "}";

static const char* REQUIRE_GUARD_SCRIPT_SHORT_HASH =
  "{\n"
  "  \"type\": \"guard\",\n"
  "  \"keyHash\": \"00112233445566778899aabbccddeeff00112233445566778899aa\"\n"
  "}";

static const char* AFTER_SCRIPT =
  "{\n"
  "  \"type\": \"before\",\n"
  "  \"slot\": 3000\n"
  "}";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Reads a script_require_guard from a CBOR hex string.
 *
 * @param cbor_hex The CBOR hex string.
 *
 * @return The script_require_guard object.
 */
static cardano_script_require_guard_t*
create_require_guard(const char* cbor_hex)
{
  cardano_cbor_reader_t*          reader        = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_cbor(reader, &require_guard), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return require_guard;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_script_require_guard_new, returnsErrorIfRequireGuardIsNull)
{
  // Act
  cardano_error_t error = cardano_script_require_guard_new((cardano_credential_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_new, returnsErrorIfCredentialIsNull)
{
  // Act
  cardano_error_t error = cardano_script_require_guard_new(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_script_require_guard_t* require_guard = NULL;

  // Act
  cardano_error_t error = cardano_script_require_guard_new((cardano_credential_t*)"", &require_guard);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_require_guard_from_cbor, returnsErrorIfReaderIsNull)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_cbor(nullptr, &require_guard), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_from_cbor, returnsErrorIfRequireGuardIsNull)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8206", strlen("8206"));

  EXPECT_EQ(cardano_script_require_guard_from_cbor(reader, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_require_guard_from_cbor, returnsErrorIfInvalidCborNoArray)
{
  cardano_script_require_guard_t* require_guard = NULL;
  cardano_cbor_reader_t*          reader        = cardano_cbor_reader_from_hex("fe01", strlen("fe01"));

  EXPECT_EQ(cardano_script_require_guard_from_cbor(reader, &require_guard), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_require_guard_from_cbor, returnsErrorIfInvalidCborNoInt)
{
  cardano_script_require_guard_t* require_guard = NULL;
  cardano_cbor_reader_t*          reader        = cardano_cbor_reader_from_hex("82fe", strlen("82fe"));

  EXPECT_EQ(cardano_script_require_guard_from_cbor(reader, &require_guard), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_require_guard_from_cbor, returnsErrorIfWrongTypeTag)
{
  cardano_script_require_guard_t* require_guard = NULL;
  cardano_cbor_reader_t*          reader        = cardano_cbor_reader_from_hex("8200581c00112233445566778899aabbccddeeff00112233445566778899aabb", strlen("8200581c00112233445566778899aabbccddeeff00112233445566778899aabb"));

  EXPECT_EQ(cardano_script_require_guard_from_cbor(reader, &require_guard), CARDANO_ERROR_INVALID_CBOR_VALUE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_require_guard_from_cbor, returnsErrorIfInvalidCredential)
{
  cardano_script_require_guard_t* require_guard = NULL;
  cardano_cbor_reader_t*          reader        = cardano_cbor_reader_from_hex("8206fe", strlen("8206fe"));

  EXPECT_EQ(cardano_script_require_guard_from_cbor(reader, &require_guard), CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_require_guard_from_cbor, canRoundTripKeyHashCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = create_require_guard(KEY_HASH_CREDENTIAL_CBOR);
  cardano_cbor_writer_t*          writer        = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_script_require_guard_to_cbor(require_guard, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(cbor, KEY_HASH_CREDENTIAL_CBOR);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_script_require_guard_from_cbor, canRoundTripScriptHashCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = create_require_guard(SCRIPT_HASH_CREDENTIAL_CBOR);
  cardano_cbor_writer_t*          writer        = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_script_require_guard_to_cbor(require_guard, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(cbor, SCRIPT_HASH_CREDENTIAL_CBOR);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_script_require_guard_to_cbor, returnsErrorIfRequireGuardIsNull)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_script_require_guard_to_cbor(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_require_guard_to_cbor, returnsErrorIfWriterIsNull)
{
  cardano_script_require_guard_t* require_guard = (cardano_script_require_guard_t*)"";

  EXPECT_EQ(cardano_script_require_guard_to_cbor(require_guard, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfJsonIsNull)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_json(nullptr, 0, &require_guard), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfRequireGuardIsNull)
{
  EXPECT_EQ(cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_script_require_guard_t* require_guard = NULL;

  // Act
  EXPECT_EQ(cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard), CARDANO_ERROR_INVALID_JSON);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfJsonStringIsInvalid)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_json("{\"key\": \"value\"}", strlen("{\"key\": \"value\"}"), &require_guard), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfJsonStringIsInvalid2)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_json("}", strlen("}"), &require_guard), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfTypeIsInvalid)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_json("{\"type\": \"value\",\"slot\": 0 }", strlen("{\"type\": \"value\",\"slot\": 0 }"), &require_guard), CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfNoCredentialField)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_json("{\"type\": \"guard\"}", strlen("{\"type\": \"guard\"}"), &require_guard), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfHashIsTooShort)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT_SHORT_HASH, strlen(REQUIRE_GUARD_SCRIPT_SHORT_HASH), &require_guard), CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
}

TEST(cardano_script_require_guard_from_json, returnsErrorIfSizeIsZero)
{
  cardano_script_require_guard_t* require_guard = NULL;

  EXPECT_EQ(cardano_script_require_guard_from_json("{\"type\": \"value\"}", 0, &require_guard), CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_script_require_guard_from_json, canDecodeKeyHashCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_script_require_guard_to_cbor(require_guard, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(cbor, KEY_HASH_CREDENTIAL_CBOR);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_script_require_guard_from_json, canDecodeScriptHashCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT2, strlen(REQUIRE_GUARD_SCRIPT2), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  EXPECT_EQ(cardano_script_require_guard_to_cbor(require_guard, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(cbor, SCRIPT_HASH_CREDENTIAL_CBOR);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_script_require_guard_to_cip116_json, canSerializeKeyHashCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;
  cardano_json_writer_t*          json          = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_error_t                 error         = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_require_guard_to_cip116_json(require_guard, json);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t json_size = cardano_json_writer_get_encoded_size(json);
  char*  json_str  = (char*)malloc(json_size);

  ASSERT_EQ(cardano_json_writer_encode(json, json_str, json_size), CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, "{\"tag\":\"require_guard\",\"credential\":{\"tag\":\"pubkey_hash\",\"value\":\"00112233445566778899aabbccddeeff00112233445566778899aabb\"}}");

  // Cleanup
  free(json_str);
  cardano_json_writer_unref(&json);
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_to_cip116_json, canSerializeScriptHashCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;
  cardano_json_writer_t*          json          = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_error_t                 error         = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT2, strlen(REQUIRE_GUARD_SCRIPT2), &require_guard);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_require_guard_to_cip116_json(require_guard, json);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t json_size = cardano_json_writer_get_encoded_size(json);
  char*  json_str  = (char*)malloc(json_size);

  ASSERT_EQ(cardano_json_writer_encode(json, json_str, json_size), CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, "{\"tag\":\"require_guard\",\"credential\":{\"tag\":\"script_hash\",\"value\":\"aabbccddeeff00112233445566778899aabbccddeeff001122334455\"}}");

  // Cleanup
  free(json_str);
  cardano_json_writer_unref(&json);
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_to_cip116_json, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error = cardano_script_require_guard_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_script_require_guard_equals, returnsFalseIfRequireGuardIsNull)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_require_guard_equals(nullptr, require_guard);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_equals, returnsFalseIfRequireGuardIsNull2)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_require_guard_equals(require_guard, nullptr);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_equals, returnsTrueIfBothAreTheSame)
{
  // Arrange
  cardano_script_require_guard_t* require_guard1 = NULL;
  cardano_script_require_guard_t* require_guard2 = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_require_guard_equals(require_guard1, require_guard2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard1);
  cardano_script_require_guard_unref(&require_guard2);
}

TEST(cardano_script_require_guard_equals, returnsFalseIfBothAreDifferent)
{
  // Arrange
  cardano_script_require_guard_t* require_guard1 = NULL;
  cardano_script_require_guard_t* require_guard2 = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT2, strlen(REQUIRE_GUARD_SCRIPT2), &require_guard2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_require_guard_equals(require_guard1, require_guard2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard1);
  cardano_script_require_guard_unref(&require_guard2);
}

TEST(cardano_script_require_guard_equals, returnsFalseIfOneIsNull)
{
  // Arrange
  cardano_script_require_guard_t* require_guard1 = NULL;
  cardano_script_require_guard_t* require_guard2 = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_require_guard_equals(require_guard1, require_guard2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard1);
}

TEST(cardano_script_require_guard_equals, returnsTrueIfBothAreNull)
{
  // Act
  bool result = cardano_script_require_guard_equals(nullptr, nullptr);

  // Assert
  ASSERT_TRUE(result);
}

TEST(cardano_script_require_guard_equals, returnsFalseIfNotTheSameType)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;
  cardano_script_invalid_after_t* after         = NULL;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_script_require_guard_equals(require_guard, (cardano_script_require_guard_t*)after);
  ASSERT_FALSE(result);

  result = cardano_script_require_guard_equals((cardano_script_require_guard_t*)after, require_guard);
  ASSERT_FALSE(result);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_script_invalid_after_unref(&after);
}

TEST(cardano_script_require_guard_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;
  cardano_error_t                 error         = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_require_guard_ref(require_guard);

  // Assert
  EXPECT_THAT(require_guard, testing::Not((cardano_script_require_guard_t*)nullptr));
  EXPECT_EQ(cardano_script_require_guard_refcount(require_guard), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_script_require_guard_unref(&require_guard);
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_require_guard_ref(nullptr);
}

TEST(cardano_script_require_guard_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;

  // Act
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_require_guard_unref((cardano_script_require_guard_t**)nullptr);
}

TEST(cardano_script_require_guard_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;
  cardano_error_t                 error         = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_require_guard_ref(require_guard);
  size_t ref_count = cardano_script_require_guard_refcount(require_guard);

  cardano_script_require_guard_unref(&require_guard);
  size_t updated_ref_count = cardano_script_require_guard_refcount(require_guard);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;
  cardano_error_t                 error         = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_require_guard_ref(require_guard);
  size_t ref_count = cardano_script_require_guard_refcount(require_guard);

  cardano_script_require_guard_unref(&require_guard);
  size_t updated_ref_count = cardano_script_require_guard_refcount(require_guard);

  cardano_script_require_guard_unref(&require_guard);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(require_guard, (cardano_script_require_guard_t*)nullptr);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_script_require_guard_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_script_require_guard_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;
  const char*                     message       = "This is a test message";

  // Act
  cardano_script_require_guard_set_last_error(require_guard, message);

  // Assert
  EXPECT_STREQ(cardano_script_require_guard_get_last_error(require_guard), "Object is NULL.");
}

TEST(cardano_script_require_guard_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;
  cardano_error_t                 error         = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_script_require_guard_set_last_error(require_guard, message);

  // Assert
  EXPECT_STREQ(cardano_script_require_guard_get_last_error(require_guard), "");

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_get_credential, returnsErrorIfRequireGuardIsNull)
{
  // Act
  cardano_error_t error = cardano_script_require_guard_get_credential(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_get_credential, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_require_guard_get_credential(require_guard, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_get_credential, returnsTheCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;
  cardano_credential_t*           credential    = nullptr;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_require_guard_get_credential(require_guard, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(credential), "00112233445566778899aabbccddeeff00112233445566778899aabb");

  cardano_credential_type_t type;
  EXPECT_EQ(cardano_credential_get_type(credential, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_credential_unref(&credential);
}

TEST(cardano_script_require_guard_set_credential, returnsErrorIfRequireGuardIsNull)
{
  // Act
  cardano_error_t error = cardano_script_require_guard_set_credential(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_require_guard_set_credential, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = nullptr;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_require_guard_set_credential(require_guard, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
}

TEST(cardano_script_require_guard_set_credential, setsTheCredential)
{
  // Arrange
  cardano_script_require_guard_t* require_guard  = nullptr;
  cardano_credential_t*           new_credential = nullptr;
  cardano_credential_t*           retrieved      = nullptr;

  cardano_error_t error = cardano_script_require_guard_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &require_guard);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_credential_from_hash_hex("aabbccddeeff00112233445566778899aabbccddeeff001122334455", strlen("aabbccddeeff00112233445566778899aabbccddeeff001122334455"), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &new_credential);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_require_guard_set_credential(require_guard, new_credential);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_script_require_guard_get_credential(require_guard, &retrieved);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(cardano_credential_get_hash_hex(retrieved), "aabbccddeeff00112233445566778899aabbccddeeff001122334455");

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_credential_unref(&new_credential);
  cardano_credential_unref(&retrieved);
}

TEST(cardano_native_script_from_cbor, canDecodeRequireGuardKeyHash)
{
  // Arrange
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(KEY_HASH_CREDENTIAL_CBOR, strlen(KEY_HASH_CREDENTIAL_CBOR));
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_native_script_type_t type;
  EXPECT_EQ(cardano_native_script_get_type(native_script, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_GUARD);

  cardano_script_require_guard_t* require_guard = NULL;
  EXPECT_EQ(cardano_native_script_to_require_guard(native_script, &require_guard), CARDANO_SUCCESS);
  EXPECT_NE(require_guard, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, KEY_HASH_CREDENTIAL_CBOR);

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_script_require_guard_unref(&require_guard);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_native_script_from_cbor, canDecodeRequireGuardScriptHash)
{
  // Arrange
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(SCRIPT_HASH_CREDENTIAL_CBOR, strlen(SCRIPT_HASH_CREDENTIAL_CBOR));
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, SCRIPT_HASH_CREDENTIAL_CBOR);

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_native_script_from_cbor, canDecodeRequireGuardNestedInAll)
{
  // Arrange
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(NESTED_ALL_CBOR, strlen(NESTED_ALL_CBOR));
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "5cd04395d6c284857a0dae9f29cea4402a8be4a1b3ab8295865391f5");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, NESTED_ALL_CBOR);

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_cbor, canDecodeRequireGuardNestedInAny)
{
  // Arrange
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(NESTED_ANY_CBOR, strlen(NESTED_ANY_CBOR));
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, NESTED_ANY_CBOR);

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_native_script_from_cbor, canDecodeRequireGuardNestedInNOfK)
{
  // Arrange
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(NESTED_N_OF_K_CBOR, strlen(NESTED_N_OF_K_CBOR));
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, NESTED_N_OF_K_CBOR);

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodeRequireGuardScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "285496cf5e64a2505c944dc707d5804c7422bfd936de7c98ee282534");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, KEY_HASH_CREDENTIAL_CBOR);

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodeRequireGuardScriptHashScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT2, strlen(REQUIRE_GUARD_SCRIPT2), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "baf8f17908592241efd1f36ec3a12fd5e8dc907c83da9d9400210aee");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_equals, returnsTrueIfBothRequireGuardsAreTheSame)
{
  // Arrange
  cardano_native_script_t* native_script1 = NULL;
  cardano_native_script_t* native_script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &native_script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &native_script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(native_script1, native_script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&native_script1);
  cardano_native_script_unref(&native_script2);
}

TEST(cardano_native_script_equals, returnsFalseIfRequireGuardsAreDifferent)
{
  // Arrange
  cardano_native_script_t* native_script1 = NULL;
  cardano_native_script_t* native_script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &native_script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT2, strlen(REQUIRE_GUARD_SCRIPT2), &native_script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(native_script1, native_script2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_unref(&native_script1);
  cardano_native_script_unref(&native_script2);
}

TEST(cardano_native_script_new_require_guard, returnsErrorIfRequireGuardIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_require_guard(nullptr, &native_script);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_require_guard, returnsErrorIfNativeScriptIsNull)
{
  // Act
  cardano_error_t error = cardano_native_script_new_require_guard((cardano_script_require_guard_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_require_guard, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_require_guard((cardano_script_require_guard_t*)"", &native_script);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_new_require_guard, canCreateNativeScriptFromRequireGuard)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = create_require_guard(KEY_HASH_CREDENTIAL_CBOR);
  cardano_native_script_t*        native_script = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_require_guard(require_guard, &native_script);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_NE(native_script, nullptr);

  cardano_native_script_type_t type;
  EXPECT_EQ(cardano_native_script_get_type(native_script, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_GUARD);

  // Cleanup
  cardano_script_require_guard_unref(&require_guard);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_to_require_guard, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_require_guard_t* require_guard = NULL;

  // Act
  cardano_error_t error = cardano_native_script_to_require_guard(nullptr, &require_guard);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_native_script_to_require_guard, returnsErrorIfRequireGuardIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &native_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_native_script_to_require_guard(native_script, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_to_require_guard, returnsErrorIfNativeScriptIsNotRequireGuard)
{
  // Arrange
  cardano_native_script_t*        native_script = NULL;
  cardano_script_require_guard_t* require_guard = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &native_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_native_script_to_require_guard(native_script, &require_guard);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE);

  // Cleanup
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_to_cip116_json, canSerializeRequireGuardScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_json_writer_t*   json          = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  cardano_error_t error = cardano_native_script_from_json(REQUIRE_GUARD_SCRIPT, strlen(REQUIRE_GUARD_SCRIPT), &native_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_native_script_to_cip116_json(native_script, json);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t json_size = cardano_json_writer_get_encoded_size(json);
  char*  json_str  = (char*)malloc(json_size);

  // Assert
  ASSERT_EQ(cardano_json_writer_encode(json, json_str, json_size), CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, "{\"tag\":\"require_guard\",\"credential\":{\"tag\":\"pubkey_hash\",\"value\":\"00112233445566778899aabbccddeeff00112233445566778899aabb\"}}");

  // Cleanup
  free(json_str);
  cardano_json_writer_unref(&json);
  cardano_native_script_unref(&native_script);
}
