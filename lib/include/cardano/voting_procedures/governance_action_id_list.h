/**
 * \file governance_action_id_list.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_ID_LIST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_ID_LIST_H

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
 * \brief Each governance action that is accepted on the chain will be assigned a unique
 * identifier, consisting of the transaction hash that created it and the index within
 * the transaction body that points to it.
 */
typedef struct cardano_governance_action_id_t cardano_governance_action_id_t;

/**
 * \brief Represents a list of governance action id.
 */
typedef struct cardano_governance_action_id_list_t cardano_governance_action_id_list_t;

/**
 * \brief Creates and initializes a new instance of a governance_action_id_list.
 *
 * This function allocates and initializes a new instance of \ref cardano_governance_action_id_list_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] governance_action_id_list On successful initialization, this will point to a newly created
 *             \ref cardano_governance_action_id_list_t object. This object represents a "strong reference"
 *             to the governance_action_id_list, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the governance_action_id_list is no longer needed, the caller must release it
 *             by calling \ref cardano_governance_action_id_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the governance_action_id_list was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_list_t* governance_action_id_list = NULL;
 *
 * // Attempt to create a new governance_action_id_list
 * cardano_error_t result = cardano_governance_action_id_list_new(&governance_action_id_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the governance_action_id_list
 *
 *   // Once done, ensure to clean up and release the governance_action_id_list
 *   cardano_governance_action_id_list_unref(&governance_action_id_list);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_list_new(cardano_governance_action_id_list_t** governance_action_id_list);

/**
 * \brief Retrieves the length of a governance action id list.
 *
 * This function retrieves the number of elements in the provided \ref cardano_governance_action_id_list_t object.
 *
 * \param[in] governance_action_id_list A constant pointer to the \ref cardano_governance_action_id_list_t object whose length is to be retrieved.
 *
 * \return The number of elements in the governance_action_id_list. Return 0 if the governance_action_id_list is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_list_t* governance_action_id_list = cardano_governance_action_id_list_new();
 *
 * // Populate governance_action_id_list with elements
 *
 * size_t length = cardano_governance_action_id_list_get_length(governance_action_id_list);
 * printf("Length of the governance_action_id_list: %zu\n", length);
 *
 * // Clean up the governance_action_id_list object once done
 * cardano_governance_action_id_list_unref(&governance_action_id_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_governance_action_id_list_get_length(const cardano_governance_action_id_list_t* governance_action_id_list);

/**
 * \brief Retrieves an element from a governance action id list by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_governance_action_id_list_t object
 * and stores it in the output parameter.
 *
 * \param[in] governance_action_id_list A constant pointer to the \ref cardano_governance_action_id_list_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the governance action id list. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_governance_action_id_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_governance_action_id_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_list_t* governance_action_id_list = cardano_governance_action_id_list_new();
 *
 * // Populate governance_action_id_list with elements
 *
 * cardano_governance_action_id_t* element = NULL;
 * cardano_error_t result = cardano_governance_action_id_list_get(governance_action_id_list, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_governance_action_id_unref(&element);
 * }
 *
 * // Clean up the governance_action_id_list object once done
 * cardano_governance_action_id_list_unref(&governance_action_id_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_governance_action_id_list_get(const cardano_governance_action_id_list_t* governance_action_id_list, size_t index, cardano_governance_action_id_t** element);

/**
 * \brief Adds an element to a governance action id list.
 *
 * This function adds the specified element to the end of the provided \ref cardano_governance_action_id_list_t object.
 *
 * \param[in] governance_action_id_list A constant pointer to the \ref cardano_governance_action_id_list_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_governance_action_id_t object that is to be added to the governance_action_id_list.
 *                    The element will be referenced by the governance_action_id_list after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the governance_action_id_list, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_list_t* governance_action_id_list = cardano_governance_action_id_list_new();
 *
 * // Create and initialize a new governance_action_id element
 * cardano_governance_action_id_t* element = { ... };
 *
 * // Add the element to the governance_action_id_list
 * cardano_error_t result = cardano_governance_action_id_list_add(governance_action_id_list, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the governance_action_id_list object once done
 * cardano_governance_action_id_list_unref(&governance_action_id_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_governance_action_id_list_add(cardano_governance_action_id_list_t* governance_action_id_list, cardano_governance_action_id_t* element);

/**
 * \brief Decrements the reference count of a governance_action_id_list object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_governance_action_id_list_t object
 * by decreasing its reference count. When the reference count reaches zero, the governance_action_id_list is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] governance_action_id_list A pointer to the pointer of the governance_action_id_list object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_list_t* governance_action_id_list = cardano_governance_action_id_list_new();
 *
 * // Perform operations with the governance_action_id_list...
 *
 * cardano_governance_action_id_list_unref(&governance_action_id_list);
 * // At this point, governance_action_id_list is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_governance_action_id_list_unref, the pointer to the \ref cardano_governance_action_id_list_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_governance_action_id_list_unref(cardano_governance_action_id_list_t** governance_action_id_list);

/**
 * \brief Increases the reference count of the cardano_governance_action_id_list_t object.
 *
 * This function is used to manually increment the reference count of a governance_action_id_list
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_governance_action_id_list_unref.
 *
 * \param governance_action_id_list A pointer to the governance_action_id_list object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming governance_action_id_list is a previously created governance_action_id_list object
 *
 * cardano_governance_action_id_list_ref(governance_action_id_list);
 *
 * // Now governance_action_id_list can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_governance_action_id_list_ref there is a corresponding
 * call to \ref cardano_governance_action_id_list_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_governance_action_id_list_ref(cardano_governance_action_id_list_t* governance_action_id_list);

/**
 * \brief Retrieves the current reference count of the cardano_governance_action_id_list_t object.
 *
 * This function returns the number of active references to a governance_action_id_list object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_governance_action_id_list_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param governance_action_id_list A pointer to the governance_action_id_list object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified governance_action_id_list object. If the object
 * is properly managed (i.e., every \ref cardano_governance_action_id_list_ref call is matched with a
 * \ref cardano_governance_action_id_list_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming governance_action_id_list is a previously created governance_action_id_list object
 *
 * size_t ref_count = cardano_governance_action_id_list_refcount(governance_action_id_list);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_governance_action_id_list_refcount(const cardano_governance_action_id_list_t* governance_action_id_list);

/**
 * \brief Sets the last error message for a given governance_action_id_list object.
 *
 * Records an error message in the governance_action_id_list's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] governance_action_id_list A pointer to the \ref cardano_governance_action_id_list_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the governance_action_id_list's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_governance_action_id_list_set_last_error(cardano_governance_action_id_list_t* governance_action_id_list, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific governance_action_id_list.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_governance_action_id_list_set_last_error for the given
 * governance_action_id_list. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] governance_action_id_list A pointer to the \ref cardano_governance_action_id_list_t instance whose last error
 *                   message is to be retrieved. If the governance_action_id_list is NULL, the function
 *                   returns a generic error message indicating the null governance_action_id_list.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified governance_action_id_list. If the governance_action_id_list is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_governance_action_id_list_set_last_error for the same governance_action_id_list, or until
 *       the governance_action_id_list is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_governance_action_id_list_get_last_error(const cardano_governance_action_id_list_t* governance_action_id_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_ID_LIST_H