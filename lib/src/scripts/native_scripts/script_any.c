/**
 * \file script_any.c
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

#include <cardano/error.h>
#include <cardano/scripts/native_scripts/native_script_list.h>
#include <cardano/scripts/native_scripts/native_script_type.h>
#include <cardano/scripts/native_scripts/script_any.h>

#include <cardano/object.h>

#include "../../allocators.h"
#include "../../cbor/cbor_validation.h"

#include <assert.h>
#include <cardano/json/json_object.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief This script evaluates to true if all the sub-scripts evaluate to true.
 *
 * If the list of sub-scripts is empty, this script evaluates to true.
 */
typedef struct cardano_script_any_t
{
    cardano_object_t              base;
    cardano_native_script_type_t  type;
    cardano_native_script_list_t* scripts;

} cardano_script_any_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a script_any object.
 *
 * This function is responsible for properly deallocating a script_any object (`cardano_script_any_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the script_any object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_script_any_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the script_any
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_script_any_deallocate(void* object)
{
  assert(object != NULL);

  cardano_script_any_t* data = (cardano_script_any_t*)object;

  cardano_native_script_list_unref(&data->scripts);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_script_any_new(cardano_native_script_list_t* native_scripts, cardano_script_any_t** script_any)
{
  if (native_scripts == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_any == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_any_t* data = _cardano_malloc(sizeof(cardano_script_any_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_native_script_list_ref(native_scripts);

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_script_any_deallocate;
  data->type               = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF;
  data->scripts            = native_scripts;

  *script_any = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_any_from_cbor(cardano_cbor_reader_t* reader, cardano_script_any_t** script_any)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_any == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "script_any";

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
    CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF,
    (enum_to_string_callback_t)((void*)&cardano_native_script_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_native_script_list_t* native_scripts          = NULL;
  const cardano_error_t         read_script_list_result = cardano_native_script_list_from_cbor(reader, &native_scripts);

  if (read_script_list_result != CARDANO_SUCCESS)
  {
    return read_script_list_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_native_script_list_unref(&native_scripts);
    return expect_end_array_result;
  }

  const cardano_error_t create_any_new_result = cardano_script_any_new(native_scripts, script_any);
  cardano_native_script_list_unref(&native_scripts);

  return create_any_new_result;
}

cardano_error_t
cardano_script_any_to_cbor(
  const cardano_script_any_t* script_any,
  cardano_cbor_writer_t*      writer)
{
  if (script_any == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, 2);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, script_any->type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_native_script_list_to_cbor(script_any->scripts, writer);

  return result;
}

cardano_error_t
cardano_script_any_from_json(const char* json, size_t json_size, cardano_script_any_t** native_script)
{
  if (json == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
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

  const char* type_string = cardano_json_object_get_string(type, NULL);

  if (type_string == NULL)
  {
    cardano_json_object_unref(&json_object);
    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_native_script_list_t* native_scripts = NULL;

  if (strcmp(type_string, "any") == 0)
  {
    cardano_error_t result = cardano_native_script_list_from_json(json, json_size, &native_scripts);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&json_object);
      return result;
    }
  }
  else
  {
    cardano_json_object_unref(&json_object);
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_json_object_unref(&json_object);

  const cardano_error_t create_any_new_result = cardano_script_any_new(native_scripts, native_script);
  cardano_native_script_list_unref(&native_scripts);

  return create_any_new_result;
}

size_t
cardano_script_any_get_length(const cardano_script_any_t* script_any)
{
  if (script_any == NULL)
  {
    return 0;
  }

  return cardano_native_script_list_get_length(script_any->scripts);
}

cardano_error_t
cardano_script_any_get_scripts(
  cardano_script_any_t*          script_any,
  cardano_native_script_list_t** list)
{
  if (script_any == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_list_ref(script_any->scripts);

  *list = script_any->scripts;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_any_set_scripts(
  cardano_script_any_t*         script_any,
  cardano_native_script_list_t* list)
{
  if (script_any == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_list_ref(list);

  cardano_native_script_list_unref(&script_any->scripts);

  script_any->scripts = list;

  return CARDANO_SUCCESS;
}

bool
cardano_script_any_equals(const cardano_script_any_t* lhs, const cardano_script_any_t* rhs)
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

  assert(lhs->scripts);
  assert(rhs->scripts);

  return cardano_native_script_list_equals(lhs->scripts, rhs->scripts);
}

void
cardano_script_any_unref(cardano_script_any_t** script_any)
{
  if ((script_any == NULL) || (*script_any == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*script_any)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *script_any = NULL;
    return;
  }
}

void
cardano_script_any_ref(cardano_script_any_t* script_any)
{
  if (script_any == NULL)
  {
    return;
  }

  cardano_object_ref(&script_any->base);
}

size_t
cardano_script_any_refcount(const cardano_script_any_t* script_any)
{
  if (script_any == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&script_any->base);
}

void
cardano_script_any_set_last_error(cardano_script_any_t* script_any, const char* message)
{
  cardano_object_set_last_error(&script_any->base, message);
}

const char*
cardano_script_any_get_last_error(const cardano_script_any_t* script_any)
{
  return cardano_object_get_last_error(&script_any->base);
}
