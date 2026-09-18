/**
 * \file sub_transaction_builder.c
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#include <cardano/transaction_builder/sub_transaction_builder.h>

#include <cardano/object.h>

#include "../allocators.h"
#include "./internals/builder_config.h"
#include "./internals/builder_entities.h"
#include "./internals/builder_inputs.h"
#include "./internals/builder_mint.h"
#include "./internals/builder_outputs.h"
#include "./internals/builder_state.h"
#include "./internals/builder_sub_build.h"
#include "./internals/builder_witnesses.h"

#include <assert.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Type definition for the Cardano Sub Transaction builder.
 *
 * The `cardano_sub_tx_builder_t` type represents an instance of a sub transaction builder for creating
 * CIP-118 sub transactions. It provides methods for adding inputs, outputs and the other elements a sub
 * transaction can carry.
 *
 * A sub transaction is an intent, so the builder never balances it: the result spends exactly the inputs
 * and produces exactly the outputs that were added, and it carries no fee and no collateral.
 */
typedef struct cardano_sub_tx_builder_t
{
    cardano_object_t        base;
    cardano_error_t         last_error;
    cardano_builder_state_t state;
} cardano_sub_tx_builder_t;

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Deallocates a sub_tx_builder object.
 *
 * This function is responsible for properly deallocating a sub_tx_builder object (`cardano_sub_tx_builder_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the sub_tx_builder object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_sub_tx_builder_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the sub_tx_builder
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_sub_tx_builder_deallocate(void* object)
{
  assert(object != NULL);

  cardano_sub_tx_builder_t* builder = (cardano_sub_tx_builder_t*)object;

  cardano_builder_state_release(&builder->state);

  _cardano_free(builder);
}

/**
 * \brief Records the outcome of an internal builder operation in the builder.
 *
 * This function stores the error code of a failed internal operation as the builder deferred error
 * and, when the operation reported an error message, copies it into the builder last error message.
 * A successful outcome leaves the builder untouched.
 *
 * \param[in,out] builder A pointer to the \ref cardano_sub_tx_builder_t that receives the outcome. This
 *                        parameter must not be NULL.
 * \param[in] result The error code returned by the internal operation.
 * \param[in] error_message The error message reported by the internal operation, or NULL when the
 *                          operation did not report one.
 */
static void
track_builder_result(
  cardano_sub_tx_builder_t* builder,
  const cardano_error_t     result,
  const char*               error_message)
{
  if (result != CARDANO_SUCCESS)
  {
    if (error_message != NULL)
    {
      cardano_sub_tx_builder_set_last_error(builder, error_message);
    }

    builder->last_error = result;
  }
}

/* DEFINITIONS ****************************************************************/

cardano_sub_tx_builder_t*
cardano_sub_tx_builder_new(
  cardano_protocol_parameters_t* params,
  const cardano_slot_config_t*   slot_config)
{
  if ((params == NULL) || (slot_config == NULL))
  {
    return NULL;
  }

  cardano_sub_tx_builder_t* builder = _cardano_malloc(sizeof(cardano_sub_tx_builder_t));

  if (builder == NULL)
  {
    return NULL;
  }

  builder->base.ref_count     = 1;
  builder->base.last_error[0] = '\0';
  builder->base.deallocator   = cardano_sub_tx_builder_deallocate;

  builder->last_error = CARDANO_SUCCESS;

  const cardano_error_t result = cardano_builder_state_init(&builder->state, params, slot_config);

  if (result != CARDANO_SUCCESS)
  {
    cardano_sub_tx_builder_unref(&builder);
    return NULL;
  }

  return builder;
}

void
cardano_sub_tx_builder_set_network_id(
  cardano_sub_tx_builder_t*  builder,
  const cardano_network_id_t network_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_network_id(&builder->state, network_id, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_donation(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            donation)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_donation(&builder->state, donation, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_after(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            slot)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_after(&builder->state, slot, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_after_ex(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            unix_time)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_after_ex(&builder->state, unix_time, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_before(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            slot)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_before(&builder->state, slot, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_invalid_before_ex(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            unix_time)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_invalid_before_ex(&builder->state, unix_time, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_input(
  cardano_sub_tx_builder_t* builder,
  cardano_utxo_t*           utxo)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_input(&builder->state, utxo, NULL, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_reference_input(
  cardano_sub_tx_builder_t* builder,
  cardano_utxo_t*           utxo)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_reference_input(&builder->state, utxo, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_output(
  cardano_sub_tx_builder_t*     builder,
  cardano_transaction_output_t* output)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const cardano_error_t result = cardano_builder_add_output(&builder->state, output);

  track_builder_result(builder, result, NULL);
}

void
cardano_sub_tx_builder_send_lovelace(
  cardano_sub_tx_builder_t* builder,
  cardano_address_t*        address,
  const uint64_t            amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_lovelace(&builder->state, address, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_send_lovelace_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               address,
  size_t                    address_size,
  uint64_t                  amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_lovelace_ex(&builder->state, address, address_size, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_send_value(
  cardano_sub_tx_builder_t* builder,
  cardano_address_t*        address,
  cardano_value_t*          value)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_value(&builder->state, address, value, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_send_value_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               address,
  size_t                    address_size,
  cardano_value_t*          value)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_value_ex(&builder->state, address, address_size, value, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_metadata(
  cardano_sub_tx_builder_t* builder,
  const uint64_t            tag,
  cardano_metadatum_t*      metadata)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_metadata(&builder->state, tag, metadata, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_set_metadata_ex(
  cardano_sub_tx_builder_t* builder,
  uint64_t                  tag,
  const char*               metadata_json,
  const size_t              json_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_set_metadata_ex(&builder->state, tag, metadata_json, json_size, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token(
  cardano_sub_tx_builder_t* builder,
  cardano_blake2b_hash_t*   policy_id,
  cardano_asset_name_t*     name,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token(&builder->state, policy_id, name, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               policy_id_hex,
  size_t                    policy_id_size,
  const char*               name_hex,
  size_t                    name_size,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_ex(&builder->state, policy_id_hex, policy_id_size, name_hex, name_size, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token_with_id(
  cardano_sub_tx_builder_t* builder,
  cardano_asset_id_t*       asset_id,
  const int64_t             amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_with_id(&builder->state, asset_id, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_mint_token_with_id_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               asset_id_hex,
  size_t                    hex_size,
  int64_t                   amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_with_id_ex(&builder->state, asset_id_hex, hex_size, amount, NULL, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_script(
  cardano_sub_tx_builder_t* builder,
  cardano_script_t*         script)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_sub_transaction_script(&builder->state, script, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_guard(
  cardano_sub_tx_builder_t* builder,
  cardano_credential_t*     guard)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_guard(&builder->state, guard, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_guard_ex(
  cardano_sub_tx_builder_t*       builder,
  const char*                     hash_hex,
  size_t                          hash_hex_size,
  const cardano_credential_type_t type)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_guard_ex(&builder->state, hash_hex, hash_hex_size, type, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_require_top_level_guard(
  cardano_sub_tx_builder_t* builder,
  cardano_credential_t*     credential,
  cardano_plutus_data_t*    datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_require_top_level_guard(&builder->state, credential, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_require_top_level_guard_ex(
  cardano_sub_tx_builder_t*       builder,
  const char*                     hash_hex,
  size_t                          hash_hex_size,
  const cardano_credential_type_t type,
  cardano_plutus_data_t*          datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_require_top_level_guard_ex(&builder->state, hash_hex, hash_hex_size, type, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_direct_deposit(
  cardano_sub_tx_builder_t* builder,
  cardano_reward_address_t* reward_address,
  const uint64_t            amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_direct_deposit(&builder->state, reward_address, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_direct_deposit_ex(
  cardano_sub_tx_builder_t* builder,
  const char*               reward_address,
  size_t                    address_size,
  const uint64_t            amount)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_direct_deposit_ex(&builder->state, reward_address, address_size, amount, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_account_balance_interval(
  cardano_sub_tx_builder_t*           builder,
  cardano_reward_address_t*           reward_address,
  cardano_account_balance_interval_t* interval)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_account_balance_interval(&builder->state, reward_address, interval, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_sub_tx_builder_add_account_balance_interval_ex(
  cardano_sub_tx_builder_t*           builder,
  const char*                         reward_address,
  size_t                              address_size,
  cardano_account_balance_interval_t* interval)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_account_balance_interval_ex(&builder->state, reward_address, address_size, interval, &error_message);

  track_builder_result(builder, result, error_message);
}

cardano_error_t
cardano_sub_tx_builder_build(
  cardano_sub_tx_builder_t*   builder,
  cardano_sub_transaction_t** sub_transaction)
{
  if (builder == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (builder->last_error != CARDANO_SUCCESS)
  {
    return builder->last_error;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_build_sub_transaction(&builder->state, sub_transaction, &error_message);

  if (result != CARDANO_SUCCESS)
  {
    track_builder_result(builder, result, error_message);

    return result;
  }

  builder->last_error = CARDANO_ERROR_ILLEGAL_STATE;

  return CARDANO_SUCCESS;
}

void
cardano_sub_tx_builder_unref(cardano_sub_tx_builder_t** sub_tx_builder)
{
  if ((sub_tx_builder == NULL) || (*sub_tx_builder == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*sub_tx_builder)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *sub_tx_builder = NULL;
    return;
  }
}

void
cardano_sub_tx_builder_ref(cardano_sub_tx_builder_t* sub_tx_builder)
{
  if (sub_tx_builder == NULL)
  {
    return;
  }

  cardano_object_ref(&sub_tx_builder->base);
}

size_t
cardano_sub_tx_builder_refcount(const cardano_sub_tx_builder_t* sub_tx_builder)
{
  if (sub_tx_builder == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&sub_tx_builder->base);
}

void
cardano_sub_tx_builder_set_last_error(cardano_sub_tx_builder_t* sub_tx_builder, const char* message)
{
  cardano_object_set_last_error(&sub_tx_builder->base, message);
}

const char*
cardano_sub_tx_builder_get_last_error(const cardano_sub_tx_builder_t* sub_tx_builder)
{
  return cardano_object_get_last_error(&sub_tx_builder->base);
}
