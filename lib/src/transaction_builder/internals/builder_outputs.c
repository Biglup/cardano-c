/**
 * \file builder_outputs.c
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

#include "builder_outputs.h"

#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_output_list.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_send_lovelace(
  cardano_builder_state_t* state,
  cardano_address_t*       address,
  const uint64_t           amount,
  const char**             error_message)
{
  if (address == NULL)
  {
    *error_message = "Address is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(address, amount, &output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create output.";
    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  result = cardano_transaction_output_list_add(outputs, output);
  cardano_transaction_output_unref(&output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add output to transaction.";
    cardano_transaction_output_unref(&output);

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_send_lovelace_ex(
  cardano_builder_state_t* state,
  const char*              address,
  size_t                   address_size,
  uint64_t                 amount,
  const char**             error_message)
{
  if ((address == NULL) || (address_size == 0U))
  {
    *error_message = "Address is NULL or empty.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse address.";
    return result;
  }

  result = cardano_builder_send_lovelace(state, addr, amount, error_message);
  cardano_address_unref(&addr);

  return result;
}

cardano_error_t
cardano_builder_send_value(
  cardano_builder_state_t* state,
  cardano_address_t*       address,
  cardano_value_t*         value,
  const char**             error_message)
{
  if (address == NULL)
  {
    *error_message = "Address is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    *error_message = "Value is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(address, 0U, &output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create output.";
    return result;
  }

  result = cardano_transaction_output_set_value(output, value);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set value.";
    cardano_transaction_output_unref(&output);

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  result = cardano_transaction_output_list_add(outputs, output);
  cardano_transaction_output_unref(&output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add output to transaction.";
    cardano_transaction_output_unref(&output);

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_send_value_ex(
  cardano_builder_state_t* state,
  const char*              address,
  size_t                   address_size,
  cardano_value_t*         value,
  const char**             error_message)
{
  if ((address == NULL) || (address_size == 0U))
  {
    *error_message = "Address is NULL or empty.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    *error_message = "Value is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse address.";
    return result;
  }

  result = cardano_builder_send_value(state, addr, value, error_message);
  cardano_address_unref(&addr);

  return result;
}

cardano_error_t
cardano_builder_lock_lovelace(
  cardano_builder_state_t* state,
  cardano_address_t*       script_address,
  const uint64_t           amount,
  cardano_datum_t*         datum,
  const char**             error_message)
{
  if (script_address == NULL)
  {
    *error_message = "Script address is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(script_address, amount, &output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create output.";
    return result;
  }

  result = cardano_transaction_output_set_datum(output, datum);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set datum.";
    cardano_transaction_output_unref(&output);

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  result = cardano_transaction_output_list_add(outputs, output);
  cardano_transaction_output_unref(&output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add output to transaction.";
    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_lock_lovelace_ex(
  cardano_builder_state_t* state,
  const char*              script_address,
  size_t                   script_address_size,
  uint64_t                 amount,
  cardano_datum_t*         datum,
  const char**             error_message)
{
  if ((script_address == NULL) || (script_address_size == 0U))
  {
    *error_message = "Script address is NULL or empty.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(script_address, script_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse script address.";
    return result;
  }

  result = cardano_builder_lock_lovelace(state, addr, amount, datum, error_message);
  cardano_address_unref(&addr);

  return result;
}

cardano_error_t
cardano_builder_lock_value(
  cardano_builder_state_t* state,
  cardano_address_t*       script_address,
  cardano_value_t*         value,
  cardano_datum_t*         datum,
  const char**             error_message)
{
  if (script_address == NULL)
  {
    *error_message = "Script address is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(script_address, 0U, &output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create output.";
    return result;
  }

  result = cardano_transaction_output_set_value(output, value);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set value.";
    cardano_transaction_output_unref(&output);

    return result;
  }

  result = cardano_transaction_output_set_datum(output, datum);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set datum.";
    cardano_transaction_output_unref(&output);

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  result = cardano_transaction_output_list_add(outputs, output);
  cardano_transaction_output_unref(&output);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add output to transaction.";
    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_lock_value_ex(
  cardano_builder_state_t* state,
  const char*              script_address,
  size_t                   script_address_size,
  cardano_value_t*         value,
  cardano_datum_t*         datum,
  const char**             error_message)
{
  if ((script_address == NULL) || (script_address_size == 0U))
  {
    *error_message = "Script address is NULL or empty.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(script_address, script_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse script address.";
    return result;
  }

  result = cardano_builder_lock_value(state, addr, value, datum, error_message);
  cardano_address_unref(&addr);

  return result;
}

cardano_error_t
cardano_builder_add_output(
  cardano_builder_state_t*      state,
  cardano_transaction_output_t* output)
{
  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  return cardano_transaction_output_list_add(outputs, output);
}
