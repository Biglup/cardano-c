/**
 * \file address_type.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates types of addresses used within the Cardano blockchain in the Shelley era and beyond.
 *
 * Shelley introduces several different types of addresses, each serving distinct purposes and supporting different functionalities.
 * In addition to these new address types, Shelley continues to support Byron-era bootstrap addresses.
 */
typedef enum
{
  /** Base addresses with both payment and stake credentials as key hashes.
   * \li bits 7-6 : 00
   * \li bit  5   : stake cred is keyhash
   * \li bit  4   : payment cred is keyhash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY = 0b0000,

  /** Base addresses with payment credentials as script hash and stake credentials as key hash.
   * \li bits 7-6 : 00
   * \li bit  5   : stake cred is keyhash
   * \li bit  4   : payment cred is scripthash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY = 0b0001,

  /** Base addresses with payment credentials as key hash and stake credentials as script hash.
   * \li bits 7-6 : 00
   * \li bit  5   : stake cred is scripthash
   * \li bit  4   : payment cred is keyhash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT = 0b0010,

  /** Base addresses with both payment and stake credentials as script hashes.
   * \li bits 7-6 : 00
   * \li bit  5   : stake cred is scripthash
   * \li bit  4   : payment cred is scripthash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT = 0b0011,

  /** Pointer addresses with payment credential as keyhash.
   * \li bits 7-5 : 010
   * \li bit  4   : payment cred is keyhash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_POINTER_KEY = 0b0100,

  /** Pointer addresses with payment credential as scripthash.
   * \li bits 7-5 : 010
   * \li bit  4   : payment cred is scripthash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_POINTER_SCRIPT = 0b0101,

  /** Enterprise addresses with payment credential as keyhash.
   * \li bits 7-5 : 010
   * \li bit  4   : payment cred is keyhash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY = 0b0110,

  /** Enterprise addresses with payment credential as scripthash.
   * \li bits 7-5 : 010
   * \li bit  4   : payment cred is scripthash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT = 0b0111,

  /** Byron-era bootstrap addresses, continuing support for backward compatibility.
   * \li bits 7-4 : 1000
   * \li bits 3-0 : unrelated data (no network ID in Byron addresses)
   */
  CARDANO_ADDRESS_TYPE_BYRON = 0b1000,

  /** Reward account addresses with credential as keyhash.
   * \li bits 7-5 : 111
   * \li bit  4   : credential is keyhash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_REWARD_KEY = 0b1110,

  /** Reward account addresses with credential as scripthash.
   * \li bits 7-5 : 111
   * \li bit  4   : credential is scripthash
   * \li bits 3-0 : network id
   */
  CARDANO_ADDRESS_TYPE_REWARD_SCRIPT = 0b1111

  // 1001-1101 are reserved for future formats
} cardano_address_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_TYPE_H