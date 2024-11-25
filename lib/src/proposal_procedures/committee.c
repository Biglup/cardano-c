/**
 * \file committee.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2024
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

#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/committee.h>
#include <cardano/proposal_procedures/committee_members_map.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief The constitutional committee represents a set of individuals or entities (each associated with a pair of Ed25519 credentials)
 * that are collectively responsible for ensuring that the Constitution is respected.
 *
 * Though it cannot be enforced on-chain, the constitutional committee is only supposed to vote on the constitutionality
 * of governance actions (which should thus ensure the long-term sustainability of the blockchain) and should be replaced
 * (via the no confidence action) if they overstep this boundary.
 */
typedef struct cardano_committee_t
{
    cardano_object_t                 base;
    cardano_unit_interval_t*         quorum_threshold;
    cardano_committee_members_map_t* committee_members_map;
} cardano_committee_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a committee object.
 *
 * This function is responsible for properly deallocating a committee object (`cardano_committee_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the committee object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_committee_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the committee
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_committee_deallocate(void* object)
{
  assert(object != NULL);

  cardano_committee_t* data = (cardano_committee_t*)object;

  cardano_unit_interval_unref(&data->quorum_threshold);
  cardano_committee_members_map_unref(&data->committee_members_map);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_committee_new(
  cardano_unit_interval_t* quorum_threshold,
  cardano_committee_t**    committee)
{
  if (quorum_threshold == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_committee_t* data = _cardano_malloc(sizeof(cardano_committee_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_committee_deallocate;

  cardano_unit_interval_ref(quorum_threshold);
  data->quorum_threshold = quorum_threshold;

  cardano_error_t new_map_result = cardano_committee_members_map_new(&data->committee_members_map);

  if (new_map_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&data->quorum_threshold);
    _cardano_free(data);

    return new_map_result;
  }

  *committee = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_from_cbor(cardano_cbor_reader_t* reader, cardano_committee_t** committee)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "committee";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  cardano_unit_interval_t*         quorum_threshold      = NULL;
  cardano_committee_members_map_t* committee_members_map = NULL;

  cardano_error_t read_map_result = cardano_committee_members_map_from_cbor(reader, &committee_members_map);

  if (read_map_result != CARDANO_SUCCESS)
  {
    return read_map_result;
  }

  cardano_error_t read_quorum_result = cardano_unit_interval_from_cbor(reader, &quorum_threshold);

  if (read_quorum_result != CARDANO_SUCCESS)
  {
    cardano_committee_members_map_unref(&committee_members_map);
    return read_quorum_result;
  }

  cardano_error_t new_result = cardano_committee_new(quorum_threshold, committee);

  cardano_unit_interval_unref(&quorum_threshold);

  if (new_result != CARDANO_SUCCESS)
  {
    cardano_committee_members_map_unref(&committee_members_map);
    return new_result;
  }

  cardano_committee_members_map_unref(&(*committee)->committee_members_map);
  (*committee)->committee_members_map = committee_members_map;

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_committee_to_cbor(
  const cardano_committee_t* committee,
  cardano_cbor_writer_t*     writer)
{
  if (committee == NULL)
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

  cardano_error_t write_map_result = cardano_committee_members_map_to_cbor(committee->committee_members_map, writer);

  if (write_map_result != CARDANO_SUCCESS)
  {
    return write_map_result;
  }

  cardano_error_t write_quorum_result = cardano_unit_interval_to_cbor(committee->quorum_threshold, writer);

  if (write_quorum_result != CARDANO_SUCCESS)
  {
    return write_quorum_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_committee_set_quorum_threshold(
  cardano_committee_t*     committee,
  cardano_unit_interval_t* quorum_threshold)
{
  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (quorum_threshold == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&committee->quorum_threshold);
  cardano_unit_interval_ref(quorum_threshold);

  committee->quorum_threshold = quorum_threshold;

  return CARDANO_SUCCESS;
}

cardano_unit_interval_t*
cardano_committee_get_quorum_threshold(cardano_committee_t* committee)
{
  if (committee == NULL)
  {
    return NULL;
  }

  cardano_unit_interval_ref(committee->quorum_threshold);

  return committee->quorum_threshold;
}

cardano_error_t
cardano_committee_members_keys(cardano_committee_t* committee, cardano_credential_set_t** credentials)
{
  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credentials == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_committee_members_map_get_keys(committee->committee_members_map, credentials);
}

cardano_error_t
cardano_committee_add_member(
  cardano_committee_t*  committee,
  cardano_credential_t* credential,
  const uint64_t        epoch)
{
  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_committee_members_map_insert(committee->committee_members_map, credential, epoch);
}

uint64_t
cardano_committee_get_member_epoch(
  cardano_committee_t*  committee,
  cardano_credential_t* credential)
{
  if (committee == NULL)
  {
    return 0;
  }

  if (credential == NULL)
  {
    return 0;
  }

  uint64_t epoch = 0;

  cardano_error_t result = cardano_committee_members_map_get(committee->committee_members_map, credential, &epoch);

  if (result != CARDANO_SUCCESS)
  {
    return 0;
  }

  return epoch;
}

cardano_error_t
cardano_committee_get_key_at(
  const cardano_committee_t* committee,
  size_t                     index,
  cardano_credential_t**     credential)
{
  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_committee_members_map_get_key_at(committee->committee_members_map, index, credential);
}

cardano_error_t
cardano_committee_get_value_at(
  const cardano_committee_t* committee,
  size_t                     index,
  uint64_t*                  epoch)
{
  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (epoch == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_committee_members_map_get_value_at(committee->committee_members_map, index, epoch);
}

cardano_error_t
cardano_committee_get_key_value_at(
  const cardano_committee_t* committee,
  size_t                     index,
  cardano_credential_t**     credential,
  uint64_t*                  epoch)
{
  if (committee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (epoch == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_committee_members_map_get_key_value_at(committee->committee_members_map, index, credential, epoch);
}

void
cardano_committee_unref(cardano_committee_t** committee)
{
  if ((committee == NULL) || (*committee == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*committee)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *committee = NULL;
    return;
  }
}

void
cardano_committee_ref(cardano_committee_t* committee)
{
  if (committee == NULL)
  {
    return;
  }

  cardano_object_ref(&committee->base);
}

size_t
cardano_committee_refcount(const cardano_committee_t* committee)
{
  if (committee == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&committee->base);
}

void
cardano_committee_set_last_error(cardano_committee_t* committee, const char* message)
{
  cardano_object_set_last_error(&committee->base, message);
}

const char*
cardano_committee_get_last_error(const cardano_committee_t* committee)
{
  return cardano_object_get_last_error(&committee->base);
}
