/**
 * \file map.c
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

#include "reward_address_to_redeemer_map.h"

#include <cardano/common/reward_address_list.h>

#include "../../collections/array.h"
#include <assert.h>
#include <cardano/address/reward_address.h>
#include <cardano/object.h>
#include <cardano/witness_set/redeemer.h>
#include <src/allocators.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano reward_address to redeemer map.
 */
typedef struct cardano_reward_address_to_redeemer_map_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_reward_address_to_redeemer_map_t;

/**
 * \brief Represents a Cardano reward_address to redeemer map key value pair.
 */
typedef struct cardano_reward_address_to_redeemer_map_kvp_t
{
    cardano_object_t          base;
    cardano_reward_address_t* key;
    cardano_redeemer_t*       value;
} cardano_reward_address_to_redeemer_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates the map.
 *
 * This function is responsible for properly deallocating a reward_address to redeemer map (`cardano_reward_address_to_redeemer_map_t`)
 * and its associated resources.
 *
 * \param reward_address A void pointer to the map reward_address to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_reward_address_to_redeemer_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the map
 *       reward_address reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these reward addresses.
 */
static void
cardano_reward_address_to_redeemer_map_deallocate(void* reward_address)
{
  assert(reward_address != NULL);

  cardano_reward_address_to_redeemer_map_t* map = (cardano_reward_address_to_redeemer_map_t*)reward_address;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a reward address to redeemer map key value pair.
 *
 * This function is responsible for properly deallocating a reward_address to redeemer map key value pair (`cardano_reward_address_to_redeemer_map_kvp_t`)
 * and its associated resources.
 *
 * \param reward_address A void pointer to the map reward_address to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_reward_address_to_redeemer_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the map
 *       reward_address reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_reward_address_to_redeemer_map_kvp_deallocate(void* reward_address)
{
  assert(reward_address != NULL);

  cardano_reward_address_to_redeemer_map_kvp_t* map = (cardano_reward_address_to_redeemer_map_kvp_t*)reward_address;

  cardano_reward_address_unref(&map->key);
  cardano_redeemer_unref(&map->value);

  _cardano_free(map);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_reward_address_to_redeemer_map_new(cardano_reward_address_to_redeemer_map_t** map)
{
  if (map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *map = _cardano_malloc(sizeof(cardano_reward_address_to_redeemer_map_t));

  if ((*map) == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*map)->base.ref_count     = 1;
  (*map)->base.last_error[0] = '\0';
  (*map)->base.deallocator   = cardano_reward_address_to_redeemer_map_deallocate;

  (*map)->array = cardano_array_new(32);

  if ((*map)->array == NULL)
  {
    _cardano_free((*map));
    *map = NULL;
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_reward_address_to_redeemer_map_get_length(const cardano_reward_address_to_redeemer_map_t* map)
{
  if (map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(map->array);
}

cardano_error_t
cardano_reward_address_to_redeemer_map_get(
  cardano_reward_address_to_redeemer_map_t* map,
  const cardano_reward_address_t*           key,
  cardano_redeemer_t**                      element)
{
  if (map == NULL)
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

  for (size_t i = 0; i < cardano_array_get_size(map->array); ++i)
  {
    cardano_object_t*                             object = cardano_array_get(map->array, i);
    cardano_reward_address_to_redeemer_map_kvp_t* kvp    = (cardano_reward_address_to_redeemer_map_kvp_t*)((void*)object);

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
cardano_reward_address_to_redeemer_map_insert(
  cardano_reward_address_to_redeemer_map_t* map,
  cardano_reward_address_t*                 key,
  cardano_redeemer_t*                       value)
{
  if (map == NULL)
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

  cardano_reward_address_to_redeemer_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_reward_address_to_redeemer_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_reward_address_to_redeemer_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_reward_address_ref(key);
  cardano_redeemer_ref(value);

  const size_t old_size = cardano_array_get_size(map->array);
  const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_reward_address_to_redeemer_map_get_key_at(
  const cardano_reward_address_to_redeemer_map_t* map,
  const size_t                                    index,
  cardano_reward_address_t**                      reward_address)
{
  if (map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                             object = cardano_array_get(map->array, index);
  cardano_reward_address_to_redeemer_map_kvp_t* kvp    = (cardano_reward_address_to_redeemer_map_kvp_t*)((void*)object);

  cardano_reward_address_ref(kvp->key);
  cardano_object_unref(&object);

  *reward_address = kvp->key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_reward_address_to_redeemer_map_get_value_at(
  const cardano_reward_address_to_redeemer_map_t* map,
  size_t                                          index,
  cardano_redeemer_t**                            redeemer)
{
  if (map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                             object = cardano_array_get(map->array, index);
  cardano_reward_address_to_redeemer_map_kvp_t* kvp    = (cardano_reward_address_to_redeemer_map_kvp_t*)((void*)object);

  cardano_object_unref(&object);

  cardano_redeemer_ref(kvp->value);
  *redeemer = kvp->value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_reward_address_to_redeemer_map_get_key_value_at(
  const cardano_reward_address_to_redeemer_map_t* map,
  size_t                                          index,
  cardano_reward_address_t**                      reward_address,
  cardano_redeemer_t**                            redeemer)
{
  if (map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                             object = cardano_array_get(map->array, index);
  cardano_reward_address_to_redeemer_map_kvp_t* kvp    = (cardano_reward_address_to_redeemer_map_kvp_t*)((void*)object);

  cardano_reward_address_ref(kvp->key);
  cardano_redeemer_ref(kvp->value);
  cardano_object_unref(&object);

  *reward_address = kvp->key;
  *redeemer       = kvp->value;

  return CARDANO_SUCCESS;
}

void
cardano_reward_address_to_redeemer_map_unref(cardano_reward_address_to_redeemer_map_t** map)
{
  if ((map == NULL) || (*map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *map = NULL;
    return;
  }
}

cardano_error_t
cardano_reward_address_to_redeemer_map_update_redeemer_index(
  cardano_reward_address_to_redeemer_map_t* map,
  cardano_reward_address_t*                 reward_address,
  const uint64_t                            index)
{
  if (map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_redeemer_t* redeemer = NULL;
  cardano_error_t     result   = cardano_reward_address_to_redeemer_map_get(map, reward_address, &redeemer);
  cardano_redeemer_unref(&redeemer);

  if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
  {
    return CARDANO_SUCCESS;
  }

  result = cardano_redeemer_set_index(redeemer, index);

  return result;
}

void
cardano_reward_address_to_redeemer_map_ref(cardano_reward_address_to_redeemer_map_t* map)
{
  if (map == NULL)
  {
    return;
  }

  cardano_object_ref(&map->base);
}

size_t
cardano_reward_address_to_redeemer_map_refcount(const cardano_reward_address_to_redeemer_map_t* map)
{
  if (map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&map->base);
}

void
cardano_reward_address_to_redeemer_map_set_last_error(cardano_reward_address_to_redeemer_map_t* map, const char* message)
{
  cardano_object_set_last_error(&map->base, message);
}

const char*
cardano_reward_address_to_redeemer_map_get_last_error(const cardano_reward_address_to_redeemer_map_t* map)
{
  return cardano_object_get_last_error(&map->base);
}
