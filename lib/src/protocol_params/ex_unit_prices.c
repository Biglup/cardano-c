/**
 * \file ex_unit_prices.c
 *
 * \author angel.castillo
 * \date   May 06, 2024
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
#include <cardano/protocol_params/ex_unit_prices.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Specifies the cost (in Lovelace) of these ExUnits. In essence, they set the
 * "price" for the computational resources used by a smart contract.
 */
typedef struct cardano_ex_unit_prices_t
{
    cardano_object_t         base;
    cardano_unit_interval_t* mem_prices;
    cardano_unit_interval_t* cpu_prices;
} cardano_ex_unit_prices_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a execution unit prices object.
 *
 * This function is responsible for properly deallocating a execution unit prices object (`cardano_ex_unit_prices_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the ex_unit_prices object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_ex_unit_prices_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the ex_unit_prices
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_ex_unit_prices_deallocate(void* object)
{
  assert(object != NULL);

  cardano_ex_unit_prices_t* data = (cardano_ex_unit_prices_t*)object;

  cardano_unit_interval_unref(&data->mem_prices);
  cardano_unit_interval_unref(&data->cpu_prices);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_ex_unit_prices_new(
  cardano_unit_interval_t*   memory_prices,
  cardano_unit_interval_t*   steps_prices,
  cardano_ex_unit_prices_t** ex_unit_prices)
{
  if (ex_unit_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (memory_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (steps_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *ex_unit_prices = _cardano_malloc(sizeof(cardano_ex_unit_prices_t));

  if (*ex_unit_prices == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*ex_unit_prices)->base.deallocator   = cardano_ex_unit_prices_deallocate;
  (*ex_unit_prices)->base.ref_count     = 1;
  (*ex_unit_prices)->base.last_error[0] = '\0';

  cardano_unit_interval_ref(memory_prices);
  (*ex_unit_prices)->mem_prices = memory_prices;

  cardano_unit_interval_ref(steps_prices);
  (*ex_unit_prices)->cpu_prices = steps_prices;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ex_unit_prices_from_cbor(cardano_cbor_reader_t* reader, cardano_ex_unit_prices_t** ex_unit_prices)
{
  if (ex_unit_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *ex_unit_prices = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "ex_unit_prices";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *ex_unit_prices = NULL;
    return expect_array_result;
  }

  cardano_unit_interval_t* mem_prices = NULL;

  const cardano_error_t mem_prices_result = cardano_unit_interval_from_cbor(reader, &mem_prices);

  if (mem_prices_result != CARDANO_SUCCESS)
  {
    *ex_unit_prices = NULL;
    return mem_prices_result;
  }

  cardano_unit_interval_t* cpu_prices = NULL;

  const cardano_error_t cpu_prices_result = cardano_unit_interval_from_cbor(reader, &cpu_prices);

  if (cpu_prices_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&mem_prices);
    *ex_unit_prices = NULL;
    return cpu_prices_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *ex_unit_prices = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t result = cardano_ex_unit_prices_new(mem_prices, cpu_prices, ex_unit_prices);

  cardano_unit_interval_unref(&mem_prices);
  cardano_unit_interval_unref(&cpu_prices);

  return result;
}

cardano_error_t
cardano_ex_unit_prices_to_cbor(const cardano_ex_unit_prices_t* ex_unit_prices, cardano_cbor_writer_t* writer)
{
  if (ex_unit_prices == NULL)
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

  cardano_error_t write_mem_prices_result = cardano_unit_interval_to_cbor(ex_unit_prices->mem_prices, writer);

  if (write_mem_prices_result != CARDANO_SUCCESS)
  {
    return write_mem_prices_result;
  }

  return cardano_unit_interval_to_cbor(ex_unit_prices->cpu_prices, writer);
}

cardano_error_t
cardano_ex_unit_prices_get_memory_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t** memory_prices)
{
  if (ex_unit_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (memory_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(ex_unit_prices->mem_prices);
  *memory_prices = ex_unit_prices->mem_prices;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ex_unit_prices_get_steps_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t** steps_prices)
{
  if (ex_unit_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (steps_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(ex_unit_prices->cpu_prices);
  *steps_prices = ex_unit_prices->cpu_prices;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ex_unit_prices_set_memory_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t*  memory_prices)
{
  if (ex_unit_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (memory_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&ex_unit_prices->mem_prices);
  cardano_unit_interval_ref(memory_prices);
  ex_unit_prices->mem_prices = memory_prices;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ex_unit_prices_set_steps_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t*  steps_prices)
{
  if (ex_unit_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (steps_prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&ex_unit_prices->cpu_prices);
  cardano_unit_interval_ref(steps_prices);
  ex_unit_prices->cpu_prices = steps_prices;

  return CARDANO_SUCCESS;
}

void
cardano_ex_unit_prices_unref(cardano_ex_unit_prices_t** ex_unit_prices)
{
  if ((ex_unit_prices == NULL) || (*ex_unit_prices == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*ex_unit_prices)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *ex_unit_prices = NULL;
    return;
  }
}

void
cardano_ex_unit_prices_ref(cardano_ex_unit_prices_t* ex_unit_prices)
{
  if (ex_unit_prices == NULL)
  {
    return;
  }

  cardano_object_ref(&ex_unit_prices->base);
}

size_t
cardano_ex_unit_prices_refcount(const cardano_ex_unit_prices_t* ex_unit_prices)
{
  if (ex_unit_prices == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&ex_unit_prices->base);
}

void
cardano_ex_unit_prices_set_last_error(cardano_ex_unit_prices_t* ex_unit_prices, const char* message)
{
  cardano_object_set_last_error(&ex_unit_prices->base, message);
}

const char*
cardano_ex_unit_prices_get_last_error(const cardano_ex_unit_prices_t* ex_unit_prices)
{
  return cardano_object_get_last_error(&ex_unit_prices->base);
}