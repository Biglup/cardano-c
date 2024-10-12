/**
 * \file vkey_witness_set.cpp
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
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

#include <cardano/witness_set/vkey_witness_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR               = "d90102848258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";
static const char* CBOR_WITHOUT_TAG   = "848258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";
static const char* VKEY_WITNESS1_CBOR = "8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";
static const char* VKEY_WITNESS2_CBOR = "8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";
static const char* VKEY_WITNESS3_CBOR = "8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";
static const char* VKEY_WITNESS4_CBOR = "8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";

/**
 * Creates a new default instance of the vkey_witness.
 * @return A new instance of the vkey_witness.
 */
static cardano_vkey_witness_t*
new_default_vkey_witness(const char* cbor)
{
  cardano_vkey_witness_t* vkey_witness = nullptr;
  cardano_cbor_reader_t*  reader       = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_vkey_witness_from_cbor(reader, &vkey_witness);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_vkey_witness_unref(&vkey_witness);
    return nullptr;
  }

  return vkey_witness;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_vkey_witness_set_new, canCreateCredentialSet)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_vkey_witness_set_new(&vkey_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(vkey_witness_set, testing::Not((cardano_vkey_witness_set_t*)nullptr));

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_new, returnsErrorIfCredentialSetIsNull)
{
  // Act
  cardano_error_t error = cardano_vkey_witness_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_vkey_witness_set_new(&vkey_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(vkey_witness_set, (cardano_vkey_witness_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_vkey_witness_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_vkey_witness_set_new(&vkey_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(vkey_witness_set, (cardano_vkey_witness_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_vkey_witness_set_to_cbor, canSerializeAnEmptyCredentialSet)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_vkey_witness_set_new(&vkey_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_vkey_witness_set_to_cbor(vkey_witness_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_vkey_witness_set_to_cbor, canSerializeCredentialSet)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* vkey_witnesss[] = { VKEY_WITNESS1_CBOR, VKEY_WITNESS2_CBOR, VKEY_WITNESS3_CBOR, VKEY_WITNESS4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_vkey_witness_t* vkey_witness = new_default_vkey_witness(vkey_witnesss[i]);

    EXPECT_EQ(cardano_vkey_witness_set_add(vkey_witness_set, vkey_witness), CARDANO_SUCCESS);

    cardano_vkey_witness_unref(&vkey_witness);
  }

  // Act
  error = cardano_vkey_witness_set_to_cbor(vkey_witness_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a");

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_vkey_witness_set_to_cbor, canSerializeCredentialSetSorted)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* vkey_witnesss[] = { VKEY_WITNESS1_CBOR, VKEY_WITNESS2_CBOR, VKEY_WITNESS3_CBOR, VKEY_WITNESS4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_vkey_witness_t* vkey_witness = new_default_vkey_witness(vkey_witnesss[i]);

    EXPECT_EQ(cardano_vkey_witness_set_add(vkey_witness_set, vkey_witness), CARDANO_SUCCESS);

    cardano_vkey_witness_unref(&vkey_witness);
  }

  // Act
  error = cardano_vkey_witness_set_to_cbor(vkey_witness_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a") + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a");

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_vkey_witness_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_vkey_witness_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_vkey_witness_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  cardano_error_t error = cardano_vkey_witness_set_new(&vkey_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_vkey_witness_set_to_cbor(vkey_witness_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &vkey_witness_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_vkey_witness_set_to_cbor(vkey_witness_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_vkey_witness_set_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*      writer           = cardano_cbor_writer_new();

  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &vkey_witness_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_vkey_witness_set_to_cbor(vkey_witness_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_WITHOUT_TAG) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITHOUT_TAG);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_vkey_witness_set_from_cbor, canDeserializeCredentialSet)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &vkey_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(vkey_witness_set, testing::Not((cardano_vkey_witness_set_t*)nullptr));

  const size_t length = cardano_vkey_witness_set_get_length(vkey_witness_set);

  EXPECT_EQ(length, 4);

  cardano_vkey_witness_t* elem1 = NULL;
  cardano_vkey_witness_t* elem2 = NULL;
  cardano_vkey_witness_t* elem3 = NULL;
  cardano_vkey_witness_t* elem4 = NULL;

  EXPECT_EQ(cardano_vkey_witness_set_get(vkey_witness_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get(vkey_witness_set, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get(vkey_witness_set, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get(vkey_witness_set, 3, &elem4), CARDANO_SUCCESS);

  const char* vkey_witnesss[] = { VKEY_WITNESS1_CBOR, VKEY_WITNESS2_CBOR, VKEY_WITNESS3_CBOR, VKEY_WITNESS4_CBOR };

  cardano_vkey_witness_t* vkey_witnesss_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_vkey_witness_to_cbor(vkey_witnesss_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(vkey_witnesss[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, vkey_witnesss[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_cbor_reader_unref(&reader);

  cardano_vkey_witness_unref(&elem1);
  cardano_vkey_witness_unref(&elem2);
  cardano_vkey_witness_unref(&elem3);
  cardano_vkey_witness_unref(&elem4);
}

TEST(cardano_vkey_witness_set_from_cbor, returnErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vkey_witness_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(nullptr, &vkey_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &vkey_witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(vkey_witness_set, (cardano_vkey_witness_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vkey_witness_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_vkey_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vkey_witness_set_from_cbor, returnErrorIfInvalidRelayElements)
{
  // Arrange
  cardano_vkey_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vkey_witness_set_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_vkey_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vkey_witness_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_vkey_witness_set_t* list   = nullptr;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vkey_witness_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_vkey_witness_set_ref(vkey_witness_set);

  // Assert
  EXPECT_THAT(vkey_witness_set, testing::Not((cardano_vkey_witness_set_t*)nullptr));
  EXPECT_EQ(cardano_vkey_witness_set_refcount(vkey_witness_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_vkey_witness_set_ref(nullptr);
}

TEST(cardano_vkey_witness_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  // Act
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_vkey_witness_set_unref((cardano_vkey_witness_set_t**)nullptr);
}

TEST(cardano_vkey_witness_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_vkey_witness_set_ref(vkey_witness_set);
  size_t ref_count = cardano_vkey_witness_set_refcount(vkey_witness_set);

  cardano_vkey_witness_set_unref(&vkey_witness_set);
  size_t updated_ref_count = cardano_vkey_witness_set_refcount(vkey_witness_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_vkey_witness_set_ref(vkey_witness_set);
  size_t ref_count = cardano_vkey_witness_set_refcount(vkey_witness_set);

  cardano_vkey_witness_set_unref(&vkey_witness_set);
  size_t updated_ref_count = cardano_vkey_witness_set_refcount(vkey_witness_set);

  cardano_vkey_witness_set_unref(&vkey_witness_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(vkey_witness_set, (cardano_vkey_witness_set_t*)nullptr);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_vkey_witness_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_vkey_witness_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  const char*                 message          = "This is a test message";

  // Act
  cardano_vkey_witness_set_set_last_error(vkey_witness_set, message);

  // Assert
  EXPECT_STREQ(cardano_vkey_witness_set_get_last_error(vkey_witness_set), "Object is NULL.");
}

TEST(cardano_vkey_witness_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_vkey_witness_set_set_last_error(vkey_witness_set, message);

  // Assert
  EXPECT_STREQ(cardano_vkey_witness_set_get_last_error(vkey_witness_set), "");

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_get_length, returnsZeroIfCredentialSetIsNull)
{
  // Act
  size_t length = cardano_vkey_witness_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_vkey_witness_set_get_length, returnsZeroIfCredentialSetIsEmpty)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_vkey_witness_set_get_length(vkey_witness_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_get, returnsErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_vkey_witness_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_vkey_witness_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_vkey_witness_set_get(vkey_witness_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_vkey_witness_t* data = nullptr;
  error                        = cardano_vkey_witness_set_get(vkey_witness_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_add, returnsErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_vkey_witness_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_vkey_witness_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_vkey_witness_set_add(vkey_witness_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_set_use_tag, canSetUseTag)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_vkey_witness_set_set_use_tag(vkey_witness_set, true), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get_use_tag(vkey_witness_set), true);

  EXPECT_EQ(cardano_vkey_witness_set_set_use_tag(vkey_witness_set, false), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get_use_tag(vkey_witness_set), false);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_vkey_witness_set_set_use_tag, returnsErrorIfGivenNull)
{
  // Act
  EXPECT_EQ(cardano_vkey_witness_set_set_use_tag(nullptr, true), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_get_set_use_tag, returnsFalseIfGivenNull)
{
  // Act
  EXPECT_EQ(cardano_vkey_witness_set_get_use_tag(nullptr), false);
}

TEST(cardano_vkey_witness_set_add, replaceSignatureIfElementAlreadyExists)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_error_t             error            = cardano_vkey_witness_set_new(&vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_vkey_witness_t* vkey_witness1 = new_default_vkey_witness(VKEY_WITNESS1_CBOR);
  cardano_vkey_witness_t* vkey_witness2 = new_default_vkey_witness(VKEY_WITNESS2_CBOR);

  EXPECT_EQ(cardano_vkey_witness_set_add(vkey_witness_set, vkey_witness1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_add(vkey_witness_set, vkey_witness2), CARDANO_SUCCESS);

  // Act
  cardano_vkey_witness_t* vkey_witness3 = new_default_vkey_witness(VKEY_WITNESS3_CBOR);
  EXPECT_EQ(cardano_vkey_witness_set_add(vkey_witness_set, vkey_witness3), CARDANO_SUCCESS);

  // Assert
  cardano_vkey_witness_t* elem1 = NULL;

  EXPECT_EQ(cardano_vkey_witness_set_get_length(vkey_witness_set), 1);

  EXPECT_EQ(cardano_vkey_witness_set_get(vkey_witness_set, 0, &elem1), CARDANO_SUCCESS);

  const char* vkey_witnesss[] = { VKEY_WITNESS1_CBOR };

  cardano_vkey_witness_t* vkey_witnesss_array[] = { elem1 };

  for (size_t i = 0; i < 1; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_vkey_witness_to_cbor(vkey_witnesss_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

    EXPECT_EQ(error, CARDANO_SUCCESS);
    EXPECT_STREQ(actual_cbor, vkey_witnesss[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_vkey_witness_unref(&vkey_witness1);
  cardano_vkey_witness_unref(&vkey_witness2);
  cardano_vkey_witness_unref(&vkey_witness3);
  cardano_vkey_witness_unref(&elem1);
}

TEST(cardano_vkey_witness_set_apply, canApplyVkeyWitness)
{
  // Arrange
  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_vkey_witness_set_t* vkey_witness_set_new = nullptr;
  cardano_cbor_reader_t*      reader2              = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_vkey_witness_set_from_cbor(reader, &vkey_witness_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_vkey_witness_set_from_cbor(reader2, &vkey_witness_set_new);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_vkey_witness_set_apply(vkey_witness_set, vkey_witness_set_new);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_vkey_witness_set_unref(&vkey_witness_set_new);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_reader_unref(&reader2);
}

TEST(cardano_vkey_witness_set_apply, returnsErrorIfGivenANullPtr)
{
  // Act
  cardano_error_t error = cardano_vkey_witness_set_apply(nullptr, (cardano_vkey_witness_set_t*)"");

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_vkey_witness_set_apply, returnsErrorIfGivenANullPtr2)
{
  // Act
  cardano_error_t error = cardano_vkey_witness_set_apply((cardano_vkey_witness_set_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}