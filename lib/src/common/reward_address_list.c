/**
 * \file reward_address_list.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2024
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

#include <cardano/address/reward_address.h>
#include <cardano/common/reward_address_list.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano reward_address list.
 */
typedef struct cardano_reward_address_list_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_reward_address_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a reward_address list object.
 *
 * This function is responsible for properly deallocating a reward_address list object (`cardano_reward_address_list_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the reward_address_list object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_reward_address_list_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the reward_address_list
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_reward_address_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_reward_address_list_t* list = (cardano_reward_address_list_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/**
 * \brief Compares two cardano_object_t objects based on their reward_address.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context A pointer to a context object. This parameter is not used in this function.
 *
 * \return A negative value if the address of lhs is less than the address of rhs, zero if they are equal,
 *         and a positive value if the address of lhs is greater than the address of rhs.
 */
static int32_t
compare_by_bytes(const cardano_object_t* lhs, const cardano_object_t* rhs, void* context)
{
  assert(lhs != NULL);
  assert(rhs != NULL);

  CARDANO_UNUSED(context);

  const cardano_reward_address_t* lhs_address = (const cardano_reward_address_t*)((const void*)lhs);
  const cardano_reward_address_t* rhs_address = (const cardano_reward_address_t*)((const void*)rhs);

  const size_t lhs_size = cardano_reward_address_get_bytes_size(lhs_address);
  const size_t rhs_size = cardano_reward_address_get_bytes_size(rhs_address);

  if (lhs_size != rhs_size)
  {
    return (lhs_size < rhs_size) ? -1 : 1;
  }

  const uint8_t* lhs_bytes = cardano_reward_address_get_bytes(lhs_address);
  const uint8_t* rhs_bytes = cardano_reward_address_get_bytes(rhs_address);

  return memcmp(lhs_bytes, rhs_bytes, lhs_size);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_reward_address_list_new(cardano_reward_address_list_t** reward_address_list)
{
  if (reward_address_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_list_t* list = _cardano_malloc(sizeof(cardano_reward_address_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_reward_address_list_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *reward_address_list = list;

  return CARDANO_SUCCESS;
}

size_t
cardano_reward_address_list_get_length(const cardano_reward_address_list_t* reward_address_list)
{
  if (reward_address_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(reward_address_list->array);
}

cardano_error_t
cardano_reward_address_list_get(
  const cardano_reward_address_list_t* reward_address_list,
  size_t                               index,
  cardano_reward_address_t**           element)
{
  if (reward_address_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(reward_address_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_reward_address_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_reward_address_list_add(cardano_reward_address_list_t* reward_address_list, cardano_reward_address_t* element)
{
  if (reward_address_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(reward_address_list->array);
  const size_t new_size      = cardano_array_push(reward_address_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(reward_address_list->array, compare_by_bytes, NULL);

  return CARDANO_SUCCESS;
}

void
cardano_reward_address_list_unref(cardano_reward_address_list_t** reward_address_list)
{
  if ((reward_address_list == NULL) || (*reward_address_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*reward_address_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *reward_address_list = NULL;
    return;
  }
}

void
cardano_reward_address_list_ref(cardano_reward_address_list_t* reward_address_list)
{
  if (reward_address_list == NULL)
  {
    return;
  }

  cardano_object_ref(&reward_address_list->base);
}

size_t
cardano_reward_address_list_refcount(const cardano_reward_address_list_t* reward_address_list)
{
  if (reward_address_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&reward_address_list->base);
}

void
cardano_reward_address_list_set_last_error(cardano_reward_address_list_t* reward_address_list, const char* message)
{
  cardano_object_set_last_error(&reward_address_list->base, message);
}

const char*
cardano_reward_address_list_get_last_error(const cardano_reward_address_list_t* reward_address_list)
{
  return cardano_object_get_last_error(&reward_address_list->base);
}
