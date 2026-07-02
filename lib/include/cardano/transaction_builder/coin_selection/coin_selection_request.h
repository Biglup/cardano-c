/**
 * \file coin_selection_request.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTION_REQUEST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTION_REQUEST_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/common/utxo_list.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_body/value.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Describes all inputs to a coin selection.
 *
 * All pointers are borrowed: they must remain valid for the duration of the
 * \ref cardano_coin_selector_select call, and the selector does not take ownership of them.
 *
 * Zero-initialize the structure (<code>= { 0 }</code>) so that unset optional fields default safely.
 * Fields added in future versions will be appended to the end of the structure, with a zero value
 * meaning "default behavior".
 */
typedef struct cardano_coin_selection_request_t
{
    /**
     * \brief Optional. A set of pre-selected UTXOs that must be included in the final selection.
     */
    cardano_utxo_list_t* pre_selected_utxo;

    /**
     * \brief Required. The list of available UTXOs from which the coin selection will be made.
     */
    cardano_utxo_list_t* available_utxo;

    /**
     * \brief Required. The target value to be covered by the selection. This value is authoritative
     * for all balance arithmetic: the selection must uphold
     * <code>sum(selection) = target + sum(change_outputs)</code>.
     */
    cardano_value_t* target;

    /**
     * \brief Optional. The user-specified outputs the target was derived from.
     *
     * Selectors may use this list as a shape and weight hint when generating change outputs
     * (for example, to mimic the distribution of user payments). Selectors must not rely on it
     * for balance arithmetic; the target is authoritative.
     */
    cardano_transaction_output_list_t* outputs_to_cover;

    /**
     * \brief Required. The address to which change outputs will be sent.
     */
    cardano_address_t* change_address;

    /**
     * \brief Required. The protocol parameters, used to ensure change outputs are min-ADA compliant.
     */
    cardano_protocol_parameters_t* protocol_params;

} cardano_coin_selection_request_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTION_REQUEST_H
