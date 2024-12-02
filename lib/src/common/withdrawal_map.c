/**
 * \file withdrawal_map.c
 *
 * \author angel.castillo
 * \date   May 12, 2024
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

#include <cardano/common/reward_address_list.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/set.h"

#include <assert.h>
#include <cardano/address/reward_address.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano withdrawal map.
 */
typedef struct cardano_withdrawal_map_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_withdrawal_map_t;

/**
 * \brief Represents a Cardano withdrawal map key value pair.
 */
typedef struct cardano_withdrawal_map_kvp_t
{
    cardano_object_t          base;
    cardano_reward_address_t* key;
    uint64_t                  value;
} cardano_withdrawal_map_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a withdrawal map object.
 *
 * This function is responsible for properly deallocating a withdrawal map object (`cardano_withdrawal_map_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the withdrawal_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_withdrawal_map_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the withdrawal_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_withdrawal_map_deallocate(void* object)
{
  assert(object != NULL);

  cardano_withdrawal_map_t* map = (cardano_withdrawal_map_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a withdrawal map key value pair object.
 *
 * This function is responsible for properly deallocating a withdrawal map key value pair object (`cardano_withdrawal_map_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the withdrawal_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_withdrawal_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the withdrawal_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_withdrawal_map_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_withdrawal_map_kvp_t* map = (cardano_withdrawal_map_kvp_t*)object;

  cardano_reward_address_unref(&map->key);

  _cardano_free(map);
}

/**
 * \brief Compares two cardano_object_t objects based on their reward_address.
 *
 * @param lhs Pointer to the first cardano_object_t object.
 * @param rhs Pointer to the second cardano_object_t object.
 *
 * @return true if the addresses are equal, false otherwise.
 */
static bool
reward_address_equals(const cardano_reward_address_t* lhs, const cardano_reward_address_t* rhs)
{
  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  const size_t lhs_size = cardano_reward_address_get_bytes_size(lhs);
  const size_t rhs_size = cardano_reward_address_get_bytes_size(rhs);

  if (lhs_size != rhs_size)
  {
    return false;
  }

  const uint8_t* lhs_bytes = cardano_reward_address_get_bytes(lhs);
  const uint8_t* rhs_bytes = cardano_reward_address_get_bytes(rhs);

  return memcmp(lhs_bytes, rhs_bytes, lhs_size) == 0;
}

/**
 * \brief Compares two cardano_withdrawal_map_kvp_t objects based on their reward_address.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context Unused.
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

  const cardano_withdrawal_map_kvp_t* lhs_kvp = (const cardano_withdrawal_map_kvp_t*)((const void*)lhs);
  const cardano_withdrawal_map_kvp_t* rhs_kvp = (const cardano_withdrawal_map_kvp_t*)((const void*)rhs);

  const size_t lhs_size = cardano_reward_address_get_bytes_size(lhs_kvp->key);
  const size_t rhs_size = cardano_reward_address_get_bytes_size(rhs_kvp->key);

  if (lhs_size != rhs_size)
  {
    return (lhs_size < rhs_size) ? -1 : 1;
  }

  const uint8_t* lhs_bytes = cardano_reward_address_get_bytes(lhs_kvp->key);
  const uint8_t* rhs_bytes = cardano_reward_address_get_bytes(rhs_kvp->key);

  return memcmp(lhs_bytes, rhs_bytes, lhs_size);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_withdrawal_map_new(cardano_withdrawal_map_t** withdrawal_map)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_withdrawal_map_t* map = _cardano_malloc(sizeof(cardano_withdrawal_map_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_withdrawal_map_deallocate;

  map->array = cardano_array_new(32);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *withdrawal_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_withdrawal_map_from_cbor(cardano_cbor_reader_t* reader, cardano_withdrawal_map_t** withdrawal_map)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_withdrawal_map_t* map    = NULL;
  cardano_error_t           result = cardano_withdrawal_map_new(&map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_withdrawal_map_unref(&map);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_withdrawal_map_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_reward_address_t* key   = NULL;
    uint64_t                  value = 0;

    cardano_buffer_t* reward_address_bytes = NULL;

    cardano_error_t read_reward_address_result = cardano_cbor_reader_read_bytestring(reader, &reward_address_bytes);

    if (read_reward_address_result != CARDANO_SUCCESS)
    {
      cardano_withdrawal_map_unref(&map);
      return read_reward_address_result;
    }

    cardano_error_t reward_address_result = cardano_reward_address_from_bytes(cardano_buffer_get_data(reward_address_bytes), cardano_buffer_get_size(reward_address_bytes), &key);

    cardano_buffer_unref(&reward_address_bytes);

    if (reward_address_result != CARDANO_SUCCESS)
    {
      cardano_withdrawal_map_unref(&map);
      return reward_address_result;
    }

    result = cardano_cbor_reader_read_uint(reader, &value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_reward_address_unref(&key);
      cardano_withdrawal_map_unref(&map);
      return result;
    }

    cardano_withdrawal_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_withdrawal_map_kvp_t));

    if (kvp == NULL)
    {
      cardano_reward_address_unref(&key);
      cardano_withdrawal_map_unref(&map);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    kvp->base.ref_count     = 0;
    kvp->base.last_error[0] = '\0';
    kvp->base.deallocator   = cardano_withdrawal_map_kvp_deallocate;
    kvp->key                = key;
    kvp->value              = value;

    const size_t old_size = cardano_array_get_size(map->array);
    const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

    assert((old_size + 1U) == new_size);

    CARDANO_UNUSED(old_size);
    CARDANO_UNUSED(new_size);

    cardano_array_sort(map->array, compare_by_bytes, NULL);
  }

  result = cardano_cbor_validate_end_map("withdrawal_map", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_withdrawal_map_unref(&map);
    return result;
  }

  *withdrawal_map = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_withdrawal_map_to_cbor(const cardano_withdrawal_map_t* withdrawal_map, cardano_cbor_writer_t* writer)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  size_t map_size = cardano_array_get_size(withdrawal_map->array);
  result          = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(withdrawal_map->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(withdrawal_map->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in withdrawal map is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_withdrawal_map_kvp_t* kvp_data = (cardano_withdrawal_map_kvp_t*)((void*)kvp);

    cardano_error_t write_reward_address_result = cardano_cbor_writer_write_bytestring(writer, cardano_reward_address_get_bytes(kvp_data->key), cardano_reward_address_get_bytes_size(kvp_data->key));

    if (write_reward_address_result != CARDANO_SUCCESS)
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
cardano_withdrawal_map_get_length(const cardano_withdrawal_map_t* withdrawal_map)
{
  if (withdrawal_map == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(withdrawal_map->array);
}

cardano_error_t
cardano_withdrawal_map_get(
  cardano_withdrawal_map_t* withdrawal_map,
  cardano_reward_address_t* key,
  uint64_t*                 element)
{
  if (withdrawal_map == NULL)
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

  for (size_t i = 0; i < cardano_array_get_size(withdrawal_map->array); ++i)
  {
    cardano_object_t*             object = cardano_array_get(withdrawal_map->array, i);
    cardano_withdrawal_map_kvp_t* kvp    = (cardano_withdrawal_map_kvp_t*)((void*)object);

    if (reward_address_equals(kvp->key, key))
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
cardano_withdrawal_map_insert(
  cardano_withdrawal_map_t* withdrawal_map,
  cardano_reward_address_t* key,
  const uint64_t            value)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_withdrawal_map_kvp_t* kvp = _cardano_malloc(sizeof(cardano_withdrawal_map_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_withdrawal_map_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_reward_address_ref(key);

  const size_t old_size = cardano_array_get_size(withdrawal_map->array);
  const size_t new_size = cardano_array_push(withdrawal_map->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(withdrawal_map->array, compare_by_bytes, NULL);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_withdrawal_map_insert_ex(
  cardano_withdrawal_map_t* withdrawal_map,
  const char*               reward_address,
  size_t                    reward_address_size,
  uint64_t                  value)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_t* key = NULL;

  cardano_error_t result = cardano_reward_address_from_bech32(reward_address, reward_address_size, &key);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_withdrawal_map_insert(withdrawal_map, key, value);

  cardano_reward_address_unref(&key);

  return result;
}

cardano_error_t
cardano_withdrawal_map_get_keys(
  cardano_withdrawal_map_t*       withdrawal_map,
  cardano_reward_address_list_t** keys)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_list_t* list = NULL;

  cardano_error_t result = cardano_reward_address_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(withdrawal_map->array); ++i)
  {
    cardano_object_t*             object = cardano_array_get(withdrawal_map->array, i);
    cardano_withdrawal_map_kvp_t* kvp    = (cardano_withdrawal_map_kvp_t*)((void*)object);

    result = cardano_reward_address_list_add(list, kvp->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_reward_address_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_withdrawal_map_get_key_at(
  const cardano_withdrawal_map_t* withdrawal_map,
  const size_t                    index,
  cardano_reward_address_t**      reward_address)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(withdrawal_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*             object = cardano_array_get(withdrawal_map->array, index);
  cardano_withdrawal_map_kvp_t* kvp    = (cardano_withdrawal_map_kvp_t*)((void*)object);

  cardano_reward_address_ref(kvp->key);
  cardano_object_unref(&object);

  *reward_address = kvp->key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_withdrawal_map_get_value_at(
  const cardano_withdrawal_map_t* withdrawal_map,
  size_t                          index,
  uint64_t*                       amount)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(withdrawal_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*             object = cardano_array_get(withdrawal_map->array, index);
  cardano_withdrawal_map_kvp_t* kvp    = (cardano_withdrawal_map_kvp_t*)((void*)object);

  cardano_object_unref(&object);

  *amount = kvp->value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_withdrawal_map_get_key_value_at(
  const cardano_withdrawal_map_t* withdrawal_map,
  size_t                          index,
  cardano_reward_address_t**      reward_address,
  uint64_t*                       amount)
{
  if (withdrawal_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(withdrawal_map->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*             object = cardano_array_get(withdrawal_map->array, index);
  cardano_withdrawal_map_kvp_t* kvp    = (cardano_withdrawal_map_kvp_t*)((void*)object);

  cardano_reward_address_ref(kvp->key);
  cardano_object_unref(&object);

  *reward_address = kvp->key;
  *amount         = kvp->value;

  return CARDANO_SUCCESS;
}

void
cardano_withdrawal_map_unref(cardano_withdrawal_map_t** withdrawal_map)
{
  if ((withdrawal_map == NULL) || (*withdrawal_map == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*withdrawal_map)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *withdrawal_map = NULL;
    return;
  }
}

void
cardano_withdrawal_map_ref(cardano_withdrawal_map_t* withdrawal_map)
{
  if (withdrawal_map == NULL)
  {
    return;
  }

  cardano_object_ref(&withdrawal_map->base);
}

size_t
cardano_withdrawal_map_refcount(const cardano_withdrawal_map_t* withdrawal_map)
{
  if (withdrawal_map == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&withdrawal_map->base);
}

void
cardano_withdrawal_map_set_last_error(cardano_withdrawal_map_t* withdrawal_map, const char* message)
{
  cardano_object_set_last_error(&withdrawal_map->base, message);
}

const char*
cardano_withdrawal_map_get_last_error(const cardano_withdrawal_map_t* withdrawal_map)
{
  return cardano_object_get_last_error(&withdrawal_map->base);
}
