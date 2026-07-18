/**
 * \file builder_inputs.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_INPUTS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_INPUTS_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Adds a UTXO to the set of inputs the transaction must spend.
 *
 * This function appends the UTXO to the state pre selected input list so it is always included when
 * the transaction is balanced. When \p redeemer is not NULL it creates a spend redeemer with a zero
 * index and zero execution units, appends it to the witness set redeemer list and records it in the
 * state input to redeemer map so its index can be resolved during balancing. When \p datum is not
 * NULL it is added to the witness set plutus data set.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] utxo A pointer to the \ref cardano_utxo_t to spend. The list holds a reference to it.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as spend redeemer payload, or
 *                     NULL when the input does not require a redeemer.
 * \param[in] datum A pointer to the \ref cardano_plutus_data_t witness datum, or NULL when the input
 *                  does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the input was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_add_input(
  cardano_builder_state_t* state,
  cardano_utxo_t*          utxo,
  cardano_plutus_data_t*   redeemer,
  cardano_plutus_data_t*   datum,
  const char**             error_message);

/**
 * \brief Adds a UTXO to spend whose redeemer payload is resolved during balancing.
 *
 * This function adds the UTXO as a pre selected input with an empty placeholder redeemer and
 * registers the callback in the state deferred redeemer list. The callback is invoked while the
 * transaction is balanced to produce the final redeemer payload.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] utxo A pointer to the \ref cardano_utxo_t to spend. This parameter must not be NULL.
 * \param[in] callback The deferred redeemer callback invoked during balancing. This parameter must
 *                     not be NULL.
 * \param[in] user_context The opaque pointer forwarded to the callback.
 * \param[in] datum A pointer to the \ref cardano_plutus_data_t witness datum, or NULL when the input
 *                  does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the input and callback were registered, or an appropriate error
 *         code indicating the failure reason.
 */
cardano_error_t
cardano_builder_add_input_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_utxo_t*                utxo,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  cardano_plutus_data_t*         datum,
  const char**                   error_message);

/**
 * \brief Adds a UTXO as a reference input of the transaction.
 *
 * This function appends the UTXO input to the transaction body reference input set, creating the set
 * when it is missing, and records the UTXO in the state reference input list. When the UTXO output
 * carries a Plutus script reference the matching script language flag is raised in the state so cost
 * models and collateral are accounted for during balancing.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] utxo A pointer to the \ref cardano_utxo_t to reference. The list holds a reference to it.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the reference input was added, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_builder_add_reference_input(
  cardano_builder_state_t* state,
  cardano_utxo_t*          utxo,
  const char**             error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_INPUTS_H
