/**
 * \file protocol_parameters.c
 *
 * \author angel.castillo
 * \date   Sep 26, 2024
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

#include <cardano/object.h>
#include <cardano/protocol_params/protocol_parameters.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Protocol parameters govern various aspects of the Cardano network.
 */
typedef struct cardano_protocol_parameters_t
{
    cardano_object_t                  base;
    uint64_t                          min_fee_a;
    uint64_t                          min_fee_b;
    uint64_t                          max_block_body_size;
    uint64_t                          max_tx_size;
    uint64_t                          max_block_header_size;
    uint64_t                          key_deposit;
    uint64_t                          pool_deposit;
    uint64_t                          max_epoch;
    uint64_t                          n_opt;
    cardano_unit_interval_t*          pool_pledge_influence;
    cardano_unit_interval_t*          expansion_rate;
    cardano_unit_interval_t*          treasury_growth_rate;
    cardano_unit_interval_t*          d;
    cardano_buffer_t*                 extra_entropy;
    cardano_protocol_version_t*       protocol_version;
    uint64_t                          min_pool_cost;
    uint64_t                          ada_per_utxo_byte;
    cardano_costmdls_t*               cost_models;
    cardano_ex_unit_prices_t*         execution_costs;
    cardano_ex_units_t*               max_tx_ex_units;
    cardano_ex_units_t*               max_block_ex_units;
    uint64_t                          max_value_size;
    uint64_t                          collateral_percentage;
    uint64_t                          max_collateral_inputs;
    cardano_pool_voting_thresholds_t* pool_voting_thresholds;
    cardano_drep_voting_thresholds_t* drep_voting_thresholds;
    uint64_t                          min_committee_size;
    uint64_t                          committee_term_limit;
    uint64_t                          governance_action_validity_period;
    uint64_t                          governance_action_deposit;
    uint64_t                          drep_deposit;
    uint64_t                          drep_inactivity_period;
    cardano_unit_interval_t*          ref_script_cost_per_byte;
} cardano_protocol_parameters_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a protocol parameters update object.
 *
 * This function is responsible for properly deallocating a protocol parameters update object (`cardano_protocol_parameters_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the protocol_parameters object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_protocol_parameters_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the protocol_parameters
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_protocol_parameters_deallocate(void* object)
{
  assert(object != NULL);

  cardano_protocol_parameters_t* data = (cardano_protocol_parameters_t*)object;

  cardano_unit_interval_unref(&data->pool_pledge_influence);
  cardano_unit_interval_unref(&data->expansion_rate);
  cardano_unit_interval_unref(&data->treasury_growth_rate);
  cardano_unit_interval_unref(&data->d);
  cardano_buffer_unref(&data->extra_entropy);
  cardano_protocol_version_unref(&data->protocol_version);
  cardano_costmdls_unref(&data->cost_models);
  cardano_ex_unit_prices_unref(&data->execution_costs);
  cardano_ex_units_unref(&data->max_tx_ex_units);
  cardano_ex_units_unref(&data->max_block_ex_units);
  cardano_pool_voting_thresholds_unref(&data->pool_voting_thresholds);
  cardano_drep_voting_thresholds_unref(&data->drep_voting_thresholds);
  cardano_unit_interval_unref(&data->ref_script_cost_per_byte);

  _cardano_free(object);
}

/**
 * Creates a unit interval with 1/1 values.
 *
 * @return The new unit interval.
 */
static cardano_unit_interval_t*
cardano_get_one_interval(void)
{
  cardano_unit_interval_t* unit_interval = NULL;

  cardano_error_t result = cardano_unit_interval_new(1, 1, &unit_interval);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return unit_interval;
}

/**
 * Creates a protocol version with 0 0 values.
 *
 * @return The protocol version.
 */
static cardano_protocol_version_t*
cardano_get_protocol_version(void)
{
  cardano_protocol_version_t* protocol_version = NULL;

  cardano_error_t result = cardano_protocol_version_new(0, 0, &protocol_version);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return protocol_version;
}

/**
 * Creates empty costmdls.
 *
 * @return The costmdls.
 */
static cardano_costmdls_t*
cardano_get_costmdls(void)
{
  cardano_costmdls_t* costmdls = NULL;

  cardano_error_t result = cardano_costmdls_new(&costmdls);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return costmdls;
}

/**
 * Creates ex unit prices.
 *
 * @return The ex unit prices.
 */
static cardano_ex_unit_prices_t*
cardano_get_ex_unit_prices(void)
{
  cardano_ex_unit_prices_t* ex_unit_prices = NULL;

  cardano_unit_interval_t* memory_prices = cardano_get_one_interval();
  cardano_unit_interval_t* steps_prices  = cardano_get_one_interval();

  cardano_error_t result = cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices);

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return ex_unit_prices;
}

/**
 * Creates ex units.
 *
 * @return The ex units.
 */
static cardano_ex_units_t*
cardano_get_ex_unit(void)
{
  cardano_ex_units_t* ex_units = NULL;

  cardano_error_t result = cardano_ex_units_new(0, 0, &ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return ex_units;
}

/**
 * Creates voting thresholds.
 *
 * @return The voting thresholds.
 */
static cardano_pool_voting_thresholds_t*
cardano_get_pool_voting_thresholds(void)
{
  cardano_unit_interval_t* motion_no_confidence    = cardano_get_one_interval();
  cardano_unit_interval_t* committee_normal        = cardano_get_one_interval();
  cardano_unit_interval_t* committee_no_confidence = cardano_get_one_interval();
  cardano_unit_interval_t* hard_fork_initiation    = cardano_get_one_interval();
  cardano_unit_interval_t* security_relevant_param = cardano_get_one_interval();

  cardano_pool_voting_thresholds_t* thresholds = NULL;

  cardano_error_t result = cardano_pool_voting_thresholds_new(
    motion_no_confidence,
    committee_normal,
    committee_no_confidence,
    hard_fork_initiation,
    security_relevant_param,
    &thresholds);

  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&security_relevant_param);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return thresholds;
}

/**
 * Creates voting thresholds.
 *
 * @return The voting thresholds.
 */
static cardano_drep_voting_thresholds_t*
cardano_get_drep_voting_thresholds(void)
{
  cardano_unit_interval_t* motion_no_confidence    = cardano_get_one_interval();
  cardano_unit_interval_t* committee_normal        = cardano_get_one_interval();
  cardano_unit_interval_t* committee_no_confidence = cardano_get_one_interval();
  cardano_unit_interval_t* update_constitution     = cardano_get_one_interval();
  cardano_unit_interval_t* hard_fork_initiation    = cardano_get_one_interval();
  cardano_unit_interval_t* pp_network_group        = cardano_get_one_interval();
  cardano_unit_interval_t* pp_economic_group       = cardano_get_one_interval();
  cardano_unit_interval_t* pp_technical_group      = cardano_get_one_interval();
  cardano_unit_interval_t* pp_governance_group     = cardano_get_one_interval();
  cardano_unit_interval_t* treasury_withdrawal     = cardano_get_one_interval();

  cardano_drep_voting_thresholds_t* thresholds = NULL;

  cardano_error_t result = cardano_drep_voting_thresholds_new(
    motion_no_confidence,
    committee_normal,
    committee_no_confidence,
    update_constitution,
    hard_fork_initiation,
    pp_network_group,
    pp_economic_group,
    pp_technical_group,
    pp_governance_group,
    treasury_withdrawal,
    &thresholds);

  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&update_constitution);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&pp_network_group);
  cardano_unit_interval_unref(&pp_economic_group);
  cardano_unit_interval_unref(&pp_technical_group);
  cardano_unit_interval_unref(&pp_governance_group);
  cardano_unit_interval_unref(&treasury_withdrawal);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return thresholds;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_protocol_parameters_new(cardano_protocol_parameters_t** protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *protocol_parameters = _cardano_malloc(sizeof(cardano_protocol_parameters_t));

  if (*protocol_parameters == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*protocol_parameters)->base.deallocator   = cardano_protocol_parameters_deallocate;
  (*protocol_parameters)->base.ref_count     = 1;
  (*protocol_parameters)->base.last_error[0] = '\0';

  (*protocol_parameters)->min_fee_a                         = 0;
  (*protocol_parameters)->min_fee_b                         = 0;
  (*protocol_parameters)->max_block_body_size               = 0;
  (*protocol_parameters)->max_tx_size                       = 0;
  (*protocol_parameters)->max_block_header_size             = 0;
  (*protocol_parameters)->key_deposit                       = 0;
  (*protocol_parameters)->pool_deposit                      = 0;
  (*protocol_parameters)->max_epoch                         = 0;
  (*protocol_parameters)->n_opt                             = 0;
  (*protocol_parameters)->pool_pledge_influence             = cardano_get_one_interval();
  (*protocol_parameters)->expansion_rate                    = cardano_get_one_interval();
  (*protocol_parameters)->treasury_growth_rate              = cardano_get_one_interval();
  (*protocol_parameters)->d                                 = cardano_get_one_interval();
  (*protocol_parameters)->extra_entropy                     = cardano_buffer_new(1);
  (*protocol_parameters)->protocol_version                  = cardano_get_protocol_version();
  (*protocol_parameters)->min_pool_cost                     = 0;
  (*protocol_parameters)->ada_per_utxo_byte                 = 0;
  (*protocol_parameters)->cost_models                       = cardano_get_costmdls();
  (*protocol_parameters)->execution_costs                   = cardano_get_ex_unit_prices();
  (*protocol_parameters)->max_tx_ex_units                   = cardano_get_ex_unit();
  (*protocol_parameters)->max_block_ex_units                = cardano_get_ex_unit();
  (*protocol_parameters)->max_value_size                    = 0;
  (*protocol_parameters)->collateral_percentage             = 0;
  (*protocol_parameters)->max_collateral_inputs             = 0;
  (*protocol_parameters)->pool_voting_thresholds            = cardano_get_pool_voting_thresholds();
  (*protocol_parameters)->drep_voting_thresholds            = cardano_get_drep_voting_thresholds();
  (*protocol_parameters)->min_committee_size                = 0;
  (*protocol_parameters)->committee_term_limit              = 0;
  (*protocol_parameters)->governance_action_validity_period = 0;
  (*protocol_parameters)->governance_action_deposit         = 0;
  (*protocol_parameters)->drep_deposit                      = 0;
  (*protocol_parameters)->drep_inactivity_period            = 0;
  (*protocol_parameters)->ref_script_cost_per_byte          = cardano_get_one_interval();

  if (((*protocol_parameters)->pool_pledge_influence == NULL) || ((*protocol_parameters)->expansion_rate == NULL) || ((*protocol_parameters)->treasury_growth_rate == NULL) || ((*protocol_parameters)->d == NULL) || ((*protocol_parameters)->extra_entropy == NULL) || ((*protocol_parameters)->protocol_version == NULL) || ((*protocol_parameters)->cost_models == NULL) || ((*protocol_parameters)->execution_costs == NULL) || ((*protocol_parameters)->max_tx_ex_units == NULL) || ((*protocol_parameters)->max_block_ex_units == NULL) || ((*protocol_parameters)->pool_voting_thresholds == NULL) || ((*protocol_parameters)->drep_voting_thresholds == NULL) || ((*protocol_parameters)->ref_script_cost_per_byte == NULL))
  {
    cardano_protocol_parameters_unref(protocol_parameters);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

uint64_t
cardano_protocol_parameters_get_min_fee_a(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->min_fee_a;
}

uint64_t
cardano_protocol_parameters_get_min_fee_b(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->min_fee_b;
}

uint64_t
cardano_protocol_parameters_get_max_block_body_size(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->max_block_body_size;
}

uint64_t
cardano_protocol_parameters_get_max_tx_size(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->max_tx_size;
}

uint64_t
cardano_protocol_parameters_get_max_block_header_size(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->max_block_header_size;
}

uint64_t
cardano_protocol_parameters_get_key_deposit(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->key_deposit;
}

uint64_t
cardano_protocol_parameters_get_pool_deposit(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->pool_deposit;
}

uint64_t
cardano_protocol_parameters_get_max_epoch(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->max_epoch;
}

uint64_t
cardano_protocol_parameters_get_n_opt(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->n_opt;
}

cardano_unit_interval_t*
cardano_protocol_parameters_get_pool_pledge_influence(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_unit_interval_ref(protocol_parameters->pool_pledge_influence);
  return protocol_parameters->pool_pledge_influence;
}

cardano_unit_interval_t*
cardano_protocol_parameters_get_expansion_rate(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_unit_interval_ref(protocol_parameters->expansion_rate);
  return protocol_parameters->expansion_rate;
}

cardano_unit_interval_t*
cardano_protocol_parameters_get_treasury_growth_rate(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_unit_interval_ref(protocol_parameters->treasury_growth_rate);
  return protocol_parameters->treasury_growth_rate;
}

cardano_unit_interval_t*
cardano_protocol_parameters_get_d(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_unit_interval_ref(protocol_parameters->d);
  return protocol_parameters->d;
}

cardano_buffer_t*
cardano_protocol_parameters_get_extra_entropy(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_buffer_ref(protocol_parameters->extra_entropy);
  return protocol_parameters->extra_entropy;
}

cardano_protocol_version_t*
cardano_protocol_parameters_get_protocol_version(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_protocol_version_ref(protocol_parameters->protocol_version);
  return protocol_parameters->protocol_version;
}

uint64_t
cardano_protocol_parameters_get_min_pool_cost(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->min_pool_cost;
}

uint64_t
cardano_protocol_parameters_get_ada_per_utxo_byte(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->ada_per_utxo_byte;
}

cardano_costmdls_t*
cardano_protocol_parameters_get_cost_models(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_costmdls_ref(protocol_parameters->cost_models);
  return protocol_parameters->cost_models;
}

cardano_ex_unit_prices_t*
cardano_protocol_parameters_get_execution_costs(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_ex_unit_prices_ref(protocol_parameters->execution_costs);
  return protocol_parameters->execution_costs;
}

cardano_ex_units_t*
cardano_protocol_parameters_get_max_tx_ex_units(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_ex_units_ref(protocol_parameters->max_tx_ex_units);
  return protocol_parameters->max_tx_ex_units;
}

cardano_ex_units_t*
cardano_protocol_parameters_get_max_block_ex_units(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_ex_units_ref(protocol_parameters->max_block_ex_units);
  return protocol_parameters->max_block_ex_units;
}

uint64_t
cardano_protocol_parameters_get_max_value_size(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->max_value_size;
}

uint64_t
cardano_protocol_parameters_get_collateral_percentage(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->collateral_percentage;
}

uint64_t
cardano_protocol_parameters_get_max_collateral_inputs(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->max_collateral_inputs;
}

cardano_pool_voting_thresholds_t*
cardano_protocol_parameters_get_pool_voting_thresholds(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_pool_voting_thresholds_ref(protocol_parameters->pool_voting_thresholds);
  return protocol_parameters->pool_voting_thresholds;
}

cardano_drep_voting_thresholds_t*
cardano_protocol_parameters_get_drep_voting_thresholds(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_drep_voting_thresholds_ref(protocol_parameters->drep_voting_thresholds);
  return protocol_parameters->drep_voting_thresholds;
}

uint64_t
cardano_protocol_parameters_get_min_committee_size(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->min_committee_size;
}

uint64_t
cardano_protocol_parameters_get_committee_term_limit(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->committee_term_limit;
}

uint64_t
cardano_protocol_parameters_get_governance_action_validity_period(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->governance_action_validity_period;
}

uint64_t
cardano_protocol_parameters_get_governance_action_deposit(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->governance_action_deposit;
}

uint64_t
cardano_protocol_parameters_get_drep_deposit(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->drep_deposit;
}

uint64_t
cardano_protocol_parameters_get_drep_inactivity_period(
  const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return protocol_parameters->drep_inactivity_period;
}

cardano_unit_interval_t*
cardano_protocol_parameters_get_ref_script_cost_per_byte(
  cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return NULL;
  }

  cardano_unit_interval_ref(protocol_parameters->ref_script_cost_per_byte);
  return protocol_parameters->ref_script_cost_per_byte;
}

cardano_error_t
cardano_protocol_parameters_set_min_fee_a(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_fee_a)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->min_fee_a = min_fee_a;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_min_fee_b(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_fee_b)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->min_fee_b = min_fee_b;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_block_body_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_block_body_size)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->max_block_body_size = max_block_body_size;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_tx_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_tx_size)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->max_tx_size = max_tx_size;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_block_header_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_block_header_size)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->max_block_header_size = max_block_header_size;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_key_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       key_deposit)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->key_deposit = key_deposit;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_pool_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       pool_deposit)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->pool_deposit = pool_deposit;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_epoch(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_epoch)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->max_epoch = max_epoch;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_n_opt(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       n_opt)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->n_opt = n_opt;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_pool_pledge_influence(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       pool_pledge_influence)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(pool_pledge_influence);
  cardano_unit_interval_unref(&protocol_parameters->pool_pledge_influence);
  protocol_parameters->pool_pledge_influence = pool_pledge_influence;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_expansion_rate(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       expansion_rate)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(expansion_rate);
  cardano_unit_interval_unref(&protocol_parameters->expansion_rate);
  protocol_parameters->expansion_rate = expansion_rate;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_treasury_growth_rate(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       treasury_growth_rate)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(treasury_growth_rate);
  cardano_unit_interval_unref(&protocol_parameters->treasury_growth_rate);
  protocol_parameters->treasury_growth_rate = treasury_growth_rate;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_d(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       d)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(d);
  cardano_unit_interval_unref(&protocol_parameters->d);
  protocol_parameters->d = d;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_extra_entropy(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_buffer_t*              extra_entropy)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_ref(extra_entropy);
  cardano_buffer_unref(&protocol_parameters->extra_entropy);
  protocol_parameters->extra_entropy = extra_entropy;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_protocol_version(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_protocol_version_t*    protocol_version)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_protocol_version_ref(protocol_version);
  cardano_protocol_version_unref(&protocol_parameters->protocol_version);
  protocol_parameters->protocol_version = protocol_version;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_min_pool_cost(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_pool_cost)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->min_pool_cost = min_pool_cost;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_ada_per_utxo_byte(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       ada_per_utxo_byte)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->ada_per_utxo_byte = ada_per_utxo_byte;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_cost_models(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_costmdls_t*            cost_models)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_costmdls_ref(cost_models);
  cardano_costmdls_unref(&protocol_parameters->cost_models);
  protocol_parameters->cost_models = cost_models;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_execution_costs(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_ex_unit_prices_t*      execution_costs)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ex_unit_prices_ref(execution_costs);
  cardano_ex_unit_prices_unref(&protocol_parameters->execution_costs);
  protocol_parameters->execution_costs = execution_costs;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_tx_ex_units(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_ex_units_t*            max_tx_ex_units)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ex_units_ref(max_tx_ex_units);
  cardano_ex_units_unref(&protocol_parameters->max_tx_ex_units);
  protocol_parameters->max_tx_ex_units = max_tx_ex_units;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_block_ex_units(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_ex_units_t*            max_block_ex_units)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ex_units_ref(max_block_ex_units);
  cardano_ex_units_unref(&protocol_parameters->max_block_ex_units);
  protocol_parameters->max_block_ex_units = max_block_ex_units;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_value_size(
  cardano_protocol_parameters_t* protocol_parameters,
  int64_t                        max_value_size)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->max_value_size = max_value_size;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_collateral_percentage(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       collateral_percentage)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->collateral_percentage = collateral_percentage;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_max_collateral_inputs(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_collateral_inputs)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->max_collateral_inputs = max_collateral_inputs;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_pool_voting_thresholds(
  cardano_protocol_parameters_t*    protocol_parameters,
  cardano_pool_voting_thresholds_t* pool_voting_thresholds)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_pool_voting_thresholds_ref(pool_voting_thresholds);
  cardano_pool_voting_thresholds_unref(&protocol_parameters->pool_voting_thresholds);
  protocol_parameters->pool_voting_thresholds = pool_voting_thresholds;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_drep_voting_thresholds(
  cardano_protocol_parameters_t*    protocol_parameters,
  cardano_drep_voting_thresholds_t* drep_voting_thresholds)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_drep_voting_thresholds_ref(drep_voting_thresholds);
  cardano_drep_voting_thresholds_unref(&protocol_parameters->drep_voting_thresholds);
  protocol_parameters->drep_voting_thresholds = drep_voting_thresholds;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_min_committee_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_committee_size)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->min_committee_size = min_committee_size;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_committee_term_limit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       committee_term_limit)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->committee_term_limit = committee_term_limit;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_governance_action_validity_period(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       governance_action_validity_period)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->governance_action_validity_period = governance_action_validity_period;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_governance_action_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       governance_action_deposit)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->governance_action_deposit = governance_action_deposit;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_drep_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       drep_deposit)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->drep_deposit = drep_deposit;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_drep_inactivity_period(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       drep_inactivity_period)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_parameters->drep_inactivity_period = drep_inactivity_period;
  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_parameters_set_ref_script_cost_per_byte(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       ref_script_cost_per_byte)
{
  if (protocol_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(ref_script_cost_per_byte);
  cardano_unit_interval_unref(&protocol_parameters->ref_script_cost_per_byte);
  protocol_parameters->ref_script_cost_per_byte = ref_script_cost_per_byte;
  return CARDANO_SUCCESS;
}

void
cardano_protocol_parameters_unref(cardano_protocol_parameters_t** protocol_parameters)
{
  if ((protocol_parameters == NULL) || (*protocol_parameters == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*protocol_parameters)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *protocol_parameters = NULL;
    return;
  }
}

void
cardano_protocol_parameters_ref(cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return;
  }

  cardano_object_ref(&protocol_parameters->base);
}

size_t
cardano_protocol_parameters_refcount(const cardano_protocol_parameters_t* protocol_parameters)
{
  if (protocol_parameters == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&protocol_parameters->base);
}

void
cardano_protocol_parameters_set_last_error(cardano_protocol_parameters_t* protocol_parameters, const char* message)
{
  cardano_object_set_last_error(&protocol_parameters->base, message);
}

const char*
cardano_protocol_parameters_get_last_error(const cardano_protocol_parameters_t* protocol_parameters)
{
  return cardano_object_get_last_error(&protocol_parameters->base);
}