/**
 * \file builder_mint.c
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

#include "builder_mint.h"

#include <cardano/assets/multi_asset.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/witness_set/witness_set.h>

#include "builder_redeemers.h"

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_mint_token(
  cardano_builder_state_t* state,
  cardano_blake2b_hash_t*  policy_id,
  cardano_asset_name_t*    name,
  const int64_t            amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if (policy_id == NULL)
  {
    *error_message = "Policy ID is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (name == NULL)
  {
    *error_message = "Asset name is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* tokens = cardano_transaction_body_get_mint(body);

  if (tokens == NULL)
  {
    cardano_error_t result = cardano_multi_asset_new(&tokens);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create multi-asset.";

      return result;
    }

    result = cardano_transaction_body_set_mint(body, tokens);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set multi-asset.";
      cardano_multi_asset_unref(&tokens);

      return result;
    }
  }

  cardano_multi_asset_unref(&tokens);

  cardano_error_t result = cardano_multi_asset_set(tokens, policy_id, name, amount);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set multi-asset.";

    return result;
  }

  if (redeemer != NULL)
  {
    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
    cardano_witness_set_unref(&witnesses);

    result = cardano_builder_add_redeemer(witnesses, policy_id, redeemer, CARDANO_REDEEMER_TAG_MINT, state, error_message);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to add redeemer.";

      return result;
    }
  }
  else
  {
    result = cardano_blake2b_hash_to_redeemer_map_insert(state->mints_to_redeemer_map, policy_id, NULL);

    if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
    {
      *error_message = "Failed to add mint.";

      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_mint_token_ex(
  cardano_builder_state_t* state,
  const char*              policy_id_hex,
  size_t                   policy_id_size,
  const char*              name_hex,
  size_t                   name_size,
  const int64_t            amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((policy_id_hex == NULL) || (policy_id_size == 0U))
  {
    *error_message = "Policy ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((name_hex == NULL) || (name_size == 0U))
  {
    *error_message = "Asset name is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_t* policy_id = NULL;
  cardano_error_t         result    = cardano_blake2b_hash_from_hex(policy_id_hex, policy_id_size, &policy_id);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse policy ID.";

    return result;
  }

  cardano_asset_name_t* name = NULL;
  result                     = cardano_asset_name_from_hex(name_hex, name_size, &name);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&policy_id);
    *error_message = "Failed to parse asset name.";

    return result;
  }

  result = cardano_builder_mint_token(state, policy_id, name, amount, redeemer, error_message);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);

  return result;
}

cardano_error_t
cardano_builder_mint_token_with_id(
  cardano_builder_state_t* state,
  cardano_asset_id_t*      asset_id,
  const int64_t            amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if (asset_id == NULL)
  {
    *error_message = "Asset ID is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_t* policy_id = cardano_asset_id_get_policy_id(asset_id);
  cardano_asset_name_t*   name      = cardano_asset_id_get_asset_name(asset_id);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);

  return cardano_builder_mint_token(state, policy_id, name, amount, redeemer, error_message);
}

cardano_error_t
cardano_builder_mint_token_with_id_ex(
  cardano_builder_state_t* state,
  const char*              asset_id_hex,
  size_t                   hex_size,
  int64_t                  amount,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if ((asset_id_hex == NULL) || (hex_size == 0U))
  {
    *error_message = "Asset ID is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_id_t* asset_id = NULL;
  cardano_error_t     result   = cardano_asset_id_from_hex(asset_id_hex, hex_size, &asset_id);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse asset ID.";

    return result;
  }

  result = cardano_builder_mint_token_with_id(state, asset_id, amount, redeemer, error_message);
  cardano_asset_id_unref(&asset_id);

  return result;
}

cardano_error_t
cardano_builder_mint_token_with_deferred_redeemer(
  cardano_builder_state_t*       state,
  cardano_blake2b_hash_t*        policy_id,
  cardano_asset_name_t*          name,
  const int64_t                  amount,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  const char**                   error_message)
{
  if ((policy_id == NULL) || (callback == NULL))
  {
    *error_message = "Policy id and callback are required";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* placeholder = NULL;

  cardano_error_t result = cardano_builder_create_placeholder_plutus_data(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the placeholder redeemer.";

    return result;
  }

  result = cardano_builder_mint_token(state, policy_id, name, amount, placeholder, error_message);

  cardano_plutus_data_unref(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_builder_register_deferred_from_map(state, state->mints_to_redeemer_map, policy_id, callback, user_context, error_message);
}
