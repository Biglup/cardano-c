/**
 * \file voter_list.c
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

#include <cardano/object.h>
#include <cardano/voting_procedures/voter_list.h>

#include "../allocators.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano governance action id list.
 */
typedef struct cardano_voter_list_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_voter_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a governance action id list object.
 *
 * This function is responsible for properly deallocating a governance action id list object (`cardano_voter_list_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the voter_list object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_voter_list_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the voter_list
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_voter_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_voter_list_t* list = (cardano_voter_list_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_voter_list_new(cardano_voter_list_t** voter_list)
{
  if (voter_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voter_list_t* list = _cardano_malloc(sizeof(cardano_voter_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_voter_list_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *voter_list = list;

  return CARDANO_SUCCESS;
}

size_t
cardano_voter_list_get_length(const cardano_voter_list_t* voter_list)
{
  if (voter_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(voter_list->array);
}

cardano_error_t
cardano_voter_list_get(
  const cardano_voter_list_t* voter_list,
  size_t                      index,
  cardano_voter_t**           element)
{
  if (voter_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(voter_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_voter_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voter_list_add(cardano_voter_list_t* voter_list, cardano_voter_t* element)
{
  if (voter_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(voter_list->array);
  const size_t new_size      = cardano_array_push(voter_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

void
cardano_voter_list_unref(cardano_voter_list_t** voter_list)
{
  if ((voter_list == NULL) || (*voter_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*voter_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *voter_list = NULL;
    return;
  }
}

void
cardano_voter_list_ref(cardano_voter_list_t* voter_list)
{
  if (voter_list == NULL)
  {
    return;
  }

  cardano_object_ref(&voter_list->base);
}

size_t
cardano_voter_list_refcount(const cardano_voter_list_t* voter_list)
{
  if (voter_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&voter_list->base);
}

void
cardano_voter_list_set_last_error(cardano_voter_list_t* voter_list, const char* message)
{
  cardano_object_set_last_error(&voter_list->base, message);
}

const char*
cardano_voter_list_get_last_error(const cardano_voter_list_t* voter_list)
{
  return cardano_object_get_last_error(&voter_list->base);
}
