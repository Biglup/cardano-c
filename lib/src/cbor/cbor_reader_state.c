/**
 * \file cbor_reader_state.c
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

#include <cardano/cbor/cbor_reader_state.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_cbor_reader_state_to_string(const cardano_cbor_reader_state_t reader_state)
{
  const char* message;

  switch (reader_state)
  {
    case CARDANO_CBOR_READER_STATE_UNDEFINED:
      message = "Reader State: Undefined";
      break;
    case CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER:
      message = "Reader State: Unsigned Integer";
      break;
    case CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER:
      message = "Reader State: Negative Integer";
      break;
    case CARDANO_CBOR_READER_STATE_BYTESTRING:
      message = "Reader State: Byte String";
      break;
    case CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING:
      message = "Reader State: Start Indefinite Length Byte String";
      break;
    case CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_BYTESTRING:
      message = "Reader State: End Indefinite Length Byte String";
      break;
    case CARDANO_CBOR_READER_STATE_TEXTSTRING:
      message = "Reader State: Text String";
      break;
    case CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING:
      message = "Reader State: Start Indefinite Length Text String";
      break;
    case CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_TEXTSTRING:
      message = "Reader State: End Indefinite Length Text String";
      break;
    case CARDANO_CBOR_READER_STATE_START_ARRAY:
      message = "Reader State: Start Array";
      break;
    case CARDANO_CBOR_READER_STATE_END_ARRAY:
      message = "Reader State: End Array";
      break;
    case CARDANO_CBOR_READER_STATE_START_MAP:
      message = "Reader State: Start Map";
      break;
    case CARDANO_CBOR_READER_STATE_END_MAP:
      message = "Reader State: End Map";
      break;
    case CARDANO_CBOR_READER_STATE_TAG:
      message = "Reader State: Tag";
      break;
    case CARDANO_CBOR_READER_STATE_SIMPLE_VALUE:
      message = "Reader State: Simple Value";
      break;
    case CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT:
      message = "Reader State: Half-Precision Float";
      break;
    case CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT:
      message = "Reader State: Single-Precision Float";
      break;
    case CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT:
      message = "Reader State: Double-Precision Float";
      break;
    case CARDANO_CBOR_READER_STATE_NULL:
      message = "Reader State: Null";
      break;
    case CARDANO_CBOR_READER_STATE_BOOLEAN:
      message = "Reader State: Boolean";
      break;
    case CARDANO_CBOR_READER_STATE_FINISHED:
      message = "Reader State: Finished";
      break;
    default:
      message = "Reader State: Unknown";
      break;
  }

  return message;
}
