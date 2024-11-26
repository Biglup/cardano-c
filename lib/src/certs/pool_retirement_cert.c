/**
 * \file pool_retirement_cert.c
 *
 * \author angel.castillo
 * \date   Jul 31, 2024
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

#include <cardano/certs/cert_type.h>
#include <cardano/certs/pool_retirement_cert.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 3;

/* STRUCTURES ****************************************************************/

/**
 * \brief this certificate is used to retire a stake pool. it includes an epoch number indicating when the pool will be retired.
 */
typedef struct cardano_pool_retirement_cert_t
{
    cardano_object_t        base;
    cardano_blake2b_hash_t* pool_key_hash;
    uint64_t                epoch;
} cardano_pool_retirement_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a pool_retirement_cert object.
 *
 * This function is responsible for properly deallocating a pool_retirement_cert object (`cardano_pool_retirement_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the pool_retirement_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_pool_retirement_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the pool_retirement_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_pool_retirement_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_pool_retirement_cert_t* data = (cardano_pool_retirement_cert_t*)object;

  cardano_blake2b_hash_unref(&data->pool_key_hash);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_pool_retirement_cert_new(
  cardano_blake2b_hash_t*          pool_key_hash,
  const uint64_t                   epoch,
  cardano_pool_retirement_cert_t** pool_retirement_cert)
{
  if (pool_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pool_retirement_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_pool_retirement_cert_t* data = _cardano_malloc(sizeof(cardano_pool_retirement_cert_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_pool_retirement_cert_deallocate;

  cardano_blake2b_hash_ref(pool_key_hash);
  data->pool_key_hash = pool_key_hash;
  data->epoch         = epoch;

  *pool_retirement_cert = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_retirement_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_retirement_cert_t** pool_retirement_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pool_retirement_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "pool_retirement_cert";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_CERT_TYPE_POOL_RETIREMENT,
    (enum_to_string_callback_t)((void*)&cardano_cert_type_to_string),
    &type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_blake2b_hash_t* pool_key_hash = NULL;

  cardano_error_t read_hash_result = cardano_blake2b_hash_from_cbor(reader, &pool_key_hash);

  if (read_hash_result != CARDANO_SUCCESS)
  {
    return read_hash_result;
  }

  uint64_t epoch = 0U;

  cardano_error_t read_epoch_result = cardano_cbor_reader_read_uint(reader, &epoch);

  if (read_epoch_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&pool_key_hash);
    return read_epoch_result;
  }

  cardano_error_t new_result = cardano_pool_retirement_cert_new(pool_key_hash, epoch, pool_retirement_cert);

  cardano_blake2b_hash_unref(&pool_key_hash);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_pool_retirement_cert_to_cbor(
  const cardano_pool_retirement_cert_t* pool_retirement_cert,
  cardano_cbor_writer_t*                writer)
{
  if (pool_retirement_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_array_result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (write_array_result != CARDANO_SUCCESS)
  {
    return write_array_result;
  }

  cardano_error_t write_type_result = cardano_cbor_writer_write_uint(writer, CARDANO_CERT_TYPE_POOL_RETIREMENT);

  if (write_type_result != CARDANO_SUCCESS)
  {
    return write_type_result;
  }

  cardano_error_t write_hash_result = cardano_blake2b_hash_to_cbor(pool_retirement_cert->pool_key_hash, writer);

  if (write_hash_result != CARDANO_SUCCESS)
  {
    return write_hash_result;
  }

  cardano_error_t write_epoch_result = cardano_cbor_writer_write_uint(writer, pool_retirement_cert->epoch);

  if (write_epoch_result != CARDANO_SUCCESS)
  {
    return write_epoch_result;
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_pool_retirement_cert_get_pool_key_hash(cardano_pool_retirement_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(certificate->pool_key_hash);

  return certificate->pool_key_hash;
}

cardano_error_t
cardano_pool_retirement_cert_set_pool_key_hash(
  cardano_pool_retirement_cert_t* certificate,
  cardano_blake2b_hash_t*         hash)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(hash);
  cardano_blake2b_hash_unref(&certificate->pool_key_hash);
  certificate->pool_key_hash = hash;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_pool_retirement_cert_get_epoch(const cardano_pool_retirement_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return 0;
  }

  return certificate->epoch;
}

cardano_error_t
cardano_pool_retirement_cert_set_epoch(cardano_pool_retirement_cert_t* certificate, const uint64_t epoch)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  certificate->epoch = epoch;

  return CARDANO_SUCCESS;
}

void
cardano_pool_retirement_cert_unref(cardano_pool_retirement_cert_t** pool_retirement_cert)
{
  if ((pool_retirement_cert == NULL) || (*pool_retirement_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*pool_retirement_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *pool_retirement_cert = NULL;
    return;
  }
}

void
cardano_pool_retirement_cert_ref(cardano_pool_retirement_cert_t* pool_retirement_cert)
{
  if (pool_retirement_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&pool_retirement_cert->base);
}

size_t
cardano_pool_retirement_cert_refcount(const cardano_pool_retirement_cert_t* pool_retirement_cert)
{
  if (pool_retirement_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&pool_retirement_cert->base);
}

void
cardano_pool_retirement_cert_set_last_error(cardano_pool_retirement_cert_t* pool_retirement_cert, const char* message)
{
  cardano_object_set_last_error(&pool_retirement_cert->base, message);
}

const char*
cardano_pool_retirement_cert_get_last_error(const cardano_pool_retirement_cert_t* pool_retirement_cert)
{
  return cardano_object_get_last_error(&pool_retirement_cert->base);
}
