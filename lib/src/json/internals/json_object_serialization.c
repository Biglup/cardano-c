/**
 * \file json_object_serialization.c
 *
 * \author angel.castillo
 * \date   Dec 09, 2024
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

#include <cardano/json/json_object.h>
#include <cardano/typedefs.h>

#include "../../collections/array.h"
#include "json_object_common.h"
#include "json_object_serialization.h"
#include "json_writer_common.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

/* FUNCTIONS *****************************************************************/

cardano_error_t
cardano_write_json_object(cardano_json_writer_t* writer, const cardano_json_object_t* object)
{
  assert(writer != NULL);
  assert(object != NULL);

  cardano_json_object_type_t type = cardano_json_object_get_type(object);

  switch (type)
  {
    case CARDANO_JSON_OBJECT_TYPE_OBJECT:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to write nested objects.
      cardano_write_json_object_type_object(writer, object);
      break;
    }
    case CARDANO_JSON_OBJECT_TYPE_ARRAY:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to write nested objects.
      cardano_write_json_object_type_array(writer, object);
      break;
    }
    case CARDANO_JSON_OBJECT_TYPE_STRING:
    {
      cardano_write_json_object_type_string(writer, object);
      break;
    }
    case CARDANO_JSON_OBJECT_TYPE_NUMBER:
    {
      cardano_write_json_object_type_number(writer, object);
      break;
    }
    case CARDANO_JSON_OBJECT_TYPE_BOOLEAN:
    {
      cardano_write_json_object_type_boolean(writer, object);
      break;
    }
    case CARDANO_JSON_OBJECT_TYPE_NULL:
    {
      cardano_json_writer_write_null(writer);
      break;
    }
    default:
    {
      cardano_json_writer_set_last_error(writer, "Unknown JSON object type.");
      return CARDANO_ERROR_ENCODING;
    }
  }

  return CARDANO_SUCCESS;
}

void
cardano_write_json_object_type_object(cardano_json_writer_t* writer, const cardano_json_object_t* object)
{
  cardano_json_writer_write_start_object(writer);

  size_t property_count = cardano_json_object_get_property_count(object);

  for (size_t i = 0U; i < property_count; ++i)
  {
    size_t      key_length = 0;
    const char* key        = cardano_json_object_get_key_at(object, i, &key_length);

    cardano_json_writer_write_property_name(writer, key, key_length);

    cardano_json_object_t* value = cardano_json_object_get_value_at(object, i);
    cardano_json_object_unref(&value);

    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to write nested objects.
    cardano_error_t result = cardano_write_json_object(writer, value);

    cardano_json_writer_set_message_if_error(writer, result, "Failed to write object property.");
  }

  cardano_json_writer_write_end_object(writer);
}

void
cardano_write_json_object_type_array(cardano_json_writer_t* writer, const cardano_json_object_t* array)
{
  cardano_json_writer_write_start_array(writer);

  size_t array_length = cardano_json_object_array_get_length(array);

  for (size_t i = 0U; i < array_length; ++i)
  {
    cardano_json_object_t* element = cardano_json_object_array_get(array, i);
    cardano_json_object_unref(&element);

    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to write nested objects.
    cardano_error_t result = cardano_write_json_object(writer, element);

    cardano_json_writer_set_message_if_error(writer, result, "Failed to write array element.");
  }

  cardano_json_writer_write_end_array(writer);
}

void
cardano_write_json_object_type_string(cardano_json_writer_t* writer, const cardano_json_object_t* string_obj)
{
  size_t      value_size = 0;
  const char* value      = cardano_json_object_get_string(string_obj, &value_size);

  cardano_json_writer_write_string(writer, value, value_size - 1U);
}

void
cardano_write_json_object_type_number(cardano_json_writer_t* writer, const cardano_json_object_t* number_obj)
{
  assert(writer != NULL);
  assert(number_obj != NULL);

  if (number_obj->is_real)
  {
    double          value  = 0.0;
    cardano_error_t result = cardano_json_object_get_double(number_obj, &value);

    cardano_json_writer_set_message_if_error(writer, result, "Failed to get double value.");

    cardano_json_writer_write_double(writer, value);
  }
  else
  {
    if (number_obj->is_negative)
    {
      int64_t signed_value = 0;

      cardano_error_t result = cardano_json_object_get_signed_int(number_obj, &signed_value);

      cardano_json_writer_set_message_if_error(writer, result, "Failed to get signed integer value.");

      cardano_json_writer_write_signed_int(writer, signed_value);
    }
    else
    {
      uint64_t value = 0;

      cardano_error_t result = cardano_json_object_get_uint(number_obj, &value);

      cardano_json_writer_set_message_if_error(writer, result, "Failed to get unsigned integer value.");

      cardano_json_writer_write_uint(writer, value);
    }
  }
}

void
cardano_write_json_object_type_boolean(cardano_json_writer_t* writer, const cardano_json_object_t* bool_obj)
{
  bool            value  = false;
  cardano_error_t result = cardano_json_object_get_boolean(bool_obj, &value);

  cardano_json_writer_set_message_if_error(writer, result, "Failed to get boolean value.");

  cardano_json_writer_write_bool(writer, value);
}