/**
 * \file native_script_set.cpp
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

#include <cardano/witness_set/native_script_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                = "d90102848200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* CBOR_WITHOUT_TAG    = "848200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* NATIVE_SCRIPT1_CBOR = "8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* NATIVE_SCRIPT2_CBOR = "8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* NATIVE_SCRIPT3_CBOR = "8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* NATIVE_SCRIPT4_CBOR = "8200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";

/**
 * Creates a new default instance of the native_script.
 * @return A new instance of the native_script.
 */
static cardano_native_script_t*
new_default_native_script(const char* cbor)
{
  cardano_native_script_t* native_script = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_native_script_from_cbor(reader, &native_script);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_native_script_unref(&native_script);
    return nullptr;
  }

  return native_script;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_native_script_set_new, canCreateCredentialSet)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;

  // Act
  cardano_error_t error = cardano_native_script_set_new(&native_script_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(native_script_set, testing::Not((cardano_native_script_set_t*)nullptr));

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_new, returnsErrorIfCredentialSetIsNull)
{
  // Act
  cardano_error_t error = cardano_native_script_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_native_script_set_t* native_script_set = nullptr;

  // Act
  cardano_error_t error = cardano_native_script_set_new(&native_script_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(native_script_set, (cardano_native_script_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_native_script_set_t* native_script_set = nullptr;

  // Act
  cardano_error_t error = cardano_native_script_set_new(&native_script_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(native_script_set, (cardano_native_script_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_native_script_set_to_cbor, canSerializeAnEmptyCredentialSet)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_cbor_writer_t*       writer            = cardano_cbor_writer_new();

  cardano_error_t error = cardano_native_script_set_new(&native_script_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_native_script_set_to_cbor(native_script_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 9);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_native_script_set_to_cbor, canSerializeCredentialSet)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_cbor_writer_t*       writer            = cardano_cbor_writer_new();

  cardano_error_t error = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* native_scripts[] = { NATIVE_SCRIPT1_CBOR, NATIVE_SCRIPT2_CBOR, NATIVE_SCRIPT3_CBOR, NATIVE_SCRIPT4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_native_script_t* native_script = new_default_native_script(native_scripts[i]);

    EXPECT_EQ(cardano_native_script_set_add(native_script_set, native_script), CARDANO_SUCCESS);

    cardano_native_script_unref(&native_script);
  }

  // Act
  error = cardano_native_script_set_to_cbor(native_script_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_native_script_set_to_cbor, canSerializeCredentialSetSorted)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_cbor_writer_t*       writer            = cardano_cbor_writer_new();

  cardano_error_t error = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* native_scripts[] = { NATIVE_SCRIPT1_CBOR, NATIVE_SCRIPT2_CBOR, NATIVE_SCRIPT3_CBOR, NATIVE_SCRIPT4_CBOR };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_native_script_t* native_script = new_default_native_script(native_scripts[i]);

    EXPECT_EQ(cardano_native_script_set_add(native_script_set, native_script), CARDANO_SUCCESS);

    cardano_native_script_unref(&native_script);
  }

  // Act
  error = cardano_native_script_set_to_cbor(native_script_set, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_native_script_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_native_script_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_native_script_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;

  cardano_error_t error = cardano_native_script_set_new(&native_script_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_native_script_set_to_cbor(native_script_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*       writer            = cardano_cbor_writer_new();

  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &native_script_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_set_to_cbor(native_script_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_native_script_set_to_cbor, canDeserializeAndReserializeCborWithoutTag)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR_WITHOUT_TAG, strlen(CBOR_WITHOUT_TAG));
  cardano_cbor_writer_t*       writer            = cardano_cbor_writer_new();

  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &native_script_set);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_native_script_set_to_cbor(native_script_set, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_WITHOUT_TAG) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITHOUT_TAG);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_native_script_set_from_cbor, canDeserializeCredentialSet)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &native_script_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(native_script_set, testing::Not((cardano_native_script_set_t*)nullptr));

  const size_t length = cardano_native_script_set_get_length(native_script_set);

  EXPECT_EQ(length, 4);

  cardano_native_script_t* elem1 = NULL;
  cardano_native_script_t* elem2 = NULL;
  cardano_native_script_t* elem3 = NULL;
  cardano_native_script_t* elem4 = NULL;

  EXPECT_EQ(cardano_native_script_set_get(native_script_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_set_get(native_script_set, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_set_get(native_script_set, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_set_get(native_script_set, 3, &elem4), CARDANO_SUCCESS);

  const char* native_scripts[] = { NATIVE_SCRIPT1_CBOR, NATIVE_SCRIPT2_CBOR, NATIVE_SCRIPT3_CBOR, NATIVE_SCRIPT4_CBOR };

  cardano_native_script_t* native_scripts_array[] = { elem1, elem2, elem3, elem4 };

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

    error = cardano_native_script_to_cbor(native_scripts_array[i], writer);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
    EXPECT_EQ(hex_size, strlen(native_scripts[i]) + 1);

    char* actual_cbor = (char*)malloc(hex_size);

    error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(actual_cbor, native_scripts[i]);

    cardano_cbor_writer_unref(&writer);
    free(actual_cbor);
  }

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
  cardano_cbor_reader_unref(&reader);

  cardano_native_script_unref(&elem1);
  cardano_native_script_unref(&elem2);
  cardano_native_script_unref(&elem3);
  cardano_native_script_unref(&elem4);
}

TEST(cardano_native_script_set_from_cbor, returnErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(nullptr, &native_script_set);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &native_script_set);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(native_script_set, (cardano_native_script_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_native_script_set_t* list   = nullptr;
  cardano_cbor_reader_t*       reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_set_from_cbor, returnErrorIfInvalidRelayElements)
{
  // Arrange
  cardano_native_script_set_t* list   = nullptr;
  cardano_cbor_reader_t*       reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_set_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_native_script_set_t* list   = nullptr;
  cardano_cbor_reader_t*       reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_native_script_set_t* list   = nullptr;
  cardano_cbor_reader_t*       reader = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_native_script_set_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_native_script_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_set_ref(native_script_set);

  // Assert
  EXPECT_THAT(native_script_set, testing::Not((cardano_native_script_set_t*)nullptr));
  EXPECT_EQ(cardano_native_script_set_refcount(native_script_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_native_script_set_unref(&native_script_set);
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_native_script_set_ref(nullptr);
}

TEST(cardano_native_script_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;

  // Act
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_native_script_set_unref((cardano_native_script_set_t**)nullptr);
}

TEST(cardano_native_script_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_set_ref(native_script_set);
  size_t ref_count = cardano_native_script_set_refcount(native_script_set);

  cardano_native_script_set_unref(&native_script_set);
  size_t updated_ref_count = cardano_native_script_set_refcount(native_script_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_set_ref(native_script_set);
  size_t ref_count = cardano_native_script_set_refcount(native_script_set);

  cardano_native_script_set_unref(&native_script_set);
  size_t updated_ref_count = cardano_native_script_set_refcount(native_script_set);

  cardano_native_script_set_unref(&native_script_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(native_script_set, (cardano_native_script_set_t*)nullptr);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_native_script_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_native_script_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  const char*                  message           = "This is a test message";

  // Act
  cardano_native_script_set_set_last_error(native_script_set, message);

  // Assert
  EXPECT_STREQ(cardano_native_script_set_get_last_error(native_script_set), "Object is NULL.");
}

TEST(cardano_native_script_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_native_script_set_set_last_error(native_script_set, message);

  // Assert
  EXPECT_STREQ(cardano_native_script_set_get_last_error(native_script_set), "");

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_get_length, returnsZeroIfCredentialSetIsNull)
{
  // Act
  size_t length = cardano_native_script_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_native_script_set_get_length, returnsZeroIfCredentialSetIsEmpty)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_native_script_set_get_length(native_script_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_get, returnsErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_native_script_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_native_script_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_native_script_set_get(native_script_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_native_script_t* data = nullptr;
  error                         = cardano_native_script_set_get(native_script_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_add, returnsErrorIfCredentialSetIsNull)
{
  // Arrange
  cardano_native_script_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_native_script_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_native_script_set_add(native_script_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_set_use_tag, canSetUseTag)
{
  // Arrange
  cardano_native_script_set_t* native_script_set = nullptr;
  cardano_error_t              error             = cardano_native_script_set_new(&native_script_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_native_script_set_set_use_tag(native_script_set, true), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_set_get_use_tag(native_script_set), true);

  EXPECT_EQ(cardano_native_script_set_set_use_tag(native_script_set, false), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_set_get_use_tag(native_script_set), false);

  // Cleanup
  cardano_native_script_set_unref(&native_script_set);
}

TEST(cardano_native_script_set_set_use_tag, returnsErrorIfGivenNull)
{
  // Act
  EXPECT_EQ(cardano_native_script_set_set_use_tag(nullptr, true), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_native_script_get_set_use_tag, returnsFalseIfGivenNull)
{
  // Act
  EXPECT_EQ(cardano_native_script_set_get_use_tag(nullptr), false);
}
