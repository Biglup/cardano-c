/**
 * \file pool_voting_thresholds.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_POOL_VOTING_THRESHOLDS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_POOL_VOTING_THRESHOLDS_H

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
 * requirements is the approval of the action by SPOs. These thresholds specify
 * the percentage of the stake held by all stake pools that must be meet by the SPOs who
 * vote Yes for the approval to be successful.
 */
typedef struct cardano_pool_voting_thresholds_t cardano_pool_voting_thresholds_t;

/**
 * \brief Creates and initializes a new instance of the pool voting thresholds.
 *
 * This function allocates and initializes a new instance of the pool voting thresholds,
 * representing various quorum thresholds for different governance actions in the Cardano protocol.
 *
 * \param[in] motion_no_confidence Quorum threshold for a Motion of no-confidence.
 * \param[in] committee_normal Quorum threshold for electing a new committee when the current committee is in a state of confidence.
 * \param[in] committee_no_confidence Quorum threshold for electing a new committee when the current committee is in a state of no-confidence.
 * \param[in] hard_fork_initiation Quorum threshold for initiating a non-backwards compatible upgrade of the network.
 * \param[in] security_relevant_param Quorum threshold for changing security-relevant parameters.
 * \param[out] pool_voting_thresholds On successful initialization, this will point to a newly created
 *                                    pool voting thresholds object. This object represents a "strong reference",
 *                                    meaning that it is fully initialized and ready for use.
 *                                    The caller is responsible for managing the lifecycle of this object.
 *                                    Specifically, once the pool voting thresholds object is no longer needed,
 *                                    the caller must release it by calling \ref cardano_pool_voting_thresholds_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool voting thresholds object was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* motion_no_confidence = NULL;
 * cardano_unit_interval_t* committee_normal = NULL;
 * cardano_unit_interval_t* committee_no_confidence = NULL;
 * cardano_unit_interval_t* hard_fork_initiation = NULL;
 * cardano_unit_interval_t* security_relevant_param = NULL;
 *
 * // Initialize the unit intervals
 * cardano_unit_interval_from_double(0.5, &motion_no_confidence);
 * cardano_unit_interval_from_double(0.6, &committee_normal);
 * cardano_unit_interval_from_double(0.7, &committee_no_confidence);
 * cardano_unit_interval_from_double(0.8, &hard_fork_initiation);
 * cardano_unit_interval_from_double(0.9, &security_relevant_param);
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_new(
 *   motion_no_confidence,
 *   committee_normal,
 *   committee_no_confidence,
 *   hard_fork_initiation,
 *   security_relevant_param,
 *   &pool_voting_thresholds
 * );
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_voting_thresholds
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&motion_no_confidence);
 * cardano_unit_interval_unref(&committee_normal);
 * cardano_unit_interval_unref(&committee_no_confidence);
 * cardano_unit_interval_unref(&hard_fork_initiation);
 * cardano_unit_interval_unref(&security_relevant_param);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_voting_thresholds_new(
  cardano_unit_interval_t*           motion_no_confidence,
  cardano_unit_interval_t*           committee_normal,
  cardano_unit_interval_t*           committee_no_confidence,
  cardano_unit_interval_t*           hard_fork_initiation,
  cardano_unit_interval_t*           security_relevant_param,
  cardano_pool_voting_thresholds_t** pool_voting_thresholds);

/**
 * \brief Creates a \ref cardano_pool_voting_thresholds_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_pool_voting_thresholds_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a pool_voting_thresholds.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] pool_voting_thresholds A pointer to a pointer of \ref cardano_pool_voting_thresholds_t that will be set to the address
 *                        of the newly created pool_voting_thresholds object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_pool_voting_thresholds_t object by calling
 *       \ref cardano_pool_voting_thresholds_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_voting_thresholds
 *
 *   // Once done, ensure to clean up and release the pool_voting_thresholds
 *   cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode pool_voting_thresholds: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_voting_thresholds_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_voting_thresholds_t** pool_voting_thresholds);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_pool_voting_thresholds_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] pool_voting_thresholds A constant pointer to the \ref cardano_pool_voting_thresholds_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p pool_voting_thresholds or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_pool_voting_thresholds_to_cbor(pool_voting_thresholds, writer);
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
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_to_cbor(
  const cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_cbor_writer_t*                  writer);

/**
 * \brief Retrieves the motion no-confidence threshold from the pool voting thresholds.
 *
 * This function returns the motion no-confidence threshold from the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[out] motion_no_confidence On successful retrieval, this will point to the motion no-confidence threshold
 *                                  as a \ref cardano_unit_interval_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the motion no-confidence threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* motion_no_confidence = NULL;
 *
 * // Assume pool_voting_thresholds is initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_get_motion_no_confidence(pool_voting_thresholds, &motion_no_confidence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the motion_no_confidence
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&motion_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_get_motion_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         motion_no_confidence);

/**
 * \brief Retrieves the committee normal threshold from the pool voting thresholds.
 *
 * This function returns the committee normal threshold from the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[out] committee_normal On successful retrieval, this will point to the committee normal threshold
 *                              as a \ref cardano_unit_interval_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee normal threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* committee_normal = NULL;
 *
 * // Assume pool_voting_thresholds is initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_get_committee_normal(pool_voting_thresholds, &committee_normal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the committee_normal
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&committee_normal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_get_committee_normal(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         committee_normal);

/**
 * \brief Retrieves the committee no confidence threshold from the pool voting thresholds.
 *
 * This function returns the committee no confidence threshold from the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[out] committee_no_confidence On successful retrieval, this will point to the committee no confidence threshold
 *                                     as a \ref cardano_unit_interval_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee no confidence threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* committee_no_confidence = NULL;
 *
 * // Assume pool_voting_thresholds is initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_get_committee_no_confidence(pool_voting_thresholds, &committee_no_confidence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the committee_no_confidence
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&committee_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_get_committee_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         committee_no_confidence);

/**
 * \brief Retrieves the hard fork initiation threshold from the pool voting thresholds.
 *
 * This function returns the hard fork initiation threshold from the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[out] hard_fork_initiation On successful retrieval, this will point to the hard fork initiation threshold
 *                                  as a \ref cardano_unit_interval_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hard fork initiation threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* hard_fork_initiation = NULL;
 *
 * // Assume pool_voting_thresholds is initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_get_hard_fork_initiation(pool_voting_thresholds, &hard_fork_initiation);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the hard_fork_initiation
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&hard_fork_initiation);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_get_hard_fork_initiation(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         hard_fork_initiation);

/**
 * \brief Retrieves the security relevant parameter threshold from the pool voting thresholds.
 *
 * This function returns the security relevant parameter threshold from the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[out] security_relevant_param On successful retrieval, this will point to the security relevant parameter threshold
 *                                  as a \ref cardano_unit_interval_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the security relevant parameter threshold was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* security_relevant_param = NULL;
 *
 * // Assume pool_voting_thresholds is initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_get_security_relevant_param(pool_voting_thresholds, &security_relevant_param);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the security_relevant_param
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&security_relevant_param);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_get_security_relevant_param(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t**         security_relevant_param);

/**
 * \brief Sets the motion no-confidence threshold in the pool voting thresholds.
 *
 * This function sets the motion no-confidence threshold in the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[in] motion_no_confidence Pointer to the \ref cardano_unit_interval_t object representing the motion no-confidence threshold.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the motion no-confidence threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* motion_no_confidence = NULL;
 *
 * // Assume pool_voting_thresholds and motion_no_confidence are initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_set_motion_no_confidence(pool_voting_thresholds, motion_no_confidence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The motion no-confidence threshold was successfully set
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&motion_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_set_motion_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          motion_no_confidence);

/**
 * \brief Sets the committee normal threshold in the pool voting thresholds.
 *
 * This function sets the committee normal threshold in the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[in] committee_normal Pointer to the \ref cardano_unit_interval_t object representing the committee normal threshold.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee normal threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* committee_normal = NULL;
 *
 * // Assume pool_voting_thresholds and committee_normal are initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_set_committee_normal(pool_voting_thresholds, committee_normal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The committee normal threshold was successfully set
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&committee_normal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_set_committee_normal(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          committee_normal);

/**
 * \brief Sets the committee no-confidence threshold in the pool voting thresholds.
 *
 * This function sets the committee no-confidence threshold in the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[in] committee_no_confidence Pointer to the \ref cardano_unit_interval_t object representing the committee no-confidence threshold.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee no-confidence threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* committee_no_confidence = NULL;
 *
 * // Assume pool_voting_thresholds and committee_no_confidence are initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_set_committee_no_confidence(pool_voting_thresholds, committee_no_confidence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The committee no-confidence threshold was successfully set
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&committee_no_confidence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_set_committee_no_confidence(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          committee_no_confidence);

/**
 * \brief Sets the hard fork initiation threshold in the pool voting thresholds.
 *
 * This function sets the hard fork initiation threshold in the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[in] hard_fork_initiation Pointer to the \ref cardano_unit_interval_t object representing the hard fork initiation threshold.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hard fork initiation threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* hard_fork_initiation = NULL;
 *
 * // Assume pool_voting_thresholds and hard_fork_initiation are initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_set_hard_fork_initiation(pool_voting_thresholds, hard_fork_initiation);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The hard fork initiation threshold was successfully set
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&hard_fork_initiation);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_set_hard_fork_initiation(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          hard_fork_initiation);

/**
 * \brief Sets the security relevant param threshold in the pool voting thresholds.
 *
 * This function sets the security relevant param threshold in the pool voting thresholds object.
 *
 * \param[in] pool_voting_thresholds Pointer to the pool voting thresholds object.
 * \param[in] security_relevant_param Pointer to the \ref cardano_unit_interval_t object representing the security relevant param threshold.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the security relevant param threshold was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 * cardano_unit_interval_t* security_relevant_param = NULL;
 *
 * // Assume pool_voting_thresholds and hard_fork_initiation are initialized properly
 *
 * cardano_error_t result = cardano_pool_voting_thresholds_set_security_relevant_param(pool_voting_thresholds, security_relevant_param);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The security relevant param threshold was successfully set
 * }
 *
 * // Clean up
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * cardano_unit_interval_unref(&security_relevant_param);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_voting_thresholds_set_security_relevant_param(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  cardano_unit_interval_t*          security_relevant_param);

/**
 * \brief Decrements the reference count of a cardano_pool_voting_thresholds_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_pool_voting_thresholds_t object
 * by decreasing its reference count. When the reference count reaches zero, the pool_voting_thresholds is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] pool_voting_thresholds A pointer to the pointer of the pool_voting_thresholds object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = cardano_pool_voting_thresholds_new(major, minor);
 *
 * // Perform operations with the pool_voting_thresholds...
 *
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * // At this point, pool_voting_thresholds is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_pool_voting_thresholds_unref, the pointer to the \ref cardano_pool_voting_thresholds_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_pool_voting_thresholds_unref(cardano_pool_voting_thresholds_t** pool_voting_thresholds);

/**
 * \brief Increases the reference count of the cardano_pool_voting_thresholds_t object.
 *
 * This function is used to manually increment the reference count of an cardano_pool_voting_thresholds_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_pool_voting_thresholds_unref.
 *
 * \param pool_voting_thresholds A pointer to the cardano_pool_voting_thresholds_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_voting_thresholds is a previously created pool_voting_thresholds object
 *
 * cardano_pool_voting_thresholds_ref(pool_voting_thresholds);
 *
 * // Now pool_voting_thresholds can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_pool_voting_thresholds_ref there is a corresponding
 * call to \ref cardano_pool_voting_thresholds_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_pool_voting_thresholds_ref(cardano_pool_voting_thresholds_t* pool_voting_thresholds);

/**
 * \brief Retrieves the current reference count of the cardano_pool_voting_thresholds_t object.
 *
 * This function returns the number of active references to an cardano_pool_voting_thresholds_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_pool_voting_thresholds_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param pool_voting_thresholds A pointer to the cardano_pool_voting_thresholds_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_pool_voting_thresholds_t object. If the object
 * is properly managed (i.e., every \ref cardano_pool_voting_thresholds_ref call is matched with a
 * \ref cardano_pool_voting_thresholds_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_voting_thresholds is a previously created pool_voting_thresholds object
 *
 * size_t ref_count = cardano_pool_voting_thresholds_refcount(pool_voting_thresholds);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_voting_thresholds_refcount(const cardano_pool_voting_thresholds_t* pool_voting_thresholds);

/**
 * \brief Sets the last error message for a given cardano_pool_voting_thresholds_t object.
 *
 * Records an error message in the pool_voting_thresholds's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] pool_voting_thresholds A pointer to the \ref cardano_pool_voting_thresholds_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the pool_voting_thresholds's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_pool_voting_thresholds_set_last_error(
  cardano_pool_voting_thresholds_t* pool_voting_thresholds,
  const char*                       message);

/**
 * \brief Retrieves the last error message recorded for a specific pool_voting_thresholds.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_pool_voting_thresholds_set_last_error for the given
 * pool_voting_thresholds. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] pool_voting_thresholds A pointer to the \ref cardano_pool_voting_thresholds_t instance whose last error
 *                   message is to be retrieved. If the pool_voting_thresholds is NULL, the function
 *                   returns a generic error message indicating the null pool_voting_thresholds.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified pool_voting_thresholds. If the pool_voting_thresholds is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_pool_voting_thresholds_set_last_error for the same pool_voting_thresholds, or until
 *       the pool_voting_thresholds is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pool_voting_thresholds_get_last_error(
  const cardano_pool_voting_thresholds_t* pool_voting_thresholds);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_POOL_VOTING_THRESHOLDS_H