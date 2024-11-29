/**
 * \file transaction_body.c
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input_set.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief The transaction body encapsulates the core details of a transaction.
 */
typedef struct cardano_transaction_body_t
{
    cardano_object_t                   base;
    cardano_transaction_input_set_t*   inputs;
    cardano_transaction_output_list_t* outputs;
    uint64_t*                          fee;
    uint64_t*                          invalid_after;
    cardano_certificate_set_t*         certificates;
    cardano_withdrawal_map_t*          withdrawals;
    cardano_update_t*                  update;
    cardano_blake2b_hash_t*            aux_data_hash;
    uint64_t*                          invalid_before;
    cardano_multi_asset_t*             mint;
    cardano_blake2b_hash_t*            script_data_hash;
    cardano_transaction_input_set_t*   collateral;
    cardano_blake2b_hash_set_t*        required_signers;
    cardano_network_id_t*              network_id;
    cardano_transaction_output_t*      collateral_return;
    uint64_t*                          total_collateral;
    cardano_transaction_input_set_t*   reference_inputs;
    cardano_voting_procedures_t*       voting_procedures;
    cardano_proposal_procedure_set_t*  proposal_procedures;
    uint64_t*                          treasury_value;
    uint64_t*                          donation;
    cardano_buffer_t*                  cbor_cache;
} cardano_transaction_body_t;

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Type definition for a parameter handler function.
 *
 * This type defines a function pointer used for handling transaction body fields
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
 * \brief Deallocates a transaction body object.
 *
 * This function is responsible for properly deallocating a transaction body object (`cardano_transaction_body_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the transaction_body object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_transaction_body_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the transaction_body
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_transaction_body_deallocate(void* object)
{
  assert(object != NULL);

  cardano_transaction_body_t* data = (cardano_transaction_body_t*)object;

  cardano_transaction_input_set_unref(&data->inputs);
  cardano_transaction_output_list_unref(&data->outputs);
  _cardano_free(data->fee);
  _cardano_free(data->invalid_after);
  cardano_certificate_set_unref(&data->certificates);
  cardano_withdrawal_map_unref(&data->withdrawals);
  cardano_update_unref(&data->update);
  cardano_blake2b_hash_unref(&data->aux_data_hash);
  _cardano_free(data->invalid_before);
  cardano_multi_asset_unref(&data->mint);
  cardano_blake2b_hash_unref(&data->script_data_hash);
  cardano_transaction_input_set_unref(&data->collateral);
  cardano_blake2b_hash_set_unref(&data->required_signers);
  _cardano_free(data->network_id);
  cardano_transaction_output_unref(&data->collateral_return);
  _cardano_free(data->total_collateral);
  cardano_transaction_input_set_unref(&data->reference_inputs);
  cardano_voting_procedures_unref(&data->voting_procedures);
  cardano_proposal_procedure_set_unref(&data->proposal_procedures);
  _cardano_free(data->treasury_value);
  _cardano_free(data->donation);
  cardano_buffer_unref(&data->cbor_cache);

  _cardano_free(object);
}

/**
 * \brief Computes the map size of this transaction body object.
 *
 * \param body The transaction body object to compute the map size for.
 *
 * \return The map size of the transaction body object.
 */
static size_t
get_map_size(const cardano_transaction_body_t* body)
{
  assert(body != NULL);

  size_t map_size = 0U;

  if (body->inputs != NULL)
  {
    map_size += 1U;
  }

  if (body->outputs != NULL)
  {
    map_size += 1U;
  }

  if (body->fee != NULL)
  {
    map_size += 1U;
  }

  if (body->invalid_after != NULL)
  {
    map_size += 1U;
  }

  if (body->certificates != NULL)
  {
    map_size += 1U;
  }

  if (body->withdrawals != NULL)
  {
    map_size += 1U;
  }

  if (body->update != NULL)
  {
    map_size += 1U;
  }

  if (body->aux_data_hash != NULL)
  {
    map_size += 1U;
  }

  if (body->invalid_before != NULL)
  {
    map_size += 1U;
  }

  if (body->mint != NULL)
  {
    map_size += 1U;
  }

  if (body->script_data_hash != NULL)
  {
    map_size += 1U;
  }

  if (body->collateral != NULL)
  {
    map_size += 1U;
  }

  if (body->required_signers != NULL)
  {
    map_size += 1U;
  }

  if (body->network_id != NULL)
  {
    map_size += 1U;
  }

  if (body->collateral_return != NULL)
  {
    map_size += 1U;
  }

  if (body->total_collateral != NULL)
  {
    map_size += 1U;
  }

  if (body->reference_inputs != NULL)
  {
    map_size += 1U;
  }

  if (body->voting_procedures != NULL)
  {
    map_size += 1U;
  }

  if (body->proposal_procedures != NULL)
  {
    map_size += 1U;
  }

  if (body->treasury_value != NULL)
  {
    map_size += 1U;
  }

  if (body->donation != NULL)
  {
    map_size += 1U;
  }

  return map_size;
}

/**
 * \brief Retrieves a pointer to the specified field in the transaction body body object.
 *
 * This function returns a pointer to the field specified by the key in the given protocol
 * parameter body object. The key corresponds to different fields within the body object.
 * If the key does not correspond to any field, NULL is returned.
 *
 * \param[in] body Pointer to the transaction body body object.
 *
 * \return void* A pointer to the specified field, or NULL if the key does not correspond to any field.
 *
 * \note The caller must ensure that the `cardano_transaction_body_t` pointer is not NULL before calling this function.
 */
static void*
get_field_ptr(cardano_transaction_body_t* body, size_t key)
{
  assert(body != NULL);

  switch (key)
  {
    case 0:
      return (void*)&body->inputs;
    case 1:
      return (void*)&body->outputs;
    case 2:
      return (void*)&body->fee;
    case 3:
      return (void*)&body->invalid_after;
    case 4:
      return (void*)&body->certificates;
    case 5:
      return (void*)&body->withdrawals;
    case 6:
      return (void*)&body->update;
    case 7:
      return (void*)&body->aux_data_hash;
    case 8:
      return (void*)&body->invalid_before;
    case 9:
      return (void*)&body->mint;
    case 11:
      return (void*)&body->script_data_hash;
    case 13:
      return (void*)&body->collateral;
    case 14:
      return (void*)&body->required_signers;
    case 15:
      return (void*)&body->network_id;
    case 16:
      return (void*)&body->collateral_return;
    case 17:
      return (void*)&body->total_collateral;
    case 18:
      return (void*)&body->reference_inputs;
    case 19:
      return (void*)&body->voting_procedures;
    case 20:
      return (void*)&body->proposal_procedures;
    case 21:
      return (void*)&body->treasury_value;
    case 22:
      return (void*)&body->donation;

    default:
      return NULL;
  }
}

/**
 * \brief Reads a uint64_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a uint64_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as uint64_t in the transaction body body.
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
 * \brief Reads a cardano_transaction_input_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_transaction_input_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_transaction_input_set_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_transaction_input_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_transaction_input_set_t value should be stored.
 *                       The field pointer should be of type cardano_transaction_input_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_transaction_input_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_transaction_input_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_transaction_input_set_t** field = (cardano_transaction_input_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_transaction_input_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_transaction_output_list_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_transaction_output_list_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_transaction_output_list_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_transaction_output_list_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_transaction_output_list_t value should be stored.
 *                       The field pointer should be of type cardano_transaction_output_list_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_transaction_output_list_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_transaction_output_list(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_transaction_output_list_t** field = (cardano_transaction_output_list_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_transaction_output_list_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_certificate_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_certificate_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_certificate_set_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_certificate_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_certificate_set_t value should be stored.
 *                       The field pointer should be of type cardano_certificate_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_certificate_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_certificate_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_certificate_set_t** field = (cardano_certificate_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_certificate_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_withdrawal_map_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_withdrawal_map_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_withdrawal_map_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_withdrawal_map_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_withdrawal_map_t value should be stored.
 *                       The field pointer should be of type cardano_withdrawal_map_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_withdrawal_map_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_withdrawal_map(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_withdrawal_map_t** field = (cardano_withdrawal_map_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_withdrawal_map_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_update_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_update_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_update_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_update_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_update_t value should be stored.
 *                       The field pointer should be of type cardano_update_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_update_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_update(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_update_t** field = (cardano_update_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_update_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_blake2b_hash_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_blake2b_hash_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_blake2b_hash_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_blake2b_hash_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_blake2b_hash_t value should be stored.
 *                       The field pointer should be of type cardano_blake2b_hash_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_blake2b_hash_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_blake2b_hash(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_blake2b_hash_t** field = (cardano_blake2b_hash_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_blake2b_hash_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_multi_asset_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_multi_asset_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_multi_asset_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_multi_asset_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_multi_asset_t value should be stored.
 *                       The field pointer should be of type cardano_multi_asset_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_multi_asset_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_multi_asset(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_multi_asset_t** field = (cardano_multi_asset_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_multi_asset_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_blake2b_hash_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_blake2b_hash_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_blake2b_hash_set_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_blake2b_hash_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_blake2b_hash_set_t value should be stored.
 *                       The field pointer should be of type cardano_blake2b_hash_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_blake2b_hash_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_blake2b_hash_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_blake2b_hash_set_t** field = (cardano_blake2b_hash_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_blake2b_hash_set_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_network_id_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_network_id_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_network_id_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_network_id_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_network_id_t value should be stored.
 *                       The field pointer should be of type cardano_network_id_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_network_id_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_network_id(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_network_id_t** field = (cardano_network_id_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  *field = _cardano_malloc(sizeof(uint64_t));

  if (*field == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return cardano_cbor_reader_read_uint(reader, (uint64_t*)((void*)*field));
}

/**
 * \brief Reads a cardano_transaction_output_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_transaction_output_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_transaction_output_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_transaction_output_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_transaction_output_t value should be stored.
 *                       The field pointer should be of type cardano_transaction_output_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_transaction_output_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_transaction_output(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_transaction_output_t** field = (cardano_transaction_output_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_transaction_output_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_voting_procedures_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_voting_procedures_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_voting_procedures_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_voting_procedures_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_voting_procedures_t value should be stored.
 *                       The field pointer should be of type cardano_voting_procedures_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_voting_procedures_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_voting_procedures(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_voting_procedures_t** field = (cardano_voting_procedures_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_voting_procedures_from_cbor(reader, field);
}

/**
 * \brief Reads a cardano_proposal_procedure_set_t value from the CBOR reader and stores it in the specified field.
 *
 * This function reads a cardano_proposal_procedure_set_t value from the provided CBOR reader and stores the result
 * in the specified field pointer. It is used as a handler function for parameters that are
 * represented as cardano_proposal_procedure_set_t in the transaction body body.
 *
 * \param[in] reader A pointer to the CBOR reader from which to read the cardano_proposal_procedure_set_t value.
 * \param[out] field_ptr A pointer to the field where the read cardano_proposal_procedure_set_t value should be stored.
 *                       The field pointer should be of type cardano_proposal_procedure_set_t*.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the cardano_proposal_procedure_set_t value was successfully read and stored.
 *         - An appropriate error code indicating the failure reason.
 */
static cardano_error_t
handle_proposal_procedure_set(cardano_cbor_reader_t* reader, void* field_ptr)
{
  assert(reader != NULL);
  assert(field_ptr != NULL);

  cardano_proposal_procedure_set_t** field = (cardano_proposal_procedure_set_t**)field_ptr;

  if (*field != NULL)
  {
    return CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY;
  }

  return cardano_proposal_procedure_set_from_cbor(reader, field);
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
 * This function writes a key and its associated input set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the input set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_transaction_input_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_transaction_input_set_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_input_set_to_cbor(value, writer);

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
 * This function writes a key and its associated output list value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the output list value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_transaction_output_list_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_transaction_output_list_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_output_list_to_cbor(value, writer);

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
 * This function writes a key and its associated certificate set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the certificate set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_certificate_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_certificate_set_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_certificate_set_to_cbor(value, writer);

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
 * This function writes a key and its associated withdrawal map value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the withdrawal map value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_withdrawal_map_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_withdrawal_map_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_withdrawal_map_to_cbor(value, writer);

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
 * This function writes a key and its associated update value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the update value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_update_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_update_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_update_to_cbor(value, writer);

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
 * This function writes a key and its associated blake2b hash value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the blake2b hash value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_blake2b_hash_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_blake2b_hash_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_blake2b_hash_to_cbor(value, writer);

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
 * This function writes a key and its associated multi asset value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the multi asset value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_multi_asset_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_multi_asset_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_multi_asset_to_cbor(value, writer);

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
 * This function writes a key and its associated blake2b hash set value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the blake2b hash set value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_blake2b_hash_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_blake2b_hash_set_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_blake2b_hash_set_to_cbor(value, writer);

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
 * This function writes a key and its associated network id value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the network id value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_network_id_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_network_id_t* value)
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
 * This function writes a key and its associated output value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the output value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_transaction_output_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_transaction_output_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_output_to_cbor(value, writer);

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
 * This function writes a key and its associated voting procedures value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the voting procedures value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_voting_procedures_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_voting_procedures_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_voting_procedures_to_cbor(value, writer);

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
 * This function writes a key and its associated proposal procedures value to the CBOR writer
 * only if the value is not NULL. If the value is NULL, the function does nothing and
 * returns \ref CARDANO_SUCCESS.
 *
 * \param[in] writer A pointer to the CBOR writer. Must not be NULL.
 * \param[in] key The key to be written to the CBOR map.
 * \param[in] value A pointer to the proposal procedures value to be written. If NULL, nothing is written.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         - \ref CARDANO_SUCCESS if the key-value pair was successfully written or if the value is NULL.
 *         - An appropriate error code indicating the failure reason if writing to the CBOR writer fails.
 *
 * \pre \p writer must not be NULL.
 */
static cardano_error_t
write_proposal_procedure_set_if_present(cardano_cbor_writer_t* writer, const uint64_t key, const cardano_proposal_procedure_set_t* value)
{
  assert(writer != NULL);

  if (value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, key);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_proposal_procedure_set_to_cbor(value, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Allocates and initializes a new cardano_transaction_body_t structure.
 *
 * \param transaction_body A pointer to the location where the new cardano_transaction_body_t structure should be stored.
 * \return The pointer to the newly allocated cardano_transaction_body_t structure.
 */
static cardano_transaction_body_t*
create_transaction_body_new(void)
{
  cardano_transaction_body_t* transaction_body = (cardano_transaction_body_t*)_cardano_malloc(sizeof(cardano_transaction_body_t));

  if (transaction_body == NULL)
  {
    return NULL;
  }

  transaction_body->base.deallocator    = cardano_transaction_body_deallocate;
  transaction_body->base.ref_count      = 1;
  transaction_body->base.last_error[0]  = '\0';
  transaction_body->inputs              = NULL;
  transaction_body->outputs             = NULL;
  transaction_body->fee                 = NULL;
  transaction_body->invalid_after       = NULL;
  transaction_body->certificates        = NULL;
  transaction_body->withdrawals         = NULL;
  transaction_body->update              = NULL;
  transaction_body->aux_data_hash       = NULL;
  transaction_body->invalid_before      = NULL;
  transaction_body->mint                = NULL;
  transaction_body->script_data_hash    = NULL;
  transaction_body->collateral          = NULL;
  transaction_body->required_signers    = NULL;
  transaction_body->network_id          = NULL;
  transaction_body->collateral_return   = NULL;
  transaction_body->total_collateral    = NULL;
  transaction_body->reference_inputs    = NULL;
  transaction_body->voting_procedures   = NULL;
  transaction_body->proposal_procedures = NULL;
  transaction_body->treasury_value      = NULL;
  transaction_body->donation            = NULL;
  transaction_body->cbor_cache          = NULL;

  return transaction_body;
}

/* STATIC CONSTANTS ***********************************************************/

// cppcheck-suppress misra-c2012-8.9; Reason: Is more readable to define the map here
static const param_handler_t param_handlers[] = {
  handle_transaction_input_set,
  handle_transaction_output_list,
  handle_uint64,
  handle_uint64,
  handle_certificate_set,
  handle_withdrawal_map,
  handle_update,
  handle_blake2b_hash,
  handle_uint64,
  handle_multi_asset,
  handle_invalid_key, // unused key
  handle_blake2b_hash,
  handle_invalid_key, // unused key
  handle_transaction_input_set,
  handle_blake2b_hash_set,
  handle_network_id,
  handle_transaction_output,
  handle_uint64,
  handle_transaction_input_set,
  handle_voting_procedures,
  handle_proposal_procedure_set,
  handle_uint64,
  handle_uint64
};

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_transaction_body_new(
  cardano_transaction_input_set_t*   inputs,
  cardano_transaction_output_list_t* outputs,
  const uint64_t                     fee,
  const uint64_t*                    ttl,
  cardano_transaction_body_t**       transaction_body)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (outputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *transaction_body = create_transaction_body_new();

  if (*transaction_body == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*transaction_body)->fee = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

  if ((*transaction_body)->fee == NULL)
  {
    cardano_transaction_body_deallocate(*transaction_body);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_transaction_input_set_ref(inputs);
  cardano_transaction_output_list_ref(outputs);

  (*transaction_body)->inputs  = inputs;
  (*transaction_body)->outputs = outputs;
  *(*transaction_body)->fee    = fee;

  if (ttl != NULL)
  {
    (*transaction_body)->invalid_after = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

    if ((*transaction_body)->invalid_after == NULL)
    {
      cardano_transaction_body_deallocate(*transaction_body);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    *(*transaction_body)->invalid_after = *ttl;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_body_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_body_t** transaction_body)
{
  if (!reader || !transaction_body)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *transaction_body                = NULL;
  cardano_transaction_body_t* body = create_transaction_body_new();

  if (body == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_cbor_reader_t* reader_copy = NULL;
  cardano_error_t        copy_result = cardano_cbor_reader_clone(reader, &reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    cardano_transaction_body_unref(&body);
    return copy_result;
  }

  copy_result = cardano_cbor_reader_read_encoded_value(reader_copy, &body->cbor_cache);
  cardano_cbor_reader_unref(&reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    cardano_transaction_body_unref(&body);
    return copy_result;
  }

  int64_t         map_size = 0;
  cardano_error_t result   = cardano_cbor_reader_read_start_map(reader, &map_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_body_unref(&body);

    return result;
  }

  for (size_t i = 0U; i < (size_t)map_size; ++i)
  {
    uint64_t key = 0;
    result       = cardano_cbor_reader_read_uint(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_body_unref(&body);

      return result;
    }

    if (key >= (sizeof(param_handlers) / sizeof(param_handlers[0])))
    {
      cardano_transaction_body_unref(&body);

      return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
    }

    void* field_ptr = get_field_ptr(body, key);

    if (field_ptr == NULL)
    {
      cardano_transaction_body_unref(&body);

      return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
    }

    result = param_handlers[key](reader, field_ptr);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_body_unref(&body);

      return result;
    }
  }

  *transaction_body = body;

  cardano_error_t validation_result = cardano_cbor_validate_end_map("transaction_body", reader);

  if (validation_result != CARDANO_SUCCESS)
  {
    cardano_transaction_body_unref(transaction_body);
    return validation_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_body_to_cbor(const cardano_transaction_body_t* transaction_body, cardano_cbor_writer_t* writer)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (transaction_body->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(transaction_body->cbor_cache), cardano_buffer_get_size(transaction_body->cbor_cache));
  }

  const size_t map_size = get_map_size(transaction_body);

  cardano_error_t result = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_transaction_input_set_if_present(writer, 0U, transaction_body->inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_transaction_output_list_if_present(writer, 1U, transaction_body->outputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 2U, transaction_body->fee);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 3U, transaction_body->invalid_after);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_certificate_set_if_present(writer, 4U, transaction_body->certificates);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_withdrawal_map_if_present(writer, 5U, transaction_body->withdrawals);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_update_if_present(writer, 6U, transaction_body->update);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_blake2b_hash_if_present(writer, 7U, transaction_body->aux_data_hash);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 8U, transaction_body->invalid_before);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_multi_asset_if_present(writer, 9U, transaction_body->mint);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_blake2b_hash_if_present(writer, 11U, transaction_body->script_data_hash);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_transaction_input_set_if_present(writer, 13U, transaction_body->collateral);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_blake2b_hash_set_if_present(writer, 14U, transaction_body->required_signers);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_network_id_if_present(writer, 15U, transaction_body->network_id);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_transaction_output_if_present(writer, 16U, transaction_body->collateral_return);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 17U, transaction_body->total_collateral);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_transaction_input_set_if_present(writer, 18U, transaction_body->reference_inputs);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_voting_procedures_if_present(writer, 19U, transaction_body->voting_procedures);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_proposal_procedure_set_if_present(writer, 20U, transaction_body->proposal_procedures);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 21U, transaction_body->treasury_value);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = write_uint_if_present(writer, 22U, transaction_body->donation);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_transaction_input_set_t*
cardano_transaction_body_get_inputs(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_transaction_input_set_ref(transaction_body->inputs);

  return transaction_body->inputs;
}

cardano_error_t
cardano_transaction_body_set_inputs(cardano_transaction_body_t* transaction_body, cardano_transaction_input_set_t* inputs)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_input_set_ref(inputs);
  cardano_transaction_input_set_unref(&transaction_body->inputs);
  transaction_body->inputs = inputs;

  return CARDANO_SUCCESS;
}

cardano_transaction_output_list_t*
cardano_transaction_body_get_outputs(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_transaction_output_list_ref(transaction_body->outputs);

  return transaction_body->outputs;
}

cardano_error_t
cardano_transaction_body_set_outputs(cardano_transaction_body_t* transaction_body, cardano_transaction_output_list_t* outputs)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (outputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_list_ref(outputs);
  cardano_transaction_output_list_unref(&transaction_body->outputs);
  transaction_body->outputs = outputs;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_transaction_body_get_fee(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return 0U;
  }

  if (transaction_body->fee == NULL)
  {
    return 0U;
  }

  return *transaction_body->fee;
}

cardano_error_t
cardano_transaction_body_set_fee(cardano_transaction_body_t* transaction_body, const uint64_t fee)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (transaction_body->fee == NULL)
  {
    transaction_body->fee = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

    if (transaction_body->fee == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *transaction_body->fee = fee;

  return CARDANO_SUCCESS;
}

const uint64_t*
cardano_transaction_body_get_invalid_after(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  return transaction_body->invalid_after;
}

cardano_error_t
cardano_transaction_body_set_invalid_after(cardano_transaction_body_t* transaction_body, const uint64_t* epoch)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (epoch == NULL)
  {
    _cardano_free(transaction_body->invalid_after);
    transaction_body->invalid_after = NULL;

    return CARDANO_SUCCESS;
  }

  if (transaction_body->invalid_after == NULL)
  {
    transaction_body->invalid_after = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

    if (transaction_body->invalid_after == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *transaction_body->invalid_after = *epoch;

  return CARDANO_SUCCESS;
}

cardano_certificate_set_t*
cardano_transaction_body_get_certificates(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_certificate_set_ref(transaction_body->certificates);

  return transaction_body->certificates;
}

cardano_error_t
cardano_transaction_body_set_certificates(cardano_transaction_body_t* transaction_body, cardano_certificate_set_t* certificates)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificates == NULL)
  {
    cardano_certificate_set_unref(&transaction_body->certificates);
    transaction_body->certificates = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_certificate_set_ref(certificates);
  cardano_certificate_set_unref(&transaction_body->certificates);
  transaction_body->certificates = certificates;

  return CARDANO_SUCCESS;
}

cardano_withdrawal_map_t*
cardano_transaction_body_get_withdrawals(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_withdrawal_map_ref(transaction_body->withdrawals);

  return transaction_body->withdrawals;
}

cardano_error_t
cardano_transaction_body_set_withdrawals(cardano_transaction_body_t* transaction_body, cardano_withdrawal_map_t* withdrawals)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (withdrawals == NULL)
  {
    cardano_withdrawal_map_unref(&transaction_body->withdrawals);
    transaction_body->withdrawals = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_withdrawal_map_ref(withdrawals);
  cardano_withdrawal_map_unref(&transaction_body->withdrawals);
  transaction_body->withdrawals = withdrawals;

  return CARDANO_SUCCESS;
}

cardano_update_t*
cardano_transaction_body_get_update(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_update_ref(transaction_body->update);

  return transaction_body->update;
}

cardano_error_t
cardano_transaction_body_set_update(cardano_transaction_body_t* transaction_body, cardano_update_t* update)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update == NULL)
  {
    cardano_update_unref(&transaction_body->update);
    transaction_body->update = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_update_ref(update);
  cardano_update_unref(&transaction_body->update);
  transaction_body->update = update;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_transaction_body_get_aux_data_hash(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(transaction_body->aux_data_hash);

  return transaction_body->aux_data_hash;
}

cardano_error_t
cardano_transaction_body_set_aux_data_hash(cardano_transaction_body_t* transaction_body, cardano_blake2b_hash_t* aux_data_hash)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (aux_data_hash == NULL)
  {
    cardano_blake2b_hash_unref(&transaction_body->aux_data_hash);
    transaction_body->aux_data_hash = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_blake2b_hash_ref(aux_data_hash);
  cardano_blake2b_hash_unref(&transaction_body->aux_data_hash);
  transaction_body->aux_data_hash = aux_data_hash;

  return CARDANO_SUCCESS;
}

const uint64_t*
cardano_transaction_body_get_invalid_before(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  return transaction_body->invalid_before;
}

cardano_error_t
cardano_transaction_body_set_invalid_before(cardano_transaction_body_t* transaction_body, const uint64_t* epoch)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (epoch == NULL)
  {
    _cardano_free(transaction_body->invalid_before);
    transaction_body->invalid_before = NULL;

    return CARDANO_SUCCESS;
  }

  if (transaction_body->invalid_before == NULL)
  {
    transaction_body->invalid_before = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

    if (transaction_body->invalid_before == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *transaction_body->invalid_before = *epoch;

  return CARDANO_SUCCESS;
}

cardano_multi_asset_t*
cardano_transaction_body_get_mint(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_multi_asset_ref(transaction_body->mint);

  return transaction_body->mint;
}

cardano_error_t
cardano_transaction_body_set_mint(cardano_transaction_body_t* transaction_body, cardano_multi_asset_t* mint)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mint == NULL)
  {
    cardano_multi_asset_unref(&transaction_body->mint);
    transaction_body->mint = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_multi_asset_ref(mint);
  cardano_multi_asset_unref(&transaction_body->mint);
  transaction_body->mint = mint;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_transaction_body_get_script_data_hash(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(transaction_body->script_data_hash);

  return transaction_body->script_data_hash;
}

cardano_error_t
cardano_transaction_body_set_script_data_hash(cardano_transaction_body_t* transaction_body, cardano_blake2b_hash_t* script_data_hash)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_data_hash == NULL)
  {
    cardano_blake2b_hash_unref(&transaction_body->script_data_hash);
    transaction_body->script_data_hash = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_blake2b_hash_ref(script_data_hash);
  cardano_blake2b_hash_unref(&transaction_body->script_data_hash);
  transaction_body->script_data_hash = script_data_hash;

  return CARDANO_SUCCESS;
}

cardano_transaction_input_set_t*
cardano_transaction_body_get_collateral(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_transaction_input_set_ref(transaction_body->collateral);

  return transaction_body->collateral;
}

cardano_error_t
cardano_transaction_body_set_collateral(cardano_transaction_body_t* transaction_body, cardano_transaction_input_set_t* collateral)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (collateral == NULL)
  {
    cardano_transaction_input_set_unref(&transaction_body->collateral);
    transaction_body->collateral = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_transaction_input_set_ref(collateral);
  cardano_transaction_input_set_unref(&transaction_body->collateral);
  transaction_body->collateral = collateral;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_set_t*
cardano_transaction_body_get_required_signers(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_set_ref(transaction_body->required_signers);

  return transaction_body->required_signers;
}

cardano_error_t
cardano_transaction_body_set_required_signers(
  cardano_transaction_body_t* transaction_body,
  cardano_blake2b_hash_set_t* required_signers)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (required_signers == NULL)
  {
    cardano_blake2b_hash_set_unref(&transaction_body->required_signers);
    transaction_body->required_signers = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_blake2b_hash_set_ref(required_signers);
  cardano_blake2b_hash_set_unref(&transaction_body->required_signers);
  transaction_body->required_signers = required_signers;

  return CARDANO_SUCCESS;
}

const cardano_network_id_t*
cardano_transaction_body_get_network_id(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  return transaction_body->network_id;
}

cardano_error_t
cardano_transaction_body_set_network_id(cardano_transaction_body_t* transaction_body, const cardano_network_id_t* network_id)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (network_id == NULL)
  {
    _cardano_free(transaction_body->network_id);
    transaction_body->network_id = NULL;

    return CARDANO_SUCCESS;
  }

  if (transaction_body->network_id == NULL)
  {
    transaction_body->network_id = (cardano_network_id_t*)_cardano_malloc(sizeof(cardano_network_id_t));

    if (transaction_body->network_id == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *transaction_body->network_id = *network_id;

  return CARDANO_SUCCESS;
}

cardano_transaction_output_t*
cardano_transaction_body_get_collateral_return(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_transaction_output_ref(transaction_body->collateral_return);

  return transaction_body->collateral_return;
}

cardano_error_t
cardano_transaction_body_set_collateral_return(cardano_transaction_body_t* transaction_body, cardano_transaction_output_t* output)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (output == NULL)
  {
    cardano_transaction_output_unref(&transaction_body->collateral_return);
    transaction_body->collateral_return = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_transaction_output_ref(output);
  cardano_transaction_output_unref(&transaction_body->collateral_return);
  transaction_body->collateral_return = output;

  return CARDANO_SUCCESS;
}

const uint64_t*
cardano_transaction_body_get_total_collateral(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  return transaction_body->total_collateral;
}

cardano_error_t
cardano_transaction_body_set_total_collateral(cardano_transaction_body_t* transaction_body, const uint64_t* total_collateral)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (total_collateral == NULL)
  {
    _cardano_free(transaction_body->total_collateral);
    transaction_body->total_collateral = NULL;
    return CARDANO_SUCCESS;
  }

  if (transaction_body->total_collateral == NULL)
  {
    transaction_body->total_collateral = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

    if (transaction_body->total_collateral == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *transaction_body->total_collateral = *total_collateral;

  return CARDANO_SUCCESS;
}

cardano_transaction_input_set_t*
cardano_transaction_body_get_reference_inputs(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_transaction_input_set_ref(transaction_body->reference_inputs);

  return transaction_body->reference_inputs;
}

cardano_error_t
cardano_transaction_body_set_reference_inputs(
  cardano_transaction_body_t*      transaction_body,
  cardano_transaction_input_set_t* reference_inputs)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reference_inputs == NULL)
  {
    cardano_transaction_input_set_unref(&transaction_body->reference_inputs);
    transaction_body->reference_inputs = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_transaction_input_set_ref(reference_inputs);
  cardano_transaction_input_set_unref(&transaction_body->reference_inputs);
  transaction_body->reference_inputs = reference_inputs;

  return CARDANO_SUCCESS;
}

cardano_voting_procedures_t*
cardano_transaction_body_get_voting_procedures(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_voting_procedures_ref(transaction_body->voting_procedures);

  return transaction_body->voting_procedures;
}

cardano_error_t
cardano_transaction_body_set_voting_procedures(
  cardano_transaction_body_t*  transaction_body,
  cardano_voting_procedures_t* voting_procedures)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (voting_procedures == NULL)
  {
    cardano_voting_procedures_unref(&transaction_body->voting_procedures);
    transaction_body->voting_procedures = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_voting_procedures_ref(voting_procedures);
  cardano_voting_procedures_unref(&transaction_body->voting_procedures);
  transaction_body->voting_procedures = voting_procedures;

  return CARDANO_SUCCESS;
}

cardano_proposal_procedure_set_t*
cardano_transaction_body_get_proposal_procedures(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_proposal_procedure_set_ref(transaction_body->proposal_procedures);

  return transaction_body->proposal_procedures;
}

cardano_error_t
cardano_transaction_body_set_proposal_procedure(
  cardano_transaction_body_t*       transaction_body,
  cardano_proposal_procedure_set_t* proposal_procedures)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal_procedures == NULL)
  {
    cardano_proposal_procedure_set_unref(&transaction_body->proposal_procedures);
    transaction_body->proposal_procedures = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_proposal_procedure_set_ref(proposal_procedures);
  cardano_proposal_procedure_set_unref(&transaction_body->proposal_procedures);
  transaction_body->proposal_procedures = proposal_procedures;

  return CARDANO_SUCCESS;
}

const uint64_t*
cardano_transaction_body_get_treasury_value(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  return transaction_body->treasury_value;
}

cardano_error_t
cardano_transaction_body_set_treasury_value(cardano_transaction_body_t* transaction_body, const uint64_t* treasury_value)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_value == NULL)
  {
    _cardano_free(transaction_body->treasury_value);
    transaction_body->treasury_value = NULL;

    return CARDANO_SUCCESS;
  }

  if (transaction_body->treasury_value == NULL)
  {
    transaction_body->treasury_value = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

    if (transaction_body->treasury_value == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *transaction_body->treasury_value = *treasury_value;

  return CARDANO_SUCCESS;
}

const uint64_t*
cardano_transaction_body_get_donation(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  return transaction_body->donation;
}

cardano_error_t
cardano_transaction_body_set_donation(cardano_transaction_body_t* transaction_body, const uint64_t* donation)
{
  if (transaction_body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (donation == NULL)
  {
    _cardano_free(transaction_body->donation);
    transaction_body->donation = NULL;

    return CARDANO_SUCCESS;
  }

  if (transaction_body->donation == NULL)
  {
    transaction_body->donation = (uint64_t*)_cardano_malloc(sizeof(uint64_t));

    if (transaction_body->donation == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *transaction_body->donation = *donation;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_transaction_body_get_hash(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return NULL;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return NULL;
  }

  cardano_error_t result = cardano_transaction_body_to_cbor(transaction_body, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return NULL;
  }

  const size_t cbor_size = cardano_cbor_writer_get_encode_size(writer);
  byte_t*      cbor_data = (byte_t*)_cardano_malloc(cbor_size);

  if (cbor_data == NULL)
  {
    cardano_cbor_writer_unref(&writer);
    return NULL;
  }

  result = cardano_cbor_writer_encode(writer, cbor_data, cbor_size);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(cbor_data);
    cardano_cbor_writer_unref(&writer);

    return NULL;
  }

  cardano_cbor_writer_unref(&writer);

  cardano_blake2b_hash_t* hash = NULL;

  result = cardano_blake2b_compute_hash(cbor_data, cbor_size, CARDANO_BLAKE2B_HASH_SIZE_256, &hash);

  _cardano_free(cbor_data);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return hash;
}

bool
cardano_transaction_body_has_tagged_sets(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return false;
  }

  if (transaction_body->cbor_cache == NULL)
  {
    return true;
  }

  return cardano_transaction_input_set_is_tagged(transaction_body->inputs);
}

void
cardano_transaction_body_clear_cbor_cache(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return;
  }

  cardano_buffer_unref(&transaction_body->cbor_cache);
  transaction_body->cbor_cache = NULL;
}

void
cardano_transaction_body_unref(cardano_transaction_body_t** transaction_body)
{
  if ((transaction_body == NULL) || (*transaction_body == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*transaction_body)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *transaction_body = NULL;
    return;
  }
}

void
cardano_transaction_body_ref(cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return;
  }

  cardano_object_ref(&transaction_body->base);
}

size_t
cardano_transaction_body_refcount(const cardano_transaction_body_t* transaction_body)
{
  if (transaction_body == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&transaction_body->base);
}

void
cardano_transaction_body_set_last_error(cardano_transaction_body_t* transaction_body, const char* message)
{
  cardano_object_set_last_error(&transaction_body->base, message);
}

const char*
cardano_transaction_body_get_last_error(const cardano_transaction_body_t* transaction_body)
{
  return cardano_object_get_last_error(&transaction_body->base);
}