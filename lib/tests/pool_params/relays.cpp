/**
 * \file relays.cpp
 *
 * \author angel.castillo
 * \date   Jun 28, 2024
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

#include <cardano/pool_params/relays.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/pool_params/single_host_name_relay.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "858301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d8301f66b6578616d706c652e636f6d";

/* UNIT TESTS ****************************************************************/

TEST(cardano_relays_new, canCreateRelays)
{
  // Arrange
  cardano_relays_t* relays = nullptr;

  // Act
  cardano_error_t error = cardano_relays_new(&relays);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(relays, testing::Not((cardano_relays_t*)nullptr));

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_new, returnsErrorIfRelaysIsNull)
{
  // Act
  cardano_error_t error = cardano_relays_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_relays_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_relays_t* relays = nullptr;

  // Act
  cardano_error_t error = cardano_relays_new(&relays);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(relays, (cardano_relays_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_relays_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_relays_t* relays = nullptr;

  // Act
  cardano_error_t error = cardano_relays_new(&relays);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(relays, (cardano_relays_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_relays_to_cbor, canSerializeAnEmptyRelays)
{
  // Arrange
  cardano_relays_t*      relays = nullptr;
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_relays_new(&relays);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_relays_to_cbor(relays, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "80");

  // Cleanup
  cardano_relays_unref(&relays);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_relays_to_cbor, canSerializeRelays)
{
  // Arrange
  cardano_relays_t*      relays = nullptr;
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_relays_new(&relays);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  for (size_t i = 0; i < 5; ++i)
  {
    cardano_single_host_name_relay_t* data  = nullptr;
    cardano_relay_t*                  relay = NULL;

    EXPECT_EQ(cardano_single_host_name_relay_new(NULL, "example.com", strlen("example.com"), &data), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_relay_new_single_host_name(data, &relay), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_relays_add(relays, relay), CARDANO_SUCCESS);

    cardano_relay_unref(&relay);
    cardano_single_host_name_relay_unref(&data);
  }

  // Act
  error = cardano_relays_to_cbor(relays, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_relays_unref(&relays);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_relays_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_relays_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_relays_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_relays_t* relays = nullptr;

  cardano_error_t error = cardano_relays_new(&relays);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_relays_to_cbor(relays, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_relays_t*      relays = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_relays_from_cbor(reader, &relays);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_relays_to_cbor(relays, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_relays_unref(&relays);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_relays_from_cbor, canDeserializeRelays)
{
  // Arrange
  cardano_relays_t*      relays = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_relays_from_cbor(reader, &relays);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(relays, testing::Not((cardano_relays_t*)nullptr));

  const size_t length = cardano_relays_get_length(relays);

  EXPECT_EQ(length, 5);

  cardano_relay_t* elem1 = NULL;
  cardano_relay_t* elem2 = NULL;
  cardano_relay_t* elem3 = NULL;
  cardano_relay_t* elem4 = NULL;
  cardano_relay_t* elem5 = NULL;

  EXPECT_EQ(cardano_relays_get(relays, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_relays_get(relays, 1, &elem2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_relays_get(relays, 2, &elem3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_relays_get(relays, 3, &elem4), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_relays_get(relays, 4, &elem5), CARDANO_SUCCESS);

  cardano_relay_type_t type = CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS;

  EXPECT_EQ(cardano_relay_get_type(elem1, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_RELAY_TYPE_SINGLE_HOST_NAME);

  EXPECT_EQ(cardano_relay_get_type(elem2, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_RELAY_TYPE_SINGLE_HOST_NAME);

  EXPECT_EQ(cardano_relay_get_type(elem3, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_RELAY_TYPE_SINGLE_HOST_NAME);

  EXPECT_EQ(cardano_relay_get_type(elem4, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_RELAY_TYPE_SINGLE_HOST_NAME);

  EXPECT_EQ(cardano_relay_get_type(elem5, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_RELAY_TYPE_SINGLE_HOST_NAME);

  // Cleanup
  cardano_relays_unref(&relays);
  cardano_cbor_reader_unref(&reader);

  cardano_relay_unref(&elem1);
  cardano_relay_unref(&elem2);
  cardano_relay_unref(&elem3);
  cardano_relay_unref(&elem4);
  cardano_relay_unref(&elem5);
}

TEST(cardano_relays_from_cbor, returnErrorIfRelaysIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_relays_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relays_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_relays_t* relays = nullptr;

  // Act
  cardano_error_t error = cardano_relays_from_cbor(nullptr, &relays);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_relays_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_relays_t*      relays = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_relays_from_cbor(reader, &relays);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(relays, (cardano_relays_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relays_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_relays_t*      list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_relays_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relays_from_cbor, returnErrorIfInvalidRelayElements)
{
  // Arrange
  cardano_relays_t*      list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9ffeff", 6);

  // Act
  cardano_error_t error = cardano_relays_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relays_from_cbor, returnErrorIfMissingEndArray)
{
  // Arrange
  cardano_relays_t*      list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("9f01", 4);

  // Act
  cardano_error_t error = cardano_relays_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relays_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_relays_ref(relays);

  // Assert
  EXPECT_THAT(relays, testing::Not((cardano_relays_t*)nullptr));
  EXPECT_EQ(cardano_relays_refcount(relays), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_relays_unref(&relays);
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_relays_ref(nullptr);
}

TEST(cardano_relays_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_relays_t* relays = nullptr;

  // Act
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_relays_unref((cardano_relays_t**)nullptr);
}

TEST(cardano_relays_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_relays_ref(relays);
  size_t ref_count = cardano_relays_refcount(relays);

  cardano_relays_unref(&relays);
  size_t updated_ref_count = cardano_relays_refcount(relays);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_relays_ref(relays);
  size_t ref_count = cardano_relays_refcount(relays);

  cardano_relays_unref(&relays);
  size_t updated_ref_count = cardano_relays_refcount(relays);

  cardano_relays_unref(&relays);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(relays, (cardano_relays_t*)nullptr);

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_relays_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_relays_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_relays_t* relays  = nullptr;
  const char*       message = "This is a test message";

  // Act
  cardano_relays_set_last_error(relays, message);

  // Assert
  EXPECT_STREQ(cardano_relays_get_last_error(relays), "Object is NULL.");
}

TEST(cardano_relays_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_relays_set_last_error(relays, message);

  // Assert
  EXPECT_STREQ(cardano_relays_get_last_error(relays), "");

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_get_length, returnsZeroIfRelaysIsNull)
{
  // Act
  size_t length = cardano_relays_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_relays_get_length, returnsZeroIfRelaysIsEmpty)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_relays_get_length(relays);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_get, returnsErrorIfRelaysIsNull)
{
  // Arrange
  cardano_relay_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_relays_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_relays_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_relays_get(relays, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_relay_t* data = nullptr;
  error                 = cardano_relays_get(relays, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_relays_unref(&relays);
}

TEST(cardano_relays_add, returnsErrorIfRelaysIsNull)
{
  // Arrange
  cardano_relay_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_relays_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_relays_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_relays_t* relays = nullptr;
  cardano_error_t   error  = cardano_relays_new(&relays);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_relays_add(relays, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_relays_unref(&relays);
}
