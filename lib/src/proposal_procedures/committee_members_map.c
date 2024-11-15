/**
 * \file committee_members_map.c
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

#include <cardano/object.h>
#include <cardano/proposal_procedures/committee_members_map.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a map of committee members to epoch.
 */
typedef struct cardano_committee_members_map_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_committee_members_map_t;

/**
 * \brief Represents a committee members to epoch key value pair.
 */
typedef struct cardano_committee_members_map_kvp_t
{
    cardano_object_t      base;
    cardano_credential_t* key;
    uint64_t              value;
} cardano_committee_members_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a committee members object.
 *
 * This function is responsible for properly deallocating a committee members object (`cardano_committee_members_map_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the committee_members_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_committee_members_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the committee_members_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_committee_members_map_deallocate(void* object)
{
  assert(object != NULL);

  cardano_committee_members_map_t* map = (cardano_committee_members_map_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a committee members key value pair object.
 *
 * This function is responsible for properly deallocating a committee members key value pair object (`cardano_committee_members_map_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the committee_members_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_committee_members_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the committee_members_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_committee_members_map_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_committee_members_map_kvp_t* map = (cardano_committee_members_map_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_credential_unref(&map->key);
  }

  _cardano_free(map);
}

/**
 * \brief Compares two cardano_committee_members_map_kvp_t objects based on their credential.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context Unused.
 *
 * \return A negative value if the credential of lhs is less than the credential of rhs, zero if they are equal,
 *         and a positive value if the credential of lhs is greater than the credential of rhs.
 */
static int32_t
compare_by_credentials(const cardano_object_t* lhs, const cardano_object_t* rhs, void* context)
{
  assert(lhs != NULL);
  assert(rhs != NULL);

  CARDANO_UNUSED(context);

  const cardano_committee_members_map_kvp_t* lhs_kvp = (const cardano_committee_members_map_kvp_t*)((const void*)lhs);
  const cardano_committee_members_map_kvp_t* rhs_kvp = (const cardano_committee_members_map_kvp_t*)((const void*)rhs);

  return cardano_credential_compare(lhs_kvp->key, rhs_kvp->key);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_committee_members_map_new(cardano_committee_members_map_t** committee_members_map)
{
  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_committee_members_map_t* map = _cardano_malloc(sizeof(cardano_committee_members_map_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_committee_members_map_deallocate;

  map->array = cardano_array_new(32);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *committee_members_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_members_map_from_cbor(cardano_cbor_reader_t* reader, cardano_committee_members_map_t** committee_members_map)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_committee_members_map_t* map    = NULL;
  cardano_error_t                  result = cardano_committee_members_map_new(&map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_committee_members_map_unref(&map);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_committee_members_map_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_credential_t* key   = NULL;
    uint64_t              value = 0;

    result = cardano_credential_from_cbor(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_committee_members_map_unref(&map);
      return result;
    }

    result = cardano_cbor_reader_read_uint(reader, &value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_credential_unref(&key);
      cardano_committee_members_map_unref(&map);
      return result;
    }

    cardano_committee_members_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_committee_members_map_kvp_t));

    if (kvp == NULL)
    {
      cardano_credential_unref(&key);
      cardano_committee_members_map_unref(&map);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    kvp->base.ref_count     = 0;
    kvp->base.last_error[0] = '\0';
    kvp->base.deallocator   = cardano_committee_members_map_kvp_deallocate;
    kvp->key                = key;
    kvp->value              = value;

    const size_t old_size = cardano_array_get_size(map->array);
    const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

    assert((old_size + 1U) == new_size);

    CARDANO_UNUSED(old_size);
    CARDANO_UNUSED(new_size);

    cardano_array_sort(map->array, compare_by_credentials, NULL);
  }

  result = cardano_cbor_validate_end_map("committee_members_map", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_committee_members_map_unref(&map);
    return result;
  }

  *committee_members_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_members_map_to_cbor(const cardano_committee_members_map_t* committee_members_map, cardano_cbor_writer_t* writer)
{
  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  size_t map_size = cardano_array_get_size(committee_members_map->array);
  result          = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(committee_members_map->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(committee_members_map->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in committee members is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_committee_members_map_kvp_t* kvp_data = (cardano_committee_members_map_kvp_t*)((void*)kvp);

    result = cardano_credential_to_cbor(kvp_data->key, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, kvp_data->value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    cardano_object_unref(&kvp);
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_committee_members_map_get_length(const cardano_committee_members_map_t* committee_members_map)
{
  if (committee_members_map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(committee_members_map->array);
}

cardano_error_t
cardano_committee_members_map_get(
  cardano_committee_members_map_t* committee_members_map,
  cardano_credential_t*            key,
  uint64_t*                        element)
{
  if (committee_members_map == NULL)
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

  for (size_t i = 0; i < cardano_array_get_size(committee_members_map->array); ++i)
  {
    cardano_object_t*                    object = cardano_array_get(committee_members_map->array, i);
    cardano_committee_members_map_kvp_t* kvp    = (cardano_committee_members_map_kvp_t*)((void*)object);

    if (cardano_credential_equals(kvp->key, key))
    {
      *element = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_committee_members_map_insert(
  cardano_committee_members_map_t* committee_members_map,
  cardano_credential_t*            key,
  const uint64_t                   value)
{
  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_committee_members_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_committee_members_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_committee_members_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_credential_ref(key);

  const size_t old_size = cardano_array_get_size(committee_members_map->array);
  const size_t new_size = cardano_array_push(committee_members_map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(committee_members_map->array, compare_by_credentials, NULL);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_members_map_get_keys(
  cardano_committee_members_map_t* committee_members_map,
  cardano_credential_set_t**       keys)
{
  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_set_t* list = NULL;

  cardano_error_t result = cardano_credential_set_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(committee_members_map->array); ++i)
  {
    cardano_object_t*                    object = cardano_array_get(committee_members_map->array, i);
    cardano_committee_members_map_kvp_t* kvp    = (cardano_committee_members_map_kvp_t*)((void*)object);

    result = cardano_credential_set_add(list, kvp->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_credential_set_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_members_map_get_key_at(
  const cardano_committee_members_map_t* committee_members_map,
  const size_t                           index,
  cardano_credential_t**                 credential)
{
  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(committee_members_map->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_committee_members_map_kvp_t* kvp = (cardano_committee_members_map_kvp_t*)((void*)object);

  cardano_object_unref(&object);
  cardano_credential_ref(kvp->key);

  *credential = kvp->key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_members_map_get_value_at(
  const cardano_committee_members_map_t* committee_members_map,
  const size_t                           index,
  uint64_t*                              epoch)
{
  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (epoch == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(committee_members_map->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_committee_members_map_kvp_t* kvp = (cardano_committee_members_map_kvp_t*)((void*)object);

  *epoch = kvp->value;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_members_map_get_key_value_at(
  const cardano_committee_members_map_t* committee_members_map,
  const size_t                           index,
  cardano_credential_t**                 credential,
  uint64_t*                              epoch)
{
  if (committee_members_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (epoch == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(committee_members_map->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_committee_members_map_kvp_t* kvp = (cardano_committee_members_map_kvp_t*)((void*)object);

  cardano_credential_ref(kvp->key);
  *credential = kvp->key;
  *epoch      = kvp->value;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

void
cardano_committee_members_map_unref(cardano_committee_members_map_t** committee_members_map)
{
  if ((committee_members_map == NULL) || (*committee_members_map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*committee_members_map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *committee_members_map = NULL;
    return;
  }
}

void
cardano_committee_members_map_ref(cardano_committee_members_map_t* committee_members_map)
{
  if (committee_members_map == NULL)
  {
    return;
  }

  cardano_object_ref(&committee_members_map->base);
}

size_t
cardano_committee_members_map_refcount(const cardano_committee_members_map_t* committee_members_map)
{
  if (committee_members_map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&committee_members_map->base);
}

void
cardano_committee_members_map_set_last_error(cardano_committee_members_map_t* committee_members_map, const char* message)
{
  cardano_object_set_last_error(&committee_members_map->base, message);
}

const char*
cardano_committee_members_map_get_last_error(const cardano_committee_members_map_t* committee_members_map)
{
  return cardano_object_get_last_error(&committee_members_map->base);
}
