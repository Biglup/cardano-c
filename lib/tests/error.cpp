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

TEST(cardano_error_to_string, canConvertGeneric)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_GENERIC;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Generic error");
}

TEST(cardano_error_to_string, canConvertInsufficientBufferSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Insufficient buffer size");
}

TEST(cardano_error_to_string, canConvertPointerIsNull)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_POINTER_IS_NULL;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Argument is a NULL pointer");
}

TEST(cardano_error_to_string, canConvertMemoryAllocationFailed)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Requested memory could not be allocated");
}

TEST(cardano_error_to_string, canConvertOutOfBoundsMemoryRead)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Out of bounds memory read");
}

TEST(cardano_error_to_string, canConvertOutOfBoundsMemoryWrite)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Out of bounds memory write");
}

TEST(cardano_error_to_string, canConvertInvalidBlake2bHashSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid Blake2b hash size");
}

TEST(cardano_error_to_string, canConvertInvalidEd25519SignatureSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid Ed25519 signature size");
}

TEST(cardano_error_to_string, canConvertInvalidEd25519PublicKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid Ed25519 public key size");
}

TEST(cardano_error_to_string, canConvertInvalidEd25519PrivateKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid Ed25519 private key size");
}

TEST(cardano_error_to_string, canConvertInvalidBip32PublicKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid BIP32 public key size");
}

TEST(cardano_error_to_string, canConvertInvalidBip32PrivateKeySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid BIP32 private key size");
}

TEST(cardano_error_to_string, canConvertInvalidArgument)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ARGUMENT;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid argument");
}

TEST(cardano_error_to_string, canConvertInvalidBip32DerivationIndex)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid BIP32 derivation index");
}

TEST(cardano_error_to_string, canConvertEncoding)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_ENCODING;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Encoding failure");
}

TEST(cardano_error_to_string, canConvertDecoding)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_DECODING;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Decoding failure");
}

TEST(cardano_error_to_string, canConvertChecksumMismatch)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_CHECKSUM_MISMATCH;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Checksum mismatch");
}

TEST(cardano_error_to_string, canConvertLossOfPrecision)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_LOSS_OF_PRECISION;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Loss of precision");
}

TEST(cardano_error_to_string, canConvertUnexpectedCborType)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_UNEXPECTED_CBOR_TYPE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Unexpected CBOR type");
}

TEST(cardano_error_to_string, canConvertInvalidCborValue)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_CBOR_VALUE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid CBOR value");
}

TEST(cardano_error_to_string, canConvertInvalidCborArraySize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid CBOR array size");
}

TEST(cardano_error_to_string, canConvertInvalidCborMapSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_CBOR_MAP_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid CBOR map size");
}

TEST(cardano_error_to_string, canConvertInvalidAddressType)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ADDRESS_TYPE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid address type");
}

TEST(cardano_error_to_string, canConvertInvalidAddressFormat)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_ADDRESS_FORMAT;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid address format");
}

TEST(cardano_error_to_string, canConvertInvalidCredentialType)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_CREDENTIAL_TYPE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid credential type");
}

TEST(cardano_error_to_string, canConvertInvalidUrl)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_URL;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid argument. Invalid URL");
}

TEST(cardano_error_to_string, canConvertInvalidPlutusDataConversion)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid Plutus data conversion");
}

TEST(cardano_error_to_string, canConvertElementNotFound)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Element not found");
}

TEST(cardano_error_to_string, canConvertInvalidDatumType)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_DATUM_TYPE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid datum type");
}

TEST(cardano_error_to_string, canConvertInvalidScriptLanguage)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid script language");
}

TEST(cardano_error_to_string, canConvertInvalidNativeScriptType)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid native script type");
}

TEST(CARDANO_ERROR_INVALID_JSON, canConvertInvalidJson)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_JSON;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid JSON");
}

TEST(CARDANO_ERROR_INTEGER_OVERFLOW, canConvertIntegerOverflow)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INTEGER_OVERFLOW;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Integer overflow");
}

TEST(CARDANO_ERROR_INTEGER_UNDERFLOW, canConvertIntegerUnderflow)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INTEGER_UNDERFLOW;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Integer underflow");
}

TEST(CARDANO_ERROR_CONVERSION_FAILED, canConvertConversionError)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_CONVERSION_FAILED;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Conversion error");
}

TEST(cardano_error_to_string, canConvertInvalidPlutusCostModel)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_PLUTUS_COST_MODEL;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid Plutus cost model");
}

TEST(cardano_error_to_string, canConvertIndexOutOfBounds)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Index out of bounds");
}

TEST(cardano_error_to_string, canConvertDuplicatedCborMapKey)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Duplicated CBOR map key");
}

TEST(cardano_error_to_string, canConvertInvalidCborMapKey)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_CBOR_MAP_KEY;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid CBOR map key");
}

TEST(cardano_error_to_string, canConvertInvalidCertificateType)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid certificate type");
}

TEST(cardano_error_to_string, canConvertInvalidProcedureProposalType)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid procedure proposal type");
}

TEST(cardano_error_to_string, canConvertInvalidMetadatumConversion)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_METADATUM_CONVERSION;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid metadatum conversion");
}

TEST(cardano_error_to_string, canConvertInvalidMetadatumTextStringSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_METADATUM_TEXT_STRING_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid metadatum text string size, must be less than 64 bytes");
}

TEST(cardano_error_to_string, canConvertInvalidMetadatumBoundedBytesSize)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_METADATUM_BOUNDED_BYTES_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid metadatum bounded bytes size, must be less than 64 bytes");
}

TEST(cardano_error_to_string, canConvertNotImplemented)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_NOT_IMPLEMENTED;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Not implemented");
}

TEST(cardano_error_to_string, canConvertInvalidHttpRequest)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_HTTP_REQUEST;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid HTTP request");
}

TEST(cardano_error_to_string, invalidMagic)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_MAGIC;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid magic");
}

TEST(cardano_error_to_string, canConvertInvalidChecksum)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_CHECKSUM;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid checksum");
}

TEST(cardano_error_to_string, canConvertInvalidPassphrase)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_INVALID_PASSPHRASE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid passphrase");
}

TEST(cardano_error_to_string, canConvertBalanceInsufficient)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_BALANCE_INSUFFICIENT;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Insufficient balance");
}

TEST(cardano_error_to_string, canConvertUtxoNotFragmentedEnough)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_UTXO_NOT_FRAGMENTED_ENOUGH;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "UTXO not fragmented enough");
}

TEST(cardano_error_to_string, canConvertUtxoFullyDepleted)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_UTXO_FULLY_DEPLETED;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "UTXO fully depleted");
}

TEST(cardano_error_to_string, canConvertMaximumInputCountExceeded)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_MAXIMUM_INPUT_COUNT_EXCEEDED;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Maximum input count exceeded");
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
