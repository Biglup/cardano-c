/**
 * \file utxo.c
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
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

#include <cardano/common/utxo.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano UTxO.
 */
typedef struct cardano_utxo_t
{
    cardano_object_t              base;
    cardano_transaction_input_t*  input;
    cardano_transaction_output_t* output;
} cardano_utxo_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a utxo object.
 *
 * This function is responsible for properly deallocating a utxo object (`cardano_utxo_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the utxo object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_utxo_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the utxo
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_utxo_deallocate(void* object)
{
  assert(object != NULL);

  cardano_utxo_t* utxo = (cardano_utxo_t*)object;

  cardano_transaction_input_unref(&utxo->input);
  cardano_transaction_output_unref(&utxo->output);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_utxo_new(
  cardano_transaction_input_t*  input,
  cardano_transaction_output_t* output,
  cardano_utxo_t**              utxo)
{
  if (utxo == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (input == NULL)
  {
    *utxo = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (output == NULL)
  {
    *utxo = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *utxo = _cardano_malloc(sizeof(cardano_utxo_t));

  if (*utxo == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*utxo)->base.deallocator   = cardano_utxo_deallocate;
  (*utxo)->base.ref_count     = 1;
  (*utxo)->base.last_error[0] = '\0';

  cardano_transaction_input_ref(input);
  (*utxo)->input = input;

  cardano_transaction_output_ref(output);
  (*utxo)->output = output;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_utxo_from_cbor(cardano_cbor_reader_t* reader, cardano_utxo_t** utxo)
{
  if (utxo == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *utxo = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "utxo";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *utxo = NULL;
    return expect_array_result;
  }

  cardano_transaction_input_t*  input  = NULL;
  cardano_transaction_output_t* output = NULL;

  cardano_error_t input_from_cbor_result = cardano_transaction_input_from_cbor(reader, &input);

  if (input_from_cbor_result != CARDANO_SUCCESS)
  {
    *utxo = NULL;
    return input_from_cbor_result;
  }

  cardano_error_t output_from_cbor_result = cardano_transaction_output_from_cbor(reader, &output);

  if (output_from_cbor_result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_unref(&input);
    *utxo = NULL;
    return output_from_cbor_result;
  }

  cardano_error_t utxo_new_result = cardano_utxo_new(input, output, utxo);

  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);

  if (utxo_new_result != CARDANO_SUCCESS)
  {
    *utxo = NULL;
    return utxo_new_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_utxo_unref(utxo);

    return expect_end_array_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_utxo_to_cbor(const cardano_utxo_t* utxo, cardano_cbor_writer_t* writer)
{
  if (utxo == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t input_to_cbor_result = cardano_transaction_input_to_cbor(utxo->input, writer);

  if (input_to_cbor_result != CARDANO_SUCCESS)
  {
    return input_to_cbor_result;
  }

  return cardano_transaction_output_to_cbor(utxo->output, writer);
}

cardano_transaction_input_t*
cardano_utxo_get_input(cardano_utxo_t* utxo)
{
  if (utxo == NULL)
  {
    return NULL;
  }

  cardano_transaction_input_ref(utxo->input);

  return utxo->input;
}

cardano_error_t
cardano_utxo_set_input(cardano_utxo_t* utxo, cardano_transaction_input_t* input)
{
  if (utxo == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_input_ref(input);
  cardano_transaction_input_unref(&utxo->input);
  utxo->input = input;

  return CARDANO_SUCCESS;
}

cardano_transaction_output_t*
cardano_utxo_get_output(cardano_utxo_t* utxo)
{
  if (utxo == NULL)
  {
    return NULL;
  }

  cardano_transaction_output_ref(utxo->output);

  return utxo->output;
}

cardano_error_t
cardano_utxo_set_output(cardano_utxo_t* utxo, cardano_transaction_output_t* output)
{
  if (utxo == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_ref(output);
  cardano_transaction_output_unref(&utxo->output);
  utxo->output = output;

  return CARDANO_SUCCESS;
}

bool
cardano_utxo_equals(const cardano_utxo_t* lhs, const cardano_utxo_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  return cardano_transaction_input_equals(lhs->input, rhs->input) &&
    cardano_transaction_output_equals(lhs->output, rhs->output);
}

void
cardano_utxo_unref(cardano_utxo_t** utxo)
{
  if ((utxo == NULL) || (*utxo == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*utxo)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *utxo = NULL;
    return;
  }
}

void
cardano_utxo_ref(cardano_utxo_t* utxo)
{
  if (utxo == NULL)
  {
    return;
  }

  cardano_object_ref(&utxo->base);
}

size_t
cardano_utxo_refcount(const cardano_utxo_t* utxo)
{
  if (utxo == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&utxo->base);
}

void
cardano_utxo_set_last_error(cardano_utxo_t* utxo, const char* message)
{
  cardano_object_set_last_error(&utxo->base, message);
}

const char*
cardano_utxo_get_last_error(const cardano_utxo_t* utxo)
{
  return cardano_object_get_last_error(&utxo->base);
}