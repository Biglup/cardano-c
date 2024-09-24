/**
 * \file network_id.h
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_NETWORK_ID_H
#define BIGLUP_LABS_INCLUDE_CARDANO_NETWORK_ID_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Defines the network identifiers used within the Cardano ecosystem.
 *
 * This enumeration is used to specify the network for which a Cardano transaction or operation is intended.
 */
typedef enum
{
  /**
   * \brief Represents the test network (testnet).
   */
  CARDANO_NETWORK_ID_TEST_NET = 0,

  /**
   * \brief Represents the main network (mainnet).
   */
  CARDANO_NETWORK_ID_MAIN_NET = 1
} cardano_network_id_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_NETWORK_ID_H