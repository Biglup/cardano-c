/**
 * \file utxo_list.c
 *
 * \author angel.castillo
 * \date   Sep 25, 2024
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

#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano utxo list.
 */
typedef struct cardano_utxo_list_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_utxo_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a utxo list object.
 *
 * This function is responsible for properly deallocating a utxo list object (`cardano_utxo_list_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the utxo_list object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_utxo_list_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the utxo_list
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_utxo_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_utxo_list_t* list = (cardano_utxo_list_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_utxo_list_new(cardano_utxo_list_t** utxo_list)
{
  if (utxo_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_utxo_list_t* list = _cardano_malloc(sizeof(cardano_utxo_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_utxo_list_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *utxo_list = list;

  return CARDANO_SUCCESS;
}

size_t
cardano_utxo_list_get_length(const cardano_utxo_list_t* utxo_list)
{
  if (utxo_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(utxo_list->array);
}

cardano_error_t
cardano_utxo_list_get(
  const cardano_utxo_list_t* utxo_list,
  size_t                     index,
  cardano_utxo_t**           element)
{
  if (utxo_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(utxo_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_utxo_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_utxo_list_add(cardano_utxo_list_t* utxo_list, cardano_utxo_t* element)
{
  if (utxo_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(utxo_list->array);

  // cppcheck-suppress misra-c2012-11.1; Reason: We need this so we can have typesafe parameters.
  const size_t new_size = cardano_array_push(utxo_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

void
cardano_utxo_list_clear(cardano_utxo_list_t* utxo_list)
{
  if (utxo_list == NULL)
  {
    return;
  }

  cardano_array_clear(utxo_list->array);
}

void
cardano_utxo_list_sort(cardano_utxo_list_t* utxo_list, cardano_utxo_list_compare_item_t compare, void* context)
{
  if (utxo_list == NULL)
  {
    return;
  }

  if (compare == NULL)
  {
    return;
  }

  // cppcheck-suppress misra-c2012-11.1; Reason: We need this so we can have typesafe parameters.
  cardano_array_sort(utxo_list->array, (cardano_array_compare_item_t)(void*)compare, context);
}

cardano_utxo_t*
cardano_utxo_list_find(const cardano_utxo_list_t* utxo_list, cardano_utxo_list_unary_predicate_t predicate, const void* context)
{
  if (utxo_list == NULL)
  {
    return NULL;
  }

  // cppcheck-suppress misra-c2012-11.1; Reason: We need this so we can have typesafe parameters.
  return (cardano_utxo_t*)(void*)cardano_array_find(utxo_list->array, (cardano_array_unary_predicate_t)predicate, context);
}

cardano_utxo_list_t*
cardano_utxo_list_filter(const cardano_utxo_list_t* utxo_list, cardano_utxo_list_unary_predicate_t predicate, const void* context)
{
  if (utxo_list == NULL)
  {
    return NULL;
  }

  cardano_utxo_list_t* result = NULL;

  cardano_error_t error = cardano_utxo_list_new(&result);

  if (error != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_array_unref(&result->array);
  result->array = NULL;

  // cppcheck-suppress misra-c2012-11.1; Reason: We need this so we can have typesafe parameters.
  result->array = cardano_array_filter(utxo_list->array, (cardano_array_unary_predicate_t)(void*)predicate, context);

  if (result->array == NULL)
  {
    cardano_utxo_list_unref(&result);
    return NULL;
  }

  return result;
}

cardano_utxo_list_t*
cardano_utxo_list_concat(const cardano_utxo_list_t* lhs, const cardano_utxo_list_t* rhs)
{
  if ((lhs == NULL) || (rhs == NULL))
  {
    return NULL;
  }

  cardano_utxo_list_t* result = NULL;

  cardano_error_t error = cardano_utxo_list_new(&result);

  if (error != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_array_unref(&result->array);
  result->array = NULL;
  result->array = cardano_array_concat(lhs->array, rhs->array);

  if (result->array == NULL)
  {
    cardano_utxo_list_unref(&result);
    return NULL;
  }

  return result;
}

cardano_utxo_list_t*
cardano_utxo_list_slice(const cardano_utxo_list_t* utxo_list, size_t start, size_t end)
{
  if (utxo_list == NULL)
  {
    return NULL;
  }

  cardano_utxo_list_t* result = NULL;

  cardano_error_t error = cardano_utxo_list_new(&result);

  if (error != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_array_unref(&result->array);
  result->array = NULL;
  result->array = cardano_array_slice(utxo_list->array, start, end);

  if (result->array == NULL)
  {
    cardano_utxo_list_unref(&result);
    return NULL;
  }

  return result;
}

cardano_utxo_list_t*
cardano_utxo_list_erase(
  cardano_utxo_list_t* utxo_list,
  int64_t              start,
  size_t               delete_count)
{
  if (utxo_list == NULL)
  {
    return NULL;
  }

  cardano_utxo_list_t* result = NULL;

  cardano_error_t error = cardano_utxo_list_new(&result);

  if (error != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_array_unref(&result->array);
  result->array = NULL;
  result->array = cardano_array_erase(utxo_list->array, start, delete_count);

  if (result->array == NULL)
  {
    cardano_utxo_list_unref(&result);
    return NULL;
  }

  return result;
}

cardano_error_t
cardano_utxo_list_remove(cardano_utxo_list_t* utxo_list, cardano_utxo_t* element)
{
  int32_t found_at = -1;

  const size_t length = cardano_utxo_list_get_length(utxo_list);

  for (size_t i = 0; i < length; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    cardano_error_t result = cardano_utxo_list_get(utxo_list, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (cardano_utxo_equals(element, utxo) == true)
    {
      found_at = (int32_t)i;
      break;
    }
  }

  if (found_at >= 0)
  {
    cardano_utxo_list_t* erased = cardano_utxo_list_erase(utxo_list, found_at, 1);

    if (erased == NULL)
    {
      cardano_utxo_list_unref(&erased);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    cardano_utxo_list_unref(&erased);
  }

  return CARDANO_SUCCESS;
}

cardano_utxo_list_t*
cardano_utxo_list_clone(cardano_utxo_list_t* utxo_list)
{
  if (utxo_list == NULL)
  {
    return NULL;
  }

  return cardano_utxo_list_slice(utxo_list, 0, cardano_utxo_list_get_length(utxo_list));
}

void
cardano_utxo_list_unref(cardano_utxo_list_t** utxo_list)
{
  if ((utxo_list == NULL) || (*utxo_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*utxo_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *utxo_list = NULL;
    return;
  }
}

void
cardano_utxo_list_ref(cardano_utxo_list_t* utxo_list)
{
  if (utxo_list == NULL)
  {
    return;
  }

  cardano_object_ref(&utxo_list->base);
}

size_t
cardano_utxo_list_refcount(const cardano_utxo_list_t* utxo_list)
{
  if (utxo_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&utxo_list->base);
}

void
cardano_utxo_list_set_last_error(cardano_utxo_list_t* utxo_list, const char* message)
{
  cardano_object_set_last_error(&utxo_list->base, message);
}

const char*
cardano_utxo_list_get_last_error(const cardano_utxo_list_t* utxo_list)
{
  return cardano_object_get_last_error(&utxo_list->base);
}
