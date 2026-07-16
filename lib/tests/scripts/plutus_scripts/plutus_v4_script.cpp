/**
 * \file plutus_v4_script.cpp
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
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
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v4_script.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <cstdlib>
#include <gmock/gmock.h>

static const char* PLUTUS_V4_SCRIPT = "4d01000033222220051200120011";
static const char* PLUTUS_V4_HASH   = "4d0bedb209c225070654ef9753d314d16527a5910691adb50d717df9";
static const char* PLUTUS_V4_CBOR   = "4e4d01000033222220051200120011";

static auto
hexToBytes(const std::string& hex) -> std::vector<byte_t>
{
  std::vector<byte_t> bytes;

  for (unsigned int i = 0; i < hex.length(); i += 2)
  {
    std::string const byteString = hex.substr(i, 2);
    char              byte       = (char)strtol(byteString.c_str(), nullptr, 16);
    bytes.push_back(byte);
  }

  return bytes;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_plutus_v4_script_new, canCreateAPlutusV4Script)
{
  // Arrange
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_plutus_v4_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v4_script_to_cbor(script, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_THAT(hex, testing::StrEq(PLUTUS_V4_CBOR));

  // cleanup
  cardano_plutus_v4_script_unref(&script);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_plutus_v4_script_new, returnsErrorIfGivenNullScript)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(NULL, 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_new, returnsErrorIfGivenNullScriptPointer)
{
  // Arrange
  std::vector<byte_t> bytes = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t     error = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_new, returnsErrorIfGivenEmptyScript)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_plutus_v4_script_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v4_script_new, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v4_script_new_bytes_from_hex, canCreateAPlutusV4ScriptFromHex)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes_from_hex(PLUTUS_V4_SCRIPT, strlen(PLUTUS_V4_SCRIPT), &script);
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v4_script_to_cbor(script, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_THAT(hex, testing::StrEq(PLUTUS_V4_CBOR));

  // cleanup
  cardano_plutus_v4_script_unref(&script);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_plutus_v4_script_new_bytes_from_hex, returnsErrorIfGivenNullHex)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes_from_hex(NULL, 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_new_bytes_from_hex, returnsErrorIfGivenEmptyHex)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes_from_hex(PLUTUS_V4_SCRIPT, 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_plutus_v4_script_new_bytes_from_hex, returnsErrorIfGivenNullScriptPointer)
{
  // Arrange
  cardano_error_t error = cardano_plutus_v4_script_new_bytes_from_hex(PLUTUS_V4_SCRIPT, strlen(PLUTUS_V4_SCRIPT), NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_new_bytes_from_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v4_script_new_bytes_from_hex(PLUTUS_V4_SCRIPT, strlen(PLUTUS_V4_SCRIPT), &script);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v4_script_new_bytes_from_hex, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v4_script_new_bytes_from_hex(PLUTUS_V4_SCRIPT, strlen(PLUTUS_V4_SCRIPT), &script);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v4_script_from_cbor, canCreateAPlutusV4ScriptFromCbor)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(PLUTUS_V4_CBOR, strlen(PLUTUS_V4_CBOR));
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_v4_script_from_cbor(reader, &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v4_script_to_cbor(script, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_THAT(hex, testing::StrEq(PLUTUS_V4_CBOR));

  // cleanup
  cardano_plutus_v4_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_plutus_v4_script_from_cbor, returnsErrorIfGivenNullReader)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v4_script_from_cbor(NULL, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_from_cbor, returnsErrorIfGivenNullScriptPointer)
{
  // Arrange
  cardano_error_t error = cardano_plutus_v4_script_from_cbor((cardano_cbor_reader_t*)"", NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(PLUTUS_V4_CBOR, strlen(PLUTUS_V4_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v4_script_from_cbor(reader, &script);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_v4_script_to_cbor, returnsErrorIfGivenNullScript)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_error_t        error  = cardano_plutus_v4_script_to_cbor(NULL, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_v4_script_to_cbor, returnsErrorIfGivenNullWriter)
{
  // Arrange
  cardano_error_t error = cardano_plutus_v4_script_to_cbor((cardano_plutus_v4_script_t*)"", NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_to_raw_bytes, canConvertPlutusV4ScriptToRawBytes)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);
  cardano_buffer_t*           buffer = NULL;

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v4_script_to_raw_bytes(script, &buffer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_EQ(cardano_buffer_get_size(buffer), bytes.size());

  for (size_t i = 0; i < bytes.size(); i++)
  {
    EXPECT_EQ(cardano_buffer_get_data(buffer)[i], bytes[i]);
  }

  // cleanup
  cardano_plutus_v4_script_unref(&script);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_plutus_v4_script_to_raw_bytes, returnsErrorIfGivenNullScript)
{
  // Arrange
  cardano_buffer_t* buffer = NULL;
  cardano_error_t   error  = cardano_plutus_v4_script_to_raw_bytes(NULL, &buffer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_plutus_v4_script_to_raw_bytes, returnsErrorIfGivenNullBufferPointer)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_v4_script_to_raw_bytes(script, NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // cleanup
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_get_hash, canGetTheHashOfAPlutusV4Script)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* hash = cardano_plutus_v4_script_get_hash(script);

  // Assert
  size_t hash_size = cardano_blake2b_hash_get_hex_size(hash);
  char*  hash_hex  = (char*)malloc(hash_size);

  cardano_error_t hash_error = cardano_blake2b_hash_to_hex(hash, hash_hex, hash_size);

  EXPECT_EQ(hash_error, CARDANO_SUCCESS);

  EXPECT_THAT(hash_hex, testing::StrEq(PLUTUS_V4_HASH));

  // cleanup
  cardano_plutus_v4_script_unref(&script);
  cardano_blake2b_hash_unref(&hash);
  free(hash_hex);
}

TEST(cardano_plutus_v4_script_get_hash, hashesWithADifferentPrefixThanPlutusV3)
{
  // Arrange
  cardano_plutus_v4_script_t* v4_script = NULL;
  cardano_plutus_v3_script_t* v3_script = NULL;
  std::vector<byte_t>         bytes     = hexToBytes(PLUTUS_V4_SCRIPT);

  EXPECT_EQ(cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &v4_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v3_script_new_bytes(bytes.data(), bytes.size(), &v3_script), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* v4_hash = cardano_plutus_v4_script_get_hash(v4_script);
  cardano_blake2b_hash_t* v3_hash = cardano_plutus_v3_script_get_hash(v3_script);

  // Assert
  EXPECT_FALSE(cardano_blake2b_hash_equals(v4_hash, v3_hash));

  // cleanup
  cardano_plutus_v4_script_unref(&v4_script);
  cardano_plutus_v3_script_unref(&v3_script);
  cardano_blake2b_hash_unref(&v4_hash);
  cardano_blake2b_hash_unref(&v3_hash);
}

TEST(cardano_plutus_v4_script_get_hash, returnsNullIfGivenNullScript)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_plutus_v4_script_get_hash(NULL);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_plutus_v4_script_get_hash, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_blake2b_hash_t* hash = cardano_plutus_v4_script_get_hash(script);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // cleanup
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_equals, returnsTrueIfTwoPlutusV4ScriptsAreEqual)
{
  // Arrange
  cardano_plutus_v4_script_t* script1 = NULL;
  cardano_plutus_v4_script_t* script2 = NULL;
  std::vector<byte_t>         bytes   = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error   = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script1);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script2);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_plutus_v4_script_equals(script1, script2);

  // Assert
  EXPECT_TRUE(are_equal);

  // cleanup
  cardano_plutus_v4_script_unref(&script1);
  cardano_plutus_v4_script_unref(&script2);
}

TEST(cardano_plutus_v4_script_equals, returnsFalseIfTwoPlutusV4ScriptsAreNotEqual)
{
  // Arrange
  cardano_plutus_v4_script_t* script1 = NULL;
  cardano_plutus_v4_script_t* script2 = NULL;
  std::vector<byte_t>         bytes   = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error   = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script1);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size() - 1, &script2);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_plutus_v4_script_equals(script1, script2);

  // Assert
  EXPECT_FALSE(are_equal);

  // cleanup
  cardano_plutus_v4_script_unref(&script1);
  cardano_plutus_v4_script_unref(&script2);
}

TEST(cardano_plutus_v4_script_equals, returnsFalseIfGivenNullScript)
{
  // Arrange
  cardano_plutus_v4_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_plutus_v4_script_equals(script, NULL);

  // Assert
  EXPECT_FALSE(are_equal);

  // cleanup
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_v4_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v4_script_ref(script);

  // Assert
  EXPECT_THAT(script, testing::Not((cardano_plutus_v4_script_t*)nullptr));
  EXPECT_EQ(cardano_plutus_v4_script_refcount(script), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_v4_script_unref(&script);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_v4_script_ref(nullptr);
}

TEST(cardano_plutus_v4_script_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_v4_script_t* script = nullptr;

  // Act
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_v4_script_unref((cardano_plutus_v4_script_t**)nullptr);
}

TEST(cardano_plutus_v4_script_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_v4_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v4_script_ref(script);
  size_t ref_count = cardano_plutus_v4_script_refcount(script);

  cardano_plutus_v4_script_unref(&script);
  size_t updated_ref_count = cardano_plutus_v4_script_refcount(script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_v4_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v4_script_ref(script);
  size_t ref_count = cardano_plutus_v4_script_refcount(script);

  cardano_plutus_v4_script_unref(&script);
  size_t updated_ref_count = cardano_plutus_v4_script_refcount(script);

  cardano_plutus_v4_script_unref(&script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(script, (cardano_plutus_v4_script_t*)nullptr);

  // Cleanup
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_v4_script_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_v4_script_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_v4_script_t* script  = nullptr;
  const char*                 message = "This is a test message";

  // Act
  cardano_plutus_v4_script_set_last_error(script, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_v4_script_get_last_error(script), "Object is NULL.");
}

TEST(cardano_plutus_v4_script_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_v4_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V4_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v4_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_v4_script_set_last_error(script, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_v4_script_get_last_error(script), "");

  // Cleanup
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_plutus_v4_script_to_cip116_json, canSerializePlutusV4Script)
{
  // Arrange
  cardano_json_writer_t*      json           = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_plutus_v4_script_t* script         = nullptr;
  const byte_t                SCRIPT_BYTES[] = { 0x01, 0x02, 0x03, 0x04 };

  cardano_error_t error = cardano_plutus_v4_script_new_bytes(SCRIPT_BYTES, sizeof(SCRIPT_BYTES), &script);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_v4_script_to_cip116_json(script, json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t json_size = cardano_json_writer_get_encoded_size(json);
  char*  json_str  = (char*)malloc(json_size);

  ASSERT_EQ(cardano_json_writer_encode(json, json_str, json_size), CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, "{\"language\":\"plutus_v4\",\"bytes\":\"01020304\"}");

  // Cleanup
  free(json_str);
  cardano_plutus_v4_script_unref(&script);
  cardano_json_writer_unref(&json);
}

TEST(cardano_plutus_v4_script_to_cip116_json, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error = cardano_plutus_v4_script_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_plutus_v4_script_to_cip116_json, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_plutus_v4_script_t* script         = nullptr;
  const byte_t                SCRIPT_BYTES[] = { 0x01 };

  ASSERT_EQ(cardano_plutus_v4_script_new_bytes(SCRIPT_BYTES, sizeof(SCRIPT_BYTES), &script), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_plutus_v4_script_to_cip116_json(script, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v4_script_unref(&script);
}