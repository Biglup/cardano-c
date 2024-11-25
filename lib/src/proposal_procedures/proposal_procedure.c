/**
 * \file proposal_procedure.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2024
 *
 * Copyright 2024 Biglup labs
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

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include <cardano/address/reward_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/anchor.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/proposal_procedures/info_action.h>
#include <cardano/proposal_procedures/new_constitution_action.h>
#include <cardano/proposal_procedures/no_confidence_action.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 4;

/* STRUCTURES ****************************************************************/

/**
 * \brief Governance proposal procedure for the Cardano blockchain, it supports various types of governance actions.
 */
typedef struct cardano_proposal_procedure_t
{
    cardano_object_t                       base;
    cardano_hard_fork_initiation_action_t* hard_fork_initiation_action;
    cardano_info_action_t*                 info_action;
    cardano_new_constitution_action_t*     new_constitution_action;
    cardano_no_confidence_action_t*        no_confidence_action;
    cardano_parameter_change_action_t*     parameter_change_action;
    cardano_treasury_withdrawals_action_t* treasury_withdrawals_action;
    cardano_update_committee_action_t*     update_committee_action;
    cardano_governance_action_type_t       action_type;
    cardano_reward_address_t*              reward_address;
    uint64_t                               deposit;
    cardano_anchor_t*                      anchor;
} cardano_proposal_procedure_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a proposal_procedure object.
 *
 * This function is responsible for properly deallocating a proposal_procedure object (`cardano_proposal_procedure_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the proposal_procedure object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_proposal_procedure_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the proposal_procedure
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_proposal_procedure_deallocate(void* object)
{
  assert(object != NULL);

  cardano_proposal_procedure_t* data = (cardano_proposal_procedure_t*)object;

  cardano_anchor_unref(&data->anchor);
  cardano_hard_fork_initiation_action_unref(&data->hard_fork_initiation_action);
  cardano_info_action_unref(&data->info_action);
  cardano_new_constitution_action_unref(&data->new_constitution_action);
  cardano_no_confidence_action_unref(&data->no_confidence_action);
  cardano_parameter_change_action_unref(&data->parameter_change_action);
  cardano_treasury_withdrawals_action_unref(&data->treasury_withdrawals_action);
  cardano_update_committee_action_unref(&data->update_committee_action);
  cardano_reward_address_unref(&data->reward_address);

  _cardano_free(data);
}

/**
 * \brief Creates a new proposal_procedure object.
 *
 * @return A pointer to the newly created proposal_procedure object, or NULL if the allocation failed.
 */
static cardano_proposal_procedure_t*
new_proposal_procedure(void)
{
  cardano_proposal_procedure_t* data = _cardano_malloc(sizeof(cardano_proposal_procedure_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_proposal_procedure_deallocate;

  data->hard_fork_initiation_action = NULL;
  data->info_action                 = NULL;
  data->new_constitution_action     = NULL;
  data->no_confidence_action        = NULL;
  data->parameter_change_action     = NULL;
  data->treasury_withdrawals_action = NULL;
  data->update_committee_action     = NULL;
  data->action_type                 = CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE;
  data->reward_address              = NULL;
  data->anchor                      = NULL;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_proposal_procedure_new_parameter_change_action(
  const uint64_t                     deposit,
  cardano_reward_address_t*          reward_address,
  cardano_anchor_t*                  anchor,
  cardano_parameter_change_action_t* parameter_change_action,
  cardano_proposal_procedure_t**     proposal)
{
  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (parameter_change_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposal_procedure_t* data = new_proposal_procedure();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_reward_address_ref(reward_address);
  data->reward_address = reward_address;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  cardano_parameter_change_action_ref(parameter_change_action);
  data->parameter_change_action = parameter_change_action;

  data->action_type = CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE;
  data->deposit     = deposit;

  *proposal = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_new_hard_fork_initiation_action(
  const uint64_t                         deposit,
  cardano_reward_address_t*              reward_address,
  cardano_anchor_t*                      anchor,
  cardano_hard_fork_initiation_action_t* hard_fork_initiation_action,
  cardano_proposal_procedure_t**         proposal)
{
  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposal_procedure_t* data = new_proposal_procedure();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_reward_address_ref(reward_address);
  data->reward_address = reward_address;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  cardano_hard_fork_initiation_action_ref(hard_fork_initiation_action);
  data->hard_fork_initiation_action = hard_fork_initiation_action;

  data->action_type = CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION;
  data->deposit     = deposit;

  *proposal = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_new_treasury_withdrawals_action(
  const uint64_t                         deposit,
  cardano_reward_address_t*              reward_address,
  cardano_anchor_t*                      anchor,
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action,
  cardano_proposal_procedure_t**         proposal)
{
  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_withdrawals_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposal_procedure_t* data = new_proposal_procedure();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_reward_address_ref(reward_address);
  data->reward_address = reward_address;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  cardano_treasury_withdrawals_action_ref(treasury_withdrawals_action);
  data->treasury_withdrawals_action = treasury_withdrawals_action;

  data->action_type = CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS;
  data->deposit     = deposit;

  *proposal = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_new_no_confidence_action(
  const uint64_t                  deposit,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_no_confidence_action_t* no_confidence_action,
  cardano_proposal_procedure_t**  proposal)
{
  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (no_confidence_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposal_procedure_t* data = new_proposal_procedure();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_reward_address_ref(reward_address);
  data->reward_address = reward_address;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  cardano_no_confidence_action_ref(no_confidence_action);
  data->no_confidence_action = no_confidence_action;

  data->action_type = CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE;
  data->deposit     = deposit;

  *proposal = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_new_update_committee_action(
  const uint64_t                     deposit,
  cardano_reward_address_t*          reward_address,
  cardano_anchor_t*                  anchor,
  cardano_update_committee_action_t* update_committee_action,
  cardano_proposal_procedure_t**     proposal)
{
  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposal_procedure_t* data = new_proposal_procedure();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_reward_address_ref(reward_address);
  data->reward_address = reward_address;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  cardano_update_committee_action_ref(update_committee_action);
  data->update_committee_action = update_committee_action;

  data->action_type = CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE;
  data->deposit     = deposit;

  *proposal = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_new_constitution_action(
  const uint64_t                     deposit,
  cardano_reward_address_t*          reward_address,
  cardano_anchor_t*                  anchor,
  cardano_new_constitution_action_t* new_constitution_action,
  cardano_proposal_procedure_t**     proposal)
{
  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (new_constitution_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposal_procedure_t* data = new_proposal_procedure();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_reward_address_ref(reward_address);
  data->reward_address = reward_address;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  cardano_new_constitution_action_ref(new_constitution_action);
  data->new_constitution_action = new_constitution_action;

  data->action_type = CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION;
  data->deposit     = deposit;

  *proposal = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_new_info_action(
  const uint64_t                 deposit,
  cardano_reward_address_t*      reward_address,
  cardano_anchor_t*              anchor,
  cardano_info_action_t*         info_action,
  cardano_proposal_procedure_t** proposal)
{
  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (info_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposal_procedure_t* data = new_proposal_procedure();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_reward_address_ref(reward_address);
  data->reward_address = reward_address;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  cardano_info_action_ref(info_action);
  data->info_action = info_action;

  data->action_type = CARDANO_GOVERNANCE_ACTION_TYPE_INFO;
  data->deposit     = deposit;

  *proposal = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_from_cbor(cardano_cbor_reader_t* reader, cardano_proposal_procedure_t** proposal_procedure)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal_procedure == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "proposal_procedure";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t deposit = 0;

  cardano_error_t read_deposit_result = cardano_cbor_reader_read_uint(reader, &deposit);

  if (read_deposit_result != CARDANO_SUCCESS)
  {
    return read_deposit_result;
  }

  cardano_buffer_t*         reward_address_bytes = NULL;
  cardano_reward_address_t* reward_address       = NULL;
  cardano_anchor_t*         anchor               = NULL;

  cardano_error_t read_reward_address_result = cardano_cbor_reader_read_bytestring(reader, &reward_address_bytes);

  if (read_reward_address_result != CARDANO_SUCCESS)
  {
    return read_reward_address_result;
  }

  cardano_error_t reward_address_result = cardano_reward_address_from_bytes(cardano_buffer_get_data(reward_address_bytes), cardano_buffer_get_size(reward_address_bytes), &reward_address);

  cardano_buffer_unref(&reward_address_bytes);

  if (reward_address_result != CARDANO_SUCCESS)
  {
    return reward_address_result;
  }

  cardano_buffer_t* encoded_action = NULL;

  cardano_error_t read_action_result = cardano_cbor_reader_read_encoded_value(reader, &encoded_action);

  if (read_action_result != CARDANO_SUCCESS)
  {
    cardano_reward_address_unref(&reward_address);
    return read_action_result;
  }

  cardano_error_t read_anchor_result = cardano_anchor_from_cbor(reader, &anchor);

  if (read_anchor_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&encoded_action);
    cardano_reward_address_unref(&reward_address);
    return read_anchor_result;
  }

  cardano_cbor_reader_t* action_reader        = cardano_cbor_reader_new(cardano_buffer_get_data(encoded_action), cardano_buffer_get_size(encoded_action));
  cardano_cbor_reader_t* action_reader_cloned = cardano_cbor_reader_new(cardano_buffer_get_data(encoded_action), cardano_buffer_get_size(encoded_action));

  cardano_buffer_unref(&encoded_action);

  int64_t         array_size  = 0;
  cardano_error_t peek_result = cardano_cbor_reader_read_start_array(action_reader_cloned, &array_size);

  if (peek_result != CARDANO_SUCCESS)
  {
    cardano_reward_address_unref(&reward_address);
    cardano_anchor_unref(&anchor);
    cardano_cbor_reader_unref(&action_reader);
    cardano_cbor_reader_unref(&action_reader_cloned);

    return peek_result;
  }

  uint64_t type = 0;

  cardano_error_t read_type_result = cardano_cbor_reader_read_uint(action_reader_cloned, &type);
  cardano_cbor_reader_unref(&action_reader_cloned);

  if (read_type_result != CARDANO_SUCCESS)
  {
    cardano_reward_address_unref(&reward_address);
    cardano_anchor_unref(&anchor);
    cardano_cbor_reader_unref(&action_reader);

    return read_type_result;
  }

  cardano_error_t final_result = CARDANO_SUCCESS;

  switch (type)
  {
    case CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE:
    {
      cardano_parameter_change_action_t* parameter_change_action = NULL;

      cardano_error_t read_parameter_change_action_result = cardano_parameter_change_action_from_cbor(action_reader, &parameter_change_action);

      if (read_parameter_change_action_result != CARDANO_SUCCESS)
      {
        cardano_reward_address_unref(&reward_address);
        cardano_anchor_unref(&anchor);
        cardano_cbor_reader_unref(&action_reader);

        return read_parameter_change_action_result;
      }

      cardano_error_t new_instance_result = cardano_proposal_procedure_new_parameter_change_action(deposit, reward_address, anchor, parameter_change_action, proposal_procedure);

      cardano_parameter_change_action_unref(&parameter_change_action);
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      final_result = new_instance_result;
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION:
    {
      cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = NULL;

      cardano_error_t read_hard_fork_initiation_action_result = cardano_hard_fork_initiation_action_from_cbor(action_reader, &hard_fork_initiation_action);

      if (read_hard_fork_initiation_action_result != CARDANO_SUCCESS)
      {
        cardano_reward_address_unref(&reward_address);
        cardano_anchor_unref(&anchor);
        cardano_cbor_reader_unref(&action_reader);

        return read_hard_fork_initiation_action_result;
      }

      cardano_error_t new_instance_result = cardano_proposal_procedure_new_hard_fork_initiation_action(deposit, reward_address, anchor, hard_fork_initiation_action, proposal_procedure);

      cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      final_result = new_instance_result;
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS:
    {
      cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;

      cardano_error_t read_treasury_withdrawals_action_result = cardano_treasury_withdrawals_action_from_cbor(action_reader, &treasury_withdrawals_action);

      if (read_treasury_withdrawals_action_result != CARDANO_SUCCESS)
      {
        cardano_reward_address_unref(&reward_address);
        cardano_anchor_unref(&anchor);
        cardano_cbor_reader_unref(&action_reader);

        return read_treasury_withdrawals_action_result;
      }

      cardano_error_t new_instance_result = cardano_proposal_procedure_new_treasury_withdrawals_action(deposit, reward_address, anchor, treasury_withdrawals_action, proposal_procedure);

      cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      final_result = new_instance_result;
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE:
    {
      cardano_no_confidence_action_t* no_confidence_action = NULL;

      cardano_error_t read_no_confidence_action_result = cardano_no_confidence_action_from_cbor(action_reader, &no_confidence_action);

      if (read_no_confidence_action_result != CARDANO_SUCCESS)
      {
        cardano_reward_address_unref(&reward_address);
        cardano_anchor_unref(&anchor);
        cardano_cbor_reader_unref(&action_reader);

        return read_no_confidence_action_result;
      }

      cardano_error_t new_instance_result = cardano_proposal_procedure_new_no_confidence_action(deposit, reward_address, anchor, no_confidence_action, proposal_procedure);

      cardano_no_confidence_action_unref(&no_confidence_action);
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      final_result = new_instance_result;
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE:
    {
      cardano_update_committee_action_t* update_committee_action = NULL;

      cardano_error_t read_update_committee_action_result = cardano_update_committee_action_from_cbor(action_reader, &update_committee_action);

      if (read_update_committee_action_result != CARDANO_SUCCESS)
      {
        cardano_reward_address_unref(&reward_address);
        cardano_anchor_unref(&anchor);
        cardano_cbor_reader_unref(&action_reader);

        return read_update_committee_action_result;
      }

      cardano_error_t new_instance_result = cardano_proposal_procedure_new_update_committee_action(deposit, reward_address, anchor, update_committee_action, proposal_procedure);

      cardano_update_committee_action_unref(&update_committee_action);
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      final_result = new_instance_result;
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION:
    {
      cardano_new_constitution_action_t* new_constitution_action = NULL;

      cardano_error_t read_new_constitution_action_result = cardano_new_constitution_action_from_cbor(action_reader, &new_constitution_action);

      if (read_new_constitution_action_result != CARDANO_SUCCESS)
      {
        cardano_reward_address_unref(&reward_address);
        cardano_anchor_unref(&anchor);
        cardano_cbor_reader_unref(&action_reader);

        return read_new_constitution_action_result;
      }

      cardano_error_t new_instance_result = cardano_proposal_procedure_new_constitution_action(deposit, reward_address, anchor, new_constitution_action, proposal_procedure);

      cardano_new_constitution_action_unref(&new_constitution_action);
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      final_result = new_instance_result;
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_INFO:
    {
      cardano_info_action_t* info_action = NULL;

      cardano_error_t read_info_action_result = cardano_info_action_from_cbor(action_reader, &info_action);

      if (read_info_action_result != CARDANO_SUCCESS)
      {
        cardano_reward_address_unref(&reward_address);
        cardano_anchor_unref(&anchor);
        cardano_cbor_reader_unref(&action_reader);

        return read_info_action_result;
      }

      cardano_error_t new_instance_result = cardano_proposal_procedure_new_info_action(deposit, reward_address, anchor, info_action, proposal_procedure);

      cardano_info_action_unref(&info_action);
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      final_result = new_instance_result;
      break;
    }

    default:
    {
      cardano_reward_address_unref(&reward_address);
      cardano_anchor_unref(&anchor);
      cardano_cbor_reader_unref(&action_reader);

      return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
    }
  }

  if (final_result != CARDANO_SUCCESS)
  {
    return final_result;
  }

  return cardano_cbor_reader_read_end_array(reader);
}

cardano_error_t
cardano_proposal_procedure_to_cbor(
  const cardano_proposal_procedure_t* proposal_procedure,
  cardano_cbor_writer_t*              writer)
{
  if (proposal_procedure == NULL)
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

  cardano_error_t write_deposit_result = cardano_cbor_writer_write_uint(writer, proposal_procedure->deposit);

  if (write_deposit_result != CARDANO_SUCCESS)
  {
    return write_deposit_result;
  }

  cardano_error_t write_reward_address_result = cardano_cbor_writer_write_bytestring(writer, cardano_reward_address_get_bytes(proposal_procedure->reward_address), cardano_reward_address_get_bytes_size(proposal_procedure->reward_address));

  if (write_reward_address_result != CARDANO_SUCCESS)
  {
    return write_reward_address_result;
  }

  switch (proposal_procedure->action_type)
  {
    case CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE:
    {
      cardano_error_t write_action_result = cardano_parameter_change_action_to_cbor(proposal_procedure->parameter_change_action, writer);

      if (write_action_result != CARDANO_SUCCESS)
      {
        return write_action_result;
      }

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION:
    {
      cardano_error_t write_action_result = cardano_hard_fork_initiation_action_to_cbor(proposal_procedure->hard_fork_initiation_action, writer);

      if (write_action_result != CARDANO_SUCCESS)
      {
        return write_action_result;
      }

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS:
    {
      cardano_error_t write_action_result = cardano_treasury_withdrawals_action_to_cbor(proposal_procedure->treasury_withdrawals_action, writer);

      if (write_action_result != CARDANO_SUCCESS)
      {
        return write_action_result;
      }

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE:
    {
      cardano_error_t write_action_result = cardano_no_confidence_action_to_cbor(proposal_procedure->no_confidence_action, writer);

      if (write_action_result != CARDANO_SUCCESS)
      {
        return write_action_result;
      }

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE:
    {
      cardano_error_t write_action_result = cardano_update_committee_action_to_cbor(proposal_procedure->update_committee_action, writer);

      if (write_action_result != CARDANO_SUCCESS)
      {
        return write_action_result;
      }

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION:
    {
      cardano_error_t write_action_result = cardano_new_constitution_action_to_cbor(proposal_procedure->new_constitution_action, writer);

      if (write_action_result != CARDANO_SUCCESS)
      {
        return write_action_result;
      }

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_INFO:
    {
      cardano_error_t write_action_result = cardano_info_action_to_cbor(proposal_procedure->info_action, writer);

      if (write_action_result != CARDANO_SUCCESS)
      {
        return write_action_result;
      }

      break;
    }

    default:
    {
      return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
    }
  }

  cardano_error_t write_anchor_result = cardano_anchor_to_cbor(proposal_procedure->anchor, writer);

  if (write_anchor_result != CARDANO_SUCCESS)
  {
    return write_anchor_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_get_action_type(
  const cardano_proposal_procedure_t* certificate,
  cardano_governance_action_type_t*   type)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = certificate->action_type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_to_parameter_change_action(
  cardano_proposal_procedure_t*       proposal,
  cardano_parameter_change_action_t** parameter_change_action)
{
  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (parameter_change_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal->action_type != CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE)
  {
    return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
  }

  cardano_parameter_change_action_ref(proposal->parameter_change_action);
  *parameter_change_action = proposal->parameter_change_action;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_to_hard_fork_initiation_action(
  cardano_proposal_procedure_t*           proposal,
  cardano_hard_fork_initiation_action_t** hard_fork_initiation_action)
{
  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal->action_type != CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION)
  {
    return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
  }

  cardano_hard_fork_initiation_action_ref(proposal->hard_fork_initiation_action);
  *hard_fork_initiation_action = proposal->hard_fork_initiation_action;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_to_treasury_withdrawals_action(
  cardano_proposal_procedure_t*           proposal,
  cardano_treasury_withdrawals_action_t** treasury_withdrawals_action)
{
  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_withdrawals_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal->action_type != CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS)
  {
    return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
  }

  cardano_treasury_withdrawals_action_ref(proposal->treasury_withdrawals_action);
  *treasury_withdrawals_action = proposal->treasury_withdrawals_action;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_to_no_confidence_action(
  cardano_proposal_procedure_t*    proposal,
  cardano_no_confidence_action_t** no_confidence_action)
{
  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (no_confidence_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal->action_type != CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE)
  {
    return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
  }

  cardano_no_confidence_action_ref(proposal->no_confidence_action);
  *no_confidence_action = proposal->no_confidence_action;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_to_update_committee_action(
  cardano_proposal_procedure_t*       proposal,
  cardano_update_committee_action_t** update_committee_action)
{
  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal->action_type != CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE)
  {
    return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
  }

  cardano_update_committee_action_ref(proposal->update_committee_action);
  *update_committee_action = proposal->update_committee_action;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_to_constitution_action(
  cardano_proposal_procedure_t*       proposal,
  cardano_new_constitution_action_t** constitution_action)
{
  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constitution_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal->action_type != CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION)
  {
    return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
  }

  cardano_new_constitution_action_ref(proposal->new_constitution_action);
  *constitution_action = proposal->new_constitution_action;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_to_info_action(
  cardano_proposal_procedure_t* proposal,
  cardano_info_action_t**       info_action)
{
  if (proposal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (info_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposal->action_type != CARDANO_GOVERNANCE_ACTION_TYPE_INFO)
  {
    return CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE;
  }

  cardano_info_action_ref(proposal->info_action);
  *info_action = proposal->info_action;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_proposal_procedure_set_anchor(cardano_proposal_procedure_t* proposal_procedure, cardano_anchor_t* anchor)
{
  if (proposal_procedure == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_ref(anchor);
  cardano_anchor_unref(&proposal_procedure->anchor);
  proposal_procedure->anchor = anchor;

  return CARDANO_SUCCESS;
}

cardano_anchor_t*
cardano_proposal_procedure_get_anchor(cardano_proposal_procedure_t* proposal_procedure)
{
  if (proposal_procedure == NULL)
  {
    return NULL;
  }

  cardano_anchor_ref(proposal_procedure->anchor);
  return proposal_procedure->anchor;
}

cardano_error_t
cardano_proposal_procedure_set_reward_address(cardano_proposal_procedure_t* proposal_procedure, cardano_reward_address_t* reward_address)
{
  if (proposal_procedure == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reward_address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_reward_address_ref(reward_address);
  cardano_reward_address_unref(&proposal_procedure->reward_address);
  proposal_procedure->reward_address = reward_address;

  return CARDANO_SUCCESS;
}

cardano_reward_address_t*
cardano_proposal_procedure_get_reward_address(cardano_proposal_procedure_t* proposal_procedure)
{
  if (proposal_procedure == NULL)
  {
    return NULL;
  }

  cardano_reward_address_ref(proposal_procedure->reward_address);
  return proposal_procedure->reward_address;
}

uint64_t
cardano_proposal_procedure_get_deposit(const cardano_proposal_procedure_t* certificate)
{
  if (certificate == NULL)
  {
    return 0;
  }

  return certificate->deposit;
}

cardano_error_t
cardano_proposal_procedure_set_deposit(cardano_proposal_procedure_t* certificate, const uint64_t deposit)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  certificate->deposit = deposit;

  return CARDANO_SUCCESS;
}

void
cardano_proposal_procedure_unref(cardano_proposal_procedure_t** proposal_procedure)
{
  if ((proposal_procedure == NULL) || (*proposal_procedure == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*proposal_procedure)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *proposal_procedure = NULL;
    return;
  }
}

void
cardano_proposal_procedure_ref(cardano_proposal_procedure_t* proposal_procedure)
{
  if (proposal_procedure == NULL)
  {
    return;
  }

  cardano_object_ref(&proposal_procedure->base);
}

size_t
cardano_proposal_procedure_refcount(const cardano_proposal_procedure_t* proposal_procedure)
{
  if (proposal_procedure == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&proposal_procedure->base);
}

void
cardano_proposal_procedure_set_last_error(cardano_proposal_procedure_t* proposal_procedure, const char* message)
{
  cardano_object_set_last_error(&proposal_procedure->base, message);
}

const char*
cardano_proposal_procedure_get_last_error(const cardano_proposal_procedure_t* proposal_procedure)
{
  return cardano_object_get_last_error(&proposal_procedure->base);
}
