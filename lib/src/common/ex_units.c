/**
 * \file ex_units.c
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

#include <cardano/common/ex_units.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EX_UNITS_EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano ex_units.
 */
typedef struct cardano_ex_units_t
{
    cardano_object_t base;
    uint64_t         memory;
    uint64_t         cpu;
} cardano_ex_units_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a ex_units object.
 *
 * This function is responsible for properly deallocating a execution units object (`cardano_ex_units_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the ex_units object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_ex_units_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the ex_units
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_ex_units_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_ex_units_new(const uint64_t memory, const uint64_t cpu_steps, cardano_ex_units_t** ex_units)
{
  if (ex_units == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *ex_units = _cardano_malloc(sizeof(cardano_ex_units_t));

  if (*ex_units == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*ex_units)->base.deallocator   = cardano_ex_units_deallocate;
  (*ex_units)->base.ref_count     = 1;
  (*ex_units)->base.last_error[0] = '\0';

  (*ex_units)->memory = memory;
  (*ex_units)->cpu    = cpu_steps;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ex_units_from_cbor(cardano_cbor_reader_t* reader, cardano_ex_units_t** ex_units)
{
  if (ex_units == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *ex_units = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "ex_units";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EX_UNITS_EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *ex_units = NULL;
    return expect_array_result;
  }

  uint64_t              memory           = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "memory",
    reader,
    &memory,
    0,
    UINT64_MAX);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *ex_units = NULL;
    return read_uint_result;
  }

  uint64_t              cpu_steps       = 0U;
  const cardano_error_t read_cpu_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "cpu",
    reader,
    &cpu_steps,
    0,
    UINT64_MAX);

  if (read_cpu_result != CARDANO_SUCCESS)
  {
    *ex_units = NULL;
    return read_cpu_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *ex_units = NULL;

    return expect_end_array_result;
  }

  return cardano_ex_units_new(memory, cpu_steps, ex_units);
}

cardano_error_t
cardano_ex_units_to_cbor(const cardano_ex_units_t* ex_units, cardano_cbor_writer_t* writer)
{
  if (ex_units == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, EX_UNITS_EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, ex_units->memory);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  return cardano_cbor_writer_write_uint(writer, ex_units->cpu);
}

uint64_t
cardano_ex_units_get_memory(const cardano_ex_units_t* ex_units)
{
  if (ex_units == NULL)
  {
    return 0;
  }

  return ex_units->memory;
}

cardano_error_t
cardano_ex_units_set_memory(cardano_ex_units_t* ex_units, const uint64_t memory)
{
  if (ex_units == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  ex_units->memory = memory;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_ex_units_get_cpu_steps(const cardano_ex_units_t* ex_units)
{
  if (ex_units == NULL)
  {
    return 0;
  }

  return ex_units->cpu;
}

cardano_error_t
cardano_ex_units_set_cpu_steps(cardano_ex_units_t* ex_units, const uint64_t cpu_steps)
{
  if (ex_units == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  ex_units->cpu = cpu_steps;

  return CARDANO_SUCCESS;
}

void
cardano_ex_units_unref(cardano_ex_units_t** ex_units)
{
  if ((ex_units == NULL) || (*ex_units == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*ex_units)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *ex_units = NULL;
    return;
  }
}

void
cardano_ex_units_ref(cardano_ex_units_t* ex_units)
{
  if (ex_units == NULL)
  {
    return;
  }

  cardano_object_ref(&ex_units->base);
}

size_t
cardano_ex_units_refcount(const cardano_ex_units_t* ex_units)
{
  if (ex_units == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&ex_units->base);
}

void
cardano_ex_units_set_last_error(cardano_ex_units_t* ex_units, const char* message)
{
  cardano_object_set_last_error(&ex_units->base, message);
}

const char*
cardano_ex_units_get_last_error(const cardano_ex_units_t* ex_units)
{
  return cardano_object_get_last_error(&ex_units->base);
}