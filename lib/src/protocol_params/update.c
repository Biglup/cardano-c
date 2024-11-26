/**
 * \file update.c
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/object.h>
#include <cardano/protocol_params/proposed_param_updates.h>
#include <cardano/protocol_params/update.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief When stakeholders wish to propose changes to the system's parameters, they submit an update proposal.
 * Such proposals are then voted on by the community. If approved, the protocol parameters are adjusted
 * accordingly in the specified epoch.
 */
typedef struct cardano_update_t
{
    cardano_object_t                  base;
    uint64_t                          epoch;
    cardano_proposed_param_updates_t* proposed_param_updates;
} cardano_update_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a update object.
 *
 * This function is responsible for properly deallocating a update object (`cardano_update_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the update object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_update_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the update
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_update_deallocate(void* object)
{
  assert(object != NULL);

  cardano_update_t* data = (cardano_update_t*)object;

  cardano_proposed_param_updates_unref(&data->proposed_param_updates);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_update_new(uint64_t epoch, cardano_proposed_param_updates_t* updates, cardano_update_t** update)
{
  if (update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *update = _cardano_malloc(sizeof(cardano_update_t));

  if (*update == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*update)->base.deallocator   = cardano_update_deallocate;
  (*update)->base.ref_count     = 1;
  (*update)->base.last_error[0] = '\0';

  (*update)->epoch = epoch;

  cardano_proposed_param_updates_ref(updates);

  (*update)->proposed_param_updates = updates;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_from_cbor(cardano_cbor_reader_t* reader, cardano_update_t** update)
{
  if (update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *update = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "update";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *update = NULL;
    return expect_array_result;
  }

  cardano_proposed_param_updates_t* proposed_param_updates = NULL;

  cardano_error_t error = cardano_proposed_param_updates_from_cbor(reader, &proposed_param_updates);

  if (error != CARDANO_SUCCESS)
  {
    *update = NULL;
    return error;
  }

  uint64_t epoch = 0;

  error = cardano_cbor_reader_read_uint(reader, &epoch);

  if (error != CARDANO_SUCCESS)
  {
    cardano_proposed_param_updates_unref(&proposed_param_updates);
    *update = NULL;
    return error;
  }

  error = cardano_cbor_validate_end_array(validator_name, reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_proposed_param_updates_unref(&proposed_param_updates);
    *update = NULL;
    return error;
  }

  error = cardano_update_new(epoch, proposed_param_updates, update);

  cardano_proposed_param_updates_unref(&proposed_param_updates);

  if (error != CARDANO_SUCCESS)
  {
    *update = NULL;
    return error;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_to_cbor(const cardano_update_t* update, cardano_cbor_writer_t* writer)
{
  if (update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t error = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_proposed_param_updates_to_cbor(update->proposed_param_updates, writer);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_cbor_writer_write_uint(writer, update->epoch);

  return error;
}

cardano_error_t
cardano_update_get_epoch(
  const cardano_update_t* update,
  uint64_t*               epoch)
{
  if (update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (epoch == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *epoch = update->epoch;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_get_proposed_parameters(
  const cardano_update_t*            update,
  cardano_proposed_param_updates_t** proposed_parameters)
{
  if (update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (proposed_parameters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *proposed_parameters = update->proposed_param_updates;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_set_epoch(
  cardano_update_t* update,
  uint64_t          epoch)
{
  if (update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  update->epoch = epoch;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_set_proposed_parameters(
  cardano_update_t*                 update,
  cardano_proposed_param_updates_t* proposed_parameters)
{
  if (update == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_proposed_param_updates_ref(proposed_parameters);

  cardano_proposed_param_updates_unref(&update->proposed_param_updates);

  update->proposed_param_updates = proposed_parameters;

  return CARDANO_SUCCESS;
}

void
cardano_update_unref(cardano_update_t** update)
{
  if ((update == NULL) || (*update == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*update)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *update = NULL;
    return;
  }
}

void
cardano_update_ref(cardano_update_t* update)
{
  if (update == NULL)
  {
    return;
  }

  cardano_object_ref(&update->base);
}

size_t
cardano_update_refcount(const cardano_update_t* update)
{
  if (update == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&update->base);
}

void
cardano_update_set_last_error(cardano_update_t* update, const char* message)
{
  cardano_object_set_last_error(&update->base, message);
}

const char*
cardano_update_get_last_error(const cardano_update_t* update)
{
  return cardano_object_get_last_error(&update->base);
}