/**
 * \file builder_withdrawals.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_WITHDRAWALS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_WITHDRAWALS_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Records a reward withdrawal in the transaction.
 *
 * This function inserts the withdrawal for \p address with the given amount into the withdrawal map
 * of the transaction body, creating the map when it is missing. When \p redeemer is not NULL it is
 * attached as a reward redeemer keyed by the sortable id of the reward credential in the state
 * withdrawal redeemer map. When \p redeemer is NULL and the reward credential is a script credential
 * the sortable id is still recorded in the map with a NULL redeemer so redeemer indices can be
 * computed; script credentials are processed by the node before key hash credentials regardless of
 * the canonical sorting, so key hash credentials are skipped.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_reward_address_t to withdraw from. This parameter
 *                    must not be NULL.
 * \param[in] amount The amount of lovelace to withdraw. It must not be negative.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as reward redeemer payload,
 *                     or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the withdrawal was recorded, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_withdraw_rewards(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* address,
  int64_t                   amount,
  cardano_plutus_data_t*    redeemer,
  const char**              error_message);

/**
 * \brief Records a reward withdrawal given the reward address as a bech32 string.
 *
 * This function parses the reward address and delegates to \ref cardano_builder_withdraw_rewards.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] amount The amount of lovelace to withdraw. It must not be negative.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as reward redeemer payload,
 *                     or NULL when the credential does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the withdrawal was recorded, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_withdraw_rewards_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  int64_t                  amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message);

/**
 * \brief Records a reward withdrawal whose redeemer payload is resolved during balancing.
 *
 * This function records the withdrawal with an empty placeholder redeemer and registers the callback
 * in the state deferred redeemer list keyed by the sortable id of the reward credential. The callback
 * is invoked while the transaction is balanced to produce the final redeemer payload.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_reward_address_t to withdraw from. This parameter
 *                    must not be NULL.
 * \param[in] amount The amount of lovelace to withdraw. It must not be negative.
 * \param[in] callback The deferred redeemer callback invoked during balancing. This parameter must
 *                     not be NULL.
 * \param[in] user_context The opaque pointer forwarded to the callback.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the withdrawal and callback were registered, or an appropriate
 *         error code indicating the failure reason.
 */
cardano_error_t
cardano_builder_withdraw_rewards_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_reward_address_t*      address,
  int64_t                        amount,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  const char**                   error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_WITHDRAWALS_H
