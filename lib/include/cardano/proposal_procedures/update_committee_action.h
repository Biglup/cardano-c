/**
 * \file update_committee_action.h
 *
 * \author angel.castillo
 * \date   Aug 14, 2024
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

#ifndef UPDATE_COMMITTEE_ACTION_H
#define UPDATE_COMMITTEE_ACTION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/committee_members_map.h>
#include <cardano/proposal_procedures/credential_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Modifies the composition of the constitutional committee, its signature threshold, or its terms of operation.
 */
typedef struct cardano_update_committee_action_t cardano_update_committee_action_t;

/**
 * \brief Creates and initializes a new instance of the Update Committee Action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_update_committee_action_t object,
 * which represents an action to update the constitutional committee within the Cardano network. This action includes
 * specifying members to be added and removed, as well as updating the quorum threshold.
 *
 * The action requires a governance action ID to reference the most recent enacted action of the
 * same type. You can retrieve this information from the gov-state query:
 *
 * \code{.sh}
 * cardano-cli conway query gov-state | jq .nextRatifyState.nextEnactState.prevGovActionIds
 * \endcode
 *
 * Example output:
 * \code{.json}
 * {
 *   "Committee": {
 *     "govActionIx": 0,
 *     "txId": "6bff8515060c08e9cae4d4e203a4d8b2e876848aae8c4e896acda7202d3ac679"
 *   },
 *   "Constitution": null,
 *   "HardFork": null,
 *   "PParamUpdate": {
 *     "govActionIx": 0,
 *     "txId": "7e199d036f1e8d725ea8aba30c5f8d0d2ab9dbd45c7f54e7d85c92c022673f0f"
 *   }
 * }
 * \endcode
 *
 * \param[in] members_to_be_removed A pointer to a \ref cardano_credential_set_t object representing the committee members to be removed.
 * \param[in] members_to_be_added A pointer to a \ref cardano_committee_members_map_t object representing the committee members to be added.
 * \param[in] new_quorum A pointer to a \ref cardano_unit_interval_t object representing the new quorum threshold for the committee.
 * \param[in] governance_action_id An optional pointer to a \ref cardano_governance_action_id_t object representing the last enacted action
 *                                 of the same type. It can be NULL if no governance action of this type has been enacted.
 * \param[out] update_committee_action On successful initialization, this will point to a newly created
 *             \ref cardano_update_committee_action_t object. This object represents a "strong reference"
 *             to the update committee action, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object;
 *             specifically, once the update committee action is no longer needed, the caller must release it
 *             by calling \ref cardano_update_committee_action_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the update committee action was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_set_t* members_to_be_removed = cardano_credential_set_new(...); // Assume initialized
 * cardano_committee_members_map_t* members_to_be_added = cardano_committee_members_map_new(...); // Assume initialized
 * cardano_unit_interval_t* new_quorum = cardano_unit_interval_new(...); // Assume initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...); // Optionally initialized
 * cardano_update_committee_action_t* update_committee_action = NULL;
 * cardano_error_t result = cardano_update_committee_action_new(members_to_be_removed, members_to_be_added, new_quorum, governance_action_id, &update_committee_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the update committee action
 *   // Free resources when done
 *   cardano_update_committee_action_unref(&update_committee_action);
 * }
 * else
 * {
 *   printf("Failed to create the update committee action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup the inputs
 * cardano_credential_set_unref(&members_to_be_removed);
 * cardano_committee_members_map_unref(&members_to_be_added);
 * cardano_unit_interval_unref(&new_quorum);
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_committee_action_new(
  cardano_credential_set_t*           members_to_be_removed,
  cardano_committee_members_map_t*    members_to_be_added,
  cardano_unit_interval_t*            new_quorum,
  cardano_governance_action_id_t*     governance_action_id,
  cardano_update_committee_action_t** update_committee_action);

/**
 * \brief Creates a \ref cardano_update_committee_action_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_update_committee_action_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a update_committee_action.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] update_committee_action A pointer to a pointer of \ref cardano_update_committee_action_t that will be set to the address
 *                        of the newly created update_committee_action object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_update_committee_action_t object by calling
 *       \ref cardano_update_committee_action_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_update_committee_action_t* update_committee_action = NULL;
 *
 * cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the update_committee_action
 *
 *   // Once done, ensure to clean up and release the update_committee_action
 *   cardano_update_committee_action_unref(&update_committee_action);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode update_committee_action: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_committee_action_from_cbor(cardano_cbor_reader_t* reader, cardano_update_committee_action_t** update_committee_action);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_update_committee_action_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] update_committee_action A constant pointer to the \ref cardano_update_committee_action_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p update_committee_action or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_update_committee_action_to_cbor(update_committee_action, writer);
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
 * cardano_update_committee_action_unref(&update_committee_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_update_committee_action_to_cbor(
  const cardano_update_committee_action_t* update_committee_action,
  cardano_cbor_writer_t*                   writer);

/**
 * \brief Sets the members to be removed in the update committee action.
 *
 * This function updates the list of members to be removed from a constitutional committee within a \ref cardano_update_committee_action_t object.
 * It takes a set of credentials identifying the members that are no longer part of the committee.
 *
 * \param[in,out] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object to which the members to be removed will be set.
 * \param[in] members_to_be_removed A pointer to an initialized \ref cardano_credential_set_t object representing the committee members to be removed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the members to be removed were
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         update_committee_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume update_committee_action is already initialized
 * cardano_credential_set_t* members_to_be_removed = cardano_credential_set_new(...); // Assume members_to_be_removed is initialized
 *
 * cardano_error_t result = cardano_update_committee_action_set_members_to_be_removed(update_committee_action, members_to_be_removed);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The members to be removed are now set for the update committee action
 * }
 * else
 * {
 *   printf("Failed to set the members to be removed.\n");
 * }
 *
 * // Cleanup the update_committee_action and members_to_be_removed after use
 * cardano_update_committee_action_unref(&update_committee_action);
 * cardano_credential_set_unref(&members_to_be_removed);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_committee_action_set_members_to_be_removed(cardano_update_committee_action_t* update_committee_action, cardano_credential_set_t* members_to_be_removed);

/**
 * \brief Retrieves the set of members to be removed from the update committee action.
 *
 * This function retrieves the set of members to be removed from a given \ref cardano_update_committee_action_t object.
 * The members to be removed are represented as a \ref cardano_credential_set_t object.
 *
 * \param[in] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object from which the members to be removed are retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_credential_set_t object representing the members to be removed.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_credential_set_unref
 *         when it is no longer needed. If the update_committee_action does not have members set to be removed, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume initialized
 * cardano_credential_set_t* members_to_be_removed = cardano_update_committee_action_get_members_to_be_removed(update_committee_action);
 *
 * if (members_to_be_removed != NULL)
 * {
 *   // Process the members to be removed
 *   // Display, modify, or use the credentials in some way
 *
 *   // Once done, ensure to clean up and release the members to be removed
 *   cardano_credential_set_unref(&members_to_be_removed);
 * }
 * else
 * {
 *   printf("ERROR: No members to be removed are set for this update committee action.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_set_t*
cardano_update_committee_action_get_members_to_be_removed(cardano_update_committee_action_t* update_committee_action);

/**
 * \brief Sets the members to be added in the update committee action.
 *
 * This function updates the set of members to be added to a \ref cardano_update_committee_action_t object.
 * The members to be added are represented as a \ref cardano_committee_members_map_t object.
 *
 * \param[in,out] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object to which the members to be added will be set.
 * \param[in] members_to_be_added A pointer to an initialized \ref cardano_committee_members_map_t object representing the new members to be added. This parameter
 *            can be NULL if the members to be added are to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the members were
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         update_committee_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume update_committee_action is already initialized
 * cardano_committee_members_map_t* members_to_be_added = cardano_committee_members_map_new(...); // Assume members_to_be_added is initialized
 *
 * cardano_error_t result = cardano_update_committee_action_set_members_to_be_added(update_committee_action, members_to_be_added);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The new members are now set for the update committee action
 * }
 * else
 * {
 *   printf("Failed to set the new members.\n");
 * }
 *
 * // Cleanup the update_committee_action and members_to_be_added after use
 * cardano_update_committee_action_unref(&update_committee_action);
 * if (members_to_be_added)
 * {
 *   cardano_committee_members_map_unref(&members_to_be_added);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_committee_action_set_members_to_be_added(cardano_update_committee_action_t* update_committee_action, cardano_committee_members_map_t* members_to_be_added);

/**
 * \brief Retrieves the members to be added from an update committee action.
 *
 * This function retrieves the set of members to be added from a given \ref cardano_update_committee_action_t object.
 * The members to be added are represented as a \ref cardano_committee_members_map_t object.
 *
 * \param[in] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object from which the members to be added are retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_committee_members_map_t object representing the members to be added.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_committee_members_map_unref
 *         when it is no longer needed. If the update_committee_action does not have members to be added set, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume initialized
 * cardano_committee_members_map_t* members_to_be_added = cardano_update_committee_action_get_members_to_be_added(update_committee_action);
 *
 * if (members_to_be_added != NULL)
 * {
 *   // Use the members to be added
 *
 *   // Once done, ensure to clean up and release the members to be added
 *   cardano_committee_members_map_unref(&members_to_be_added);
 * }
 * else
 * {
 *   printf("No members to be added are set for this update committee action.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_committee_members_map_t*
cardano_update_committee_action_get_members_to_be_added(cardano_update_committee_action_t* update_committee_action);

/**
 * \brief Sets the quorum in an update committee action.
 *
 * This function updates the quorum of a \ref cardano_update_committee_action_t object.
 * The quorum is represented as a \ref cardano_unit_interval_t object, which specifies the minimum percentage of committee
 * members that must participate for a vote to be valid.
 *
 * \param[in,out] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object to which the quorum will be set.
 * \param[in] quorum A pointer to an initialized \ref cardano_unit_interval_t object representing the new quorum threshold.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the quorum was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume update_committee_action is already initialized
 * cardano_unit_interval_t* quorum = cardano_unit_interval_new(...); // Assume quorum is already initialized
 *
 * cardano_error_t result = cardano_update_committee_action_set_quorum(update_committee_action, quorum);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The quorum is now set for the update committee action
 * }
 * else
 * {
 *   printf("Failed to set the quorum.\n");
 * }
 *
 * // Cleanup the update_committee_action and quorum after use
 * cardano_unit_interval_unref(&quorum);
 * cardano_update_committee_action_unref(&update_committee_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_committee_action_set_quorum(cardano_update_committee_action_t* update_committee_action, cardano_unit_interval_t* quorum);

/**
 * \brief Retrieves the quorum from an update committee action.
 *
 * This function fetches the quorum threshold set within a \ref cardano_update_committee_action_t object.
 * The quorum is represented as a \ref cardano_unit_interval_t object, detailing the minimum percentage of committee
 * members required for a vote to be valid.
 *
 * \param[in] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object from which the quorum is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_unit_interval_t object representing the quorum threshold.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_unit_interval_unref
 *         when it is no longer needed. If the update_committee_action does not have a quorum set, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume initialized
 * cardano_unit_interval_t* quorum = cardano_update_committee_action_get_quorum(update_committee_action);
 *
 * if (quorum != NULL)
 * {
 *   printf("Quorum Threshold: %u/%u\n", quorum->numerator, quorum->denominator);
 *   // Use the quorum threshold
 *
 *   // Once done, ensure to clean up and release the quorum threshold
 *   cardano_unit_interval_unref(&quorum);
 * }
 * else
 * {
 *   printf("No quorum threshold set for this update committee action.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_unit_interval_t*
cardano_update_committee_action_get_quorum(cardano_update_committee_action_t* update_committee_action);

/**
 * \brief Sets the governance action ID in the update_committee_action.
 *
 * This function updates the governance action ID of a \ref cardano_update_committee_action_t object.
 * The governance action ID is a \ref cardano_governance_action_id_t object representing the last enacted action of the same type.
 *
 * \param[in,out] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object to which the governance action ID will be set.
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the last enacted action of the same type. This parameter
 *            can be NULL if the governance action ID is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the governance action ID was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         update_committee_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume update_committee_action is already initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...); // Optionally initialized
 *
 * cardano_error_t result = cardano_update_committee_action_set_governance_action_id(update_committee_action, governance_action_id);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The governance action ID is now set for the update_committee_action
 * }
 * else
 * {
 *   printf("Failed to set the governance action ID.\n");
 * }
 *
 * // Cleanup the update_committee_action and optionally the governance action ID
 * cardano_update_committee_action_unref(&update_committee_action);
 *
 * if (governance_action_id)
 * {
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_committee_action_set_governance_action_id(cardano_update_committee_action_t* update_committee_action, cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the governance action ID from a update_committee_action.
 *
 * This function retrieves the governance action ID from a given \ref cardano_update_committee_action_t object. The governance action ID
 * is represented as a \ref cardano_governance_action_id_t object.
 *
 * \param[in] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object from which the governance action ID is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_governance_action_id_t object representing the governance action ID.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_governance_action_id_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = ...; // Assume initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_update_committee_action_get_governance_action_id(update_committee_action);
 *
 * if (governance_action_id != NULL)
 * {
 *   printf("Governance Action ID: %u\n", cardano_governance_action_id_to_uint(governance_action_id));
 *   // Use the governance action ID
 *
 *   // Once done, ensure to clean up and release the governance action ID
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_governance_action_id_t*
cardano_update_committee_action_get_governance_action_id(cardano_update_committee_action_t* update_committee_action);

/**
 * \brief Decrements the reference count of a cardano_update_committee_action_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_update_committee_action_t object
 * by decreasing its reference count. When the reference count reaches zero, the update_committee_action is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] update_committee_action A pointer to the pointer of the update_committee_action object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_committee_action_t* update_committee_action = cardano_update_committee_action_new(major, minor);
 *
 * // Perform operations with the update_committee_action...
 *
 * cardano_update_committee_action_unref(&update_committee_action);
 * // At this point, update_committee_action is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_update_committee_action_unref, the pointer to the \ref cardano_update_committee_action_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_update_committee_action_unref(cardano_update_committee_action_t** update_committee_action);

/**
 * \brief Increases the reference count of the cardano_update_committee_action_t object.
 *
 * This function is used to manually increment the reference count of an cardano_update_committee_action_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_update_committee_action_unref.
 *
 * \param update_committee_action A pointer to the cardano_update_committee_action_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming update_committee_action is a previously created update_committee_action object
 *
 * cardano_update_committee_action_ref(update_committee_action);
 *
 * // Now update_committee_action can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_update_committee_action_ref there is a corresponding
 * call to \ref cardano_update_committee_action_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_update_committee_action_ref(cardano_update_committee_action_t* update_committee_action);

/**
 * \brief Retrieves the current reference count of the cardano_update_committee_action_t object.
 *
 * This function returns the number of active references to an cardano_update_committee_action_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_update_committee_action_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param update_committee_action A pointer to the cardano_update_committee_action_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_update_committee_action_t object. If the object
 * is properly managed (i.e., every \ref cardano_update_committee_action_ref call is matched with a
 * \ref cardano_update_committee_action_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming update_committee_action is a previously created update_committee_action object
 *
 * size_t ref_count = cardano_update_committee_action_refcount(update_committee_action);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_update_committee_action_refcount(const cardano_update_committee_action_t* update_committee_action);

/**
 * \brief Sets the last error message for a given cardano_update_committee_action_t object.
 *
 * Records an error message in the update_committee_action's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] update_committee_action A pointer to the \ref cardano_update_committee_action_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the update_committee_action's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_update_committee_action_set_last_error(
  cardano_update_committee_action_t* update_committee_action,
  const char*                        message);

/**
 * \brief Retrieves the last error message recorded for a specific update_committee_action.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_update_committee_action_set_last_error for the given
 * update_committee_action. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] update_committee_action A pointer to the \ref cardano_update_committee_action_t instance whose last error
 *                   message is to be retrieved. If the update_committee_action is NULL, the function
 *                   returns a generic error message indicating the null update_committee_action.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified update_committee_action. If the update_committee_action is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_update_committee_action_set_last_error for the same update_committee_action, or until
 *       the update_committee_action is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_update_committee_action_get_last_error(
  const cardano_update_committee_action_t* update_committee_action);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // UPDATE_COMMITTEE_ACTION_H