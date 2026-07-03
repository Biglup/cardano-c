/**
 * \file random_improve_utxo_utils.h
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_UTXO_UTILS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_UTXO_UTILS_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/asset_id.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction_body/value.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Returns the quantity of the given asset in the given value (0 if absent).
 *
 * \param[in] value    The value to inspect.
 * \param[in] asset_id The asset to look up.
 *
 * \return The quantity of the asset, or 0 if the value does not contain the asset.
 */
int64_t
_cardano_random_improve_get_asset_quantity(cardano_value_t* value, cardano_asset_id_t* asset_id);

/**
 * \brief Returns the value of the output of the given UTXO as a borrowed reference.
 *
 * \param[in] utxo The UTXO whose output value to borrow.
 *
 * \return A borrowed pointer to the value; valid while the UTXO is alive.
 */
cardano_value_t*
_cardano_random_improve_borrow_utxo_value(cardano_utxo_t* utxo);

/**
 * \brief Indicates whether two asset ids identify the same asset.
 *
 * \param[in] lhs The first asset id.
 * \param[in] rhs The second asset id.
 *
 * \return true if both ids refer to the same policy and asset name, false otherwise.
 */
bool
_cardano_random_improve_asset_id_equals(cardano_asset_id_t* lhs, cardano_asset_id_t* rhs);

/**
 * \brief Returns the total selected quantity of the given asset (NULL asset id = lovelace).
 *
 * \param[in] selection The list of selected UTXOs.
 * \param[in] asset_id  The asset to total, or NULL for lovelace.
 *
 * \return The total quantity of the asset across all selected UTXOs.
 */
uint64_t
_cardano_random_improve_selected_quantity(cardano_utxo_list_t* selection, cardano_asset_id_t* asset_id);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RANDOM_IMPROVE_UTXO_UTILS_H
