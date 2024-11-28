/**
 * \file voting_procedure_map.c
 *
 * \author angel.castillo
 * \date   Aug 08, 2024
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

#include <cardano/common/governance_action_id.h>
#include <cardano/object.h>
#include <cardano/voting_procedures/governance_action_id_list.h>
#include <cardano/voting_procedures/voting_procedure.h>
#include <cardano/voting_procedures/voting_procedure_list.h>

#include "../allocators.h"
#include "../collections/array.h"
#include "voting_procedure_map.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano voting_procedure map.
 */
typedef struct cardano_voting_procedure_map_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_voting_procedure_map_t;

/**
 * \brief Represents a Cardano voting_procedure map key value pair.
 */
typedef struct cardano_voting_procedure_map_kvp_t
{
    cardano_object_t                base;
    cardano_governance_action_id_t* key;
    cardano_voting_procedure_t*     value;
} cardano_voting_procedure_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a voting_procedure map object.
 *
 * This function is responsible for properly deallocating a voting_procedure map object (`cardano_voting_procedure_map_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the voting_procedure_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_voting_procedure_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the voting_procedure_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_voting_procedure_map_deallocate(void* object)
{
  assert(object != NULL);

  cardano_voting_procedure_map_t* map = (cardano_voting_procedure_map_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a voting_procedure map key value pair object.
 *
 * This function is responsible for properly deallocating a voting_procedure map key value pair object (`cardano_voting_procedure_map_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the voting_procedure_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_voting_procedure_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the voting_procedure_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_voting_procedure_map_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_voting_procedure_map_kvp_t* map = (cardano_voting_procedure_map_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_governance_action_id_unref(&map->key);
  }

  if (map->value != NULL)
  {
    cardano_voting_procedure_unref(&map->value);
  }

  _cardano_free(map);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_voting_procedure_map_new(cardano_voting_procedure_map_t** voting_procedure_map)
{
  if (voting_procedure_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voting_procedure_map_t* map = _cardano_malloc(sizeof(cardano_voting_procedure_map_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_voting_procedure_map_deallocate;

  map->array = cardano_array_new(128);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *voting_procedure_map = map;

  return CARDANO_SUCCESS;
}

size_t
cardano_voting_procedure_map_get_length(const cardano_voting_procedure_map_t* voting_procedure_map)
{
  if (voting_procedure_map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(voting_procedure_map->array);
}

cardano_error_t
cardano_voting_procedure_map_get(
  cardano_voting_procedure_map_t* voting_procedure_map,
  cardano_governance_action_id_t* key,
  cardano_voting_procedure_t**    element)
{
  if (voting_procedure_map == NULL)
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

  for (size_t i = 0; i < cardano_array_get_size(voting_procedure_map->array); ++i)
  {
    cardano_object_t*                   object = cardano_array_get(voting_procedure_map->array, i);
    cardano_voting_procedure_map_kvp_t* kvp    = (cardano_voting_procedure_map_kvp_t*)((void*)object);

    if (cardano_governance_action_id_equals(kvp->key, key))
    {
      cardano_voting_procedure_ref(kvp->value);
      *element = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_voting_procedure_map_insert(
  cardano_voting_procedure_map_t* voting_procedure_map,
  cardano_governance_action_id_t* key,
  cardano_voting_procedure_t*     value)
{
  if (voting_procedure_map == NULL)
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

  cardano_voting_procedure_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_voting_procedure_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_voting_procedure_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_governance_action_id_ref(key);
  cardano_voting_procedure_ref(value);

  const size_t old_size = cardano_array_get_size(voting_procedure_map->array);
  const size_t new_size = cardano_array_push(voting_procedure_map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voting_procedure_map_get_keys(
  cardano_voting_procedure_map_t*       voting_procedure_map,
  cardano_governance_action_id_list_t** keys)
{
  if (voting_procedure_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_governance_action_id_list_t* list = NULL;

  cardano_error_t result = cardano_governance_action_id_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(voting_procedure_map->array); ++i)
  {
    cardano_object_t*                   object = cardano_array_get(voting_procedure_map->array, i);
    cardano_voting_procedure_map_kvp_t* kvp    = (cardano_voting_procedure_map_kvp_t*)((void*)object);

    result = cardano_governance_action_id_list_add(list, kvp->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_governance_action_id_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voting_procedure_map_get_values(
  cardano_voting_procedure_map_t*   voting_procedure_map,
  cardano_voting_procedure_list_t** values)
{
  if (voting_procedure_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (values == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voting_procedure_list_t* list = NULL;

  cardano_error_t result = cardano_voting_procedure_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(voting_procedure_map->array); ++i)
  {
    cardano_object_t*                   object = cardano_array_get(voting_procedure_map->array, i);
    cardano_voting_procedure_map_kvp_t* kvp    = (cardano_voting_procedure_map_kvp_t*)((void*)object);

    result = cardano_voting_procedure_list_add(list, kvp->value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_voting_procedure_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *values = list;

  return CARDANO_SUCCESS;
}

void
cardano_voting_procedure_map_unref(cardano_voting_procedure_map_t** voting_procedure_map)
{
  if ((voting_procedure_map == NULL) || (*voting_procedure_map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*voting_procedure_map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *voting_procedure_map = NULL;
    return;
  }
}

void
cardano_voting_procedure_map_ref(cardano_voting_procedure_map_t* voting_procedure_map)
{
  if (voting_procedure_map == NULL)
  {
    return;
  }

  cardano_object_ref(&voting_procedure_map->base);
}

size_t
cardano_voting_procedure_map_refcount(const cardano_voting_procedure_map_t* voting_procedure_map)
{
  if (voting_procedure_map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&voting_procedure_map->base);
}

void
cardano_voting_procedure_map_set_last_error(cardano_voting_procedure_map_t* voting_procedure_map, const char* message)
{
  cardano_object_set_last_error(&voting_procedure_map->base, message);
}

const char*
cardano_voting_procedure_map_get_last_error(const cardano_voting_procedure_map_t* voting_procedure_map)
{
  return cardano_object_get_last_error(&voting_procedure_map->base);
}
