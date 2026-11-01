/**
 * \file json_object_common.c
 *
 * \author angel.castillo
 * \date   Dec 08, 2024
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

#include <cardano/json/json_object.h>
#include <cardano/typedefs.h>

#include "../../allocators.h"
#include "../../collections/array.h"
#include "json_object_common.h"

#include <assert.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a JSON object and its associated resources.
 *
 * This function is responsible for properly releasing the memory and resources
 * associated with a \ref cardano_json_object_t. It ensures that all nested objects,
 * arrays, strings, and other dynamically allocated components are also cleaned up.
 *
 * \param[in] object A void pointer to the \ref cardano_json_object_t instance
 *                   to be deallocated. This must not be \c NULL.
 */
static void
cardano_json_object_deallocate(void* object)
{
  assert(object != NULL);

  cardano_json_object_t* json_object = (cardano_json_object_t*)object;

  cardano_buffer_unref(&json_object->string);
  cardano_array_unref(&json_object->pairs);
  cardano_array_unref(&json_object->array);

  if (json_object->json_string != NULL)
  {
    _cardano_free(json_object->json_string);
  }

  _cardano_free(json_object);
}

/**
 * \brief Deallocates a JSON key-value pair and its associated resources.
 *
 * This function is responsible for properly releasing the memory and resources
 * associated with a \ref cardano_json_kvp_t. It ensures that both the key and
 * the value components are appropriately cleaned up.
 *
 * \param[in] object A void pointer to the \ref cardano_json_kvp_t instance
 *                   to be deallocated. This must not be \c NULL.
 */
static void
cardano_json_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_json_kvp_t* map = (cardano_json_kvp_t*)object;

  cardano_buffer_unref(&map->key);
  cardano_json_object_unref(&map->value);

  _cardano_free(map);
}

/* FUNCTIONS *****************************************************************/

cardano_json_object_t*
cardano_json_object_new(void)
{
  cardano_json_object_t* object = (cardano_json_object_t*)_cardano_malloc(sizeof(cardano_json_object_t));

  if (object != NULL)
  {
    object->base.ref_count     = 1U;
    object->base.deallocator   = cardano_json_object_deallocate;
    object->type               = CARDANO_JSON_OBJECT_TYPE_NULL;
    object->pairs              = NULL;
    object->array              = NULL;
    object->string             = NULL;
    object->int_value          = 0;
    object->uint_value         = 0;
    object->double_value       = 0.0;
    object->bool_value         = false;
    object->is_real            = false;
    object->is_negative        = false;
    object->json_string        = NULL;
    object->json_string_length = 0U;
  }

  return object;
}

cardano_json_kvp_t*
cardano_json_kvp_new(void)
{
  cardano_json_kvp_t* kvp = (cardano_json_kvp_t*)_cardano_malloc(sizeof(cardano_json_kvp_t));

  if (kvp != NULL)
  {
    kvp->base.ref_count   = 1U;
    kvp->base.deallocator = cardano_json_kvp_deallocate;
    kvp->key              = NULL;
    kvp->value            = NULL;
  }

  return kvp;
}

/**
 * Deep copies a private map entry and its owned values.
 */
static cardano_error_t
cardano_json_kvp_deep_clone(const cardano_json_kvp_t* source, cardano_json_kvp_t** clone)
{
  if (clone == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *clone = NULL;

  if (source == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_kvp_t* copy = _cardano_malloc(sizeof(cardano_json_kvp_t));

  if (copy == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  copy->base.ref_count     = 1U;
  copy->base.deallocator   = source->base.deallocator;
  copy->base.last_error[0] = '\0';
  copy->key                = NULL;
  copy->value              = NULL;

  if (source->key != NULL)
  {
    const cardano_error_t result = cardano_buffer_deep_clone(source->key, &copy->key);

    if (result != CARDANO_SUCCESS)
    {
      copy->base.deallocator(copy);
      return result;
    }
  }

  if (source->value != NULL)
  {
    const cardano_error_t result = cardano_json_object_deep_clone(source->value, &copy->value);

    if (result != CARDANO_SUCCESS)
    {
      copy->base.deallocator(copy);
      return result;
    }
  }

  *clone = copy;

  return CARDANO_SUCCESS;
}

/**
 * Deep copies one element for the typed collection.
 */
static cardano_error_t
clone_json_pair(const cardano_object_t* source, cardano_object_t** clone)
{
  cardano_json_kvp_t*   copy   = NULL;
  const cardano_error_t result = cardano_json_kvp_deep_clone((const cardano_json_kvp_t*)((const void*)source), &copy);

  *clone = (cardano_object_t*)((void*)copy);

  return result;
}

/**
 * Deep copies one element for the typed collection.
 */
static cardano_error_t
clone_array_element(const cardano_object_t* source, cardano_object_t** clone)
{
  cardano_json_object_t* copy   = NULL;
  const cardano_error_t  result = cardano_json_object_deep_clone((const cardano_json_object_t*)((const void*)source), &copy);

  *clone = (cardano_object_t*)((void*)copy);

  return result;
}

cardano_error_t
cardano_json_object_deep_clone(const cardano_json_object_t* source, cardano_json_object_t** clone)
{
  if (clone == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *clone = NULL;

  if (source == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_object_t* copy = _cardano_malloc(sizeof(cardano_json_object_t));

  if (copy == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  copy->base.ref_count     = 1U;
  copy->base.deallocator   = source->base.deallocator;
  copy->base.last_error[0] = '\0';
  copy->type               = source->type;
  copy->pairs              = NULL;
  copy->array              = NULL;
  copy->string             = NULL;
  copy->is_real            = source->is_real;
  copy->is_negative        = source->is_negative;
  copy->int_value          = source->int_value;
  copy->uint_value         = source->uint_value;
  copy->double_value       = source->double_value;
  copy->bool_value         = source->bool_value;
  copy->json_string        = NULL;
  copy->json_string_length = source->json_string_length;

  if (source->pairs != NULL)
  {
    const cardano_error_t result = cardano_array_deep_clone(source->pairs, clone_json_pair, &copy->pairs);

    if (result != CARDANO_SUCCESS)
    {
      copy->base.deallocator(copy);
      return result;
    }
  }

  if (source->array != NULL)
  {
    const cardano_error_t result = cardano_array_deep_clone(source->array, clone_array_element, &copy->array);

    if (result != CARDANO_SUCCESS)
    {
      copy->base.deallocator(copy);
      return result;
    }
  }

  if (source->string != NULL)
  {
    const cardano_error_t result = cardano_buffer_deep_clone(source->string, &copy->string);

    if (result != CARDANO_SUCCESS)
    {
      copy->base.deallocator(copy);
      return result;
    }
  }

  if (source->json_string != NULL)
  {
    const size_t size = source->json_string_length;

    copy->json_string = _cardano_malloc((size > 0U) ? size : 1U);

    if (copy->json_string == NULL)
    {
      copy->base.deallocator(copy);
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    CARDANO_UNUSED(memcpy(copy->json_string, source->json_string, size));
  }

  *clone = copy;

  return CARDANO_SUCCESS;
}
