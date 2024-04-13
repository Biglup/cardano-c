/**
 * \file cbor_simple_value.c
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

#include <cardano/cbor/cbor_simple_value.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_cbor_simple_value_to_string(const cardano_cbor_simple_value_t simple_value)
{
  const char* message;

  switch (simple_value)
  {
    case CARDANO_CBOR_SIMPLE_VALUE_FALSE:
      message = "Simple Value: False";
      break;
    case CARDANO_CBOR_SIMPLE_VALUE_TRUE:
      message = "Simple Value: True";
      break;
    case CARDANO_CBOR_SIMPLE_VALUE_NULL:
      message = "Simple Value: Null";
      break;
    case CARDANO_CBOR_SIMPLE_VALUE_UNDEFINED:
      message = "Simple Value: Undefined";
      break;
    default:
      message = "Simple Value: Unknown";
      break;
  }

  return message;
}
