/**
 * \file transaction.c
 *
 * \author angel.castillo
 * \date   Sep 23, 2024
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

#include <cardano/certs/certificate_set.h>
#include <cardano/object.h>
#include <cardano/transaction/transaction.h>
#include <cardano/voting_procedures/voter_list.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/redeemer.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../transaction_builder/balancing/internals/unique_signers.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t ALONZO_ERA_FRAME_SIZE = 4;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano protocol version.
 */
typedef struct cardano_transaction_t
{
    cardano_object_t            base;
    cardano_transaction_body_t* body;
    cardano_witness_set_t*      witness_set;
    cardano_auxiliary_data_t*   auxiliary_data;
    bool                        is_valid;
    int64_t                     frame_size;
} cardano_transaction_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a protocol version object.
 *
 * This function is responsible for properly deallocating a protocol version object (`cardano_transaction_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the transaction object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_transaction_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the transaction
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_transaction_deallocate(void* object)
{
  assert(object != NULL);

  cardano_transaction_t* transaction = (cardano_transaction_t*)object;

  cardano_transaction_body_unref(&transaction->body);
  cardano_witness_set_unref(&transaction->witness_set);
  cardano_auxiliary_data_unref(&transaction->auxiliary_data);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_transaction_new(
  cardano_transaction_body_t* body,
  cardano_witness_set_t*      witness_set,
  cardano_auxiliary_data_t*   auxiliary_data,
  cardano_transaction_t**     transaction)
{
  if (body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *transaction = _cardano_malloc(sizeof(cardano_transaction_t));

  if (*transaction == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*transaction)->base.deallocator   = cardano_transaction_deallocate;
  (*transaction)->base.ref_count     = 1;
  (*transaction)->base.last_error[0] = '\0';
  (*transaction)->body               = body;
  (*transaction)->witness_set        = witness_set;
  (*transaction)->auxiliary_data     = auxiliary_data;
  (*transaction)->is_valid           = true;
  (*transaction)->frame_size         = ALONZO_ERA_FRAME_SIZE;

  cardano_transaction_body_ref(body);
  cardano_witness_set_ref(witness_set);
  cardano_auxiliary_data_ref(auxiliary_data);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_t** transaction)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *transaction = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "transaction";

  int64_t array_size = 0U;

  cardano_error_t result = cardano_cbor_reader_read_start_array(reader, &array_size);

  if (result != CARDANO_SUCCESS)
  {
    *transaction = NULL;
    return result;
  }

  cardano_transaction_body_t* body           = NULL;
  cardano_witness_set_t*      witness_set    = NULL;
  cardano_auxiliary_data_t*   auxiliary_data = NULL;
  bool                        is_valid       = true;

  cardano_error_t body_result = cardano_transaction_body_from_cbor(reader, &body);

  if (body_result != CARDANO_SUCCESS)
  {
    *transaction = NULL;
    return body_result;
  }

  cardano_error_t witness_set_result = cardano_witness_set_from_cbor(reader, &witness_set);

  if (witness_set_result != CARDANO_SUCCESS)
  {
    cardano_transaction_body_unref(&body);
    *transaction = NULL;
    return witness_set_result;
  }

  if (array_size == ALONZO_ERA_FRAME_SIZE)
  {
    cardano_error_t read_bool_result = cardano_cbor_reader_read_bool(reader, &is_valid);

    if (read_bool_result != CARDANO_SUCCESS)
    {
      cardano_transaction_body_unref(&body);
      cardano_witness_set_unref(&witness_set);
      *transaction = NULL;

      return read_bool_result;
    }
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t state_result = cardano_cbor_reader_peek_state(reader, &state);

  if (state_result != CARDANO_SUCCESS)
  {
    cardano_transaction_body_unref(&body);
    cardano_witness_set_unref(&witness_set);
    *transaction = NULL;

    return state_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null_result = cardano_cbor_reader_read_null(reader);

    if (read_null_result != CARDANO_SUCCESS)
    {
      cardano_transaction_body_unref(&body);
      cardano_witness_set_unref(&witness_set);
      *transaction = NULL;

      return read_null_result;
    }
  }
  else
  {
    cardano_error_t auxiliary_data_result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

    if (auxiliary_data_result != CARDANO_SUCCESS)
    {
      cardano_transaction_body_unref(&body);
      cardano_witness_set_unref(&witness_set);
      *transaction = NULL;

      return auxiliary_data_result;
    }
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *transaction = NULL;

    cardano_transaction_body_unref(&body);
    cardano_witness_set_unref(&witness_set);
    cardano_auxiliary_data_unref(&auxiliary_data);

    return expect_end_array_result;
  }

  cardano_error_t create_instance_result = cardano_transaction_new(body, witness_set, auxiliary_data, transaction);

  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);
  cardano_auxiliary_data_unref(&auxiliary_data);

  if (create_instance_result != CARDANO_SUCCESS)
  {
    cardano_transaction_unref(transaction);
    return create_instance_result;
  }

  cardano_error_t set_is_valid_result = cardano_transaction_set_is_valid(*transaction, is_valid);

  if (set_is_valid_result != CARDANO_SUCCESS)
  {
    cardano_transaction_unref(transaction);
    return set_is_valid_result;
  }

  const int64_t DIJKSTRA_ERA_FRAME_SIZE = 3;

  if (array_size == DIJKSTRA_ERA_FRAME_SIZE)
  {
    (*transaction)->frame_size = DIJKSTRA_ERA_FRAME_SIZE;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_to_cbor(const cardano_transaction_t* transaction, cardano_cbor_writer_t* writer)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t frame_size = transaction->frame_size;

  if (!transaction->is_valid)
  {
    frame_size = ALONZO_ERA_FRAME_SIZE;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    frame_size);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t body_result = cardano_transaction_body_to_cbor(transaction->body, writer);

  if (body_result != CARDANO_SUCCESS)
  {
    return body_result;
  }

  cardano_error_t witness_set_result = cardano_witness_set_to_cbor(transaction->witness_set, writer);

  if (witness_set_result != CARDANO_SUCCESS)
  {
    return witness_set_result;
  }

  if (frame_size == ALONZO_ERA_FRAME_SIZE)
  {
    cardano_error_t write_bool_result = cardano_cbor_writer_write_bool(writer, transaction->is_valid);

    if (write_bool_result != CARDANO_SUCCESS)
    {
      return write_bool_result;
    }
  }

  if (transaction->auxiliary_data == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t auxiliary_data_result = cardano_auxiliary_data_to_cbor(transaction->auxiliary_data, writer);

    if (auxiliary_data_result != CARDANO_SUCCESS)
    {
      return auxiliary_data_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_to_cip116_json(
  const cardano_transaction_t* tx,
  cardano_json_writer_t*       writer)
{
  if ((tx == NULL) || (writer == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_writer_write_start_object(writer);
  cardano_error_t error = CARDANO_SUCCESS;

  cardano_json_writer_write_property_name(writer, "body", 4);
  error = cardano_transaction_body_to_cip116_json(tx->body, writer);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  cardano_json_writer_write_property_name(writer, "is_valid", 8);
  cardano_json_writer_write_bool(writer, tx->is_valid);

  cardano_json_writer_write_property_name(writer, "witness_set", 11);
  error = cardano_witness_set_to_cip116_json(tx->witness_set, writer);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  if (tx->auxiliary_data != NULL)
  {
    cardano_json_writer_write_property_name(writer, "auxiliary_data", 14);

    error = cardano_auxiliary_data_to_cip116_json(tx->auxiliary_data, writer);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }

  cardano_json_writer_write_end_object(writer);

  return error;
}

cardano_transaction_body_t*
cardano_transaction_get_body(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return NULL;
  }

  cardano_transaction_body_ref(transaction->body);

  return transaction->body;
}

cardano_error_t
cardano_transaction_set_body(cardano_transaction_t* transaction, cardano_transaction_body_t* body)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_unref(&transaction->body);

  transaction->body = body;

  cardano_transaction_body_ref(body);

  return CARDANO_SUCCESS;
}

cardano_witness_set_t*
cardano_transaction_get_witness_set(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return NULL;
  }

  cardano_witness_set_ref(transaction->witness_set);

  return transaction->witness_set;
}

cardano_error_t
cardano_transaction_set_witness_set(cardano_transaction_t* transaction, cardano_witness_set_t* witness_set)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_witness_set_unref(&transaction->witness_set);

  transaction->witness_set = witness_set;

  cardano_witness_set_ref(witness_set);

  return CARDANO_SUCCESS;
}

cardano_auxiliary_data_t*
cardano_transaction_get_auxiliary_data(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return NULL;
  }

  cardano_auxiliary_data_ref(transaction->auxiliary_data);

  return transaction->auxiliary_data;
}

cardano_error_t
cardano_transaction_set_auxiliary_data(cardano_transaction_t* transaction, cardano_auxiliary_data_t* auxiliary_data)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_auxiliary_data_unref(&transaction->auxiliary_data);

  transaction->auxiliary_data = auxiliary_data;

  cardano_auxiliary_data_ref(auxiliary_data);

  return CARDANO_SUCCESS;
}

bool
cardano_transaction_get_is_valid(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return false;
  }

  return transaction->is_valid;
}

cardano_error_t
cardano_transaction_set_is_valid(cardano_transaction_t* transaction, const bool is_valid)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  transaction->is_valid = is_valid;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_transaction_get_id(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return NULL;
  }

  return cardano_transaction_body_get_hash(transaction->body);
}

void
cardano_transaction_clear_cbor_cache(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return;
  }

  cardano_transaction_body_clear_cbor_cache(transaction->body);
  cardano_witness_set_clear_cbor_cache(transaction->witness_set);
  cardano_auxiliary_data_clear_cbor_cache(transaction->auxiliary_data);
}

cardano_error_t
cardano_transaction_apply_vkey_witnesses(
  cardano_transaction_t*      transaction,
  cardano_vkey_witness_set_t* new_vkeys)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (new_vkeys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(transaction);

  cardano_witness_set_unref(&witness_set);

  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t             result;
  cardano_vkey_witness_set_t* inner_vkeys = cardano_witness_set_get_vkeys(witness_set);

  if (inner_vkeys == NULL)
  {
    result = cardano_vkey_witness_set_new(&inner_vkeys);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_witness_set_set_vkeys(witness_set, inner_vkeys);
    cardano_vkey_witness_set_unref(&inner_vkeys);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }
  else
  {
    cardano_vkey_witness_set_unref(&inner_vkeys);
  }

  return cardano_vkey_witness_set_apply(inner_vkeys, new_vkeys);
}

bool
cardano_transaction_has_script_data(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return false;
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(transaction);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witness_set);
  cardano_plutus_data_set_unref(&datums);

  const size_t redeemers_size = cardano_redeemer_list_get_length(redeemers);
  const size_t datums_size    = cardano_plutus_data_set_get_length(datums);

  return (redeemers_size > 0U) || (datums_size > 0U);
}

void
cardano_transaction_unref(cardano_transaction_t** transaction)
{
  if ((transaction == NULL) || (*transaction == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*transaction)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *transaction = NULL;
    return;
  }
}

cardano_error_t
cardano_transaction_get_unique_signers(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  cardano_blake2b_hash_set_t** unique_signers)
{
  return _cardano_get_unique_signers(tx, resolved_inputs, unique_signers);
}

void
cardano_transaction_ref(cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return;
  }

  cardano_object_ref(&transaction->base);
}

size_t
cardano_transaction_refcount(const cardano_transaction_t* transaction)
{
  if (transaction == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&transaction->base);
}

void
cardano_transaction_set_last_error(cardano_transaction_t* transaction, const char* message)
{
  cardano_object_set_last_error(&transaction->base, message);
}

const char*
cardano_transaction_get_last_error(const cardano_transaction_t* transaction)
{
  return cardano_object_get_last_error(&transaction->base);
}

/**
 * \brief Finds the position of an input matching the given id and index within an input set.
 *
 * \param[in]  inputs     The input set to search.
 * \param[in]  id         The transaction id to match.
 * \param[in]  utxo_index The output index to match.
 * \param[out] index      The position of the matching input.
 *
 * \return \ref CARDANO_SUCCESS if found, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 */
static cardano_error_t
find_in_input_set(
  cardano_transaction_input_set_t* inputs,
  cardano_blake2b_hash_t*          id,
  const uint64_t                   utxo_index,
  uint64_t*                        index)
{
  const size_t length = cardano_transaction_input_set_get_length(inputs);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_transaction_input_t* input = NULL;

    cardano_error_t result = cardano_transaction_input_set_get(inputs, i, &input);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_blake2b_hash_t* input_id = cardano_transaction_input_get_id(input);

    const bool matches = cardano_blake2b_hash_equals(input_id, id) && (cardano_transaction_input_get_index(input) == utxo_index);

    cardano_blake2b_hash_unref(&input_id);
    cardano_transaction_input_unref(&input);

    if (matches)
    {
      *index = i;

      return CARDANO_SUCCESS;
    }
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_transaction_find_input_index(
  cardano_transaction_t*  transaction,
  cardano_blake2b_hash_t* id,
  const uint64_t          utxo_index,
  uint64_t*               index)
{
  if ((transaction == NULL) || (id == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  if (inputs == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  return find_in_input_set(inputs, id, utxo_index, index);
}

cardano_error_t
cardano_transaction_find_reference_input_index(
  cardano_transaction_t*  transaction,
  cardano_blake2b_hash_t* id,
  const uint64_t          utxo_index,
  uint64_t*               index)
{
  if ((transaction == NULL) || (id == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  if (inputs == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  return find_in_input_set(inputs, id, utxo_index, index);
}

cardano_error_t
cardano_transaction_find_output_index(
  cardano_transaction_t* transaction,
  cardano_address_t*     address,
  const uint64_t         min_lovelace,
  uint64_t*              index)
{
  if ((transaction == NULL) || (address == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  if (outputs == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  const size_t length = cardano_transaction_output_list_get_length(outputs);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_transaction_output_t* output = NULL;

    cardano_error_t result = cardano_transaction_output_list_get(outputs, i, &output);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_address_t* output_address = cardano_transaction_output_get_address(output);
    cardano_value_t*   value          = cardano_transaction_output_get_value(output);

    const bool matches = cardano_address_equals(output_address, address) && (cardano_value_get_coin(value) >= (int64_t)min_lovelace);

    cardano_address_unref(&output_address);
    cardano_value_unref(&value);
    cardano_transaction_output_unref(&output);

    if (matches)
    {
      *index = i;

      return CARDANO_SUCCESS;
    }
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_transaction_find_mint_policy_index(
  cardano_transaction_t*  transaction,
  cardano_blake2b_hash_t* policy_id,
  uint64_t*               index)
{
  if ((transaction == NULL) || (policy_id == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  if (mint == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_policy_id_list_t* policies = NULL;

  cardano_error_t result = cardano_multi_asset_get_keys(mint, &policies);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  const size_t length = cardano_policy_id_list_get_length(policies);

  for (size_t i = 0U; (i < length) && (result == CARDANO_ERROR_ELEMENT_NOT_FOUND); ++i)
  {
    cardano_blake2b_hash_t* policy = NULL;

    const cardano_error_t get_result = cardano_policy_id_list_get(policies, i, &policy);

    if (get_result != CARDANO_SUCCESS)
    {
      result = get_result;
      break;
    }

    if (cardano_blake2b_hash_equals(policy, policy_id))
    {
      *index = i;
      result = CARDANO_SUCCESS;
    }

    cardano_blake2b_hash_unref(&policy);
  }

  cardano_policy_id_list_unref(&policies);

  return result;
}

cardano_error_t
cardano_transaction_find_withdrawal_index(
  cardano_transaction_t*    transaction,
  cardano_reward_address_t* reward_address,
  uint64_t*                 index)
{
  if ((transaction == NULL) || (reward_address == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
  cardano_withdrawal_map_unref(&withdrawals);

  if (withdrawals == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_address_t* target = cardano_reward_address_to_address(reward_address);

  if (target == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  const size_t length = cardano_withdrawal_map_get_length(withdrawals);

  for (size_t i = 0U; (i < length) && (result == CARDANO_ERROR_ELEMENT_NOT_FOUND); ++i)
  {
    cardano_reward_address_t* key = NULL;

    const cardano_error_t get_result = cardano_withdrawal_map_get_key_at(withdrawals, i, &key);

    if (get_result != CARDANO_SUCCESS)
    {
      result = get_result;
      break;
    }

    cardano_address_t* key_address = cardano_reward_address_to_address(key);

    if (key_address == NULL)
    {
      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    else if (cardano_address_equals(key_address, target))
    {
      *index = i;
      result = CARDANO_SUCCESS;
    }
    else
    {
      /* Keep searching. */
    }

    cardano_address_unref(&key_address);
    cardano_reward_address_unref(&key);
  }

  cardano_address_unref(&target);

  return result;
}

/**
 * \brief Serializes a certificate into a freshly allocated buffer.
 *
 * \param[in]  certificate The certificate to serialize.
 * \param[out] buffer      The serialized bytes. The caller takes ownership.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
certificate_to_cbor_buffer(cardano_certificate_t* certificate, cardano_buffer_t** buffer)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_certificate_to_cbor(certificate, writer);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_cbor_writer_encode_in_buffer(writer, buffer);
  }

  cardano_cbor_writer_unref(&writer);

  return result;
}

cardano_error_t
cardano_transaction_find_certificate_index(
  cardano_transaction_t* transaction,
  cardano_certificate_t* certificate,
  uint64_t*              index)
{
  if ((transaction == NULL) || (certificate == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certificates);

  if (certificates == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_buffer_t* target_bytes = NULL;

  cardano_error_t result = certificate_to_cbor_buffer(certificate, &target_bytes);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  const size_t length = cardano_certificate_set_get_length(certificates);

  for (size_t i = 0U; (i < length) && (result == CARDANO_ERROR_ELEMENT_NOT_FOUND); ++i)
  {
    cardano_certificate_t* entry = NULL;

    const cardano_error_t get_result = cardano_certificate_set_get(certificates, i, &entry);

    if (get_result != CARDANO_SUCCESS)
    {
      result = get_result;
    }
    else
    {
      cardano_buffer_t* entry_bytes = NULL;

      const cardano_error_t serialize_result = certificate_to_cbor_buffer(entry, &entry_bytes);

      cardano_certificate_unref(&entry);

      if (serialize_result != CARDANO_SUCCESS)
      {
        result = serialize_result;
      }
      else
      {
        if (cardano_buffer_equals(entry_bytes, target_bytes))
        {
          *index = i;
          result = CARDANO_SUCCESS;
        }

        cardano_buffer_unref(&entry_bytes);
      }
    }
  }

  cardano_buffer_unref(&target_bytes);

  return result;
}

cardano_error_t
cardano_transaction_find_vote_index(
  cardano_transaction_t* transaction,
  cardano_voter_t*       voter,
  uint64_t*              index)
{
  if ((transaction == NULL) || (voter == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_voting_procedures_t* votes = cardano_transaction_body_get_voting_procedures(body);
  cardano_voting_procedures_unref(&votes);

  if (votes == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_voter_list_t* voters = NULL;

  cardano_error_t result = cardano_voting_procedures_get_voters(votes, &voters);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  const size_t length = cardano_voter_list_get_length(voters);

  for (size_t i = 0U; (i < length) && (result == CARDANO_ERROR_ELEMENT_NOT_FOUND); ++i)
  {
    cardano_voter_t* entry = NULL;

    const cardano_error_t get_result = cardano_voter_list_get(voters, i, &entry);

    if (get_result != CARDANO_SUCCESS)
    {
      result = get_result;
      break;
    }

    if (cardano_voter_equals(entry, voter))
    {
      *index = i;
      result = CARDANO_SUCCESS;
    }

    cardano_voter_unref(&entry);
  }

  cardano_voter_list_unref(&voters);

  return result;
}

cardano_error_t
cardano_transaction_find_redeemer_index(
  cardano_transaction_t*       transaction,
  const cardano_redeemer_tag_t tag,
  const uint64_t               purpose_index,
  uint64_t*                    index)
{
  if ((transaction == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(transaction);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  if (redeemers == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  const size_t length = cardano_redeemer_list_get_length(redeemers);

  uint64_t rank  = 0U;
  bool     found = false;

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    cardano_error_t result = cardano_redeemer_list_get(redeemers, i, &redeemer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    const cardano_redeemer_tag_t entry_tag   = cardano_redeemer_get_tag(redeemer);
    const uint64_t               entry_index = cardano_redeemer_get_index(redeemer);

    cardano_redeemer_unref(&redeemer);

    if ((entry_tag == tag) && (entry_index == purpose_index))
    {
      found = true;
    }
    else if ((entry_tag < tag) || ((entry_tag == tag) && (entry_index < purpose_index)))
    {
      ++rank;
    }
    else
    {
      /* Entry orders after the requested redeemer. */
    }
  }

  if (!found)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  *index = rank;

  return CARDANO_SUCCESS;
}
