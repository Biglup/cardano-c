/**
 * \file blake2b_hash_size.h
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_SIZE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_SIZE_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \enum cardano_blake2b_hash_size_t
 * \brief Represents the types of BLAKE2b hash functions distinguished by their output bit length.
 * These hash types are utilized in various contexts throughout Cardano, including but not limited
 * to verification keys, multi-signature scripts, and key derivation processes.
 */
typedef enum
{
  /**
   * \brief BLAKE2b-224 (hash28)
   *
   * Produces a 224-bit output. Primarily used for hashing verification keys and
   * multi-signature scripts.
   */
  CARDANO_BLAKE2B_HASH_SIZE_224 = 28,

  /**
   * \brief BLAKE2b-256 (hash32)
   *
   * Produces a 256-bit output. This is the standard BLAKE2b hash variant used
   * across most of Cardano for a variety of purposes, including transaction
   * IDs, address hashing, and more.
   */
  CARDANO_BLAKE2B_HASH_SIZE_256 = 32,

  /**
   * \brief BLAKE2b-512 (hash64)
   *
   * Produces a 512-bit output. Used specifically for key derivation functions
   * within Cardano.
   */
  CARDANO_BLAKE2B_HASH_SIZE_512 = 64
} cardano_blake2b_hash_size_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_SIZE_H
