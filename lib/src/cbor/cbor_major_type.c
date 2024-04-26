/**
 * \file cbor_major_type.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
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

#include <cardano/cbor/cbor_major_type.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_cbor_major_type_to_string(const cardano_cbor_major_type_t major_type)
{
  const char* message;

  switch (major_type)
  {
    case CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER:
      message = "Major Type: Unsigned Integer";
      break;
    case CARDANO_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER:
      message = "Major Type: Negative Integer";
      break;
    case CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING:
      message = "Major Type: Byte String";
      break;
    case CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING:
      message = "Major Type: UTF-8 String";
      break;
    case CARDANO_CBOR_MAJOR_TYPE_ARRAY:
      message = "Major Type: Array";
      break;
    case CARDANO_CBOR_MAJOR_TYPE_MAP:
      message = "Major Type: Map";
      break;
    case CARDANO_CBOR_MAJOR_TYPE_TAG:
      message = "Major Type: Tag";
      break;
    case CARDANO_CBOR_MAJOR_TYPE_SIMPLE:
      message = "Major Type: Simple";
      break;
    default:
      message = "Major Type: Unknown";
      break;
  }

  return message;
}
