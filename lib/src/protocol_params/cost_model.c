/**
 * \file cost_model.c
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

#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/scripts/plutus_scripts/plutus_language_version.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const size_t PLUTUS_V1_COST_MODEL_OP_COUNT = 166U;
static const size_t PLUTUS_V2_COST_MODEL_OP_COUNT = 175U;
static const size_t PLUTUS_V3_COST_MODEL_OP_COUNT = 179U;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano cost model.
 */
typedef struct cardano_cost_model_t
{
    cardano_object_t                  base;
    cardano_plutus_language_version_t language_version;
    int64_t                           costs[179U];
    size_t                            size;
} cardano_cost_model_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a cost model object.
 *
 * This function is responsible for properly deallocating a cost model object (`cardano_cost_model_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the cost_model object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_cost_model_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the cost_model
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_cost_model_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_cost_model_new(
  cardano_plutus_language_version_t language,
  const int64_t*                    cost_array,
  size_t                            costs_size,
  cardano_cost_model_t**            cost_model)
{
  if (cost_model == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  switch (language)
  {
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V1:
      if (costs_size != PLUTUS_V1_COST_MODEL_OP_COUNT)
      {
        return CARDANO_INVALID_PLUTUS_COST_MODEL;
      }
      break;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V2:
      if (costs_size != PLUTUS_V2_COST_MODEL_OP_COUNT)
      {
        return CARDANO_INVALID_PLUTUS_COST_MODEL;
      }
      break;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V3:
      if (costs_size != PLUTUS_V3_COST_MODEL_OP_COUNT)
      {
        return CARDANO_INVALID_PLUTUS_COST_MODEL;
      }
      break;
    default:
      return CARDANO_INVALID_PLUTUS_COST_MODEL;
  }

  *cost_model = _cardano_malloc(sizeof(cardano_cost_model_t));

  if (*cost_model == NULL)
  {
    return CARDANO_MEMORY_ALLOCATION_FAILED;
  }

  (*cost_model)->base.deallocator   = cardano_cost_model_deallocate;
  (*cost_model)->base.ref_count     = 1;
  (*cost_model)->base.last_error[0] = '\0';

  (*cost_model)->language_version = language;
  (*cost_model)->size             = costs_size;

  assert(costs_size <= 179U);

  for (size_t i = 0U; i < costs_size; ++i)
  {
    (*cost_model)->costs[i] = cost_array[i];
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cost_model_from_cbor(cardano_cbor_reader_t* reader, cardano_cost_model_t** cost_model)
{
  if (cost_model == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *cost_model = NULL;
    return CARDANO_POINTER_IS_NULL;
  }

  static const char* validator_name = "cost_model";

  uint64_t              language_version  = 0U;
  const cardano_error_t read_minor_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "language_version",
    reader,
    &language_version,
    (uint64_t)CARDANO_PLUTUS_LANGUAGE_VERSION_V1,
    (uint64_t)CARDANO_PLUTUS_LANGUAGE_VERSION_V3);

  if (read_minor_result != CARDANO_SUCCESS)
  {
    *cost_model = NULL;
    return read_minor_result;
  }

  const cardano_plutus_language_version_t language                 = (cardano_plutus_language_version_t)language_version;
  size_t                                  expected_cost_model_size = 0U;
  int64_t                                 costs[179U]              = { 0 };

  switch (language)
  {
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V1:
      expected_cost_model_size = PLUTUS_V1_COST_MODEL_OP_COUNT;
      break;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V2:
      expected_cost_model_size = PLUTUS_V2_COST_MODEL_OP_COUNT;
      break;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V3:
      expected_cost_model_size = PLUTUS_V3_COST_MODEL_OP_COUNT;
      break;
    // LCOV_EXCL_START
    default:
      *cost_model = NULL;
      return CARDANO_INVALID_PLUTUS_COST_MODEL;
      // LCOV_EXCL_STOP
  }

  const cardano_error_t expect_start_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, expected_cost_model_size);

  if (expect_start_array_result != CARDANO_SUCCESS)
  {
    *cost_model = NULL;
    return expect_start_array_result;
  }

  for (size_t i = 0U; i < expected_cost_model_size; ++i)
  {
    const cardano_error_t read_cost_result = cardano_cbor_reader_read_int(reader, &costs[i]);

    if (read_cost_result != CARDANO_SUCCESS)
    {
      *cost_model = NULL;
      return read_cost_result;
    }
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    /* LCOV_EXCL_START */
    *cost_model = NULL;

    return expect_end_array_result;
    /* LCOV_EXCL_STOP */
  }

  return cardano_cost_model_new(language_version, costs, expected_cost_model_size, cost_model);
}

cardano_error_t
cardano_cost_model_to_cbor(const cardano_cost_model_t* cost_model, cardano_cbor_writer_t* writer)
{
  if (cost_model == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_unsigned_int(writer, cost_model->language_version);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result; /* LCOV_EXCL_LINE */
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    (int32_t)cost_model->size);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result; /* LCOV_EXCL_LINE */
  }

  for (size_t i = 0U; i < cost_model->size; ++i)
  {
    const cardano_error_t write_int_result = cardano_cbor_writer_write_signed_int(writer, cost_model->costs[i]);

    if (write_int_result != CARDANO_SUCCESS)
    {
      return write_int_result; /* LCOV_EXCL_LINE */
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cost_model_set_cost(
  cardano_cost_model_t* cost_model,
  size_t                operation,
  int64_t               cost)
{
  if (cost_model == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (operation >= cost_model->size)
  {
    return CARDANO_INDEX_OUT_OF_BOUNDS;
  }

  cost_model->costs[operation] = cost;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cost_model_get_cost(const cardano_cost_model_t* cost_model, size_t operation, int64_t* cost)
{
  if (cost_model == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (operation >= cost_model->size)
  {
    return CARDANO_INDEX_OUT_OF_BOUNDS;
  }

  if (cost == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *cost = cost_model->costs[operation];

  return CARDANO_SUCCESS;
}

size_t
cardano_cost_model_get_costs_size(const cardano_cost_model_t* cost_model)
{
  if (cost_model == NULL)
  {
    return 0U;
  }

  return cost_model->size;
}

const int64_t*
cardano_cost_model_get_costs(const cardano_cost_model_t* cost_model)
{
  if (cost_model == NULL)
  {
    return NULL;
  }

  return cost_model->costs;
}

cardano_error_t
cardano_cost_model_get_language(
  const cardano_cost_model_t*        cost_model,
  cardano_plutus_language_version_t* language)
{
  if (cost_model == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (language == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *language = cost_model->language_version;

  return CARDANO_SUCCESS;
}

void
cardano_cost_model_unref(cardano_cost_model_t** cost_model)
{
  if ((cost_model == NULL) || (*cost_model == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*cost_model)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *cost_model = NULL;
    return;
  }
}

void
cardano_cost_model_ref(cardano_cost_model_t* cost_model)
{
  if (cost_model == NULL)
  {
    return;
  }

  cardano_object_ref(&cost_model->base);
}

size_t
cardano_cost_model_refcount(const cardano_cost_model_t* cost_model)
{
  if (cost_model == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&cost_model->base);
}

void
cardano_cost_model_set_last_error(cardano_cost_model_t* cost_model, const char* message)
{
  cardano_object_set_last_error(&cost_model->base, message);
}

const char*
cardano_cost_model_get_last_error(const cardano_cost_model_t* cost_model)
{
  return cardano_object_get_last_error(&cost_model->base);
}