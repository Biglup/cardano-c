/**
 * \file pool_owners.c
 *
 * \author angel.castillo
 * \date   jun 26, 2024
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
#include <cardano/pool_params/pool_owners.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano pool_owners list.
 */
typedef struct cardano_pool_owners_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_pool_owners_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a pool_owners list object.
 *
 * This function is responsible for properly deallocating a pool_owners list object (`cardano_pool_owners_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the pool_owners object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_pool_owners_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the pool_owners
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_pool_owners_deallocate(void* object)
{
  assert(object != NULL);

  cardano_pool_owners_t* list = (cardano_pool_owners_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/**
 * \brief Compares two cardano_object_t objects based on their hash.
 *
 * This function casts the cardano_object_t objects to cardano_blake2b_hash_t
 * and compares their hashes using the cardano_blake2b_hash_compare function.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context Unused.
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

  const cardano_blake2b_hash_t* lhs_hash = (const cardano_blake2b_hash_t*)((const void*)lhs);
  const cardano_blake2b_hash_t* rhs_hash = (const cardano_blake2b_hash_t*)((const void*)rhs);

  return cardano_blake2b_hash_compare(lhs_hash, rhs_hash);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_pool_owners_new(cardano_pool_owners_t** pool_owners)
{
  if (pool_owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_pool_owners_t* list = _cardano_malloc(sizeof(cardano_pool_owners_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_pool_owners_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *pool_owners = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_owners_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_owners_t** pool_owners)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pool_owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_pool_owners_t* list   = NULL;
  cardano_error_t        result = cardano_pool_owners_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_state_t state;

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_pool_owners_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  if (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    const cardano_error_t read_tag_result = cardano_cbor_validate_tag("pool_owners", reader, CARDANO_CBOR_TAG_SET);

    if (read_tag_result != CARDANO_SUCCESS)
    {
      cardano_pool_owners_unref(&list);
      return read_tag_result;
    }
  }

  int64_t length = 0;
  result         = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_pool_owners_unref(&list);
    return result;
  }

  state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_pool_owners_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_blake2b_hash_t* element = NULL;

    result = cardano_blake2b_hash_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_pool_owners_unref(&list);
      return result;
    }

    const size_t old_size = cardano_array_get_size(list->array);
    const size_t new_size = cardano_array_push(list->array, (cardano_object_t*)((void*)element));

    cardano_blake2b_hash_unref(&element);

    if ((old_size + 1U) != new_size)
    {
      cardano_pool_owners_unref(&list);
      return result;
    }
  }

  result = cardano_cbor_validate_end_array("pool_owners", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_pool_owners_unref(&list);
    return result;
  }

  cardano_array_sort(list->array, compare_by_hash, NULL);

  *pool_owners = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_owners_to_cbor(const cardano_pool_owners_t* pool_owners, cardano_cbor_writer_t* writer)
{
  if (pool_owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(pool_owners->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_SET);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  size_t array_size = cardano_array_get_size(pool_owners->array);
  result            = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(pool_owners->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(pool_owners->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in pool_owners list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    result = cardano_blake2b_hash_to_cbor((cardano_blake2b_hash_t*)((void*)element), writer);

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

size_t
cardano_pool_owners_get_length(const cardano_pool_owners_t* pool_owners)
{
  if (pool_owners == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(pool_owners->array);
}

cardano_error_t
cardano_pool_owners_get(
  const cardano_pool_owners_t* pool_owners,
  size_t                       index,
  cardano_blake2b_hash_t**     element)
{
  if (pool_owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(pool_owners->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_blake2b_hash_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_owners_add(cardano_pool_owners_t* pool_owners, cardano_blake2b_hash_t* element)
{
  if (pool_owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(pool_owners->array);
  const size_t new_size      = cardano_array_push(pool_owners->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(pool_owners->array, compare_by_hash, NULL);

  return CARDANO_SUCCESS;
}

void
cardano_pool_owners_unref(cardano_pool_owners_t** pool_owners)
{
  if ((pool_owners == NULL) || (*pool_owners == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*pool_owners)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *pool_owners = NULL;
    return;
  }
}

void
cardano_pool_owners_ref(cardano_pool_owners_t* pool_owners)
{
  if (pool_owners == NULL)
  {
    return;
  }

  cardano_object_ref(&pool_owners->base);
}

size_t
cardano_pool_owners_refcount(const cardano_pool_owners_t* pool_owners)
{
  if (pool_owners == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&pool_owners->base);
}

void
cardano_pool_owners_set_last_error(cardano_pool_owners_t* pool_owners, const char* message)
{
  cardano_object_set_last_error(&pool_owners->base, message);
}

const char*
cardano_pool_owners_get_last_error(const cardano_pool_owners_t* pool_owners)
{
  return cardano_object_get_last_error(&pool_owners->base);
}
