/**
 * \file blake2b_hash_to_redeemer_map.h
 *
 * \author angel.castillo
 * \date   Nov 16, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BLAKE2B_HASH_TO_REDEEMER_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BLAKE2B_HASH_TO_REDEEMER_MAP_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/export.h>
#include <cardano/witness_set/redeemer.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of blake2b hash to redeemers.
 */
typedef struct cardano_blake2b_hash_to_redeemer_map_t cardano_blake2b_hash_to_redeemer_map_t;

/**
 * \brief Creates and initializes a new instance of a map.
 *
 * This function allocates and initializes a new instance of \ref cardano_blake2b_hash_to_redeemer_map_t,
 * representing a map structure. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] map A pointer to a pointer to a \ref cardano_blake2b_hash_to_redeemer_map_t blake2b_hash. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_blake2b_hash_to_redeemer_map_t
 *                        blake2b_hash. This blake2b hash represents a "strong reference" to the map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this blake2b_hash. Specifically, once the map is no longer
 *                        needed, the caller must release it by calling \ref cardano_blake2b_hash_to_redeemer_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = NULL;
 *
 * // Attempt to create a new map
 * cardano_error_t result = cardano_blake2b_hash_to_redeemer_map_new(&blake2b_hash_to_redeemer_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the map
 *
 *   // Once done, ensure to clean up and release the map
 *   cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_blake2b_hash_to_redeemer_map_new(cardano_blake2b_hash_to_redeemer_map_t** map);

/**
 * \brief Retrieves the length of the map.
 *
 * This function returns the number of key-value pairs contained in the specified map.
 *
 * \param[in] map A constant pointer to the \ref cardano_blake2b_hash_to_redeemer_map_t blake2b hash for which
 *                the length is to be retrieved.
 *
 * \return The number of key-value pairs in the map. Returns 0 if the map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = cardano_blake2b_hash_to_redeemer_map_new();
 *
 * // Populate the map with key-value pairs
 *
 * size_t length = cardano_blake2b_hash_to_redeemer_map_get_length(blake2b_hash_to_redeemer_map);
 * printf("The length of the map is: %zu\n", length);
 *
 * cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_blake2b_hash_to_redeemer_map_get_length(const cardano_blake2b_hash_to_redeemer_map_t* map);

/**
 * \brief Retrieves the value associated with a given key in the map.
 *
 * This function retrieves the value associated with the specified key in the provided map.
 * It returns the value through the output parameter `element`. If the key is not found in the map,
 * the output parameter `element` will be set to NULL.
 *
 * \param[in] map A constant pointer to the \ref cardano_blake2b_hash_to_redeemer_map_t blake2b hash from which
 *                       the value is to be retrieved.
 * \param[in] key The key whose associated value is to be retrieved from the map.
 * \param[out] element A pointer to a variable where the retrieved value will be stored. If the key
 *                     is found in the map, this variable will be set to the associated value.
 *                     If the key is not found, this variable will be set to NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value associated with the key was successfully retrieved, or
 *         an appropriate error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = cardano_blake2b_hash_to_redeemer_map_new();
 *
 * // Populate the map with key-value pairs
 *
 * cardano_blake2b_hash_t* key = ...; // Create a blake2b hash representing the key
 * cardano_redeemer_t* value = NULL;
 * cardano_error_t result = cardano_blake2b_hash_to_redeemer_map_get(blake2b_hash_to_redeemer_map, key, &value);
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
 * cardano_blake2b_hash_unref(&key); // Clean up the key resource
 * cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
 * cardano_redeemer_unref(&value); // Clean up the value resource
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_redeemer_map_get(cardano_blake2b_hash_to_redeemer_map_t* map, const cardano_blake2b_hash_t* key, cardano_redeemer_t** element);

/**
 * \brief Inserts a key-value pair into the map.
 *
 * This function inserts the specified key-value pair into the provided map.
 *
 * \param[in] map A constant pointer to the \ref cardano_blake2b_hash_to_redeemer_map_t blake2b hash where
 *                       the key-value pair is to be inserted.
 * \param[in] key The key to be inserted into the map. The caller is responsible for managing
 *                the lifecycle of the key blake2b_hash.
 * \param[in] value The value to be associated with the key and inserted into the map.
 *                  The caller is responsible for managing the lifecycle of the value blake2b_hash.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = cardano_blake2b_hash_to_redeemer_map_new();
 *
 * // Create key and value blake2b hash
 * cardano_blake2b_hash_t* key = ...;
 * cardano_redeemer_t* value = ...;
 *
 * // Insert the key-value pair into the map
 * cardano_error_t result = cardano_blake2b_hash_to_redeemer_map_insert(blake2b_hash_to_redeemer_map, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value blake2b hash
 * cardano_blake2b_hash_unref(&key);
 *
 * cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_redeemer_map_insert(cardano_blake2b_hash_to_redeemer_map_t* map, cardano_blake2b_hash_t* key, cardano_redeemer_t* value);

/**
 * \brief Retrieves the blake2b hash at a specific index from the map.
 *
 * This function retrieves the blake2b hash at the specified index from the map.
 *
 * \param[in] map Pointer to the map blake2b_hash.
 * \param[in] index The index of the blake2b hash to retrieve.
 * \param[out] blake2b_hash On successful retrieval, this will point to the blake2b hash
 *                            at the specified index. The caller is responsible for managing the lifecycle
 *                            of this blake2b_hash. Specifically, once the blake2b hash is no longer needed,
 *                            the caller must release it by calling \ref cardano_blake2b_hash_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the blake2b hash was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = NULL;
 * cardano_blake2b_hash_t* blake2b hash = NULL;
 * size_t index = 0; // Index of the key hash to retrieve
 *
 * // Assume map is initialized properly
 *
 * cardano_error_t result = cardano_blake2b_hash_to_redeemer_map_get_key_at(blake2b_hash_to_redeemer_map, index, &blake2b_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the blake2b_hash
 *
 *   // Once done, ensure to clean up and release the blake2b_hash
 *   cardano_blake2b_hash_unref(&blake2b_hash);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_redeemer_map_get_key_at(
  const cardano_blake2b_hash_to_redeemer_map_t* map,
  size_t                                        index,
  cardano_blake2b_hash_t**                      blake2b_hash);

/**
 * \brief Retrieves the redeemer at a specific index from the map.
 *
 * This function retrieves the redeemer at the specified index from the map.
 *
 * \param[in] map Pointer to the map blake2b_hash.
 * \param[in] index The index of the redeemer to retrieve.
 * \param[out] redeemer On successful retrieval, this will point to the redeemer
 *                    at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the redeemer was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = NULL;
 * cardano_redeemer_t* redeemer = 0;
 * size_t index = 0; // Index of the redeemer to retrieve
 *
 * // Assume map is initialized properly
 *
 * cardano_error_t result = cardano_blake2b_hash_to_redeemer_map_get_value_at(blake2b_hash_to_redeemer_map, index, &redeemer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the redeemer
 * }
 * else
 * {
 *   // Handle the error
 * }
 *
 * cardano_redeemer_unref(&redeemer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_redeemer_map_get_value_at(
  const cardano_blake2b_hash_to_redeemer_map_t* map,
  size_t                                        index,
  cardano_redeemer_t**                          redeemer);

/**
 * \brief Retrieves the blake2b hash and redeemer at the specified index.
 *
 * This function retrieves the blake2b hash and redeemer from the proposed redeemers
 * at the specified index.
 *
 * \param[in]  map    Pointer to the proposed redeemers blake2b_hash.
 * \param[in]  index             The index at which to retrieve the key-value pair.
 * \param[out] blake2b_hash On successful retrieval, this will point to the blake2b hash at the specified index.
 *                            The caller is responsible for managing the lifecycle of this blake2b hash and should release it using
 *                            \ref cardano_blake2b_hash_unref when it is no longer needed.
 * \param[out] redeemer On successful retrieval, this will point to the redeemer at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = NULL;
 * // Assume map is initialized properly
 *
 * size_t index = 0;
 * cardano_blake2b_hash_t* blake2b hash = NULL;
 * cardano_redeemer_t* redeemer = 0;
 *
 * cardano_error_t result = cardano_blake2b_hash_to_redeemer_map_get_key_value_at(blake2b_hash_to_redeemer_map, index, &blake2b_hash, &redeemer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (blake2b_hash != NULL && redeemer != NULL)
 *   {
 *     // Use the blake2b hash and redeemer
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
 * cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
 * cardano_blake2b_hash_unref(&blake2b_hash);
 * cardano_redeemer_unref(&redeemer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_redeemer_map_get_key_value_at(
  const cardano_blake2b_hash_to_redeemer_map_t* map,
  size_t                                        index,
  cardano_blake2b_hash_t**                      blake2b_hash,
  cardano_redeemer_t**                          redeemer);

/**
 * \brief Finds a redeemer that matches the given blake2b hash reference. If it finds it, it updates its index. Otherwise
 * it does nothing.
 *
 * \param map The map to search for the blake2b_hash.
 * \param blake2b_hash The blake2b hash key that will be used to search for the redeemer.
 * \param index The new index to assign to the redeemer.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS.
 */
CARDANO_NODISCARD
cardano_error_t cardano_blake2b_hash_to_redeemer_map_update_redeemer_index(
  cardano_blake2b_hash_to_redeemer_map_t* map,
  cardano_blake2b_hash_t*                 blake2b_hash,
  uint64_t                                index);

/**
 * \brief Decrements the reference count of a map blake2b_hash.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_blake2b_hash_to_redeemer_map_t blake2b_hash
 * by decreasing its reference count. When the reference count reaches zero, the map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] map A pointer to the pointer of the map blake2b_hash. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the blake2b hash has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_to_redeemer_map_t* map = cardano_blake2b_hash_to_redeemer_map_new();
 *
 * // Perform operations with the map...
 *
 * cardano_blake2b_hash_to_redeemer_map_unref(&blake2b_hash_to_redeemer_map);
 * // At this point, map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_blake2b_hash_to_redeemer_map_unref, the pointer to the \ref cardano_blake2b_hash_to_redeemer_map_t blake2b_hash
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_blake2b_hash_to_redeemer_map_unref(cardano_blake2b_hash_to_redeemer_map_t** map);

/**
 * \brief Increases the reference count of the cardano_blake2b_hash_to_redeemer_map_t blake2b_hash.
 *
 * This function is used to manually increment the reference count of a map
 * blake2b_hash, indicating that another part of the code has taken ownership of it. This
 * ensures the blake2b hash remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_blake2b_hash_to_redeemer_map_unref.
 *
 * \param map A pointer to the map blake2b hash whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming map is a previously created map blake2b_hash
 *
 * cardano_blake2b_hash_to_redeemer_map_ref(blake2b_hash_to_redeemer_map);
 *
 * // Now map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_blake2b_hash_to_redeemer_map_ref there is a corresponding
 * call to \ref cardano_blake2b_hash_to_redeemer_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_blake2b_hash_to_redeemer_map_ref(cardano_blake2b_hash_to_redeemer_map_t* map);

/**
 * \brief Retrieves the current reference count of the cardano_blake2b_hash_to_redeemer_map_t blake2b_hash.
 *
 * This function returns the number of active references to a map blake2b_hash. It's useful
 * for debugging purposes or managing the lifecycle of the blake2b hash in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an blake2b hash holds a reference to another blake2b_hash, rather than directly to the
 * cardano_blake2b_hash_to_redeemer_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param map A pointer to the map blake2b hash whose reference count is queried.
 *                    The blake2b hash must not be NULL.
 *
 * \return The number of active references to the specified map blake2b_hash. If the blake2b_hash
 * is properly managed (i.e., every \ref cardano_blake2b_hash_to_redeemer_map_ref call is matched with a
 * \ref cardano_blake2b_hash_to_redeemer_map_unref call), this count should reach zero right before the blake2b_hash
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming map is a previously created map blake2b_hash
 *
 * size_t ref_count = cardano_blake2b_hash_to_redeemer_map_refcount(blake2b_hash_to_redeemer_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_blake2b_hash_to_redeemer_map_refcount(const cardano_blake2b_hash_to_redeemer_map_t* map);

/**
 * \brief Sets the last error message for a given map blake2b_hash.
 *
 * Records an error message in the map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] map A pointer to the \ref cardano_blake2b_hash_to_redeemer_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_blake2b_hash_to_redeemer_map_set_last_error(cardano_blake2b_hash_to_redeemer_map_t* map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_blake2b_hash_to_redeemer_map_set_last_error for the given
 * map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] map A pointer to the \ref cardano_blake2b_hash_to_redeemer_map_t instance whose last error
 *                   message is to be retrieved. If the map is NULL, the function
 *                   returns a generic error message indicating the null map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified map. If the map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the blake2b hash and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_blake2b_hash_to_redeemer_map_set_last_error for the same map, or until
 *       the map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_blake2b_hash_to_redeemer_map_get_last_error(const cardano_blake2b_hash_to_redeemer_map_t* map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BLAKE2B_HASH_TO_REDEEMER_MAP_H