/**
 * \file coin_selector_impl.h
 *
 * \author angel.castillo
 * \date   Oct 14, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTOR_IMPL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTOR_IMPL_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_body/value.h>
#include <cardano/transaction_builder/coin_selection/coin_selection_request.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FORWARD DECLARATIONS *******************************************************/

/**
 * \brief Opaque structure representing the coin selection implementation.
 *
 * The `cardano_coin_selector_impl_t` is an opaque structure that implements coin selection logic.
 * It abstracts away the internal details of coin selection.
 */
typedef struct cardano_coin_selector_impl_t cardano_coin_selector_impl_t;

/* CALLBACKS *****************************************************************/

/**
 * \brief Callback function type for performing coin selection.
 *
 * The `cardano_coin_select_func_t` typedef defines the signature for a callback function responsible
 * for selecting a set of UTXOs (Unspent Transaction Outputs) from an available list to meet a specific target value,
 * and for producing the change outputs that return any excess value to the change address.
 *
 * This function is expected to:
 * - Use the request's `available_utxo` list and optionally its `pre_selected_utxo` list to select UTXOs
 *   that meet the required `target` value.
 * - Store the selected UTXOs in the `selection` list, ensuring the selection covers the target value.
 * - Store any remaining UTXOs that were not selected in the `remaining_utxo` list.
 * - Produce one or more change outputs in the `change_outputs` list such that the selection is locally balanced:
 *   sum(selection) = target + sum(change_outputs).
 * - Ensure every change output is min-ADA compliant as per the protocol parameters (selecting additional UTXOs
 *   from the available pool if necessary to top up the change).
 *
 * The selector must not attempt fee calculation or global transaction balancing; those remain the
 * responsibility of the balancer.
 *
 * \param[in] coin_selector A pointer to the \ref cardano_coin_selector_impl_t object implementing the coin selection strategy.
 * \param[in] request The selection request. See \ref cardano_coin_selection_request_t. The wrapper guarantees
 *                    that the request pointer and its required fields are not NULL.
 * \param[out] selection A pointer to the list of UTXOs that were selected to meet the target value.
 * \param[out] remaining_utxo A pointer to the list of UTXOs that were not selected and remain available for future transactions.
 * \param[out] change_outputs A pointer to the list of change outputs produced by the selection. The list may be empty
 *                            if the selection matches the target exactly.
 *
 * \return \ref CARDANO_SUCCESS if the coin selection was successful and UTXOs were selected to meet the target, or an appropriate error code indicating failure.
 *
 * \note The caller is responsible for managing the memory of the `selection`, `remaining_utxo` and `change_outputs` lists,
 *       ensuring they are properly freed when no longer needed.
 */
typedef cardano_error_t (*cardano_coin_select_func_t)(
  cardano_coin_selector_impl_t*           coin_selector,
  const cardano_coin_selection_request_t* request,
  cardano_utxo_list_t**                   selection,
  cardano_utxo_list_t**                   remaining_utxo,
  cardano_transaction_output_list_t**     change_outputs);

/* STRUCTURES ****************************************************************/

/**
 * \brief Opaque structure representing the coin selection implementation.
 *
 * The `cardano_coin_selector_impl_t` is an opaque structure that implements coin selection logic.
 * It abstracts away the internal details of coin selection.
 */
typedef struct cardano_coin_selector_impl_t
{
    /**
     * \brief Name of the coin selector implementation.
     */
    char name[256];

    /**
     * \brief Error message buffer for coin selector specific error messages.
     */
    char error_message[1024];

    /**
     * \brief Opaque pointer to the implementation-specific context.
     *
     * This pointer holds the state or context required by the coin selector implementation.
     * Users should not access or modify this directly.
     */
    cardano_object_t* context;

    /**
     * \brief Function to select UTXOs to meet a target value.
     *
     * \see cardano_coin_select_func_t
     */
    cardano_coin_select_func_t select;

} cardano_coin_selector_impl_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTOR_IMPL_H