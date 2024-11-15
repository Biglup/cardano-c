/**
 * \file vkey_witness_set.c
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
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
#include <cardano/witness_set/vkey_witness_set.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano vkey_witness_set list.
 */
typedef struct cardano_vkey_witness_set_t
{
    cardano_object_t base;
    cardano_array_t* array;
    bool             uses_tags;
} cardano_vkey_witness_set_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a vkey_witness_set list object.
 *
 * This function is responsible for properly deallocating a vkey_witness_set list object (`cardano_vkey_witness_set_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the vkey_witness_set object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_vkey_witness_set_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the vkey_witness_set
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_vkey_witness_set_deallocate(void* object)
{
  assert(object != NULL);

  cardano_vkey_witness_set_t* list = (cardano_vkey_witness_set_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/**
 * \brief Determines if a vkey_witness public key is equal to a given public key.
 *
 * \param item The vkey_witness object to compare.
 * \param context The public key to compare against.
 * \return true if the public key is equal to the given public key, false otherwise.
 */
static bool
is_pub_key_equal(const cardano_object_t* item, const void* context)
{
  const cardano_vkey_witness_t*       vkey_witness = (const cardano_vkey_witness_t*)((const void*)item);
  const cardano_ed25519_public_key_t* pubkey       = (const cardano_ed25519_public_key_t*)context;

  return cardano_vkey_witness_has_public_key(vkey_witness, pubkey);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_vkey_witness_set_new(cardano_vkey_witness_set_t** vkey_witness_set)
{
  if (vkey_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_vkey_witness_set_t* list = _cardano_malloc(sizeof(cardano_vkey_witness_set_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_vkey_witness_set_deallocate;
  list->uses_tags          = true;

  list->array = cardano_array_new(128);

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *vkey_witness_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_vkey_witness_set_from_cbor(cardano_cbor_reader_t* reader, cardano_vkey_witness_set_t** vkey_witness_set)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vkey_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_vkey_witness_set_t* list   = NULL;
  cardano_error_t             result = cardano_vkey_witness_set_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_state_t state;

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_vkey_witness_set_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  list->uses_tags = (state == CARDANO_CBOR_READER_STATE_TAG);

  if (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    const cardano_error_t read_tag_result = cardano_cbor_validate_tag("vkey_witness_set", reader, CARDANO_CBOR_TAG_SET);

    if (read_tag_result != CARDANO_SUCCESS)
    {
      cardano_vkey_witness_set_unref(&list);
      return read_tag_result;
    }
  }

  int64_t length = 0;
  result         = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_vkey_witness_set_unref(&list);
    return result;
  }

  state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_vkey_witness_set_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_vkey_witness_t* element = NULL;

    result = cardano_vkey_witness_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_vkey_witness_set_unref(&list);
      return result;
    }

    const size_t old_size = cardano_array_get_size(list->array);
    const size_t new_size = cardano_array_push(list->array, (cardano_object_t*)((void*)element));

    cardano_vkey_witness_unref(&element);

    if ((old_size + 1U) != new_size)
    {
      cardano_vkey_witness_set_unref(&list);
      return result;
    }
  }

  result = cardano_cbor_validate_end_array("vkey_witness_set", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_vkey_witness_set_unref(&list);
    return result;
  }

  *vkey_witness_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_vkey_witness_set_to_cbor(const cardano_vkey_witness_set_t* vkey_witness_set, cardano_cbor_writer_t* writer)
{
  if (vkey_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(vkey_witness_set->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  if (vkey_witness_set->uses_tags)
  {
    result = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_SET);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  size_t array_size = cardano_array_get_size(vkey_witness_set->array);
  result            = cardano_cbor_writer_write_start_array(writer, (int64_t)array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(vkey_witness_set->array); ++i)
  {
    cardano_object_t* element = cardano_array_get(vkey_witness_set->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in vkey_witness_set list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    result = cardano_vkey_witness_to_cbor((cardano_vkey_witness_t*)((void*)element), writer);

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

size_t
cardano_vkey_witness_set_get_length(const cardano_vkey_witness_set_t* vkey_witness_set)
{
  if (vkey_witness_set == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(vkey_witness_set->array);
}

cardano_error_t
cardano_vkey_witness_set_get(
  const cardano_vkey_witness_set_t* vkey_witness_set,
  size_t                            index,
  cardano_vkey_witness_t**          element)
{
  if (vkey_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(vkey_witness_set->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_vkey_witness_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_vkey_witness_set_add(cardano_vkey_witness_set_t* vkey_witness_set, cardano_vkey_witness_t* element)
{
  if (vkey_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ed25519_public_key_t* vkey = cardano_vkey_witness_get_vkey(element);
  cardano_ed25519_signature_t*  sig  = cardano_vkey_witness_get_signature(element);

  cardano_ed25519_signature_unref(&sig);
  cardano_ed25519_public_key_unref(&vkey);

  if ((vkey == NULL) || (sig == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_vkey_witness_t* found_elem = (cardano_vkey_witness_t*)((void*)cardano_array_find(vkey_witness_set->array, is_pub_key_equal, (void*)vkey));

  cardano_vkey_witness_unref(&found_elem);

  if (found_elem != NULL)
  {
    cardano_error_t result = cardano_vkey_witness_set_signature(found_elem, sig);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    return CARDANO_SUCCESS;
  }

  const size_t original_size = cardano_array_get_size(vkey_witness_set->array);
  const size_t new_size      = cardano_array_push(vkey_witness_set->array, (cardano_object_t*)((void*)element));

  assert((original_size + 1U) == new_size);

  CARDANO_UNUSED(original_size);
  CARDANO_UNUSED(new_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_vkey_witness_set_apply(
  cardano_vkey_witness_set_t* vkey_witness_set,
  cardano_vkey_witness_set_t* new_vkey_witnesses)
{
  if (vkey_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (new_vkey_witnesses == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  for (size_t i = 0; i < cardano_array_get_size(new_vkey_witnesses->array); ++i)
  {
    cardano_vkey_witness_t* element = NULL;

    cardano_error_t result = cardano_vkey_witness_set_get(new_vkey_witnesses, i, &element);
    cardano_vkey_witness_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_vkey_witness_set_add(vkey_witness_set, element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

bool
cardano_vkey_witness_set_get_use_tag(const cardano_vkey_witness_set_t* vkey_witness_set)
{
  if (vkey_witness_set == NULL)
  {
    return false;
  }

  return vkey_witness_set->uses_tags;
}

cardano_error_t
cardano_vkey_witness_set_set_use_tag(cardano_vkey_witness_set_t* vkey_witness_set, const bool use_tag)
{
  if (vkey_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  vkey_witness_set->uses_tags = use_tag;

  return CARDANO_SUCCESS;
}

void
cardano_vkey_witness_set_unref(cardano_vkey_witness_set_t** vkey_witness_set)
{
  if ((vkey_witness_set == NULL) || (*vkey_witness_set == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*vkey_witness_set)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *vkey_witness_set = NULL;
    return;
  }
}

void
cardano_vkey_witness_set_ref(cardano_vkey_witness_set_t* vkey_witness_set)
{
  if (vkey_witness_set == NULL)
  {
    return;
  }

  cardano_object_ref(&vkey_witness_set->base);
}

size_t
cardano_vkey_witness_set_refcount(const cardano_vkey_witness_set_t* vkey_witness_set)
{
  if (vkey_witness_set == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&vkey_witness_set->base);
}

void
cardano_vkey_witness_set_set_last_error(cardano_vkey_witness_set_t* vkey_witness_set, const char* message)
{
  cardano_object_set_last_error(&vkey_witness_set->base, message);
}

const char*
cardano_vkey_witness_set_get_last_error(const cardano_vkey_witness_set_t* vkey_witness_set)
{
  return cardano_object_get_last_error(&vkey_witness_set->base);
}
