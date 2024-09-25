/**
 * \file drep_voting_thresholds.h
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_DREP_VOTING_THRESHOLDS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_DREP_VOTING_THRESHOLDS_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Governance actions are ratified through on-chain voting. Different
 * kinds of governance actions have different ratification requirements. One of those
 * requirements is the approval of the action by DReps. These thresholds specify
 * the percentage of the total active voting stake that must be meet by the DReps who vote Yes
 * for the approval to be successful.
 */
typedef struct cardano_drep_voting_thresholds_t cardano_drep_voting_thresholds_t;

/**
 * \brief Creates and initializes a new instance of the DRep Voting Thresholds.
 *
 * This function allocates and initializes a new instance of the DRep Voting Thresholds,
 * representing various quorum thresholds required for different governance actions.
 *
 * \param[in] motion_no_confidence The quorum threshold (percentage of the total active voting stake) that
 *                                 needs to be met for a Motion of no-confidence to be enacted.
 * \param[in] committee_normal The quorum threshold (percentage of the total active voting stake) that
 *                             needs to be met for a new committee to be elected if the constitutional committee
 *                             is in a state of confidence.
 * \param[in] committee_no_confidence The quorum threshold (percentage of the total active voting stake) that
 *                                    needs to be met for a new committee to be elected if the constitutional committee
 *                                    is in a state of no-confidence.
 * \param[in] update_constitution The quorum threshold (percentage of the total active voting stake) that
 *                                needs to be met for a modification to the Constitution to be enacted.
 * \param[in] hard_fork_initiation The quorum threshold (percentage of the total active voting stake) that
 *                                 needs to be met to trigger a non-backwards compatible upgrade of the network
 *                                 (requires a prior software upgrade).
 * \param[in] pp_network_group The quorum threshold (percentage of the total active voting stake) that
 *                             needs to be met to update the protocol parameters in the network group.
 * \param[in] pp_economic_group The quorum threshold (percentage of the total active voting stake) that
 *                              needs to be met to update the protocol parameters in the economic group.
 * \param[in] pp_technical_group The quorum threshold (percentage of the total active voting stake) that
 *                               needs to be met to update the protocol parameters in the technical group.
 * \param[in] pp_governance_group The quorum threshold (percentage of the total active voting stake) that
 *                                needs to be met to update the protocol parameters in the governance group.
 * \param[in] treasury_withdrawal The quorum threshold (percentage of the total active voting stake) that
 *                                needs to be met to withdraw from the treasury.
 * \param[out] drep_voting_thresholds On successful initialization, this will point to a newly created
 *                                    DRep Voting Thresholds object. This object represents a "strong reference",
 *                                    meaning that it is fully initialized and ready for use.
 *                                    The caller is responsible for managing the lifecycle of this object.
 *                                    Specifically, once the DRep Voting Thresholds is no longer needed, the caller
 *                                    must release it by calling \ref cardano_drep_voting_thresholds_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep Voting Thresholds was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * cardano_unit_interval_t* motion_no_confidence = NULL;
 * cardano_unit_interval_t* committee_normal = NULL;
 * cardano_unit_interval_t* committee_no_confidence = NULL;
 * cardano_unit_interval_t* update_constitution = NULL;
 * cardano_unit_interval_t* hard_fork_initiation = NULL;
 * cardano_unit_interval_t* pp_network_group = NULL;
 * cardano_unit_interval_t* pp_economic_group = NULL;
 * cardano_unit_interval_t* pp_technical_group = NULL;
 * cardano_unit_interval_t* pp_governance_group = NULL;
 * cardano_unit_interval_t* treasury_withdrawal = NULL;
 *
 * // Assume the unit intervals are initialized properly
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_new(
 *     motion_no_confidence,
 *     committee_normal,
 *     committee_no_confidence,
 *     update_constitution,
 *     hard_fork_initiation,
 *     pp_network_group,
 *     pp_economic_group,
 *     pp_technical_group,
 *     pp_governance_group,
 *     treasury_withdrawal,
 *     &drep_voting_thresholds);
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&motion_no_confidence);
 * cardano_unit_interval_unref(&committee_normal);
 * cardano_unit_interval_unref(&committee_no_confidence);
 * cardano_unit_interval_unref(&update_constitution);
 * cardano_unit_interval_unref(&hard_fork_initiation);
 * cardano_unit_interval_unref(&pp_network_group);
 * cardano_unit_interval_unref(&pp_economic_group);
 * cardano_unit_interval_unref(&pp_technical_group);
 * cardano_unit_interval_unref(&pp_governance_group);
 * cardano_unit_interval_unref(&treasury_withdrawal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
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
  cardano_drep_voting_thresholds_t** drep_voting_thresholds);

/**
 * \brief Creates a \ref cardano_drep_voting_thresholds_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_drep_voting_thresholds_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a drep_voting_thresholds.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] drep_voting_thresholds A pointer to a pointer of \ref cardano_drep_voting_thresholds_t that will be set to the address
 *                        of the newly created drep_voting_thresholds object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_drep_voting_thresholds_t object by calling
 *       \ref cardano_drep_voting_thresholds_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the drep_voting_thresholds
 *
 *   // Once done, ensure to clean up and release the drep_voting_thresholds
 *   cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode drep_voting_thresholds: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_drep_voting_thresholds_from_cbor(cardano_cbor_reader_t* reader, cardano_drep_voting_thresholds_t** drep_voting_thresholds);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_drep_voting_thresholds_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] drep_voting_thresholds A constant pointer to the \ref cardano_drep_voting_thresholds_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p drep_voting_thresholds or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_drep_voting_thresholds_to_cbor(drep_voting_thresholds, writer);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the writer's buffer containing the serialized data
 *   }
 *   else
 *   {
 *     const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *     printf("Serialization failed: %s\n", error_message);
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 *
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_to_cbor(
  const cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_cbor_writer_t*                  writer);

/**
 * \brief Retrieves the quorum threshold for a motion of no-confidence.
 *
 * This function returns the quorum threshold necessary for a motion of no-confidence to be enacted, which is expressed
 * as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] motion_no_confidence On successful retrieval, this will point to the quorum threshold
 *                                  for a motion of no-confidence.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* motion_no_confidence = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_motion_no_confidence(drep_voting_thresholds, &motion_no_confidence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Motion No Confidence Threshold: %f\n", cardano_unit_interval_to_double(motion_no_confidence));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&motion_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_motion_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         motion_no_confidence);

/**
 * \brief Retrieves the quorum threshold for electing a new committee under normal conditions.
 *
 * This function returns the quorum threshold required for electing a new committee when the constitutional committee
 * is in a state of confidence. The threshold is expressed as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] committee_normal On successful retrieval, this will point to the quorum threshold
 *                              for electing a new committee under normal conditions.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* committee_normal = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_committee_normal(drep_voting_thresholds, &committee_normal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Committee Normal Threshold: %f\n",cardano_unit_interval_to_double(committee_normal));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&committee_normal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_committee_normal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         committee_normal);

/**
 * \brief Retrieves the quorum threshold for electing a new committee under no-confidence conditions.
 *
 * This function returns the quorum threshold required for electing a new committee when the constitutional committee
 * is in a state of no-confidence. The threshold is expressed as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] committee_no_confidence On successful retrieval, this will point to the quorum threshold
 *                                     for electing a new committee under no-confidence conditions.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* committee_no_confidence = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_committee_no_confidence(drep_voting_thresholds, &committee_no_confidence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Committee No-Confidence Threshold: %f\n", cardano_unit_interval_to_double(committee_no_confidence));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&committee_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_committee_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         committee_no_confidence);

/**
 * \brief Retrieves the quorum threshold for constitutional updates.
 *
 * This function returns the quorum threshold required to enact modifications to the Constitution. The threshold
 * is expressed as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] update_constitution On successful retrieval, this will point to the unit interval representing
 *                                 the quorum threshold for constitutional updates.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* update_constitution = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_update_constitution(drep_voting_thresholds, &update_constitution);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Constitutional Update Threshold: %f\n", cardano_unit_interval_to_double(update_constitution));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&update_constitution);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_update_constitution(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         update_constitution);

/**
 * \brief Retrieves the quorum threshold required for initiating a hard fork.
 *
 * This function returns the quorum threshold necessary to initiate a non-backwards compatible upgrade of the network.
 * The threshold is expressed as a percentage of the total active voting stake and requires prior software upgrades.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] hard_fork_initiation On successful retrieval, this will point to the unit interval representing
 *                                  the quorum threshold for initiating a hard fork.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* hard_fork_initiation = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_hard_fork_initiation(drep_voting_thresholds, &hard_fork_initiation);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Hard Fork Initiation Threshold: %f\n", cardano_unit_interval_to_double(hard_fork_initiation));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&hard_fork_initiation);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_hard_fork_initiation(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         hard_fork_initiation);

/**
 * \brief Retrieves the quorum threshold for updating protocol parameters in the network group.
 *
 * This function returns the quorum threshold required for updates to the protocol parameters that affect the network group.
 * The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] pp_network_group On successful retrieval, this will point to the unit interval representing
 *                              the quorum threshold for updating protocol parameters in the network group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* pp_network_group = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_pp_network_group(drep_voting_thresholds, &pp_network_group);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Network Group Update Threshold: %f\n", cardano_unit_interval_to_double(pp_network_group));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_network_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_pp_network_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_network_group);

/**
 * \brief Retrieves the quorum threshold for updating protocol parameters in the economic group.
 *
 * This function returns the quorum threshold required for updates to the protocol parameters that affect the economic group.
 * The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] pp_economic_group On successful retrieval, this will point to the unit interval representing
 *                               the quorum threshold for updating protocol parameters in the economic group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* pp_economic_group = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_pp_economic_group(drep_voting_thresholds, &pp_economic_group);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Economic Group Update Threshold: %f\n", cardano_unit_interval_to_double(pp_economic_group));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_economic_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_pp_economic_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_economic_group);

/**
 * \brief Retrieves the quorum threshold for updating protocol parameters in the technical group.
 *
 * This function returns the quorum threshold required for updates to the protocol parameters that affect the technical group.
 * The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] pp_technical_group On successful retrieval, this will point to the unit interval representing
 *                                the quorum threshold for updating protocol parameters in the technical group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* pp_technical_group = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_pp_technical_group(drep_voting_thresholds, &pp_technical_group);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Technical Group Update Threshold: %f\n", cardano_unit_interval_to_double(pp_technical_group));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_technical_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_pp_technical_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_technical_group);

/**
 * \brief Retrieves the quorum threshold for updating protocol parameters in the governance group.
 *
 * This function returns the quorum threshold required for updates to the protocol parameters that affect the governance group.
 * The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] pp_governance_group On successful retrieval, this will point to the unit interval representing
 *                                 the quorum threshold for updating protocol parameters in the governance group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* pp_governance_group = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_pp_governance_group(drep_voting_thresholds, &pp_governance_group);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance Group Update Threshold: %f\n", cardano_unit_interval_to_double(pp_governance_group));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_governance_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_pp_governance_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         pp_governance_group);

/**
 * \brief Retrieves the quorum threshold for withdrawing from the treasury.
 *
 * This function returns the quorum threshold required for treasury withdrawals. The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[out] treasury_withdrawal On successful retrieval, this will point to the unit interval representing
 *                                 the quorum threshold for treasury withdrawals.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 * // Assume drep_voting_thresholds is initialized properly
 * cardano_unit_interval_t* treasury_withdrawal = NULL;
 *
 * cardano_error_t result = cardano_drep_voting_thresholds_get_treasury_withdrawal(drep_voting_thresholds, &treasury_withdrawal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Treasury Withdrawal Threshold: %f\n", cardano_unit_interval_to_double(treasury_withdrawal));
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&treasury_withdrawal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_get_treasury_withdrawal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t**         treasury_withdrawal);

/**
 * \brief Sets the quorum threshold for a Motion of No Confidence.
 *
 * This function sets the quorum threshold required for a Motion of No Confidence to be enacted. The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] motion_no_confidence Pointer to the unit interval representing the quorum threshold for a Motion of No Confidence.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* motion_no_confidence = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.75;
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &motion_no_confidence);
 *
 * if (init_result == CARDANO_SUCCESS && motion_no_confidence != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_motion_no_confidence(drep_voting_thresholds, motion_no_confidence);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Motion No Confidence Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&motion_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_motion_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          motion_no_confidence);

/**
 * \brief Sets the quorum threshold for a new committee to be elected if the constitutional committee is in a state of confidence.
 *
 * This function sets the quorum threshold required for a new committee to be elected under normal conditions. The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] committee_normal Pointer to the unit interval representing the quorum threshold for normal committee elections.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* committee_normal = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.60; // 60% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &committee_normal);
 *
 * if (init_result == CARDANO_SUCCESS && committee_normal != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_committee_normal(drep_voting_thresholds, committee_normal);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Committee Normal Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&committee_normal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_committee_normal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          committee_normal);

/**
 * \brief Sets the quorum threshold for a new committee to be elected if the constitutional committee is in a state of no-confidence.
 *
 * This function sets the quorum threshold required for a new committee to be elected under no-confidence conditions. The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] committee_no_confidence Pointer to the unit interval representing the quorum threshold for no-confidence committee elections.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* committee_no_confidence = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.75; // 75% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &committee_no_confidence);
 *
 * if (init_result == CARDANO_SUCCESS && committee_no_confidence != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_committee_no_confidence(drep_voting_thresholds, committee_no_confidence);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Committee No Confidence Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&committee_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_committee_no_confidence(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          committee_no_confidence);

/**
 * \brief Sets the quorum threshold for enacting modifications to the Constitution.
 *
 * This function sets the quorum threshold required for enacting modifications to the Constitution. The threshold is defined as a percentage of the total active voting stake.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] update_constitution Pointer to the unit interval representing the quorum threshold for constitutional updates.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* update_constitution = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.60; // 60% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &update_constitution);
 *
 * if (init_result == CARDANO_SUCCESS && update_constitution != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_update_constitution(drep_voting_thresholds, update_constitution);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Update Constitution Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&update_constitution);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_update_constitution(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          update_constitution);

/**
 * \brief Sets the quorum threshold for initiating a hard fork.
 *
 * This function sets the quorum threshold required for initiating a hard fork of the network. The threshold is defined as a percentage of the total active voting stake that must be met to trigger a non-backwards compatible upgrade of the network.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] hard_fork_initiation Pointer to the unit interval representing the quorum threshold for hard fork initiation.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* hard_fork_initiation = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.75; // 75% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &hard_fork_initiation);
 *
 * if (init_result == CARDANO_SUCCESS && hard_fork_initiation != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_hard_fork_initiation(drep_voting_thresholds, hard_fork_initiation);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Hard Fork Initiation Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&hard_fork_initiation);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_hard_fork_initiation(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          hard_fork_initiation);

/**
 * \brief Sets the quorum threshold for updating protocol parameters in the network group.
 *
 * This function sets the quorum threshold required for updating protocol parameters within the network group. The threshold is defined as a percentage of the total active voting stake that must be met to enact changes.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] pp_network_group Pointer to the unit interval representing the quorum threshold for updates in the network group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* pp_network_group = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.60; // 60% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &pp_network_group);
 *
 * if (init_result == CARDANO_SUCCESS && pp_network_group != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_pp_network_group(drep_voting_thresholds, pp_network_group);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Network Group Update Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_network_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_pp_network_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_network_group);

/**
 * \brief Sets the quorum threshold for updating protocol parameters in the economic group.
 *
 * This function sets the quorum threshold required for updating protocol parameters within the economic group. The threshold is defined as a percentage of the total active voting stake that must be met to enact changes.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] pp_economic_group Pointer to the unit interval representing the quorum threshold for updates in the economic group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* pp_economic_group = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.55; // 55% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &pp_economic_group);
 *
 * if (init_result == CARDANO_SUCCESS && pp_economic_group != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_pp_economic_group(drep_voting_thresholds, pp_economic_group);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Economic Group Update Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_economic_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_pp_economic_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_economic_group);

/**
 * \brief Sets the quorum threshold for updating protocol parameters in the technical group.
 *
 * This function sets the quorum threshold required for updating protocol parameters within the technical group. The threshold is defined as a percentage of the total active voting stake that must be met to enact changes.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] pp_technical_group Pointer to the unit interval representing the quorum threshold for updates in the technical group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* pp_technical_group = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.60; // 60% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &pp_technical_group);
 *
 * if (init_result == CARDANO_SUCCESS && pp_technical_group != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_pp_technical_group(drep_voting_thresholds, pp_technical_group);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Technical Group Update Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_technical_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_pp_technical_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_technical_group);

/**
 * \brief Sets the quorum threshold for updating protocol parameters in the governance group.
 *
 * This function sets the quorum threshold required for updating protocol parameters within the governance group. The threshold is defined as a percentage of the total active voting stake that must be met to enact changes.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] pp_governance_group Pointer to the unit interval representing the quorum threshold for updates in the governance group.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* pp_governance_group = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.75; // 75% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &pp_governance_group);
 *
 * if (init_result == CARDANO_SUCCESS && pp_governance_group != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_pp_governance_group(drep_voting_thresholds, pp_governance_group);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Governance Group Update Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&pp_governance_group);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_pp_governance_group(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          pp_governance_group);

/**
 * \brief Sets the quorum threshold for treasury withdrawals.
 *
 * This function sets the quorum threshold required for treasury withdrawals. The threshold is defined as a percentage of the total active voting stake that must be met to approve a withdrawal from the treasury.
 *
 * \param[in] drep_voting_thresholds Pointer to the \ref cardano_drep_voting_thresholds_t object.
 * \param[in] treasury_withdrawal Pointer to the unit interval representing the quorum threshold for treasury withdrawals.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = ...;
 * cardano_unit_interval_t* treasury_withdrawal = NULL;
 *
 * // Example threshold
 * double threshold_value = 0.60; // 60% quorum required
 * cardano_error_t init_result = cardano_unit_interval_from_double(threshold_value, &treasury_withdrawal);
 *
 * if (init_result == CARDANO_SUCCESS && treasury_withdrawal != NULL)
 * {
 *   // Assume drep_voting_thresholds is initialized properly
 *   cardano_error_t result = cardano_drep_voting_thresholds_set_treasury_withdrawal(drep_voting_thresholds, treasury_withdrawal);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Treasury Withdrawal Threshold set successfully.\n");
 *   }
 * }
 *
 * // Clean up
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * cardano_unit_interval_unref(&treasury_withdrawal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_voting_thresholds_set_treasury_withdrawal(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  cardano_unit_interval_t*          treasury_withdrawal);

/**
 * \brief Decrements the reference count of a cardano_drep_voting_thresholds_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_drep_voting_thresholds_t object
 * by decreasing its reference count. When the reference count reaches zero, the drep_voting_thresholds is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] drep_voting_thresholds A pointer to the pointer of the drep_voting_thresholds object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = cardano_drep_voting_thresholds_new(major, minor);
 *
 * // Perform operations with the drep_voting_thresholds...
 *
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * // At this point, drep_voting_thresholds is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_drep_voting_thresholds_unref, the pointer to the \ref cardano_drep_voting_thresholds_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_drep_voting_thresholds_unref(cardano_drep_voting_thresholds_t** drep_voting_thresholds);

/**
 * \brief Increases the reference count of the cardano_drep_voting_thresholds_t object.
 *
 * This function is used to manually increment the reference count of an cardano_drep_voting_thresholds_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_drep_voting_thresholds_unref.
 *
 * \param drep_voting_thresholds A pointer to the cardano_drep_voting_thresholds_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming drep_voting_thresholds is a previously created drep_voting_thresholds object
 *
 * cardano_drep_voting_thresholds_ref(drep_voting_thresholds);
 *
 * // Now drep_voting_thresholds can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_drep_voting_thresholds_ref there is a corresponding
 * call to \ref cardano_drep_voting_thresholds_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_drep_voting_thresholds_ref(cardano_drep_voting_thresholds_t* drep_voting_thresholds);

/**
 * \brief Retrieves the current reference count of the cardano_drep_voting_thresholds_t object.
 *
 * This function returns the number of active references to an cardano_drep_voting_thresholds_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_drep_voting_thresholds_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param drep_voting_thresholds A pointer to the cardano_drep_voting_thresholds_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_drep_voting_thresholds_t object. If the object
 * is properly managed (i.e., every \ref cardano_drep_voting_thresholds_ref call is matched with a
 * \ref cardano_drep_voting_thresholds_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming drep_voting_thresholds is a previously created drep_voting_thresholds object
 *
 * size_t ref_count = cardano_drep_voting_thresholds_refcount(drep_voting_thresholds);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_drep_voting_thresholds_refcount(const cardano_drep_voting_thresholds_t* drep_voting_thresholds);

/**
 * \brief Sets the last error message for a given cardano_drep_voting_thresholds_t object.
 *
 * Records an error message in the drep_voting_thresholds's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] drep_voting_thresholds A pointer to the \ref cardano_drep_voting_thresholds_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the drep_voting_thresholds's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_drep_voting_thresholds_set_last_error(
  cardano_drep_voting_thresholds_t* drep_voting_thresholds,
  const char*                       message);

/**
 * \brief Retrieves the last error message recorded for a specific drep_voting_thresholds.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_drep_voting_thresholds_set_last_error for the given
 * drep_voting_thresholds. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] drep_voting_thresholds A pointer to the \ref cardano_drep_voting_thresholds_t instance whose last error
 *                   message is to be retrieved. If the drep_voting_thresholds is NULL, the function
 *                   returns a generic error message indicating the null drep_voting_thresholds.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified drep_voting_thresholds. If the drep_voting_thresholds is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_drep_voting_thresholds_set_last_error for the same drep_voting_thresholds, or until
 *       the drep_voting_thresholds is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_drep_voting_thresholds_get_last_error(
  const cardano_drep_voting_thresholds_t* drep_voting_thresholds);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_DREP_VOTING_THRESHOLDS_H