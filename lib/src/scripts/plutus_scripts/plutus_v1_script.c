/**
 * \file plutus_v1_script.c
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

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>

#include "../../allocators.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Plutus V1 was the initial version of Plutus, introduced in the Alonzo hard fork.
 */
typedef struct cardano_plutus_v1_script_t
{
    cardano_object_t  base;
    cardano_buffer_t* compiled_code;
} cardano_plutus_v1_script_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a plutus v1 script object.
 *
 * This function is responsible for properly deallocating a plutus v1 script object (`cardano_plutus_v1_script_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the plutus v1 script object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_plutus_v1_script_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the plutus_v1_script
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_plutus_v1_script_deallocate(void* object)
{
  assert(object != NULL);

  cardano_plutus_v1_script_t* data = (cardano_plutus_v1_script_t*)object;

  cardano_buffer_unref(&data->compiled_code);

  _cardano_free(data);
}

/**
 * \brief Creates a new plutus_v1_script object.
 *
 * \return A pointer to the newly created plutus_v1_script object, or `NULL` if the operation failed.
 */
static cardano_plutus_v1_script_t*
cardano_plutus_v1_script_new(void)
{
  cardano_plutus_v1_script_t* data = _cardano_malloc(sizeof(cardano_plutus_v1_script_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_plutus_v1_script_deallocate;
  data->compiled_code      = cardano_buffer_new(128);

  if (data->compiled_code == NULL)
  {
    _cardano_free(data);
    return NULL;
  }

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_plutus_v1_script_new_bytes(
  const byte_t*                bytes,
  size_t                       size,
  cardano_plutus_v1_script_t** plutus_v1_script)
{
  if (bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size == 0U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if (plutus_v1_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_v1_script_t* data = cardano_plutus_v1_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const cardano_error_t error = cardano_buffer_write(data->compiled_code, bytes, size);

  if (error != CARDANO_SUCCESS)
  {
    cardano_plutus_v1_script_unref(&data);
    return error;
  }

  *plutus_v1_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_v1_script_new_bytes_from_hex(
  const char*                  hex,
  size_t                       size,
  cardano_plutus_v1_script_t** plutus_v1_script)
{
  if (hex == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size == 0U)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if (plutus_v1_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_v1_script_t* data = cardano_plutus_v1_script_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = cardano_buffer_from_hex(hex, size);

  if (buffer == NULL)
  {
    cardano_plutus_v1_script_unref(&data);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t error = cardano_buffer_write(data->compiled_code, cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer));

  cardano_buffer_unref(&buffer);

  if (error != CARDANO_SUCCESS)
  {
    cardano_plutus_v1_script_unref(&data);
    return error;
  }

  *plutus_v1_script = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_plutus_v1_script_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_v1_script_t** plutus_v1_script)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (plutus_v1_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t* compiled_code = NULL;
  cardano_error_t   result        = cardano_cbor_reader_read_bytestring(reader, &compiled_code);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&compiled_code);
    return result;
  }

  result = cardano_plutus_v1_script_new_bytes(
    cardano_buffer_get_data(compiled_code),
    cardano_buffer_get_size(compiled_code),
    plutus_v1_script);

  cardano_buffer_unref(&compiled_code);

  return result;
}

cardano_error_t
cardano_plutus_v1_script_to_cbor(
  const cardano_plutus_v1_script_t* plutus_v1_script,
  cardano_cbor_writer_t*            writer)
{
  if (plutus_v1_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_cbor_writer_write_bytestring(
    writer,
    cardano_buffer_get_data(plutus_v1_script->compiled_code),
    cardano_buffer_get_size(plutus_v1_script->compiled_code));
}

cardano_error_t
cardano_plutus_v1_script_to_raw_bytes(
  cardano_plutus_v1_script_t* plutus_v1_script,
  cardano_buffer_t**          compiled_script)
{
  if (plutus_v1_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (compiled_script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_ref(plutus_v1_script->compiled_code);

  *compiled_script = plutus_v1_script->compiled_code;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_plutus_v1_script_get_hash(const cardano_plutus_v1_script_t* plutus_v1_script)
{
  if (plutus_v1_script == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_t* hash = NULL;

  // To compute a script hash, note that you must prepend a tag to the bytes of
  // the script before hashing. The tags in the Babbage era for PlutusV1 is "\x01"
  static const byte_t plutus_v1_prefix = 0x01;

  cardano_buffer_t* hash_input = cardano_buffer_new((size_t)CARDANO_BLAKE2B_HASH_SIZE_224 + 1U);

  if (hash_input == NULL)
  {
    return NULL;
  }

  cardano_error_t error = cardano_buffer_write(hash_input, &plutus_v1_prefix, 1);

  if (error != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&hash_input);
    return NULL;
  }

  error = cardano_buffer_write(hash_input, cardano_buffer_get_data(plutus_v1_script->compiled_code), cardano_buffer_get_size(plutus_v1_script->compiled_code));

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
cardano_plutus_v1_script_equals(const cardano_plutus_v1_script_t* lhs, const cardano_plutus_v1_script_t* rhs)
{
  if ((lhs == NULL) || (rhs == NULL))
  {
    return false;
  }

  return cardano_buffer_equals(lhs->compiled_code, rhs->compiled_code);
}

void
cardano_plutus_v1_script_unref(cardano_plutus_v1_script_t** plutus_v1_script)
{
  if ((plutus_v1_script == NULL) || (*plutus_v1_script == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*plutus_v1_script)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *plutus_v1_script = NULL;
    return;
  }
}

void
cardano_plutus_v1_script_ref(cardano_plutus_v1_script_t* plutus_v1_script)
{
  if (plutus_v1_script == NULL)
  {
    return;
  }

  cardano_object_ref(&plutus_v1_script->base);
}

size_t
cardano_plutus_v1_script_refcount(const cardano_plutus_v1_script_t* plutus_v1_script)
{
  if (plutus_v1_script == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&plutus_v1_script->base);
}

void
cardano_plutus_v1_script_set_last_error(cardano_plutus_v1_script_t* plutus_v1_script, const char* message)
{
  cardano_object_set_last_error(&plutus_v1_script->base, message);
}

const char*
cardano_plutus_v1_script_get_last_error(const cardano_plutus_v1_script_t* plutus_v1_script)
{
  return cardano_object_get_last_error(&plutus_v1_script->base);
}
