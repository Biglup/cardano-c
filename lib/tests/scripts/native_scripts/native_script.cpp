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

static const char* NESTED_NATIVE_SCRIPT =
  "{\n"
  "  \"type\": \"any\",\n"
  "  \"scripts\":\n"
  "  [\n"
  "    {\n"
  "      \"type\": \"sig\",\n"
  "      \"keyHash\": \"b275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538\"\n"
  "    },\n"
  "    {\n"
  "      \"type\": \"all\",\n"
  "      \"scripts\":\n"
  "      [\n"
  "        {\n"
  "          \"type\": \"after\",\n"
  "          \"slot\": 3000\n"
  "        },\n"
  "        {\n"
  "          \"type\": \"sig\",\n"
  "          \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "        },\n"
  "        {\n"
  "          \"type\": \"before\",\n"
  "          \"slot\": 4000\n"
  "        },\n"
  "      ]\n"
  "    }\n"
  "  ]\n"
  "}";

static const char* PUBKEY_SCRIPT =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* BEFORE_SCRIPT =
  "{\n"
  "  \"type\": \"before\",\n"
  "  \"slot\": 40000010\n"
  "}";

static const char* BEFORE_SCRIPT_SMALL =
  "{\n"
  "  \"type\": \"before\",\n"
  "  \"slot\": 4000\n"
  "}";

static const char* AFTER_SCRIPT =
  "{\n"
  "  \"type\": \"after\",\n"
  "  \"slot\": 3000\n"
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

static const char* ANY_SCRIPT =
  "{\n"
  "  \"type\": \"any\",\n"
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

static const char* AT_LEAST_SCRIPT =
  "{\n"
  "  \"type\": \"atLeast\",\n"
  "  \"required\": 2,\n"
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

/* UNIT TESTS ****************************************************************/

TEST(cardano_native_script_from_json, canDecodeNestedScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(NESTED_NATIVE_SCRIPT, strlen(NESTED_NATIVE_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "8b8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor, "8202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodePubKeyScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "44e8537337e941f125478607b7ab91515b5eca4ef647b10c16c63ed2");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor, "8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodeBeforeScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "bdda6da5dcca0c3dcb5a1000b23febf79e5741f3f1872b8aadaf92f6");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor      = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor, "82041a02625a0a");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodeAfterScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "e638e31a6c57bde95c0b644ec0c584a239fab33ba99f41c91b410d1d");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);

  char* cbor = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor, "8205190bb8");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodeAllScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "5ea7df92c0b5c88f60061d04140aee2b69414bafe04fbe19144bb693");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);

  char* cbor = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor, "8201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodeAnyScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(ANY_SCRIPT, strlen(ANY_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "70e5950987ed08bf51fa0138fbda822f216b0aa9dca48ae947c1e511");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);

  char* cbor = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor, "8202838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_json, canDecodeAtLeastScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(AT_LEAST_SCRIPT, strlen(AT_LEAST_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native_script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "a1fe3a12ce7c1d7e8c0621d97970cf3092f5c1f7677adc954a96c09b");

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_native_script_to_cbor(native_script, writer), CARDANO_SUCCESS);

  const size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);

  char* cbor = (char*)malloc(cbor_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor, "830302838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
  free(cbor);
}

TEST(cardano_native_script_from_cbor, canDecodeNestedScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("8202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

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
  EXPECT_STREQ(hex, "8b8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_from_cbor, canDecodePubKeyScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37", strlen("8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37"));

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
  EXPECT_STREQ(hex, "44e8537337e941f125478607b7ab91515b5eca4ef647b10c16c63ed2");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_from_cbor, canDecodeBeforeScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("82041a02625a0a", strlen("82041a02625a0a"));

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
  EXPECT_STREQ(hex, "bdda6da5dcca0c3dcb5a1000b23febf79e5741f3f1872b8aadaf92f6");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_from_cbor, canDecodeAfterScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8205190bb8", strlen("8205190bb8"));

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
  EXPECT_STREQ(hex, "e638e31a6c57bde95c0b644ec0c584a239fab33ba99f41c91b410d1d");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_from_cbor, canDecodeAllScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("8201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

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
  EXPECT_STREQ(hex, "5ea7df92c0b5c88f60061d04140aee2b69414bafe04fbe19144bb693");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_from_cbor, canDecodeAnyScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8202838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("8202838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

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
  EXPECT_STREQ(hex, "70e5950987ed08bf51fa0138fbda822f216b0aa9dca48ae947c1e511");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_from_cbor, canDecodeAtLeastScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("830302838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("830302838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

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
  EXPECT_STREQ(hex, "a1fe3a12ce7c1d7e8c0621d97970cf3092f5c1f7677adc954a96c09b");

  // Cleanup
  cardano_native_script_unref(&native_script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_native_script_t* native_script = nullptr;
  cardano_error_t          error         = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &native_script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_ref(native_script);

  // Assert
  EXPECT_THAT(native_script, testing::Not((cardano_native_script_t*)nullptr));
  EXPECT_EQ(cardano_native_script_refcount(native_script), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_native_script_unref(&native_script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_native_script_ref(nullptr);
}

TEST(cardano_native_script_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_native_script_t* native_script = nullptr;

  // Act
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_native_script_unref((cardano_native_script_t**)nullptr);
}

TEST(cardano_native_script_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_native_script_t* native_script = nullptr;
  cardano_error_t          error         = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &native_script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_ref(native_script);
  size_t ref_count = cardano_native_script_refcount(native_script);

  cardano_native_script_unref(&native_script);
  size_t updated_ref_count = cardano_native_script_refcount(native_script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_native_script_t* native_script = nullptr;
  cardano_error_t          error         = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &native_script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_ref(native_script);
  size_t ref_count = cardano_native_script_refcount(native_script);

  cardano_native_script_unref(&native_script);
  size_t updated_ref_count = cardano_native_script_refcount(native_script);

  cardano_native_script_unref(&native_script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(native_script, (cardano_native_script_t*)nullptr);

  // Cleanup
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_native_script_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_native_script_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_native_script_set_last_error(native_script, message);

  // Assert
  EXPECT_STREQ(cardano_native_script_get_last_error(native_script), "Object is NULL.");
}

TEST(cardano_native_script_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = nullptr;
  cardano_error_t          error         = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &native_script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_native_script_set_last_error(native_script, message);

  // Assert
  EXPECT_STREQ(cardano_native_script_get_last_error(native_script), "");

  // Cleanup
  cardano_native_script_unref(&native_script);
}

TEST(cardano_native_script_new_all, canCreateAllScript)
{
  // Arrange
  cardano_script_all_t*         all_script    = NULL;
  cardano_native_script_t*      after_script  = NULL;
  cardano_native_script_t*      sig_script    = NULL;
  cardano_native_script_t*      before_script = NULL;
  cardano_native_script_list_t* scripts       = NULL;
  cardano_native_script_t*      native        = NULL;

  cardano_error_t error = cardano_native_script_list_new(&scripts);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &sig_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(BEFORE_SCRIPT_SMALL, strlen(BEFORE_SCRIPT_SMALL), &before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, sig_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_all_new(scripts, &all_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_new_all(all_script, &native);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(all_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "5ea7df92c0b5c88f60061d04140aee2b69414bafe04fbe19144bb693");

  // Cleanup
  cardano_native_script_list_unref(&scripts);
  cardano_script_all_unref(&all_script);
  cardano_native_script_unref(&native);
  cardano_native_script_unref(&after_script);
  cardano_native_script_unref(&sig_script);
  cardano_native_script_unref(&before_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_new_all, returnsErrorIfScriptsAreNull)
{
  // Act
  cardano_error_t error = cardano_native_script_new_all(nullptr, nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_all, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_all_t*    all_script = NULL;
  cardano_native_script_t* native     = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_all((cardano_script_all_t*)"", nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_all_unref(&all_script);
}

TEST(cardano_native_script_new_all, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_native_script_new_all((cardano_script_all_t*)"", (cardano_native_script_t**)"");
  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_new_any, canCreateAnyScript)
{
  // Arrange
  cardano_script_any_t*         any_script    = NULL;
  cardano_native_script_t*      after_script  = NULL;
  cardano_native_script_t*      sig_script    = NULL;
  cardano_native_script_t*      before_script = NULL;
  cardano_native_script_list_t* scripts       = NULL;
  cardano_native_script_t*      native        = NULL;

  cardano_error_t error = cardano_native_script_list_new(&scripts);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &sig_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(BEFORE_SCRIPT_SMALL, strlen(BEFORE_SCRIPT_SMALL), &before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, sig_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_any_new(scripts, &any_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_new_any(any_script, &native);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(any_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "70e5950987ed08bf51fa0138fbda822f216b0aa9dca48ae947c1e511");

  // Cleanup
  cardano_native_script_list_unref(&scripts);
  cardano_script_any_unref(&any_script);
  cardano_native_script_unref(&native);
  cardano_native_script_unref(&after_script);
  cardano_native_script_unref(&sig_script);
  cardano_native_script_unref(&before_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_new_any, returnsErrorIfScriptsAreNull)
{
  // Act
  cardano_error_t error = cardano_native_script_new_any(nullptr, nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_any, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_any_t*    any_script = NULL;
  cardano_native_script_t* native     = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_any((cardano_script_any_t*)"", nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_any_unref(&any_script);
}

TEST(cardano_native_script_new_any, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_native_script_new_any((cardano_script_any_t*)"", (cardano_native_script_t**)"");
  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_new_n_of_k, canCreateAtLeastScript)
{
  // Arrange
  cardano_script_n_of_k_t*      n_of_k_script = NULL;
  cardano_native_script_t*      after_script  = NULL;
  cardano_native_script_t*      sig_script    = NULL;
  cardano_native_script_t*      before_script = NULL;
  cardano_native_script_list_t* scripts       = NULL;
  cardano_native_script_t*      native        = NULL;

  cardano_error_t error = cardano_native_script_list_new(&scripts);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &sig_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(BEFORE_SCRIPT_SMALL, strlen(BEFORE_SCRIPT_SMALL), &before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, sig_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_list_add(scripts, before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_n_of_k_new(scripts, 2, &n_of_k_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_new_n_of_k(n_of_k_script, &native);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(n_of_k_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "a1fe3a12ce7c1d7e8c0621d97970cf3092f5c1f7677adc954a96c09b");

  // Cleanup
  cardano_native_script_list_unref(&scripts);
  cardano_script_n_of_k_unref(&n_of_k_script);
  cardano_native_script_unref(&native);
  cardano_native_script_unref(&after_script);
  cardano_native_script_unref(&sig_script);
  cardano_native_script_unref(&before_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_new_n_of_k, returnsErrorIfScriptsAreNull)
{
  // Act
  cardano_error_t error = cardano_native_script_new_n_of_k(nullptr, nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_n_of_k, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_n_of_k_t* n_of_k_script = NULL;
  cardano_native_script_t* native        = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_n_of_k((cardano_script_n_of_k_t*)"", nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_n_of_k_unref(&n_of_k_script);
}

TEST(cardano_native_script_new_n_of_k, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_native_script_new_n_of_k((cardano_script_n_of_k_t*)"", (cardano_native_script_t**)"");
  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_new_pubkey, canCreatePubKeyScript)
{
  // Arrange
  cardano_script_pubkey_t* pubkey_script = NULL;
  cardano_native_script_t* sig_script    = NULL;
  cardano_native_script_t* native        = NULL;

  cardano_error_t error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &sig_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_pubkey_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &pubkey_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_new_pubkey(pubkey_script, &native);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(pubkey_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "44e8537337e941f125478607b7ab91515b5eca4ef647b10c16c63ed2");

  // Cleanup
  cardano_script_pubkey_unref(&pubkey_script);
  cardano_native_script_unref(&native);
  cardano_native_script_unref(&sig_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_new_pubkey, returnsErrorIfScriptsAreNull)
{
  // Act
  cardano_error_t error = cardano_native_script_new_pubkey(nullptr, nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_pubkey, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_pubkey_t* pubkey_script = NULL;
  cardano_native_script_t* native        = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_pubkey((cardano_script_pubkey_t*)"", nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_pubkey_unref(&pubkey_script);
}

TEST(cardano_native_script_new_pubkey, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_native_script_new_pubkey((cardano_script_pubkey_t*)"", (cardano_native_script_t**)"");
  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_new_invalid_after, canCreateAfterScript)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after_script = NULL;
  cardano_native_script_t*        after_script         = NULL;
  cardano_native_script_t*        native               = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_script_invalid_after_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &invalid_after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_new_invalid_after(invalid_after_script, &native);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(invalid_after_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "e638e31a6c57bde95c0b644ec0c584a239fab33ba99f41c91b410d1d");

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after_script);
  cardano_native_script_unref(&native);
  cardano_native_script_unref(&after_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_new_invalid_after, returnsErrorIfScriptsAreNull)
{
  // Act
  cardano_error_t error = cardano_native_script_new_invalid_after(nullptr, nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_invalid_after, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after_script = NULL;
  cardano_native_script_t*        native               = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_invalid_after((cardano_script_invalid_after_t*)"", nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_invalid_after_unref(&invalid_after_script);
}

TEST(cardano_native_script_new_invalid_after, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_native_script_new_invalid_after((cardano_script_invalid_after_t*)"", (cardano_native_script_t**)"");
  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_new_invalid_before, canCreateBeforeScript)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before_script = NULL;
  cardano_native_script_t*         before_script         = NULL;
  cardano_native_script_t*         native                = NULL;

  // Act
  cardano_error_t error = cardano_script_invalid_before_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &invalid_before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_new_invalid_before(invalid_before_script, &native);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(invalid_before_script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(native);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "bdda6da5dcca0c3dcb5a1000b23febf79e5741f3f1872b8aadaf92f6");

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before_script);
  cardano_native_script_unref(&native);
  cardano_native_script_unref(&before_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_new_invalid_before, returnsErrorIfScriptsAreNull)
{
  // Act
  cardano_error_t error = cardano_native_script_new_invalid_before(nullptr, nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_new_invalid_before, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before_script = NULL;
  cardano_native_script_t*         native                = NULL;

  // Act
  cardano_error_t error = cardano_native_script_new_invalid_before((cardano_script_invalid_before_t*)"", nullptr);
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_invalid_before_unref(&invalid_before_script);
}

TEST(cardano_native_script_new_invalid_before, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_native_script_new_invalid_before((cardano_script_invalid_before_t*)"", (cardano_native_script_t**)"");
  ASSERT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(nullptr, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82041a02625a0a", strlen("82041a02625a0a"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("82041a02625a0a", strlen("82041a02625a0a"));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_native_script_unref(&native_script);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfNotAnArray)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("fe041a02625a0a", strlen("fe041a02625a0a"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfNotAnInt)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("81fe1a02625a0a", strlen("81fe1a02625a0a"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfInvalidPubKeyScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8200", strlen("8200"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfInvalidAllScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8201", strlen("8201"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfInvalidAnyScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8202", strlen("8202"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfInvalidAtLeastScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8203", strlen("8203"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfInvalidABeforeScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8204", strlen("8204"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfInvalidAfterScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8205", strlen("8205"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_from_cbor, returnsErrorIfInvalidScriptType)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("8209", strlen("8209"));

  // Act
  cardano_error_t result = cardano_native_script_from_cbor(reader, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_to_cbor, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_native_script_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_native_script_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_native_script_to_cbor((cardano_native_script_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(nullptr, 0, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_from_json, returnsErrorIfNativeScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_from_json, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonStringIsMissingTypeField)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"key\": \"value\"}", strlen("{\"key\": \"value\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonStringPubKeyScriptIsInvalid)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"type\": \"sig\"}", strlen("{\"type\": \"sig\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonStringAllScriptIsInvalid)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"type\": \"all\"}", strlen("{\"type\": \"all\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonStringAnyScriptIsInvalid)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"type\": \"any\"}", strlen("{\"type\": \"any\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonStringAtLeastScriptIsInvalid)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"type\": \"atLeast\"}", strlen("{\"type\": \"atLeast\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonStringBeforeScriptIsInvalid)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"type\": \"before\"}", strlen("{\"type\": \"before\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_from_json, returnsErrorIfJsonStringAfterScriptIsInvalid)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"type\": \"after\"}", strlen("{\"type\": \"after\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_JSON);
}

TEST(cardano_native_script_from_json, returnsErrorIfUnknownType)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json("{\"type\": \"xxxx\"}", strlen("{\"type\": \"xxxx\"}"), &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_NATIVE_SCRIPT_TYPE);
}

TEST(cardano_native_script_get_type, returnsTheTypeOfTheScript)
{
  // Arrange
  cardano_native_script_type_t type;
  cardano_native_script_t*     after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_get_type(after_script, &type);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(type, CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_get_type, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_native_script_type_t type;

  // Act
  cardano_error_t result = cardano_native_script_get_type(nullptr, &type);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_get_type, returnsErrorIfTypeIsNull)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_get_type(after_script, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_all, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_all_t* all_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_to_all(nullptr, &all_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_to_all, returnsErrorIfAllScriptIsNull)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_all(after_script, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_all, returnErrorIfGivenWrongScript)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;
  cardano_script_all_t*    all_script   = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_all(after_script, &all_script);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_NATIVE_SCRIPT_TYPE);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_all, createsANewInstanceOfNativeScript)
{
  cardano_native_script_t* script = NULL;
  cardano_script_all_t*    all    = NULL;

  cardano_error_t error = cardano_native_script_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_to_all(script, &all);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_native_script_unref(&script);
  cardano_script_all_unref(&all);
}

TEST(cardano_native_script_to_any, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_any_t* any_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_to_any(nullptr, &any_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_to_any, returnsErrorIfAnyScriptIsNull)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_any(after_script, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_any, returnErrorIfGivenWrongScript)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;
  cardano_script_any_t*    any_script   = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_any(after_script, &any_script);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_NATIVE_SCRIPT_TYPE);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_any, createsANewInstanceOfNativeScript)
{
  cardano_native_script_t* script = NULL;
  cardano_script_any_t*    any    = NULL;

  cardano_error_t error = cardano_native_script_from_json(ANY_SCRIPT, strlen(ANY_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_to_any(script, &any);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_native_script_unref(&script);
  cardano_script_any_unref(&any);
}

TEST(cardano_native_script_to_n_of_k, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_n_of_k_t* n_of_k_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_to_n_of_k(nullptr, &n_of_k_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_to_n_of_k, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_n_of_k(after_script, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_n_of_k, returnErrorIfGivenWrongScript)
{
  // Arrange
  cardano_native_script_t* after_script  = NULL;
  cardano_script_n_of_k_t* n_of_k_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_n_of_k(after_script, &n_of_k_script);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_NATIVE_SCRIPT_TYPE);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_n_of_k, createsANewInstanceOfNativeScript)
{
  cardano_native_script_t* script = NULL;
  cardano_script_n_of_k_t* n_of_k = NULL;

  cardano_error_t error = cardano_native_script_from_json(AT_LEAST_SCRIPT, strlen(AT_LEAST_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_to_n_of_k(script, &n_of_k);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_native_script_unref(&script);
  cardano_script_n_of_k_unref(&n_of_k);
}

TEST(cardano_native_script_to_pubkey, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_pubkey_t* pubkey_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_to_pubkey(nullptr, &pubkey_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_to_pubkey, returnsErrorIfPubkeyScriptIsNull)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_pubkey(after_script, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_pubkey, returnErrorIfGivenWrongScript)
{
  // Arrange
  cardano_native_script_t* after_script  = NULL;
  cardano_script_pubkey_t* pubkey_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_pubkey(after_script, &pubkey_script);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_NATIVE_SCRIPT_TYPE);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_pubkey, createsANewInstanceOfNativeScript)
{
  cardano_native_script_t* script = NULL;
  cardano_script_pubkey_t* pubkey = NULL;

  cardano_error_t error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_to_pubkey(script, &pubkey);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_native_script_unref(&script);
  cardano_script_pubkey_unref(&pubkey);
}

TEST(cardano_native_script_to_invalid_after, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_invalid_after_t* invalid_after_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_to_invalid_after(nullptr, &invalid_after_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_to_invalid_after, returnsErrorIfInvalidAfterScriptIsNull)
{
  // Arrange
  cardano_native_script_t* after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_invalid_after(after_script, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_invalid_after, returnErrorIfGivenWrongScript)
{
  // Arrange
  cardano_native_script_t*        after_script         = NULL;
  cardano_script_invalid_after_t* invalid_after_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &after_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_invalid_after(after_script, &invalid_after_script);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_NATIVE_SCRIPT_TYPE);

  // Cleanup
  cardano_native_script_unref(&after_script);
}

TEST(cardano_native_script_to_invalid_after, createsANewInstanceOfNativeScript)
{
  cardano_native_script_t*        script        = NULL;
  cardano_script_invalid_after_t* invalid_after = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_to_invalid_after(script, &invalid_after);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_native_script_unref(&script);
  cardano_script_invalid_after_unref(&invalid_after);
}

TEST(cardano_native_script_to_invalid_before, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_invalid_before_t* invalid_before_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_to_invalid_before(nullptr, &invalid_before_script);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_to_invalid_before, returnsErrorIfInvalidBeforeScriptIsNull)
{
  // Arrange
  cardano_native_script_t* before_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_invalid_before(before_script, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&before_script);
}

TEST(cardano_native_script_to_invalid_before, returnErrorIfGivenWrongScript)
{
  // Arrange
  cardano_native_script_t*         before_script         = NULL;
  cardano_script_invalid_before_t* invalid_before_script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &before_script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_native_script_to_invalid_before(before_script, &invalid_before_script);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_NATIVE_SCRIPT_TYPE);

  // Cleanup
  cardano_native_script_unref(&before_script);
}

TEST(cardano_native_script_to_invalid_before, createsANewInstanceOfNativeScript)
{
  cardano_native_script_t*         script         = NULL;
  cardano_script_invalid_before_t* invalid_before = NULL;

  cardano_error_t error = cardano_native_script_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_to_invalid_before(script, &invalid_before);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_native_script_unref(&script);
  cardano_script_invalid_before_unref(&invalid_before);
}

TEST(cardano_native_script_equals, returnsFalseIfScriptsAreDifferent)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsTrueIfScriptsAreEqual)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsFalseIfScriptsAreDifferentTypes)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsFalseIfOneScriptIsNull)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
}

TEST(cardano_native_script_equals, returnsTrueIfBothScriptsAreNull)
{
  // Act
  bool result = cardano_native_script_equals(nullptr, nullptr);

  // Assert
  ASSERT_TRUE(result);
}

TEST(cardano_native_script_get_hash, returnErrorIfNativeScriptIsNull)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(nullptr);

  // Assert
  ASSERT_EQ(hash, nullptr);
}

TEST(cardano_native_script_get_hash, returnsTheHashOfTheScript)
{
  // Arrange
  cardano_native_script_t* script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(script);

  // Assert
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "e638e31a6c57bde95c0b644ec0c584a239fab33ba99f41c91b410d1d");

  // Cleanup
  cardano_native_script_unref(&script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_native_script_get_hash, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_native_script_t* script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(script);

  // Assert
  ASSERT_EQ(hash, nullptr);

  // Cleanup
  cardano_native_script_unref(&script);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_get_hash, returnErrorIfMemoryAllocationFails1)
{
  // Arrange
  cardano_native_script_t* script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(script);

  // Assert
  ASSERT_EQ(hash, nullptr);

  // Cleanup
  cardano_native_script_unref(&script);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_get_hash, returnErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_native_script_t* script = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_five_malloc, realloc, free);

  // Act
  cardano_blake2b_hash_t* hash = cardano_native_script_get_hash(script);

  // Assert
  ASSERT_EQ(hash, nullptr);

  // Cleanup
  cardano_native_script_unref(&script);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_equals, returnsFalseIfLshAscriptIsNull)
{
  // Arrange
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(nullptr, script2);

  // Assert
  ASSERT_FALSE(result);

  // Cleanup
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsTrueIfBothAreTheSameAllScript)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(ALL_SCRIPT, strlen(ALL_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsTrueIfBothAreTheSameAnyScript)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(ANY_SCRIPT, strlen(ANY_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(ANY_SCRIPT, strlen(ANY_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsTrueIfBothAreTheSameAtLeastScript)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(AT_LEAST_SCRIPT, strlen(AT_LEAST_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(AT_LEAST_SCRIPT, strlen(AT_LEAST_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsTrueIfBothAreTheSamePubkeyScript)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsTrueIfBothAreTheSameInvalidAfterScript)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(AFTER_SCRIPT, strlen(AFTER_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}

TEST(cardano_native_script_equals, returnsTrueIfBothAreTheSameInvalidBeforeScript)
{
  // Arrange
  cardano_native_script_t* script1 = NULL;
  cardano_native_script_t* script2 = NULL;

  cardano_error_t error = cardano_native_script_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_from_json(BEFORE_SCRIPT, strlen(BEFORE_SCRIPT), &script2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_native_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(result);

  // Cleanup
  cardano_native_script_unref(&script1);
  cardano_native_script_unref(&script2);
}
