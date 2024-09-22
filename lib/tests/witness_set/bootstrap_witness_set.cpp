/**
 * \file bootstrap_witness_set.cpp
 *
 * \author angel.castillo
 * \date   Sep 1, 2024
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

#include <cardano/witness_set/bootstrap_witness_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                    = "d90102848458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
static const char* CBOR_WITHOUT_TAG        = "848458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a08458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
static const char* BOOTSTRAP_WITNESS1_CBOR = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
static const char* BOOTSTRAP_WITNESS2_CBOR = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
static const char* BOOTSTRAP_WITNESS3_CBOR = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
static const char* BOOTSTRAP_WITNESS4_CBOR = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";

/**
 * Creates a new default instance of the bootstrap_witness.
 * @return A new instance of the bootstrap_witness.
 */
static cardano_bootstrap_witness_t*
new_default_bootstrap_witness(const char* cbor)
{
  cardano_bootstrap_witness_t* bootstrap_witness = nullptr;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_bootstrap_witness_from_cbor(reader, &bootstrap_witness);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_bootstrap_witness_unref(&bootstrap_witness);
    return nullptr;
  }

  return bootstrap_witness;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_bootstrap_witness_set_new, canCreateCredentialSet)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(bootstrap_witness_set, testing::Not((cardano_bootstrap_witness_set_t*)nullptr));

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_new, returnsErrorIfCredentialSetIsNull)
{
  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(bootstrap_witness_set, (cardano_bootstrap_witness_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bootstrap_witness_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(bootstrap_witness_set, (cardano_bootstrap_witness_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bootstrap_witness_set_to_cbor, canSerializeAnEmptyCredentialSet)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bootstrap_witness_set_to_cbor(bootstrap_witness_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_bootstrap_witness_set_to_cbor, canSerializeCredentialSet)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* bootstrap_witnesss[] = { BOOTSTRAP_WITNESS1_CBOR, BOOTSTRAP_WITNESS2_CBOR, BOOTSTRAP_WITNESS3_CBOR, BOOTSTRAP_WITNESS4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_bootstrap_witness_t* bootstrap_witness = new_default_bootstrap_witness(bootstrap_witnesss[i]);

    EXPECT_EQ(cardano_bootstrap_witness_set_add(bootstrap_witness_set, bootstrap_witness), CARDANO_SUCCESS);

    cardano_bootstrap_witness_unref(&bootstrap_witness);
  }

  // Act
  error = cardano_bootstrap_witness_set_to_cbor(bootstrap_witness_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_bootstrap_witness_set_to_cbor, canSerializeCredentialSetSorted)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* bootstrap_witnesss[] = { BOOTSTRAP_WITNESS1_CBOR, BOOTSTRAP_WITNESS2_CBOR, BOOTSTRAP_WITNESS3_CBOR, BOOTSTRAP_WITNESS4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_bootstrap_witness_t* bootstrap_witness = new_default_bootstrap_witness(bootstrap_witnesss[i]);

    EXPECT_EQ(cardano_bootstrap_witness_set_add(bootstrap_witness_set, bootstrap_witness), CARDANO_SUCCESS);

    cardano_bootstrap_witness_unref(&bootstrap_witness);
  }

  // Act
  error = cardano_bootstrap_witness_set_to_cbor(bootstrap_witness_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_bootstrap_witness_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_bootstrap_witness_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;

  cardano_error_t error = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bootstrap_witness_set_to_cbor(bootstrap_witness_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &bootstrap_witness_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_bootstrap_witness_set_to_cbor(bootstrap_witness_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_bootstrap_witness_set_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &bootstrap_witness_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_bootstrap_witness_set_to_cbor(bootstrap_witness_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_WITHOUT_TAG) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITHOUT_TAG);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_bootstrap_witness_set_from_cbor, canDeserializeCredentialSet)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &bootstrap_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(bootstrap_witness_set, testing::Not((cardano_bootstrap_witness_set_t*)nullptr));

  const size_t length = cardano_bootstrap_witness_set_get_length(bootstrap_witness_set);

  EXPECT_EQ(length, 4);

  cardano_bootstrap_witness_t* elem1 = NULL;
  cardano_bootstrap_witness_t* elem2 = NULL;
  cardano_bootstrap_witness_t* elem3 = NULL;
  cardano_bootstrap_witness_t* elem4 = NULL;

  EXPECT_EQ(cardano_bootstrap_witness_set_get(bootstrap_witness_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bootstrap_witness_set_get(bootstrap_witness_set, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bootstrap_witness_set_get(bootstrap_witness_set, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bootstrap_witness_set_get(bootstrap_witness_set, 3, &elem4), CARDANO_SUCCESS);

  const char* bootstrap_witnesss[] = { BOOTSTRAP_WITNESS1_CBOR, BOOTSTRAP_WITNESS2_CBOR, BOOTSTRAP_WITNESS3_CBOR, BOOTSTRAP_WITNESS4_CBOR };

  cardano_bootstrap_witness_t* bootstrap_witnesss_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_bootstrap_witness_to_cbor(bootstrap_witnesss_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(bootstrap_witnesss[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, bootstrap_witnesss[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  cardano_cbor_reader_unref(&reader);

  cardano_bootstrap_witness_unref(&elem1);
  cardano_bootstrap_witness_unref(&elem2);
  cardano_bootstrap_witness_unref(&elem3);
  cardano_bootstrap_witness_unref(&elem4);
}

TEST(cardano_bootstrap_witness_set_from_cbor, returnErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(nullptr, &bootstrap_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &bootstrap_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(bootstrap_witness_set, (cardano_bootstrap_witness_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_bootstrap_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_set_from_cbor, returnErrorIfInvalidRelayElements)
{
  // Arrange
  cardano_bootstrap_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_set_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_bootstrap_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_bootstrap_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bootstrap_witness_set_ref(bootstrap_witness_set);

  // Assert
  EXPECT_THAT(bootstrap_witness_set, testing::Not((cardano_bootstrap_witness_set_t*)nullptr));
  EXPECT_EQ(cardano_bootstrap_witness_set_refcount(bootstrap_witness_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bootstrap_witness_set_ref(nullptr);
}

TEST(cardano_bootstrap_witness_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;

  // Act
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bootstrap_witness_set_unref((cardano_bootstrap_witness_set_t**)nullptr);
}

TEST(cardano_bootstrap_witness_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bootstrap_witness_set_ref(bootstrap_witness_set);
  size_t ref_count = cardano_bootstrap_witness_set_refcount(bootstrap_witness_set);

  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  size_t updated_ref_count = cardano_bootstrap_witness_set_refcount(bootstrap_witness_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bootstrap_witness_set_ref(bootstrap_witness_set);
  size_t ref_count = cardano_bootstrap_witness_set_refcount(bootstrap_witness_set);

  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
  size_t updated_ref_count = cardano_bootstrap_witness_set_refcount(bootstrap_witness_set);

  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(bootstrap_witness_set, (cardano_bootstrap_witness_set_t*)nullptr);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_bootstrap_witness_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_bootstrap_witness_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  const char*                      message               = "This is a test message";

  // Act
  cardano_bootstrap_witness_set_set_last_error(bootstrap_witness_set, message);

  // Assert
  EXPECT_STREQ(cardano_bootstrap_witness_set_get_last_error(bootstrap_witness_set), "Object is NULL.");
}

TEST(cardano_bootstrap_witness_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_bootstrap_witness_set_set_last_error(bootstrap_witness_set, message);

  // Assert
  EXPECT_STREQ(cardano_bootstrap_witness_set_get_last_error(bootstrap_witness_set), "");

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_get_length, returnsZeroIfCredentialSetIsNull)
{
  // Act
  size_t length = cardano_bootstrap_witness_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_bootstrap_witness_set_get_length, returnsZeroIfCredentialSetIsEmpty)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_bootstrap_witness_set_get_length(bootstrap_witness_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_get, returnsErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bootstrap_witness_set_get(bootstrap_witness_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bootstrap_witness_t* data = nullptr;
  error                             = cardano_bootstrap_witness_set_get(bootstrap_witness_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}

TEST(cardano_bootstrap_witness_set_add, returnsErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_bootstrap_witness_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_bootstrap_witness_set_t* bootstrap_witness_set = nullptr;
  cardano_error_t                  error                 = cardano_bootstrap_witness_set_new(&bootstrap_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bootstrap_witness_set_add(bootstrap_witness_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_bootstrap_witness_set_unref(&bootstrap_witness_set);
}
