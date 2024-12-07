/**
 * \file error.c
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

/* INCLUDES ******************************************************************/

#include <cardano/error.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_error_to_string(const cardano_error_t error)
{
  const char* message;

  switch (error)
  {
    case CARDANO_SUCCESS:
      message = "Successful operation";
      break;
    case CARDANO_ERROR_GENERIC:
      message = "Generic error";
      break;
    case CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE:
      message = "Insufficient buffer size";
      break;
    case CARDANO_ERROR_POINTER_IS_NULL:
      message = "Argument is a NULL pointer";
      break;
    case CARDANO_ERROR_MEMORY_ALLOCATION_FAILED:
      message = "Requested memory could not be allocated";
      break;
    case CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ:
      message = "Out of bounds memory read";
      break;
    case CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE:
      message = "Out of bounds memory write";
      break;
    case CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE:
      message = "Invalid Blake2b hash size";
      break;
    case CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE:
      message = "Invalid Ed25519 signature size";
      break;
    case CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE:
      message = "Invalid Ed25519 public key size";
      break;
    case CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE:
      message = "Invalid Ed25519 private key size";
      break;
    case CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE:
      message = "Invalid BIP32 public key size";
      break;
    case CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE:
      message = "Invalid BIP32 private key size";
      break;
    case CARDANO_ERROR_INVALID_ARGUMENT:
      message = "Invalid argument";
      break;
    case CARDANO_ERROR_INVALID_URL:
      message = "Invalid argument. Invalid URL";
      break;
    case CARDANO_ERROR_ELEMENT_NOT_FOUND:
      message = "Element not found";
      break;
    case CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX:
      message = "Invalid BIP32 derivation index";
      break;
    case CARDANO_ERROR_ENCODING:
      message = "Encoding failure";
      break;
    case CARDANO_ERROR_DECODING:
      message = "Decoding failure";
      break;
    case CARDANO_ERROR_CHECKSUM_MISMATCH:
      message = "Checksum mismatch";
      break;
    case CARDANO_ERROR_INVALID_JSON:
      message = "Invalid JSON";
      break;
    case CARDANO_ERROR_INTEGER_OVERFLOW:
      message = "Integer overflow";
      break;
    case CARDANO_ERROR_INTEGER_UNDERFLOW:
      message = "Integer underflow";
      break;
    case CARDANO_ERROR_CONVERSION_FAILED:
      message = "Conversion error";
      break;
    case CARDANO_ERROR_LOSS_OF_PRECISION:
      message = "Loss of precision";
      break;
    case CARDANO_ERROR_INVALID_MAGIC:
      message = "Invalid magic";
      break;
    case CARDANO_ERROR_INVALID_CHECKSUM:
      message = "Invalid checksum";
      break;
    case CARDANO_ERROR_UNEXPECTED_CBOR_TYPE:
      message = "Unexpected CBOR type";
      break;
    case CARDANO_ERROR_INVALID_CBOR_VALUE:
      message = "Invalid CBOR value";
      break;
    case CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE:
      message = "Invalid CBOR array size";
      break;
    case CARDANO_ERROR_INVALID_CBOR_MAP_SIZE:
      message = "Invalid CBOR map size";
      break;
    case CARDANO_ERROR_INVALID_ADDRESS_TYPE:
      message = "Invalid address type";
      break;
    case CARDANO_ERROR_INVALID_ADDRESS_FORMAT:
      message = "Invalid address format";
      break;
    case CARDANO_ERROR_INVALID_CREDENTIAL_TYPE:
      message = "Invalid credential type";
      break;
    case CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION:
      message = "Invalid Plutus data conversion";
      break;
    case CARDANO_ERROR_INVALID_DATUM_TYPE:
      message = "Invalid datum type";
      break;
    case CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE:
      message = "Invalid script language";
      break;
    case CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE:
      message = "Invalid native script type";
      break;
    case CARDANO_ERROR_INVALID_PLUTUS_COST_MODEL:
      message = "Invalid Plutus cost model";
      break;
    case CARDANO_ERROR_INDEX_OUT_OF_BOUNDS:
      message = "Index out of bounds";
      break;
    case CARDANO_ERROR_INVALID_CERTIFICATE_TYPE:
      message = "Invalid certificate type";
      break;
    case CARDANO_ERROR_NOT_IMPLEMENTED:
      message = "Not implemented";
      break;
    case CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY:
      message = "Duplicated CBOR map key";
      break;
    case CARDANO_ERROR_INVALID_CBOR_MAP_KEY:
      message = "Invalid CBOR map key";
      break;
    case CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE:
      message = "Invalid procedure proposal type";
      break;
    case CARDANO_ERROR_INVALID_METADATUM_CONVERSION:
      message = "Invalid metadatum conversion";
      break;
    case CARDANO_ERROR_INVALID_METADATUM_TEXT_STRING_SIZE:
      message = "Invalid metadatum text string size, must be less than 64 bytes";
      break;
    case CARDANO_ERROR_INVALID_METADATUM_BOUNDED_BYTES_SIZE:
      message = "Invalid metadatum bounded bytes size, must be less than 64 bytes";
      break;
    case CARDANO_ERROR_INVALID_HTTP_REQUEST:
      message = "Invalid HTTP request";
      break;
    case CARDANO_ERROR_INVALID_PASSPHRASE:
      message = "Invalid passphrase";
      break;
    case CARDANO_ERROR_ILLEGAL_STATE:
      message = "Illegal state";
      break;
    case CARDANO_ERROR_DUPLICATED_KEY:
      message = "Duplicated key";
      break;
    case CARDANO_ERROR_BALANCE_INSUFFICIENT:
      message = "Insufficient balance";
      break;
    case CARDANO_ERROR_UTXO_NOT_FRAGMENTED_ENOUGH:
      message = "UTXO not fragmented enough";
      break;
    case CARDANO_ERROR_UTXO_FULLY_DEPLETED:
      message = "UTXO fully depleted";
      break;
    case CARDANO_ERROR_MAXIMUM_INPUT_COUNT_EXCEEDED:
      message = "Maximum input count exceeded";
      break;
    case CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE:
      message = "Script evaluation failure";
      break;
    case CARDANO_ERROR_JSON_TYPE_MISMATCH:
      message = "JSON type mismatch";
      break;
    default:
      message = "Unknown error code";
      break;
  }

  return message;
}
