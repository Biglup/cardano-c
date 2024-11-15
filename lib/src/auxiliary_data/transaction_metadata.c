/**
 * \file transaction_metadata.c
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

#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/metadatum_label_list.h>
#include <cardano/auxiliary_data/transaction_metadata.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/set.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano transaction metadata map.
 */
typedef struct cardano_transaction_metadata_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_transaction_metadata_t;

/**
 * \brief Represents a Cardano transaction metadata map key value pair.
 */
typedef struct cardano_transaction_metadata_kvp_t
{
    cardano_object_t     base;
    uint64_t             key;
    cardano_metadatum_t* value;
} cardano_transaction_metadata_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a transaction metadata map object.
 *
 * This function is responsible for properly deallocating a transaction metadata map object (`cardano_transaction_metadata_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the transaction_metadata object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_transaction_metadata_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the transaction_metadata
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_transaction_metadata_deallocate(void* object)
{
  assert(object != NULL);

  cardano_transaction_metadata_t* map = (cardano_transaction_metadata_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a transaction metadata map key value pair object.
 *
 * This function is responsible for properly deallocating a transaction metadata map key value pair object (`cardano_transaction_metadata_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the transaction_metadata object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_transaction_metadata_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the transaction_metadata
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_transaction_metadata_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_transaction_metadata_kvp_t* map = (cardano_transaction_metadata_kvp_t*)object;

  cardano_metadatum_unref(&map->value);

  _cardano_free(map);
}

/**
 * \brief Compares two cardano_transaction_metadata_kvp_t objects based on their reward_address.
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

  const cardano_transaction_metadata_kvp_t* lhs_kvp = (const cardano_transaction_metadata_kvp_t*)((const void*)lhs);
  const cardano_transaction_metadata_kvp_t* rhs_kvp = (const cardano_transaction_metadata_kvp_t*)((const void*)rhs);

  return (lhs_kvp->key < rhs_kvp->key) ? -1 : 1;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_transaction_metadata_new(cardano_transaction_metadata_t** transaction_metadata)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_metadata_t* map = _cardano_malloc(sizeof(cardano_transaction_metadata_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_transaction_metadata_deallocate;

  map->array = cardano_array_new(32);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *transaction_metadata = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_metadata_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_metadata_t** transaction_metadata)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_metadata_t* map    = NULL;
  cardano_error_t                 result = cardano_transaction_metadata_new(&map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_metadata_unref(&map);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_metadata_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    uint64_t             key   = 0;
    cardano_metadatum_t* value = NULL;

    result = cardano_cbor_reader_read_uint(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_metadata_unref(&map);
      return result;
    }

    result = cardano_metadatum_from_cbor(reader, &value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_transaction_metadata_unref(&map);
      return result;
    }

    cardano_transaction_metadata_kvp_t* kvp = _cardano_malloc(sizeof(cardano_transaction_metadata_kvp_t));

    if (kvp == NULL)
    {
      cardano_metadatum_unref(&value);
      cardano_transaction_metadata_unref(&map);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    kvp->base.ref_count     = 0;
    kvp->base.last_error[0] = '\0';
    kvp->base.deallocator   = cardano_transaction_metadata_kvp_deallocate;
    kvp->key                = key;
    kvp->value              = value;

    const size_t old_size = cardano_array_get_size(map->array);
    const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

    assert((old_size + 1U) == new_size);

    CARDANO_UNUSED(old_size);
    CARDANO_UNUSED(new_size);

    cardano_array_sort(map->array, compare_by_value, NULL);
  }

  result = cardano_cbor_validate_end_map("transaction_metadata", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_metadata_unref(&map);
    return result;
  }

  *transaction_metadata = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_metadata_to_cbor(const cardano_transaction_metadata_t* transaction_metadata, cardano_cbor_writer_t* writer)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  size_t map_size = cardano_array_get_size(transaction_metadata->array);
  result          = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(transaction_metadata->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(transaction_metadata->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in transaction metadata map is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_transaction_metadata_kvp_t* kvp_data = (cardano_transaction_metadata_kvp_t*)((void*)kvp);

    result = cardano_cbor_writer_write_uint(writer, kvp_data->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    result = cardano_metadatum_to_cbor(kvp_data->value, writer);

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
cardano_transaction_metadata_get_length(const cardano_transaction_metadata_t* transaction_metadata)
{
  if (transaction_metadata == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(transaction_metadata->array);
}

cardano_error_t
cardano_transaction_metadata_get(
  cardano_transaction_metadata_t* transaction_metadata,
  const uint64_t                  key,
  cardano_metadatum_t**           element)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0; i < cardano_array_get_size(transaction_metadata->array); ++i)
  {
    cardano_object_t*                   object = cardano_array_get(transaction_metadata->array, i);
    cardano_transaction_metadata_kvp_t* kvp    = (cardano_transaction_metadata_kvp_t*)((void*)object);

    if (kvp->key == key)
    {
      cardano_metadatum_ref(kvp->value);
      *element = kvp->value;

      cardano_object_unref(&object);
      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_transaction_metadata_insert(
  cardano_transaction_metadata_t* transaction_metadata,
  const uint64_t                  key,
  cardano_metadatum_t*            value)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_metadata_kvp_t* kvp = _cardano_malloc(sizeof(cardano_transaction_metadata_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_transaction_metadata_kvp_deallocate;
  kvp->key                = key;
  kvp->value              = value;

  cardano_metadatum_ref(value);

  const size_t old_size = cardano_array_get_size(transaction_metadata->array);
  const size_t new_size = cardano_array_push(transaction_metadata->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(transaction_metadata->array, compare_by_value, NULL);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_metadata_get_keys(
  cardano_transaction_metadata_t*  transaction_metadata,
  cardano_metadatum_label_list_t** keys)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (keys == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_metadatum_label_list_t* list = NULL;

  cardano_error_t result = cardano_metadatum_label_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(transaction_metadata->array); ++i)
  {
    cardano_object_t*                   object = cardano_array_get(transaction_metadata->array, i);
    cardano_transaction_metadata_kvp_t* kvp    = (cardano_transaction_metadata_kvp_t*)((void*)object);

    result = cardano_metadatum_label_list_add(list, kvp->key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_metadatum_label_list_unref(&list);
      cardano_object_unref(&object);
      return result;
    }

    cardano_object_unref(&object);
  }

  *keys = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_metadata_get_key_at(
  const cardano_transaction_metadata_t* transaction_metadata,
  const size_t                          index,
  uint64_t*                             metadatum_label)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (metadatum_label == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(transaction_metadata->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                   object = cardano_array_get(transaction_metadata->array, index);
  cardano_transaction_metadata_kvp_t* kvp    = (cardano_transaction_metadata_kvp_t*)((void*)object);

  cardano_object_unref(&object);

  *metadatum_label = kvp->key;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_metadata_get_value_at(
  const cardano_transaction_metadata_t* transaction_metadata,
  size_t                                index,
  cardano_metadatum_t**                 amount)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(transaction_metadata->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                   object = cardano_array_get(transaction_metadata->array, index);
  cardano_transaction_metadata_kvp_t* kvp    = (cardano_transaction_metadata_kvp_t*)((void*)object);

  cardano_object_unref(&object);

  cardano_metadatum_ref(kvp->value);
  *amount = kvp->value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_metadata_get_key_value_at(
  const cardano_transaction_metadata_t* transaction_metadata,
  const size_t                          index,
  uint64_t*                             metadatum_label,
  cardano_metadatum_t**                 metadatum)
{
  if (transaction_metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (metadatum_label == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (metadatum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(transaction_metadata->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t*                   object = cardano_array_get(transaction_metadata->array, index);
  cardano_transaction_metadata_kvp_t* kvp    = (cardano_transaction_metadata_kvp_t*)((void*)object);

  cardano_object_unref(&object);

  cardano_metadatum_ref(kvp->value);

  *metadatum_label = kvp->key;
  *metadatum       = kvp->value;

  return CARDANO_SUCCESS;
}

void
cardano_transaction_metadata_unref(cardano_transaction_metadata_t** transaction_metadata)
{
  if ((transaction_metadata == NULL) || (*transaction_metadata == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*transaction_metadata)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *transaction_metadata = NULL;
    return;
  }
}

void
cardano_transaction_metadata_ref(cardano_transaction_metadata_t* transaction_metadata)
{
  if (transaction_metadata == NULL)
  {
    return;
  }

  cardano_object_ref(&transaction_metadata->base);
}

size_t
cardano_transaction_metadata_refcount(const cardano_transaction_metadata_t* transaction_metadata)
{
  if (transaction_metadata == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&transaction_metadata->base);
}

void
cardano_transaction_metadata_set_last_error(cardano_transaction_metadata_t* transaction_metadata, const char* message)
{
  cardano_object_set_last_error(&transaction_metadata->base, message);
}

const char*
cardano_transaction_metadata_get_last_error(const cardano_transaction_metadata_t* transaction_metadata)
{
  return cardano_object_get_last_error(&transaction_metadata->base);
}
