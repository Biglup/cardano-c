/**
 * \file register_drep_cert.h
 *
 * \author angel.castillo
 * \date   Jul 23, 2024
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

#ifndef REGISTER_DREP_CERT_H
#define REGISTER_DREP_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/mir_cert_pot_type.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief In Voltaire, existing stake credentials will be able to delegate their stake to DReps for voting
 * purposes, in addition to the current delegation to stake pools for block production.
 * DRep delegation will mimic the existing stake delegation mechanisms (via on-chain certificates).
 *
 * This certificate register a stake key as a DRep.
 */
typedef struct cardano_register_drep_cert_t cardano_register_drep_cert_t;

/**
 * \brief Creates a \ref cardano_register_drep_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_register_drep_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a auth_committee_hot.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] auth_committee_hot A pointer to a pointer of \ref cardano_register_drep_cert_t that will be set to the address
 *                        of the newly created auth_committee_hot object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_register_drep_cert_t object by calling
 *       \ref cardano_auth_committee_hot_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_register_drep_cert_t* auth_committee_hot = NULL;
 *
 * cardano_error_t result = cardano_auth_committee_hot_from_cbor(reader, &auth_committee_hot);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the auth_committee_hot
 *
 *   // Once done, ensure to clean up and release the auth_committee_hot
 *   cardano_auth_committee_hot_unref(&auth_committee_hot);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode auth_committee_hot: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auth_committee_hot_from_cbor(cardano_cbor_reader_t* reader, cardano_register_drep_cert_t** auth_committee_hot);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_register_drep_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] auth_committee_hot A constant pointer to the \ref cardano_register_drep_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p auth_committee_hot or \p writer
 *         is NULL, returns \ref CARDANO_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* auth_committee_hot = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_register_drep_cert_to_cbor(auth_committee_hot, writer);
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
 * cardano_auth_committee_hot_unref(&auth_committee_hot);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_register_drep_cert_to_cbor(
  const cardano_register_drep_cert_t* auth_committee_hot,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Decrements the reference count of a cardano_register_drep_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_register_drep_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the auth_committee_hot is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] auth_committee_hot A pointer to the pointer of the auth_committee_hot object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* auth_committee_hot = cardano_auth_committee_hot_new(major, minor);
 *
 * // Perform operations with the auth_committee_hot...
 *
 * cardano_auth_committee_hot_unref(&auth_committee_hot);
 * // At this point, auth_committee_hot is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_auth_committee_hot_unref, the pointer to the \ref cardano_register_drep_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_auth_committee_hot_unref(cardano_register_drep_cert_t** auth_committee_hot);

/**
 * \brief Increases the reference count of the cardano_register_drep_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_register_drep_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_auth_committee_hot_unref.
 *
 * \param auth_committee_hot A pointer to the cardano_register_drep_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * cardano_auth_committee_hot_ref(auth_committee_hot);
 *
 * // Now auth_committee_hot can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_auth_committee_hot_ref there is a corresponding
 * call to \ref cardano_auth_committee_hot_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_auth_committee_hot_ref(cardano_register_drep_cert_t* auth_committee_hot);

/**
 * \brief Retrieves the current reference count of the cardano_register_drep_cert_t object.
 *
 * This function returns the number of active references to an cardano_register_drep_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_register_drep_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param auth_committee_hot A pointer to the cardano_register_drep_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_register_drep_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_auth_committee_hot_ref call is matched with a
 * \ref cardano_auth_committee_hot_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * size_t ref_count = cardano_auth_committee_hot_refcount(auth_committee_hot);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_auth_committee_hot_refcount(const cardano_register_drep_cert_t* auth_committee_hot);

/**
 * \brief Sets the last error message for a given cardano_register_drep_cert_t object.
 *
 * Records an error message in the auth_committee_hot's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_register_drep_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the auth_committee_hot's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_auth_committee_hot_set_last_error(
  cardano_register_drep_cert_t* auth_committee_hot,
  const char*                     message);

/**
 * \brief Retrieves the last error message recorded for a specific auth_committee_hot.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_auth_committee_hot_set_last_error for the given
 * auth_committee_hot. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_register_drep_cert_t instance whose last error
 *                   message is to be retrieved. If the auth_committee_hot is NULL, the function
 *                   returns a generic error message indicating the null auth_committee_hot.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified auth_committee_hot. If the auth_committee_hot is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_auth_committee_hot_set_last_error for the same auth_committee_hot, or until
 *       the auth_committee_hot is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_auth_committee_hot_get_last_error(
  const cardano_register_drep_cert_t* auth_committee_hot);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REGISTER_DREP_CERT_H