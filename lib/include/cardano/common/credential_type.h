/**
 * \file credential_type.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CREDENTIAL_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CREDENTIAL_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the types of credentials.
 */
typedef enum
{
  /** Credential is a hash of a public key.
   */
  CARDANO_CREDENTIAL_TYPE_KEY_HASH = 0,

  /** Credential is a hash of a script.
   */
  CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH = 1
} cardano_credential_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CREDENTIAL_TYPE_H