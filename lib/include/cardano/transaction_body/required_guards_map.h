/**
 * \file required_guards_map.h
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_REQUIRED_GUARDS_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_REQUIRED_GUARDS_MAP_H

/* INCLUDES ******************************************************************/

#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/proposal_procedures/credential_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of credentials to an optional plutus datum.
 *
 * A sub transaction uses this map to restrict which top-level guards must be present;
 * a top-level transaction body may also carry it as a self assertion. Each entry maps
 * a credential to either a plutus datum or to no datum at all (encoded as CBOR null
 * on the wire).
 */
typedef struct cardano_required_guards_map_t cardano_required_guards_map_t;

/**
 * \brief Creates and initializes a new instance of a required guards map.
 *
 * This function allocates and initializes a new instance of \ref cardano_required_guards_map_t,
 * representing a map structure. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] required_guards_map A pointer to a pointer to a \ref cardano_required_guards_map_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_required_guards_map_t
 *                        object. This object represents a "strong reference" to the required_guards_map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the required_guards_map is no longer
 *                        needed, the caller must release it by calling \ref cardano_required_guards_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the required_guards_map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = NULL;
 *
 * // Attempt to create a new required_guards_map
 * cardano_error_t result = cardano_required_guards_map_new(&required_guards_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the required_guards_map
 *
 *   // Once done, ensure to clean up and release the required_guards_map
 *   cardano_required_guards_map_unref(&required_guards_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_required_guards_map_new(cardano_required_guards_map_t** required_guards_map);

/**
 * \brief Creates a required guards map from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_required_guards_map_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a required guards map.
 *
 * The map must not be empty on the wire; decoding an empty map fails with
 * \ref CARDANO_ERROR_INVALID_CBOR_MAP_SIZE. An entry whose value is CBOR null is decoded
 * as a credential without a datum.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded required guards map data.
 * \param[out] required_guards_map A pointer to a pointer of \ref cardano_required_guards_map_t that will be set to the address
 *                        of the newly created required guards map object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the required_guards_map was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_required_guards_map_t object by calling
 *       \ref cardano_required_guards_map_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_required_guards_map_t* required_guards_map = NULL;
 *
 * cardano_error_t result = cardano_required_guards_map_from_cbor(reader, &required_guards_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the required_guards_map
 *
 *   // Once done, ensure to clean up and release the required_guards_map
 *   cardano_required_guards_map_unref(&required_guards_map);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode required_guards_map: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_required_guards_map_from_cbor(cardano_cbor_reader_t* reader, cardano_required_guards_map_t** required_guards_map);

/**
 * \brief Serializes a required guards map into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_required_guards_map_t object using a \ref cardano_cbor_writer_t.
 * Entries are written in insertion order. An entry without a datum is written with a CBOR null value.
 *
 * \param[in] required_guards_map A constant pointer to the \ref cardano_required_guards_map_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p required_guards_map or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_required_guards_map_to_cbor(required_guards_map, writer);
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
 * cardano_required_guards_map_unref(&required_guards_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_required_guards_map_to_cbor(
  const cardano_required_guards_map_t* required_guards_map,
  cardano_cbor_writer_t*               writer);

/**
 * \brief Retrieves the length of the required_guards_map.
 *
 * This function returns the number of key-value pairs contained in the specified required_guards_map.
 *
 * \param[in] required_guards_map A constant pointer to the \ref cardano_required_guards_map_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the required_guards_map. Returns 0 if the required_guards_map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = NULL;
 * cardano_error_t result = cardano_required_guards_map_new(&required_guards_map);
 *
 * // Populate the required_guards_map with key-value pairs
 *
 * size_t length = cardano_required_guards_map_get_length(required_guards_map);
 * printf("The length of the required_guards_map is: %zu\n", length);
 *
 * cardano_required_guards_map_unref(&required_guards_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_required_guards_map_get_length(const cardano_required_guards_map_t* required_guards_map);

/**
 * \brief Retrieves the datum associated with a given credential in the required guards map.
 *
 * This function retrieves the datum associated with the specified credential in the provided required guards map.
 * It returns the datum through the output parameter `element`. Entries without a datum yield NULL through
 * `element` while still returning \ref CARDANO_SUCCESS. If the credential is not found in the map, the
 * function returns \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \param[in] required_guards_map A constant pointer to the \ref cardano_required_guards_map_t object from which
 *                       the datum is to be retrieved.
 * \param[in] key The credential whose associated datum is to be retrieved from the required_guards_map.
 * \param[out] element A pointer to a variable where the retrieved datum will be stored. If the credential
 *                     is found and has a datum, this variable will be set to the associated datum, and the
 *                     caller must release it by calling \ref cardano_plutus_data_unref. If the credential
 *                     is found but has no datum, this variable will be set to NULL.
 *
 * \return \ref CARDANO_SUCCESS if the credential was found, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if the
 *         credential is not present in the map, or an appropriate error code if the input parameters are invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = ...;
 * cardano_credential_t* key = ...; // Create a credential object representing the key
 * cardano_plutus_data_t* datum = NULL;
 *
 * cardano_error_t result = cardano_required_guards_map_get(required_guards_map, key, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (datum != NULL)
 *   {
 *     // Use the retrieved datum
 *     cardano_plutus_data_unref(&datum);
 *   }
 * }
 * else
 * {
 *   // Handle error or key not found
 * }
 *
 * cardano_credential_unref(&key); // Clean up the key resource
 * cardano_required_guards_map_unref(&required_guards_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_required_guards_map_get(
  const cardano_required_guards_map_t* required_guards_map,
  cardano_credential_t*                key,
  cardano_plutus_data_t**              element);

/**
 * \brief Inserts a key-value pair into the required guards map.
 *
 * This function inserts the specified credential and its optional datum into the provided required guards map.
 * Entries keep their insertion order when the map is serialized. Inserting a credential that is already
 * present in the map fails with \ref CARDANO_ERROR_DUPLICATED_KEY.
 *
 * \param[in] required_guards_map A constant pointer to the \ref cardano_required_guards_map_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The credential to be inserted into the required guards map. The caller is responsible for managing
 *                the lifecycle of the key object.
 * \param[in] value The datum to be associated with the credential, or NULL if the entry has no datum.
 *                  The caller is responsible for managing the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = NULL;
 * cardano_error_t result = cardano_required_guards_map_new(&required_guards_map);
 *
 * // Create key and value objects
 * cardano_credential_t* key = ...;
 * cardano_plutus_data_t* value = ...; // May be NULL for an entry without a datum
 *
 * // Insert the key-value pair into the required_guards_map
 * result = cardano_required_guards_map_insert(required_guards_map, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value objects
 * cardano_credential_unref(&key);
 * cardano_plutus_data_unref(&value);
 *
 * cardano_required_guards_map_unref(&required_guards_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_required_guards_map_insert(
  cardano_required_guards_map_t* required_guards_map,
  cardano_credential_t*          key,
  cardano_plutus_data_t*         value);

/**
 * \brief Retrieves the keys from the required guards map.
 *
 * This function retrieves all the keys from the provided required guards map and returns them as a set.
 * The caller is responsible for managing the lifecycle of the returned set by calling
 * \ref cardano_credential_set_unref when it is no longer needed.
 *
 * \note The returned set is canonically ordered (by credential type, then hash bytes) and may not
 *       match the map's insertion-order iteration via the index based accessors.
 *
 * \param[in] required_guards_map A pointer to the \ref cardano_required_guards_map_t object from which
 *                       the keys are to be retrieved.
 * \param[out] keys A pointer to a variable where the retrieved keys will be stored as a set.
 *                  If successful, this variable will be set to point to the set of keys.
 *                  The caller is responsible for managing the lifecycle of this set.
 *                  It must be released by calling \ref cardano_credential_set_unref when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the keys were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = NULL;
 * cardano_error_t result = cardano_required_guards_map_new(&required_guards_map);
 *
 * // Populate the required_guards_map with key-value pairs
 *
 * cardano_credential_set_t* keys = NULL;
 * result = cardano_required_guards_map_get_keys(required_guards_map, &keys);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the set of keys
 *   // Keys must also be freed if retrieved from the set
 *
 *   // Once done, ensure to clean up and release the keys set
 *   cardano_credential_set_unref(&keys);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_required_guards_map_unref(&required_guards_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_required_guards_map_get_keys(
  cardano_required_guards_map_t* required_guards_map,
  cardano_credential_set_t**     keys);

/**
 * \brief Retrieves the credential at a specific index from the required guards map.
 *
 * This function retrieves the credential at the specified index from the required guards map.
 *
 * \param[in] required_guards_map Pointer to the required guards map object.
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
 * cardano_required_guards_map_t* required_guards_map = NULL;
 * cardano_credential_t* credential = NULL;
 * size_t index = 0; // Index of the credential to retrieve
 *
 * // Assume required_guards_map is initialized properly
 *
 * cardano_error_t result = cardano_required_guards_map_get_key_at(required_guards_map, index, &credential);
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
CARDANO_EXPORT cardano_error_t cardano_required_guards_map_get_key_at(
  const cardano_required_guards_map_t* required_guards_map,
  size_t                               index,
  cardano_credential_t**               credential);

/**
 * \brief Retrieves the datum at a specific index from the required guards map.
 *
 * This function retrieves the datum at the specified index from the required guards map.
 * Entries without a datum yield NULL through the output parameter.
 *
 * \param[in] required_guards_map Pointer to the required guards map object.
 * \param[in] index The index of the datum to retrieve.
 * \param[out] datum On successful retrieval, this will point to the datum at the specified index,
 *                   or be set to NULL if the entry has no datum. When a datum is returned, the caller
 *                   is responsible for managing its lifecycle and must release it by calling
 *                   \ref cardano_plutus_data_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the datum was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = NULL;
 * cardano_plutus_data_t* datum = NULL;
 * size_t index = 0; // Index of the datum to retrieve
 *
 * // Assume required_guards_map is initialized properly
 *
 * cardano_error_t result = cardano_required_guards_map_get_value_at(required_guards_map, index, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (datum != NULL)
 *   {
 *     // Use the datum
 *     cardano_plutus_data_unref(&datum);
 *   }
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_required_guards_map_get_value_at(
  const cardano_required_guards_map_t* required_guards_map,
  size_t                               index,
  cardano_plutus_data_t**              datum);

/**
 * \brief Retrieves the credential and datum at the specified index.
 *
 * This function retrieves the credential and its optional datum from the required guards map
 * at the specified index.
 *
 * \param[in]  required_guards_map    Pointer to the required guards map object.
 * \param[in]  index             The index at which to retrieve the key-value pair.
 * \param[out] credential On successful retrieval, this will point to the credential at the specified index.
 *                            The caller is responsible for managing the lifecycle of this object and should release it using
 *                            \ref cardano_credential_unref when it is no longer needed.
 * \param[out] datum On successful retrieval, this will point to the datum at the specified index, or be set
 *                   to NULL if the entry has no datum. When a datum is returned, the caller is responsible for
 *                   managing its lifecycle and must release it by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = NULL;
 * // Assume required_guards_map is initialized properly
 *
 * size_t index = 0;
 * cardano_credential_t* credential = NULL;
 * cardano_plutus_data_t* datum = NULL;
 *
 * cardano_error_t result = cardano_required_guards_map_get_key_value_at(required_guards_map, index, &credential, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential and the datum (the datum is NULL for entries without one)
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get key-value pair at index %zu.\n", index);
 * }
 *
 * // Clean up
 * cardano_required_guards_map_unref(&required_guards_map);
 * cardano_credential_unref(&credential);
 * cardano_plutus_data_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_required_guards_map_get_key_value_at(
  const cardano_required_guards_map_t* required_guards_map,
  size_t                               index,
  cardano_credential_t**               credential,
  cardano_plutus_data_t**              datum);

/**
 * \brief Decrements the reference count of a required_guards_map object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_required_guards_map_t object
 * by decreasing its reference count. When the reference count reaches zero, the required_guards_map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] required_guards_map A pointer to the pointer of the required_guards_map object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_required_guards_map_t* required_guards_map = NULL;
 * cardano_error_t result = cardano_required_guards_map_new(&required_guards_map);
 *
 * // Perform operations with the required_guards_map...
 *
 * cardano_required_guards_map_unref(&required_guards_map);
 * // At this point, required_guards_map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_required_guards_map_unref, the pointer to the \ref cardano_required_guards_map_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_required_guards_map_unref(cardano_required_guards_map_t** required_guards_map);

/**
 * \brief Increases the reference count of the cardano_required_guards_map_t object.
 *
 * This function is used to manually increment the reference count of a required_guards_map
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_required_guards_map_unref.
 *
 * \param required_guards_map A pointer to the required_guards_map object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming required_guards_map is a previously created required_guards_map object
 *
 * cardano_required_guards_map_ref(required_guards_map);
 *
 * // Now required_guards_map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_required_guards_map_ref there is a corresponding
 * call to \ref cardano_required_guards_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_required_guards_map_ref(cardano_required_guards_map_t* required_guards_map);

/**
 * \brief Retrieves the current reference count of the cardano_required_guards_map_t object.
 *
 * This function returns the number of active references to a required_guards_map object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_required_guards_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param required_guards_map A pointer to the required_guards_map object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified required_guards_map object. If the object
 * is properly managed (i.e., every \ref cardano_required_guards_map_ref call is matched with a
 * \ref cardano_required_guards_map_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming required_guards_map is a previously created required_guards_map object
 *
 * size_t ref_count = cardano_required_guards_map_refcount(required_guards_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_required_guards_map_refcount(const cardano_required_guards_map_t* required_guards_map);

/**
 * \brief Sets the last error message for a given required_guards_map object.
 *
 * Records an error message in the required_guards_map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] required_guards_map A pointer to the \ref cardano_required_guards_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the required_guards_map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_required_guards_map_set_last_error(cardano_required_guards_map_t* required_guards_map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific required_guards_map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_required_guards_map_set_last_error for the given
 * required_guards_map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] required_guards_map A pointer to the \ref cardano_required_guards_map_t instance whose last error
 *                   message is to be retrieved. If the required_guards_map is NULL, the function
 *                   returns a generic error message indicating the null required_guards_map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified required_guards_map. If the required_guards_map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_required_guards_map_set_last_error for the same required_guards_map, or until
 *       the required_guards_map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_required_guards_map_get_last_error(const cardano_required_guards_map_t* required_guards_map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_REQUIRED_GUARDS_MAP_H
