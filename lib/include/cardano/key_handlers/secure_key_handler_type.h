/**
 * \file secure_key_handler_type.h
 *
 * \author angel.castillo
 * \date   Oct 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Secure key handler type.
 */
typedef enum
{
  /**
   * \brief Ed25519 key handler. This key handler doesnt support BIP32 derivation scheme and must use
   * the secure_key_handler_ed25519_* family of functions.
   */
  CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519,

  /**
   * \brief BIP32 key handler. This key handler supports the BIP32 derivation scheme and must use
   * the secure_key_handler_bip32_* family of functions.
   */
  CARDANO_SECURE_KEY_HANDLER_TYPE_BIP32,
} cardano_secure_key_handler_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_TYPE_H
