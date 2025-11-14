/**
 * \file relay.cpp
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/pool_params/relay.h>

#include "../json_helpers.h"
#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <cardano/pool_params/multi_host_name_relay.h>
#include <cardano/pool_params/relay.h>
#include <cardano/pool_params/single_host_addr_relay.h>
#include <cardano/pool_params/single_host_name_relay.h>
#include <gmock/gmock.h>

// clang-format on

/* CONSTS ********************************************************************/

static const char* SINGLE_HOST_NAME_RELAY_CBOR                  = "83010a6b6578616d706c652e636f6d";
static const char* SINGLE_HOST_NAME_RELAY_NO_PORT_CBOR          = "8301f66b6578616d706c652e636f6d";
static const char* MULTI_HOST_NAME_RELAY_CBOR                   = "82026b6578616d706c652e636f6d";
static const char* SINGLE_HOST_ADDR_RELAY_CBOR                  = "84000a440a03020a5001020304010203040102030401020304";
static const char* SINGLE_HOST_ADDR_RELAY_IPV4_MAPPED_IPV6_CBOR = "84000a440a03020a5000000000000000000000ffff0a03020a";

/* UNIT TESTS ****************************************************************/

TEST(cardano_relay_new_single_host_addr, canCreateRelay)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_addr_relay_t* single_host_addr = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_relay_new_single_host_addr(single_host_addr, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(relay, nullptr);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_addr_relay_unref(&single_host_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_single_host_addr, returnsErrorIfSingleHostAddrIsNull)
{
  // Arrange
  cardano_relay_t* relay = NULL;

  // Act
  cardano_error_t result = cardano_relay_new_single_host_addr(NULL, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(relay, nullptr);
}

TEST(cardano_relay_new_single_host_addr, returnsErrorIfRelayIsNull)
{
  cardano_single_host_addr_relay_t* single_host_addr = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_single_host_addr(single_host_addr, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_single_host_addr, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_addr_relay_t* single_host_addr = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_relay_new_single_host_addr(single_host_addr, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(relay, nullptr);

  // Cleanup
  cardano_single_host_addr_relay_unref(&single_host_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_single_host_name, canCreateRelay)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_name_relay_t* single_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_relay_new_single_host_name(single_host_name, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(relay, nullptr);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_single_host_name, returnsErrorIfSingleHostAddrIsNull)
{
  // Arrange
  cardano_relay_t* relay = NULL;

  // Act
  cardano_error_t result = cardano_relay_new_single_host_name(NULL, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(relay, nullptr);
}

TEST(cardano_relay_new_single_host_name, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_relay_new_single_host_name(single_host_name, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_single_host_name, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_name_relay_t* single_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  result = cardano_relay_new_single_host_name(single_host_name, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(relay, nullptr);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_single_host_name, canCreateRelayWithoutPort)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_name_relay_t* single_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_NO_PORT_CBOR, strlen(SINGLE_HOST_NAME_RELAY_NO_PORT_CBOR));
  cardano_error_t        result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_relay_new_single_host_name(single_host_name, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(relay, nullptr);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_multi_host_name, canCreateRelay)
{
  // Arrange
  cardano_relay_t*                 relay           = NULL;
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_relay_new_multi_host_name(multi_host_name, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(relay, nullptr);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&multi_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_multi_host_name, returnsErrorIfMultiHostNameIsNull)
{
  // Arrange
  cardano_relay_t* relay = NULL;

  // Act
  cardano_error_t result = cardano_relay_new_multi_host_name(NULL, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(relay, nullptr);
}

TEST(cardano_relay_new_multi_host_name, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_relay_new_multi_host_name(multi_host_name, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_host_name_relay_unref(&multi_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_new_multi_host_name, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_relay_t*                 relay           = NULL;
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  result = cardano_relay_new_multi_host_name(multi_host_name, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(relay, nullptr);

  // Cleanup
  cardano_multi_host_name_relay_unref(&multi_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, canCreateRelayFromCBORSingleHostName)
{
  // Arrange
  cardano_relay_t*       relay  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(relay, nullptr);

  cardano_relay_type_t type;

  ASSERT_EQ(cardano_relay_get_type(relay, &type), CARDANO_SUCCESS);
  ASSERT_EQ(type, CARDANO_RELAY_TYPE_SINGLE_HOST_NAME);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, canCreateRelayFromCBORMultiHostName)
{
  // Arrange
  cardano_relay_t*       relay  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(relay, nullptr);

  cardano_relay_type_t type;

  ASSERT_EQ(cardano_relay_get_type(relay, &type), CARDANO_SUCCESS);
  ASSERT_EQ(type, CARDANO_RELAY_TYPE_MULTI_HOST_NAME);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, canCreateRelayFromCBORSingleHostAddr)
{
  // Arrange
  cardano_relay_t*       relay  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(relay, nullptr);

  cardano_relay_type_t type;

  ASSERT_EQ(cardano_relay_get_type(relay, &type), CARDANO_SUCCESS);
  ASSERT_EQ(type, CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, returnsErrorIfInvalidCBOR)
{
  // Arrange
  cardano_relay_t*       relay  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("a10101", strlen("a10101"));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_cbor, canConvertSingleHostAddreRelayToCBOR)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_addr_relay_t* single_host_addr = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_single_host_addr(single_host_addr, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_relay_to_cbor(relay, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hex, SINGLE_HOST_ADDR_RELAY_CBOR);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_addr_relay_unref(&single_host_addr);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_relay_to_cbor, canConvertSingleHostNameRelayToCBOR)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_name_relay_t* single_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_single_host_name(single_host_name, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_relay_to_cbor(relay, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hex, SINGLE_HOST_NAME_RELAY_CBOR);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_relay_to_cbor, canConvertMultiHostNameRelayToCBOR)
{
  // Arrange
  cardano_relay_t*                 relay           = NULL;
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_multi_host_name(multi_host_name, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_relay_to_cbor(relay, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hex, MULTI_HOST_NAME_RELAY_CBOR);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&multi_host_name);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_relay_to_cbor, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_relay_to_cbor(NULL, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_relay_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_name_relay_t* single_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_single_host_name(single_host_name, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_relay_to_cbor(relay, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_single_host_addr, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_single_host_addr_relay_t* single_host_addr = NULL;

  // Act
  cardano_error_t result = cardano_relay_to_single_host_addr(NULL, &single_host_addr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(single_host_addr, nullptr);
}

TEST(cardano_relay_to_single_host_addr, returnsErrorIfSingleHostAddrIsNull)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_addr_relay_t* single_host_addr = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_relay_new_single_host_addr(single_host_addr, &relay), CARDANO_SUCCESS);

  cardano_single_host_addr_relay_t* single_host_addr2 = NULL;

  result = cardano_relay_to_single_host_addr(relay, &single_host_addr2);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(single_host_addr2, nullptr);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_addr_relay_unref(&single_host_addr2);
  cardano_single_host_addr_relay_unref(&single_host_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_single_host_name, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_single_host_name_relay_t* single_host_name = NULL;

  // Act
  cardano_error_t result = cardano_relay_to_single_host_name(NULL, &single_host_name);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(single_host_name, nullptr);
}

TEST(cardano_relay_to_single_host_name, returnsErrorIfSingleHostAddrIsNull)
{
  // Arrange
  cardano_relay_t*                  relay            = NULL;
  cardano_single_host_name_relay_t* single_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_relay_new_single_host_name(single_host_name, &relay), CARDANO_SUCCESS);

  cardano_single_host_name_relay_t* single_host_name2 = NULL;

  // Act
  result = cardano_relay_to_single_host_name(relay, &single_host_name2);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(single_host_name2, nullptr);

  // Cleanup
  cardano_single_host_name_relay_unref(&single_host_name2);
  cardano_relay_unref(&relay);
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_multi_host_name, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  // Act
  cardano_error_t result = cardano_relay_to_multi_host_name(NULL, &multi_host_name);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(multi_host_name, nullptr);
}

TEST(cardano_relay_to_multi_host_name, returnsErrorIfMultiHostNameIsNull)
{
  // Arrange
  cardano_relay_t*                 relay           = NULL;
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_relay_new_multi_host_name(multi_host_name, &relay), CARDANO_SUCCESS);

  cardano_multi_host_name_relay_t* multi_host_name2 = NULL;

  // Act
  result = cardano_relay_to_multi_host_name(relay, &multi_host_name2);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(multi_host_name2, nullptr);

  // Cleanup
  cardano_multi_host_name_relay_unref(&multi_host_name2);
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&multi_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_relay_t*                 relay           = NULL;
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_multi_host_name(multi_host_name, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_relay_ref(relay);

  // Assert
  EXPECT_THAT(relay, testing::Not((cardano_relay_t*)nullptr));
  EXPECT_EQ(cardano_relay_refcount(relay), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_relay_unref(&relay);
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&multi_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_relay_ref(nullptr);
}

TEST(cardano_relay_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_relay_t* relay = nullptr;

  // Act
  cardano_relay_unref(&relay);
}

TEST(cardano_relay_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_relay_unref((cardano_relay_t**)nullptr);
}

TEST(cardano_relay_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_relay_t*                 relay           = NULL;
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_multi_host_name(multi_host_name, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_relay_ref(relay);
  size_t ref_count = cardano_relay_refcount(relay);

  cardano_relay_unref(&relay);
  size_t updated_ref_count = cardano_relay_refcount(relay);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
  cardano_multi_host_name_relay_unref(&multi_host_name);
}

TEST(cardano_relay_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_relay_t*                 relay           = NULL;
  cardano_multi_host_name_relay_t* multi_host_name = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_relay_new_multi_host_name(multi_host_name, &relay);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_relay_ref(relay);
  size_t ref_count = cardano_relay_refcount(relay);

  cardano_relay_unref(&relay);
  size_t updated_ref_count = cardano_relay_refcount(relay);

  cardano_relay_unref(&relay);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(relay, (cardano_relay_t*)nullptr);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
  cardano_multi_host_name_relay_unref(&multi_host_name);
}

TEST(cardano_relay_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_relay_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_relay_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_relay_t* relay   = nullptr;
  const char*      message = "This is a test message";

  // Act
  cardano_relay_set_last_error(relay, message);

  // Assert
  EXPECT_STREQ(cardano_relay_get_last_error(relay), "Object is NULL.");
}

TEST(cardano_relay_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_relay_t*       relay  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        error  = cardano_relay_from_cbor(reader, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_relay_set_last_error(relay, message);

  // Assert
  EXPECT_STREQ(cardano_relay_get_last_error(relay), "");

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, returnsErrorWhenReaderIsNull)
{
  // Arrange
  cardano_relay_t* relay = nullptr;

  // Act
  cardano_error_t result = cardano_relay_from_cbor(nullptr, &relay);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(relay, nullptr);
}

TEST(cardano_relay_from_cbor, returnsErrorWhenMemoryAllocationFails)
{
  // Arrange
  cardano_relay_t*       relay  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(relay, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, returnsErrorIfInvalidCbor)
{
  // Arrange
  cardano_relay_t*       relay  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("81ef", strlen("81ef"));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(relay, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, returnsErrorIfInvalidSingleHostAddrCbor)
{
  // Arrange
  cardano_relay_t*       relay  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200ef", strlen("8200ef"));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(relay, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, returnsErrorIfInvalidSingleHostNameCbor)
{
  // Arrange
  cardano_relay_t*       relay  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8201ef", strlen("8201ef"));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(relay, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_from_cbor, returnsErrorIfInvalidMultiHostNameCbor)
{
  // Arrange
  cardano_relay_t*       relay  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8202ef", strlen("8202ef"));

  // Act
  cardano_error_t result = cardano_relay_from_cbor(reader, &relay);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(relay, nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_get_type, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_relay_t*     relay = nullptr;
  cardano_relay_type_t type;

  // Act
  cardano_error_t result = cardano_relay_get_type(relay, &type);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_relay_get_type, returnsErrorIfTypeIsNull)
{
  // Arrange
  cardano_relay_t* relay = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        error  = cardano_relay_from_cbor(reader, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_relay_get_type(relay, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_single_host_addr, returnsErrorIfSingleHostAddressIsNull)
{
  // Arrange
  cardano_relay_t*                  relay            = nullptr;
  cardano_single_host_addr_relay_t* single_host_addr = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        error  = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_relay_new_single_host_addr(single_host_addr, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_relay_to_single_host_addr(relay, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_addr_relay_unref(&single_host_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_single_host_addr, returnsErrorIfInvalidRelayType)
{
  // Arrange
  cardano_relay_t*                 relay          = nullptr;
  cardano_multi_host_name_relay_t* host_name_addr = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        error  = cardano_multi_host_name_relay_from_cbor(reader, &host_name_addr);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_relay_new_multi_host_name(host_name_addr, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_single_host_addr_relay_t* single_host_addr = nullptr;
  cardano_error_t                   result           = cardano_relay_to_single_host_addr(relay, &single_host_addr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&host_name_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_single_host_name, returnsErrorIfSingleHostNameIsNull)
{
  // Arrange
  cardano_relay_t*                  relay            = nullptr;
  cardano_single_host_name_relay_t* single_host_name = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_NAME_RELAY_CBOR, strlen(SINGLE_HOST_NAME_RELAY_CBOR));
  cardano_error_t        error  = cardano_single_host_name_relay_from_cbor(reader, &single_host_name);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_relay_new_single_host_name(single_host_name, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_relay_to_single_host_name(relay, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_name_relay_unref(&single_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_single_host_name, returnsErrorIfInvalidRelayType)
{
  // Arrange
  cardano_relay_t*                  relay          = nullptr;
  cardano_single_host_addr_relay_t* host_name_addr = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        error  = cardano_single_host_addr_relay_from_cbor(reader, &host_name_addr);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_relay_new_single_host_addr(host_name_addr, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_single_host_name_relay_t* single_host_name = nullptr;
  cardano_error_t                   result           = cardano_relay_to_single_host_name(relay, &single_host_name);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_addr_relay_unref(&host_name_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_multi_host_name, returnsErrorIfInvalidRelayType)
{
  // Arrange
  cardano_relay_t*                  relay          = nullptr;
  cardano_single_host_addr_relay_t* host_name_addr = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SINGLE_HOST_ADDR_RELAY_CBOR, strlen(SINGLE_HOST_ADDR_RELAY_CBOR));
  cardano_error_t        error  = cardano_single_host_addr_relay_from_cbor(reader, &host_name_addr);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_relay_new_single_host_addr(host_name_addr, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_multi_host_name_relay_t* multi_host_name = nullptr;
  cardano_error_t                  result          = cardano_relay_to_multi_host_name(relay, &multi_host_name);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_single_host_addr_relay_unref(&host_name_addr);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_multi_host_name, returnsErrorWhenMultiHostIsNull)
{
  // Arrange
  cardano_relay_t*                 relay           = nullptr;
  cardano_multi_host_name_relay_t* multi_host_name = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MULTI_HOST_NAME_RELAY_CBOR, strlen(MULTI_HOST_NAME_RELAY_CBOR));
  cardano_error_t        error  = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_relay_new_multi_host_name(multi_host_name, &relay);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_relay_to_multi_host_name(relay, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&multi_host_name);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_relay_to_cip116_json, canConvertSingleHostAddrRelay)
{
  // Arrange
  uint16_t        port = 3000;
  cardano_ipv4_t* ipv4 = NULL;
  EXPECT_EQ(cardano_ipv4_from_string("127.0.0.1", 9, &ipv4), CARDANO_SUCCESS);

  cardano_single_host_addr_relay_t* addr_relay = NULL;
  EXPECT_EQ(cardano_single_host_addr_relay_new(&port, ipv4, NULL, &addr_relay), CARDANO_SUCCESS);

  cardano_relay_t* relay = NULL;
  EXPECT_EQ(cardano_relay_new_single_host_addr(addr_relay, &relay), CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_relay_to_cip116_json(relay, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"tag":"single_host_addr","port":3000,"ipv4":"127.0.0.1","ipv6":null})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_relay_unref(&relay);
  cardano_single_host_addr_relay_unref(&addr_relay);
  cardano_ipv4_unref(&ipv4);
  free(json_str);
}

TEST(cardano_relay_to_cip116_json, canConvertSingleHostNameRelay)
{
  // Arrange
  uint16_t                          port       = 4000;
  cardano_single_host_name_relay_t* name_relay = NULL;
  EXPECT_EQ(cardano_single_host_name_relay_new(&port, "relay.io", strlen("relay.io"), &name_relay), CARDANO_SUCCESS);

  cardano_relay_t* relay = NULL;
  EXPECT_EQ(cardano_relay_new_single_host_name(name_relay, &relay), CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_relay_to_cip116_json(relay, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"tag":"single_host_name","port":4000,"dns_name":"relay.io"})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_relay_unref(&relay);
  cardano_single_host_name_relay_unref(&name_relay);
  free(json_str);
}

TEST(cardano_relay_to_cip116_json, canConvertMultiHostNameRelay)
{
  // Arrange
  cardano_multi_host_name_relay_t* multi_relay = NULL;
  EXPECT_EQ(cardano_multi_host_name_relay_new("multi.io", strlen("multi.io"), &multi_relay), CARDANO_SUCCESS);

  cardano_relay_t* relay = NULL;
  EXPECT_EQ(cardano_relay_new_multi_host_name(multi_relay, &relay), CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_relay_to_cip116_json(relay, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"tag":"multi_host_name","dns_name":"multi.io"})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&multi_relay);
  free(json_str);
}

TEST(cardano_relay_to_cip116_json, returnsErrorIfRelayIsNull)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error = cardano_relay_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_relay_to_cip116_json, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_multi_host_name_relay_t* multi_relay = NULL;
  EXPECT_EQ(cardano_multi_host_name_relay_new("multi.io", strlen("multi.io"), &multi_relay), CARDANO_SUCCESS);

  cardano_relay_t* relay = NULL;
  EXPECT_EQ(cardano_relay_new_multi_host_name(multi_relay, &relay), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_relay_to_cip116_json(relay, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_relay_unref(&relay);
  cardano_multi_host_name_relay_unref(&multi_relay);
}