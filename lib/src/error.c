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
    case CARDANO_INSUFFICIENT_BUFFER_SIZE:
      message = "Invalid operation. Insufficient buffer size";
      break;
    case CARDANO_POINTER_IS_NULL:
      message = "Invalid operation. Argument is a NULL pointer";
      break;
    case CARDANO_MEMORY_ALLOCATION_FAILED:
      message = "Invalid operation. Requested memory could not be allocated";
      break;
    case CARDANO_OUT_OF_BOUNDS_MEMORY_READ:
      message = "Invalid operation. Out of bounds memory read";
      break;
    case CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE:
      message = "Invalid operation. Out of bounds memory write";
      break;
    case CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE:
      message = "Invalid operation. Invalid Blake2b hash size";
      break;
    case CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE:
      message = "Invalid operation. Invalid Ed25519 signature size";
      break;
    case CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE:
      message = "Invalid operation. Invalid Ed25519 public key size";
      break;
    case CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE:
      message = "Invalid operation. Invalid Ed25519 private key size";
      break;
    case CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE:
      message = "Invalid operation. Invalid BIP32 public key size";
      break;
    case CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE:
      message = "Invalid operation. Invalid BIP32 private key size";
      break;
    case CARDANO_ERROR_INVALID_ARGUMENT:
      message = "Invalid operation. Invalid argument";
      break;
    case CARDANO_ERROR_INVALID_URL:
      message = "Invalid argument. Invalid URL";
      break;
    case CARDANO_ELEMENT_NOT_FOUND:
      message = "Invalid operation. Element not found";
      break;
    case CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX:
      message = "Invalid operation. Invalid BIP32 derivation index";
      break;
    case CARDANO_ERROR_ENCODING:
      message = "Invalid operation. Encoding failure";
      break;
    case CARDANO_ERROR_DECODING:
      message = "Invalid operation. Decoding failure";
      break;
    case CARDANO_ERROR_CHECKSUM_MISMATCH:
      message = "Invalid operation. Checksum mismatch";
      break;
    case CARDANO_ERROR_INVALID_JSON:
      message = "Invalid operation. Invalid JSON";
      break;
    case CARDANO_INTEGER_OVERFLOW:
      message = "Invalid operation. Integer overflow";
      break;
    case CARDANO_INTEGER_UNDERFLOW:
      message = "Invalid operation. Integer underflow";
      break;
    case CARDANO_CONVERSION_ERROR:
      message = "Invalid operation. Conversion error";
      break;
    case CARDANO_ERROR_LOSS_OF_PRECISION:
      message = "Invalid operation. Loss of precision";
      break;
    case CARDANO_ERROR_UNEXPECTED_CBOR_TYPE:
      message = "Invalid operation. Unexpected CBOR type";
      break;
    case CARDANO_ERROR_INVALID_CBOR_VALUE:
      message = "Invalid operation. Invalid CBOR value";
      break;
    case CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE:
      message = "Invalid operation. Invalid CBOR array size";
      break;
    case CARDANO_ERROR_INVALID_CBOR_MAP_SIZE:
      message = "Invalid operation. Invalid CBOR map size";
      break;
    case CARDANO_INVALID_ADDRESS_TYPE:
      message = "Invalid operation. Invalid address type";
      break;
    case CARDANO_INVALID_ADDRESS_FORMAT:
      message = "Invalid operation. Invalid address format";
      break;
    case CARDANO_INVALID_CREDENTIAL_TYPE:
      message = "Invalid operation. Invalid credential type";
      break;
    case CARDANO_ERROR_INVALID_PLUTUS_DATA_CONVERSION:
      message = "Invalid operation. Invalid Plutus data conversion";
      break;
    case CARDANO_INVALID_DATUM_TYPE:
      message = "Invalid operation. Invalid datum type";
      break;
    case CARDANO_INVALID_SCRIPT_LANGUAGE:
      message = "Invalid operation. Invalid script language";
      break;
    case CARDANO_INVALID_NATIVE_SCRIPT_TYPE:
      message = "Invalid operation. Invalid native script type";
      break;
    case CARDANO_INVALID_PLUTUS_COST_MODEL:
      message = "Invalid operation. Invalid Plutus cost model";
      break;
    case CARDANO_INDEX_OUT_OF_BOUNDS:
      message = "Invalid operation. Index out of bounds";
      break;
    case CARDANO_INVALID_CERTIFICATE_TYPE:
      message = "Invalid operation. Invalid certificate type";
      break;
    case CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY:
      message = "Invalid operation. Duplicated CBOR map key";
      break;
    case CARDANO_ERROR_INVALID_CBOR_MAP_KEY:
      message = "Invalid operation. Invalid CBOR map key";
      break;
    case CARDANO_INVALID_PROCEDURE_PROPOSAL_TYPE:
      message = "Invalid operation. Invalid procedure proposal type";
      break;
    case CARDANO_ERROR_INVALID_METADATUM_CONVERSION:
      message = "Invalid operation. Invalid metadatum conversion";
      break;
    case CARDANO_ERROR_INVALID_METADATUM_TEXT_STRING_SIZE:
      message = "Invalid operation. Invalid metadatum text string size, must be less than 64 bytes";
      break;
    case CARDANO_ERROR_INVALID_METADATUM_BOUNDED_BYTES_SIZE:
      message = "Invalid operation. Invalid metadatum bounded bytes size, must be less than 64 bytes";
      break;
    default:
      message = "Unknown error code";
      break;
  }

  return message;
}
