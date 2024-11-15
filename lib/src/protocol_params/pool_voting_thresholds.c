/**
 * \file pool_voting_thresholds.c
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
#include <cardano/protocol_params/pool_voting_thresholds.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Governance actions are ratified through on-chain voting. Different
 * kinds of governance actions have different ratification requirements. One of those
 * requirements is the approval of the action by SPOs. These thresholds specify
 * the percentage of the stake held by all stake pools that must be meet by the SPOs who
 * vote Yes for the approval to be successful.
 */
typedef struct cardano_pool_voting_thresholds_t
{
    cardano_object_t         base;
    cardano_unit_interval_t* motion_no_confidence;
    cardano_unit_interval_t* committee_normal;
    cardano_unit_interval_t* committee_no_confidence;
    cardano_unit_interval_t* hard_fork_initiation;
    cardano_unit_interval_t* security_relevant_param;
} cardano_pool_voting_thresholds_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a pool voting thresholds object.
 *
 * This function is responsible for properly deallocating a pool voting thresholds object (`cardano_pool_voting_thresholds_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the pool_voting_thresholds object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_pool_voting_thresholds_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the pool_voting_thresholds
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_pool_voting_thresholds_deallocate(void* object)
{
  assert(object != NULL);

  cardano_pool_voting_thresholds_t* data = (cardano_pool_voting_thresholds_t*)object;

  cardano_unit_interval_unref(&data->motion_no_confidence);
  cardano_unit_interval_unref(&data->committee_normal);
  cardano_unit_interval_unref(&data->committee_no_confidence);
  cardano_unit_interval_unref(&data->hard_fork_initiation);
  cardano_unit_interval_unref(&data->security_relevant_param);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_pool_voting_thresholds_new(
  cardano_unit_interval_t*           motion_no_confidence,
  cardano_unit_interval_t*           committee_normal,
  cardano_unit_interval_t*           committee_no_confidence,
  cardano_unit_interval_t*           hard_fork_initiation,
  cardano_unit_interval_t*           security_relevant_param,
  cardano_pool_voting_thresholds_t** pool_voting_thresholds)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (motion_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_normal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (security_relevant_param == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *pool_voting_thresholds = _cardano_malloc(sizeof(cardano_pool_voting_thresholds_t));

  if (*pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*pool_voting_thresholds)->base.deallocator   = cardano_pool_voting_thresholds_deallocate;
  (*pool_voting_thresholds)->base.ref_count     = 1;
  (*pool_voting_thresholds)->base.last_error[0] = '\0';

  cardano_unit_interval_ref(motion_no_confidence);
  (*pool_voting_thresholds)->motion_no_confidence = motion_no_confidence;

  cardano_unit_interval_ref(committee_normal);
  (*pool_voting_thresholds)->committee_normal = committee_normal;

  cardano_unit_interval_ref(committee_no_confidence);
  (*pool_voting_thresholds)->committee_no_confidence = committee_no_confidence;

  cardano_unit_interval_ref(hard_fork_initiation);
  (*pool_voting_thresholds)->hard_fork_initiation = hard_fork_initiation;

  cardano_unit_interval_ref(security_relevant_param);
  (*pool_voting_thresholds)->security_relevant_param = security_relevant_param;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_voting_thresholds_t** pool_voting_thresholds)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *pool_voting_thresholds = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "pool_voting_thresholds";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, 5);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *pool_voting_thresholds = NULL;

    return expect_array_result;
  }

  cardano_unit_interval_t* motion_no_confidence = NULL;

  const cardano_error_t motion_no_confidence_result = cardano_unit_interval_from_cbor(reader, &motion_no_confidence);

  if (motion_no_confidence_result != CARDANO_SUCCESS)
  {
    *pool_voting_thresholds = NULL;

    return motion_no_confidence_result;
  }

  cardano_unit_interval_t* committee_normal = NULL;

  const cardano_error_t committee_normal_result = cardano_unit_interval_from_cbor(reader, &committee_normal);

  if (committee_normal_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    *pool_voting_thresholds = NULL;

    return committee_normal_result;
  }

  cardano_unit_interval_t* committee_no_confidence = NULL;

  const cardano_error_t committee_no_confidence_result = cardano_unit_interval_from_cbor(reader, &committee_no_confidence);

  if (committee_no_confidence_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    *pool_voting_thresholds = NULL;

    return committee_no_confidence_result;
  }

  cardano_unit_interval_t* hard_fork_initiation = NULL;

  const cardano_error_t hard_fork_initiation_result = cardano_unit_interval_from_cbor(reader, &hard_fork_initiation);

  if (hard_fork_initiation_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    *pool_voting_thresholds = NULL;

    return hard_fork_initiation_result;
  }

  cardano_unit_interval_t* security_relevant_param = NULL;

  const cardano_error_t security_relevant_param_result = cardano_unit_interval_from_cbor(reader, &security_relevant_param);

  if (security_relevant_param_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&hard_fork_initiation);
    *pool_voting_thresholds = NULL;

    return security_relevant_param_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&hard_fork_initiation);
    cardano_unit_interval_unref(&security_relevant_param);
    *pool_voting_thresholds = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t result = cardano_pool_voting_thresholds_new(
    motion_no_confidence,
    committee_normal,
    committee_no_confidence,
    hard_fork_initiation,
    security_relevant_param,
    pool_voting_thresholds);

  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&security_relevant_param);

  return result;
}

cardano_error_t
cardano_pool_voting_thresholds_to_cbor(const cardano_pool_voting_thresholds_t* pool_voting_thresholds, cardano_cbor_writer_t* writer)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    5);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t motion_no_confidence_result = cardano_unit_interval_to_cbor(pool_voting_thresholds->motion_no_confidence, writer);

  if (motion_no_confidence_result != CARDANO_SUCCESS)
  {
    return motion_no_confidence_result;
  }

  cardano_error_t committee_normal_result = cardano_unit_interval_to_cbor(pool_voting_thresholds->committee_normal, writer);

  if (committee_normal_result != CARDANO_SUCCESS)
  {
    return committee_normal_result;
  }

  cardano_error_t committee_no_confidence_result = cardano_unit_interval_to_cbor(pool_voting_thresholds->committee_no_confidence, writer);

  if (committee_no_confidence_result != CARDANO_SUCCESS)
  {
    return committee_no_confidence_result;
  }

  cardano_error_t hard_fork_initiation_result = cardano_unit_interval_to_cbor(pool_voting_thresholds->hard_fork_initiation, writer);

  if (hard_fork_initiation_result != CARDANO_SUCCESS)
  {
    return hard_fork_initiation_result;
  }

  return cardano_unit_interval_to_cbor(pool_voting_thresholds->security_relevant_param, writer);
}

cardano_error_t
cardano_pool_voting_thresholds_get_motion_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         motion_no_confidence)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (motion_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(pool_voting_thresholds->motion_no_confidence);
  *motion_no_confidence = pool_voting_thresholds->motion_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_get_committee_normal(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         committee_normal)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_normal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(pool_voting_thresholds->committee_normal);
  *committee_normal = pool_voting_thresholds->committee_normal;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_get_committee_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         committee_no_confidence)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(pool_voting_thresholds->committee_no_confidence);
  *committee_no_confidence = pool_voting_thresholds->committee_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_get_hard_fork_initiation(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         hard_fork_initiation)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(pool_voting_thresholds->hard_fork_initiation);
  *hard_fork_initiation = pool_voting_thresholds->hard_fork_initiation;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_get_security_relevant_param(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         security_relevant_param)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (security_relevant_param == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(pool_voting_thresholds->security_relevant_param);
  *security_relevant_param = pool_voting_thresholds->security_relevant_param;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_set_motion_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          motion_no_confidence)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (motion_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&pool_voting_thresholds->motion_no_confidence);
  cardano_unit_interval_ref(motion_no_confidence);
  pool_voting_thresholds->motion_no_confidence = motion_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_set_committee_normal(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          committee_normal)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_normal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&pool_voting_thresholds->committee_normal);
  cardano_unit_interval_ref(committee_normal);
  pool_voting_thresholds->committee_normal = committee_normal;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_set_committee_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          committee_no_confidence)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&pool_voting_thresholds->committee_no_confidence);
  cardano_unit_interval_ref(committee_no_confidence);
  pool_voting_thresholds->committee_no_confidence = committee_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_set_hard_fork_initiation(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          hard_fork_initiation)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&pool_voting_thresholds->hard_fork_initiation);
  cardano_unit_interval_ref(hard_fork_initiation);
  pool_voting_thresholds->hard_fork_initiation = hard_fork_initiation;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_pool_voting_thresholds_set_security_relevant_param(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          security_relevant_param)
{
  if (pool_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (security_relevant_param == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&pool_voting_thresholds->security_relevant_param);
  cardano_unit_interval_ref(security_relevant_param);
  pool_voting_thresholds->security_relevant_param = security_relevant_param;

  return CARDANO_SUCCESS;
}

void
cardano_pool_voting_thresholds_unref(cardano_pool_voting_thresholds_t** pool_voting_thresholds)
{
  if ((pool_voting_thresholds == NULL) || (*pool_voting_thresholds == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*pool_voting_thresholds)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *pool_voting_thresholds = NULL;
    return;
  }
}

void
cardano_pool_voting_thresholds_ref(cardano_pool_voting_thresholds_t* pool_voting_thresholds)
{
  if (pool_voting_thresholds == NULL)
  {
    return;
  }

  cardano_object_ref(&pool_voting_thresholds->base);
}

size_t
cardano_pool_voting_thresholds_refcount(const cardano_pool_voting_thresholds_t* pool_voting_thresholds)
{
  if (pool_voting_thresholds == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&pool_voting_thresholds->base);
}

void
cardano_pool_voting_thresholds_set_last_error(cardano_pool_voting_thresholds_t* pool_voting_thresholds, const char* message)
{
  cardano_object_set_last_error(&pool_voting_thresholds->base, message);
}

const char*
cardano_pool_voting_thresholds_get_last_error(const cardano_pool_voting_thresholds_t* pool_voting_thresholds)
{
  return cardano_object_get_last_error(&pool_voting_thresholds->base);
}