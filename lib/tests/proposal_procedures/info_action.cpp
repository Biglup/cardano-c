/**
 * \file info_action.cpp
 *
 * \author angel.castillo
 * \date   Aug 31, 2024
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
#include <cardano/proposal_procedures/info_action.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "8106";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the info_action.
 * @return A new instance of the info_action.
 */
static cardano_info_action_t*
new_default_info_action()
{
  cardano_info_action_t* info_action = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t        result      = cardano_info_action_from_cbor(reader, &info_action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return info_action;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_info_action_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_info_action_t* info_action = new_default_info_action();
  EXPECT_NE(info_action, nullptr);

  // Act
  cardano_info_action_ref(info_action);

  // Assert
  EXPECT_THAT(info_action, testing::Not((cardano_info_action_t*)nullptr));
  EXPECT_EQ(cardano_info_action_refcount(info_action), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_info_action_unref(&info_action);
  cardano_info_action_unref(&info_action);
}

TEST(cardano_info_action_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_info_action_ref(nullptr);
}

TEST(cardano_info_action_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_info_action_t* info_action = nullptr;

  // Act
  cardano_info_action_unref(&info_action);
}

TEST(cardano_info_action_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_info_action_unref((cardano_info_action_t**)nullptr);
}

TEST(cardano_info_action_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_info_action_t* info_action = new_default_info_action();
  EXPECT_NE(info_action, nullptr);

  // Act
  cardano_info_action_ref(info_action);
  size_t ref_count = cardano_info_action_refcount(info_action);

  cardano_info_action_unref(&info_action);
  size_t updated_ref_count = cardano_info_action_refcount(info_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_info_action_unref(&info_action);
}

TEST(cardano_info_action_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_info_action_t* info_action = new_default_info_action();
  EXPECT_NE(info_action, nullptr);

  // Act
  cardano_info_action_ref(info_action);
  size_t ref_count = cardano_info_action_refcount(info_action);

  cardano_info_action_unref(&info_action);
  size_t updated_ref_count = cardano_info_action_refcount(info_action);

  cardano_info_action_unref(&info_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(info_action, (cardano_info_action_t*)nullptr);

  // Cleanup
  cardano_info_action_unref(&info_action);
}

TEST(cardano_info_action_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_info_action_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_info_action_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_info_action_t* info_action = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_info_action_set_last_error(info_action, message);

  // Assert
  EXPECT_STREQ(cardano_info_action_get_last_error(info_action), "Object is NULL.");
}

TEST(cardano_info_action_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_info_action_t* info_action = new_default_info_action();
  EXPECT_NE(info_action, nullptr);

  const char* message = nullptr;

  // Act
  cardano_info_action_set_last_error(info_action, message);

  // Assert
  EXPECT_STREQ(cardano_info_action_get_last_error(info_action), "");

  // Cleanup
  cardano_info_action_unref(&info_action);
}

TEST(cardano_info_action_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_info_action_t* info_action = NULL;

  // Act
  cardano_error_t result = cardano_info_action_from_cbor(nullptr, &info_action);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_info_action_from_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_info_action_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_info_action_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_info_action_t* cert   = new_default_info_action();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_info_action_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_info_action_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_info_action_to_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_info_action_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_info_action_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_info_action_to_cbor((cardano_info_action_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Action specific tests

TEST(cardano_info_action_new, returnsErrorIfActionIsNull)
{
  // Act
  cardano_error_t result = cardano_info_action_new(NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_info_action_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_info_action_t* info_action = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_info_action_new(&info_action);

  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_info_action_unref(&info_action);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_info_action_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_info_action_t* info_action = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_info_action_from_cbor(reader, &info_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_info_action_unref(&info_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_info_action_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_info_action_t* info_action = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("8300", strlen("8300"));

  // Act
  cardano_error_t result = cardano_info_action_from_cbor(reader, &info_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_info_action_unref(&info_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_info_action_from_cbor, returnsErrorIfInvalidId)
{
  // Arrange
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("81ef", strlen("8101"));
  cardano_info_action_t* info_action = NULL;

  // Act
  cardano_error_t result = cardano_info_action_from_cbor(reader, &info_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}
