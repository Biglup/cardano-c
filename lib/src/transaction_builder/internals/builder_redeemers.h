/**
 * \file builder_redeemers.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_REDEEMERS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_REDEEMERS_H

/* INCLUDES ******************************************************************/

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/witness_set.h>

#include "blake2b_hash_to_redeemer_map.h"
#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Adds a redeemer for the given tag to a witness set and records it in the matching redeemer map.
 *
 * This function ensures the witness set has a redeemer list, creating and attaching an empty one when it
 * is missing. When \p data is not NULL it creates a redeemer with the given tag, a zero index and zero
 * execution units, inserts it keyed by \p hash into the state redeemer map that matches the tag (mint,
 * reward or vote) and appends it to the witness set redeemer list unless the key was already present in
 * the map.
 *
 * \param[in,out] witness_set A pointer to the \ref cardano_witness_set_t that receives the redeemer. This
 *                            parameter must not be NULL.
 * \param[in] hash A pointer to the \ref cardano_blake2b_hash_t used as the key in the redeemer map. This
 *                 parameter must not be NULL.
 * \param[in] data A pointer to the \ref cardano_plutus_data_t with the redeemer payload, or NULL to only
 *                 ensure the witness set redeemer list exists.
 * \param[in] tag The \ref cardano_redeemer_tag_t identifying the redeemer purpose. Only the mint, reward
 *                and vote tags are accepted.
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t holding the redeemer maps. This
 *                      parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the redeemer was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_add_redeemer(
  cardano_witness_set_t*   witness_set,
  cardano_blake2b_hash_t*  hash,
  cardano_plutus_data_t*   data,
  cardano_redeemer_tag_t   tag,
  cardano_builder_state_t* state,
  const char**             error_message);

/**
 * \brief Creates the placeholder payload used by deferred redeemers until they are resolved.
 *
 * The placeholder is an empty constructor (constr 0 []), so the transaction can be sized and
 * hashed before the first balancing iteration replaces it with the real payload.
 *
 * \param[out] data A pointer to a pointer to a \ref cardano_plutus_data_t. On success it points to the
 *                  newly created placeholder and the caller must release it with
 *                  \ref cardano_plutus_data_unref. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
cardano_error_t
cardano_builder_create_placeholder_plutus_data(cardano_plutus_data_t** data);

/**
 * \brief Registers a deferred callback for the redeemer stored under the given hash in one of
 * the builder state redeemer maps.
 *
 * This function looks up the redeemer keyed by \p hash in \p map and, when found, adds it to the
 * state deferred redeemer list together with the callback and its context so the payload can be
 * resolved during balancing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t holding the deferred redeemer
 *                      list. This parameter must not be NULL.
 * \param[in] map A pointer to the \ref cardano_blake2b_hash_to_redeemer_map_t holding the redeemer
 *                (mint, withdrawal or vote map). This parameter must not be NULL.
 * \param[in] hash A pointer to the \ref cardano_blake2b_hash_t key of the redeemer in the map. This
 *                 parameter must not be NULL.
 * \param[in] callback The deferred redeemer callback invoked during balancing.
 * \param[in] user_context The opaque pointer forwarded to the callback.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the deferred redeemer was registered, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_register_deferred_from_map(
  cardano_builder_state_t*                state,
  cardano_blake2b_hash_to_redeemer_map_t* map,
  cardano_blake2b_hash_t*                 hash,
  cardano_deferred_redeemer_fn_t          callback,
  void*                                   user_context,
  const char**                            error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_REDEEMERS_H
