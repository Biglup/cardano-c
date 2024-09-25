/**
 * \file byron_address_type.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the types of spending data associated with Byron addresses.
 *
 * This enumeration defines the type of data that can be associated with a Byron address. Each type corresponds to a different method of controlling the spending of funds:
 */
typedef enum
{
  /**
   * \brief Indicates that the address uses a public key to control spending. The payload for
   * this address type is a public key.
   */
  CARDANO_BYRON_ADDRESS_TYPE_PUBKEY = 0,

  /**
   * \brief Indicates that the address uses a script to control spending. The payload for
   * this address type includes the script itself and possibly its version, depending on the script type.
   */
  CARDANO_BYRON_ADDRESS_TYPE_SCRIPT = 1,

  /**
   * \brief Indicates that the address uses a redeem public key. Redeem addresses are a
   * special type of address mainly used during the initial distribution phase of ADA.
   * The payload is a redeem public key.
   */
  CARDANO_BYRON_ADDRESS_TYPE_REDEEM = 2
} cardano_byron_address_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_TYPE_H