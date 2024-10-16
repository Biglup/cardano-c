/**
 * \file large_first_helpers.h
 *
 * \author angel.castillo
 * \date   Oct 16, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_LARGE_FIRST_HELPERS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_LARGE_FIRST_HELPERS_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Retrieves the amount of a specific asset from a Cardano value.
 *
 * This function retrieves the amount of a specific asset identified by `asset_id` from a `cardano_value_t` object.
 *
 * \param[in] value A pointer to the \ref cardano_value_t object representing the value containing multiple assets.
 * \param[in] asset_id A pointer to the \ref cardano_asset_id_t object identifying the specific asset to look up.
 *
 * \return \ref CARDANO_SUCCESS if the asset amount was successfully retrieved, or an appropriate error code indicating failure.
 */
int64_t _cardano_large_fist_get_amount(cardano_value_t* value, cardano_asset_id_t* asset_id);

/**
 * \brief Compares two cardano_value_t objects to check if lhs is greater than or equal to rhs.
 *
 * This function compares the ADA and multi-asset values in the lhs and rhs `cardano_value_t` objects.
 * It sets the result to `true` if lhs >= rhs for all assets (including ADA), and `false` otherwise.
 *
 * \param[in] lhs The left-hand side value to compare.
 * \param[in] rhs The right-hand side value to compare.
 * \param[out] result Pointer to a boolean that will be set to true if lhs >= rhs, otherwise false.
 *
 * \return \ref CARDANO_SUCCESS if the comparison was successful, or an appropriate error code indicating failure.
 *
 * \note This function only compares ADA and multi-asset amounts between the two values. If rhs has any asset that is
 * greater than the same asset in lhs, the result will be set to `false`.
 */
cardano_error_t _cardano_large_fist_value_gte(
  cardano_value_t* lhs,
  cardano_value_t* rhs,
  bool*            result);

/**
 * \brief Compares two UTXOs by a specific asset amount in descending order.
 *
 * This function compares the specified asset amounts in the values of two UTXOs. It sorts in descending order,
 * meaning the UTXO with the greater amount of the asset comes first.
 *
 * \param[in] lhs The left-hand side UTXO to compare.
 * \param[in] rhs The right-hand side UTXO to compare.
 * \param[in] context A pointer to the specific \ref cardano_asset_id_t representing the asset to compare.
 *
 * \return A negative value if lhs has more of the asset than rhs, a positive value if rhs has more of the asset than lhs,
 *         or 0 if they are equal in terms of the asset amount.
 */
int32_t _cardano_large_fist_compare_utxos(cardano_utxo_t* lhs, cardano_utxo_t* rhs, void* context);

/**
 * \brief Checks if the pre-selected UTXOs satisfy the target value.
 *
 * This function iterates over the provided list of pre-selected UTXOs and accumulates their value.
 * It checks whether the accumulated value satisfies the target value.
 *
 * \param[in] pre_selected_utxo A list of pre-selected UTXOs.
 * \param[in] target_value The target value to satisfy with the pre-selected UTXOs.
 * \param[out] accumulated_value A pointer to a \ref cardano_value_t object where the accumulated value will be stored.
 * \param[out] satisfies_target A boolean flag that will be set to true if the accumulated value satisfies the target value,
 *                              false otherwise.
 *
 * \return \ref CARDANO_SUCCESS if the UTXOs were processed successfully, or an appropriate error code.
 */
cardano_error_t _cardano_large_fist_check_preselected(
  cardano_utxo_list_t* pre_selected_utxo,
  cardano_value_t*     target_value,
  cardano_value_t**    accumulated_value,
  bool*                satisfies_target);

/**
 * \brief Selects UTXOs containing the specified asset to satisfy the required amount.
 *
 * This function iterates over the available UTXOs and selects those that contain the specified asset,
 * accumulating their value until the required amount is met or exceeded.
 *
 * \param[in] asset_req The asset ID for which UTXOs are being selected.
 * \param[in] required_amount The amount of the asset required.
 * \param[in] available_utxos A list of available UTXOs to select from.
 * \param[out] selected_utxos A list where the selected UTXOs will be added.
 * \param[out] accumulated_value A pointer to a \ref cardano_value_t object that will accumulate the selected asset's value.
 *
 * \return \ref CARDANO_SUCCESS if UTXOs were successfully selected, or an appropriate error code indicating failure.
 *
 * \note The caller is responsible for managing the memory of the `selected_utxos` and `accumulated_value` lists.
 */
cardano_error_t _cardano_large_fist_select_utxos(
  cardano_asset_id_t*  asset_req,
  int64_t              required_amount,
  cardano_utxo_list_t* available_utxos,
  cardano_utxo_list_t* selected_utxos,
  cardano_value_t**    accumulated_value);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_LARGE_FIRST_HELPERS_H