/**
 * \file hard_fork_initiation_action.h
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

#ifndef HARD_FORK_INITIATION_ACTION_H
#define HARD_FORK_INITIATION_ACTION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/protocol_version.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents the initiation action for a hard fork in the Cardano network.
 */
typedef struct cardano_hard_fork_initiation_action_t cardano_hard_fork_initiation_action_t;

/**
 * \brief Creates and initializes a new instance of the Hard Fork Initiation Action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_hard_fork_initiation_action_t object,
 * which represents an action to initiate a hard fork in the Cardano network. The action is defined by specifying
 * a protocol version and an optional governance action identifier.
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
 * \param[in] version A pointer to a \ref cardano_protocol_version_t object representing the protocol version for the hard fork.
 * \param[in] governance_action_id An optional pointer to a \ref cardano_governance_action_id_t object representing the unique identifier
 *                                 for the last enacted governance action of the same type. This parameter can be NULL if no governance action of this type is enacted.
 * \param[out] hard_fork_initiation_action On successful initialization, this will point to a newly created
 *             \ref cardano_hard_fork_initiation_action_t object. This object represents a "strong reference"
 *             to the hard fork initiation action, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the hard fork initiation action was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* version = cardano_protocol_version_new(...); // Assume version is already initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = NULL;
 * cardano_error_t result = cardano_hard_fork_initiation_action_new(version, governance_action_id, &hard_fork_initiation_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the hard fork initiation action
 *   // Free resources when done
 *   cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 * }
 * else
 * {
 *   printf("Failed to create the hard fork initiation action: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_protocol_version_unref(&version);
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_hard_fork_initiation_action_new(
  cardano_protocol_version_t*             version,
  cardano_governance_action_id_t*         governance_action_id,
  cardano_hard_fork_initiation_action_t** hard_fork_initiation_action);

/**
 * \brief Creates a \ref cardano_hard_fork_initiation_action_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_hard_fork_initiation_action_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a hard_fork_initiation_action.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] hard_fork_initiation_action A pointer to a pointer of \ref cardano_hard_fork_initiation_action_t that will be set to the address
 *                        of the newly created hard_fork_initiation_action object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_hard_fork_initiation_action_t object by calling
 *       \ref cardano_hard_fork_initiation_action_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = NULL;
 *
 * cardano_error_t result = cardano_hard_fork_initiation_action_from_cbor(reader, &hard_fork_initiation_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the hard_fork_initiation_action
 *
 *   // Once done, ensure to clean up and release the hard_fork_initiation_action
 *   cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode hard_fork_initiation_action: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_hard_fork_initiation_action_from_cbor(cardano_cbor_reader_t* reader, cardano_hard_fork_initiation_action_t** hard_fork_initiation_action);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_hard_fork_initiation_action_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] hard_fork_initiation_action A constant pointer to the \ref cardano_hard_fork_initiation_action_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p hard_fork_initiation_action or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_hard_fork_initiation_action_to_cbor(hard_fork_initiation_action, writer);
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
 * cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_hard_fork_initiation_action_to_cbor(
  const cardano_hard_fork_initiation_action_t* hard_fork_initiation_action,
  cardano_cbor_writer_t*                       writer);

/**
 * \brief Sets the protocol version in the hard fork initiation action.
 *
 * This function updates the protocol version of a \ref cardano_hard_fork_initiation_action_t object.
 * The protocol version specifies the new set of rules that will become active after the hard fork.
 *
 * \param[in,out] hard_fork_initiation_action A pointer to an initialized \ref cardano_hard_fork_initiation_action_t object to which the protocol version will be set.
 * \param[in] protocol_version A pointer to an initialized \ref cardano_protocol_version_t object representing the new protocol version.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the protocol version was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = ...; // Assume hard_fork_initiation_action is already initialized
 * cardano_protocol_version_t* protocol_version = cardano_protocol_version_new(...); // Assume protocol_version is already initialized
 *
 * cardano_error_t result = cardano_hard_fork_initiation_action_set_protocol_version(hard_fork_initiation_action, protocol_version);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The protocol version is now set for the hard fork initiation action
 * }
 * else
 * {
 *   printf("Failed to set the protocol version.\n");
 * }
 * // Clean up the hard_fork_initiation_action and protocol_version after use
 * cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 * cardano_protocol_version_unref(&protocol_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_hard_fork_initiation_action_set_protocol_version(cardano_hard_fork_initiation_action_t* hard_fork_initiation_action, cardano_protocol_version_t* protocol_version);

/**
 * \brief Gets the protocol version from a hard_fork_initiation_action.
 *
 * This function retrieves the protocol version from a given \ref cardano_hard_fork_initiation_action_t object.
 * The protocol version specifies the set of rules that will govern the network following the hard fork.
 *
 * \param[in] hard_fork_initiation_action A pointer to an initialized \ref cardano_hard_fork_initiation_action_t object from which the protocol version is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_protocol_version_t object representing the protocol version.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_protocol_version_unref when it is no longer needed.
 *         If the hard_fork_initiation_action does not have a protocol version set, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = ...; // Assume initialized
 * cardano_protocol_version_t* protocol_version = cardano_hard_fork_initiation_action_get_protocol_version(hard_fork_initiation_action);
 *
 * if (protocol_version != NULL)
 * {
 *   // Use the protocol version
 *
 *   // Once done, ensure to clean up and release the protocol version
 *   cardano_protocol_version_unref(&protocol_version);
 * }
 * else
 * {
 *   printf("ERROR: No protocol version set for this hard fork initiation action.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_protocol_version_t*
cardano_hard_fork_initiation_action_get_protocol_version(cardano_hard_fork_initiation_action_t* hard_fork_initiation_action);

/**
 * \brief Sets the governance action ID in the hard fork initiation action.
 *
 * This function updates the governance action ID of a \ref cardano_hard_fork_initiation_action_t object. The governance action ID
 * represents the unique identifier for the most recently enacted governance action associated with a hard fork.
 *
 * \param[in,out] hard_fork_initiation_action A pointer to an initialized \ref cardano_hard_fork_initiation_action_t object to which
 *                                            the governance action ID will be set.
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the last enacted action of the same type. This parameter
 *            can be NULL if the governance action ID is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the governance action ID was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         hard_fork_initiation_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = ...; // Assume hard_fork_initiation_action is already initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...); // Optionally initialized
 *
 * cardano_error_t result = cardano_hard_fork_initiation_action_set_governance_action_id(hard_fork_initiation_action, governance_action_id);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The governance action ID is now set for the hard_fork_initiation_action
 * }
 * else
 * {
 *   printf("Failed to set the governance action ID.\n");
 * }
 *
 * // Cleanup the hard_fork_initiation_action and optionally the governance action ID
 * cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 *
 * if (governance_action_id)
 * {
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 *
 * \note This function maintains governance continuity by referencing the latest governance action ID of the same type,
 *       allowing only one active action at a time for each governance type.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_hard_fork_initiation_action_set_governance_action_id(cardano_hard_fork_initiation_action_t* hard_fork_initiation_action, cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the governance action ID from a hard fork initiation action.
 *
 * This function retrieves the governance action ID from a given \ref cardano_hard_fork_initiation_action_t object. The governance
 * action ID references the most recent action of the same type.
 *
 * \param[in] hard_fork_initiation_action A pointer to an initialized \ref cardano_hard_fork_initiation_action_t object from
 *                                        which the governance action ID is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_governance_action_id_t object representing the governance action ID.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_governance_action_id_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = ...; // Assume initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_hard_fork_initiation_action_get_governance_action_id(hard_fork_initiation_action);
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
cardano_hard_fork_initiation_action_get_governance_action_id(cardano_hard_fork_initiation_action_t* hard_fork_initiation_action);

/**
 * \brief Decrements the reference count of a cardano_hard_fork_initiation_action_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_hard_fork_initiation_action_t object
 * by decreasing its reference count. When the reference count reaches zero, the hard_fork_initiation_action is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] hard_fork_initiation_action A pointer to the pointer of the hard_fork_initiation_action object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = cardano_hard_fork_initiation_action_new(major, minor);
 *
 * // Perform operations with the hard_fork_initiation_action...
 *
 * cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 * // At this point, hard_fork_initiation_action is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_hard_fork_initiation_action_unref, the pointer to the \ref cardano_hard_fork_initiation_action_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_hard_fork_initiation_action_unref(cardano_hard_fork_initiation_action_t** hard_fork_initiation_action);

/**
 * \brief Increases the reference count of the cardano_hard_fork_initiation_action_t object.
 *
 * This function is used to manually increment the reference count of an cardano_hard_fork_initiation_action_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_hard_fork_initiation_action_unref.
 *
 * \param hard_fork_initiation_action A pointer to the cardano_hard_fork_initiation_action_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming hard_fork_initiation_action is a previously created hard_fork_initiation_action object
 *
 * cardano_hard_fork_initiation_action_ref(hard_fork_initiation_action);
 *
 * // Now hard_fork_initiation_action can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_hard_fork_initiation_action_ref there is a corresponding
 * call to \ref cardano_hard_fork_initiation_action_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_hard_fork_initiation_action_ref(cardano_hard_fork_initiation_action_t* hard_fork_initiation_action);

/**
 * \brief Retrieves the current reference count of the cardano_hard_fork_initiation_action_t object.
 *
 * This function returns the number of active references to an cardano_hard_fork_initiation_action_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_hard_fork_initiation_action_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param hard_fork_initiation_action A pointer to the cardano_hard_fork_initiation_action_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_hard_fork_initiation_action_t object. If the object
 * is properly managed (i.e., every \ref cardano_hard_fork_initiation_action_ref call is matched with a
 * \ref cardano_hard_fork_initiation_action_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming hard_fork_initiation_action is a previously created hard_fork_initiation_action object
 *
 * size_t ref_count = cardano_hard_fork_initiation_action_refcount(hard_fork_initiation_action);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_hard_fork_initiation_action_refcount(const cardano_hard_fork_initiation_action_t* hard_fork_initiation_action);

/**
 * \brief Sets the last error message for a given cardano_hard_fork_initiation_action_t object.
 *
 * Records an error message in the hard_fork_initiation_action's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] hard_fork_initiation_action A pointer to the \ref cardano_hard_fork_initiation_action_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the hard_fork_initiation_action's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_hard_fork_initiation_action_set_last_error(
  cardano_hard_fork_initiation_action_t* hard_fork_initiation_action,
  const char*                            message);

/**
 * \brief Retrieves the last error message recorded for a specific hard_fork_initiation_action.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_hard_fork_initiation_action_set_last_error for the given
 * hard_fork_initiation_action. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] hard_fork_initiation_action A pointer to the \ref cardano_hard_fork_initiation_action_t instance whose last error
 *                   message is to be retrieved. If the hard_fork_initiation_action is NULL, the function
 *                   returns a generic error message indicating the null hard_fork_initiation_action.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified hard_fork_initiation_action. If the hard_fork_initiation_action is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_hard_fork_initiation_action_set_last_error for the same hard_fork_initiation_action, or until
 *       the hard_fork_initiation_action is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_hard_fork_initiation_action_get_last_error(
  const cardano_hard_fork_initiation_action_t* hard_fork_initiation_action);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HARD_FORK_INITIATION_ACTION_H