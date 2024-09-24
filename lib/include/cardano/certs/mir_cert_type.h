/**
 * \file mir_cert_type.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the types of MIR certificates.
 *
 * This enumeration defines the possible types of Move Instantaneous Rewards (MIR) certificates in the Cardano system.
 */
typedef enum
{
  /**
   * \brief This MIR certificate moves instantaneous rewards funds between accounting pots.
   */
  CARDANO_MIR_CERT_TYPE_TO_POT = 0,

  /**
   * \brief This MIR certificate transfers funds to the given set of reward accounts.
   */
  CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS = 1
} cardano_mir_cert_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_TYPE_H