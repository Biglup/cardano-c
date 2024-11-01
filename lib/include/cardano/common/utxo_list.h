/**
 * \file utxo_list.h
 *
 * \author angel.castillo
 * \date   Sep 25, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UTXO_LIST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UTXO_LIST_H

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
 * \brief Represents a unspent output (UTxO).
 */
typedef struct cardano_utxo_t cardano_utxo_t;

/**
 * \brief Represents a list of UTxO.
 */
typedef struct cardano_utxo_list_t cardano_utxo_list_t;

/**
 * \brief Function pointer type that compares two utxos and returns a value
 * indicating whether one utxo is less than, equal to, or greater than the other.
 *
 * \param[in] lhs The left-hand side utxo to compare.
 * \param[in] rhs The right-hand side utxo to compare.
 * \param[in] context An optional context pointer that will be passed to the comparison function.
 *
 * \return A negative value if `lhs` is less than `rhs`, 0 if `lhs` is equal to `rhs`, or a positive
 * value if `lhs` is greater than `rhs`.
 */
typedef int (*cardano_utxo_list_compare_item_t)(cardano_utxo_t* lhs, cardano_utxo_t* rhs, void* context);

/**
 * \brief Defines a function pointer for evaluating whether a specific utxo meets a defined set of criteria.
 *
 * This typedef defines a function pointer type that represents a predicate function. A predicate function
 * takes an utxo of type `cardano_utxo_t` and an optional context as parameters and evaluates whether
 * the utxo meets a specific set of criteria.
 *
 * \param[in] item The utxo to be evaluated against the predicate's criteria.
 * \param[in] context An optional context providing additional data or parameters for the predicate evaluation.
 *
 * \return Returns \c true if the `item` satisfies the predicate's conditions, \c false otherwise.
 */
typedef bool (*cardano_utxo_list_unary_predicate_t)(cardano_utxo_t* item, const void* context);

/**
 * \brief Creates and initializes a new instance of a utxo_list.
 *
 * This function allocates and initializes a new instance of \ref cardano_utxo_list_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] utxo_list On successful initialization, this will point to a newly created
 *             \ref cardano_utxo_list_t object. This object represents a "strong reference"
 *             to the utxo_list, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the utxo_list is no longer needed, the caller must release it
 *             by calling \ref cardano_utxo_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the utxo_list was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = NULL;
 *
 * // Attempt to create a new utxo_list
 * cardano_error_t result = cardano_utxo_list_new(&utxo_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the utxo_list
 *
 *   // Once done, ensure to clean up and release the utxo_list
 *   cardano_utxo_list_unref(&utxo_list);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_utxo_list_new(cardano_utxo_list_t** utxo_list);

/**
 * \brief Retrieves the length of a UTxO list.
 *
 * This function retrieves the number of elements in the provided \ref cardano_utxo_list_t object.
 *
 * \param[in] utxo_list A constant pointer to the \ref cardano_utxo_list_t object whose length is to be retrieved.
 *
 * \return The number of elements in the utxo_list. Return 0 if the utxo_list is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = cardano_utxo_list_new();
 *
 * // Populate utxo_list with elements
 *
 * size_t length = cardano_utxo_list_get_length(utxo_list);
 * printf("Length of the utxo_list: %zu\n", length);
 *
 * // Clean up the utxo_list object once done
 * cardano_utxo_list_unref(&utxo_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_utxo_list_get_length(const cardano_utxo_list_t* utxo_list);

/**
 * \brief Retrieves an element from a UTxO list by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_utxo_list_t object
 * and stores it in the output parameter.
 *
 * \param[in] utxo_list A constant pointer to the \ref cardano_utxo_list_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the UTxO list. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_utxo_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_utxo_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = cardano_utxo_list_new();
 *
 * // Populate utxo_list with elements
 *
 * cardano_utxo_t* element = NULL;
 * cardano_error_t result = cardano_utxo_list_get(utxo_list, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_utxo_unref(&element);
 * }
 *
 * // Clean up the utxo_list object once done
 * cardano_utxo_list_unref(&utxo_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_utxo_list_get(const cardano_utxo_list_t* utxo_list, size_t index, cardano_utxo_t** element);

/**
 * \brief Adds an element to a UTxO list.
 *
 * This function adds the specified element to the end of the provided \ref cardano_utxo_list_t object.
 *
 * \param[in] utxo_list A constant pointer to the \ref cardano_utxo_list_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_utxo_t object that is to be added to the utxo_list.
 *                    The element will be referenced by the utxo_list after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the utxo_list, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = cardano_utxo_list_new();
 *
 * // Create and initialize a new UTxO element
 * cardano_utxo_t* element = { ... };
 *
 * // Add the element to the utxo_list
 * cardano_error_t result = cardano_utxo_list_add(utxo_list, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the utxo_list object once done
 * cardano_utxo_list_unref(&utxo_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_utxo_list_add(cardano_utxo_list_t* utxo_list, cardano_utxo_t* element);

/**
 * \brief Removes a specific UTXO from a UTXO list.
 *
 * The `cardano_utxo_list_remove` function allows you to remove a specified UTXO from a \ref cardano_utxo_list_t object.
 *
 * \param[in,out] utxo_list A pointer to the \ref cardano_utxo_list_t from which the specified UTXO will be removed.
 * \param[in] element A pointer to the \ref cardano_utxo_t object representing the UTXO to be removed.
 *
 * \return \ref CARDANO_SUCCESS if the UTXO was successfully removed from the list, or an appropriate error code if
 *         the UTXO could not be found or removed.
 *
 * \note This function does not modify the original UTXO object referenced by `element`, only removes it from the list.
 *
 * \note The caller is responsible for managing the memory of both `utxo_list` and `element`, ensuring that
 *       unreferenced UTXOs are properly deallocated to avoid memory leaks.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = ...;  // List of UTXOs
 * cardano_utxo_t* utxo_to_remove = ...;  // UTXO element to be removed
 *
 * cardano_error_t result = cardano_utxo_list_remove(utxo_list, utxo_to_remove);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // UTXO was successfully removed from the list
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_utxo_list_remove(cardano_utxo_list_t* utxo_list, cardano_utxo_t* element);

/**
 * Clears the contents of the specified utxo_list.
 *
 * This function removes all elements from the utxo_list, leaving it empty.
 *
 * \param[in,out] utxo_list The utxo_list to clear.
 */
CARDANO_EXPORT void cardano_utxo_list_clear(cardano_utxo_list_t* utxo_list);

/**
 * Sorts the elements of the utxo_list according to the provided comparison function.
 *
 * The comparison function should return a negative value if the first argument is less
 * than the second, zero if they are equal, and a positive value if the first is greater
 * than the second.
 *
 * \param[in,out] utxo_list The utxo_list to sort.
 * \param[in] compare The comparison function used to determine the order of the elements.
 *                    The function must not modify the elements.
 * \param[in] context An optional context pointer that will be passed to the comparison function.
 */
CARDANO_EXPORT void cardano_utxo_list_sort(cardano_utxo_list_t* utxo_list, cardano_utxo_list_compare_item_t compare, void* context);

/**
 * Searches for an element in the utxo_list that satisfies a predicate.
 *
 * Iterates over the utxo_list elements and returns the first element for which
 * the predicate returns true. If no such element is found, returns NULL.
 *
 * \param[in] utxo_list The utxo_list to search in.
 * \param[in] predicate The unary predicate function used to evaluate each element.
 *                      The function should return true for the target element
 *                      and false otherwise.
 * \param[in] context An optional context pointer that will be passed to the predicate function.
 * \return A pointer to the first element satisfying the predicate, or NULL if
 *         no such element is found.
 */
CARDANO_NODISCARD
CARDANO_EXPORT
cardano_utxo_t* cardano_utxo_list_find(const cardano_utxo_list_t* utxo_list, cardano_utxo_list_unary_predicate_t predicate, const void* context);

/**
 * Creates and returns a new utxo_list containing only the elements that satisfy the given predicate.
 *
 * This function iterates over each element of the input utxo_list and applies the predicate function to it.
 * Elements for which the predicate returns true are included in the new utxo_list. The original utxo_list remains
 * unchanged. The caller is responsible for managing the unreferencing of the returned utxo_list to avoid memory leaks.
 *
 * \param[in] utxo_list The source utxo_list to filter.
 * \param[in] predicate The unary predicate function used to test each element. Elements that
 *                      satisfy the predicate (i.e., for which the predicate returns true) are
 *                      included in the new utxo_list.
 * \param[in] context An optional context pointer that will be passed to the predicate function.
 * \return A new utxo_list containing only elements that satisfy the predicate. Returns NULL if
 *         the operation fails due to memory allocation errors. The caller is responsible for unreferencing
 *         the returned utxo_list.
 */
CARDANO_NODISCARD
CARDANO_EXPORT
cardano_utxo_list_t* cardano_utxo_list_filter(const cardano_utxo_list_t* utxo_list, cardano_utxo_list_unary_predicate_t predicate, const void* context);

/**
 * \brief Concatenates two utxo_lists into a new one.
 *
 * Creates a new utxo_list containing the combined data of the provided utxo_lists.
 *
 * \param[in] lhs   The first utxo_list.
 * \param[in] rhs  The second utxo_list.
 *
 * \return A new utxo_list containing the concatenated data or NULL on memory allocation failure.
 * The caller assumes ownership of the returned utxo_list and must manage its lifecycle,
 * ensuring it is dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_utxo_list_t* cardano_utxo_list_concat(const cardano_utxo_list_t* lhs, const cardano_utxo_list_t* rhs);

/**
 * \brief Slices a utxo_list.
 *
 * Extracts a portion of the utxo_list between the given indices.
 *
 * \param[in] utxo_list  Source utxo_list.
 * \param[in] start  Start index of the slice (inclusive).
 * \param[in] end    End index of the slice (exclusive).
 *
 * \return A new utxo_list containing the slice or NULL for invalid input or memory allocation failure.
 * The caller assumes ownership of the returned utxo_list and must manage its lifecycle,
 * ensuring it is dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_utxo_list_t* cardano_utxo_list_slice(const cardano_utxo_list_t* utxo_list, size_t start, size_t end);

/**
 * \brief Removes elements from the utxo_list starting at a given index.
 *
 * This function removes `delete_count` elements from `utxo_list` starting at index `start`.
 *
 * \param[in,out] utxo_list The utxo_list to modify.
 * \param[in] start The index at which to start removing elements. Supports negative indices.
 * \param[in] delete_count The number of elements to remove from the utxo_list starting at `start`.
 *                         If `delete_count` exceeds the number of elements from `start` to the end,
 *                         it will be adjusted to remove till the end.
 *
 * \return A new utxo_list containing the removed elements. Returns `NULL` on error.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_utxo_list_t* cardano_utxo_list_erase(
  cardano_utxo_list_t* utxo_list,
  int64_t              start,
  size_t               delete_count);

/**
 * \brief Creates a shallow clone of a UTXO list.
 *
 * This function creates a new \ref cardano_utxo_list_t object that is a shallow copy of the provided
 * UTXO list. The cloned list contains references to the same UTXO elements as the original list.
 * The UTXO elements themselves are not duplicated; instead, their reference counts are incremented.
 * This means both the original list and the cloned list share the same UTXO elements.
 *
 * \param[in] utxo_list The UTXO list to be cloned. Must not be \c NULL.
 *
 * \return A pointer to the new \ref cardano_utxo_list_t object containing the same elements as \p utxo_list.
 * Returns \c NULL if \p utxo_list is \c NULL or if memory allocation fails.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* original_list = cardano_utxo_list_new();
 *
 * // Populate original_list with UTXO elements...
 *
 * cardano_utxo_list_t* cloned_list = cardano_utxo_list_clone(original_list);
 * if (cloned_list == NULL)
 * {
 *     // Handle error (e.g., memory allocation failure)
 * }
 *
 * // Use cloned_list as needed...
 *
 * // When done, release both lists
 * cardano_utxo_list_unref(&cloned_list);
 * cardano_utxo_list_unref(&original_list);
 * \endcode
 *
 * \note The cloned list and the original list share the same UTXO elements. Modifications to the UTXO elements
 *       will be visible in both lists. Ensure that this behavior is acceptable in your use case.
 *
 * \see cardano_utxo_list_unref()
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_utxo_list_t* cardano_utxo_list_clone(cardano_utxo_list_t* utxo_list);

/**
 * \brief Decrements the reference count of a utxo_list object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_utxo_list_t object
 * by decreasing its reference count. When the reference count reaches zero, the utxo_list is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] utxo_list A pointer to the pointer of the utxo_list object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_list_t* utxo_list = cardano_utxo_list_new();
 *
 * // Perform operations with the utxo_list...
 *
 * cardano_utxo_list_unref(&utxo_list);
 * // At this point, utxo_list is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_utxo_list_unref, the pointer to the \ref cardano_utxo_list_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_utxo_list_unref(cardano_utxo_list_t** utxo_list);

/**
 * \brief Increases the reference count of the cardano_utxo_list_t object.
 *
 * This function is used to manually increment the reference count of a utxo_list
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_utxo_list_unref.
 *
 * \param utxo_list A pointer to the utxo_list object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming utxo_list is a previously created utxo_list object
 *
 * cardano_utxo_list_ref(utxo_list);
 *
 * // Now utxo_list can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_utxo_list_ref there is a corresponding
 * call to \ref cardano_utxo_list_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_utxo_list_ref(cardano_utxo_list_t* utxo_list);

/**
 * \brief Retrieves the current reference count of the cardano_utxo_list_t object.
 *
 * This function returns the number of active references to a utxo_list object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_utxo_list_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param utxo_list A pointer to the utxo_list object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified utxo_list object. If the object
 * is properly managed (i.e., every \ref cardano_utxo_list_ref call is matched with a
 * \ref cardano_utxo_list_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming utxo_list is a previously created utxo_list object
 *
 * size_t ref_count = cardano_utxo_list_refcount(utxo_list);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_utxo_list_refcount(const cardano_utxo_list_t* utxo_list);

/**
 * \brief Sets the last error message for a given utxo_list object.
 *
 * Records an error message in the utxo_list's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] utxo_list A pointer to the \ref cardano_utxo_list_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the utxo_list's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_utxo_list_set_last_error(cardano_utxo_list_t* utxo_list, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific utxo_list.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_utxo_list_set_last_error for the given
 * utxo_list. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] utxo_list A pointer to the \ref cardano_utxo_list_t instance whose last error
 *                   message is to be retrieved. If the utxo_list is NULL, the function
 *                   returns a generic error message indicating the null utxo_list.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified utxo_list. If the utxo_list is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_utxo_list_set_last_error for the same utxo_list, or until
 *       the utxo_list is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_utxo_list_get_last_error(const cardano_utxo_list_t* utxo_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UTXO_LIST_H