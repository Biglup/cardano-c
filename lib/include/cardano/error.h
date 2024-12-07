/**
 * \file error.h
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ERROR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ERROR_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Cardano C library error codes.
 */
typedef enum
{
  /**
   * \brief Successful operation.
   */
  CARDANO_SUCCESS = 0,

  /**
   * \brief Generic error.
   */
  CARDANO_ERROR_GENERIC = 1,

  /**
   * \brief Insufficient buffer size.
   */
  CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE = 2,

  /**
   * \brief Operation over a null pointer.
   */
  CARDANO_ERROR_POINTER_IS_NULL = 3,

  /**
   * \brief Memory could not be allocated.
   */
  CARDANO_ERROR_MEMORY_ALLOCATION_FAILED = 4,

  /**
   * \brief Out of bounds memory read.
   */
  CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ = 5,

  /**
   * \brief Out of bounds memory write.
   */
  CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE = 6,

  /**
   * \brief Invalid argument.
   */
  CARDANO_ERROR_INVALID_ARGUMENT = 7,

  /**
   * \brief Invalid URL.
   */
  CARDANO_ERROR_INVALID_URL = 8,

  /**
   * \brief Element not found.
   */
  CARDANO_ERROR_ELEMENT_NOT_FOUND = 9,

  /**
   * \brief Encoding failure.
   */
  CARDANO_ERROR_ENCODING = 10,

  /**
   * \brief Decoding failure.
   */
  CARDANO_ERROR_DECODING = 11,

  /**
   * \brief Invalid checksum.
   */
  CARDANO_ERROR_CHECKSUM_MISMATCH = 12,

  /**
   * \brief Invalid JSON.
   */
  CARDANO_ERROR_INVALID_JSON = 13,

  /**
   * \brief Size overflow.
   */
  CARDANO_ERROR_INTEGER_OVERFLOW = 14,

  /**
   * \brief Size underflow.
   */
  CARDANO_ERROR_INTEGER_UNDERFLOW = 15,

  /**
   * \brief Conversion error.
   */
  CARDANO_ERROR_CONVERSION_FAILED = 16,

  /**
   * \brief Index out of bounds.
   */
  CARDANO_ERROR_INDEX_OUT_OF_BOUNDS = 17,

  /**
   * \brief Invalid certificate type.
   */
  CARDANO_ERROR_INVALID_CERTIFICATE_TYPE = 18,

  /**
   * \brief The operation is not implemented.
   */
  CARDANO_ERROR_NOT_IMPLEMENTED = 19,

  /**
   * \brief The passphrase is invalid.
   */
  CARDANO_ERROR_INVALID_PASSPHRASE = 20,

  /**
   * \brief The state of the object is illegal.
   */
  CARDANO_ERROR_ILLEGAL_STATE = 21,

  /**
   * \brief The element is already present.
   */
  CARDANO_ERROR_DUPLICATED_KEY = 22,

  /**
   * \brief The JSON type is unexpected.
   */
  CARDANO_ERROR_JSON_TYPE_MISMATCH = 23,

  /* Serialization errors */

  /**
   * \brief The serialization or deserialization process resulted in a loss of precision.
   */
  CARDANO_ERROR_LOSS_OF_PRECISION = 100,

  /**
   * \brief The magic number is invalid.
   */
  CARDANO_ERROR_INVALID_MAGIC = 101,

  /**
   * \brief The checksum is invalid.
   */
  CARDANO_ERROR_INVALID_CHECKSUM = 102,

  /* Crypto errors */

  /**
   * \brief The hash size is invalid.
   */
  CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE = 200,

  /**
   * \brief The Ed25519 signature size is invalid.
   */
  CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE = 201,

  /**
   * \brief The Ed25519 public key size is invalid.
   */
  CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE = 202,

  /**
   * \brief The Ed25519 private key size is invalid.
   */
  CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE = 203,

  /**
   * \brief The BIP32 public key size is invalid.
   */
  CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE = 204,

  /**
   * \brief The BIP32 private key size is invalid.
   */
  CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE = 205,

  /**
   * \brief The BIP32 derivation index is invalid.
   */
  CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX = 206,

  /* CBOR errors */

  /**
   * \brief The CBOR type is unexpected.
   */
  CARDANO_ERROR_UNEXPECTED_CBOR_TYPE = 300,

  /**
   * \brief The CBOR value is of the right type, but the value is invalid (I.E out of range).
   */
  CARDANO_ERROR_INVALID_CBOR_VALUE = 301,

  /**
   * \brief The CBOR array size is invalid.
   */
  CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE = 302,

  /**
   * \brief The CBOR map size is invalid.
   */
  CARDANO_ERROR_INVALID_CBOR_MAP_SIZE = 303,

  /**
   * \brief The CBOR map key is duplicated.
   */
  CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY = 304,

  /**
   * \brief The CBOR map key is invalid.
   */
  CARDANO_ERROR_INVALID_CBOR_MAP_KEY = 305,

  // Address errors

  /**
   * \brief The address type is invalid.
   */
  CARDANO_ERROR_INVALID_ADDRESS_TYPE = 400,

  /**
   * \brief The address format is invalid.
   */
  CARDANO_ERROR_INVALID_ADDRESS_FORMAT = 401,

  // Credential errors

  /**
   * \brief The credential type is invalid.
   */
  CARDANO_ERROR_INVALID_CREDENTIAL_TYPE = 500,

  // Plutus errors

  /**
   * \brief The Plutus data conversion is invalid.
   */
  CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION = 600,

  /**
   * \brief The Plutus datum type is invalid.
   */
  CARDANO_ERROR_INVALID_DATUM_TYPE = 601,

  // Script errors

  /**
   * \brief The script language is invalid.
   */
  CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE = 700,

  /**
   * \brief The native script type is invalid.
   */
  CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE = 701,

  /**
   * \brief The Plutus cost model is invalid.
   */
  CARDANO_ERROR_INVALID_PLUTUS_COST_MODEL = 702,

  // Proposal procedure errors

  /**
   * \brief The governance action type is invalid.
   */
  CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE = 800,

  // Auxiliary data errors

  /**
   * \brief The metadatum conversion is invalid.
   */
  CARDANO_ERROR_INVALID_METADATUM_CONVERSION = 900,

  /**
   * \brief The metadatum text string size.
   */
  CARDANO_ERROR_INVALID_METADATUM_TEXT_STRING_SIZE = 901,

  /**
   * \brief The metadatum bounded bytes size.
   */
  CARDANO_ERROR_INVALID_METADATUM_BOUNDED_BYTES_SIZE = 902,

  // HTTP

  /**
   * \brief The HTTP request is invalid.
   */
  CARDANO_ERROR_INVALID_HTTP_REQUEST = 1000,

  // Coin Selection

  /**
   * \brief Total value of the entries within the initial UTxO set (the amount of money available)
   * is less than the the total value of all entries in the requested output set (the amount of money required).
   */
  CARDANO_ERROR_BALANCE_INSUFFICIENT = 1100,

  /**
   * \brief The number of entries in the initial UTxO set is smaller than the number of entries in the requested output set,
   * for algorithms that impose the restriction that a single UTxO entry can only be used to pay for at most one output.
   */
  CARDANO_ERROR_UTXO_NOT_FRAGMENTED_ENOUGH = 1101,

  /**
   * \brief The algorithm depletes all entries from the initial UTxO set
   * before it is able to pay for all outputs in the requested output set.
   * This can happen even if the total value of entries within the initial UTxO set
   * is greater than the total value of all entries in the requested output set,
   * due to various restrictions that coin selection algorithms impose on themselves when selecting UTxO entries.
   */
  CARDANO_ERROR_UTXO_FULLY_DEPLETED = 1102,

  /**
   * \brief Another input must be selected by the algorithm in order to continue making progress,
   * but doing so will increase the size of the resulting selection beyond an acceptable limit,
   * specified by the maximum input count parameter.
   */
  CARDANO_ERROR_MAXIMUM_INPUT_COUNT_EXCEEDED = 1103,

  // TX Evaluation Failures

  /**
   * \brief The transaction script evaluation failed.
   */
  CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE = 1200,
} cardano_error_t;

/**
 * \brief Converts error codes to their human readable form.
 *
 * \param[in] error The error code to get the string representation for.
 * \return Human readable form of the given error code.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_error_to_string(cardano_error_t error);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ERROR_H