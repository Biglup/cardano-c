/**
 * \file reward_address.cpp
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

#include <cardano/address/reward_address.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"
#include "cip19_test_vectors.h"

extern "C" {
#include "../../src/address/internals/addr_common.h"
}

#include <cardano/address/address.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_reward_address_from_credentials, returnsErrorWhenPaymentIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, NULL, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_credentials, returnsErrorWhenrewardAddressIsNull)
{
  // Arrange
  cardano_credential_t* payment = (cardano_credential_t*)"";

  // Act
  cardano_error_t result = cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_reward_address_from_credentials, canCreateARewardAddressFromCredential)
{
  // Arrange
  cardano_credential_t*     payment        = NULL;
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::stakeKeyHashHex.c_str(),
      Cip19TestVectors::stakeKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(reward_address, nullptr);
  EXPECT_EQ(cardano_reward_address_get_string(reward_address), Cip19TestVectors::rewardKey);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_from_credentials, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*     payment        = NULL;
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(reward_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_reward_address_unref(&reward_address);
  cardano_set_allocators(_cardano_malloc, _cardano_realloc, _cardano_free);
}

TEST(cardano_reward_address_from_credentials, returnErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*     payment        = NULL;
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(reward_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_reward_address_unref(&reward_address);
  cardano_set_allocators(_cardano_malloc, _cardano_realloc, _cardano_free);
}

TEST(cardano_reward_address_from_address, returnsErrorWhenAddressIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_address(NULL, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_address, returnsErrorWhenrewardAddressIsNull)
{
  // Arrange
  cardano_address_t*        address        = (cardano_address_t*)"";
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_address(address, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_address, returnsErrorWhenAddressTypeIsInvalid)
{
  // Arrange
  cardano_address_t*        address        = NULL;
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_reward_address_from_address(address, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_TYPE);
  EXPECT_EQ(reward_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_reward_address_from_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t*        address        = NULL;
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::rewardKeyBytes, sizeof(Cip19TestVectors::rewardKeyBytes), &address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_reward_address_from_address(address, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(reward_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
  cardano_set_allocators(_cardano_malloc, _cardano_realloc, _cardano_free);
}

TEST(cardano_reward_address_from_address, canConvertARewardAddressFromAValidAddress)
{
  // Arrange
  cardano_address_t*        address        = NULL;
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::rewardKeyBytes, sizeof(Cip19TestVectors::rewardKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_reward_address_from_address(address, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(reward_address, nullptr);
  EXPECT_EQ(cardano_reward_address_get_string(reward_address), Cip19TestVectors::rewardKey);

  // Cleanup
  cardano_address_unref(&address);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_to_address, returnsErrorWhenrewardAddressIsNull)
{
  // Act
  cardano_address_t* address = cardano_reward_address_to_address(NULL);

  // Assert
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_reward_address_to_address, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_address_t* address = cardano_reward_address_to_address(reward_address);

  // Assert
  EXPECT_EQ(address, nullptr);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_set_allocators(_cardano_malloc, _cardano_realloc, _cardano_free);
}

TEST(cardano_reward_address_get_payment_credential, returnsErrorWhenrewardAddressIsNull)
{
  // Act
  cardano_credential_t* payment = cardano_reward_address_get_payment_credential(NULL);

  // Assert
  EXPECT_EQ(payment, nullptr);
}

TEST(cardano_reward_address_get_payment_credential, canGetPaymentCredential)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  cardano_credential_t*     payment        = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &reward_address), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* payment_credential = cardano_reward_address_get_payment_credential(reward_address);

  // Assert
  EXPECT_NE(payment_credential, nullptr);
  EXPECT_EQ(cardano_credential_get_hash_hex(payment_credential), Cip19TestVectors::paymentKeyHashHex);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&payment_credential);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_to_address, canConvertrewardAddressToAddress)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  cardano_address_t* address = cardano_reward_address_to_address(reward_address);

  // Assert
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::rewardKey);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_address_unref(&address);
}

TEST(cardano_reward_address_from_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bytes(NULL, 0, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_bytes, returnsErrorWhenrewardAddressIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bytes(Cip19TestVectors::rewardKeyBytes, sizeof(Cip19TestVectors::rewardKeyBytes), NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_bytes, returnsErrorWhenInvalidSize)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bytes(Cip19TestVectors::rewardKeyBytes, 0, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_bytes, canCreatereward_addressFromReward_addressBytes)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bytes(Cip19TestVectors::rewardKeyBytes, sizeof(Cip19TestVectors::rewardKeyBytes), &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(reward_address, nullptr);
  EXPECT_EQ(cardano_reward_address_get_string(reward_address), Cip19TestVectors::rewardKey);

  // compare bytes
  const byte_t* bytes = cardano_reward_address_get_bytes(reward_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::rewardKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::rewardKeyBytes[i]);
  }

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_from_bech32, canCreatereward_addressFromString)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(reward_address, nullptr);
  EXPECT_EQ(cardano_reward_address_get_string(reward_address), Cip19TestVectors::rewardKey);

  // compare bytes
  const byte_t* bytes = cardano_reward_address_get_bytes(reward_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::rewardKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::rewardKeyBytes[i]);
  }

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_from_bech32, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(reward_address, nullptr);
  cardano_set_allocators(_cardano_malloc, _cardano_realloc, _cardano_free);
}

TEST(cardano_reward_address_from_bech32, returnsErrorIfSizeIsZero)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), 0, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_bech32, returnsErrorIfAddressIsNull)
{
  // Act
  cardano_error_t result = cardano_reward_address_from_bech32("a", 1, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_reward_address_from_bech32, returnErrorIfInvalidPrefix)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bech32("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r", strlen("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r"), &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_get_bytes_size, canGetreward_addressBytesSize)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_reward_address_get_bytes_size(reward_address);

  // Assert
  EXPECT_EQ(size, sizeof(Cip19TestVectors::rewardKeyBytes));

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_get_bytes, canGetreward_addressBytes)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_reward_address_get_bytes(reward_address);

  // Assert
  for (size_t i = 0; i < sizeof(Cip19TestVectors::rewardKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::rewardKeyBytes[i]);
  }

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_to_bytes, canConvertreward_addressToBytes)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_reward_address_to_bytes(reward_address, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::rewardKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::rewardKeyBytes[i]);
  }

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_to_bytes, returnsErrorWhenreward_addressIsNull)
{
  // Arrange
  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_reward_address_to_bytes(NULL, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_reward_address_to_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_reward_address_to_bytes(reward_address, NULL, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_to_bytes, returnsErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_reward_address_to_bytes(reward_address, bytes, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_from_bech32, returnsErrorWhenreward_addressIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bech32(NULL, 0, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_from_bech32, returnsErrorWhenreward_addressIsInvalid)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;

  // Act
  cardano_error_t result = cardano_reward_address_from_bech32("invalid_reward_address", 15, &reward_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_reward_address_get_string_size, canGetreward_addressStringSize)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_reward_address_get_bech32_size(reward_address);

  // Assert
  EXPECT_EQ(size, Cip19TestVectors::rewardKey.size() + 1 /* null terminator */);

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_to_bech32, canConvertreward_addressToString)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_reward_address_to_bech32(reward_address, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(std::string(buffer), Cip19TestVectors::rewardKey);

  // Clean up
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  cardano_reward_address_ref(reward_address);

  // Assert
  EXPECT_THAT(reward_address, testing::Not((cardano_reward_address_t*)nullptr));
  EXPECT_EQ(cardano_reward_address_refcount(reward_address), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_reward_address_ref(nullptr);
}

TEST(cardano_reward_address_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_reward_address_unref((cardano_reward_address_t**)nullptr);
}

TEST(cardano_reward_address_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  cardano_reward_address_ref(reward_address);
  size_t ref_count = cardano_reward_address_refcount(reward_address);

  cardano_reward_address_unref(&reward_address);
  size_t updated_ref_count = cardano_reward_address_refcount(reward_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  cardano_reward_address_ref(reward_address);
  size_t ref_count = cardano_reward_address_refcount(reward_address);

  cardano_reward_address_unref(&reward_address);
  size_t updated_ref_count = cardano_reward_address_refcount(reward_address);

  cardano_reward_address_unref(&reward_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(reward_address, (cardano_reward_address_t*)nullptr);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  // Act
  EXPECT_THAT(cardano_reward_address_move(reward_address), testing::Not((cardano_reward_address_t*)nullptr));
  size_t ref_count = cardano_reward_address_refcount(reward_address);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(reward_address, testing::Not((cardano_reward_address_t*)nullptr));

  // Cleanup
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_reward_address_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_reward_address_move, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_reward_address_t* reward_address = cardano_reward_address_move(nullptr);

  // Assert
  EXPECT_EQ(reward_address, (cardano_reward_address_t*)nullptr);
}

TEST(cardano_reward_address_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  const char* message = "This is a test message";

  // Act
  cardano_reward_address_set_last_error(reward_address, message);
  const char* last_error = cardano_reward_address_get_last_error(reward_address);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_reward_address_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  const char* last_error = cardano_reward_address_get_last_error(reward_address);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_reward_address_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;
  const char*               message        = "This is a test message";

  // Act
  cardano_reward_address_set_last_error(reward_address, message);

  // Assert
  EXPECT_STREQ(cardano_reward_address_get_last_error(reward_address), "Object is NULL.");
}

TEST(cardano_reward_address_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &reward_address), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_reward_address_set_last_error(reward_address, message);

  // Assert
  EXPECT_STREQ(cardano_reward_address_get_last_error(reward_address), "");

  // Cleanup
  cardano_reward_address_unref(&reward_address);
}
