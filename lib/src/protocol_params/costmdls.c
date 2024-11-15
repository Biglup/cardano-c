/**
 * \file costmdls.c
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

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/object.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/scripts/plutus_scripts/plutus_language_version.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Map of PlutusLanguageVersion to CostModel.
 */
typedef struct cardano_costmdls_t
{
    cardano_object_t      base;
    cardano_cost_model_t* plutus_v1_costs;
    cardano_cost_model_t* plutus_v2_costs;
    cardano_cost_model_t* plutus_v3_costs;
} cardano_costmdls_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a cost model object.
 *
 * This function is responsible for properly deallocating a cost model object (`cardano_costmdls_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the costmdls object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_costmdls_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the costmdls
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_costmdls_deallocate(void* object)
{
  assert(object != NULL);

  cardano_costmdls_t* data = (cardano_costmdls_t*)object;

  cardano_cost_model_unref(&data->plutus_v1_costs);
  cardano_cost_model_unref(&data->plutus_v2_costs);
  cardano_cost_model_unref(&data->plutus_v3_costs);

  _cardano_free(object);
}

/**
 * \brief Computes the map size of this costmdls object.
 *
 * \param costmdls The costmdls object to compute the map size for.
 *
 * \return The map size of the costmdls object.
 */
static size_t
get_map_size(const cardano_costmdls_t* costmdls)
{
  size_t map_size = 0U;

  if (costmdls->plutus_v1_costs != NULL)
  {
    ++map_size;
  }

  if (costmdls->plutus_v2_costs != NULL)
  {
    ++map_size;
  }

  if (costmdls->plutus_v3_costs != NULL)
  {
    ++map_size;
  }

  return map_size;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_costmdls_new(cardano_costmdls_t** costmdls)
{
  if (costmdls == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *costmdls = _cardano_malloc(sizeof(cardano_costmdls_t));

  if (*costmdls == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*costmdls)->base.deallocator   = cardano_costmdls_deallocate;
  (*costmdls)->base.ref_count     = 1;
  (*costmdls)->base.last_error[0] = '\0';
  (*costmdls)->plutus_v1_costs    = NULL;
  (*costmdls)->plutus_v2_costs    = NULL;
  (*costmdls)->plutus_v3_costs    = NULL;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_costmdls_from_cbor(cardano_cbor_reader_t* reader, cardano_costmdls_t** costmdls)
{
  if (costmdls == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *costmdls = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t map_size = 0U;

  const cardano_error_t read_map_result = cardano_cbor_reader_read_start_map(reader, &map_size);

  if (read_map_result != CARDANO_SUCCESS)
  {
    *costmdls = NULL;
    return read_map_result;
  }

  cardano_costmdls_t*   costmdls_data       = NULL;
  const cardano_error_t costmdls_new_result = cardano_costmdls_new(&costmdls_data);

  if (costmdls_new_result != CARDANO_SUCCESS)
  {
    *costmdls = NULL;
    return costmdls_new_result;
  }

  for (size_t i = 0U; i < (size_t)map_size; ++i)
  {
    cardano_cost_model_t* model           = NULL;
    const cardano_error_t read_cost_model = cardano_cost_model_from_cbor(reader, &model);

    if (read_cost_model != CARDANO_SUCCESS)
    {
      cardano_costmdls_unref(&costmdls_data);
      *costmdls = NULL;
      return read_cost_model;
    }

    cardano_plutus_language_version_t language;

    const cardano_error_t get_language_result = cardano_cost_model_get_language(model, &language);

    if (get_language_result != CARDANO_SUCCESS)
    {
      cardano_cost_model_unref(&model);
      cardano_costmdls_unref(&costmdls_data);
      *costmdls = NULL;

      return get_language_result;
    }

    switch (language)
    {
      case CARDANO_PLUTUS_LANGUAGE_VERSION_V1:
        cardano_cost_model_unref(&costmdls_data->plutus_v1_costs);
        costmdls_data->plutus_v1_costs = model;
        break;
      case CARDANO_PLUTUS_LANGUAGE_VERSION_V2:
        cardano_cost_model_unref(&costmdls_data->plutus_v2_costs);
        costmdls_data->plutus_v2_costs = model;
        break;
      case CARDANO_PLUTUS_LANGUAGE_VERSION_V3:
        cardano_cost_model_unref(&costmdls_data->plutus_v3_costs);
        costmdls_data->plutus_v3_costs = model;
        break;

      default:
        cardano_costmdls_unref(&costmdls_data);
        *costmdls = NULL;
        return CARDANO_ERROR_INVALID_PLUTUS_COST_MODEL;
    }
  }

  *costmdls = costmdls_data;

  return cardano_cbor_validate_end_map("costmdls", reader);
}

cardano_error_t
cardano_costmdls_to_cbor(const cardano_costmdls_t* costmdls, cardano_cbor_writer_t* writer)
{
  if (costmdls == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t map_size = get_map_size(costmdls);

  const cardano_error_t write_map_result = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (write_map_result != CARDANO_SUCCESS)
  {
    return write_map_result;
  }

  if (costmdls->plutus_v1_costs != NULL)
  {
    const cardano_error_t write_v1_result = cardano_cost_model_to_cbor(costmdls->plutus_v1_costs, writer);

    if (write_v1_result != CARDANO_SUCCESS)
    {
      return write_v1_result;
    }
  }

  if (costmdls->plutus_v2_costs != NULL)
  {
    const cardano_error_t write_v2_result = cardano_cost_model_to_cbor(costmdls->plutus_v2_costs, writer);

    if (write_v2_result != CARDANO_SUCCESS)
    {
      return write_v2_result;
    }
  }

  if (costmdls->plutus_v3_costs != NULL)
  {
    const cardano_error_t write_v3_result = cardano_cost_model_to_cbor(costmdls->plutus_v3_costs, writer);

    if (write_v3_result != CARDANO_SUCCESS)
    {
      return write_v3_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_costmdls_insert(
  cardano_costmdls_t*   costmdls,
  cardano_cost_model_t* cost_model)
{
  if (costmdls == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cost_model == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_language_version_t language;

  const cardano_error_t get_language_result = cardano_cost_model_get_language(cost_model, &language);

  if (get_language_result != CARDANO_SUCCESS)
  {
    return get_language_result;
  }

  switch (language)
  {
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V1:
      cardano_cost_model_unref(&costmdls->plutus_v1_costs);
      costmdls->plutus_v1_costs = cost_model;
      cardano_cost_model_ref(costmdls->plutus_v1_costs);
      break;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V2:
      cardano_cost_model_unref(&costmdls->plutus_v2_costs);
      costmdls->plutus_v2_costs = cost_model;
      cardano_cost_model_ref(costmdls->plutus_v2_costs);
      break;

    case CARDANO_PLUTUS_LANGUAGE_VERSION_V3:
      cardano_cost_model_unref(&costmdls->plutus_v3_costs);
      costmdls->plutus_v3_costs = cost_model;
      cardano_cost_model_ref(costmdls->plutus_v3_costs);
      break;

    default:
      return CARDANO_ERROR_INVALID_PLUTUS_COST_MODEL;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_costmdls_get(
  cardano_costmdls_t*                     costmdls,
  const cardano_plutus_language_version_t language,
  cardano_cost_model_t**                  cost_model)
{
  if (costmdls == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cost_model == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  switch (language)
  {
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V1:
      *cost_model = costmdls->plutus_v1_costs;
      cardano_cost_model_ref(costmdls->plutus_v1_costs);
      break;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V2:
      *cost_model = costmdls->plutus_v2_costs;
      cardano_cost_model_ref(costmdls->plutus_v2_costs);
      break;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V3:
      *cost_model = costmdls->plutus_v3_costs;
      cardano_cost_model_ref(costmdls->plutus_v3_costs);
      break;
    default:
      return CARDANO_ERROR_INVALID_PLUTUS_COST_MODEL;
  }

  return CARDANO_SUCCESS;
}

bool
cardano_costmdls_has(
  const cardano_costmdls_t*               costmdls,
  const cardano_plutus_language_version_t language)
{
  if (costmdls == NULL)
  {
    return false;
  }

  switch (language)
  {
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V1:
      return costmdls->plutus_v1_costs != NULL;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V2:
      return costmdls->plutus_v2_costs != NULL;
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V3:
      return costmdls->plutus_v3_costs != NULL;
    default:
      return false;
  }
}

cardano_error_t
cardano_costmdls_get_language_views_encoding(
  const cardano_costmdls_t* costmdls,
  cardano_buffer_t**        language_views)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const size_t map_size = get_map_size(costmdls);

  const cardano_error_t write_map_result = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (write_map_result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);

    return write_map_result;
  }

  if (costmdls->plutus_v2_costs != NULL)
  {
    const cardano_error_t write_plutus_v2 = cardano_cost_model_to_cbor(costmdls->plutus_v2_costs, writer);

    if (write_plutus_v2 != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return write_plutus_v2;
    }
  }

  if (costmdls->plutus_v3_costs != NULL)
  {
    const cardano_error_t write_plutus_v3 = cardano_cost_model_to_cbor(costmdls->plutus_v3_costs, writer);

    if (write_plutus_v3 != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return write_plutus_v3;
    }
  }

  if (costmdls->plutus_v1_costs != NULL)
  {
    cardano_cbor_writer_t* inner_writer = cardano_cbor_writer_new();

    if (inner_writer == NULL)
    {
      cardano_cbor_writer_unref(&writer);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    const cardano_error_t write_v1_result = cardano_cbor_writer_write_start_array(inner_writer, -1);

    if (write_v1_result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&inner_writer);
      cardano_cbor_writer_unref(&writer);

      return write_v1_result;
    }

    for (size_t i = 0U; i < 166U; ++i)
    {
      int64_t cost = 0;

      cardano_error_t get_cost_result = cardano_cost_model_get_cost(costmdls->plutus_v1_costs, i, &cost);

      if (get_cost_result != CARDANO_SUCCESS)
      {
        cardano_cbor_writer_unref(&inner_writer);
        cardano_cbor_writer_unref(&writer);

        return get_cost_result;
      }

      const cardano_error_t write_result = cardano_cbor_writer_write_signed_int(inner_writer, cost);

      if (write_result != CARDANO_SUCCESS)
      {
        cardano_cbor_writer_unref(&inner_writer);
        cardano_cbor_writer_unref(&writer);

        return write_result;
      }
    }

    const cardano_error_t write_v1_end_result = cardano_cbor_writer_write_end_array(inner_writer);

    if (write_v1_end_result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&inner_writer);
      cardano_cbor_writer_unref(&writer);

      return write_v1_end_result;
    }

    cardano_buffer_t*     buffer             = NULL;
    const cardano_error_t get_encoded_result = cardano_cbor_writer_encode_in_buffer(inner_writer, &buffer);

    if (get_encoded_result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&inner_writer);
      cardano_cbor_writer_unref(&writer);

      return get_encoded_result;
    }

    byte_t plutus_v1_id = 0U;

    cardano_error_t write_inner_data_result = cardano_cbor_writer_write_bytestring(writer, &plutus_v1_id, 1);
    cardano_cbor_writer_unref(&inner_writer);

    if (write_inner_data_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&buffer);
      cardano_cbor_writer_unref(&inner_writer);
      cardano_cbor_writer_unref(&writer);

      return write_inner_data_result;
    }

    write_inner_data_result = cardano_cbor_writer_write_bytestring(writer, cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer));
    cardano_buffer_unref(&buffer);

    if (write_inner_data_result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&inner_writer);
      cardano_cbor_writer_unref(&writer);

      return write_inner_data_result;
    }
  }

  cardano_buffer_t* buffer = NULL;

  const cardano_error_t get_encoded_result = cardano_cbor_writer_encode_in_buffer(writer, &buffer);

  cardano_cbor_writer_unref(&writer);

  if (get_encoded_result != CARDANO_SUCCESS)
  {
    return get_encoded_result;
  }

  *language_views = buffer;

  return CARDANO_SUCCESS;
}

void
cardano_costmdls_unref(cardano_costmdls_t** costmdls)
{
  if ((costmdls == NULL) || (*costmdls == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*costmdls)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *costmdls = NULL;
    return;
  }
}

void
cardano_costmdls_ref(cardano_costmdls_t* costmdls)
{
  if (costmdls == NULL)
  {
    return;
  }

  cardano_object_ref(&costmdls->base);
}

size_t
cardano_costmdls_refcount(const cardano_costmdls_t* costmdls)
{
  if (costmdls == NULL)
  {
    return 0U;
  }

  return cardano_object_refcount(&costmdls->base);
}

void
cardano_costmdls_set_last_error(cardano_costmdls_t* costmdls, const char* message)
{
  cardano_object_set_last_error(&costmdls->base, message);
}

const char*
cardano_costmdls_get_last_error(const cardano_costmdls_t* costmdls)
{
  return cardano_object_get_last_error(&costmdls->base);
}