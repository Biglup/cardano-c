/**
 * \file transaction_builder.c
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
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

#include <cardano/transaction_builder/transaction_builder.h>

#include <cardano/object.h>
#include <cardano/time.h>
#include <cardano/transaction_builder/balancing/input_to_redeemer_map.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/script_data_hash.h>
#include <cardano/witness_set/redeemer.h>

#include "../allocators.h"
#include "./internals/blake2b_hash_to_redeemer_map.h"
#include "./internals/builder_certs.h"
#include "./internals/builder_inputs.h"
#include "./internals/builder_mint.h"
#include "./internals/builder_outputs.h"
#include "./internals/builder_proposals.h"
#include "./internals/builder_redeemers.h"
#include "./internals/builder_state.h"
#include "./internals/builder_votes.h"
#include "./internals/builder_withdrawals.h"

#include <assert.h>

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
    cardano_object_t        base;
    cardano_error_t         last_error;
    cardano_builder_state_t state;
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
 * \param[in,out] builder A pointer to the \ref cardano_tx_builder_t that receives the outcome. This
 *                        parameter must not be NULL.
 * \param[in] result The error code returned by the internal operation.
 * \param[in] error_message The error message reported by the internal operation, or NULL when the
 *                          operation did not report one.
 */
static void
track_builder_result(
  cardano_tx_builder_t* builder,
  const cardano_error_t result,
  const char*           error_message)
{
  if (result != CARDANO_SUCCESS)
  {
    if (error_message != NULL)
    {
      cardano_tx_builder_set_last_error(builder, error_message);
    }

    builder->last_error = result;
  }
}

/**
 * \brief Sets a dummy script data hash in the transaction.
 *
 * This function assigns a placeholder script data hash to the specified transaction for fee calculation
 * purposes when the transaction includes script data.
 *
 * \param[in,out] tx A pointer to a \ref cardano_transaction_t object representing the transaction to which
 *                   the dummy script data hash will be set. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the dummy
 *         script data hash was successfully set, or an appropriate error code indicating the failure reason.
 */
static cardano_error_t
set_dummy_script_data_hash(cardano_transaction_t* tx)
{
  if (!cardano_transaction_has_script_data(tx))
  {
    return CARDANO_SUCCESS;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  static byte_t           dummy_hash[32] = { 0 };
  cardano_blake2b_hash_t* hash           = NULL;

  cardano_error_t new_hash_result = cardano_blake2b_hash_from_bytes(dummy_hash, sizeof(dummy_hash), &hash);

  if (new_hash_result != CARDANO_SUCCESS)
  {
    return new_hash_result;
  }

  new_hash_result = cardano_transaction_body_set_script_data_hash(body, hash);
  cardano_blake2b_hash_unref(&hash);

  if (new_hash_result != CARDANO_SUCCESS)
  {
    return new_hash_result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the cost model for a specified Plutus language version.
 *
 * This function searches for the cost model matching the specified Plutus language version within a
 * provided set of source cost models and, if found, copies it into a destination cost model set.
 *
 * \param[in] source_models A pointer to the \ref cardano_costmdls_t object containing the source cost models.
 * \param[out] dest_models A pointer to the \ref cardano_costmdls_t object where the matching cost model
 *                         for the specified version will be copied. This parameter must not be NULL.
 * \param[in] version The Plutus language version of the cost model to retrieve.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the cost model
 *         was successfully found and copied, or an appropriate error code if an internal error occurs.
 */
static cardano_error_t
get_cost_model_for_version(
  cardano_costmdls_t*                     source_models,
  cardano_costmdls_t*                     dest_models,
  const cardano_plutus_language_version_t version)
{
  assert(source_models != NULL);
  assert(dest_models != NULL);

  cardano_cost_model_t* cost_model = NULL;

  cardano_error_t result = cardano_costmdls_get(source_models, version, &cost_model);
  cardano_cost_model_unref(&cost_model);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_costmdls_insert(dest_models, cost_model);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the cost models flagged by the the transaction builder.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance from which the cost models will be retrieved.
 * \param[out] costmdls A pointer to an initialized \ref cardano_costmdls_t object where the cost models will be stored.
 *                      This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the cost models
 *         were successfully retrieved and stored, or an appropriate error code if an error occurs during retrieval.
 */
static cardano_error_t
get_cost_models(cardano_tx_builder_t* builder, cardano_costmdls_t* costmdls)
{
  cardano_costmdls_t* pparams_costmdls = cardano_protocol_parameters_get_cost_models(builder->state.params);
  cardano_costmdls_unref(&pparams_costmdls);

  cardano_error_t result = CARDANO_SUCCESS;

  if (builder->state.has_plutus_v1)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (builder->state.has_plutus_v2)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V2);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (builder->state.has_plutus_v3)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V3);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

/**
 * \brief Computes and updates the script data hash in a transaction.
 *
 * This function calculates the script data hash based on the redeemers, datums, and cost models
 * associated with the transaction in the \ref cardano_tx_builder_t instance, then updates this
 * hash in the provided \ref cardano_transaction_t.
 *
 * \param[in] builder A pointer to the \ref cardano_tx_builder_t instance containing the necessary
 *                    transaction data, including redeemers, datums, and cost models.
 * \param[in,out] tx A pointer to an initialized \ref cardano_transaction_t object. This transaction
 *                   will be updated with the computed script data hash.
 *
 * \return \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script data hash was successfully computed and updated in the transaction, or an
 *         appropriate error code if the computation or update fails.
 */
static cardano_error_t
update_script_data_hash(cardano_tx_builder_t* builder, cardano_transaction_t* tx)
{
  if (!cardano_transaction_has_script_data(tx))
  {
    return CARDANO_SUCCESS;
  }

  cardano_costmdls_t* costmdls = NULL;

  cardano_error_t result = cardano_costmdls_new(&costmdls);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = get_cost_models(builder, costmdls);

  if (result != CARDANO_SUCCESS)
  {
    cardano_costmdls_unref(&costmdls);
    return result;
  }

  cardano_blake2b_hash_t* hash = NULL;

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witness_set);
  cardano_plutus_data_set_unref(&datums);

  result = cardano_compute_script_data_hash(costmdls, redeemers, datums, &hash);
  cardano_costmdls_unref(&costmdls);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&hash);

    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  result = cardano_transaction_body_set_script_data_hash(body, hash);
  cardano_blake2b_hash_unref(&hash);

  return result;
}

/* DEFINITIONS ****************************************************************/

cardano_tx_builder_t*
cardano_tx_builder_new(
  cardano_protocol_parameters_t* params,
  const cardano_slot_config_t*   slot_config)
{
  if ((params == NULL) || (slot_config == NULL))
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

  builder->last_error = CARDANO_SUCCESS;

  const cardano_error_t result = cardano_builder_state_init(&builder->state, params, slot_config);

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
    cardano_tx_builder_set_last_error(builder, "Coin selector is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_coin_selector_ref(coin_selector);
  cardano_coin_selector_unref(&builder->state.coin_selector);
  builder->state.coin_selector = coin_selector;
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
    cardano_tx_builder_set_last_error(builder, "Transaction evaluator is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_tx_evaluator_ref(tx_evaluator);
  cardano_tx_evaluator_unref(&builder->state.tx_evaluator);
  builder->state.tx_evaluator = tx_evaluator;
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_network_id(body, &network_id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set network ID.");
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
    cardano_tx_builder_set_last_error(builder, "Change address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_ref(change_address);
  cardano_address_unref(&builder->state.change_address);
  builder->state.change_address = change_address;
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
    cardano_tx_builder_set_last_error(builder, "Change address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* address = NULL;
  cardano_error_t    result  = cardano_address_from_string(change_address, address_size, &address);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse change address.");
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
    cardano_tx_builder_set_last_error(builder, "Collateral change address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_ref(collateral_change_address);
  cardano_address_unref(&builder->state.collateral_address);
  builder->state.collateral_address = collateral_change_address;
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
    cardano_tx_builder_set_last_error(builder, "Collateral change address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* address = NULL;
  cardano_error_t    result  = cardano_address_from_string(collateral_change_address, address_size, &address);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse collateral change address.");
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_fee(body, minimum_fee);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set fee.");
    builder->last_error = result;
  }
}

void
cardano_tx_builder_set_donation(
  cardano_tx_builder_t* builder,
  const uint64_t        donation)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
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
    cardano_tx_builder_set_last_error(builder, "Failed to set donation.");
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
    cardano_tx_builder_set_last_error(builder, "UTXOs list is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_utxo_list_ref(utxos);
  cardano_utxo_list_unref(&builder->state.available_utxos);
  builder->state.available_utxos = utxos;
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
    cardano_tx_builder_set_last_error(builder, "Collateral UTXOs list is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_utxo_list_ref(utxos);
  cardano_utxo_list_unref(&builder->state.collateral_utxos);
  builder->state.collateral_utxos = utxos;
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_invalid_after(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set invalid after.");
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  const uint64_t  slot   = unix_time_to_enclosing_slot(unix_time * 1000U, &builder->state.slot_config);
  cardano_error_t result = cardano_transaction_body_set_invalid_after(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set invalid after.");
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_error_t result = cardano_transaction_body_set_invalid_before(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set invalid before.");
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  const uint64_t  slot   = unix_time_to_enclosing_slot(unix_time * 1000U, &builder->state.slot_config);
  cardano_error_t result = cardano_transaction_body_set_invalid_before(body, &slot);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set invalid before.");
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

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_reference_input(&builder->state, utxo, &error_message);

  track_builder_result(builder, result, error_message);
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

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_lovelace(&builder->state, address, amount, &error_message);

  track_builder_result(builder, result, error_message);
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

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_lovelace_ex(&builder->state, address, address_size, amount, &error_message);

  track_builder_result(builder, result, error_message);
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

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_value(&builder->state, address, value, &error_message);

  track_builder_result(builder, result, error_message);
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

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_send_value_ex(&builder->state, address, address_size, value, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_lock_lovelace(
  cardano_tx_builder_t* builder,
  cardano_address_t*    script_address,
  const uint64_t        amount,
  cardano_datum_t*      datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_lock_lovelace(&builder->state, script_address, amount, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_lock_lovelace_ex(
  cardano_tx_builder_t* builder,
  const char*           script_address,
  size_t                script_address_size,
  uint64_t              amount,
  cardano_datum_t*      datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_lock_lovelace_ex(&builder->state, script_address, script_address_size, amount, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_lock_value(
  cardano_tx_builder_t* builder,
  cardano_address_t*    script_address,
  cardano_value_t*      value,
  cardano_datum_t*      datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_lock_value(&builder->state, script_address, value, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_lock_value_ex(
  cardano_tx_builder_t* builder,
  const char*           script_address,
  size_t                script_address_size,
  cardano_value_t*      value,
  cardano_datum_t*      datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_lock_value_ex(&builder->state, script_address, script_address_size, value, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_add_input(
  cardano_tx_builder_t*  builder,
  cardano_utxo_t*        utxo,
  cardano_plutus_data_t* redeemer,
  cardano_plutus_data_t* datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_input(&builder->state, utxo, redeemer, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_add_output(
  cardano_tx_builder_t*         builder,
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
cardano_tx_builder_set_metadata(
  cardano_tx_builder_t* builder,
  const uint64_t        tag,
  cardano_metadatum_t*  metadata)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (metadata == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Metadata is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_auxiliary_data_t* auxiliary_data = cardano_transaction_get_auxiliary_data(builder->state.transaction);

  if (auxiliary_data == NULL)
  {
    cardano_error_t result = cardano_auxiliary_data_new(&auxiliary_data);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create auxiliary data.");
      builder->last_error = result;
      return;
    }

    result = cardano_transaction_set_auxiliary_data(builder->state.transaction, auxiliary_data);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set auxiliary data.");
      cardano_auxiliary_data_unref(&auxiliary_data);
      builder->last_error = result;
      return;
    }
  }

  cardano_auxiliary_data_unref(&auxiliary_data);

  cardano_transaction_metadata_t* tx_metadata = cardano_auxiliary_data_get_transaction_metadata(auxiliary_data);

  if (tx_metadata == NULL)
  {
    cardano_error_t result = cardano_transaction_metadata_new(&tx_metadata);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create transaction metadata.");
      builder->last_error = result;
      return;
    }

    result = cardano_auxiliary_data_set_transaction_metadata(auxiliary_data, tx_metadata);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set transaction metadata.");
      cardano_transaction_metadata_unref(&tx_metadata);
      builder->last_error = result;
      return;
    }
  }

  cardano_transaction_metadata_unref(&tx_metadata);

  cardano_error_t result = cardano_transaction_metadata_insert(tx_metadata, tag, metadata);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to insert metadata.");
    builder->last_error = result;

    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_t* hash = cardano_auxiliary_data_get_hash(auxiliary_data);

  result = cardano_transaction_body_set_aux_data_hash(body, hash);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set auxiliary data hash.");
    builder->last_error = result;
  }
}

void
cardano_tx_builder_set_metadata_ex(
  cardano_tx_builder_t* builder,
  uint64_t              tag,
  const char*           metadata_json,
  const size_t          json_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((metadata_json == NULL) || (json_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_metadatum_t* metadata = NULL;
  cardano_error_t      result   = cardano_metadatum_from_json(metadata_json, json_size, &metadata);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse metadata.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_set_metadata(builder, tag, metadata);
  cardano_metadatum_unref(&metadata);
}

void
cardano_tx_builder_mint_token(
  cardano_tx_builder_t*   builder,
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   name,
  const int64_t           amount,
  cardano_plutus_data_t*  redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token(&builder->state, policy_id, name, amount, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_mint_token_ex(
  cardano_tx_builder_t*  builder,
  const char*            policy_id_hex,
  size_t                 policy_id_size,
  const char*            name_hex,
  size_t                 name_size,
  const int64_t          amount,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_ex(&builder->state, policy_id_hex, policy_id_size, name_hex, name_size, amount, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_mint_token_with_id(
  cardano_tx_builder_t*  builder,
  cardano_asset_id_t*    asset_id,
  const int64_t          amount,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_with_id(&builder->state, asset_id, amount, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_mint_token_with_id_ex(
  cardano_tx_builder_t*  builder,
  const char*            asset_id_hex,
  size_t                 hex_size,
  int64_t                amount,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_with_id_ex(&builder->state, asset_id_hex, hex_size, amount, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
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

  builder->state.additional_signature_count = count;
}

void
cardano_tx_builder_add_signer(
  cardano_tx_builder_t*   builder,
  cardano_blake2b_hash_t* pub_key_hash)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (pub_key_hash == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Public key hash is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);

  if (guards == NULL)
  {
    cardano_error_t result = cardano_guard_set_new(&guards);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create guards set.");
      builder->last_error = result;

      return;
    }

    result = cardano_transaction_body_set_guards(body, guards);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set guards set.");
      cardano_guard_set_unref(&guards);

      builder->last_error = result;

      return;
    }
  }

  cardano_guard_set_unref(&guards);

  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = cardano_credential_new(pub_key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create signer credential.");
    builder->last_error = result;

    return;
  }

  result = cardano_guard_set_add(guards, credential);

  cardano_credential_unref(&credential);

  if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add signer.");
    builder->last_error = result;
  }
}

void
cardano_tx_builder_add_signer_ex(
  cardano_tx_builder_t* builder,
  const char*           pub_key_hash,
  size_t                hash_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((pub_key_hash == NULL) || (hash_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Public key hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_blake2b_hash_t* hash   = NULL;
  cardano_error_t         result = cardano_blake2b_hash_from_hex(pub_key_hash, hash_size, &hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse public key hash.");
    builder->last_error = result;

    return;
  }

  cardano_tx_builder_add_signer(builder, hash);
  cardano_blake2b_hash_unref(&hash);
}

void
cardano_tx_builder_add_datum(
  cardano_tx_builder_t*  builder,
  cardano_plutus_data_t* datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (datum == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Datum is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(builder->state.transaction);
  cardano_witness_set_unref(&witness_set);

  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witness_set);

  if (datums == NULL)
  {
    cardano_error_t result = cardano_plutus_data_set_new(&datums);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create datums set.");
      builder->last_error = result;

      return;
    }

    result = cardano_witness_set_set_plutus_data(witness_set, datums);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set datums set.");
      cardano_plutus_data_set_unref(&datums);

      builder->last_error = result;

      return;
    }
  }

  cardano_plutus_data_set_unref(&datums);

  cardano_error_t result = cardano_plutus_data_set_add(datums, datum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add datum.");
    builder->last_error = result;

    return;
  }
}

void
cardano_tx_builder_withdraw_rewards(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  const int64_t             amount,
  cardano_plutus_data_t*    redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_withdraw_rewards(&builder->state, address, amount, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_withdraw_rewards_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  const int64_t          amount,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_withdraw_rewards_ex(&builder->state, reward_address, address_size, amount, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_register_reward_address(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_reward_address(&builder->state, address, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_register_reward_address_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_reward_address_ex(&builder->state, reward_address, address_size, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_deregister_reward_address(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_plutus_data_t*    redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_reward_address(&builder->state, address, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_deregister_reward_address_ex(
  cardano_tx_builder_t*  builder,
  const char*            reward_address,
  size_t                 address_size,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_reward_address_ex(&builder->state, reward_address, address_size, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_delegate_stake(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_blake2b_hash_t*   pool_id,
  cardano_plutus_data_t*    redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_stake(&builder->state, address, pool_id, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
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
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_stake_ex(&builder->state, reward_address, address_size, pool_id, pool_id_size, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_delegate_voting_power(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* address,
  cardano_drep_t*           drep,
  cardano_plutus_data_t*    redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_voting_power(&builder->state, address, drep, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
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
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_delegate_voting_power_ex(&builder->state, reward_address, address_size, drep_id, drep_id_size, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_register_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_drep(&builder->state, drep, anchor, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_register_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  const size_t           drep_id_size,
  const char*            metadata_url,
  const size_t           metadata_url_size,
  const char*            metadata_hash_hex,
  const size_t           metadata_hash_hex_size,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_register_drep_ex(&builder->state, drep_id, drep_id_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_update_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_anchor_t*      anchor,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_update_drep(&builder->state, drep, anchor, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_update_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  const size_t           drep_id_size,
  const char*            metadata_url,
  const size_t           metadata_url_size,
  const char*            metadata_hash_hex,
  const size_t           metadata_hash_hex_size,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_update_drep_ex(&builder->state, drep_id, drep_id_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_deregister_drep(
  cardano_tx_builder_t*  builder,
  cardano_drep_t*        drep,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_drep(&builder->state, drep, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_deregister_drep_ex(
  cardano_tx_builder_t*  builder,
  const char*            drep_id,
  size_t                 drep_id_size,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_deregister_drep_ex(&builder->state, drep_id, drep_id_size, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_vote(
  cardano_tx_builder_t*           builder,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_plutus_data_t*          redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_vote(&builder->state, voter, action_id, vote, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_add_certificate(
  cardano_tx_builder_t*  builder,
  cardano_certificate_t* certificate,
  cardano_plutus_data_t* redeemer)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_certificate(&builder->state, certificate, redeemer, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_add_script(
  cardano_tx_builder_t* builder,
  cardano_script_t*     script)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (script == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Script is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_script_language_t language;
  cardano_error_t           result = cardano_script_get_language(script, &language);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to get script language.");
    builder->last_error = result;

    return;
  }

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      builder->state.has_plutus_v1 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      builder->state.has_plutus_v2 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      builder->state.has_plutus_v3 = true;
      break;
    }
    default:
    {
      break;
    }
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(builder->state.transaction);
  cardano_witness_set_unref(&witness_set);

  result = cardano_witness_set_add_script(witness_set, script);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add script to witness set.");
    builder->last_error = result;
  }
}

void
cardano_tx_builder_propose_parameter_change(
  cardano_tx_builder_t*            builder,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_blake2b_hash_t*          policy_hash)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_parameter_change(&builder->state, reward_address, anchor, protocol_param_update, governance_action_id, policy_hash, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_parameter_change_ex(
  cardano_tx_builder_t*            builder,
  const char*                      reward_address,
  const size_t                     reward_address_size,
  const char*                      metadata_url,
  const size_t                     metadata_url_size,
  const char*                      metadata_hash_hex,
  const size_t                     metadata_hash_hex_size,
  const char*                      gov_action_id,
  const size_t                     gov_action_id_size,
  const char*                      policy_hash_hash_hex,
  const size_t                     policy_hash_hash_hex_size,
  cardano_protocol_param_update_t* protocol_param_update)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_parameter_change_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, policy_hash_hash_hex, policy_hash_hash_hex_size, protocol_param_update, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_hardfork(
  cardano_tx_builder_t*           builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_protocol_version_t*     version,
  cardano_governance_action_id_t* governance_action_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_hardfork(&builder->state, reward_address, anchor, version, governance_action_id, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_hardfork_ex(
  cardano_tx_builder_t* builder,
  const char*           reward_address,
  const size_t          reward_address_size,
  const char*           metadata_url,
  const size_t          metadata_url_size,
  const char*           metadata_hash_hex,
  const size_t          metadata_hash_hex_size,
  const char*           gov_action_id,
  const size_t          gov_action_id_size,
  const uint64_t        minor_protocol_version,
  const uint64_t        major_protocol_version)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_hardfork_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, minor_protocol_version, major_protocol_version, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_treasury_withdrawals(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor,
  cardano_withdrawal_map_t* withdrawals,
  cardano_blake2b_hash_t*   policy_hash)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_treasury_withdrawals(&builder->state, reward_address, anchor, withdrawals, policy_hash, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_treasury_withdrawals_ex(
  cardano_tx_builder_t*     builder,
  const char*               reward_address,
  const size_t              reward_address_size,
  const char*               metadata_url,
  const size_t              metadata_url_size,
  const char*               metadata_hash_hex,
  const size_t              metadata_hash_hex_size,
  const char*               policy_hash_hash_hex,
  const size_t              policy_hash_hash_hex_size,
  cardano_withdrawal_map_t* withdrawals)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_treasury_withdrawals_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, policy_hash_hash_hex, policy_hash_hash_hex_size, withdrawals, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_no_confidence(
  cardano_tx_builder_t*           builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_no_confidence(&builder->state, reward_address, anchor, governance_action_id, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_no_confidence_ex(
  cardano_tx_builder_t* builder,
  const char*           reward_address,
  const size_t          reward_address_size,
  const char*           metadata_url,
  const size_t          metadata_url_size,
  const char*           metadata_hash_hex,
  const size_t          metadata_hash_hex_size,
  const char*           gov_action_id,
  const size_t          gov_action_id_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_no_confidence_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_update_committee(
  cardano_tx_builder_t*            builder,
  cardano_reward_address_t*        reward_address,
  cardano_anchor_t*                anchor,
  cardano_governance_action_id_t*  governance_action_id,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  cardano_unit_interval_t*         new_quorum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_update_committee(&builder->state, reward_address, anchor, governance_action_id, members_to_be_removed, members_to_be_added, new_quorum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_update_committee_ex(
  cardano_tx_builder_t*            builder,
  const char*                      reward_address,
  const size_t                     reward_address_size,
  const char*                      metadata_url,
  const size_t                     metadata_url_size,
  const char*                      metadata_hash_hex,
  const size_t                     metadata_hash_hex_size,
  const char*                      gov_action_id,
  const size_t                     gov_action_id_size,
  cardano_credential_set_t*        members_to_be_removed,
  cardano_committee_members_map_t* members_to_be_added,
  const double                     new_quorum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_update_committee_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, members_to_be_removed, members_to_be_added, new_quorum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_new_constitution(
  cardano_tx_builder_t*           builder,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_governance_action_id_t* governance_action_id,
  cardano_constitution_t*         constitution)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_new_constitution(&builder->state, reward_address, anchor, governance_action_id, constitution, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_new_constitution_ex(
  cardano_tx_builder_t*   builder,
  const char*             reward_address,
  const size_t            reward_address_size,
  const char*             metadata_url,
  const size_t            metadata_url_size,
  const char*             metadata_hash_hex,
  const size_t            metadata_hash_hex_size,
  const char*             gov_action_id,
  const size_t            gov_action_id_size,
  cardano_constitution_t* constitution)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_new_constitution_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, gov_action_id, gov_action_id_size, constitution, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_info(
  cardano_tx_builder_t*     builder,
  cardano_reward_address_t* reward_address,
  cardano_anchor_t*         anchor)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_info(&builder->state, reward_address, anchor, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_propose_info_ex(
  cardano_tx_builder_t* builder,
  const char*           reward_address,
  const size_t          reward_address_size,
  const char*           metadata_url,
  const size_t          metadata_url_size,
  const char*           metadata_hash_hex,
  const size_t          metadata_hash_hex_size)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_propose_info_ex(&builder->state, reward_address, reward_address_size, metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &error_message);

  track_builder_result(builder, result, error_message);
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

  if (builder->state.change_address == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_tx_builder_set_last_error(
      builder,
      "You must set a change address before calling `build`.");

    return builder->last_error;
  }

  if (builder->state.available_utxos == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_tx_builder_set_last_error(
      builder,
      "You must set the available UTXOs for input selection before calling `build`.");

    return builder->last_error;
  }

  if (cardano_transaction_has_script_data(builder->state.transaction))
  {
    if (builder->state.collateral_address == NULL)
    {
      builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
      cardano_tx_builder_set_last_error(
        builder,
        "This transaction interacts with plutus validators. You must set a collateral change address before calling `build`.");

      return builder->last_error;
    }

    if (builder->state.collateral_utxos == NULL)
    {
      builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
      cardano_tx_builder_set_last_error(
        builder,
        "This transaction interacts with plutus validators. You must set the collateral UTXOs before calling `build`.");

      return builder->last_error;
    }
  }

  cardano_transaction_t* tx = builder->state.transaction;

  cardano_error_t result = set_dummy_script_data_hash(tx);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return result;
  }

  result = cardano_balance_transaction(
    tx,
    builder->state.additional_signature_count,
    builder->state.params,
    builder->state.reference_inputs,
    builder->state.pre_selected_inputs,
    builder->state.input_to_redeemer_map,
    builder->state.available_utxos,
    builder->state.coin_selector,
    builder->state.change_address,
    builder->state.collateral_utxos,
    builder->state.collateral_address,
    builder->state.tx_evaluator,
    builder->state.deferred_redeemers);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    cardano_tx_builder_set_last_error(
      builder,
      cardano_transaction_get_last_error(tx));

    return result;
  }

  result = update_script_data_hash(builder, tx);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return result;
  }

  cardano_transaction_ref(tx);
  *transaction = tx;

  // Build method can only be called once
  builder->last_error = CARDANO_ERROR_ILLEGAL_STATE;

  return CARDANO_SUCCESS;
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

void
cardano_tx_builder_add_input_with_deferred_redeemer(
  cardano_tx_builder_t*          builder,
  cardano_utxo_t*                utxo,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context,
  cardano_plutus_data_t*         datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_input_with_deferred_redeemer(&builder->state, utxo, callback, user_context, datum, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_mint_token_with_deferred_redeemer(
  cardano_tx_builder_t*          builder,
  cardano_blake2b_hash_t*        policy_id,
  cardano_asset_name_t*          name,
  const int64_t                  amount,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_mint_token_with_deferred_redeemer(&builder->state, policy_id, name, amount, callback, user_context, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(
  cardano_tx_builder_t*          builder,
  cardano_reward_address_t*      address,
  const int64_t                  amount,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_withdraw_rewards_with_deferred_redeemer(&builder->state, address, amount, callback, user_context, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_add_certificate_with_deferred_redeemer(
  cardano_tx_builder_t*          builder,
  cardano_certificate_t*         certificate,
  cardano_deferred_redeemer_fn_t callback,
  void*                          user_context)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_add_certificate_with_deferred_redeemer(&builder->state, certificate, callback, user_context, &error_message);

  track_builder_result(builder, result, error_message);
}

void
cardano_tx_builder_vote_with_deferred_redeemer(
  cardano_tx_builder_t*           builder,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_deferred_redeemer_fn_t  callback,
  void*                           user_context)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  const char* error_message = NULL;

  const cardano_error_t result = cardano_builder_vote_with_deferred_redeemer(&builder->state, voter, action_id, vote, callback, user_context, &error_message);

  track_builder_result(builder, result, error_message);
}
