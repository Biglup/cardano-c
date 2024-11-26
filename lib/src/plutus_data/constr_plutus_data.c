/**
 * \file constr_plutus_data.c
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
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t  CONSTR_PLUTUS_EMBEDDED_GROUP_SIZE = 2;
static const uint64_t GENERAL_FORM_TAG                  = 102;
static const uint64_t ALTERNATIVE_TAG_OFFSET            = 7;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano constr plutus data.
 */
typedef struct cardano_constr_plutus_data_t
{
    cardano_object_t       base;
    uint64_t               alternative;
    cardano_plutus_list_t* data;
    cardano_buffer_t*      cbor_cache;
} cardano_constr_plutus_data_t;

/* STATIC FUNCTIONS **********************************************************/

// Mapping functions to and from alternative to and from CBOR tags.
// See https://github.com/input-output-hk/plutus/blob/1f31e640e8a258185db01fa899da63f9018c0e85/plutus-core/plutus-core/src/PlutusCore/Data.hs#L69-L72

/**
 * \brief Converts a CBOR compact tag to a Constr alternative.
 *
 * \param tag The tag to be converted.
 *
 * \returns The Constr alternative.
 */
static uint64_t
compact_cbor_tag_to_alternative(const uint64_t tag)
{
  if ((tag >= 121U) && (tag <= 127U))
  {
    return tag - 121U;
  }

  if ((tag >= 1280U) && (tag <= 1400U))
  {
    return (tag - 1280U) + ALTERNATIVE_TAG_OFFSET;
  }

  return GENERAL_FORM_TAG;
}

/**
 * \brief Converts the constructor alternative to its CBOR compact tag.
 *
 * \param alternative The Constr alternative to be converted.
 *
 * \returns The compact CBOR tag.
 */
static uint64_t
alternative_to_compact_cbor_tag(const uint64_t alternative)
{
  if (alternative <= 6U)
  {
    return 121U + alternative;
  }

  if (alternative <= 127U)
  {
    return 1280U - ALTERNATIVE_TAG_OFFSET + alternative;
  }

  return GENERAL_FORM_TAG;
}

/**
 * \brief Deallocates a constr plutus data object.
 *
 * This function is responsible for properly deallocating a constr plutus data object (`cardano_constr_plutus_data_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the constr_plutus_data object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_constr_plutus_data_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the constr_plutus_data
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_constr_plutus_data_deallocate(void* object)
{
  assert(object != NULL);

  cardano_constr_plutus_data_t* constr = (cardano_constr_plutus_data_t*)object;

  cardano_plutus_list_unref(&constr->data);
  cardano_buffer_unref(&constr->cbor_cache);

  _cardano_free(constr);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_constr_plutus_data_new(
  uint64_t                       alternative,
  cardano_plutus_list_t*         data,
  cardano_constr_plutus_data_t** constr_plutus_data)
{
  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *constr_plutus_data = _cardano_malloc(sizeof(cardano_constr_plutus_data_t));

  if (*constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*constr_plutus_data)->base.deallocator   = cardano_constr_plutus_data_deallocate;
  (*constr_plutus_data)->base.ref_count     = 1;
  (*constr_plutus_data)->base.last_error[0] = '\0';
  (*constr_plutus_data)->cbor_cache         = NULL;

  (*constr_plutus_data)->alternative = alternative;

  cardano_plutus_list_ref(data);

  (*constr_plutus_data)->data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_constr_plutus_data_from_cbor(cardano_cbor_reader_t* reader, cardano_constr_plutus_data_t** constr_plutus_data)
{
  if (constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *constr_plutus_data = NULL;
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
    cardano_buffer_unref(&cbor_cache);
    *constr_plutus_data = NULL;
    return copy_result;
  }

  cardano_cbor_tag_t    tag               = 0;
  const cardano_error_t expect_tag_result = cardano_cbor_reader_read_tag(reader, &tag);

  if (expect_tag_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    *constr_plutus_data = NULL;
    return expect_tag_result;
  }

  if ((uint64_t)tag == GENERAL_FORM_TAG)
  {
    static const char* validator_name = "constr_plutus_data";

    const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)CONSTR_PLUTUS_EMBEDDED_GROUP_SIZE);

    if (expect_array_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&cbor_cache);
      *constr_plutus_data = NULL;
      return expect_array_result;
    }

    uint64_t              alternative      = 0U;
    const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
      validator_name,
      "alternative",
      reader,
      &alternative,
      0,
      UINT64_MAX);

    if (read_uint_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&cbor_cache);
      *constr_plutus_data = NULL;
      return read_uint_result;
    }

    cardano_plutus_list_t* data             = NULL;
    const cardano_error_t  read_data_result = cardano_plutus_list_from_cbor(reader, &data);

    if (read_data_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&cbor_cache);
      *constr_plutus_data = NULL;
      return read_data_result;
    }

    const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

    if (expect_end_array_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&cbor_cache);
      *constr_plutus_data = NULL;
      cardano_plutus_list_unref(&data);

      return expect_end_array_result;
    }

    cardano_error_t result = cardano_constr_plutus_data_new(alternative, data, constr_plutus_data);
    cardano_plutus_list_unref(&data);

    if (result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&cbor_cache);
      *constr_plutus_data = NULL;
      return result;
    }

    (*constr_plutus_data)->cbor_cache = cbor_cache;

    return result;
  }

  const uint64_t alternative = compact_cbor_tag_to_alternative(tag);

  cardano_plutus_list_t* data             = NULL;
  const cardano_error_t  read_data_result = cardano_plutus_list_from_cbor(reader, &data);

  if (read_data_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    *constr_plutus_data = NULL;
    return read_data_result;
  }

  cardano_error_t result = cardano_constr_plutus_data_new(alternative, data, constr_plutus_data);
  cardano_plutus_list_unref(&data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor_cache);
    *constr_plutus_data = NULL;
    return result;
  }

  (*constr_plutus_data)->cbor_cache = cbor_cache;

  return result;
}

cardano_error_t
cardano_constr_plutus_data_to_cbor(const cardano_constr_plutus_data_t* constr_plutus_data, cardano_cbor_writer_t* writer)
{
  if (constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constr_plutus_data->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(constr_plutus_data->cbor_cache), cardano_buffer_get_size(constr_plutus_data->cbor_cache));
  }

  const uint64_t tag = alternative_to_compact_cbor_tag(constr_plutus_data->alternative);

  const cardano_error_t write_tag_result = cardano_cbor_writer_write_tag(writer, tag);

  if (write_tag_result != CARDANO_SUCCESS)
  {
    return write_tag_result;
  }

  if (tag != GENERAL_FORM_TAG)
  {
    return cardano_plutus_list_to_cbor(constr_plutus_data->data, writer);
  }

  const cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, CONSTR_PLUTUS_EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  const cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, constr_plutus_data->alternative);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  return cardano_plutus_list_to_cbor(constr_plutus_data->data, writer);
}

cardano_error_t
cardano_constr_plutus_data_get_data(
  cardano_constr_plutus_data_t* constr_plutus_data,
  cardano_plutus_list_t**       data)
{
  if (constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_list_ref(constr_plutus_data->data);
  *data = constr_plutus_data->data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_constr_plutus_data_set_data(
  cardano_constr_plutus_data_t* constr_plutus_data,
  cardano_plutus_list_t*        data)
{
  if (constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_list_ref(data);

  cardano_plutus_list_unref(&constr_plutus_data->data);

  constr_plutus_data->data = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_constr_plutus_data_get_alternative(
  const cardano_constr_plutus_data_t* constr_plutus_data,
  uint64_t*                           alternative)
{
  if (constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (alternative == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *alternative = constr_plutus_data->alternative;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_constr_plutus_data_set_alternative(
  cardano_constr_plutus_data_t* constr_plutus_data,
  const uint64_t                alternative)
{
  if (constr_plutus_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  constr_plutus_data->alternative = alternative;

  return CARDANO_SUCCESS;
}

bool
cardano_constr_plutus_equals(
  const cardano_constr_plutus_data_t* lhs,
  const cardano_constr_plutus_data_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if ((lhs == NULL) || (rhs == NULL))
  {
    return false;
  }

  if (lhs->alternative != rhs->alternative)
  {
    return false;
  }

  return cardano_plutus_list_equals(lhs->data, rhs->data);
}

void
cardano_constr_plutus_data_clear_cbor_cache(cardano_constr_plutus_data_t* constr_plutus_data)
{
  if (constr_plutus_data == NULL)
  {
    return;
  }

  cardano_buffer_unref(&constr_plutus_data->cbor_cache);
  constr_plutus_data->cbor_cache = NULL;
}

void
cardano_constr_plutus_data_unref(cardano_constr_plutus_data_t** constr_plutus_data)
{
  if ((constr_plutus_data == NULL) || (*constr_plutus_data == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*constr_plutus_data)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *constr_plutus_data = NULL;
    return;
  }
}

void
cardano_constr_plutus_data_ref(cardano_constr_plutus_data_t* constr_plutus_data)
{
  if (constr_plutus_data == NULL)
  {
    return;
  }

  cardano_object_ref(&constr_plutus_data->base);
}

size_t
cardano_constr_plutus_data_refcount(const cardano_constr_plutus_data_t* constr_plutus_data)
{
  if (constr_plutus_data == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&constr_plutus_data->base);
}

void
cardano_constr_plutus_data_set_last_error(cardano_constr_plutus_data_t* constr_plutus_data, const char* message)
{
  cardano_object_set_last_error(&constr_plutus_data->base, message);
}

const char*
cardano_constr_plutus_data_get_last_error(const cardano_constr_plutus_data_t* constr_plutus_data)
{
  return cardano_object_get_last_error(&constr_plutus_data->base);
}
