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
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/transaction_builder/evaluation/provider_tx_evaluator.h>
#include <cardano/transaction_builder/script_data_hash.h>
#include <cardano/witness_set/redeemer.h>

#include "../allocators.h"
#include "./internals/blake2b_hash_to_redeemer_map.h"

#include <assert.h>
#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/stake_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/certs/vote_delegation_cert.h>
#include <cardano/encoding/bech32.h>
#include <cardano/proposal_procedures/constitution.h>
#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/proposal_procedures/info_action.h>
#include <cardano/proposal_procedures/new_constitution_action.h>
#include <cardano/proposal_procedures/no_confidence_action.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>
#include <src/string_safe.h>
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

    // Redeemer maps
    cardano_input_to_redeemer_map_t*        input_to_redeemer_map;
    cardano_blake2b_hash_to_redeemer_map_t* withdrawals_to_redeemer_map;
    cardano_blake2b_hash_to_redeemer_map_t* mints_to_redeemer_map;
    cardano_blake2b_hash_to_redeemer_map_t* votes_to_redeemer_map;
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
  cardano_input_to_redeemer_map_unref(&builder->input_to_redeemer_map);
  cardano_blake2b_hash_to_redeemer_map_unref(&builder->withdrawals_to_redeemer_map);
  cardano_blake2b_hash_to_redeemer_map_unref(&builder->mints_to_redeemer_map);
  cardano_blake2b_hash_to_redeemer_map_unref(&builder->votes_to_redeemer_map);

  _cardano_free(builder);
}

/**
 * \brief Creates and initializes a new, empty transaction.
 *
 * This function allocates and initializes a new instance of a \ref cardano_transaction_t object.
 *
 * \return A pointer to the newly created \ref cardano_transaction_t object if successful. Returns
 * \c NULL if memory allocation fails.
 */
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
  cardano_costmdls_t* pparams_costmdls = cardano_protocol_parameters_get_cost_models(builder->params);
  cardano_costmdls_unref(&pparams_costmdls);

  cardano_error_t result = CARDANO_SUCCESS;

  if (builder->has_plutus_v1)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (builder->has_plutus_v2)
  {
    result = get_cost_model_for_version(pparams_costmdls, costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V2);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (builder->has_plutus_v3)
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

/**
 * \brief Adds a redeemer to the witness set.
 *
 * This function creates and adds a new redeemer to the given witness set. The redeemer is initialized with
 * the provided data, tag.
 *
 * \param[in,out] witness_set A pointer to the \ref cardano_witness_set_t object where the redeemer will be added.
 * \param[in]     data        A pointer to the \ref cardano_plutus_data_t object containing the execution data for the redeemer.
 * \param[in]     tag         The tag indicating the context in which the redeemer is used (e.g., spending, minting, etc.).
 *
 * \return \ref CARDANO_SUCCESS if the redeemer was successfully added, or an appropriate error code indicating the failure reason.
 */
static cardano_error_t
add_redeemer(
  cardano_witness_set_t*       witness_set,
  cardano_blake2b_hash_t*      hash,
  cardano_plutus_data_t*       data,
  const cardano_redeemer_tag_t tag,
  cardano_tx_builder_t*        builder)
{
  cardano_error_t          result    = CARDANO_SUCCESS;
  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);

  if (redeemers == NULL)
  {
    result = cardano_redeemer_list_new(&redeemers);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_witness_set_set_redeemers(witness_set, redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&redeemers);
      return result;
    }
  }

  cardano_redeemer_list_unref(&redeemers);

  if (data != NULL)
  {
    cardano_redeemer_t* rdmer = NULL;

    cardano_ex_units_t* ex_units = NULL;
    result                       = cardano_ex_units_new(0, 0, &ex_units);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_redeemer_new(tag, 0, data, ex_units, &rdmer);
    cardano_ex_units_unref(&ex_units);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    bool duplicate = false;

    switch (tag)
    {
      case CARDANO_REDEEMER_TAG_MINT:
      {
        result = cardano_blake2b_hash_to_redeemer_map_insert(builder->mints_to_redeemer_map, hash, rdmer);

        if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
        {
          cardano_tx_builder_set_last_error(builder, "Failed to insert plutus data to redeemer map.");
          builder->last_error = result;

          cardano_redeemer_unref(&rdmer);

          return result;
        }

        if (result == CARDANO_ERROR_DUPLICATED_KEY)
        {
          duplicate = true;
          result    = CARDANO_SUCCESS;
        }

        break;
      }
      case CARDANO_REDEEMER_TAG_REWARD:
      {
        result = cardano_blake2b_hash_to_redeemer_map_insert(builder->withdrawals_to_redeemer_map, hash, rdmer);

        if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
        {
          cardano_tx_builder_set_last_error(builder, "Failed to insert plutus data to redeemer map.");
          builder->last_error = result;

          cardano_redeemer_unref(&rdmer);

          return result;
        }

        if (result == CARDANO_ERROR_DUPLICATED_KEY)
        {
          duplicate = true;
          result    = CARDANO_SUCCESS;
        }

        break;
      }
      case CARDANO_REDEEMER_TAG_VOTING:
      {
        result = cardano_blake2b_hash_to_redeemer_map_insert(builder->votes_to_redeemer_map, hash, rdmer);

        if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
        {
          cardano_tx_builder_set_last_error(builder, "Failed to insert plutus data to redeemer map.");
          builder->last_error = result;

          cardano_redeemer_unref(&rdmer);

          return result;
        }

        if (result == CARDANO_ERROR_DUPLICATED_KEY)
        {
          duplicate = true;
          result    = CARDANO_SUCCESS;
        }

        break;
      }
      case CARDANO_REDEEMER_TAG_CERTIFYING:
      case CARDANO_REDEEMER_TAG_PROPOSING:
      case CARDANO_REDEEMER_TAG_SPEND:
      default:
      {
        cardano_tx_builder_set_last_error(builder, "Invalid redeemer tag.");
        cardano_redeemer_unref(&rdmer);

        return CARDANO_ERROR_ILLEGAL_STATE;
      }
    }

    if (!duplicate)
    {
      result = cardano_redeemer_list_add(redeemers, rdmer);
    }

    cardano_redeemer_unref(&rdmer);
  }

  return result;
}

/**
 * \brief Gets the proposal procedures set from the transaction body. If there are no proposal
 * procedures set, it creates a new list.
 *
 * \param builder The transaction builder.
 * \param body The transaction body.
 *
 * \return The proposal procedures set.
 */
static cardano_proposal_procedure_set_t*
get_proposal_procedure_set(cardano_tx_builder_t* builder, cardano_transaction_body_t* body)
{
  cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);

  if (proposals == NULL)
  {
    cardano_error_t result = cardano_proposal_procedure_set_new(&proposals);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure set.");
      builder->last_error = result;

      return NULL;
    }

    result = cardano_transaction_body_set_proposal_procedure(body, proposals);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set proposal procedure set.");
      cardano_proposal_procedure_set_unref(&proposals);
      builder->last_error = result;

      return NULL;
    }
  }

  cardano_proposal_procedure_set_unref(&proposals);

  return proposals;
}

/**
 * \brief Creates an empty plutus data object.
 *
 * \return The empty plutus data object.
 */
static cardano_plutus_data_t*
create_empty_plutus_data(void)
{
  static const char* VOID_DATA = "d87980";

  cardano_plutus_data_t* plutus_data = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(VOID_DATA, cardano_safe_strlen(VOID_DATA, 6));

  cardano_error_t result = cardano_plutus_data_from_cbor(reader, &plutus_data);
  cardano_cbor_reader_unref(&reader);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return plutus_data;
}

/**
 * \brief Adds an empty proposing redeemer to the witness set.
 *
 * \param builder The transaction builder.
 * \param witness_set The witness set.
 * \param proposal_index The index of the proposal that requires the redeemer.
 *
 * \return The result of the operation.
 */
static cardano_error_t
add_proposing_redeemer(
  cardano_tx_builder_t*  builder,
  cardano_witness_set_t* witness_set,
  const size_t           proposal_index)
{
  cardano_redeemer_list_t* redeemers         = cardano_witness_set_get_redeemers(witness_set);
  cardano_plutus_data_t*   empty_plutus_data = create_empty_plutus_data();
  cardano_redeemer_t*      rdmer             = NULL;
  cardano_ex_units_t*      ex_units          = NULL;

  cardano_error_t result = cardano_ex_units_new(0, 0, &ex_units);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_data_unref(&empty_plutus_data);
    return result;
  }

  result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_PROPOSING, proposal_index, empty_plutus_data, ex_units, &rdmer);

  cardano_plutus_data_unref(&empty_plutus_data);
  cardano_ex_units_unref(&ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (redeemers == NULL)
  {
    result = cardano_redeemer_list_new(&redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_unref(&rdmer);
      cardano_tx_builder_set_last_error(builder, "Failed to create redeemer list.");
      builder->last_error = result;

      return result;
    }

    result = cardano_witness_set_set_redeemers(witness_set, redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_unref(&rdmer);
      cardano_tx_builder_set_last_error(builder, "Failed to set redeemer list.");
      cardano_redeemer_list_unref(&redeemers);
      builder->last_error = result;

      return result;
    }
  }

  cardano_redeemer_list_unref(&redeemers);

  result = cardano_redeemer_list_add(redeemers, rdmer);
  cardano_redeemer_unref(&rdmer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add redeemer.");
    builder->last_error = result;
  }

  return result;
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

  builder->last_error                  = CARDANO_SUCCESS;
  builder->transaction                 = NULL;
  builder->params                      = NULL;
  builder->provider                    = NULL;
  builder->coin_selector               = NULL;
  builder->tx_evaluator                = NULL;
  builder->change_address              = NULL;
  builder->collateral_address          = NULL;
  builder->available_utxos             = NULL;
  builder->collateral_utxos            = NULL;
  builder->pre_selected_inputs         = NULL;
  builder->reference_inputs            = NULL;
  builder->input_to_redeemer_map       = NULL;
  builder->withdrawals_to_redeemer_map = NULL;
  builder->mints_to_redeemer_map       = NULL;
  builder->votes_to_redeemer_map       = NULL;

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

  result = cardano_input_to_redeemer_map_new(&builder->input_to_redeemer_map);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  result = cardano_blake2b_hash_to_redeemer_map_new(&builder->withdrawals_to_redeemer_map);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  result = cardano_blake2b_hash_to_redeemer_map_new(&builder->mints_to_redeemer_map);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_unref(&builder);
    return NULL;
  }

  result = cardano_blake2b_hash_to_redeemer_map_new(&builder->votes_to_redeemer_map);

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
    cardano_tx_builder_set_last_error(builder, "Transaction evaluator is NULL.");
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
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
    cardano_tx_builder_set_last_error(builder, "Collateral UTXOs list is NULL.");
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_network_magic_t network_magic = cardano_provider_get_network_magic(builder->provider);
  const uint64_t          slot          = cardano_compute_slot_from_unix_time(network_magic, unix_time);
  cardano_error_t         result        = cardano_transaction_body_set_invalid_after(body, &slot);

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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  if (body == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Transaction body is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_network_magic_t network_magic = cardano_provider_get_network_magic(builder->provider);
  const uint64_t          slot          = cardano_compute_slot_from_unix_time(network_magic, unix_time);
  cardano_error_t         result        = cardano_transaction_body_set_invalid_before(body, &slot);

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

  if (utxo == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "UTXO is NULL.");
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
      cardano_tx_builder_set_last_error(builder, "Failed to create reference inputs.");
      builder->last_error = result;
      return;
    }

    result = cardano_transaction_body_set_reference_inputs(body, inputs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set reference inputs.");
      cardano_transaction_input_set_unref(&inputs);
      builder->last_error = result;

      return;
    }
  }

  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&input);

  cardano_error_t result = cardano_transaction_input_set_add(inputs, input);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add input to reference inputs.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&output);

  if (output == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Output is NULL.");
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
      cardano_tx_builder_set_last_error(builder, "Failed to get script language.");
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
    cardano_tx_builder_set_last_error(builder, "Failed to add UTXO to reference inputs.");
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
    cardano_tx_builder_set_last_error(builder, "Address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(address, amount, &output);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create output.");
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
    cardano_tx_builder_set_last_error(builder, "Failed to add output to transaction.");
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
    cardano_tx_builder_set_last_error(builder, "Address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse address.");
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

  if (address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if (value == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Value is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(address, 0U, &output);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create output.");
    builder->last_error = result;
    return;
  }

  result = cardano_transaction_output_set_value(output, value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set value.");
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
    cardano_tx_builder_set_last_error(builder, "Failed to add output to transaction.");
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

  if ((address == NULL) || (address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if (value == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Value is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse address.");
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
  const uint64_t        amount,
  cardano_datum_t*      datum)
{
  if ((builder == NULL) || (builder->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (script_address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Script address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(script_address, amount, &output);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create output.");
    builder->last_error = result;
    return;
  }

  result = cardano_transaction_output_set_datum(output, datum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set datum.");
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
    cardano_tx_builder_set_last_error(builder, "Failed to add output to transaction.");
    builder->last_error = result;
    return;
  }
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

  if ((script_address == NULL) || (script_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Script address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(script_address, script_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse script address.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_lock_lovelace(builder, addr, amount, datum);
  cardano_address_unref(&addr);
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

  if (script_address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Script address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(script_address, 0U, &output);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create output.");
    builder->last_error = result;
    return;
  }

  result = cardano_transaction_output_set_value(output, value);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set value.");
    builder->last_error = result;
    cardano_transaction_output_unref(&output);
    return;
  }

  result = cardano_transaction_output_set_datum(output, datum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set datum.");
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
    cardano_tx_builder_set_last_error(builder, "Failed to add output to transaction.");
    builder->last_error = result;
    return;
  }
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

  if ((script_address == NULL) || (script_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Script address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_address_t* addr   = NULL;
  cardano_error_t    result = cardano_address_from_string(script_address, script_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse script address.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_lock_value(builder, addr, value, datum);
  cardano_address_unref(&addr);
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

  if (utxo == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "UTXO is required");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&output);

  cardano_address_t* address = cardano_transaction_output_get_address(output);
  cardano_address_unref(&address);

  cardano_address_type_t address_type;

  cardano_error_t result = cardano_address_get_type(address, &address_type);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to get address type");
    builder->last_error = result;
    return;
  }

  result = cardano_utxo_list_add(builder->pre_selected_inputs, utxo);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add UTXO to pre-selected inputs");
    builder->last_error = result;

    return;
  }

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(builder->transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);

  if (redeemers == NULL)
  {
    result = cardano_redeemer_list_new(&redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create redeemers list");
      builder->last_error = result;
      return;
    }

    result = cardano_witness_set_set_redeemers(witnesses, redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set redeemers list");
      cardano_redeemer_list_unref(&redeemers);
      builder->last_error = result;
      return;
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
      cardano_tx_builder_set_last_error(builder, "Failed to create execution units");
      builder->last_error = result;
      return;
    }

    result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, redeemer, ex_units, &rdmer);
    cardano_ex_units_unref(&ex_units);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create redeemer");
      builder->last_error = result;
      return;
    }

    result = cardano_redeemer_list_add(redeemers, rdmer);
    cardano_redeemer_unref(&rdmer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to add redeemer to list");
      builder->last_error = result;
      return;
    }

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_transaction_input_unref(&input);

    result = cardano_input_to_redeemer_map_insert(builder->input_to_redeemer_map, input, rdmer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to insert input to redeemer map");
      builder->last_error = result;
      return;
    }
  }

  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witnesses);

  if (datums == NULL)
  {
    result = cardano_plutus_data_set_new(&datums);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create datums set");
      builder->last_error = result;
      return;
    }

    result = cardano_witness_set_set_plutus_data(witnesses, datums);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set datums set");
      cardano_plutus_data_set_unref(&datums);
      builder->last_error = result;
      return;
    }
  }

  cardano_plutus_data_set_unref(&datums);

  if (datum != NULL)
  {
    result = cardano_plutus_data_set_add(datums, datum);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to add datum to set");
      builder->last_error = result;
      return;
    }
  }
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

  if (output == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_error_t result = cardano_transaction_output_list_add(outputs, output);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
  }
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

  cardano_auxiliary_data_t* auxiliary_data = cardano_transaction_get_auxiliary_data(builder->transaction);

  if (auxiliary_data == NULL)
  {
    cardano_error_t result = cardano_auxiliary_data_new(&auxiliary_data);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create auxiliary data.");
      builder->last_error = result;
      return;
    }

    result = cardano_transaction_set_auxiliary_data(builder->transaction, auxiliary_data);

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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
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

  if (policy_id == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Policy ID is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  if (name == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Asset name is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* tokens = cardano_transaction_body_get_mint(body);

  if (tokens == NULL)
  {
    cardano_error_t result = cardano_multi_asset_new(&tokens);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create multi-asset.");
      builder->last_error = result;

      return;
    }

    result = cardano_transaction_body_set_mint(body, tokens);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set multi-asset.");
      cardano_multi_asset_unref(&tokens);

      builder->last_error = result;

      return;
    }
  }

  cardano_multi_asset_unref(&tokens);

  cardano_error_t result = cardano_multi_asset_set(tokens, policy_id, name, amount);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to set multi-asset.");
    builder->last_error = result;

    return;
  }

  if (redeemer != NULL)
  {
    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(builder->transaction);
    cardano_witness_set_unref(&witnesses);

    result = add_redeemer(witnesses, policy_id, redeemer, CARDANO_REDEEMER_TAG_MINT, builder);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to add redeemer.");
      builder->last_error = result;
    }
  }
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

  if ((policy_id_hex == NULL) || (policy_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Policy ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  if ((name_hex == NULL) || (name_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Asset name is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_blake2b_hash_t* policy_id = NULL;
  cardano_error_t         result    = cardano_blake2b_hash_from_hex(policy_id_hex, policy_id_size, &policy_id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse policy ID.");
    builder->last_error = result;

    return;
  }

  cardano_asset_name_t* name = NULL;
  result                     = cardano_asset_name_from_hex(name_hex, name_size, &name);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&policy_id);
    cardano_tx_builder_set_last_error(builder, "Failed to parse asset name.");
    builder->last_error = result;

    return;
  }

  cardano_tx_builder_mint_token(builder, policy_id, name, amount, redeemer);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);
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

  if (asset_id == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Asset ID is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_blake2b_hash_t* policy_id = cardano_asset_id_get_policy_id(asset_id);
  cardano_asset_name_t*   name      = cardano_asset_id_get_asset_name(asset_id);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);

  cardano_tx_builder_mint_token(builder, policy_id, name, amount, redeemer);
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

  if ((asset_id_hex == NULL) || (hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Asset ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  cardano_asset_id_t* asset_id = NULL;
  cardano_error_t     result   = cardano_asset_id_from_hex(asset_id_hex, hex_size, &asset_id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse asset ID.");
    builder->last_error = result;

    return;
  }

  cardano_tx_builder_mint_token_with_id(builder, asset_id, amount, redeemer);
  cardano_asset_id_unref(&asset_id);
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

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_set_t* signers = cardano_transaction_body_get_required_signers(body);

  if (signers == NULL)
  {
    cardano_error_t result = cardano_blake2b_hash_set_new(&signers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create signers set.");
      builder->last_error = result;

      return;
    }

    result = cardano_transaction_body_set_required_signers(body, signers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set signers set.");
      cardano_blake2b_hash_set_unref(&signers);

      builder->last_error = result;

      return;
    }
  }

  cardano_blake2b_hash_set_unref(&signers);

  cardano_error_t result = cardano_blake2b_hash_set_add(signers, pub_key_hash);

  if (result != CARDANO_SUCCESS)
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

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(builder->transaction);
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

  if (address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if (amount < 0)
  {
    cardano_tx_builder_set_last_error(builder, "Amount must be greater than zero.");
    builder->last_error = CARDANO_ERROR_INVALID_ARGUMENT;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);

  if (withdrawals == NULL)
  {
    cardano_error_t result = cardano_withdrawal_map_new(&withdrawals);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create withdrawal map.");
      builder->last_error = result;
      return;
    }

    result = cardano_transaction_body_set_withdrawals(body, withdrawals);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set withdrawal map.");
      cardano_withdrawal_map_unref(&withdrawals);
      builder->last_error = result;
      return;
    }
  }

  cardano_withdrawal_map_unref(&withdrawals);

  cardano_error_t result = cardano_withdrawal_map_insert(withdrawals, address, amount);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to insert withdrawal.");
    builder->last_error = result;
    return;
  }

  if (redeemer != NULL)
  {
    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(builder->transaction);
    cardano_witness_set_unref(&witnesses);

    cardano_credential_t* credential = cardano_reward_address_get_credential(address);
    cardano_credential_unref(&credential);

    cardano_blake2b_hash_t* hash = cardano_credential_get_hash(credential);

    result = add_redeemer(witnesses, hash, redeemer, CARDANO_REDEEMER_TAG_REWARD, builder);

    cardano_blake2b_hash_unref(&hash);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to add redeemer.");
      builder->last_error = result;
    }
  }
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

  if ((reward_address == NULL) || (address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_reward_address_t* addr   = NULL;
  cardano_error_t           result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_withdraw_rewards(builder, addr, amount, redeemer);
  cardano_reward_address_unref(&addr);
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

  if (address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  const uint64_t deposit = cardano_protocol_parameters_get_key_deposit(builder->params);

  cardano_certificate_t*       cert         = NULL;
  cardano_registration_cert_t* registration = NULL;
  cardano_error_t              result       = cardano_registration_cert_new(credential, deposit, &registration);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create registration certificate.");
    builder->last_error = result;

    return;
  }

  result = cardano_certificate_new_registration(registration, &cert);
  cardano_registration_cert_unref(&registration);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create certificate.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_add_certificate(builder, cert, redeemer);
  cardano_certificate_unref(&cert);
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

  if ((reward_address == NULL) || (address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_reward_address_t* addr   = NULL;
  cardano_error_t           result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_register_reward_address(builder, addr, redeemer);
  cardano_reward_address_unref(&addr);
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

  if (address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  const uint64_t deposit = cardano_protocol_parameters_get_key_deposit(builder->params);

  cardano_certificate_t*         cert           = NULL;
  cardano_unregistration_cert_t* unregistration = NULL;

  cardano_error_t result = cardano_unregistration_cert_new(credential, deposit, &unregistration);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create unregistration certificate.");
    builder->last_error = result;

    return;
  }

  result = cardano_certificate_new_unregistration(unregistration, &cert);
  cardano_unregistration_cert_unref(&unregistration);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create certificate.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_add_certificate(builder, cert, redeemer);
  cardano_certificate_unref(&cert);
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

  if ((reward_address == NULL) || (address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_reward_address_t* addr = NULL;

  cardano_error_t result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_deregister_reward_address(builder, addr, redeemer);
  cardano_reward_address_unref(&addr);
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

  if (address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if (pool_id == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Pool ID is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  cardano_certificate_t*           cert       = NULL;
  cardano_stake_delegation_cert_t* delegation = NULL;

  cardano_error_t result = cardano_stake_delegation_cert_new(credential, pool_id, &delegation);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create delegation certificate.");
    builder->last_error = result;
    return;
  }

  result = cardano_certificate_new_stake_delegation(delegation, &cert);
  cardano_stake_delegation_cert_unref(&delegation);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create certificate.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_add_certificate(builder, cert, redeemer);
  cardano_certificate_unref(&cert);
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

  if ((reward_address == NULL) || (address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  if ((pool_id == NULL) || (pool_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Pool ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

    return;
  }

  byte_t pool_id_hex[28] = { 0 };
  char   pool_hrp[5]     = { 0 };

  cardano_error_t result = cardano_encoding_bech32_decode(pool_id, pool_id_size, pool_hrp, sizeof(pool_hrp), pool_id_hex, sizeof(pool_id_hex));

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to decode pool ID.");
    builder->last_error = result;

    return;
  }

  if (strncmp(pool_hrp, "pool", 4) != 0)
  {
    cardano_tx_builder_set_last_error(builder, "Invalid pool ID.");
    builder->last_error = CARDANO_ERROR_INVALID_ARGUMENT;

    return;
  }

  cardano_blake2b_hash_t* hash = NULL;
  result                       = cardano_blake2b_hash_from_bytes(pool_id_hex, sizeof(pool_id_hex), &hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse pool ID.");
    builder->last_error = result;

    return;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&hash);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;

    return;
  }

  cardano_tx_builder_delegate_stake(builder, addr, hash, redeemer);
  cardano_reward_address_unref(&addr);
  cardano_blake2b_hash_unref(&hash);
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

  if (address == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if (drep == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "DREP is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_credential_t* credential = cardano_reward_address_get_credential(address);
  cardano_credential_unref(&credential);

  cardano_certificate_t*          cert       = NULL;
  cardano_vote_delegation_cert_t* delegation = NULL;

  cardano_error_t result = cardano_vote_delegation_cert_new(credential, drep, &delegation);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create delegation certificate.");
    builder->last_error = result;
    return;
  }

  result = cardano_certificate_new_vote_delegation(delegation, &cert);
  cardano_vote_delegation_cert_unref(&delegation);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create certificate.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_add_certificate(builder, cert, redeemer);
  cardano_certificate_unref(&cert);
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

  if ((reward_address == NULL) || (address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "DREP ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_reward_address_t* addr   = NULL;
  cardano_error_t           result = cardano_reward_address_from_bech32(reward_address, address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_drep_t* drep = NULL;
  result               = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse DREP ID.");
    cardano_reward_address_unref(&addr);
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_delegate_voting_power(builder, addr, drep, redeemer);

  cardano_reward_address_unref(&addr);
  cardano_drep_unref(&drep);
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

  if (drep == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "DRep is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_certificate_t*        cert          = NULL;
  cardano_register_drep_cert_t* register_drep = NULL;
  cardano_credential_t*         credential    = NULL;

  cardano_error_t result = cardano_drep_get_credential(drep, &credential);
  cardano_credential_unref(&credential);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to get credential.");
    builder->last_error = result;
    return;
  }

  const uint64_t deposit = cardano_protocol_parameters_get_drep_deposit(builder->params);
  result                 = cardano_register_drep_cert_new(credential, deposit, anchor, &register_drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create DRep registration certificate.");
    builder->last_error = result;

    return;
  }

  result = cardano_certificate_new_register_drep(register_drep, &cert);
  cardano_register_drep_cert_unref(&register_drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create certificate.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_add_certificate(builder, cert, redeemer);
  cardano_certificate_unref(&cert);
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

  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "DRep ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_drep_t* drep = NULL;
  result               = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse DRep ID.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_register_drep(builder, drep, anchor, redeemer);

  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
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

  if (drep == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "DRep is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_certificate_t*      cert        = NULL;
  cardano_update_drep_cert_t* update_drep = NULL;
  cardano_credential_t*       credential  = NULL;

  cardano_error_t result = cardano_drep_get_credential(drep, &credential);
  cardano_credential_unref(&credential);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to get credential.");
    builder->last_error = result;
    return;
  }

  result = cardano_update_drep_cert_new(credential, anchor, &update_drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create DRep update certificate.");
    builder->last_error = result;
    return;
  }

  result = cardano_certificate_new_update_drep(update_drep, &cert);
  cardano_update_drep_cert_unref(&update_drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create certificate.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_add_certificate(builder, cert, redeemer);
  cardano_certificate_unref(&cert);
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

  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "DRep ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_drep_t* drep = NULL;
  result               = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse DRep ID.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_update_drep(builder, drep, anchor, redeemer);

  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
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

  if (drep == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "DRep is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_certificate_t*          cert           = NULL;
  cardano_unregister_drep_cert_t* deregistration = NULL;
  cardano_credential_t*           credential     = NULL;

  cardano_error_t result = cardano_drep_get_credential(drep, &credential);
  cardano_credential_unref(&credential);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to get credential.");
    builder->last_error = result;
    return;
  }

  const uint64_t deposit = cardano_protocol_parameters_get_drep_deposit(builder->params);
  result                 = cardano_unregister_drep_cert_new(credential, deposit, &deregistration);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create DRep deregistration certificate.");
    builder->last_error = result;
    return;
  }

  result = cardano_certificate_new_unregister_drep(deregistration, &cert);
  cardano_unregister_drep_cert_unref(&deregistration);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create certificate.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_add_certificate(builder, cert, redeemer);
  cardano_certificate_unref(&cert);
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

  if ((drep_id == NULL) || (drep_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "DRep ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_drep_t* drep   = NULL;
  cardano_error_t result = cardano_drep_from_string(drep_id, drep_id_size, &drep);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to parse DRep ID.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_deregister_drep(builder, drep, redeemer);
  cardano_drep_unref(&drep);
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

  if (voter == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Voter is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if (action_id == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Action ID is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if (vote == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Vote is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_voting_procedures_t* votes = cardano_transaction_body_get_voting_procedures(body);

  if (votes == NULL)
  {
    cardano_error_t result = cardano_voting_procedures_new(&votes);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create voting procedures.");
      builder->last_error = result;
      return;
    }

    result = cardano_transaction_body_set_voting_procedures(body, votes);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set voting procedures.");
      cardano_voting_procedures_unref(&votes);
      builder->last_error = result;
      return;
    }
  }

  cardano_voting_procedures_unref(&votes);

  cardano_error_t result = cardano_voting_procedures_insert(votes, voter, action_id, vote);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to insert vote.");
    builder->last_error = result;
    return;
  }

  if (redeemer != NULL)
  {
    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(builder->transaction);
    cardano_witness_set_unref(&witnesses);

    cardano_credential_t* credential = cardano_voter_get_credential(voter);
    cardano_credential_unref(&credential);

    cardano_blake2b_hash_t* hash = cardano_credential_get_hash(credential);

    result = add_redeemer(witnesses, hash, redeemer, CARDANO_REDEEMER_TAG_VOTING, builder);
    cardano_blake2b_hash_unref(&hash);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to add redeemer.");
      builder->last_error = result;
    }
  }
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

  if (certificate == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Certificate is NULL.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);

  if (certs == NULL)
  {
    cardano_error_t result = cardano_certificate_set_new(&certs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create certificate set.");
      builder->last_error = result;
      return;
    }

    result = cardano_transaction_body_set_certificates(body, certs);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to set certificate set.");
      cardano_certificate_set_unref(&certs);
      builder->last_error = result;
      return;
    }
  }

  cardano_certificate_set_unref(&certs);

  cardano_error_t result = cardano_certificate_set_add(certs, certificate);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add certificate.");
    builder->last_error = result;
    return;
  }

  if (redeemer != NULL)
  {
    cardano_redeemer_t* rdmer = NULL;

    cardano_ex_units_t* ex_units = NULL;
    result                       = cardano_ex_units_new(0, 0, &ex_units);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create execution units.");
      builder->last_error = result;
      return;
    }

    const size_t certs_size = cardano_certificate_set_get_length(certs);

    if (certs_size == 0U)
    {
      cardano_redeemer_unref(&rdmer);
      cardano_tx_builder_set_last_error(builder, "Unexpected empty certificate set.");
      builder->last_error = CARDANO_ERROR_INVALID_ARGUMENT;

      return;
    }

    const size_t cert_index = cardano_certificate_set_get_length(certs) - 1U;

    result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_CERTIFYING, cert_index, redeemer, ex_units, &rdmer);
    cardano_ex_units_unref(&ex_units);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to create redeemer.");
      builder->last_error = result;

      return;
    }

    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(builder->transaction);
    cardano_witness_set_unref(&witnesses);

    cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);

    if (redeemers == NULL)
    {
      result = cardano_redeemer_list_new(&redeemers);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_unref(&rdmer);
        cardano_tx_builder_set_last_error(builder, "Failed to create redeemer list.");
        builder->last_error = result;

        return;
      }

      result = cardano_witness_set_set_redeemers(witnesses, redeemers);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_unref(&rdmer);
        cardano_tx_builder_set_last_error(builder, "Failed to set redeemer list.");
        cardano_redeemer_list_unref(&redeemers);
        builder->last_error = result;

        return;
      }
    }

    cardano_redeemer_list_unref(&redeemers);

    result = cardano_redeemer_list_add(redeemers, rdmer);
    cardano_redeemer_unref(&rdmer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_tx_builder_set_last_error(builder, "Failed to add redeemer.");
      builder->last_error = result;
    }
  }
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

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(builder->transaction);
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

  if (policy_hash == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Policy hash is NULL. You must provide the constitution script hash.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_parameter_change_action_t* action = NULL;
  cardano_error_t                    result = cardano_parameter_change_action_new(protocol_param_update, governance_action_id, policy_hash, &action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create parameter change action.");
    builder->last_error = result;
    return;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(builder->params);

  result = cardano_proposal_procedure_new_parameter_change_action(deposit, reward_address, anchor, action, &proposal);

  cardano_parameter_change_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(builder, body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposal procedure.");
    builder->last_error = result;
  }

  cardano_proposal_procedure_unref(&proposal);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(builder->transaction);
  cardano_witness_set_unref(&witnesses);
  const size_t proposal_index = cardano_proposal_procedure_set_get_length(proposals) - 1U;

  result = add_proposing_redeemer(builder, witnesses, proposal_index);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposing redeemer.");
    builder->last_error = result;
  }
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

  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata URL is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Governance action ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((policy_hash_hash_hex == NULL) || (policy_hash_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Policy hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;

    return;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;

    return;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_tx_builder_set_last_error(builder, "Failed to parse governance action ID.");
    builder->last_error = result;

    return;
  }

  cardano_blake2b_hash_t* policy_hash = NULL;

  result = cardano_blake2b_hash_from_hex(policy_hash_hash_hex, policy_hash_hash_hex_size, &policy_hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_governance_action_id_unref(&id);
    cardano_tx_builder_set_last_error(builder, "Failed to parse policy hash.");
    builder->last_error = result;

    return;
  }

  cardano_tx_builder_propose_parameter_change(builder, addr, anchor, protocol_param_update, id, policy_hash);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);
  cardano_blake2b_hash_unref(&policy_hash);
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

  cardano_hard_fork_initiation_action_t* action = NULL;
  cardano_error_t                        result = cardano_hard_fork_initiation_action_new(version, governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create hard fork action.");
    builder->last_error = result;
    return;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(builder->params);

  result = cardano_proposal_procedure_new_hard_fork_initiation_action(deposit, reward_address, anchor, action, &proposal);

  cardano_hard_fork_initiation_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(builder, body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposal procedure.");
    builder->last_error = result;
  }
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

  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata URL is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Governance action ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_tx_builder_set_last_error(builder, "Failed to parse governance action ID.");
    builder->last_error = result;
    return;
  }

  cardano_protocol_version_t* version = NULL;
  result                              = cardano_protocol_version_new(minor_protocol_version, major_protocol_version, &version);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_governance_action_id_unref(&id);
    cardano_tx_builder_set_last_error(builder, "Failed to create protocol version.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_propose_hardfork(builder, addr, anchor, version, id);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);
  cardano_protocol_version_unref(&version);
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

  if (policy_hash == NULL)
  {
    cardano_tx_builder_set_last_error(builder, "Policy hash is NULL. You must provide the constitution script hash.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_treasury_withdrawals_action_t* action = NULL;
  cardano_error_t                        result = cardano_treasury_withdrawals_action_new(withdrawals, policy_hash, &action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create treasury withdrawal action.");
    builder->last_error = result;
    return;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(builder->params);

  result = cardano_proposal_procedure_new_treasury_withdrawals_action(deposit, reward_address, anchor, action, &proposal);

  cardano_treasury_withdrawals_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(builder, body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposal procedure.");
    builder->last_error = result;
  }

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(builder->transaction);
  cardano_witness_set_unref(&witnesses);
  const size_t proposal_index = cardano_proposal_procedure_set_get_length(proposals) - 1U;

  result = add_proposing_redeemer(builder, witnesses, proposal_index);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposing redeemer.");
    builder->last_error = result;
  }
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

  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata URL is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((policy_hash_hash_hex == NULL) || (policy_hash_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Policy hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_blake2b_hash_t* policy_hash = NULL;
  result                              = cardano_blake2b_hash_from_hex(policy_hash_hash_hex, policy_hash_hash_hex_size, &policy_hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_tx_builder_set_last_error(builder, "Failed to parse policy hash.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_propose_treasury_withdrawals(builder, addr, anchor, withdrawals, policy_hash);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_blake2b_hash_unref(&policy_hash);
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

  cardano_no_confidence_action_t* action = NULL;
  cardano_error_t                 result = cardano_no_confidence_action_new(governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create no confidence action.");
    builder->last_error = result;
    return;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(builder->params);

  result = cardano_proposal_procedure_new_no_confidence_action(deposit, reward_address, anchor, action, &proposal);
  cardano_no_confidence_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(builder, body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposal procedure.");
    builder->last_error = result;
  }
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

  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata URL is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Governance action ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_tx_builder_set_last_error(builder, "Failed to parse governance action ID.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_propose_no_confidence(builder, addr, anchor, id);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);
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

  cardano_update_committee_action_t* action = NULL;
  cardano_error_t                    result = cardano_update_committee_action_new(members_to_be_removed, members_to_be_added, new_quorum, governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create update committee action.");
    builder->last_error = result;
    return;
  }

  cardano_proposal_procedure_t* proposal;

  const uint64_t deposit = cardano_protocol_parameters_get_governance_action_deposit(builder->params);

  result = cardano_proposal_procedure_new_update_committee_action(deposit, reward_address, anchor, action, &proposal);

  cardano_update_committee_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(builder, body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposal procedure.");
    builder->last_error = result;
  }
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

  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata URL is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Governance action ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_reward_address_t* addr = NULL;
  result                         = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_tx_builder_set_last_error(builder, "Failed to parse governance action ID.");
    builder->last_error = result;
    return;
  }

  cardano_unit_interval_t* quorum = NULL;
  result                          = cardano_unit_interval_from_double(new_quorum, &quorum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_governance_action_id_unref(&id);
    cardano_tx_builder_set_last_error(builder, "Failed to create unit interval.");
    builder->last_error = result;

    return;
  }

  cardano_tx_builder_propose_update_committee(builder, addr, anchor, id, members_to_be_removed, members_to_be_added, quorum);

  cardano_unit_interval_unref(&quorum);
  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);
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

  cardano_new_constitution_action_t* action = NULL;

  cardano_error_t result = cardano_new_constitution_action_new(constitution, governance_action_id, &action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create new constitution action.");
    builder->last_error = result;
    return;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(builder->params);

  result = cardano_proposal_procedure_new_constitution_action(deposit, reward_address, anchor, action, &proposal);

  cardano_new_constitution_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(builder, body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);
  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposal procedure.");
    builder->last_error = result;
  }
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

  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata URL is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((gov_action_id == NULL) || (gov_action_id_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Governance action ID is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_reward_address_t* addr = NULL;

  result = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_governance_action_id_t* id = NULL;

  result = cardano_governance_action_id_from_bech32(gov_action_id, gov_action_id_size, &id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_reward_address_unref(&addr);
    cardano_tx_builder_set_last_error(builder, "Failed to parse governance action ID.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_propose_new_constitution(builder, addr, anchor, id, constitution);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
  cardano_governance_action_id_unref(&id);
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

  cardano_info_action_t* action = NULL;
  cardano_error_t        result = cardano_info_action_new(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create info action.");
    builder->last_error = result;
    return;
  }

  cardano_proposal_procedure_t* proposal;
  const uint64_t                deposit = cardano_protocol_parameters_get_governance_action_deposit(builder->params);

  result = cardano_proposal_procedure_new_info_action(deposit, reward_address, anchor, action, &proposal);

  cardano_info_action_unref(&action);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create proposal procedure.");
    builder->last_error = result;
    return;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = get_proposal_procedure_set(builder, body);

  result = cardano_proposal_procedure_set_add(proposals, proposal);

  cardano_proposal_procedure_unref(&proposal);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to add proposal procedure.");
    builder->last_error = result;
  }
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

  if ((reward_address == NULL) || (reward_address_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Reward address is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_url == NULL) || (metadata_url_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata URL is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  if ((metadata_hash_hex == NULL) || (metadata_hash_hex_size == 0U))
  {
    cardano_tx_builder_set_last_error(builder, "Metadata hash is NULL or empty.");
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    return;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t result = cardano_anchor_from_hash_hex(metadata_url, metadata_url_size, metadata_hash_hex, metadata_hash_hex_size, &anchor);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_builder_set_last_error(builder, "Failed to create anchor.");
    builder->last_error = result;
    return;
  }

  cardano_reward_address_t* addr = NULL;

  result = cardano_reward_address_from_bech32(reward_address, reward_address_size, &addr);

  if (result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    cardano_tx_builder_set_last_error(builder, "Failed to parse reward address.");
    builder->last_error = result;
    return;
  }

  cardano_tx_builder_propose_info(builder, addr, anchor);

  cardano_anchor_unref(&anchor);
  cardano_reward_address_unref(&addr);
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
      "You must set a change address before calling `build`.");

    return builder->last_error;
  }

  if (builder->available_utxos == NULL)
  {
    builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_tx_builder_set_last_error(
      builder,
      "You must set the available UTXOs for input selection before calling `build`.");

    return builder->last_error;
  }

  if (cardano_transaction_has_script_data(builder->transaction))
  {
    if (builder->collateral_address == NULL)
    {
      builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
      cardano_tx_builder_set_last_error(
        builder,
        "This transaction interacts with plutus validators. You must set a collateral change address before calling `build`.");

      return builder->last_error;
    }

    if (builder->collateral_utxos == NULL)
    {
      builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
      cardano_tx_builder_set_last_error(
        builder,
        "This transaction interacts with plutus validators. You must set the collateral UTXOs before calling `build`.");

      return builder->last_error;
    }
  }

  cardano_transaction_t* tx = builder->transaction;

  cardano_error_t result = set_dummy_script_data_hash(tx);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
    return result;
  }

  result = cardano_balance_transaction(
    tx,
    builder->additional_signature_count,
    builder->params,
    builder->reference_inputs,
    builder->pre_selected_inputs,
    builder->input_to_redeemer_map,
    builder->available_utxos,
    builder->coin_selector,
    builder->change_address,
    builder->collateral_utxos,
    builder->collateral_address,
    builder->tx_evaluator);

  if (result != CARDANO_SUCCESS)
  {
    builder->last_error = result;
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