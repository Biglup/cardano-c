/**
 * \file account_balance_interval.cpp
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
 *
 * Copyright 2026 Biglup Labs
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

#include <cardano/transaction_body/account_balance_interval.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* BOTH_BOUNDS_CBOR      = "821864191388";
static const char* BOTH_BOUNDS_WIDE_CBOR = "821864192710";
static const char* LOWER_ONLY_CBOR       = "821901f4f6";
static const char* UPPER_ONLY_CBOR       = "82f6192710";
static const char* BOTH_NIL_CBOR         = "82f6f6";
static const char* ZERO_LOWER_ONLY_CBOR  = "8200f6";
static const char* ZERO_LOWER_UPPER_CBOR = "8200192710";
static const char* ONE_ELEMENT_CBOR      = "811864";
static const char* THREE_ELEMENTS_CBOR   = "8318641913880a";

static const uint64_t INCLUSIVE_LOWER_BOUND = 100;
static const uint64_t EXCLUSIVE_UPPER_BOUND = 5000;

/**
 * Decodes the given CBOR hex string into an account balance interval.
 * @return A new instance of the account balance interval.
 */
static cardano_account_balance_interval_t*
new_default_account_balance_interval(const char* cbor)
{
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_account_balance_interval_unref(&account_balance_interval);
    return nullptr;
  }

  return account_balance_interval;
}

/**
 * Encodes the given account balance interval to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_account_balance_interval(cardano_account_balance_interval_t* account_balance_interval)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_account_balance_interval_to_cbor(account_balance_interval, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_account_balance_interval_new, canCreateIntervalWithBothBounds)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(account_balance_interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval);
  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, INCLUSIVE_LOWER_BOUND);
  EXPECT_EQ(*exclusive_upper_bound, EXCLUSIVE_UPPER_BOUND);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_new, canCreateIntervalWithOnlyLowerBound)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, nullptr, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, INCLUSIVE_LOWER_BOUND);
  EXPECT_EQ(cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval), (const uint64_t*)nullptr);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_new, canCreateIntervalWithOnlyUpperBound)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_interval_new(nullptr, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval);

  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*exclusive_upper_bound, EXCLUSIVE_UPPER_BOUND);
  EXPECT_EQ(cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval), (const uint64_t*)nullptr);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_new, returnsErrorIfBothBoundsAreAbsent)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_interval_new(nullptr, nullptr, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);
}

TEST(cardano_account_balance_interval_new, returnsErrorIfIntervalIsNull)
{
  // Act
  cardano_error_t error = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_interval_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_account_balance_interval_from_cbor, canDeserializeIntervalWithBothBounds)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(BOTH_BOUNDS_CBOR, strlen(BOTH_BOUNDS_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(account_balance_interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval);
  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, 100);
  EXPECT_EQ(*exclusive_upper_bound, 5000);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, canDeserializeIntervalWithOnlyLowerBound)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(LOWER_ONLY_CBOR, strlen(LOWER_ONLY_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, 500);
  EXPECT_EQ(cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval), (const uint64_t*)nullptr);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, canDeserializeIntervalWithOnlyUpperBound)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(UPPER_ONLY_CBOR, strlen(UPPER_ONLY_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval);

  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*exclusive_upper_bound, 10000);
  EXPECT_EQ(cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval), (const uint64_t*)nullptr);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, decodesZeroLowerBoundAsZeroRatherThanNil)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(ZERO_LOWER_ONLY_CBOR, strlen(ZERO_LOWER_ONLY_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, 0);
  EXPECT_EQ(cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval), (const uint64_t*)nullptr);

  char* actual_cbor = encode_account_balance_interval(account_balance_interval);
  EXPECT_STREQ(actual_cbor, ZERO_LOWER_ONLY_CBOR);

  // Cleanup
  free(actual_cbor);
  cardano_account_balance_interval_unref(&account_balance_interval);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfBothBoundsAreNil)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(BOTH_NIL_CBOR, strlen(BOTH_NIL_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Both interval bounds cannot be nil.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfArrayHasOneElement)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(ONE_ELEMENT_CBOR, strlen(ONE_ELEMENT_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfArrayHasThreeElements)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(THREE_ELEMENTS_CBOR, strlen(THREE_ELEMENTS_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfLowerBoundIsInvalid)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex("8261611864", 10);

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfUpperBoundIsInvalid)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex("8218646161", 10);

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfIntervalIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(BOTH_BOUNDS_CBOR, strlen(BOTH_BOUNDS_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(nullptr, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_interval_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(BOTH_BOUNDS_CBOR, strlen(BOTH_BOUNDS_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_interval_to_cbor, canRoundTripIntervalWithBothBounds)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = new_default_account_balance_interval(BOTH_BOUNDS_CBOR);
  ASSERT_THAT(account_balance_interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  // Act
  char* actual_cbor = encode_account_balance_interval(account_balance_interval);

  // Assert
  EXPECT_STREQ(actual_cbor, BOTH_BOUNDS_CBOR);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
  free(actual_cbor);
}

TEST(cardano_account_balance_interval_to_cbor, canRoundTripIntervalWithBothBoundsAndWideUpperBound)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = new_default_account_balance_interval(BOTH_BOUNDS_WIDE_CBOR);
  ASSERT_THAT(account_balance_interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval);
  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, 100);
  EXPECT_EQ(*exclusive_upper_bound, 10000);

  // Act
  char* actual_cbor = encode_account_balance_interval(account_balance_interval);

  // Assert
  EXPECT_STREQ(actual_cbor, BOTH_BOUNDS_WIDE_CBOR);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
  free(actual_cbor);
}

TEST(cardano_account_balance_interval_to_cbor, canRoundTripIntervalWithOnlyLowerBound)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = new_default_account_balance_interval(LOWER_ONLY_CBOR);
  ASSERT_THAT(account_balance_interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  // Act
  char* actual_cbor = encode_account_balance_interval(account_balance_interval);

  // Assert
  EXPECT_STREQ(actual_cbor, LOWER_ONLY_CBOR);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
  free(actual_cbor);
}

TEST(cardano_account_balance_interval_to_cbor, canRoundTripIntervalWithOnlyUpperBound)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = new_default_account_balance_interval(UPPER_ONLY_CBOR);
  ASSERT_THAT(account_balance_interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  // Act
  char* actual_cbor = encode_account_balance_interval(account_balance_interval);

  // Assert
  EXPECT_STREQ(actual_cbor, UPPER_ONLY_CBOR);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
  free(actual_cbor);
}

TEST(cardano_account_balance_interval_to_cbor, encodesZeroBoundDistinctlyFromAbsentBound)
{
  // Arrange
  cardano_account_balance_interval_t* zero_lower_interval   = nullptr;
  cardano_account_balance_interval_t* absent_lower_interval = nullptr;

  const uint64_t zero_lower_bound = 0;
  const uint64_t upper_bound      = 10000;

  ASSERT_EQ(cardano_account_balance_interval_new(&zero_lower_bound, &upper_bound, &zero_lower_interval), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_account_balance_interval_new(nullptr, &upper_bound, &absent_lower_interval), CARDANO_SUCCESS);

  // Act
  char* zero_lower_cbor   = encode_account_balance_interval(zero_lower_interval);
  char* absent_lower_cbor = encode_account_balance_interval(absent_lower_interval);

  // Assert
  EXPECT_STREQ(zero_lower_cbor, ZERO_LOWER_UPPER_CBOR);
  EXPECT_STREQ(absent_lower_cbor, UPPER_ONLY_CBOR);

  // Cleanup
  cardano_account_balance_interval_unref(&zero_lower_interval);
  cardano_account_balance_interval_unref(&absent_lower_interval);
  free(zero_lower_cbor);
  free(absent_lower_cbor);
}

TEST(cardano_account_balance_interval_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_account_balance_interval_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_account_balance_interval_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  cardano_error_t error = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_interval_to_cbor(account_balance_interval, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_get_inclusive_lower_bound, returnsNullIfIntervalIsNull)
{
  // Act
  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(nullptr);

  // Assert
  EXPECT_EQ(inclusive_lower_bound, (const uint64_t*)nullptr);
}

TEST(cardano_account_balance_interval_get_exclusive_upper_bound, returnsNullIfIntervalIsNull)
{
  // Act
  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(nullptr);

  // Assert
  EXPECT_EQ(exclusive_upper_bound, (const uint64_t*)nullptr);
}

TEST(cardano_account_balance_interval_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_error_t                     error                    = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_interval_ref(account_balance_interval);

  // Assert
  EXPECT_THAT(account_balance_interval, testing::Not((cardano_account_balance_interval_t*)nullptr));
  EXPECT_EQ(cardano_account_balance_interval_refcount(account_balance_interval), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_account_balance_interval_unref(&account_balance_interval);
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_account_balance_interval_ref(nullptr);
}

TEST(cardano_account_balance_interval_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;

  // Act
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_account_balance_interval_unref((cardano_account_balance_interval_t**)nullptr);
}

TEST(cardano_account_balance_interval_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_error_t                     error                    = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_interval_ref(account_balance_interval);
  size_t ref_count = cardano_account_balance_interval_refcount(account_balance_interval);

  cardano_account_balance_interval_unref(&account_balance_interval);
  size_t updated_ref_count = cardano_account_balance_interval_refcount(account_balance_interval);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_error_t                     error                    = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_interval_ref(account_balance_interval);
  size_t ref_count = cardano_account_balance_interval_refcount(account_balance_interval);

  cardano_account_balance_interval_unref(&account_balance_interval);
  size_t updated_ref_count = cardano_account_balance_interval_refcount(account_balance_interval);

  cardano_account_balance_interval_unref(&account_balance_interval);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(account_balance_interval, (cardano_account_balance_interval_t*)nullptr);

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
}

TEST(cardano_account_balance_interval_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_account_balance_interval_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_account_balance_interval_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  const char*                         message                  = "This is a test message";

  // Act
  cardano_account_balance_interval_set_last_error(account_balance_interval, message);

  // Assert
  EXPECT_STREQ(cardano_account_balance_interval_get_last_error(account_balance_interval), "Object is NULL.");
}

TEST(cardano_account_balance_interval_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_error_t                     error                    = cardano_account_balance_interval_new(&INCLUSIVE_LOWER_BOUND, &EXCLUSIVE_UPPER_BOUND, &account_balance_interval);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_account_balance_interval_set_last_error(account_balance_interval, message);

  // Assert
  EXPECT_STREQ(cardano_account_balance_interval_get_last_error(account_balance_interval), "");

  // Cleanup
  cardano_account_balance_interval_unref(&account_balance_interval);
}
