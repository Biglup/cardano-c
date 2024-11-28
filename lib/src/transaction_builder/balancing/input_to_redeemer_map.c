/**
 * \file input_to_redeemer_map.c
 *
 * \author angel.castillo
 * \date   Nov 11, 2024
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

#include <cardano/transaction_builder/balancing/input_to_redeemer_map.h>

#include "../../allocators.h"
#include "../../collections/array.h"
#include <assert.h>
#include <cardano/object.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/witness_set/redeemer.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano input to redeemer input_to_redeemer_map.
 */
typedef struct cardano_input_to_redeemer_map_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_input_to_redeemer_map_t;

/**
 * \brief Represents a Cardano input to redeemer input_to_redeemer_map key value pair.
 */
typedef struct cardano_input_to_redeemer_map_kvp_t
{
    cardano_object_t             base;
    cardano_transaction_input_t* key;
    cardano_redeemer_t*          value;
} cardano_input_to_redeemer_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates the input_to_redeemer_map.
 *
 * This function is responsible for properly deallocating a input to redeemer input_to_redeemer_map (`cardano_input_to_redeemer_map_t`)
 * and its associated resources.
 *
 * \param input A void pointer to the input_to_redeemer_map input to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_input_to_redeemer_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the input_to_redeemer_map
 *       input reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these reward addresses.
 */
static void
cardano_input_to_redeemer_map_deallocate(void* input)
{
  assert(input != NULL);

  cardano_input_to_redeemer_map_t* input_to_redeemer_map = (cardano_input_to_redeemer_map_t*)input;

  if (input_to_redeemer_map->array != NULL)
  {
    cardano_array_unref(&input_to_redeemer_map->array);
  }

  _cardano_free(input_to_redeemer_map);
}

/**
 * \brief Deallocates a reward address to redeemer input_to_redeemer_map key value pair.
 *
 * This function is responsible for properly deallocating a input to redeemer input_to_redeemer_map key value pair (`cardano_input_to_redeemer_map_kvp_t`)
 * and its associated resources.
 *
 * \param input A void pointer to the input_to_redeemer_map input to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_input_to_redeemer_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the input_to_redeemer_map
 *       input reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_input_to_redeemer_map_kvp_deallocate(void* input)
{
  assert(input != NULL);

  cardano_input_to_redeemer_map_kvp_t* input_to_redeemer_map = (cardano_input_to_redeemer_map_kvp_t*)input;

  cardano_transaction_input_unref(&input_to_redeemer_map->key);
  cardano_redeemer_unref(&input_to_redeemer_map->value);

  _cardano_free(input_to_redeemer_map);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_input_to_redeemer_map_new(cardano_input_to_redeemer_map_t** input_to_redeemer_map)
{
  if (input_to_redeemer_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *input_to_redeemer_map = _cardano_malloc(sizeof(cardano_input_to_redeemer_map_t));

  if ((*input_to_redeemer_map) == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*input_to_redeemer_map)->base.ref_count     = 1;
  (*input_to_redeemer_map)->base.last_error[0] = '\0';
  (*input_to_redeemer_map)->base.deallocator   = cardano_input_to_redeemer_map_deallocate;

  (*input_to_redeemer_map)->array = cardano_array_new(32);

  if ((*input_to_redeemer_map)->array == NULL)
  {
    _cardano_free((*input_to_redeemer_map));
    *input_to_redeemer_map = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_input_to_redeemer_map_get_length(const cardano_input_to_redeemer_map_t* input_to_redeemer_map)
{
  if (input_to_redeemer_map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(input_to_redeemer_map->array);
}

cardano_error_t
cardano_input_to_redeemer_map_get(
  cardano_input_to_redeemer_map_t*   input_to_redeemer_map,
  const cardano_transaction_input_t* key,
  cardano_redeemer_t**               element)
{
  if (input_to_redeemer_map == NULL)
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

  for (size_t i = 0; i < cardano_array_get_size(input_to_redeemer_map->array); ++i)
  {
    cardano_object_t*                    object = cardano_array_get(input_to_redeemer_map->array, i);
    cardano_input_to_redeemer_map_kvp_t* kvp    = (cardano_input_to_redeemer_map_kvp_t*)((void*)object);

    if (kvp->key == key)
    {
      cardano_redeemer_ref(kvp->value);
      *element = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_input_to_redeemer_map_insert(
  cardano_input_to_redeemer_map_t* input_to_redeemer_map,
  cardano_transaction_input_t*     key,
  cardano_redeemer_t*              value)
{
  if (input_to_redeemer_map == NULL)
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

  cardano_input_to_redeemer_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_input_to_redeemer_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_input_to_redeemer_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_transaction_input_ref(key);
  cardano_redeemer_ref(value);

  const size_t old_size = cardano_array_get_size(input_to_redeemer_map->array);
  const size_t new_size = cardano_array_push(input_to_redeemer_map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_input_to_redeemer_map_get_key_at(
  const cardano_input_to_redeemer_map_t* input_to_redeemer_map,
  const size_t                           index,
  cardano_transaction_input_t**          input)
{
  if (input_to_redeemer_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(input_to_redeemer_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                    object = cardano_array_get(input_to_redeemer_map->array, index);
  cardano_input_to_redeemer_map_kvp_t* kvp    = (cardano_input_to_redeemer_map_kvp_t*)((void*)object);

  cardano_transaction_input_ref(kvp->key);
  cardano_object_unref(&object);

  *input = kvp->key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_input_to_redeemer_map_get_value_at(
  const cardano_input_to_redeemer_map_t* input_to_redeemer_map,
  size_t                                 index,
  cardano_redeemer_t**                   redeemer)
{
  if (input_to_redeemer_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(input_to_redeemer_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                    object = cardano_array_get(input_to_redeemer_map->array, index);
  cardano_input_to_redeemer_map_kvp_t* kvp    = (cardano_input_to_redeemer_map_kvp_t*)((void*)object);

  cardano_object_unref(&object);

  cardano_redeemer_ref(kvp->value);
  *redeemer = kvp->value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_input_to_redeemer_map_get_key_value_at(
  const cardano_input_to_redeemer_map_t* input_to_redeemer_map,
  size_t                                 index,
  cardano_transaction_input_t**          input,
  cardano_redeemer_t**                   redeemer)
{
  if (input_to_redeemer_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(input_to_redeemer_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                    object = cardano_array_get(input_to_redeemer_map->array, index);
  cardano_input_to_redeemer_map_kvp_t* kvp    = (cardano_input_to_redeemer_map_kvp_t*)((void*)object);

  cardano_transaction_input_ref(kvp->key);
  cardano_redeemer_ref(kvp->value);
  cardano_object_unref(&object);

  *input    = kvp->key;
  *redeemer = kvp->value;

  return CARDANO_SUCCESS;
}

void
cardano_input_to_redeemer_map_unref(cardano_input_to_redeemer_map_t** input_to_redeemer_map)
{
  if ((input_to_redeemer_map == NULL) || (*input_to_redeemer_map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*input_to_redeemer_map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *input_to_redeemer_map = NULL;
    return;
  }
}

cardano_error_t
cardano_input_to_redeemer_map_update_redeemer_index(
  cardano_input_to_redeemer_map_t* input_to_redeemer_map,
  cardano_transaction_input_t*     input,
  const uint64_t                   index)
{
  if (input_to_redeemer_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (input == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_redeemer_t* redeemer = NULL;
  cardano_error_t     result   = cardano_input_to_redeemer_map_get(input_to_redeemer_map, input, &redeemer);
  cardano_redeemer_unref(&redeemer);

  if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
  {
    return CARDANO_SUCCESS;
  }

  result = cardano_redeemer_set_index(redeemer, index);

  return result;
}

void
cardano_input_to_redeemer_map_ref(cardano_input_to_redeemer_map_t* input_to_redeemer_map)
{
  if (input_to_redeemer_map == NULL)
  {
    return;
  }

  cardano_object_ref(&input_to_redeemer_map->base);
}

size_t
cardano_input_to_redeemer_map_refcount(const cardano_input_to_redeemer_map_t* input_to_redeemer_map)
{
  if (input_to_redeemer_map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&input_to_redeemer_map->base);
}

void
cardano_input_to_redeemer_map_set_last_error(cardano_input_to_redeemer_map_t* input_to_redeemer_map, const char* message)
{
  cardano_object_set_last_error(&input_to_redeemer_map->base, message);
}

const char*
cardano_input_to_redeemer_map_get_last_error(const cardano_input_to_redeemer_map_t* input_to_redeemer_map)
{
  return cardano_object_get_last_error(&input_to_redeemer_map->base);
}
