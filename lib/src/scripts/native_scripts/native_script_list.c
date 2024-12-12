/**
 * \file native_script_list.c
 *
 * \author angel.castillo
 * \date   Jun 2, 2024
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
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/native_scripts/native_script_list.h>

#include "../../allocators.h"
#include "../../cbor/cbor_validation.h"
#include "../../collections/array.h"

#include <assert.h>
#include <cardano/json/json_object.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano native script list.
 */
typedef struct cardano_native_script_list_t
{
    cardano_object_t base;
    cardano_array_t* array;
    bool             use_indefinite_encoding;
} cardano_native_script_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a native script list object.
 *
 * This function is responsible for properly deallocating a native script list object (`cardano_native_script_list_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the native_script_list object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_native_script_list_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the native_script_list
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_native_script_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_native_script_list_t* list = (cardano_native_script_list_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_native_script_list_new(cardano_native_script_list_t** native_script_list)
{
  if (native_script_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_list_t* list = _cardano_malloc(sizeof(cardano_native_script_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_native_script_list_deallocate;

  list->array                   = cardano_array_new(128);
  list->use_indefinite_encoding = false;

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *native_script_list = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_list_from_cbor(cardano_cbor_reader_t* reader, cardano_native_script_list_t** native_script_list)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_list_t* list   = NULL;
  cardano_error_t               result = cardano_native_script_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_native_script_list_unref(&list);
    return result;
  }

  list->use_indefinite_encoding = length < 0;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_native_script_list_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_native_script_t* element = NULL;

    result = cardano_native_script_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_native_script_list_unref(&list);
      return result;
    }

    const size_t old_size = cardano_array_get_size(list->array);
    const size_t new_size = cardano_array_push(list->array, (cardano_object_t*)((void*)element));

    cardano_native_script_unref(&element);

    if ((old_size + 1U) != new_size)
    {
      cardano_native_script_list_unref(&list);
      return result;
    }
  }

  result = cardano_cbor_validate_end_array("native_script_list", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_native_script_list_unref(&list);
    return result;
  }

  *native_script_list = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_list_from_json(
  const char*                    json,
  size_t                         json_size,
  cardano_native_script_list_t** native_script_list)
{
  if (json == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (json_size == 0U)
  {
    return CARDANO_ERROR_INVALID_JSON;
  }

  if (native_script_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_object_t* root = cardano_json_object_parse(json, json_size);

  if (root == NULL)
  {
    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_native_script_list_t* list   = NULL;
  cardano_error_t               result = cardano_native_script_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_object_unref(&root);
    return result;
  }

  cardano_json_object_t* array = NULL;

  if (!cardano_json_object_get_ex(root, "scripts", 7, &array))
  {
    cardano_native_script_list_unref(&list);
    cardano_json_object_unref(&root);

    return CARDANO_ERROR_INVALID_JSON;
  }

  if (cardano_json_object_get_type(array) != CARDANO_JSON_OBJECT_TYPE_ARRAY)
  {
    cardano_native_script_list_unref(&list);
    cardano_json_object_unref(&root);

    return CARDANO_ERROR_INVALID_JSON;
  }

  const size_t array_size = cardano_json_object_array_get_length(array);

  for (size_t i = 0U; i < array_size; ++i)
  {
    cardano_json_object_t* element = cardano_json_object_array_get_ex(array, i);

    if (element == NULL)
    {
      cardano_native_script_list_unref(&list);
      cardano_json_object_unref(&root);

      return CARDANO_ERROR_INVALID_JSON;
    }

    cardano_native_script_t* script = NULL;

    size_t      json_length  = 0;
    const char* element_json = cardano_json_object_to_json_string(element, CARDANO_JSON_FORMAT_COMPACT, &json_length);

    result = cardano_native_script_from_json(element_json, json_length - 1U, &script);

    if (result != CARDANO_SUCCESS)
    {
      cardano_native_script_list_unref(&list);
      cardano_json_object_unref(&root);

      return result;
    }

    result = cardano_native_script_list_add(list, script);

    cardano_native_script_unref(&script);

    if (result != CARDANO_SUCCESS)
    {
      cardano_native_script_list_unref(&list);
      cardano_json_object_unref(&root);

      return result;
    }
  }

  cardano_json_object_unref(&root);

  *native_script_list = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_list_to_cbor(const cardano_native_script_list_t* native_script_list, cardano_cbor_writer_t* writer)
{
  if (native_script_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(native_script_list->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  if (native_script_list->use_indefinite_encoding)
  {
    result = cardano_cbor_writer_write_start_array(writer, -1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    };
  }
  else
  {
    size_t array_size = cardano_array_get_size(native_script_list->array);
    result            = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  for (size_t i = 0; i < cardano_array_get_size(native_script_list->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(native_script_list->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in native script list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    result = cardano_native_script_to_cbor((cardano_native_script_t*)((void*)element), writer);

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (native_script_list->use_indefinite_encoding)
  {
    result = cardano_cbor_writer_write_end_array(writer);
  }

  return result;
}

size_t
cardano_native_script_list_get_length(const cardano_native_script_list_t* native_script_list)
{
  if (native_script_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(native_script_list->array);
}

cardano_error_t
cardano_native_script_list_get(
  const cardano_native_script_list_t* native_script_list,
  size_t                              index,
  cardano_native_script_t**           element)
{
  if (native_script_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(native_script_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_native_script_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_list_add(cardano_native_script_list_t* native_script_list, cardano_native_script_t* element)
{
  if (native_script_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(native_script_list->array);
  const size_t new_size      = cardano_array_push(native_script_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

bool
cardano_native_script_list_equals(const cardano_native_script_list_t* lhs, const cardano_native_script_list_t* rhs)
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
    cardano_native_script_t* lhs_element = (cardano_native_script_t*)((void*)cardano_array_get(lhs->array, i));
    cardano_native_script_t* rhs_element = (cardano_native_script_t*)((void*)cardano_array_get(rhs->array, i));

    cardano_native_script_unref(&lhs_element);
    cardano_native_script_unref(&rhs_element);

    if (!cardano_native_script_equals(lhs_element, rhs_element))
    {
      return false;
    }
  }

  return true;
}

void
cardano_native_script_list_unref(cardano_native_script_list_t** native_script_list)
{
  if ((native_script_list == NULL) || (*native_script_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*native_script_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *native_script_list = NULL;
    return;
  }
}

void
cardano_native_script_list_ref(cardano_native_script_list_t* native_script_list)
{
  if (native_script_list == NULL)
  {
    return;
  }

  cardano_object_ref(&native_script_list->base);
}

size_t
cardano_native_script_list_refcount(const cardano_native_script_list_t* native_script_list)
{
  if (native_script_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&native_script_list->base);
}

void
cardano_native_script_list_set_last_error(cardano_native_script_list_t* native_script_list, const char* message)
{
  cardano_object_set_last_error(&native_script_list->base, message);
}

const char*
cardano_native_script_list_get_last_error(const cardano_native_script_list_t* native_script_list)
{
  return cardano_object_get_last_error(&native_script_list->base);
}
