/**
 * \file script_n_of_k.c
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
 * WITHOUT WARRANTIES OR CONDITIONS OF n_of_k KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/scripts/native_scripts/native_script_list.h>
#include <cardano/scripts/native_scripts/native_script_type.h>
#include <cardano/scripts/native_scripts/script_n_of_k.h>

#include <cardano/object.h>

#include "../../allocators.h"
#include "../../cbor/cbor_validation.h"

#include <assert.h>
#include <cardano/json/json_object.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief This script evaluates to true if at least M (required field) of the sub-scripts evaluate to true.
 */
typedef struct cardano_script_n_of_k_t
{
    cardano_object_t              base;
    cardano_native_script_type_t  type;
    size_t                        required;
    cardano_native_script_list_t* scripts;

} cardano_script_n_of_k_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a script_n_of_k object.
 *
 * This function is responsible for properly deallocating a script_n_of_k object (`cardano_script_n_of_k_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the script_n_of_k object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_script_n_of_k_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the script_n_of_k
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_script_n_of_k_deallocate(void* object)
{
  assert(object != NULL);

  cardano_script_n_of_k_t* data = (cardano_script_n_of_k_t*)object;

  cardano_native_script_list_unref(&data->scripts);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_script_n_of_k_new(cardano_native_script_list_t* native_scripts, const size_t required, cardano_script_n_of_k_t** script_n_of_k)
{
  if (native_scripts == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_n_of_k_t* data = _cardano_malloc(sizeof(cardano_script_n_of_k_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_native_script_list_ref(native_scripts);

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_script_n_of_k_deallocate;
  data->type               = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K;
  data->scripts            = native_scripts;
  data->required           = required;

  *script_n_of_k = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_n_of_k_from_cbor(cardano_cbor_reader_t* reader, cardano_script_n_of_k_t** script_n_of_k)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "script_n_of_k";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, 3);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K,
    (enum_to_string_callback_t)((void*)&cardano_native_script_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  uint64_t        required             = 0;
  cardano_error_t read_required_result = cardano_cbor_reader_read_uint(reader, &required);

  if (read_required_result != CARDANO_SUCCESS)
  {
    return read_required_result;
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

  const cardano_error_t create_n_of_k_new_result = cardano_script_n_of_k_new(native_scripts, (size_t)required, script_n_of_k);
  cardano_native_script_list_unref(&native_scripts);

  return create_n_of_k_new_result;
}

cardano_error_t
cardano_script_n_of_k_to_cbor(
  const cardano_script_n_of_k_t* script_n_of_k,
  cardano_cbor_writer_t*         writer)
{
  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, 3);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, script_n_of_k->type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, (uint64_t)script_n_of_k->required);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_native_script_list_to_cbor(script_n_of_k->scripts, writer);

  return result;
}

cardano_error_t
cardano_script_n_of_k_from_json(const char* json, size_t json_size, cardano_script_n_of_k_t** native_script)
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

  cardano_json_object_t* type     = NULL;
  bool                   has_type = cardano_json_object_get_ex(json_object, "type", 4, &type);

  if (!has_type)
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* req = NULL;

  if (!cardano_json_object_get_ex(json_object, "required", 8, &req))
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

  uint64_t required = 0U;

  cardano_error_t result = cardano_json_object_get_uint(req, &required);

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_object_unref(&json_object);

    return result;
  }

  cardano_native_script_list_t* native_scripts = NULL;

  if (strcmp(type_string, "atLeast") == 0)
  {
    result = cardano_native_script_list_from_json(json, json_size, &native_scripts);

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

  const cardano_error_t create_n_of_k_new_result = cardano_script_n_of_k_new(native_scripts, (size_t)required, native_script);
  cardano_native_script_list_unref(&native_scripts);

  return create_n_of_k_new_result;
}

size_t
cardano_script_n_of_k_get_required(const cardano_script_n_of_k_t* script_n_of_k)
{
  if (script_n_of_k == NULL)
  {
    return 0;
  }

  return script_n_of_k->required;
}

cardano_error_t
cardano_script_n_of_k_set_required(cardano_script_n_of_k_t* script_n_of_k, size_t required)
{
  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  script_n_of_k->required = required;

  return CARDANO_SUCCESS;
}

size_t
cardano_script_n_of_k_get_length(const cardano_script_n_of_k_t* script_n_of_k)
{
  if (script_n_of_k == NULL)
  {
    return 0;
  }

  return cardano_native_script_list_get_length(script_n_of_k->scripts);
}

cardano_error_t
cardano_script_n_of_k_get_scripts(
  cardano_script_n_of_k_t*       script_n_of_k,
  cardano_native_script_list_t** list)
{
  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_list_ref(script_n_of_k->scripts);

  *list = script_n_of_k->scripts;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_n_of_k_set_scripts(
  cardano_script_n_of_k_t*      script_n_of_k,
  cardano_native_script_list_t* list)
{
  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_list_ref(list);

  cardano_native_script_list_unref(&script_n_of_k->scripts);

  script_n_of_k->scripts = list;

  return CARDANO_SUCCESS;
}

bool
cardano_script_n_of_k_equals(const cardano_script_n_of_k_t* lhs, const cardano_script_n_of_k_t* rhs)
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
cardano_script_n_of_k_unref(cardano_script_n_of_k_t** script_n_of_k)
{
  if ((script_n_of_k == NULL) || (*script_n_of_k == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*script_n_of_k)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *script_n_of_k = NULL;
    return;
  }
}

void
cardano_script_n_of_k_ref(cardano_script_n_of_k_t* script_n_of_k)
{
  if (script_n_of_k == NULL)
  {
    return;
  }

  cardano_object_ref(&script_n_of_k->base);
}

size_t
cardano_script_n_of_k_refcount(const cardano_script_n_of_k_t* script_n_of_k)
{
  if (script_n_of_k == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&script_n_of_k->base);
}

void
cardano_script_n_of_k_set_last_error(cardano_script_n_of_k_t* script_n_of_k, const char* message)
{
  cardano_object_set_last_error(&script_n_of_k->base, message);
}

const char*
cardano_script_n_of_k_get_last_error(const cardano_script_n_of_k_t* script_n_of_k)
{
  return cardano_object_get_last_error(&script_n_of_k->base);
}
