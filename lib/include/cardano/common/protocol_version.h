/**
 * \file protocol_version.h
 *
 * \author angel.castillo
 * \date   Mar 07, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_VERSION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_VERSION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The protocol can be thought of as the set of rules that nodes in the network agree to follow,
 *  and this versioning system helps nodes to keep track of which set of rules they are adhering to and also
 *  allows for the decentralized updating of the protocol parameters without requiring a hard fork.
 */
typedef struct cardano_protocol_version_t cardano_protocol_version_t;

/**
 * \brief Creates and initializes a new instance of the Protocol Version.
 *
 * This function allocates and initializes a new instance of the Protocol Version,
 * representing a version of the Cardano protocol.
 *
 * \param[in] major The major version number, indicating significant alterations to the protocol
 *                  that are not backward compatible. Nodes would need to upgrade to continue
 *                  participating in the network.
 * \param[in] minor The minor version number, reflecting backward-compatible changes.
 * \param[out] protocol_version On successful initialization, this will point to a newly created
 *            Protocol Version object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the Protocol Version is no longer needed, the caller must release it
 *            by calling \ref cardano_protocol_version_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Protocol Version was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* protocol_version = NULL;
 * uint64_t major = 1; // Major version 1
 * uint64_t minor = 0; // Minor version 0
 *
 * // Attempt to create a new Protocol Version object
 * cardano_error_t result = cardano_protocol_version_new(major, minor, &protocol_version);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol_version
 *
 *   // Once done, ensure to clean up and release the protocol_version
 *   cardano_protocol_version_unref(&protocol_version);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_protocol_version_new(
  uint64_t                     major,
  uint64_t                     minor,
  cardano_protocol_version_t** protocol_version);

/**
 * \brief Creates a \ref cardano_protocol_version_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_protocol_version_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a protocol_version.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] protocol_version A pointer to a pointer of \ref cardano_protocol_version_t that will be set to the address
 *                        of the newly created protocol_version object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the protocol version were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_protocol_version_t object by calling
 *       \ref cardano_protocol_version_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_protocol_version_t* protocol_version = NULL;
 *
 * cardano_error_t result = cardano_protocol_version_from_cbor(reader, &protocol_version);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol_version
 *
 *   // Once done, ensure to clean up and release the protocol_version
 *   cardano_protocol_version_unref(&protocol_version);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode protocol_version: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_protocol_version_from_cbor(cardano_cbor_reader_t* reader, cardano_protocol_version_t** protocol_version);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_protocol_version_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] protocol_version A constant pointer to the \ref cardano_protocol_version_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p protocol_version or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* protocol_version = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_protocol_version_to_cbor(protocol_version, writer);
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
 * cardano_protocol_version_unref(&protocol_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_version_to_cbor(
  const cardano_protocol_version_t* protocol_version,
  cardano_cbor_writer_t*            writer);

/**
 * \brief Retrieves the major version number of the Protocol Version.
 *
 * This function returns the major version number of the Protocol Version,
 *
 * \param[in] protocol_version Pointer to the Protocol Version object.
 *
 * \return The major version number of the Protocol Version.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* protocol_version = NULL;
 * // Assume protocol_version is initialized properly
 *
 * uint64_t major_version = cardano_protocol_version_get_major(protocol_version);
 * printf("Major Version: %lu\n", major_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_version_get_major(
  const cardano_protocol_version_t* protocol_version);

/**
 * \brief Sets the major version number of the Protocol Version.
 *
 * This function sets the major version number of the Protocol Version,
 *
 * \param[in] protocol_version Pointer to the Protocol Version object.
 * \param[in] major The major version number to set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the major version number was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* protocol_version = NULL;
 * // Assume protocol_version is initialized properly
 *
 * cardano_error_t result = cardano_protocol_version_set_major(protocol_version, 2);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Major version set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set major version: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_version_set_major(
  cardano_protocol_version_t* protocol_version,
  uint64_t                    major);

/**
 * \brief Retrieves the minor version number of the Protocol Version.
 *
 * This function returns the minor version number of the Protocol Version.
 *
 * \param[in] protocol_version Pointer to the Protocol Version object.
 *
 * \return The minor version number of the Protocol Version.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* protocol_version = NULL;
 * // Assume protocol_version is initialized properly
 *
 * uint64_t minor_version = cardano_protocol_version_get_minor(protocol_version);
 * printf("Minor Version: %lu\n", minor_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_version_get_minor(const cardano_protocol_version_t* protocol_version);

/**
 * \brief Sets the minor version number of the Protocol Version.
 *
 * This function sets the minor version number of the Protocol Version.
 *
 * \param[in] protocol_version Pointer to the Protocol Version object.
 * \param[in] minor The minor version number to set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minor version number was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* protocol_version = NULL;
 * // Assume protocol_version is initialized properly
 *
 * cardano_error_t result = cardano_protocol_version_set_minor(protocol_version, 1);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minor version set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set minor version: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_version_set_minor(
  cardano_protocol_version_t* protocol_version,
  uint64_t                    minor);

/**
 * \brief Decrements the reference count of a cardano_protocol_version_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_protocol_version_t object
 * by decreasing its reference count. When the reference count reaches zero, the protocol_version is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] protocol_version A pointer to the pointer of the protocol_version object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_version_t* protocol_version = cardano_protocol_version_new(major, minor);
 *
 * // Perform operations with the protocol_version...
 *
 * cardano_protocol_version_unref(&protocol_version);
 * // At this point, protocol_version is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_protocol_version_unref, the pointer to the \ref cardano_protocol_version_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_protocol_version_unref(cardano_protocol_version_t** protocol_version);

/**
 * \brief Increases the reference count of the cardano_protocol_version_t object.
 *
 * This function is used to manually increment the reference count of an cardano_protocol_version_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_protocol_version_unref.
 *
 * \param protocol_version A pointer to the cardano_protocol_version_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming protocol_version is a previously created protocol_version object
 *
 * cardano_protocol_version_ref(protocol_version);
 *
 * // Now protocol_version can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_protocol_version_ref there is a corresponding
 * call to \ref cardano_protocol_version_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_protocol_version_ref(cardano_protocol_version_t* protocol_version);

/**
 * \brief Retrieves the current reference count of the cardano_protocol_version_t object.
 *
 * This function returns the number of active references to an cardano_protocol_version_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_protocol_version_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param protocol_version A pointer to the cardano_protocol_version_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_protocol_version_t object. If the object
 * is properly managed (i.e., every \ref cardano_protocol_version_ref call is matched with a
 * \ref cardano_protocol_version_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming protocol_version is a previously created protocol_version object
 *
 * size_t ref_count = cardano_protocol_version_refcount(protocol_version);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_protocol_version_refcount(const cardano_protocol_version_t* protocol_version);

/**
 * \brief Sets the last error message for a given cardano_protocol_version_t object.
 *
 * Records an error message in the protocol_version's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] protocol_version A pointer to the \ref cardano_protocol_version_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the protocol_version's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_protocol_version_set_last_error(
  cardano_protocol_version_t* protocol_version,
  const char*                 message);

/**
 * \brief Retrieves the last error message recorded for a specific protocol_version.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_protocol_version_set_last_error for the given
 * protocol_version. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] protocol_version A pointer to the \ref cardano_protocol_version_t instance whose last error
 *                   message is to be retrieved. If the protocol_version is NULL, the function
 *                   returns a generic error message indicating the null protocol_version.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified protocol_version. If the protocol_version is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_protocol_version_set_last_error for the same protocol_version, or until
 *       the protocol_version is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_protocol_version_get_last_error(
  const cardano_protocol_version_t* protocol_version);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_VERSION_H