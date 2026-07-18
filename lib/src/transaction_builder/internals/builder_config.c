/**
 * \file builder_config.c
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

#include "builder_config.h"

#include <cardano/auxiliary_data/auxiliary_data.h>
#include <cardano/auxiliary_data/transaction_metadata.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/time.h>
#include <cardano/transaction_body/transaction_body.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_set_coin_selector(
  cardano_builder_state_t* state,
  cardano_coin_selector_t* coin_selector,
  const char**             error_message)
{
  if (coin_selector == NULL)
  {
    *error_message = "Coin selector is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_coin_selector_ref(coin_selector);
  cardano_coin_selector_unref(&state->coin_selector);
  state->coin_selector = coin_selector;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_set_tx_evaluator(
  cardano_builder_state_t* state,
  cardano_tx_evaluator_t*  tx_evaluator,
  const char**             error_message)
{
  if (tx_evaluator == NULL)
  {
    *error_message = "Transaction evaluator is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_tx_evaluator_ref(tx_evaluator);
  cardano_tx_evaluator_unref(&state->tx_evaluator);
  state->tx_evaluator = tx_evaluator;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_set_network_id(
  cardano_builder_state_t*   state,
  const cardano_network_id_t network_id,
  const char**               error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    *error_message = "Transaction body is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_transaction_body_set_network_id(body, &network_id);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set network ID.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_change_address(
  cardano_builder_state_t* state,
  cardano_address_t*       change_address,
  const char**             error_message)
{
  if (change_address == NULL)
  {
    *error_message = "Change address is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_ref(change_address);
  cardano_address_unref(&state->change_address);
  state->change_address = change_address;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_set_change_address_ex(
  cardano_builder_state_t* state,
  const char*              change_address,
  const size_t             address_size,
  const char**             error_message)
{
  if ((change_address == NULL) || (address_size == 0U))
  {
    *error_message = "Change address is NULL or empty.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* address = NULL;
  cardano_error_t    result  = cardano_address_from_string(change_address, address_size, &address);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse change address.";
    return result;
  }

  result = cardano_builder_set_change_address(state, address, error_message);
  cardano_address_unref(&address);

  return result;
}

cardano_error_t
cardano_builder_set_collateral_change_address(
  cardano_builder_state_t* state,
  cardano_address_t*       collateral_change_address,
  const char**             error_message)
{
  if (collateral_change_address == NULL)
  {
    *error_message = "Collateral change address is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_ref(collateral_change_address);
  cardano_address_unref(&state->collateral_address);
  state->collateral_address = collateral_change_address;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_set_collateral_change_address_ex(
  cardano_builder_state_t* state,
  const char*              collateral_change_address,
  const size_t             address_size,
  const char**             error_message)
{
  if ((collateral_change_address == NULL) || (address_size == 0U))
  {
    *error_message = "Collateral change address is NULL or empty.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_t* address = NULL;
  cardano_error_t    result  = cardano_address_from_string(collateral_change_address, address_size, &address);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse collateral change address.";
    return result;
  }

  result = cardano_builder_set_collateral_change_address(state, address, error_message);
  cardano_address_unref(&address);

  return result;
}

cardano_error_t
cardano_builder_set_minimum_fee(
  cardano_builder_state_t* state,
  const uint64_t           minimum_fee,
  const char**             error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    *error_message = "Transaction body is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_transaction_body_set_fee(body, minimum_fee);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set fee.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_donation(
  cardano_builder_state_t* state,
  const uint64_t           donation,
  const char**             error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    *error_message = "Transaction body is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (donation == 0U)
  {
    result = cardano_transaction_body_set_donation(body, NULL);
  }
  else
  {
    result = cardano_transaction_body_set_donation(body, &donation);
  }

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set donation.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_utxos(
  cardano_builder_state_t* state,
  cardano_utxo_list_t*     utxos,
  const char**             error_message)
{
  if (utxos == NULL)
  {
    *error_message = "UTXOs list is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_utxo_list_ref(utxos);
  cardano_utxo_list_unref(&state->available_utxos);
  state->available_utxos = utxos;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_set_collateral_utxos(
  cardano_builder_state_t* state,
  cardano_utxo_list_t*     utxos,
  const char**             error_message)
{
  if (utxos == NULL)
  {
    *error_message = "Collateral UTXOs list is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_utxo_list_ref(utxos);
  cardano_utxo_list_unref(&state->collateral_utxos);
  state->collateral_utxos = utxos;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_set_invalid_after(
  cardano_builder_state_t* state,
  const uint64_t           slot,
  const char**             error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    *error_message = "Transaction body is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_transaction_body_set_invalid_after(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set invalid after.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_invalid_after_ex(
  cardano_builder_state_t* state,
  const uint64_t           unix_time,
  const char**             error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    *error_message = "Transaction body is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const uint64_t  slot   = unix_time_to_enclosing_slot(unix_time * 1000U, &state->slot_config);
  cardano_error_t result = cardano_transaction_body_set_invalid_after(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set invalid after.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_invalid_before(
  cardano_builder_state_t* state,
  const uint64_t           slot,
  const char**             error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    *error_message = "Transaction body is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_transaction_body_set_invalid_before(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set invalid before.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_invalid_before_ex(
  cardano_builder_state_t* state,
  const uint64_t           unix_time,
  const char**             error_message)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    *error_message = "Transaction body is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const uint64_t  slot   = unix_time_to_enclosing_slot(unix_time * 1000U, &state->slot_config);
  cardano_error_t result = cardano_transaction_body_set_invalid_before(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set invalid before.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_metadata(
  cardano_builder_state_t* state,
  const uint64_t           tag,
  cardano_metadatum_t*     metadata,
  const char**             error_message)
{
  if (metadata == NULL)
  {
    *error_message = "Metadata is NULL.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_auxiliary_data_t* auxiliary_data = cardano_transaction_get_auxiliary_data(state->transaction);

  if (auxiliary_data == NULL)
  {
    cardano_error_t result = cardano_auxiliary_data_new(&auxiliary_data);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create auxiliary data.";
      return result;
    }

    result = cardano_transaction_set_auxiliary_data(state->transaction, auxiliary_data);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set auxiliary data.";
      cardano_auxiliary_data_unref(&auxiliary_data);
      return result;
    }
  }

  cardano_auxiliary_data_unref(&auxiliary_data);

  cardano_transaction_metadata_t* tx_metadata = cardano_auxiliary_data_get_transaction_metadata(auxiliary_data);

  if (tx_metadata == NULL)
  {
    cardano_error_t result = cardano_transaction_metadata_new(&tx_metadata);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create transaction metadata.";
      return result;
    }

    result = cardano_auxiliary_data_set_transaction_metadata(auxiliary_data, tx_metadata);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set transaction metadata.";
      cardano_transaction_metadata_unref(&tx_metadata);
      return result;
    }
  }

  cardano_transaction_metadata_unref(&tx_metadata);

  cardano_error_t result = cardano_transaction_metadata_insert(tx_metadata, tag, metadata);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to insert metadata.";

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_t* hash = cardano_auxiliary_data_get_hash(auxiliary_data);

  result = cardano_transaction_body_set_aux_data_hash(body, hash);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set auxiliary data hash.";
  }

  return result;
}

cardano_error_t
cardano_builder_set_metadata_ex(
  cardano_builder_state_t* state,
  const uint64_t           tag,
  const char*              metadata_json,
  const size_t             json_size,
  const char**             error_message)
{
  if ((metadata_json == NULL) || (json_size == 0U))
  {
    *error_message = "Metadata is NULL or empty.";
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_t* metadata = NULL;
  cardano_error_t      result   = cardano_metadatum_from_json(metadata_json, json_size, &metadata);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse metadata.";
    return result;
  }

  result = cardano_builder_set_metadata(state, tag, metadata, error_message);
  cardano_metadatum_unref(&metadata);

  return result;
}
