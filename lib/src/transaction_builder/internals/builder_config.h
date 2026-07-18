/**
 * \file builder_config.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_CONFIG_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_CONFIG_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/common/network_id.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Sets the coin selection strategy used during balancing.
 *
 * This function takes a reference on \p coin_selector and stores it in the state, releasing the
 * previously configured coin selector.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] coin_selector A pointer to the \ref cardano_coin_selector_t to use. This parameter must
 *                          not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the coin selector was set, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_set_coin_selector(
  cardano_builder_state_t* state,
  cardano_coin_selector_t* coin_selector,
  const char**             error_message);

/**
 * \brief Sets the evaluator used to compute execution units for Plutus scripts.
 *
 * This function takes a reference on \p tx_evaluator and stores it in the state, releasing the
 * previously configured transaction evaluator.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] tx_evaluator A pointer to the \ref cardano_tx_evaluator_t to use. This parameter must
 *                         not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the transaction evaluator was set, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_set_tx_evaluator(
  cardano_builder_state_t* state,
  cardano_tx_evaluator_t*  tx_evaluator,
  const char**             error_message);

/**
 * \brief Sets the network identifier in the transaction body.
 *
 * This function writes \p network_id into the body of the transaction under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] network_id The \ref cardano_network_id_t to set.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the network identifier was set, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_set_network_id(
  cardano_builder_state_t* state,
  cardano_network_id_t     network_id,
  const char**             error_message);

/**
 * \brief Sets the address that receives any change produced while balancing.
 *
 * This function takes a reference on \p change_address and stores it in the state, releasing the
 * previously configured change address.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] change_address A pointer to the \ref cardano_address_t to use. This parameter must not
 *                           be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the change address was set, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_set_change_address(
  cardano_builder_state_t* state,
  cardano_address_t*       change_address,
  const char**             error_message);

/**
 * \brief Sets the change address given its string representation.
 *
 * This function parses \p change_address and delegates to \ref cardano_builder_set_change_address.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] change_address A pointer to the string with the change address.
 * \param[in] address_size The size of the change address string in bytes.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the change address was set, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_set_change_address_ex(
  cardano_builder_state_t* state,
  const char*              change_address,
  size_t                   address_size,
  const char**             error_message);

/**
 * \brief Sets the address that receives any collateral change output.
 *
 * This function takes a reference on \p collateral_change_address and stores it in the state,
 * releasing the previously configured collateral change address.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] collateral_change_address A pointer to the \ref cardano_address_t to use. This parameter
 *                                      must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the collateral change address was set, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_set_collateral_change_address(
  cardano_builder_state_t* state,
  cardano_address_t*       collateral_change_address,
  const char**             error_message);

/**
 * \brief Sets the collateral change address given its string representation.
 *
 * This function parses \p collateral_change_address and delegates to
 * \ref cardano_builder_set_collateral_change_address.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] collateral_change_address A pointer to the string with the collateral change address.
 * \param[in] address_size The size of the collateral change address string in bytes.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the collateral change address was set, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_set_collateral_change_address_ex(
  cardano_builder_state_t* state,
  const char*              collateral_change_address,
  size_t                   address_size,
  const char**             error_message);

/**
 * \brief Sets the fee in the transaction body.
 *
 * This function writes \p minimum_fee as the fee of the transaction under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] minimum_fee The fee in lovelace to set.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the fee was set, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_set_minimum_fee(
  cardano_builder_state_t* state,
  uint64_t                 minimum_fee,
  const char**             error_message);

/**
 * \brief Sets the treasury donation in the transaction body.
 *
 * This function writes \p donation as the donation of the transaction under construction, removing
 * any previously set donation when \p donation is zero.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] donation The donation in lovelace to set.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the donation was set, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_set_donation(
  cardano_builder_state_t* state,
  uint64_t                 donation,
  const char**             error_message);

/**
 * \brief Sets the UTXO set available to the coin selector for input selection.
 *
 * This function takes a reference on \p utxos and stores it in the state, releasing the previously
 * configured UTXO list.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t to use. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the UTXO list was set, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_set_utxos(
  cardano_builder_state_t* state,
  cardano_utxo_list_t*     utxos,
  const char**             error_message);

/**
 * \brief Sets the UTXO set available for collateral selection.
 *
 * This function takes a reference on \p utxos and stores it in the state, releasing the previously
 * configured collateral UTXO list.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t to use. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the collateral UTXO list was set, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_set_collateral_utxos(
  cardano_builder_state_t* state,
  cardano_utxo_list_t*     utxos,
  const char**             error_message);

/**
 * \brief Sets the slot after which the transaction is invalid.
 *
 * This function writes \p slot as the invalid after bound of the transaction under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] slot The slot number to set.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the bound was set, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_set_invalid_after(
  cardano_builder_state_t* state,
  uint64_t                 slot,
  const char**             error_message);

/**
 * \brief Sets the slot after which the transaction is invalid given a Unix time.
 *
 * This function converts \p unix_time to the enclosing slot using the state slot configuration and
 * writes it as the invalid after bound of the transaction under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] unix_time The Unix time in seconds to convert.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the bound was set, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_set_invalid_after_ex(
  cardano_builder_state_t* state,
  uint64_t                 unix_time,
  const char**             error_message);

/**
 * \brief Sets the slot before which the transaction is invalid.
 *
 * This function writes \p slot as the invalid before bound of the transaction under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] slot The slot number to set.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the bound was set, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_set_invalid_before(
  cardano_builder_state_t* state,
  uint64_t                 slot,
  const char**             error_message);

/**
 * \brief Sets the slot before which the transaction is invalid given a Unix time.
 *
 * This function converts \p unix_time to the enclosing slot using the state slot configuration and
 * writes it as the invalid before bound of the transaction under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] unix_time The Unix time in seconds to convert.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the bound was set, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_set_invalid_before_ex(
  cardano_builder_state_t* state,
  uint64_t                 unix_time,
  const char**             error_message);

/**
 * \brief Inserts a metadatum under a tag in the transaction metadata.
 *
 * This function inserts \p metadata under \p tag in the transaction metadata of the auxiliary data,
 * creating the auxiliary data and the metadata map when they are missing, and refreshes the
 * auxiliary data hash in the transaction body.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] tag The metadata label under which the metadatum is inserted.
 * \param[in] metadata A pointer to the \ref cardano_metadatum_t to insert. This parameter must not
 *                     be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was inserted, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_set_metadata(
  cardano_builder_state_t* state,
  uint64_t                 tag,
  cardano_metadatum_t*     metadata,
  const char**             error_message);

/**
 * \brief Inserts a metadatum under a tag given its JSON representation.
 *
 * This function parses \p metadata_json into a metadatum and delegates to
 * \ref cardano_builder_set_metadata.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] tag The metadata label under which the metadatum is inserted.
 * \param[in] metadata_json A pointer to the JSON string with the metadatum.
 * \param[in] json_size The size of the JSON string in bytes.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was inserted, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_set_metadata_ex(
  cardano_builder_state_t* state,
  uint64_t                 tag,
  const char*              metadata_json,
  size_t                   json_size,
  const char**             error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_CONFIG_H
