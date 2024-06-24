/**
 * \file relay_type.c
 *
 * \author angel.castillo
 * \date   Jun 26, 2024
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

#include <cardano/pool_params/relay_type.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_relay_type_to_string(const cardano_relay_type_t relay_type)
{
  const char* message;

  switch (relay_type)
  {
    case CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS:
      message = "Relay Type: Single Host Address";
      break;
    case CARDANO_RELAY_TYPE_SINGLE_HOST_NAME:
      message = "Relay Type: Single Host Name";
      break;
    case CARDANO_RELAY_TYPE_MULTI_HOST_NAME:
      message = "Relay Type: Multi Host Name";
      break;
    default:
      message = "Relay Type: Unknown";
      break;
  }

  return message;
}
