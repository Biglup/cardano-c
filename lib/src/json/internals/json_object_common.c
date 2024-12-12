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