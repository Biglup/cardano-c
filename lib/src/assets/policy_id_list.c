/**
 * \file policy_id_list.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license");
 * you may not use this file except in compliance with the license.
 * you may obtain a copy of the license at
 *
 *     www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */

/* INCLUDES ******************************************************************/

#include <cardano/assets/policy_id_list.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano asset name list.
 */
typedef struct cardano_policy_id_list_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_policy_id_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a asset name object.
 *
 * This function is responsible for properly deallocating a asset name object (`cardano_blake2b_hash_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the policy_id object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_blake2b_hash_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the policy_id
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_policy_id_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_policy_id_list_t* list = (cardano_policy_id_list_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_policy_id_list_new(cardano_policy_id_list_t** policy_id)
{
  if (policy_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_policy_id_list_t* list = _cardano_malloc(sizeof(cardano_policy_id_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_policy_id_list_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *policy_id = list;

  return CARDANO_SUCCESS;
}

size_t
cardano_policy_id_list_get_length(const cardano_policy_id_list_t* policy_id_list)
{
  if (policy_id_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(policy_id_list->array);
}

cardano_error_t
cardano_policy_id_list_get(
  const cardano_policy_id_list_t* policy_id_list,
  size_t                          index,
  cardano_blake2b_hash_t**        element)
{
  if (policy_id_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(policy_id_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_blake2b_hash_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_policy_id_list_add(cardano_policy_id_list_t* policy_id_list, cardano_blake2b_hash_t* element)
{
  if (policy_id_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(policy_id_list->array);
  const size_t new_size      = cardano_array_push(policy_id_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

void
cardano_policy_id_list_unref(cardano_policy_id_list_t** policy_id_list)
{
  if ((policy_id_list == NULL) || (*policy_id_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*policy_id_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *policy_id_list = NULL;
    return;
  }
}

void
cardano_policy_id_list_ref(cardano_policy_id_list_t* policy_id_list)
{
  if (policy_id_list == NULL)
  {
    return;
  }

  cardano_object_ref(&policy_id_list->base);
}

size_t
cardano_policy_id_list_refcount(const cardano_policy_id_list_t* policy_id_list)
{
  if (policy_id_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&policy_id_list->base);
}

void
cardano_policy_id_list_set_last_error(cardano_policy_id_list_t* policy_id_list, const char* message)
{
  cardano_object_set_last_error(&policy_id_list->base, message);
}

const char*
cardano_policy_id_list_get_last_error(const cardano_policy_id_list_t* policy_id_list)
{
  return cardano_object_get_last_error(&policy_id_list->base);
}
