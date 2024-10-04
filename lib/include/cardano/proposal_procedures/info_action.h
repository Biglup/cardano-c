/**
 * \file info_action.h
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

#ifndef INFO_ACTION_H
#define INFO_ACTION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents an action that has no direct effect on the blockchain,
 * but serves as an on-chain record or informative notice.
 */
typedef struct cardano_info_action_t cardano_info_action_t;

/**
 * \brief Creates and initializes a new instance of an Info Action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_info_action_t object,
 * which represents an informative action within the Cardano network. This type of action serves as an on-chain
 * record or notice without directly affecting the blockchain's state.
 *
 * \param[out] info_action On successful initialization, this will point to a newly created
 *             \ref cardano_info_action_t object. This object represents a "strong reference"
 *             to the info action, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the info action is no longer needed, the caller must release it
 *             by calling \ref cardano_info_action_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the info action was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_info_action_t* info_action = NULL;
 * cardano_error_t result = cardano_info_action_new(&info_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the info action as an on-chain notice
 *   // Free resources when done
 *   cardano_info_action_unref(&info_action);
 * }
 * else
 * {
 *   printf("Failed to create the info action: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_info_action_new(cardano_info_action_t** info_action);

/**
 * \brief Creates a \ref cardano_info_action_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_info_action_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a info_action.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] info_action A pointer to a pointer of \ref cardano_info_action_t that will be set to the address
 *                        of the newly created info_action object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_info_action_t object by calling
 *       \ref cardano_info_action_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_info_action_t* info_action = NULL;
 *
 * cardano_error_t result = cardano_info_action_from_cbor(reader, &info_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the info_action
 *
 *   // Once done, ensure to clean up and release the info_action
 *   cardano_info_action_unref(&info_action);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode info_action: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_info_action_from_cbor(cardano_cbor_reader_t* reader, cardano_info_action_t** info_action);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_info_action_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] info_action A constant pointer to the \ref cardano_info_action_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p info_action or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_info_action_t* info_action = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_info_action_to_cbor(info_action, writer);
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
 * cardano_info_action_unref(&info_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_info_action_to_cbor(
  const cardano_info_action_t* info_action,
  cardano_cbor_writer_t*       writer);

/**
 * \brief Decrements the reference count of a cardano_info_action_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_info_action_t object
 * by decreasing its reference count. When the reference count reaches zero, the info_action is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] info_action A pointer to the pointer of the info_action object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_info_action_t* info_action = cardano_info_action_new(major, minor);
 *
 * // Perform operations with the info_action...
 *
 * cardano_info_action_unref(&info_action);
 * // At this point, info_action is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_info_action_unref, the pointer to the \ref cardano_info_action_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_info_action_unref(cardano_info_action_t** info_action);

/**
 * \brief Increases the reference count of the cardano_info_action_t object.
 *
 * This function is used to manually increment the reference count of an cardano_info_action_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_info_action_unref.
 *
 * \param info_action A pointer to the cardano_info_action_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming info_action is a previously created info_action object
 *
 * cardano_info_action_ref(info_action);
 *
 * // Now info_action can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_info_action_ref there is a corresponding
 * call to \ref cardano_info_action_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_info_action_ref(cardano_info_action_t* info_action);

/**
 * \brief Retrieves the current reference count of the cardano_info_action_t object.
 *
 * This function returns the number of active references to an cardano_info_action_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_info_action_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param info_action A pointer to the cardano_info_action_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_info_action_t object. If the object
 * is properly managed (i.e., every \ref cardano_info_action_ref call is matched with a
 * \ref cardano_info_action_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming info_action is a previously created info_action object
 *
 * size_t ref_count = cardano_info_action_refcount(info_action);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_info_action_refcount(const cardano_info_action_t* info_action);

/**
 * \brief Sets the last error message for a given cardano_info_action_t object.
 *
 * Records an error message in the info_action's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] info_action A pointer to the \ref cardano_info_action_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the info_action's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_info_action_set_last_error(
  cardano_info_action_t* info_action,
  const char*            message);

/**
 * \brief Retrieves the last error message recorded for a specific info_action.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_info_action_set_last_error for the given
 * info_action. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] info_action A pointer to the \ref cardano_info_action_t instance whose last error
 *                   message is to be retrieved. If the info_action is NULL, the function
 *                   returns a generic error message indicating the null info_action.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified info_action. If the info_action is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_info_action_set_last_error for the same info_action, or until
 *       the info_action is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_info_action_get_last_error(
  const cardano_info_action_t* info_action);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // INFO_ACTION_H