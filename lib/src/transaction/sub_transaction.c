/**
 * \file sub_transaction.c
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
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

#include <cardano/object.h>
#include <cardano/transaction/sub_transaction.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>

/* CONSTANTS *****************************************************************/

static const uint32_t SUB_TRANSACTION_FRAME_SIZE = 3U;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano sub transaction.
 */
typedef struct cardano_sub_transaction_t
{
    cardano_object_t                base;
    cardano_sub_transaction_body_t* body;
    cardano_witness_set_t*          witness_set;
    cardano_auxiliary_data_t*       auxiliary_data;
} cardano_sub_transaction_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a sub transaction object.
 *
 * This function is responsible for properly deallocating a sub transaction object (`cardano_sub_transaction_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the sub transaction object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_sub_transaction_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the sub transaction
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_sub_transaction_deallocate(void* object)
{
  assert(object != NULL);

  cardano_sub_transaction_t* sub_transaction = (cardano_sub_transaction_t*)object;

  cardano_sub_transaction_body_unref(&sub_transaction->body);
  cardano_witness_set_unref(&sub_transaction->witness_set);
  cardano_auxiliary_data_unref(&sub_transaction->auxiliary_data);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_sub_transaction_new(
  cardano_sub_transaction_body_t* body,
  cardano_witness_set_t*          witness_set,
  cardano_auxiliary_data_t*       auxiliary_data,
  cardano_sub_transaction_t**     sub_transaction)
{
  if (body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *sub_transaction = _cardano_malloc(sizeof(cardano_sub_transaction_t));

  if (*sub_transaction == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*sub_transaction)->base.deallocator   = cardano_sub_transaction_deallocate;
  (*sub_transaction)->base.ref_count     = 1;
  (*sub_transaction)->base.last_error[0] = '\0';
  (*sub_transaction)->body               = body;
  (*sub_transaction)->witness_set        = witness_set;
  (*sub_transaction)->auxiliary_data     = auxiliary_data;

  cardano_sub_transaction_body_ref(body);
  cardano_witness_set_ref(witness_set);
  cardano_auxiliary_data_ref(auxiliary_data);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_sub_transaction_from_cbor(cardano_cbor_reader_t* reader, cardano_sub_transaction_t** sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *sub_transaction = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "sub_transaction";

  cardano_error_t result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, SUB_TRANSACTION_FRAME_SIZE);

  if (result != CARDANO_SUCCESS)
  {
    *sub_transaction = NULL;
    return result;
  }

  cardano_sub_transaction_body_t* body           = NULL;
  cardano_witness_set_t*          witness_set    = NULL;
  cardano_auxiliary_data_t*       auxiliary_data = NULL;

  cardano_error_t body_result = cardano_sub_transaction_body_from_cbor(reader, &body);

  if (body_result != CARDANO_SUCCESS)
  {
    *sub_transaction = NULL;
    return body_result;
  }

  cardano_error_t witness_set_result = cardano_witness_set_from_cbor(reader, &witness_set);

  if (witness_set_result != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_body_unref(&body);
    *sub_transaction = NULL;

    return witness_set_result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t state_result = cardano_cbor_reader_peek_state(reader, &state);

  if (state_result != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_body_unref(&body);
    cardano_witness_set_unref(&witness_set);
    *sub_transaction = NULL;

    return state_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null_result = cardano_cbor_reader_read_null(reader);

    if (read_null_result != CARDANO_SUCCESS)
    {
      cardano_sub_transaction_body_unref(&body);
      cardano_witness_set_unref(&witness_set);
      *sub_transaction = NULL;

      return read_null_result;
    }
  }
  else
  {
    cardano_error_t auxiliary_data_result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

    if (auxiliary_data_result != CARDANO_SUCCESS)
    {
      cardano_sub_transaction_body_unref(&body);
      cardano_witness_set_unref(&witness_set);
      *sub_transaction = NULL;

      return auxiliary_data_result;
    }
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *sub_transaction = NULL;

    cardano_sub_transaction_body_unref(&body);
    cardano_witness_set_unref(&witness_set);
    cardano_auxiliary_data_unref(&auxiliary_data);

    return expect_end_array_result;
  }

  cardano_error_t create_instance_result = cardano_sub_transaction_new(body, witness_set, auxiliary_data, sub_transaction);

  cardano_sub_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);
  cardano_auxiliary_data_unref(&auxiliary_data);

  if (create_instance_result != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_unref(sub_transaction);
    return create_instance_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_sub_transaction_to_cbor(const cardano_sub_transaction_t* sub_transaction, cardano_cbor_writer_t* writer)
{
  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    (int64_t)SUB_TRANSACTION_FRAME_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t body_result = cardano_sub_transaction_body_to_cbor(sub_transaction->body, writer);

  if (body_result != CARDANO_SUCCESS)
  {
    return body_result;
  }

  cardano_error_t witness_set_result = cardano_witness_set_to_cbor(sub_transaction->witness_set, writer);

  if (witness_set_result != CARDANO_SUCCESS)
  {
    return witness_set_result;
  }

  if (sub_transaction->auxiliary_data == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t auxiliary_data_result = cardano_auxiliary_data_to_cbor(sub_transaction->auxiliary_data, writer);

    if (auxiliary_data_result != CARDANO_SUCCESS)
    {
      return auxiliary_data_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_sub_transaction_body_t*
cardano_sub_transaction_get_body(cardano_sub_transaction_t* sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return NULL;
  }

  cardano_sub_transaction_body_ref(sub_transaction->body);

  return sub_transaction->body;
}

cardano_error_t
cardano_sub_transaction_set_body(cardano_sub_transaction_t* sub_transaction, cardano_sub_transaction_body_t* body)
{
  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (body == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_sub_transaction_body_unref(&sub_transaction->body);

  sub_transaction->body = body;

  cardano_sub_transaction_body_ref(body);

  return CARDANO_SUCCESS;
}

cardano_witness_set_t*
cardano_sub_transaction_get_witness_set(cardano_sub_transaction_t* sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return NULL;
  }

  cardano_witness_set_ref(sub_transaction->witness_set);

  return sub_transaction->witness_set;
}

cardano_error_t
cardano_sub_transaction_set_witness_set(cardano_sub_transaction_t* sub_transaction, cardano_witness_set_t* witness_set)
{
  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_witness_set_unref(&sub_transaction->witness_set);

  sub_transaction->witness_set = witness_set;

  cardano_witness_set_ref(witness_set);

  return CARDANO_SUCCESS;
}

cardano_auxiliary_data_t*
cardano_sub_transaction_get_auxiliary_data(cardano_sub_transaction_t* sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return NULL;
  }

  cardano_auxiliary_data_ref(sub_transaction->auxiliary_data);

  return sub_transaction->auxiliary_data;
}

cardano_error_t
cardano_sub_transaction_set_auxiliary_data(cardano_sub_transaction_t* sub_transaction, cardano_auxiliary_data_t* auxiliary_data)
{
  if (sub_transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_auxiliary_data_unref(&sub_transaction->auxiliary_data);

  sub_transaction->auxiliary_data = auxiliary_data;

  cardano_auxiliary_data_ref(auxiliary_data);

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_sub_transaction_get_id(cardano_sub_transaction_t* sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return NULL;
  }

  return cardano_sub_transaction_body_get_hash(sub_transaction->body);
}

void
cardano_sub_transaction_clear_cbor_cache(cardano_sub_transaction_t* sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return;
  }

  cardano_sub_transaction_body_clear_cbor_cache(sub_transaction->body);
  cardano_witness_set_clear_cbor_cache(sub_transaction->witness_set);
  cardano_auxiliary_data_clear_cbor_cache(sub_transaction->auxiliary_data);
}

void
cardano_sub_transaction_unref(cardano_sub_transaction_t** sub_transaction)
{
  if ((sub_transaction == NULL) || (*sub_transaction == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*sub_transaction)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *sub_transaction = NULL;
    return;
  }
}

void
cardano_sub_transaction_ref(cardano_sub_transaction_t* sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return;
  }

  cardano_object_ref(&sub_transaction->base);
}

size_t
cardano_sub_transaction_refcount(const cardano_sub_transaction_t* sub_transaction)
{
  if (sub_transaction == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&sub_transaction->base);
}

void
cardano_sub_transaction_set_last_error(cardano_sub_transaction_t* sub_transaction, const char* message)
{
  cardano_object_set_last_error(&sub_transaction->base, message);
}

const char*
cardano_sub_transaction_get_last_error(const cardano_sub_transaction_t* sub_transaction)
{
  return cardano_object_get_last_error(&sub_transaction->base);
}
