/**
 * \file info_action.c
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

#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/info_action.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 1;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an action that has no direct effect on the blockchain,
 * but serves as an on-chain record or informative notice.
 */
typedef struct cardano_info_action_t
{
    cardano_object_t base;
} cardano_info_action_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a info_action object.
 *
 * This function is responsible for properly deallocating a info_action object (`cardano_info_action_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the info_action object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_info_action_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the info_action
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_info_action_deallocate(void* object)
{
  assert(object != NULL);

  cardano_info_action_t* data = (cardano_info_action_t*)object;

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_info_action_new(cardano_info_action_t** info_action)
{
  if (info_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_info_action_t* data = _cardano_malloc(sizeof(cardano_info_action_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_info_action_deallocate;

  *info_action = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_info_action_from_cbor(cardano_cbor_reader_t* reader, cardano_info_action_t** info_action)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (info_action == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "info_action";

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
    CARDANO_GOVERNANCE_ACTION_TYPE_INFO,
    (enum_to_string_callback_t)((void*)&cardano_governance_action_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_error_t end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (end_array_result != CARDANO_SUCCESS)
  {
    return end_array_result;
  }

  return cardano_info_action_new(info_action);
}

cardano_error_t
cardano_info_action_to_cbor(
  const cardano_info_action_t* info_action,
  cardano_cbor_writer_t*       writer)
{
  if (info_action == NULL)
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

  cardano_error_t write_enum_result = cardano_cbor_writer_write_uint(writer, CARDANO_GOVERNANCE_ACTION_TYPE_INFO);

  if (write_enum_result != CARDANO_SUCCESS)
  {
    return write_enum_result;
  }

  return CARDANO_SUCCESS;
}

void
cardano_info_action_unref(cardano_info_action_t** info_action)
{
  if ((info_action == NULL) || (*info_action == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*info_action)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *info_action = NULL;
    return;
  }
}

void
cardano_info_action_ref(cardano_info_action_t* info_action)
{
  if (info_action == NULL)
  {
    return;
  }

  cardano_object_ref(&info_action->base);
}

size_t
cardano_info_action_refcount(const cardano_info_action_t* info_action)
{
  if (info_action == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&info_action->base);
}

void
cardano_info_action_set_last_error(cardano_info_action_t* info_action, const char* message)
{
  cardano_object_set_last_error(&info_action->base, message);
}

const char*
cardano_info_action_get_last_error(const cardano_info_action_t* info_action)
{
  return cardano_object_get_last_error(&info_action->base);
}
