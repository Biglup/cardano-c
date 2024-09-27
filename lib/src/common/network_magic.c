/**
 * \file network_magic.c
 *
 * \author angel.castillo
 * \date   Sep 28, 2024
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

#include <cardano/common/network_magic.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_network_magic_to_string(const cardano_network_magic_t network_magic)
{
  const char* message;

  switch (network_magic)
  {
    case CARDANO_NETWORK_MAGIC_PREPROD:
      message = "preprod";
      break;
    case CARDANO_NETWORK_MAGIC_PREVIEW:
      message = "preview";
      break;
    case CARDANO_NETWORK_MAGIC_SANCHONET:
      message = "sanchonet";
      break;
    case CARDANO_NETWORK_MAGIC_MAINNET:
      message = "mainnet";
      break;
    default:
      message = "unknown";
      break;
  }

  return message;
}
