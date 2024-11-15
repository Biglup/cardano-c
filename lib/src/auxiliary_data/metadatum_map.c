/**
 * \file metadatum_map.c
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
 *
 * Copyright 2024 Biglup Labs
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

#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/metadatum_list.h>
#include <cardano/auxiliary_data/metadatum_map.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/set.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano metadatum map.
 */
typedef struct cardano_metadatum_map_t
{
    cardano_object_t base;
    cardano_array_t* array;
    bool             use_indefinite_encoding;
} cardano_metadatum_map_t;

/**
 * \brief Represents a Cardano metadatum map key value pair.
 */
typedef struct cardano_metadatum_map_kvp_t
{
    cardano_object_t     base;
    cardano_metadatum_t* key;
    cardano_metadatum_t* value;
} cardano_metadatum_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a metadatum map object.
 *
 * This function is responsible for properly deallocating a metadatum map object (`cardano_metadatum_map_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the metadatum_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_metadatum_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the metadatum_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_metadatum_map_deallocate(void* object)
{
  assert(object != NULL);

  cardano_metadatum_map_t* map = (cardano_metadatum_map_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a metadatum map key value pair object.
 *
 * This function is responsible for properly deallocating a metadatum map key value pair object (`cardano_metadatum_map_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the metadatum_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_metadatum_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the metadatum_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_metadatum_map_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_metadatum_map_kvp_t* map = (cardano_metadatum_map_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_metadatum_unref(&map->key);
  }

  if (map->value != NULL)
  {
    cardano_metadatum_unref(&map->value);
  }

  _cardano_free(map);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_metadatum_map_new(cardano_metadatum_map_t** metadatum_map)
{
  if (metadatum_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_map_t* map = _cardano_malloc(sizeof(cardano_metadatum_map_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count          = 1;
  map->base.last_error[0]      = '\0';
  map->base.deallocator        = cardano_metadatum_map_deallocate;
  map->use_indefinite_encoding = false;

  map->array = cardano_array_new(128);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *metadatum_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_map_from_cbor(cardano_cbor_reader_t* reader, cardano_metadatum_map_t** metadatum_map)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (metadatum_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_map_t* map    = NULL;
  cardano_error_t          result = cardano_metadatum_map_new(&map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_metadatum_map_unref(&map);
    return result;
  }

  map->use_indefinite_encoding = length < 0;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_metadatum_map_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_metadatum_t* key   = NULL;
    cardano_metadatum_t* value = NULL;

    result = cardano_metadatum_from_cbor(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_metadatum_map_unref(&map);
      return result;
    }

    result = cardano_metadatum_from_cbor(reader, &value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_metadatum_unref(&key);
      cardano_metadatum_map_unref(&map);
      return result;
    }

    cardano_metadatum_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_metadatum_map_kvp_t));

    if (kvp == NULL)
    {
      cardano_metadatum_unref(&key);
      cardano_metadatum_unref(&value);
      cardano_metadatum_map_unref(&map);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    kvp->base.ref_count     = 0;
    kvp->base.last_error[0] = '\0';
    kvp->base.deallocator   = cardano_metadatum_map_kvp_deallocate;
    kvp->key                = key;
    kvp->value              = value;

    const size_t old_size = cardano_array_get_size(map->array);
    const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

    assert((old_size + 1U) == new_size);
    CARDANO_UNUSED(old_size);
    CARDANO_UNUSED(new_size);
  }

  result = cardano_cbor_validate_end_map("metadatum_map", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_metadatum_map_unref(&map);
    return result;
  }

  *metadatum_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_map_to_cbor(const cardano_metadatum_map_t* metadatum_map, cardano_cbor_writer_t* writer)
{
  if (metadatum_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (metadatum_map->use_indefinite_encoding)
  {
    result = cardano_cbor_writer_write_start_map(writer, -1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    };
  }
  else
  {
    size_t map_size = cardano_array_get_size(metadatum_map->array);
    result          = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  for (size_t i = 0; i < cardano_array_get_size(metadatum_map->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(metadatum_map->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in metadatum map is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_metadatum_map_kvp_t* kvp_data = (cardano_metadatum_map_kvp_t*)((void*)kvp);

    result = cardano_metadatum_to_cbor(kvp_data->key, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    result = cardano_metadatum_to_cbor(kvp_data->value, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    cardano_object_unref(&kvp);
  }

  if (metadatum_map->use_indefinite_encoding)
  {
    result = cardano_cbor_writer_write_end_map(writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_metadatum_map_get_length(const cardano_metadatum_map_t* metadatum_map)
{
  if (metadatum_map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(metadatum_map->array);
}

cardano_error_t
cardano_metadatum_map_get(
  cardano_metadatum_map_t* metadatum_map,
  cardano_metadatum_t*     key,
  cardano_metadatum_t**    element)
{
  if (metadatum_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0; i < cardano_array_get_size(metadatum_map->array); ++i)
  {
    cardano_object_t*            object = cardano_array_get(metadatum_map->array, i);
    cardano_metadatum_map_kvp_t* kvp    = (cardano_metadatum_map_kvp_t*)((void*)object);

    if (cardano_metadatum_equals(kvp->key, key))
    {
      cardano_metadatum_ref(kvp->value);
      *element = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_metadatum_map_insert(
  cardano_metadatum_map_t* metadatum_map,
  cardano_metadatum_t*     key,
  cardano_metadatum_t*     value)
{
  if (metadatum_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_metadatum_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_metadatum_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_metadatum_ref(key);
  cardano_metadatum_ref(value);

  const size_t old_size = cardano_array_get_size(metadatum_map->array);
  const size_t new_size = cardano_array_push(metadatum_map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_map_get_keys(
  cardano_metadatum_map_t*   metadatum_map,
  cardano_metadatum_list_t** keys)
{
  if (metadatum_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_list_t* list = NULL;

  cardano_error_t result = cardano_metadatum_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(metadatum_map->array); ++i)
  {
    cardano_object_t*            object = cardano_array_get(metadatum_map->array, i);
    cardano_metadatum_map_kvp_t* kvp    = (cardano_metadatum_map_kvp_t*)((void*)object);

    result = cardano_metadatum_list_add(list, kvp->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_metadatum_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_map_get_values(
  cardano_metadatum_map_t*   metadatum_map,
  cardano_metadatum_list_t** values)
{
  if (metadatum_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (values == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_list_t* list = NULL;

  cardano_error_t result = cardano_metadatum_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(metadatum_map->array); ++i)
  {
    cardano_object_t*            object = cardano_array_get(metadatum_map->array, i);
    cardano_metadatum_map_kvp_t* kvp    = (cardano_metadatum_map_kvp_t*)((void*)object);

    result = cardano_metadatum_list_add(list, kvp->value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_metadatum_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *values = list;

  return CARDANO_SUCCESS;
}

bool
cardano_metadatum_map_equals(const cardano_metadatum_map_t* lhs, const cardano_metadatum_map_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if ((lhs == NULL) || (rhs == NULL))
  {
    return false;
  }

  if (cardano_array_get_size(lhs->array) != cardano_array_get_size(rhs->array))
  {
    return false;
  }

  for (size_t i = 0; i < cardano_array_get_size(lhs->array); ++i)
  {
    cardano_object_t* lhs_element = cardano_array_get(lhs->array, i);
    cardano_object_t* rhs_element = cardano_array_get(rhs->array, i);

    cardano_object_unref(&lhs_element);
    cardano_object_unref(&rhs_element);

    cardano_metadatum_map_kvp_t* lhs_element_data = (cardano_metadatum_map_kvp_t*)((void*)lhs_element);
    cardano_metadatum_map_kvp_t* rhs_element_data = (cardano_metadatum_map_kvp_t*)((void*)rhs_element);

    if (!cardano_metadatum_equals(lhs_element_data->key, rhs_element_data->key))
    {
      return false;
    }

    if (!cardano_metadatum_equals(lhs_element_data->value, rhs_element_data->value))
    {
      return false;
    }
  }

  return true;
}

void
cardano_metadatum_map_unref(cardano_metadatum_map_t** metadatum_map)
{
  if ((metadatum_map == NULL) || (*metadatum_map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*metadatum_map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *metadatum_map = NULL;
    return;
  }
}

void
cardano_metadatum_map_ref(cardano_metadatum_map_t* metadatum_map)
{
  if (metadatum_map == NULL)
  {
    return;
  }

  cardano_object_ref(&metadatum_map->base);
}

size_t
cardano_metadatum_map_refcount(const cardano_metadatum_map_t* metadatum_map)
{
  if (metadatum_map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&metadatum_map->base);
}

void
cardano_metadatum_map_set_last_error(cardano_metadatum_map_t* metadatum_map, const char* message)
{
  cardano_object_set_last_error(&metadatum_map->base, message);
}

const char*
cardano_metadatum_map_get_last_error(const cardano_metadatum_map_t* metadatum_map)
{
  return cardano_object_get_last_error(&metadatum_map->base);
}
