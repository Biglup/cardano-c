/**
 * \file redeemer.c
 *
 * \author angel.castillo
 * \date   Sep 21, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t REDEEMER_EMBEDDED_GROUP_SIZE = 4;

/* STRUCTURES ****************************************************************/

/**
 * The Redeemer is an argument provided to a Plutus smart contract (script) when
 * you are attempting to redeem a UTxO that's protected by that script.
 */
typedef struct cardano_redeemer_t
{
    cardano_object_t       base;
    cardano_redeemer_tag_t tag;
    uint64_t               index;
    cardano_plutus_data_t* data;
    cardano_ex_units_t*    execution_units;
    cardano_buffer_t*      cbor_cache;
} cardano_redeemer_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a redeemer object.
 *
 * This function is responsible for properly deallocating a redeemer object (`cardano_redeemer_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the redeemer object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_redeemer_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the redeemer
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_redeemer_deallocate(void* object)
{
  assert(object != NULL);

  cardano_redeemer_t* redeemer = (cardano_redeemer_t*)object;

  cardano_plutus_data_unref(&redeemer->data);
  cardano_ex_units_unref(&redeemer->execution_units);
  cardano_buffer_unref(&redeemer->cbor_cache);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_redeemer_new(
  const cardano_redeemer_tag_t tag,
  const uint64_t               index,
  cardano_plutus_data_t*       data,
  cardano_ex_units_t*          ex_units,
  cardano_redeemer_t**         redeemer)
{
  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (ex_units == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *redeemer = (cardano_redeemer_t*)_cardano_malloc(sizeof(cardano_redeemer_t));

  if (*redeemer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*redeemer)->base.deallocator   = cardano_redeemer_deallocate;
  (*redeemer)->base.ref_count     = 1;
  (*redeemer)->base.last_error[0] = '\0';

  (*redeemer)->tag             = tag;
  (*redeemer)->index           = index;
  (*redeemer)->data            = data;
  (*redeemer)->execution_units = ex_units;
  (*redeemer)->cbor_cache      = NULL;

  cardano_plutus_data_ref(data);
  cardano_ex_units_ref(ex_units);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_redeemer_from_cbor(cardano_cbor_reader_t* reader, cardano_redeemer_t** redeemer)
{
  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
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
    *redeemer = NULL;
    return copy_result;
  }

  static const char* validator_name = "redeemer";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)REDEEMER_EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    *redeemer = NULL;
    return expect_array_result;
  }

  uint64_t              tag              = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "tag",
    reader,
    &tag,
    CARDANO_REDEEMER_TAG_SPEND,
    CARDANO_REDEEMER_TAG_PROPOSING);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    *redeemer = NULL;
    return read_uint_result;
  }

  uint64_t              index             = 0U;
  const cardano_error_t read_minor_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "index",
    reader,
    &index,
    0,
    UINT64_MAX);

  if (read_minor_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    *redeemer = NULL;
    return read_minor_result;
  }

  cardano_plutus_data_t* data     = NULL;
  cardano_ex_units_t*    ex_units = NULL;

  cardano_error_t read_data_result = cardano_plutus_data_from_cbor(reader, &data);

  if (read_data_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    *redeemer = NULL;
    return read_data_result;
  }

  cardano_error_t read_ex_units_result = cardano_ex_units_from_cbor(reader, &ex_units);

  if (read_ex_units_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    cardano_plutus_data_unref(&data);
    *redeemer = NULL;
    return read_ex_units_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    cardano_plutus_data_unref(&data);
    cardano_ex_units_unref(&ex_units);
    *redeemer = NULL;

    return expect_end_array_result;
  }

  cardano_error_t new_redeemer_result = cardano_redeemer_new((cardano_redeemer_tag_t)tag, index, data, ex_units, redeemer);

  cardano_plutus_data_unref(&data);
  cardano_ex_units_unref(&ex_units);

  if (new_redeemer_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);

    *redeemer = NULL;

    return new_redeemer_result;
  }

  (*redeemer)->cbor_cache = cbor_cache;

  return new_redeemer_result;
}

cardano_error_t
cardano_redeemer_to_cbor(const cardano_redeemer_t* redeemer, cardano_cbor_writer_t* writer)
{
  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(redeemer->cbor_cache), cardano_buffer_get_size(redeemer->cbor_cache));
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    REDEEMER_EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_tag_result = cardano_cbor_writer_write_uint(
    writer,
    (uint64_t)redeemer->tag);

  if (write_tag_result != CARDANO_SUCCESS)
  {
    return write_tag_result;
  }

  cardano_error_t write_index_result = cardano_cbor_writer_write_uint(
    writer,
    redeemer->index);

  if (write_index_result != CARDANO_SUCCESS)
  {
    return write_index_result;
  }

  cardano_error_t write_data_result = cardano_plutus_data_to_cbor(redeemer->data, writer);

  if (write_data_result != CARDANO_SUCCESS)
  {
    return write_data_result;
  }

  cardano_error_t write_ex_units_result = cardano_ex_units_to_cbor(redeemer->execution_units, writer);

  if (write_ex_units_result != CARDANO_SUCCESS)
  {
    return write_ex_units_result;
  }

  return CARDANO_SUCCESS;
}

cardano_redeemer_tag_t
cardano_redeemer_get_tag(const cardano_redeemer_t* redeemer)
{
  if (redeemer == NULL)
  {
    return CARDANO_REDEEMER_TAG_SPEND;
  }

  return redeemer->tag;
}

cardano_error_t
cardano_redeemer_set_tag(cardano_redeemer_t* redeemer, cardano_redeemer_tag_t tag)
{
  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  redeemer->tag = tag;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_redeemer_get_index(const cardano_redeemer_t* redeemer)
{
  if (redeemer == NULL)
  {
    return 0;
  }

  return redeemer->index;
}

cardano_error_t
cardano_redeemer_set_index(cardano_redeemer_t* redeemer, uint64_t index)
{
  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  redeemer->index = index;

  return CARDANO_SUCCESS;
}

cardano_plutus_data_t*
cardano_redeemer_get_data(cardano_redeemer_t* redeemer)
{
  if (redeemer == NULL)
  {
    return NULL;
  }

  cardano_plutus_data_ref(redeemer->data);
  return redeemer->data;
}

cardano_error_t
cardano_redeemer_set_data(cardano_redeemer_t* redeemer, cardano_plutus_data_t* data)
{
  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_unref(&redeemer->data);
  redeemer->data = data;

  cardano_plutus_data_ref(data);

  return CARDANO_SUCCESS;
}

cardano_ex_units_t*
cardano_redeemer_get_ex_units(cardano_redeemer_t* redeemer)
{
  if (redeemer == NULL)
  {
    return NULL;
  }

  cardano_ex_units_ref(redeemer->execution_units);
  return redeemer->execution_units;
}

cardano_error_t
cardano_redeemer_set_ex_units(cardano_redeemer_t* redeemer, cardano_ex_units_t* ex_units)
{
  if (redeemer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (ex_units == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ex_units_unref(&redeemer->execution_units);
  redeemer->execution_units = ex_units;

  cardano_ex_units_ref(ex_units);

  return CARDANO_SUCCESS;
}

void
cardano_redeemer_clear_cbor_cache(cardano_redeemer_t* redeemer)
{
  if (redeemer == NULL)
  {
    return;
  }

  cardano_buffer_unref(&redeemer->cbor_cache);
  cardano_plutus_data_clear_cbor_cache(redeemer->data);

  redeemer->cbor_cache = NULL;
}

void
cardano_redeemer_unref(cardano_redeemer_t** redeemer)
{
  if ((redeemer == NULL) || (*redeemer == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*redeemer)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *redeemer = NULL;
    return;
  }
}

void
cardano_redeemer_ref(cardano_redeemer_t* redeemer)
{
  if (redeemer == NULL)
  {
    return;
  }

  cardano_object_ref(&redeemer->base);
}

size_t
cardano_redeemer_refcount(const cardano_redeemer_t* redeemer)
{
  if (redeemer == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&redeemer->base);
}

void
cardano_redeemer_set_last_error(cardano_redeemer_t* redeemer, const char* message)
{
  cardano_object_set_last_error(&redeemer->base, message);
}

const char*
cardano_redeemer_get_last_error(const cardano_redeemer_t* redeemer)
{
  return cardano_object_get_last_error(&redeemer->base);
}