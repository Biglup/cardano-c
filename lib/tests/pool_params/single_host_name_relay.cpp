/**
 * \file single_host_name_relay.cpp
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

#include <cardano/pool_params/single_host_name_relay.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR           = "8301f66b6578616d706c652e636f6d";
static const char* CBOR_WITH_PORT = "83010a6b6578616d706c652e636f6d";

static const char* URL = "example.com";

/* UNIT TESTS ****************************************************************/

TEST(cardano_single_host_name_relay_new, canCreate)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_name_relay, testing::Not((cardano_single_host_name_relay_t*)nullptr));

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_new, canCreateWithPort)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  const uint16_t port = 8080;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(&port, URL, strlen(URL), &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_name_relay, testing::Not((cardano_single_host_name_relay_t*)nullptr));

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_new, returnsErrorIfRelayIsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(NULL, URL, strlen(URL), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_name_relay_new, returnsErrorIfDnsIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(NULL, nullptr, 0, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(single_host_name_relay, (cardano_single_host_name_relay_t*)nullptr);
}

TEST(cardano_single_host_name_relay_new, returnsErrorIfDnsSizeIsZero)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(NULL, URL, 0, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(single_host_name_relay, (cardano_single_host_name_relay_t*)nullptr);
}

TEST(cardano_single_host_name_relay_new, returnsErrorIfDnsSizeIsGreaterThan64)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(NULL, URL, 65, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(single_host_name_relay, (cardano_single_host_name_relay_t*)nullptr);
}

TEST(cardano_single_host_name_relay_new, returnsErrorIfMemoryAllocationFails)
{
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(single_host_name_relay, (cardano_single_host_name_relay_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_single_host_name_relay_new, returnsErrorIfMemoryAllocationFails2)
{
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  const uint16_t port = 8080;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_new(&port, URL, strlen(URL), &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(single_host_name_relay, (cardano_single_host_name_relay_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_single_host_name_relay_to_cbor, canSerialize)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_to_cbor(single_host_name_relay, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_single_host_name_relay_to_cbor, canSerializeWithPort)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  const uint16_t port = 10;

  EXPECT_EQ(cardano_single_host_name_relay_new(&port, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_to_cbor(single_host_name_relay, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_WITH_PORT) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITH_PORT);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_single_host_name_relay_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_single_host_name_relay_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_single_host_name_relay_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  cardano_error_t error = cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_single_host_name_relay_to_cbor(single_host_name_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_from_cbor, canDeserialize)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_name_relay, testing::Not((cardano_single_host_name_relay_t*)nullptr));

  const char* dns = cardano_single_host_name_relay_get_dns(single_host_name_relay);

  EXPECT_STREQ(dns, URL);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_from_cbor, canDeserializeWithPort)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR_WITH_PORT, strlen(CBOR_WITH_PORT));

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(single_host_name_relay, testing::Not((cardano_single_host_name_relay_t*)nullptr));

  const char*     dns  = cardano_single_host_name_relay_get_dns(single_host_name_relay);
  const uint16_t* port = cardano_single_host_name_relay_get_port(single_host_name_relay);

  EXPECT_EQ(*port, 10);
  EXPECT_STREQ(dns, URL);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_from_cbor, returnErrorIfUrlIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(nullptr, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_name_relay_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'single_host_name_relay', expected a 'Major Type: Byte String' (2) of 3 element(s) but got a 'Major Type: Byte String' (2) of 1 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_from_cbor, returnErrorIfCborDataFirstElementInArrayIsNotUint)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("82ff", 4);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'single_host_name_relay', expected a 'Major Type: Byte String' (2) of 3 element(s) but got a 'Major Type: Byte String' (2) of 2 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_from_cbor, returnErrorIfSecondElementIsNotTextString)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8202ef", 6);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'single_host_name_relay', expected a 'Major Type: Byte String' (2) of 3 element(s) but got a 'Major Type: Byte String' (2) of 2 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_single_host_name_relay_ref(single_host_name_relay);

  // Assert
  EXPECT_THAT(single_host_name_relay, testing::Not((cardano_single_host_name_relay_t*)nullptr));
  EXPECT_EQ(cardano_single_host_name_relay_refcount(single_host_name_relay), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_single_host_name_relay_unref(&single_host_name_relay);
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_single_host_name_relay_ref(nullptr);
}

TEST(cardano_single_host_name_relay_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  // Act
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_single_host_name_relay_unref((cardano_single_host_name_relay_t**)nullptr);
}

TEST(cardano_single_host_name_relay_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_single_host_name_relay_ref(single_host_name_relay);
  size_t ref_count = cardano_single_host_name_relay_refcount(single_host_name_relay);

  cardano_single_host_name_relay_unref(&single_host_name_relay);
  size_t updated_ref_count = cardano_single_host_name_relay_refcount(single_host_name_relay);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_single_host_name_relay_ref(single_host_name_relay);
  size_t ref_count = cardano_single_host_name_relay_refcount(single_host_name_relay);

  cardano_single_host_name_relay_unref(&single_host_name_relay);
  size_t updated_ref_count = cardano_single_host_name_relay_refcount(single_host_name_relay);

  cardano_single_host_name_relay_unref(&single_host_name_relay);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(single_host_name_relay, (cardano_single_host_name_relay_t*)nullptr);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_single_host_name_relay_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_single_host_name_relay_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  const char*                       message                = "This is a test message";

  // Act
  cardano_single_host_name_relay_set_last_error(single_host_name_relay, message);

  // Assert
  EXPECT_STREQ(cardano_single_host_name_relay_get_last_error(single_host_name_relay), "Object is NULL.");
}

TEST(cardano_single_host_name_relay_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_single_host_name_relay_set_last_error(single_host_name_relay, message);

  // Assert
  EXPECT_STREQ(cardano_single_host_name_relay_get_last_error(single_host_name_relay), "");

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_from_cbor, returnsErrorIfMemoryIsInvalid)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("83d81ea20102d81e820103", strlen("82026b6578616d706c652e636f6d"));

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_from_cbor, returnsErrorIfStepsIsInvalid)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("83d81e820102d81ea20103", strlen("82d81e820102d81ea20103"));

  // Act
  cardano_error_t error = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_single_host_name_relay_to_cbor, returnErrorIfWriterIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_to_cbor(single_host_name_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_to_cbor, returnErrorIfObjectIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_single_host_name_relay_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_single_host_name_relay_get_dns_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t dns_size = cardano_single_host_name_relay_get_dns_size(nullptr);

  // Assert
  EXPECT_EQ(dns_size, 0);
}

TEST(cardano_single_host_name_relay_get_dns_size, canGetDnsSize)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  size_t dns_size = cardano_single_host_name_relay_get_dns_size(single_host_name_relay);

  // Assert
  EXPECT_EQ(dns_size, strlen(URL) + 1);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_set_dns, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_dns(URL, strlen(URL), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_name_relay_set_dns, returnsErrorIfDnsIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_dns(nullptr, 0, single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_set_dns, returnsErrorIfDnsSizeIsGreaterThan64)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_dns(URL, 65, single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_set_dns, canSetDns)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_dns("new.example.com", strlen("new.example.com"), single_host_name_relay);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_single_host_name_relay_get_dns(single_host_name_relay), "new.example.com");

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_get_dns, returnsNullIfObjectIsNull)
{
  // Act
  const char* dns = cardano_single_host_name_relay_get_dns(nullptr);

  // Assert
  EXPECT_EQ(dns, (const char*)nullptr);
}

TEST(cardano_single_host_name_relay_get_port, returnsNullIfObjectIsNull)
{
  // Act
  const uint16_t* port = cardano_single_host_name_relay_get_port(nullptr);

  // Assert
  EXPECT_EQ(port, (const uint16_t*)nullptr);
}

TEST(cardano_single_host_name_relay_set_port, returnsErrorIfObjectIsNull)
{
  // Act
  const uint16_t port = 8080;

  cardano_error_t error = cardano_single_host_name_relay_set_port(nullptr, &port);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_single_host_name_relay_set_port, canSetNullPort)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_port(single_host_name_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_name_relay_get_port(single_host_name_relay), (const uint16_t*)nullptr);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_set_port, canSetPort)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  const uint16_t port = 8080;

  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_port(single_host_name_relay, &port);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(*cardano_single_host_name_relay_get_port(single_host_name_relay), port);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_set_port, canUnsetAPortByGivingNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  const uint16_t port = 8080;

  EXPECT_EQ(cardano_single_host_name_relay_new(&port, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_port(single_host_name_relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_single_host_name_relay_get_port(single_host_name_relay), (const uint16_t*)nullptr);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}

TEST(cardano_single_host_name_relay_set_port, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name_relay = nullptr;

  const uint16_t port = 8080;

  EXPECT_EQ(cardano_single_host_name_relay_new(NULL, URL, strlen(URL), &single_host_name_relay), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_single_host_name_relay_set_port(single_host_name_relay, &port);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_single_host_name_relay_unref(&single_host_name_relay);
}