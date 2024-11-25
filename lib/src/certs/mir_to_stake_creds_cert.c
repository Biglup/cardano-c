/**
 * \file mir_to_stake_creds_cert.c
 *
 * \author angel.castillo
 * \date   Jun 19, 2024
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

#include <cardano/certs/mir_cert_pot_type.h>
#include <cardano/certs/mir_to_stake_creds_cert.h>
#include <cardano/common/credential.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../collections/array.h"

#include <assert.h>
#include <cardano/certs/mir_cert_type.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Creates a move instantaneous rewards certificate that transfers funds to the given set of reward accounts.
 */
typedef struct cardano_mir_to_stake_creds_cert_t
{
    cardano_object_t            base;
    cardano_mir_cert_pot_type_t pot;
    cardano_array_t*            array;
} cardano_mir_to_stake_creds_cert_t;

/**
 * \brief Represents a key-value pair for Move Instantaneous Rewards to Stake Credentials certificate.
 *
 * This structure is used to hold the association between a stake credential and the amount of ADA
 * to be rewarded to that credential. It is utilized within the context of a MIR to Stake Credentials
 * certificate, allowing the specification of multiple such key-value pairs where each pair represents
 * a unique stake credential and the associated reward amount.
 */
typedef struct cardano_mir_to_stake_creds_cert_kvp_t
{
    cardano_object_t      base;
    cardano_credential_t* key;
    uint64_t              value;
} cardano_mir_to_stake_creds_cert_kvp_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a MIR to stake credentials certificate object.
 *
 * This function is responsible for properly deallocating a MIR to stake credentials certificate object (`cardano_mir_to_stake_creds_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the mir_to_stake_creds_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_mir_to_stake_creds_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the mir_to_stake_creds_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_mir_to_stake_creds_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_mir_to_stake_creds_cert_t* map = (cardano_mir_to_stake_creds_cert_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a MIR to stake credentials certificate key value pair object.
 *
 * This function is responsible for properly deallocating a MIR to stake credentials certificate key value pair object (`cardano_mir_to_stake_creds_cert_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the mir_to_stake_creds_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_mir_to_stake_creds_cert_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the mir_to_stake_creds_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_mir_to_stake_creds_cert_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_mir_to_stake_creds_cert_kvp_t* map = (cardano_mir_to_stake_creds_cert_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_credential_unref(&map->key);
  }

  _cardano_free(map);
}

/**
 * \brief Compares two cardano_object_t objects based on their credentials.
 *
 * This function casts the cardano_object_t objects to cardano_mir_to_stake_creds_cert_kvp_t
 * and compares their credentials using the cardano_credential_compare function.
 *
 * \param[in] lhs Pointer to the first cardano_object_t object.
 * \param[in] rhs Pointer to the second cardano_object_t object.
 * \param[in] context Unused.
 *
 * \return A negative value if the hash of lhs is less than the hash of rhs, zero if they are equal,
 *         and a positive value if the hash of lhs is greater than the hash of rhs.
 */
static int32_t
compare_by_credential(const cardano_object_t* lhs, const cardano_object_t* rhs, void* context)
{
  assert(lhs != NULL);
  assert(rhs != NULL);

  CARDANO_UNUSED(context);

  const cardano_mir_to_stake_creds_cert_kvp_t* kvp_lhs = (const cardano_mir_to_stake_creds_cert_kvp_t*)((const void*)lhs);
  const cardano_mir_to_stake_creds_cert_kvp_t* kvp_rhs = (const cardano_mir_to_stake_creds_cert_kvp_t*)((const void*)rhs);

  return cardano_credential_compare(kvp_lhs->key, kvp_rhs->key);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_mir_to_stake_creds_cert_new(const cardano_mir_cert_pot_type_t pot_type, cardano_mir_to_stake_creds_cert_t** mir_to_stake_creds_cert)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_mir_to_stake_creds_cert_t* map = _cardano_malloc(sizeof(cardano_mir_to_stake_creds_cert_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_mir_to_stake_creds_cert_deallocate;

  map->array = cardano_array_new(128);
  map->pot   = pot_type;

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *mir_to_stake_creds_cert = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_stake_creds_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_mir_to_stake_creds_cert_t** mir_to_stake_creds_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir_to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "mir_to_stake_creds";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t        pot      = 0U;
  cardano_error_t read_pot = cardano_cbor_validate_uint_in_range(
    validator_name,
    "pot",
    reader,
    &pot,
    CARDANO_MIR_CERT_TYPE_TO_POT,
    CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS);

  if (read_pot != CARDANO_SUCCESS)
  {
    return read_pot;
  }

  cardano_mir_to_stake_creds_cert_t* map    = NULL;
  cardano_error_t                    result = cardano_mir_to_stake_creds_cert_new(pot, &map);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t length = 0;

  result = cardano_cbor_reader_read_start_map(reader, &length);

  if (result != CARDANO_SUCCESS)
  {
    cardano_mir_to_stake_creds_cert_unref(&map);
    return result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);

    if (result != CARDANO_SUCCESS)
    {
      cardano_mir_to_stake_creds_cert_unref(&map);
      return result;
    }

    if (state == CARDANO_CBOR_READER_STATE_END_MAP)
    {
      break;
    }

    cardano_credential_t* key   = NULL;
    uint64_t              value = 0;

    result = cardano_credential_from_cbor(reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_mir_to_stake_creds_cert_unref(&map);
      return result;
    }

    result = cardano_cbor_reader_read_uint(reader, &value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_credential_unref(&key);
      cardano_mir_to_stake_creds_cert_unref(&map);
      return result;
    }

    cardano_mir_to_stake_creds_cert_kvp_t* kvp = _cardano_malloc(sizeof(cardano_mir_to_stake_creds_cert_kvp_t));

    if (kvp == NULL)
    {
      cardano_credential_unref(&key);
      cardano_mir_to_stake_creds_cert_unref(&map);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    kvp->base.ref_count     = 0;
    kvp->base.last_error[0] = '\0';
    kvp->base.deallocator   = cardano_mir_to_stake_creds_cert_kvp_deallocate;
    kvp->key                = key;
    kvp->value              = value;

    const size_t old_size = cardano_array_get_size(map->array);
    const size_t new_size = cardano_array_push(map->array, (cardano_object_t*)((void*)kvp));

    assert((old_size + 1U) == new_size);
    CARDANO_UNUSED(old_size);
    CARDANO_UNUSED(new_size);
  }

  result = cardano_cbor_validate_end_map("mir_to_stake_creds_cert", reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_mir_to_stake_creds_cert_unref(&map);
    return result;
  }

  *mir_to_stake_creds_cert = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_stake_creds_cert_to_cbor(const cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert, cardano_cbor_writer_t* writer)
{
  if (mir_to_stake_creds_cert == NULL)
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

  result = cardano_cbor_writer_write_uint(writer, mir_to_stake_creds_cert->pot);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  size_t map_size = cardano_array_get_size(mir_to_stake_creds_cert->array);
  result          = cardano_cbor_writer_write_start_map(writer, (int64_t)map_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0; i < cardano_array_get_size(mir_to_stake_creds_cert->array); ++i)
  {
    cardano_object_t* kvp = cardano_array_get(mir_to_stake_creds_cert->array, i);

    if (kvp == NULL)
    {
      cardano_cbor_writer_set_last_error(writer, "Element in MIR to stake credentials certificate is NULL");
      return CARDANO_ERROR_ENCODING;
    }

    cardano_mir_to_stake_creds_cert_kvp_t* kvp_data = (cardano_mir_to_stake_creds_cert_kvp_t*)((void*)kvp);

    result = cardano_credential_to_cbor(kvp_data->key, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, kvp_data->value);

    if (result != CARDANO_SUCCESS)
    {
      cardano_object_unref(&kvp);
      return result;
    }

    cardano_object_unref(&kvp);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_stake_creds_cert_get_pot(const cardano_mir_to_stake_creds_cert_t* mir_cert, cardano_mir_cert_pot_type_t* type)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = mir_cert->pot;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_stake_creds_cert_set_pot(cardano_mir_to_stake_creds_cert_t* mir_cert, cardano_mir_cert_pot_type_t type)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  mir_cert->pot = type;

  return CARDANO_SUCCESS;
}

size_t
cardano_mir_to_stake_creds_cert_get_size(const cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return 0;
  }

  return cardano_array_get_size(mir_to_stake_creds_cert->array);
}

cardano_error_t
cardano_mir_to_stake_creds_cert_insert(
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert,
  cardano_credential_t*              credential,
  uint64_t                           amount)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_mir_to_stake_creds_cert_kvp_t* kvp = _cardano_malloc(sizeof(cardano_mir_to_stake_creds_cert_kvp_t));

  if (kvp == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_mir_to_stake_creds_cert_kvp_deallocate;
  kvp->key                = credential;
  kvp->value              = amount;

  cardano_credential_ref(credential);

  const size_t old_size = cardano_array_get_size(mir_to_stake_creds_cert->array);
  const size_t new_size = cardano_array_push(mir_to_stake_creds_cert->array, (cardano_object_t*)((void*)kvp));

  assert((old_size + 1U) == new_size);

  CARDANO_UNUSED(old_size);
  CARDANO_UNUSED(new_size);

  cardano_array_sort(mir_to_stake_creds_cert->array, compare_by_credential, NULL);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_stake_creds_cert_get_key_at(
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert,
  size_t                             index,
  cardano_credential_t**             credential)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(mir_to_stake_creds_cert->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t* object = cardano_array_get(mir_to_stake_creds_cert->array, index);

  cardano_mir_to_stake_creds_cert_kvp_t* kvp = (cardano_mir_to_stake_creds_cert_kvp_t*)((void*)object);

  cardano_credential_ref(kvp->key);

  *credential = kvp->key;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_stake_creds_cert_get_value_at(
  const cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert,
  size_t                                   index,
  uint64_t*                                amount)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(mir_to_stake_creds_cert->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t* object = cardano_array_get(mir_to_stake_creds_cert->array, index);

  cardano_mir_to_stake_creds_cert_kvp_t* kvp = (cardano_mir_to_stake_creds_cert_kvp_t*)((void*)object);

  *amount = kvp->value;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_stake_creds_cert_get_key_value_at(
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert,
  size_t                             index,
  cardano_credential_t**             credential,
  uint64_t*                          amount)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index >= cardano_array_get_size(mir_to_stake_creds_cert->array))
  {
    return CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  cardano_object_t* object = cardano_array_get(mir_to_stake_creds_cert->array, index);

  cardano_mir_to_stake_creds_cert_kvp_t* kvp = (cardano_mir_to_stake_creds_cert_kvp_t*)((void*)object);

  cardano_credential_ref(kvp->key);

  *credential = kvp->key;
  *amount     = kvp->value;

  cardano_object_unref(&object);

  return CARDANO_SUCCESS;
}

void
cardano_mir_to_stake_creds_cert_unref(cardano_mir_to_stake_creds_cert_t** mir_to_stake_creds_cert)
{
  if ((mir_to_stake_creds_cert == NULL) || (*mir_to_stake_creds_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*mir_to_stake_creds_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *mir_to_stake_creds_cert = NULL;
    return;
  }
}

void
cardano_mir_to_stake_creds_cert_ref(cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&mir_to_stake_creds_cert->base);
}

size_t
cardano_mir_to_stake_creds_cert_refcount(const cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert)
{
  if (mir_to_stake_creds_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&mir_to_stake_creds_cert->base);
}

void
cardano_mir_to_stake_creds_cert_set_last_error(cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert, const char* message)
{
  cardano_object_set_last_error(&mir_to_stake_creds_cert->base, message);
}

const char*
cardano_mir_to_stake_creds_cert_get_last_error(const cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert)
{
  return cardano_object_get_last_error(&mir_to_stake_creds_cert->base);
}
