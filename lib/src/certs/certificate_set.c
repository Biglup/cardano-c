/**
 * \file certificate_set.c
 *
 * \author angel.castillo
 * \date   Sept 17, 2024
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

#include <cardano/certs/certificate_set.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano certificate set.
 */
typedef struct cardano_certificate_set_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_certificate_set_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a certificate set list object.
 *
 * This function is responsible for properly deallocating a certificate_set list object (`cardano_certificate_set_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the certificate_set object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_certificate_set_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the certificate_set
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_certificate_set_deallocate(void* object)
{
  assert(object != NULL);

  cardano_certificate_set_t* list = (cardano_certificate_set_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_certificate_set_new(cardano_certificate_set_t** certificate_set)
{
  if (certificate_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_set_t* list = _cardano_malloc(sizeof(cardano_certificate_set_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_certificate_set_deallocate;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *certificate_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_set_from_cbor(cardano_cbor_reader_t* reader, cardano_certificate_set_t** certificate_set)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (certificate_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_certificate_set_t* list   = NULL;
  cardano_error_t            result = cardano_certificate_set_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_state_t state;

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_certificate_set_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  if (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    const cardano_error_t read_tag_result = cardano_cbor_validate_tag("certificate_set", reader, CARDANO_CBOR_TAG_SET);

    if (read_tag_result != CARDANO_SUCCESS)
    {
      cardano_certificate_set_unref(&list);
      return read_tag_result;
    }
  }

  int64_t length = 0;
  result         = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_certificate_set_unref(&list);
    return result;
  }

  state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_certificate_set_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_certificate_t* element = NULL;

    result = cardano_certificate_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_certificate_set_unref(&list);
      return result;
    }

    const size_t old_size = cardano_array_get_size(list->array);
    const size_t new_size = cardano_array_push(list->array, (cardano_object_t*)((void*)element));

    cardano_certificate_unref(&element);

    if ((old_size + 1U) != new_size)
    {
      cardano_certificate_set_unref(&list);
      return result;
    }
  }

  result = cardano_cbor_validate_end_array("certificate_set", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_certificate_set_unref(&list);
    return result;
  }

  *certificate_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_set_to_cbor(const cardano_certificate_set_t* certificate_set, cardano_cbor_writer_t* writer)
{
  if (certificate_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(certificate_set->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_SET);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  size_t array_size = cardano_array_get_size(certificate_set->array);
  result            = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(certificate_set->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(certificate_set->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in certificate_set list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    result = cardano_certificate_to_cbor((cardano_certificate_t*)((void*)element), writer);

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

size_t
cardano_certificate_set_get_length(const cardano_certificate_set_t* certificate_set)
{
  if (certificate_set == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(certificate_set->array);
}

cardano_error_t
cardano_certificate_set_get(
  const cardano_certificate_set_t* certificate_set,
  size_t                           index,
  cardano_certificate_t**          element)
{
  if (certificate_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(certificate_set->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_certificate_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_certificate_set_add(cardano_certificate_set_t* certificate_set, cardano_certificate_t* element)
{
  if (certificate_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(certificate_set->array);
  const size_t new_size      = cardano_array_push(certificate_set->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

void
cardano_certificate_set_unref(cardano_certificate_set_t** certificate_set)
{
  if ((certificate_set == NULL) || (*certificate_set == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*certificate_set)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *certificate_set = NULL;
    return;
  }
}

void
cardano_certificate_set_ref(cardano_certificate_set_t* certificate_set)
{
  if (certificate_set == NULL)
  {
    return;
  }

  cardano_object_ref(&certificate_set->base);
}

size_t
cardano_certificate_set_refcount(const cardano_certificate_set_t* certificate_set)
{
  if (certificate_set == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&certificate_set->base);
}

void
cardano_certificate_set_set_last_error(cardano_certificate_set_t* certificate_set, const char* message)
{
  cardano_object_set_last_error(&certificate_set->base, message);
}

const char*
cardano_certificate_set_get_last_error(const cardano_certificate_set_t* certificate_set)
{
  return cardano_object_get_last_error(&certificate_set->base);
}
