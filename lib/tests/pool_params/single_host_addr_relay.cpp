/**
 * \file single_host_addr_relay.cpp
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

#include <cardano/pool_params/single_host_addr_relay.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char*    CBOR = "84000a440a03020a5001020304010203040102030401020304";
static const char*    IPV4 = "10.3.2.10";
static const char*    IPV6 = "0102:0304:0102:0304:0102:0304:0102:0304";
static const uint16_t PORT = 10;

/* UNIT TESTS ****************************************************************/

TEST(cardano_single_host_addr_relay_new, canCreate)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_new(&PORT, ipv4_addr, ipv6_addr, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_addr_relay, testing::Not((cardano_single_host_addr_relay_t*)nullptr));

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_new, canCreateWithoutPort)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_addr_relay, testing::Not((cardano_single_host_addr_relay_t*)nullptr));

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_new, returnsErrorIfRelayIsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_addr_relay_new(nullptr, nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_single_host_addr_relay_new, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  const uint16_t port = 10;

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_new(&port, ipv4_addr, ipv6_addr, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_single_host_addr_relay_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_single_host_addr_relay_new(&PORT, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_to_cbor(single_host_addr_relay, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_cbor_writer_unref(&writer);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
  free(actual_cbor);
}

TEST(cardano_single_host_addr_relay_to_cbor, canSerializeWithoutPort)
{
  // Arrange
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_to_cbor(single_host_addr_relay, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("8400f6440a03020a5001020304010203040102030401020304") + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "8400f6440a03020a5001020304010203040102030401020304");

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_cbor_writer_unref(&writer);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
  free(actual_cbor);
}

TEST(cardano_single_host_addr_relay_to_cbor, canSerializeWillAllNull)
{
  // Arrange
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;

  // Act
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, NULL, NULL, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_to_cbor(single_host_addr_relay, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen("8400f6f6f6") + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "8400f6f6f6");

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_single_host_addr_relay_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_single_host_addr_relay_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_to_cbor(single_host_addr_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_from_cbor, canDeserialize)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_addr_relay, testing::Not((cardano_single_host_addr_relay_t*)nullptr));

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfUrlIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(nullptr, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("82", 2);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'single_host_addr_relay', expected a 'Major Type: Byte String' (2) of 4 element(s) but got a 'Major Type: Byte String' (2) of 2 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfCborDataInvalidPort)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8400ef", 4);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfCborDataInvalidPort2)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8400ef", 6);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfCborDataInvalidIpv4)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("840000ef", 8);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfCborDataInvalidIpv6)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("840000440A03020Aef", strlen("840000440A03020Aef"));

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotUint)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("84ff", 4);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Unexpected break byte.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_single_host_addr_relay_ref(single_host_addr_relay);

  // Assert
  EXPECT_THAT(single_host_addr_relay, testing::Not((cardano_single_host_addr_relay_t*)nullptr));
  EXPECT_EQ(cardano_single_host_addr_relay_refcount(single_host_addr_relay), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);

  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_single_host_addr_relay_ref(nullptr);
}

TEST(cardano_single_host_addr_relay_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;

  // Act
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
}

TEST(cardano_single_host_addr_relay_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_single_host_addr_relay_unref((cardano_single_host_addr_relay_t**)nullptr);
}

TEST(cardano_single_host_addr_relay_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_single_host_addr_relay_ref(single_host_addr_relay);
  size_t ref_count = cardano_single_host_addr_relay_refcount(single_host_addr_relay);

  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  size_t updated_ref_count = cardano_single_host_addr_relay_refcount(single_host_addr_relay);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_single_host_addr_relay_ref(single_host_addr_relay);
  size_t ref_count = cardano_single_host_addr_relay_refcount(single_host_addr_relay);

  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  size_t updated_ref_count = cardano_single_host_addr_relay_refcount(single_host_addr_relay);

  cardano_single_host_addr_relay_unref(&single_host_addr_relay);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(single_host_addr_relay, (cardano_single_host_addr_relay_t*)nullptr);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_single_host_addr_relay_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_single_host_addr_relay_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  const char*                       message                = "This is a test message";

  // Act
  cardano_single_host_addr_relay_set_last_error(single_host_addr_relay, message);

  // Assert
  EXPECT_STREQ(cardano_single_host_addr_relay_get_last_error(single_host_addr_relay), "Object is NULL.");
}

TEST(cardano_single_host_addr_relay_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_single_host_addr_relay_set_last_error(single_host_addr_relay, message);

  // Assert
  EXPECT_STREQ(cardano_single_host_addr_relay_get_last_error(single_host_addr_relay), "");

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_from_cbor, returnsErrorIfMemoryIsInvalid)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("84d81ea20102d81e820103", strlen("82026b6578616d706c652e636f6d"));

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_addr_relay_to_cbor, returnErrorIfWriterIsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(NULL, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_to_cbor(single_host_addr_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_to_cbor, returnErrorIfObjectIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_single_host_addr_relay_get_port, returnsPort)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t port = 10;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(&port, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  const uint16_t* actual_port = cardano_single_host_addr_relay_get_port(single_host_addr_relay);

  // Assert
  EXPECT_EQ(*actual_port, port);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_get_port, returnsNullIfObjectIsNull)
{
  // Act
  const uint16_t* actual_port = cardano_single_host_addr_relay_get_port(nullptr);

  // Assert
  EXPECT_EQ(actual_port, (uint16_t*)nullptr);
}

TEST(cardano_single_host_addr_relay_get_ipv4, returnsIpv4)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t port = 10;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(&port, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_ipv4_t* actual_ipv4 = NULL;

  EXPECT_EQ(cardano_single_host_addr_relay_get_ipv4(single_host_addr_relay, &actual_ipv4), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(actual_ipv4, testing::Not((cardano_ipv4_t*)nullptr));

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&actual_ipv4);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_get_ipv4, returnsNullIfObjectIsNull)
{
  // Arrange
  cardano_ipv4_t* ipv4_addr = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_get_ipv4(nullptr, &ipv4_addr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_get_ipv4, returnsErrorIfIpv4IsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_addr_relay_get_ipv4((cardano_single_host_addr_relay_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_get_ipv6, returnsIpv6)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t port = 10;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(&port, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_ipv6_t* actual_ipv6 = NULL;

  EXPECT_EQ(cardano_single_host_addr_relay_get_ipv6(single_host_addr_relay, &actual_ipv6), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(actual_ipv6, testing::Not((cardano_ipv6_t*)nullptr));

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
  cardano_ipv6_unref(&actual_ipv6);
}

TEST(cardano_single_host_addr_relay_get_ipv6, returnsNullIfObjectIsNull)
{
  // Arrange
  cardano_ipv6_t* ipv6_addr = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_get_ipv6(nullptr, &ipv6_addr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_get_ipv6, returnsErrorIfIpv6IsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_addr_relay_get_ipv6((cardano_single_host_addr_relay_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_set_port, canSetPort)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t port     = 10;
  const uint16_t new_port = 20;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(&port, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_port(single_host_addr_relay, &new_port);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(*cardano_single_host_addr_relay_get_port(single_host_addr_relay), new_port);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_set_port, canSetPortToZero)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t port     = 10;
  const uint16_t new_port = 0;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(&port, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_port(single_host_addr_relay, &new_port);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(*cardano_single_host_addr_relay_get_port(single_host_addr_relay), new_port);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_set_port, canUnsetPortWithNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t port = 10;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(&port, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_port(single_host_addr_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_get_port(single_host_addr_relay), (uint16_t*)nullptr);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_set_port, returnErrorIfRelayIsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_port(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_set_port, canSetPortThatWasNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t new_port = 0;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(nullptr, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_port(single_host_addr_relay, &new_port);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(*cardano_single_host_addr_relay_get_port(single_host_addr_relay), new_port);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_set_port, returnsMemoryAllocationErrorWhileSetPortThatWasNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  const uint16_t new_port = 0;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_addr_relay_new(nullptr, ipv4_addr, ipv6_addr, &single_host_addr_relay), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_port(single_host_addr_relay, &new_port);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_single_host_addr_relay_set_ipv4, canSetIpv4)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  cardano_error_t error = cardano_single_host_addr_relay_new(&PORT, ipv4_addr, ipv6_addr, &single_host_addr_relay);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv4_t* new_ipv4_addr = nullptr;

  error = cardano_ipv4_from_string("10.3.2.10", strlen("10.3.2.10"), &new_ipv4_addr);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_single_host_addr_relay_set_ipv4(single_host_addr_relay, new_ipv4_addr);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_addr_relay, testing::Not((cardano_single_host_addr_relay_t*)nullptr));

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&new_ipv4_addr);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_set_ipv4, returnsErrorIfIpv4IsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  cardano_error_t error = cardano_single_host_addr_relay_new(&PORT, nullptr, ipv6_addr, &single_host_addr_relay);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_single_host_addr_relay_set_ipv4(single_host_addr_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv6_unref(&ipv6_addr);
}

TEST(cardano_single_host_addr_relay_set_ipv4, returnErrorIfRelayIsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_ipv4(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_addr_relay_set_ipv6, canSetIpv6)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;
  cardano_ipv6_t*                   ipv6_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ipv6_from_string(IPV6, strlen(IPV6), &ipv6_addr), CARDANO_SUCCESS);

  cardano_error_t error = cardano_single_host_addr_relay_new(&PORT, ipv4_addr, ipv6_addr, &single_host_addr_relay);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ipv6_t* new_ipv6_addr = nullptr;

  error = cardano_ipv6_from_string("2001:0db8:85a3:0000:0000:8a2e:0370:7334", strlen("2001:0db8:85a3:0000:0000:8a2e:0370:7334"), &new_ipv6_addr);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_single_host_addr_relay_set_ipv6(single_host_addr_relay, new_ipv6_addr);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_addr_relay, testing::Not((cardano_single_host_addr_relay_t*)nullptr));

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
  cardano_ipv6_unref(&ipv6_addr);
  cardano_ipv6_unref(&new_ipv6_addr);
}

TEST(cardano_single_host_addr_relay_set_ipv6, returnsErrorIfIpv6IsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr_relay = nullptr;
  cardano_ipv4_t*                   ipv4_addr              = nullptr;

  EXPECT_EQ(cardano_ipv4_from_string(IPV4, strlen(IPV4), &ipv4_addr), CARDANO_SUCCESS);

  cardano_error_t error = cardano_single_host_addr_relay_new(&PORT, ipv4_addr, nullptr, &single_host_addr_relay);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_single_host_addr_relay_set_ipv6(single_host_addr_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr_relay);
  cardano_ipv4_unref(&ipv4_addr);
}

TEST(cardano_single_host_addr_relay_set_ipv6, returnErrorIfRelayIsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_addr_relay_set_ipv6(nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}