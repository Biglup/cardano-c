/**
 * \file unit_interval.cpp
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
#include <cardano/common/unit_interval.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PROTOCOL_VERSION_CBOR = "d81e820105";

/* UNIT TESTS ****************************************************************/

TEST(cardano_unit_interval_new, canCreateProtocolVersion)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;

  // Act
  cardano_error_t error = cardano_unit_interval_new(1, 5, &unit_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(unit_interval, testing::Not((cardano_unit_interval_t*)nullptr));

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_new, returnsErrorIfProtocolVersionIsNull)
{
  // Act
  cardano_error_t error = cardano_unit_interval_new(1, 5, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_unit_interval_new, returnsErrorIfdenominatorAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_unit_interval_t* unit_interval = nullptr;

  // Act
  cardano_error_t error = cardano_unit_interval_new(1, 5, &unit_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(unit_interval, (cardano_unit_interval_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_unit_interval_to_cbor, canSerializeProtocolVersion)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_cbor_writer_t*   writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_unit_interval_to_cbor(unit_interval, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(PROTOCOL_VERSION_CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, PROTOCOL_VERSION_CBOR);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_unit_interval_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_unit_interval_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_unit_interval_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;

  cardano_error_t error = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_unit_interval_to_cbor(unit_interval, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_from_cbor, canDeserializeProtocolVersion)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex(PROTOCOL_VERSION_CBOR, strlen(PROTOCOL_VERSION_CBOR));

  // Act
  cardano_error_t error = cardano_unit_interval_from_cbor(reader, &unit_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(unit_interval, testing::Not((cardano_unit_interval_t*)nullptr));

  const uint64_t numerator   = cardano_unit_interval_get_numerator(unit_interval);
  const uint64_t denominator = cardano_unit_interval_get_denominator(unit_interval);

  EXPECT_EQ(numerator, 1);
  EXPECT_EQ(denominator, 5);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unit_interval_from_cbor, returnErrorIfProtocolVersionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PROTOCOL_VERSION_CBOR, strlen(PROTOCOL_VERSION_CBOR));

  // Act
  cardano_error_t error = cardano_unit_interval_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unit_interval_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;

  // Act
  cardano_error_t error = cardano_unit_interval_from_cbor(nullptr, &unit_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_unit_interval_from_cbor, returnErrorIfCborDataIsMissingTheTag)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_unit_interval_from_cbor(reader, &unit_interval);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding the unit_interval, expected Reader State: Tag (13) but got Reader State: Start Array (9).");
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unit_interval_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("d81e850105", 10);

  // Act
  cardano_error_t error = cardano_unit_interval_from_cbor(reader, &unit_interval);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding the unit_interval, expected a Major Type: Byte String (2) of 2 element(s) but got a Major Type: Byte String (2) of 5 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unit_interval_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotUint)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("d81e82ff05", 10);

  // Act
  cardano_error_t error = cardano_unit_interval_from_cbor(reader, &unit_interval);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected break byte.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unit_interval_from_cbor, returnErrorIfCborDataSecondElementIsNotUint)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_cbor_reader_t*   reader        = cardano_cbor_reader_from_hex("d81e8201fe", 10);

  // Act
  cardano_error_t error = cardano_unit_interval_from_cbor(reader, &unit_interval);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding the unit_interval, expected Reader State: Unsigned Integer (1) but got Reader State: Simple Value (14).");
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unit_interval_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_unit_interval_ref(unit_interval);

  // Assert
  EXPECT_THAT(unit_interval, testing::Not((cardano_unit_interval_t*)nullptr));
  EXPECT_EQ(cardano_unit_interval_refcount(unit_interval), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_unit_interval_unref(&unit_interval);
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_unit_interval_ref(nullptr);
}

TEST(cardano_unit_interval_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;

  // Act
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_unit_interval_unref((cardano_unit_interval_t**)nullptr);
}

TEST(cardano_unit_interval_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_unit_interval_ref(unit_interval);
  size_t ref_count = cardano_unit_interval_refcount(unit_interval);

  cardano_unit_interval_unref(&unit_interval);
  size_t updated_ref_count = cardano_unit_interval_refcount(unit_interval);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_unit_interval_ref(unit_interval);
  size_t ref_count = cardano_unit_interval_refcount(unit_interval);

  cardano_unit_interval_unref(&unit_interval);
  size_t updated_ref_count = cardano_unit_interval_refcount(unit_interval);

  cardano_unit_interval_unref(&unit_interval);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(unit_interval, (cardano_unit_interval_t*)nullptr);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_unit_interval_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_unit_interval_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_unit_interval_set_last_error(unit_interval, message);

  // Assert
  EXPECT_STREQ(cardano_unit_interval_get_last_error(unit_interval), "Object is NULL.");
}

TEST(cardano_unit_interval_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_unit_interval_set_last_error(unit_interval, message);

  // Assert
  EXPECT_STREQ(cardano_unit_interval_get_last_error(unit_interval), "");

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_get_denominator, returnsThedenominatorValue)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const uint64_t denominator = cardano_unit_interval_get_denominator(unit_interval);

  // Assert
  EXPECT_EQ(denominator, 5);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_get_denominator, returnZeroIfProtocolVersionIsNull)
{
  // Act
  const uint64_t denominator = cardano_unit_interval_get_denominator(nullptr);

  // Assert
  EXPECT_EQ(denominator, 0);
}

TEST(cardano_unit_interval_get_numerator, returnsThenumeratorStepsValue)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const uint64_t numerator = cardano_unit_interval_get_numerator(unit_interval);

  // Assert
  EXPECT_EQ(numerator, 1);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_get_numerator, returnZeroIfProtocolVersionIsNull)
{
  // Act
  const uint64_t numerator = cardano_unit_interval_get_numerator(nullptr);

  // Assert
  EXPECT_EQ(numerator, 0);
}

TEST(cardano_unit_interval_set_denominator, setsTheDenominatorValue)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_unit_interval_set_denominator(unit_interval, 123456789);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_get_denominator(unit_interval), 123456789);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_set_denominator, returnErrorIfProtocolVersionIsNull)
{
  // Act
  cardano_error_t error = cardano_unit_interval_set_denominator(nullptr, 123456789);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_unit_interval_set_numerator, setsTheNumeratorStepsValue)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_unit_interval_set_numerator(unit_interval, 987654321);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_get_numerator(unit_interval), 987654321);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_set_numerator, returnErrorIfProtocolVersionIsNull)
{
  // Act
  cardano_error_t error = cardano_unit_interval_set_numerator(nullptr, 987654321);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_unit_interval_to_double, returnsTheDoubleValue)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;
  cardano_error_t          error         = cardano_unit_interval_new(1, 5, &unit_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const double value = cardano_unit_interval_to_double(unit_interval);

  // Assert
  EXPECT_DOUBLE_EQ(value, 0.2);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_to_double, returnZeroIfProtocolVersionIsNull)
{
  // Act
  const double value = cardano_unit_interval_to_double(nullptr);

  // Assert
  EXPECT_DOUBLE_EQ(value, 0.0);
}

TEST(cardano_unit_interval_from_double, setsTheDoubleValue)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;

  // Act
  cardano_error_t error = cardano_unit_interval_from_double(0.2, &unit_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(unit_interval, testing::Not((cardano_unit_interval_t*)nullptr));
  EXPECT_EQ(cardano_unit_interval_get_numerator(unit_interval), 1);
  EXPECT_EQ(cardano_unit_interval_get_denominator(unit_interval), 5);

  // Cleanup
  cardano_unit_interval_unref(&unit_interval);
}

TEST(cardano_unit_interval_from_double, returnErrorIfValueIsNegative)
{
  // Arrange
  cardano_unit_interval_t* unit_interval = nullptr;

  // Act
  cardano_error_t error = cardano_unit_interval_from_double(-0.2, &unit_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(unit_interval, (cardano_unit_interval_t*)nullptr);
}