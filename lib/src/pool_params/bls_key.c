/**
 * \file bls_key.c
 *
 * \author angel.castillo
 * \date   Sep 05, 2026
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

#include <cardano/buffer.h>
#include <cardano/object.h>
#include <cardano/pool_params/bls_key.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <sodium/core.h>
#include <sodium/utils.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE       = 2;
static const size_t  BLS_PUBLIC_KEY_SIZE       = 96U;
static const size_t  BLS_POSSESSION_PROOF_SIZE = 48U;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a stake pool BLS key.
 *
 * Each instance of `cardano_bls_key_t` holds a BLS public key together with its proof of possession.
 */
typedef struct cardano_bls_key_t
{
    cardano_object_t base;
    byte_t           public_key[96];
    byte_t           possession_proof[48];
} cardano_bls_key_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a bls_key object.
 *
 * This function is responsible for properly deallocating a bls_key object (`cardano_bls_key_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the bls_key object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_bls_key_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the bls_key
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_bls_key_deallocate(void* object)
{
  assert(object != NULL);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_bls_key_new(
  const byte_t*       public_key,
  const size_t        public_key_size,
  const byte_t*       possession_proof,
  const size_t        possession_proof_size,
  cardano_bls_key_t** bls_key)
{
  if (bls_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *bls_key = NULL;

  if ((public_key == NULL) || (possession_proof == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((public_key_size != BLS_PUBLIC_KEY_SIZE) || (possession_proof_size != BLS_POSSESSION_PROOF_SIZE))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *bls_key = _cardano_malloc(sizeof(cardano_bls_key_t));

  if (*bls_key == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*bls_key)->base.deallocator   = cardano_bls_key_deallocate;
  (*bls_key)->base.ref_count     = 1;
  (*bls_key)->base.last_error[0] = '\0';

  cardano_safe_memcpy((*bls_key)->public_key, sizeof((*bls_key)->public_key), public_key, public_key_size);
  cardano_safe_memcpy((*bls_key)->possession_proof, sizeof((*bls_key)->possession_proof), possession_proof, possession_proof_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_bls_key_new_from_hex(
  const char*         public_key_hex,
  const size_t        public_key_hex_size,
  const char*         possession_proof_hex,
  const size_t        possession_proof_hex_size,
  cardano_bls_key_t** bls_key)
{
  if (bls_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *bls_key = NULL;

  if ((public_key_hex == NULL) || (possession_proof_hex == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const size_t hex_characters_per_byte = 2U;

  if ((public_key_hex_size != (BLS_PUBLIC_KEY_SIZE * hex_characters_per_byte)) || (possession_proof_hex_size != (BLS_POSSESSION_PROOF_SIZE * hex_characters_per_byte)))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  byte_t public_key[96]       = { 0 };
  byte_t possession_proof[48] = { 0 };

  const int init_result = sodium_init();

  if (init_result == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  if (sodium_hex2bin(public_key, sizeof(public_key), public_key_hex, public_key_hex_size, NULL, NULL, NULL) < 0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (sodium_hex2bin(possession_proof, sizeof(possession_proof), possession_proof_hex, possession_proof_hex_size, NULL, NULL, NULL) < 0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  return cardano_bls_key_new(public_key, sizeof(public_key), possession_proof, sizeof(possession_proof), bls_key);
}

cardano_error_t
cardano_bls_key_from_cbor(cardano_cbor_reader_t* reader, cardano_bls_key_t** bls_key)
{
  if (bls_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *bls_key = NULL;

  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "bls_key";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  cardano_buffer_t*     public_key             = NULL;
  const cardano_error_t read_public_key_result = cardano_cbor_validate_byte_string_of_size(validator_name, reader, &public_key, (uint32_t)BLS_PUBLIC_KEY_SIZE);

  if (read_public_key_result != CARDANO_SUCCESS)
  {
    return read_public_key_result;
  }

  cardano_buffer_t*     possession_proof             = NULL;
  const cardano_error_t read_possession_proof_result = cardano_cbor_validate_byte_string_of_size(validator_name, reader, &possession_proof, (uint32_t)BLS_POSSESSION_PROOF_SIZE);

  if (read_possession_proof_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&public_key);
    return read_possession_proof_result;
  }

  const cardano_error_t new_result = cardano_bls_key_new(
    cardano_buffer_get_data(public_key),
    cardano_buffer_get_size(public_key),
    cardano_buffer_get_data(possession_proof),
    cardano_buffer_get_size(possession_proof),
    bls_key);

  cardano_buffer_unref(&public_key);
  cardano_buffer_unref(&possession_proof);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  const cardano_error_t end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (end_array_result != CARDANO_SUCCESS)
  {
    cardano_bls_key_unref(bls_key);
  }

  return end_array_result;
}

cardano_error_t
cardano_bls_key_to_cbor(const cardano_bls_key_t* bls_key, cardano_cbor_writer_t* writer)
{
  if (bls_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  const cardano_error_t write_public_key_result = cardano_cbor_writer_write_bytestring(writer, bls_key->public_key, sizeof(bls_key->public_key));

  if (write_public_key_result != CARDANO_SUCCESS)
  {
    return write_public_key_result;
  }

  return cardano_cbor_writer_write_bytestring(writer, bls_key->possession_proof, sizeof(bls_key->possession_proof));
}

cardano_error_t
cardano_bls_key_to_cip116_json(
  const cardano_bls_key_t* bls_key,
  cardano_json_writer_t*   writer)
{
  if ((bls_key == NULL) || (writer == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_json_writer_write_start_object(writer);

  cardano_json_writer_write_property_name(writer, "public_key", 10);
  cardano_json_writer_write_bytes_as_hex(writer, bls_key->public_key, sizeof(bls_key->public_key));

  cardano_json_writer_write_property_name(writer, "possession_proof", 16);
  cardano_json_writer_write_bytes_as_hex(writer, bls_key->possession_proof, sizeof(bls_key->possession_proof));

  cardano_json_writer_write_end_object(writer);

  return CARDANO_SUCCESS;
}

size_t
cardano_bls_key_get_public_key_size(const cardano_bls_key_t* bls_key)
{
  if (bls_key == NULL)
  {
    return 0;
  }

  return sizeof(bls_key->public_key);
}

const byte_t*
cardano_bls_key_get_public_key_bytes(const cardano_bls_key_t* bls_key)
{
  if (bls_key == NULL)
  {
    return NULL;
  }

  return bls_key->public_key;
}

size_t
cardano_bls_key_get_possession_proof_size(const cardano_bls_key_t* bls_key)
{
  if (bls_key == NULL)
  {
    return 0;
  }

  return sizeof(bls_key->possession_proof);
}

const byte_t*
cardano_bls_key_get_possession_proof_bytes(const cardano_bls_key_t* bls_key)
{
  if (bls_key == NULL)
  {
    return NULL;
  }

  return bls_key->possession_proof;
}

void
cardano_bls_key_unref(cardano_bls_key_t** bls_key)
{
  if ((bls_key == NULL) || (*bls_key == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*bls_key)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *bls_key = NULL;
    return;
  }
}

void
cardano_bls_key_ref(cardano_bls_key_t* bls_key)
{
  if (bls_key == NULL)
  {
    return;
  }

  cardano_object_ref(&bls_key->base);
}

size_t
cardano_bls_key_refcount(const cardano_bls_key_t* bls_key)
{
  if (bls_key == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&bls_key->base);
}

void
cardano_bls_key_set_last_error(cardano_bls_key_t* bls_key, const char* message)
{
  cardano_object_set_last_error(&bls_key->base, message);
}

const char*
cardano_bls_key_get_last_error(const cardano_bls_key_t* bls_key)
{
  return cardano_object_get_last_error(&bls_key->base);
}
