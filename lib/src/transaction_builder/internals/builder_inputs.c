/**
 * \file builder_inputs.c
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

/* INCLUDES ******************************************************************/

#include "builder_inputs.h"

#include <cardano/common/ex_units.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "builder_redeemers.h"

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_add_input(
  cardano_builder_state_t* state,
  cardano_utxo_t*          utxo,
  cardano_plutus_data_t*   redeemer,
  cardano_plutus_data_t*   datum,
  const char**             error_message)
{
  if (utxo == NULL)
  {
    *error_message = "UTXO is required";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&output);

  cardano_address_t* address = cardano_transaction_output_get_address(output);
  cardano_address_unref(&address);

  cardano_address_type_t address_type;

  cardano_error_t result = cardano_address_get_type(address, &address_type);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to get address type";
    return result;
  }

  result = cardano_utxo_list_add(state->pre_selected_inputs, utxo);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add UTXO to pre-selected inputs";

    return result;
  }

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);

  if (redeemers == NULL)
  {
    result = cardano_redeemer_list_new(&redeemers);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create redeemers list";
      return result;
    }

    result = cardano_witness_set_set_redeemers(witnesses, redeemers);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set redeemers list";
      cardano_redeemer_list_unref(&redeemers);
      return result;
    }
  }

  cardano_redeemer_list_unref(&redeemers);

  if (redeemer != NULL)
  {
    cardano_redeemer_t* rdmer = NULL;

    cardano_ex_units_t* ex_units = NULL;
    result                       = cardano_ex_units_new(0, 0, &ex_units);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create execution units";
      return result;
    }

    result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, redeemer, ex_units, &rdmer);
    cardano_ex_units_unref(&ex_units);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create redeemer";
      return result;
    }

    result = cardano_redeemer_list_add(redeemers, rdmer);
    cardano_redeemer_unref(&rdmer);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to add redeemer to list";
      return result;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    result = cardano_input_to_redeemer_map_insert(state->input_to_redeemer_map, input, rdmer);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to insert input to redeemer map";
      return result;
    }
  }

  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witnesses);

  if (datums == NULL)
  {
    result = cardano_plutus_data_set_new(&datums);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create datums set";
      return result;
    }

    result = cardano_witness_set_set_plutus_data(witnesses, datums);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set datums set";
      cardano_plutus_data_set_unref(&datums);
      return result;
    }
  }

  cardano_plutus_data_set_unref(&datums);

  if (datum != NULL)
  {
    result = cardano_plutus_data_set_add(datums, datum);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to add datum to set";
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_add_input_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_utxo_t*                utxo,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  cardano_plutus_data_t*         datum,
  const char**                   error_message)
{
  if ((utxo == NULL) || (callback == NULL))
  {
    *error_message = "UTXO and callback are required";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* placeholder = NULL;

  cardano_error_t result = cardano_builder_create_placeholder_plutus_data(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the placeholder redeemer.";

    return result;
  }

  result = cardano_builder_add_input(state, utxo, placeholder, datum, error_message);

  cardano_plutus_data_unref(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&input);

  cardano_redeemer_t* redeemer = NULL;

  result = cardano_input_to_redeemer_map_get(state->input_to_redeemer_map, input, &redeemer);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_deferred_redeemer_list_add(state->deferred_redeemers, redeemer, callback, user_context);
  }

  cardano_redeemer_unref(&redeemer);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to register the deferred redeemer.";
  }

  return result;
}

cardano_error_t
cardano_builder_add_reference_input(
  cardano_builder_state_t* state,
  cardano_utxo_t*          utxo,
  const char**             error_message)
{
  if (utxo == NULL)
  {
    *error_message = "UTXO is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_reference_inputs(body);

  if (inputs == NULL)
  {
    cardano_error_t result = cardano_transaction_input_set_new(&inputs);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create reference inputs.";
      return result;
    }

    result = cardano_transaction_body_set_reference_inputs(body, inputs);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set reference inputs.";
      cardano_transaction_input_set_unref(&inputs);

      return result;
    }
  }

  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&input);

  cardano_error_t result = cardano_transaction_input_set_add(inputs, input);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add input to reference inputs.";
    return result;
  }

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&output);

  if (output == NULL)
  {
    *error_message = "Output is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_t* script = cardano_transaction_output_get_script_ref(output);
  cardano_script_unref(&script);

  if (script != NULL)
  {
    cardano_script_language_t language;
    result = cardano_script_get_language(script, &language);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to get script language.";
      return result;
    }

    switch (language)
    {
      case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
      {
        state->has_plutus_v1 = true;
        break;
      }
      case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
      {
        state->has_plutus_v2 = true;
        break;
      }
      case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
      {
        state->has_plutus_v3 = true;
        break;
      }
      default:
      {
        break;
      }
    }
  }

  result = cardano_utxo_list_add(state->reference_inputs, utxo);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add UTXO to reference inputs.";
  }

  return result;
}
