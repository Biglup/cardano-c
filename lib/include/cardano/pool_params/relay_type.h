/**
 * \file relay_type.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RELAY_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RELAY_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the types of relays used in the Cardano network.
 *
 * This enumeration defines the different types of relay nodes that can be configured in the Cardano network.
 * Each type represents a different method of connecting to the network.
 */
typedef enum
{
  /**
   * \brief Relay connects to a single host using an IP address and a port number. This is the most direct way of specifying
   * a relay and does not depend on DNS resolution.
   */
  CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS = 0,

  /**
   * \brief Relay connects using a DNS name and a port number. This type allows the relay's IP address to be resolved
   * dynamically, which can provide resilience in environments where IP addresses may change.
   */
  CARDANO_RELAY_TYPE_SINGLE_HOST_NAME = 1,

  /**
   * \brief Relay uses a multi-host name via a DNS SRV record to resolve multiple potential IP addresses and ports.
   * This type is used for more complex network setups where load balancing across multiple servers is required.
   */
  CARDANO_RELAY_TYPE_MULTI_HOST_NAME = 2
} cardano_relay_type_t;

/**
 * \brief Converts relay types to their human readable form.
 *
 * \param[in] type The relay type to get the string representation for.
 * \return Human readable form of the given relay type.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_relay_type_to_string(cardano_relay_type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RELAY_TYPE_H