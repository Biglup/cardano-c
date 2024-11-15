/**
 * \file redeemer_list.c
 *
 * \author angel.castillo
 * \date   Sep 21, 2024
 *
 * Copyright 2024 Biglup labs
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
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano redeemer list.
 */
typedef struct cardano_redeemer_list_t
{
    cardano_object_t  base;
    cardano_array_t*  array;
    cardano_buffer_t* cbor_cache;
} cardano_redeemer_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a redeemer_list list object.
 *
 * This function is responsible for properly deallocating a redeemer_list list object (`cardano_redeemer_list_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the redeemer_list object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_redeemer_list_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the redeemer_list
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_redeemer_list_deallocate(void* object)
{
  assert(object != NULL);

  cardano_redeemer_list_t* list = (cardano_redeemer_list_t*)object;

  cardano_array_unref(&list->array);
  cardano_buffer_unref(&list->cbor_cache);

  _cardano_free(list);
}

/**
 * \brief Compares two cardano_object_t objects based on their redeemer key.
 *
 * This function casts the cardano_object_t objects to cardano_redeemer_t
 * and compares their hashes using the cardano_redeemer_compare function.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context Unused.
 *
 * \return A negative value if the redeemer of lhs is less than the redeemer of rhs, zero if they are equal,
 *         and a positive value if the redeemer of lhs is greater than the redeemer of rhs.
 */
static int32_t
compare_by_key(const cardano_object_t* lhs, const cardano_object_t* rhs, void* context)
{
  assert(lhs != NULL);
  assert(rhs != NULL);

  CARDANO_UNUSED(context);

  const cardano_redeemer_t* lhs_redeemer = (const cardano_redeemer_t*)((const void*)lhs);
  const cardano_redeemer_t* rhs_redeemer = (const cardano_redeemer_t*)((const void*)rhs);

  const uint64_t               lhs_index = cardano_redeemer_get_index(lhs_redeemer);
  const uint64_t               rhs_index = cardano_redeemer_get_index(rhs_redeemer);
  const cardano_redeemer_tag_t lhs_tag   = cardano_redeemer_get_tag(lhs_redeemer);
  const cardano_redeemer_tag_t rhs_tag   = cardano_redeemer_get_tag(rhs_redeemer);

  if (lhs_tag < rhs_tag)
  {
    return -1;
  }

  if (lhs_tag > rhs_tag)
  {
    return 1;
  }

  if (lhs_index < rhs_index)
  {
    return -1;
  }

  if (lhs_index > rhs_index)
  {
    return 1;
  }

  return 0;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_redeemer_list_new(cardano_redeemer_list_t** redeemer_list)
{
  if (redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_redeemer_list_t* list = _cardano_malloc(sizeof(cardano_redeemer_list_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_redeemer_list_deallocate;

  list->array      = cardano_array_new(128);
  list->cbor_cache = NULL;

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *redeemer_list = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_redeemer_list_from_cbor(cardano_cbor_reader_t* reader, cardano_redeemer_list_t** redeemer_list)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_redeemer_list_t* list   = NULL;
  cardano_error_t          result = cardano_redeemer_list_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_t* reader_copy = NULL;
  cardano_error_t        copy_result = cardano_cbor_reader_clone(reader, &reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    cardano_redeemer_list_unref(&list);

    return copy_result;
  }

  copy_result = cardano_cbor_reader_read_encoded_value(reader_copy, &list->cbor_cache);
  cardano_cbor_reader_unref(&reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    cardano_redeemer_list_unref(&list);

    *redeemer_list = NULL;
    return copy_result;
  }

  cardano_cbor_reader_state_t state;

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_redeemer_list_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  if (state == CARDANO_CBOR_READER_STATE_START_MAP)
  {
    int64_t map_size = 0;

    result = cardano_cbor_reader_read_start_map(reader, &map_size);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&list);
      return result;
    }

    CARDANO_UNUSED(map_size);

    while (state != CARDANO_CBOR_READER_STATE_END_MAP)
    {
      result = cardano_cbor_reader_peek_state(reader, &state);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      if (state == CARDANO_CBOR_READER_STATE_END_MAP)
      {
        break;
      }

      int64_t key_array = 0;

      result = cardano_cbor_reader_read_start_array(reader, &key_array);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      if (key_array != 2)
      {
        cardano_redeemer_list_unref(&list);
        cardano_cbor_reader_set_last_error(reader, "Invalid key array size in redeemer_list");

        return CARDANO_ERROR_DECODING;
      }

      uint64_t tag   = 0;
      uint64_t index = 0;

      result = cardano_cbor_reader_read_uint(reader, &tag);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      result = cardano_cbor_reader_read_uint(reader, &index);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      result = cardano_cbor_validate_end_array("redeemer_list key", reader);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      int64_t value_array = 0;

      result = cardano_cbor_reader_read_start_array(reader, &value_array);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      if (value_array != 2)
      {
        cardano_redeemer_list_unref(&list);
        cardano_cbor_reader_set_last_error(reader, "Invalid value array size in redeemer_list");

        return CARDANO_ERROR_DECODING;
      }

      cardano_plutus_data_t* data     = NULL;
      cardano_ex_units_t*    ex_units = NULL;

      result = cardano_plutus_data_from_cbor(reader, &data);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      result = cardano_ex_units_from_cbor(reader, &ex_units);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_unref(&data);
        cardano_redeemer_list_unref(&list);
        return result;
      }

      result = cardano_cbor_validate_end_array("redeemer_list value", reader);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_data_unref(&data);
        cardano_ex_units_unref(&ex_units);
        cardano_redeemer_list_unref(&list);
        return result;
      }

      cardano_redeemer_t* redeemer = NULL;

      result = cardano_redeemer_new(tag, index, data, ex_units, &redeemer);
      cardano_plutus_data_unref(&data);
      cardano_ex_units_unref(&ex_units);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }

      result = cardano_redeemer_list_add(list, redeemer);
      cardano_redeemer_unref(&redeemer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_redeemer_list_unref(&list);
        return result;
      }
    }

    result = cardano_cbor_validate_end_map("redeemer_list", reader);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&list);
      return result;
    }

    cardano_array_sort(list->array, compare_by_key, NULL);

    *redeemer_list = list;

    return CARDANO_SUCCESS;
  }

  int64_t array_size = 0;

  result = cardano_cbor_reader_read_start_array(reader, &array_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_redeemer_list_unref(&list);
    return result;
  }

  CARDANO_UNUSED(array_size);

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_redeemer_list_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_redeemer_t* redeemer = NULL;

    result = cardano_redeemer_from_cbor(reader, &redeemer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&list);
      return result;
    }

    result = cardano_redeemer_list_add(list, redeemer);
    cardano_redeemer_unref(&redeemer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&list);
      return result;
    }
  }

  cardano_array_sort(list->array, compare_by_key, NULL);

  *redeemer_list = list;

  return cardano_cbor_validate_end_array("redeemer_list", reader);
}

cardano_error_t
cardano_redeemer_list_to_cbor(const cardano_redeemer_list_t* redeemer_list, cardano_cbor_writer_t* writer)
{
  if (redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer_list->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(redeemer_list->cbor_cache), cardano_buffer_get_size(redeemer_list->cbor_cache));
  }

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_start_map(writer, (int64_t)cardano_redeemer_list_get_length(redeemer_list));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(redeemer_list); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    result = cardano_redeemer_list_get(redeemer_list, i, &redeemer);
    cardano_redeemer_unref(&redeemer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_start_array(writer, 2);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, cardano_redeemer_get_tag(redeemer));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, cardano_redeemer_get_index(redeemer));

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_start_array(writer, 2);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_plutus_data_t* data = cardano_redeemer_get_data(redeemer);
    cardano_plutus_data_unref(&data);

    result = cardano_plutus_data_to_cbor(data, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(redeemer);
    cardano_ex_units_unref(&ex_units);

    result = cardano_ex_units_to_cbor(ex_units, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_redeemer_list_get_length(const cardano_redeemer_list_t* redeemer_list)
{
  if (redeemer_list == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(redeemer_list->array);
}

cardano_error_t
cardano_redeemer_list_get(
  const cardano_redeemer_list_t* redeemer_list,
  size_t                         index,
  cardano_redeemer_t**           element)
{
  if (redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(redeemer_list->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_redeemer_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_redeemer_list_add(cardano_redeemer_list_t* redeemer_list, cardano_redeemer_t* element)
{
  if (redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  const size_t original_size = cardano_array_get_size(redeemer_list->array);
  const size_t new_size      = cardano_array_push(redeemer_list->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(redeemer_list->array, compare_by_key, NULL);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_redeemer_list_set_ex_units(
  cardano_redeemer_list_t*     redeemer_list,
  const cardano_redeemer_tag_t tag,
  const uint64_t               index,
  const uint64_t               mem,
  const uint64_t               steps)
{
  if (redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(redeemer_list); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    cardano_error_t result = cardano_redeemer_list_get(redeemer_list, i, &redeemer);
    cardano_redeemer_unref(&redeemer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if ((cardano_redeemer_get_tag(redeemer) == tag) && (cardano_redeemer_get_index(redeemer) == index))
    {
      cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(redeemer);
      cardano_ex_units_unref(&ex_units);

      if (ex_units == NULL)
      {
        return CARDANO_ERROR_POINTER_IS_NULL;
      }

      cardano_error_t set_result = cardano_ex_units_set_memory(ex_units, mem);

      if (set_result != CARDANO_SUCCESS)
      {
        return set_result;
      }

      set_result = cardano_ex_units_set_cpu_steps(ex_units, steps);

      return set_result;
    }
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}

cardano_error_t
cardano_redeemer_list_clone(
  cardano_redeemer_list_t*  redeemer_list,
  cardano_redeemer_list_t** cloned_redeemer_list)
{
  if (redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cloned_redeemer_list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_redeemer_list_to_cbor(redeemer_list, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  cardano_buffer_t* buffer = NULL;
  result                   = cardano_cbor_writer_encode_in_buffer(writer, &buffer);

  cardano_cbor_writer_unref(&writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer));
  cardano_buffer_unref(&buffer);

  if (reader == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_redeemer_list_from_cbor(reader, cloned_redeemer_list);

  cardano_cbor_reader_unref(&reader);

  cardano_redeemer_list_clear_cbor_cache(*cloned_redeemer_list);

  cardano_array_sort(redeemer_list->array, compare_by_key, NULL);

  return result;
}

void
cardano_redeemer_list_clear_cbor_cache(cardano_redeemer_list_t* redeemer_list)
{
  if (redeemer_list == NULL)
  {
    return;
  }

  cardano_buffer_unref(&redeemer_list->cbor_cache);
  redeemer_list->cbor_cache = NULL;

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(redeemer_list); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    cardano_error_t result = cardano_redeemer_list_get(redeemer_list, i, &redeemer);

    cardano_redeemer_unref(&redeemer);

    if (result == CARDANO_SUCCESS)
    {
      cardano_redeemer_clear_cbor_cache(redeemer);
    }
  }
}

void
cardano_redeemer_list_unref(cardano_redeemer_list_t** redeemer_list)
{
  if ((redeemer_list == NULL) || (*redeemer_list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*redeemer_list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *redeemer_list = NULL;
    return;
  }
}

void
cardano_redeemer_list_ref(cardano_redeemer_list_t* redeemer_list)
{
  if (redeemer_list == NULL)
  {
    return;
  }

  cardano_object_ref(&redeemer_list->base);
}

size_t
cardano_redeemer_list_refcount(const cardano_redeemer_list_t* redeemer_list)
{
  if (redeemer_list == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&redeemer_list->base);
}

void
cardano_redeemer_list_set_last_error(cardano_redeemer_list_t* redeemer_list, const char* message)
{
  cardano_object_set_last_error(&redeemer_list->base, message);
}

const char*
cardano_redeemer_list_get_last_error(const cardano_redeemer_list_t* redeemer_list)
{
  return cardano_object_get_last_error(&redeemer_list->base);
}
