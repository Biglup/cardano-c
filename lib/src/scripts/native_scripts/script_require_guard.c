/**
 * \file script_require_guard.c
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
 *
 * Copyright 2026 Biglup Labs
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
#include <cardano/scripts/native_scripts/script_require_guard.h>

#include <cardano/object.h>

#include "../../allocators.h"
#include "../../cbor/cbor_validation.h"

#include <assert.h>
#include <cardano/json/json_object.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief This script evaluates to true if the given credential is present in the
 * transaction guards.
 *
 * In other words, this checks that the transaction was authorized by a particular guard,
 * identified by its key hash or script hash credential.
 */
typedef struct cardano_script_require_guard_t
{
    cardano_object_t             base;
    cardano_native_script_type_t type;
    cardano_credential_t*        credential;

} cardano_script_require_guard_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a script_require_guard object.
 *
 * This function is responsible for properly deallocating a script_require_guard object (`cardano_script_require_guard_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the script_require_guard object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_script_require_guard_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the script_require_guard
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_script_require_guard_deallocate(void* object)
{
  assert(object != NULL);

  cardano_script_require_guard_t* data = (cardano_script_require_guard_t*)object;

  cardano_credential_unref(&data->credential);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_script_require_guard_new(cardano_credential_t* credential, cardano_script_require_guard_t** script_require_guard)
{
  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_require_guard == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_require_guard_t* data = _cardano_malloc(sizeof(cardano_script_require_guard_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_credential_ref(credential);

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_script_require_guard_deallocate;
  data->type               = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_GUARD;
  data->credential         = credential;

  *script_require_guard = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_require_guard_from_cbor(cardano_cbor_reader_t* reader, cardano_script_require_guard_t** script_require_guard)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_require_guard == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "script_require_guard";

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
    CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_GUARD,
    (enum_to_string_callback_t)((void*)&cardano_native_script_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_credential_t* credential             = NULL;
  const cardano_error_t read_credential_result = cardano_credential_from_cbor(reader, &credential);

  if (read_credential_result != CARDANO_SUCCESS)
  {
    return read_credential_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);
    return expect_end_array_result;
  }

  const cardano_error_t create_require_guard_new_result = cardano_script_require_guard_new(credential, script_require_guard);
  cardano_credential_unref(&credential);

  return create_require_guard_new_result;
}

cardano_error_t
cardano_script_require_guard_to_cbor(
  const cardano_script_require_guard_t* script_require_guard,
  cardano_cbor_writer_t*                writer)
{
  if (script_require_guard == NULL)
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

  result = cardano_cbor_writer_write_uint(writer, script_require_guard->type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_credential_to_cbor(script_require_guard->credential, writer);

  return result;
}

cardano_error_t
cardano_script_require_guard_to_cip116_json(
  const cardano_script_require_guard_t* script_require_guard,
  cardano_json_writer_t*                writer)
{
  if ((script_require_guard == NULL) || (writer == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "tag", 3);
  cardano_json_writer_write_string(writer, "require_guard", 13);
  cardano_json_writer_write_property_name(writer, "credential", 10);
  cardano_error_t error = cardano_credential_to_cip116_json(script_require_guard->credential, writer);

  assert(error == CARDANO_SUCCESS);
  CARDANO_UNUSED(error);

  cardano_json_writer_write_end_object(writer);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_require_guard_from_json(const char* json, size_t json_size, cardano_script_require_guard_t** native_script)
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

  cardano_json_object_t* type     = NULL;
  bool                   has_type = cardano_json_object_get_ex(json_object, "type", 4, &type);

  if (!has_type)
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

  if (strcmp(type_string, "guard") != 0)
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  cardano_credential_type_t credential_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  cardano_json_object_t*    hash_object     = NULL;

  if (!cardano_json_object_get_ex(json_object, "keyHash", 7, &hash_object))
  {
    if (!cardano_json_object_get_ex(json_object, "scriptHash", 10, &hash_object))
    {
      cardano_json_object_unref(&json_object);

      return CARDANO_ERROR_INVALID_JSON;
    }

    credential_type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
  }

  size_t      hash_len    = 0U;
  const char* hash_string = cardano_json_object_get_string(hash_object, &hash_len);

  if (hash_string == NULL)
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_credential_t* credential = NULL;

  cardano_error_t result = cardano_credential_from_hash_hex(hash_string, hash_len - 1U, credential_type, &credential);

  cardano_json_object_unref(&json_object);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const cardano_error_t create_require_guard_new_result = cardano_script_require_guard_new(credential, native_script);
  cardano_credential_unref(&credential);

  return create_require_guard_new_result;
}

cardano_error_t
cardano_script_require_guard_get_credential(cardano_script_require_guard_t* script_require_guard, cardano_credential_t** credential)
{
  if (script_require_guard == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(script_require_guard->credential);
  *credential = script_require_guard->credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_require_guard_set_credential(cardano_script_require_guard_t* script_require_guard, cardano_credential_t* credential)
{
  if (script_require_guard == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(credential);
  cardano_credential_unref(&script_require_guard->credential);
  script_require_guard->credential = credential;

  return CARDANO_SUCCESS;
}

bool
cardano_script_require_guard_equals(const cardano_script_require_guard_t* lhs, const cardano_script_require_guard_t* rhs)
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

  return cardano_credential_equals(lhs->credential, rhs->credential);
}

void
cardano_script_require_guard_unref(cardano_script_require_guard_t** script_require_guard)
{
  if ((script_require_guard == NULL) || (*script_require_guard == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*script_require_guard)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *script_require_guard = NULL;
    return;
  }
}

void
cardano_script_require_guard_ref(cardano_script_require_guard_t* script_require_guard)
{
  if (script_require_guard == NULL)
  {
    return;
  }

  cardano_object_ref(&script_require_guard->base);
}

size_t
cardano_script_require_guard_refcount(const cardano_script_require_guard_t* script_require_guard)
{
  if (script_require_guard == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&script_require_guard->base);
}

void
cardano_script_require_guard_set_last_error(cardano_script_require_guard_t* script_require_guard, const char* message)
{
  cardano_object_set_last_error(&script_require_guard->base, message);
}

const char*
cardano_script_require_guard_get_last_error(const cardano_script_require_guard_t* script_require_guard)
{
  return cardano_object_get_last_error(&script_require_guard->base);
}
