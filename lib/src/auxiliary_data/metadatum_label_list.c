/**
 * \file metadatum_label_list.c
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

#include <cardano/auxiliary_data/metadatum_label_list.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano metadatum_label list.
 */
typedef struct cardano_metadatum_label_list_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_metadatum_label_list_t;

/**
 * \brief Represents a Cardano metadatum_label.
 */
typedef struct cardano_metadatum_label_t
{
    cardano_object_t base;
    uint64_t         value;
} cardano_metadatum_label_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a metadatum_label list object.
 *
 * This function is responsible for properly deallocating a metadatum_label list object (`cardano_metadatum_label_list_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the metadatum_label_list object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_metadatum_label_list_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the metadatum_label_list
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_metadatum_label_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_metadatum_label_list_t* list = (cardano_metadatum_label_list_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/**
 * \brief Compares two cardano_object_t objects based on their metadatum_label.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context Unused.
 *
 * \return A negative value if the address of lhs is less than the address of rhs, zero if they are equal,
 *         and a positive value if the address of lhs is greater than the address of rhs.
 */
static int32_t
compare_by_value(const cardano_object_t* lhs, const cardano_object_t* rhs, void* context)
{
  assert(lhs != NULL);
  assert(rhs != NULL);

  CARDANO_UNUSED(context);

  const cardano_metadatum_label_t* lhs_label = (const cardano_metadatum_label_t*)((const void*)lhs);
  const cardano_metadatum_label_t* rhs_label = (const cardano_metadatum_label_t*)((const void*)rhs);

  return (lhs_label->value < rhs_label->value) ? -1 : 1;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_metadatum_label_list_new(cardano_metadatum_label_list_t** metadatum_label_list)
{
  if (metadatum_label_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_label_list_t* list = _cardano_malloc(sizeof(cardano_metadatum_label_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_metadatum_label_list_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *metadatum_label_list = list;

  return CARDANO_SUCCESS;
}

size_t
cardano_metadatum_label_list_get_length(const cardano_metadatum_label_list_t* metadatum_label_list)
{
  if (metadatum_label_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(metadatum_label_list->array);
}

cardano_error_t
cardano_metadatum_label_list_get(
  const cardano_metadatum_label_list_t* metadatum_label_list,
  size_t                                index,
  uint64_t*                             element)
{
  if (metadatum_label_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(metadatum_label_list->array, index);
  cardano_object_unref(&object);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_metadatum_label_t* elem = (cardano_metadatum_label_t*)((void*)object);

  *element = elem->value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_metadatum_label_list_add(cardano_metadatum_label_list_t* metadatum_label_list, const uint64_t element)
{
  if (metadatum_label_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_label_t* metadatum_label = _cardano_malloc(sizeof(cardano_metadatum_label_t));

  if (metadatum_label == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  metadatum_label->base.ref_count     = 0;
  metadatum_label->base.last_error[0] = '\0';
  metadatum_label->base.deallocator   = _cardano_free;
  metadatum_label->value              = element;

  const size_t original_size = cardano_array_get_size(metadatum_label_list->array);
  const size_t new_size      = cardano_array_push(metadatum_label_list->array, (cardano_object_t*)((void*)metadatum_label));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(metadatum_label_list->array, compare_by_value, NULL);

  return CARDANO_SUCCESS;
}

void
cardano_metadatum_label_list_unref(cardano_metadatum_label_list_t** metadatum_label_list)
{
  if ((metadatum_label_list == NULL) || (*metadatum_label_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*metadatum_label_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *metadatum_label_list = NULL;
    return;
  }
}

void
cardano_metadatum_label_list_ref(cardano_metadatum_label_list_t* metadatum_label_list)
{
  if (metadatum_label_list == NULL)
  {
    return;
  }

  cardano_object_ref(&metadatum_label_list->base);
}

size_t
cardano_metadatum_label_list_refcount(const cardano_metadatum_label_list_t* metadatum_label_list)
{
  if (metadatum_label_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&metadatum_label_list->base);
}

void
cardano_metadatum_label_list_set_last_error(cardano_metadatum_label_list_t* metadatum_label_list, const char* message)
{
  cardano_object_set_last_error(&metadatum_label_list->base, message);
}

const char*
cardano_metadatum_label_list_get_last_error(const cardano_metadatum_label_list_t* metadatum_label_list)
{
  return cardano_object_get_last_error(&metadatum_label_list->base);
}
