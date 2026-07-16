/**
 * \file guard_set.c
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
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

#include <cardano/common/guard_set.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano guard set.
 */
typedef struct cardano_guard_set_t
{
    cardano_object_t base;
    cardano_array_t* array;
    bool             use_tags;
} cardano_guard_set_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a guard set object.
 *
 * This function is responsible for properly deallocating a guard set object (`cardano_guard_set_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the guard_set object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_guard_set_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the guard_set
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_guard_set_deallocate(void* object)
{
  assert(object != NULL);

  cardano_guard_set_t* list = (cardano_guard_set_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/**
 * \brief Determines whether a guard set must be encoded as an ordered set of credentials.
 *
 * A guard set is encoded as a set of bare key hashes while every member is a key hash
 * credential; the presence of at least one script hash credential switches the encoding
 * to the ordered set of credentials form.
 *
 * \param[in] array Pointer to the array holding the members of the guard set.
 *
 * \return \c true if at least one member is a script hash credential; \c false otherwise.
 */
static bool
uses_credential_form(const cardano_array_t* array)
{
  assert(array != NULL);

  bool result = false;

  for (size_t i = 0U; i < cardano_array_get_size(array); ++i)
  {
    cardano_object_t* element = cardano_array_get(array, i);

    assert(element != NULL);

    cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

    const cardano_error_t get_type_result = cardano_credential_get_type((const cardano_credential_t*)((const void*)element), &type);

    assert(get_type_result == CARDANO_SUCCESS);
    CARDANO_UNUSED(get_type_result);

    cardano_object_unref(&element);

    if (type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH)
    {
      result = true;
      break;
    }
  }

  return result;
}

/**
 * \brief Reads a bare key hash guard from a CBOR stream.
 *
 * Reads a 28 byte key hash byte string and wraps it as a key hash credential.
 *
 * \param[in,out] reader The CBOR reader positioned at the key hash byte string.
 * \param[out] element On success, points to a newly created key hash credential.
 *
 * \return \ref CARDANO_SUCCESS if the key hash was successfully read, or an appropriate
 *         error code indicating the failure reason.
 */
static cardano_error_t
read_key_hash_guard(cardano_cbor_reader_t* reader, cardano_credential_t** element)
{
  assert(reader != NULL);
  assert(element != NULL);

  cardano_buffer_t* key_hash = NULL;

  cardano_error_t result = cardano_cbor_validate_byte_string_of_size(
    "guard_set",
    reader,
    &key_hash,
    CARDANO_BLAKE2B_HASH_SIZE_224);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_credential_from_hash_bytes(
    cardano_buffer_get_data(key_hash),
    cardano_buffer_get_size(key_hash),
    CARDANO_CREDENTIAL_TYPE_KEY_HASH,
    element);

  cardano_buffer_unref(&key_hash);

  return result;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_guard_set_new(cardano_guard_set_t** guard_set)
{
  if (guard_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_guard_set_t* list = _cardano_malloc(sizeof(cardano_guard_set_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_guard_set_deallocate;

  list->array    = cardano_array_new(128);
  list->use_tags = true;

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *guard_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_guard_set_from_cbor(cardano_cbor_reader_t* reader, cardano_guard_set_t** guard_set)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (guard_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_guard_set_t* list   = NULL;
  cardano_error_t      result = cardano_guard_set_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_state_t state;

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_guard_set_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  list->use_tags = state == CARDANO_CBOR_READER_STATE_TAG;

  if (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    const cardano_error_t read_tag_result = cardano_cbor_validate_tag("guard_set", reader, CARDANO_CBOR_TAG_SET);

    if (read_tag_result != CARDANO_SUCCESS)
    {
      cardano_guard_set_unref(&list);
      return read_tag_result;
    }
  }

  int64_t length = 0;
  result         = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_guard_set_unref(&list);
    return result;
  }

  result = cardano_cbor_reader_peek_state(reader, &state);

  if (result != CARDANO_SUCCESS)
  {
    cardano_guard_set_unref(&list);
    return result;
  }

  if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    cardano_cbor_reader_set_last_error(reader, "There was an error decoding 'guard_set', the set must not be empty.");
    cardano_guard_set_unref(&list);

    return CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE;
  }

  const bool is_credential_form = state == CARDANO_CBOR_READER_STATE_START_ARRAY;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_guard_set_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_credential_t* element = NULL;

    if (is_credential_form)
    {
      result = cardano_credential_from_cbor(reader, &element);
    }
    else
    {
      result = read_key_hash_guard(reader, &element);
    }

    if (result != CARDANO_SUCCESS)
    {
      cardano_guard_set_unref(&list);
      return result;
    }

    result = cardano_guard_set_add(list, element);

    cardano_credential_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_reader_set_last_error(reader, "There was an error decoding 'guard_set', the set must not contain duplicated elements.");
      cardano_guard_set_unref(&list);

      return result;
    }
  }

  result = cardano_cbor_validate_end_array("guard_set", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_guard_set_unref(&list);
    return result;
  }

  *guard_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_guard_set_to_cbor(const cardano_guard_set_t* guard_set, cardano_cbor_writer_t* writer)
{
  if (guard_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(guard_set->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  if (guard_set->use_tags)
  {
    result = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_SET);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  const bool   credential_form = uses_credential_form(guard_set->array);
  const size_t array_size      = cardano_array_get_size(guard_set->array);

  result = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(guard_set->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(guard_set->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in guard_set list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    const cardano_credential_t* credential = (const cardano_credential_t*)((const void*)element);

    if (credential_form)
    {
      result = cardano_credential_to_cbor(credential, writer);
    }
    else
    {
      result = cardano_cbor_writer_write_bytestring(
        writer,
        cardano_credential_get_hash_bytes(credential),
        cardano_credential_get_hash_bytes_size(credential));
    }

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

size_t
cardano_guard_set_get_length(const cardano_guard_set_t* guard_set)
{
  if (guard_set == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(guard_set->array);
}

cardano_error_t
cardano_guard_set_get(
  const cardano_guard_set_t* guard_set,
  size_t                     index,
  cardano_credential_t**     element)
{
  if (guard_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(guard_set->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_credential_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_guard_set_add(cardano_guard_set_t* guard_set, cardano_credential_t* element)
{
  if (guard_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0U; i < cardano_array_get_size(guard_set->array); ++i)
  {
    cardano_object_t* object = cardano_array_get(guard_set->array, i);

    const bool duplicated = cardano_credential_equals((const cardano_credential_t*)((const void*)object), element);

    cardano_object_unref(&object);

    if (duplicated)
    {
      return CARDANO_ERROR_DUPLICATED_KEY;
    }
  }

  const size_t original_size = cardano_array_get_size(guard_set->array);
  const size_t new_size      = cardano_array_push(guard_set->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

bool
cardano_guard_set_is_tagged(const cardano_guard_set_t* guard_set)
{
  if (guard_set == NULL)
  {
    return false;
  }

  return guard_set->use_tags;
}

void
cardano_guard_set_unref(cardano_guard_set_t** guard_set)
{
  if ((guard_set == NULL) || (*guard_set == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*guard_set)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *guard_set = NULL;
    return;
  }
}

void
cardano_guard_set_ref(cardano_guard_set_t* guard_set)
{
  if (guard_set == NULL)
  {
    return;
  }

  cardano_object_ref(&guard_set->base);
}

size_t
cardano_guard_set_refcount(const cardano_guard_set_t* guard_set)
{
  if (guard_set == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&guard_set->base);
}

void
cardano_guard_set_set_last_error(cardano_guard_set_t* guard_set, const char* message)
{
  cardano_object_set_last_error(&guard_set->base, message);
}

const char*
cardano_guard_set_get_last_error(const cardano_guard_set_t* guard_set)
{
  return cardano_object_get_last_error(&guard_set->base);
}
