/**
 * \file enterprise_address.cpp
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

#include <cardano/address/enterprise_address.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"
#include "cip19_test_vectors.h"

extern "C" {
#include "../../src/address/internals/addr_common.h"
}

#include <cardano/address/address.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_enterprise_address_from_credentials, returnsErrorWhenPaymentIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, NULL, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_credentials, returnsErrorWhenenterpriseAddressIsNull)
{
  // Arrange
  cardano_credential_t* payment = (cardano_credential_t*)"";

  // Act
  cardano_error_t result = cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_enterprise_address_from_credentials, canCreateAEnterpriseAddressFromCredential)
{
  // Arrange
  cardano_credential_t*         payment            = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(enterprise_address, nullptr);
  EXPECT_EQ(cardano_enterprise_address_get_string(enterprise_address), Cip19TestVectors::enterpriseKey);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_from_credentials, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*         payment            = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

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
  cardano_error_t result = cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(enterprise_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_from_credentials, returnErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t*         payment            = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

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
  cardano_error_t result = cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(enterprise_address, nullptr);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_from_address, returnsErrorWhenAddressIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_address(NULL, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_address, returnsErrorWhenenterpriseAddressIsNull)
{
  // Arrange
  cardano_address_t*            address            = (cardano_address_t*)"";
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_address(address, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_address, returnsErrorWhenAddressTypeIsInvalid)
{
  // Arrange
  cardano_address_t*            address            = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_enterprise_address_from_address(address, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_TYPE);
  EXPECT_EQ(enterprise_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_enterprise_address_from_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t*            address            = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::enterpriseKeyBytes, sizeof(Cip19TestVectors::enterpriseKeyBytes), &address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_enterprise_address_from_address(address, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(enterprise_address, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_enterprise_address_from_address, canConvertAEnterpriseAddressFromAValidAddress)
{
  // Arrange
  cardano_address_t*            address            = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(cardano_address_from_bytes(Cip19TestVectors::enterpriseKeyBytes, sizeof(Cip19TestVectors::enterpriseKeyBytes), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_enterprise_address_from_address(address, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(enterprise_address, nullptr);
  EXPECT_EQ(cardano_enterprise_address_get_string(enterprise_address), Cip19TestVectors::enterpriseKey);

  // Cleanup
  cardano_address_unref(&address);
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_to_address, returnsErrorWhenenterpriseAddressIsNull)
{
  // Act
  cardano_address_t* address = cardano_enterprise_address_to_address(NULL);

  // Assert
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_enterprise_address_to_address, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise_address);

  // Assert
  EXPECT_EQ(address, nullptr);

  // Cleanup
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_get_payment_credential, returnsErrorWhenenterpriseAddressIsNull)
{
  // Act
  cardano_credential_t* payment = cardano_enterprise_address_get_payment_credential(NULL);

  // Assert
  EXPECT_EQ(payment, nullptr);
}

TEST(cardano_enterprise_address_get_payment_credential, canGetPaymentCredential)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  cardano_credential_t*         payment            = NULL;

  EXPECT_EQ(
    cardano_credential_from_hash_hex(
      Cip19TestVectors::paymentKeyHashHex.c_str(),
      Cip19TestVectors::paymentKeyHashHex.size(),
      CARDANO_CREDENTIAL_TYPE_KEY_HASH,
      &payment),
    CARDANO_SUCCESS);

  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_MAIN_NET, payment, &enterprise_address), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* payment_credential = cardano_enterprise_address_get_payment_credential(enterprise_address);

  // Assert
  EXPECT_NE(payment_credential, nullptr);
  EXPECT_EQ(cardano_credential_get_hash_hex(payment_credential), Cip19TestVectors::paymentKeyHashHex);

  // Clean up
  cardano_credential_unref(&payment);
  cardano_credential_unref(&payment_credential);
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_to_address, canConvertenterpriseAddressToAddress)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise_address);

  // Assert
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::enterpriseKey);

  // Cleanup
  cardano_enterprise_address_unref(&enterprise_address);
  cardano_address_unref(&address);
}

TEST(cardano_enterprise_address_from_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bytes(NULL, 0, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_bytes, returnsErrorWhenenterpriseAddressIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bytes(Cip19TestVectors::enterpriseKeyBytes, sizeof(Cip19TestVectors::enterpriseKeyBytes), NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_bytes, returnsErrorWhenInvalidSize)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bytes(Cip19TestVectors::enterpriseKeyBytes, 0, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_bytes, canCreateenterprise_addressFromEnterprise_addressBytes)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bytes(Cip19TestVectors::enterpriseKeyBytes, sizeof(Cip19TestVectors::enterpriseKeyBytes), &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(enterprise_address, nullptr);
  EXPECT_EQ(cardano_enterprise_address_get_string(enterprise_address), Cip19TestVectors::enterpriseKey);

  // compare bytes
  const byte_t* bytes = cardano_enterprise_address_get_bytes(enterprise_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::enterpriseKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::enterpriseKeyBytes[i]);
  }

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_from_bech32, canCreateenterprise_addressFromString)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(enterprise_address, nullptr);
  EXPECT_EQ(cardano_enterprise_address_get_string(enterprise_address), Cip19TestVectors::enterpriseKey);

  // compare bytes
  const byte_t* bytes = cardano_enterprise_address_get_bytes(enterprise_address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::enterpriseKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::enterpriseKeyBytes[i]);
  }

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_from_bech32, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_bech32, returnsErrorIfSizeIsZero)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), 0, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_bech32, returnsErrorIfAddressIsNull)
{
  // Act
  cardano_error_t result = cardano_enterprise_address_from_bech32("a", 1, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_enterprise_address_from_bech32, returnErrorIfInvalidPrefix)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bech32("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r", strlen("split1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfjcf7r"), &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_get_bytes_size, canGetenterprise_addressBytesSize)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_enterprise_address_get_bytes_size(enterprise_address);

  // Assert
  EXPECT_EQ(size, sizeof(Cip19TestVectors::enterpriseKeyBytes));

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_get_bytes, canGetenterprise_addressBytes)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_enterprise_address_get_bytes(enterprise_address);

  // Assert
  for (size_t i = 0; i < sizeof(Cip19TestVectors::enterpriseKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::enterpriseKeyBytes[i]);
  }

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_to_bytes, canConvertenterprise_addressToBytes)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_enterprise_address_to_bytes(enterprise_address, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::enterpriseKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::enterpriseKeyBytes[i]);
  }

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_to_bytes, returnsErrorWhenenterprise_addressIsNull)
{
  // Arrange
  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_enterprise_address_to_bytes(NULL, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_enterprise_address_to_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_enterprise_address_to_bytes(enterprise_address, NULL, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_to_bytes, returnsErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_enterprise_address_to_bytes(enterprise_address, bytes, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_from_bech32, returnsErrorWhenenterprise_addressIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bech32(NULL, 0, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_from_bech32, returnsErrorWhenenterprise_addressIsInvalid)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;

  // Act
  cardano_error_t result = cardano_enterprise_address_from_bech32("invalid_enterprise_address", 15, &enterprise_address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_enterprise_address_get_string_size, canGetenterprise_addressStringSize)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_enterprise_address_get_bech32_size(enterprise_address);

  // Assert
  EXPECT_EQ(size, Cip19TestVectors::enterpriseKey.size() + 1 /* null terminator */);

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_to_bech32, canConvertenterprise_addressToString)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_enterprise_address_to_bech32(enterprise_address, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(std::string(buffer), Cip19TestVectors::enterpriseKey);

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  cardano_enterprise_address_ref(enterprise_address);

  // Assert
  EXPECT_THAT(enterprise_address, testing::Not((cardano_enterprise_address_t*)nullptr));
  EXPECT_EQ(cardano_enterprise_address_refcount(enterprise_address), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_enterprise_address_unref(&enterprise_address);
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_enterprise_address_ref(nullptr);
}

TEST(cardano_enterprise_address_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = nullptr;

  // Act
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_enterprise_address_unref((cardano_enterprise_address_t**)nullptr);
}

TEST(cardano_enterprise_address_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  cardano_enterprise_address_ref(enterprise_address);
  size_t ref_count = cardano_enterprise_address_refcount(enterprise_address);

  cardano_enterprise_address_unref(&enterprise_address);
  size_t updated_ref_count = cardano_enterprise_address_refcount(enterprise_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  cardano_enterprise_address_ref(enterprise_address);
  size_t ref_count = cardano_enterprise_address_refcount(enterprise_address);

  cardano_enterprise_address_unref(&enterprise_address);
  size_t updated_ref_count = cardano_enterprise_address_refcount(enterprise_address);

  cardano_enterprise_address_unref(&enterprise_address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(enterprise_address, (cardano_enterprise_address_t*)nullptr);

  // Cleanup
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  // Act
  EXPECT_THAT(cardano_enterprise_address_move(enterprise_address), testing::Not((cardano_enterprise_address_t*)nullptr));
  size_t ref_count = cardano_enterprise_address_refcount(enterprise_address);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(enterprise_address, testing::Not((cardano_enterprise_address_t*)nullptr));

  // Cleanup
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_enterprise_address_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_enterprise_address_move, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_enterprise_address_t* enterprise_address = cardano_enterprise_address_move(nullptr);

  // Assert
  EXPECT_EQ(enterprise_address, (cardano_enterprise_address_t*)nullptr);
}

TEST(cardano_enterprise_address_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  const char* message = "This is a test message";

  // Act
  cardano_enterprise_address_set_last_error(enterprise_address, message);
  const char* last_error = cardano_enterprise_address_get_last_error(enterprise_address);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_enterprise_address_unref(&enterprise_address);
}

TEST(cardano_enterprise_address_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = nullptr;

  // Act
  const char* last_error = cardano_enterprise_address_get_last_error(enterprise_address);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_enterprise_address_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_enterprise_address_set_last_error(enterprise_address, message);

  // Assert
  EXPECT_STREQ(cardano_enterprise_address_get_last_error(enterprise_address), "Object is NULL.");
}

TEST(cardano_enterprise_address_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_enterprise_address_t* enterprise_address = NULL;
  EXPECT_EQ(cardano_enterprise_address_from_bech32(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &enterprise_address), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_enterprise_address_set_last_error(enterprise_address, message);

  // Assert
  EXPECT_STREQ(cardano_enterprise_address_get_last_error(enterprise_address), "");

  // Cleanup
  cardano_enterprise_address_unref(&enterprise_address);
}
