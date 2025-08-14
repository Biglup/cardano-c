/**
 * \file emscripten_coin_selector.c
 *
 * \author angel.castillo
 * \date   Aug 14, 2025
 *
 * Copyright 2025 Biglup Labs
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

#ifdef __EMSCRIPTEN__

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction_body/value.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/coin_selector_impl.h>
#include <emscripten.h>

#include "../../../allocators.h"
#include "../../../string_safe.h"

#include <stdio.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Context for the JavaScript coin selector implementation.
 */
typedef struct emscripten_coin_selector_context_t
{
    cardano_object_t base;
    uint32_t         object_id;
    char             name[256];
} emscripten_coin_selector_context_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Creates a new JavaScript coin selector context.
 *
 * \return A pointer to the newly created context, or NULL if allocation failed.
 */
static emscripten_coin_selector_context_t*
emscripten_coin_selector_context_new(const uint32_t object_id)
{
  emscripten_coin_selector_context_t* context = (emscripten_coin_selector_context_t*)_cardano_malloc(sizeof(emscripten_coin_selector_context_t));

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
 * @brief Retrieves a JavaScript coin_selector object from a central registry.
 *
 * This function is called from C to get a handle to a JavaScript object that
 * implements the coin_selector interface.
 *
 * @param[in] object_id The unique ID for the registered coin_selector object.
 * @return A handle to the JavaScript coin_selector object, or `null` if not found.
 */
extern void* get_coin_selector_from_registry(uint32_t object_id);

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
extern void report_coin_selector_bridge_error(uint32_t object_id, void* exception);

/**
 * @brief Marshals a C `cardano_value_t` object into a JavaScript `Value` object.
 *
 * @param[in] value_obj A pointer to the `cardano_value_t` object to be marshalled.
 * @return A handle to a new JavaScript object. The memory for this JS object is
 * managed by the JavaScript garbage collector.
 */
extern void* marshall_value_to_js(void* value_obj);

/**
 * @brief Marshals a JavaScript array of UTXO objects into a C `cardano_utxo_list_t`.
 *
 * @param[in] utxo_list_obj A handle to a JavaScript array, where each element is a UTXO object.
 * @return A pointer to a newly allocated `cardano_utxo_list_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_utxo_list(void* utxo_list_obj);

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

/* C -> JS ASYNC BRIDGE FUNCTIONS ********************************************/

/**
 * @brief Asynchronously performs coin selection by bridging to a JavaScript implementation.
 *
 * This function acts as the low-level bridge between the C coin selection API and a
 * registered JavaScript coin selector object. It marshals the C input objects (UTXO lists, target value)
 * into JavaScript types, calls the asynchronous `select` method on the JS object, and then
 * marshals the resulting JS selection and remaining UTXO arrays back into C objects.
 *
 * @note This function's body is implemented in JavaScript and relies on Emscripten's Asyncify
 * feature to pause C execution while awaiting the result from the JavaScript `Promise`.
 *
 * @param[in] object_id The unique ID for the registered JavaScript coin selector object.
 * @param[in] pre_selected_utxo A pointer to a C `cardano_utxo_list_t` of UTxOs to force into the selection.
 * @param[in] available_utxo A pointer to a C `cardano_utxo_list_t` of UTxOs available for selection.
 * @param[in] target A pointer to a C `cardano_value_t` representing the total value the selection must cover.
 * @param[out] selection_ptr A pointer to a `cardano_utxo_list_t*` where the pointer to the newly
 * created C list of selected UTxOs will be stored.
 * @param[out] remaining_utxo_ptr A pointer to a `cardano_utxo_list_t*` where the pointer to the newly
 * created C list of remaining (unselected) UTxOs will be stored.
 * @return A `cardano_error_t` indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_coin_selector_bridge_select, (uint32_t object_id, cardano_utxo_list_t* pre_selected_utxo, cardano_utxo_list_t* available_utxo, cardano_value_t* target, cardano_utxo_list_t** selection_ptr, cardano_utxo_list_t** remaining_utxo_ptr), {
  const coin_selector = get_coin_selector_from_registry(object_id);
  if (!coin_selector)
  {
    return 1;
  }

  try
  {
    const preSelectedUtxoArray = marshall_utxo_list_to_js(pre_selected_utxo);
    const availableUtxoArray   = marshall_utxo_list_to_js(available_utxo);
    const targetValue          = marshall_value_to_js(target);

    const result = await coin_selector.select({
      preSelectedUtxo : preSelectedUtxoArray.length > 0 ? preSelectedUtxoArray : undefined,
      availableUtxo : availableUtxoArray,
      targetValue : targetValue
    });

    const selectionListPtr = marshal_utxo_list(result.selection);
    const remainingListPtr = marshal_utxo_list(result.remaining);

    if (selectionListPtr === 0 || remainingListPtr === 0)
    {
      if (selectionListPtr !== 0)
        cardano_utxo_list_unref(selectionListPtr);
      if (remainingListPtr !== 0)
        cardano_utxo_list_unref(remainingListPtr);
      return 4;
    }

    HEAPU32[selection_ptr >> 2]      = selectionListPtr;
    HEAPU32[remaining_utxo_ptr >> 2] = remainingListPtr;

    return 0;
  }
  catch (e)
  {
    report_coin_selector_bridge_error(object_id, e);
    return 1;
  }
});

/* PROVIDER IMPLEMENTATION **************************************************/

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
select(
  cardano_coin_selector_impl_t* coin_selector,
  cardano_utxo_list_t*          pre_selected_utxo,
  cardano_utxo_list_t*          available_utxo,
  cardano_value_t*              target,
  cardano_utxo_list_t**         selection,
  cardano_utxo_list_t**         remaining_utxo)
{
  emscripten_coin_selector_context_t* ctx = (emscripten_coin_selector_context_t*)coin_selector->context;

  if (!ctx || !available_utxo || !target || !selection || !remaining_utxo)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_coin_selector_bridge_select(ctx->object_id, pre_selected_utxo, available_utxo, target, selection, remaining_utxo);
}

/* DEFINITIONS ****************************************************************/

EMSCRIPTEN_KEEPALIVE
cardano_error_t
create_emscripten_coin_selector(
  const char*               name,
  const size_t              name_size,
  const uint32_t            object_id,
  cardano_coin_selector_t** coin_selector_ptr)
{
  if ((coin_selector_ptr == NULL) || (name == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((name_size >= 256U) || (name_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  emscripten_coin_selector_context_t* context = emscripten_coin_selector_context_new(object_id);

  if (context == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(context->name, 256, name, name_size);

  cardano_coin_selector_impl_t impl = { 0 };

  impl.select  = select;
  impl.context = (cardano_object_t*)context;

  cardano_error_t result = cardano_coin_selector_new(impl, coin_selector_ptr);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(context);
    return result;
  }

  return CARDANO_SUCCESS;
}

#endif /* __EMSCRIPTEN__ */