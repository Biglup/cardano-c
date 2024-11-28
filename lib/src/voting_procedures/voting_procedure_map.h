/**
 * \file voting_procedure_map.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURE_MAP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURE_MAP_H

/* INCLUDES ******************************************************************/

#include <cardano/common/governance_action_id.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/voting_procedures/governance_action_id_list.h>
#include <cardano/voting_procedures/voting_procedure.h>
#include <cardano/voting_procedures/voting_procedure_list.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of voting procedure.
 */
typedef struct cardano_voting_procedure_map_t cardano_voting_procedure_map_t;

/**
 * \brief Creates and initializes a new instance of a voting procedure map.
 *
 * This function allocates and initializes a new instance of \ref cardano_voting_procedure_map_t,
 * representing a map structure. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] voting_procedure_map A pointer to a pointer to a \ref cardano_voting_procedure_map_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_voting_procedure_map_t
 *                        object. This object represents a "strong reference" to the voting_procedure_map,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the voting_procedure_map is no longer
 *                        needed, the caller must release it by calling \ref cardano_voting_procedure_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the voting_procedure_map was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_map_t* voting_procedure_map = NULL;
 *
 * // Attempt to create a new voting_procedure_map
 * cardano_error_t result = cardano_voting_procedure_map_new(&voting_procedure_map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the voting_procedure_map
 *
 *   // Once done, ensure to clean up and release the voting_procedure_map
 *   cardano_voting_procedure_map_unref(&voting_procedure_map);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_voting_procedure_map_new(cardano_voting_procedure_map_t** voting_procedure_map);

/**
 * \brief Retrieves the length of the voting_procedure_map.
 *
 * This function returns the number of key-value pairs contained in the specified voting_procedure_map.
 *
 * \param[in] voting_procedure_map A constant pointer to the \ref cardano_voting_procedure_map_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the voting_procedure_map. Returns 0 if the voting_procedure_map is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_map_t* voting_procedure_map = cardano_voting_procedure_map_new();
 *
 * // Populate the voting_procedure_map with key-value pairs
 *
 * size_t length = cardano_voting_procedure_map_get_length(voting_procedure_map);
 * printf("The length of the voting_procedure_map is: %zu\n", length);
 *
 * cardano_voting_procedure_map_unref(&voting_procedure_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_voting_procedure_map_get_length(const cardano_voting_procedure_map_t* voting_procedure_map);

/**
 * \brief Retrieves the value associated with a given key in the voting procedure map.
 *
 * This function retrieves the value associated with the specified key in the provided voting procedure map.
 * It returns the value through the output parameter `element`. If the key is not found in the voting procedure map,
 * the output parameter `element` will be set to NULL.
 *
 * \param[in] voting_procedure_map A constant pointer to the \ref cardano_voting_procedure_map_t object from which
 *                       the value is to be retrieved.
 * \param[in] key The key whose associated value is to be retrieved from the voting_procedure_map.
 * \param[out] element A pointer to a variable where the retrieved value will be stored. If the key
 *                     is found in the voting procedure map, this variable will be set to the associated value.
 *                     If the key is not found, this variable will be set to NULL. The caller is
 *                     responsible for releasing the value by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the value associated with the key was successfully retrieved, or
 *         an appropriate error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_map_t* voting_procedure_map = cardano_voting_procedure_map_new();
 *
 * // Populate the voting_procedure_map with key-value pairs
 *
 * cardano_plutus_data_t* key = ...; // Create a plutus_data object representing the key
 * cardano_plutus_data_t* value = NULL;
 * cardano_error_t result = cardano_voting_procedure_map_get(voting_procedure_map, key, &value);
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
 * cardano_voting_procedure_map_unref(&voting_procedure_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedure_map_get(cardano_voting_procedure_map_t* voting_procedure_map, cardano_governance_action_id_t* key, cardano_voting_procedure_t** element);

/**
 * \brief Inserts a key-value pair into the voting procedure map.
 *
 * This function inserts the specified key-value pair into the provided voting procedure map.
 *
 * \param[in] voting_procedure_map A constant pointer to the \ref cardano_voting_procedure_map_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The key to be inserted into the voting procedure map. The caller is responsible for managing
 *                the lifecycle of the key object.
 * \param[in] value The value to be associated with the key and inserted into the voting_procedure_map.
 *                  The caller is responsible for managing the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_map_t* voting_procedure_map = cardano_voting_procedure_map_new();
 *
 * // Create key and value objects
 * cardano_plutus_data_t* key = ...; // Create a plutus_data object representing the key
 * cardano_plutus_data_t* value = ...; // Create a plutus_data object representing the value
 *
 * // Insert the key-value pair into the voting_procedure_map
 * cardano_error_t result = cardano_voting_procedure_map_insert(voting_procedure_map, key, value);
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
 * cardano_voting_procedure_map_unref(&voting_procedure_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedure_map_insert(cardano_voting_procedure_map_t* voting_procedure_map, cardano_governance_action_id_t* key, cardano_voting_procedure_t* value);

/**
 * \brief Retrieves the keys from the voting procedure map.
 *
 * This function retrieves all the keys from the provided voting procedure map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_plutus_list_unref when it is no longer needed.
 *
 * \param[in] voting_procedure_map A constant pointer to the \ref cardano_voting_procedure_map_t object from which
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
 * cardano_voting_procedure_map_t* voting_procedure_map = cardano_voting_procedure_map_new();
 *
 * // Populate the voting_procedure_map with key-value pairs
 *
 * cardano_plutus_list_t* keys = NULL;
 * cardano_error_t result = cardano_voting_procedure_map_get_keys(voting_procedure_map, &keys);
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
 * cardano_voting_procedure_map_unref(&voting_procedure_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedure_map_get_keys(cardano_voting_procedure_map_t* voting_procedure_map, cardano_governance_action_id_list_t** keys);

/**
 * \brief Retrieves the values from the voting procedure map.
 *
 * This function retrieves all the values from the provided voting procedure map and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_plutus_list_unref when it is no longer needed.
 *
 * \param[in] voting_procedure_map A constant pointer to the \ref cardano_voting_procedure_map_t object from which
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
 * cardano_voting_procedure_map_t* voting_procedure_map = cardano_voting_procedure_map_new();
 *
 * // Populate the voting_procedure_map with key-value pairs
 *
 * cardano_plutus_list_t* values = NULL;
 * cardano_error_t result = cardano_voting_procedure_map_get_values(voting_procedure_map, &values);
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
 * cardano_voting_procedure_map_unref(&voting_procedure_map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedure_map_get_values(cardano_voting_procedure_map_t* voting_procedure_map, cardano_voting_procedure_list_t** values);

/**
 * \brief Decrements the reference count of a voting_procedure_map object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_voting_procedure_map_t object
 * by decreasing its reference count. When the reference count reaches zero, the voting_procedure_map is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] voting_procedure_map A pointer to the pointer of the voting_procedure_map object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_map_t* voting_procedure_map = cardano_voting_procedure_map_new();
 *
 * // Perform operations with the voting_procedure_map...
 *
 * cardano_voting_procedure_map_unref(&voting_procedure_map);
 * // At this point, voting_procedure_map is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_voting_procedure_map_unref, the pointer to the \ref cardano_voting_procedure_map_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_voting_procedure_map_unref(cardano_voting_procedure_map_t** voting_procedure_map);

/**
 * \brief Increases the reference count of the cardano_voting_procedure_map_t object.
 *
 * This function is used to manually increment the reference count of a voting_procedure_map
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_voting_procedure_map_unref.
 *
 * \param voting_procedure_map A pointer to the voting_procedure_map object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voting_procedure_map is a previously created voting_procedure_map object
 *
 * cardano_voting_procedure_map_ref(voting_procedure_map);
 *
 * // Now voting_procedure_map can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_voting_procedure_map_ref there is a corresponding
 * call to \ref cardano_voting_procedure_map_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_voting_procedure_map_ref(cardano_voting_procedure_map_t* voting_procedure_map);

/**
 * \brief Retrieves the current reference count of the cardano_voting_procedure_map_t object.
 *
 * This function returns the number of active references to a voting_procedure_map object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_voting_procedure_map_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param voting_procedure_map A pointer to the voting_procedure_map object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified voting_procedure_map object. If the object
 * is properly managed (i.e., every \ref cardano_voting_procedure_map_ref call is matched with a
 * \ref cardano_voting_procedure_map_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voting_procedure_map is a previously created voting_procedure_map object
 *
 * size_t ref_count = cardano_voting_procedure_map_refcount(voting_procedure_map);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_voting_procedure_map_refcount(const cardano_voting_procedure_map_t* voting_procedure_map);

/**
 * \brief Sets the last error message for a given voting_procedure_map object.
 *
 * Records an error message in the voting_procedure_map's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] voting_procedure_map A pointer to the \ref cardano_voting_procedure_map_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the voting_procedure_map's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_voting_procedure_map_set_last_error(cardano_voting_procedure_map_t* voting_procedure_map, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific voting_procedure_map.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_voting_procedure_map_set_last_error for the given
 * voting_procedure_map. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] voting_procedure_map A pointer to the \ref cardano_voting_procedure_map_t instance whose last error
 *                   message is to be retrieved. If the voting_procedure_map is NULL, the function
 *                   returns a generic error message indicating the null voting_procedure_map.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified voting_procedure_map. If the voting_procedure_map is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_voting_procedure_map_set_last_error for the same voting_procedure_map, or until
 *       the voting_procedure_map is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_voting_procedure_map_get_last_error(const cardano_voting_procedure_map_t* voting_procedure_map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURE_MAP_H