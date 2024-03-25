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
    case CARDANO_ERROR_LOSS_OF_PRECISION:
      message = "Invalid conversion. Loss of precision";
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
    case CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX:
      message = "Invalid operation. Invalid BIP32 derivation index";
      break;
    default:
      message = "Unknown error code";
      break;
  }

  return message;
}
