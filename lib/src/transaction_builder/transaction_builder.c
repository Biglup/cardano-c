/**
 * \file transaction_builder.c
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/transaction_builder/transaction_builder.h>

#include "../allocators.h"

#include <assert.h>
#include <cardano/object.h>
#include <cardano/time.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/transaction_builder/evaluation/provider_tx_evaluator.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Type definition for the Cardano Transaction builder.
 *
 * The `cardano_tx_builder_t` type represents an instance of a transaction builder for creating
 * and managing Cardano transactions. It provides methods for adding inputs and outputs, setting
 * fees, and ensuring that the transaction is balanced according to protocol parameters.
 *
 * A transaction builder simplifies the creation of complex transactions by allowing the incremental
 * addition of transaction elements, handling necessary computations such as fee calculations and
 * change outputs, and enforcing protocol compliance.
 */
typedef struct cardano_tx_builder_t
{
    cardano_object_t               base;
    cardano_error_t                last_error;
    cardano_transaction_t*         transaction;
    cardano_protocol_parameters_t* params;
    cardano_provider_t*            provider;
    cardano_coin_selector_t*       coin_selector;
    cardano_tx_evaluator_t*        tx_evaluator;
    cardano_address_t*             change_address;
    cardano_address_t*             collateral_address;
    cardano_utxo_list_t*           available_utxos;
    cardano_utxo_list_t*           collateral_utxos;
    cardano_utxo_list_t*           pre_selected_inputs;
    cardano_utxo_list_t*           reference_inputs;
    bool                           has_plutus_v1;
    bool                           has_plutus_v2;
    bool                           has_plutus_v3;
    size_t                         additional_signature_count;
} cardano_tx_builder_t;

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Deallocates a tx_builder object.
 *
 * This function is responsible for properly deallocating a tx_builder object (`cardano_tx_builder_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the tx_builder object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_tx_builder_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the tx_builder
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_tx_builder_deallocate(void* object)
{
  assert(object != NULL);

  cardano_tx_builder_t* builder = (cardano_tx_builder_t*)object;

  cardano_transaction_unref(&builder->transaction);
  cardano_provider_unref(&builder->provider);
  cardano_protocol_parameters_unref(&builder->params);
  cardano_coin_selector_unref(&builder->coin_selector);
  cardano_tx_evaluator_unref(&builder->tx_evaluator);
  cardano_address_unref(&builder->change_address);
  cardano_address_unref(&builder->collateral_address);
  cardano_utxo_list_unref(&builder->available_utxos);
  cardano_utxo_list_unref(&builder->collateral_utxos);
  cardano_utxo_list_unref(&builder->pre_selected_inputs);
  cardano_utxo_list_unref(&builder->reference_inputs);

  _cardano_free(builder);
}

static cardano_transaction_t*
transaction_new(void)
{
  cardano_transaction_body_t*        body        = NULL;
  cardano_transaction_input_set_t*   inputs      = NULL;
  cardano_transaction_output_list_t* outputs     = NULL;
  cardano_witness_set_t*             witnesses   = NULL;
  cardano_transaction_t*             transaction = NULL;

  cardano_error_t result = cardano_transaction_input_set_new(&inputs);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  result = cardano_transaction_output_list_new(&outputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    return NULL;
  }

  result = cardano_transaction_body_new(inputs, outputs, 0U, NULL, &body);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    cardano_transaction_output_list_unref(&outputs);
    return NULL;
  }

  result = cardano_witness_set_new(&witnesses);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    cardano_transaction_output_list_unref(&outputs);
    cardano_transaction_body_unref(&body);
    return NULL;
  }

  result = cardano_transaction_new(body, witnesses, NULL, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);
    cardano_transaction_output_list_unref(&outputs);
    cardano_transaction_body_unref(&body);
    cardano_witness_set_unref(&witnesses);

    return NULL;
  }

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&witnesses);

  return transaction;
}

/* DEFINITIONS ****************************************************************/

cardano_tx_builder_t*
cardano_tx_builder_new(
  cardano_protocol_parameters_t* params,
  cardano_provider_t*            provider)
{
  if ((params == NULL) || (provider == NULL))
  {
    return NULL;
  }

  cardano_tx_builder_t* builder = _cardano_malloc(sizeof(cardano_tx_builder_t));

  if (builder == NULL)
  {
    return NULL;
  }

  builder->base.ref_count     = 1;
  builder->base.last_error[0] = '\0';
  builder->base.deallocator   = cardano_tx_builder_deallocate;

  builder->last_error          = CARDANO_SUCCESS;
  builder->transaction         = NULL;
  builder->params              = NULL;
  builder->provider            = NULL;
  builder->coin_selector       = NULL;
  builder->tx_evaluator        = NULL;
  builder->change_address      = NULL;
  builder->collateral_address  = NULL;
  builder->available_utxos     = NULL;
  builder->collateral_utxos    = NULL;
  builder->pre_selected_inputs = NULL;
  builder->reference_inputs    = NULL;

  cardano_protocol_parameters_ref(params);
  builder->params = params;

  cardano_provider_ref(provider);
  builder->provider = provider;

  cardano_error_t result = cardano_large_first_coin_selector_new(&builder->coin_selector);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  result = cardano_tx_evaluator_from_provider(provider, &builder->tx_evaluator);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  builder->transaction = transaction_new();

  if (builder->transaction == NULL)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  builder->change_address             = NULL;
  builder->collateral_address         = NULL;
  builder->available_utxos            = NULL;
  builder->collateral_utxos           = NULL;
  builder->pre_selected_inputs        = NULL;
  builder->reference_inputs           = NULL;
  builder->has_plutus_v1              = false;
  builder->has_plutus_v2              = false;
  builder->has_plutus_v3              = false;
  builder->additional_signature_count = 0U;

  result = cardano_utxo_list_new(&builder->pre_selected_inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  result = cardano_utxo_list_new(&builder->reference_inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  return builder;
}

void
cardano_tx_builder_set_coin_selector(
  cardano_tx_builder_t*    builder,
  cardano_coin_selector_t* coin_selector)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (coin_selector == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_coin_selector_ref(coin_selector);
  cardano_coin_selector_unref(&builder->coin_selector);
  builder->coin_selector = coin_selector;
}

void
cardano_tx_builder_set_tx_evaluator(
  cardano_tx_builder_t*   builder,
  cardano_tx_evaluator_t* tx_evaluator)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (tx_evaluator == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_tx_evaluator_ref(tx_evaluator);
  cardano_tx_evaluator_unref(&builder->tx_evaluator);
  builder->tx_evaluator = tx_evaluator;
}

void
cardano_tx_builder_set_network_id(
  cardano_tx_builder_t*      builder,
  const cardano_network_id_t network_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_network_id(body, &network_id);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
}

void
cardano_tx_builder_set_change_address(
  cardano_tx_builder_t* builder,
  cardano_address_t*    change_address)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (change_address == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_ref(change_address);
  cardano_address_unref(&builder->change_address);
  builder->change_address = change_address;
}

void
cardano_tx_builder_set_change_address_ex(
  cardano_tx_builder_t* builder,
  const char*           change_address,
  const size_t          address_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((change_address == NULL) || (address_size == 0U))
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* address = NULL;
  cardano_error_t    result  = cardano_address_from_string(change_address, address_size, &address);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_set_change_address(builder, address);
  cardano_address_unref(&address);
}

void
cardano_tx_builder_set_collateral_change_address(
  cardano_tx_builder_t* builder,
  cardano_address_t*    collateral_change_address)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (collateral_change_address == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_ref(collateral_change_address);
  cardano_address_unref(&builder->collateral_address);
  builder->collateral_address = collateral_change_address;
}

void
cardano_tx_builder_set_collateral_change_address_ex(
  cardano_tx_builder_t* builder,
  const char*           collateral_change_address,
  const size_t          address_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((collateral_change_address == NULL) || (address_size == 0U))
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* address = NULL;
  cardano_error_t    result  = cardano_address_from_string(collateral_change_address, address_size, &address);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_set_collateral_change_address(builder, address);
  cardano_address_unref(&address);
}

void
cardano_tx_builder_set_minimum_fee(
  cardano_tx_builder_t* builder,
  const uint64_t        minimum_fee)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_fee(body, minimum_fee);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
}

void
cardano_tx_builder_set_utxos(
  cardano_tx_builder_t* builder,
  cardano_utxo_list_t*  utxos)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (utxos == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_utxo_list_ref(utxos);
  cardano_utxo_list_unref(&builder->available_utxos);
  builder->available_utxos = utxos;
}

void
cardano_tx_builder_set_collateral_utxos(
  cardano_tx_builder_t* builder,
  cardano_utxo_list_t*  utxos)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (utxos == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_utxo_list_ref(utxos);
  cardano_utxo_list_unref(&builder->collateral_utxos);
  builder->collateral_utxos = utxos;
}

void
cardano_tx_builder_set_invalid_after(
  cardano_tx_builder_t* builder,
  const uint64_t        slot)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_invalid_after(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
}

void
cardano_tx_builder_set_invalid_after_ex(
  cardano_tx_builder_t* builder,
  const uint64_t        unix_time)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_network_magic_t network_magic = cardano_provider_get_network_magic(builder->provider);
  const uint64_t          slot          = cardano_compute_slot_from_unix_time(network_magic, unix_time);
  cardano_error_t         result        = cardano_transaction_body_set_invalid_after(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
}

void
cardano_tx_builder_set_invalid_before(
  cardano_tx_builder_t* builder,
  const uint64_t        slot)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_invalid_before(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
}

void
cardano_tx_builder_set_invalid_before_ex(
  cardano_tx_builder_t* builder,
  const uint64_t        unix_time)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_network_magic_t network_magic = cardano_provider_get_network_magic(builder->provider);
  const uint64_t          slot          = cardano_compute_slot_from_unix_time(network_magic, unix_time);
  cardano_error_t         result        = cardano_transaction_body_set_invalid_before(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
}

void
cardano_tx_builder_add_reference_input(
  cardano_tx_builder_t* builder,
  cardano_utxo_t*       utxo)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (utxo == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_reference_inputs(body);

  if (inputs == NULL)
  {
    cardano_error_t result = cardano_transaction_input_set_new(&inputs);

    if (result != CARDANO_SUCCESS)
    {
      builder->last_error = result;
      return;
    }

    result = cardano_transaction_body_set_reference_inputs(body, inputs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_input_set_unref(&inputs);
      builder->last_error = result;

      return;
    }
  }

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&input);

  if (input == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_error_t result = cardano_transaction_input_set_add(inputs, input);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return;
  }

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&output);

  if (output == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_script_t* script = cardano_transaction_output_get_script_ref(output);
  cardano_script_unref(&script);

  if (script != NULL)
  {
    cardano_script_language_t language;
    result = cardano_script_get_language(script, &language);

    if (result != CARDANO_SUCCESS)
    {
      builder->last_error = result;
      return;
    }

    switch (language)
    {
      case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
      {
        builder->has_plutus_v1 = true;
        break;
      }
      case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
      {
        builder->has_plutus_v2 = true;
        break;
      }
      case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
      {
        builder->has_plutus_v3 = true;
        break;
      }
      default:
      {
        break;
      }
    }
  }

  result = cardano_utxo_list_add(builder->reference_inputs, utxo);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
}

void
cardano_tx_builder_send_lovelace(
  cardano_tx_builder_t* builder,
  cardano_address_t*    address,
  const uint64_t        amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (address == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(address, amount, &output);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  result = cardano_transaction_output_list_add(outputs, output);
  cardano_transaction_output_unref(&output);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    cardano_transaction_output_unref(&output);
    return;
  }
}

void
cardano_tx_builder_send_lovelace_ex(
  cardano_tx_builder_t* builder,
  const char*           address,
  size_t                address_size,
  uint64_t              amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((address == NULL) || (address_size == 0U))
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_send_lovelace(builder, addr, amount);
  cardano_address_unref(&addr);
}

void
cardano_tx_builder_send_value(
  cardano_tx_builder_t* builder,
  cardano_address_t*    address,
  cardano_value_t*      value)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((address == NULL) || (value == NULL))
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(address, 0U, &output);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return;
  }

  result = cardano_transaction_output_set_value(output, value);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    cardano_transaction_output_unref(&output);
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  result = cardano_transaction_output_list_add(outputs, output);
  cardano_transaction_output_unref(&output);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    cardano_transaction_output_unref(&output);
    return;
  }
}

void
cardano_tx_builder_send_value_ex(
  cardano_tx_builder_t* builder,
  const char*           address,
  size_t                address_size,
  cardano_value_t*      value)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((address == NULL) || (address_size == 0U) || (value == NULL))
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_send_value(builder, addr, value);
  cardano_address_unref(&addr);
}

void
cardano_tx_builder_lock_lovelace(
  cardano_tx_builder_t* builder,
  cardano_address_t*    script_address,
  uint64_t              amount,
  cardano_datum_t*      datum)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(script_address);
  CARDANO_UNUSED(amount);
  CARDANO_UNUSED(datum);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_lock_lovelace_ex(
  cardano_tx_builder_t* builder,
  const char*           script_address,
  size_t                script_address_size,
  uint64_t              amount,
  cardano_datum_t*      datum)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(script_address);
  CARDANO_UNUSED(script_address_size);
  CARDANO_UNUSED(amount);
  CARDANO_UNUSED(datum);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_lock_value(
  cardano_tx_builder_t* builder,
  cardano_address_t*    script_address,
  cardano_value_t*      value,
  cardano_datum_t*      datum)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(script_address);
  CARDANO_UNUSED(value);
  CARDANO_UNUSED(datum);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_lock_value_ex(
  cardano_tx_builder_t* builder,
  const char*           script_address,
  size_t                script_address_size,
  cardano_value_t*      value,
  cardano_datum_t*      datum)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(script_address);
  CARDANO_UNUSED(script_address_size);
  CARDANO_UNUSED(value);
  CARDANO_UNUSED(datum);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_add_input(
  cardano_tx_builder_t*  builder,
  cardano_utxo_t*        utxo,
  cardano_plutus_data_t* redeemer,
  cardano_plutus_data_t* datum)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(utxo);
  CARDANO_UNUSED(redeemer);
  CARDANO_UNUSED(datum);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_add_output(
  cardano_tx_builder_t*         builder,
  cardano_transaction_output_t* output)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(output);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_set_metadata(
  cardano_tx_builder_t* builder,
  uint64_t              tag,
  cardano_metadatum_t*  metadata)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(tag);
  CARDANO_UNUSED(metadata);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_set_metadata_ex(
  cardano_tx_builder_t* builder,
  uint64_t              tag,
  const char*           metadata_json,
  size_t                json_size)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(tag);
  CARDANO_UNUSED(metadata_json);
  CARDANO_UNUSED(json_size);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_mint_token(
  cardano_tx_builder_t*   builder,
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   name,
  int64_t                 amount,
  cardano_plutus_data_t*  redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(policy_id);
  CARDANO_UNUSED(name);
  CARDANO_UNUSED(amount);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_mint_token_ex(
  cardano_tx_builder_t*  builder,
  const char*            policy_id_hex,
  size_t                 policy_id_size,
  const char*            name_hex,
  size_t                 name_size,
  int64_t                amount,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(policy_id_hex);
  CARDANO_UNUSED(policy_id_size);
  CARDANO_UNUSED(name_hex);
  CARDANO_UNUSED(name_size);
  CARDANO_UNUSED(amount);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_mint_token_with_id(
  cardano_tx_builder_t*  builder,
  cardano_asset_id_t*    asset_id,
  int64_t                amount,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(asset_id);
  CARDANO_UNUSED(amount);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_mint_token_with_id_ex(
  cardano_tx_builder_t*  builder,
  const char*            asset_id_hex,
  size_t                 hex_size,
  int64_t                amount,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(asset_id_hex);
  CARDANO_UNUSED(hex_size);
  CARDANO_UNUSED(amount);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_add_mint(
  cardano_tx_builder_t*  builder,
  cardano_multi_asset_t* tokens,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(tokens);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_pad_signer_count(
  cardano_tx_builder_t* builder,
  const size_t          count)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  builder->additional_signature_count = count;
}

void
cardano_tx_builder_add_signer(
  cardano_tx_builder_t*   builder,
  cardano_blake2b_hash_t* pub_key_hash)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(pub_key_hash);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_add_signer_ex(
  cardano_tx_builder_t* builder,
  const char*           pub_key_hash,
  size_t                hash_size)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(pub_key_hash);
  CARDANO_UNUSED(hash_size);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_add_datum(
  cardano_tx_builder_t*  builder,
  cardano_plutus_data_t* datum)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(datum);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_withdraw_rewards(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(address);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_withdraw_rewards_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(reward_address);
  CARDANO_UNUSED(address_size);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_register_reward_address(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(address);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_register_reward_address_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(reward_address);
  CARDANO_UNUSED(address_size);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_deregister_reward_address(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(address);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_deregister_reward_address_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(reward_address);
  CARDANO_UNUSED(address_size);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_delegate_stake(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_blake2b_hash_t*   pool_id,
  cardano_plutus_data_t*    redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(address);
  CARDANO_UNUSED(pool_id);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_delegate_stake_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  const char*            pool_id,
  size_t                 pool_id_size,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(reward_address);
  CARDANO_UNUSED(address_size);
  CARDANO_UNUSED(pool_id);
  CARDANO_UNUSED(pool_id_size);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_delegate_voting_power(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_drep_t*           drep,
  cardano_plutus_data_t*    redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(address);
  CARDANO_UNUSED(drep);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_delegate_voting_power_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(reward_address);
  CARDANO_UNUSED(address_size);
  CARDANO_UNUSED(drep_id);
  CARDANO_UNUSED(drep_id_size);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_register_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(drep);
  CARDANO_UNUSED(anchor);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_register_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(drep_id);
  CARDANO_UNUSED(drep_id_size);
  CARDANO_UNUSED(anchor);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_update_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(drep);
  CARDANO_UNUSED(anchor);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_update_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(drep_id);
  CARDANO_UNUSED(drep_id_size);
  CARDANO_UNUSED(anchor);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_deregister_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(drep);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_deregister_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(drep_id);
  CARDANO_UNUSED(drep_id_size);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_vote(
  cardano_tx_builder_t*           builder,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_plutus_data_t*          redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(voter);
  CARDANO_UNUSED(action_id);
  CARDANO_UNUSED(vote);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_add_certificate(
  cardano_tx_builder_t*  builder,
  cardano_certificate_t* certificate,
  cardano_plutus_data_t* redeemer)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(certificate);
  CARDANO_UNUSED(redeemer);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

void
cardano_tx_builder_add_script(
  cardano_tx_builder_t* builder,
  cardano_script_t*     script)
{
  CARDANO_UNUSED(builder);
  CARDANO_UNUSED(script);

  builder->last_error = CARDANO_ERROR_NOT_IMPLEMENTED;
}

cardano_error_t
cardano_tx_builder_build(
  cardano_tx_builder_t*   builder,
  cardano_transaction_t** transaction)
{
  if (builder == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (builder->last_error != CARDANO_SUCCESS)
  {
    return builder->last_error;
  }

  if (builder->change_address == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_tx_builder_set_last_error(
      builder,
      "You must set a change address by calling `cardano_tx_builder_set_change_address` before calling `cardano_tx_builder_build`.");

    return builder->last_error;
  }

  if (builder->available_utxos == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_tx_builder_set_last_error(
      builder,
      "You must set the available UTXOs for input selection by calling `cardano_tx_builder_set_utxos` before calling `cardano_tx_builder_build`.");

    return builder->last_error;
  }

  cardano_transaction_t* tx = builder->transaction;

  cardano_error_t result = cardano_balance_transaction(
    tx,
    builder->additional_signature_count,
    builder->params,
    builder->reference_inputs,
    builder->pre_selected_inputs,
    builder->available_utxos,
    builder->coin_selector,
    builder->change_address,
    builder->tx_evaluator);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return result;
  }

  cardano_transaction_ref(tx);
  *transaction = tx;

  return builder->last_error;
}

void
cardano_tx_builder_unref(cardano_tx_builder_t** tx_builder)
{
  if ((tx_builder == NULL) || (*tx_builder == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*tx_builder)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *tx_builder = NULL;
    return;
  }
}

void
cardano_tx_builder_ref(cardano_tx_builder_t* tx_builder)
{
  if (tx_builder == NULL)
  {
    return;
  }

  cardano_object_ref(&tx_builder->base);
}

size_t
cardano_tx_builder_refcount(const cardano_tx_builder_t* tx_builder)
{
  if (tx_builder == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&tx_builder->base);
}

void
cardano_tx_builder_set_last_error(cardano_tx_builder_t* tx_builder, const char* message)
{
  cardano_object_set_last_error(&tx_builder->base, message);
}

const char*
cardano_tx_builder_get_last_error(const cardano_tx_builder_t* tx_builder)
{
  return cardano_object_get_last_error(&tx_builder->base);
}