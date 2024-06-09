/**
 * \file ex_unit_prices.cpp
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#include <cardano/protocol_params/ex_unit_prices.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "82d81e820102d81e820103";

/* UNIT TESTS ****************************************************************/

TEST(cardano_ex_unit_prices_new, canCreateExUnitPrices)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ex_unit_prices, testing::Not((cardano_ex_unit_prices_t*)nullptr));

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_new, returnsErrorIfExUnitPricesIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_unit_prices_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_new, returnsErrorIfMemoryAllocationFails)
{
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(ex_unit_prices, (cardano_ex_unit_prices_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_to_cbor, canSerializeExUnitPrices)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_to_cbor(ex_unit_prices, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_ex_unit_prices_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_ex_unit_prices_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_ex_unit_prices_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_to_cbor(ex_unit_prices, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_from_cbor, canDeserializeExUnitPrices)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_ex_unit_prices_from_cbor(reader, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ex_unit_prices, testing::Not((cardano_ex_unit_prices_t*)nullptr));

  cardano_unit_interval_t* memory_prices = nullptr;
  cardano_unit_interval_t* steps_prices  = nullptr;

  EXPECT_EQ(cardano_ex_unit_prices_get_memory_prices(ex_unit_prices, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_get_steps_prices(ex_unit_prices, &steps_prices), CARDANO_SUCCESS);

  EXPECT_NEAR(cardano_unit_interval_to_double(memory_prices), 0.5, 0.01);
  EXPECT_NEAR(cardano_unit_interval_to_double(steps_prices), 0.33, 0.01);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_unit_prices_from_cbor, returnErrorIfExUnitPricesIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_ex_unit_prices_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_unit_prices_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;

  // Act
  cardano_error_t error = cardano_ex_unit_prices_from_cbor(nullptr, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_from_cbor(reader, &ex_unit_prices);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'ex_unit_prices', expected a 'Major Type: Byte String' (2) of 2 element(s) but got a 'Major Type: Byte String' (2) of 1 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_unit_prices_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotUint)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("82ff", 4);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_from_cbor(reader, &ex_unit_prices);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected break byte.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_unit_prices_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  // Act
  cardano_ex_unit_prices_ref(ex_unit_prices);

  // Assert
  EXPECT_THAT(ex_unit_prices, testing::Not((cardano_ex_unit_prices_t*)nullptr));
  EXPECT_EQ(cardano_ex_unit_prices_refcount(ex_unit_prices), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ex_unit_prices_ref(nullptr);
}

TEST(cardano_ex_unit_prices_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;

  // Act
  cardano_ex_unit_prices_unref(&ex_unit_prices);
}

TEST(cardano_ex_unit_prices_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ex_unit_prices_unref((cardano_ex_unit_prices_t**)nullptr);
}

TEST(cardano_ex_unit_prices_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  // Act
  cardano_ex_unit_prices_ref(ex_unit_prices);
  size_t ref_count = cardano_ex_unit_prices_refcount(ex_unit_prices);

  cardano_ex_unit_prices_unref(&ex_unit_prices);
  size_t updated_ref_count = cardano_ex_unit_prices_refcount(ex_unit_prices);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  // Act
  cardano_ex_unit_prices_ref(ex_unit_prices);
  size_t ref_count = cardano_ex_unit_prices_refcount(ex_unit_prices);

  cardano_ex_unit_prices_unref(&ex_unit_prices);
  size_t updated_ref_count = cardano_ex_unit_prices_refcount(ex_unit_prices);

  cardano_ex_unit_prices_unref(&ex_unit_prices);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(ex_unit_prices, (cardano_ex_unit_prices_t*)nullptr);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_ex_unit_prices_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_ex_unit_prices_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  const char*               message        = "This is a test message";

  // Act
  cardano_ex_unit_prices_set_last_error(ex_unit_prices, message);

  // Assert
  EXPECT_STREQ(cardano_ex_unit_prices_get_last_error(ex_unit_prices), "Object is NULL.");
}

TEST(cardano_ex_unit_prices_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_ex_unit_prices_set_last_error(ex_unit_prices, message);

  // Assert
  EXPECT_STREQ(cardano_ex_unit_prices_get_last_error(ex_unit_prices), "");

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_new, returnsErrorIfMemoryIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_new(nullptr, steps_prices, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_new, returnsErrorIfStepsIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_new(memory_prices, nullptr, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&memory_prices);
}

TEST(cardano_ex_unit_prices_from_cbor, returnsErrorIfMemoryIsInvalid)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("82d81ea20102d81e820103", strlen("82d81e820102d81e820103"));

  // Act
  cardano_error_t error = cardano_ex_unit_prices_from_cbor(reader, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_unit_prices_from_cbor, returnsErrorIfStepsIsInvalid)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("82d81e820102d81ea20103", strlen("82d81e820102d81ea20103"));

  // Act
  cardano_error_t error = cardano_ex_unit_prices_from_cbor(reader, &ex_unit_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ex_unit_prices_to_cbor, returnErrorIfWriterIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_to_cbor(ex_unit_prices, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_get_memory_prices, returnsErrorIfExUnitPricesIsNull)
{
  // Arrange
  cardano_unit_interval_t* memory_prices = nullptr;

  // Act
  cardano_error_t error = cardano_ex_unit_prices_get_memory_prices(nullptr, &memory_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_get_memory_prices, returnsErrorIfMemoryPricesIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_unit_prices_get_memory_prices((cardano_ex_unit_prices_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_get_steps_prices, returnsErrorIfExUnitPricesIsNull)
{
  // Arrange
  cardano_unit_interval_t* steps_prices = nullptr;

  // Act
  cardano_error_t error = cardano_ex_unit_prices_get_steps_prices(nullptr, &steps_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_get_steps_prices, returnsErrorIfStepsPricesIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_unit_prices_get_steps_prices((cardano_ex_unit_prices_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_set_memory_prices, returnsErrorIfExUnitPricesIsNull)
{
  // Arrange
  cardano_unit_interval_t* memory_prices = nullptr;

  // Act
  cardano_error_t error = cardano_ex_unit_prices_set_memory_prices(nullptr, memory_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_set_memory_prices, returnsErrorIfMemoryPricesIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_unit_prices_set_memory_prices((cardano_ex_unit_prices_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_set_steps_prices, returnsErrorIfExUnitPricesIsNull)
{
  // Arrange
  cardano_unit_interval_t* steps_prices = nullptr;

  // Act
  cardano_error_t error = cardano_ex_unit_prices_set_steps_prices(nullptr, steps_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_set_steps_prices, returnsErrorIfStepsPricesIsNull)
{
  // Act
  cardano_error_t error = cardano_ex_unit_prices_set_steps_prices((cardano_ex_unit_prices_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ex_unit_prices_set_steps_prices, canBeSet)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_set_steps_prices(ex_unit_prices, steps_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}

TEST(cardano_ex_unit_prices_set_memory_prices, canBeSet)
{
  // Arrange
  cardano_ex_unit_prices_t* ex_unit_prices = nullptr;
  cardano_unit_interval_t*  memory_prices  = nullptr;
  cardano_unit_interval_t*  steps_prices   = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(1, 2, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 3, &steps_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_ex_unit_prices_set_memory_prices(ex_unit_prices, memory_prices);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
}