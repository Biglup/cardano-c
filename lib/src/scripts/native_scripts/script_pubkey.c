/**
 * \file script_pubkey.c
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
#include <cardano/scripts/native_scripts/script_pubkey.h>

#include <cardano/object.h>

#include "../../allocators.h"
#include "../../cbor/cbor_validation.h"

#include <assert.h>
#include <cardano/json/json_object.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief This script evaluates to true if the transaction also includes a valid key witness
 * where the witness verification key hashes to the given hash.
 *
 * In other words, this checks that the transaction is signed by a particular key, identified by its verification
 * key hash.
 */
typedef struct cardano_script_pubkey_t
{
    cardano_object_t             base;
    cardano_native_script_type_t type;
    cardano_blake2b_hash_t*      key_hash;

} cardano_script_pubkey_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a script_pubkey object.
 *
 * This function is responsible for properly deallocating a script_pubkey object (`cardano_script_pubkey_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the script_pubkey object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_script_pubkey_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the script_pubkey
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_script_pubkey_deallocate(void* object)
{
  assert(object != NULL);

  cardano_script_pubkey_t* data = (cardano_script_pubkey_t*)object;

  cardano_blake2b_hash_unref(&data->key_hash);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_script_pubkey_new(cardano_blake2b_hash_t* key_hash, cardano_script_pubkey_t** script_pubkey)
{
  if (key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_pubkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_pubkey_t* data = _cardano_malloc(sizeof(cardano_script_pubkey_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_blake2b_hash_ref(key_hash);

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_script_pubkey_deallocate;
  data->type               = CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY;
  data->key_hash           = key_hash;

  *script_pubkey = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_script_pubkey_from_cbor(cardano_cbor_reader_t* reader, cardano_script_pubkey_t** script_pubkey)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_pubkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "script_pubkey";

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
    CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY,
    (enum_to_string_callback_t)((void*)&cardano_native_script_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_blake2b_hash_t* key_hash             = NULL;
  const cardano_error_t   read_key_hash_result = cardano_blake2b_hash_from_cbor(reader, &key_hash);

  if (read_key_hash_result != CARDANO_SUCCESS)
  {
    return read_key_hash_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&key_hash);
    return expect_end_array_result;
  }

  const cardano_error_t create_pubkey_new_result = cardano_script_pubkey_new(key_hash, script_pubkey);
  cardano_blake2b_hash_unref(&key_hash);

  return create_pubkey_new_result;
}

cardano_error_t
cardano_script_pubkey_to_cbor(
  const cardano_script_pubkey_t* script_pubkey,
  cardano_cbor_writer_t*         writer)
{
  if (script_pubkey == NULL)
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

  result = cardano_cbor_writer_write_uint(writer, script_pubkey->type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_blake2b_hash_to_cbor(script_pubkey->key_hash, writer);

  return result;
}

cardano_error_t
cardano_script_pubkey_from_json(const char* json, size_t json_size, cardano_script_pubkey_t** native_script)
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

  cardano_error_t result = CARDANO_SUCCESS;

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

  cardano_json_object_t* hash_object = NULL;

  if (!cardano_json_object_get_ex(json_object, "keyHash", 7, &hash_object))
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_JSON;
  }

  size_t      hash_len    = 0U;
  const char* hash_string = cardano_json_object_get_string(hash_object, &hash_len);

  if (hash_string == NULL)
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_blake2b_hash_t* hash = NULL;

  if (strcmp(type_string, "sig") != 0)
  {
    cardano_json_object_unref(&json_object);

    return CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
  }

  result = cardano_blake2b_hash_from_hex(hash_string, hash_len - 1U, &hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_object_unref(&json_object);

    return result;
  }

  cardano_json_object_unref(&json_object);

  const cardano_error_t create_pubkey_new_result = cardano_script_pubkey_new(hash, native_script);
  cardano_blake2b_hash_unref(&hash);

  return create_pubkey_new_result;
}

cardano_error_t
cardano_script_pubkey_get_key_hash(cardano_script_pubkey_t* script_pubkey, cardano_blake2b_hash_t** key_hash)
{
  if (script_pubkey == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(script_pubkey->key_hash);
  *key_hash = script_pubkey->key_hash;

  return CARDANO_SUCCESS;
}

bool
cardano_script_pubkey_equals(const cardano_script_pubkey_t* lhs, const cardano_script_pubkey_t* rhs)
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

  if (cardano_blake2b_hash_get_bytes_size(lhs->key_hash) != cardano_blake2b_hash_get_bytes_size(rhs->key_hash))
  {
    return false;
  }

  if (memcmp(cardano_blake2b_hash_get_data(lhs->key_hash), cardano_blake2b_hash_get_data(rhs->key_hash), cardano_blake2b_hash_get_bytes_size(rhs->key_hash)) != 0)
  {
    return false;
  }

  return true;
}

void
cardano_script_pubkey_unref(cardano_script_pubkey_t** script_pubkey)
{
  if ((script_pubkey == NULL) || (*script_pubkey == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*script_pubkey)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *script_pubkey = NULL;
    return;
  }
}

void
cardano_script_pubkey_ref(cardano_script_pubkey_t* script_pubkey)
{
  if (script_pubkey == NULL)
  {
    return;
  }

  cardano_object_ref(&script_pubkey->base);
}

size_t
cardano_script_pubkey_refcount(const cardano_script_pubkey_t* script_pubkey)
{
  if (script_pubkey == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&script_pubkey->base);
}

void
cardano_script_pubkey_set_last_error(cardano_script_pubkey_t* script_pubkey, const char* message)
{
  cardano_object_set_last_error(&script_pubkey->base, message);
}

const char*
cardano_script_pubkey_get_last_error(const cardano_script_pubkey_t* script_pubkey)
{
  return cardano_object_get_last_error(&script_pubkey->base);
}
