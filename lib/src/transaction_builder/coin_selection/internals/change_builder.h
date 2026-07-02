/**
 * \file change_builder.h
 *
 * \author angel.castillo
 * \date   Jul 02, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CHANGE_BUILDER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CHANGE_BUILDER_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Builds min-ADA compliant change outputs for a coin selection.
 *
 * This function computes the change left over by a selection (`sum(selection) - target`) and materializes it
 * as change output(s) sent to `change_address`, upholding the local balance invariant:
 *
 * \code
 * sum(selection) = target + sum(change_outputs)
 * \endcode
 *
 * If the change is below the min-ADA requirement for its output, additional UTXOs are moved from
 * `remaining_utxo` into `selection` (largest lovelace first) until the change output is min-ADA compliant.
 * If the available pool is exhausted before compliance is reached, \ref CARDANO_ERROR_BALANCE_INSUFFICIENT
 * is returned.
 *
 * \param[in]     target          The target value the selection must cover.
 * \param[in]     change_address  The address to which the change outputs will be sent.
 * \param[in]     protocol_params The protocol parameters, used to compute the min-ADA requirement.
 * \param[in,out] selection       The list of selected UTXOs. May grow if additional UTXOs are needed to make
 *                                the change min-ADA compliant.
 * \param[in,out] remaining_utxo  The list of unselected UTXOs. May shrink as UTXOs are moved into `selection`.
 * \param[out]    change_outputs  A pointer to the list where the change outputs will be stored. The list is
 *                                empty if the selection matches the target exactly.
 *
 * \return \ref CARDANO_SUCCESS if the change outputs were successfully built, or an appropriate error code.
 *
 * \note The caller is responsible for releasing the `change_outputs` list when it is no longer needed.
 */
cardano_error_t
_cardano_coin_selector_build_change(
  cardano_value_t*                    target,
  cardano_address_t*                  change_address,
  cardano_protocol_parameters_t*      protocol_params,
  cardano_utxo_list_t*                selection,
  cardano_utxo_list_t*                remaining_utxo,
  cardano_transaction_output_list_t** change_outputs);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CHANGE_BUILDER_H
