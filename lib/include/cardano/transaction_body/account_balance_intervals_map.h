/**
 * \file account_balance_intervals_map.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_BALANCE_INTERVALS_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_BALANCE_INTERVALS_MAP_H

/* INCLUDES ******************************************************************/

#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/credential_set.h>
#include <cardano/transaction_body/account_balance_interval.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of credentials to account balance intervals.
 *
 * A transaction body carries this map to state, for each credential, a half open
 * interval of lovelace amounts. Each entry maps a credential to an
 * \ref cardano_account_balance_interval_t describing the interval bounds.
 */
typedef struct cardano_account_balance_intervals_map_t cardano_account_balance_intervals_map_t;

/**
 * \brief Creates and initializes a new instance of an account balance intervals map.
 *
 * This function allocates and initializes a new instance of \ref cardano_account_balance_intervals_map_t,
 * representing a map structure. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] account_balance_intervals_map A pointer to a pointer to a \ref cardano_account_balance_intervals_map_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_account_balance_intervals_map_t
 *                        object. This object represents a "strong reference" to the account_balance_intervals_map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the account_balance_intervals_map is no longer
 *                        needed, the caller must release it by calling \ref cardano_account_balance_intervals_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the account_balance_intervals_map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 *
 * // Attempt to create a new account_balance_intervals_map
 * cardano_error_t result = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the account_balance_intervals_map
 *
 *   // Once done, ensure to clean up and release the account_balance_intervals_map
 *   cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_account_balance_intervals_map_new(cardano_account_balance_intervals_map_t** account_balance_intervals_map);

/**
 * \brief Creates an account balance intervals map from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_account_balance_intervals_map_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for an account balance intervals map.
 *
 * The map must not be empty on the wire; decoding an empty map fails with
 * \ref CARDANO_ERROR_INVALID_CBOR_MAP_SIZE. Duplicated credentials fail with
 * \ref CARDANO_ERROR_DUPLICATED_KEY.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded account balance intervals map data.
 * \param[out] account_balance_intervals_map A pointer to a pointer of \ref cardano_account_balance_intervals_map_t that will be set to the address
 *                        of the newly created account balance intervals map object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the account_balance_intervals_map was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_account_balance_intervals_map_t object by calling
 *       \ref cardano_account_balance_intervals_map_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 *
 * cardano_error_t result = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the account_balance_intervals_map
 *
 *   // Once done, ensure to clean up and release the account_balance_intervals_map
 *   cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode account_balance_intervals_map: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_account_balance_intervals_map_from_cbor(cardano_cbor_reader_t* reader, cardano_account_balance_intervals_map_t** account_balance_intervals_map);

/**
 * \brief Serializes an account balance intervals map into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_account_balance_intervals_map_t object using a \ref cardano_cbor_writer_t.
 * Entries are written in insertion order.
 *
 * \param[in] account_balance_intervals_map A constant pointer to the \ref cardano_account_balance_intervals_map_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p account_balance_intervals_map or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_account_balance_intervals_map_to_cbor(account_balance_intervals_map, writer);
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
 * cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_account_balance_intervals_map_to_cbor(
  const cardano_account_balance_intervals_map_t* account_balance_intervals_map,
  cardano_cbor_writer_t*                         writer);

/**
 * \brief Retrieves the length of the account_balance_intervals_map.
 *
 * This function returns the number of key-value pairs contained in the specified account_balance_intervals_map.
 *
 * \param[in] account_balance_intervals_map A constant pointer to the \ref cardano_account_balance_intervals_map_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the account_balance_intervals_map. Returns 0 if the account_balance_intervals_map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 * cardano_error_t result = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
 *
 * // Populate the account_balance_intervals_map with key-value pairs
 *
 * size_t length = cardano_account_balance_intervals_map_get_length(account_balance_intervals_map);
 * printf("The length of the account_balance_intervals_map is: %zu\n", length);
 *
 * cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_account_balance_intervals_map_get_length(const cardano_account_balance_intervals_map_t* account_balance_intervals_map);

/**
 * \brief Retrieves the interval associated with a given credential in the account balance intervals map.
 *
 * This function retrieves the interval associated with the specified credential in the provided account balance intervals map.
 * It returns the interval through the output parameter `element`. If the credential is not found in the map, the
 * function returns \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \param[in] account_balance_intervals_map A constant pointer to the \ref cardano_account_balance_intervals_map_t object from which
 *                       the interval is to be retrieved.
 * \param[in] key The credential whose associated interval is to be retrieved from the account_balance_intervals_map.
 * \param[out] element A pointer to a variable where the retrieved interval will be stored. If the credential
 *                     is found, this variable will be set to the associated interval, and the caller must
 *                     release it by calling \ref cardano_account_balance_interval_unref.
 *
 * \return \ref CARDANO_SUCCESS if the credential was found, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if the
 *         credential is not present in the map, or an appropriate error code if the input parameters are invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = ...;
 * cardano_credential_t* key = ...; // Create a credential object representing the key
 * cardano_account_balance_interval_t* interval = NULL;
 *
 * cardano_error_t result = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, &interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved interval
 *   cardano_account_balance_interval_unref(&interval);
 * }
 * else
 * {
 *   // Handle error or key not found
 * }
 *
 * cardano_credential_unref(&key); // Clean up the key resource
 * cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_account_balance_intervals_map_get(
  const cardano_account_balance_intervals_map_t* account_balance_intervals_map,
  cardano_credential_t*                          key,
  cardano_account_balance_interval_t**           element);

/**
 * \brief Inserts a key-value pair into the account balance intervals map.
 *
 * This function inserts the specified credential and its interval into the provided account balance intervals map.
 * Entries keep their insertion order when the map is serialized. Inserting a credential that is already
 * present in the map fails with \ref CARDANO_ERROR_DUPLICATED_KEY.
 *
 * \param[in] account_balance_intervals_map A constant pointer to the \ref cardano_account_balance_intervals_map_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The credential to be inserted into the account balance intervals map. The caller is responsible for managing
 *                the lifecycle of the key object.
 * \param[in] value The interval to be associated with the credential. The caller is responsible for managing
 *                  the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 * cardano_error_t result = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
 *
 * // Create key and value objects
 * cardano_credential_t* key = ...;
 * cardano_account_balance_interval_t* value = ...;
 *
 * // Insert the key-value pair into the account_balance_intervals_map
 * result = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value objects
 * cardano_credential_unref(&key);
 * cardano_account_balance_interval_unref(&value);
 *
 * cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_account_balance_intervals_map_insert(
  cardano_account_balance_intervals_map_t* account_balance_intervals_map,
  cardano_credential_t*                    key,
  cardano_account_balance_interval_t*      value);

/**
 * \brief Retrieves the keys from the account balance intervals map.
 *
 * This function retrieves all the keys from the provided account balance intervals map and returns them as a set.
 * The caller is responsible for managing the lifecycle of the returned set by calling
 * \ref cardano_credential_set_unref when it is no longer needed.
 *
 * \note The returned set is canonically ordered (by credential type, then hash bytes) and may not
 *       match the map's insertion-order iteration via the index based accessors.
 *
 * \param[in] account_balance_intervals_map A pointer to the \ref cardano_account_balance_intervals_map_t object from which
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
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 * cardano_error_t result = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
 *
 * // Populate the account_balance_intervals_map with key-value pairs
 *
 * cardano_credential_set_t* keys = NULL;
 * result = cardano_account_balance_intervals_map_get_keys(account_balance_intervals_map, &keys);
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
 * cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_account_balance_intervals_map_get_keys(
  cardano_account_balance_intervals_map_t* account_balance_intervals_map,
  cardano_credential_set_t**               keys);

/**
 * \brief Retrieves the credential at a specific index from the account balance intervals map.
 *
 * This function retrieves the credential at the specified index from the account balance intervals map.
 *
 * \param[in] account_balance_intervals_map Pointer to the account balance intervals map object.
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
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 * cardano_credential_t* credential = NULL;
 * size_t index = 0; // Index of the credential to retrieve
 *
 * // Assume account_balance_intervals_map is initialized properly
 *
 * cardano_error_t result = cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, index, &credential);
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
CARDANO_EXPORT cardano_error_t cardano_account_balance_intervals_map_get_key_at(
  const cardano_account_balance_intervals_map_t* account_balance_intervals_map,
  size_t                                         index,
  cardano_credential_t**                         credential);

/**
 * \brief Retrieves the interval at a specific index from the account balance intervals map.
 *
 * This function retrieves the interval at the specified index from the account balance intervals map.
 *
 * \param[in] account_balance_intervals_map Pointer to the account balance intervals map object.
 * \param[in] index The index of the interval to retrieve.
 * \param[out] interval On successful retrieval, this will point to the interval at the specified index.
 *                      The caller is responsible for managing the lifecycle of this object and must
 *                      release it by calling \ref cardano_account_balance_interval_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the interval was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 * cardano_account_balance_interval_t* interval = NULL;
 * size_t index = 0; // Index of the interval to retrieve
 *
 * // Assume account_balance_intervals_map is initialized properly
 *
 * cardano_error_t result = cardano_account_balance_intervals_map_get_value_at(account_balance_intervals_map, index, &interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the interval
 *
 *   // Once done, ensure to clean up and release the interval
 *   cardano_account_balance_interval_unref(&interval);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_account_balance_intervals_map_get_value_at(
  const cardano_account_balance_intervals_map_t* account_balance_intervals_map,
  size_t                                         index,
  cardano_account_balance_interval_t**           interval);

/**
 * \brief Retrieves the credential and interval at the specified index.
 *
 * This function retrieves the credential and its interval from the account balance intervals map
 * at the specified index.
 *
 * \param[in]  account_balance_intervals_map    Pointer to the account balance intervals map object.
 * \param[in]  index             The index at which to retrieve the key-value pair.
 * \param[out] credential On successful retrieval, this will point to the credential at the specified index.
 *                            The caller is responsible for managing the lifecycle of this object and should release it using
 *                            \ref cardano_credential_unref when it is no longer needed.
 * \param[out] interval On successful retrieval, this will point to the interval at the specified index.
 *                      The caller is responsible for managing the lifecycle of this object and must release
 *                      it by calling \ref cardano_account_balance_interval_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 * // Assume account_balance_intervals_map is initialized properly
 *
 * size_t index = 0;
 * cardano_credential_t* credential = NULL;
 * cardano_account_balance_interval_t* interval = NULL;
 *
 * cardano_error_t result = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, index, &credential, &interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential and the interval
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get key-value pair at index %zu.\n", index);
 * }
 *
 * // Clean up
 * cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * cardano_credential_unref(&credential);
 * cardano_account_balance_interval_unref(&interval);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_account_balance_intervals_map_get_key_value_at(
  const cardano_account_balance_intervals_map_t* account_balance_intervals_map,
  size_t                                         index,
  cardano_credential_t**                         credential,
  cardano_account_balance_interval_t**           interval);

/**
 * \brief Decrements the reference count of an account_balance_intervals_map object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_account_balance_intervals_map_t object
 * by decreasing its reference count. When the reference count reaches zero, the account_balance_intervals_map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] account_balance_intervals_map A pointer to the pointer of the account_balance_intervals_map object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_intervals_map_t* account_balance_intervals_map = NULL;
 * cardano_error_t result = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
 *
 * // Perform operations with the account_balance_intervals_map...
 *
 * cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
 * // At this point, account_balance_intervals_map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_account_balance_intervals_map_unref, the pointer to the \ref cardano_account_balance_intervals_map_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_account_balance_intervals_map_unref(cardano_account_balance_intervals_map_t** account_balance_intervals_map);

/**
 * \brief Increases the reference count of the cardano_account_balance_intervals_map_t object.
 *
 * This function is used to manually increment the reference count of an account_balance_intervals_map
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_account_balance_intervals_map_unref.
 *
 * \param account_balance_intervals_map A pointer to the account_balance_intervals_map object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming account_balance_intervals_map is a previously created account_balance_intervals_map object
 *
 * cardano_account_balance_intervals_map_ref(account_balance_intervals_map);
 *
 * // Now account_balance_intervals_map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_account_balance_intervals_map_ref there is a corresponding
 * call to \ref cardano_account_balance_intervals_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_account_balance_intervals_map_ref(cardano_account_balance_intervals_map_t* account_balance_intervals_map);

/**
 * \brief Retrieves the current reference count of the cardano_account_balance_intervals_map_t object.
 *
 * This function returns the number of active references to an account_balance_intervals_map object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_account_balance_intervals_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param account_balance_intervals_map A pointer to the account_balance_intervals_map object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified account_balance_intervals_map object. If the object
 * is properly managed (i.e., every \ref cardano_account_balance_intervals_map_ref call is matched with a
 * \ref cardano_account_balance_intervals_map_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming account_balance_intervals_map is a previously created account_balance_intervals_map object
 *
 * size_t ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_account_balance_intervals_map_refcount(const cardano_account_balance_intervals_map_t* account_balance_intervals_map);

/**
 * \brief Sets the last error message for a given account_balance_intervals_map object.
 *
 * Records an error message in the account_balance_intervals_map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] account_balance_intervals_map A pointer to the \ref cardano_account_balance_intervals_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the account_balance_intervals_map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_account_balance_intervals_map_set_last_error(cardano_account_balance_intervals_map_t* account_balance_intervals_map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific account_balance_intervals_map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_account_balance_intervals_map_set_last_error for the given
 * account_balance_intervals_map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] account_balance_intervals_map A pointer to the \ref cardano_account_balance_intervals_map_t instance whose last error
 *                   message is to be retrieved. If the account_balance_intervals_map is NULL, the function
 *                   returns a generic error message indicating the null account_balance_intervals_map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified account_balance_intervals_map. If the account_balance_intervals_map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_account_balance_intervals_map_set_last_error for the same account_balance_intervals_map, or until
 *       the account_balance_intervals_map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_account_balance_intervals_map_get_last_error(const cardano_account_balance_intervals_map_t* account_balance_intervals_map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_BALANCE_INTERVALS_MAP_H
