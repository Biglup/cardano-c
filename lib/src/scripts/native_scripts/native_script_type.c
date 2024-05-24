/**
 * \file native_script_type.c
 *
 * \author angel.castillo
 * \date   Jun 03, 2024
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

#include <cardano/scripts/native_scripts/native_script_type.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_native_script_type_to_string(const cardano_native_script_type_t native_script_type)
{
  const char* message;

  switch (native_script_type)
  {
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY:
      message = "Native Script Type: Require Pubkey";
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF:
      message = "Native Script Type: Require All Of";
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF:
      message = "Native Script Type: Require Any Of";
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K:
      message = "Native Script Type: Require N Of K";
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE:
      message = "Native Script Type: Invalid Before";
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER:
      message = "Native Script Type: Invalid After";
      break;
    default:
      message = "Native Script Type: Unknown";
      break;
  }

  return message;
}
