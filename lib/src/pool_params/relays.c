/**
 * \file relays.c
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
#include <cardano/pool_params/relay.h>
#include <cardano/pool_params/relays.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano relays list.
 */
typedef struct cardano_relays_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_relays_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a relays list object.
 *
 * This function is responsible for properly deallocating a relays list object (`cardano_relays_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the relays object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_relays_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the relays
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_relays_deallocate(void* object)
{
  assert(object != NULL);

  cardano_relays_t* list = (cardano_relays_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_relays_new(cardano_relays_t** relays)
{
  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_relays_t* list = _cardano_malloc(sizeof(cardano_relays_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_relays_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *relays = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relays_from_cbor(cardano_cbor_reader_t* reader, cardano_relays_t** relays)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_relays_t* list   = NULL;
  cardano_error_t   result = cardano_relays_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_relays_unref(&list);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_relays_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_relay_t* element = NULL;

    result = cardano_relay_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_relays_unref(&list);
      return result;
    }

    const size_t old_size = cardano_array_get_size(list->array);
    const size_t new_size = cardano_array_push(list->array, (cardano_object_t*)((void*)element));

    cardano_relay_unref(&element);

    if ((old_size + 1U) != new_size)
    {
      cardano_relays_unref(&list);
      return result;
    }
  }

  result = cardano_cbor_validate_end_array("relays", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_relays_unref(&list);
    return result;
  }

  *relays = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relays_to_cbor(const cardano_relays_t* relays, cardano_cbor_writer_t* writer)
{
  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(relays->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  size_t array_size = cardano_array_get_size(relays->array);
  result            = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(relays->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(relays->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in relays list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    result = cardano_relay_to_cbor((cardano_relay_t*)((void*)element), writer);

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

size_t
cardano_relays_get_length(const cardano_relays_t* relays)
{
  if (relays == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(relays->array);
}

cardano_error_t
cardano_relays_get(
  const cardano_relays_t* relays,
  size_t                  index,
  cardano_relay_t**       element)
{
  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(relays->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_relay_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relays_add(cardano_relays_t* relays, cardano_relay_t* element)
{
  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(relays->array);
  const size_t new_size      = cardano_array_push(relays->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

void
cardano_relays_unref(cardano_relays_t** relays)
{
  if ((relays == NULL) || (*relays == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*relays)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *relays = NULL;
    return;
  }
}

void
cardano_relays_ref(cardano_relays_t* relays)
{
  if (relays == NULL)
  {
    return;
  }

  cardano_object_ref(&relays->base);
}

size_t
cardano_relays_refcount(const cardano_relays_t* relays)
{
  if (relays == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&relays->base);
}

void
cardano_relays_set_last_error(cardano_relays_t* relays, const char* message)
{
  cardano_object_set_last_error(&relays->base, message);
}

const char*
cardano_relays_get_last_error(const cardano_relays_t* relays)
{
  return cardano_object_get_last_error(&relays->base);
}
