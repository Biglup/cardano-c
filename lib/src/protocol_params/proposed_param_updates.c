/**
 * \file proposed_params_updates.c
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/object.h>
#include <cardano/protocol_params/proposed_param_updates.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief In the Cardano network, stakeholders can propose changes to the protocol parameters. These proposals are then
 * collected into a set which represents the ProposedProtocolParameterUpdates.
 */
typedef struct cardano_proposed_param_updates_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_proposed_param_updates_t;

/**
 * \brief This proposed protocol parameter updates are represented as a map of genesis delegate key hash to parameters updates. So in principles,
 * each genesis delegate can propose a different update.
 */
typedef struct cardano_proposed_param_updates_kvp_t
{
    cardano_object_t                 base;
    cardano_blake2b_hash_t*          key;
    cardano_protocol_param_update_t* value;
} cardano_proposed_param_updates_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a proposed params updates object.
 *
 * This function is responsible for properly deallocating a proposed params updates object (`cardano_proposed_param_updates_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the proposed_params_updates object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_proposed_param_updates_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the proposed_params_updates
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_proposed_param_updates_deallocate(void* object)
{
  assert(object != NULL);

  cardano_proposed_param_updates_t* map = (cardano_proposed_param_updates_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a proposed params updates key value pair object.
 *
 * This function is responsible for properly deallocating a proposed params updates key value pair object (`cardano_proposed_param_updates_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the proposed_params_updates object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_proposed_param_updates_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the proposed_params_updates
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_proposed_param_updates_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_proposed_param_updates_kvp_t* map = (cardano_proposed_param_updates_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_blake2b_hash_unref(&map->key);
  }

  if (map->value != NULL)
  {
    cardano_protocol_param_update_unref(&map->value);
  }

  _cardano_free(map);
}

/**
 * \brief Compares two cardano_object_t objects based on their Blake2b hashes.
 *
 * This function casts the cardano_object_t objects to cardano_proposed_param_updates_kvp_t
 * and compares their Blake2b hashes using the cardano_blake2b_hash_compare function.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context A pointer to a context object. This parameter is not used in this function.
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

  const cardano_proposed_param_updates_kvp_t* kvp_lhs = (const cardano_proposed_param_updates_kvp_t*)((const void*)lhs);
  const cardano_proposed_param_updates_kvp_t* kvp_rhs = (const cardano_proposed_param_updates_kvp_t*)((const void*)rhs);

  return cardano_blake2b_hash_compare(kvp_lhs->key, kvp_rhs->key);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_proposed_param_updates_new(cardano_proposed_param_updates_t** proposed_params_updates)
{
  if (proposed_params_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposed_param_updates_t* map = _cardano_malloc(sizeof(cardano_proposed_param_updates_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_proposed_param_updates_deallocate;

  map->array = cardano_array_new(128);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *proposed_params_updates = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposed_param_updates_from_cbor(cardano_cbor_reader_t* reader, cardano_proposed_param_updates_t** proposed_params_updates)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposed_params_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposed_param_updates_t* map    = NULL;
  cardano_error_t                   result = cardano_proposed_param_updates_new(&map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_proposed_param_updates_unref(&map);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_proposed_param_updates_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_blake2b_hash_t*          key   = NULL;
    cardano_protocol_param_update_t* value = NULL;

    result = cardano_blake2b_hash_from_cbor(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_proposed_param_updates_unref(&map);
      return result;
    }

    result = cardano_protocol_param_update_from_cbor(reader, &value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&key);
      cardano_proposed_param_updates_unref(&map);
      return result;
    }

    cardano_proposed_param_updates_kvp_t* kvp = _cardano_malloc(sizeof(cardano_proposed_param_updates_kvp_t));

    if (kvp == NULL)
    {
      cardano_blake2b_hash_unref(&key);
      cardano_protocol_param_update_unref(&value);
      cardano_proposed_param_updates_unref(&map);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    kvp->base.ref_count     = 0;
    kvp->base.last_error[0] = '\0';
    kvp->base.deallocator   = cardano_proposed_param_updates_kvp_deallocate;
    kvp->key                = key;
    kvp->value              = value;

    const size_t old_size = cardano_array_get_size(map->array);
    const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

    assert((old_size + 1U) == new_size);
    CARDANO_UNUSED(old_size);
    CARDANO_UNUSED(new_size);
  }

  result = cardano_cbor_validate_end_map("proposed_params_updates", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_proposed_param_updates_unref(&map);
    return result;
  }

  *proposed_params_updates = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposed_param_updates_to_cbor(const cardano_proposed_param_updates_t* proposed_params_updates, cardano_cbor_writer_t* writer)
{
  if (proposed_params_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t          map_size = cardano_array_get_size(proposed_params_updates->array);
  cardano_error_t result   = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(proposed_params_updates->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(proposed_params_updates->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in proposed params updates is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_proposed_param_updates_kvp_t* kvp_data = (cardano_proposed_param_updates_kvp_t*)((void*)kvp);

    result = cardano_blake2b_hash_to_cbor(kvp_data->key, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    result = cardano_protocol_param_update_to_cbor(kvp_data->value, writer);

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
cardano_proposed_param_updates_get_size(const cardano_proposed_param_updates_t* proposed_params_updates)
{
  if (proposed_params_updates == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(proposed_params_updates->array);
}

cardano_error_t
cardano_proposed_param_updates_insert(
  cardano_proposed_param_updates_t* proposed_param_updates,
  cardano_blake2b_hash_t*           genesis_delegate_key_hash,
  cardano_protocol_param_update_t*  protocol_param_update)
{
  if (proposed_param_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (genesis_delegate_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposed_param_updates_kvp_t* kvp = _cardano_malloc(sizeof(cardano_proposed_param_updates_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_proposed_param_updates_kvp_deallocate;
  kvp->key                = genesis_delegate_key_hash;
  kvp->value              = protocol_param_update;

  cardano_blake2b_hash_ref(genesis_delegate_key_hash);
  cardano_protocol_param_update_ref(protocol_param_update);

  const size_t old_size = cardano_array_get_size(proposed_param_updates->array);
  const size_t new_size = cardano_array_push(proposed_param_updates->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(proposed_param_updates->array, compare_by_hash, NULL);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposed_param_updates_get(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  cardano_blake2b_hash_t*                 genesis_delegate_key_hash,
  cardano_protocol_param_update_t**       protocol_param_update)
{
  if (proposed_param_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (genesis_delegate_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0; i < cardano_array_get_size(proposed_param_updates->array); ++i)
  {
    cardano_object_t*                     object = cardano_array_get(proposed_param_updates->array, i);
    cardano_proposed_param_updates_kvp_t* kvp    = (cardano_proposed_param_updates_kvp_t*)((void*)object);

    if (cardano_blake2b_hash_equals(kvp->key, genesis_delegate_key_hash))
    {
      cardano_protocol_param_update_ref(kvp->value);
      *protocol_param_update = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_proposed_param_updates_get_key_at(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  size_t                                  index,
  cardano_blake2b_hash_t**                genesis_delegate_key_hash)
{
  if (proposed_param_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (genesis_delegate_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(proposed_param_updates->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t* object = cardano_array_get(proposed_param_updates->array, index);

  cardano_proposed_param_updates_kvp_t* kvp = (cardano_proposed_param_updates_kvp_t*)((void*)object);

  cardano_blake2b_hash_ref(kvp->key);

  *genesis_delegate_key_hash = kvp->key;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposed_param_updates_get_value_at(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  size_t                                  index,
  cardano_protocol_param_update_t**       protocol_param_update)
{
  if (proposed_param_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(proposed_param_updates->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t* object = cardano_array_get(proposed_param_updates->array, index);

  cardano_proposed_param_updates_kvp_t* kvp = (cardano_proposed_param_updates_kvp_t*)((void*)object);

  cardano_protocol_param_update_ref(kvp->value);

  *protocol_param_update = kvp->value;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposed_param_updates_get_key_value_at(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  size_t                                  index,
  cardano_blake2b_hash_t**                genesis_delegate_key_hash,
  cardano_protocol_param_update_t**       protocol_param_update)
{
  if (proposed_param_updates == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (genesis_delegate_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(proposed_param_updates->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t* object = cardano_array_get(proposed_param_updates->array, index);

  cardano_proposed_param_updates_kvp_t* kvp = (cardano_proposed_param_updates_kvp_t*)((void*)object);

  cardano_blake2b_hash_ref(kvp->key);
  cardano_protocol_param_update_ref(kvp->value);

  *genesis_delegate_key_hash = kvp->key;
  *protocol_param_update     = kvp->value;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

void
cardano_proposed_param_updates_unref(cardano_proposed_param_updates_t** proposed_params_updates)
{
  if ((proposed_params_updates == NULL) || (*proposed_params_updates == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*proposed_params_updates)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *proposed_params_updates = NULL;
    return;
  }
}

void
cardano_proposed_param_updates_ref(cardano_proposed_param_updates_t* proposed_params_updates)
{
  if (proposed_params_updates == NULL)
  {
    return;
  }

  cardano_object_ref(&proposed_params_updates->base);
}

size_t
cardano_proposed_param_updates_refcount(const cardano_proposed_param_updates_t* proposed_params_updates)
{
  if (proposed_params_updates == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&proposed_params_updates->base);
}

void
cardano_proposed_param_updates_set_last_error(cardano_proposed_param_updates_t* proposed_params_updates, const char* message)
{
  cardano_object_set_last_error(&proposed_params_updates->base, message);
}

const char*
cardano_proposed_param_updates_get_last_error(const cardano_proposed_param_updates_t* proposed_params_updates)
{
  return cardano_object_get_last_error(&proposed_params_updates->base);
}
