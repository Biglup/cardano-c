/**
 * \file metadatum_map.h
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_METADATUM_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_METADATUM_MAP_H

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
 * \brief Represents a map of metadatum.
 */
typedef struct cardano_metadatum_map_t cardano_metadatum_map_t;

/**
 * \brief A type corresponding to the metadatum type.
 *
 * Use this type to build metadata structures that you want to be representable on-chain.
 */
typedef struct cardano_metadatum_t cardano_metadatum_t;

/**
 * \brief Represents a list of metadatum.
 */
typedef struct cardano_metadatum_list_t cardano_metadatum_list_t;

/**
 * \brief Creates and initializes a new instance of a metadatum map.
 *
 * This function allocates and initializes a new instance of \ref cardano_metadatum_map_t,
 * representing a map structure. It returns an error code
 * to indicate the success or failure of the operation.
 *
 * \param[out] metadatum_map A pointer to a pointer to a \ref cardano_metadatum_map_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_metadatum_map_t
 *                        object. This object represents a "strong reference" to the metadatum_map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the metadatum_map is no longer
 *                        needed, the caller must release it by calling \ref cardano_metadatum_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum_map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = NULL;
 *
 * // Attempt to create a new metadatum_map
 * cardano_error_t result = cardano_metadatum_map_new(&metadatum_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum_map
 *
 *   // Once done, ensure to clean up and release the metadatum_map
 *   cardano_metadatum_map_unref(&metadatum_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_map_new(cardano_metadatum_map_t** metadatum_map);

/**
 * \brief Creates a metadatum map from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_metadatum_map_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a metadatum_map.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded metadatum_map data.
 * \param[out] metadatum_map A pointer to a pointer of \ref cardano_metadatum_map_t that will be set to the address
 *                        of the newly created metadatum_map object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the metadatum_map was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_metadatum_map_t object by calling
 *       \ref cardano_metadatum_map_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_metadatum_map_t* metadatum_map = NULL;
 *
 * cardano_error_t result = cardano_metadatum_map_from_cbor(reader, &metadatum_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum_map
 *
 *   // Once done, ensure to clean up and release the metadatum_map
 *   cardano_metadatum_map_unref(&metadatum_map);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode metadatum_map: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_map_from_cbor(cardano_cbor_reader_t* reader, cardano_metadatum_map_t** metadatum_map);

/**
 * \brief Serializes a metadatum map into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_metadatum_map_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] metadatum_map A constant pointer to the \ref cardano_metadatum_map_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p metadatum_map or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_metadatum_map_to_cbor(metadatum_map, writer);
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
 * cardano_metadatum_map_unref(&metadatum_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_map_to_cbor(
  const cardano_metadatum_map_t* metadatum_map,
  cardano_cbor_writer_t*         writer);

/**
 * \brief Retrieves the length of the metadatum_map.
 *
 * This function returns the number of key-value pairs contained in the specified metadatum_map.
 *
 * \param[in] metadatum_map A constant pointer to the \ref cardano_metadatum_map_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the metadatum_map. Returns 0 if the metadatum_map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = cardano_metadatum_map_new();
 *
 * // Populate the metadatum_map with key-value pairs
 *
 * size_t length = cardano_metadatum_map_get_length(metadatum_map);
 * printf("The length of the metadatum_map is: %zu\n", length);
 *
 * cardano_metadatum_map_unref(&metadatum_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_metadatum_map_get_length(const cardano_metadatum_map_t* metadatum_map);

/**
 * \brief Retrieves the value associated with a given key in the metadatum map.
 *
 * This function retrieves the value associated with the specified key in the provided metadatum map.
 * It returns the value through the output parameter `element`. If the key is not found in the metadatum map,
 * the output parameter `element` will be set to NULL.
 *
 * \param[in] metadatum_map A constant pointer to the \ref cardano_metadatum_map_t object from which
 *                       the value is to be retrieved.
 * \param[in] key The key whose associated value is to be retrieved from the metadatum_map.
 * \param[out] element A pointer to a variable where the retrieved value will be stored. If the key
 *                     is found in the metadatum map, this variable will be set to the associated value.
 *                     If the key is not found, this variable will be set to NULL. The caller is
 *                     responsible for releasing the value by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the value associated with the key was successfully retrieved, or
 *         an appropriate error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = cardano_metadatum_map_new();
 *
 * // Populate the metadatum_map with key-value pairs
 *
 * cardano_metadatum_t* key = ...; // Create a metadatum object representing the key
 * cardano_metadatum_t* value = NULL;
 * cardano_error_t result = cardano_metadatum_map_get(metadatum_map, key, &value);
 *
 * if (result == CARDANO_SUCCESS && value != NULL)
 * {
 *   // Use the retrieved value
 *   // ...
 *   cardano_metadatum_unref(&value); // Clean up the value resource
 * }
 * else
 * {
 *   // Handle error or key not found
 * }
 *
 * cardano_metadatum_unref(&key); // Clean up the key resource
 * cardano_metadatum_map_unref(&metadatum_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_map_get(cardano_metadatum_map_t* metadatum_map, cardano_metadatum_t* key, cardano_metadatum_t** element);

/**
 * \brief Inserts a key-value pair into the metadatum map.
 *
 * This function inserts the specified key-value pair into the provided metadatum map.
 *
 * \param[in] metadatum_map A constant pointer to the \ref cardano_metadatum_map_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The key to be inserted into the metadatum map. The caller is responsible for managing
 *                the lifecycle of the key object.
 * \param[in] value The value to be associated with the key and inserted into the metadatum_map.
 *                  The caller is responsible for managing the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = cardano_metadatum_map_new();
 *
 * // Create key and value objects
 * cardano_metadatum_t* key = ...; // Create a metadatum object representing the key
 * cardano_metadatum_t* value = ...; // Create a metadatum object representing the value
 *
 * // Insert the key-value pair into the metadatum_map
 * cardano_error_t result = cardano_metadatum_map_insert(metadatum_map, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value objects
 * cardano_metadatum_unref(&key);
 * cardano_metadatum_unref(&value);
 *
 * cardano_metadatum_map_unref(&metadatum_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_map_insert(cardano_metadatum_map_t* metadatum_map, cardano_metadatum_t* key, cardano_metadatum_t* value);

/**
 * \brief Retrieves the keys from the metadatum map.
 *
 * This function retrieves all the keys from the provided metadatum map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_metadatum_list_unref when it is no longer needed.
 *
 * \param[in] metadatum_map A constant pointer to the \ref cardano_metadatum_map_t object from which
 *                       the keys are to be retrieved.
 * \param[out] keys A pointer to a variable where the retrieved keys will be stored as a list.
 *                  If successful, this variable will be set to point to the list of keys.
 *                  The caller is responsible for managing the lifecycle of this list.
 *                  It must be released by calling \ref cardano_metadatum_list_unref when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the keys were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = cardano_metadatum_map_new();
 *
 * // Populate the metadatum_map with key-value pairs
 *
 * cardano_metadatum_list_t* keys = NULL;
 * cardano_error_t result = cardano_metadatum_map_get_keys(metadatum_map, &keys);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list of keys
 *   // Keys must also be freed if retrieved from the list
 *
 *   // Once done, ensure to clean up and release the keys list
 *   cardano_metadatum_list_unref(&keys);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_metadatum_map_unref(&metadatum_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_map_get_keys(cardano_metadatum_map_t* metadatum_map, cardano_metadatum_list_t** keys);

/**
 * \brief Retrieves the values from the metadatum map.
 *
 * This function retrieves all the values from the provided metadatum map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_metadatum_list_unref when it is no longer needed.
 *
 * \param[in] metadatum_map A constant pointer to the \ref cardano_metadatum_map_t object from which
 *                       the values are to be retrieved.
 * \param[out] values A pointer to a variable where the retrieved values will be stored as a list.
 *                    If successful, this variable will be set to point to the list of values.
 *                    The caller is responsible for managing the lifecycle of this list.
 *                    It must be released by calling \ref cardano_metadatum_list_unref when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the values were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = cardano_metadatum_map_new();
 *
 * // Populate the metadatum_map with key-value pairs
 *
 * cardano_metadatum_list_t* values = NULL;
 * cardano_error_t result = cardano_metadatum_map_get_values(metadatum_map, &values);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list of values
 *
 *   // Once done, ensure to clean up and release the values list
 *   // Values must also be freed if retrieved from the list
 *   cardano_metadatum_list_unref(&values);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_metadatum_map_unref(&metadatum_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_map_get_values(cardano_metadatum_map_t* metadatum_map, cardano_metadatum_list_t** values);

/**
 * \brief Checks if two metadatum maps are equal.
 *
 * This function compares two metadatum maps for equality. Two metadatum maps are considered equal if they
 * have the same keys and associated values.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_metadatum_map_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_metadatum_map_t object to be compared.
 *
 * \return \c true if the two metadatum maps are equal, otherwise \c false.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* map1 = ...; // Populate the first metadatum map
 * cardano_metadatum_map_t* map2 = ...; // Populate the second metadatum map
 *
 * if (cardano_metadatum_map_equals(map1, map2))
 * {
 *   // The two metadatum maps are equal
 * }
 * else
 * {
 *   // The two metadatum maps are not equal
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_metadatum_map_equals(const cardano_metadatum_map_t* lhs, const cardano_metadatum_map_t* rhs);

/**
 * \brief Decrements the reference count of a metadatum_map object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_metadatum_map_t object
 * by decreasing its reference count. When the reference count reaches zero, the metadatum_map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] metadatum_map A pointer to the pointer of the metadatum_map object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* metadatum_map = cardano_metadatum_map_new();
 *
 * // Perform operations with the metadatum_map...
 *
 * cardano_metadatum_map_unref(&metadatum_map);
 * // At this point, metadatum_map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_metadatum_map_unref, the pointer to the \ref cardano_metadatum_map_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_metadatum_map_unref(cardano_metadatum_map_t** metadatum_map);

/**
 * \brief Increases the reference count of the cardano_metadatum_map_t object.
 *
 * This function is used to manually increment the reference count of a metadatum_map
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_metadatum_map_unref.
 *
 * \param metadatum_map A pointer to the metadatum_map object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming metadatum_map is a previously created metadatum_map object
 *
 * cardano_metadatum_map_ref(metadatum_map);
 *
 * // Now metadatum_map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_metadatum_map_ref there is a corresponding
 * call to \ref cardano_metadatum_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_metadatum_map_ref(cardano_metadatum_map_t* metadatum_map);

/**
 * \brief Retrieves the current reference count of the cardano_metadatum_map_t object.
 *
 * This function returns the number of active references to a metadatum_map object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_metadatum_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param metadatum_map A pointer to the metadatum_map object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified metadatum_map object. If the object
 * is properly managed (i.e., every \ref cardano_metadatum_map_ref call is matched with a
 * \ref cardano_metadatum_map_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming metadatum_map is a previously created metadatum_map object
 *
 * size_t ref_count = cardano_metadatum_map_refcount(metadatum_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_metadatum_map_refcount(const cardano_metadatum_map_t* metadatum_map);

/**
 * \brief Sets the last error message for a given metadatum_map object.
 *
 * Records an error message in the metadatum_map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] metadatum_map A pointer to the \ref cardano_metadatum_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the metadatum_map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_metadatum_map_set_last_error(cardano_metadatum_map_t* metadatum_map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific metadatum_map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_metadatum_map_set_last_error for the given
 * metadatum_map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] metadatum_map A pointer to the \ref cardano_metadatum_map_t instance whose last error
 *                   message is to be retrieved. If the metadatum_map is NULL, the function
 *                   returns a generic error message indicating the null metadatum_map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified metadatum_map. If the metadatum_map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_metadatum_map_set_last_error for the same metadatum_map, or until
 *       the metadatum_map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_metadatum_map_get_last_error(const cardano_metadatum_map_t* metadatum_map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_METADATUM_MAP_H