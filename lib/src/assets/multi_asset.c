/**
 * \file multi_asset.c
 *
 * \author angel.castillo
 * \date   Sep 13, 2024
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

#include <cardano/assets/asset_name_map.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano multi asset key value pair.
 */
typedef struct cardano_multi_asset_kvp_t
{
    cardano_object_t          base;
    cardano_blake2b_hash_t*   key;
    cardano_asset_name_map_t* value;
} cardano_multi_asset_kvp_t;

/**
 * \brief Represents a collection of assets in the Cardano blockchain.
 */
typedef struct cardano_multi_asset_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_multi_asset_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a multi_asset object.
 *
 * This function is responsible for properly deallocating a multi_asset object (`cardano_multi_asset_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the multi_asset object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_multi_asset_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the multi_asset
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_multi_asset_deallocate(void* object)
{
  assert(object != NULL);

  cardano_multi_asset_t* map = (cardano_multi_asset_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a multi asset map key value pair object.
 *
 * This function is responsible for properly deallocating a multi_asset map key value pair object (`cardano_multi_asset_map_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the multi_asset_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_multi_asset_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the multi_asset_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_multi_asset_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_multi_asset_kvp_t* map = (cardano_multi_asset_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_blake2b_hash_unref(&map->key);
  }

  if (map->value != NULL)
  {
    cardano_asset_name_map_unref(&map->value);
  }

  _cardano_free(map);
}

/**
 * \brief Compares two cardano_object_t objects based on their key hash value.
 *
 * This function casts the cardano_object_t objects to cardano_multi_asset_kvp_t
 * and compares their key hashes using the cardano_blake2b_hash_compare function.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context Unused context parameter.
 *
 * \return A negative value if the hash of lhs is less than the hash of rhs, zero if they are equal,
 *         and a positive value if the hash of lhs is greater than the hash of rhs.
 */
static int32_t
compare_by_hash(const cardano_object_t* lhs, const cardano_object_t* rhs, void* context)
{
  assert(lhs != NULL);
  assert(rhs != NULL);

  CARDANO_UNUSED(context);

  const cardano_multi_asset_kvp_t* lhs_kvp = (const cardano_multi_asset_kvp_t*)((const void*)lhs);
  const cardano_multi_asset_kvp_t* rhs_kvp = (const cardano_multi_asset_kvp_t*)((const void*)rhs);

  return cardano_blake2b_hash_compare(lhs_kvp->key, rhs_kvp->key);
}

/**
 * \brief Predicate function that returns true if the value of the cardano_multi_asset_kvp_t object is different than empty.
 *
 * @param element Pointer to the cardano_object_t object.
 * @return true if the value of the cardano_multi_asset_kvp_t object is different than empty, false otherwise.
 */
static bool
different_than_empty(const cardano_object_t* element, const void* context)
{
  CARDANO_UNUSED(context);

  const cardano_multi_asset_kvp_t* kvp = (const cardano_multi_asset_kvp_t*)((const void*)element);

  if (kvp == NULL)
  {
    return false;
  }

  return cardano_asset_name_map_get_length(kvp->value) > 0U;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_multi_asset_new(cardano_multi_asset_t** multi_asset)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t* map = _cardano_malloc(sizeof(cardano_multi_asset_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_multi_asset_deallocate;

  map->array = cardano_array_new(128);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *multi_asset = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_multi_asset_from_cbor(cardano_cbor_reader_t* reader, cardano_multi_asset_t** multi_asset)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t         map_size          = 0;
  cardano_error_t expect_map_result = cardano_cbor_reader_read_start_map(reader, &map_size);

  CARDANO_UNUSED(map_size);

  if (expect_map_result != CARDANO_SUCCESS)
  {
    return expect_map_result;
  }

  cardano_multi_asset_t* data       = NULL;
  cardano_error_t        new_result = cardano_multi_asset_new(&data);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  cardano_cbor_reader_state_t state;

  cardano_error_t read_state = cardano_cbor_reader_peek_state(reader, &state);

  if (read_state != CARDANO_SUCCESS)
  {
    cardano_multi_asset_unref(&data);
    return read_state;
  }

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    read_state = cardano_cbor_reader_peek_state(reader, &state);

    if (read_state != CARDANO_SUCCESS)
    {
      cardano_multi_asset_unref(&data);
      return read_state;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_blake2b_hash_t* script_hash = NULL;

    cardano_error_t read_script_hash_result = cardano_blake2b_hash_from_cbor(reader, &script_hash);

    if (read_script_hash_result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&script_hash);
      cardano_multi_asset_unref(&data);

      return read_script_hash_result;
    }

    cardano_asset_name_map_t* asset_name_map = NULL;

    cardano_error_t read_asset_name_map_result = cardano_asset_name_map_from_cbor(reader, &asset_name_map);

    if (read_asset_name_map_result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&script_hash);
      cardano_asset_name_map_unref(&asset_name_map);
      cardano_multi_asset_unref(&data);

      return read_asset_name_map_result;
    }

    cardano_error_t insert_result = cardano_multi_asset_insert_assets(data, script_hash, asset_name_map);

    if (insert_result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&script_hash);
      cardano_asset_name_map_unref(&asset_name_map);
      cardano_multi_asset_unref(&data);

      return insert_result;
    }

    cardano_blake2b_hash_unref(&script_hash);
    cardano_asset_name_map_unref(&asset_name_map);
  }

  cardano_array_sort(data->array, compare_by_hash, NULL);

  *multi_asset = data;

  return cardano_cbor_reader_read_end_map(reader);
}

cardano_error_t
cardano_multi_asset_to_cbor(
  const cardano_multi_asset_t* multi_asset,
  cardano_cbor_writer_t*       writer)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t size = cardano_array_get_size(multi_asset->array);

  cardano_error_t write_map_result = cardano_cbor_writer_write_start_map(writer, (int64_t)size);

  if (write_map_result != CARDANO_SUCCESS)
  {
    return write_map_result;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_multi_asset_kvp_t* kvp = (cardano_multi_asset_kvp_t*)((void*)cardano_array_get(multi_asset->array, i));
    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    if (kvp == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    cardano_error_t write_voter_result = cardano_blake2b_hash_to_cbor(kvp->key, writer);

    if (write_voter_result != CARDANO_SUCCESS)
    {
      return write_voter_result;
    }

    cardano_error_t write_asset_name_map_result = cardano_asset_name_map_to_cbor(kvp->value, writer);

    if (write_asset_name_map_result != CARDANO_SUCCESS)
    {
      return write_asset_name_map_result;
    }
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_multi_asset_get_policy_count(const cardano_multi_asset_t* multi_asset)
{
  if (multi_asset == NULL)
  {
    return 0U;
  }

  return cardano_array_get_size(multi_asset->array);
}

cardano_error_t
cardano_multi_asset_insert_assets(
  cardano_multi_asset_t*    multi_asset,
  cardano_blake2b_hash_t*   policy_id,
  cardano_asset_name_map_t* assets)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (policy_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (assets == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t size = cardano_array_get_size(multi_asset->array);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_multi_asset_kvp_t* kvp = (cardano_multi_asset_kvp_t*)((void*)cardano_array_get(multi_asset->array, i));
    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    if (kvp == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    if (cardano_blake2b_hash_equals(kvp->key, policy_id))
    {
      cardano_asset_name_map_unref(&kvp->value);
      cardano_asset_name_map_ref(assets);

      kvp->value = assets;

      return CARDANO_SUCCESS;
    }
  }

  cardano_multi_asset_kvp_t* kvp = _cardano_malloc(sizeof(cardano_multi_asset_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_multi_asset_kvp_deallocate;
  kvp->key                = policy_id;
  kvp->value              = assets;

  cardano_blake2b_hash_ref(policy_id);
  cardano_asset_name_map_ref(assets);

  const size_t old_size = cardano_array_get_size(multi_asset->array);
  const size_t new_size = cardano_array_push(multi_asset->array, (cardano_object_t*)((void*)kvp));

  if (new_size != (old_size + 1U))
  {
    cardano_multi_asset_kvp_deallocate(kvp);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_array_sort(multi_asset->array, compare_by_hash, NULL);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_multi_asset_get_assets(
  const cardano_multi_asset_t* multi_asset,
  cardano_blake2b_hash_t*      policy_id,
  cardano_asset_name_map_t**   assets)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (policy_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (assets == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t size = cardano_array_get_size(multi_asset->array);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_multi_asset_kvp_t* kvp = (cardano_multi_asset_kvp_t*)((void*)cardano_array_get(multi_asset->array, i));
    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    if (kvp == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    if (cardano_blake2b_hash_equals(kvp->key, policy_id))
    {
      cardano_asset_name_map_ref(kvp->value);
      *assets = kvp->value;

      return CARDANO_SUCCESS;
    }
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_multi_asset_get(
  const cardano_multi_asset_t* multi_asset,
  cardano_blake2b_hash_t*      policy_id,
  cardano_asset_name_t*        asset,
  int64_t*                     value)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (policy_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_name_map_t* assets = NULL;

  cardano_error_t get_assets_result = cardano_multi_asset_get_assets(multi_asset, policy_id, &assets);

  if (get_assets_result != CARDANO_SUCCESS)
  {
    return get_assets_result;
  }

  cardano_error_t get_result = cardano_asset_name_map_get(assets, asset, value);

  cardano_asset_name_map_unref(&assets);

  return get_result;
}

cardano_error_t
cardano_multi_asset_get_with_id(
  const cardano_multi_asset_t* multi_asset,
  cardano_asset_id_t*          id,
  int64_t*                     value)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_t* policy_id = cardano_asset_id_get_policy_id(id);
  cardano_asset_name_t*   name      = cardano_asset_id_get_asset_name(id);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);

  if ((policy_id == NULL) || (name == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_multi_asset_get(multi_asset, policy_id, name, value);
}

cardano_error_t
cardano_multi_asset_set(
  cardano_multi_asset_t*  multi_asset,
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   asset,
  const int64_t           value)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (policy_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_name_map_t* assets = NULL;

  cardano_error_t get_assets_result = cardano_multi_asset_get_assets(multi_asset, policy_id, &assets);

  if ((get_assets_result != CARDANO_SUCCESS) && (get_assets_result != CARDANO_ERROR_ELEMENT_NOT_FOUND))
  {
    return get_assets_result;
  }

  if (get_assets_result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
  {
    cardano_asset_name_map_t* new_assets = NULL;

    cardano_error_t new_result = cardano_asset_name_map_new(&new_assets);

    if (new_result != CARDANO_SUCCESS)
    {
      return new_result;
    }

    cardano_error_t insert_result = cardano_multi_asset_insert_assets(multi_asset, policy_id, new_assets);

    if (insert_result != CARDANO_SUCCESS)
    {
      cardano_asset_name_map_unref(&new_assets);

      return insert_result;
    }

    assets = new_assets;
  }

  cardano_error_t set_result = cardano_asset_name_map_insert(assets, asset, value);

  cardano_asset_name_map_unref(&assets);

  return set_result;
}

cardano_error_t
cardano_multi_asset_get_keys(
  const cardano_multi_asset_t* multi_asset,
  cardano_policy_id_list_t**   keys)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_policy_id_list_t* list = NULL;

  cardano_error_t new_result = cardano_policy_id_list_new(&list);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  size_t size = cardano_array_get_size(multi_asset->array);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_multi_asset_kvp_t* kvp = (cardano_multi_asset_kvp_t*)((void*)cardano_array_get(multi_asset->array, i));
    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    if (kvp == NULL)
    {
      cardano_policy_id_list_unref(&list);
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    cardano_error_t insert_result = cardano_policy_id_list_add(list, kvp->key);

    if (insert_result != CARDANO_SUCCESS)
    {
      cardano_policy_id_list_unref(&list);
      return insert_result;
    }
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_multi_asset_add(
  const cardano_multi_asset_t* lhs,
  const cardano_multi_asset_t* rhs,
  cardano_multi_asset_t**      result)
{
  if (lhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (rhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t* map = NULL;

  cardano_error_t create_result = cardano_multi_asset_new(&map);

  if (create_result != CARDANO_SUCCESS)
  {
    return create_result;
  }

  for (size_t i = 0; i < cardano_array_get_size(lhs->array); ++i)
  {
    cardano_object_t*          object = cardano_array_get(lhs->array, i);
    cardano_multi_asset_kvp_t* kvp    = (cardano_multi_asset_kvp_t*)((void*)object);
    cardano_object_unref(&object);

    cardano_error_t insert_result = cardano_multi_asset_insert_assets(map, kvp->key, kvp->value);

    if (insert_result != CARDANO_SUCCESS)
    {
      cardano_multi_asset_unref(&map);
      return insert_result;
    }
  }

  for (size_t i = 0; i < cardano_array_get_size(rhs->array); ++i)
  {
    cardano_object_t*          object = cardano_array_get(rhs->array, i);
    cardano_multi_asset_kvp_t* kvp    = (cardano_multi_asset_kvp_t*)((void*)object);
    cardano_object_unref(&object);

    cardano_asset_name_map_t* value      = NULL;
    cardano_error_t           get_result = cardano_multi_asset_get_assets(map, kvp->key, &value);

    if (get_result == CARDANO_SUCCESS)
    {
      cardano_asset_name_map_t* add_result = NULL;

      cardano_error_t add_assets_result = cardano_asset_name_map_add(value, kvp->value, &add_result);

      if (add_assets_result != CARDANO_SUCCESS)
      {
        cardano_asset_name_map_unref(&value);
        cardano_asset_name_map_unref(&add_result);
        cardano_multi_asset_unref(&map);

        return add_assets_result;
      }

      cardano_error_t insert_result = cardano_multi_asset_insert_assets(map, kvp->key, add_result);

      cardano_asset_name_map_unref(&value);
      cardano_asset_name_map_unref(&add_result);

      if (insert_result != CARDANO_SUCCESS)
      {
        cardano_multi_asset_unref(&map);

        return insert_result;
      }
    }
    else
    {
      cardano_error_t insert_result = cardano_multi_asset_insert_assets(map, kvp->key, kvp->value);

      if (insert_result != CARDANO_SUCCESS)
      {
        cardano_multi_asset_unref(&map);

        return insert_result;
      }
    }
  }

  cardano_array_t* filtered = cardano_array_filter(map->array, different_than_empty, NULL);

  cardano_array_unref(&map->array);
  map->array = filtered;

  *result = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_multi_asset_subtract(
  const cardano_multi_asset_t* lhs,
  const cardano_multi_asset_t* rhs,
  cardano_multi_asset_t**      result)
{
  if (lhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (rhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t* map = NULL;

  cardano_error_t create_result = cardano_multi_asset_new(&map);

  if (create_result != CARDANO_SUCCESS)
  {
    return create_result;
  }

  for (size_t i = 0; i < cardano_array_get_size(lhs->array); ++i)
  {
    cardano_object_t*          object = cardano_array_get(lhs->array, i);
    cardano_multi_asset_kvp_t* kvp    = (cardano_multi_asset_kvp_t*)((void*)object);
    cardano_object_unref(&object);

    cardano_error_t insert_result = cardano_multi_asset_insert_assets(map, kvp->key, kvp->value);

    if (insert_result != CARDANO_SUCCESS)
    {
      cardano_multi_asset_unref(&map);

      return insert_result;
    }
  }

  for (size_t i = 0; i < cardano_array_get_size(rhs->array); ++i)
  {
    cardano_object_t*          object = cardano_array_get(rhs->array, i);
    cardano_multi_asset_kvp_t* kvp    = (cardano_multi_asset_kvp_t*)((void*)object);
    cardano_object_unref(&object);

    cardano_asset_name_map_t* value      = NULL;
    cardano_error_t           get_result = cardano_multi_asset_get_assets(map, kvp->key, &value);

    if (get_result == CARDANO_SUCCESS)
    {
      cardano_asset_name_map_t* subtract_result = NULL;

      cardano_error_t subtract_assets_result = cardano_asset_name_map_subtract(value, kvp->value, &subtract_result);

      if (subtract_assets_result != CARDANO_SUCCESS)
      {
        cardano_asset_name_map_unref(&value);
        cardano_asset_name_map_unref(&subtract_result);
        cardano_multi_asset_unref(&map);

        return subtract_assets_result;
      }

      cardano_error_t insert_result = cardano_multi_asset_insert_assets(map, kvp->key, subtract_result);

      cardano_asset_name_map_unref(&value);
      cardano_asset_name_map_unref(&subtract_result);

      if (insert_result != CARDANO_SUCCESS)
      {
        cardano_multi_asset_unref(&map);

        return insert_result;
      }
    }
    else
    {
      cardano_asset_name_map_t* empty_map       = NULL;
      cardano_asset_name_map_t* subtract_result = NULL;

      cardano_error_t create_empty_result = cardano_asset_name_map_new(&empty_map);

      if (create_empty_result != CARDANO_SUCCESS)
      {
        cardano_asset_name_map_unref(&empty_map);
        cardano_multi_asset_unref(&map);

        return create_empty_result;
      }

      cardano_error_t subtract_assets_result = cardano_asset_name_map_subtract(empty_map, kvp->value, &subtract_result);

      if (subtract_assets_result != CARDANO_SUCCESS)
      {
        cardano_asset_name_map_unref(&empty_map);
        cardano_asset_name_map_unref(&subtract_result);
        cardano_multi_asset_unref(&map);

        return subtract_assets_result;
      }

      cardano_error_t insert_result = cardano_multi_asset_insert_assets(map, kvp->key, subtract_result);

      cardano_asset_name_map_unref(&empty_map);
      cardano_asset_name_map_unref(&subtract_result);

      if (insert_result != CARDANO_SUCCESS)
      {
        cardano_multi_asset_unref(&map);

        return insert_result;
      }
    }
  }

  cardano_array_t* filtered = cardano_array_filter(map->array, different_than_empty, NULL);

  cardano_array_unref(&map->array);
  map->array = filtered;

  *result = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_multi_asset_get_positive(
  cardano_multi_asset_t*  multi_asset,
  cardano_multi_asset_t** result)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t* map = NULL;

  cardano_error_t create_result = cardano_multi_asset_new(&map);

  if (create_result != CARDANO_SUCCESS)
  {
    return create_result;
  }

  for (size_t i = 0; i < cardano_array_get_size(multi_asset->array); ++i)
  {
    cardano_object_t*          object = cardano_array_get(multi_asset->array, i);
    cardano_multi_asset_kvp_t* kvp    = (cardano_multi_asset_kvp_t*)((void*)object);
    cardano_object_unref(&object);

    cardano_asset_name_map_t* value      = NULL;
    cardano_error_t           get_result = cardano_multi_asset_get_assets(multi_asset, kvp->key, &value);

    cardano_asset_name_map_unref(&value);

    if (get_result != CARDANO_SUCCESS)
    {
      cardano_multi_asset_unref(&map);

      return get_result;
    }

    const size_t asset_names_count = cardano_asset_name_map_get_length(value);

    for (size_t j = 0; j < asset_names_count; ++j)
    {
      cardano_asset_name_t* asset_name  = NULL;
      int64_t               asset_value = 0;

      cardano_error_t get_asset_result = cardano_asset_name_map_get_key_value_at(value, j, &asset_name, &asset_value);
      cardano_asset_name_unref(&asset_name);

      if (get_asset_result != CARDANO_SUCCESS)
      {
        cardano_multi_asset_unref(&map);

        return get_asset_result;
      }

      if (asset_value > 0)
      {
        cardano_error_t insert_result = cardano_multi_asset_set(map, kvp->key, asset_name, asset_value);

        if (insert_result != CARDANO_SUCCESS)
        {
          cardano_multi_asset_unref(&map);

          return insert_result;
        }
      }
    }
  }

  *result = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_multi_asset_get_negative(
  cardano_multi_asset_t*  multi_asset,
  cardano_multi_asset_t** result)
{
  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t* map = NULL;

  cardano_error_t create_result = cardano_multi_asset_new(&map);

  if (create_result != CARDANO_SUCCESS)
  {
    return create_result;
  }

  for (size_t i = 0; i < cardano_array_get_size(multi_asset->array); ++i)
  {
    cardano_object_t*          object = cardano_array_get(multi_asset->array, i);
    cardano_multi_asset_kvp_t* kvp    = (cardano_multi_asset_kvp_t*)((void*)object);
    cardano_object_unref(&object);

    cardano_asset_name_map_t* value      = NULL;
    cardano_error_t           get_result = cardano_multi_asset_get_assets(multi_asset, kvp->key, &value);

    cardano_asset_name_map_unref(&value);

    if (get_result != CARDANO_SUCCESS)
    {
      cardano_multi_asset_unref(&map);

      return get_result;
    }

    const size_t asset_names_count = cardano_asset_name_map_get_length(value);

    for (size_t j = 0; j < asset_names_count; ++j)
    {
      cardano_asset_name_t* asset_name  = NULL;
      int64_t               asset_value = 0;

      cardano_error_t get_asset_result = cardano_asset_name_map_get_key_value_at(value, j, &asset_name, &asset_value);
      cardano_asset_name_unref(&asset_name);

      if (get_asset_result != CARDANO_SUCCESS)
      {
        cardano_multi_asset_unref(&map);

        return get_asset_result;
      }

      if (asset_value < 0)
      {
        cardano_error_t insert_result = cardano_multi_asset_set(map, kvp->key, asset_name, asset_value);

        if (insert_result != CARDANO_SUCCESS)
        {
          cardano_multi_asset_unref(&map);

          return insert_result;
        }
      }
    }
  }

  *result = map;

  return CARDANO_SUCCESS;
}

bool
cardano_multi_asset_equals(const cardano_multi_asset_t* lhs, const cardano_multi_asset_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (cardano_array_get_size(lhs->array) != cardano_array_get_size(rhs->array))
  {
    return false;
  }

  for (size_t i = 0; i < cardano_array_get_size(lhs->array); ++i)
  {
    cardano_object_t*          lhs_object = cardano_array_get(lhs->array, i);
    cardano_multi_asset_kvp_t* lhs_kvp    = (cardano_multi_asset_kvp_t*)((void*)lhs_object);
    cardano_object_unref(&lhs_object);

    if (lhs_kvp == NULL)
    {
      return false;
    }

    cardano_object_t*          rhs_object = cardano_array_get(rhs->array, i);
    cardano_multi_asset_kvp_t* rhs_kvp    = (cardano_multi_asset_kvp_t*)((void*)rhs_object);
    cardano_object_unref(&rhs_object);

    if (rhs_kvp == NULL)
    {
      return false;
    }

    if (!cardano_blake2b_hash_equals(lhs_kvp->key, rhs_kvp->key))
    {
      return false;
    }

    if (!cardano_asset_name_map_equals(lhs_kvp->value, rhs_kvp->value))
    {
      return false;
    }
  }

  return true;
}

void
cardano_multi_asset_unref(cardano_multi_asset_t** multi_asset)
{
  if ((multi_asset == NULL) || (*multi_asset == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*multi_asset)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *multi_asset = NULL;
    return;
  }
}

void
cardano_multi_asset_ref(cardano_multi_asset_t* multi_asset)
{
  if (multi_asset == NULL)
  {
    return;
  }

  cardano_object_ref(&multi_asset->base);
}

size_t
cardano_multi_asset_refcount(const cardano_multi_asset_t* multi_asset)
{
  if (multi_asset == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&multi_asset->base);
}

void
cardano_multi_asset_set_last_error(cardano_multi_asset_t* multi_asset, const char* message)
{
  cardano_object_set_last_error(&multi_asset->base, message);
}

const char*
cardano_multi_asset_get_last_error(const cardano_multi_asset_t* multi_asset)
{
  return cardano_object_get_last_error(&multi_asset->base);
}
