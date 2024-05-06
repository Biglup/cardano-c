/**
 * \file ex_units.cpp
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
#include <cardano/common/ex_units.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* EX_UNITS_CBOR = "821b000086788ffc4e831b00015060e9e46451";

/* UNIT TESTS ****************************************************************/

TEST(cardano_ex_units_new, canCreateExUnits)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;

  // Act
  cardano_error_t error = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ex_units, testing::Not((cardano_ex_units_t*)nullptr));

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_new, returnsErrorIfExUnitsIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_units_new(147852369874563U, 369852147852369U, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_units_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_ex_units_t* ex_units = nullptr;

  // Act
  cardano_error_t error = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(ex_units, (cardano_ex_units_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ex_units_to_cbor, canSerializeExUnits)
{
  // Arrange
  cardano_ex_units_t*    ex_units = nullptr;
  cardano_cbor_writer_t* writer   = cardano_cbor_writer_new();

  cardano_error_t error = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ex_units_to_cbor(ex_units, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(EX_UNITS_CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, EX_UNITS_CBOR);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_ex_units_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_ex_units_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_ex_units_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;

  cardano_error_t error = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ex_units_to_cbor(ex_units, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_from_cbor, canDeserializeExUnits)
{
  // Arrange
  cardano_ex_units_t*    ex_units = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(EX_UNITS_CBOR, strlen(EX_UNITS_CBOR));

  // Act
  cardano_error_t error = cardano_ex_units_from_cbor(reader, &ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ex_units, testing::Not((cardano_ex_units_t*)nullptr));

  const uint64_t cpu    = cardano_ex_units_get_cpu_steps(ex_units);
  const uint64_t memory = cardano_ex_units_get_memory(ex_units);

  EXPECT_EQ(cpu, 369852147852369U);
  EXPECT_EQ(memory, 147852369874563U);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_units_from_cbor, returnErrorIfExUnitsIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(EX_UNITS_CBOR, strlen(EX_UNITS_CBOR));

  // Act
  cardano_error_t error = cardano_ex_units_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_units_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;

  // Act
  cardano_error_t error = cardano_ex_units_from_cbor(nullptr, &ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_units_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_ex_units_t*    ex_units = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_ex_units_from_cbor(reader, &ex_units);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding the ex_units, expected a Major Type: Byte String (2) of 2 element(s) but got a Major Type: Byte String (2) of 1 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_units_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotUint)
{
  // Arrange
  cardano_ex_units_t*    ex_units = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("82ff", 4);

  // Act
  cardano_error_t error = cardano_ex_units_from_cbor(reader, &ex_units);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected break byte.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_units_from_cbor, returnErrorIfCborDataSecondElementIsNotUint)
{
  // Arrange
  cardano_ex_units_t*    ex_units = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("8200ff", 4);

  // Act
  cardano_error_t error = cardano_ex_units_from_cbor(reader, &ex_units);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected end of buffer.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_units_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ex_units_ref(ex_units);

  // Assert
  EXPECT_THAT(ex_units, testing::Not((cardano_ex_units_t*)nullptr));
  EXPECT_EQ(cardano_ex_units_refcount(ex_units), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_ex_units_unref(&ex_units);
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ex_units_ref(nullptr);
}

TEST(cardano_ex_units_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;

  // Act
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ex_units_unref((cardano_ex_units_t**)nullptr);
}

TEST(cardano_ex_units_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ex_units_ref(ex_units);
  size_t ref_count = cardano_ex_units_refcount(ex_units);

  cardano_ex_units_unref(&ex_units);
  size_t updated_ref_count = cardano_ex_units_refcount(ex_units);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ex_units_ref(ex_units);
  size_t ref_count = cardano_ex_units_refcount(ex_units);

  cardano_ex_units_unref(&ex_units);
  size_t updated_ref_count = cardano_ex_units_refcount(ex_units);

  cardano_ex_units_unref(&ex_units);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(ex_units, (cardano_ex_units_t*)nullptr);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_ex_units_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_ex_units_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  const char*         message  = "This is a test message";

  // Act
  cardano_ex_units_set_last_error(ex_units, message);

  // Assert
  EXPECT_STREQ(cardano_ex_units_get_last_error(ex_units), "Object is NULL.");
}

TEST(cardano_ex_units_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_ex_units_set_last_error(ex_units, message);

  // Assert
  EXPECT_STREQ(cardano_ex_units_get_last_error(ex_units), "");

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_get_memory, returnsTheMemoryValue)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const uint64_t memory = cardano_ex_units_get_memory(ex_units);

  // Assert
  EXPECT_EQ(memory, 147852369874563U);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_get_memory, returnZeroIfExUnitsIsNull)
{
  // Act
  const uint64_t memory = cardano_ex_units_get_memory(nullptr);

  // Assert
  EXPECT_EQ(memory, 0);
}

TEST(cardano_ex_units_get_cpu_steps, returnsTheCpuStepsValue)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const uint64_t cpu = cardano_ex_units_get_cpu_steps(ex_units);

  // Assert
  EXPECT_EQ(cpu, 369852147852369U);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_get_cpu_steps, returnZeroIfExUnitsIsNull)
{
  // Act
  const uint64_t cpu = cardano_ex_units_get_cpu_steps(nullptr);

  // Assert
  EXPECT_EQ(cpu, 0);
}

TEST(cardano_ex_units_set_memory, setsTheMemoryValue)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ex_units_set_memory(ex_units, 123456789);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_units_get_memory(ex_units), 123456789);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_set_memory, returnErrorIfExUnitsIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_units_set_memory(nullptr, 123456789);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_units_set_cpu_steps, setsTheCpuStepsValue)
{
  // Arrange
  cardano_ex_units_t* ex_units = nullptr;
  cardano_error_t     error    = cardano_ex_units_new(147852369874563U, 369852147852369U, &ex_units);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ex_units_set_cpu_steps(ex_units, 987654321);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_units_get_cpu_steps(ex_units), 987654321);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_ex_units_set_cpu_steps, returnErrorIfExUnitsIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_units_set_cpu_steps(nullptr, 987654321);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}
