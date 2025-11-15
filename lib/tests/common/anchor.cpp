/**
 * \file anchor.cpp
 *
 * \author angel.castillo
 * \date   Apr 14, 2024
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

#include <cardano/buffer.h>
#include <cardano/common/anchor.h>

#include "../allocators_helpers.h"
#include "../json_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* INVALID_HASH_HEX = "000000000000000000000000000000000000000000000000";
static const char* HASH_HEX         = "0000000000000000000000000000000000000000000000000000000000000000";
static const char* HASH_HEX_2       = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
static const char* ANCHOR_CBOR      = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* URL              = "https://www.someurl.io";
static const char* URL_2            = "https://www.someotherurl.io";

/* UNIT TESTS ****************************************************************/

TEST(cardano_anchor_to_cbor, canSerializeAnchor)
{
  // Arrange
  cardano_anchor_t*      anchor = nullptr;
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_to_cbor(anchor, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(ANCHOR_CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, ANCHOR_CBOR);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_anchor_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_anchor_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_anchor_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_to_cbor(anchor, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_new, returnErrorIfUrlIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_new(nullptr, 0, nullptr, &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_new, returnErrorIfUrlIsEmpty)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_new("", 0, nullptr, &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_URL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_new, returnErrorIfHashIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_new(URL, strlen(URL), nullptr, &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_new, returnErrorIfAnchorIsNull)
{
  // Act
  cardano_error_t error = cardano_anchor_new(URL, strlen(URL), (cardano_blake2b_hash_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_anchor_new, ifHashIsTheWrongSize)
{
  // Arrange
  cardano_anchor_t*       anchor = nullptr;
  cardano_blake2b_hash_t* hash   = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_HASH_HEX,
    strlen(INVALID_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_new(URL, strlen(URL), hash, &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_anchor_from_hash_bytes, canCreateAnchorFromBytes)
{
  // Arrange
  cardano_anchor_t*       anchor = nullptr;
  cardano_blake2b_hash_t* hash   = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH_HEX,
    strlen(HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_from_hash_bytes(
    URL,
    strlen(URL),
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(anchor, testing::Not((cardano_anchor_t*)nullptr));

  cardano_blake2b_hash_t* hash2       = cardano_anchor_get_hash(anchor);
  const byte_t*           hash2_bytes = cardano_blake2b_hash_get_data(hash2);
  const byte_t*           hash3_bytes = cardano_anchor_get_hash_bytes(anchor);
  const char*             hex         = cardano_anchor_get_hash_hex(anchor);

  EXPECT_EQ(memcmp(hash2_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_EQ(memcmp(hash3_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, HASH_HEX);

  const char* url = cardano_anchor_get_url(anchor);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(url, URL);
  EXPECT_EQ(cardano_anchor_get_url_size(anchor), strlen(URL) + 1);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_anchor_set_url, canSetUrl)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_set_url(anchor, URL_2, strlen(URL_2));

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_anchor_get_url(anchor), URL_2);
  EXPECT_EQ(cardano_anchor_get_url_size(anchor), strlen(URL_2) + 1);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_set_url, returnErrorIfUrlIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_set_url(anchor, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_set_url, returnErrorIfAnchorIsNull)
{
  // Act
  cardano_error_t error = cardano_anchor_set_url(nullptr, URL, strlen(URL));

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_anchor_set_url, returnErrorIfUrlIsEmpty)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_set_url(anchor, "", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_URL);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_from_hash_hex, canCreateAnchor)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(anchor, testing::Not((cardano_anchor_t*)nullptr));

  cardano_blake2b_hash_t* hash        = cardano_anchor_get_hash(anchor);
  const byte_t*           hash_bytes  = cardano_blake2b_hash_get_data(hash);
  const byte_t*           hash_bytes2 = cardano_anchor_get_hash_bytes(anchor);
  const char*             hex         = cardano_anchor_get_hash_hex(anchor);

  EXPECT_EQ(memcmp(hash_bytes, hash_bytes2, cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, HASH_HEX);

  const char* url = cardano_anchor_get_url(anchor);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(url, URL);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_anchor_from_hash_hex, returnsErrorIfUrlIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_hash_hex(
    nullptr,
    0,
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_from_hash_hex, returnsErrorIfUrlIsEmpty)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_hash_hex(
    "",
    0,
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_URL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_from_hash_hex, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    nullptr,
    0,
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_from_hash_hex, returnsErrorIfAnchorIsNull)
{
  // Act
  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_anchor_from_hash_hex, returnsErrorIfHashIsInvalid)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    "",
    0,
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_from_hash_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_anchor_t*       anchor = nullptr;
  cardano_blake2b_hash_t* hash   = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH_HEX,
    strlen(HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_anchor_from_hash_bytes(
    URL,
    strlen(URL),
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_anchor_from_hash_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_anchor_from_cbor, canDeserializeAnchor)
{
  // Arrange
  cardano_anchor_t*      anchor = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));

  // Act
  cardano_error_t error = cardano_anchor_from_cbor(reader, &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(anchor, testing::Not((cardano_anchor_t*)nullptr));

  const byte_t* hash_bytes = cardano_anchor_get_hash_bytes(anchor);
  const char*   url        = cardano_anchor_get_url(anchor);

  static const byte_t expected_hash[32] = { 0 };
  EXPECT_EQ(memcmp(hash_bytes, expected_hash, cardano_anchor_get_hash_bytes_size(anchor)), 0);
  EXPECT_STREQ(url, URL);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(url, URL);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_anchor_from_cbor, returnErrorIfAnchorIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));

  // Act
  cardano_error_t error = cardano_anchor_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_anchor_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_cbor(nullptr, &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_anchor_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_anchor_t*      anchor = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_anchor_from_cbor(reader, &anchor);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'anchor', expected a 'Major Type: Byte String' (2) of 2 element(s) but got a 'Major Type: Byte String' (2) of 1 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_anchor_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotTextString)
{
  // Arrange
  cardano_anchor_t*      anchor = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("822d", 4);

  // Act
  cardano_error_t error = cardano_anchor_from_cbor(reader, &anchor);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'anchor', expected 'Reader State: Text String' (6) but got 'Reader State: Negative Integer' (2).");
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_anchor_from_cbor, returnErrorIfCborDataSecondElementIsNot32BytesByteString)
{
  // Arrange
  cardano_anchor_t*      anchor = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8268747470733a2f2f7777772e736f6d6575726c2e696f582d", 46);

  // Act
  cardano_error_t error = cardano_anchor_from_cbor(reader, &anchor);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'anchor', expected 'Reader State: Byte String' (3) but got 'Reader State: Text String' (6).");
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_anchor_from_hash_bytes, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_anchor_t*       anchor = nullptr;
  cardano_blake2b_hash_t* hash   = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH_HEX,
    strlen(HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  error = cardano_anchor_from_hash_bytes(
    URL,
    strlen(URL),
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_anchor_from_hash_bytes, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_anchor_t*       anchor = nullptr;
  cardano_blake2b_hash_t* hash   = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_HASH_HEX,
    strlen(INVALID_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_from_hash_bytes(
    URL,
    strlen(URL),
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_anchor_from_hash_bytes, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_hash_bytes(
    "",
    1,
    nullptr,
    0,
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_from_hash_bytes, returnsErrorIfanchorIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH_HEX,
    strlen(HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_from_hash_bytes(
    URL,
    strlen(URL),
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_anchor_from_hash_bytes, returnsErrorIfHashIsInvalid)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_from_hash_bytes(
    "",
    1,
    nullptr,
    0,
    &anchor);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);
}

TEST(cardano_anchor_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;
  cardano_error_t   error  = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_anchor_ref(anchor);

  // Assert
  EXPECT_THAT(anchor, testing::Not((cardano_anchor_t*)nullptr));
  EXPECT_EQ(cardano_anchor_refcount(anchor), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_anchor_unref(&anchor);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_anchor_ref(nullptr);
}

TEST(cardano_anchor_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;

  // Act
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_anchor_unref((cardano_anchor_t**)nullptr);
}

TEST(cardano_anchor_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;
  cardano_error_t   error  = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_anchor_ref(anchor);
  size_t ref_count = cardano_anchor_refcount(anchor);

  cardano_anchor_unref(&anchor);
  size_t updated_ref_count = cardano_anchor_refcount(anchor);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;
  cardano_error_t   error  = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_anchor_ref(anchor);
  size_t ref_count = cardano_anchor_refcount(anchor);

  cardano_anchor_unref(&anchor);
  size_t updated_ref_count = cardano_anchor_refcount(anchor);

  cardano_anchor_unref(&anchor);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(anchor, (cardano_anchor_t*)nullptr);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_anchor_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_anchor_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_anchor_t* anchor  = nullptr;
  const char*       message = "This is a test message";

  // Act
  cardano_anchor_set_last_error(anchor, message);

  // Assert
  EXPECT_STREQ(cardano_anchor_get_last_error(anchor), "Object is NULL.");
}

TEST(cardano_anchor_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = nullptr;
  cardano_error_t   error  = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_anchor_set_last_error(anchor, message);

  // Assert
  EXPECT_STREQ(cardano_anchor_get_last_error(anchor), "");

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_anchor_get_hash, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_anchor_get_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_anchor_get_hash_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  const byte_t* hash = cardano_anchor_get_hash_bytes(nullptr);

  // Assert
  EXPECT_EQ(hash, (const byte_t*)nullptr);
}

TEST(cardano_anchor_get_hash_hex, returnsNullIfGivenANullPtr)
{
  // Act
  const char* hash = cardano_anchor_get_hash_hex(nullptr);

  // Assert
  EXPECT_EQ(hash, (const char*)nullptr);
}

TEST(cardano_anchor_set_hash, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_error_t error = cardano_anchor_set_hash(nullptr, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_anchor_set_hash, returnsErrorIfHashIsNull)
{
  // Act
  cardano_error_t error = cardano_anchor_set_hash((cardano_anchor_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_anchor_set_hash, canSetHash)
{
  // Arrange
  cardano_anchor_t*       anchor = nullptr;
  cardano_blake2b_hash_t* hash   = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH_HEX_2,
    strlen(HASH_HEX_2),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_set_hash(anchor, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash2 = cardano_anchor_get_hash(anchor);
  const char*             hex   = cardano_anchor_get_hash_hex(anchor);

  EXPECT_EQ(memcmp(cardano_blake2b_hash_get_data(hash2), cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, HASH_HEX_2);
  EXPECT_EQ(cardano_anchor_get_hash_hex_size(anchor), strlen(HASH_HEX_2) + 1);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_anchor_set_hash, returnErrorIfWorngHashSize)
{
  // Arrange
  cardano_anchor_t*       anchor = nullptr;
  cardano_blake2b_hash_t* hash   = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_HASH_HEX,
    strlen(INVALID_HASH_HEX),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_anchor_from_hash_hex(
    URL,
    strlen(URL),
    HASH_HEX,
    strlen(HASH_HEX),
    &anchor);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_anchor_set_hash(anchor, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_anchor_get_url_size, returnsNullIfGivenANullPtr)
{
  // Act
  size_t size = cardano_anchor_get_url_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_anchor_get_url, returnsNullIfGivenANullPtr)
{
  // Act
  const char* url = cardano_anchor_get_url(nullptr);

  // Assert
  EXPECT_EQ(url, (const char*)nullptr);
}

TEST(cardano_anchor_get_hash_hex_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_anchor_get_hash_hex_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_anchor_get_hash_bytes_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_anchor_get_hash_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_anchor_to_cip116_json, canConvertToCip116Json)
{
  // Arrange
  cardano_error_t error = CARDANO_SUCCESS;

  const char* url      = "https://example.com/metadata.json";
  const char* hash_hex = "2a3f9a878b3b9ac18a65c16ed1c92c37fd4f5a16e629580a23330f6e0f6e0f6e";

  cardano_blake2b_hash_t* hash = NULL;
  error                        = cardano_blake2b_hash_from_hex(hash_hex, strlen(hash_hex), &hash);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_anchor_t* anchor = NULL;
  error                    = cardano_anchor_new(url, strlen(url), hash, &anchor);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  error          = cardano_anchor_to_cip116_json(anchor, json);
  char* json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"url":"https://example.com/metadata.json","data_hash":"2a3f9a878b3b9ac18a65c16ed1c92c37fd4f5a16e629580a23330f6e0f6e0f6e"})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&hash);
  free(json_str);
}

TEST(cardano_anchor_to_cip116_json, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error = cardano_anchor_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_anchor_to_cip116_json, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = NULL;
  EXPECT_EQ(cardano_blake2b_hash_from_hex("2a3f9a878b3b9ac18a65c16ed1c92c37fd4f5a16e629580a23330f6e0f6e0f6e", 64, &hash), CARDANO_SUCCESS);

  cardano_anchor_t* anchor = NULL;
  EXPECT_EQ(cardano_anchor_new("url", 3, hash, &anchor), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_anchor_to_cip116_json(anchor, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&hash);
}