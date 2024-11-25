/**
 * \file script.c
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

#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/scripts/script.h>
#include <cardano/scripts/script_language.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Program that decides whether the transaction that spends the output is authorized to do so.
 */
typedef struct cardano_script_t
{
    cardano_object_t            base;
    cardano_script_language_t   language;
    cardano_native_script_t*    native_script;
    cardano_plutus_v1_script_t* plutus_v1_script;
    cardano_plutus_v2_script_t* plutus_v2_script;
    cardano_plutus_v3_script_t* plutus_v3_script;
} cardano_script_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a script object.
 *
 * This function is responsible for properly deallocating a script object (`cardano_script_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the script object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_script_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the script
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_script_deallocate(void* object)
{
  assert(object != NULL);

  cardano_script_t* data = (cardano_script_t*)object;

  cardano_native_script_unref(&data->native_script);
  cardano_plutus_v1_script_unref(&data->plutus_v1_script);
  cardano_plutus_v2_script_unref(&data->plutus_v2_script);
  cardano_plutus_v3_script_unref(&data->plutus_v3_script);

  _cardano_free(data);
}

/**
 * \brief Creates a new script object.
 *
 * \return A pointer to the newly created script object, or `NULL` if the operation failed.
 */
static cardano_script_t*
cardano_script_new(void)
{
  cardano_script_t* data = _cardano_malloc(sizeof(cardano_script_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_script_deallocate;

  data->native_script    = NULL;
  data->plutus_v1_script = NULL;
  data->plutus_v2_script = NULL;
  data->plutus_v3_script = NULL;
  data->language         = CARDANO_SCRIPT_LANGUAGE_NATIVE;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_script_new_native(
  cardano_native_script_t* native_script,
  cardano_script_t**       script)
{
  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_t* data = cardano_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_native_script_ref(native_script);

  data->native_script = native_script;
  data->language      = CARDANO_SCRIPT_LANGUAGE_NATIVE;

  *script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_new_plutus_v1(
  cardano_plutus_v1_script_t* plutus_v1_script,
  cardano_script_t**          script)
{
  if (plutus_v1_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_t* data = cardano_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_plutus_v1_script_ref(plutus_v1_script);

  data->plutus_v1_script = plutus_v1_script;
  data->language         = CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1;

  *script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_new_plutus_v2(
  cardano_plutus_v2_script_t* plutus_v2_script,
  cardano_script_t**          script)
{
  if (plutus_v2_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_t* data = cardano_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_plutus_v2_script_ref(plutus_v2_script);

  data->plutus_v2_script = plutus_v2_script;
  data->language         = CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2;

  *script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_new_plutus_v3(
  cardano_plutus_v3_script_t* plutus_v3_script,
  cardano_script_t**          script)
{
  if (plutus_v3_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_t* data = cardano_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_plutus_v3_script_ref(plutus_v3_script);

  data->plutus_v3_script = plutus_v3_script;
  data->language         = CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3;

  *script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_from_cbor(cardano_cbor_reader_t* reader, cardano_script_t** script)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "script";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              language         = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "language",
    reader,
    &language,
    CARDANO_SCRIPT_LANGUAGE_NATIVE,
    CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_error_t result = CARDANO_SUCCESS;
  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
    {
      cardano_native_script_t* native_script = NULL;
      result                                 = cardano_native_script_from_cbor(reader, &native_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_script_new_native(native_script, script);
      cardano_native_script_unref(&native_script);

      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      cardano_plutus_v1_script_t* plutus_v1_script = NULL;
      result                                       = cardano_plutus_v1_script_from_cbor(reader, &plutus_v1_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_script_new_plutus_v1(plutus_v1_script, script);
      cardano_plutus_v1_script_unref(&plutus_v1_script);

      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      cardano_plutus_v2_script_t* plutus_v2_script = NULL;
      result                                       = cardano_plutus_v2_script_from_cbor(reader, &plutus_v2_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_script_new_plutus_v2(plutus_v2_script, script);
      cardano_plutus_v2_script_unref(&plutus_v2_script);

      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      cardano_plutus_v3_script_t* plutus_v3_script = NULL;
      result                                       = cardano_plutus_v3_script_from_cbor(reader, &plutus_v3_script);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_script_new_plutus_v3(plutus_v3_script, script);
      cardano_plutus_v3_script_unref(&plutus_v3_script);

      break;
    }

    default:
      result = CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
      break;
  }

  return result;
}

cardano_error_t
cardano_script_to_cbor(
  const cardano_script_t* script,
  cardano_cbor_writer_t*  writer)
{
  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, script->language);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (script->language)
  {
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
      result = cardano_native_script_to_cbor(script->native_script, writer);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
      result = cardano_plutus_v1_script_to_cbor(script->plutus_v1_script, writer);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
      result = cardano_plutus_v2_script_to_cbor(script->plutus_v2_script, writer);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
      result = cardano_plutus_v3_script_to_cbor(script->plutus_v3_script, writer);
      break;

    default:
      result = CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
      break;
  }

  return result;
}

cardano_error_t
cardano_script_get_language(
  const cardano_script_t*    script,
  cardano_script_language_t* language)
{
  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (language == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *language = script->language;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_to_native(
  cardano_script_t*         script,
  cardano_native_script_t** native_script)
{
  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (native_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script->language != CARDANO_SCRIPT_LANGUAGE_NATIVE)
  {
    return CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
  }

  cardano_native_script_ref(script->native_script);

  *native_script = script->native_script;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_to_plutus_v1(
  cardano_script_t*            script,
  cardano_plutus_v1_script_t** plutus_v1)
{
  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_v1 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script->language != CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1)
  {
    return CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
  }

  cardano_plutus_v1_script_ref(script->plutus_v1_script);

  *plutus_v1 = script->plutus_v1_script;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_to_plutus_v2(
  cardano_script_t*            script,
  cardano_plutus_v2_script_t** plutus_v2)
{
  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_v2 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script->language != CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2)
  {
    return CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
  }

  cardano_plutus_v2_script_ref(script->plutus_v2_script);

  *plutus_v2 = script->plutus_v2_script;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_to_plutus_v3(
  cardano_script_t*            script,
  cardano_plutus_v3_script_t** plutus_v3)
{
  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_v3 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script->language != CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3)
  {
    return CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
  }

  cardano_plutus_v3_script_ref(script->plutus_v3_script);

  *plutus_v3 = script->plutus_v3_script;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_script_get_hash(const cardano_script_t* script)
{
  if (script == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_t* hash = NULL;

  switch (script->language)
  {
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
      hash = cardano_native_script_get_hash(script->native_script);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
      hash = cardano_plutus_v1_script_get_hash(script->plutus_v1_script);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
      hash = cardano_plutus_v2_script_get_hash(script->plutus_v2_script);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
      hash = cardano_plutus_v3_script_get_hash(script->plutus_v3_script);
      break;

    default:
      break;
  }

  return hash;
}

bool
cardano_script_equals(const cardano_script_t* lhs, const cardano_script_t* rhs)
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

  if (lhs->language != rhs->language)
  {
    return false;
  }

  bool result = false;
  switch (lhs->language)
  {
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
      result = cardano_native_script_equals(lhs->native_script, rhs->native_script);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
      result = cardano_plutus_v1_script_equals(lhs->plutus_v1_script, rhs->plutus_v1_script);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
      result = cardano_plutus_v2_script_equals(lhs->plutus_v2_script, rhs->plutus_v2_script);
      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
      result = cardano_plutus_v3_script_equals(lhs->plutus_v3_script, rhs->plutus_v3_script);
      break;

    default:
      return false;
  }

  return result;
}

void
cardano_script_unref(cardano_script_t** script)
{
  if ((script == NULL) || (*script == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*script)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *script = NULL;
    return;
  }
}

void
cardano_script_ref(cardano_script_t* script)
{
  if (script == NULL)
  {
    return;
  }

  cardano_object_ref(&script->base);
}

size_t
cardano_script_refcount(const cardano_script_t* script)
{
  if (script == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&script->base);
}

void
cardano_script_set_last_error(cardano_script_t* script, const char* message)
{
  cardano_object_set_last_error(&script->base, message);
}

const char*
cardano_script_get_last_error(const cardano_script_t* script)
{
  return cardano_object_get_last_error(&script->base);
}
