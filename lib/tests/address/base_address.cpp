/**
 * \file base_address.cpp
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

#include <cardano/address/base_address.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"
#include "cip19_test_vectors.h"

extern "C" {
#include "../../src/address/internals/addr_common.h"
}

#include <cardano/address/address.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_base_address_from_credentials, returnsErrorWhenPaymentIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  cardano_credential_t*   stake        = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, NULL, stake, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_credentials, returnsErrorWhenStakeIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  cardano_credential_t*   payment      = (cardano_credential_t*)"";

  // Act
  cardano_error_t result = cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, NULL, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_credentials, returnsErrorWhenBaseAddressIsNull)
{
  // Arrange
  cardano_credential_t* payment = (cardano_credential_t*)"";
  cardano_credential_t* stake   = (cardano_credential_t*)"";

  // Act
  cardano_error_t result = cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, stake, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_base_address_from_credentials, canCreateABaseAddressFromCredential)
{
  // Arrange
  cardano_credential_t*   payment      = NULL;
  cardano_credential_t*   stake        = (cardano_credential_t*)"";
  cardano_base_address_t* base_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::stakeKeyHashHex.c_str(),
      Cip19TestVectors::stakeKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &stake),
    CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, stake, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::basePaymentKeyStakeKey);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&stake);
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_credentials, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*   payment      = NULL;
  cardano_credential_t*   stake        = (cardano_credential_t*)"";
  cardano_base_address_t* base_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::stakeKeyHashHex.c_str(),
      Cip19TestVectors::stakeKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &stake),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, stake, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(base_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&stake);
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_credentials, returnErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*   payment      = NULL;
  cardano_credential_t*   stake        = (cardano_credential_t*)"";
  cardano_base_address_t* base_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::stakeKeyHashHex.c_str(),
      Cip19TestVectors::stakeKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &stake),
    CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, stake, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(base_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&stake);
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_address, returnsErrorWhenAddressIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_address(NULL, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_address, returnsErrorWhenBaseAddressIsNull)
{
  // Arrange
  cardano_address_t*      address      = (cardano_address_t*)"";
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_address(address, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_address, returnsErrorWhenAddressTypeIsInvalid)
{
  // Arrange
  cardano_address_t*      address      = NULL;
  cardano_base_address_t* base_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_base_address_from_address(address, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_TYPE);
  EXPECT_EQ(base_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_base_address_from_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t*      address      = NULL;
  cardano_base_address_t* base_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes), &address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_base_address_from_address(address, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(base_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_base_address_from_address, canConvertABaseAddressFromAValidAddress)
{
  // Arrange
  cardano_address_t*      address      = NULL;
  cardano_base_address_t* base_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_base_address_from_address(address, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::basePaymentKeyStakeKey);

  // Cleanup
  cardano_address_unref(&address);
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_to_address, returnsErrorWhenBaseAddressIsNull)
{
  // Act
  cardano_address_t* address = cardano_base_address_to_address(NULL);

  // Assert
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_base_address_to_address, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_address_t* address = cardano_base_address_to_address(base_address);

  // Assert
  EXPECT_EQ(address, nullptr);

  // Cleanup
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_get_payment_credential, returnsErrorWhenBaseAddressIsNull)
{
  // Act
  cardano_credential_t* payment = cardano_base_address_get_payment_credential(NULL);

  // Assert
  EXPECT_EQ(payment, nullptr);
}

TEST(cardano_base_address_get_payment_credential, canGetPaymentCredential)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  cardano_credential_t*   payment      = NULL;
  cardano_credential_t*   stake        = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::stakeKeyHashHex.c_str(),
      Cip19TestVectors::stakeKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &stake),
    CARDANO_SUCCESS);

  EXPECT_EQ(cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, stake, &base_address), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* payment_credential = cardano_base_address_get_payment_credential(base_address);

  // Assert
  EXPECT_NE(payment_credential, nullptr);
  EXPECT_EQ(cardano_credential_get_hash_hex(payment_credential), Cip19TestVectors::paymentKeyHashHex);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&stake);
  cardano_credential_unref(&payment_credential);
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_get_stake_credential, returnsErrorWhenBaseAddressIsNull)
{
  // Act
  cardano_credential_t* stake = cardano_base_address_get_stake_credential(NULL);

  // Assert
  EXPECT_EQ(stake, nullptr);
}

TEST(cardano_base_address_get_stake_credential, canGetStakeCredential)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  cardano_credential_t*   payment      = NULL;
  cardano_credential_t*   stake        = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::stakeKeyHashHex.c_str(),
      Cip19TestVectors::stakeKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &stake),
    CARDANO_SUCCESS);

  EXPECT_EQ(cardano_base_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, stake, &base_address), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* stake_credential = cardano_base_address_get_stake_credential(base_address);

  // Assert
  EXPECT_NE(stake_credential, nullptr);
  EXPECT_EQ(cardano_credential_get_hash_hex(stake_credential), Cip19TestVectors::stakeKeyHashHex);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&stake);
  cardano_credential_unref(&stake_credential);
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_to_address, canConvertBaseAddressToAddress)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  cardano_address_t* address = cardano_base_address_to_address(base_address);

  // Assert
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::basePaymentKeyStakeKey);

  // Cleanup
  cardano_base_address_unref(&base_address);
  cardano_address_unref(&address);
}

TEST(cardano_base_address_from_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bytes(NULL, 0, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_bytes, returnsErrorWhenBaseAddressIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes), NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_bytes, returnsErrorWhenInvalidSize)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, 0, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_bytes, canCreatebase_addressFromBasebase_addressBytes)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::basePaymentKeyStakeKey);

  // compare bytes
  const byte_t* bytes = cardano_base_address_get_bytes(base_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromString)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::basePaymentKeyStakeKey);

  // compare bytes
  const byte_t* bytes = cardano_base_address_get_bytes(base_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromBaseBasePaymentScriptStakeKey)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::basePaymentScriptStakeKey.c_str(), Cip19TestVectors::basePaymentScriptStakeKey.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::basePaymentScriptStakeKey);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromBasePaymentKeyStakeScript)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeScript.c_str(), Cip19TestVectors::basePaymentKeyStakeScript.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::basePaymentKeyStakeScript);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromBasePaymentScriptStakeScript)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::basePaymentScriptStakeScript.c_str(), Cip19TestVectors::basePaymentScriptStakeScript.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::basePaymentScriptStakeScript);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromTestnetBasePaymentKeyStakeKey)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::testnetBasePaymentKeyStakeKey.c_str(), Cip19TestVectors::testnetBasePaymentKeyStakeKey.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::testnetBasePaymentKeyStakeKey);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromTestnetBasePaymentScriptStakeKey)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::testnetBasePaymentScriptStakeKey.c_str(), Cip19TestVectors::testnetBasePaymentScriptStakeKey.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::testnetBasePaymentScriptStakeKey);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromTestnetBasePaymentKeyStakeScript)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::testnetBasePaymentKeyStakeScript.c_str(), Cip19TestVectors::testnetBasePaymentKeyStakeScript.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::testnetBasePaymentKeyStakeScript);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, canCreatebase_addressFromTestnetBasePaymentScriptStakeScript)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::testnetBasePaymentScriptStakeScript.c_str(), Cip19TestVectors::testnetBasePaymentScriptStakeScript.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(cardano_base_address_get_string(base_address), Cip19TestVectors::testnetBasePaymentScriptStakeScript);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_bech32, returnsErrorIfSizeIsZero)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), 0, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_bech32, returnsErrorIfAddressIsNull)
{
  // Act
  cardano_error_t result = cardano_base_address_from_bech32("a", 1, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_base_address_from_bech32, returnErrorIfInvalidPrefix)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r", strlen("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r"), &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_get_bytes_size, canGetbase_addressBytesSize)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_base_address_get_bytes_size(base_address);

  // Assert
  EXPECT_EQ(size, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes));

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_get_bytes, canGetbase_addressBytes)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_base_address_get_bytes(base_address);

  // Assert
  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_to_bytes, canConvertbase_addressToBytes)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_base_address_to_bytes(base_address, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_to_bytes, returnsErrorWhenbase_addressIsNull)
{
  // Arrange
  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_base_address_to_bytes(NULL, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_base_address_to_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_base_address_to_bytes(base_address, NULL, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_to_bytes, returnsErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_base_address_to_bytes(base_address, bytes, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_from_bech32, returnsErrorWhenbase_addressIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32(NULL, 0, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_from_bech32, returnsErrorWhenbase_addressIsInvalid)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;

  // Act
  cardano_error_t result = cardano_base_address_from_bech32("invalid_base_address", 15, &base_address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_base_address_get_string_size, canGetbase_addressStringSize)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_base_address_get_bech32_size(base_address);

  // Assert
  EXPECT_EQ(size, Cip19TestVectors::basePaymentKeyStakeKey.size() + 1 /* null terminator */);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_to_bech32, canConvertbase_addressToString)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_base_address_to_bech32(base_address, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(std::string(buffer), Cip19TestVectors::basePaymentKeyStakeKey);

  // Clean up
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  cardano_base_address_ref(base_address);

  // Assert
  EXPECT_THAT(base_address, testing::Not((cardano_base_address_t*)nullptr));
  EXPECT_EQ(cardano_base_address_refcount(base_address), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_base_address_unref(&base_address);
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_base_address_ref(nullptr);
}

TEST(cardano_base_address_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_base_address_t* base_address = nullptr;

  // Act
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_base_address_unref((cardano_base_address_t**)nullptr);
}

TEST(cardano_base_address_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  cardano_base_address_ref(base_address);
  size_t ref_count = cardano_base_address_refcount(base_address);

  cardano_base_address_unref(&base_address);
  size_t updated_ref_count = cardano_base_address_refcount(base_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  // Act
  cardano_base_address_ref(base_address);
  size_t ref_count = cardano_base_address_refcount(base_address);

  cardano_base_address_unref(&base_address);
  size_t updated_ref_count = cardano_base_address_refcount(base_address);

  cardano_base_address_unref(&base_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(base_address, (cardano_base_address_t*)nullptr);

  // Cleanup
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_base_address_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_base_address_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  const char* message = "This is a test message";

  // Act
  cardano_base_address_set_last_error(base_address, message);
  const char* last_error = cardano_base_address_get_last_error(base_address);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_base_address_t* base_address = nullptr;

  // Act
  const char* last_error = cardano_base_address_get_last_error(base_address);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_base_address_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = nullptr;
  const char*             message      = "This is a test message";

  // Act
  cardano_base_address_set_last_error(base_address, message);

  // Assert
  EXPECT_STREQ(cardano_base_address_get_last_error(base_address), "Object is NULL.");
}

TEST(cardano_base_address_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_base_address_t* base_address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &base_address), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_base_address_set_last_error(base_address, message);

  // Assert
  EXPECT_STREQ(cardano_base_address_get_last_error(base_address), "");

  // Cleanup
  cardano_base_address_unref(&base_address);
}

TEST(cardano_base_address_get_network_id, canGetNetworkId)
{
  // Arrange
  cardano_base_address_t* address = NULL;
  EXPECT_EQ(cardano_base_address_from_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_network_id_t network_id;

  EXPECT_EQ(cardano_base_address_get_network_id(address, &network_id), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Clean up
  cardano_base_address_unref(&address);
}