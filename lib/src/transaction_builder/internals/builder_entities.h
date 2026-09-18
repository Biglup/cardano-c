/**
 * \file builder_entities.h
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_ENTITIES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_ENTITIES_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/error.h>
#include <cardano/transaction_body/account_balance_interval.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Records a direct deposit in the transaction.
 *
 * A direct deposit pays lovelace straight into a reward account without creating a UTxO. This
 * function adds \p amount to the direct deposit map of the transaction body, creating the map when
 * it is missing. When the reward account already has a direct deposit the amounts are accumulated,
 * keeping the position of the account in the map.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t of the reward account that
 *                           receives the deposit. This parameter must not be NULL.
 * \param[in] amount The amount of lovelace to deposit. It must be greater than zero.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the direct deposit was recorded, \ref CARDANO_ERROR_INTEGER_OVERFLOW
 *         if the accumulated amount of the reward account does not fit in 64 bits, or an appropriate
 *         error code indicating the failure reason. The transaction is left unchanged on failure.
 */
cardano_error_t
cardano_builder_add_direct_deposit(
  cardano_builder_state_t*  state,
  cardano_reward_address_t* reward_address,
  uint64_t                  amount,
  const char**              error_message);

/**
 * \brief Records a direct deposit given the reward address as a bech32 string.
 *
 * This function parses the reward address and delegates to \ref cardano_builder_add_direct_deposit.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] amount The amount of lovelace to deposit. It must be greater than zero.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the direct deposit was recorded, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_add_direct_deposit_ex(
  cardano_builder_state_t* state,
  const char*              reward_address,
  size_t                   address_size,
  uint64_t                 amount,
  const char**             error_message);

/**
 * \brief Records an account balance interval in the transaction.
 *
 * An account balance interval makes the transaction valid only while the balance of the reward
 * account sits inside the interval at the point the transaction is applied. This function stores
 * \p interval in the account balance intervals map of the transaction body, creating the map when it
 * is missing. When the reward account already has an interval it is replaced, keeping the position
 * of the account in the map.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t of the reward account the
 *                           interval constrains. This parameter must not be NULL.
 * \param[in] interval A pointer to the \ref cardano_account_balance_interval_t the balance must sit
 *                     in. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the interval was recorded, or an appropriate error code indicating
 *         the failure reason. The transaction is left unchanged on failure.
 */
cardano_error_t
cardano_builder_add_account_balance_interval(
  cardano_builder_state_t*            state,
  cardano_reward_address_t*           reward_address,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message);

/**
 * \brief Records an account balance interval given the reward address as a bech32 string.
 *
 * This function parses the reward address and delegates to
 * \ref cardano_builder_add_account_balance_interval.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] interval A pointer to the \ref cardano_account_balance_interval_t the balance must sit
 *                     in. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the interval was recorded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_add_account_balance_interval_ex(
  cardano_builder_state_t*            state,
  const char*                         reward_address,
  size_t                              address_size,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message);

/**
 * \brief Records a starting account balance interval in the transaction.
 *
 * A starting account balance interval makes the transaction valid only while the balance the reward
 * account had before any sub transaction was applied sits inside the interval. This function stores
 * \p interval in the starting account balance intervals map of the transaction body, creating the
 * map when it is missing. When the reward account already has an interval it is replaced, keeping
 * the position of the account in the map.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the \ref cardano_reward_address_t of the reward account the
 *                           interval constrains. This parameter must not be NULL.
 * \param[in] interval A pointer to the \ref cardano_account_balance_interval_t the starting balance
 *                     must sit in. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the interval was recorded, or an appropriate error code indicating
 *         the failure reason. The transaction is left unchanged on failure.
 */
cardano_error_t
cardano_builder_add_starting_account_balance_interval(
  cardano_builder_state_t*            state,
  cardano_reward_address_t*           reward_address,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message);

/**
 * \brief Records a starting account balance interval given the reward address as a bech32 string.
 *
 * This function parses the reward address and delegates to
 * \ref cardano_builder_add_starting_account_balance_interval.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] reward_address A pointer to the bech32 string with the reward address.
 * \param[in] address_size The size of the reward address string in bytes.
 * \param[in] interval A pointer to the \ref cardano_account_balance_interval_t the starting balance
 *                     must sit in. This parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the interval was recorded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_add_starting_account_balance_interval_ex(
  cardano_builder_state_t*            state,
  const char*                         reward_address,
  size_t                              address_size,
  cardano_account_balance_interval_t* interval,
  const char**                        error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_ENTITIES_H
