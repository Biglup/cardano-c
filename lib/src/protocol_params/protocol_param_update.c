/**
 * \file protocol_param_update.c
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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
#include <cardano/protocol_params/protocol_param_update.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief The ProtocolParamUpdate structure in Cardano is used to propose changes to
 * the protocol parameters of the blockchain. Protocol parameters govern various
 * aspects of the Cardano network.
 */
typedef struct cardano_protocol_param_update_t
{
    cardano_object_t                  base;
    uint64_t*                         min_fee_a;
    uint64_t*                         min_fee_b;
    uint64_t*                         max_block_body_size;
    uint64_t*                         max_tx_size;
    uint64_t*                         max_block_header_size;
    uint64_t*                         key_deposit;
    uint64_t*                         pool_deposit;
    uint64_t*                         max_epoch;
    uint64_t*                         n_opt;
    cardano_unit_interval_t*          pool_pledge_influence;
    cardano_unit_interval_t*          expansion_rate;
    cardano_unit_interval_t*          treasury_growth_rate;
    cardano_unit_interval_t*          d;
    cardano_buffer_t*                 extra_entropy;
    cardano_protocol_version_t*       protocol_version;
    uint64_t*                         min_pool_cost;
    uint64_t*                         ada_per_utxo_byte;
    cardano_costmdls_t*               cost_models;
    cardano_ex_unit_prices_t*         execution_costs;
    cardano_ex_units_t*               max_tx_ex_units;
    cardano_ex_units_t*               max_block_ex_units;
    uint64_t*                         max_value_size;
    uint64_t*                         collateral_percentage;
    uint64_t*                         max_collateral_inputs;
    cardano_pool_voting_thresholds_t* pool_voting_thresholds;
    cardano_drep_voting_thresholds_t* drep_voting_thresholds;
    uint64_t*                         min_committee_size;
    uint64_t*                         committee_term_limit;
    uint64_t*                         governance_action_validity_period;
    uint64_t*                         governance_action_deposit;
    uint64_t*                         drep_deposit;
    uint64_t*                         drep_inactivity_period;
    cardano_unit_interval_t*          ref_script_cost_per_byte;
} cardano_protocol_param_update_t;

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Type definition for a parameter handler function.
 *
 * This type defines a function pointer used for handling protocol parameter fields
 * when reading from a CBOR reader. Each handler function reads a specific type of
 * field from the CBOR reader and stores the result in the specified field pointer.
 *
 * The function pointed to by `param_handler_t` is responsible for:
 * - Reading the field value from the provided CBOR reader.
 * - Storing the field value in the provided field pointer.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the field value.
 * \param[out] field_ptr A pointer to the field where the read value should be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the field was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
typedef cardano_error_t (*param_handler_t)(cardano_cbor_reader_t*, void*);

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a protocol parameters update object.
 *
 * This function is responsible for properly deallocating a protocol parameters update object (`cardano_protocol_param_update_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the protocol_param_update object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_protocol_param_update_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the protocol_param_update
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_protocol_param_update_deallocate(void* object)
{
  assert(object != NULL);

  cardano_protocol_param_update_t* data = (cardano_protocol_param_update_t*)object;

  _cardano_free(data->min_fee_a);
  _cardano_free(data->min_fee_b);
  _cardano_free(data->max_block_body_size);
  _cardano_free(data->max_tx_size);
  _cardano_free(data->max_block_header_size);
  _cardano_free(data->key_deposit);
  _cardano_free(data->pool_deposit);
  _cardano_free(data->max_epoch);
  _cardano_free(data->n_opt);
  cardano_unit_interval_unref(&data->pool_pledge_influence);
  cardano_unit_interval_unref(&data->expansion_rate);
  cardano_unit_interval_unref(&data->treasury_growth_rate);
  cardano_unit_interval_unref(&data->d);
  cardano_buffer_unref(&data->extra_entropy);
  cardano_protocol_version_unref(&data->protocol_version);
  _cardano_free(data->min_pool_cost);
  _cardano_free(data->ada_per_utxo_byte);
  cardano_costmdls_unref(&data->cost_models);
  cardano_ex_unit_prices_unref(&data->execution_costs);
  cardano_ex_units_unref(&data->max_tx_ex_units);
  cardano_ex_units_unref(&data->max_block_ex_units);
  _cardano_free(data->max_value_size);
  _cardano_free(data->collateral_percentage);
  _cardano_free(data->max_collateral_inputs);
  cardano_pool_voting_thresholds_unref(&data->pool_voting_thresholds);
  cardano_drep_voting_thresholds_unref(&data->drep_voting_thresholds);
  _cardano_free(data->min_committee_size);
  _cardano_free(data->committee_term_limit);
  _cardano_free(data->governance_action_validity_period);
  _cardano_free(data->governance_action_deposit);
  _cardano_free(data->drep_deposit);
  _cardano_free(data->drep_inactivity_period);
  cardano_unit_interval_unref(&data->ref_script_cost_per_byte);

  _cardano_free(object);
}

/**
 * \brief Computes the map size of this update object.
 *
 * \param update The update object to compute the map size for.
 *
 * \return The map size of the update object.
 */
static size_t
get_map_size(const cardano_protocol_param_update_t* update)
{
  assert(update != NULL);

  size_t map_size = 0U;

  if (update->min_fee_a != NULL)
  {
    ++map_size;
  }

  if (update->min_fee_b != NULL)
  {
    ++map_size;
  }

  if (update->max_block_body_size != NULL)
  {
    ++map_size;
  }

  if (update->max_tx_size != NULL)
  {
    ++map_size;
  }

  if (update->max_block_header_size != NULL)
  {
    ++map_size;
  }

  if (update->key_deposit != NULL)
  {
    ++map_size;
  }

  if (update->pool_deposit != NULL)
  {
    ++map_size;
  }

  if (update->max_epoch != NULL)
  {
    ++map_size;
  }

  if (update->n_opt != NULL)
  {
    ++map_size;
  }

  if (update->pool_pledge_influence != NULL)
  {
    ++map_size;
  }

  if (update->expansion_rate != NULL)
  {
    ++map_size;
  }

  if (update->treasury_growth_rate != NULL)
  {
    ++map_size;
  }

  if (update->d != NULL)
  {
    ++map_size;
  }

  if (update->extra_entropy != NULL)
  {
    ++map_size;
  }

  if (update->protocol_version != NULL)
  {
    ++map_size;
  }

  if (update->min_pool_cost != NULL)
  {
    ++map_size;
  }

  if (update->ada_per_utxo_byte != NULL)
  {
    ++map_size;
  }

  if (update->cost_models != NULL)
  {
    ++map_size;
  }

  if (update->execution_costs != NULL)
  {
    ++map_size;
  }

  if (update->max_tx_ex_units != NULL)
  {
    ++map_size;
  }

  if (update->max_block_ex_units != NULL)
  {
    ++map_size;
  }

  if (update->max_value_size != NULL)
  {
    ++map_size;
  }

  if (update->collateral_percentage != NULL)
  {
    ++map_size;
  }

  if (update->max_collateral_inputs != NULL)
  {
    ++map_size;
  }

  if (update->pool_voting_thresholds != NULL)
  {
    ++map_size;
  }

  if (update->drep_voting_thresholds != NULL)
  {
    ++map_size;
  }

  if (update->min_committee_size != NULL)
  {
    ++map_size;
  }

  if (update->committee_term_limit != NULL)
  {
    ++map_size;
  }

  if (update->governance_action_validity_period != NULL)
  {
    ++map_size;
  }

  if (update->governance_action_deposit != NULL)
  {
    ++map_size;
  }

  if (update->drep_deposit != NULL)
  {
    ++map_size;
  }

  if (update->drep_inactivity_period != NULL)
  {
    ++map_size;
  }

  if (update->ref_script_cost_per_byte != NULL)
  {
    ++map_size;
  }

  return map_size;
}

/**
 * \brief Retrieves a pointer to the specified field in the protocol parameter update object.
 *
 * This function returns a pointer to the field specified by the key in the given protocol
 * parameter update object. The key corresponds to different fields within the update object.
 * If the key does not correspond to any field, NULL is returned.
 *
 * \param[in] update Pointer to the protocol parameter update object.
 *
 * \return void* A pointer to the specified field, or NULL if the key does not correspond to any field.
 *
 * \note The caller must ensure that the `update` pointer is not NULL before calling this function.
 */
static void*
get_field_ptr(cardano_protocol_param_update_t* update, size_t key)
{
  assert(update != NULL);

  switch (key)
  {
    case 0:
      return (void*)&update->min_fee_a;
    case 1:
      return (void*)&update->min_fee_b;
    case 2:
      return (void*)&update->max_block_body_size;
    case 3:
      return (void*)&update->max_tx_size;
    case 4:
      return (void*)&update->max_block_header_size;
    case 5:
      return (void*)&update->key_deposit;
    case 6:
      return (void*)&update->pool_deposit;
    case 7:
      return (void*)&update->max_epoch;
    case 8:
      return (void*)&update->n_opt;
    case 9:
      return (void*)&update->pool_pledge_influence;
    case 10:
      return (void*)&update->expansion_rate;
    case 11:
      return (void*)&update->treasury_growth_rate;
    case 12:
      return (void*)&update->d;
    case 13:
      return (void*)&update->extra_entropy;
    case 14:
      return (void*)&update->protocol_version;
    case 16:
      return (void*)&update->min_pool_cost;
    case 17:
      return (void*)&update->ada_per_utxo_byte;
    case 18:
      return (void*)&update->cost_models;
    case 19:
      return (void*)&update->execution_costs;
    case 20:
      return (void*)&update->max_tx_ex_units;
    case 21:
      return (void*)&update->max_block_ex_units;
    case 22:
      return (void*)&update->max_value_size;
    case 23:
      return (void*)&update->collateral_percentage;
    case 24:
      return (void*)&update->max_collateral_inputs;
    case 25:
      return (void*)&update->pool_voting_thresholds;
    case 26:
      return (void*)&update->drep_voting_thresholds;
    case 27:
      return (void*)&update->min_committee_size;
    case 28:
      return (void*)&update->committee_term_limit;
    case 29:
      return (void*)&update->governance_action_validity_period;
    case 30:
      return (void*)&update->governance_action_deposit;
    case 31:
      return (void*)&update->drep_deposit;
    case 32:
      return (void*)&update->drep_inactivity_period;
    case 33:
      return (void*)&update->ref_script_cost_per_byte;

    default:
      return NULL;
  }
}

/**
 * \brief Reads a uint64_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a uint64_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as uint64_t in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the uint64_t value.
 * \param[out] field_ptr A pointer to the field where the read uint64_t value should be stored.
 *                       The field pointer should be of type uint64_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the uint64_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_uint64(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  uint64_t** field = (uint64_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  *field = _cardano_malloc(sizeof(uint64_t));

  if (*field == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return cardano_cbor_reader_read_uint(reader, *field);
}

/**
 * \brief Reads a unit interval value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a unit interval value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_unit_interval_t` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the unit interval value.
 * \param[out] field_ptr A pointer to the field where the read unit interval value should be stored.
 *                       The field pointer should be of type `cardano_unit_interval_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the unit interval value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_unit_interval(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_unit_interval_t** field = (cardano_unit_interval_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_unit_interval_from_cbor(reader, field);
}

/**
 * \brief Reads an entropy value from the CBOR reader and stores it in the specified field.
 *
 * This function reads an entropy value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_buffer_t*` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the entropy value.
 * \param[out] field_ptr A pointer to the field where the read entropy value should be stored.
 *                       The field pointer should be of type `cardano_buffer_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the entropy value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_entropy(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_buffer_t** field = (cardano_buffer_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  int64_t array_size = 0;

  cardano_error_t read_extra_entropy_array = cardano_cbor_reader_read_start_array(reader, &array_size);

  if (read_extra_entropy_array != CARDANO_SUCCESS)
  {
    return read_extra_entropy_array;
  }

  uint64_t entropy_key = 0U;

  cardano_error_t read_extra_entropy = cardano_cbor_reader_read_uint(reader, &entropy_key);

  if (read_extra_entropy != CARDANO_SUCCESS)
  {
    return read_extra_entropy;
  }

  if (array_size == 1)
  {
    (*field) = cardano_buffer_new(1);

    return cardano_cbor_validate_end_array("entropy", reader);
  }

  if (array_size != 2)
  {
    return CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE;
  }

  read_extra_entropy = cardano_cbor_reader_read_bytestring(reader, field);

  if (read_extra_entropy != CARDANO_SUCCESS)
  {
    return read_extra_entropy;
  }

  return cardano_cbor_validate_end_array("entropy", reader);
}

/**
 * \brief Reads a protocol version value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a protocol version value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_protocol_version_t*` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the protocol version value.
 * \param[out] field_ptr A pointer to the field where the read protocol version value should be stored.
 *                       The field pointer should be of type `cardano_protocol_version_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the protocol version value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_protocol_version(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_protocol_version_t** field = (cardano_protocol_version_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_protocol_version_from_cbor(reader, field);
}

/**
 * \brief Reads a cost models value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cost models value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_costmdls_t*` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cost models value.
 * \param[out] field_ptr A pointer to the field where the read cost models value should be stored.
 *                       The field pointer should be of type `cardano_costmdls_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cost models value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_cost_models(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_costmdls_t** field = (cardano_costmdls_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_costmdls_from_cbor(reader, field);
}

/**
 * \brief Reads an execution unit prices value from the CBOR reader and stores it in the specified field.
 *
 * This function reads an execution unit prices value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_ex_unit_prices_t*` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the execution unit prices value.
 * \param[out] field_ptr A pointer to the field where the read execution unit prices value should be stored.
 *                       The field pointer should be of type `cardano_ex_unit_prices_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the execution unit prices value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_ex_unit_prices(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_ex_unit_prices_t** field = (cardano_ex_unit_prices_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_ex_unit_prices_from_cbor(reader, field);
}

/**
 * \brief Reads an execution unit prices value from the CBOR reader and stores it in the specified field.
 *
 * This function reads an execution unit prices value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_ex_unit_prices_t*` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the execution unit prices value.
 * \param[out] field_ptr A pointer to the field where the read execution unit prices value should be stored.
 *                       The field pointer should be of type `cardano_ex_unit_prices_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the execution unit prices value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_ex_units(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_ex_units_t** field = (cardano_ex_units_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_ex_units_from_cbor(reader, field);
}

/**
 * \brief Reads a pool voting thresholds value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a pool voting thresholds value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_pool_voting_thresholds_t*` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the pool voting thresholds value.
 * \param[out] field_ptr A pointer to the field where the read pool voting thresholds value should be stored.
 *                       The field pointer should be of type `cardano_pool_voting_thresholds_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the pool voting thresholds value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_pool_voting_thresholds(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_pool_voting_thresholds_t** field = (cardano_pool_voting_thresholds_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_pool_voting_thresholds_from_cbor(reader, field);
}

/**
 * \brief Reads a DRep voting thresholds value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a DRep voting thresholds value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as `cardano_drep_voting_thresholds_t*` in the protocol parameter update.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the DRep voting thresholds value.
 * \param[out] field_ptr A pointer to the field where the read DRep voting thresholds value should be stored.
 *                       The field pointer should be of type `cardano_drep_voting_thresholds_t**`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the DRep voting thresholds value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_drep_voting_thresholds(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_drep_voting_thresholds_t** field = (cardano_drep_voting_thresholds_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_drep_voting_thresholds_from_cbor(reader, field);
}

/**
 * \brief Handles an invalid key in the CBOR map during deserialization.
 *
 * This function is used as a handler for invalid keys encountered in the CBOR map.
 * It does nothing with the provided reader and field pointer, and always returns
 * \ref CARDANO_ERROR_INVALID_CBOR_MAP_KEY to indicate the error.
 *
 * \param[in] reader A pointer to the CBOR reader. This parameter is unused.
 * \param[in] field_ptr A pointer to the field where the value should be stored. This parameter is unused.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - Always returns \ref CARDANO_ERROR_INVALID_CBOR_MAP_KEY.
 */
static cardano_error_t
handle_invalid_key(cardano_cbor_reader_t* reader, void* field_ptr)
{
  CARDANO_UNUSED(reader);
  CARDANO_UNUSED(field_ptr);

  return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated unsigned integer value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the unsigned integer value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_uint_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const uint64_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, *value);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated unit interval value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the unit interval value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_unit_interval_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_unit_interval_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_unit_interval_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair for entropy to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated entropy value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the entropy buffer value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_entropy_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_buffer_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (cardano_buffer_get_size(value) == 0U)
    {
      result = cardano_cbor_writer_write_start_array(writer, 1U);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_cbor_writer_write_uint(writer, 0U);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      return CARDANO_SUCCESS;
    }

    result = cardano_cbor_writer_write_start_array(writer, 2U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, 1U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_bytestring(writer, cardano_buffer_get_data(value), cardano_buffer_get_size(value));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair for the protocol version to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated protocol version value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the protocol version value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_protocol_version_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_protocol_version_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_protocol_version_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair for the cost models to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated cost models value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the cost models value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_cost_models_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_costmdls_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_costmdls_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair for the ex unit prices to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated ex unit prices value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the ex unit prices value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_ex_unit_prices_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_ex_unit_prices_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_ex_unit_prices_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair for the execution unit prices to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated execution unit prices value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the execution unit prices value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_ex_units_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_ex_units_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_ex_units_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair for the pool voting thresholds to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated pool voting thresholds value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the pool voting thresholds value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_pool_voting_thresholds_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_pool_voting_thresholds_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_pool_voting_thresholds_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Writes a key-value pair for the DRep voting thresholds to the CBOR writer if the value is present.
 *
 * This function writes a key and its associated DRep voting thresholds value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the DRep voting thresholds value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_drep_voting_thresholds_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_drep_voting_thresholds_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_drep_voting_thresholds_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/* STATIC CONSTANTS ***********************************************************/

// cppcheck-suppress misra-c2012-8.9; Reason: Is more readable to define the map here
static const param_handler_t param_handlers[] = {
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_unit_interval,
  handle_unit_interval,
  handle_unit_interval,
  handle_unit_interval,
  handle_entropy,
  handle_protocol_version,
  handle_invalid_key, // unused key
  handle_uint64,
  handle_uint64,
  handle_cost_models,
  handle_ex_unit_prices,
  handle_ex_units,
  handle_ex_units,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_pool_voting_thresholds,
  handle_drep_voting_thresholds,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_uint64,
  handle_unit_interval
};

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_protocol_param_update_new(cardano_protocol_param_update_t** protocol_param_update)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *protocol_param_update = _cardano_malloc(sizeof(cardano_protocol_param_update_t));

  if (*protocol_param_update == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*protocol_param_update)->base.deallocator   = cardano_protocol_param_update_deallocate;
  (*protocol_param_update)->base.ref_count     = 1;
  (*protocol_param_update)->base.last_error[0] = '\0';

  (*protocol_param_update)->min_fee_a                         = NULL;
  (*protocol_param_update)->min_fee_b                         = NULL;
  (*protocol_param_update)->max_block_body_size               = NULL;
  (*protocol_param_update)->max_tx_size                       = NULL;
  (*protocol_param_update)->max_block_header_size             = NULL;
  (*protocol_param_update)->key_deposit                       = NULL;
  (*protocol_param_update)->pool_deposit                      = NULL;
  (*protocol_param_update)->max_epoch                         = NULL;
  (*protocol_param_update)->n_opt                             = NULL;
  (*protocol_param_update)->pool_pledge_influence             = NULL;
  (*protocol_param_update)->expansion_rate                    = NULL;
  (*protocol_param_update)->treasury_growth_rate              = NULL;
  (*protocol_param_update)->d                                 = NULL;
  (*protocol_param_update)->extra_entropy                     = NULL;
  (*protocol_param_update)->protocol_version                  = NULL;
  (*protocol_param_update)->min_pool_cost                     = NULL;
  (*protocol_param_update)->ada_per_utxo_byte                 = NULL;
  (*protocol_param_update)->cost_models                       = NULL;
  (*protocol_param_update)->execution_costs                   = NULL;
  (*protocol_param_update)->max_tx_ex_units                   = NULL;
  (*protocol_param_update)->max_block_ex_units                = NULL;
  (*protocol_param_update)->max_value_size                    = NULL;
  (*protocol_param_update)->collateral_percentage             = NULL;
  (*protocol_param_update)->max_collateral_inputs             = NULL;
  (*protocol_param_update)->pool_voting_thresholds            = NULL;
  (*protocol_param_update)->drep_voting_thresholds            = NULL;
  (*protocol_param_update)->min_committee_size                = NULL;
  (*protocol_param_update)->committee_term_limit              = NULL;
  (*protocol_param_update)->governance_action_validity_period = NULL;
  (*protocol_param_update)->governance_action_deposit         = NULL;
  (*protocol_param_update)->drep_deposit                      = NULL;
  (*protocol_param_update)->drep_inactivity_period            = NULL;
  (*protocol_param_update)->ref_script_cost_per_byte          = NULL;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_from_cbor(cardano_cbor_reader_t* reader, cardano_protocol_param_update_t** protocol_param_update)
{
  if (!reader || !protocol_param_update)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *protocol_param_update                  = NULL;
  cardano_protocol_param_update_t* update = NULL;
  cardano_error_t                  result = cardano_protocol_param_update_new(&update);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t map_size = 0;
  result           = cardano_cbor_reader_read_start_map(reader, &map_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_protocol_param_update_unref(&update);

    return result;
  }

  for (size_t i = 0U; i < (size_t)map_size; ++i)
  {
    uint64_t key = 0;
    result       = cardano_cbor_reader_read_uint(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_protocol_param_update_unref(&update);

      return result;
    }

    if (key >= (sizeof(param_handlers) / sizeof(param_handlers[0])))
    {
      cardano_protocol_param_update_unref(&update);

      return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
    }

    void* field_ptr = get_field_ptr(update, key);

    if (field_ptr == NULL)
    {
      cardano_protocol_param_update_unref(&update);

      return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
    }

    result = param_handlers[key](reader, field_ptr);

    if (result != CARDANO_SUCCESS)
    {
      cardano_protocol_param_update_unref(&update);

      return result;
    }
  }

  *protocol_param_update = update;

  return cardano_cbor_validate_end_map("protocol_param_update", reader);
}

cardano_error_t
cardano_protocol_param_update_to_cbor(const cardano_protocol_param_update_t* protocol_param_update, cardano_cbor_writer_t* writer)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t map_size = get_map_size(protocol_param_update);

  cardano_error_t result = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 0U, protocol_param_update->min_fee_a);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 1U, protocol_param_update->min_fee_b);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 2U, protocol_param_update->max_block_body_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 3U, protocol_param_update->max_tx_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 4U, protocol_param_update->max_block_header_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 5U, protocol_param_update->key_deposit);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 6U, protocol_param_update->pool_deposit);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 7U, protocol_param_update->max_epoch);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 8U, protocol_param_update->n_opt);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_unit_interval_if_present(writer, 9U, protocol_param_update->pool_pledge_influence);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_unit_interval_if_present(writer, 10U, protocol_param_update->expansion_rate);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_unit_interval_if_present(writer, 11U, protocol_param_update->treasury_growth_rate);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_unit_interval_if_present(writer, 12U, protocol_param_update->d);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_entropy_if_present(writer, 13U, protocol_param_update->extra_entropy);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_protocol_version_if_present(writer, 14U, protocol_param_update->protocol_version);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 16U, protocol_param_update->min_pool_cost);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 17U, protocol_param_update->ada_per_utxo_byte);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_cost_models_if_present(writer, 18U, protocol_param_update->cost_models);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_ex_unit_prices_if_present(writer, 19U, protocol_param_update->execution_costs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_ex_units_if_present(writer, 20U, protocol_param_update->max_tx_ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_ex_units_if_present(writer, 21U, protocol_param_update->max_block_ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 22U, protocol_param_update->max_value_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 23U, protocol_param_update->collateral_percentage);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 24U, protocol_param_update->max_collateral_inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_pool_voting_thresholds_if_present(writer, 25U, protocol_param_update->pool_voting_thresholds);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_drep_voting_thresholds_if_present(writer, 26U, protocol_param_update->drep_voting_thresholds);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 27U, protocol_param_update->min_committee_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 28U, protocol_param_update->committee_term_limit);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 29U, protocol_param_update->governance_action_validity_period);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 30U, protocol_param_update->governance_action_deposit);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 31U, protocol_param_update->drep_deposit);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 32U, protocol_param_update->drep_inactivity_period);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_unit_interval_if_present(writer, 33U, protocol_param_update->ref_script_cost_per_byte);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_min_fee_a(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_fee_a)
{
  if ((protocol_param_update == NULL) || (min_fee_a == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_fee_a == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *min_fee_a = *protocol_param_update->min_fee_a;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_min_fee_b(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_fee_b)
{
  if ((protocol_param_update == NULL) || (min_fee_b == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_fee_b == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *min_fee_b = *protocol_param_update->min_fee_b;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_block_body_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_block_body_size)
{
  if ((protocol_param_update == NULL) || (max_block_body_size == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_block_body_size == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *max_block_body_size = *protocol_param_update->max_block_body_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_tx_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_tx_size)
{
  if ((protocol_param_update == NULL) || (max_tx_size == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_tx_size == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *max_tx_size = *protocol_param_update->max_tx_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_block_header_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_block_header_size)
{
  if ((protocol_param_update == NULL) || (max_block_header_size == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_block_header_size == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *max_block_header_size = *protocol_param_update->max_block_header_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_key_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              key_deposit)
{
  if ((protocol_param_update == NULL) || (key_deposit == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->key_deposit == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *key_deposit = *protocol_param_update->key_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_pool_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              pool_deposit)
{
  if ((protocol_param_update == NULL) || (pool_deposit == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->pool_deposit == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *pool_deposit = *protocol_param_update->pool_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_epoch(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_epoch)
{
  if ((protocol_param_update == NULL) || (max_epoch == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_epoch == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *max_epoch = *protocol_param_update->max_epoch;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_n_opt(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              n_opt)
{
  if ((protocol_param_update == NULL) || (n_opt == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->n_opt == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *n_opt = *protocol_param_update->n_opt;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_pool_pledge_influence(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        pool_pledge_influence)
{
  if ((protocol_param_update == NULL) || (pool_pledge_influence == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->pool_pledge_influence == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_unit_interval_ref(protocol_param_update->pool_pledge_influence);
  *pool_pledge_influence = protocol_param_update->pool_pledge_influence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_expansion_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        expansion_rate)
{
  if ((protocol_param_update == NULL) || (expansion_rate == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->expansion_rate == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_unit_interval_ref(protocol_param_update->expansion_rate);
  *expansion_rate = protocol_param_update->expansion_rate;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_treasury_growth_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        treasury_growth_rate)
{
  if ((protocol_param_update == NULL) || (treasury_growth_rate == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->treasury_growth_rate == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_unit_interval_ref(protocol_param_update->treasury_growth_rate);
  *treasury_growth_rate = protocol_param_update->treasury_growth_rate;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_d(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        d)
{
  if ((protocol_param_update == NULL) || (d == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->d == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_unit_interval_ref(protocol_param_update->d);
  *d = protocol_param_update->d;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_extra_entropy(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_buffer_t**               extra_entropy)
{
  if ((protocol_param_update == NULL) || (extra_entropy == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->extra_entropy == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_buffer_ref(protocol_param_update->extra_entropy);
  *extra_entropy = protocol_param_update->extra_entropy;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_protocol_version(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_protocol_version_t**     protocol_version)
{
  if ((protocol_param_update == NULL) || (protocol_version == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->protocol_version == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_protocol_version_ref(protocol_param_update->protocol_version);
  *protocol_version = protocol_param_update->protocol_version;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_min_pool_cost(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_pool_cost)
{
  if ((protocol_param_update == NULL) || (min_pool_cost == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_pool_cost == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *min_pool_cost = *protocol_param_update->min_pool_cost;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_ada_per_utxo_byte(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              ada_per_utxo_byte)
{
  if ((protocol_param_update == NULL) || (ada_per_utxo_byte == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->ada_per_utxo_byte == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *ada_per_utxo_byte = *protocol_param_update->ada_per_utxo_byte;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_cost_models(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_costmdls_t**             cost_models)
{
  if ((protocol_param_update == NULL) || (cost_models == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->cost_models == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_costmdls_ref(protocol_param_update->cost_models);
  *cost_models = protocol_param_update->cost_models;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_execution_costs(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_unit_prices_t**       execution_costs)
{
  if ((protocol_param_update == NULL) || (execution_costs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->execution_costs == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_ex_unit_prices_ref(protocol_param_update->execution_costs);
  *execution_costs = protocol_param_update->execution_costs;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_tx_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t**             max_tx_ex_units)
{
  if ((protocol_param_update == NULL) || (max_tx_ex_units == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_tx_ex_units == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_ex_units_ref(protocol_param_update->max_tx_ex_units);
  *max_tx_ex_units = protocol_param_update->max_tx_ex_units;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_block_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t**             max_block_ex_units)
{
  if ((protocol_param_update == NULL) || (max_block_ex_units == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_block_ex_units == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_ex_units_ref(protocol_param_update->max_block_ex_units);
  *max_block_ex_units = protocol_param_update->max_block_ex_units;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_value_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_value_size)
{
  if ((protocol_param_update == NULL) || (max_value_size == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_value_size == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *max_value_size = *protocol_param_update->max_value_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_collateral_percentage(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              collateral_percentage)
{
  if ((protocol_param_update == NULL) || (collateral_percentage == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->collateral_percentage == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *collateral_percentage = *protocol_param_update->collateral_percentage;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_max_collateral_inputs(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_collateral_inputs)
{
  if ((protocol_param_update == NULL) || (max_collateral_inputs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_collateral_inputs == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *max_collateral_inputs = *protocol_param_update->max_collateral_inputs;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_pool_voting_thresholds(
  cardano_protocol_param_update_t*   protocol_param_update,
  cardano_pool_voting_thresholds_t** pool_voting_thresholds)
{
  if ((protocol_param_update == NULL) || (pool_voting_thresholds == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_pool_voting_thresholds_ref(protocol_param_update->pool_voting_thresholds);
  *pool_voting_thresholds = protocol_param_update->pool_voting_thresholds;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_drep_voting_thresholds(
  cardano_protocol_param_update_t*   protocol_param_update,
  cardano_drep_voting_thresholds_t** drep_voting_thresholds)
{
  if ((protocol_param_update == NULL) || (drep_voting_thresholds == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_drep_voting_thresholds_ref(protocol_param_update->drep_voting_thresholds);
  *drep_voting_thresholds = protocol_param_update->drep_voting_thresholds;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_min_committee_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_committee_size)
{
  if ((protocol_param_update == NULL) || (min_committee_size == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_committee_size == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *min_committee_size = *protocol_param_update->min_committee_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_committee_term_limit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              committee_term_limit)
{
  if ((protocol_param_update == NULL) || (committee_term_limit == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->committee_term_limit == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *committee_term_limit = *protocol_param_update->committee_term_limit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_governance_action_validity_period(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              governance_action_validity_period)
{
  if ((protocol_param_update == NULL) || (governance_action_validity_period == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->governance_action_validity_period == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *governance_action_validity_period = *protocol_param_update->governance_action_validity_period;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_governance_action_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              governance_action_deposit)
{
  if ((protocol_param_update == NULL) || (governance_action_deposit == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->governance_action_deposit == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *governance_action_deposit = *protocol_param_update->governance_action_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_drep_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              drep_deposit)
{
  if ((protocol_param_update == NULL) || (drep_deposit == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->drep_deposit == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *drep_deposit = *protocol_param_update->drep_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_drep_inactivity_period(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              drep_inactivity_period)
{
  if ((protocol_param_update == NULL) || (drep_inactivity_period == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->drep_inactivity_period == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *drep_inactivity_period = *protocol_param_update->drep_inactivity_period;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_get_ref_script_cost_per_byte(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        ref_script_cost_per_byte)
{
  if ((protocol_param_update == NULL) || (ref_script_cost_per_byte == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->ref_script_cost_per_byte == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_unit_interval_ref(protocol_param_update->ref_script_cost_per_byte);
  *ref_script_cost_per_byte = protocol_param_update->ref_script_cost_per_byte;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_min_fee_a(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_fee_a)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_fee_a != NULL)
  {
    _cardano_free(protocol_param_update->min_fee_a);
  }

  if (min_fee_a == NULL)
  {
    protocol_param_update->min_fee_a = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->min_fee_a = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->min_fee_a == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->min_fee_a = *min_fee_a;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_min_fee_b(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_fee_b)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_fee_b != NULL)
  {
    _cardano_free(protocol_param_update->min_fee_b);
  }

  if (min_fee_b == NULL)
  {
    protocol_param_update->min_fee_b = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->min_fee_b = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->min_fee_b == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->min_fee_b = *min_fee_b;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_block_body_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_block_body_size)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_block_body_size != NULL)
  {
    _cardano_free(protocol_param_update->max_block_body_size);
  }

  if (max_block_body_size == NULL)
  {
    protocol_param_update->max_block_body_size = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->max_block_body_size = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->max_block_body_size == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->max_block_body_size = *max_block_body_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_tx_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_tx_size)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_tx_size != NULL)
  {
    _cardano_free(protocol_param_update->max_tx_size);
  }

  if (max_tx_size == NULL)
  {
    protocol_param_update->max_tx_size = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->max_tx_size = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->max_tx_size == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->max_tx_size = *max_tx_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_block_header_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_block_header_size)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_block_header_size != NULL)
  {
    _cardano_free(protocol_param_update->max_block_header_size);
  }

  if (max_block_header_size == NULL)
  {
    protocol_param_update->max_block_header_size = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->max_block_header_size = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->max_block_header_size == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->max_block_header_size = *max_block_header_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_key_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  key_deposit)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->key_deposit != NULL)
  {
    _cardano_free(protocol_param_update->key_deposit);
  }

  if (key_deposit == NULL)
  {
    protocol_param_update->key_deposit = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->key_deposit = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->key_deposit == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->key_deposit = *key_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_pool_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  pool_deposit)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->pool_deposit != NULL)
  {
    _cardano_free(protocol_param_update->pool_deposit);
  }

  if (pool_deposit == NULL)
  {
    protocol_param_update->pool_deposit = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->pool_deposit = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->pool_deposit == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->pool_deposit = *pool_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_epoch(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_epoch)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_epoch != NULL)
  {
    _cardano_free(protocol_param_update->max_epoch);
  }

  if (max_epoch == NULL)
  {
    protocol_param_update->max_epoch = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->max_epoch = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->max_epoch == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->max_epoch = *max_epoch;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_n_opt(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  n_opt)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->n_opt != NULL)
  {
    _cardano_free(protocol_param_update->n_opt);
  }

  if (n_opt == NULL)
  {
    protocol_param_update->n_opt = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->n_opt = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->n_opt == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->n_opt = *n_opt;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_pool_pledge_influence(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         pool_pledge_influence)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->pool_pledge_influence != NULL)
  {
    cardano_unit_interval_unref(&protocol_param_update->pool_pledge_influence);
  }

  if (pool_pledge_influence == NULL)
  {
    protocol_param_update->pool_pledge_influence = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_unit_interval_ref(pool_pledge_influence);
  protocol_param_update->pool_pledge_influence = (cardano_unit_interval_t*)pool_pledge_influence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_expansion_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         expansion_rate)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->expansion_rate != NULL)
  {
    cardano_unit_interval_unref(&protocol_param_update->expansion_rate);
  }

  if (expansion_rate == NULL)
  {
    protocol_param_update->expansion_rate = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_unit_interval_ref(expansion_rate);
  protocol_param_update->expansion_rate = (cardano_unit_interval_t*)expansion_rate;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_treasury_growth_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         treasury_growth_rate)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->treasury_growth_rate != NULL)
  {
    cardano_unit_interval_unref(&protocol_param_update->treasury_growth_rate);
  }

  if (treasury_growth_rate == NULL)
  {
    protocol_param_update->treasury_growth_rate = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_unit_interval_ref(treasury_growth_rate);
  protocol_param_update->treasury_growth_rate = (cardano_unit_interval_t*)treasury_growth_rate;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_d(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         d)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->d != NULL)
  {
    cardano_unit_interval_unref(&protocol_param_update->d);
  }

  if (d == NULL)
  {
    protocol_param_update->d = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_unit_interval_ref(d);
  protocol_param_update->d = (cardano_unit_interval_t*)d;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_extra_entropy(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_buffer_t*                extra_entropy)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->extra_entropy != NULL)
  {
    cardano_buffer_unref(&protocol_param_update->extra_entropy);
  }

  if (extra_entropy == NULL)
  {
    protocol_param_update->extra_entropy = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_buffer_ref(extra_entropy);
  protocol_param_update->extra_entropy = (cardano_buffer_t*)extra_entropy;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_protocol_version(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_protocol_version_t*      protocol_version)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->protocol_version != NULL)
  {
    cardano_protocol_version_unref(&protocol_param_update->protocol_version);
  }

  if (protocol_version == NULL)
  {
    protocol_param_update->protocol_version = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_protocol_version_ref(protocol_version);
  protocol_param_update->protocol_version = (cardano_protocol_version_t*)protocol_version;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_min_pool_cost(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_pool_cost)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_pool_cost != NULL)
  {
    _cardano_free(protocol_param_update->min_pool_cost);
  }

  if (min_pool_cost == NULL)
  {
    protocol_param_update->min_pool_cost = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->min_pool_cost = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->min_pool_cost == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->min_pool_cost = *min_pool_cost;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_ada_per_utxo_byte(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  ada_per_utxo_byte)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->ada_per_utxo_byte != NULL)
  {
    _cardano_free(protocol_param_update->ada_per_utxo_byte);
  }

  if (ada_per_utxo_byte == NULL)
  {
    protocol_param_update->ada_per_utxo_byte = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->ada_per_utxo_byte = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->ada_per_utxo_byte == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->ada_per_utxo_byte = *ada_per_utxo_byte;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_cost_models(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_costmdls_t*              cost_models)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->cost_models != NULL)
  {
    cardano_costmdls_unref(&protocol_param_update->cost_models);
  }

  if (cost_models == NULL)
  {
    protocol_param_update->cost_models = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_costmdls_ref(cost_models);
  protocol_param_update->cost_models = (cardano_costmdls_t*)cost_models;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_execution_costs(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_unit_prices_t*        execution_costs)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->execution_costs != NULL)
  {
    cardano_ex_unit_prices_unref(&protocol_param_update->execution_costs);
  }

  if (execution_costs == NULL)
  {
    protocol_param_update->execution_costs = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_ex_unit_prices_ref(execution_costs);
  protocol_param_update->execution_costs = (cardano_ex_unit_prices_t*)execution_costs;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_tx_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t*              max_tx_ex_units)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_tx_ex_units != NULL)
  {
    cardano_ex_units_unref(&protocol_param_update->max_tx_ex_units);
  }

  if (max_tx_ex_units == NULL)
  {
    protocol_param_update->max_tx_ex_units = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_ex_units_ref(max_tx_ex_units);
  protocol_param_update->max_tx_ex_units = (cardano_ex_units_t*)max_tx_ex_units;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_block_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t*              max_block_ex_units)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_block_ex_units != NULL)
  {
    cardano_ex_units_unref(&protocol_param_update->max_block_ex_units);
  }

  if (max_block_ex_units == NULL)
  {
    protocol_param_update->max_block_ex_units = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_ex_units_ref(max_block_ex_units);
  protocol_param_update->max_block_ex_units = (cardano_ex_units_t*)max_block_ex_units;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_value_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_value_size)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_value_size != NULL)
  {
    _cardano_free(protocol_param_update->max_value_size);
  }

  if (max_value_size == NULL)
  {
    protocol_param_update->max_value_size = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->max_value_size = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->max_value_size == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->max_value_size = *max_value_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_collateral_percentage(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  collateral_percentage)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->collateral_percentage != NULL)
  {
    _cardano_free(protocol_param_update->collateral_percentage);
  }

  if (collateral_percentage == NULL)
  {
    protocol_param_update->collateral_percentage = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->collateral_percentage = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->collateral_percentage == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->collateral_percentage = *collateral_percentage;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_max_collateral_inputs(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_collateral_inputs)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->max_collateral_inputs != NULL)
  {
    _cardano_free(protocol_param_update->max_collateral_inputs);
  }

  if (max_collateral_inputs == NULL)
  {
    protocol_param_update->max_collateral_inputs = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->max_collateral_inputs = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->max_collateral_inputs == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->max_collateral_inputs = *max_collateral_inputs;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_pool_voting_thresholds(
  cardano_protocol_param_update_t*  protocol_param_update,
  cardano_pool_voting_thresholds_t* pool_voting_thresholds)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->pool_voting_thresholds != NULL)
  {
    cardano_pool_voting_thresholds_unref(&protocol_param_update->pool_voting_thresholds);
  }

  if (pool_voting_thresholds == NULL)
  {
    protocol_param_update->pool_voting_thresholds = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_pool_voting_thresholds_ref(pool_voting_thresholds);
  protocol_param_update->pool_voting_thresholds = (cardano_pool_voting_thresholds_t*)pool_voting_thresholds;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_drep_voting_thresholds(
  cardano_protocol_param_update_t*  protocol_param_update,
  cardano_drep_voting_thresholds_t* drep_voting_thresholds)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->drep_voting_thresholds != NULL)
  {
    cardano_drep_voting_thresholds_unref(&protocol_param_update->drep_voting_thresholds);
  }

  if (drep_voting_thresholds == NULL)
  {
    protocol_param_update->drep_voting_thresholds = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_drep_voting_thresholds_ref(drep_voting_thresholds);
  protocol_param_update->drep_voting_thresholds = (cardano_drep_voting_thresholds_t*)drep_voting_thresholds;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_min_committee_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_committee_size)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->min_committee_size != NULL)
  {
    _cardano_free(protocol_param_update->min_committee_size);
  }

  if (min_committee_size == NULL)
  {
    protocol_param_update->min_committee_size = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->min_committee_size = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->min_committee_size == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->min_committee_size = *min_committee_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_committee_term_limit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  committee_term_limit)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->committee_term_limit != NULL)
  {
    _cardano_free(protocol_param_update->committee_term_limit);
  }

  if (committee_term_limit == NULL)
  {
    protocol_param_update->committee_term_limit = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->committee_term_limit = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->committee_term_limit == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->committee_term_limit = *committee_term_limit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_governance_action_validity_period(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  governance_action_validity_period)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->governance_action_validity_period != NULL)
  {
    _cardano_free(protocol_param_update->governance_action_validity_period);
  }

  if (governance_action_validity_period == NULL)
  {
    protocol_param_update->governance_action_validity_period = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->governance_action_validity_period = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->governance_action_validity_period == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->governance_action_validity_period = *governance_action_validity_period;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_governance_action_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  governance_action_deposit)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->governance_action_deposit != NULL)
  {
    _cardano_free(protocol_param_update->governance_action_deposit);
  }

  if (governance_action_deposit == NULL)
  {
    protocol_param_update->governance_action_deposit = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->governance_action_deposit = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->governance_action_deposit == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->governance_action_deposit = *governance_action_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_drep_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  drep_deposit)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->drep_deposit != NULL)
  {
    _cardano_free(protocol_param_update->drep_deposit);
  }

  if (drep_deposit == NULL)
  {
    protocol_param_update->drep_deposit = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->drep_deposit = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->drep_deposit == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->drep_deposit = *drep_deposit;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_drep_inactivity_period(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  drep_inactivity_period)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->drep_inactivity_period != NULL)
  {
    _cardano_free(protocol_param_update->drep_inactivity_period);
  }

  if (drep_inactivity_period == NULL)
  {
    protocol_param_update->drep_inactivity_period = NULL;

    return CARDANO_SUCCESS;
  }

  protocol_param_update->drep_inactivity_period = _cardano_malloc(sizeof(uint64_t));

  if (protocol_param_update->drep_inactivity_period == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *protocol_param_update->drep_inactivity_period = *drep_inactivity_period;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_param_update_set_ref_script_cost_per_byte(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         ref_script_cost_per_byte)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update->ref_script_cost_per_byte != NULL)
  {
    cardano_unit_interval_unref(&protocol_param_update->ref_script_cost_per_byte);
  }

  if (ref_script_cost_per_byte == NULL)
  {
    protocol_param_update->ref_script_cost_per_byte = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_unit_interval_ref(ref_script_cost_per_byte);
  protocol_param_update->ref_script_cost_per_byte = (cardano_unit_interval_t*)ref_script_cost_per_byte;

  return CARDANO_SUCCESS;
}

void
cardano_protocol_param_update_unref(cardano_protocol_param_update_t** protocol_param_update)
{
  if ((protocol_param_update == NULL) || (*protocol_param_update == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*protocol_param_update)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *protocol_param_update = NULL;
    return;
  }
}

void
cardano_protocol_param_update_ref(cardano_protocol_param_update_t* protocol_param_update)
{
  if (protocol_param_update == NULL)
  {
    return;
  }

  cardano_object_ref(&protocol_param_update->base);
}

size_t
cardano_protocol_param_update_refcount(const cardano_protocol_param_update_t* protocol_param_update)
{
  if (protocol_param_update == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&protocol_param_update->base);
}

void
cardano_protocol_param_update_set_last_error(cardano_protocol_param_update_t* protocol_param_update, const char* message)
{
  cardano_object_set_last_error(&protocol_param_update->base, message);
}

const char*
cardano_protocol_param_update_get_last_error(const cardano_protocol_param_update_t* protocol_param_update)
{
  return cardano_object_get_last_error(&protocol_param_update->base);
}