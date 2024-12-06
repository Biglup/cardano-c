/**
 * \file string_safe.cpp
 *
 * \author luisd.bianchi
 * \date   Apr 28, 2024
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

#include "../src/string_safe.h"
#include <gmock/gmock.h>

#include <cardano/typedefs.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_safe_memcpy, canCopyBytes)
{
  const size_t dest_size = 10U;
  const size_t count     = 6U;

  byte_t       dest[dest_size]     = { 0 };
  const byte_t src[count]          = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  const byte_t expected[dest_size] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0, 0, 0, 0 };

  cardano_safe_memcpy(dest, dest_size, src, count);

  ASSERT_THAT(dest, testing::ElementsAreArray(expected));
}

TEST(cardano_safe_memcpy, dontOverflowBuffer)
{
  const size_t dest_size = 4U;
  const size_t count     = 6U;

  byte_t       dest[dest_size]     = { 0 };
  const byte_t src[count]          = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  const byte_t expected[dest_size] = { 0x01, 0x02, 0x03, 0x04 };

  cardano_safe_memcpy(dest, dest_size, src, count);

  ASSERT_THAT(dest, testing::ElementsAreArray(expected));
}

TEST(cardano_safe_memcpy, canHandleNullDestination)
{
  const size_t dest_size = 10U;
  const size_t count     = 6U;

  const byte_t src[count] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

  cardano_safe_memcpy(nullptr, dest_size, src, count);
}

TEST(cardano_safe_memcpy, canHandleNullSource)
{
  const size_t dest_size = 10U;
  const size_t count     = 6U;

  byte_t dest[dest_size] = { 0 };

  cardano_safe_memcpy(dest, dest_size, nullptr, count);
}

TEST(cardano_safe_memcpy, canHandleZeroDestinationSize)
{
  const size_t dest_size = 0U;
  const size_t count     = 6U;

  byte_t       dest[10]   = { 0 };
  const byte_t src[count] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

  cardano_safe_memcpy(dest, dest_size, src, count);
}

TEST(cardano_safe_memcpy, canHandleZeroCount)
{
  const size_t dest_size = 10U;
  const size_t count     = 0U;

  byte_t       dest[dest_size] = { 0 };
  const byte_t src[6]          = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

  cardano_safe_memcpy(dest, dest_size, src, count);
}

TEST(cardano_safe_strlen, canMeasureLength)
{
  const char*  str        = "Hello, World!";
  const size_t max_length = 20U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 13U);
}

TEST(cardano_safe_strlen, canLimitLength)
{
  const char*  str        = "Hello, World!";
  const size_t max_length = 5U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 5U);
}

TEST(cardano_safe_strlen, canHandleEmptyString)
{
  const char*  str        = "";
  const size_t max_length = 20U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_strlen, canHandleNullString)
{
  const char*  str        = nullptr;
  const size_t max_length = 20U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_strlen, canHandleNullMaxLength)
{
  const char*  str        = "Hello, World!";
  const size_t max_length = 0U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_int64_to_string, canConvertInt64)
{
  const int64_t value = 1234567890;
  const size_t  size  = 20U;

  char buffer[size] = { 0 };

  const size_t length = cardano_safe_int64_to_string(value, buffer, size);

  ASSERT_EQ(length, 10U);
  ASSERT_STREQ(buffer, "1234567890");
}

TEST(cardano_safe_int64_to_string, canHandleNullBuffer)
{
  const int64_t value = 1234567890;
  const size_t  size  = 20U;

  const size_t length = cardano_safe_int64_to_string(value, nullptr, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_int64_to_string, canHandleZeroBufferSize)
{
  const int64_t value = 1234567890;
  const size_t  size  = 0U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_int64_to_string(value, buffer, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_int64_to_string, canHandleZeroValue)
{
  const int64_t value = 0;
  const size_t  size  = 20U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_int64_to_string(value, buffer, size);

  ASSERT_EQ(length, 1U);
  ASSERT_STREQ(buffer, "0");
}

TEST(cardano_safe_int64_to_string, canHandleNegativeValue)
{
  const int64_t value = -1234567890;
  const size_t  size  = 20U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_int64_to_string(value, buffer, size);

  ASSERT_EQ(length, 11U);
  ASSERT_STREQ(buffer, "-1234567890");
}

TEST(cardano_safe_string_to_int64, canConvertString)
{
  const char*  str         = "1234567890";
  const size_t string_size = 10U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(value, 1234567890);
}

TEST(cardano_safe_string_to_int64, canHandleNullString)
{
  const char*  str         = nullptr;
  const size_t string_size = 10U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_int64, canHandleZeroSize)
{
  const char*  str         = "1234567890";
  const size_t string_size = 0U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_int64, canHandleNullValue)
{
  const char*  str         = "1234567890";
  const size_t string_size = 10U;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, nullptr);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_int64, canHandleEmptyString)
{
  const char*  str         = "";
  const size_t string_size = 0U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_int64, canHandleInvalidString)
{
  const char*  str         = "1234567890a";
  const size_t string_size = 11U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_safe_string_to_int64, canHandleOverflow)
{
  const char*  str         = "12345678901234567890";
  const size_t string_size = 20U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INTEGER_OVERFLOW);
}

TEST(cardano_safe_string_to_int64, canHandleNegativeOverflow)
{
  const char*  str         = "-12345678901234567890";
  const size_t string_size = 21U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INTEGER_OVERFLOW);
}

TEST(cardano_safe_string_to_int64, canHandleNegativeValue)
{
  const char*  str         = "-1234567890";
  const size_t string_size = 11U;
  int64_t      value       = 0;

  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(value, -1234567890);
}

TEST(cardano_safe_double_to_string, canConvertDouble)
{
  const double value = 1234567890.123456789;
  const size_t size  = 20U;

  char buffer[size] = { 0 };

  const size_t length = cardano_safe_double_to_string(value, buffer, size);

  ASSERT_EQ(length, 18U);
  ASSERT_STREQ(buffer, "1234567890.1234567");
}

TEST(cardano_safe_double_to_string, canHandleNullBuffer)
{
  const double value = 1234567890.123456789;
  const size_t size  = 20U;

  const size_t length = cardano_safe_double_to_string(value, nullptr, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_double_to_string, canHandleZeroBufferSize)
{
  const double value = 1234567890.123456789;
  const size_t size  = 0U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_double_to_string(value, buffer, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_double_to_string, canHandleZeroValue)
{
  const double value = 0.0;
  const size_t size  = 20U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_double_to_string(value, buffer, size);

  ASSERT_EQ(length, 1U);
  ASSERT_STREQ(buffer, "0");
}

TEST(cardano_safe_double_to_string, canHandleNaN)
{
  const double value = NAN;
  const size_t size  = 20U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_double_to_string(value, buffer, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_double_to_string, canHandleInfinity)
{
  const double value = INFINITY;
  const size_t size  = 20U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_double_to_string(value, buffer, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_string_to_double, canConvertString)
{
  const char*  str         = "1234567890.1234567";
  const size_t string_size = 20U;
  double       value       = 0.0;

  const cardano_error_t error = cardano_safe_string_to_double(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_DOUBLE_EQ(value, 1234567890.1234567);
}

TEST(cardano_safe_string_to_double, canHandleNullString)
{
  const char*  str         = nullptr;
  const size_t string_size = 20U;
  double       value       = 0.0;

  const cardano_error_t error = cardano_safe_string_to_double(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_double, canHandleZeroSize)
{
  const char*  str         = "1234567890.1234567";
  const size_t string_size = 0U;
  double       value       = 0.0;

  const cardano_error_t error = cardano_safe_string_to_double(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_double, canHandleNullValue)
{
  const char*  str         = "1234567890.1234567";
  const size_t string_size = 20U;

  const cardano_error_t error = cardano_safe_string_to_double(str, string_size, nullptr);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_double, canHandleEmptyString)
{
  const char*  str         = "";
  const size_t string_size = 0U;
  double       value       = 0.0;

  const cardano_error_t error = cardano_safe_string_to_double(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_double, canHandleInvalidString)
{
  const char*  str         = "1234567890.1234567a";
  const size_t string_size = 21U;
  double       value       = 0.0;

  const cardano_error_t error = cardano_safe_string_to_double(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_safe_string_to_double, canHandleNegativeValue)
{
  const char*  str         = "-1234567890.1234567";
  const size_t string_size = 21U;
  double       value       = 0.0;

  const cardano_error_t error = cardano_safe_string_to_double(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_DOUBLE_EQ(value, -1234567890.1234567);
}

TEST(cardano_safe_string_to_uint64, canConvertString)
{
  const char*  str         = "12345678901234567890";
  const size_t string_size = 20U;
  uint64_t     value       = 0;

  const cardano_error_t error = cardano_safe_string_to_uint64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(value, 12345678901234567890U);
}

TEST(cardano_safe_string_to_uint64, canHandleNullString)
{
  const char*  str         = nullptr;
  const size_t string_size = 20U;
  uint64_t     value       = 0;

  const cardano_error_t error = cardano_safe_string_to_uint64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_uint64, canHandleZeroSize)
{
  const char*  str         = "12345678901234567890";
  const size_t string_size = 0U;
  uint64_t     value       = 0;

  const cardano_error_t error = cardano_safe_string_to_uint64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_uint64, canHandleNullValue)
{
  const char*  str         = "12345678901234567890";
  const size_t string_size = 20U;

  const cardano_error_t error = cardano_safe_string_to_uint64(str, string_size, nullptr);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_uint64, canHandleEmptyString)
{
  const char*  str         = "";
  const size_t string_size = 0U;
  uint64_t     value       = 0;

  const cardano_error_t error = cardano_safe_string_to_uint64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_safe_string_to_uint64, canHandleInvalidString)
{
  const char*  str         = "12345678901234567890a";
  const size_t string_size = 21U;
  uint64_t     value       = 0;

  const cardano_error_t error = cardano_safe_string_to_uint64(str, string_size, &value);

  ASSERT_EQ(error, CARDANO_ERROR_DECODING);
}

TEST(cardano_safe_uint64_to_string, canConvertUInt64)
{
  const uint64_t value = 12345678901234567890U;
  const size_t   size  = 21U;

  char buffer[size] = { 0 };

  const size_t length = cardano_safe_uint64_to_string(value, buffer, size);

  ASSERT_EQ(length, 20U);
  ASSERT_STREQ(buffer, "12345678901234567890");
}

TEST(cardano_safe_uint64_to_string, canHandleNullBuffer)
{
  const uint64_t value = 12345678901234567890U;
  const size_t   size  = 20U;

  const size_t length = cardano_safe_uint64_to_string(value, nullptr, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_uint64_to_string, canHandleZeroBufferSize)
{
  const uint64_t value = 12345678901234567890U;
  const size_t   size  = 0U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_uint64_to_string(value, buffer, size);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_uint64_to_string, canHandleZeroValue)
{
  const uint64_t value = 0U;
  const size_t   size  = 20U;

  char buffer[20] = { 0 };

  const size_t length = cardano_safe_uint64_to_string(value, buffer, size);

  ASSERT_EQ(length, 1U);
  ASSERT_STREQ(buffer, "0");
}

TEST(cardano_safe_string_to_int64, returnsErrorOnOverflow)
{
  // Arrange
  const char*  str         = "922337203685477580800000000000000";
  const size_t string_size = 33U;
  int64_t      value       = 0;

  // Act
  const cardano_error_t error = cardano_safe_string_to_int64(str, string_size, &value);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_INTEGER_OVERFLOW);
}

TEST(cardano_safe_string_to_uint64, returnsErrorOnOverflow)
{
  // Arrange
  const char*  str         = "1844674407370955161600000000000000";
  const size_t string_size = 34U;
  uint64_t     value       = 0;

  // Act
  const cardano_error_t error = cardano_safe_string_to_uint64(str, string_size, &value);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_INTEGER_OVERFLOW);
}

TEST(cardano_safe_string_to_double, returnsErrorOnOverflow)
{
  // Arrange
  const char* str   = "1.797693134862315708145274237317043567981e+308";
  double      value = 0.0;

  // Act
  const cardano_error_t error = cardano_safe_string_to_double(str, 256, &value);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_DECODING);
}