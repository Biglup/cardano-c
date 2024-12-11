/**
 * \file native_script.c
 *
 * \author angel.castillo
 * \date   May 28, 2024
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

#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/error.h>
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/native_scripts/native_script_type.h>
#include <cardano/scripts/native_scripts/script_all.h>
#include <cardano/scripts/native_scripts/script_any.h>
#include <cardano/scripts/native_scripts/script_invalid_after.h>
#include <cardano/scripts/native_scripts/script_invalid_before.h>
#include <cardano/scripts/native_scripts/script_n_of_k.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>

#include <cardano/object.h>

#include "../../allocators.h"
#include "../../cbor/cbor_validation.h"
#include "../../string_safe.h"

#include <assert.h>
#include <cardano/json/json_object.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief The Native scripts form an expression tree, the evaluation of the script produces either true or false.
 */
typedef struct cardano_native_script_t
{
    cardano_object_t                 base;
    cardano_native_script_type_t     type;
    cardano_script_all_t*            all;
    cardano_script_any_t*            any;
    cardano_script_invalid_after_t*  invalid_after;
    cardano_script_invalid_before_t* invalid_before;
    cardano_script_n_of_k_t*         n_of_k;
    cardano_script_pubkey_t*         pubkey;

} cardano_native_script_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a native_script object.
 *
 * This function is responsible for properly deallocating a native_script object (`cardano_native_script_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the native_script object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_native_script_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the native_script
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_native_script_deallocate(void* object)
{
  assert(object != NULL);

  cardano_native_script_t* data = (cardano_native_script_t*)object;

  cardano_script_all_unref(&data->all);
  cardano_script_any_unref(&data->any);
  cardano_script_invalid_after_unref(&data->invalid_after);
  cardano_script_invalid_before_unref(&data->invalid_before);
  cardano_script_n_of_k_unref(&data->n_of_k);
  cardano_script_pubkey_unref(&data->pubkey);

  _cardano_free(data);
}

/**
 * \brief Creates a new native_script object.
 *
 * \return A pointer to the newly created native_script object, or `NULL` if the operation failed.
 */
static cardano_native_script_t*
cardano_native_script_new(void)
{
  cardano_native_script_t* data = _cardano_malloc(sizeof(cardano_native_script_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_native_script_deallocate;

  data->type           = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF;
  data->all            = NULL;
  data->any            = NULL;
  data->invalid_after  = NULL;
  data->invalid_before = NULL;
  data->n_of_k         = NULL;
  data->pubkey         = NULL;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_native_script_new_all(
  cardano_script_all_t*     script_all,
  cardano_native_script_t** native_script)
{
  if (script_all == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_t* data = cardano_native_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF;
  data->all  = script_all;

  cardano_script_all_ref(script_all);

  *native_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_new_any(
  cardano_script_any_t*     script_any,
  cardano_native_script_t** native_script)
{
  if (script_any == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_t* data = cardano_native_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF;
  data->any  = script_any;

  cardano_script_any_ref(script_any);

  *native_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_new_n_of_k(
  cardano_script_n_of_k_t*  script_n_of_k,
  cardano_native_script_t** native_script)
{
  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_t* data = cardano_native_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type   = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K;
  data->n_of_k = script_n_of_k;

  cardano_script_n_of_k_ref(script_n_of_k);

  *native_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_new_pubkey(
  cardano_script_pubkey_t*  script_pubkey,
  cardano_native_script_t** native_script)
{
  if (script_pubkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_t* data = cardano_native_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type   = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY;
  data->pubkey = script_pubkey;

  cardano_script_pubkey_ref(script_pubkey);

  *native_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_new_invalid_after(
  cardano_script_invalid_after_t* invalid_after,
  cardano_native_script_t**       native_script)
{
  if (invalid_after == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_t* data = cardano_native_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type          = CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER;
  data->invalid_after = invalid_after;

  cardano_script_invalid_after_ref(invalid_after);

  *native_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_new_invalid_before(
  cardano_script_invalid_before_t* invalid_before,
  cardano_native_script_t**        native_script)
{
  if (invalid_before == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_native_script_t* data = cardano_native_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type           = CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE;
  data->invalid_before = invalid_before;

  cardano_script_invalid_before_ref(invalid_before);

  *native_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_from_cbor(cardano_cbor_reader_t* reader, cardano_native_script_t** native_script)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_reader_t* reader_clone = NULL;

  cardano_error_t result = cardano_cbor_reader_clone(reader, &reader_clone);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  static const char* validator_name = "native_script";

  int64_t array_size = 0;
  result             = cardano_cbor_reader_read_start_array(reader_clone, &array_size);

  CARDANO_UNUSED(array_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader_clone);
    return result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "type",
    reader_clone,
    &type,
    CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY,
    CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER);

  cardano_cbor_reader_unref(&reader_clone);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  switch (type)
  {
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF:
    {
      cardano_script_all_t* script_all = NULL;
      result                           = cardano_script_all_from_cbor(reader, &script_all);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_native_script_new_all(script_all, native_script);
      cardano_script_all_unref(&script_all);

      break;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF:
    {
      cardano_script_any_t* script_any = NULL;
      result                           = cardano_script_any_from_cbor(reader, &script_any);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_native_script_new_any(script_any, native_script);
      cardano_script_any_unref(&script_any);

      break;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K:
    {
      cardano_script_n_of_k_t* script_n_of_k = NULL;
      result                                 = cardano_script_n_of_k_from_cbor(reader, &script_n_of_k);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_native_script_new_n_of_k(script_n_of_k, native_script);
      cardano_script_n_of_k_unref(&script_n_of_k);

      break;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY:
    {
      cardano_script_pubkey_t* script_pubkey = NULL;
      result                                 = cardano_script_pubkey_from_cbor(reader, &script_pubkey);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_native_script_new_pubkey(script_pubkey, native_script);
      cardano_script_pubkey_unref(&script_pubkey);

      break;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER:
    {
      cardano_script_invalid_after_t* invalid_after = NULL;
      result                                        = cardano_script_invalid_after_from_cbor(reader, &invalid_after);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_native_script_new_invalid_after(invalid_after, native_script);
      cardano_script_invalid_after_unref(&invalid_after);

      break;
    }
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE:
    {
      cardano_script_invalid_before_t* invalid_before = NULL;
      result                                          = cardano_script_invalid_before_from_cbor(reader, &invalid_before);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_native_script_new_invalid_before(invalid_before, native_script);
      cardano_script_invalid_before_unref(&invalid_before);

      break;
    }

    default:
      result = CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
      break;
  }

  return result;
}

cardano_error_t
cardano_native_script_to_cbor(
  const cardano_native_script_t* native_script,
  cardano_cbor_writer_t*         writer)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  switch (native_script->type)
  {
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF:
      result = cardano_script_all_to_cbor(native_script->all, writer);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF:
      result = cardano_script_any_to_cbor(native_script->any, writer);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K:
      result = cardano_script_n_of_k_to_cbor(native_script->n_of_k, writer);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY:
      result = cardano_script_pubkey_to_cbor(native_script->pubkey, writer);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER:
      result = cardano_script_invalid_after_to_cbor(native_script->invalid_after, writer);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE:
      result = cardano_script_invalid_before_to_cbor(native_script->invalid_before, writer);
      break;

    default:
      result = CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
      break;
  }

  return result;
}

cardano_error_t
cardano_native_script_from_json(const char* json, const size_t json_size, cardano_native_script_t** native_script)
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

  cardano_error_t          result = CARDANO_SUCCESS;
  cardano_native_script_t* data   = NULL;

  cardano_json_object_t* type = NULL;

  if (!cardano_json_object_get_ex(json_object, "type", 4, &type))
  {
    cardano_json_object_unref(&json_object);
    cardano_native_script_unref(&data);

    return CARDANO_ERROR_INVALID_JSON;
  }

  const char* type_string = cardano_json_object_get_string(type, NULL);

  if (type_string == NULL)
  {
    cardano_json_object_unref(&json_object);
    cardano_native_script_unref(&data);

    return CARDANO_ERROR_INVALID_JSON;
  }

  if (strcmp(type_string, "all") == 0)
  {
    cardano_script_all_t* script_all = NULL;
    result                           = cardano_script_all_from_json(json, json_size, &script_all);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&json_object);
      cardano_native_script_unref(&data);
      return result;
    }

    result = cardano_native_script_new_all(script_all, &data);
    cardano_script_all_unref(&script_all);
  }
  else if (strcmp(type_string, "any") == 0)
  {
    cardano_script_any_t* script_any = NULL;
    result                           = cardano_script_any_from_json(json, json_size, &script_any);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&json_object);
      cardano_native_script_unref(&data);
      return result;
    }

    result = cardano_native_script_new_any(script_any, &data);
    cardano_script_any_unref(&script_any);
  }
  else if (strcmp(type_string, "atLeast") == 0)
  {
    cardano_script_n_of_k_t* script_n_of_k = NULL;
    result                                 = cardano_script_n_of_k_from_json(json, json_size, &script_n_of_k);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&json_object);
      cardano_native_script_unref(&data);
      return result;
    }

    result = cardano_native_script_new_n_of_k(script_n_of_k, &data);
    cardano_script_n_of_k_unref(&script_n_of_k);
  }
  else if (strcmp(type_string, "sig") == 0)
  {
    cardano_script_pubkey_t* script_pubkey = NULL;
    result                                 = cardano_script_pubkey_from_json(json, json_size, &script_pubkey);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&json_object);
      cardano_native_script_unref(&data);
      return result;
    }

    result = cardano_native_script_new_pubkey(script_pubkey, &data);
    cardano_script_pubkey_unref(&script_pubkey);
  }
  else if (strcmp(type_string, "after") == 0)
  {
    cardano_script_invalid_after_t* invalid_after = NULL;
    result                                        = cardano_script_invalid_after_from_json(json, json_size, &invalid_after);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&json_object);
      cardano_native_script_unref(&data);
      return result;
    }

    result = cardano_native_script_new_invalid_after(invalid_after, &data);
    cardano_script_invalid_after_unref(&invalid_after);
  }
  else if (strcmp(type_string, "before") == 0)
  {
    cardano_script_invalid_before_t* invalid_before = NULL;
    result                                          = cardano_script_invalid_before_from_json(json, json_size, &invalid_before);

    if (result != CARDANO_SUCCESS)
    {
      cardano_json_object_unref(&json_object);
      cardano_native_script_unref(&data);
      return result;
    }

    result = cardano_native_script_new_invalid_before(invalid_before, &data);
    cardano_script_invalid_before_unref(&invalid_before);
  }
  else
  {
    cardano_json_object_unref(&json_object);
    cardano_native_script_unref(&data);
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_json_object_unref(&json_object);

  if (result != CARDANO_SUCCESS)
  {
    cardano_native_script_unref(&data);
    return result;
  }

  *native_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_get_type(
  const cardano_native_script_t* native_script,
  cardano_native_script_type_t*  type)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = native_script->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_to_all(
  cardano_native_script_t* native_script,
  cardano_script_all_t**   script_all)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_all == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script->type != CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF)
  {
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_script_all_ref(native_script->all);

  *script_all = native_script->all;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_to_any(
  cardano_native_script_t* native_script,
  cardano_script_any_t**   script_any)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_any == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script->type != CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF)
  {
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_script_any_ref(native_script->any);

  *script_any = native_script->any;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_to_n_of_k(
  cardano_native_script_t*  native_script,
  cardano_script_n_of_k_t** script_n_of_k)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_n_of_k == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script->type != CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K)
  {
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_script_n_of_k_ref(native_script->n_of_k);

  *script_n_of_k = native_script->n_of_k;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_to_pubkey(
  cardano_native_script_t*  native_script,
  cardano_script_pubkey_t** script_pubkey)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_pubkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script->type != CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY)
  {
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_script_pubkey_ref(native_script->pubkey);

  *script_pubkey = native_script->pubkey;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_to_invalid_after(
  cardano_native_script_t*         native_script,
  cardano_script_invalid_after_t** invalid_after)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (invalid_after == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script->type != CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER)
  {
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_script_invalid_after_ref(native_script->invalid_after);

  *invalid_after = native_script->invalid_after;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_native_script_to_invalid_before(
  cardano_native_script_t*          native_script,
  cardano_script_invalid_before_t** invalid_before)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (invalid_before == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script->type != CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE)
  {
    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_script_invalid_before_ref(native_script->invalid_before);

  *invalid_before = native_script->invalid_before;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_native_script_get_hash(
  const cardano_native_script_t* native_script)
{
  if (native_script == NULL)
  {
    return NULL;
  }

  cardano_buffer_t*      cbor   = NULL;
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return NULL;
  }

  cardano_error_t result = cardano_native_script_to_cbor(native_script, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return NULL;
  }

  result = cardano_cbor_writer_encode_in_buffer(writer, &cbor);

  cardano_cbor_writer_unref(&writer);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_blake2b_hash_t* hash = NULL;

  static const byte_t native_prefix = 0x00;

  cardano_buffer_t* hash_input = cardano_buffer_new(cardano_buffer_get_size(cbor) + 1U);

  if (hash_input == NULL)
  {
    cardano_buffer_unref(&cbor);
    return NULL;
  }

  cardano_error_t error = cardano_buffer_write(hash_input, &native_prefix, 1);

  if (error != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&cbor);
    cardano_buffer_unref(&hash_input);

    return NULL;
  }

  error = cardano_buffer_write(hash_input, cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor));

  cardano_buffer_unref(&cbor);

  if (error != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&hash_input);

    return NULL;
  }

  error = cardano_blake2b_compute_hash(
    cardano_buffer_get_data(hash_input),
    cardano_buffer_get_size(hash_input),
    CARDANO_BLAKE2B_HASH_SIZE_224,
    &hash);

  cardano_buffer_unref(&hash_input);

  if (error != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return hash;
}

bool
cardano_native_script_equals(
  const cardano_native_script_t* lhs,
  const cardano_native_script_t* rhs)
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

  bool result = false;
  switch (lhs->type)
  {
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF:
      result = cardano_script_all_equals(lhs->all, rhs->all);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF:
      result = cardano_script_any_equals(lhs->any, rhs->any);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K:
      result = cardano_script_n_of_k_equals(lhs->n_of_k, rhs->n_of_k);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY:
      result = cardano_script_pubkey_equals(lhs->pubkey, rhs->pubkey);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER:
      result = cardano_script_invalid_after_equals(lhs->invalid_after, rhs->invalid_after);
      break;
    case CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE:
      result = cardano_script_invalid_before_equals(lhs->invalid_before, rhs->invalid_before);
      break;

    default:
      return false;
  }

  return result;
}

void
cardano_native_script_unref(cardano_native_script_t** native_script)
{
  if ((native_script == NULL) || (*native_script == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*native_script)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *native_script = NULL;
    return;
  }
}

void
cardano_native_script_ref(cardano_native_script_t* native_script)
{
  if (native_script == NULL)
  {
    return;
  }

  cardano_object_ref(&native_script->base);
}

size_t
cardano_native_script_refcount(const cardano_native_script_t* native_script)
{
  if (native_script == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&native_script->base);
}

void
cardano_native_script_set_last_error(cardano_native_script_t* native_script, const char* message)
{
  cardano_object_set_last_error(&native_script->base, message);
}

const char*
cardano_native_script_get_last_error(const cardano_native_script_t* native_script)
{
  return cardano_object_get_last_error(&native_script->base);
}
