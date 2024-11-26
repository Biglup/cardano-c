/**
 * \file parameter_change_action.c
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

#include <cardano/common/governance_action_id.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/protocol_params/protocol_param_update.h>

#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/parameter_change_action.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 4;

/* STRUCTURES ****************************************************************/

/**
 * \brief Updates one or more updatable protocol parameters, excluding changes to major protocol versions (i.e., "hard forks").
 */
typedef struct cardano_parameter_change_action_t
{
    cardano_object_t                 base;
    cardano_protocol_param_update_t* protocol_param_update;
    cardano_blake2b_hash_t*          policy_hash;
    cardano_governance_action_id_t*  governance_action_id;
} cardano_parameter_change_action_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a parameter_change_action object.
 *
 * This function is responsible for properly deallocating a parameter_change_action object (`cardano_parameter_change_action_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the parameter_change_action object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_parameter_change_action_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the parameter_change_action
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_parameter_change_action_deallocate(void* object)
{
  assert(object != NULL);

  cardano_parameter_change_action_t* data = (cardano_parameter_change_action_t*)object;

  cardano_protocol_param_update_unref(&data->protocol_param_update);
  cardano_blake2b_hash_unref(&data->policy_hash);
  cardano_governance_action_id_unref(&data->governance_action_id);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_parameter_change_action_new(
  cardano_protocol_param_update_t*    protocol_param_update,
  cardano_governance_action_id_t*     governance_action_id,
  cardano_blake2b_hash_t*             policy_hash,
  cardano_parameter_change_action_t** parameter_change_action)
{
  if (protocol_param_update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (parameter_change_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_parameter_change_action_t* data = _cardano_malloc(sizeof(cardano_parameter_change_action_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count        = 1;
  data->base.last_error[0]    = '\0';
  data->base.deallocator      = cardano_parameter_change_action_deallocate;
  data->governance_action_id  = NULL;
  data->protocol_param_update = NULL;
  data->policy_hash           = NULL;

  cardano_protocol_param_update_ref(protocol_param_update);
  data->protocol_param_update = protocol_param_update;

  if (policy_hash != NULL)
  {
    cardano_blake2b_hash_ref(policy_hash);
    data->policy_hash = policy_hash;
  }

  if (governance_action_id != NULL)
  {
    cardano_governance_action_id_ref(governance_action_id);
    data->governance_action_id = governance_action_id;
  }

  *parameter_change_action = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_parameter_change_action_from_cbor(cardano_cbor_reader_t* reader, cardano_parameter_change_action_t** parameter_change_action)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (parameter_change_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "parameter_change_action";

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
    CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE,
    (enum_to_string_callback_t)((void*)&cardano_governance_action_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_governance_action_id_t*  governance_action_id  = NULL;
  cardano_protocol_param_update_t* protocol_param_update = NULL;
  cardano_blake2b_hash_t*          policy_hash           = NULL;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_NULL;

  cardano_error_t read_map_result = cardano_cbor_reader_peek_state(reader, &state);

  if (read_map_result != CARDANO_SUCCESS)
  {
    return read_map_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null = cardano_cbor_reader_read_null(reader);

    if (read_null != CARDANO_SUCCESS)
    {
      return read_null;
    }
  }
  else
  {
    cardano_error_t governance_action_id_result = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

    if (governance_action_id_result != CARDANO_SUCCESS)
    {
      return governance_action_id_result;
    }
  }

  cardano_error_t protocol_param_update_result = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  if (protocol_param_update_result != CARDANO_SUCCESS)
  {
    cardano_governance_action_id_unref(&governance_action_id);
    return protocol_param_update_result;
  }

  read_map_result = cardano_cbor_reader_peek_state(reader, &state);

  if (read_map_result != CARDANO_SUCCESS)
  {
    cardano_governance_action_id_unref(&governance_action_id);
    cardano_protocol_param_update_unref(&protocol_param_update);

    return read_map_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null = cardano_cbor_reader_read_null(reader);

    if (read_null != CARDANO_SUCCESS)
    {
      cardano_governance_action_id_unref(&governance_action_id);
      cardano_protocol_param_update_unref(&protocol_param_update);

      return read_null;
    }
  }
  else
  {
    cardano_error_t policy_hash_result = cardano_blake2b_hash_from_cbor(reader, &policy_hash);

    if (policy_hash_result != CARDANO_SUCCESS)
    {
      cardano_governance_action_id_unref(&governance_action_id);
      cardano_protocol_param_update_unref(&protocol_param_update);
      return policy_hash_result;
    }
  }

  cardano_error_t new_result = cardano_parameter_change_action_new(protocol_param_update, governance_action_id, policy_hash, parameter_change_action);

  cardano_governance_action_id_unref(&governance_action_id);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_blake2b_hash_unref(&policy_hash);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_parameter_change_action_to_cbor(
  const cardano_parameter_change_action_t* parameter_change_action,
  cardano_cbor_writer_t*                   writer)
{
  if (parameter_change_action == NULL)
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

  cardano_error_t write_enum_result = cardano_cbor_writer_write_uint(writer, CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE);

  if (write_enum_result != CARDANO_SUCCESS)
  {
    return write_enum_result;
  }

  if (parameter_change_action->governance_action_id == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t governance_action_id_result = cardano_governance_action_id_to_cbor(parameter_change_action->governance_action_id, writer);

    if (governance_action_id_result != CARDANO_SUCCESS)
    {
      return governance_action_id_result;
    }
  }

  cardano_error_t protocol_param_update_result = cardano_protocol_param_update_to_cbor(parameter_change_action->protocol_param_update, writer);

  if (protocol_param_update_result != CARDANO_SUCCESS)
  {
    return protocol_param_update_result;
  }

  if (parameter_change_action->policy_hash == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t policy_hash_result = cardano_blake2b_hash_to_cbor(parameter_change_action->policy_hash, writer);

    if (policy_hash_result != CARDANO_SUCCESS)
    {
      return policy_hash_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_parameter_change_action_set_protocol_param_update(
  cardano_parameter_change_action_t* parameter_change_action,
  cardano_protocol_param_update_t*   protocol_param_update)
{
  if (parameter_change_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_protocol_param_update_ref(protocol_param_update);
  cardano_protocol_param_update_unref(&parameter_change_action->protocol_param_update);
  parameter_change_action->protocol_param_update = protocol_param_update;

  return CARDANO_SUCCESS;
}

cardano_protocol_param_update_t*
cardano_parameter_change_action_get_protocol_param_update(cardano_parameter_change_action_t* parameter_change_action)
{
  if (parameter_change_action == NULL)
  {
    return NULL;
  }

  cardano_protocol_param_update_ref(parameter_change_action->protocol_param_update);

  return parameter_change_action->protocol_param_update;
}

cardano_error_t
cardano_parameter_change_action_set_policy_hash(cardano_parameter_change_action_t* parameter_change_action, cardano_blake2b_hash_t* policy_hash)
{
  if (parameter_change_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_ref(policy_hash);
  cardano_blake2b_hash_unref(&parameter_change_action->policy_hash);
  parameter_change_action->policy_hash = policy_hash;

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_parameter_change_action_get_policy_hash(cardano_parameter_change_action_t* parameter_change_action)
{
  if (parameter_change_action == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(parameter_change_action->policy_hash);

  return parameter_change_action->policy_hash;
}

cardano_error_t
cardano_parameter_change_action_set_governance_action_id(
  cardano_parameter_change_action_t* parameter_change_action,
  cardano_governance_action_id_t*    governance_action_id)
{
  if (parameter_change_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_governance_action_id_ref(governance_action_id);
  cardano_governance_action_id_unref(&parameter_change_action->governance_action_id);
  parameter_change_action->governance_action_id = governance_action_id;

  return CARDANO_SUCCESS;
}

cardano_governance_action_id_t*
cardano_parameter_change_action_get_governance_action_id(
  cardano_parameter_change_action_t* parameter_change_action)
{
  if (parameter_change_action == NULL)
  {
    return NULL;
  }

  cardano_governance_action_id_ref(parameter_change_action->governance_action_id);

  return parameter_change_action->governance_action_id;
}

void
cardano_parameter_change_action_unref(cardano_parameter_change_action_t** parameter_change_action)
{
  if ((parameter_change_action == NULL) || (*parameter_change_action == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*parameter_change_action)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *parameter_change_action = NULL;
    return;
  }
}

void
cardano_parameter_change_action_ref(cardano_parameter_change_action_t* parameter_change_action)
{
  if (parameter_change_action == NULL)
  {
    return;
  }

  cardano_object_ref(&parameter_change_action->base);
}

size_t
cardano_parameter_change_action_refcount(const cardano_parameter_change_action_t* parameter_change_action)
{
  if (parameter_change_action == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&parameter_change_action->base);
}

void
cardano_parameter_change_action_set_last_error(cardano_parameter_change_action_t* parameter_change_action, const char* message)
{
  cardano_object_set_last_error(&parameter_change_action->base, message);
}

const char*
cardano_parameter_change_action_get_last_error(const cardano_parameter_change_action_t* parameter_change_action)
{
  return cardano_object_get_last_error(&parameter_change_action->base);
}
