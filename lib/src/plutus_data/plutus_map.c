/**
 * \file plutus_map.c
 *
 * \author angel.castillo
 * \date   May 12, 2024
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

#include <cardano/object.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/set.h"

#include <assert.h>
#include <cardano/plutus_data/plutus_data.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano plutus map.
 */
typedef struct cardano_plutus_map_t
{
    cardano_object_t  base;
    cardano_array_t*  array;
    cardano_buffer_t* cbor_cache;
    bool              use_indefinite_encoding;
} cardano_plutus_map_t;

/**
 * \brief Represents a Cardano plutus map key value pair.
 */
typedef struct cardano_plutus_map_kvp_t
{
    cardano_object_t       base;
    cardano_plutus_data_t* key;
    cardano_plutus_data_t* value;
} cardano_plutus_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a plutus map object.
 *
 * This function is responsible for properly deallocating a plutus map object (`cardano_plutus_map_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the plutus_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_plutus_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the plutus_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_plutus_map_deallocate(void* object)
{
  assert(object != NULL);

  cardano_plutus_map_t* map = (cardano_plutus_map_t*)object;

  cardano_array_unref(&map->array);
  cardano_buffer_unref(&map->cbor_cache);

  _cardano_free(map);
}

/**
 * \brief Deallocates a plutus map key value pair object.
 *
 * This function is responsible for properly deallocating a plutus map key value pair object (`cardano_plutus_map_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the plutus_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_plutus_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the plutus_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_plutus_map_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_plutus_map_kvp_t* map = (cardano_plutus_map_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_plutus_data_unref(&map->key);
  }

  if (map->value != NULL)
  {
    cardano_plutus_data_unref(&map->value);
  }

  _cardano_free(map);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_plutus_map_new(cardano_plutus_map_t** plutus_map)
{
  if (plutus_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_map_t* map = _cardano_malloc(sizeof(cardano_plutus_map_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count          = 1;
  map->base.last_error[0]      = '\0';
  map->base.deallocator        = cardano_plutus_map_deallocate;
  map->use_indefinite_encoding = false;
  map->cbor_cache              = NULL;

  map->array = cardano_array_new(128);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *plutus_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_map_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_map_t** plutus_map)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t*      cbor_cache  = NULL;
  cardano_cbor_reader_t* reader_copy = NULL;
  cardano_error_t        copy_result = cardano_cbor_reader_clone(reader, &reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    return copy_result;
  }

  copy_result = cardano_cbor_reader_read_encoded_value(reader_copy, &cbor_cache);
  cardano_cbor_reader_unref(&reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    *plutus_map = NULL;
    return copy_result;
  }

  cardano_plutus_map_t* map    = NULL;
  cardano_error_t       result = cardano_plutus_map_new(&map);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    return result;
  }

  map->cbor_cache = cbor_cache;

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_map_unref(&map);
    return result;
  }

  map->use_indefinite_encoding = length < 0;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_map_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_plutus_data_t* key   = NULL;
    cardano_plutus_data_t* value = NULL;

    result = cardano_plutus_data_from_cbor(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_map_unref(&map);
      return result;
    }

    result = cardano_plutus_data_from_cbor(reader, &value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_data_unref(&key);
      cardano_plutus_map_unref(&map);
      return result;
    }

    cardano_plutus_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_plutus_map_kvp_t));

    if (kvp == NULL)
    {
      cardano_plutus_data_unref(&key);
      cardano_plutus_data_unref(&value);
      cardano_plutus_map_unref(&map);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    kvp->base.ref_count     = 0;
    kvp->base.last_error[0] = '\0';
    kvp->base.deallocator   = cardano_plutus_map_kvp_deallocate;
    kvp->key                = key;
    kvp->value              = value;

    const size_t old_size = cardano_array_get_size(map->array);
    const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

    assert((old_size + 1U) == new_size);
    CARDANO_UNUSED(old_size);
    CARDANO_UNUSED(new_size);
  }

  result = cardano_cbor_validate_end_map("plutus_map", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_map_unref(&map);
    return result;
  }

  *plutus_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_map_to_cbor(const cardano_plutus_map_t* plutus_map, cardano_cbor_writer_t* writer)
{
  if (plutus_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_map->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(plutus_map->cbor_cache), cardano_buffer_get_size(plutus_map->cbor_cache));
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (plutus_map->use_indefinite_encoding)
  {
    result = cardano_cbor_writer_write_start_map(writer, -1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    };
  }
  else
  {
    size_t map_size = cardano_array_get_size(plutus_map->array);
    result          = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  for (size_t i = 0; i < cardano_array_get_size(plutus_map->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(plutus_map->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in plutus map is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_plutus_map_kvp_t* kvp_data = (cardano_plutus_map_kvp_t*)((void*)kvp);

    result = cardano_plutus_data_to_cbor(kvp_data->key, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    result = cardano_plutus_data_to_cbor(kvp_data->value, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    cardano_object_unref(&kvp);
  }

  if (plutus_map->use_indefinite_encoding)
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
cardano_plutus_map_get_length(const cardano_plutus_map_t* plutus_map)
{
  if (plutus_map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(plutus_map->array);
}

cardano_error_t
cardano_plutus_map_get(
  cardano_plutus_map_t*   plutus_map,
  cardano_plutus_data_t*  key,
  cardano_plutus_data_t** element)
{
  if (plutus_map == NULL)
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

  for (size_t i = 0; i < cardano_array_get_size(plutus_map->array); ++i)
  {
    cardano_object_t*         object = cardano_array_get(plutus_map->array, i);
    cardano_plutus_map_kvp_t* kvp    = (cardano_plutus_map_kvp_t*)((void*)object);

    if (cardano_plutus_data_equals(kvp->key, key))
    {
      cardano_plutus_data_ref(kvp->value);
      *element = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_plutus_map_insert(
  cardano_plutus_map_t*  plutus_map,
  cardano_plutus_data_t* key,
  cardano_plutus_data_t* value)
{
  if (plutus_map == NULL)
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

  cardano_plutus_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_plutus_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_plutus_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_plutus_data_ref(key);
  cardano_plutus_data_ref(value);

  const size_t old_size = cardano_array_get_size(plutus_map->array);
  const size_t new_size = cardano_array_push(plutus_map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_map_get_keys(
  cardano_plutus_map_t*   plutus_map,
  cardano_plutus_list_t** keys)
{
  if (plutus_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_list_t* list = NULL;

  cardano_error_t result = cardano_plutus_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(plutus_map->array); ++i)
  {
    cardano_object_t*         object = cardano_array_get(plutus_map->array, i);
    cardano_plutus_map_kvp_t* kvp    = (cardano_plutus_map_kvp_t*)((void*)object);

    result = cardano_plutus_list_add(list, kvp->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_map_get_values(
  cardano_plutus_map_t*   plutus_map,
  cardano_plutus_list_t** values)
{
  if (plutus_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (values == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_list_t* list = NULL;

  cardano_error_t result = cardano_plutus_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(plutus_map->array); ++i)
  {
    cardano_object_t*         object = cardano_array_get(plutus_map->array, i);
    cardano_plutus_map_kvp_t* kvp    = (cardano_plutus_map_kvp_t*)((void*)object);

    result = cardano_plutus_list_add(list, kvp->value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *values = list;

  return CARDANO_SUCCESS;
}

bool
cardano_plutus_map_equals(const cardano_plutus_map_t* lhs, const cardano_plutus_map_t* rhs)
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

    cardano_plutus_map_kvp_t* lhs_element_data = (cardano_plutus_map_kvp_t*)((void*)lhs_element);
    cardano_plutus_map_kvp_t* rhs_element_data = (cardano_plutus_map_kvp_t*)((void*)rhs_element);

    if (!cardano_plutus_data_equals(lhs_element_data->key, rhs_element_data->key))
    {
      return false;
    }

    if (!cardano_plutus_data_equals(lhs_element_data->value, rhs_element_data->value))
    {
      return false;
    }
  }

  return true;
}

void
cardano_plutus_map_clear_cbor_cache(cardano_plutus_map_t* plutus_map)
{
  if (plutus_map == NULL)
  {
    return;
  }

  cardano_buffer_unref(&plutus_map->cbor_cache);

  plutus_map->cbor_cache = NULL;
}

void
cardano_plutus_map_unref(cardano_plutus_map_t** plutus_map)
{
  if ((plutus_map == NULL) || (*plutus_map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*plutus_map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *plutus_map = NULL;
    return;
  }
}

void
cardano_plutus_map_ref(cardano_plutus_map_t* plutus_map)
{
  if (plutus_map == NULL)
  {
    return;
  }

  cardano_object_ref(&plutus_map->base);
}

size_t
cardano_plutus_map_refcount(const cardano_plutus_map_t* plutus_map)
{
  if (plutus_map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&plutus_map->base);
}

void
cardano_plutus_map_set_last_error(cardano_plutus_map_t* plutus_map, const char* message)
{
  cardano_object_set_last_error(&plutus_map->base, message);
}

const char*
cardano_plutus_map_get_last_error(const cardano_plutus_map_t* plutus_map)
{
  return cardano_object_get_last_error(&plutus_map->base);
}
