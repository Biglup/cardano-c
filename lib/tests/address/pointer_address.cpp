/**
 * \file pointer_address.cpp
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

#include <cardano/address/pointer_address.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"
#include "cip19_test_vectors.h"

extern "C" {
#include "../../src/address/internals/addr_common.h"
}

#include <cardano/address/address.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_pointer_address_from_credentials, returnsErrorWhenPaymentIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, NULL, Cip19TestVectors::stakePointer, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_credentials, returnsErrorWhenPointerAddressIsNull)
{
  // Arrange
  cardano_credential_t* payment = (cardano_credential_t*)"";

  // Act
  cardano_error_t result = cardano_pointer_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, Cip19TestVectors::stakePointer, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pointer_address_from_credentials, canCreateAPointerAddressFromCredential)
{
  // Arrange
  cardano_credential_t*      payment         = NULL;
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_pointer_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, Cip19TestVectors::stakePointer, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(pointer_address, nullptr);
  EXPECT_EQ(cardano_pointer_address_get_string(pointer_address), Cip19TestVectors::pointerKey);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_from_credentials, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*      payment         = NULL;
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_pointer_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, Cip19TestVectors::stakePointer, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pointer_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_from_credentials, returnErrorIfMemoryAllocationEventuallyFails)
{
  // Arrange
  cardano_credential_t*      payment         = NULL;
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_pointer_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, Cip19TestVectors::stakePointer, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pointer_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_from_credentials, returnErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*      payment         = NULL;
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_pointer_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, Cip19TestVectors::stakePointer, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pointer_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_from_address, returnsErrorWhenAddressIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_address(NULL, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_address, returnsErrorWhenPointerAddressIsNull)
{
  // Arrange
  cardano_address_t*         address         = (cardano_address_t*)"";
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_address(address, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_address, returnsErrorWhenAddressTypeIsInvalid)
{
  // Arrange
  cardano_address_t*         address         = NULL;
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::rewardKeyBytes, sizeof(Cip19TestVectors::rewardKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_pointer_address_from_address(address, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_TYPE);
  EXPECT_EQ(pointer_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_pointer_address_from_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t*         address         = NULL;
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_pointer_address_from_address(address, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(pointer_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_pointer_address_from_address, canConvertAPointerAddressFromAValidAddress)
{
  // Arrange
  cardano_address_t*         address         = NULL;
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_pointer_address_from_address(address, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(pointer_address, nullptr);
  EXPECT_EQ(cardano_pointer_address_get_string(pointer_address), Cip19TestVectors::pointerKey);

  // Cleanup
  cardano_address_unref(&address);
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_to_address, returnsErrorWhenPointerAddressIsNull)
{
  // Act
  cardano_address_t* address = cardano_pointer_address_to_address(NULL);

  // Assert
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_pointer_address_to_address, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_address_t* address = cardano_pointer_address_to_address(pointer_address);

  // Assert
  EXPECT_EQ(address, nullptr);

  // Cleanup
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_get_payment_credential, returnsErrorWhenPointerAddressIsNull)
{
  // Act
  cardano_credential_t* payment = cardano_pointer_address_get_payment_credential(NULL);

  // Assert
  EXPECT_EQ(payment, nullptr);
}

TEST(cardano_pointer_address_get_payment_credential, canGetPaymentCredential)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  cardano_credential_t*      payment         = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(cardano_pointer_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, Cip19TestVectors::stakePointer, &pointer_address), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* payment_credential = cardano_pointer_address_get_payment_credential(pointer_address);

  // Assert
  EXPECT_NE(payment_credential, nullptr);
  EXPECT_EQ(cardano_credential_get_hash_hex(payment_credential), Cip19TestVectors::paymentKeyHashHex);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&payment_credential);
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_get_stake_pointer, returnsAZeroInitializedStakePointerIfGivenANullPointer)
{
  // Act
  cardano_stake_pointer_t stake_pointer = { 0 };

  cardano_error_t result = cardano_pointer_address_get_stake_pointer(NULL, &stake_pointer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(stake_pointer.tx_index, 0);
  EXPECT_EQ(stake_pointer.cert_index, 0);
  EXPECT_EQ(stake_pointer.slot, 0);
}

TEST(cardano_pointer_address_get_stake_pointer, canGetStakePointer)
{
  // Arrange
  cardano_stake_pointer_t    stake_pointer   = { 0 };
  cardano_pointer_address_t* pointer_address = NULL;

  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_pointer_address_get_stake_pointer(pointer_address, &stake_pointer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(stake_pointer.tx_index, Cip19TestVectors::stakePointer.tx_index);
  EXPECT_EQ(stake_pointer.cert_index, Cip19TestVectors::stakePointer.cert_index);
  EXPECT_EQ(stake_pointer.slot, Cip19TestVectors::stakePointer.slot);

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_to_address, canConvertpointerAddressToAddress)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  cardano_address_t* address = cardano_pointer_address_to_address(pointer_address);

  // Assert
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::pointerKey);

  // Cleanup
  cardano_pointer_address_unref(&pointer_address);
  cardano_address_unref(&address);
}

TEST(cardano_pointer_address_from_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bytes(NULL, 0, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_bytes, returnsErrorWhenPointerAddressIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_bytes, returnsErrorWhenInvalidSize)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bytes(Cip19TestVectors::pointerKeyBytes, 0, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_bytes, canCreatepointer_addressFromPointer_addressBytes)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(pointer_address, nullptr);
  EXPECT_EQ(cardano_pointer_address_get_string(pointer_address), Cip19TestVectors::pointerKey);

  // compare bytes
  const byte_t* bytes = cardano_pointer_address_get_bytes(pointer_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::pointerKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::pointerKeyBytes[i]);
  }

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_from_bech32, canCreatepointer_addressFromString)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(pointer_address, nullptr);
  EXPECT_EQ(cardano_pointer_address_get_string(pointer_address), Cip19TestVectors::pointerKey);

  // compare bytes
  const byte_t* bytes = cardano_pointer_address_get_bytes(pointer_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::pointerKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::pointerKeyBytes[i]);
  }

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_from_bech32, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_bech32, returnsErrorIfSizeIsZero)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), 0, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_bech32, returnsErrorIfAddressIsNull)
{
  // Act
  cardano_error_t result = cardano_pointer_address_from_bech32("a", 1, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pointer_address_from_bech32, returnErrorIfInvalidPrefix)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bech32("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r", strlen("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r"), &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_get_bytes_size, canGetpointer_addressBytesSize)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_pointer_address_get_bytes_size(pointer_address);

  // Assert
  EXPECT_EQ(size, sizeof(Cip19TestVectors::pointerKeyBytes));

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_get_bytes, canGetpointer_addressBytes)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_pointer_address_get_bytes(pointer_address);

  // Assert
  for (size_t i = 0; i < sizeof(Cip19TestVectors::pointerKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::pointerKeyBytes[i]);
  }

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_to_bytes, canConvertpointer_addressToBytes)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_pointer_address_to_bytes(pointer_address, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::pointerKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::pointerKeyBytes[i]);
  }

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_to_bytes, returnsErrorWhenPointer_addressIsNull)
{
  // Arrange
  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_pointer_address_to_bytes(NULL, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_pointer_address_to_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_pointer_address_to_bytes(pointer_address, NULL, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_to_bytes, returnsErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_pointer_address_to_bytes(pointer_address, bytes, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_from_bech32, returnsErrorWhenPointer_addressIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bech32(NULL, 0, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_from_bech32, returnsErrorWhenPointerAddressIsInvalid)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;

  // Act
  cardano_error_t result = cardano_pointer_address_from_bech32("invalid_pointer_address", 15, &pointer_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_pointer_address_get_string_size, canGetpointer_addressStringSize)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_pointer_address_get_bech32_size(pointer_address);

  // Assert
  EXPECT_EQ(size, Cip19TestVectors::pointerKey.size() + 1 /* null terminator */);

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_to_bech32, canConvertpointer_addressToString)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_pointer_address_to_bech32(pointer_address, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(std::string(buffer), Cip19TestVectors::pointerKey);

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  cardano_pointer_address_ref(pointer_address);

  // Assert
  EXPECT_THAT(pointer_address, testing::Not((cardano_pointer_address_t*)nullptr));
  EXPECT_EQ(cardano_pointer_address_refcount(pointer_address), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_pointer_address_unref(&pointer_address);
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pointer_address_ref(nullptr);
}

TEST(cardano_pointer_address_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = nullptr;

  // Act
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pointer_address_unref((cardano_pointer_address_t**)nullptr);
}

TEST(cardano_pointer_address_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  cardano_pointer_address_ref(pointer_address);
  size_t ref_count = cardano_pointer_address_refcount(pointer_address);

  cardano_pointer_address_unref(&pointer_address);
  size_t updated_ref_count = cardano_pointer_address_refcount(pointer_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  // Act
  cardano_pointer_address_ref(pointer_address);
  size_t ref_count = cardano_pointer_address_refcount(pointer_address);

  cardano_pointer_address_unref(&pointer_address);
  size_t updated_ref_count = cardano_pointer_address_refcount(pointer_address);

  cardano_pointer_address_unref(&pointer_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(pointer_address, (cardano_pointer_address_t*)nullptr);

  // Cleanup
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_pointer_address_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_pointer_address_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  const char* message = "This is a test message";

  // Act
  cardano_pointer_address_set_last_error(pointer_address, message);
  const char* last_error = cardano_pointer_address_get_last_error(pointer_address);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = nullptr;

  // Act
  const char* last_error = cardano_pointer_address_get_last_error(pointer_address);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_pointer_address_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = nullptr;
  const char*                message         = "This is a test message";

  // Act
  cardano_pointer_address_set_last_error(pointer_address, message);

  // Assert
  EXPECT_STREQ(cardano_pointer_address_get_last_error(pointer_address), "Object is NULL.");
}

TEST(cardano_pointer_address_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_pointer_address_t* pointer_address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &pointer_address), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_pointer_address_set_last_error(pointer_address, message);

  // Assert
  EXPECT_STREQ(cardano_pointer_address_get_last_error(pointer_address), "");

  // Cleanup
  cardano_pointer_address_unref(&pointer_address);
}

TEST(cardano_pointer_address_get_network_id, canGetNetworkId)
{
  // Arrange
  cardano_pointer_address_t* address = NULL;
  EXPECT_EQ(cardano_pointer_address_from_bech32(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_network_id_t network_id;

  EXPECT_EQ(cardano_pointer_address_get_network_id(address, &network_id), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Clean up
  cardano_pointer_address_unref(&address);
}