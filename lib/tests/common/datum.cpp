/**
 * \file datum.cpp
 *
 * \author luisd.bianchi
 * \date   May 15, 2024
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

#include <cardano/common/datum.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* INLINE_DATUM_CBOR       = "8201d8799f0102030405ff";
static const char* DATUM_HASH_CBOR         = "820058200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_DATUM_HASH_CBOR = "82005821000000000000000000000000000000000000000000000000000000000000000000";
static const char* HASH                    = "0000000000000000000000000000000000000000000000000000000000000000";

/* UNIT TESTS ****************************************************************/

TEST(cardano_datum_to_cbor, canSerializeInlineDatum)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(INLINE_DATUM_CBOR, strlen(INLINE_DATUM_CBOR));
  cardano_datum_t*       datum  = nullptr;
  cardano_error_t        error  = cardano_datum_from_cbor(reader, &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_to_cbor(datum, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex      = (char*)malloc(cbor_hex_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, INLINE_DATUM_CBOR);

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_to_cbor, canSerializeDataHash)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DATUM_HASH_CBOR, strlen(DATUM_HASH_CBOR));
  cardano_datum_t*       datum  = nullptr;
  cardano_error_t        error  = cardano_datum_from_cbor(reader, &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_to_cbor(datum, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex      = (char*)malloc(cbor_hex_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, DATUM_HASH_CBOR);

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DATUM_HASH_CBOR, strlen(DATUM_HASH_CBOR));
  cardano_datum_t*       datum  = nullptr;
  cardano_error_t        error  = cardano_datum_from_cbor(reader, &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_to_cbor(datum, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_datum_unref(&datum);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_datum_to_cbor, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_datum_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_datum_from_cbor, returnErrorIfInvalidArraySize)
{
  // Arrange
  const char*            invalid_cbor = "8100581c00000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_from_cbor(reader, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  const char* error_msg = cardano_cbor_reader_get_last_error(reader);
  EXPECT_STREQ(error_msg, "There was an error decoding the Datum, expected a Major Type: Byte String (2) of 2 element(s) but got a Major Type: Byte String (2) of 1 element(s).");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_datum_from_cbor, returnErrorIfInvalidDatumType)
{
  // Arrange
  const char*            invalid_cbor = "8203581c00000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_from_cbor(reader, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  const char* error_msg = cardano_cbor_reader_get_last_error(reader);
  EXPECT_STREQ(error_msg, "There was an error decoding the Datum, datum_type must have a value between 0 and 1, but got 3.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_datum_from_cbor, returnErrorIfInvalidByteStringSize)
{
  // Arrange
  const char*            invalid_cbor = "8200581b0000000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t* reader       = cardano_cbor_reader_from_hex(
    invalid_cbor,
    strlen(invalid_cbor));

  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_from_cbor(reader, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_datum_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_from_cbor(nullptr, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_from_cbor, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DATUM_HASH_CBOR, strlen(DATUM_HASH_CBOR));

  // Act
  cardano_error_t error = cardano_datum_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_datum_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DATUM_HASH_CBOR, strlen(DATUM_HASH_CBOR));
  cardano_datum_t*       datum  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_fourteen_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_datum_from_cbor(reader, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_from_cbor, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(INLINE_DATUM_CBOR, strlen(INLINE_DATUM_CBOR));
  cardano_datum_t*       datum  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_fourteen_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_datum_from_cbor(reader, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_from_cbor, returnsErrorIfMemoryAllocationFails3)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(INLINE_DATUM_CBOR, strlen(INLINE_DATUM_CBOR));
  cardano_datum_t*       datum  = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_thirty_seven_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_datum_from_cbor(reader, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_new, returnsErrorIfHashIsInvalidSize)
{
  // Arrange
  cardano_datum_t*        datum = nullptr;
  cardano_blake2b_hash_t* hash  = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_DATUM_HASH_CBOR,
    strlen(INVALID_DATUM_HASH_CBOR),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_new_data_hash(hash, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_datum_new, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_new_data_hash(nullptr, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);
}

TEST(cardano_datum_new, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INLINE_DATUM_CBOR,
    strlen(INLINE_DATUM_CBOR),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_new_data_hash(
    hash,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_datum_new_data_hash_bytes, canCreateHashDatum)
{
  // Arrange
  cardano_datum_t*        datum = nullptr;
  cardano_blake2b_hash_t* hash  = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH,
    strlen(HASH),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_new_data_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(datum, testing::Not((cardano_datum_t*)nullptr));

  cardano_blake2b_hash_t* hash2       = cardano_datum_get_data_hash(datum);
  const byte_t*           hash2_bytes = cardano_blake2b_hash_get_data(hash2);
  const byte_t*           hash3_bytes = cardano_datum_get_data_hash_bytes(datum);
  const char*             hex         = cardano_datum_get_data_hash_hex(datum);

  EXPECT_EQ(memcmp(hash2_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_EQ(memcmp(hash3_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, HASH);

  cardano_datum_type_t type = CARDANO_DATUM_TYPE_DATA_HASH;
  error                     = cardano_datum_get_type(datum, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_DATUM_TYPE_DATA_HASH);

  // Cleanup
  cardano_datum_unref(&datum);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_datum_new_data_hash_bytes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_datum_t*        datum = nullptr;
  cardano_blake2b_hash_t* hash  = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH,
    strlen(HASH),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_datum_new_data_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_new_data_hash_bytes, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_datum_t*        datum = nullptr;
  cardano_blake2b_hash_t* hash  = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH,
    strlen(HASH),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  error = cardano_datum_new_data_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_new_data_hash_bytes, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_new_data_hash_bytes(
    nullptr,
    0,
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);
}

TEST(cardano_datum_new_data_hash_bytes, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INLINE_DATUM_CBOR,
    strlen(INLINE_DATUM_CBOR),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_new_data_hash_bytes(
    cardano_blake2b_hash_get_data(hash),
    cardano_blake2b_hash_get_bytes_size(hash),
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_datum_new_data_hash_bytes, returnsErrorIfHashIsInvalid)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_new_data_hash_bytes(
    nullptr,
    0,
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);
}

TEST(cardano_datum_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_datum_t* datum = nullptr;
  cardano_error_t  error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_datum_ref(datum);

  // Assert
  EXPECT_THAT(datum, testing::Not((cardano_datum_t*)nullptr));
  EXPECT_EQ(cardano_datum_refcount(datum), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_datum_unref(&datum);
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_datum_ref(nullptr);
}

TEST(cardano_datum_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_datum_unref((cardano_datum_t**)nullptr);
}

TEST(cardano_datum_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_datum_t* datum = nullptr;
  cardano_error_t  error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_datum_ref(datum);
  size_t ref_count = cardano_datum_refcount(datum);

  cardano_datum_unref(&datum);
  size_t updated_ref_count = cardano_datum_refcount(datum);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_datum_t* datum = nullptr;
  cardano_error_t  error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_datum_ref(datum);
  size_t ref_count = cardano_datum_refcount(datum);

  cardano_datum_unref(&datum);
  size_t updated_ref_count = cardano_datum_refcount(datum);

  cardano_datum_unref(&datum);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_datum_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_datum_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_datum_t* datum   = nullptr;
  const char*      message = "This is a test message";

  // Act
  cardano_datum_set_last_error(datum, message);

  // Assert
  EXPECT_STREQ(cardano_datum_get_last_error(datum), "Object is NULL.");
}

TEST(cardano_datum_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_datum_t* datum = nullptr;
  cardano_error_t  error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_datum_set_last_error(datum, message);

  // Assert
  EXPECT_STREQ(cardano_datum_get_last_error(datum), "");

  // Cleanup
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_get_hash, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_datum_get_data_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_datum_get_hash_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  const byte_t* hash = cardano_datum_get_data_hash_bytes(nullptr);

  // Assert
  EXPECT_EQ(hash, (const byte_t*)nullptr);
}

TEST(cardano_datum_get_hash_hex, returnsNullIfGivenANullPtr)
{
  // Act
  const char* hash = cardano_datum_get_data_hash_hex(nullptr);

  // Assert
  EXPECT_EQ(hash, (const char*)nullptr);
}

TEST(cardano_datum_get_type, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_datum_type_t type = CARDANO_DATUM_TYPE_DATA_HASH;

  // Act
  cardano_error_t error = cardano_datum_get_type(nullptr, &type);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_get_type, returnsErrorIfTypeIsNull)
{
  // Act
  cardano_error_t error = cardano_datum_get_type((cardano_datum_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_set_hash, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_error_t error = cardano_datum_set_data_hash(nullptr, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_set_hash, returnsErrorIfHashIsNull)
{
  // Act
  cardano_error_t error = cardano_datum_set_data_hash((cardano_datum_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_get_hash_hex_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_datum_get_data_hash_hex_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_datum_get_hash_bytes_size, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t size = cardano_datum_get_data_hash_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_datum_new_data_hash_hex, returnsErrorIfDatumIsNull)
{
  // Act
  cardano_error_t error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_new_data_hash_hex, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_new_data_hash_hex(
    nullptr,
    0,
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_new_data_hash_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_new_data_hash_hex, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_new_inline_data, returnsErrorIfDatumIsNull)
{
  // Act
  cardano_error_t error = cardano_datum_new_inline_data(
    (cardano_plutus_data_t*)HASH,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_new_inline_data, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_new_inline_data(
    nullptr,
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_datum_new_inline_data, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_datum_new_inline_data(
    (cardano_plutus_data_t*)HASH,
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_datum_get_inline_data, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_plutus_data_t* data = cardano_datum_get_inline_data(nullptr);

  // Assert
  EXPECT_EQ(data, (cardano_plutus_data_t*)nullptr);
}

TEST(cardano_datum_get_inline_data, canGetTheInlineData)
{
  // Arrange
  cardano_datum_t*       datum = nullptr;
  cardano_plutus_data_t* data  = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(
    INLINE_DATUM_CBOR,
    strlen(INLINE_DATUM_CBOR));

  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &data);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_datum_new_inline_data(
    data,
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_t* data2 = cardano_datum_get_inline_data(datum);

  // Assert
  EXPECT_THAT(data, testing::Not((cardano_plutus_data_t*)nullptr));

  // Cleanup
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
  cardano_plutus_data_unref(&data2);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_datum_get_data_hash_bytes_size, returnsTheSize)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  cardano_error_t error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_datum_get_data_hash_bytes_size(datum);

  // Assert
  EXPECT_EQ(size, 32);

  // Cleanup
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_get_data_hash_hex_size, returnsTheSize)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  cardano_error_t error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_datum_get_data_hash_hex_size(datum);

  // Assert
  EXPECT_EQ(size, 65);

  // Cleanup
  cardano_datum_unref(&datum);
}

TEST(cardano_datum_set_data_hash, canSetTheDataHash)
{
  // Arrange
  cardano_datum_t*        datum = nullptr;
  cardano_blake2b_hash_t* hash  = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    HASH,
    strlen(HASH),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_set_data_hash(datum, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash2       = cardano_datum_get_data_hash(datum);
  const byte_t*           hash2_bytes = cardano_blake2b_hash_get_data(hash2);
  const byte_t*           hash3_bytes = cardano_datum_get_data_hash_bytes(datum);
  const char*             hex         = cardano_datum_get_data_hash_hex(datum);

  EXPECT_EQ(memcmp(hash2_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_EQ(memcmp(hash3_bytes, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash)), 0);
  EXPECT_STREQ(hex, HASH);

  // Cleanup
  cardano_datum_unref(&datum);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash2);
}

TEST(cardano_datum_set_data_hash, returnsErrorIfSetsHashOfWrongSize)
{
  // Arrange
  cardano_datum_t*        datum = nullptr;
  cardano_blake2b_hash_t* hash  = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(
    INVALID_DATUM_HASH_CBOR,
    strlen(INVALID_DATUM_HASH_CBOR),
    &hash);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_datum_new_data_hash_hex(
    HASH,
    strlen(HASH),
    &datum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_datum_set_data_hash(datum, hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);

  // Cleanup
  cardano_datum_unref(&datum);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_datum_new_data_hash_bytes, returnsErrorIfBytesAreTheWorngSize)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_new_data_hash_bytes(
    (const byte_t*)HASH,
    31,
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);
}

TEST(cardano_datum_new_data_hash_hex, returnsErrorIfHexIsTheWrongSize)
{
  // Arrange
  cardano_datum_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_datum_new_data_hash_hex(
    HASH,
    31,
    &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
  EXPECT_EQ(datum, (cardano_datum_t*)nullptr);
}