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

/* CONSTANTS *****************************************************************/

#define COSTMDLS_LANGUAGE_ID_COUNT ((size_t)256U)
#define KNOWN_LANGUAGE_ID_COUNT    ((size_t)CARDANO_PLUTUS_LANGUAGE_VERSION_V4 + 1U)

/* STRUCTURES ****************************************************************/

/**
 * \brief Map of language id to CostModel.
 *
 * The models are indexed by their language id. Ids 0 to 3 are the known Plutus language versions; the other ids
 * hold the cost models of languages this library does not know yet, which the ledger accepts and re-encodes.
 */
typedef struct cardano_costmdls_t
{
    cardano_object_t      base;
    cardano_cost_model_t* models[COSTMDLS_LANGUAGE_ID_COUNT];
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

  for (size_t i = 0U; i < COSTMDLS_LANGUAGE_ID_COUNT; ++i)
  {
    cardano_cost_model_unref(&data->models[i]);
  }

  _cardano_free(object);
}

/**
 * \brief Checks whether the given language is a known Plutus language version.
 *
 * \param language The language to check.
 *
 * \return \c true if the language is a known Plutus language version, \c false otherwise.
 */
static bool
is_known_language(const cardano_plutus_language_version_t language)
{
  switch (language)
  {
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V1:
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V2:
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V3:
    case CARDANO_PLUTUS_LANGUAGE_VERSION_V4:
      return true;
    default:
      return false;
  }
}

/**
 * \brief Counts the cost models stored for the language ids in the given range.
 *
 * \param costmdls The costmdls object to count the cost models of.
 * \param first_id The first language id of the range.
 * \param end_id The language id one past the end of the range.
 *
 * \return The number of cost models stored for the language ids in the range.
 */
static size_t
count_models(const cardano_costmdls_t* costmdls, const size_t first_id, const size_t end_id)
{
  size_t count = 0U;

  for (size_t i = first_id; i < end_id; ++i)
  {
    if (costmdls->models[i] != NULL)
    {
      ++count;
    }
  }

  return count;
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

  for (size_t i = 0U; i < COSTMDLS_LANGUAGE_ID_COUNT; ++i)
  {
    (*costmdls)->models[i] = NULL;
  }

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

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    const cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

    if (peek_result != CARDANO_SUCCESS)
    {
      cardano_costmdls_unref(&costmdls_data);
      *costmdls = NULL;

      return peek_result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_cost_model_t* model           = NULL;
    const cardano_error_t read_cost_model = cardano_cost_model_from_cbor(reader, &model);

    if (read_cost_model != CARDANO_SUCCESS)
    {
      cardano_costmdls_unref(&costmdls_data);
      *costmdls = NULL;
      return read_cost_model;
    }

    const cardano_error_t insert_result = cardano_costmdls_insert(costmdls_data, model);
    cardano_cost_model_unref(&model);

    if (insert_result != CARDANO_SUCCESS)
    {
      cardano_costmdls_unref(&costmdls_data);
      *costmdls = NULL;

      return insert_result;
    }
  }

  const cardano_error_t end_map_result = cardano_cbor_validate_end_map("costmdls", reader);

  if (end_map_result != CARDANO_SUCCESS)
  {
    cardano_costmdls_unref(&costmdls_data);
    *costmdls = NULL;

    return end_map_result;
  }

  *costmdls = costmdls_data;

  return CARDANO_SUCCESS;
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

  const size_t map_size = count_models(costmdls, 0U, COSTMDLS_LANGUAGE_ID_COUNT);

  const cardano_error_t write_map_result = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (write_map_result != CARDANO_SUCCESS)
  {
    return write_map_result;
  }

  for (size_t i = 0U; i < COSTMDLS_LANGUAGE_ID_COUNT; ++i)
  {
    if (costmdls->models[i] != NULL)
    {
      const cardano_error_t write_model_result = cardano_cost_model_to_cbor(costmdls->models[i], writer);

      if (write_model_result != CARDANO_SUCCESS)
      {
        return write_model_result;
      }
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_costmdls_to_cip116_json(
  const cardano_costmdls_t* costmdls,
  cardano_json_writer_t*    writer)
{
  if ((costmdls == NULL) || (writer == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* property_names[] = { "plutus_v1", "plutus_v2", "plutus_v3", "plutus_v4" };

  cardano_json_writer_write_start_object(writer);

  for (size_t i = 0U; i < (sizeof(property_names) / sizeof(property_names[0])); ++i)
  {
    if (costmdls->models[i] != NULL)
    {
      cardano_json_writer_write_property_name(writer, property_names[i], strlen(property_names[i]));
      cardano_error_t write_model_result = cardano_cost_model_to_cip116_json(costmdls->models[i], writer);

      if (write_model_result != CARDANO_SUCCESS)
      {
        return write_model_result;
      }
    }
  }

  cardano_json_writer_write_end_object(writer);

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

  uint8_t language_id = 0U;

  const cardano_error_t get_language_result = cardano_cost_model_get_language_id(cost_model, &language_id);

  if (get_language_result != CARDANO_SUCCESS)
  {
    return get_language_result;
  }

  cardano_cost_model_ref(cost_model);
  cardano_cost_model_unref(&costmdls->models[language_id]);
  costmdls->models[language_id] = cost_model;

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

  if (!is_known_language(language))
  {
    return CARDANO_ERROR_INVALID_PLUTUS_COST_MODEL;
  }

  *cost_model = costmdls->models[(size_t)language];
  cardano_cost_model_ref(*cost_model);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_costmdls_get_by_language_id(
  cardano_costmdls_t*    costmdls,
  const uint8_t          language_id,
  cardano_cost_model_t** cost_model)
{
  if (costmdls == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cost_model == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *cost_model = costmdls->models[language_id];
  cardano_cost_model_ref(*cost_model);

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

  if (!is_known_language(language))
  {
    return false;
  }

  return costmdls->models[(size_t)language] != NULL;
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

  const size_t map_size = count_models(costmdls, 0U, KNOWN_LANGUAGE_ID_COUNT);

  const cardano_error_t write_map_result = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (write_map_result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);

    return write_map_result;
  }

  for (size_t i = (size_t)CARDANO_PLUTUS_LANGUAGE_VERSION_V2; i < KNOWN_LANGUAGE_ID_COUNT; ++i)
  {
    if (costmdls->models[i] != NULL)
    {
      const cardano_error_t write_model_result = cardano_cost_model_to_cbor(costmdls->models[i], writer);

      if (write_model_result != CARDANO_SUCCESS)
      {
        cardano_cbor_writer_unref(&writer);
        return write_model_result;
      }
    }
  }

  const cardano_cost_model_t* plutus_v1_costs = costmdls->models[(size_t)CARDANO_PLUTUS_LANGUAGE_VERSION_V1];

  if (plutus_v1_costs != NULL)
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

    const size_t v1_costs_size = cardano_cost_model_get_costs_size(plutus_v1_costs);

    for (size_t i = 0U; i < v1_costs_size; ++i)
    {
      int64_t cost = 0;

      cardano_error_t get_cost_result = cardano_cost_model_get_cost(plutus_v1_costs, i, &cost);

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