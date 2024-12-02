/**
 * \file withdrawal_map.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_WITHDRAWAL_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_WITHDRAWAL_MAP_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/reward_address_list.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of reward address to lovelace amount.
 */
typedef struct cardano_withdrawal_map_t cardano_withdrawal_map_t;

/**
 * \brief Creates and initializes a new instance of a withdrawal map.
 *
 * This function allocates and initializes a new instance of \ref cardano_withdrawal_map_t,
 * representing a map structure. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] withdrawal_map A pointer to a pointer to a \ref cardano_withdrawal_map_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_withdrawal_map_t
 *                        object. This object represents a "strong reference" to the withdrawal_map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the withdrawal_map is no longer
 *                        needed, the caller must release it by calling \ref cardano_withdrawal_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the withdrawal_map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = NULL;
 *
 * // Attempt to create a new withdrawal_map
 * cardano_error_t result = cardano_withdrawal_map_new(&withdrawal_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the withdrawal_map
 *
 *   // Once done, ensure to clean up and release the withdrawal_map
 *   cardano_withdrawal_map_unref(&withdrawal_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_withdrawal_map_new(cardano_withdrawal_map_t** withdrawal_map);

/**
 * \brief Creates a withdrawal map from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_withdrawal_map_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a withdrawal_map.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded withdrawal_map data.
 * \param[out] withdrawal_map A pointer to a pointer of \ref cardano_withdrawal_map_t that will be set to the address
 *                        of the newly created withdrawal_map object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the withdrawal_map was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_withdrawal_map_t object by calling
 *       \ref cardano_withdrawal_map_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_withdrawal_map_t* withdrawal_map = NULL;
 *
 * cardano_error_t result = cardano_withdrawal_map_from_cbor(reader, &withdrawal_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the withdrawal_map
 *
 *   // Once done, ensure to clean up and release the withdrawal_map
 *   cardano_withdrawal_map_unref(&withdrawal_map);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode withdrawal_map: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_withdrawal_map_from_cbor(cardano_cbor_reader_t* reader, cardano_withdrawal_map_t** withdrawal_map);

/**
 * \brief Serializes a withdrawal map into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_withdrawal_map_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] withdrawal_map A constant pointer to the \ref cardano_withdrawal_map_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p withdrawal_map or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_withdrawal_map_to_cbor(withdrawal_map, writer);
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
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_to_cbor(
  const cardano_withdrawal_map_t* withdrawal_map,
  cardano_cbor_writer_t*          writer);

/**
 * \brief Retrieves the length of the withdrawal_map.
 *
 * This function returns the number of key-value pairs contained in the specified withdrawal_map.
 *
 * \param[in] withdrawal_map A constant pointer to the \ref cardano_withdrawal_map_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the withdrawal_map. Returns 0 if the withdrawal_map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = cardano_withdrawal_map_new();
 *
 * // Populate the withdrawal_map with key-value pairs
 *
 * size_t length = cardano_withdrawal_map_get_length(withdrawal_map);
 * printf("The length of the withdrawal_map is: %zu\n", length);
 *
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_withdrawal_map_get_length(const cardano_withdrawal_map_t* withdrawal_map);

/**
 * \brief Retrieves the value associated with a given key in the withdrawal map.
 *
 * This function retrieves the value associated with the specified key in the provided withdrawal map.
 * It returns the value through the output parameter `element`. If the key is not found in the withdrawal map,
 * the output parameter `element` will be set to NULL.
 *
 * \param[in] withdrawal_map A constant pointer to the \ref cardano_withdrawal_map_t object from which
 *                       the value is to be retrieved.
 * \param[in] key The key whose associated value is to be retrieved from the withdrawal_map.
 * \param[out] element A pointer to a variable where the retrieved value will be stored. If the key
 *                     is found in the withdrawal map, this variable will be set to the associated value.
 *                     If the key is not found, this variable will be set to NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value associated with the key was successfully retrieved, or
 *         an appropriate error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = cardano_withdrawal_map_new();
 *
 * // Populate the withdrawal_map with key-value pairs
 *
 * cardano_reward_address_t* key = ...; // Create a reward_address object representing the key
 * uint64_t value = 0;
 * cardano_error_t result = cardano_withdrawal_map_get(withdrawal_map, key, &value);
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
 * cardano_reward_address_unref(&key); // Clean up the key resource
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_get(cardano_withdrawal_map_t* withdrawal_map, cardano_reward_address_t* key, uint64_t* element);

/**
 * \brief Inserts a key-value pair into the withdrawal map.
 *
 * This function inserts the specified key-value pair into the provided withdrawal map.
 *
 * \param[in] withdrawal_map A constant pointer to the \ref cardano_withdrawal_map_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The key to be inserted into the withdrawal map. The caller is responsible for managing
 *                the lifecycle of the key object.
 * \param[in] value The value to be associated with the key and inserted into the withdrawal_map.
 *                  The caller is responsible for managing the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = cardano_withdrawal_map_new();
 *
 * // Create key and value objects
 * cardano_reward_address_t* key = ...;
 * uint64_t value = 0;
 *
 * // Insert the key-value pair into the withdrawal_map
 * cardano_error_t result = cardano_withdrawal_map_insert(withdrawal_map, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value objects
 * cardano_reward_address_unref(&key);
 *
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_insert(cardano_withdrawal_map_t* withdrawal_map, cardano_reward_address_t* key, uint64_t value);

/**
 * \brief Inserts a withdrawal entry into the withdrawal map using a reward address.
 *
 * This function inserts a withdrawal entry into the specified \ref cardano_withdrawal_map_t object. The withdrawal is associated
 * with a reward address and specifies the amount to be withdrawn in lovelace.
 *
 * \param[in, out] withdrawal_map A pointer to the \ref cardano_withdrawal_map_t object where the withdrawal entry will be inserted.
 * \param[in] reward_address A pointer to a string representing the Bech32-encoded reward address. This parameter must not be NULL.
 * \param[in] reward_address_size The length of the reward address string in bytes.
 * \param[in] value The amount of ADA (in lovelace) to withdraw from the reward address.
 *
 * \return \ref CARDANO_SUCCESS if the withdrawal entry was successfully inserted into the map, or an appropriate error code
 * indicating the type of failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = ...; // Assume initialized
 * const char* reward_address = "stake1u..."; // Bech32-encoded reward address
 * uint64_t withdrawal_value = 1000000; // 1 ADA in lovelace
 *
 * cardano_error_t result = cardano_withdrawal_map_insert_ex(withdrawal_map, reward_address, strlen(reward_address), withdrawal_value);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Withdrawal entry successfully added
 * }
 * else
 * {
 *   printf("Failed to insert withdrawal entry: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup when done
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_insert_ex(
  cardano_withdrawal_map_t* withdrawal_map,
  const char*               reward_address,
  size_t                    reward_address_size,
  uint64_t                  value);

/**
 * \brief Retrieves the keys from the withdrawal map.
 *
 * This function retrieves all the keys from the provided withdrawal map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_reward_address_list_t when it is no longer needed.
 *
 * \param[in] withdrawal_map A constant pointer to the \ref cardano_withdrawal_map_t object from which
 *                       the keys are to be retrieved.
 * \param[out] keys A pointer to a variable where the retrieved keys will be stored as a list.
 *                  If successful, this variable will be set to point to the list of keys.
 *                  The caller is responsible for managing the lifecycle of this list.
 *                  It must be released by calling \ref cardano_reward_address_list_t when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the keys were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = cardano_withdrawal_map_new();
 *
 * // Populate the withdrawal_map with key-value pairs
 *
 * cardano_reward_address_list_t* keys = NULL;
 * cardano_error_t result = cardano_withdrawal_map_get_keys(withdrawal_map, &keys);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list of keys
 *   // Keys must also be freed if retrieved from the list
 *
 *   // Once done, ensure to clean up and release the keys list
 *   cardano_reward_address_list_unref(&keys);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_get_keys(cardano_withdrawal_map_t* withdrawal_map, cardano_reward_address_list_t** keys);

/**
 * \brief Retrieves the reward address at a specific index from the withdrawal map.
 *
 * This function retrieves the reward address at the specified index from the withdrawal map.
 *
 * \param[in] withdrawal_map Pointer to the withdrawal map object.
 * \param[in] index The index of the reward address to retrieve.
 * \param[out] reward_address On successful retrieval, this will point to the reward address
 *                            at the specified index. The caller is responsible for managing the lifecycle
 *                            of this object. Specifically, once the reward address is no longer needed,
 *                            the caller must release it by calling \ref cardano_reward_address_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the reward address was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = NULL;
 * cardano_reward_address_t* reward_address = NULL;
 * size_t index = 0; // Index of the key hash to retrieve
 *
 * // Assume withdrawal_map is initialized properly
 *
 * cardano_error_t result = cardano_withdrawal_map_get_key_at(withdrawal_map, index, &reward_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the reward_address
 *
 *   // Once done, ensure to clean up and release the reward_address
 *   cardano_reward_address_unref(&reward_address);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_get_key_at(
  const cardano_withdrawal_map_t* withdrawal_map,
  size_t                          index,
  cardano_reward_address_t**      reward_address);

/**
 * \brief Retrieves the withdrawal amount at a specific index from the withdrawal map.
 *
 * This function retrieves the withdrawal amount at the specified index from the withdrawal map.
 *
 * \param[in] withdrawal_map Pointer to the withdrawal map object.
 * \param[in] index The index of the withdrawal amount to retrieve.
 * \param[out] amount On successful retrieval, this will point to the withdrawal amount
 *                    at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the withdrawal amount was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = NULL;
 * uint64_t amount = 0;
 * size_t index = 0; // Index of the withdrawal amount to retrieve
 *
 * // Assume withdrawal_map is initialized properly
 *
 * cardano_error_t result = cardano_withdrawal_map_get_value_at(withdrawal_map, index, &amount);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the amount
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_get_value_at(
  const cardano_withdrawal_map_t* withdrawal_map,
  size_t                          index,
  uint64_t*                       amount);

/**
 * \brief Retrieves the reward address and withdrawal amount at the specified index.
 *
 * This function retrieves the reward address and withdrawal amount from the proposed withdrawal amounts
 * at the specified index.
 *
 * \param[in]  withdrawal_map    Pointer to the proposed withdrawal amounts object.
 * \param[in]  index             The index at which to retrieve the key-value pair.
 * \param[out] reward_address On successful retrieval, this will point to the reward address at the specified index.
 *                            The caller is responsible for managing the lifecycle of this object and should release it using
 *                            \ref cardano_reward_address_unref when it is no longer needed.
 * \param[out] amount On successful retrieval, this will point to the withdrawal amount at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = NULL;
 * // Assume withdrawal_map is initialized properly
 *
 * size_t index = 0;
 * cardano_reward_address_t* reward_address = NULL;
 * uint64_t amount = 0;
 *
 * cardano_error_t result = cardano_withdrawal_map_get_key_value_at(withdrawal_map, index, &reward_address, &amount);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (reward_address != NULL && amount != NULL)
 *   {
 *     // Use the reward address and withdrawal amount
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
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * cardano_reward_address_unref(&reward_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_withdrawal_map_get_key_value_at(
  const cardano_withdrawal_map_t* withdrawal_map,
  size_t                          index,
  cardano_reward_address_t**      reward_address,
  uint64_t*                       amount);

/**
 * \brief Decrements the reference count of a withdrawal_map object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_withdrawal_map_t object
 * by decreasing its reference count. When the reference count reaches zero, the withdrawal_map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] withdrawal_map A pointer to the pointer of the withdrawal_map object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_withdrawal_map_t* withdrawal_map = cardano_withdrawal_map_new();
 *
 * // Perform operations with the withdrawal_map...
 *
 * cardano_withdrawal_map_unref(&withdrawal_map);
 * // At this point, withdrawal_map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_withdrawal_map_unref, the pointer to the \ref cardano_withdrawal_map_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_withdrawal_map_unref(cardano_withdrawal_map_t** withdrawal_map);

/**
 * \brief Increases the reference count of the cardano_withdrawal_map_t object.
 *
 * This function is used to manually increment the reference count of a withdrawal_map
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_withdrawal_map_unref.
 *
 * \param withdrawal_map A pointer to the withdrawal_map object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming withdrawal_map is a previously created withdrawal_map object
 *
 * cardano_withdrawal_map_ref(withdrawal_map);
 *
 * // Now withdrawal_map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_withdrawal_map_ref there is a corresponding
 * call to \ref cardano_withdrawal_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_withdrawal_map_ref(cardano_withdrawal_map_t* withdrawal_map);

/**
 * \brief Retrieves the current reference count of the cardano_withdrawal_map_t object.
 *
 * This function returns the number of active references to a withdrawal_map object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_withdrawal_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param withdrawal_map A pointer to the withdrawal_map object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified withdrawal_map object. If the object
 * is properly managed (i.e., every \ref cardano_withdrawal_map_ref call is matched with a
 * \ref cardano_withdrawal_map_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming withdrawal_map is a previously created withdrawal_map object
 *
 * size_t ref_count = cardano_withdrawal_map_refcount(withdrawal_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_withdrawal_map_refcount(const cardano_withdrawal_map_t* withdrawal_map);

/**
 * \brief Sets the last error message for a given withdrawal_map object.
 *
 * Records an error message in the withdrawal_map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] withdrawal_map A pointer to the \ref cardano_withdrawal_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the withdrawal_map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_withdrawal_map_set_last_error(cardano_withdrawal_map_t* withdrawal_map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific withdrawal_map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_withdrawal_map_set_last_error for the given
 * withdrawal_map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] withdrawal_map A pointer to the \ref cardano_withdrawal_map_t instance whose last error
 *                   message is to be retrieved. If the withdrawal_map is NULL, the function
 *                   returns a generic error message indicating the null withdrawal_map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified withdrawal_map. If the withdrawal_map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_withdrawal_map_set_last_error for the same withdrawal_map, or until
 *       the withdrawal_map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_withdrawal_map_get_last_error(const cardano_withdrawal_map_t* withdrawal_map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_WITHDRAWAL_MAP_H