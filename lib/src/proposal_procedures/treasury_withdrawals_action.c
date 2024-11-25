/**
 * \file treasury_withdrawals_action.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license");
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

#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>

#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 3;

/* STRUCTURES ****************************************************************/

/**
 * \brief Withdraws funds from the treasury.
 */
typedef struct cardano_treasury_withdrawals_action_t
{
    cardano_object_t          base;
    cardano_withdrawal_map_t* withdrawals;
    cardano_blake2b_hash_t*   policy_hash;
} cardano_treasury_withdrawals_action_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a treasury_withdrawals_action object.
 *
 * This function is responsible for properly deallocating a treasury_withdrawals_action object (`cardano_treasury_withdrawals_action_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the treasury_withdrawals_action object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_treasury_withdrawals_action_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the treasury_withdrawals_action
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_treasury_withdrawals_action_deallocate(void* object)
{
  assert(object != NULL);

  cardano_treasury_withdrawals_action_t* data = (cardano_treasury_withdrawals_action_t*)object;

  cardano_withdrawal_map_unref(&data->withdrawals);
  cardano_blake2b_hash_unref(&data->policy_hash);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_treasury_withdrawals_action_new(
  cardano_withdrawal_map_t*               withdrawals,
  cardano_blake2b_hash_t*                 policy_hash,
  cardano_treasury_withdrawals_action_t** treasury_withdrawals_action)
{
  if (withdrawals == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_withdrawals_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_treasury_withdrawals_action_t* data = _cardano_malloc(sizeof(cardano_treasury_withdrawals_action_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_treasury_withdrawals_action_deallocate;
  data->withdrawals        = NULL;
  data->policy_hash        = NULL;

  cardano_withdrawal_map_ref(withdrawals);
  data->withdrawals = withdrawals;

  if (policy_hash != NULL)
  {
    cardano_blake2b_hash_ref(policy_hash);
    data->policy_hash = policy_hash;
  }

  *treasury_withdrawals_action = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_treasury_withdrawals_action_from_cbor(cardano_cbor_reader_t* reader, cardano_treasury_withdrawals_action_t** treasury_withdrawals_action)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_withdrawals_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "treasury_withdrawals_action";

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
    CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS,
    (enum_to_string_callback_t)((void*)&cardano_governance_action_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_withdrawal_map_t* withdrawals = NULL;
  cardano_blake2b_hash_t*   policy_hash = NULL;

  cardano_error_t withdrawals_result = cardano_withdrawal_map_from_cbor(reader, &withdrawals);

  if (withdrawals_result != CARDANO_SUCCESS)
  {
    cardano_withdrawal_map_unref(&withdrawals);
    return withdrawals_result;
  }

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_NULL;

  cardano_error_t read_map_result = cardano_cbor_reader_peek_state(reader, &state);

  if (read_map_result != CARDANO_SUCCESS)
  {
    cardano_withdrawal_map_unref(&withdrawals);
    return read_map_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null = cardano_cbor_reader_read_null(reader);

    if (read_null != CARDANO_SUCCESS)
    {
      cardano_withdrawal_map_unref(&withdrawals);
      return read_null;
    }
  }
  else
  {
    cardano_error_t policy_hash_result = cardano_blake2b_hash_from_cbor(reader, &policy_hash);

    if (policy_hash_result != CARDANO_SUCCESS)
    {
      cardano_withdrawal_map_unref(&withdrawals);
      return policy_hash_result;
    }
  }

  cardano_error_t new_result = cardano_treasury_withdrawals_action_new(withdrawals, policy_hash, treasury_withdrawals_action);

  cardano_withdrawal_map_unref(&withdrawals);
  cardano_blake2b_hash_unref(&policy_hash);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_treasury_withdrawals_action_to_cbor(
  const cardano_treasury_withdrawals_action_t* treasury_withdrawals_action,
  cardano_cbor_writer_t*                       writer)
{
  if (treasury_withdrawals_action == NULL)
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

  cardano_error_t write_enum_result = cardano_cbor_writer_write_uint(writer, CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS);

  if (write_enum_result != CARDANO_SUCCESS)
  {
    return write_enum_result;
  }

  cardano_error_t withdrawals_result = cardano_withdrawal_map_to_cbor(treasury_withdrawals_action->withdrawals, writer);

  if (withdrawals_result != CARDANO_SUCCESS)
  {
    return withdrawals_result;
  }

  if (treasury_withdrawals_action->policy_hash == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t policy_hash_result = cardano_blake2b_hash_to_cbor(treasury_withdrawals_action->policy_hash, writer);

    if (policy_hash_result != CARDANO_SUCCESS)
    {
      return policy_hash_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_treasury_withdrawals_action_set_withdrawals(
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action,
  cardano_withdrawal_map_t*              withdrawals)
{
  if (treasury_withdrawals_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (withdrawals == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_withdrawal_map_ref(withdrawals);
  cardano_withdrawal_map_unref(&treasury_withdrawals_action->withdrawals);
  treasury_withdrawals_action->withdrawals = withdrawals;

  return CARDANO_SUCCESS;
}

cardano_withdrawal_map_t*
cardano_treasury_withdrawals_action_get_withdrawals(
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action)
{
  if (treasury_withdrawals_action == NULL)
  {
    return NULL;
  }

  cardano_withdrawal_map_ref(treasury_withdrawals_action->withdrawals);

  return treasury_withdrawals_action->withdrawals;
}

cardano_error_t
cardano_treasury_withdrawals_action_set_policy_hash(
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action,
  cardano_blake2b_hash_t*                policy_hash)
{
  if (treasury_withdrawals_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(policy_hash);
  cardano_blake2b_hash_unref(&treasury_withdrawals_action->policy_hash);
  treasury_withdrawals_action->policy_hash = policy_hash;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_treasury_withdrawals_action_get_policy_hash(
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action)
{
  if (treasury_withdrawals_action == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(treasury_withdrawals_action->policy_hash);

  return treasury_withdrawals_action->policy_hash;
}

void
cardano_treasury_withdrawals_action_unref(cardano_treasury_withdrawals_action_t** treasury_withdrawals_action)
{
  if ((treasury_withdrawals_action == NULL) || (*treasury_withdrawals_action == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*treasury_withdrawals_action)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *treasury_withdrawals_action = NULL;
    return;
  }
}

void
cardano_treasury_withdrawals_action_ref(cardano_treasury_withdrawals_action_t* treasury_withdrawals_action)
{
  if (treasury_withdrawals_action == NULL)
  {
    return;
  }

  cardano_object_ref(&treasury_withdrawals_action->base);
}

size_t
cardano_treasury_withdrawals_action_refcount(const cardano_treasury_withdrawals_action_t* treasury_withdrawals_action)
{
  if (treasury_withdrawals_action == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&treasury_withdrawals_action->base);
}

void
cardano_treasury_withdrawals_action_set_last_error(cardano_treasury_withdrawals_action_t* treasury_withdrawals_action, const char* message)
{
  cardano_object_set_last_error(&treasury_withdrawals_action->base, message);
}

const char*
cardano_treasury_withdrawals_action_get_last_error(const cardano_treasury_withdrawals_action_t* treasury_withdrawals_action)
{
  return cardano_object_get_last_error(&treasury_withdrawals_action->base);
}
