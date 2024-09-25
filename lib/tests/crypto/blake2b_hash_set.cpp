/**
 * \file blake2b_hash_set.cpp
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#include <cardano/crypto/blake2b_hash_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR               = "d9010284581c00000000000000000000000000000000000000000000000000000001581c00000000000000000000000000000000000000000000000000000002581c00000000000000000000000000000000000000000000000000000003581c00000000000000000000000000000000000000000000000000000004";
static const char* CBOR_WITHOUT_TAG   = "84581c00000000000000000000000000000000000000000000000000000001581c00000000000000000000000000000000000000000000000000000002581c00000000000000000000000000000000000000000000000000000003581c00000000000000000000000000000000000000000000000000000004";
static const char* BLAKE2B_HASH1_CBOR = "581c00000000000000000000000000000000000000000000000000000001";
static const char* BLAKE2B_HASH2_CBOR = "581c00000000000000000000000000000000000000000000000000000002";
static const char* BLAKE2B_HASH3_CBOR = "581c00000000000000000000000000000000000000000000000000000003";
static const char* BLAKE2B_HASH4_CBOR = "581c00000000000000000000000000000000000000000000000000000004";

/**
 * Creates a new default instance of the blake2b_hash.
 * @return A new instance of the blake2b_hash.
 */
static cardano_blake2b_hash_t*
new_default_blake2b_hash(const char* cbor)
{
  cardano_blake2b_hash_t* blake2b_hash = nullptr;
  cardano_cbor_reader_t*  reader       = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_blake2b_hash_from_cbor(reader, &blake2b_hash);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&blake2b_hash);
    return nullptr;
  }

  return blake2b_hash;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_blake2b_hash_set_new, canCreateBlake2bHashSet)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(blake2b_hash_set, testing::Not((cardano_blake2b_hash_set_t*)nullptr));

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_new, returnsErrorIfBlake2bHashSetIsNull)
{
  // Act
  cardano_error_t error = cardano_blake2b_hash_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(blake2b_hash_set, (cardano_blake2b_hash_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(blake2b_hash_set, (cardano_blake2b_hash_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_blake2b_hash_set_to_cbor, canSerializeAnEmptyBlake2bHashSet)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_set_to_cbor(blake2b_hash_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_blake2b_hash_set_to_cbor, canSerializeBlake2bHashSet)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* blake2b_hashs[] = { BLAKE2B_HASH1_CBOR, BLAKE2B_HASH2_CBOR, BLAKE2B_HASH3_CBOR, BLAKE2B_HASH4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_blake2b_hash_t* blake2b_hash = new_default_blake2b_hash(blake2b_hashs[i]);

    EXPECT_EQ(cardano_blake2b_hash_set_add(blake2b_hash_set, blake2b_hash), CARDANO_SUCCESS);

    cardano_blake2b_hash_unref(&blake2b_hash);
  }

  // Act
  error = cardano_blake2b_hash_set_to_cbor(blake2b_hash_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_blake2b_hash_set_to_cbor, canSerializeBlake2bHashSetSorted)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* blake2b_hashs[] = { BLAKE2B_HASH1_CBOR, BLAKE2B_HASH3_CBOR, BLAKE2B_HASH2_CBOR, BLAKE2B_HASH4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_blake2b_hash_t* blake2b_hash = new_default_blake2b_hash(blake2b_hashs[i]);

    EXPECT_EQ(cardano_blake2b_hash_set_add(blake2b_hash_set, blake2b_hash), CARDANO_SUCCESS);

    cardano_blake2b_hash_unref(&blake2b_hash);
  }

  // Act
  error = cardano_blake2b_hash_set_to_cbor(blake2b_hash_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_blake2b_hash_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_blake2b_hash_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;

  cardano_error_t error = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_set_to_cbor(blake2b_hash_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &blake2b_hash_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_set_to_cbor(blake2b_hash_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_blake2b_hash_set_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &blake2b_hash_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_set_to_cbor(blake2b_hash_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_blake2b_hash_set_from_cbor, canDeserializeBlake2bHashSet)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &blake2b_hash_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(blake2b_hash_set, testing::Not((cardano_blake2b_hash_set_t*)nullptr));

  const size_t length = cardano_blake2b_hash_set_get_length(blake2b_hash_set);

  EXPECT_EQ(length, 4);

  cardano_blake2b_hash_t* elem1 = NULL;
  cardano_blake2b_hash_t* elem2 = NULL;
  cardano_blake2b_hash_t* elem3 = NULL;
  cardano_blake2b_hash_t* elem4 = NULL;

  EXPECT_EQ(cardano_blake2b_hash_set_get(blake2b_hash_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_set_get(blake2b_hash_set, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_set_get(blake2b_hash_set, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_set_get(blake2b_hash_set, 3, &elem4), CARDANO_SUCCESS);

  const char* blake2b_hashs[] = { BLAKE2B_HASH1_CBOR, BLAKE2B_HASH2_CBOR, BLAKE2B_HASH3_CBOR, BLAKE2B_HASH4_CBOR };

  cardano_blake2b_hash_t* blake2b_hashs_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_blake2b_hash_to_cbor(blake2b_hashs_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(blake2b_hashs[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, blake2b_hashs[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  cardano_cbor_reader_unref(&reader);

  cardano_blake2b_hash_unref(&elem1);
  cardano_blake2b_hash_unref(&elem2);
  cardano_blake2b_hash_unref(&elem3);
  cardano_blake2b_hash_unref(&elem4);
}

TEST(cardano_blake2b_hash_set_from_cbor, returnErrorIfBlake2bHashSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(nullptr, &blake2b_hash_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &blake2b_hash_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(blake2b_hash_set, (cardano_blake2b_hash_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_blake2b_hash_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_set_from_cbor, returnErrorIfInvalidRelayElements)
{
  // Arrange
  cardano_blake2b_hash_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_set_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_blake2b_hash_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_blake2b_hash_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_blake2b_hash_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_set_ref(blake2b_hash_set);

  // Assert
  EXPECT_THAT(blake2b_hash_set, testing::Not((cardano_blake2b_hash_set_t*)nullptr));
  EXPECT_EQ(cardano_blake2b_hash_set_refcount(blake2b_hash_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_set_ref(nullptr);
}

TEST(cardano_blake2b_hash_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;

  // Act
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_blake2b_hash_set_unref((cardano_blake2b_hash_set_t**)nullptr);
}

TEST(cardano_blake2b_hash_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_set_ref(blake2b_hash_set);
  size_t ref_count = cardano_blake2b_hash_set_refcount(blake2b_hash_set);

  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  size_t updated_ref_count = cardano_blake2b_hash_set_refcount(blake2b_hash_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_set_ref(blake2b_hash_set);
  size_t ref_count = cardano_blake2b_hash_set_refcount(blake2b_hash_set);

  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
  size_t updated_ref_count = cardano_blake2b_hash_set_refcount(blake2b_hash_set);

  cardano_blake2b_hash_set_unref(&blake2b_hash_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(blake2b_hash_set, (cardano_blake2b_hash_set_t*)nullptr);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_blake2b_hash_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_blake2b_hash_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  const char*                 message          = "This is a test message";

  // Act
  cardano_blake2b_hash_set_set_last_error(blake2b_hash_set, message);

  // Assert
  EXPECT_STREQ(cardano_blake2b_hash_set_get_last_error(blake2b_hash_set), "Object is NULL.");
}

TEST(cardano_blake2b_hash_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_blake2b_hash_set_set_last_error(blake2b_hash_set, message);

  // Assert
  EXPECT_STREQ(cardano_blake2b_hash_set_get_last_error(blake2b_hash_set), "");

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_get_length, returnsZeroIfBlake2bHashSetIsNull)
{
  // Act
  size_t length = cardano_blake2b_hash_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_blake2b_hash_set_get_length, returnsZeroIfBlake2bHashSetIsEmpty)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_blake2b_hash_set_get_length(blake2b_hash_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_get, returnsErrorIfBlake2bHashSetIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_set_get(blake2b_hash_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* data = nullptr;
  error                        = cardano_blake2b_hash_set_get(blake2b_hash_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}

TEST(cardano_blake2b_hash_set_add, returnsErrorIfBlake2bHashSetIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_blake2b_hash_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_blake2b_hash_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_blake2b_hash_set_t* blake2b_hash_set = nullptr;
  cardano_error_t             error            = cardano_blake2b_hash_set_new(&blake2b_hash_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_blake2b_hash_set_add(blake2b_hash_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_set_unref(&blake2b_hash_set);
}
