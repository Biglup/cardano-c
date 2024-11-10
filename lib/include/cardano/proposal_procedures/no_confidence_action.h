/**
 * \file no_confidence_action.h
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

#ifndef NO_CONFIDENCE_ACTION_H
#define NO_CONFIDENCE_ACTION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/constitution.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Propose a state of no-confidence in the current constitutional committee.
 * Allows Ada holders to challenge the authority granted to the existing committee.
 */
typedef struct cardano_no_confidence_action_t cardano_no_confidence_action_t;

/**
 * \brief Creates and initializes a new instance of the No Confidence Action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_no_confidence_action_t object,
 * which represents an action to propose a state of no-confidence in the current constitutional committee within the Cardano network.
 * This action allows Ada holders to challenge the authority of the committee, potentially leading to its reconstitution.
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
 * \param[in] governance_action_id An optional pointer to a \ref cardano_governance_action_id_t object representing the last enacted
 *                                 action of the same type. This parameter can be NULL if no governance action of this type has been enacted.
 * \param[out] no_confidence_action On successful initialization, this will point to a newly created
 *             \ref cardano_no_confidence_action_t object. This object represents a "strong reference"
 *             to the no-confidence action, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the no-confidence action is no longer needed, the caller must release it
 *             by calling \ref cardano_no_confidence_action_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the no-confidence action was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...); // Optionally initialized
 * cardano_no_confidence_action_t* no_confidence_action = NULL;
 * cardano_error_t result = cardano_no_confidence_action_new(governance_action_id, &no_confidence_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the no-confidence action
 *   // Free resources when done
 *   cardano_no_confidence_action_unref(&no_confidence_action);
 * }
 * else
 * {
 *   printf("Failed to create the no-confidence action: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_no_confidence_action_new(
  cardano_governance_action_id_t*  governance_action_id,
  cardano_no_confidence_action_t** no_confidence_action);

/**
 * \brief Creates a \ref cardano_no_confidence_action_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_no_confidence_action_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a no_confidence_action.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] no_confidence_action A pointer to a pointer of \ref cardano_no_confidence_action_t that will be set to the address
 *                        of the newly created no_confidence_action object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_no_confidence_action_t object by calling
 *       \ref cardano_no_confidence_action_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_no_confidence_action_t* no_confidence_action = NULL;
 *
 * cardano_error_t result = cardano_no_confidence_action_from_cbor(reader, &no_confidence_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the no_confidence_action
 *
 *   // Once done, ensure to clean up and release the no_confidence_action
 *   cardano_no_confidence_action_unref(&no_confidence_action);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode no_confidence_action: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_no_confidence_action_from_cbor(cardano_cbor_reader_t* reader, cardano_no_confidence_action_t** no_confidence_action);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_no_confidence_action_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] no_confidence_action A constant pointer to the \ref cardano_no_confidence_action_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p no_confidence_action or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_no_confidence_action_t* no_confidence_action = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_no_confidence_action_to_cbor(no_confidence_action, writer);
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
 * cardano_no_confidence_action_unref(&no_confidence_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_no_confidence_action_to_cbor(
  const cardano_no_confidence_action_t* no_confidence_action,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Sets the governance action ID in the no_confidence_action.
 *
 * This function updates the governance action ID of a \ref cardano_no_confidence_action_t object.
 * The governance action ID is a \ref cardano_governance_action_id_t object representing the unique identifier for the last enacted action of the same type.
 *
 * \param[in,out] no_confidence_action A pointer to an initialized \ref cardano_no_confidence_action_t object to which the governance action ID will be set.
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the last enacted action of the same type. This parameter
 *            can be NULL if the governance action ID is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the governance action ID was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         no_confidence_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_no_confidence_action_t* no_confidence_action = ...; // Assume no_confidence_action is already initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...); // Optionally initialized
 *
 * cardano_error_t result = cardano_no_confidence_action_set_governance_action_id(no_confidence_action, governance_action_id);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The governance action ID is now set for the no_confidence_action
 * }
 * else
 * {
 *   printf("Failed to set the governance action ID.\n");
 * }
 *
 * // Cleanup the no_confidence_action and optionally the governance action ID
 * cardano_no_confidence_action_unref(&no_confidence_action);
 *
 * if (governance_action_id)
 * {
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_no_confidence_action_set_governance_action_id(cardano_no_confidence_action_t* no_confidence_action, cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the governance action ID from a no_confidence_action.
 *
 * This function retrieves the governance action ID from a given \ref cardano_no_confidence_action_t object. The governance action ID
 * is represented as a \ref cardano_governance_action_id_t object.
 *
 * \param[in] no_confidence_action A pointer to an initialized \ref cardano_no_confidence_action_t object from which the governance action ID is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_governance_action_id_t object representing the governance action ID.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_governance_action_id_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_no_confidence_action_t* no_confidence_action = ...; // Assume initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_no_confidence_action_get_governance_action_id(no_confidence_action);
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
cardano_no_confidence_action_get_governance_action_id(cardano_no_confidence_action_t* no_confidence_action);

/**
 * \brief Decrements the reference count of a cardano_no_confidence_action_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_no_confidence_action_t object
 * by decreasing its reference count. When the reference count reaches zero, the no_confidence_action is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] no_confidence_action A pointer to the pointer of the no_confidence_action object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_no_confidence_action_t* no_confidence_action = cardano_no_confidence_action_new(major, minor);
 *
 * // Perform operations with the no_confidence_action...
 *
 * cardano_no_confidence_action_unref(&no_confidence_action);
 * // At this point, no_confidence_action is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_no_confidence_action_unref, the pointer to the \ref cardano_no_confidence_action_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_no_confidence_action_unref(cardano_no_confidence_action_t** no_confidence_action);

/**
 * \brief Increases the reference count of the cardano_no_confidence_action_t object.
 *
 * This function is used to manually increment the reference count of an cardano_no_confidence_action_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_no_confidence_action_unref.
 *
 * \param no_confidence_action A pointer to the cardano_no_confidence_action_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming no_confidence_action is a previously created no_confidence_action object
 *
 * cardano_no_confidence_action_ref(no_confidence_action);
 *
 * // Now no_confidence_action can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_no_confidence_action_ref there is a corresponding
 * call to \ref cardano_no_confidence_action_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_no_confidence_action_ref(cardano_no_confidence_action_t* no_confidence_action);

/**
 * \brief Retrieves the current reference count of the cardano_no_confidence_action_t object.
 *
 * This function returns the number of active references to an cardano_no_confidence_action_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_no_confidence_action_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param no_confidence_action A pointer to the cardano_no_confidence_action_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_no_confidence_action_t object. If the object
 * is properly managed (i.e., every \ref cardano_no_confidence_action_ref call is matched with a
 * \ref cardano_no_confidence_action_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming no_confidence_action is a previously created no_confidence_action object
 *
 * size_t ref_count = cardano_no_confidence_action_refcount(no_confidence_action);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_no_confidence_action_refcount(const cardano_no_confidence_action_t* no_confidence_action);

/**
 * \brief Sets the last error message for a given cardano_no_confidence_action_t object.
 *
 * Records an error message in the no_confidence_action's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] no_confidence_action A pointer to the \ref cardano_no_confidence_action_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the no_confidence_action's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_no_confidence_action_set_last_error(
  cardano_no_confidence_action_t* no_confidence_action,
  const char*                     message);

/**
 * \brief Retrieves the last error message recorded for a specific no_confidence_action.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_no_confidence_action_set_last_error for the given
 * no_confidence_action. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] no_confidence_action A pointer to the \ref cardano_no_confidence_action_t instance whose last error
 *                   message is to be retrieved. If the no_confidence_action is NULL, the function
 *                   returns a generic error message indicating the null no_confidence_action.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified no_confidence_action. If the no_confidence_action is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_no_confidence_action_set_last_error for the same no_confidence_action, or until
 *       the no_confidence_action is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_no_confidence_action_get_last_error(
  const cardano_no_confidence_action_t* no_confidence_action);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // NO_CONFIDENCE_ACTION_H