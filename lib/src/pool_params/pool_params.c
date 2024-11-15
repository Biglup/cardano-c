/**
 * \file pool_params.c
 *
 * \author angel.castillo
 * \date   jun 26, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license") {}
 * you may not use this file except in compliance with the license.
 * you may obtain a copy of the license at
 *
 *     www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/common/unit_interval.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/object.h>
#include <cardano/pool_params/pool_metadata.h>
#include <cardano/pool_params/pool_owners.h>
#include <cardano/pool_params/pool_params.h>
#include <cardano/pool_params/relays.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 9;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an pool params.
 *
 * Each instance of `cardano_pool_params_t` holds a pool params object.
 */
typedef struct cardano_pool_params_t
{
    cardano_object_t          base;
    cardano_blake2b_hash_t*   operator_hash;
    cardano_blake2b_hash_t*   vrf_vk_hash;
    uint64_t                  pledge;
    uint64_t                  cost;
    cardano_unit_interval_t*  margin;
    cardano_reward_address_t* reward_account;
    cardano_pool_owners_t*    owners;
    cardano_relays_t*         relays;
    cardano_pool_metadata_t*  metadata;
} cardano_pool_params_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a pool_params object.
 *
 * This function is responsible for properly deallocating a pool_params object (`cardano_pool_params_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the pool_params object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_pool_params_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the pool_params
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_pool_params_deallocate(void* object)
{
  assert(object != NULL);

  cardano_pool_params_t* pool_params = (cardano_pool_params_t*)object;

  cardano_blake2b_hash_unref(&pool_params->operator_hash);
  cardano_blake2b_hash_unref(&pool_params->vrf_vk_hash);
  cardano_unit_interval_unref(&pool_params->margin);
  cardano_reward_address_unref(&pool_params->reward_account);
  cardano_pool_owners_unref(&pool_params->owners);
  cardano_relays_unref(&pool_params->relays);
  cardano_pool_metadata_unref(&pool_params->metadata);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_pool_params_new(
  cardano_blake2b_hash_t*   operator_key_hash,
  cardano_blake2b_hash_t*   vrf_vk_hash,
  uint64_t                  pledge,
  uint64_t                  cost,
  cardano_unit_interval_t*  margin,
  cardano_reward_address_t* reward_account,
  cardano_pool_owners_t*    owners,
  cardano_relays_t*         relays,
  cardano_pool_metadata_t*  metadata,
  cardano_pool_params_t**   pool_params)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (operator_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vrf_vk_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (margin == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_account == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *pool_params = _cardano_malloc(sizeof(cardano_pool_params_t));

  if (*pool_params == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*pool_params)->base.deallocator   = cardano_pool_params_deallocate;
  (*pool_params)->base.ref_count     = 1;
  (*pool_params)->base.last_error[0] = '\0';

  cardano_blake2b_hash_ref(operator_key_hash);
  (*pool_params)->operator_hash = operator_key_hash;

  cardano_blake2b_hash_ref(vrf_vk_hash);
  (*pool_params)->vrf_vk_hash = vrf_vk_hash;

  (*pool_params)->pledge = pledge;
  (*pool_params)->cost   = cost;

  cardano_unit_interval_ref(margin);
  (*pool_params)->margin = margin;

  cardano_reward_address_ref(reward_account);
  (*pool_params)->reward_account = reward_account;

  cardano_pool_owners_ref(owners);
  (*pool_params)->owners = owners;

  cardano_relays_ref(relays);
  (*pool_params)->relays = relays;

  cardano_pool_metadata_ref(metadata);
  (*pool_params)->metadata = metadata;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_params_t** pool_params)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *pool_params = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_t*   operator_key_hash = NULL;
  cardano_blake2b_hash_t*   vrf_vk_hash       = NULL;
  uint64_t                  pledge            = 0;
  uint64_t                  cost              = 0;
  cardano_unit_interval_t*  margin            = NULL;
  cardano_reward_address_t* reward_account    = NULL;
  cardano_pool_owners_t*    owners            = NULL;
  cardano_relays_t*         relays            = NULL;
  cardano_pool_metadata_t*  metadata          = NULL;

  const cardano_error_t operator_key_hash_result = cardano_blake2b_hash_from_cbor(reader, &operator_key_hash);

  if (operator_key_hash_result != CARDANO_SUCCESS)
  {
    return operator_key_hash_result;
  }

  const cardano_error_t vrf_vk_hash_result = cardano_blake2b_hash_from_cbor(reader, &vrf_vk_hash);

  if (vrf_vk_hash_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    return vrf_vk_hash_result;
  }

  cardano_error_t read_uint_result = cardano_cbor_reader_read_uint(reader, &pledge);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    return read_uint_result;
  }

  read_uint_result = cardano_cbor_reader_read_uint(reader, &cost);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    return read_uint_result;
  }

  const cardano_error_t read_margin_result = cardano_unit_interval_from_cbor(reader, &margin);

  if (read_margin_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    return read_margin_result;
  }

  cardano_buffer_t* reward_account_buffer = NULL;

  const cardano_error_t read_reward_account_result = cardano_cbor_reader_read_bytestring(reader, &reward_account_buffer);

  if (read_reward_account_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    cardano_unit_interval_unref(&margin);
    return read_reward_account_result;
  }

  cardano_error_t reward_account_result = cardano_reward_address_from_bytes(cardano_buffer_get_data(reward_account_buffer), cardano_buffer_get_size(reward_account_buffer), &reward_account);

  cardano_buffer_unref(&reward_account_buffer);

  if (reward_account_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    cardano_unit_interval_unref(&margin);
    return reward_account_result;
  }

  const cardano_error_t read_owners_result = cardano_pool_owners_from_cbor(reader, &owners);

  if (read_owners_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    cardano_unit_interval_unref(&margin);
    cardano_reward_address_unref(&reward_account);

    return read_owners_result;
  }

  const cardano_error_t read_relays_result = cardano_relays_from_cbor(reader, &relays);

  if (read_relays_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    cardano_unit_interval_unref(&margin);
    cardano_reward_address_unref(&reward_account);
    cardano_pool_owners_unref(&owners);

    return read_relays_result;
  }

  cardano_cbor_reader_state_t state;

  cardano_error_t peek_state_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_state_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&operator_key_hash);
    cardano_blake2b_hash_unref(&vrf_vk_hash);
    cardano_unit_interval_unref(&margin);
    cardano_reward_address_unref(&reward_account);
    cardano_pool_owners_unref(&owners);
    cardano_relays_unref(&relays);

    return peek_state_result;
  }

  if (state != CARDANO_CBOR_READER_STATE_NULL)
  {
    const cardano_error_t read_metadata_result = cardano_pool_metadata_from_cbor(reader, &metadata);

    if (read_metadata_result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&operator_key_hash);
      cardano_blake2b_hash_unref(&vrf_vk_hash);
      cardano_unit_interval_unref(&margin);
      cardano_reward_address_unref(&reward_account);
      cardano_pool_owners_unref(&owners);
      cardano_relays_unref(&relays);

      return read_metadata_result;
    }
  }
  else
  {
    metadata = NULL;

    const cardano_error_t read_null_result = cardano_cbor_reader_read_null(reader);

    if (read_null_result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&operator_key_hash);
      cardano_blake2b_hash_unref(&vrf_vk_hash);
      cardano_unit_interval_unref(&margin);
      cardano_reward_address_unref(&reward_account);
      cardano_pool_owners_unref(&owners);
      cardano_relays_unref(&relays);

      return read_null_result;
    }
  }

  const cardano_error_t create_instance_result = cardano_pool_params_new(
    operator_key_hash,
    vrf_vk_hash,
    pledge,
    cost,
    margin,
    reward_account,
    owners,
    relays,
    metadata,
    pool_params);

  cardano_blake2b_hash_unref(&operator_key_hash);
  cardano_blake2b_hash_unref(&vrf_vk_hash);
  cardano_unit_interval_unref(&margin);
  cardano_reward_address_unref(&reward_account);
  cardano_pool_owners_unref(&owners);
  cardano_relays_unref(&relays);
  cardano_pool_metadata_unref(&metadata);

  return create_instance_result;
}

cardano_error_t
cardano_pool_params_to_cbor(const cardano_pool_params_t* pool_params, cardano_cbor_writer_t* writer)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_operator_key_hash_result = cardano_blake2b_hash_to_cbor(pool_params->operator_hash, writer);

  if (write_operator_key_hash_result != CARDANO_SUCCESS)
  {
    return write_operator_key_hash_result;
  }

  cardano_error_t write_vrf_vk_hash_result = cardano_blake2b_hash_to_cbor(pool_params->vrf_vk_hash, writer);

  if (write_vrf_vk_hash_result != CARDANO_SUCCESS)
  {
    return write_vrf_vk_hash_result;
  }

  cardano_error_t write_pledge_result = cardano_cbor_writer_write_uint(writer, pool_params->pledge);

  if (write_pledge_result != CARDANO_SUCCESS)
  {
    return write_pledge_result;
  }

  cardano_error_t write_cost_result = cardano_cbor_writer_write_uint(writer, pool_params->cost);

  if (write_cost_result != CARDANO_SUCCESS)
  {
    return write_cost_result;
  }

  cardano_error_t write_margin_result = cardano_unit_interval_to_cbor(pool_params->margin, writer);

  if (write_margin_result != CARDANO_SUCCESS)
  {
    return write_margin_result;
  }

  cardano_error_t write_reward_account_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_reward_address_get_bytes(pool_params->reward_account),
    cardano_reward_address_get_bytes_size(pool_params->reward_account));

  if (write_reward_account_result != CARDANO_SUCCESS)
  {
    return write_reward_account_result;
  }

  cardano_error_t write_owners_result = cardano_pool_owners_to_cbor(pool_params->owners, writer);

  if (write_owners_result != CARDANO_SUCCESS)
  {
    return write_owners_result;
  }

  cardano_error_t write_relays_result = cardano_relays_to_cbor(pool_params->relays, writer);

  if (write_relays_result != CARDANO_SUCCESS)
  {
    return write_relays_result;
  }

  if (pool_params->metadata != NULL)
  {
    return cardano_pool_metadata_to_cbor(pool_params->metadata, writer);
  }

  return cardano_cbor_writer_write_null(writer);
}

cardano_error_t
cardano_pool_params_get_operator_key_hash(
  cardano_pool_params_t*   pool_params,
  cardano_blake2b_hash_t** operator_key_hash)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (operator_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(pool_params->operator_hash);
  *operator_key_hash = pool_params->operator_hash;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_operator_key_hash(
  cardano_pool_params_t*  pool_params,
  cardano_blake2b_hash_t* operator_key_hash)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (operator_key_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(operator_key_hash);
  cardano_blake2b_hash_unref(&pool_params->operator_hash);
  pool_params->operator_hash = operator_key_hash;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_vrf_vk_hash(
  cardano_pool_params_t*   pool_params,
  cardano_blake2b_hash_t** vrf_vk_hash)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vrf_vk_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(pool_params->vrf_vk_hash);
  *vrf_vk_hash = pool_params->vrf_vk_hash;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_vrf_vk_hash(
  cardano_pool_params_t*  pool_params,
  cardano_blake2b_hash_t* vrf_vk_hash)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vrf_vk_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(vrf_vk_hash);
  cardano_blake2b_hash_unref(&pool_params->vrf_vk_hash);
  pool_params->vrf_vk_hash = vrf_vk_hash;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_pledge(
  cardano_pool_params_t* pool_params,
  uint64_t*              pledge)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pledge == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *pledge = pool_params->pledge;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_pledge(
  cardano_pool_params_t* pool_params,
  uint64_t               pledge)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  pool_params->pledge = pledge;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_cost(
  cardano_pool_params_t* pool_params,
  uint64_t*              cost)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cost == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *cost = pool_params->cost;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_cost(
  cardano_pool_params_t* pool_params,
  uint64_t               cost)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  pool_params->cost = cost;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_margin(
  cardano_pool_params_t*    pool_params,
  cardano_unit_interval_t** margin)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (margin == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(pool_params->margin);
  *margin = pool_params->margin;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_margin(
  cardano_pool_params_t*   pool_params,
  cardano_unit_interval_t* margin)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (margin == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(margin);
  cardano_unit_interval_unref(&pool_params->margin);
  pool_params->margin = margin;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_reward_account(
  cardano_pool_params_t*     pool_params,
  cardano_reward_address_t** reward_account)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_account == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_ref(pool_params->reward_account);
  *reward_account = pool_params->reward_account;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_reward_account(
  cardano_pool_params_t*    pool_params,
  cardano_reward_address_t* reward_account)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_account == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_ref(reward_account);
  cardano_reward_address_unref(&pool_params->reward_account);
  pool_params->reward_account = reward_account;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_owners(
  cardano_pool_params_t*  pool_params,
  cardano_pool_owners_t** owners)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_pool_owners_ref(pool_params->owners);
  *owners = pool_params->owners;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_owners(
  cardano_pool_params_t* pool_params,
  cardano_pool_owners_t* owners)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (owners == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_pool_owners_ref(owners);
  cardano_pool_owners_unref(&pool_params->owners);
  pool_params->owners = owners;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_relays(
  cardano_pool_params_t* pool_params,
  cardano_relays_t**     relays)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_relays_ref(pool_params->relays);
  *relays = pool_params->relays;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_relays(
  cardano_pool_params_t* pool_params,
  cardano_relays_t*      relays)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relays == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_relays_ref(relays);
  cardano_relays_unref(&pool_params->relays);
  pool_params->relays = relays;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_get_metadata(
  cardano_pool_params_t*    pool_params,
  cardano_pool_metadata_t** metadata)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (metadata == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_pool_metadata_ref(pool_params->metadata);
  *metadata = pool_params->metadata;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_params_set_metadata(
  cardano_pool_params_t*   pool_params,
  cardano_pool_metadata_t* metadata)
{
  if (pool_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (metadata == NULL)
  {
    cardano_pool_metadata_unref(&pool_params->metadata);
    pool_params->metadata = NULL;

    return CARDANO_SUCCESS;
  }

  cardano_pool_metadata_ref(metadata);
  cardano_pool_metadata_unref(&pool_params->metadata);
  pool_params->metadata = metadata;

  return CARDANO_SUCCESS;
}

void
cardano_pool_params_unref(cardano_pool_params_t** pool_params)
{
  if ((pool_params == NULL) || (*pool_params == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*pool_params)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *pool_params = NULL;
    return;
  }
}

void
cardano_pool_params_ref(cardano_pool_params_t* pool_params)
{
  if (pool_params == NULL)
  {
    return;
  }

  cardano_object_ref(&pool_params->base);
}

size_t
cardano_pool_params_refcount(const cardano_pool_params_t* pool_params)
{
  if (pool_params == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&pool_params->base);
}

void
cardano_pool_params_set_last_error(cardano_pool_params_t* pool_params, const char* message)
{
  cardano_object_set_last_error(&pool_params->base, message);
}

const char*
cardano_pool_params_get_last_error(const cardano_pool_params_t* pool_params)
{
  return cardano_object_get_last_error(&pool_params->base);
}