/**
 * \file error.cpp
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
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
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_error_to_string, canConvertSuccess)
{
  // Arrange
  cardano_error_t error = CARDANO_SUCCESS;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Successful operation");
}

TEST(cardano_error_to_string, canConvertGenericError)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_GENERIC;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Generic error");
}

TEST(cardano_error_to_string, canConvertLossOfPrecisionr)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_LOSS_OF_PRECISION;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid conversion. Loss of precision");
}

TEST(cardano_error_to_string, canConvertInsufficientBufferSize)
{
  // Arrange
  cardano_error_t error = CARDANO_INSUFFICIENT_BUFFER_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Insufficient buffer size");
}

TEST(cardano_error_to_string, canConvertNullPointer)
{
  // Arrange
  cardano_error_t error = CARDANO_POINTER_IS_NULL;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Argument is a NULL pointer");
}

TEST(cardano_error_to_string, canConvertUnknown)
{
  // Arrange
  cardano_error_t error = (cardano_error_t)99999999;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Unknown error code");
}

TEST(cardano_error_to_string, canConvertMemoryAllocationFailed)
{
  // Arrange
  cardano_error_t error = CARDANO_MEMORY_ALLOCATION_FAILED;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Requested memory could not be allocated");
}

TEST(cardano_error_to_string, canConvertOutOfBoundsMemoryRead)
{
  // Arrange
  cardano_error_t error = CARDANO_OUT_OF_BOUNDS_MEMORY_READ;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Out of bounds memory read");
}

TEST(cardano_error_to_string, canConvertOutOfBoundsMemoryWrite)
{
  // Arrange
  cardano_error_t error = CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Out of bounds memory write");
}

TEST(cardano_error_to_string, canConvertInvalidBlake2bHashSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid Blake2b hash size");
}

TEST(cardano_error_to_string, canConvertInvalidEd25519SignatureSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid Ed25519 signature size");
}

TEST(cardano_error_to_string, canConvertInvalidEd25519PublicKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid Ed25519 public key size");
}

TEST(cardano_error_to_string, canConvertInvalidEd25519PrivateKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid Ed25519 private key size");
}

TEST(cardano_error_to_string, canConvertInvalidBip32PublicKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid BIP32 public key size");
}

TEST(cardano_error_to_string, canConvertInvalidArgument)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ARGUMENT;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid argument");
}

TEST(cardano_error_to_string, canConvertInvalidBip32PrivateKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid BIP32 private key size");
}

TEST(cardano_error_to_string, canConvertInvalidBip32DerivationIndex)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Invalid BIP32 derivation index");
}
