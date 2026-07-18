/**
 * \file builder_witnesses.h
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_WITNESSES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_WITNESSES_H

/* INCLUDES ******************************************************************/

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/scripts/script.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Sets the number of additional signatures accounted for when computing fees.
 *
 * This function stores \p count in the state so that balancing reserves fee room for that many
 * signatures beyond the ones the builder can infer from the transaction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] count The number of additional signatures to account for.
 *
 * \return \ref CARDANO_SUCCESS.
 */
cardano_error_t
cardano_builder_pad_signer_count(
  cardano_builder_state_t* state,
  size_t                   count);

/**
 * \brief Adds a required signer to the transaction.
 *
 * This function adds the key hash credential of \p pub_key_hash to the guard set of the transaction
 * body, creating the set when it is missing. Adding a signer that is already present leaves the
 * transaction unchanged and succeeds.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] pub_key_hash A pointer to the \ref cardano_blake2b_hash_t with the public key hash of
 *                         the signer. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the signer was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_add_signer(
  cardano_builder_state_t* state,
  cardano_blake2b_hash_t*  pub_key_hash,
  const char**             error_message);

/**
 * \brief Adds a required signer given the public key hash as a hexadecimal string.
 *
 * This function parses the public key hash and delegates to \ref cardano_builder_add_signer.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] pub_key_hash A pointer to the hexadecimal string with the public key hash.
 * \param[in] hash_size The size of the public key hash string in bytes.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the signer was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_add_signer_ex(
  cardano_builder_state_t* state,
  const char*              pub_key_hash,
  size_t                   hash_size,
  const char**             error_message);

/**
 * \brief Adds a datum to the witness set.
 *
 * This function appends \p datum to the plutus data set of the witness set, creating the set when it
 * is missing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] datum A pointer to the \ref cardano_plutus_data_t to add. This parameter must not be
 *                  NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the datum was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_add_datum(
  cardano_builder_state_t* state,
  cardano_plutus_data_t*   datum,
  const char**             error_message);

/**
 * \brief Adds a script to the witness set.
 *
 * This function adds \p script to the witness set of the transaction under construction and flags
 * the Plutus language version of the script in the state when it is a Plutus script.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] script A pointer to the \ref cardano_script_t to add. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the script was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_add_script(
  cardano_builder_state_t* state,
  cardano_script_t*        script,
  const char**             error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_WITNESSES_H
