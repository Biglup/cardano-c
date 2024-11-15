/**
 * \file auxiliary_data.c
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#include <cardano/auxiliary_data/auxiliary_data.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Auxiliary Data encapsulate certain optional information that can be attached
 * to a transaction. This data includes transaction metadata and scripts.
 *
 * The Auxiliary Data is hashed and referenced in the transaction body.
 */
typedef struct cardano_auxiliary_data_t
{
    cardano_object_t                 base;
    cardano_transaction_metadata_t*  metadata;
    cardano_native_script_list_t*    native_scripts;
    cardano_plutus_v1_script_list_t* plutus_v1_scripts;
    cardano_plutus_v2_script_list_t* plutus_v2_scripts;
    cardano_plutus_v3_script_list_t* plutus_v3_scripts;
    cardano_buffer_t*                cbor_cache;
} cardano_auxiliary_data_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a auxiliary_data object.
 *
 * This function is responsible for properly deallocating a auxiliary_data object (`cardano_auxiliary_data_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the auxiliary_data object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_auxiliary_data_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the auxiliary_data
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_auxiliary_data_deallocate(void* object)
{
  assert(object != NULL);

  cardano_auxiliary_data_t* auxiliary_data = (cardano_auxiliary_data_t*)object;

  cardano_transaction_metadata_unref(&auxiliary_data->metadata);
  cardano_native_script_list_unref(&auxiliary_data->native_scripts);
  cardano_plutus_v1_script_list_unref(&auxiliary_data->plutus_v1_scripts);
  cardano_plutus_v2_script_list_unref(&auxiliary_data->plutus_v2_scripts);
  cardano_plutus_v3_script_list_unref(&auxiliary_data->plutus_v3_scripts);
  cardano_buffer_unref(&auxiliary_data->cbor_cache);

  _cardano_free(object);
}

/**
 * \brief Returns the size of the map that will be created when serializing the auxiliary data.
 *
 * \param auxiliary_data The auxiliary data object to be serialized.
 *
 * \return The size of the map that will be created when serializing the auxiliary data.
 */
static int64_t
get_map_size(const cardano_auxiliary_data_t* auxiliary_data)
{
  int64_t map_size = 0;

  if (auxiliary_data->metadata != NULL)
  {
    ++map_size;
  }

  if (auxiliary_data->native_scripts != NULL)
  {
    ++map_size;
  }

  if (auxiliary_data->plutus_v1_scripts != NULL)
  {
    ++map_size;
  }

  if (auxiliary_data->plutus_v2_scripts != NULL)
  {
    ++map_size;
  }

  if (auxiliary_data->plutus_v3_scripts != NULL)
  {
    ++map_size;
  }

  return map_size;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_auxiliary_data_new(cardano_auxiliary_data_t** auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_auxiliary_data_t* new_auxiliary_data = (cardano_auxiliary_data_t*)_cardano_malloc(sizeof(cardano_auxiliary_data_t));

  if (new_auxiliary_data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  new_auxiliary_data->base.ref_count     = 1;
  new_auxiliary_data->base.last_error[0] = '\0';
  new_auxiliary_data->base.deallocator   = cardano_auxiliary_data_deallocate;
  new_auxiliary_data->metadata           = NULL;
  new_auxiliary_data->native_scripts     = NULL;
  new_auxiliary_data->plutus_v1_scripts  = NULL;
  new_auxiliary_data->plutus_v2_scripts  = NULL;
  new_auxiliary_data->plutus_v3_scripts  = NULL;
  new_auxiliary_data->cbor_cache         = NULL;

  *auxiliary_data = new_auxiliary_data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_auxiliary_data_from_cbor(cardano_cbor_reader_t* reader, cardano_auxiliary_data_t** auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *auxiliary_data = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_auxiliary_data_t* new_auxiliary_data = NULL;

  cardano_error_t create_aux_data_result = cardano_auxiliary_data_new(&new_auxiliary_data);

  if (create_aux_data_result != CARDANO_SUCCESS)
  {
    *auxiliary_data = NULL;
    return create_aux_data_result;
  }

  cardano_cbor_reader_t* reader_copy = NULL;
  cardano_error_t        copy_result = cardano_cbor_reader_clone(reader, &reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    cardano_auxiliary_data_unref(&new_auxiliary_data);
    *auxiliary_data = NULL;

    return copy_result;
  }

  copy_result = cardano_cbor_reader_read_encoded_value(reader_copy, &new_auxiliary_data->cbor_cache);
  cardano_cbor_reader_unref(&reader_copy);

  if (copy_result != CARDANO_SUCCESS)
  {
    cardano_auxiliary_data_unref(&new_auxiliary_data);
    *auxiliary_data = NULL;

    return copy_result;
  }

  cardano_cbor_reader_state_t state;

  cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_result != CARDANO_SUCCESS)
  {
    *auxiliary_data = NULL;
    return peek_result;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (state == CARDANO_CBOR_READER_STATE_START_MAP)
  {
    result = cardano_transaction_metadata_from_cbor(reader, &new_auxiliary_data->metadata);

    if (result != CARDANO_SUCCESS)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      return result;
    }

    *auxiliary_data = new_auxiliary_data;

    return CARDANO_SUCCESS;
  }
  else if (state == CARDANO_CBOR_READER_STATE_TAG)
  {
    cardano_cbor_tag_t tag;

    result = cardano_cbor_reader_read_tag(reader, &tag);

    if (result != CARDANO_SUCCESS)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      return result;
    }

    if ((size_t)tag != 259U)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      cardano_cbor_reader_set_last_error(reader, "Invalid tag value for auxiliary data. Expected 259.");

      return CARDANO_ERROR_INVALID_CBOR_VALUE;
    }

    int64_t map_size = 0;
    result           = cardano_cbor_reader_read_start_map(reader, &map_size);

    if (result != CARDANO_SUCCESS)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      return result;
    }

    state = CARDANO_CBOR_READER_STATE_START_MAP;

    while (state != CARDANO_CBOR_READER_STATE_END_MAP)
    {
      result = cardano_cbor_reader_peek_state(reader, &state);

      if (result != CARDANO_SUCCESS)
      {
        cardano_auxiliary_data_unref(&new_auxiliary_data);
        *auxiliary_data = NULL;

        return result;
      }

      if (state == CARDANO_CBOR_READER_STATE_END_MAP)
      {
        break;
      }

      uint64_t key = 0;
      result       = cardano_cbor_reader_read_uint(reader, &key);

      if (result != CARDANO_SUCCESS)
      {
        cardano_auxiliary_data_unref(&new_auxiliary_data);
        *auxiliary_data = NULL;

        return result;
      }

      switch (key)
      {
        case 0:
        {
          result = cardano_transaction_metadata_from_cbor(reader, &new_auxiliary_data->metadata);

          if (result != CARDANO_SUCCESS)
          {
            cardano_auxiliary_data_unref(&new_auxiliary_data);
            *auxiliary_data = NULL;

            return result;
          }

          break;
        }
        case 1:
        {
          result = cardano_native_script_list_from_cbor(reader, &new_auxiliary_data->native_scripts);

          if (result != CARDANO_SUCCESS)
          {
            cardano_auxiliary_data_unref(&new_auxiliary_data);
            *auxiliary_data = NULL;

            return result;
          }

          break;
        }
        case 2:
        {
          result = cardano_plutus_v1_script_list_from_cbor(reader, &new_auxiliary_data->plutus_v1_scripts);

          if (result != CARDANO_SUCCESS)
          {
            cardano_auxiliary_data_unref(&new_auxiliary_data);
            *auxiliary_data = NULL;

            return result;
          }

          break;
        }
        case 3:
        {
          result = cardano_plutus_v2_script_list_from_cbor(reader, &new_auxiliary_data->plutus_v2_scripts);

          if (result != CARDANO_SUCCESS)
          {
            cardano_auxiliary_data_unref(&new_auxiliary_data);
            *auxiliary_data = NULL;

            return result;
          }

          break;
        }
        case 4:
        {
          result = cardano_plutus_v3_script_list_from_cbor(reader, &new_auxiliary_data->plutus_v3_scripts);

          if (result != CARDANO_SUCCESS)
          {
            cardano_auxiliary_data_unref(&new_auxiliary_data);
            *auxiliary_data = NULL;

            return result;
          }

          break;
        }
        default:
        {
          cardano_auxiliary_data_unref(&new_auxiliary_data);
          *auxiliary_data = NULL;

          cardano_cbor_reader_set_last_error(reader, "Invalid key value for auxiliary data.");

          return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
        }
      }
    }

    result = cardano_cbor_validate_end_map("auxiliary_data", reader);

    if (result != CARDANO_SUCCESS)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      return result;
    }
  }
  else
  {
    int64_t array_size = 0;

    result = cardano_cbor_reader_read_start_array(reader, &array_size);

    if (result != CARDANO_SUCCESS)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      return result;
    }

    result = cardano_transaction_metadata_from_cbor(reader, &new_auxiliary_data->metadata);

    if (result != CARDANO_SUCCESS)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      return result;
    }

    if (array_size > 1)
    {
      result = cardano_native_script_list_from_cbor(reader, &new_auxiliary_data->native_scripts);

      if (result != CARDANO_SUCCESS)
      {
        cardano_auxiliary_data_unref(&new_auxiliary_data);
        *auxiliary_data = NULL;

        return result;
      }
    }

    result = cardano_cbor_validate_end_array("auxiliary_data", reader);

    if (result != CARDANO_SUCCESS)
    {
      cardano_auxiliary_data_unref(&new_auxiliary_data);
      *auxiliary_data = NULL;

      return result;
    }
  }

  *auxiliary_data = new_auxiliary_data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_auxiliary_data_to_cbor(
  const cardano_auxiliary_data_t* auxiliary_data,
  cardano_cbor_writer_t*          writer)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (auxiliary_data->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(auxiliary_data->cbor_cache), cardano_buffer_get_size(auxiliary_data->cbor_cache));
  }

  cardano_error_t result = CARDANO_SUCCESS;

  result = cardano_cbor_writer_write_tag(writer, 259);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t map_size = get_map_size(auxiliary_data);

  result = cardano_cbor_writer_write_start_map(writer, map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (auxiliary_data->metadata != NULL)
  {
    result = cardano_cbor_writer_write_uint(writer, 0);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_metadata_to_cbor(auxiliary_data->metadata, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (auxiliary_data->native_scripts != NULL)
  {
    result = cardano_cbor_writer_write_uint(writer, 1);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_native_script_list_to_cbor(auxiliary_data->native_scripts, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (auxiliary_data->plutus_v1_scripts != NULL)
  {
    result = cardano_cbor_writer_write_uint(writer, 2);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_v1_script_list_to_cbor(auxiliary_data->plutus_v1_scripts, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (auxiliary_data->plutus_v2_scripts != NULL)
  {
    result = cardano_cbor_writer_write_uint(writer, 3);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_v2_script_list_to_cbor(auxiliary_data->plutus_v2_scripts, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (auxiliary_data->plutus_v3_scripts != NULL)
  {
    result = cardano_cbor_writer_write_uint(writer, 4);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_plutus_v3_script_list_to_cbor(auxiliary_data->plutus_v3_scripts, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_transaction_metadata_t*
cardano_auxiliary_data_get_transaction_metadata(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return NULL;
  }

  cardano_transaction_metadata_ref(auxiliary_data->metadata);

  return auxiliary_data->metadata;
}

cardano_error_t
cardano_auxiliary_data_set_transaction_metadata(
  cardano_auxiliary_data_t*       auxiliary_data,
  cardano_transaction_metadata_t* metadata)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (metadata == NULL)
  {
    cardano_transaction_metadata_unref(&auxiliary_data->metadata);
    auxiliary_data->metadata = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_transaction_metadata_unref(&auxiliary_data->metadata);
  cardano_transaction_metadata_ref(metadata);

  auxiliary_data->metadata = metadata;

  return CARDANO_SUCCESS;
}

cardano_native_script_list_t*
cardano_auxiliary_data_get_native_scripts(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return NULL;
  }

  cardano_native_script_list_ref(auxiliary_data->native_scripts);

  return auxiliary_data->native_scripts;
}

cardano_error_t
cardano_auxiliary_data_set_native_scripts(
  cardano_auxiliary_data_t*     auxiliary_data,
  cardano_native_script_list_t* scripts)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (scripts == NULL)
  {
    cardano_native_script_list_unref(&auxiliary_data->native_scripts);
    auxiliary_data->native_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_native_script_list_unref(&auxiliary_data->native_scripts);
  cardano_native_script_list_ref(scripts);

  auxiliary_data->native_scripts = scripts;

  return CARDANO_SUCCESS;
}

cardano_plutus_v1_script_list_t*
cardano_auxiliary_data_get_plutus_v1_scripts(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return NULL;
  }

  cardano_plutus_v1_script_list_ref(auxiliary_data->plutus_v1_scripts);

  return auxiliary_data->plutus_v1_scripts;
}

cardano_error_t
cardano_auxiliary_data_set_plutus_v1_scripts(
  cardano_auxiliary_data_t*        auxiliary_data,
  cardano_plutus_v1_script_list_t* scripts)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (scripts == NULL)
  {
    cardano_plutus_v1_script_list_unref(&auxiliary_data->plutus_v1_scripts);
    auxiliary_data->plutus_v1_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_plutus_v1_script_list_unref(&auxiliary_data->plutus_v1_scripts);
  cardano_plutus_v1_script_list_ref(scripts);

  auxiliary_data->plutus_v1_scripts = scripts;

  return CARDANO_SUCCESS;
}

cardano_plutus_v2_script_list_t*
cardano_auxiliary_data_get_plutus_v2_scripts(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return NULL;
  }

  cardano_plutus_v2_script_list_ref(auxiliary_data->plutus_v2_scripts);

  return auxiliary_data->plutus_v2_scripts;
}

cardano_error_t
cardano_auxiliary_data_set_plutus_v2_scripts(
  cardano_auxiliary_data_t*        auxiliary_data,
  cardano_plutus_v2_script_list_t* scripts)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (scripts == NULL)
  {
    cardano_plutus_v2_script_list_unref(&auxiliary_data->plutus_v2_scripts);
    auxiliary_data->plutus_v2_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_plutus_v2_script_list_unref(&auxiliary_data->plutus_v2_scripts);
  cardano_plutus_v2_script_list_ref(scripts);

  auxiliary_data->plutus_v2_scripts = scripts;

  return CARDANO_SUCCESS;
}

cardano_plutus_v3_script_list_t*
cardano_auxiliary_data_get_plutus_v3_scripts(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return NULL;
  }

  cardano_plutus_v3_script_list_ref(auxiliary_data->plutus_v3_scripts);

  return auxiliary_data->plutus_v3_scripts;
}

cardano_error_t
cardano_auxiliary_data_set_plutus_v3_scripts(
  cardano_auxiliary_data_t*        auxiliary_data,
  cardano_plutus_v3_script_list_t* scripts)
{
  if (auxiliary_data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (scripts == NULL)
  {
    cardano_plutus_v3_script_list_unref(&auxiliary_data->plutus_v3_scripts);
    auxiliary_data->plutus_v3_scripts = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_plutus_v3_script_list_unref(&auxiliary_data->plutus_v3_scripts);
  cardano_plutus_v3_script_list_ref(scripts);

  auxiliary_data->plutus_v3_scripts = scripts;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_auxiliary_data_get_hash(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return NULL;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return NULL;
  }

  cardano_error_t result = cardano_auxiliary_data_to_cbor(auxiliary_data, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return NULL;
  }

  const size_t cbor_size = cardano_cbor_writer_get_encode_size(writer);
  byte_t*      cbor_data = (byte_t*)_cardano_malloc(cbor_size);

  if (cbor_data == NULL)
  {
    cardano_cbor_writer_unref(&writer);
    return NULL;
  }

  result = cardano_cbor_writer_encode(writer, cbor_data, cbor_size);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(cbor_data);
    cardano_cbor_writer_unref(&writer);

    return NULL;
  }

  cardano_cbor_writer_unref(&writer);

  cardano_blake2b_hash_t* hash = NULL;

  result = cardano_blake2b_compute_hash(cbor_data, cbor_size, CARDANO_BLAKE2B_HASH_SIZE_256, &hash);

  _cardano_free(cbor_data);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return hash;
}

void
cardano_auxiliary_data_clear_cbor_cache(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return;
  }

  cardano_buffer_unref(&auxiliary_data->cbor_cache);
  auxiliary_data->cbor_cache = NULL;
}

void
cardano_auxiliary_data_unref(cardano_auxiliary_data_t** auxiliary_data)
{
  if ((auxiliary_data == NULL) || (*auxiliary_data == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*auxiliary_data)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *auxiliary_data = NULL;
    return;
  }
}

void
cardano_auxiliary_data_ref(cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return;
  }

  cardano_object_ref(&auxiliary_data->base);
}

size_t
cardano_auxiliary_data_refcount(const cardano_auxiliary_data_t* auxiliary_data)
{
  if (auxiliary_data == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&auxiliary_data->base);
}

void
cardano_auxiliary_data_set_last_error(cardano_auxiliary_data_t* auxiliary_data, const char* message)
{
  cardano_object_set_last_error(&auxiliary_data->base, message);
}

const char*
cardano_auxiliary_data_get_last_error(const cardano_auxiliary_data_t* auxiliary_data)
{
  return cardano_object_get_last_error(&auxiliary_data->base);
}