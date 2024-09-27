/**
 * \file network_magic.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_NETWORK_MAGIC_H
#define BIGLUP_LABS_INCLUDE_CARDANO_NETWORK_MAGIC_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the available Cardano network environments.
 *
 * This enumeration defines the different network environments that can be used
 * with the Cardano provider.
 */
typedef enum
{
  /**
   * \brief The Pre-Production test network.
   *
   * The Pre-Production network is a Cardano testnet used for testing features
   * before they are deployed to the Mainnet. It closely mirrors the Mainnet
   * environment, providing a final testing ground for applications.
   */
  CARDANO_NETWORK_MAGIC_PREPROD = 1,

  /**
   * \brief The Preview test network.
   *
   * The Preview network is a Cardano testnet used for testing upcoming features
   * before they are released to the Pre-Production network. It allows developers
   * to experiment with new functionalities in a controlled environment.
   */
  CARDANO_NETWORK_MAGIC_PREVIEW = 2,

  /**
   * \brief The SanchoNet test network.
   *
   * SanchoNet is the testnet for rolling out governance features for the Cardano blockchain,
   * aligning with the comprehensive CIP-1694 specifications.
   */
  CARDANO_NETWORK_MAGIC_SANCHONET = 4,

  /**
   * \brief The Mainnet network.
   *
   * The Mainnet is the live Cardano network where real transactions occur.
   * Applications interacting with the Mainnet are dealing with actual ADA and
   * other assets. Caution should be exercised to ensure correctness and security.
   */
  CARDANO_NETWORK_MAGIC_MAINNET = 764824073,
} cardano_network_magic_t;

/**
 * \brief Converts network magics to their human readable form.
 *
 * \param[in] magic The network magic to get the string representation for.
 * \return Human readable form of the given network magic.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_network_magic_to_string(cardano_network_magic_t magic);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_NETWORK_MAGIC_H