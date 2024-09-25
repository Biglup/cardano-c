/**
 * \file plutus_map.h
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_MAP_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of plutus data.
 */
typedef struct cardano_plutus_map_t cardano_plutus_map_t;

/**
 * \brief A type corresponding to the Plutus Core Data datatype.
 */
typedef struct cardano_plutus_data_t cardano_plutus_data_t;

/**
 * \brief Represents a list of plutus data.
 */
typedef struct cardano_plutus_list_t cardano_plutus_list_t;

/**
 * \brief Creates and initializes a new instance of a plutus map.
 *
 * This function allocates and initializes a new instance of \ref cardano_plutus_map_t,
 * representing a map structure in the Plutus smart contract language. It returns an error code
 * to indicate the success or failure of the operation.
 *
 * \param[out] plutus_map A pointer to a pointer to a \ref cardano_plutus_map_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_plutus_map_t
 *                        object. This object represents a "strong reference" to the plutus_map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the plutus_map is no longer
 *                        needed, the caller must release it by calling \ref cardano_plutus_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus_map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = NULL;
 *
 * // Attempt to create a new plutus_map
 * cardano_error_t result = cardano_plutus_map_new(&plutus_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_map
 *
 *   // Once done, ensure to clean up and release the plutus_map
 *   cardano_plutus_map_unref(&plutus_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_map_new(cardano_plutus_map_t** plutus_map);

/**
 * \brief Creates a plutus map from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_plutus_map_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a plutus_map.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded plutus_map data.
 * \param[out] plutus_map A pointer to a pointer of \ref cardano_plutus_map_t that will be set to the address
 *                        of the newly created plutus_map object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the plutus_map was successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the data and invalidate any existing signatures.
 *         To prevent this, when a plutus data object is created using \ref cardano_plutus_map_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_plutus_map_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_plutus_map_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_plutus_map_t object by calling
 *       \ref cardano_plutus_map_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_plutus_map_t* plutus_map = NULL;
 *
 * cardano_error_t result = cardano_plutus_map_from_cbor(reader, &plutus_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_map
 *
 *   // Once done, ensure to clean up and release the plutus_map
 *   cardano_plutus_map_unref(&plutus_map);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode plutus_map: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_map_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_map_t** plutus_map);

/**
 * \brief Serializes a plutus map into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_plutus_map_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] plutus_map A constant pointer to the \ref cardano_plutus_map_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p plutus_map or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the data and invalidate any existing signatures.
 *         To prevent this, when a plutus data object is created using \ref cardano_plutus_map_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_plutus_map_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_plutus_map_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_plutus_map_to_cbor(plutus_map, writer);
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
 * cardano_plutus_map_unref(&plutus_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_map_to_cbor(
  const cardano_plutus_map_t* plutus_map,
  cardano_cbor_writer_t*      writer);

/**
 * \brief Retrieves the length of the plutus_map.
 *
 * This function returns the number of key-value pairs contained in the specified plutus_map.
 *
 * \param[in] plutus_map A constant pointer to the \ref cardano_plutus_map_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the plutus_map. Returns 0 if the plutus_map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = cardano_plutus_map_new();
 *
 * // Populate the plutus_map with key-value pairs
 *
 * size_t length = cardano_plutus_map_get_length(plutus_map);
 * printf("The length of the plutus_map is: %zu\n", length);
 *
 * cardano_plutus_map_unref(&plutus_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_plutus_map_get_length(const cardano_plutus_map_t* plutus_map);

/**
 * \brief Retrieves the value associated with a given key in the plutus map.
 *
 * This function retrieves the value associated with the specified key in the provided plutus map.
 * It returns the value through the output parameter `element`. If the key is not found in the plutus map,
 * the output parameter `element` will be set to NULL.
 *
 * \param[in] plutus_map A constant pointer to the \ref cardano_plutus_map_t object from which
 *                       the value is to be retrieved.
 * \param[in] key The key whose associated value is to be retrieved from the plutus_map.
 * \param[out] element A pointer to a variable where the retrieved value will be stored. If the key
 *                     is found in the plutus map, this variable will be set to the associated value.
 *                     If the key is not found, this variable will be set to NULL. The caller is
 *                     responsible for releasing the value by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the value associated with the key was successfully retrieved, or
 *         an appropriate error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = cardano_plutus_map_new();
 *
 * // Populate the plutus_map with key-value pairs
 *
 * cardano_plutus_data_t* key = ...; // Create a plutus_data object representing the key
 * cardano_plutus_data_t* value = NULL;
 * cardano_error_t result = cardano_plutus_map_get(plutus_map, key, &value);
 *
 * if (result == CARDANO_SUCCESS && value != NULL)
 * {
 *   // Use the retrieved value
 *   // ...
 *   cardano_plutus_data_unref(&value); // Clean up the value resource
 * }
 * else
 * {
 *   // Handle error or key not found
 * }
 *
 * cardano_plutus_data_unref(&key); // Clean up the key resource
 * cardano_plutus_map_unref(&plutus_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_map_get(cardano_plutus_map_t* plutus_map, cardano_plutus_data_t* key, cardano_plutus_data_t** element);

/**
 * \brief Inserts a key-value pair into the plutus map.
 *
 * This function inserts the specified key-value pair into the provided plutus map.
 *
 * \param[in] plutus_map A constant pointer to the \ref cardano_plutus_map_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The key to be inserted into the plutus map. The caller is responsible for managing
 *                the lifecycle of the key object.
 * \param[in] value The value to be associated with the key and inserted into the plutus_map.
 *                  The caller is responsible for managing the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = cardano_plutus_map_new();
 *
 * // Create key and value objects
 * cardano_plutus_data_t* key = ...; // Create a plutus_data object representing the key
 * cardano_plutus_data_t* value = ...; // Create a plutus_data object representing the value
 *
 * // Insert the key-value pair into the plutus_map
 * cardano_error_t result = cardano_plutus_map_insert(plutus_map, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value objects
 * cardano_plutus_data_unref(&key);
 * cardano_plutus_data_unref(&value);
 *
 * cardano_plutus_map_unref(&plutus_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_map_insert(cardano_plutus_map_t* plutus_map, cardano_plutus_data_t* key, cardano_plutus_data_t* value);

/**
 * \brief Retrieves the keys from the plutus map.
 *
 * This function retrieves all the keys from the provided plutus map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_plutus_list_unref when it is no longer needed.
 *
 * \param[in] plutus_map A constant pointer to the \ref cardano_plutus_map_t object from which
 *                       the keys are to be retrieved.
 * \param[out] keys A pointer to a variable where the retrieved keys will be stored as a list.
 *                  If successful, this variable will be set to point to the list of keys.
 *                  The caller is responsible for managing the lifecycle of this list.
 *                  It must be released by calling \ref cardano_plutus_list_unref when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the keys were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = cardano_plutus_map_new();
 *
 * // Populate the plutus_map with key-value pairs
 *
 * cardano_plutus_list_t* keys = NULL;
 * cardano_error_t result = cardano_plutus_map_get_keys(plutus_map, &keys);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list of keys
 *   // Keys must also be freed if retrieved from the list
 *
 *   // Once done, ensure to clean up and release the keys list
 *   cardano_plutus_list_unref(&keys);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_plutus_map_unref(&plutus_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_map_get_keys(cardano_plutus_map_t* plutus_map, cardano_plutus_list_t** keys);

/**
 * \brief Retrieves the values from the plutus map.
 *
 * This function retrieves all the values from the provided plutus map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_plutus_list_unref when it is no longer needed.
 *
 * \param[in] plutus_map A constant pointer to the \ref cardano_plutus_map_t object from which
 *                       the values are to be retrieved.
 * \param[out] values A pointer to a variable where the retrieved values will be stored as a list.
 *                    If successful, this variable will be set to point to the list of values.
 *                    The caller is responsible for managing the lifecycle of this list.
 *                    It must be released by calling \ref cardano_plutus_list_unref when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the values were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = cardano_plutus_map_new();
 *
 * // Populate the plutus_map with key-value pairs
 *
 * cardano_plutus_list_t* values = NULL;
 * cardano_error_t result = cardano_plutus_map_get_values(plutus_map, &values);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list of values
 *
 *   // Once done, ensure to clean up and release the values list
 *   // Values must also be freed if retrieved from the list
 *   cardano_plutus_list_unref(&values);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_plutus_map_unref(&plutus_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_map_get_values(cardano_plutus_map_t* plutus_map, cardano_plutus_list_t** values);

/**
 * \brief Checks if two plutus maps are equal.
 *
 * This function compares two plutus maps for equality. Two plutus maps are considered equal if they
 * have the same keys and associated values.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_plutus_map_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_plutus_map_t object to be compared.
 *
 * \return \c true if the two plutus maps are equal, otherwise \c false.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* map1 = ...; // Populate the first plutus map
 * cardano_plutus_map_t* map2 = ...; // Populate the second plutus map
 *
 * if (cardano_plutus_map_equals(map1, map2))
 * {
 *   // The two plutus maps are equal
 * }
 * else
 * {
 *   // The two plutus maps are not equal
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_plutus_map_equals(const cardano_plutus_map_t* lhs, const cardano_plutus_map_t* rhs);

/**
 * \brief Clears the cached CBOR representation from a plutus map.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_plutus_map_t object.
 * It is useful when you have modified the plutus map after it was created from CBOR using
 * \ref cardano_plutus_map_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the plutus map, rather than using the original cached CBOR.
 *
 * \param[in,out] plutus_map A pointer to an initialized \ref cardano_plutus_map_t object
 *                         from which the CBOR cache will be cleared.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the plutus map when
 *          serialized, which can alter the plutus map and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume plutus_map was created using cardano_plutus_map_from_cbor
 * cardano_plutus_map_t* plutus_map = ...;
 *
 * // Modify the plutus_map as needed
 * // For example, change the fee

 * if (result != CARDANO_SUCCESS)
 * {
 *   printf("Failed to set new fee: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clear the CBOR cache to ensure serialization uses the updated plutus_map
 * cardano_plutus_map_clear_cbor_cache(plutus_map);
 *
 * // Serialize the plutus_map to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_plutus_map_to_cbor(plutus_map, writer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the CBOR data as needed
 * }
 * else
 * {
 *   const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *   printf("Serialization failed: %s\n", error_message);
 * }
 *
 * // Clean up resources
 * cardano_cbor_writer_unref(&writer);
 * cardano_plutus_map_unref(&plutus_map);
 * \endcode
 */
CARDANO_EXPORT void cardano_plutus_map_clear_cbor_cache(cardano_plutus_map_t* plutus_map);

/**
 * \brief Decrements the reference count of a plutus_map object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_plutus_map_t object
 * by decreasing its reference count. When the reference count reaches zero, the plutus_map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] plutus_map A pointer to the pointer of the plutus_map object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* plutus_map = cardano_plutus_map_new();
 *
 * // Perform operations with the plutus_map...
 *
 * cardano_plutus_map_unref(&plutus_map);
 * // At this point, plutus_map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_plutus_map_unref, the pointer to the \ref cardano_plutus_map_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_plutus_map_unref(cardano_plutus_map_t** plutus_map);

/**
 * \brief Increases the reference count of the cardano_plutus_map_t object.
 *
 * This function is used to manually increment the reference count of a plutus_map
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_plutus_map_unref.
 *
 * \param plutus_map A pointer to the plutus_map object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_map is a previously created plutus_map object
 *
 * cardano_plutus_map_ref(plutus_map);
 *
 * // Now plutus_map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_plutus_map_ref there is a corresponding
 * call to \ref cardano_plutus_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_plutus_map_ref(cardano_plutus_map_t* plutus_map);

/**
 * \brief Retrieves the current reference count of the cardano_plutus_map_t object.
 *
 * This function returns the number of active references to a plutus_map object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_plutus_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param plutus_map A pointer to the plutus_map object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified plutus_map object. If the object
 * is properly managed (i.e., every \ref cardano_plutus_map_ref call is matched with a
 * \ref cardano_plutus_map_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_map is a previously created plutus_map object
 *
 * size_t ref_count = cardano_plutus_map_refcount(plutus_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_plutus_map_refcount(const cardano_plutus_map_t* plutus_map);

/**
 * \brief Sets the last error message for a given plutus_map object.
 *
 * Records an error message in the plutus_map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] plutus_map A pointer to the \ref cardano_plutus_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the plutus_map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_plutus_map_set_last_error(cardano_plutus_map_t* plutus_map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific plutus_map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_plutus_map_set_last_error for the given
 * plutus_map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] plutus_map A pointer to the \ref cardano_plutus_map_t instance whose last error
 *                   message is to be retrieved. If the plutus_map is NULL, the function
 *                   returns a generic error message indicating the null plutus_map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified plutus_map. If the plutus_map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_plutus_map_set_last_error for the same plutus_map, or until
 *       the plutus_map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_plutus_map_get_last_error(const cardano_plutus_map_t* plutus_map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_MAP_H