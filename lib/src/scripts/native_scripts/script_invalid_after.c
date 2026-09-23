/**
 * \file script_invalid_after.c
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

#include <cardano/buffer.h>
#include <cardano/error.h>
#include <cardano/scripts/native_scripts/native_script_list.h>
#include <cardano/scripts/native_scripts/native_script_type.h>
#include <cardano/scripts/native_scripts/script_invalid_after.h>

#include <cardano/object.h>

#include "../../allocators.h"
#include "../../cbor/cbor_validation.h"
#include "../../string_safe.h"

#include <assert.h>
#include <cardano/json/json_object.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief This script evaluates to true if the upper bound of the transaction validity interval is a
 * slot number Y, and X <= Y.
 *
 * This condition guarantees that the actual slot number in which the transaction is included is
 * (strictly) less than slot number X.
 */
typedef struct cardano_script_invalid_after_t
{
    cardano_object_t             base;
    cardano_native_script_type_t type;
    uint64_t                     slot;
    cardano_buffer_t*            cbor_cache;

} cardano_script_invalid_after_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a script_invalid_after object.
 *
 * This function is responsible for properly deallocating a script_invalid_after object (`cardano_script_invalid_after_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the script_invalid_after object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_script_invalid_after_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the script_invalid_after
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_script_invalid_after_deallocate(void* object)
{
  assert(object != NULL);

  cardano_script_invalid_after_t* data = (cardano_script_invalid_after_t*)object;

  cardano_buffer_unref(&data->cbor_cache);

  _cardano_free(data);
}

/**
 * \brief Decodes the fields of a script_invalid_after from a CBOR reader.
 *
 * This function reads the CBOR encoded script_invalid_after at the current position of the reader and creates a new
 * \ref cardano_script_invalid_after_t object from its fields. It does not cache the original CBOR representation, that is done
 * by \ref cardano_script_invalid_after_from_cbor.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t positioned at the script_invalid_after.
 * \param[out] script_invalid_after On success, set to the newly created \ref cardano_script_invalid_after_t object.
 *
 * \return \ref CARDANO_SUCCESS if the script_invalid_after was decoded, or an appropriate error code otherwise.
 */
static cardano_error_t
cardano_script_invalid_after_decode(cardano_cbor_reader_t* reader, cardano_script_invalid_after_t** script_invalid_after)
{
  static const char* validator_name = "script_invalid_after";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, 2);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER,
    (enum_to_string_callback_t)((void*)&cardano_native_script_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  uint64_t slot = 0U;

  const cardano_error_t read_slot_result = cardano_cbor_reader_read_uint(reader, &slot);

  if (read_slot_result != CARDANO_SUCCESS)
  {
    return read_slot_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    return expect_end_array_result;
  }

  return cardano_script_invalid_after_new(slot, script_invalid_after);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_script_invalid_after_new(const uint64_t slot, cardano_script_invalid_after_t** script_invalid_after)
{
  if (script_invalid_after == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_invalid_after_t* data = _cardano_malloc(sizeof(cardano_script_invalid_after_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_script_invalid_after_deallocate;
  data->cbor_cache         = NULL;
  data->type               = CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER;
  data->slot               = slot;

  *script_invalid_after = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_invalid_after_from_cbor(cardano_cbor_reader_t* reader, cardano_script_invalid_after_t** script_invalid_after)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_invalid_after == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_reader_t* reader_copy = NULL;
  cardano_error_t        result      = cardano_cbor_reader_clone(reader, &reader_copy);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_script_invalid_after_decode(reader, script_invalid_after);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader_copy);
    return result;
  }

  cardano_buffer_t* cbor_cache = NULL;

  result = cardano_cbor_reader_read_encoded_value(reader_copy, &cbor_cache);
  cardano_cbor_reader_unref(&reader_copy);

  if ((result != CARDANO_SUCCESS) || (cbor_cache == NULL))
  {
    cardano_script_invalid_after_unref(script_invalid_after);

    return (result != CARDANO_SUCCESS) ? result : CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*script_invalid_after)->cbor_cache = cbor_cache;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_invalid_after_to_cbor(
  const cardano_script_invalid_after_t* script_invalid_after,
  cardano_cbor_writer_t*                writer)
{
  if (script_invalid_after == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_invalid_after->cbor_cache != NULL)
  {
    return cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(script_invalid_after->cbor_cache), cardano_buffer_get_size(script_invalid_after->cbor_cache));
  }

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, 2);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, script_invalid_after->type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, script_invalid_after->slot);

  return result;
}

cardano_error_t
cardano_script_invalid_after_to_cip116_json(
  const cardano_script_invalid_after_t* script_invalid_after,
  cardano_json_writer_t*                writer)
{
  if ((script_invalid_after == NULL) || (writer == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "tag", 3);
  cardano_json_writer_write_string(writer, "timelock_expiry", 15);
  cardano_json_writer_write_property_name(writer, "slot", 4);

  char         number_str[32] = { 0 };
  const size_t size           = cardano_safe_uint64_to_string(script_invalid_after->slot, number_str, sizeof(number_str));

  assert(size > 0U);
  CARDANO_UNUSED(size);

  cardano_json_writer_write_string(writer, number_str, cardano_safe_strlen(number_str, 32));

  cardano_json_writer_write_end_object(writer);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_invalid_after_from_json(const char* json, size_t json_size, cardano_script_invalid_after_t** native_script)
{
  if (json == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (json_size == 0U)
  {
    return CARDANO_ERROR_INVALID_JSON;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_object_t* json_object = cardano_json_object_parse(json, json_size);

  if (json_object == NULL)
  {
    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* type = NULL;

  if (!cardano_json_object_get_ex(json_object, "type", 4, &type))
  {
    cardano_json_object_unref(&json_object);
    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* slot_object = NULL;

  if (!cardano_json_object_get_ex(json_object, "slot", 4, &slot_object))
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_JSON;
  }

  const char* type_string = cardano_json_object_get_string(type, NULL);

  if (type_string == NULL)
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_JSON;
  }

  uint64_t slot = 0U;

  cardano_error_t result = cardano_json_object_get_uint(slot_object, &slot);

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_object_unref(&json_object);

    return result;
  }

  if (strcmp(type_string, "before") != 0)
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_json_object_unref(&json_object);

  return cardano_script_invalid_after_new(slot, native_script);
}

cardano_error_t
cardano_script_invalid_after_get_slot(const cardano_script_invalid_after_t* script_invalid_after, uint64_t* slot)
{
  if (script_invalid_after == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (slot == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *slot = script_invalid_after->slot;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_invalid_after_set_slot(cardano_script_invalid_after_t* script_invalid_after, uint64_t slot)
{
  if (script_invalid_after == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  script_invalid_after->slot = slot;
  cardano_buffer_unref(&script_invalid_after->cbor_cache);
  script_invalid_after->cbor_cache = NULL;

  return CARDANO_SUCCESS;
}

bool
cardano_script_invalid_after_equals(const cardano_script_invalid_after_t* lhs, const cardano_script_invalid_after_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (lhs->type != rhs->type)
  {
    return false;
  }

  if (lhs->slot != rhs->slot)
  {
    return false;
  }

  return true;
}

void
cardano_script_invalid_after_clear_cbor_cache(cardano_script_invalid_after_t* script_invalid_after)
{
  if (script_invalid_after == NULL)
  {
    return;
  }

  cardano_buffer_unref(&script_invalid_after->cbor_cache);
  script_invalid_after->cbor_cache = NULL;
}

void
cardano_script_invalid_after_unref(cardano_script_invalid_after_t** script_invalid_after)
{
  if ((script_invalid_after == NULL) || (*script_invalid_after == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*script_invalid_after)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *script_invalid_after = NULL;
    return;
  }
}

void
cardano_script_invalid_after_ref(cardano_script_invalid_after_t* script_invalid_after)
{
  if (script_invalid_after == NULL)
  {
    return;
  }

  cardano_object_ref(&script_invalid_after->base);
}

size_t
cardano_script_invalid_after_refcount(const cardano_script_invalid_after_t* script_invalid_after)
{
  if (script_invalid_after == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&script_invalid_after->base);
}

void
cardano_script_invalid_after_set_last_error(cardano_script_invalid_after_t* script_invalid_after, const char* message)
{
  cardano_object_set_last_error(&script_invalid_after->base, message);
}

const char*
cardano_script_invalid_after_get_last_error(const cardano_script_invalid_after_t* script_invalid_after)
{
  return cardano_object_get_last_error(&script_invalid_after->base);
}
