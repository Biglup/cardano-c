/**
 * \file transaction_output.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
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

#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>
#include <cardano/transaction_body/transaction_output.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief A transaction output object includes the address which represents a public key
 * hash or a script hash that can unlock the output, and the funds that are held
 * inside.
 */
typedef struct cardano_transaction_output_t
{
    cardano_object_t   base;
    cardano_address_t* address;
    cardano_value_t*   value;
    cardano_datum_t*   datum;
    cardano_script_t*  script_ref;
} cardano_transaction_output_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a transaction_output object.
 *
 * This function is responsible for properly deallocating a transaction_output object (`cardano_transaction_output_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the transaction_output object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_transaction_output_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the transaction_output
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_transaction_output_deallocate(void* object)
{
  assert(object != NULL);

  cardano_transaction_output_t* transaction_output = (cardano_transaction_output_t*)object;

  cardano_address_unref(&transaction_output->address);
  cardano_value_unref(&transaction_output->value);
  cardano_datum_unref(&transaction_output->datum);
  cardano_script_unref(&transaction_output->script_ref);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_transaction_output_new(
  cardano_address_t*             address,
  const uint64_t                 amount,
  cardano_transaction_output_t** transaction_output)
{
  if (transaction_output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_output_t* new_transaction_output = (cardano_transaction_output_t*)_cardano_malloc(sizeof(cardano_transaction_output_t));

  if (new_transaction_output == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t new_val_result = cardano_value_new((int64_t)amount, NULL, &new_transaction_output->value);

  if (new_val_result != CARDANO_SUCCESS)
  {
    _cardano_free(new_transaction_output);
    *transaction_output = NULL;

    return new_val_result;
  }

  new_transaction_output->base.ref_count     = 1;
  new_transaction_output->base.last_error[0] = '\0';
  new_transaction_output->base.deallocator   = cardano_transaction_output_deallocate;
  new_transaction_output->address            = NULL;
  new_transaction_output->datum              = NULL;
  new_transaction_output->script_ref         = NULL;

  cardano_address_ref(address);
  new_transaction_output->address = address;

  *transaction_output = new_transaction_output;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_output_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_output_t** transaction_output)
{
  if (transaction_output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *transaction_output = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "transaction_output";

  cardano_address_t* address    = NULL;
  cardano_value_t*   value      = NULL;
  cardano_datum_t*   datum      = NULL;
  cardano_script_t*  script_ref = NULL;

  cardano_cbor_reader_state_t state;

  cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_result != CARDANO_SUCCESS)
  {
    *transaction_output = NULL;
    return peek_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_START_MAP)
  {
    int64_t map_size = 0U;

    const cardano_error_t read_map_result = cardano_cbor_reader_read_start_map(reader, &map_size);

    CARDANO_UNUSED(read_map_result);

    if (read_map_result != CARDANO_SUCCESS)
    {
      *transaction_output = NULL;
      return read_map_result;
    }

    while (state != CARDANO_CBOR_READER_STATE_END_MAP)
    {
      peek_result = cardano_cbor_reader_peek_state(reader, &state);

      if (peek_result != CARDANO_SUCCESS)
      {
        cardano_address_unref(&address);
        cardano_value_unref(&value);
        cardano_datum_unref(&datum);
        cardano_script_unref(&script_ref);

        *transaction_output = NULL;

        return peek_result;
      }

      if (state == CARDANO_CBOR_READER_STATE_END_MAP)
      {
        break;
      }

      uint64_t key = 0U;

      const cardano_error_t read_key_result = cardano_cbor_reader_read_uint(reader, &key);

      if (read_key_result != CARDANO_SUCCESS)
      {
        cardano_address_unref(&address);
        cardano_value_unref(&value);
        cardano_datum_unref(&datum);
        cardano_script_unref(&script_ref);

        *transaction_output = NULL;
        return read_key_result;
      }

      switch (key)
      {
        case 0U:
        {
          cardano_address_unref(&address);
          cardano_buffer_t* address_bytes = NULL;

          const cardano_error_t read_address_result = cardano_cbor_reader_read_bytestring(reader, &address_bytes);

          if (read_address_result != CARDANO_SUCCESS)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return read_address_result;
          }

          const cardano_error_t address_from_bytes_result = cardano_address_from_bytes(cardano_buffer_get_data(address_bytes), cardano_buffer_get_size(address_bytes), &address);

          cardano_buffer_unref(&address_bytes);

          if (address_from_bytes_result != CARDANO_SUCCESS)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return address_from_bytes_result;
          }

          break;
        }
        case 1U:
        {
          cardano_value_unref(&value);
          const cardano_error_t read_value_result = cardano_value_from_cbor(reader, &value);

          if (read_value_result != CARDANO_SUCCESS)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return read_value_result;
          }

          break;
        }
        case 2U:
        {
          cardano_datum_unref(&datum);
          const cardano_error_t read_datum_result = cardano_datum_from_cbor(reader, &datum);

          if (read_datum_result != CARDANO_SUCCESS)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return read_datum_result;
          }

          break;
        }
        case 3U:
        {
          cardano_script_unref(&script_ref);
          cardano_cbor_tag_t tag;

          const cardano_error_t read_tag_result = cardano_cbor_reader_read_tag(reader, &tag);

          if (read_tag_result != CARDANO_SUCCESS)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return read_tag_result;
          }

          if (tag != CARDANO_ENCODED_CBOR_DATA_ITEM)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return CARDANO_ERROR_INVALID_CBOR_VALUE;
          }

          cardano_buffer_t* script_bytes = NULL;

          const cardano_error_t read_script_bytes_result = cardano_cbor_reader_read_bytestring(reader, &script_bytes);

          if (read_script_bytes_result != CARDANO_SUCCESS)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return read_script_bytes_result;
          }

          cardano_cbor_reader_t* script_reader = cardano_cbor_reader_new(cardano_buffer_get_data(script_bytes), cardano_buffer_get_size(script_bytes));

          cardano_buffer_unref(&script_bytes);

          const cardano_error_t read_script_result = cardano_script_from_cbor(script_reader, &script_ref);

          cardano_cbor_reader_unref(&script_reader);

          if (read_script_result != CARDANO_SUCCESS)
          {
            cardano_address_unref(&address);
            cardano_value_unref(&value);
            cardano_datum_unref(&datum);
            cardano_script_unref(&script_ref);

            *transaction_output = NULL;

            return read_script_result;
          }

          break;
        }
        default:
        {
          cardano_address_unref(&address);
          cardano_value_unref(&value);
          cardano_datum_unref(&datum);
          cardano_script_unref(&script_ref);

          *transaction_output = NULL;

          return CARDANO_ERROR_INVALID_CBOR_MAP_KEY;
        }
      }
    }

    const cardano_error_t expect_end_map_result = cardano_cbor_validate_end_map(validator_name, reader);

    if (expect_end_map_result != CARDANO_SUCCESS)
    {
      cardano_address_unref(&address);
      cardano_value_unref(&value);
      cardano_datum_unref(&datum);
      cardano_script_unref(&script_ref);

      *transaction_output = NULL;

      return expect_end_map_result;
    }

    const cardano_error_t new_output_result = cardano_transaction_output_new(address, 0U, transaction_output);
    cardano_error_t       set_value_result  = cardano_transaction_output_set_value(*transaction_output, value);
    cardano_error_t       set_datum_result  = cardano_transaction_output_set_datum(*transaction_output, datum);
    cardano_error_t       set_script_result = cardano_transaction_output_set_script_ref(*transaction_output, script_ref);

    cardano_address_unref(&address);
    cardano_value_unref(&value);
    cardano_datum_unref(&datum);
    cardano_script_unref(&script_ref);

    if ((new_output_result != CARDANO_SUCCESS) || (set_value_result != CARDANO_SUCCESS) || (set_datum_result != CARDANO_SUCCESS) || (set_script_result != CARDANO_SUCCESS))
    {
      cardano_transaction_output_unref(transaction_output);
      *transaction_output = NULL;

      return new_output_result;
    }
  }
  else
  {
    int64_t length = 0U;

    const cardano_error_t read_array_result = cardano_cbor_reader_read_start_array(reader, &length);

    if (read_array_result != CARDANO_SUCCESS)
    {
      cardano_address_unref(&address);
      cardano_value_unref(&value);
      cardano_datum_unref(&datum);
      cardano_script_unref(&script_ref);

      *transaction_output = NULL;

      return read_array_result;
    }

    cardano_buffer_t* address_bytes = NULL;

    const cardano_error_t read_address_result = cardano_cbor_reader_read_bytestring(reader, &address_bytes);

    if (read_address_result != CARDANO_SUCCESS)
    {
      cardano_address_unref(&address);
      cardano_value_unref(&value);
      cardano_datum_unref(&datum);
      cardano_script_unref(&script_ref);

      *transaction_output = NULL;

      return read_address_result;
    }

    const cardano_error_t address_from_bytes_result = cardano_address_from_bytes(cardano_buffer_get_data(address_bytes), cardano_buffer_get_size(address_bytes), &address);

    cardano_buffer_unref(&address_bytes);

    if (address_from_bytes_result != CARDANO_SUCCESS)
    {
      cardano_address_unref(&address);
      cardano_value_unref(&value);
      cardano_datum_unref(&datum);
      cardano_script_unref(&script_ref);

      *transaction_output = NULL;

      return address_from_bytes_result;
    }

    const cardano_error_t read_value_result = cardano_value_from_cbor(reader, &value);

    if (read_value_result != CARDANO_SUCCESS)
    {
      cardano_address_unref(&address);
      cardano_value_unref(&value);
      cardano_datum_unref(&datum);
      cardano_script_unref(&script_ref);

      *transaction_output = NULL;

      return read_value_result;
    }

    if (length == 3)
    {
      cardano_blake2b_hash_t* datum_hash_data   = NULL;
      const cardano_error_t   read_datum_result = cardano_blake2b_hash_from_cbor(reader, &datum_hash_data);

      if (read_datum_result != CARDANO_SUCCESS)
      {
        cardano_address_unref(&address);
        cardano_value_unref(&value);
        cardano_datum_unref(&datum);
        cardano_script_unref(&script_ref);

        *transaction_output = NULL;

        return read_datum_result;
      }

      cardano_error_t datum_from_hash_result = cardano_datum_new_data_hash(datum_hash_data, &datum);
      cardano_blake2b_hash_unref(&datum_hash_data);

      if (datum_from_hash_result != CARDANO_SUCCESS)
      {
        cardano_address_unref(&address);
        cardano_value_unref(&value);
        cardano_datum_unref(&datum);
        cardano_script_unref(&script_ref);

        *transaction_output = NULL;

        return datum_from_hash_result;
      }
    }

    const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

    if (expect_end_array_result != CARDANO_SUCCESS)
    {
      cardano_address_unref(&address);
      cardano_value_unref(&value);
      cardano_datum_unref(&datum);
      cardano_script_unref(&script_ref);

      *transaction_output = NULL;

      return expect_end_array_result;
    }

    const cardano_error_t new_output_result = cardano_transaction_output_new(address, 0U, transaction_output);
    cardano_error_t       set_value_result  = cardano_transaction_output_set_value(*transaction_output, value);
    cardano_error_t       set_datum_result  = cardano_transaction_output_set_datum(*transaction_output, datum);

    cardano_address_unref(&address);
    cardano_value_unref(&value);
    cardano_datum_unref(&datum);
    cardano_script_unref(&script_ref);

    if ((new_output_result != CARDANO_SUCCESS) || (set_value_result != CARDANO_SUCCESS) || (set_datum_result != CARDANO_SUCCESS))
    {
      cardano_transaction_output_unref(transaction_output);
      *transaction_output = NULL;

      return new_output_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_transaction_output_to_cbor(
  const cardano_transaction_output_t* transaction_output,
  cardano_cbor_writer_t*              writer)
{
  if (transaction_output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t map_size = 2;

  if (transaction_output->datum != NULL)
  {
    ++map_size;
  }

  if (transaction_output->script_ref != NULL)
  {
    ++map_size;
  }

  cardano_error_t write_start_map_result = cardano_cbor_writer_write_start_map(writer, map_size);

  if (write_start_map_result != CARDANO_SUCCESS)
  {
    return write_start_map_result;
  }

  cardano_error_t write_address_result = cardano_cbor_writer_write_uint(writer, 0U);

  if (write_address_result != CARDANO_SUCCESS)
  {
    return write_address_result;
  }

  write_address_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_address_get_bytes(transaction_output->address),
    cardano_address_get_bytes_size(transaction_output->address));

  if (write_address_result != CARDANO_SUCCESS)
  {
    return write_address_result;
  }

  cardano_error_t write_value_result = cardano_cbor_writer_write_uint(writer, 1U);

  if (write_value_result != CARDANO_SUCCESS)
  {
    return write_value_result;
  }

  write_value_result = cardano_value_to_cbor(transaction_output->value, writer);

  if (write_value_result != CARDANO_SUCCESS)
  {
    return write_value_result;
  }

  if (transaction_output->datum != NULL)
  {
    cardano_error_t write_datum_result = cardano_cbor_writer_write_uint(writer, 2U);

    if (write_datum_result != CARDANO_SUCCESS)
    {
      return write_datum_result;
    }

    write_datum_result = cardano_datum_to_cbor(transaction_output->datum, writer);

    if (write_datum_result != CARDANO_SUCCESS)
    {
      return write_datum_result;
    }
  }

  if (transaction_output->script_ref != NULL)
  {
    cardano_error_t write_script_result = cardano_cbor_writer_write_uint(writer, 3U);

    if (write_script_result != CARDANO_SUCCESS)
    {
      return write_script_result;
    }

    cardano_error_t write_tag_result = cardano_cbor_writer_write_tag(writer, CARDANO_ENCODED_CBOR_DATA_ITEM);

    if (write_tag_result != CARDANO_SUCCESS)
    {
      return write_tag_result;
    }

    cardano_cbor_writer_t* script_writer = cardano_cbor_writer_new();

    write_script_result = cardano_script_to_cbor(transaction_output->script_ref, script_writer);

    if (write_script_result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&script_writer);
      return write_script_result;
    }

    cardano_buffer_t* script_bytes = NULL;

    cardano_error_t script_bytes_result = cardano_cbor_writer_encode_in_buffer(script_writer, &script_bytes);

    cardano_cbor_writer_unref(&script_writer);

    if (script_bytes_result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&script_bytes);
      return script_bytes_result;
    }

    write_script_result = cardano_cbor_writer_write_bytestring(writer, cardano_buffer_get_data(script_bytes), cardano_buffer_get_size(script_bytes));

    cardano_buffer_unref(&script_bytes);

    if (write_script_result != CARDANO_SUCCESS)
    {
      return write_script_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_address_t*
cardano_transaction_output_get_address(cardano_transaction_output_t* output)
{
  if (output == NULL)
  {
    return NULL;
  }

  cardano_address_ref(output->address);

  return output->address;
}

cardano_error_t
cardano_transaction_output_set_address(cardano_transaction_output_t* output, cardano_address_t* address)
{
  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_address_ref(address);
  cardano_address_unref(&output->address);

  output->address = address;

  return CARDANO_SUCCESS;
}

cardano_value_t*
cardano_transaction_output_get_value(cardano_transaction_output_t* output)
{
  if (output == NULL)
  {
    return NULL;
  }

  cardano_value_ref(output->value);

  return output->value;
}

cardano_error_t
cardano_transaction_output_set_value(cardano_transaction_output_t* output, cardano_value_t* value)
{
  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_value_ref(value);
  cardano_value_unref(&output->value);

  output->value = value;

  return CARDANO_SUCCESS;
}

cardano_datum_t*
cardano_transaction_output_get_datum(cardano_transaction_output_t* output)
{
  if (output == NULL)
  {
    return NULL;
  }

  cardano_datum_ref(output->datum);

  return output->datum;
}

cardano_error_t
cardano_transaction_output_set_datum(cardano_transaction_output_t* output, cardano_datum_t* datum)
{
  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_datum_ref(datum);
  cardano_datum_unref(&output->datum);

  output->datum = datum;

  return CARDANO_SUCCESS;
}

cardano_script_t*
cardano_transaction_output_get_script_ref(cardano_transaction_output_t* output)
{
  if (output == NULL)
  {
    return NULL;
  }

  cardano_script_ref(output->script_ref);

  return output->script_ref;
}

cardano_error_t
cardano_transaction_output_set_script_ref(cardano_transaction_output_t* output, cardano_script_t* script_ref)
{
  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_ref(script_ref);
  cardano_script_unref(&output->script_ref);

  output->script_ref = script_ref;

  return CARDANO_SUCCESS;
}

bool
cardano_transaction_output_equals(
  const cardano_transaction_output_t* lhs,
  const cardano_transaction_output_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if ((lhs == NULL) || (rhs == NULL))
  {
    return false;
  }

  if (!cardano_address_equals(lhs->address, rhs->address))
  {
    return false;
  }

  if (!cardano_value_equals(lhs->value, rhs->value))
  {
    return false;
  }

  if (!cardano_datum_equals(lhs->datum, rhs->datum))
  {
    return false;
  }

  if (!cardano_script_equals(lhs->script_ref, rhs->script_ref))
  {
    return false;
  }

  return true;
}

void
cardano_transaction_output_unref(cardano_transaction_output_t** transaction_output)
{
  if ((transaction_output == NULL) || (*transaction_output == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*transaction_output)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *transaction_output = NULL;
    return;
  }
}

void
cardano_transaction_output_ref(cardano_transaction_output_t* transaction_output)
{
  if (transaction_output == NULL)
  {
    return;
  }

  cardano_object_ref(&transaction_output->base);
}

size_t
cardano_transaction_output_refcount(const cardano_transaction_output_t* transaction_output)
{
  if (transaction_output == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&transaction_output->base);
}

void
cardano_transaction_output_set_last_error(cardano_transaction_output_t* transaction_output, const char* message)
{
  cardano_object_set_last_error(&transaction_output->base, message);
}

const char*
cardano_transaction_output_get_last_error(const cardano_transaction_output_t* transaction_output)
{
  return cardano_object_get_last_error(&transaction_output->base);
}