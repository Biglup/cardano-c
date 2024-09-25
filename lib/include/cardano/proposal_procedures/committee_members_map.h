/**
 * \file committee_members_map.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COMMITTEE_MEMBERS_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COMMITTEE_MEMBERS_MAP_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/credential_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of committee members to epoch.
 */
typedef struct cardano_committee_members_map_t cardano_committee_members_map_t;

/**
 * \brief Creates and initializes a new instance of a committee members map.
 *
 * This function allocates and initializes a new instance of \ref cardano_committee_members_map_t,
 * representing a map structure. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] committee_members_map A pointer to a pointer to a \ref cardano_committee_members_map_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_committee_members_map_t
 *                        object. This object represents a "strong reference" to the committee_members_map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the committee_members_map is no longer
 *                        needed, the caller must release it by calling \ref cardano_committee_members_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the committee_members_map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = NULL;
 *
 * // Attempt to create a new committee_members_map
 * cardano_error_t result = cardano_committee_members_map_new(&committee_members_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the committee_members_map
 *
 *   // Once done, ensure to clean up and release the committee_members_map
 *   cardano_committee_members_map_unref(&committee_members_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_committee_members_map_new(cardano_committee_members_map_t** committee_members_map);

/**
 * \brief Creates a committee_members set from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_committee_members_map_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a committee_members.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded committee_members data.
 * \param[out] committee_members A pointer to a pointer of \ref cardano_committee_members_map_t that will be set to the address
 *                        of the newly created committee_members object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee_members was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_committee_members_map_t object by calling
 *       \ref cardano_committee_members_map_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_committee_members_map_t* committee_members = NULL;
 *
 * cardano_error_t result = cardano_committee_members_map_from_cbor(reader, &committee_members);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the committee_members
 *
 *   // Once done, ensure to clean up and release the committee_members
 *   cardano_committee_members_map_unref(&committee_members);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode committee_members: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_committee_members_map_from_cbor(cardano_cbor_reader_t* reader, cardano_committee_members_map_t** committee_members);

/**
 * \brief Serializes a committee_members into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_committee_members_map_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] committee_members A constant pointer to the \ref cardano_committee_members_map_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p committee_members or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_committee_members_map_to_cbor(committee_members, writer);
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
 * cardano_committee_members_map_unref(&committee_members);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_members_map_to_cbor(
  const cardano_committee_members_map_t* committee_members,
  cardano_cbor_writer_t*                 writer);

/**
 * \brief Retrieves the length of the committee_members_map.
 *
 * This function returns the number of key-value pairs contained in the specified committee_members_map.
 *
 * \param[in] committee_members_map A constant pointer to the \ref cardano_committee_members_map_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the committee_members_map. Returns 0 if the committee_members_map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = cardano_committee_members_map_new();
 *
 * // Populate the committee_members_map with key-value pairs
 *
 * size_t length = cardano_committee_members_map_get_length(committee_members_map);
 * printf("The length of the committee_members_map is: %zu\n", length);
 *
 * cardano_committee_members_map_unref(&committee_members_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_committee_members_map_get_length(const cardano_committee_members_map_t* committee_members_map);

/**
 * \brief Retrieves the value associated with a given key in the committee members map.
 *
 * This function retrieves the value associated with the specified key in the provided committee members map.
 * It returns the value through the output parameter `element`. If the key is not found in the committee members map,
 * the output parameter `element` will be set to NULL.
 *
 * \param[in] committee_members_map A constant pointer to the \ref cardano_committee_members_map_t object from which
 *                       the value is to be retrieved.
 * \param[in] key The key whose associated value is to be retrieved from the committee_members_map.
 * \param[out] element A pointer to a variable where the retrieved value will be stored. If the key
 *                     is found in the committee members map, this variable will be set to the associated value.
 *                     If the key is not found, this variable will be set to NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value associated with the key was successfully retrieved, or
 *         an appropriate error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = cardano_committee_members_map_new();
 *
 * // Populate the committee_members_map with key-value pairs
 *
 * cardano_credential_t* key = ...; // Create a credential object representing the key
 * uint64_t value = 0;
 * cardano_error_t result = cardano_committee_members_map_get(committee_members_map, key, &value);
 *
 * if (result == CARDANO_SUCCESS && value != NULL)
 * {
 *   // Use the retrieved value
 *   // ...
 * }
 * else
 * {
 *   // Handle error or key not found
 * }
 *
 * cardano_credential_unref(&key); // Clean up the key resource
 * cardano_committee_members_map_unref(&committee_members_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_members_map_get(cardano_committee_members_map_t* committee_members_map, cardano_credential_t* key, uint64_t* element);

/**
 * \brief Inserts a key-value pair into the committee members map.
 *
 * This function inserts the specified key-value pair into the provided committee members map.
 *
 * \param[in] committee_members_map A constant pointer to the \ref cardano_committee_members_map_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The key to be inserted into the committee members map. The caller is responsible for managing
 *                the lifecycle of the key object.
 * \param[in] value The value to be associated with the key and inserted into the committee_members_map.
 *                  The caller is responsible for managing the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = cardano_committee_members_map_new();
 *
 * // Create key and value objects
 * cardano_credential_t* key = ...;
 * uint64_t value = 0;
 *
 * // Insert the key-value pair into the committee_members_map
 * cardano_error_t result = cardano_committee_members_map_insert(committee_members_map, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value objects
 * cardano_credential_unref(&key);
 *
 * cardano_committee_members_map_unref(&committee_members_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_members_map_insert(cardano_committee_members_map_t* committee_members_map, cardano_credential_t* key, uint64_t value);

/**
 * \brief Retrieves the keys from the committee members map.
 *
 * This function retrieves all the keys from the provided committee members map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_credential_set_t when it is no longer needed.
 *
 * \param[in] committee_members_map A constant pointer to the \ref cardano_committee_members_map_t object from which
 *                       the keys are to be retrieved.
 * \param[out] keys A pointer to a variable where the retrieved keys will be stored as a list.
 *                  If successful, this variable will be set to point to the list of keys.
 *                  The caller is responsible for managing the lifecycle of this list.
 *                  It must be released by calling \ref cardano_credential_set_t when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the keys were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = cardano_committee_members_map_new();
 *
 * // Populate the committee_members_map with key-value pairs
 *
 * cardano_credential_set_t* keys = NULL;
 * cardano_error_t result = cardano_committee_members_map_get_keys(committee_members_map, &keys);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list of keys
 *   // Keys must also be freed if retrieved from the list
 *
 *   // Once done, ensure to clean up and release the keys list
 *   cardano_credential_list_unref(&keys);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_committee_members_map_unref(&committee_members_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_members_map_get_keys(cardano_committee_members_map_t* committee_members_map, cardano_credential_set_t** keys);

/**
 * \brief Retrieves the credential at a specific index from the committee_members map.
 *
 * This function retrieves the credential at the specified index from the committee members map.
 *
 * \param[in] committee_members_map Pointer to the committee members map object.
 * \param[in] index The index of the credential to retrieve.
 * \param[out] credential On successful retrieval, this will point to the credential
 *                            at the specified index. The caller is responsible for managing the lifecycle
 *                            of this object. Specifically, once the credential is no longer needed,
 *                            the caller must release it by calling \ref cardano_credential_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the credential was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = NULL;
 * cardano_credential_t* credential = NULL;
 * size_t index = 0; // Index of the credential to retrieve
 *
 * // Assume committee_members_map is initialized properly
 *
 * cardano_error_t result = cardano_committee_members_map_get_key_at(committee_members_map, index, &credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_members_map_get_key_at(
  const cardano_committee_members_map_t* committee_members_map,
  size_t                                 index,
  cardano_credential_t**                 credential);

/**
 * \brief Retrieves the committee member epoch at a specific index from the committee members map.
 *
 * This function retrieves the committee member epoch at the specified index from the committee_members map.
 *
 * \param[in] committee_members_map Pointer to the committee members map object.
 * \param[in] index The index of the committee member epoch to retrieve.
 * \param[out] epoch On successful retrieval, this will point to the committee member epoch
 *                    at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee member epoch was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = NULL;
 * uint64_t epoch = 0;
 * size_t index = 0; // Index of the committee member epoch to retrieve
 *
 * // Assume committee_members_map is initialized properly
 *
 * cardano_error_t result = cardano_committee_members_map_get_value_at(committee_members_map, index, &epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the epoch
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_members_map_get_value_at(
  const cardano_committee_members_map_t* committee_members_map,
  size_t                                 index,
  uint64_t*                              epoch);

/**
 * \brief Retrieves the credential and committee member epoch at the specified index.
 *
 * This function retrieves the credential and committee member epoch from the proposed committee member epochs
 * at the specified index.
 *
 * \param[in]  committee_members_map    Pointer to the proposed committee member epochs object.
 * \param[in]  index             The index at which to retrieve the key-value pair.
 * \param[out] credential On successful retrieval, this will point to the credential at the specified index.
 *                            The caller is responsible for managing the lifecycle of this object and should release it using
 *                            \ref cardano_credential_unref when it is no longer needed.
 * \param[out] epoch On successful retrieval, this will point to the committee member epoch at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = NULL;
 * // Assume committee_members_map is initialized properly
 *
 * size_t index = 0;
 * cardano_credential_t* credential = NULL;
 * uint64_t epoch = 0;
 *
 * cardano_error_t result = cardano_committee_members_map_get_key_value_at(committee_members_map, index, &credential, &epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (credential != NULL && epoch != NULL)
 *   {
 *     // Use the credential and committee member epoch
 *   }
 *   else
 *   {
 *     printf("Key-value pair not set at index %zu.\n", index);
 *   }
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get key-value pair at index %zu.\n", index);
 * }
 *
 * // Clean up
 * cardano_committee_members_map_unref(&committee_members_map);
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_members_map_get_key_value_at(
  const cardano_committee_members_map_t* committee_members_map,
  size_t                                 index,
  cardano_credential_t**                 credential,
  uint64_t*                              epoch);

/**
 * \brief Decrements the reference count of a committee_members_map object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_committee_members_map_t object
 * by decreasing its reference count. When the reference count reaches zero, the committee_members_map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] committee_members_map A pointer to the pointer of the committee_members_map object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_members_map_t* committee_members_map = cardano_committee_members_map_new();
 *
 * // Perform operations with the committee_members_map...
 *
 * cardano_committee_members_map_unref(&committee_members_map);
 * // At this point, committee_members_map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_committee_members_map_unref, the pointer to the \ref cardano_committee_members_map_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_committee_members_map_unref(cardano_committee_members_map_t** committee_members_map);

/**
 * \brief Increases the reference count of the cardano_committee_members_map_t object.
 *
 * This function is used to manually increment the reference count of a committee_members_map
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_committee_members_map_unref.
 *
 * \param committee_members_map A pointer to the committee_members_map object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming committee_members_map is a previously created committee_members_map object
 *
 * cardano_committee_members_map_ref(committee_members_map);
 *
 * // Now committee_members_map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_committee_members_map_ref there is a corresponding
 * call to \ref cardano_committee_members_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_committee_members_map_ref(cardano_committee_members_map_t* committee_members_map);

/**
 * \brief Retrieves the current reference count of the cardano_committee_members_map_t object.
 *
 * This function returns the number of active references to a committee_members_map object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_committee_members_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param committee_members_map A pointer to the committee_members_map object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified committee_members_map object. If the object
 * is properly managed (i.e., every \ref cardano_committee_members_map_ref call is matched with a
 * \ref cardano_committee_members_map_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming committee_members_map is a previously created committee_members_map object
 *
 * size_t ref_count = cardano_committee_members_map_refcount(committee_members_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_committee_members_map_refcount(const cardano_committee_members_map_t* committee_members_map);

/**
 * \brief Sets the last error message for a given committee_members_map object.
 *
 * Records an error message in the committee_members_map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] committee_members_map A pointer to the \ref cardano_committee_members_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the committee_members_map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_committee_members_map_set_last_error(cardano_committee_members_map_t* committee_members_map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific committee_members_map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_committee_members_map_set_last_error for the given
 * committee_members_map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] committee_members_map A pointer to the \ref cardano_committee_members_map_t instance whose last error
 *                   message is to be retrieved. If the committee_members_map is NULL, the function
 *                   returns a generic error message indicating the null committee_members_map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified committee_members_map. If the committee_members_map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_committee_members_map_set_last_error for the same committee_members_map, or until
 *       the committee_members_map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_committee_members_map_get_last_error(const cardano_committee_members_map_t* committee_members_map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COMMITTEE_MEMBERS_MAP_H