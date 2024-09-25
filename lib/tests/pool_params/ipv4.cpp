/**
 * \file ipv4.cpp
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

#include <cardano/pool_params/ipv4.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR       = "440a03020a";
static byte_t      IP_BYTES[] = { 10, 3, 2, 10 };

/* UNIT TESTS ****************************************************************/

TEST(cardano_ipv4_new, canCreateIp)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ipv4, testing::Not((cardano_ipv4_t*)nullptr));

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_new, returnsErrorIfIpBytesAreNull)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_new(nullptr, sizeof(IP_BYTES), &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);
}

TEST(cardano_ipv4_new, returnsErrorIfIpBytesAreInvalid)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, 0, &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);
}

TEST(cardano_ipv4_new, returnsErrorIfIpIsNull)
{
  // Act
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ipv4_new, returnsErrorIfmajorAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ipv4_from_string, canDecodeIp)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_from_string("10.3.2.10", strlen("10.3.2.10"), &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_THAT(ipv4, testing::Not((cardano_ipv4_t*)nullptr));
  EXPECT_STREQ(cardano_ipv4_get_string(ipv4), "10.3.2.10");

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_from_string, returnsErrorIfIpIsNull)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_from_string(nullptr, 0, &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);
}

TEST(cardano_ipv4_from_string, returnsErrorIfIpIsEmpty)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_from_string("", 0, &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);
}

TEST(cardano_ipv4_from_string, returnsErrorIfIpIsInvalid)
{
  // Act
  cardano_error_t error = cardano_ipv4_from_string("10.3.2", strlen("10.3.2"), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ipv4_from_string, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_from_string("10.3.2.10", strlen("10.3.2.10"), &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ipv4_from_string, returnErrorIfInvalidIpString)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_from_string("10.32.23", strlen("10.32.23"), &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);
}

TEST(cardano_ipv4_from_string, returnErrorIfInvalidIpString2)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_from_string("10.3.2.1216", strlen("10.3.2.1216"), &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);
}

TEST(cardano_ipv4_to_cbor, canSerializeIp)
{
  // Arrange
  cardano_ipv4_t*        ipv4   = nullptr;
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ipv4_to_cbor(ipv4, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_ipv4_unref(&ipv4);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_ipv4_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_ipv4_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_ipv4_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ipv4_to_cbor(ipv4, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_from_cbor, canDeserializeIp)
{
  // Arrange
  cardano_ipv4_t*        ipv4   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_ipv4_from_cbor(reader, &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ipv4, testing::Not((cardano_ipv4_t*)nullptr));

  EXPECT_STREQ(cardano_ipv4_get_string(ipv4), "10.3.2.10");

  // Cleanup
  cardano_ipv4_unref(&ipv4);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ipv4_from_cbor, returnErrorIfIpIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_ipv4_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ipv4_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv4_from_cbor(nullptr, &ipv4);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ipv4_from_cbor, returnErrorIfCborDataInvalidByteString)
{
  // Arrange
  cardano_ipv4_t*        ipv4   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_ipv4_from_cbor(reader, &ipv4);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ipv4_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_ipv4_t* ipv4  = nullptr;
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv4_ref(ipv4);

  // Assert
  EXPECT_THAT(ipv4, testing::Not((cardano_ipv4_t*)nullptr));
  EXPECT_EQ(cardano_ipv4_refcount(ipv4), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_ipv4_unref(&ipv4);
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ipv4_ref(nullptr);
}

TEST(cardano_ipv4_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_ipv4_t* ipv4 = nullptr;

  // Act
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ipv4_unref((cardano_ipv4_t**)nullptr);
}

TEST(cardano_ipv4_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_ipv4_t* ipv4  = nullptr;
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv4_ref(ipv4);
  size_t ref_count = cardano_ipv4_refcount(ipv4);

  cardano_ipv4_unref(&ipv4);
  size_t updated_ref_count = cardano_ipv4_refcount(ipv4);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_ipv4_t* ipv4  = nullptr;
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv4_ref(ipv4);
  size_t ref_count = cardano_ipv4_refcount(ipv4);

  cardano_ipv4_unref(&ipv4);
  size_t updated_ref_count = cardano_ipv4_refcount(ipv4);

  cardano_ipv4_unref(&ipv4);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(ipv4, (cardano_ipv4_t*)nullptr);

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_ipv4_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_ipv4_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_ipv4_t* ipv4    = nullptr;
  const char*     message = "This is a test message";

  // Act
  cardano_ipv4_set_last_error(ipv4, message);

  // Assert
  EXPECT_STREQ(cardano_ipv4_get_last_error(ipv4), "Object is NULL.");
}

TEST(cardano_ipv4_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_ipv4_t* ipv4  = nullptr;
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_ipv4_set_last_error(ipv4, message);

  // Assert
  EXPECT_STREQ(cardano_ipv4_get_last_error(ipv4), "");

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_get_bytes_size, returnsTheSizeOfTheIpBytes)
{
  // Arrange
  cardano_ipv4_t* ipv4  = nullptr;
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t bytes_size = cardano_ipv4_get_bytes_size(ipv4);

  // Assert
  EXPECT_EQ(bytes_size, sizeof(IP_BYTES));

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_get_bytes_size, returnsZeroWhenObjectIsNull)
{
  // Act
  size_t bytes_size = cardano_ipv4_get_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(bytes_size, 0);
}

TEST(cardano_ipv4_get_bytes, returnsTheIpBytes)
{
  // Arrange
  cardano_ipv4_t* ipv4  = nullptr;
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_ipv4_get_bytes(ipv4);

  // Assert
  EXPECT_THAT(bytes, testing::NotNull());
  EXPECT_EQ(memcmp(bytes, IP_BYTES, sizeof(IP_BYTES)), 0);

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}

TEST(cardano_ipv4_get_bytes, returnsNullWhenObjectIsNull)
{
  // Act
  const byte_t* bytes = cardano_ipv4_get_bytes(nullptr);

  // Assert
  EXPECT_EQ(bytes, (byte_t*)nullptr);
}

TEST(cardano_ipv4_get_string, returnNullWhenObjectIsNull)
{
  // Act
  const char* ip = cardano_ipv4_get_string(nullptr);

  // Assert
  EXPECT_EQ(ip, (char*)nullptr);
}

TEST(cardano_ipv4_get_string_size, returnsZeroWhenObjectIsNull)
{
  // Act
  size_t ip_size = cardano_ipv4_get_string_size(nullptr);

  // Assert
  EXPECT_EQ(ip_size, 0);
}

TEST(cardano_ipv4_get_string_size, returnsTheSizeOfTheIpString)
{
  // Arrange
  cardano_ipv4_t* ipv4  = nullptr;
  cardano_error_t error = cardano_ipv4_new(IP_BYTES, sizeof(IP_BYTES), &ipv4);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t ip_size = cardano_ipv4_get_string_size(ipv4);

  // Assert
  EXPECT_EQ(ip_size, strlen("10.3.2.10"));

  // Cleanup
  cardano_ipv4_unref(&ipv4);
}
