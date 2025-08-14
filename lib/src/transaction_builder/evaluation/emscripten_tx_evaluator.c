/**
 * \file emscripten_tx_evaluator.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2025
 *
 * Copyright 2025 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __EMSCRIPTEN__

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <emscripten.h>

#include "../../allocators.h"
#include "../../string_safe.h"

#include <stdio.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Context for the JavaScript transaction evaluator implementation.
 */
typedef struct emscripten_tx_evaluator_context_t
{
    cardano_object_t base;
    uint32_t         object_id;
    char             name[256];
} emscripten_tx_evaluator_context_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Creates a new JavaScript transaction evaluator context.
 *
 * \return A pointer to the newly created context, or NULL if allocation failed.
 */
static emscripten_tx_evaluator_context_t*
emscripten_tx_evaluator_context_new(const uint32_t object_id)
{
  emscripten_tx_evaluator_context_t* context = (emscripten_tx_evaluator_context_t*)_cardano_malloc(sizeof(emscripten_tx_evaluator_context_t));

  if (context == NULL)
  {
    return NULL;
  }

  context->base.ref_count     = 1;
  context->base.last_error[0] = '\0';
  context->base.deallocator   = _cardano_free;
  context->object_id          = object_id;

  return context;
}

/* JAVASCRIPT INTERFACE *****************************************************/

/**
 * @brief Retrieves a JavaScript tx_evaluator object from a central registry.
 *
 * This function is called from C to get a handle to a JavaScript object that
 * implements the tx_evaluator interface.
 *
 * @param[in] object_id The unique ID for the registered tx_evaluator object.
 * @return A handle to the JavaScript tx_evaluator object, or `null` if not found.
 */
extern void* get_tx_evaluator_from_registry(uint32_t object_id);

/**
 * @brief Reports an exception from the C provider layer back to the JavaScript side.
 *
 * This function is implemented in JavaScript and imported into the C/WASM module. It
 * serves as a bridge to pass an error that occurred within a C provider's asynchronous
 * operation back to the JavaScript error handling system.
 *
 * @param object_id The unique identifier of the JavaScript provider instance that
 * initiated the operation.
 * @param exception A pointer to an object or string containing the details of the
 * exception.
 */
extern void report_tx_evaluator_bridge_error(uint32_t object_id, void* exception);

/**
 * @brief Marshals a C `cardano_transaction_t` into its CBOR representation as a JavaScript hex string.
 *
 * @param[in] tx_ptr A pointer to a `cardano_transaction_t` object in WASM memory.
 * @return A handle to a JavaScript hex string of the transaction's CBOR.
 */
extern void* marshall_transaction_to_cbor_hex(void* tx_ptr);

/**
 * @brief Marshals a C `cardano_utxo_list_t` into a JavaScript array of UTXO objects.
 *
 * This is useful for passing complex C structures to JS functions that expect plain objects,
 * like in `evaluate_transaction`.
 *
 * @param[in] utxo_list_ptr A pointer to a `cardano_utxo_list_t` object in WASM memory.
 * @return A handle to a JavaScript array of UTXO objects.
 */
extern void* marshall_utxo_list_to_js(void* utxo_list_ptr);

/**
 * @brief Marshals a JavaScript array of redeemer objects into a C `cardano_redeemer_list_t`.
 *
 * @param[in] js_redeemer_list_obj A handle to a JavaScript array of redeemer objects.
 * @return A pointer to a newly allocated `cardano_redeemer_list_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_redeemer_list(void* js_redeemer_array);

/* C -> JS ASYNC BRIDGE FUNCTIONS ********************************************/

/**
 * @brief Asynchronously evaluates the execution units (ExUnits) for a transaction.
 *
 * This function acts as a bridge to the JavaScript `tx_evaluator.evaluateTransaction()` method.
 */
EM_ASYNC_JS(cardano_error_t, cardano_tx_evaluator_bridge_evaluate, (uint32_t object_id, cardano_transaction_t* tx, cardano_utxo_list_t* additional_utxos, cardano_redeemer_list_t** redeemers_ptr), {
  const tx_evaluator = get_tx_evaluator_from_registry(object_id);
  if (!tx_evaluator)
  {
    return 1;
  }

  try
  {
    const tx_cbor_hex            = marshall_transaction_to_cbor_hex(tx);
    const additional_utxos_array = marshall_utxo_list_to_js(additional_utxos);

    const result_redeemers = await tx_evaluator.evaluate(tx_cbor_hex, additional_utxos_array);

    const marshaled_ptr = marshal_redeemer_list(result_redeemers);

    if (marshaled_ptr === 0)
    {
      return 4;
    }

    HEAPU32[redeemers_ptr >> 2] = marshaled_ptr;

    return 0;
  }
  catch (e)
  {
    report_tx_evaluator_bridge_error(object_id, e);
    return 1;
  }
});

/* C -> JS ASYNC BRIDGE FUNCTIONS ********************************************/

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
  emscripten_tx_evaluator_context_t* ctx = (emscripten_tx_evaluator_context_t*)tx_evaluator_impl->context;

  if (!ctx || !tx || !redeemers)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_tx_evaluator_bridge_evaluate(ctx->object_id, tx, additional_utxos, redeemers);
}

/* DEFINITIONS ****************************************************************/

/**
 * @brief Creates a new transaction evaluator that is implemented in JavaScript.
 */
EMSCRIPTEN_KEEPALIVE
cardano_error_t
create_emscripten_tx_evaluator(
  const char*              name,
  const size_t             name_size,
  const uint32_t           object_id,
  cardano_tx_evaluator_t** tx_evaluator_ptr)
{
  if ((tx_evaluator_ptr == NULL) || (name == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((name_size >= 256U) || (name_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  emscripten_tx_evaluator_context_t* context = emscripten_tx_evaluator_context_new(object_id);

  if (context == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(context->name, 256, name, name_size);

  cardano_tx_evaluator_impl_t impl = { 0 };

  impl.evaluate = evaluate;
  impl.context  = (cardano_object_t*)context;

  cardano_error_t result = cardano_tx_evaluator_new(impl, tx_evaluator_ptr);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(context);
    return result;
  }

  return CARDANO_SUCCESS;
}

#endif /* __EMSCRIPTEN__ */