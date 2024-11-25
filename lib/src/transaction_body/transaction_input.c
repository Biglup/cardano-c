/**
 * \file transaction_input.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
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
#include <cardano/transaction_body/transaction_input.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t TRANSACTION_INPUT_ARRAY_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a reference to an unspent transaction output (UTxO) from a previous
 * transaction, which the current transaction intends to spend.
 */
typedef struct cardano_transaction_input_t
{
    cardano_object_t        base;
    cardano_blake2b_hash_t* id;
    uint64_t                index;
} cardano_transaction_input_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a transaction_input object.
 *
 * This function is responsible for properly deallocating a transaction_input object (`cardano_transaction_input_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the transaction_input object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_transaction_input_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the transaction_input
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_transaction_input_deallocate(void* object)
{
  assert(object != NULL);

  cardano_transaction_input_t* transaction_input = (cardano_transaction_input_t*)object;
  cardano_blake2b_hash_unref(&transaction_input->id);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_transaction_input_new(
  cardano_blake2b_hash_t*       id,
  uint64_t                      index,
  cardano_transaction_input_t** transaction_input)
{
  if (transaction_input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cardano_blake2b_hash_get_bytes_size(id) != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_transaction_input_t* new_transaction_input = (cardano_transaction_input_t*)_cardano_malloc(sizeof(cardano_transaction_input_t));

  if (new_transaction_input == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  new_transaction_input->base.ref_count     = 1;
  new_transaction_input->base.last_error[0] = '\0';
  new_transaction_input->base.deallocator   = cardano_transaction_input_deallocate;

  cardano_blake2b_hash_ref(id);
  new_transaction_input->id    = id;
  new_transaction_input->index = index;

  *transaction_input = new_transaction_input;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_input_from_hex(
  const char*                   id_hex,
  size_t                        id_hex_size,
  uint64_t                      index,
  cardano_transaction_input_t** transaction_input)
{
  if (transaction_input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (id_hex == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((id_hex_size / 2U) != (size_t)CARDANO_BLAKE2B_HASH_SIZE_256)
  {
    return CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE;
  }

  cardano_blake2b_hash_t* id        = NULL;
  cardano_error_t         id_result = cardano_blake2b_hash_from_hex(id_hex, id_hex_size, &id);

  if (id_result != CARDANO_SUCCESS)
  {
    *transaction_input = NULL;

    return id_result;
  }

  const cardano_error_t new_transaction_input_result = cardano_transaction_input_new(id, index, transaction_input);

  cardano_blake2b_hash_unref(&id);

  if (new_transaction_input_result != CARDANO_SUCCESS)
  {
    *transaction_input = NULL;

    return new_transaction_input_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_input_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_input_t** transaction_input)
{
  if (transaction_input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *transaction_input = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "transaction_input";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)TRANSACTION_INPUT_ARRAY_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *transaction_input = NULL;
    return expect_array_result;
  }

  cardano_blake2b_hash_t* id = NULL;

  const cardano_error_t read_id_error = cardano_blake2b_hash_from_cbor(reader, &id);

  if (read_id_error != CARDANO_SUCCESS)
  {
    *transaction_input = NULL;

    return read_id_error;
  }

  uint64_t index = 0;

  const cardano_error_t read_index_result = cardano_cbor_reader_read_uint(reader, &index);

  if (read_index_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&id);
    *transaction_input = NULL;

    return read_index_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&id);
    *transaction_input = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t new_transaction_input_result = cardano_transaction_input_new(
    id,
    index,
    transaction_input);

  cardano_blake2b_hash_unref(&id);

  if (new_transaction_input_result != CARDANO_SUCCESS)
  {
    *transaction_input = NULL;
    return new_transaction_input_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_input_to_cbor(
  const cardano_transaction_input_t* transaction_input,
  cardano_cbor_writer_t*             writer)
{
  if (transaction_input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, TRANSACTION_INPUT_ARRAY_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_id_result = cardano_blake2b_hash_to_cbor(transaction_input->id, writer);

  if (write_id_result != CARDANO_SUCCESS)
  {
    return write_id_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, transaction_input->index);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_transaction_input_get_id(cardano_transaction_input_t* input)
{
  if (input == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(input->id);

  return input->id;
}

cardano_error_t
cardano_transaction_input_set_id(cardano_transaction_input_t* input, cardano_blake2b_hash_t* id)
{
  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_unref(&input->id);
  cardano_blake2b_hash_ref(id);

  input->id = id;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_transaction_input_get_index(const cardano_transaction_input_t* transaction_input)
{
  if (transaction_input == NULL)
  {
    return 0;
  }

  return transaction_input->index;
}

cardano_error_t
cardano_transaction_input_set_index(cardano_transaction_input_t* transaction_input, uint64_t index)
{
  if (transaction_input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  transaction_input->index = index;

  return CARDANO_SUCCESS;
}

int32_t
cardano_transaction_input_compare(
  const cardano_transaction_input_t* lhs,
  const cardano_transaction_input_t* rhs)
{
  if (lhs == rhs)
  {
    return 0;
  }

  if (lhs == NULL)
  {
    return -1;
  }

  if (rhs == NULL)
  {
    return 1;
  }

  const int32_t id_comparison = cardano_blake2b_hash_compare(lhs->id, rhs->id);

  if (id_comparison != 0)
  {
    return id_comparison;
  }

  if (lhs->index < rhs->index)
  {
    return -1;
  }

  if (lhs->index > rhs->index)
  {
    return 1;
  }

  return 0;
}

bool
cardano_transaction_input_equals(const cardano_transaction_input_t* lhs, const cardano_transaction_input_t* rhs)
{
  return cardano_transaction_input_compare(lhs, rhs) == 0;
}

void
cardano_transaction_input_unref(cardano_transaction_input_t** transaction_input)
{
  if ((transaction_input == NULL) || (*transaction_input == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*transaction_input)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *transaction_input = NULL;
    return;
  }
}

void
cardano_transaction_input_ref(cardano_transaction_input_t* transaction_input)
{
  if (transaction_input == NULL)
  {
    return;
  }

  cardano_object_ref(&transaction_input->base);
}

size_t
cardano_transaction_input_refcount(const cardano_transaction_input_t* transaction_input)
{
  if (transaction_input == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&transaction_input->base);
}

void
cardano_transaction_input_set_last_error(cardano_transaction_input_t* transaction_input, const char* message)
{
  cardano_object_set_last_error(&transaction_input->base, message);
}

const char*
cardano_transaction_input_get_last_error(const cardano_transaction_input_t* transaction_input)
{
  return cardano_object_get_last_error(&transaction_input->base);
}