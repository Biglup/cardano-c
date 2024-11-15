/**
 * \file plutus_list.c
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

#include <cardano/object.h>
#include <cardano/plutus_data/plutus_list.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <cardano/plutus_data/plutus_data.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano plutus list.
 */
typedef struct cardano_plutus_list_t
{
    cardano_object_t  base;
    cardano_array_t*  array;
    cardano_buffer_t* cbor_cache;
} cardano_plutus_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a plutus list object.
 *
 * This function is responsible for properly deallocating a plutus list object (`cardano_plutus_list_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the plutus_list object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_plutus_list_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the plutus_list
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_plutus_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_plutus_list_t* list = (cardano_plutus_list_t*)object;

  cardano_array_unref(&list->array);
  cardano_buffer_unref(&list->cbor_cache);

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_plutus_list_new(cardano_plutus_list_t** plutus_list)
{
  if (plutus_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_list_t* list = _cardano_malloc(sizeof(cardano_plutus_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_plutus_list_deallocate;
  list->cbor_cache         = NULL;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *plutus_list = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_list_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_list_t** plutus_list)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t*      cbor_cache  = NULL;
  cardano_cbor_reader_t* reader_copy = NULL;
  cardano_error_t        copy_result = cardano_cbor_reader_clone(reader, &reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    return copy_result;
  }

  copy_result = cardano_cbor_reader_read_encoded_value(reader_copy, &cbor_cache);
  cardano_cbor_reader_unref(&reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    *plutus_list = NULL;
    return copy_result;
  }

  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    return result;
  }

  list->cbor_cache = cbor_cache;

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_list_unref(&list);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_plutus_data_t* element = NULL;

    result = cardano_plutus_data_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&list);
      return result;
    }

    const size_t old_size = cardano_array_get_size(list->array);
    const size_t new_size = cardano_array_push(list->array, (cardano_object_t*)((void*)element));

    cardano_plutus_data_unref(&element);

    if ((old_size + 1U) != new_size)
    {
      cardano_plutus_list_unref(&list);
      return result;
    }
  }

  result = cardano_cbor_validate_end_array("plutus_list", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_list_unref(&list);
    return result;
  }

  *plutus_list = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_list_to_cbor(const cardano_plutus_list_t* plutus_list, cardano_cbor_writer_t* writer)
{
  if (plutus_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_list->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(plutus_list->cbor_cache), cardano_buffer_get_size(plutus_list->cbor_cache));
  }

  assert(plutus_list->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  size_t array_size = cardano_array_get_size(plutus_list->array);

  if (array_size > 0U)
  {
    result = cardano_cbor_writer_write_start_array(writer, -1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    };
  }
  else
  {
    result = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  for (size_t i = 0; i < cardano_array_get_size(plutus_list->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(plutus_list->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in plutus list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    result = cardano_plutus_data_to_cbor((cardano_plutus_data_t*)((void*)element), writer);

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (array_size > 0U)
  {
    result = cardano_cbor_writer_write_end_array(writer);
  }

  return result;
}

size_t
cardano_plutus_list_get_length(const cardano_plutus_list_t* plutus_list)
{
  if (plutus_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(plutus_list->array);
}

cardano_error_t
cardano_plutus_list_get(
  const cardano_plutus_list_t* plutus_list,
  size_t                       index,
  cardano_plutus_data_t**      element)
{
  if (plutus_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(plutus_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_plutus_data_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_list_add(cardano_plutus_list_t* plutus_list, cardano_plutus_data_t* element)
{
  if (plutus_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(plutus_list->array);
  const size_t new_size      = cardano_array_push(plutus_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

bool
cardano_plutus_list_equals(const cardano_plutus_list_t* lhs, const cardano_plutus_list_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if ((lhs == NULL) || (rhs == NULL))
  {
    return false;
  }

  if (cardano_array_get_size(lhs->array) != cardano_array_get_size(rhs->array))
  {
    return false;
  }

  for (size_t i = 0; i < cardano_array_get_size(lhs->array); ++i)
  {
    cardano_plutus_data_t* lhs_element = (cardano_plutus_data_t*)((void*)cardano_array_get(lhs->array, i));
    cardano_plutus_data_t* rhs_element = (cardano_plutus_data_t*)((void*)cardano_array_get(rhs->array, i));

    cardano_plutus_data_unref(&lhs_element);
    cardano_plutus_data_unref(&rhs_element);

    if (!cardano_plutus_data_equals(lhs_element, rhs_element))
    {
      return false;
    }
  }

  return true;
}

void
cardano_plutus_list_clear_cbor_cache(cardano_plutus_list_t* plutus_list)
{
  if (plutus_list == NULL)
  {
    return;
  }

  cardano_buffer_unref(&plutus_list->cbor_cache);

  for (size_t i = 0; i < cardano_array_get_size(plutus_list->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(plutus_list->array, i);
    cardano_object_unref(&element);

    cardano_plutus_data_clear_cbor_cache((cardano_plutus_data_t*)(void*)element);
  }

  plutus_list->cbor_cache = NULL;
}

void
cardano_plutus_list_unref(cardano_plutus_list_t** plutus_list)
{
  if ((plutus_list == NULL) || (*plutus_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*plutus_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *plutus_list = NULL;
    return;
  }
}

void
cardano_plutus_list_ref(cardano_plutus_list_t* plutus_list)
{
  if (plutus_list == NULL)
  {
    return;
  }

  cardano_object_ref(&plutus_list->base);
}

size_t
cardano_plutus_list_refcount(const cardano_plutus_list_t* plutus_list)
{
  if (plutus_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&plutus_list->base);
}

void
cardano_plutus_list_set_last_error(cardano_plutus_list_t* plutus_list, const char* message)
{
  cardano_object_set_last_error(&plutus_list->base, message);
}

const char*
cardano_plutus_list_get_last_error(const cardano_plutus_list_t* plutus_list)
{
  return cardano_object_get_last_error(&plutus_list->base);
}
