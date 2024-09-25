/**
 * \file update.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPDATE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPDATE_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/protocol_params/proposed_param_updates.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief When stakeholders wish to propose changes to the system's parameters, they submit an update proposal.
 * Such proposals are then voted on by the community. If approved, the protocol parameters are adjusted
 * accordingly in the specified epoch.
 */
typedef struct cardano_update_t cardano_update_t;

/**
 * \brief Creates and initializes a new instance of the update.
 *
 * This function allocates and initializes a new instance of the update,
 * representing an update proposal for the Cardano protocol.
 *
 * \param[in] epoch The epoch number in which the proposal will come into effect if accepted.
 * \param[in] updates A pointer to the proposed protocol parameter updates.
 * \param[out] update On successful initialization, this will point to a newly created
 *            update object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the update is no longer needed, the caller must release it
 *            by calling \ref cardano_update_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the update was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_t* update = NULL;
 * uint64_t epoch = 123; // Example epoch number
 * cardano_proposed_param_updates_t* proposed_updates = NULL;
 *
 * // Initialize proposed_updates properly before using
 *
 * // Attempt to create a new update object
 * cardano_error_t result = cardano_update_new(epoch, proposed_updates, &update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the update
 *
 *   // Once done, ensure to clean up and release the update
 *   cardano_update_unref(&update);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_new(
  uint64_t                          epoch,
  cardano_proposed_param_updates_t* updates,
  cardano_update_t**                update);

/**
 * \brief Creates a \ref cardano_update_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_update_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a update.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] update A pointer to a pointer of \ref cardano_update_t that will be set to the address
 *                        of the newly created update object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_update_t object by calling
 *       \ref cardano_update_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_update_t* update = NULL;
 *
 * cardano_error_t result = cardano_update_from_cbor(reader, &update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the update
 *
 *   // Once done, ensure to clean up and release the update
 *   cardano_update_unref(&update);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode update: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_from_cbor(cardano_cbor_reader_t* reader, cardano_update_t** update);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_update_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] update A constant pointer to the \ref cardano_update_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p update or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_t* update = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_update_to_cbor(update, writer);
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
 * cardano_update_unref(&update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_update_to_cbor(
  const cardano_update_t* update,
  cardano_cbor_writer_t*  writer);

/**
 * \brief Retrieves the epoch number from the update.
 *
 * This function returns the epoch number in which the proposal will come into effect if accepted.
 *
 * \param[in] update Pointer to the update object.
 * \param[out] epoch On successful retrieval, this will point to the epoch number.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the epoch number was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_t* update = NULL;
 * uint64_t epoch = 0;
 *
 * // Assume update is initialized properly
 *
 * cardano_error_t result = cardano_update_get_epoch(update, &epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Epoch: %lu\n", epoch);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_update_get_epoch(
  const cardano_update_t* update,
  uint64_t*               epoch);

/**
 * \brief Retrieves the proposed parameters from the update.
 *
 * This function returns the proposed protocol parameter updates from the update object.
 *
 * \param[in] update Pointer to the update object.
 * \param[out] proposed_parameters On successful retrieval, this will point to the proposed protocol parameter updates.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the proposed parameters were successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_t* update = NULL;
 * cardano_proposed_param_updates_t* proposed_parameters = NULL;
 *
 * // Assume update is initialized properly
 *
 * cardano_error_t result = cardano_update_get_proposed_parameters(update, &proposed_parameters);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposed_parameters
 *
 *   // Once done, ensure to clean up and release the proposed_parameters
 *   cardano_proposed_param_updates_unref(&proposed_parameters);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_update_get_proposed_parameters(
  const cardano_update_t*            update,
  cardano_proposed_param_updates_t** proposed_parameters);

/**
 * \brief Sets the epoch in the update object.
 *
 * This function sets the epoch in the given update object, specifying when the proposed
 * protocol parameter updates will take effect if accepted.
 *
 * \param[in] update Pointer to the update object.
 * \param[in] epoch The epoch number to set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the epoch was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_t* update = NULL;
 * uint64_t epoch = 300; // Example epoch number
 *
 * // Assume update is initialized properly
 *
 * cardano_error_t result = cardano_update_set_epoch(update, epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The epoch has been successfully set
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_update_set_epoch(
  cardano_update_t* update,
  uint64_t          epoch);

/**
 * \brief Sets the proposed protocol parameters in the update object.
 *
 * This function sets the proposed protocol parameter updates in the given update object.
 * These parameters will take effect in the specified epoch if the proposal is accepted.
 *
 * \param[in] update Pointer to the update object.
 * \param[in] proposed_parameters Pointer to the proposed protocol parameter updates to set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the proposed parameters were successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_t* update = NULL;
 * cardano_proposed_param_updates_t* proposed_parameters = NULL;
 *
 * // Assume update and proposed_parameters are initialized properly
 *
 * cardano_error_t result = cardano_update_set_proposed_parameters(update, proposed_parameters);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The proposed parameters have been successfully set
 * }
 *
 * // Clean up
 * cardano_update_unref(&update);
 * cardano_proposed_param_updates_unref(&proposed_parameters);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_update_set_proposed_parameters(
  cardano_update_t*                 update,
  cardano_proposed_param_updates_t* proposed_parameters);

/**
 * \brief Decrements the reference count of a cardano_update_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_update_t object
 * by decreasing its reference count. When the reference count reaches zero, the update is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] update A pointer to the pointer of the update object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_t* update = cardano_update_new(major, minor);
 *
 * // Perform operations with the update...
 *
 * cardano_update_unref(&update);
 * // At this point, update is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_update_unref, the pointer to the \ref cardano_update_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_update_unref(cardano_update_t** update);

/**
 * \brief Increases the reference count of the cardano_update_t object.
 *
 * This function is used to manually increment the reference count of an cardano_update_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_update_unref.
 *
 * \param update A pointer to the cardano_update_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming update is a previously created update object
 *
 * cardano_update_ref(update);
 *
 * // Now update can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_update_ref there is a corresponding
 * call to \ref cardano_update_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_update_ref(cardano_update_t* update);

/**
 * \brief Retrieves the current reference count of the cardano_update_t object.
 *
 * This function returns the number of active references to an cardano_update_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_update_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param update A pointer to the cardano_update_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_update_t object. If the object
 * is properly managed (i.e., every \ref cardano_update_ref call is matched with a
 * \ref cardano_update_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming update is a previously created update object
 *
 * size_t ref_count = cardano_update_refcount(update);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_update_refcount(const cardano_update_t* update);

/**
 * \brief Sets the last error message for a given cardano_update_t object.
 *
 * Records an error message in the update's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] update A pointer to the \ref cardano_update_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the update's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_update_set_last_error(
  cardano_update_t* update,
  const char*       message);

/**
 * \brief Retrieves the last error message recorded for a specific update.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_update_set_last_error for the given
 * update. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] update A pointer to the \ref cardano_update_t instance whose last error
 *                   message is to be retrieved. If the update is NULL, the function
 *                   returns a generic error message indicating the null update.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified update. If the update is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_update_set_last_error for the same update, or until
 *       the update is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_update_get_last_error(
  const cardano_update_t* update);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UPDATE_H