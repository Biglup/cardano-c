/**
 * \file no_confidence_action.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license");
 * you may not use this file except in COMPLIANCE with the license.
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
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/no_confidence_action.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Propose a state of no-confidence in the current constitutional committee.
 * Allows Ada holders to challenge the authority granted to the existing committee.
 */
typedef struct cardano_no_confidence_action_t
{
    cardano_object_t                base;
    cardano_governance_action_id_t* governance_action_id;
} cardano_no_confidence_action_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a no_confidence_action object.
 *
 * This function is responsible for properly deallocating a no_confidence_action object (`cardano_no_confidence_action_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the no_confidence_action object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_no_confidence_action_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the no_confidence_action
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_no_confidence_action_deallocate(void* object)
{
  assert(object != NULL);

  cardano_no_confidence_action_t* data = (cardano_no_confidence_action_t*)object;

  cardano_governance_action_id_unref(&data->governance_action_id);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_no_confidence_action_new(
  cardano_governance_action_id_t*  governance_action_id,
  cardano_no_confidence_action_t** no_confidence_action)
{
  if (no_confidence_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_no_confidence_action_t* data = _cardano_malloc(sizeof(cardano_no_confidence_action_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count       = 1;
  data->base.last_error[0]   = '\0';
  data->base.deallocator     = cardano_no_confidence_action_deallocate;
  data->governance_action_id = NULL;

  if (governance_action_id != NULL)
  {
    cardano_governance_action_id_ref(governance_action_id);
    data->governance_action_id = governance_action_id;
  }

  *no_confidence_action = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_no_confidence_action_from_cbor(cardano_cbor_reader_t* reader, cardano_no_confidence_action_t** no_confidence_action)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (no_confidence_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "no_confidence_action";

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
    CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE,
    (enum_to_string_callback_t)((void*)&cardano_governance_action_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_governance_action_id_t* governance_action_id = NULL;

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

  cardano_error_t new_result = cardano_no_confidence_action_new(governance_action_id, no_confidence_action);

  cardano_governance_action_id_unref(&governance_action_id);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_no_confidence_action_to_cbor(
  const cardano_no_confidence_action_t* no_confidence_action,
  cardano_cbor_writer_t*                writer)
{
  if (no_confidence_action == NULL)
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

  cardano_error_t write_enum_result = cardano_cbor_writer_write_uint(writer, CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE);

  if (write_enum_result != CARDANO_SUCCESS)
  {
    return write_enum_result;
  }

  if (no_confidence_action->governance_action_id == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t governance_action_id_result = cardano_governance_action_id_to_cbor(no_confidence_action->governance_action_id, writer);

    if (governance_action_id_result != CARDANO_SUCCESS)
    {
      return governance_action_id_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_no_confidence_action_set_governance_action_id(
  cardano_no_confidence_action_t* no_confidence_action,
  cardano_governance_action_id_t* governance_action_id)
{
  if (no_confidence_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_governance_action_id_ref(governance_action_id);
  cardano_governance_action_id_unref(&no_confidence_action->governance_action_id);
  no_confidence_action->governance_action_id = governance_action_id;

  return CARDANO_SUCCESS;
}

cardano_governance_action_id_t*
cardano_no_confidence_action_get_governance_action_id(
  cardano_no_confidence_action_t* no_confidence_action)
{
  if (no_confidence_action == NULL)
  {
    return NULL;
  }

  cardano_governance_action_id_ref(no_confidence_action->governance_action_id);

  return no_confidence_action->governance_action_id;
}

void
cardano_no_confidence_action_unref(cardano_no_confidence_action_t** no_confidence_action)
{
  if ((no_confidence_action == NULL) || (*no_confidence_action == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*no_confidence_action)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *no_confidence_action = NULL;
    return;
  }
}

void
cardano_no_confidence_action_ref(cardano_no_confidence_action_t* no_confidence_action)
{
  if (no_confidence_action == NULL)
  {
    return;
  }

  cardano_object_ref(&no_confidence_action->base);
}

size_t
cardano_no_confidence_action_refcount(const cardano_no_confidence_action_t* no_confidence_action)
{
  if (no_confidence_action == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&no_confidence_action->base);
}

void
cardano_no_confidence_action_set_last_error(cardano_no_confidence_action_t* no_confidence_action, const char* message)
{
  cardano_object_set_last_error(&no_confidence_action->base, message);
}

const char*
cardano_no_confidence_action_get_last_error(const cardano_no_confidence_action_t* no_confidence_action)
{
  return cardano_object_get_last_error(&no_confidence_action->base);
}
