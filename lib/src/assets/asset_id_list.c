/**
 * \file asset_id_list.c
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

#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_id_list.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano asset id list.
 */
typedef struct cardano_asset_id_list_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_asset_id_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a asset id object.
 *
 * This function is responsible for properly deallocating a asset id object (`cardano_asset_id_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the asset_id object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_asset_id_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the asset_id
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_asset_id_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_asset_id_list_t* list = (cardano_asset_id_list_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_asset_id_list_new(cardano_asset_id_list_t** asset_id)
{
  if (asset_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_id_list_t* list = _cardano_malloc(sizeof(cardano_asset_id_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_asset_id_list_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *asset_id = list;

  return CARDANO_SUCCESS;
}

size_t
cardano_asset_id_list_get_length(const cardano_asset_id_list_t* asset_id_list)
{
  if (asset_id_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(asset_id_list->array);
}

cardano_error_t
cardano_asset_id_list_get(
  const cardano_asset_id_list_t* asset_id_list,
  size_t                         index,
  cardano_asset_id_t**           element)
{
  if (asset_id_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(asset_id_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_asset_id_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_asset_id_list_add(cardano_asset_id_list_t* asset_id_list, cardano_asset_id_t* element)
{
  if (asset_id_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(asset_id_list->array);
  const size_t new_size      = cardano_array_push(asset_id_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

void
cardano_asset_id_list_unref(cardano_asset_id_list_t** asset_id_list)
{
  if ((asset_id_list == NULL) || (*asset_id_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*asset_id_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *asset_id_list = NULL;
    return;
  }
}

void
cardano_asset_id_list_ref(cardano_asset_id_list_t* asset_id_list)
{
  if (asset_id_list == NULL)
  {
    return;
  }

  cardano_object_ref(&asset_id_list->base);
}

size_t
cardano_asset_id_list_refcount(const cardano_asset_id_list_t* asset_id_list)
{
  if (asset_id_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&asset_id_list->base);
}

void
cardano_asset_id_list_set_last_error(cardano_asset_id_list_t* asset_id_list, const char* message)
{
  cardano_object_set_last_error(&asset_id_list->base, message);
}

const char*
cardano_asset_id_list_get_last_error(const cardano_asset_id_list_t* asset_id_list)
{
  return cardano_object_get_last_error(&asset_id_list->base);
}
