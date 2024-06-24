/**
 * \file ipv6.cpp
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

#include <cardano/pool_params/ipv6.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR       = "5001020304010203040102030401020304";
static byte_t      IP_BYTES[] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };

/* UNIT TESTS ****************************************************************/

TEST(cardano_ipv6_new, canCreateIp)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ipv6, testing::Not((cardano_ipv6_t*)nullptr));

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_new, returnsErrorIfIpBytesAreNull)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_new(nullptr, sizeof(IP_BYTES), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);
}

TEST(cardano_ipv6_new, returnsErrorIfIpBytesAreInvalid)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, 0, &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);
}

TEST(cardano_ipv6_new, returnsErrorIfIpIsNull)
{
  // Act
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ipv6_new, returnsErrorIfmajorAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ipv6_from_string, canDecodeIp)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_string("0102:0304:0102:0304:0102:0304:0102:0304", strlen("0102:0304:0102:0304:0102:0304:0102:0304"), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_THAT(ipv6, testing::Not((cardano_ipv6_t*)nullptr));
  EXPECT_STREQ(cardano_ipv6_get_string(ipv6), "0102:0304:0102:0304:0102:0304:0102:0304");

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_from_string, returnsErrorIfIpIsNull)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_string(nullptr, 0, &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);
}

TEST(cardano_ipv6_from_string, returnsErrorIfIpIsEmpty)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_string("", 0, &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);
}

TEST(cardano_ipv6_from_string, returnsErrorIfIpIsInvalid)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_string("mm02:0304:0102:0304:0102:0304:0102:0304", strlen("0102:0304:0102:0304:0102:0304:0102:0304"), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_ipv6_from_string, returnsErrorIfIpIsInvalid2)
{
  // Act
  cardano_error_t error = cardano_ipv6_from_string("10.3.2", strlen("10.3.2"), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ipv6_from_string, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_string("0102:0304:0102:0304:0102:0304:0102:0304", strlen("0102:0304:0102:0304:0102:0304:0102:0304"), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ipv6_from_string, returnErrorIfInvalidIpString)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_string("10.32.23", strlen("10.32.23"), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);
}

TEST(cardano_ipv6_from_string, returnErrorIfInvalidIpString2)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_string("10.3.2.1216", strlen("10.3.2.1216"), &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);
}

TEST(cardano_ipv6_to_cbor, canSerializeIp)
{
  // Arrange
  cardano_ipv6_t*        ipv6   = nullptr;
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ipv6_to_cbor(ipv6, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_ipv6_unref(&ipv6);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_ipv6_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_ipv6_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_ipv6_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ipv6_to_cbor(ipv6, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_from_cbor, canDeserializeIp)
{
  // Arrange
  cardano_ipv6_t*        ipv6   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_ipv6_from_cbor(reader, &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(ipv6, testing::Not((cardano_ipv6_t*)nullptr));

  EXPECT_STREQ(cardano_ipv6_get_string(ipv6), "0102:0304:0102:0304:0102:0304:0102:0304");

  // Cleanup
  cardano_ipv6_unref(&ipv6);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ipv6_from_cbor, returnErrorIfIpIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_ipv6_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ipv6_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_error_t error = cardano_ipv6_from_cbor(nullptr, &ipv6);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ipv6_from_cbor, returnErrorIfCborDataInvalidByteString)
{
  // Arrange
  cardano_ipv6_t*        ipv6   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_ipv6_from_cbor(reader, &ipv6);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_ipv6_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_ipv6_t* ipv6  = nullptr;
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv6_ref(ipv6);

  // Assert
  EXPECT_THAT(ipv6, testing::Not((cardano_ipv6_t*)nullptr));
  EXPECT_EQ(cardano_ipv6_refcount(ipv6), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_ipv6_unref(&ipv6);
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ipv6_ref(nullptr);
}

TEST(cardano_ipv6_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_ipv6_t* ipv6 = nullptr;

  // Act
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ipv6_unref((cardano_ipv6_t**)nullptr);
}

TEST(cardano_ipv6_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_ipv6_t* ipv6  = nullptr;
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv6_ref(ipv6);
  size_t ref_count = cardano_ipv6_refcount(ipv6);

  cardano_ipv6_unref(&ipv6);
  size_t updated_ref_count = cardano_ipv6_refcount(ipv6);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_ipv6_t* ipv6  = nullptr;
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv6_ref(ipv6);
  size_t ref_count = cardano_ipv6_refcount(ipv6);

  cardano_ipv6_unref(&ipv6);
  size_t updated_ref_count = cardano_ipv6_refcount(ipv6);

  cardano_ipv6_unref(&ipv6);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(ipv6, (cardano_ipv6_t*)nullptr);

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_ipv6_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_ipv6_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_ipv6_t* ipv6    = nullptr;
  const char*     message = "This is a test message";

  // Act
  cardano_ipv6_set_last_error(ipv6, message);

  // Assert
  EXPECT_STREQ(cardano_ipv6_get_last_error(ipv6), "Object is NULL.");
}

TEST(cardano_ipv6_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_ipv6_t* ipv6  = nullptr;
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_ipv6_set_last_error(ipv6, message);

  // Assert
  EXPECT_STREQ(cardano_ipv6_get_last_error(ipv6), "");

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_get_bytes_size, returnsTheSizeOfTheIpBytes)
{
  // Arrange
  cardano_ipv6_t* ipv6  = nullptr;
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t bytes_size = cardano_ipv6_get_bytes_size(ipv6);

  // Assert
  EXPECT_EQ(bytes_size, sizeof(IP_BYTES));

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_get_bytes_size, returnsZeroWhenObjectIsNull)
{
  // Act
  size_t bytes_size = cardano_ipv6_get_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(bytes_size, 0);
}

TEST(cardano_ipv6_get_bytes, returnsTheIpBytes)
{
  // Arrange
  cardano_ipv6_t* ipv6  = nullptr;
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_ipv6_get_bytes(ipv6);

  // Assert
  EXPECT_THAT(bytes, testing::NotNull());
  EXPECT_EQ(memcmp(bytes, IP_BYTES, sizeof(IP_BYTES)), 0);

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}

TEST(cardano_ipv6_get_bytes, returnsNullWhenObjectIsNull)
{
  // Act
  const byte_t* bytes = cardano_ipv6_get_bytes(nullptr);

  // Assert
  EXPECT_EQ(bytes, (byte_t*)nullptr);
}

TEST(cardano_ipv6_get_string, returnNullWhenObjectIsNull)
{
  // Act
  const char* ip = cardano_ipv6_get_string(nullptr);

  // Assert
  EXPECT_EQ(ip, (char*)nullptr);
}

TEST(cardano_ipv6_get_string_size, returnsZeroWhenObjectIsNull)
{
  // Act
  size_t ip_size = cardano_ipv6_get_string_size(nullptr);

  // Assert
  EXPECT_EQ(ip_size, 0);
}

TEST(cardano_ipv6_get_string_size, returnsTheSizeOfTheIpString)
{
  // Arrange
  cardano_ipv6_t* ipv6  = nullptr;
  cardano_error_t error = cardano_ipv6_new(IP_BYTES, sizeof(IP_BYTES), &ipv6);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t ip_size = cardano_ipv6_get_string_size(ipv6);

  // Assert
  EXPECT_EQ(ip_size, strlen("0102:0304:0102:0304:0102:0304:0102:0304"));

  // Cleanup
  cardano_ipv6_unref(&ipv6);
}
