/**
 * \file array.h
 *
 * \author luisd.bianchi
 * \date   Mar 04, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ARRAY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ARRAY_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Function pointer type that compares two objects of the same type and returns a value
 * indicating whether one object is less than, equal to, or greater than the other.
 *
 * \param[in] lhs The left-hand side object to compare.
 * \param[in] rhs The right-hand side object to compare.
 * \param[in] context An optional context providing additional data or parameters for the compare function.
 *
 * \return A negative value if `lhs` is less than `rhs`, 0 if `lhs` is equal to `rhs`, or a positive
 * value if `lhs` is greater than `rhs`.
 */
typedef int (*cardano_array_compare_item_t)(const cardano_object_t* lhs, const cardano_object_t* rhs, void* context);

/**
 * \brief Defines a function pointer for evaluating whether a specific object meets a defined set of criteria.
 *
 * This typedef defines a function pointer type that represents a predicate function. A predicate function
 * takes an object of type `cardano_object_t` and an optional context as parameters and evaluates whether
 * the object meets a specific set of criteria.
 *
 * \param[in] item The object to be evaluated against the predicate's criteria.
 * \param[in] context An optional context providing additional data or parameters for the predicate evaluation.
 *
 * \return Returns \c true if the `item` satisfies the predicate's conditions, \c false otherwise.
 */
typedef bool (*cardano_array_unary_predicate_t)(const cardano_object_t* item, const void* context);

/**
 * \brief A dynamic, reference-counted array with configurable exponential growth.
 *
 * \remarks The array employs an exponential growth strategy, increasing its capacity by a factor of 1.5 by default
 * when the array becomes full. This default growth factor is based on a
 * <a href="http://groups.google.com/group/comp.lang.c++.moderated/msg/ba558b4924758e2e">recommendation from Andrew Koenig's</a>
 * (growth factor should be less than (1+sqrt(5))/2 (~1.6)).
 *
 * \note The growth factor of the array can be configured at compilation time using the environment variable `COLLECTION_GROW_FACTOR`
 **/
typedef struct cardano_array_t cardano_array_t;

/**
 * \brief Creates a new dynamic array with the specified initial capacity.
 *
 * \param[in] capacity   Initial capacity of the array. The capacity must be greater than 0.
 * If 0 is provided, this function will return NULL.
 *
 * \return The newly created array or NULL on memory allocation failure.
 * The caller assumes ownership of the returned array and is responsible for its lifecycle.
 * It must be dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_array_t* cardano_array_new(size_t capacity);

/**
 * \brief Concatenates two arrays into a new one.
 *
 * Creates a new array containing the combined data of the provided arrays.
 *
 * \param[in] lhs   The first array.
 * \param[in] rhs  The second array.
 *
 * \return A new array containing the concatenated data or NULL on memory allocation failure.
 * The caller assumes ownership of the returned array and must manage its lifecycle,
 * ensuring it is dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_array_t* cardano_array_concat(const cardano_array_t* lhs, const cardano_array_t* rhs);

/**
 * \brief Slices a array.
 *
 * Extracts a portion of the array between the given indices.
 *
 * \param[in] array  Source array.
 * \param[in] start  Start index of the slice (inclusive).
 * \param[in] end    End index of the slice (exclusive).
 *
 * \return A new array containing the slice or NULL for invalid input or memory allocation failure.
 * The caller assumes ownership of the returned array and must manage its lifecycle,
 * ensuring it is dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_array_t* cardano_array_slice(const cardano_array_t* array, size_t start, size_t end);

/**
 * \brief Removes elements from the array starting at a given index.
 *
 * This function removes `delete_count` elements from `array` starting at index `start`.
 *
 * \param[in,out] array The array to modify.
 * \param[in] start The index at which to start removing elements. Supports negative indices.
 * \param[in] delete_count The number of elements to remove from the array starting at `start`.
 *                         If `delete_count` exceeds the number of elements from `start` to the end,
 *                         it will be adjusted to remove till the end.
 *
 * \return A new array containing the removed elements. Returns `NULL` on error.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_array_t* cardano_array_erase(
  cardano_array_t* array,
  int64_t          start,
  size_t           delete_count);

/**
 * \brief Decrements the array's reference count.
 *
 * If the reference count reaches zero, the array memory is deallocated.
 *
 * \param[in] array Pointer to the array whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_array_unref(cardano_array_t** array);

/**
 * \brief Increments the array's reference count.
 *
 * Ensures that the array remains allocated until the last reference is released.
 *
 * \param[in] array Array whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_array_ref(cardano_array_t* array);

/**
 * \brief Retrieves the array's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param[in] array Target array.
 * \return Current reference count of the array.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_array_refcount(const cardano_array_t* array);

/**
 * \brief Fetches a direct pointer to the array's data.
 *
 * \warning This returns a pointer to the internal data, not a copy. Do not manually deallocate.
 *
 * \param[in] array Target array.
 * \param[in] index Index of the data to be fetched.
 *
 * \return Pointer to the array's internal data or NULL if the array is empty.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_object_t* cardano_array_get(const cardano_array_t* array, size_t index);

/**
 * \brief Adds an item to the end of the array.
 *
 * This function appends the specified item to the end of the array, increasing the array's size by one.
 * The item is added by reference, and the internal array's data pointer may change if the array is resized.
 *
 * \warning This function increases the reference count of item, caller must free its own reference by calling \ref cardano_array_unref.
 *
 * \param[in] array Target array to which the item will be added.
 * \param[in] item  Pointer to the item to be added to the array.
 *
 * \return The new size of the array after the item has been added.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_array_push(cardano_array_t* array, cardano_object_t* item);

/**
 * \brief Pops an item from the end of a cardano_array_t.
 *
 * This function removes the last item from the array and returns it. If the array is empty,
 * NULL is returned and no operation is performed.
 *
 * \param[in] array A pointer to the cardano_array_t from which the item will be popped.
 *
 * \return On success, returns a pointer to the popped cardano_object_t item. The caller
 *         becomes the owner of this object and is responsible for calling the appropriate
 *         unref function on it. If the array is empty, returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_array_t* myArray = cardano_array_new();
 *
 * // Assume the array has been populated with items
 * cardano_object_t* item = cardano_array_pop(myArray);
 *
 * if (item != NULL)
 * {
 *   // Do something with the item
 *   cardano_object_unref(item); // Assume cardano_object_unref is the correct unref function
 * }
 * cardano_array_unref(&myArray);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_object_t* cardano_array_pop(cardano_array_t* array);

/**
 * \brief Fetches the current size (used space) of the array.
 *
 * \param[in] array Target array.
 * \return Used space in the array. Returns 0 if array is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_array_get_size(const cardano_array_t* array);

/**
 * Clears the contents of the specified array.
 *
 * This function removes all elements from the array, leaving it empty.
 *
 * @param[in,out] array The array to clear.
 */
CARDANO_EXPORT void cardano_array_clear(cardano_array_t* array);

/**
 * Sorts the elements of the array according to the provided comparison function.
 *
 * The comparison function should return a negative value if the first argument is less
 * than the second, zero if they are equal, and a positive value if the first is greater
 * than the second.
 *
 * @param[in,out] array The array to sort.
 * @param[in] compare The comparison function used to determine the order of the elements.
 *                    The function must not modify the elements.
 * @param[in] context An optional context pointer that will be passed to the compare function.
 */
CARDANO_EXPORT void cardano_array_sort(cardano_array_t* array, cardano_array_compare_item_t compare, void* context);

/**
 * Searches for an element in the array that satisfies a predicate.
 *
 * Iterates over the array elements and returns the first element for which
 * the predicate returns true. If no such element is found, returns NULL.
 *
 * @param[in] array The array to search in.
 * @param[in] predicate The unary predicate function used to evaluate each element.
 *                      The function should return true for the target element
 *                      and false otherwise.
 * @param[in] context An optional context pointer that will be passed to the predicate function.
 * @return A pointer to the first element satisfying the predicate, or NULL if
 *         no such element is found.
 */
CARDANO_NODISCARD
CARDANO_EXPORT
cardano_object_t* cardano_array_find(const cardano_array_t* array, cardano_array_unary_predicate_t predicate, const void* context);

/**
 * Creates and returns a new array containing only the elements that satisfy the given predicate.
 *
 * This function iterates over each element of the input array and applies the predicate function to it.
 * Elements for which the predicate returns true are included in the new array. The original array remains
 * unchanged. The caller is responsible for managing the unreferencing of the returned array to avoid memory leaks.
 *
 * @param[in] array The source array to filter.
 * @param[in] predicate The unary predicate function used to test each element. Elements that
 *                      satisfy the predicate (i.e., for which the predicate returns true) are
 *                      included in the new array.
 * @param[in] context An optional context pointer that will be passed to the predicate function.
 * @return A new array containing only elements that satisfy the predicate. Returns NULL if
 *         the operation fails due to memory allocation errors. The caller is responsible for unreferencing
 *         the returned array.
 */
CARDANO_NODISCARD
CARDANO_EXPORT
cardano_array_t* cardano_array_filter(const cardano_array_t* array, cardano_array_unary_predicate_t predicate, const void* context);

/**
 * \brief Fetches the array's total capacity.
 *
 * \param[in] array Target array.
 * \return Total capacity of the array. Returns 0 if array is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_array_get_capacity(const cardano_array_t* array);

/**
 * \brief Sets the last error message for a given object.
 *
 * This function records an error message in the object's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_array_get_last_error.
 *
 * \param[in,out] array A pointer to the \ref cardano_array_t instance whose last error
 *               message is to be set. If the object is \c NULL, the function
 *               has no effect.
 * \param[in] message A null-terminated string containing the error message to be
 *                recorded. If the message is \c NULL, the object's last_error
 *                will be set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters due to the fixed size
 *       of the last_error buffer (1024 characters), including the null
 *       terminator. Messages longer than this limit will be truncated.
 */
CARDANO_EXPORT void cardano_array_set_last_error(cardano_array_t* array, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific object.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_object_set_last_error for the given
 * object. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in,out] array A pointer to the \ref cardano_array_t instance whose last error
 *               message is to be retrieved. If the object is \c NULL, the function
 *               returns a generic error message indicating the null object.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified object. If the object is \c NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_array_set_last_error for the same object, or until
 *       the object is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_array_get_last_error(const cardano_array_t* array);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ARRAY_H
