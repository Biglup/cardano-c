/**
 * \file update_committee_action.c
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
#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/committee_members_map.h>
#include <cardano/proposal_procedures/credential_set.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/update_committee_action.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 5;

/* STRUCTURES ****************************************************************/

/**
 * \brief Modifies the composition of the constitutional committee, its signature threshold, or its terms of operation.
 */
typedef struct cardano_update_committee_action_t
{
    cardano_object_t                 base;
    cardano_credential_set_t*        members_to_be_removed;
    cardano_committee_members_map_t* members_to_be_added;
    cardano_unit_interval_t*         new_quorum;
    cardano_governance_action_id_t*  governance_action_id;
} cardano_update_committee_action_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a update_committee_action object.
 *
 * This function is responsible for properly deallocating a update_committee_action object (`cardano_update_committee_action_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the update_committee_action object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_update_committee_action_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the update_committee_action
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_update_committee_action_deallocate(void* object)
{
  assert(object != NULL);

  cardano_update_committee_action_t* data = (cardano_update_committee_action_t*)object;

  cardano_governance_action_id_unref(&data->governance_action_id);
  cardano_credential_set_unref(&data->members_to_be_removed);
  cardano_committee_members_map_unref(&data->members_to_be_added);
  cardano_unit_interval_unref(&data->new_quorum);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_update_committee_action_new(
  cardano_credential_set_t*           members_to_be_removed,
  cardano_committee_members_map_t*    members_to_be_added,
  cardano_unit_interval_t*            new_quorum,
  cardano_governance_action_id_t*     governance_action_id,
  cardano_update_committee_action_t** update_committee_action)
{
  if (members_to_be_removed == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (members_to_be_added == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (new_quorum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_update_committee_action_t* data = _cardano_malloc(sizeof(cardano_update_committee_action_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count        = 1;
  data->base.last_error[0]    = '\0';
  data->base.deallocator      = cardano_update_committee_action_deallocate;
  data->governance_action_id  = NULL;
  data->members_to_be_removed = NULL;
  data->members_to_be_added   = NULL;
  data->new_quorum            = NULL;

  cardano_credential_set_ref(members_to_be_removed);
  data->members_to_be_removed = members_to_be_removed;

  cardano_committee_members_map_ref(members_to_be_added);
  data->members_to_be_added = members_to_be_added;

  cardano_unit_interval_ref(new_quorum);
  data->new_quorum = new_quorum;

  data->governance_action_id = NULL;

  if (governance_action_id != NULL)
  {
    cardano_governance_action_id_ref(governance_action_id);
    data->governance_action_id = governance_action_id;
  }

  *update_committee_action = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_committee_action_from_cbor(cardano_cbor_reader_t* reader, cardano_update_committee_action_t** update_committee_action)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "update_committee_action";

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
    CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE,
    (enum_to_string_callback_t)((void*)&cardano_governance_action_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_credential_set_t*        members_to_be_removed = NULL;
  cardano_committee_members_map_t* members_to_be_added   = NULL;
  cardano_unit_interval_t*         new_quorum            = NULL;
  cardano_governance_action_id_t*  governance_action_id  = NULL;

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

  cardano_error_t members_to_be_removed_result = cardano_credential_set_from_cbor(reader, &members_to_be_removed);

  if (members_to_be_removed_result != CARDANO_SUCCESS)
  {
    cardano_governance_action_id_unref(&governance_action_id);
    return members_to_be_removed_result;
  }

  cardano_error_t members_to_be_added_result = cardano_committee_members_map_from_cbor(reader, &members_to_be_added);

  if (members_to_be_added_result != CARDANO_SUCCESS)
  {
    cardano_governance_action_id_unref(&governance_action_id);
    cardano_credential_set_unref(&members_to_be_removed);
    return members_to_be_added_result;
  }

  cardano_error_t new_quorum_result = cardano_unit_interval_from_cbor(reader, &new_quorum);

  if (new_quorum_result != CARDANO_SUCCESS)
  {
    cardano_governance_action_id_unref(&governance_action_id);
    cardano_credential_set_unref(&members_to_be_removed);
    cardano_committee_members_map_unref(&members_to_be_added);
    return new_quorum_result;
  }

  cardano_error_t new_result = cardano_update_committee_action_new(
    members_to_be_removed,
    members_to_be_added,
    new_quorum,
    governance_action_id,
    update_committee_action);

  cardano_governance_action_id_unref(&governance_action_id);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_update_committee_action_to_cbor(
  const cardano_update_committee_action_t* update_committee_action,
  cardano_cbor_writer_t*                   writer)
{
  if (update_committee_action == NULL)
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

  cardano_error_t write_enum_result = cardano_cbor_writer_write_uint(writer, CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE);

  if (write_enum_result != CARDANO_SUCCESS)
  {
    return write_enum_result;
  }

  if (update_committee_action->governance_action_id == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t governance_action_id_result = cardano_governance_action_id_to_cbor(update_committee_action->governance_action_id, writer);

    if (governance_action_id_result != CARDANO_SUCCESS)
    {
      return governance_action_id_result;
    }
  }

  cardano_error_t members_to_be_removed_result = cardano_credential_set_to_cbor(update_committee_action->members_to_be_removed, writer);

  if (members_to_be_removed_result != CARDANO_SUCCESS)
  {
    return members_to_be_removed_result;
  }

  cardano_error_t members_to_be_added_result = cardano_committee_members_map_to_cbor(update_committee_action->members_to_be_added, writer);

  if (members_to_be_added_result != CARDANO_SUCCESS)
  {
    return members_to_be_added_result;
  }

  cardano_error_t new_quorum_result = cardano_unit_interval_to_cbor(update_committee_action->new_quorum, writer);

  if (new_quorum_result != CARDANO_SUCCESS)
  {
    return new_quorum_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_committee_action_set_members_to_be_removed(
  cardano_update_committee_action_t* update_committee_action,
  cardano_credential_set_t*          members_to_be_removed)
{
  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (members_to_be_removed == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_set_ref(members_to_be_removed);
  cardano_credential_set_unref(&update_committee_action->members_to_be_removed);
  update_committee_action->members_to_be_removed = members_to_be_removed;

  return CARDANO_SUCCESS;
}

cardano_credential_set_t*
cardano_update_committee_action_get_members_to_be_removed(
  cardano_update_committee_action_t* update_committee_action)
{
  if (update_committee_action == NULL)
  {
    return NULL;
  }

  cardano_credential_set_ref(update_committee_action->members_to_be_removed);

  return update_committee_action->members_to_be_removed;
}

cardano_error_t
cardano_update_committee_action_set_members_to_be_added(
  cardano_update_committee_action_t* update_committee_action,
  cardano_committee_members_map_t*   members_to_be_added)
{
  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (members_to_be_added == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_committee_members_map_ref(members_to_be_added);
  cardano_committee_members_map_unref(&update_committee_action->members_to_be_added);
  update_committee_action->members_to_be_added = members_to_be_added;

  return CARDANO_SUCCESS;
}

cardano_committee_members_map_t*
cardano_update_committee_action_get_members_to_be_added(
  cardano_update_committee_action_t* update_committee_action)
{
  if (update_committee_action == NULL)
  {
    return NULL;
  }

  cardano_committee_members_map_ref(update_committee_action->members_to_be_added);

  return update_committee_action->members_to_be_added;
}

cardano_error_t
cardano_update_committee_action_set_quorum(
  cardano_update_committee_action_t* update_committee_action,
  cardano_unit_interval_t*           quorum)
{
  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (quorum == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(quorum);
  cardano_unit_interval_unref(&update_committee_action->new_quorum);
  update_committee_action->new_quorum = quorum;

  return CARDANO_SUCCESS;
}

cardano_unit_interval_t*
cardano_update_committee_action_get_quorum(cardano_update_committee_action_t* update_committee_action)
{
  if (update_committee_action == NULL)
  {
    return NULL;
  }

  cardano_unit_interval_ref(update_committee_action->new_quorum);

  return update_committee_action->new_quorum;
}

cardano_error_t
cardano_update_committee_action_set_governance_action_id(
  cardano_update_committee_action_t* update_committee_action,
  cardano_governance_action_id_t*    governance_action_id)
{
  if (update_committee_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_governance_action_id_ref(governance_action_id);
  cardano_governance_action_id_unref(&update_committee_action->governance_action_id);
  update_committee_action->governance_action_id = governance_action_id;

  return CARDANO_SUCCESS;
}

cardano_governance_action_id_t*
cardano_update_committee_action_get_governance_action_id(
  cardano_update_committee_action_t* update_committee_action)
{
  if (update_committee_action == NULL)
  {
    return NULL;
  }

  cardano_governance_action_id_ref(update_committee_action->governance_action_id);

  return update_committee_action->governance_action_id;
}

void
cardano_update_committee_action_unref(cardano_update_committee_action_t** update_committee_action)
{
  if ((update_committee_action == NULL) || (*update_committee_action == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*update_committee_action)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *update_committee_action = NULL;
    return;
  }
}

void
cardano_update_committee_action_ref(cardano_update_committee_action_t* update_committee_action)
{
  if (update_committee_action == NULL)
  {
    return;
  }

  cardano_object_ref(&update_committee_action->base);
}

size_t
cardano_update_committee_action_refcount(const cardano_update_committee_action_t* update_committee_action)
{
  if (update_committee_action == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&update_committee_action->base);
}

void
cardano_update_committee_action_set_last_error(cardano_update_committee_action_t* update_committee_action, const char* message)
{
  cardano_object_set_last_error(&update_committee_action->base, message);
}

const char*
cardano_update_committee_action_get_last_error(const cardano_update_committee_action_t* update_committee_action)
{
  return cardano_object_get_last_error(&update_committee_action->base);
}
