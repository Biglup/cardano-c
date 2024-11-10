/**
 * \file provider_tx_evaluator.c
 *
 * \author angel.castillo
 * \date   Nov 05, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>

#include "../../string_safe.h"

#include <assert.h>
#include <cardano/providers/provider.h>
#include <cardano/transaction_builder/evaluation/provider_tx_evaluator.h>
#include <string.h>

/* STATIC FUNCTIONS ************************************************************/

/**
 * \brief Selects UTXOs from the available list and pre-selected UTXOs to meet the target value.
 *
 * This function selects UTXOs from both the pre-selected UTXO list and available UTXOs to meet a specified target value.
 * The selected UTXOs are stored in the `selection` list, and any remaining UTXOs are stored in the `remaining_utxo` list.
 *
 * \param[in] coin_selector A pointer to the coin selector implementation object.
 * \param[in] pre_selected_utxo A list of pre-selected UTXOs that must be included in the final selection.
 * \param[in] available_utxo A list of available UTXOs to select from.
 * \param[in] target A pointer to a \ref cardano_value_t object that defines the target amount of ADA and assets.
 * \param[out] selection A pointer to the list of selected UTXOs that meet the target value.
 * \param[out] remaining_utxo A pointer to the list of UTXOs that were not selected and remain available for future transactions.
 *
 * \return \ref CARDANO_SUCCESS if UTXOs were successfully selected, or an appropriate error code indicating failure.
 */
static cardano_error_t
evaluate(
  cardano_tx_evaluator_impl_t* tx_evaluator_impl,
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         additional_utxos,
  cardano_redeemer_list_t**    redeemers)
{
  assert(tx_evaluator_impl != NULL);
  assert(tx_evaluator_impl->context != NULL);
  assert(tx != NULL);
  assert(redeemers != NULL);

  cardano_provider_t* provider = (cardano_provider_t*)((void*)tx_evaluator_impl->context);

  return cardano_provider_evaluate_transaction(provider, tx, additional_utxos, redeemers);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_tx_evaluator_from_provider(
  cardano_provider_t*      provider,
  cardano_tx_evaluator_t** tx_evaluator)
{
  cardano_tx_evaluator_impl_t impl = { 0 };

  if ((provider == NULL) || (tx_evaluator == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_provider_ref(provider);
  impl.context = (cardano_object_t*)((void*)provider);

  cardano_safe_memcpy(impl.name, 256U, "Provider transaction evaluator", 256U);

  impl.evaluate = evaluate;

  cardano_error_t result = cardano_tx_evaluator_new(impl, tx_evaluator);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_unref(&provider);
  }

  return result;
}