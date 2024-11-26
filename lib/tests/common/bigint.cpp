/**
 * \file bigint.cpp
 *
 * \author luisd.bianchi
 * \date   Jun 05, 2024
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
#include <cardano/common/bigint.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_bigint_from_string, test_cardano_bigint_from_string)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_string("-1234567890000000000000000000000000000000000000000000", strlen("-1234567890000000000000000000000000000000000000000000"), 10, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_string("2", strlen("2"), 10, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_multiply(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "-2469135780000000000000000000000000000000000000000000");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_from_string, doesntCrashIfGivenANullPtr)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_string(NULL, 0, 10, &bigint);

  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_from_string, returnsErrorIfMemoryAllocationFails)
{
  cardano_bigint_t* bigint = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_bigint_from_string("123456789", strlen("123456789"), 10, &bigint);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_bigint_unref(&bigint);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bigint_from_string, canDecodeBigNumbers)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_string("340199290171201906221318119490500689920", strlen("340199290171201906221318119490500689920"), 10, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t  size   = cardano_bigint_get_bytes_size(bigint);
  byte_t* buffer = (byte_t*)malloc(size);

  result = cardano_bigint_to_bytes(bigint, CARDANO_BYTE_ORDER_BIG_ENDIAN, buffer, size);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  const byte_t expected[] = {
    0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  // use loops to compare each bytes
  for (size_t i = 0; i < size; i++)
  {
    ASSERT_EQ(buffer[i], expected[i]);
  }

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_from_string, returnErrorIfEmptyString)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_string("", 0, 10, &bigint);

  ASSERT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_from_string, returnErrorIfInvalidString)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_string("123456789a", strlen("123456789a"), 10, &bigint);

  ASSERT_EQ(result, CARDANO_ERROR_CONVERSION_FAILED);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_get_string_size, returnZeroIfPointerIsNull)
{
  size_t size = cardano_bigint_get_string_size(NULL, 10);

  ASSERT_EQ(size, 0);
}

TEST(cardano_bigint_from_int, canCreateABignumFromInt)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(-123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t size = cardano_bigint_get_string_size(bigint, 10);

  char* buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "-123456789");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_from_int, returnsErrorIfMemoryAllocationFails)
{
  cardano_bigint_t* bigint = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_bigint_unref(&bigint);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bigint_from_int, returnsErrorIfPointerIsNull)
{
  cardano_error_t result = cardano_bigint_from_int(123456789, NULL);

  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bigint_from_unsigned_int, canCreateABignumFromUnsignedInt)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_unsigned_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t size = cardano_bigint_get_string_size(bigint, 10);

  char* buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456789");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_from_unsigned_int, returnsErrorIfMemoryAllocationFails)
{
  cardano_bigint_t* bigint = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_bigint_from_unsigned_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_bigint_unref(&bigint);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bigint_from_unsigned_int, returnsErrorIfPointerIsNull)
{
  cardano_error_t result = cardano_bigint_from_unsigned_int(123456789, NULL);

  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bigint_from_bytes, fromBytesBigEndian)
{
  cardano_bigint_t* bigint = NULL;

  const byte_t expected[] = {
    0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  cardano_error_t result = cardano_bigint_from_bytes(expected, sizeof(expected), CARDANO_BYTE_ORDER_BIG_ENDIAN, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t size = cardano_bigint_get_string_size(bigint, 10);

  char* buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "340199290171201906221318119490500689920");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_from_bytes, fromBytesLittleEndian)
{
  cardano_bigint_t* bigint = NULL;

  const byte_t expected[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF
  };

  cardano_error_t result = cardano_bigint_from_bytes(expected, sizeof(expected), CARDANO_BYTE_ORDER_LITTLE_ENDIAN, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t size = cardano_bigint_get_string_size(bigint, 10);

  char* buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "340199290171201906221318119490500689920");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_to_string, returnErrorIfBufferTooSmall)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t size = cardano_bigint_get_string_size(bigint, 10);

  char* buffer = (char*)malloc(size - 1);

  result = cardano_bigint_to_string(bigint, buffer, size - 1, 10);

  ASSERT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_from_bytes, returnsErrorIfMemoryAllocationFails)
{
  cardano_bigint_t* bigint = NULL;

  const byte_t expected[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF
  };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_bigint_from_bytes(expected, sizeof(expected), CARDANO_BYTE_ORDER_LITTLE_ENDIAN, &bigint);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_bigint_unref(&bigint);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bigint_from_bytes, returnsErrorIfPointerIsNull)
{
  cardano_error_t result = cardano_bigint_from_bytes(NULL, 0, CARDANO_BYTE_ORDER_LITTLE_ENDIAN, NULL);

  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bigint_to_string, returnsErrorIfPointerIsNull)
{
  cardano_error_t result = cardano_bigint_to_string(NULL, NULL, 10, 0);

  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bigint_to_bytes, returnsErrorIfBuffIsToSmall)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t size = cardano_bigint_get_bytes_size(bigint);

  byte_t* buffer = (byte_t*)malloc(size);

  result = cardano_bigint_to_bytes(bigint, CARDANO_BYTE_ORDER_BIG_ENDIAN, buffer, size - 1);

  ASSERT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_clone, canClone)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;

  cardano_error_t result = cardano_bigint_from_string("123456789", strlen("123456789"), 10, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_clone(lhs, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t size   = cardano_bigint_get_string_size(rhs, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(rhs, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456789");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
}

TEST(cardano_bigint_clone, returnsErrorIfMemoryAllocationFails)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;

  cardano_error_t result = cardano_bigint_from_string("123456789", strlen("123456789"), 10, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_bigint_clone(lhs, &rhs);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
}

TEST(cardano_bigint_clone, returnsErrorIfPointerIsNull)
{
  cardano_error_t result = cardano_bigint_clone(NULL, NULL);

  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bigint_to_int, canConvertToAnInt)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  int64_t value = cardano_bigint_to_int(bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 123456789);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_to_int, returnZeroIfPointerIsNull)
{
  int64_t value = cardano_bigint_to_int(NULL);

  ASSERT_EQ(value, 0);
}

TEST(cardano_bigint_to_unsigned_int, canConvertToAnUnsignedInt)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_unsigned_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  uint64_t value = cardano_bigint_to_unsigned_int(bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(value, 123456789);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_to_unsigned_int, returnZeroIfPointerIsNull)
{
  uint64_t value = cardano_bigint_to_unsigned_int(NULL);

  ASSERT_EQ(value, 0);
}

TEST(cardano_bigint_to_bytes, returnsErrorIfPointerIsNull)
{
  cardano_error_t result = cardano_bigint_to_bytes(NULL, CARDANO_BYTE_ORDER_BIG_ENDIAN, NULL, 0);

  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bigint_add, canAddTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_add(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "1111111110");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_add, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_add(lhs, rhs, NULL);
  cardano_bigint_add(lhs, NULL, res);
  cardano_bigint_add(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_subtract, canSubtractTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_subtract(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "864197532");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_subtract, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_subtract(lhs, rhs, NULL);
  cardano_bigint_subtract(lhs, NULL, res);
  cardano_bigint_subtract(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_multiply, canMultiplyTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_multiply(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "121932631112635269");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_multiply, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_multiply(lhs, rhs, NULL);
  cardano_bigint_multiply(lhs, NULL, res);
  cardano_bigint_multiply(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_divide, canDivideTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_divide(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "8");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_divide, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_divide(lhs, rhs, NULL);
  cardano_bigint_divide(lhs, NULL, res);
  cardano_bigint_divide(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_divide_and_reminder, canDivideTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;
  cardano_bigint_t* rem = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &rem);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_divide_and_reminder(lhs, rhs, res, rem);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "8");

  free(buffer);

  size   = cardano_bigint_get_string_size(rem, 10);
  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(rem, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "9");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
  cardano_bigint_unref(&rem);
}

TEST(cardano_bigint_divide_and_reminder, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;
  cardano_bigint_t* rem = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &rem);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_divide_and_reminder(lhs, rhs, NULL, rem);
  cardano_bigint_divide_and_reminder(lhs, rhs, res, NULL);
  cardano_bigint_divide_and_reminder(lhs, NULL, res, rem);
  cardano_bigint_divide_and_reminder(NULL, rhs, res, rem);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  size   = cardano_bigint_get_string_size(rem, 10);
  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(rem, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
  cardano_bigint_unref(&rem);
}

TEST(cardano_bigint_mod, canModTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_mod(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "9");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_mod, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_mod(lhs, rhs, NULL);
  cardano_bigint_mod(lhs, NULL, res);
  cardano_bigint_mod(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_reminder, canGetTheReminderOfTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_reminder(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "9");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_reminder, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_reminder(lhs, rhs, NULL);
  cardano_bigint_reminder(lhs, NULL, res);
  cardano_bigint_reminder(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_abs, canGetTheAbsoluteValue)
{
  cardano_bigint_t* bigint  = NULL;
  cardano_bigint_t* bigint2 = NULL;

  cardano_error_t result = cardano_bigint_from_int(-123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &bigint2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_abs(bigint, bigint2);

  size_t size   = cardano_bigint_get_string_size(bigint2, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint2, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456789");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&bigint2);
}

TEST(cardano_bigint_abs, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* bigint  = NULL;
  cardano_bigint_t* bigint2 = NULL;

  cardano_error_t result = cardano_bigint_from_int(-123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &bigint2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_abs(bigint, NULL);
  cardano_bigint_abs(NULL, bigint2);

  size_t size   = cardano_bigint_get_string_size(bigint2, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint2, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&bigint2);
}

TEST(cardano_bigint_gcd, canGetTheGreatestCommonDivisor)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_gcd(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "9");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_gcd, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_gcd(lhs, rhs, NULL);
  cardano_bigint_gcd(lhs, NULL, res);
  cardano_bigint_gcd(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_negate, canNegateANumber)
{
  cardano_bigint_t* bigint  = NULL;
  cardano_bigint_t* bigint2 = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &bigint2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_negate(bigint, bigint2);

  size_t size   = cardano_bigint_get_string_size(bigint2, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint2, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "-123456789");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&bigint2);
}

TEST(cardano_bigint_negate, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* bigint  = NULL;
  cardano_bigint_t* bigint2 = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &bigint2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_negate(bigint, NULL);
  cardano_bigint_negate(NULL, bigint2);

  size_t size   = cardano_bigint_get_string_size(bigint2, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint2, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&bigint2);
}

TEST(cardano_bigint_signum, canGetTheSignOfANumber)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  int sign = cardano_bigint_signum(bigint);

  ASSERT_EQ(sign, 1);

  cardano_bigint_unref(&bigint);

  result = cardano_bigint_from_int(-123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  sign = cardano_bigint_signum(bigint);

  ASSERT_EQ(sign, -1);

  cardano_bigint_unref(&bigint);

  result = cardano_bigint_from_int(0, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  sign = cardano_bigint_signum(bigint);

  ASSERT_EQ(sign, 0);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_signum, returnsZeroIfPointerIsNull)
{
  int sign = cardano_bigint_signum(NULL);

  ASSERT_EQ(sign, 0);
}

TEST(cardano_bigint_mod_pow, canGetTheModPow)
{
  cardano_bigint_t* base = NULL;
  cardano_bigint_t* exp  = NULL;
  cardano_bigint_t* mod  = NULL;
  cardano_bigint_t* res  = NULL;

  cardano_error_t result = cardano_bigint_from_int(2, &base);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(10, &exp);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(100, &mod);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_mod_pow(base, exp, mod, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "24");

  free(buffer);

  cardano_bigint_unref(&base);
  cardano_bigint_unref(&exp);
  cardano_bigint_unref(&mod);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_mod_pow, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* base = NULL;
  cardano_bigint_t* exp  = NULL;
  cardano_bigint_t* mod  = NULL;
  cardano_bigint_t* res  = NULL;

  cardano_error_t result = cardano_bigint_from_int(2, &base);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(10, &exp);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(100, &mod);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_mod_pow(base, exp, mod, NULL);
  cardano_bigint_mod_pow(base, exp, NULL, res);
  cardano_bigint_mod_pow(base, NULL, mod, res);
  cardano_bigint_mod_pow(NULL, exp, mod, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&base);
  cardano_bigint_unref(&exp);
  cardano_bigint_unref(&mod);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_mod_inverse, canGetTheModInverse)
{
  cardano_bigint_t* base = NULL;
  cardano_bigint_t* mod  = NULL;
  cardano_bigint_t* res  = NULL;

  cardano_error_t result = cardano_bigint_from_int(3, &base);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(11, &mod);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_mod_inverse(base, mod, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "4");

  free(buffer);

  cardano_bigint_unref(&base);
  cardano_bigint_unref(&mod);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_mod_inverse, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* base = NULL;
  cardano_bigint_t* mod  = NULL;
  cardano_bigint_t* res  = NULL;

  cardano_error_t result = cardano_bigint_from_int(3, &base);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(11, &mod);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_mod_inverse(base, mod, NULL);
  cardano_bigint_mod_inverse(base, NULL, res);
  cardano_bigint_mod_inverse(NULL, mod, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&base);
  cardano_bigint_unref(&mod);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_and, canGetTheAnd)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0b1100110011, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_and(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "546");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_and, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0b1100110011, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_and(lhs, rhs, NULL);
  cardano_bigint_and(lhs, NULL, res);
  cardano_bigint_and(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_or, canGetTheOr)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0b1100110011, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_or(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "955");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_or, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0b1100110011, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_or(lhs, rhs, NULL);
  cardano_bigint_or(lhs, NULL, res);
  cardano_bigint_or(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_xor, canGetTheXor)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0b1100110011, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_xor(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "409");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_xor, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0b1100110011, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_xor(lhs, rhs, NULL);
  cardano_bigint_xor(lhs, NULL, res);
  cardano_bigint_xor(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_not, canGetTheNot)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_not(bigint, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "-683");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_not, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_not(bigint, NULL);
  cardano_bigint_not(NULL, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_test_bit, canTestABit)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  int bit = cardano_bigint_test_bit(bigint, 0);

  ASSERT_EQ(bit, 0);

  bit = cardano_bigint_test_bit(bigint, 1);

  ASSERT_EQ(bit, 1);

  bit = cardano_bigint_test_bit(bigint, 2);

  ASSERT_EQ(bit, 0);

  bit = cardano_bigint_test_bit(bigint, 3);

  ASSERT_EQ(bit, 1);

  bit = cardano_bigint_test_bit(bigint, 4);

  ASSERT_EQ(bit, 0);

  bit = cardano_bigint_test_bit(bigint, 5);

  ASSERT_EQ(bit, 1);

  bit = cardano_bigint_test_bit(bigint, 6);

  ASSERT_EQ(bit, 0);

  bit = cardano_bigint_test_bit(bigint, 7);

  ASSERT_EQ(bit, 1);

  bit = cardano_bigint_test_bit(bigint, 8);

  ASSERT_EQ(bit, 0);

  bit = cardano_bigint_test_bit(bigint, 9);

  ASSERT_EQ(bit, 1);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_test_bit, returnsZeroIfPointerIsNull)
{
  int bit = cardano_bigint_test_bit(NULL, 0);

  ASSERT_EQ(bit, 0);
}

TEST(cardano_bigint_set_bit, canSetABit)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_set_bit(bigint, 0);

  size_t size   = cardano_bigint_get_string_size(bigint, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "683");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_set_bit, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_set_bit(NULL, 0);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_clear_bit, canClearABit)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_clear_bit(bigint, 0);

  size_t size   = cardano_bigint_get_string_size(bigint, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "682");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_clear_bit, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_clear_bit(NULL, 0);
}

TEST(cardano_bigint_flip_bit, canFlipABit)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_flip_bit(bigint, 0);

  size_t size   = cardano_bigint_get_string_size(bigint, 2);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "1010101011");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_flip_bit, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_flip_bit(NULL, 0);
}

TEST(cardano_bigint_min, canGetTheMinimum)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_min(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456789");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_min, canGetTheMinimumRhs)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_min(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456789");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_min, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_min(lhs, rhs, NULL);
  cardano_bigint_min(lhs, NULL, res);
  cardano_bigint_min(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_max, canGetTheMaximum)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_max(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "987654321");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_max, canGetTheMaximumRhs)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_max(lhs, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "987654321");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_max, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;
  cardano_bigint_t* res = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_max(lhs, rhs, NULL);
  cardano_bigint_max(lhs, NULL, res);
  cardano_bigint_max(NULL, rhs, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_shift_left, canShiftLeft)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_shift_left(bigint, 3, res);

  size_t size   = cardano_bigint_get_string_size(res, 2);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "1010101010000");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_shift_left, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_shift_left(bigint, 1, NULL);
  cardano_bigint_shift_left(NULL, 1, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_shift_right, canShiftRight)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_shift_right(bigint, 1, res);

  size_t size   = cardano_bigint_get_string_size(res, 2);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "101010101");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_shift_right, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_shift_right(bigint, 1, NULL);
  cardano_bigint_shift_right(NULL, 1, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_equals, canCompareTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  int equal = cardano_bigint_equals(lhs, rhs);

  ASSERT_EQ(equal, 1);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);

  result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  equal = cardano_bigint_equals(lhs, rhs);

  ASSERT_EQ(equal, 0);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
}

TEST(cardano_bigint_equals, returnsZeroIfPointerIsNull)
{
  int equal = cardano_bigint_equals(NULL, NULL);

  ASSERT_EQ(equal, 0);
}

TEST(cardano_bigint_compare, canCompareTwoNumbers)
{
  cardano_bigint_t* lhs = NULL;
  cardano_bigint_t* rhs = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  int cmp = cardano_bigint_compare(lhs, rhs);

  ASSERT_EQ(cmp, 0);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);

  result = cardano_bigint_from_int(123456789, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(987654321, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cmp = cardano_bigint_compare(lhs, rhs);

  ASSERT_EQ(cmp, -1);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);

  result = cardano_bigint_from_int(987654321, &lhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(123456789, &rhs);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cmp = cardano_bigint_compare(lhs, rhs);

  ASSERT_EQ(cmp, 1);

  cardano_bigint_unref(&lhs);
  cardano_bigint_unref(&rhs);
}

TEST(cardano_bigint_compare, returnsZeroIfPointerIsNull)
{
  int cmp = cardano_bigint_compare(NULL, NULL);

  ASSERT_EQ(cmp, 0);
}

TEST(cardano_bigint_is_zero, canCheckIfANumberIsZero)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  int is_zero = cardano_bigint_is_zero(bigint);

  ASSERT_EQ(is_zero, 1);

  cardano_bigint_unref(&bigint);

  result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  is_zero = cardano_bigint_is_zero(bigint);

  ASSERT_EQ(is_zero, 0);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_is_zero, returnsZeroIfPointerIsNull)
{
  int is_zero = cardano_bigint_is_zero(NULL);

  ASSERT_EQ(is_zero, 0);
}

TEST(cardano_bigint_increment, canIncrementANumber)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_increment(bigint);

  size_t size   = cardano_bigint_get_string_size(bigint, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456790");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_increment, hasNoEffectIfPointerIsNull)
{
  cardano_bigint_increment(NULL);
}

TEST(cardano_bigint_decrement, canDecrementANumber)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_decrement(bigint);

  size_t size   = cardano_bigint_get_string_size(bigint, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(bigint, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456788");

  free(buffer);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_decrement, hasNoEffectIfPointerIsNull)
{
  cardano_bigint_decrement(NULL);
}

TEST(cardano_bigint_pow, canRaiseANumberToAPower)
{
  cardano_bigint_t* base = NULL;
  uint64_t          exp  = 10;
  cardano_bigint_t* res  = NULL;

  cardano_error_t result = cardano_bigint_from_int(2, &base);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_pow(base, exp, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "1024");

  free(buffer);

  cardano_bigint_unref(&base);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_pow, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* base = NULL;
  uint64_t          exp  = 0;
  cardano_bigint_t* res  = NULL;

  cardano_error_t result = cardano_bigint_from_int(2, &base);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_pow(base, exp, NULL);
  cardano_bigint_pow(NULL, exp, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&base);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_assign, canAssignANumber)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_assign(bigint, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "123456789");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_assign, hasNoEffectIfAPointerIsNull)
{
  cardano_bigint_t* bigint = NULL;
  cardano_bigint_t* res    = NULL;

  cardano_error_t result = cardano_bigint_from_int(123456789, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_bigint_from_int(0, &res);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_bigint_assign(bigint, NULL);
  cardano_bigint_assign(NULL, res);

  size_t size   = cardano_bigint_get_string_size(res, 10);
  char*  buffer = (char*)malloc(size);

  result = cardano_bigint_to_string(res, buffer, size, 10);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_STREQ(buffer, "0");

  free(buffer);

  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&res);
}

TEST(cardano_bigint_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_bigint_t* bigint = nullptr;
  cardano_error_t   error  = cardano_bigint_from_int(0, &bigint);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_ref(bigint);

  // Assert
  EXPECT_THAT(bigint, testing::Not((cardano_bigint_t*)nullptr));
  EXPECT_EQ(cardano_bigint_refcount(bigint), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_bigint_unref(&bigint);
  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bigint_ref(nullptr);
}

TEST(cardano_bigint_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_bigint_t* bigint = nullptr;

  // Act
  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bigint_unref((cardano_bigint_t**)nullptr);
}

TEST(cardano_bigint_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_bigint_t* bigint = nullptr;
  cardano_error_t   error  = cardano_bigint_from_int(0, &bigint);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_ref(bigint);
  size_t ref_count = cardano_bigint_refcount(bigint);

  cardano_bigint_unref(&bigint);
  size_t updated_ref_count = cardano_bigint_refcount(bigint);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_bigint_t* bigint = nullptr;
  cardano_error_t   error  = cardano_bigint_from_int(0, &bigint);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bigint_ref(bigint);
  size_t ref_count = cardano_bigint_refcount(bigint);

  cardano_bigint_unref(&bigint);
  size_t updated_ref_count = cardano_bigint_refcount(bigint);

  cardano_bigint_unref(&bigint);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(bigint, (cardano_bigint_t*)nullptr);

  // Cleanup
  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_bigint_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_bigint_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_bigint_t* bigint  = nullptr;
  const char*       message = "This is a test message";

  // Act
  cardano_bigint_set_last_error(bigint, message);

  // Assert
  EXPECT_STREQ(cardano_bigint_get_last_error(bigint), "Object is NULL.");
}

TEST(cardano_bigint_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_bigint_t* bigint = nullptr;
  cardano_error_t   error  = cardano_bigint_from_int(0, &bigint);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_bigint_set_last_error(bigint, message);

  // Assert
  EXPECT_STREQ(cardano_bigint_get_last_error(bigint), "");

  // Cleanup
  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_bit_count, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t bit_count = cardano_bigint_bit_count(nullptr);

  // Assert
  EXPECT_EQ(bit_count, 0);
}

TEST(cardano_bigint_bit_count, getBitCount)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t bit_count = cardano_bigint_bit_count(bigint);

  ASSERT_EQ(bit_count, 5);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_bit_length, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t bit_length = cardano_bigint_bit_length(nullptr);

  // Assert
  EXPECT_EQ(bit_length, 0);
}

TEST(cardano_bigint_bit_length, getBitLength)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t bit_length = cardano_bigint_bit_length(bigint);

  ASSERT_EQ(bit_length, 10);

  cardano_bigint_unref(&bigint);
}

TEST(cardano_bigint_bit_length, ofNegativeNumber)
{
  cardano_bigint_t* bigint = NULL;

  cardano_error_t result = cardano_bigint_from_int(-0b1010101010, &bigint);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t bit_length = cardano_bigint_bit_length(bigint);

  ASSERT_EQ(bit_length, 10);

  cardano_bigint_unref(&bigint);
}