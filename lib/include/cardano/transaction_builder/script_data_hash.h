/**
 * \file script_data_hash.h
 *
 * \author angel.castillo
 * \date   Nov 12, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_DATA_HASH_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_DATA_HASH_H

/* INCLUDES ******************************************************************/

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/redeemer_list.h>

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Computes the hash of script data in a transaction, including redeemers, datums, and cost models.
 *
 * This function processes arrays of redeemers and datums, along with specified cost models, to compute a
 * 32-byte hash that represents the script data for a transaction. The data is encoded in CBOR
 * (Concise Binary Object Representation) format and hashed using the Blake2b hashing algorithm.
 *
 * \param[in] costmdls A pointer to \ref cardano_costmdls_t representing the cost models for script execution.
 * \param[in] redeemers A pointer to \ref cardano_redeemer_list_t containing the transaction's redeemers.
 *                      If this parameter is NULL or empty, the function may return undefined behavior.
 * \param[in] datums A pointer to \ref cardano_plutus_data_set_t representing the datums included in the transaction.
 * \param[out] data_hash On successful computation, this will point to a \ref cardano_blake2b_hash_t containing
 *                       the computed 32-byte hash for the script data.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         script data hash was successfully computed, or an appropriate error code if a failure occurred.
 *         If no redeemers are provided, the function may return an undefined result.
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = ...;      // Initialized cost models
 * cardano_redeemer_list_t* redeemers = ...; // List of redeemers
 * cardano_plutus_data_set_t* datums = ...;  // Set of datums
 * cardano_blake2b_hash_t* data_hash = NULL;
 *
 * cardano_error_t result = cardano_compute_script_data_hash(costmdls, redeemers, datums, &data_hash);
 *
 * if (result == CARDANO_SUCCESS && data_hash != NULL)
 * {
 *   // Successfully computed the hash of the script data
 * }
 * else
 * {
 *   printf("Failed to compute the script data hash.\n");
 * }
 *
 * cardano_blake2b_hash_unref(&data_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_compute_script_data_hash(
  cardano_costmdls_t*        costmdls,
  cardano_redeemer_list_t*   redeemers,
  cardano_plutus_data_set_t* datums,
  cardano_blake2b_hash_t**   data_hash);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_DATA_HASH_H