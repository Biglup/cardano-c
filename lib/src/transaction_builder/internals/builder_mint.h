/**
 * \file builder_mint.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_MINT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_MINT_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_name.h>
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
 * \brief Records a token mint or burn in the transaction.
 *
 * This function sets the amount for the asset identified by \p policy_id and \p name in the mint
 * field of the transaction body, creating the mint multi asset when it is missing. When \p redeemer
 * is not NULL it is attached as a mint redeemer keyed by the policy in the state mint redeemer map.
 * When \p redeemer is NULL the policy is still recorded in the map with a NULL redeemer so redeemer
 * indices can be computed, and an already recorded policy is tolerated.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] policy_id A pointer to the \ref cardano_blake2b_hash_t with the minting policy hash.
 *                      This parameter must not be NULL.
 * \param[in] name A pointer to the \ref cardano_asset_name_t of the token. This parameter must not
 *                 be NULL.
 * \param[in] amount The amount to mint when positive or burn when negative.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as mint redeemer payload, or
 *                     NULL when the policy does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the mint was recorded, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_mint_token(
  cardano_builder_state_t* state,
  cardano_blake2b_hash_t*  policy_id,
  cardano_asset_name_t*    name,
  int64_t                  amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Records a token mint or burn given the policy hash and asset name as hexadecimal strings.
 *
 * This function parses the policy hash and asset name and delegates to
 * \ref cardano_builder_mint_token.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] policy_id_hex A pointer to the hexadecimal string with the minting policy hash.
 * \param[in] policy_id_size The size of the policy hash string in bytes.
 * \param[in] name_hex A pointer to the hexadecimal string with the asset name.
 * \param[in] name_size The size of the asset name string in bytes.
 * \param[in] amount The amount to mint when positive or burn when negative.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as mint redeemer payload, or
 *                     NULL when the policy does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the mint was recorded, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_mint_token_ex(
  cardano_builder_state_t* state,
  const char*              policy_id_hex,
  size_t                   policy_id_size,
  const char*              name_hex,
  size_t                   name_size,
  int64_t                  amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Records a token mint or burn given a fully formed asset id.
 *
 * This function extracts the policy hash and asset name from \p asset_id and delegates to
 * \ref cardano_builder_mint_token.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] asset_id A pointer to the \ref cardano_asset_id_t identifying the token. This parameter
 *                     must not be NULL.
 * \param[in] amount The amount to mint when positive or burn when negative.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as mint redeemer payload, or
 *                     NULL when the policy does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the mint was recorded, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_mint_token_with_id(
  cardano_builder_state_t* state,
  cardano_asset_id_t*      asset_id,
  int64_t                  amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Records a token mint or burn given an asset id as a hexadecimal string.
 *
 * This function parses the asset id and delegates to \ref cardano_builder_mint_token_with_id.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] asset_id_hex A pointer to the hexadecimal string with the asset id.
 * \param[in] hex_size The size of the asset id string in bytes.
 * \param[in] amount The amount to mint when positive or burn when negative.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as mint redeemer payload, or
 *                     NULL when the policy does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the mint was recorded, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_mint_token_with_id_ex(
  cardano_builder_state_t* state,
  const char*              asset_id_hex,
  size_t                   hex_size,
  int64_t                  amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Records a token mint or burn whose redeemer payload is resolved during balancing.
 *
 * This function records the mint with an empty placeholder redeemer and registers the callback in
 * the state deferred redeemer list. The callback is invoked while the transaction is balanced to
 * produce the final redeemer payload.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] policy_id A pointer to the \ref cardano_blake2b_hash_t with the minting policy hash.
 *                      This parameter must not be NULL.
 * \param[in] name A pointer to the \ref cardano_asset_name_t of the token. This parameter must not
 *                 be NULL.
 * \param[in] amount The amount to mint when positive or burn when negative.
 * \param[in] callback The deferred redeemer callback invoked during balancing. This parameter must
 *                     not be NULL.
 * \param[in] user_context The opaque pointer forwarded to the callback.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the mint and callback were registered, or an appropriate error
 *         code indicating the failure reason.
 */
cardano_error_t
cardano_builder_mint_token_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_blake2b_hash_t*        policy_id,
  cardano_asset_name_t*          name,
  int64_t                        amount,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  const char**                   error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_MINT_H
