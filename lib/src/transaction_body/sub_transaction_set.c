/**
 * \file sub_transaction_set.c
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
 *
 * Copyright 2026 Biglup Labs
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
#include <cardano/transaction_body/sub_transaction_set.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano sub transaction set.
 */
typedef struct cardano_sub_transaction_set_t
{
    cardano_object_t base;
    cardano_array_t* array;
    bool             use_tags;
} cardano_sub_transaction_set_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a sub transaction set object.
 *
 * This function is responsible for properly deallocating a sub transaction set object (`cardano_sub_transaction_set_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the sub_transaction_set object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_sub_transaction_set_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the sub_transaction_set
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_sub_transaction_set_deallocate(void* object)
{
  assert(object != NULL);

  cardano_sub_transaction_set_t* list = (cardano_sub_transaction_set_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_sub_transaction_set_new(cardano_sub_transaction_set_t** sub_transaction_set)
{
  if (sub_transaction_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_sub_transaction_set_t* list = _cardano_malloc(sizeof(cardano_sub_transaction_set_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_sub_transaction_set_deallocate;

  list->array    = cardano_array_new(128);
  list->use_tags = true;

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *sub_transaction_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_sub_transaction_set_from_cbor(cardano_cbor_reader_t* reader, cardano_sub_transaction_set_t** sub_transaction_set)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (sub_transaction_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_sub_transaction_set_t* list   = NULL;
  cardano_error_t                result = cardano_sub_transaction_set_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_state_t state;

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_set_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  list->use_tags = state == CARDANO_CBOR_READER_STATE_TAG;

  if (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    const cardano_error_t read_tag_result = cardano_cbor_validate_tag("sub_transaction_set", reader, CARDANO_CBOR_TAG_SET);

    if (read_tag_result != CARDANO_SUCCESS)
    {
      cardano_sub_transaction_set_unref(&list);
      return read_tag_result;
    }
  }

  int64_t length = 0;
  result         = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_set_unref(&list);
    return result;
  }

  result = cardano_cbor_reader_peek_state(reader, &state);

  if (result != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_set_unref(&list);
    return result;
  }

  if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    cardano_cbor_reader_set_last_error(reader, "There was an error decoding 'sub_transaction_set', the set must not be empty.");
    cardano_sub_transaction_set_unref(&list);

    return CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE;
  }

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_sub_transaction_set_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_sub_transaction_t* element = NULL;

    result = cardano_sub_transaction_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_sub_transaction_set_unref(&list);
      return result;
    }

    result = cardano_sub_transaction_set_add(list, element);

    cardano_sub_transaction_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_reader_set_last_error(reader, "There was an error decoding 'sub_transaction_set', the set must not contain duplicated elements.");
      cardano_sub_transaction_set_unref(&list);

      return result;
    }
  }

  result = cardano_cbor_validate_end_array("sub_transaction_set", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_sub_transaction_set_unref(&list);
    return result;
  }

  *sub_transaction_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_sub_transaction_set_to_cbor(const cardano_sub_transaction_set_t* sub_transaction_set, cardano_cbor_writer_t* writer)
{
  if (sub_transaction_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(sub_transaction_set->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  if (sub_transaction_set->use_tags)
  {
    result = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_SET);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  const size_t array_size = cardano_array_get_size(sub_transaction_set->array);

  result = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(sub_transaction_set->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(sub_transaction_set->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in sub_transaction_set list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    result = cardano_sub_transaction_to_cbor((const cardano_sub_transaction_t*)((const void*)element), writer);

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

size_t
cardano_sub_transaction_set_get_length(const cardano_sub_transaction_set_t* sub_transaction_set)
{
  if (sub_transaction_set == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(sub_transaction_set->array);
}

cardano_error_t
cardano_sub_transaction_set_get(
  const cardano_sub_transaction_set_t* sub_transaction_set,
  size_t                               index,
  cardano_sub_transaction_t**          element)
{
  if (sub_transaction_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(sub_transaction_set->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_sub_transaction_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_sub_transaction_set_add(cardano_sub_transaction_set_t* sub_transaction_set, cardano_sub_transaction_t* element)
{
  if (sub_transaction_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_t* id = cardano_sub_transaction_get_id(element);

  if (id == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0U; i < cardano_array_get_size(sub_transaction_set->array); ++i)
  {
    cardano_object_t* object = cardano_array_get(sub_transaction_set->array, i);

    cardano_blake2b_hash_t* member_id = cardano_sub_transaction_get_id((cardano_sub_transaction_t*)((void*)object));

    cardano_object_unref(&object);

    if (member_id == NULL)
    {
      cardano_blake2b_hash_unref(&id);
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    const bool duplicated = cardano_blake2b_hash_equals(member_id, id);

    cardano_blake2b_hash_unref(&member_id);

    if (duplicated)
    {
      cardano_blake2b_hash_unref(&id);
      return CARDANO_ERROR_DUPLICATED_KEY;
    }
  }

  cardano_blake2b_hash_unref(&id);

  const size_t original_size = cardano_array_get_size(sub_transaction_set->array);
  const size_t new_size      = cardano_array_push(sub_transaction_set->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_sub_transaction_set_find_by_id(
  const cardano_sub_transaction_set_t* sub_transaction_set,
  const cardano_blake2b_hash_t*        id,
  cardano_sub_transaction_t**          element)
{
  if (sub_transaction_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0U; i < cardano_array_get_size(sub_transaction_set->array); ++i)
  {
    cardano_object_t* object = cardano_array_get(sub_transaction_set->array, i);

    cardano_blake2b_hash_t* member_id = cardano_sub_transaction_get_id((cardano_sub_transaction_t*)((void*)object));

    if (member_id == NULL)
    {
      cardano_object_unref(&object);
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    const bool found = cardano_blake2b_hash_equals(member_id, id);

    cardano_blake2b_hash_unref(&member_id);

    if (found)
    {
      *element = (cardano_sub_transaction_t*)((void*)object);

      return CARDANO_SUCCESS;
    }

    cardano_object_unref(&object);
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

bool
cardano_sub_transaction_set_is_tagged(const cardano_sub_transaction_set_t* sub_transaction_set)
{
  if (sub_transaction_set == NULL)
  {
    return false;
  }

  return sub_transaction_set->use_tags;
}

cardano_error_t
cardano_sub_transaction_set_set_use_tag(cardano_sub_transaction_set_t* sub_transaction_set, const bool use_tag)
{
  if (sub_transaction_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  sub_transaction_set->use_tags = use_tag;

  return CARDANO_SUCCESS;
}

void
cardano_sub_transaction_set_unref(cardano_sub_transaction_set_t** sub_transaction_set)
{
  if ((sub_transaction_set == NULL) || (*sub_transaction_set == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*sub_transaction_set)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *sub_transaction_set = NULL;
    return;
  }
}

void
cardano_sub_transaction_set_ref(cardano_sub_transaction_set_t* sub_transaction_set)
{
  if (sub_transaction_set == NULL)
  {
    return;
  }

  cardano_object_ref(&sub_transaction_set->base);
}

size_t
cardano_sub_transaction_set_refcount(const cardano_sub_transaction_set_t* sub_transaction_set)
{
  if (sub_transaction_set == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&sub_transaction_set->base);
}

void
cardano_sub_transaction_set_set_last_error(cardano_sub_transaction_set_t* sub_transaction_set, const char* message)
{
  cardano_object_set_last_error(&sub_transaction_set->base, message);
}

const char*
cardano_sub_transaction_set_get_last_error(const cardano_sub_transaction_set_t* sub_transaction_set)
{
  return cardano_object_get_last_error(&sub_transaction_set->base);
}
