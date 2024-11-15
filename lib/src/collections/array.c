/**
 * \file array.c
 *
 * \author luisd.bianchi
 * \date   Mar 04, 2024
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

#include "array.h"

#include <cardano/object.h>

#include "../allocators.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"

/* STRUCTS *******************************************************************/

/**
 * \brief Represents a dynamic array of cardano_object_t pointers.
 *
 * This structure models a dynamic array that holds pointers to \ref cardano_object_t instances.
 * It supports operations such as insertion, deletion, and iteration over the contained objects.
 * The array automatically resizes to accommodate new elements.
 */
typedef struct cardano_array_t
{
    cardano_object_t   base;
    cardano_object_t** items;
    size_t             size;
    size_t             head;
    size_t             capacity;
    size_t             ref_count;
} cardano_array_t;

/* STATIC FUNCTIONS ***********************************************************/

/**
 * \brief Grows the array if there is not enough capacity to hold the new item.
 *
 * \param array The array to grow.
 *
 * \return A \c cardano_error_t indicating the result of the operation: \c CARDANO_SUCCESS on success,
 *         or an appropriate error code indicating the failure reason. Refer to \c cardano_error_t documentation
 *         for details on possible error codes.
 */
static cardano_error_t
grow_array_if_needed(cardano_array_t* array)
{
  assert(array != NULL);

  if ((array->size + 1U) >= array->capacity)
  {
    size_t             new_capacity = (size_t)ceil((float)array->capacity * (float)LIB_CARDANO_C_COLLECTION_GROW_FACTOR);
    cardano_object_t** new_items    = (cardano_object_t**)_cardano_realloc(array->items, new_capacity * sizeof(cardano_object_t*));

    if (new_items == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    array->items    = new_items;
    array->capacity = new_capacity;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Deallocates a array object.
 *
 * This function is responsible for properly deallocating a array object (`cardano_array_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the array object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_array_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the array
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_array_deallocate(void* object)
{
  assert(object != NULL);

  cardano_array_t* array = (cardano_array_t*)object;

  for (size_t i = 0; i < array->size; ++i)
  {
    cardano_object_unref(&array->items[i]);
  }

  if (array->items != NULL)
  {
    _cardano_free(array->items);
    array->items = NULL;
  }

  _cardano_free(array);
}

/**
 * \brief Sorts an array of cardano_object_t* pointers using the insertion sort algorithm.
 *
 * This function iterates through the array, sequentially placing each element in its correct
 * position in the sorted portion of the array. The sort is stable and operates in-place,
 * requiring no additional memory allocation. It is well-suited for small to medium-sized arrays.
 *
 * \param array A pointer to the first element of an array of cardano_object_t* pointers that will be sorted.
 *              The array must be valid and initialized before calling this function.
 * \param size The number of elements in the array. Must be greater than 0 for the function to perform any action.
 * \param compare A function pointer to a comparison function that takes two const cardano_object_t* arguments
 *                (left-hand side and right-hand side) and returns an int. The function should return a value
 *                less than 0 if the first argument is considered to go before the second, 0 if they are considered
 *                equivalent, and a value greater than 0 if the first argument is considered to go after the second.
 *                This comparison function defines the ordering of the elements in the sorted array.
 *  \param context A pointer to an optional context object that can be passed to the comparison function.
 */
static void
insertion_sort(cardano_object_t** array, const size_t size, const cardano_array_compare_item_t compare, void* context)
{
  assert(array != NULL);
  assert(compare != NULL);

  cardano_object_t* key = NULL;

  for (size_t i = 1u; i < size; ++i)
  {
    key      = array[i];
    size_t j = i;

    while ((j > 0u) && (compare(array[j - 1u], key, context) > 0))
    {
      array[j] = array[j - 1u];
      --j;
    }

    array[j] = key;
  }
}

/* DEFINITIONS ****************************************************************/

cardano_array_t*
cardano_array_new(const size_t capacity)
{
  cardano_array_t* array = (cardano_array_t*)_cardano_malloc(sizeof(cardano_array_t));

  if (array == NULL)
  {
    return NULL;
  }

  array->items = (cardano_object_t**)_cardano_malloc(capacity * sizeof(cardano_object_t*));

  if (array->items == NULL)
  {
    _cardano_free(array);
    return NULL;
  }

  array->size               = 0;
  array->head               = 0;
  array->capacity           = capacity;
  array->base.ref_count     = 1;
  array->base.last_error[0] = '\0';
  array->base.deallocator   = cardano_array_deallocate;

  return array;
}

cardano_array_t*
cardano_array_concat(const cardano_array_t* lhs, const cardano_array_t* rhs)
{
  if (lhs == NULL)
  {
    return NULL;
  }

  if (rhs == NULL)
  {
    return NULL;
  }

  cardano_array_t* array = (cardano_array_t*)_cardano_malloc(sizeof(cardano_array_t));

  if (array == NULL)
  {
    return NULL;
  }

  array->items = (cardano_object_t**)_cardano_malloc((lhs->size + rhs->size) * sizeof(cardano_object_t*));

  if (array->items == NULL)
  {
    _cardano_free(array);
    return NULL;
  }

  const size_t lhs_size = lhs->size;
  const size_t rhs_size = rhs->size;

  for (size_t i = 0; i < lhs_size; ++i)
  {
    cardano_object_t* item = lhs->items[i];
    cardano_object_ref(item);
    array->items[i] = item;
  }

  for (size_t i = 0; i < rhs_size; ++i)
  {
    cardano_object_t* item = rhs->items[i];
    cardano_object_ref(item);
    array->items[lhs_size + i] = item;
  }

  array->size               = lhs->size + rhs->size;
  array->head               = 0;
  array->capacity           = array->size;
  array->base.ref_count     = 1;
  array->base.last_error[0] = '\0';
  array->base.deallocator   = cardano_array_deallocate;

  return array;
}

cardano_array_t*
cardano_array_slice(const cardano_array_t* array, size_t start, size_t end)
{
  if (array == NULL)
  {
    return NULL;
  }

  if (start > array->size)
  {
    return NULL;
  }

  if (end > array->size)
  {
    return NULL;
  }

  if (end < start)
  {
    return NULL;
  }

  size_t slice_size = end - start;

  if (slice_size == 0U)
  {
    return NULL;
  }

  cardano_object_t** slice_items = (cardano_object_t**)_cardano_malloc(slice_size * sizeof(cardano_object_t*));

  if (slice_items == NULL)
  {
    return NULL;
  }

  assert(array->items != NULL);

  for (size_t i = 0; i < slice_size; ++i)
  {
    cardano_object_t* item = array->items[start + i];
    cardano_object_ref(item);
    slice_items[i] = item;
  }

  cardano_array_t* sliced_array = (cardano_array_t*)_cardano_malloc(sizeof(cardano_array_t));

  if (sliced_array == NULL)
  {
    for (size_t i = 0; i < slice_size; ++i)
    {
      cardano_object_unref(&slice_items[i]);
    }

    _cardano_free(slice_items);
    return NULL;
  }

  sliced_array->items              = slice_items;
  sliced_array->size               = slice_size;
  sliced_array->head               = 0;
  sliced_array->capacity           = sliced_array->size;
  sliced_array->base.ref_count     = 1;
  sliced_array->base.last_error[0] = '\0';
  sliced_array->base.deallocator   = cardano_array_deallocate;

  return sliced_array;
}

cardano_array_t*
cardano_array_erase(
  cardano_array_t* array,
  const int64_t    start,
  const size_t     delete_count)
{
  if (array == NULL)
  {
    return NULL;
  }

  size_t  array_size     = array->size;
  int64_t adjusted_start = start;

  if (adjusted_start < 0)
  {
    adjusted_start += (int64_t)array_size;
  }

  if ((adjusted_start < 0) || ((size_t)adjusted_start >= array_size))
  {
    return NULL;
  }

  size_t adjusted_start_sz     = (size_t)adjusted_start;
  size_t adjusted_delete_count = delete_count;

  if ((adjusted_start_sz > (SIZE_MAX - adjusted_delete_count)) || ((adjusted_start_sz + adjusted_delete_count) > array_size))
  {
    return NULL;
  }

  if (adjusted_delete_count == 0U)
  {
    return cardano_array_new(1);
  }

  cardano_array_t* deleted_array = cardano_array_new(adjusted_delete_count);

  if (deleted_array == NULL)
  {
    return NULL;
  }

  for (size_t i = 0; i < adjusted_delete_count; ++i)
  {
    size_t idx              = adjusted_start_sz + i;
    deleted_array->items[i] = array->items[idx];
  }

  deleted_array->size = adjusted_delete_count;

  size_t elements_after_deleted = array_size - (adjusted_start_sz + adjusted_delete_count);

  for (size_t i = 0; i < elements_after_deleted; ++i)
  {
    array->items[adjusted_start_sz + i] = array->items[adjusted_start_sz + adjusted_delete_count + i];
  }

  for (size_t i = array_size - adjusted_delete_count; i < array_size; ++i)
  {
    array->items[i] = NULL;
  }

  array->size -= adjusted_delete_count;

  return deleted_array;
}

void
cardano_array_unref(cardano_array_t** array)
{
  if ((array == NULL) || (*array == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*array)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *array = NULL;
    return;
  }
}

void
cardano_array_ref(cardano_array_t* array)
{
  if (array == NULL)
  {
    return;
  }

  cardano_object_ref(&array->base);
}

size_t
cardano_array_refcount(const cardano_array_t* array)
{
  if (array == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&array->base);
}

cardano_object_t*
cardano_array_get(const cardano_array_t* array, const size_t index)
{
  if (array == NULL)
  {
    return NULL;
  }

  if (index >= array->size)
  {
    return NULL;
  }

  cardano_object_t* item = array->items[index];

  cardano_object_ref(item);

  return item;
}

size_t
cardano_array_push(cardano_array_t* array, cardano_object_t* item)
{
  if (array == NULL)
  {
    return 0U;
  }

  if (item == NULL)
  {
    return array->size;
  }

  cardano_error_t error = grow_array_if_needed(array);

  if (error != CARDANO_SUCCESS)
  {
    cardano_array_set_last_error(array, cardano_error_to_string(error));
    return array->size;
  }

  cardano_object_ref(item);
  array->items[array->size] = item;
  ++array->size;

  return array->size;
}

cardano_object_t*
cardano_array_pop(cardano_array_t* array)
{
  if (array == NULL)
  {
    return NULL;
  }

  if (array->size == 0U)
  {
    return NULL;
  }

  cardano_object_t* item    = array->items[array->size - 1U];
  array->items[array->size] = NULL;

  --array->size;

  return item;
}

size_t
cardano_array_get_size(const cardano_array_t* array)
{
  if (array == NULL)
  {
    return 0;
  }

  return array->size;
}

size_t
cardano_array_get_capacity(const cardano_array_t* array)
{
  if (array == NULL)
  {
    return 0;
  }

  return array->capacity;
}

void
cardano_array_clear(cardano_array_t* array)
{
  if (array == NULL)
  {
    return;
  }

  for (size_t i = 0; i < array->size; ++i)
  {
    cardano_object_unref(&array->items[i]);
  }

  array->size = 0;
}

void
cardano_array_sort(cardano_array_t* array, cardano_array_compare_item_t compare, void* context)
{
  if (array == NULL)
  {
    return;
  }

  if (compare == NULL)
  {
    return;
  }

  insertion_sort(array->items, array->size, compare, context);
}

cardano_object_t*
cardano_array_find(const cardano_array_t* array, cardano_array_unary_predicate_t predicate, const void* context)
{
  if (array == NULL)
  {
    return NULL;
  }

  if (predicate == NULL)
  {
    return NULL;
  }

  for (size_t i = 0; i < array->size; ++i)
  {
    cardano_object_t* item = array->items[i];

    if (predicate(item, context))
    {
      cardano_object_ref(item);
      return item;
    }
  }

  return NULL;
}

cardano_array_t*
cardano_array_filter(const cardano_array_t* array, cardano_array_unary_predicate_t predicate, const void* context)
{
  if (array == NULL)
  {
    return NULL;
  }

  if (predicate == NULL)
  {
    return NULL;
  }

  cardano_array_t* filtered_array = cardano_array_new(array->size);

  if (filtered_array == NULL)
  {
    return NULL;
  }

  for (size_t i = 0; i < array->size; ++i)
  {
    cardano_object_t* item = array->items[i];

    if (predicate(item, context))
    {
      size_t new_size = cardano_array_push(filtered_array, item);
      CARDANO_UNUSED(new_size);
    }
  }

  return filtered_array;
}

void
cardano_array_set_last_error(cardano_array_t* array, const char* message)
{
  cardano_object_set_last_error(&array->base, message);
}

const char*
cardano_array_get_last_error(const cardano_array_t* array)
{
  return cardano_object_get_last_error(&array->base);
}
