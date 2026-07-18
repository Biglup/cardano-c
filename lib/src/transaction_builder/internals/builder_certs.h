/**
 * \file builder_certs.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_CERTS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_CERTS_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/certs/certificate.h>
#include <cardano/common/anchor.h>
#include <cardano/common/drep.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Appends a certificate to the transaction.
 *
 * This function adds \p certificate to the certificate set of the transaction body, creating the
 * set when it is missing. When \p redeemer is not NULL a certifying redeemer with the index of the
 * appended certificate and zero execution units is created and appended to the witness set redeemer
 * list, creating the list when it is missing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] certificate A pointer to the \ref cardano_certificate_t to add. This parameter must not
 *                        be NULL.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the certificate does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_add_certificate(
  cardano_builder_state_t* state,
  cardano_certificate_t*   certificate,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a stake registration certificate for a reward address.
 *
 * This function creates a registration certificate for the credential of \p address using the key
 * deposit from the state protocol parameters and delegates to \ref cardano_builder_add_certificate.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_reward_address_t to register. This parameter must
 *                    not be NULL.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_register_reward_address(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message);

/**
 * \brief Adds a stake registration certificate given the reward address as a bech32 string.
 *
 * This function parses the reward address and delegates to
 * \ref cardano_builder_register_reward_address.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_register_reward_address_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a stake unregistration certificate for a reward address.
 *
 * This function creates an unregistration certificate for the credential of \p address using the
 * key deposit from the state protocol parameters and delegates to
 * \ref cardano_builder_add_certificate.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_reward_address_t to deregister. This parameter
 *                    must not be NULL.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_deregister_reward_address(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message);

/**
 * \brief Adds a stake unregistration certificate given the reward address as a bech32 string.
 *
 * This function parses the reward address and delegates to
 * \ref cardano_builder_deregister_reward_address.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_deregister_reward_address_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a stake delegation certificate for a reward address and a pool.
 *
 * This function creates a stake delegation certificate delegating the credential of \p address to
 * the pool identified by \p pool_id and delegates to \ref cardano_builder_add_certificate.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_reward_address_t whose stake is delegated. This
 *                    parameter must not be NULL.
 * \param[in] pool_id A pointer to the \ref cardano_blake2b_hash_t with the pool key hash. This
 *                    parameter must not be NULL.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_delegate_stake(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_blake2b_hash_t*   pool_id,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message);

/**
 * \brief Adds a stake delegation certificate given the reward address and pool id as bech32 strings.
 *
 * This function decodes the pool id, requiring the pool prefix, parses the reward address and
 * delegates to \ref cardano_builder_delegate_stake.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] pool_id A pointer to the bech32 string with the pool id.
 * \param[in] pool_id_size The size of the pool id string in bytes.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_delegate_stake_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  const char*              pool_id,
  size_t                   pool_id_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a vote delegation certificate for a reward address and a DRep.
 *
 * This function creates a vote delegation certificate delegating the voting power of the credential
 * of \p address to \p drep and delegates to \ref cardano_builder_add_certificate.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_reward_address_t whose voting power is delegated.
 *                    This parameter must not be NULL.
 * \param[in] drep A pointer to the \ref cardano_drep_t receiving the voting power. This parameter
 *                 must not be NULL.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_delegate_voting_power(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  cardano_drep_t*           drep,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message);

/**
 * \brief Adds a vote delegation certificate given the reward address and DRep id as strings.
 *
 * This function parses the reward address and the DRep id and delegates to
 * \ref cardano_builder_delegate_voting_power.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] drep_id A pointer to the string with the DRep id.
 * \param[in] drep_id_size The size of the DRep id string in bytes.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_delegate_voting_power_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  const char*              drep_id,
  size_t                   drep_id_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a DRep registration certificate.
 *
 * This function creates a DRep registration certificate for the credential of \p drep using the
 * DRep deposit from the state protocol parameters and the optional \p anchor, and delegates to
 * \ref cardano_builder_add_certificate.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] drep A pointer to the \ref cardano_drep_t to register. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the DRep metadata, or NULL when the
 *                   DRep has none.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_register_drep(
  cardano_builder_state_t* state,
  cardano_drep_t*          drep,
  cardano_anchor_t*        anchor,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a DRep registration certificate given the DRep id and metadata as strings.
 *
 * This function creates the metadata anchor from the url and hash strings, parses the DRep id and
 * delegates to \ref cardano_builder_register_drep.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] drep_id A pointer to the string with the DRep id.
 * \param[in] drep_id_size The size of the DRep id string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata url.
 * \param[in] metadata_url_size The size of the metadata url string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hexadecimal string with the metadata hash.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_register_drep_ex(
  cardano_builder_state_t* state,
  const char*              drep_id,
  size_t                   drep_id_size,
  const char*              metadata_url,
  size_t                   metadata_url_size,
  const char*              metadata_hash_hex,
  size_t                   metadata_hash_hex_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a DRep update certificate.
 *
 * This function creates a DRep update certificate for the credential of \p drep carrying the
 * optional \p anchor and delegates to \ref cardano_builder_add_certificate.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] drep A pointer to the \ref cardano_drep_t to update. This parameter must not be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t with the DRep metadata, or NULL when the
 *                   DRep has none.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_update_drep(
  cardano_builder_state_t* state,
  cardano_drep_t*          drep,
  cardano_anchor_t*        anchor,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a DRep update certificate given the DRep id and metadata as strings.
 *
 * This function creates the metadata anchor from the url and hash strings, parses the DRep id and
 * delegates to \ref cardano_builder_update_drep.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] drep_id A pointer to the string with the DRep id.
 * \param[in] drep_id_size The size of the DRep id string in bytes.
 * \param[in] metadata_url A pointer to the string with the metadata url.
 * \param[in] metadata_url_size The size of the metadata url string in bytes.
 * \param[in] metadata_hash_hex A pointer to the hexadecimal string with the metadata hash.
 * \param[in] metadata_hash_hex_size The size of the metadata hash string in bytes.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_update_drep_ex(
  cardano_builder_state_t* state,
  const char*              drep_id,
  size_t                   drep_id_size,
  const char*              metadata_url,
  size_t                   metadata_url_size,
  const char*              metadata_hash_hex,
  size_t                   metadata_hash_hex_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a DRep deregistration certificate.
 *
 * This function creates a DRep deregistration certificate for the credential of \p drep using the
 * DRep deposit from the state protocol parameters and delegates to
 * \ref cardano_builder_add_certificate.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] drep A pointer to the \ref cardano_drep_t to deregister. This parameter must not be
 *                 NULL.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_deregister_drep(
  cardano_builder_state_t* state,
  cardano_drep_t*          drep,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Adds a DRep deregistration certificate given the DRep id as a string.
 *
 * This function parses the DRep id and delegates to \ref cardano_builder_deregister_drep.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] drep_id A pointer to the string with the DRep id.
 * \param[in] drep_id_size The size of the DRep id string in bytes.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as certifying redeemer
 *                     payload, or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate was added, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_deregister_drep_ex(
  cardano_builder_state_t* state,
  const char*              drep_id,
  size_t                   drep_id_size,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Appends a certificate whose redeemer payload is resolved during balancing.
 *
 * This function appends the certificate with an empty placeholder redeemer, finds the certifying
 * redeemer created for it in the witness set redeemer list and registers the callback for it in the
 * state deferred redeemer list. The callback is invoked while the transaction is balanced to produce
 * the final redeemer payload.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] certificate A pointer to the \ref cardano_certificate_t to add. This parameter must not
 *                        be NULL.
 * \param[in] callback The deferred redeemer callback invoked during balancing. This parameter must
 *                     not be NULL.
 * \param[in] user_context The opaque pointer forwarded to the callback.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the certificate and callback were registered, or an appropriate
 *         error code indicating the failure reason.
 */
cardano_error_t
cardano_builder_add_certificate_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_certificate_t*         certificate,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  const char**                   error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_CERTS_H
