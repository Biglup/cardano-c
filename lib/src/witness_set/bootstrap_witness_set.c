/**
 * \file bootstrap_witness_set.c
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
#include <cardano/witness_set/bootstrap_witness_set.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano bootstrap_witness_set list.
 */
typedef struct cardano_bootstrap_witness_set_t
{
    cardano_object_t base;
    cardano_array_t* array;
    bool             uses_tags;
    bool             preserves_duplicates;
} cardano_bootstrap_witness_set_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a bootstrap_witness_set list object.
 *
 * This function is responsible for properly deallocating a bootstrap_witness_set list object (`cardano_bootstrap_witness_set_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the bootstrap_witness_set object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_bootstrap_witness_set_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the bootstrap_witness_set
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_bootstrap_witness_set_deallocate(void* object)
{
  assert(object != NULL);

  cardano_bootstrap_witness_set_t* list = (cardano_bootstrap_witness_set_t*)object;

  if (list->array != NULL)
  {
    cardano_array_unref(&list->array);
  }

  _cardano_free(list);
}

/**
 * \brief Determines whether two bootstrap witnesses are for the same Byron address root.
 *
 * The address root of a bootstrap witness is the hash of its public key, chain code and attributes, so two bootstrap
 * witnesses are for the same address root when they have the same public key, chain code and attributes. Their
 * signatures are not compared.
 *
 * \param lhs The first bootstrap witness to compare.
 * \param rhs The second bootstrap witness to compare.
 * \return true if both witnesses are for the same address root, false otherwise.
 */
static bool
is_same_address_root(cardano_bootstrap_witness_t* lhs, cardano_bootstrap_witness_t* rhs)
{
  cardano_ed25519_public_key_t* lhs_vkey       = cardano_bootstrap_witness_get_vkey(lhs);
  cardano_ed25519_public_key_t* rhs_vkey       = cardano_bootstrap_witness_get_vkey(rhs);
  cardano_buffer_t*             lhs_chain_code = cardano_bootstrap_witness_get_chain_code(lhs);
  cardano_buffer_t*             rhs_chain_code = cardano_bootstrap_witness_get_chain_code(rhs);
  cardano_buffer_t*             lhs_attributes = cardano_bootstrap_witness_get_attributes(lhs);
  cardano_buffer_t*             rhs_attributes = cardano_bootstrap_witness_get_attributes(rhs);

  cardano_ed25519_public_key_unref(&lhs_vkey);
  cardano_ed25519_public_key_unref(&rhs_vkey);
  cardano_buffer_unref(&lhs_chain_code);
  cardano_buffer_unref(&rhs_chain_code);
  cardano_buffer_unref(&lhs_attributes);
  cardano_buffer_unref(&rhs_attributes);

  if ((lhs_vkey == NULL) || (rhs_vkey == NULL))
  {
    return false;
  }

  const size_t lhs_size = cardano_ed25519_public_key_get_bytes_size(lhs_vkey);
  const size_t rhs_size = cardano_ed25519_public_key_get_bytes_size(rhs_vkey);

  if ((lhs_size != rhs_size) || (memcmp(cardano_ed25519_public_key_get_data(lhs_vkey), cardano_ed25519_public_key_get_data(rhs_vkey), lhs_size) != 0))
  {
    return false;
  }

  return cardano_buffer_equals(lhs_chain_code, rhs_chain_code) && cardano_buffer_equals(lhs_attributes, rhs_attributes);
}

/**
 * \brief Determines whether a later witness of the bootstrap witness set is for the same address root as the witness
 * at the given index.
 *
 * \param bootstrap_witness_set The bootstrap witness set to search.
 * \param index The index of the witness to look for.
 * \return true if a witness after \p index is for the same address root, false otherwise.
 */
static bool
is_repeated_later(const cardano_bootstrap_witness_set_t* bootstrap_witness_set, const size_t index)
{
  cardano_object_t* item  = cardano_array_get(bootstrap_witness_set->array, index);
  bool              found = false;

  for (size_t i = index + 1U; (i < cardano_array_get_size(bootstrap_witness_set->array)) && !found; ++i)
  {
    cardano_object_t* other = cardano_array_get(bootstrap_witness_set->array, i);

    found = is_same_address_root((cardano_bootstrap_witness_t*)((void*)item), (cardano_bootstrap_witness_t*)((void*)other));

    cardano_object_unref(&other);
  }

  cardano_object_unref(&item);

  return found;
}

/**
 * \brief Determines whether the witness at the given index is part of the encoding of the bootstrap witness set.
 *
 * A bootstrap witness set decoded from CBOR encodes every witness it was decoded with. Otherwise, a witness followed
 * by another witness for the same address root is left out, so that the encoding holds one witness per address root
 * and, as in the ledger, the last one wins.
 *
 * \param bootstrap_witness_set The bootstrap witness set being encoded.
 * \param index The index of the witness.
 * \return true if the witness is written when the set is encoded, false otherwise.
 */
static bool
is_encoded(const cardano_bootstrap_witness_set_t* bootstrap_witness_set, const size_t index)
{
  return bootstrap_witness_set->preserves_duplicates || !is_repeated_later(bootstrap_witness_set, index);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_bootstrap_witness_set_new(cardano_bootstrap_witness_set_t** bootstrap_witness_set)
{
  if (bootstrap_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bootstrap_witness_set_t* list = _cardano_malloc(sizeof(cardano_bootstrap_witness_set_t));

  if (list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  list->base.ref_count     = 1;
  list->base.last_error[0] = '\0';
  list->base.deallocator   = cardano_bootstrap_witness_set_deallocate;

  list->array                = cardano_array_new(128);
  list->uses_tags            = true;
  list->preserves_duplicates = false;

  if (list->array == NULL)
  {
    _cardano_free(list);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *bootstrap_witness_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bootstrap_witness_set_from_cbor(cardano_cbor_reader_t* reader, cardano_bootstrap_witness_set_t** bootstrap_witness_set)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (bootstrap_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_bootstrap_witness_set_t* list   = NULL;
  cardano_error_t                  result = cardano_bootstrap_witness_set_new(&list);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_state_t state;

  if (cardano_cbor_reader_peek_state(reader, &state) != CARDANO_SUCCESS)
  {
    cardano_bootstrap_witness_set_unref(&list);
    return CARDANO_ERROR_DECODING;
  }

  list->uses_tags            = (state == CARDANO_CBOR_READER_STATE_TAG);
  list->preserves_duplicates = true;

  if (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    const cardano_error_t read_tag_result = cardano_cbor_validate_tag("bootstrap_witness_set", reader, CARDANO_CBOR_TAG_SET);

    if (read_tag_result != CARDANO_SUCCESS)
    {
      cardano_bootstrap_witness_set_unref(&list);
      return read_tag_result;
    }
  }

  int64_t length = 0;
  result         = cardano_cbor_reader_read_start_array(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_bootstrap_witness_set_unref(&list);
    return result;
  }

  state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_bootstrap_witness_set_unref(&list);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_ARRAY)
    {
      break;
    }

    cardano_bootstrap_witness_t* element = NULL;

    result = cardano_bootstrap_witness_from_cbor(reader, &element);

    if (result != CARDANO_SUCCESS)
    {
      cardano_bootstrap_witness_set_unref(&list);
      return result;
    }

    const size_t old_size = cardano_array_get_size(list->array);
    const size_t new_size = cardano_array_push(list->array, (cardano_object_t*)((void*)element));

    cardano_bootstrap_witness_unref(&element);

    if ((old_size + 1U) != new_size)
    {
      cardano_bootstrap_witness_set_unref(&list);
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  result = cardano_cbor_validate_end_array("bootstrap_witness_set", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_bootstrap_witness_set_unref(&list);
    return result;
  }

  *bootstrap_witness_set = list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bootstrap_witness_set_to_cbor(const cardano_bootstrap_witness_set_t* bootstrap_witness_set, cardano_cbor_writer_t* writer)
{
  if (bootstrap_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(bootstrap_witness_set->array != NULL);

  cardano_error_t result = CARDANO_SUCCESS;

  if (bootstrap_witness_set->uses_tags)
  {
    result = cardano_cbor_writer_write_tag(writer, CARDANO_CBOR_TAG_SET);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  const size_t array_size    = cardano_array_get_size(bootstrap_witness_set->array);
  size_t       encoded_count = 0U;

  for (size_t i = 0U; i < array_size; ++i)
  {
    if (is_encoded(bootstrap_witness_set, i))
    {
      ++encoded_count;
    }
  }

  result = cardano_cbor_writer_write_start_array(writer, (int64_t)encoded_count);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0U; i < array_size; ++i)
  {
    cardano_object_t* element = cardano_array_get(bootstrap_witness_set->array, i);

    if (element == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in bootstrap_witness_set list is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    if (is_encoded(bootstrap_witness_set, i))
    {
      result = cardano_bootstrap_witness_to_cbor((cardano_bootstrap_witness_t*)((void*)element), writer);
    }

    cardano_object_unref(&element);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return result;
}

cardano_error_t
cardano_bootstrap_witness_set_to_cip116_json(
  const cardano_bootstrap_witness_set_t* witnesses,
  cardano_json_writer_t*                 writer)
{
  if ((witnesses == NULL) || (writer == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_writer_write_start_array(writer);

  size_t          length = cardano_bootstrap_witness_set_get_length(witnesses);
  cardano_error_t error  = CARDANO_SUCCESS;

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_bootstrap_witness_t* witness = NULL;
    error                                = cardano_bootstrap_witness_set_get(witnesses, i, &witness);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }

    error = cardano_bootstrap_witness_to_cip116_json(witness, writer);
    cardano_bootstrap_witness_unref(&witness);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }
  }

  cardano_json_writer_write_end_array(writer);

  return error;
}

size_t
cardano_bootstrap_witness_set_get_length(const cardano_bootstrap_witness_set_t* bootstrap_witness_set)
{
  if (bootstrap_witness_set == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(bootstrap_witness_set->array);
}

cardano_error_t
cardano_bootstrap_witness_set_get(
  const cardano_bootstrap_witness_set_t* bootstrap_witness_set,
  size_t                                 index,
  cardano_bootstrap_witness_t**          element)
{
  if (bootstrap_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_object_t* object = cardano_array_get(bootstrap_witness_set->array, index);

  if (object == NULL)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  *element = (cardano_bootstrap_witness_t*)((void*)object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bootstrap_witness_set_add(cardano_bootstrap_witness_set_t* bootstrap_witness_set, cardano_bootstrap_witness_t* element)
{
  if (bootstrap_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (element == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ed25519_signature_t* sig = cardano_bootstrap_witness_get_signature(element);

  cardano_ed25519_signature_unref(&sig);

  if (sig == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  bool found = false;

  for (size_t i = 0U; i < cardano_array_get_size(bootstrap_witness_set->array); ++i)
  {
    cardano_object_t* item = cardano_array_get(bootstrap_witness_set->array, i);

    cardano_bootstrap_witness_t* witness = (cardano_bootstrap_witness_t*)((void*)item);
    cardano_object_unref(&item);

    if (is_same_address_root(witness, element))
    {
      cardano_error_t result = cardano_bootstrap_witness_set_signature(witness, sig);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      found = true;
    }
  }

  if (!found)
  {
    const size_t original_size = cardano_array_get_size(bootstrap_witness_set->array);
    const size_t new_size      = cardano_array_push(bootstrap_witness_set->array, (cardano_object_t*)((void*)element));

    if (new_size != (original_size + 1U))
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  bootstrap_witness_set->preserves_duplicates = false;

  return CARDANO_SUCCESS;
}

bool
cardano_bootstrap_witness_set_get_use_tag(const cardano_bootstrap_witness_set_t* bootstrap_witness_set)
{
  if (bootstrap_witness_set == NULL)
  {
    return false;
  }

  return bootstrap_witness_set->uses_tags;
}

cardano_error_t
cardano_bootstrap_witness_set_set_use_tag(cardano_bootstrap_witness_set_t* bootstrap_witness_set, const bool use_tag)
{
  if (bootstrap_witness_set == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  bootstrap_witness_set->uses_tags = use_tag;

  return CARDANO_SUCCESS;
}

void
cardano_bootstrap_witness_set_clear_cbor_cache(cardano_bootstrap_witness_set_t* bootstrap_witness_set)
{
  if (bootstrap_witness_set == NULL)
  {
    return;
  }

  bootstrap_witness_set->uses_tags            = true;
  bootstrap_witness_set->preserves_duplicates = false;
}

void
cardano_bootstrap_witness_set_unref(cardano_bootstrap_witness_set_t** bootstrap_witness_set)
{
  if ((bootstrap_witness_set == NULL) || (*bootstrap_witness_set == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*bootstrap_witness_set)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *bootstrap_witness_set = NULL;
    return;
  }
}

void
cardano_bootstrap_witness_set_ref(cardano_bootstrap_witness_set_t* bootstrap_witness_set)
{
  if (bootstrap_witness_set == NULL)
  {
    return;
  }

  cardano_object_ref(&bootstrap_witness_set->base);
}

size_t
cardano_bootstrap_witness_set_refcount(const cardano_bootstrap_witness_set_t* bootstrap_witness_set)
{
  if (bootstrap_witness_set == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&bootstrap_witness_set->base);
}

void
cardano_bootstrap_witness_set_set_last_error(cardano_bootstrap_witness_set_t* bootstrap_witness_set, const char* message)
{
  cardano_object_set_last_error(&bootstrap_witness_set->base, message);
}

const char*
cardano_bootstrap_witness_set_get_last_error(const cardano_bootstrap_witness_set_t* bootstrap_witness_set)
{
  return cardano_object_get_last_error(&bootstrap_witness_set->base);
}
