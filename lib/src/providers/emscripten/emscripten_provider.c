/**
 * \file emscripten_provider.c
 *
 * \author angel.castillo
 * \date   Apr 06, 2025
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

#include <cardano/object.h>
#include <cardano/providers/provider.h>
#include <cardano/providers/provider_impl.h>
#include <emscripten.h>

#include "../../allocators.h"
#include "../../string_safe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Context for the JavaScript provider implementation.
 */
typedef struct emscripten_provider_context_t
{
    cardano_object_t base;
    uint32_t         object_id;
    char             name[256];
} emscripten_provider_context_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Creates a new JavaScript provider context.
 *
 * \return A pointer to the newly created context, or NULL if allocation failed.
 */
static emscripten_provider_context_t*
emscripten_provider_context_new(const uint32_t object_id)
{
  emscripten_provider_context_t* context = (emscripten_provider_context_t*)_cardano_malloc(sizeof(emscripten_provider_context_t));

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
 * @brief Retrieves a JavaScript provider object from a central registry.
 *
 * This function is called from C to get a handle to a JavaScript object that
 * implements the provider interface.
 *
 * @param[in] object_id The unique ID for the registered provider object.
 * @return A handle to the JavaScript provider object, or `null` if not found.
 */
extern void* get_provider_from_registry(uint32_t object_id);

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
extern void report_provider_bridge_error(uint32_t object_id, void* exception);

/**
 * @brief Marshals a JavaScript object into a C `cardano_protocol_parameters_t`.
 *
 * @param[in] params_obj A handle to the JavaScript object containing protocol parameters.
 * @return A pointer to a newly allocated `cardano_protocol_parameters_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_protocol_parameters(void* params_obj);

/**
 * @brief Marshals a C `cardano_address_t` object into a JavaScript string (Bech32).
 *
 * @param[in] address_obj A pointer to a `cardano_address_t` object in WASM memory.
 * @return A handle to a JavaScript string representing the address.
 */
extern void* marshall_address(void* address_obj);

/**
 * @brief Marshals a JavaScript array of UTXO objects into a C `cardano_utxo_list_t`.
 *
 * @param[in] utxo_list_obj A handle to a JavaScript array, where each element is a UTXO object.
 * @return A pointer to a newly allocated `cardano_utxo_list_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_utxo_list(void* utxo_list_obj);

/**
 * @brief Marshals a C `cardano_reward_address_t` object into a JavaScript string (Bech32).
 *
 * @param[in] reward_address_ptr A pointer to a `cardano_reward_address_t` object in WASM memory.
 * @return A handle to a JavaScript string representing the reward address.
 */
extern void* marshall_reward_address(void* reward_address_ptr);

/**
 * @brief Marshals a C `cardano_asset_id_t` object into a JavaScript hex string.
 *
 * @param[in] asset_id_ptr A pointer to a `cardano_asset_id_t` object in WASM memory.
 * @return A handle to a JavaScript hex string representing the asset ID.
 */
extern void* marshall_asset_id(void* asset_id_ptr);

/**
 * @brief Marshals a C `cardano_transaction_input_set_t` into a JavaScript array of input objects.
 *
 * @param[in] tx_input_set_ptr A pointer to a `cardano_transaction_input_set_t` object in WASM memory.
 * @return A handle to a JavaScript array of transaction input objects.
 */
extern void* marshall_tx_input_set(void* tx_input_set_ptr);

/**
 * @brief Marshals a C `cardano_blake2b_hash_t` into a JavaScript hex string.
 *
 * @param[in] hash_ptr A pointer to a `cardano_blake2b_hash_t` object in WASM memory.
 * @return A handle to a JavaScript hex string representing the hash.
 */
extern void* marshall_blake2b_hash(void* hash_ptr);

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
 * @brief Marshals a single JavaScript UTXO object into a C `cardano_utxo_t`.
 *
 * @param[in] js_utxo_obj A handle to a single JavaScript UTXO object.
 * @return A pointer to a newly allocated `cardano_utxo_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_utxo(void* js_utxo_obj);

/**
 * @brief Marshals a JavaScript string of Plutus data CBOR hex into a C `cardano_plutus_data_t`.
 *
 * @param[in] js_plutus_data_cbor_hex A handle to a JavaScript string containing the CBOR hex of a Plutus datum.
 * @return A pointer to a newly allocated `cardano_plutus_data_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_plutus_data(void* js_plutus_data_cbor_hex);

/**
 * @brief Marshals a JavaScript hex string into a C `cardano_blake2b_hash_t`.
 *
 * @param[in] js_hex_string A pointer to a null-terminated UTF8 string passed from JavaScript.
 * @return A pointer to a newly allocated `cardano_blake2b_hash_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_blake2b_hash_from_hex(const char* js_hex_string);

/**
 * @brief Marshals a JavaScript array of redeemer objects into a C `cardano_redeemer_list_t`.
 *
 * @param[in] js_redeemer_list_obj A handle to a JavaScript array of redeemer objects.
 * @return A pointer to a newly allocated `cardano_redeemer_list_t` in WASM memory. Returns `0` on failure.
 * @note The caller in C is responsible for freeing the returned object via `cardano_object_unref`.
 */
extern void* marshal_redeemer_list(void* js_redeemer_list_obj);

/* C -> JS ASYNC BRIDGE FUNCTIONS ********************************************/

/**
 * @brief Asynchronously fetches the latest protocol parameters from the provider.
 *
 * This function acts as a bridge to the JavaScript `provider.getParameters()` method.
 * It uses Emscripten's Asyncify feature to pause C execution and await the result from JavaScript.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[out] parameters_ptr A pointer to a `cardano_protocol_parameters_t*` where the pointer to the
 * newly created C object will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_get_parameters, (uint32_t object_id, cardano_protocol_parameters_t** parameters_ptr), {
  const provider = get_provider_from_registry(object_id);

  if (!provider)
  {
    return 1;
  }

  try
  {
    const params               = await provider.getParameters();
    const marshaled_params_ptr = marshal_protocol_parameters(params);

    if (marshaled_params_ptr === 0)
    {
      return 1;
    }

    HEAPU32[parameters_ptr >> 2] = marshaled_params_ptr;

    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously fetches all unspent transaction outputs (UTxOs) for a given address.
 *
 * This function acts as a bridge to the JavaScript `provider.getUnspentOutputs()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] address A pointer to the `cardano_address_t` object to query.
 * @param[out] utxo_list A pointer to a `cardano_utxo_list_t*` where the pointer to the
 * newly created list of UTxOs will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_get_unspent_outputs, (uint32_t object_id, cardano_address_t* address, cardano_utxo_list_t** utxo_list), {
  const provider = get_provider_from_registry(object_id);

  if (!provider || !address || !utxo_list)
  {
    return 1;
  }

  try
  {
    const stringAddress           = marshall_address(address);
    const utxos                   = await provider.getUnspentOutputs(stringAddress);
    const marshaled_utxo_list_ptr = marshal_utxo_list(utxos);

    if (marshaled_utxo_list_ptr === 0)
    {
      return ;
    }

    HEAPU32[utxo_list >> 2] = marshaled_utxo_list_ptr;

    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously fetches the rewards balance for a given reward address.
 *
 * This function acts as a bridge to the JavaScript `provider.getRewardsBalance()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] address A pointer to the `cardano_reward_address_t` object to query.
 * @param[out] rewards_ptr A pointer to a `uint64_t` where the reward amount (in Lovelace) will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_get_rewards_balance, (uint32_t object_id, cardano_reward_address_t* address, uint64_t* rewards_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider) {
    return 1;
  }

  try
  {
    const address_str = marshall_reward_address(address);
    const result_rewards = await provider.getRewardsBalance(address_str);

    const low32  = Number(result_rewards & 0xFFFFFFFFn);
    const high32 = Number(result_rewards >> 32n);

    setValue(rewards_ptr,     low32,  'i32');
    setValue(rewards_ptr + 4, high32, 'i32');

    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously fetches all UTxOs for an address that contain a specific asset.
 *
 * This function acts as a bridge to the JavaScript `provider.getUnspentOutputsWithAsset()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] address A pointer to the `cardano_address_t` object to query.
 * @param[in] asset_id A pointer to the `cardano_asset_id_t` to filter by.
 * @param[out] utxo_list_ptr A pointer to a `cardano_utxo_list_t*` where the pointer to the
 * resulting list will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_get_unspent_outputs_with_asset, (uint32_t object_id, cardano_address_t* address, cardano_asset_id_t* asset_id, cardano_utxo_list_t** utxo_list_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider)
    return 1;

  try
  {
    const address_str   = marshall_address(address);
    const asset_id_str  = marshall_asset_id(asset_id);
    const result_utxos  = await provider.getUnspentOutputsWithAsset(address_str, asset_id_str);
    const marshaled_ptr = marshal_utxo_list(result_utxos);

    if (marshaled_ptr === 0)
      return 1;

    HEAPU32[utxo_list_ptr >> 2] = marshaled_ptr;
    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously fetches the single UTxO that contains a specific NFT.
 *
 * This function acts as a bridge to the JavaScript `provider.getUnspentOutputByNft()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] asset_id A pointer to the `cardano_asset_id_t` of the NFT to find.
 * @param[out] utxo_ptr A pointer to a `cardano_utxo_t*` where the pointer to the
 * resulting UTxO will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation. Returns
 * `CARDANO_ERROR_ELEMENT_NOT_FOUND` if the NFT's UTxO does not exist.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_get_unspent_output_by_nft, (uint32_t object_id, cardano_asset_id_t* asset_id, cardano_utxo_t** utxo_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider)
    return 1;

  try
  {
    const asset_id_str  = marshall_asset_id(asset_id);
    const result_utxo   = await provider.getUnspentOutputByNft(asset_id_str);
    const marshaled_ptr = marshal_utxo(result_utxo);

    if (marshaled_ptr === 0)
      return 1;

    HEAPU32[utxo_ptr >> 2] = marshaled_ptr;
    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously resolves a set of transaction inputs to their corresponding UTxOs.
 *
 * This function acts as a bridge to the JavaScript `provider.resolveUnspentOutputs()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] tx_ins A pointer to the `cardano_transaction_input_set_t` to resolve.
 * @param[out] utxo_list_ptr A pointer to a `cardano_utxo_list_t*` where the pointer to the
 * list of resolved UTxOs will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_resolve_unspent_outputs, (uint32_t object_id, cardano_transaction_input_set_t* tx_ins, cardano_utxo_list_t** utxo_list_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider)
    return 1;

  try
  {
    const inputs_array  = marshall_tx_input_set(tx_ins);
    const result_utxos  = await provider.resolveUnspentOutputs(inputs_array);
    const marshaled_ptr = marshal_utxo_list(result_utxos);

    if (marshaled_ptr === 0)
      return 1;

    HEAPU32[utxo_list_ptr >> 2] = marshaled_ptr;
    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously fetches a Plutus datum from its hash.
 *
 * This function acts as a bridge to the JavaScript `provider.resolveDatum()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] datum_hash A pointer to the `cardano_blake2b_hash_t` of the datum to resolve.
 * @param[out] datum_ptr A pointer to a `cardano_plutus_data_t*` where the pointer to the
 * resolved datum will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation. Returns
 * `CARDANO_ERROR_ELEMENT_NOT_FOUND` if the datum does not exist.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_resolve_datum, (uint32_t object_id, cardano_blake2b_hash_t* datum_hash, cardano_plutus_data_t** datum_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider)
    return 1;

  try
  {
    const datum_hash_hex    = marshall_blake2b_hash(datum_hash);
    const result_datum_cbor = await provider.resolveDatum(datum_hash_hex);
    const marshaled_ptr     = marshal_plutus_data(result_datum_cbor);

    if (marshaled_ptr === 0)
      return 1;

    HEAPU32[datum_ptr >> 2] = marshaled_ptr;
    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously waits for a transaction to be confirmed on the blockchain.
 *
 * This function acts as a bridge to the JavaScript `provider.confirmTransaction()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] tx_id A pointer to the `cardano_blake2b_hash_t` of the transaction to confirm.
 * @param[in] timeout_ms The maximum time in milliseconds to wait for confirmation.
 * @param[out] confirmed_ptr A pointer to a `bool` where the confirmation status (true/false) will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_confirm_transaction, (uint32_t object_id, cardano_blake2b_hash_t* tx_id, uint32_t timeout_ms, bool* confirmed_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider)
    return 1;

  try
  {
    const tx_id_hex    = marshall_blake2b_hash(tx_id);
    const is_confirmed = await provider.confirmTransaction(tx_id_hex, Number(timeout_ms));

    HEAPU8[confirmed_ptr] = is_confirmed ? 1 : 0;

    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously submits a signed transaction to the network.
 *
 * This function acts as a bridge to the JavaScript `provider.submitTransaction()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] tx A pointer to the `cardano_transaction_t` to be submitted.
 * @param[out] tx_id_ptr A pointer to a `cardano_blake2b_hash_t*` where the pointer to the
 * resulting transaction ID (hash) will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_submit_transaction, (uint32_t object_id, cardano_transaction_t* tx, cardano_blake2b_hash_t** tx_id_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider)
    return 1;

  try
  {
    const tx_cbor_hex        = marshall_transaction_to_cbor_hex(tx);
    const result_tx_hash_hex = await provider.submitTransaction(tx_cbor_hex);

    const marshaled_hash_ptr = marshal_blake2b_hash_from_hex(result_tx_hash_hex);
    if (marshaled_hash_ptr === 0)
      return 1;

    HEAPU32[tx_id_ptr >> 2] = marshaled_hash_ptr;

    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/**
 * @brief Asynchronously evaluates the execution units (ExUnits) for a transaction.
 *
 * This function acts as a bridge to the JavaScript `provider.evaluateTransaction()` method.
 *
 * @param[in] object_id The unique ID for the registered JavaScript provider object.
 * @param[in] tx A pointer to the `cardano_transaction_t` to evaluate.
 * @param[in] additional_utxos A pointer to a `cardano_utxo_list_t` of additional UTxOs to use
 * for evaluation. Can be `NULL`.
 * @param[out] redeemers_ptr A pointer to a `cardano_redeemer_list_t*` where the pointer to the
 * list of redeemers with their calculated ExUnits will be stored.
 * @return A cardano_error_t indicating the success or failure of the operation. Returns
 * `CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE` if the transaction fails evaluation.
 */
EM_ASYNC_JS(cardano_error_t, cardano_provider_bridge_evaluate_transaction, (uint32_t object_id, cardano_transaction_t* tx, cardano_utxo_list_t* additional_utxos, cardano_redeemer_list_t** redeemers_ptr), {
  const provider = get_provider_from_registry(object_id);
  if (!provider)
    return 1;

  try
  {
    const tx_cbor_hex = marshall_transaction_to_cbor_hex(tx);
    const additional_utxos_array = marshall_utxo_list_to_js(additional_utxos);

    const result_redeemers = await provider.evaluateTransaction(tx_cbor_hex, additional_utxos_array);
    const marshaled_ptr    = marshal_redeemer_list(result_redeemers);

    if (marshaled_ptr === 0)
      return 1;

    HEAPU32[redeemers_ptr >> 2] = marshaled_ptr;
    return 0;
  }
  catch (e)
  {
    report_provider_bridge_error(object_id, e);
    return 1;
  }
});

/* PROVIDER IMPLEMENTATION **************************************************/

/**
 * \brief Retrieves the current protocol parameters from the Cardano blockchain.
 *
 * The protocol parameters include important configuration details of the Cardano network,
 * such as parameters to compute fees, maximum block sizes, and other protocol-related settings.
 *
 * \param[in]  provider   Pointer to the \ref cardano_provider_t instance.
 *                        Must not be `NULL`.
 * \param[out] parameters Pointer to a location where the function will store a pointer
 *                        to the retrieved \ref cardano_protocol_parameters_t structure.
 *                        This parameter must not be `NULL`. On success, the caller is
 *                        responsible for freeing the returned \ref cardano_protocol_parameters_t
 *                        object using the \ref cardano_protocol_parameters_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - An appropriate error code on failure.
dwq */
static cardano_error_t
get_parameters(cardano_provider_impl_t* provider_impl, cardano_protocol_parameters_t** parameters)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !parameters)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_get_parameters(ctx->object_id, parameters);
}

/**
 * \brief Retrieves unspent transaction outputs (UTXOs) for a given address.
 *
 * This function obtains a list of unspent transaction outputs (UTXOs) associated with the specified
 * address using the provided Cardano provider instance. UTXOs represent funds that can be used as
 * inputs in new transactions. Retrieving UTXOs is essential for constructing transactions and
 * managing balances.
 *
 * \param[in]  provider  Pointer to the \ref cardano_provider_t instance.
 *                       Must not be `NULL`.
 * \param[in]  address   Pointer to a \ref cardano_address_t representing the address for which
 *                       to retrieve unspent outputs. Must not be `NULL`.
 * \param[out] utxo_list Pointer to a location where the function will store a pointer to the
 *                       retrieved \ref cardano_utxo_list_t structure. Must not be `NULL`. On success,
 *                       the caller is responsible for freeing the returned \ref cardano_utxo_list_t
 *                       object using the \ref cardano_utxo_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - An appropriate error code on failure.
 */
static cardano_error_t
get_unspent_outputs(
  cardano_provider_impl_t* provider_impl,
  cardano_address_t*       address,
  cardano_utxo_list_t**    utxo_list)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !address || !utxo_list)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_get_unspent_outputs(ctx->object_id, address, utxo_list);
}

/**
 * \brief Retrieves the staking rewards for a given address.
 *
 * This function obtains the current available staking rewards associated with the specified address.
 * It uses the provided Cardano provider instance to access blockchain data and retrieve
 * the reward balance for the address. Staking rewards are accumulated for addresses that
 * delegate their stake to a stake pool.
 *
 * \param[in]  provider Pointer to the \ref cardano_provider_t instance.
 *                      Must not be `NULL`.
 * \param[in]  address  Pointer to a \ref cardano_address_t representing the address
 *                      for which to retrieve staking rewards. Must not be `NULL`.
 * \param[out] rewards  Pointer to a `uint64_t` variable where the function will store
 *                      the amount of rewards (in Lovelace). Must not be `NULL`.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 */
static cardano_error_t
get_rewards_balance(
  cardano_provider_impl_t*  provider_impl,
  cardano_reward_address_t* address,
  uint64_t*                 rewards)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !address || !rewards)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_get_rewards_balance(ctx->object_id, address, rewards);
}

/**
 * \brief Retrieves unspent transaction outputs (UTXOs) for a given address that contain a specific asset.
 *
 * This function obtains a list of unspent transaction outputs (UTXOs) associated with the specified
 * address that contain the specified asset. It uses the provided Cardano provider instance to access
 * the blockchain data. UTXOs represent funds that can be used as inputs in new transactions.
 *
 * \param[in]  provider  Pointer to the \ref cardano_provider_t instance.
 *                       Must not be `NULL`.
 * \param[in]  address   Pointer to a \ref cardano_address_t representing the address for which
 *                       to retrieve unspent outputs. Must not be `NULL`.
 * \param[in]  asset_id  Pointer to a \ref cardano_asset_id_t representing the asset identifier
 *                       used to filter the UTXOs. Must not be `NULL`.
 * \param[out] utxo_list Pointer to a location where the function will store a pointer to the
 *                       retrieved \ref cardano_utxo_list_t structure containing UTXOs that include
 *                       the specified asset. Must not be `NULL`. On success, the caller is responsible
 *                       for freeing the returned \ref cardano_utxo_list_t object using the \ref cardano_utxo_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - Other error codes for provider-specific failures.
 */
static cardano_error_t
get_unspent_outputs_with_asset(
  cardano_provider_impl_t* provider_impl,
  cardano_address_t*       address,
  cardano_asset_id_t*      asset_id,
  cardano_utxo_list_t**    utxo_list)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !address || !asset_id || !utxo_list)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_get_unspent_outputs_with_asset(ctx->object_id, address, asset_id, utxo_list);
}

/**
 * \brief Retrieves an unspent transaction output (UTXO) containing a specific Non-Fungible Token (NFT).
 *
 * This function obtains a UTXO that contains the specified NFT (Non-Fungible Token) identified by the given asset ID.
 * It uses the provided Cardano provider instance to access the blockchain data.
 *
 * \param[in]  provider Pointer to the \ref cardano_provider_t instance.
 *                      Must not be `NULL`.
 * \param[in]  asset_id Pointer to a \ref cardano_asset_id_t representing the asset identifier of the NFT.
 *                      Must not be `NULL`.
 * \param[out] utxo     Pointer to a location where the function will store a pointer to the retrieved
 *                      \ref cardano_utxo_t structure containing the NFT. Must not be `NULL`.
 *                      On success, the caller is responsible for freeing the returned \ref cardano_utxo_t
 *                      object using the \ref cardano_utxo_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - \ref CARDANO_SUCCESS on success.
 *          - Other error codes for provider-specific failures.
 */
static cardano_error_t
get_unspent_output_by_nft(
  cardano_provider_impl_t* provider_impl,
  cardano_asset_id_t*      asset_id,
  cardano_utxo_t**         utxo)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !asset_id || !utxo)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_get_unspent_output_by_nft(ctx->object_id, asset_id, utxo);
}

/**
 * \brief Resolves unspent transaction outputs (UTXOs) for given transaction inputs.
 *
 * This function obtains the unspent transaction outputs (UTXOs) corresponding to the specified
 * set of transaction inputs. It uses the provided Cardano provider instance to access the blockchain
 * data.
 *
 * \param[in]  provider  Pointer to the \ref cardano_provider_t instance.
 *                       Must not be `NULL`.
 * \param[in]  tx_ins    Pointer to a \ref cardano_transaction_input_set_t containing the set of
 *                       transaction inputs to resolve. Must not be `NULL`.
 * \param[out] utxo_list Pointer to a location where the function will store a pointer to the
 *                       retrieved \ref cardano_utxo_list_t structure containing the resolved UTXOs.
 *                       Must not be `NULL`. On success, the caller is responsible for freeing the
 *                       returned \ref cardano_utxo_list_t object using the \ref cardano_utxo_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 */
static cardano_error_t
resolve_unspent_outputs(
  cardano_provider_impl_t*         provider_impl,
  cardano_transaction_input_set_t* tx_ins,
  cardano_utxo_list_t**            utxo_list)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !tx_ins || !utxo_list)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_resolve_unspent_outputs(ctx->object_id, tx_ins, utxo_list);
}

/**
 * \brief Resolves a Plutus datum from its hash.
 *
 * This function retrieves the Plutus datum associated with the given datum hash.
 * It uses the provided Cardano provider instance to access the blockchain data.
 *
 * \param[in]  provider    Pointer to the \ref cardano_provider_t instance.
 *                         Must not be `NULL`.
 * \param[in]  datum_hash  Pointer to a \ref cardano_blake2b_hash_t representing the hash
 *                         of the datum to resolve. Must not be `NULL`.
 * \param[out] datum       Pointer to a location where the function will store a pointer
 *                         to the retrieved \ref cardano_plutus_data_t structure containing
 *                         the datum. Must not be `NULL`. On success, the caller is responsible
 *                         for freeing the returned \ref cardano_plutus_data_t object using
 *                         the \ref cardano_plutus_data_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 */
static cardano_error_t
resolve_datum(
  cardano_provider_impl_t* provider_impl,
  cardano_blake2b_hash_t*  datum_hash,
  cardano_plutus_data_t**  datum)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !datum_hash || !datum)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_resolve_datum(ctx->object_id, datum_hash, datum);
}

/**
 * \brief Confirms the inclusion of a transaction in the blockchain within a specified timeout period.
 *
 * This function waits for the specified transaction to be confirmed (i.e., included in a block)
 * within a given timeout period. It uses the provided Cardano provider instance to monitor
 * the blockchain for the transaction's confirmation.
 *
 * \param[in] provider    Pointer to the \ref cardano_provider_t instance.
 *                        Must not be `NULL`.
 * \param[in] tx_id       Pointer to a \ref cardano_blake2b_hash_t representing the transaction ID
 *                        to confirm. Must not be `NULL`.
 * \param[in] timeout_ms  Timeout in milliseconds to wait for the transaction confirmation.
 *                        If the transaction is not confirmed within this period, the function will return
 *                        an appropriate error code.
 *  \param[out] confirmed  Pointer to a boolean variable where the function will store the confirmation status.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 */
static cardano_error_t
confirm_transaction(
  cardano_provider_impl_t* provider_impl,
  cardano_blake2b_hash_t*  tx_id,
  uint64_t                 timeout_ms,
  bool*                    confirmed)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !tx_id || !confirmed)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_confirm_transaction(ctx->object_id, tx_id, timeout_ms, confirmed);
}

/**
 * \brief Submits a transaction to the Cardano blockchain network.
 *
 * This function submits the given transaction to the Cardano network for processing.
 * Upon successful submission, it returns the transaction ID (hash) of the submitted transaction.
 * The transaction ID can be used to track the transaction's status and confirmation.
 *
 * \param[in]  provider Pointer to the \ref cardano_provider_t instance.
 *                      Must not be `NULL`.
 * \param[in]  tx       Pointer to the \ref cardano_transaction_t representing the transaction to submit.
 *                      Must not be `NULL`.
 * \param[out] tx_id    Pointer to a location where the function will store a pointer to the
 *                      newly allocated \ref cardano_blake2b_hash_t representing the transaction ID.
 *                      Must not be `NULL`. On success, the caller is responsible for freeing the
 *                      returned \ref cardano_blake2b_hash_t object using the appropriate deallocation function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 */
static cardano_error_t
submit_transaction(
  cardano_provider_impl_t* provider_impl,
  cardano_transaction_t*   tx,
  cardano_blake2b_hash_t** tx_id)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !tx || !tx_id)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_submit_transaction(ctx->object_id, tx, tx_id);
}

/**
 * \brief Evaluates a transaction to estimate the required execution units for Plutus scripts.
 *
 * This function evaluates the given transaction to determine the execution units (e.g., memory and CPU usage)
 * required by any Plutus scripts involved in the transaction. The evaluation considers any additional UTXOs
 * and redeemers provided.
 *
 * \param[in]  provider         Pointer to the \ref cardano_provider_t instance.
 *                              Must not be `NULL`.
 * \param[in]  tx               Pointer to the \ref cardano_transaction_t representing the transaction to evaluate.
 *                              Must not be `NULL`.
 * \param[in]  additional_utxos Pointer to a \ref cardano_utxo_list_t containing additional UTXOs required
 *                              for evaluation. Can be `NULL` if not needed.
 * \param[out] redeemers        Pointer to a location where the function will store a pointer to the
 *                              \ref cardano_redeemer_list_t containing updated redeemers with estimated
 *                              execution units. Must not be `NULL`. On success, the caller is responsible
 *                              for freeing the returned \ref cardano_redeemer_list_t object using the
 *                              \ref cardano_redeemer_list_unref function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 */
static cardano_error_t
evaluate_transaction(
  cardano_provider_impl_t*  provider_impl,
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      additional_utxos,
  cardano_redeemer_list_t** redeemers)
{
  emscripten_provider_context_t* ctx = (emscripten_provider_context_t*)provider_impl->context;

  if (!ctx || !tx || !redeemers)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_provider_bridge_evaluate_transaction(ctx->object_id, tx, additional_utxos, redeemers);
}

/* DEFINITIONS ****************************************************************/

EMSCRIPTEN_KEEPALIVE
cardano_error_t
create_emscripten_provider(
  const cardano_network_magic_t network,
  const char*                   name,
  const size_t                  name_size,
  const uint32_t                object_id,
  cardano_provider_t**          provider)
{
  if ((provider == NULL) || (name == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((name_size > 256U) || (name_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  emscripten_provider_context_t* context = emscripten_provider_context_new(object_id);

  if (context == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(context->name, 256, name, name_size);

  cardano_provider_impl_t impl = { 0 };

  impl.context       = (cardano_object_t*)context;
  impl.network_magic = network;

  impl.get_parameters                 = get_parameters;
  impl.get_unspent_outputs            = get_unspent_outputs;
  impl.get_rewards_balance            = get_rewards_balance;
  impl.get_unspent_outputs_with_asset = get_unspent_outputs_with_asset;
  impl.get_unspent_output_by_nft      = get_unspent_output_by_nft;
  impl.resolve_unspent_outputs        = resolve_unspent_outputs;
  impl.resolve_datum                  = resolve_datum;
  impl.await_transaction_confirmation = confirm_transaction;
  impl.post_transaction_to_chain      = submit_transaction;
  impl.evaluate_transaction           = evaluate_transaction;

  cardano_error_t result = cardano_provider_new(impl, provider);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(context);
    return result;
  }

  return result;
}

#endif /* __EMSCRIPTEN__ */