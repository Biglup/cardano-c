/**
 * \file address.cpp
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

#include <cardano/address/address.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"
#include "cip19_test_vectors.h"

extern "C" {
#include "../../src/address/internals/addr_common.h"
}

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_address_from_bytes, canCreateAddressFromBaseAddressBytes)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::basePaymentKeyStakeKey);

  // compare bytes
  const byte_t* bytes = cardano_address_get_bytes(address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_bytes, canCreateAddressFromByronYoroiAddressBytes)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::byronMainnetYoroiBytes, sizeof(Cip19TestVectors::byronMainnetYoroiBytes), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::byronMainnetYoroi);

  // compare bytes
  const byte_t* bytes = cardano_address_get_bytes(address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::byronMainnetYoroiBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::byronMainnetYoroiBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_bytes, canCreateAddressFromByronDedalusAddressBytes)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::byronTestnetDaedalusBytes, sizeof(Cip19TestVectors::byronTestnetDaedalusBytes), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::byronTestnetDaedalus);

  // compare bytes
  const byte_t* bytes = cardano_address_get_bytes(address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::byronTestnetDaedalusBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::byronTestnetDaedalusBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_bytes, canCreateAddressFromEnterpriseAddressBytes)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::enterpriseKeyBytes, sizeof(Cip19TestVectors::enterpriseKeyBytes), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::enterpriseKey);

  // compare bytes
  const byte_t* bytes = cardano_address_get_bytes(address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::enterpriseKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::enterpriseKeyBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_bytes, canCreateAddressFromPointerAddressBytes)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, sizeof(Cip19TestVectors::pointerKeyBytes), &address);

  // print bytes
  const byte_t* bytes = cardano_address_get_bytes(address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::pointerKey);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::pointerKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::pointerKeyBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_bytes, canCreateAddressFromRewardAddressBytes)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::rewardKeyBytes, sizeof(Cip19TestVectors::rewardKeyBytes), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::rewardKey);

  // compare bytes
  const byte_t* bytes = cardano_address_get_bytes(address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::rewardKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::rewardKeyBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_bytes, returnErrorWhenDataIsNull)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(NULL, 0, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_bytes, returnErrorWhenAddressIsNull)
{
  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes), NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_address_from_bytes, returnErrorWhenDataSizeIsZero)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, 0, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_bytes, returnErrorWhenBaseAddressnsInvalid)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::basePaymentKeyStakeKeyBytes, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes) - 1, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_bytes, returnErrorWhenEnterpriseAddressnsInvalid)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::enterpriseKeyBytes, sizeof(Cip19TestVectors::enterpriseKeyBytes) - 1, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_bytes, returnErrorWhenByronAddressnsInvalid)
{
  const byte_t invalid_byron_address[] = { 0x82, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97 };

  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(invalid_byron_address, sizeof(invalid_byron_address), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_bytes, returnErrorWhenAddressIsInvalid)
{
  const byte_t invalid_byron_address[] = { 0x90, 0xd8, 0x18, 0x58, 0x21, 0x83, 0x58, 0x1c, 0xba, 0x97 };

  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(invalid_byron_address, sizeof(invalid_byron_address), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_bytes, returnErrorWhenPointerAddressnsInvalid)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::pointerKeyBytes, 29, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_bytes, returnErrorWhenRewardAddressnsInvalid)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_bytes(Cip19TestVectors::rewardKeyBytes, sizeof(Cip19TestVectors::rewardKeyBytes) - 1, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, canCreateAddressFromString)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::basePaymentKeyStakeKey);

  // compare bytes
  const byte_t* bytes = cardano_address_get_bytes(address);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromBaseBasePaymentScriptStakeKey)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::basePaymentScriptStakeKey.c_str(), Cip19TestVectors::basePaymentScriptStakeKey.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::basePaymentScriptStakeKey);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromBasePaymentKeyStakeScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeScript.c_str(), Cip19TestVectors::basePaymentKeyStakeScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::basePaymentKeyStakeScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromBasePaymentScriptStakeScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::basePaymentScriptStakeScript.c_str(), Cip19TestVectors::basePaymentScriptStakeScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::basePaymentScriptStakeScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetBasePaymentKeyStakeKey)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetBasePaymentKeyStakeKey.c_str(), Cip19TestVectors::testnetBasePaymentKeyStakeKey.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetBasePaymentKeyStakeKey);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetBasePaymentScriptStakeKey)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetBasePaymentScriptStakeKey.c_str(), Cip19TestVectors::testnetBasePaymentScriptStakeKey.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetBasePaymentScriptStakeKey);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetBasePaymentKeyStakeScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetBasePaymentKeyStakeScript.c_str(), Cip19TestVectors::testnetBasePaymentKeyStakeScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetBasePaymentKeyStakeScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetBasePaymentScriptStakeScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetBasePaymentScriptStakeScript.c_str(), Cip19TestVectors::testnetBasePaymentScriptStakeScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetBasePaymentScriptStakeScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetPointerKey)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetPointerKey.c_str(), Cip19TestVectors::testnetPointerKey.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetPointerKey);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetPointerScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetPointerScript.c_str(), Cip19TestVectors::testnetPointerScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetPointerScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetEnterpriseKey)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetEnterpriseKey.c_str(), Cip19TestVectors::testnetEnterpriseKey.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetEnterpriseKey);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetEnterpriseScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetEnterpriseScript.c_str(), Cip19TestVectors::testnetEnterpriseScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetEnterpriseScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetRewardKey)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetRewardKey.c_str(), Cip19TestVectors::testnetRewardKey.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetRewardKey);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromTestnetRewardScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::testnetRewardScript.c_str(), Cip19TestVectors::testnetRewardScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::testnetRewardScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromPointerScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::pointerScript.c_str(), Cip19TestVectors::pointerScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::pointerScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromRewardScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::rewardScript.c_str(), Cip19TestVectors::rewardScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::rewardScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromEnterpriseScript)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::enterpriseScript.c_str(), Cip19TestVectors::enterpriseScript.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::enterpriseScript);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromByronMainnetYoroi)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::byronMainnetYoroi);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, canCreateAddressFromByronTestnetDaedalus)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(Cip19TestVectors::byronTestnetDaedalus.c_str(), Cip19TestVectors::byronTestnetDaedalus.size(), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);
  EXPECT_EQ(cardano_address_get_string(address), Cip19TestVectors::byronTestnetDaedalus);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_bytes_size, canGetAddressBytesSize)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_address_get_bytes_size(address);

  // Assert
  EXPECT_EQ(size, sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes));

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_bytes_size, returnZeroWhenAddressIsNull)
{
  // Act
  size_t size = cardano_address_get_bytes_size(NULL);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_address_get_bytes, canGetAddressBytes)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  const byte_t* bytes = cardano_address_get_bytes(address);

  // Assert
  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_bytes, returnsErrorWhenAddressIsNull)
{
  // Act
  const byte_t* bytes = cardano_address_get_bytes(NULL);

  // Assert
  EXPECT_EQ(bytes, nullptr);
}

TEST(cardano_address_to_bytes, canConvertAddressToBytes)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_address_to_bytes(address, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  for (size_t i = 0; i < sizeof(Cip19TestVectors::basePaymentKeyStakeKeyBytes); i++)
  {
    EXPECT_EQ(bytes[i], Cip19TestVectors::basePaymentKeyStakeKeyBytes[i]);
  }

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_to_bytes, returnsErrorWhenAddressIsNull)
{
  // Arrange
  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_address_to_bytes(NULL, bytes, sizeof(bytes));

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_address_to_bytes, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_address_to_bytes(address, NULL, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_to_bytes, returnsErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  byte_t bytes[64] = { 0 };

  // Act
  cardano_error_t result = cardano_address_to_bytes(address, bytes, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_from_string, returnsErrorWhenAddressIsNull)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string(NULL, 0, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenAddressIsInvalid)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string("invalid_address", 15, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenAddressSizeIsZero)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string("", 0, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenAddressIsTooSmall)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string("a", 1, &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenAddressOutIsNull)
{
  // Act
  cardano_error_t result = cardano_address_from_string("a", 1, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_address_from_string, returnsErrorWhenInvalidHrp)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string("addrqx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x", strlen("addrqx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x"), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenNoData)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string("addr_test1", strlen("addr_test1"), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenInvalidData)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string("addr_test12222222222222222222222", strlen("addr_test12222222222222222222222"), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenInvalidData2)
{
  // Arrange
  cardano_address_t* address = NULL;

  // Act
  cardano_error_t result = cardano_address_from_string("ilwwww2222222222222222222222", strlen("ilwwww2222222222222222222222"), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_from_string, returnsErrorWhenMemoryAllocationFails)
{
  // Arrange
  cardano_address_t* address = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  cardano_error_t result = cardano_address_from_string("2222222222222222", strlen("2222222222222222"), &address);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(address, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_address_get_string_size, canGetAddressStringSize)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_address_get_string_size(address);

  // Assert
  EXPECT_EQ(size, Cip19TestVectors::basePaymentKeyStakeKey.size() + 1 /* null terminator */);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_string_size, returnsZeroWhenAddressIsNull)
{
  // Act
  size_t size = cardano_address_get_string_size(NULL);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_address_to_string, canConvertAddressToString)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_address_to_string(address, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(std::string(buffer), Cip19TestVectors::basePaymentKeyStakeKey);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_to_string, returnErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  char buffer[150] = { 0 };

  // Act
  cardano_error_t result = cardano_address_to_string(address, buffer, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_to_string, returnsErrorWhenAddressIsNull)
{
  // Arrange
  char buffer[64] = { 0 };

  // Act
  cardano_error_t result = cardano_address_to_string(NULL, buffer, sizeof(buffer));

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_address_to_string, returnsErrorWhenBufferIsNull)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_address_to_string(address, NULL, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_to_string, returnsErrorWhenBufferIsTooSmall)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  char buffer[64] = { 0 };

  // Act
  cardano_error_t result = cardano_address_to_string(address, buffer, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_INSUFFICIENT_BUFFER_SIZE);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_string, returnErrorWhenAddressIsNull)
{
  // Act
  const char* address = cardano_address_get_string(NULL);

  // Assert
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_address_is_valid_bech32, canValidateAddress)
{
  // Act
  bool result = cardano_address_is_valid_bech32(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size());

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_address_is_valid_bech32, returnsFalseWhenAddressIsNull)
{
  // Act
  bool result = cardano_address_is_valid_bech32(NULL, 0);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_bech32, returnsFalseWhenAddressIsInvalid)
{
  // Act
  bool result = cardano_address_is_valid_bech32("invalid_address", 15);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_bech32, returnsFalseWhenInvalidHrp)
{
  // Act
  bool result = cardano_address_is_valid_bech32("addrqx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x", strlen("addrqx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x"));

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_bech32, returnsFalseWhenNoData)
{
  // Act
  bool result = cardano_address_is_valid_bech32("addr_test1", strlen("addr_test1"));

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_bech32, returnsFalseWhenInvalidData)
{
  // Act
  bool result = cardano_address_is_valid_bech32("addr_test12222222222222222222222", strlen("addr_test12222222222222222222222"));

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_byron, canValidateAddress)
{
  // Act
  bool result = cardano_address_is_valid_byron(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size());

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_address_is_valid_byron, returnsFalseWhenAddressIsNull)
{
  // Act
  bool result = cardano_address_is_valid_byron(NULL, 0);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_byron, returnsFalseWhenAddressIsInvalid)
{
  // Act
  bool result = cardano_address_is_valid_byron("invalid_address", 15);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_byron, returnsFalseWhenAddressIsInvalid2)
{
  // Act
  bool result = cardano_address_is_valid_byron("ilwwww2222222222222222222222", strlen("ilwwww2222222222222222222222"));

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid_byron, returnFalseIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  // Act
  bool result = cardano_address_is_valid_byron("2222222222222222", strlen("2222222222222222"));

  // Assert
  EXPECT_FALSE(result);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_address_is_valid, canValidateBech32Address)
{
  // Act
  bool result = cardano_address_is_valid(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size());

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_address_is_valid, canValidateByronAddress)
{
  // Act
  bool result = cardano_address_is_valid(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size());

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_address_is_valid, returnsFalseWhenAddressIsNull)
{
  // Act
  bool result = cardano_address_is_valid(NULL, 0);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_is_valid, returnsFalseWhenSizeIsZero)
{
  // Act
  bool result = cardano_address_is_valid("", 0);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_address_get_type, canGetAddressType)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_address_type_t type;

  EXPECT_EQ(cardano_address_get_type(address, &type), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(type, CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_type, returnsErrorWhenAddressIsNull)
{
  // Act
  cardano_address_type_t type;
  EXPECT_EQ(cardano_address_get_type(NULL, &type), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_address_get_type, returnsErrorWhenTypeIsNull)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_address_get_type(address, NULL), CARDANO_POINTER_IS_NULL);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_network_id, canGetNetworkId)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_network_id_t network_id;

  EXPECT_EQ(cardano_address_get_network_id(address, &network_id), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_network_id, returnsErrorWhenAddressIsNull)
{
  // Act
  cardano_network_id_t network_id;
  EXPECT_EQ(cardano_address_get_network_id(NULL, &network_id), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_address_get_network_id, returnsErrorWhenNetworkIdIsNull)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_address_get_network_id(address, NULL), CARDANO_POINTER_IS_NULL);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_network_id, returnsErrorWhenAddressIsInvalid)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string("invalid_address", 15, &address), CARDANO_ERROR_DECODING);

  // Act
  cardano_network_id_t network_id;
  EXPECT_EQ(cardano_address_get_network_id(address, &network_id), CARDANO_POINTER_IS_NULL);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_get_network_id, returnsErrorWhenByronAddressIsInvalid)
{
  // Arrange
  cardano_address_t* byron_address = (cardano_address_t*)_cardano_malloc(sizeof(cardano_address_t));

  byron_address->type               = CARDANO_ADDRESS_TYPE_BYRON;
  byron_address->network_id         = NULL;
  byron_address->stake_pointer      = NULL;
  byron_address->payment_credential = NULL;
  byron_address->stake_credential   = NULL;
  byron_address->byron_content      = NULL;
  byron_address->base.deallocator   = _cardano_address_deallocate;
  byron_address->base.ref_count     = 1;

  // Act
  cardano_network_id_t network_id;
  EXPECT_EQ(cardano_address_get_network_id(byron_address, &network_id), CARDANO_INVALID_ADDRESS_FORMAT);

  // Clean up
  cardano_address_unref(&byron_address);
}

TEST(cardano_address_get_network_id, returnsMainetIfByronAddressDoesntHaveMagicSet)
{
  // Arrange
  cardano_address_t* byron_address = (cardano_address_t*)_cardano_malloc(sizeof(cardano_address_t));

  byron_address->type                            = CARDANO_ADDRESS_TYPE_BYRON;
  byron_address->network_id                      = NULL;
  byron_address->stake_pointer                   = NULL;
  byron_address->payment_credential              = NULL;
  byron_address->stake_credential                = NULL;
  byron_address->base.deallocator                = _cardano_address_deallocate;
  byron_address->base.ref_count                  = 1;
  byron_address->byron_content                   = (cardano_byron_address_content_t*)_cardano_malloc(sizeof(cardano_byron_address_content_t));
  byron_address->byron_content->attributes.magic = -1;

  // Act
  cardano_network_id_t network_id;
  EXPECT_EQ(cardano_address_get_network_id(byron_address, &network_id), CARDANO_SUCCESS);
  EXPECT_EQ(network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Clean up
  cardano_address_unref(&byron_address);
}

TEST(cardano_address_get_network_id, returnsTestnetIfByronAddressHaveMagicSet)
{
  // Arrange
  cardano_address_t* byron_address = (cardano_address_t*)_cardano_malloc(sizeof(cardano_address_t));

  byron_address->type                            = CARDANO_ADDRESS_TYPE_BYRON;
  byron_address->network_id                      = NULL;
  byron_address->stake_pointer                   = NULL;
  byron_address->payment_credential              = NULL;
  byron_address->stake_credential                = NULL;
  byron_address->base.deallocator                = _cardano_address_deallocate;
  byron_address->base.ref_count                  = 1;
  byron_address->byron_content                   = (cardano_byron_address_content_t*)_cardano_malloc(sizeof(cardano_byron_address_content_t));
  byron_address->byron_content->attributes.magic = 42;

  // Act
  cardano_network_id_t network_id;
  EXPECT_EQ(cardano_address_get_network_id(byron_address, &network_id), CARDANO_SUCCESS);
  EXPECT_EQ(network_id, CARDANO_NETWORK_ID_TEST_NET);

  // Clean up
  cardano_address_unref(&byron_address);
}

TEST(cardano_address_get_network_id, returnsErrorIfAddressInternalStateIsInvalid)
{
  // Arrange
  cardano_address_t* address = (cardano_address_t*)_cardano_malloc(sizeof(cardano_address_t));

  address->type               = CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY;
  address->network_id         = NULL;
  address->stake_pointer      = NULL;
  address->payment_credential = NULL;
  address->stake_credential   = NULL;
  address->byron_content      = NULL;
  address->base.deallocator   = _cardano_address_deallocate;
  address->base.ref_count     = 1;

  // Act
  cardano_network_id_t network_id;
  EXPECT_EQ(cardano_address_get_network_id(address, &network_id), CARDANO_INVALID_ADDRESS_FORMAT);

  // Clean up
  cardano_address_unref(&address);
}

TEST(cardano_address_to_byron_address, canConvertAddressToByronAddress)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::byronMainnetYoroi.c_str(), Cip19TestVectors::byronMainnetYoroi.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_t*      byron_address      = cardano_address_to_byron_address(address);
  cardano_base_address_t*       base_address       = cardano_address_to_base_address(address);
  cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);
  cardano_pointer_address_t*    pointer_address    = cardano_address_to_pointer_address(address);
  cardano_reward_address_t*     reward_address     = cardano_address_to_reward_address(address);

  // Assert
  EXPECT_NE(byron_address, nullptr);
  EXPECT_EQ(base_address, nullptr);
  EXPECT_EQ(enterprise_address, nullptr);
  EXPECT_EQ(pointer_address, nullptr);
  EXPECT_EQ(reward_address, nullptr);

  // Clean up
  cardano_byron_address_unref(&byron_address);
  cardano_address_unref(&address);
}

TEST(cardano_address_to_base_address, canConvertAddressToBaseAddress)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_t*      byron_address      = cardano_address_to_byron_address(address);
  cardano_base_address_t*       base_address       = cardano_address_to_base_address(address);
  cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);
  cardano_pointer_address_t*    pointer_address    = cardano_address_to_pointer_address(address);
  cardano_reward_address_t*     reward_address     = cardano_address_to_reward_address(address);

  // Assert
  EXPECT_EQ(byron_address, nullptr);
  EXPECT_NE(base_address, nullptr);
  EXPECT_EQ(enterprise_address, nullptr);
  EXPECT_EQ(pointer_address, nullptr);
  EXPECT_EQ(reward_address, nullptr);

  // Clean up
  cardano_base_address_unref(&base_address);
  cardano_address_unref(&address);
}

TEST(cardano_address_to_enterprise_address, canConvertAddressToEnterpriseAddress)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::enterpriseKey.c_str(), Cip19TestVectors::enterpriseKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_t*      byron_address      = cardano_address_to_byron_address(address);
  cardano_base_address_t*       base_address       = cardano_address_to_base_address(address);
  cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);
  cardano_pointer_address_t*    pointer_address    = cardano_address_to_pointer_address(address);
  cardano_reward_address_t*     reward_address     = cardano_address_to_reward_address(address);

  // Assert
  EXPECT_EQ(byron_address, nullptr);
  EXPECT_EQ(base_address, nullptr);
  EXPECT_NE(enterprise_address, nullptr);
  EXPECT_EQ(pointer_address, nullptr);
  EXPECT_EQ(reward_address, nullptr);

  // Clean up
  cardano_enterprise_address_unref(&enterprise_address);
  cardano_address_unref(&address);
}

TEST(cardano_address_to_pointer_address, canConvertAddressToPointerAddress)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::pointerKey.c_str(), Cip19TestVectors::pointerKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_t*      byron_address      = cardano_address_to_byron_address(address);
  cardano_base_address_t*       base_address       = cardano_address_to_base_address(address);
  cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);
  cardano_pointer_address_t*    pointer_address    = cardano_address_to_pointer_address(address);
  cardano_reward_address_t*     reward_address     = cardano_address_to_reward_address(address);

  // Assert
  EXPECT_EQ(byron_address, nullptr);
  EXPECT_EQ(base_address, nullptr);
  EXPECT_EQ(enterprise_address, nullptr);
  EXPECT_NE(pointer_address, nullptr);
  EXPECT_EQ(reward_address, nullptr);

  // Clean up
  cardano_pointer_address_unref(&pointer_address);
  cardano_address_unref(&address);
}

TEST(cardano_address_to_reward_address, canConvertAddressToRewardAddress)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::rewardKey.c_str(), Cip19TestVectors::rewardKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_byron_address_t*      byron_address      = cardano_address_to_byron_address(address);
  cardano_base_address_t*       base_address       = cardano_address_to_base_address(address);
  cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);
  cardano_pointer_address_t*    pointer_address    = cardano_address_to_pointer_address(address);
  cardano_reward_address_t*     reward_address     = cardano_address_to_reward_address(address);

  // Assert
  EXPECT_EQ(byron_address, nullptr);
  EXPECT_EQ(base_address, nullptr);
  EXPECT_EQ(enterprise_address, nullptr);
  EXPECT_EQ(pointer_address, nullptr);
  EXPECT_NE(reward_address, nullptr);

  // Clean up
  cardano_reward_address_unref(&reward_address);
  cardano_address_unref(&address);
}

TEST(cardano_address_to_byron_address, returnsNullWhenAddressIsNull)
{
  // Act
  cardano_byron_address_t* byron_address = cardano_address_to_byron_address(NULL);

  // Assert
  EXPECT_EQ(byron_address, nullptr);
}

TEST(cardano_address_to_base_address, returnsNullWhenAddressIsNull)
{
  // Act
  cardano_base_address_t* base_address = cardano_address_to_base_address(NULL);

  // Assert
  EXPECT_EQ(base_address, nullptr);
}

TEST(cardano_address_to_enterprise_address, returnsNullWhenAddressIsNull)
{
  // Act
  cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(NULL);

  // Assert
  EXPECT_EQ(enterprise_address, nullptr);
}

TEST(cardano_address_to_pointer_address, returnsNullWhenAddressIsNull)
{
  // Act
  cardano_pointer_address_t* pointer_address = cardano_address_to_pointer_address(NULL);

  // Assert
  EXPECT_EQ(pointer_address, nullptr);
}

TEST(cardano_address_to_reward_address, returnsNullWhenAddressIsNull)
{
  // Act
  cardano_reward_address_t* reward_address = cardano_address_to_reward_address(NULL);

  // Assert
  EXPECT_EQ(reward_address, nullptr);
}

TEST(cardano_address_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_address_ref(address);

  // Assert
  EXPECT_THAT(address, testing::Not((cardano_address_t*)nullptr));
  EXPECT_EQ(cardano_address_refcount(address), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_address_unref(&address);
  cardano_address_unref(&address);
}

TEST(cardano_address_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_address_ref(nullptr);
}

TEST(cardano_address_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_address_t* address = nullptr;

  // Act
  cardano_address_unref(&address);
}

TEST(cardano_address_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_address_unref((cardano_address_t**)nullptr);
}

TEST(cardano_address_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_address_ref(address);
  size_t ref_count = cardano_address_refcount(address);

  cardano_address_unref(&address);
  size_t updated_ref_count = cardano_address_refcount(address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_address_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  cardano_address_ref(address);
  size_t ref_count = cardano_address_refcount(address);

  cardano_address_unref(&address);
  size_t updated_ref_count = cardano_address_refcount(address);

  cardano_address_unref(&address);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(address, (cardano_address_t*)nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_address_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  // Act
  EXPECT_THAT(cardano_address_move(address), testing::Not((cardano_address_t*)nullptr));
  size_t ref_count = cardano_address_refcount(address);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(address, testing::Not((cardano_address_t*)nullptr));

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_address_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_address_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_address_move, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_address_t* address = cardano_address_move(nullptr);

  // Assert
  EXPECT_EQ(address, (cardano_address_t*)nullptr);
}

TEST(cardano_address_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  const char* message = "This is a test message";

  // Act
  cardano_address_set_last_error(address, message);
  const char* last_error = cardano_address_get_last_error(address);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_address_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_address_t* address = nullptr;

  // Act
  const char* last_error = cardano_address_get_last_error(address);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_address_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_address_t* address = nullptr;
  const char*        message = "This is a test message";

  // Act
  cardano_address_set_last_error(address, message);

  // Assert
  EXPECT_STREQ(cardano_address_get_last_error(address), "Object is NULL.");
}

TEST(cardano_address_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_address_t* address = NULL;
  EXPECT_EQ(cardano_address_from_string(Cip19TestVectors::basePaymentKeyStakeKey.c_str(), Cip19TestVectors::basePaymentKeyStakeKey.size(), &address), CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_address_set_last_error(address, message);

  // Assert
  EXPECT_STREQ(cardano_address_get_last_error(address), "");

  // Cleanup
  cardano_address_unref(&address);
}
