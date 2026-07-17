/**
 * \file required_guards_map.c
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
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
#include <cardano/transaction_body/required_guards_map.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a map of credentials to an optional plutus datum.
 */
typedef struct cardano_required_guards_map_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_required_guards_map_t;

/**
 * \brief Represents a credential to optional plutus datum key value pair.
 */
typedef struct cardano_required_guards_map_kvp_t
{
    cardano_object_t       base;
    cardano_credential_t*  key;
    cardano_plutus_data_t* value;
} cardano_required_guards_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a required guards map object.
 *
 * This function is responsible for properly deallocating a required guards map object (`cardano_required_guards_map_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the required_guards_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_required_guards_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the required_guards_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_required_guards_map_deallocate(void* object)
{
  assert(object != NULL);

  cardano_required_guards_map_t* map = (cardano_required_guards_map_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a required guards map key value pair object.
 *
 * This function is responsible for properly deallocating a required guards map key value pair object
 * (`cardano_required_guards_map_kvp_t`) and its associated resources.
 *
 * \param object A void pointer to the required_guards_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_required_guards_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the required_guards_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_required_guards_map_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_required_guards_map_kvp_t* map = (cardano_required_guards_map_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_credential_unref(&map->key);
  }

  if (map->value != NULL)
  {
    cardano_plutus_data_unref(&map->value);
  }

  _cardano_free(map);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_required_guards_map_new(cardano_required_guards_map_t** required_guards_map)
{
  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_required_guards_map_t* map = _cardano_malloc(sizeof(cardano_required_guards_map_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_required_guards_map_deallocate;

  map->array = cardano_array_new(32);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *required_guards_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_required_guards_map_from_cbor(cardano_cbor_reader_t* reader, cardano_required_guards_map_t** required_guards_map)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_required_guards_map_t* map    = NULL;
  cardano_error_t                result = cardano_required_guards_map_new(&map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_required_guards_map_unref(&map);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  result = cardano_cbor_reader_peek_state(reader, &state);

  if (result != CARDANO_SUCCESS)
  {
    cardano_required_guards_map_unref(&map);
    return result;
  }

  if (state == CARDANO_CBOR_READER_STATE_END_MAP)
  {
    cardano_cbor_reader_set_last_error(reader, "There was an error decoding 'required_guards_map', the map must not be empty.");
    cardano_required_guards_map_unref(&map);

    return CARDANO_ERROR_INVALID_CBOR_MAP_SIZE;
  }

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_required_guards_map_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_credential_t*  key   = NULL;
    cardano_plutus_data_t* value = NULL;

    result = cardano_credential_from_cbor(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_required_guards_map_unref(&map);
      return result;
    }

    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_credential_unref(&key);
      cardano_required_guards_map_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_NULL)
    {
      result = cardano_cbor_reader_read_null(reader);
    }
    else
    {
      result = cardano_plutus_data_from_cbor(reader, &value);
    }

    if (result != CARDANO_SUCCESS)
    {
      cardano_credential_unref(&key);
      cardano_required_guards_map_unref(&map);
      return result;
    }

    result = cardano_required_guards_map_insert(map, key, value);

    cardano_credential_unref(&key);
    cardano_plutus_data_unref(&value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_reader_set_last_error(reader, "There was an error decoding 'required_guards_map', the map must not contain duplicated keys.");
      cardano_required_guards_map_unref(&map);

      return result;
    }
  }

  result = cardano_cbor_validate_end_map("required_guards_map", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_required_guards_map_unref(&map);
    return result;
  }

  *required_guards_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_required_guards_map_to_cbor(const cardano_required_guards_map_t* required_guards_map, cardano_cbor_writer_t* writer)
{
  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  size_t map_size = cardano_array_get_size(required_guards_map->array);
  result          = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(required_guards_map->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(required_guards_map->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in required guards map is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_required_guards_map_kvp_t* kvp_data = (cardano_required_guards_map_kvp_t*)((void*)kvp);

    result = cardano_credential_to_cbor(kvp_data->key, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    if (kvp_data->value == NULL)
    {
      result = cardano_cbor_writer_write_null(writer);
    }
    else
    {
      result = cardano_plutus_data_to_cbor(kvp_data->value, writer);
    }

    cardano_object_unref(&kvp);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_required_guards_map_get_length(const cardano_required_guards_map_t* required_guards_map)
{
  if (required_guards_map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(required_guards_map->array);
}

cardano_error_t
cardano_required_guards_map_get(
  const cardano_required_guards_map_t* required_guards_map,
  cardano_credential_t*                key,
  cardano_plutus_data_t**              element)
{
  if (required_guards_map == NULL)
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

  for (size_t i = 0; i < cardano_array_get_size(required_guards_map->array); ++i)
  {
    cardano_object_t*                  object = cardano_array_get(required_guards_map->array, i);
    cardano_required_guards_map_kvp_t* kvp    = (cardano_required_guards_map_kvp_t*)((void*)object);

    if (cardano_credential_equals(kvp->key, key))
    {
      if (kvp->value != NULL)
      {
        cardano_plutus_data_ref(kvp->value);
      }

      *element = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_required_guards_map_insert(
  cardano_required_guards_map_t* required_guards_map,
  cardano_credential_t*          key,
  cardano_plutus_data_t*         value)
{
  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0; i < cardano_array_get_size(required_guards_map->array); ++i)
  {
    cardano_object_t*                  object = cardano_array_get(required_guards_map->array, i);
    cardano_required_guards_map_kvp_t* kvp    = (cardano_required_guards_map_kvp_t*)((void*)object);

    const bool duplicated = cardano_credential_equals(kvp->key, key);

    cardano_object_unref(&object);

    if (duplicated)
    {
      return CARDANO_ERROR_DUPLICATED_KEY;
    }
  }

  cardano_required_guards_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_required_guards_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_required_guards_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_credential_ref(key);

  if (value != NULL)
  {
    cardano_plutus_data_ref(value);
  }

  const size_t old_size = cardano_array_get_size(required_guards_map->array);
  const size_t new_size = cardano_array_push(required_guards_map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_required_guards_map_get_keys(
  cardano_required_guards_map_t* required_guards_map,
  cardano_credential_set_t**     keys)
{
  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_set_t* list = NULL;

  cardano_error_t result = cardano_credential_set_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(required_guards_map->array); ++i)
  {
    cardano_object_t*                  object = cardano_array_get(required_guards_map->array, i);
    cardano_required_guards_map_kvp_t* kvp    = (cardano_required_guards_map_kvp_t*)((void*)object);

    result = cardano_credential_set_add(list, kvp->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_credential_set_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_required_guards_map_get_key_at(
  const cardano_required_guards_map_t* required_guards_map,
  const size_t                         index,
  cardano_credential_t**               credential)
{
  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(required_guards_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                  object = cardano_array_get(required_guards_map->array, index);
  cardano_required_guards_map_kvp_t* kvp    = (cardano_required_guards_map_kvp_t*)((void*)object);

  cardano_credential_ref(kvp->key);
  cardano_object_unref(&object);

  *credential = kvp->key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_required_guards_map_get_value_at(
  const cardano_required_guards_map_t* required_guards_map,
  const size_t                         index,
  cardano_plutus_data_t**              datum)
{
  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(required_guards_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                  object = cardano_array_get(required_guards_map->array, index);
  cardano_required_guards_map_kvp_t* kvp    = (cardano_required_guards_map_kvp_t*)((void*)object);

  if (kvp->value != NULL)
  {
    cardano_plutus_data_ref(kvp->value);
  }

  cardano_object_unref(&object);

  *datum = kvp->value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_required_guards_map_get_key_value_at(
  const cardano_required_guards_map_t* required_guards_map,
  const size_t                         index,
  cardano_credential_t**               credential,
  cardano_plutus_data_t**              datum)
{
  if (required_guards_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (datum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(required_guards_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                  object = cardano_array_get(required_guards_map->array, index);
  cardano_required_guards_map_kvp_t* kvp    = (cardano_required_guards_map_kvp_t*)((void*)object);

  cardano_credential_ref(kvp->key);

  if (kvp->value != NULL)
  {
    cardano_plutus_data_ref(kvp->value);
  }

  cardano_object_unref(&object);

  *credential = kvp->key;
  *datum      = kvp->value;

  return CARDANO_SUCCESS;
}

void
cardano_required_guards_map_unref(cardano_required_guards_map_t** required_guards_map)
{
  if ((required_guards_map == NULL) || (*required_guards_map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*required_guards_map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *required_guards_map = NULL;
    return;
  }
}

void
cardano_required_guards_map_ref(cardano_required_guards_map_t* required_guards_map)
{
  if (required_guards_map == NULL)
  {
    return;
  }

  cardano_object_ref(&required_guards_map->base);
}

size_t
cardano_required_guards_map_refcount(const cardano_required_guards_map_t* required_guards_map)
{
  if (required_guards_map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&required_guards_map->base);
}

void
cardano_required_guards_map_set_last_error(cardano_required_guards_map_t* required_guards_map, const char* message)
{
  cardano_object_set_last_error(&required_guards_map->base, message);
}

const char*
cardano_required_guards_map_get_last_error(const cardano_required_guards_map_t* required_guards_map)
{
  return cardano_object_get_last_error(&required_guards_map->base);
}
