/**
 * \file voting_procedures.h
 *
 * \author angel.castillo
 * \date   Aug 08, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURES_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/voting_procedures/governance_action_id_list.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>
#include <cardano/voting_procedures/voting_procedure.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A map of Voter + GovernanceActionId to VotingProcedure.
 */
typedef struct cardano_voting_procedures_t cardano_voting_procedures_t;

/**
 * \brief Creates and initializes a new instance of a map of Voter and GovernanceActionId to VotingProcedure.
 *
 * This function initializes a new \ref cardano_voting_procedures_t object, which maps voters and governance action
 * identifiers to their respective voting procedures. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] voting_procedures On successful initialization, this pointer will be set to a newly created
 *             \ref cardano_voting_procedures_t object. This object represents a "strong reference"
 *             to the map of voting procedures, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the voting_procedures are no longer needed, the caller must release them
 *             by calling \ref cardano_voting_procedures_unref.
 *
 * \return \ref CARDANO_SUCCESS if the map of voting procedures was successfully created, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the output pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedures_t* voting_procedures = NULL;
 *
 * // Attempt to create a new map of voting procedures
 * cardano_error_t result = cardano_voting_procedures_new(&voting_procedures);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the voting_procedures map
 *
 *   // Once done, ensure to clean up and release the voting_procedures
 *   cardano_voting_procedures_unref(&voting_procedures);
 * }
 * else
 * {
 *   printf("Failed to create the voting procedures map: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_voting_procedures_new(cardano_voting_procedures_t** voting_procedures);

/**
 * \brief Creates a voting_procedures from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_voting_procedures_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a voting_procedures.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded voting_procedures data.
 * \param[out] voting_procedures A pointer to a pointer of \ref cardano_voting_procedures_t that will be set to the address
 *                        of the newly created voting_procedures object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the voting_procedures was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_voting_procedures_t object by calling
 *       \ref cardano_voting_procedures_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_voting_procedures_t* voting_procedures = NULL;
 *
 * cardano_error_t result = cardano_voting_procedures_from_cbor(reader, &voting_procedures);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the voting_procedures
 *
 *   // Once done, ensure to clean up and release the voting_procedures
 *   cardano_voting_procedures_unref(&voting_procedures);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode voting_procedures: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_voting_procedures_from_cbor(cardano_cbor_reader_t* reader, cardano_voting_procedures_t** voting_procedures);

/**
 * \brief Serializes a voting_procedures into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_voting_procedures_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] voting_procedures A constant pointer to the \ref cardano_voting_procedures_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p voting_procedures or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedures_t* voting_procedures = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_voting_procedures_to_cbor(voting_procedures, writer);
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
 * cardano_voting_procedures_unref(&voting_procedures);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedures_to_cbor(
  const cardano_voting_procedures_t* voting_procedures,
  cardano_cbor_writer_t*             writer);

/**
 * \brief Inserts a voting procedure associated with a specific voter and governance action ID into the voting procedures map.
 *
 * This function inserts a new entry into the \ref cardano_voting_procedures_t map, linking a voter and a governance action ID
 * to a specific voting procedure. Each component (voter, governance action ID, and voting procedure) is added as a reference,
 * and the reference count for each is incremented to manage their lifecycles within the map.
 *
 * \param[in] voting_procedures A pointer to an initialized \ref cardano_voting_procedures_t object, representing the map
 *                              where the new voting procedure entry will be inserted.
 * \param[in] voter A pointer to an initialized \ref cardano_voter_t object representing the voter.
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the
 *                                 governance action ID.
 * \param[in] value A pointer to an initialized \ref cardano_voting_procedure_t object representing the voting procedure.
 *
 * \return \ref CARDANO_SUCCESS if the entry was successfully inserted, or an appropriate error code indicating the failure
 *         reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedures_t* voting_procedures = cardano_voting_procedures_new();
 * cardano_voter_t* voter = cardano_voter_new(...);
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * cardano_voting_procedure_t* voting_procedure = cardano_voting_procedure_new(...);
 *
 * cardano_error_t result = cardano_voting_procedures_insert(voting_procedures, voter, governance_action_id, voting_procedure);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Voting procedure inserted successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to insert voting procedure: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources
 * cardano_voting_procedures_unref(&voting_procedures);
 * cardano_voter_unref(&voter);
 * cardano_governance_action_id_unref(&governance_action_id);
 * cardano_voting_procedure_unref(&voting_procedure);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedures_insert(cardano_voting_procedures_t* voting_procedures, cardano_voter_t* voter, cardano_governance_action_id_t* governance_action_id, cardano_voting_procedure_t* value);

/**
 * \brief Retrieves a voting procedure associated with a specific voter and governance action ID from the voting procedures map.
 *
 * This function retrieves the \ref cardano_voting_procedure_t object associated with a given voter and governance action ID
 * from the \ref cardano_voting_procedures_t map. If the combination of voter and governance action ID is found in the map,
 * a reference to the associated voting procedure is returned. The reference count of the returned object is incremented,
 * and it is the caller's responsibility to release this reference when it is no longer needed.
 *
 * \param[in] voting_procedures A pointer to an initialized \ref cardano_voting_procedures_t object, representing the map
 *                              from which the voting procedure is to be retrieved.
 * \param[in] voter A pointer to an initialized \ref cardano_voter_t object representing the voter.
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the
 *                                 governance action ID.
 *
 * \return A pointer to the \ref cardano_voting_procedure_t object if the entry is found in the map. Returns NULL if no such entry exists
 *         or if any of the input parameters are NULL. The caller is responsible for managing the lifecycle of the returned object,
 *         including releasing it with \ref cardano_voting_procedure_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedures_t* voting_procedures = cardano_voting_procedures_new();
 * cardano_voter_t* voter = cardano_voter_new(...);
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 *
 * cardano_voting_procedure_t* voting_procedure = cardano_voting_procedures_get(voting_procedures, voter, governance_action_id);
 * if (voting_procedure)
 * {
 *   // Process the voting procedure
 *
 *   // Once done, ensure to clean up and release the voting procedure
 *   cardano_voting_procedure_unref(&voting_procedure);
 * }
 * else
 * {
 *   printf("No voting procedure found for the specified voter and governance action ID.\n");
 * }
 *
 * // Release other resources
 * cardano_voting_procedures_unref(&voting_procedures);
 * cardano_voter_unref(&voter);
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_voting_procedure_t* cardano_voting_procedures_get(cardano_voting_procedures_t* voting_procedures, cardano_voter_t* voter, cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves a list of governance action IDs associated with a specific voter from the voting procedures map.
 *
 * This function extracts all governance action IDs associated with a given voter from a \ref cardano_voting_procedures_t map.
 * The function returns a pointer to a \ref cardano_governance_action_id_list_t object containing the list of IDs.
 * If the voter is found, the list will contain one or more governance action IDs; if the voter has no associated actions,
 * the list will be empty. The caller is responsible for managing the lifecycle of the returned list object, including releasing it
 * with \ref cardano_governance_action_id_list_unref when it is no longer needed.
 *
 * \param[in] voting_procedures A pointer to an initialized \ref cardano_voting_procedures_t object, representing the map
 *                              from which the governance action IDs are to be retrieved.
 * \param[in] voter A pointer to an initialized \ref cardano_voter_t object representing the voter.
 * \param[out] governance_action_ids On successful retrieval, this will point to a newly created
 *                                   \ref cardano_governance_action_id_list_t object containing the list of governance action IDs.
 *                                   If no actions are associated with the voter, this will point to an empty list.
 *
 * \return A pointer to the \ref cardano_voting_procedure_t object associated with the voter if the voter is found in the map,
 *         otherwise returns NULL. The caller is responsible for managing the lifecycle of the returned object and any associated list.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedures_t* voting_procedures = cardano_voting_procedures_new(...);
 * cardano_voter_t* voter = cardano_voter_new(...);
 * cardano_governance_action_id_list_t* governance_action_ids = NULL;
 *
 * cardano_voting_procedure_t* voting_procedure = cardano_voting_procedures_get_governance_ids_by_voter(
 *     voting_procedures, voter, &governance_action_ids);
 *
 * if (voting_procedure)
 * {
 *   // Process each governance action ID in the list
 *   for (size_t i = 0; i < cardano_governance_action_id_list_get_length(governance_action_ids); i++)
 *   {
 *     cardano_governance_action_id_t* action_id = cardano_governance_action_id_list_get(governance_action_ids, i);
 *     // Use the action_id
 *
 *     cardano_governance_action_id_unref(&action_id);
 *   }
 *
 *   // Once done, ensure to clean up and release the voting procedure
 *   cardano_voting_procedure_unref(&voting_procedure);
 * }
 * else
 * {
 *   printf("No voting procedure found for the specified voter.\n");
 * }
 *
 * // Release the list of governance action IDs
 * cardano_governance_action_id_list_unref(&governance_action_ids);
 *
 * // Release other resources
 * cardano_voting_procedures_unref(&voting_procedures);
 * cardano_voter_unref(&voter);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedures_get_governance_ids_by_voter(cardano_voting_procedures_t* voting_procedures, cardano_voter_t* voter, cardano_governance_action_id_list_t** governance_action_ids);

/**
 * \brief Retrieves a list of voters from the voting procedures map.
 *
 * This function extracts all voters from a given \ref cardano_voting_procedures_t map and stores them in a newly created
 * \ref cardano_voter_list_t object. The caller is responsible for managing the lifecycle of the returned list object,
 * including releasing it with \ref cardano_voter_list_unref when it is no longer needed.
 *
 * \param[in] voting_procedures A pointer to an initialized \ref cardano_voting_procedures_t object, representing the map
 *                              from which the voters are to be retrieved.
 * \param[out] voters On successful retrieval, this parameter will point to a newly created \ref cardano_voter_list_t object containing
 *                    the list of voters. If the map is empty, this will point to an empty list.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the list of voters
 *         was successfully retrieved, or an appropriate error code if no voters are found or an error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedures_t* voting_procedures = ...;
 * cardano_voter_list_t* voters = NULL;
 *
 * cardano_error_t result = cardano_voting_procedures_get_voters(voting_procedures, &voters);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process each voter in the list
 *   for (size_t i = 0; i < cardano_voter_list_get_length(voters); ++i)
 *   {
 *     cardano_voter_t* voter = cardano_voter_list_get(voters, i);
 *     // Use the voter
 *
 *     cardano_voter_unref(&voter);
 *   }
 * }
 * else
 * {
 *   printf("No voters found or an error occurred.\n");
 * }
 *
 * // Release the list of voters
 * cardano_voter_list_unref(&voters);
 *
 * // Release other resources
 * cardano_voting_procedures_unref(&voting_procedures);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedures_get_voters(cardano_voting_procedures_t* voting_procedures, cardano_voter_list_t** voters);

/**
 * \brief Decrements the reference count of a voting_procedures object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_voting_procedures_t object
 * by decreasing its reference count. When the reference count reaches zero, the voting_procedures is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] voting_procedures A pointer to the pointer of the voting_procedures object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedures_t* voting_procedures = cardano_voting_procedures_new();
 *
 * // Perform operations with the voting_procedures...
 *
 * cardano_voting_procedures_unref(&voting_procedures);
 * // At this point, voting_procedures is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_voting_procedures_unref, the pointer to the \ref cardano_voting_procedures_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_voting_procedures_unref(cardano_voting_procedures_t** voting_procedures);

/**
 * \brief Increases the reference count of the cardano_voting_procedures_t object.
 *
 * This function is used to manually increment the reference count of a voting_procedures
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_voting_procedures_unref.
 *
 * \param voting_procedures A pointer to the voting_procedures object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voting_procedures is a previously created voting_procedures object
 *
 * cardano_voting_procedures_ref(voting_procedures);
 *
 * // Now voting_procedures can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_voting_procedures_ref there is a corresponding
 * call to \ref cardano_voting_procedures_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_voting_procedures_ref(cardano_voting_procedures_t* voting_procedures);

/**
 * \brief Retrieves the current reference count of the cardano_voting_procedures_t object.
 *
 * This function returns the number of active references to a voting_procedures object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_voting_procedures_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param voting_procedures A pointer to the voting_procedures object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified voting_procedures object. If the object
 * is properly managed (i.e., every \ref cardano_voting_procedures_ref call is matched with a
 * \ref cardano_voting_procedures_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voting_procedures is a previously created voting_procedures object
 *
 * size_t ref_count = cardano_voting_procedures_refcount(voting_procedures);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_voting_procedures_refcount(const cardano_voting_procedures_t* voting_procedures);

/**
 * \brief Sets the last error message for a given voting_procedures object.
 *
 * Records an error message in the voting_procedures's last_error buffer, overwriting any existing message.
 * This is useful for storing devoting_proceduresive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] voting_procedures A pointer to the \ref cardano_voting_procedures_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the voting_procedures's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_voting_procedures_set_last_error(cardano_voting_procedures_t* voting_procedures, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific voting_procedures.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_voting_procedures_set_last_error for the given
 * voting_procedures. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] voting_procedures A pointer to the \ref cardano_voting_procedures_t instance whose last error
 *                   message is to be retrieved. If the voting_procedures is NULL, the function
 *                   returns a generic error message indicating the null voting_procedures.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified voting_procedures. If the voting_procedures is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_voting_procedures_set_last_error for the same voting_procedures, or until
 *       the voting_procedures is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_voting_procedures_get_last_error(const cardano_voting_procedures_t* voting_procedures);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURES_H