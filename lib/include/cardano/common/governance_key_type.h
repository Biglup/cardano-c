/**
 * \file governance_key_type.h
 *
 * \author angel.castillo
 * \date   Nov 20, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_KEY_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_KEY_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the types of governance keys.
 */
typedef enum
{
  /**
   * \brief CIP-1852’s constitutional committee hot verification signing key
   */
  CARDANO_GOVERNANCE_KEY_TYPE_CC_HOT = 0,

  /**
   * \brief Constitutional committee cold verification key hash (cold credential).
   */
  CARDANO_GOVERNANCE_KEY_TYPE_CC_COLD = 1,

  /**
   * \brief Constitutional committee hot verification key hash (hot credential).
   */
  CARDANO_GOVERNANCE_KEY_TYPE_DREP = 2
} cardano_governance_key_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_KEY_TYPE_H