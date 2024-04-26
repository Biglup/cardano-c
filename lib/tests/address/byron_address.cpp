/**
 * \file byron_address.cpp
 *
 * \author angel.castillo
 * \date   Apr 15, 2024
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

#include <cardano/address/byron_address.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"
#include "cip19_test_vectors.h"

extern "C" {
#include "../../src/address/internals/addr_common.h"
}

#include <cardano/address/address.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_byron_address_from_credentials, returnsErrorWhenRootHashIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_credentials(NULL, Cip19TestVectors::byronAttributes, CARDANO_BYRON_ADDRESS_TYPE_PUBKEY, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_credentials, returnsErrorWhenbyronAddressIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = (cardano_blake2b_hash_t*)"";

  // Act
  cardano_error_t result = cardano_byron_address_from_credentials(hash, Cip19TestVectors::byronAttributes, CARDANO_BYRON_ADDRESS_TYPE_PUBKEY, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_byron_address_from_credentials, canCreateAByronAddressFromCredential)
{
  // Arrange
  cardano_blake2b_hash_t*  hash          = NULL;
  cardano_byron_address_t* byron_address = NULL;

  EXPECT_EQ(
    cardano_blake2b_hash_from_hex(
      Cip19TestVectors::byronYoroiMainnetRootHex.c_str(),
      Cip19TestVectors::byronYoroiMainnetRootHex.size(),
      &hash),
    CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_byron_address_from_credentials(hash, Cip19TestVectors::byronAttributes, CARDANO_BYRON_ADDRESS_TYPE_PUBKEY, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(byron_address, nullptr);
  EXPECT_EQ(cardano_byron_address_get_string(byron_address), Cip19TestVectors::byronMainnetYoroi);

  // Clean up
  cardano_blake2b_hash_unref(&hash);
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_from_credentials, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t*  hash          = NULL;
  cardano_byron_address_t* byron_address = NULL;

  EXPECT_EQ(
    cardano_blake2b_hash_from_hex(
      Cip19TestVectors::byronYoroiMainnetRootHex.c_str(),
      Cip19TestVectors::byronYoroiMainnetRootHex.size(),
      &hash),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_byron_address_from_credentials(hash, Cip19TestVectors::byronAttributes, CARDANO_BYRON_ADDRESS_TYPE_PUBKEY, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(byron_address, nullptr);

  // Clean up
  cardano_blake2b_hash_unref(&hash);
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_from_credentials, returnErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_blake2b_hash_t*  hash          = NULL;
  cardano_byron_address_t* byron_address = NULL;

  EXPECT_EQ(
    cardano_blake2b_hash_from_hex(
      Cip19TestVectors::byronYoroiMainnetRootHex.c_str(),
      Cip19TestVectors::byronYoroiMainnetRootHex.size(),
      &hash),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_byron_address_from_credentials(hash, Cip19TestVectors::byronAttributes, CARDANO_BYRON_ADDRESS_TYPE_PUBKEY, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(byron_address, nullptr);

  // Clean up
  cardano_blake2b_hash_unref(&hash);
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_from_credentials, returnErrorIfEventualMemoryAllocationFails2)
{
  // Arrange
  cardano_blake2b_hash_t*  hash          = NULL;
  cardano_byron_address_t* byron_address = NULL;

  EXPECT_EQ(
    cardano_blake2b_hash_from_hex(
      Cip19TestVectors::byronYoroiMainnetRootHex.c_str(),
      Cip19TestVectors::byronYoroiMainnetRootHex.size(),
      &hash),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_byron_address_from_credentials(hash, Cip19TestVectors::byronAttributes, CARDANO_BYRON_ADDRESS_TYPE_PUBKEY, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(byron_address, nullptr);

  // Clean up
  cardano_blake2b_hash_unref(&hash);
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_from_address, returnsErrorWhenAddressIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_address(NULL, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_address, returnsErrorWhenbyronAddressIsNull)
{
  // Arrange
  cardano_address_t*       address       = (cardano_address_t*)"";
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_address(address, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_address, returnsErrorWhenAddressTypeIsInvalid)
{
  // Arrange
  cardano_address_t*       address       = NULL;
  cardano_byron_address_t* byron_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_byron_address_from_address(address, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_TYPE);
  EXPECT_EQ(byron_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_byron_address_from_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t*       address       = NULL;
  cardano_byron_address_t* byron_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::byronMainnetYoroiBytes, sizeof(Cip19TestVectors::byronMainnetYoroiBytes), &address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_byron_address_from_address(address, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(byron_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_byron_address_from_address, canConvertAByronAddressFromAValidAddress)
{
  // Arrange
  cardano_address_t*       address       = NULL;
  cardano_byron_address_t* byron_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::byronMainnetYoroiBytes, sizeof(Cip19TestVectors::byronMainnetYoroiBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_byron_address_from_address(address, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(byron_address, nullptr);
  EXPECT_EQ(cardano_byron_address_get_string(byron_address), Cip19TestVectors::byronMainnetYoroi);

  // Cleanup
  cardano_address_unref(&address);
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_to_address, returnsErrorWhenbyronAddressIsNull)
{
  // Act
  cardano_address_t* address = cardano_byron_address_to_address(NULL);

  // Assert
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_byron_address_to_address, canConvertbyronAddressToAddress)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_address_t* address = cardano_byron_address_to_address(byron_address);

  // Assert
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::byronMainnetYoroi);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
  cardano_address_unref(&address);
}

TEST(cardano_byron_address_from_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_bytes(NULL, 0, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_bytes, returnsErrorWhenbyronAddressIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_bytes(Cip19TestVectors::byronMainnetYoroiBytes, sizeof(Cip19TestVectors::byronMainnetYoroiBytes), NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_bytes, returnsErrorWhenInvalidSize)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_bytes(Cip19TestVectors::byronMainnetYoroiBytes, 0, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_bytes, canCreatebyron_addressFromByron_addressBytes)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_bytes(Cip19TestVectors::byronMainnetYoroiBytes, sizeof(Cip19TestVectors::byronMainnetYoroiBytes), &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(byron_address, nullptr);
  EXPECT_EQ(cardano_byron_address_get_string(byron_address), Cip19TestVectors::byronMainnetYoroi);

  // compare bytes
  const byte_t* bytes = cardano_byron_address_get_bytes(byron_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::byronMainnetYoroiBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::byronMainnetYoroiBytes[i]);
  }

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_from_base58, canCreatebyron_addressFromString)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(byron_address, nullptr);
  EXPECT_EQ(cardano_byron_address_get_string(byron_address), Cip19TestVectors::byronMainnetYoroi);

  // compare bytes
  const byte_t* bytes = cardano_byron_address_get_bytes(byron_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::byronMainnetYoroiBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::byronMainnetYoroiBytes[i]);
  }

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_from_base58, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_base58, returnsErrorIfSizeIsZero)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), 0, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_base58, returnsErrorIfAddressIsNull)
{
  // Act
  cardano_error_t result = cardano_byron_address_from_base58("a", 1, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_byron_address_from_base58, returnErrorIfInvalidPrefix)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_base58("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r", strlen("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r"), &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_get_bytes_size, canGetbyron_addressBytesSize)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_byron_address_get_bytes_size(byron_address);

  // Assert
  EXPECT_EQ(size, sizeof(Cip19TestVectors::byronMainnetYoroiBytes));

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_get_bytes, canGetbyron_addressBytes)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_byron_address_get_bytes(byron_address);

  // Assert
  for (size_t i = 0; i < sizeof(Cip19TestVectors::byronMainnetYoroiBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::byronMainnetYoroiBytes[i]);
  }

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_to_bytes, canConvertbyron_addressToBytes)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_byron_address_to_bytes(byron_address, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::byronMainnetYoroiBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::byronMainnetYoroiBytes[i]);
  }

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_to_bytes, returnsErrorWhenbyron_addressIsNull)
{
  // Arrange
  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_byron_address_to_bytes(NULL, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_byron_address_to_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_byron_address_to_bytes(byron_address, NULL, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_to_bytes, returnsErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_byron_address_to_bytes(byron_address, bytes, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_from_base58, returnsErrorWhenbyron_addressIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_base58(NULL, 0, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_from_base58, returnsErrorWhenbyron_addressIsInvalid)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;

  // Act
  cardano_error_t result = cardano_byron_address_from_base58("invalid_byron_address", 15, &byron_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_byron_address_get_string_size, canGetbyron_addressStringSize)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_byron_address_get_base58_size(byron_address);

  // Assert
  EXPECT_EQ(size, Cip19TestVectors::byronMainnetYoroi.size() + 1 /* null terminator */);

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_to_base58, canConvertbyron_addressToString)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_byron_address_to_base58(byron_address, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(std::string(buffer), Cip19TestVectors::byronMainnetYoroi);

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_to_base58, returnsErrorIfBufferTooSmall)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_byron_address_to_base58(byron_address, buffer, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_ref(byron_address);

  // Assert
  EXPECT_THAT(byron_address, testing::Not((cardano_byron_address_t*)nullptr));
  EXPECT_EQ(cardano_byron_address_refcount(byron_address), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_byron_address_unref(&byron_address);
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_byron_address_ref(nullptr);
}

TEST(cardano_byron_address_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_byron_address_t* byron_address = nullptr;

  // Act
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_byron_address_unref((cardano_byron_address_t**)nullptr);
}

TEST(cardano_byron_address_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_ref(byron_address);
  size_t ref_count = cardano_byron_address_refcount(byron_address);

  cardano_byron_address_unref(&byron_address);
  size_t updated_ref_count = cardano_byron_address_refcount(byron_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_ref(byron_address);
  size_t ref_count = cardano_byron_address_refcount(byron_address);

  cardano_byron_address_unref(&byron_address);
  size_t updated_ref_count = cardano_byron_address_refcount(byron_address);

  cardano_byron_address_unref(&byron_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(byron_address, (cardano_byron_address_t*)nullptr);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  EXPECT_THAT(cardano_byron_address_move(byron_address), testing::Not((cardano_byron_address_t*)nullptr));
  size_t ref_count = cardano_byron_address_refcount(byron_address);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(byron_address, testing::Not((cardano_byron_address_t*)nullptr));

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_byron_address_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_byron_address_move, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_byron_address_t* byron_address = cardano_byron_address_move(nullptr);

  // Assert
  EXPECT_EQ(byron_address, (cardano_byron_address_t*)nullptr);
}

TEST(cardano_byron_address_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  const char* message = "This is a test message";

  // Act
  cardano_byron_address_set_last_error(byron_address, message);
  const char* last_error = cardano_byron_address_get_last_error(byron_address);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_byron_address_t* byron_address = nullptr;

  // Act
  const char* last_error = cardano_byron_address_get_last_error(byron_address);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_byron_address_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_byron_address_set_last_error(byron_address, message);

  // Assert
  EXPECT_STREQ(cardano_byron_address_get_last_error(byron_address), "Object is NULL.");
}

TEST(cardano_byron_address_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_byron_address_set_last_error(byron_address, message);

  // Assert
  EXPECT_STREQ(cardano_byron_address_get_last_error(byron_address), "");

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_to_address, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t*       address       = NULL;
  cardano_byron_address_t* byron_address = NULL;
  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  address = cardano_byron_address_to_address(byron_address);

  // Assert
  EXPECT_EQ(address, nullptr);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_get_attributes, returnsNullIfGivenANullPtr)
{
  // Arrange
  cardano_byron_address_t*           byron_address = nullptr;
  cardano_byron_address_attributes_t attributes    = { 0 };

  // Act
  cardano_error_t result = cardano_byron_address_get_attributes(byron_address, &attributes);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_byron_address_get_attributes, canGetTheAttributes)
{
  // Arrange
  cardano_byron_address_t*           byron_address = nullptr;
  cardano_byron_address_attributes_t attributes    = { 0 };

  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_byron_address_get_attributes(byron_address, &attributes);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(attributes.magic, -1);
  EXPECT_EQ(attributes.derivation_path_size, 0);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_get_type, returnsNullIfGivenANullPtr)
{
  // Arrange
  cardano_byron_address_t*     byron_address = nullptr;
  cardano_byron_address_type_t type;

  // Act
  cardano_error_t result = cardano_byron_address_get_type(byron_address, &type);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_byron_address_get_type, canGetTheType)
{
  // Arrange
  cardano_byron_address_t*     byron_address = nullptr;
  cardano_byron_address_type_t type;

  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_byron_address_get_type(byron_address, &type);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_BYRON_ADDRESS_TYPE_PUBKEY);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}

TEST(cardano_byron_address_get_root, returnsNullIfGivenANullPtr)
{
  // Arrange
  cardano_byron_address_t* byron_address = nullptr;
  cardano_blake2b_hash_t*  root          = nullptr;

  // Act
  cardano_error_t result = cardano_byron_address_get_root(byron_address, &root);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_byron_address_get_root, canGetTheRoot)
{
  // Arrange
  cardano_byron_address_t* byron_address = nullptr;
  cardano_blake2b_hash_t*  root          = nullptr;

  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_byron_address_get_root(byron_address, &root);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(root, nullptr);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
  cardano_blake2b_hash_unref(&root);
}

TEST(cardano_byron_address_get_root, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_byron_address_t* byron_address = nullptr;
  cardano_blake2b_hash_t*  root          = nullptr;

  EXPECT_EQ(cardano_byron_address_from_base58(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &byron_address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_byron_address_get_root(byron_address, &root);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(root, nullptr);

  // Cleanup
  cardano_byron_address_unref(&byron_address);
}
