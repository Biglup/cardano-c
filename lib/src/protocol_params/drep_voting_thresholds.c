/**
 * \file drep_voting_thresholds.c
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
#include <cardano/protocol_params/drep_voting_thresholds.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Governance actions are ratified through on-chain voting. Different
 * kinds of governance actions have different ratification requirements. One of those
 * requirements is the approval of the action by DReps. These thresholds specify
 * the percentage of the total active voting stake that must be meet by the DReps who vote Yes
 * for the approval to be successful.
 */
typedef struct cardano_drep_voting_thresholds_t
{
    cardano_object_t         base;
    cardano_unit_interval_t* motion_no_confidence;
    cardano_unit_interval_t* committee_normal;
    cardano_unit_interval_t* committee_no_confidence;
    cardano_unit_interval_t* update_constitution;
    cardano_unit_interval_t* hard_fork_initiation;
    cardano_unit_interval_t* pp_network_group;
    cardano_unit_interval_t* pp_economic_group;
    cardano_unit_interval_t* pp_technical_group;
    cardano_unit_interval_t* pp_governance_group;
    cardano_unit_interval_t* treasury_withdrawal;
} cardano_drep_voting_thresholds_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a drep voting thresholds object.
 *
 * This function is responsible for properly deallocating a drep voting thresholds object (`cardano_drep_voting_thresholds_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the drep_voting_thresholds object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_drep_voting_thresholds_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the drep_voting_thresholds
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_drep_voting_thresholds_deallocate(void* object)
{
  assert(object != NULL);

  cardano_drep_voting_thresholds_t* data = (cardano_drep_voting_thresholds_t*)object;

  cardano_unit_interval_unref(&data->motion_no_confidence);
  cardano_unit_interval_unref(&data->committee_normal);
  cardano_unit_interval_unref(&data->committee_no_confidence);
  cardano_unit_interval_unref(&data->update_constitution);
  cardano_unit_interval_unref(&data->hard_fork_initiation);
  cardano_unit_interval_unref(&data->pp_network_group);
  cardano_unit_interval_unref(&data->pp_economic_group);
  cardano_unit_interval_unref(&data->pp_technical_group);
  cardano_unit_interval_unref(&data->pp_governance_group);
  cardano_unit_interval_unref(&data->treasury_withdrawal);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_drep_voting_thresholds_new(
  cardano_unit_interval_t*           motion_no_confidence,
  cardano_unit_interval_t*           committee_normal,
  cardano_unit_interval_t*           committee_no_confidence,
  cardano_unit_interval_t*           update_constitution,
  cardano_unit_interval_t*           hard_fork_initiation,
  cardano_unit_interval_t*           pp_network_group,
  cardano_unit_interval_t*           pp_economic_group,
  cardano_unit_interval_t*           pp_technical_group,
  cardano_unit_interval_t*           pp_governance_group,
  cardano_unit_interval_t*           treasury_withdrawal,
  cardano_drep_voting_thresholds_t** drep_voting_thresholds)
{
  if (drep_voting_thresholds == NULL)
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

  if (update_constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_network_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_economic_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_technical_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_governance_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_withdrawal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *drep_voting_thresholds = _cardano_malloc(sizeof(cardano_drep_voting_thresholds_t));

  if (*drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*drep_voting_thresholds)->base.deallocator   = cardano_drep_voting_thresholds_deallocate;
  (*drep_voting_thresholds)->base.ref_count     = 1;
  (*drep_voting_thresholds)->base.last_error[0] = '\0';

  cardano_unit_interval_ref(motion_no_confidence);
  (*drep_voting_thresholds)->motion_no_confidence = motion_no_confidence;

  cardano_unit_interval_ref(committee_normal);
  (*drep_voting_thresholds)->committee_normal = committee_normal;

  cardano_unit_interval_ref(committee_no_confidence);
  (*drep_voting_thresholds)->committee_no_confidence = committee_no_confidence;

  cardano_unit_interval_ref(update_constitution);
  (*drep_voting_thresholds)->update_constitution = update_constitution;

  cardano_unit_interval_ref(hard_fork_initiation);
  (*drep_voting_thresholds)->hard_fork_initiation = hard_fork_initiation;

  cardano_unit_interval_ref(pp_network_group);
  (*drep_voting_thresholds)->pp_network_group = pp_network_group;

  cardano_unit_interval_ref(pp_economic_group);
  (*drep_voting_thresholds)->pp_economic_group = pp_economic_group;

  cardano_unit_interval_ref(pp_technical_group);
  (*drep_voting_thresholds)->pp_technical_group = pp_technical_group;

  cardano_unit_interval_ref(pp_governance_group);
  (*drep_voting_thresholds)->pp_governance_group = pp_governance_group;

  cardano_unit_interval_ref(treasury_withdrawal);
  (*drep_voting_thresholds)->treasury_withdrawal = treasury_withdrawal;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_from_cbor(cardano_cbor_reader_t* reader, cardano_drep_voting_thresholds_t** drep_voting_thresholds)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *drep_voting_thresholds = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "drep_voting_thresholds";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, 10);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *drep_voting_thresholds = NULL;
    return expect_array_result;
  }

  cardano_unit_interval_t* motion_no_confidence = NULL;

  const cardano_error_t motion_no_confidence_result = cardano_unit_interval_from_cbor(reader, &motion_no_confidence);

  if (motion_no_confidence_result != CARDANO_SUCCESS)
  {
    *drep_voting_thresholds = NULL;
    return motion_no_confidence_result;
  }

  cardano_unit_interval_t* committee_normal = NULL;

  const cardano_error_t committee_normal_result = cardano_unit_interval_from_cbor(reader, &committee_normal);

  if (committee_normal_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    *drep_voting_thresholds = NULL;
    return committee_normal_result;
  }

  cardano_unit_interval_t* committee_no_confidence = NULL;

  const cardano_error_t committee_no_confidence_result = cardano_unit_interval_from_cbor(reader, &committee_no_confidence);

  if (committee_no_confidence_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    *drep_voting_thresholds = NULL;
    return committee_no_confidence_result;
  }

  cardano_unit_interval_t* update_constitution = NULL;

  const cardano_error_t update_constitution_result = cardano_unit_interval_from_cbor(reader, &update_constitution);

  if (update_constitution_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    *drep_voting_thresholds = NULL;
    return update_constitution_result;
  }

  cardano_unit_interval_t* hard_fork_initiation = NULL;

  const cardano_error_t hard_fork_initiation_result = cardano_unit_interval_from_cbor(reader, &hard_fork_initiation);

  if (hard_fork_initiation_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&update_constitution);
    *drep_voting_thresholds = NULL;
    return hard_fork_initiation_result;
  }

  cardano_unit_interval_t* pp_network_group = NULL;

  const cardano_error_t pp_network_group_result = cardano_unit_interval_from_cbor(reader, &pp_network_group);

  if (pp_network_group_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&update_constitution);
    cardano_unit_interval_unref(&hard_fork_initiation);
    *drep_voting_thresholds = NULL;
    return pp_network_group_result;
  }

  cardano_unit_interval_t* pp_economic_group = NULL;

  const cardano_error_t pp_economic_group_result = cardano_unit_interval_from_cbor(reader, &pp_economic_group);

  if (pp_economic_group_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&update_constitution);
    cardano_unit_interval_unref(&hard_fork_initiation);
    cardano_unit_interval_unref(&pp_network_group);
    *drep_voting_thresholds = NULL;
    return pp_economic_group_result;
  }

  cardano_unit_interval_t* pp_technical_group = NULL;

  const cardano_error_t pp_technical_group_result = cardano_unit_interval_from_cbor(reader, &pp_technical_group);

  if (pp_technical_group_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&update_constitution);
    cardano_unit_interval_unref(&hard_fork_initiation);
    cardano_unit_interval_unref(&pp_network_group);
    cardano_unit_interval_unref(&pp_economic_group);
    *drep_voting_thresholds = NULL;
    return pp_technical_group_result;
  }

  cardano_unit_interval_t* pp_governance_group = NULL;

  const cardano_error_t pp_governance_group_result = cardano_unit_interval_from_cbor(reader, &pp_governance_group);

  if (pp_governance_group_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&update_constitution);
    cardano_unit_interval_unref(&hard_fork_initiation);
    cardano_unit_interval_unref(&pp_network_group);
    cardano_unit_interval_unref(&pp_economic_group);
    cardano_unit_interval_unref(&pp_technical_group);
    *drep_voting_thresholds = NULL;
    return pp_governance_group_result;
  }

  cardano_unit_interval_t* treasury_withdrawal = NULL;

  const cardano_error_t treasury_withdrawal_result = cardano_unit_interval_from_cbor(reader, &treasury_withdrawal);

  if (treasury_withdrawal_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&update_constitution);
    cardano_unit_interval_unref(&hard_fork_initiation);
    cardano_unit_interval_unref(&pp_network_group);
    cardano_unit_interval_unref(&pp_economic_group);
    cardano_unit_interval_unref(&pp_technical_group);
    cardano_unit_interval_unref(&pp_governance_group);
    *drep_voting_thresholds = NULL;
    return treasury_withdrawal_result;
  }

  cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&motion_no_confidence);
    cardano_unit_interval_unref(&committee_normal);
    cardano_unit_interval_unref(&committee_no_confidence);
    cardano_unit_interval_unref(&update_constitution);
    cardano_unit_interval_unref(&hard_fork_initiation);
    cardano_unit_interval_unref(&pp_network_group);
    cardano_unit_interval_unref(&pp_economic_group);
    cardano_unit_interval_unref(&pp_technical_group);
    cardano_unit_interval_unref(&pp_governance_group);
    cardano_unit_interval_unref(&treasury_withdrawal);
    *drep_voting_thresholds = NULL;
    return expect_end_array_result;
  }

  const cardano_error_t result = cardano_drep_voting_thresholds_new(
    motion_no_confidence,
    committee_normal,
    committee_no_confidence,
    update_constitution,
    hard_fork_initiation,
    pp_network_group,
    pp_economic_group,
    pp_technical_group,
    pp_governance_group,
    treasury_withdrawal,
    drep_voting_thresholds);

  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&update_constitution);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&pp_network_group);
  cardano_unit_interval_unref(&pp_economic_group);
  cardano_unit_interval_unref(&pp_technical_group);
  cardano_unit_interval_unref(&pp_governance_group);
  cardano_unit_interval_unref(&treasury_withdrawal);

  return result;
}

cardano_error_t
cardano_drep_voting_thresholds_to_cbor(const cardano_drep_voting_thresholds_t* drep_voting_thresholds, cardano_cbor_writer_t* writer)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    10);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t motion_no_confidence_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->motion_no_confidence, writer);

  if (motion_no_confidence_result != CARDANO_SUCCESS)
  {
    return motion_no_confidence_result;
  }

  cardano_error_t committee_normal_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->committee_normal, writer);

  if (committee_normal_result != CARDANO_SUCCESS)
  {
    return committee_normal_result;
  }

  cardano_error_t committee_no_confidence_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->committee_no_confidence, writer);

  if (committee_no_confidence_result != CARDANO_SUCCESS)
  {
    return committee_no_confidence_result;
  }

  cardano_error_t update_constitution_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->update_constitution, writer);

  if (update_constitution_result != CARDANO_SUCCESS)
  {
    return update_constitution_result;
  }

  cardano_error_t hard_fork_initiation_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->hard_fork_initiation, writer);

  if (hard_fork_initiation_result != CARDANO_SUCCESS)
  {
    return hard_fork_initiation_result;
  }

  cardano_error_t pp_network_group_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->pp_network_group, writer);

  if (pp_network_group_result != CARDANO_SUCCESS)
  {
    return pp_network_group_result;
  }

  cardano_error_t pp_economic_group_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->pp_economic_group, writer);

  if (pp_economic_group_result != CARDANO_SUCCESS)
  {
    return pp_economic_group_result;
  }

  cardano_error_t pp_technical_group_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->pp_technical_group, writer);

  if (pp_technical_group_result != CARDANO_SUCCESS)
  {
    return pp_technical_group_result;
  }

  cardano_error_t pp_governance_group_result = cardano_unit_interval_to_cbor(drep_voting_thresholds->pp_governance_group, writer);

  if (pp_governance_group_result != CARDANO_SUCCESS)
  {
    return pp_governance_group_result;
  }

  return cardano_unit_interval_to_cbor(drep_voting_thresholds->treasury_withdrawal, writer);
}

cardano_error_t
cardano_drep_voting_thresholds_get_motion_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         motion_no_confidence)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (motion_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->motion_no_confidence);
  *motion_no_confidence = drep_voting_thresholds->motion_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_committee_normal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         committee_normal)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_normal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->committee_normal);
  *committee_normal = drep_voting_thresholds->committee_normal;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_committee_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         committee_no_confidence)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->committee_no_confidence);
  *committee_no_confidence = drep_voting_thresholds->committee_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_update_constitution(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         update_constitution)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->update_constitution);
  *update_constitution = drep_voting_thresholds->update_constitution;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_hard_fork_initiation(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         hard_fork_initiation)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->hard_fork_initiation);
  *hard_fork_initiation = drep_voting_thresholds->hard_fork_initiation;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_pp_network_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_network_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_network_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->pp_network_group);
  *pp_network_group = drep_voting_thresholds->pp_network_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_pp_economic_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_economic_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_economic_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->pp_economic_group);
  *pp_economic_group = drep_voting_thresholds->pp_economic_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_pp_technical_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_technical_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_technical_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->pp_technical_group);
  *pp_technical_group = drep_voting_thresholds->pp_technical_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_pp_governance_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_governance_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_governance_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->pp_governance_group);
  *pp_governance_group = drep_voting_thresholds->pp_governance_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_get_treasury_withdrawal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         treasury_withdrawal)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_withdrawal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_ref(drep_voting_thresholds->treasury_withdrawal);
  *treasury_withdrawal = drep_voting_thresholds->treasury_withdrawal;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_motion_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          motion_no_confidence)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (motion_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->motion_no_confidence);
  cardano_unit_interval_ref(motion_no_confidence);
  drep_voting_thresholds->motion_no_confidence = motion_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_committee_normal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          committee_normal)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_normal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->committee_normal);
  cardano_unit_interval_ref(committee_normal);
  drep_voting_thresholds->committee_normal = committee_normal;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_committee_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          committee_no_confidence)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (committee_no_confidence == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->committee_no_confidence);
  cardano_unit_interval_ref(committee_no_confidence);
  drep_voting_thresholds->committee_no_confidence = committee_no_confidence;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_update_constitution(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          update_constitution)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->update_constitution);
  cardano_unit_interval_ref(update_constitution);
  drep_voting_thresholds->update_constitution = update_constitution;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_hard_fork_initiation(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          hard_fork_initiation)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hard_fork_initiation == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->hard_fork_initiation);
  cardano_unit_interval_ref(hard_fork_initiation);
  drep_voting_thresholds->hard_fork_initiation = hard_fork_initiation;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_pp_network_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_network_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_network_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->pp_network_group);
  cardano_unit_interval_ref(pp_network_group);
  drep_voting_thresholds->pp_network_group = pp_network_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_pp_economic_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_economic_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_economic_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->pp_economic_group);
  cardano_unit_interval_ref(pp_economic_group);
  drep_voting_thresholds->pp_economic_group = pp_economic_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_pp_technical_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_technical_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_technical_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->pp_technical_group);
  cardano_unit_interval_ref(pp_technical_group);
  drep_voting_thresholds->pp_technical_group = pp_technical_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_pp_governance_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_governance_group)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (pp_governance_group == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->pp_governance_group);
  cardano_unit_interval_ref(pp_governance_group);
  drep_voting_thresholds->pp_governance_group = pp_governance_group;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_voting_thresholds_set_treasury_withdrawal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          treasury_withdrawal)
{
  if (drep_voting_thresholds == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (treasury_withdrawal == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&drep_voting_thresholds->treasury_withdrawal);
  cardano_unit_interval_ref(treasury_withdrawal);
  drep_voting_thresholds->treasury_withdrawal = treasury_withdrawal;

  return CARDANO_SUCCESS;
}

void
cardano_drep_voting_thresholds_unref(cardano_drep_voting_thresholds_t** drep_voting_thresholds)
{
  if ((drep_voting_thresholds == NULL) || (*drep_voting_thresholds == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*drep_voting_thresholds)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *drep_voting_thresholds = NULL;
    return;
  }
}

void
cardano_drep_voting_thresholds_ref(cardano_drep_voting_thresholds_t* drep_voting_thresholds)
{
  if (drep_voting_thresholds == NULL)
  {
    return;
  }

  cardano_object_ref(&drep_voting_thresholds->base);
}

size_t
cardano_drep_voting_thresholds_refcount(const cardano_drep_voting_thresholds_t* drep_voting_thresholds)
{
  if (drep_voting_thresholds == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&drep_voting_thresholds->base);
}

void
cardano_drep_voting_thresholds_set_last_error(cardano_drep_voting_thresholds_t* drep_voting_thresholds, const char* message)
{
  cardano_object_set_last_error(&drep_voting_thresholds->base, message);
}

const char*
cardano_drep_voting_thresholds_get_last_error(const cardano_drep_voting_thresholds_t* drep_voting_thresholds)
{
  return cardano_object_get_last_error(&drep_voting_thresholds->base);
}