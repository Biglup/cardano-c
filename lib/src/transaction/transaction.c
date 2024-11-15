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

#include <cardano/object.h>
#include <cardano/transaction/transaction.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

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

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    ALONZO_ERA_FRAME_SIZE);

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

  cardano_error_t write_bool_result = cardano_cbor_writer_write_bool(writer, transaction->is_valid);

  if (write_bool_result != CARDANO_SUCCESS)
  {
    return write_bool_result;
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