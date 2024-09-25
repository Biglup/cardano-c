/**
 * \file set.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SET_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

#include "array.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A function pointer type for hashing objects within a set.
 *
 * This callback function is designed to generate a hash value based on the content or
 * the identity of a `cardano_object_t` object. The function must return a 64-bit unsigned
 * integer representing the hash value. The quality of the hash function directly impacts
 * the performance of the set, particularly in terms of collision frequency and distribution
 * uniformity.
 *
 * Implementations of this function should aim to produce unique hash values for distinct
 * objects as much as possible. However, it's understood that hash collisions—different
 * objects producing the same hash value—might occur. The set implementation handles
 * these collisions gracefully.
 *
 * \param[in] object A pointer to the `cardano_object_t` object to be hashed.
 *
 * \return A 64-bit unsigned integer representing the hash value of the object.
 *
 * \note The implementer must ensure that the function is deterministic—calling the hash
 *       function with the same object should always return the same hash value.
 */
typedef uint64_t (*cardano_set_hash_func_t)(const cardano_object_t* object);

/**
 * \brief Function pointer type that compares two objects of the same type and returns a value
 * indicating whether one object is less than, equal to, or greater than the other.
 *
 * \param[in] lhs The left-hand side object to compare.
 * \param[in] rhs The right-hand side object to compare.
 *
 * \return A negative value if `lhs` is less than `rhs`, 0 if `lhs` is equal to `rhs`, or a positive
 * value if `lhs` is greater than `rhs`.
 */
typedef int (*cardano_set_compare_item_t)(const cardano_object_t* lhs, const cardano_object_t* rhs);

/**
 * \brief Defines a function pointer for a predicate to assess if an object in a set meets specific criteria.
 *
 * This typedef specifies a function pointer type for a predicate function designed to evaluate objects
 * within a set. The predicate function receives an object and an optional context, assessing whether the
 * object satisfies a predefined condition.
 *
 * \param[in] item The object being evaluated against the set criteria.
 * \param[in] context An optional parameter providing additional data for the predicate's evaluation.
 *
 * \return Returns \c true if the `item` conforms to the criteria defined within the predicate function;
 * \c false otherwise.
 */
typedef bool (*cardano_set_unary_predicate_t)(const cardano_object_t* item, const void* context);

/**
 * \brief A dynamic, reference-counted set.
 **/
typedef struct cardano_set_t cardano_set_t;

/**
 * \brief Creates a new dynamic set with the specified initial capacity.
 *
 * \return The newly created set or NULL on memory allocation failure.
 * The caller assumes ownership of the returned set and is responsible for its lifecycle.
 * It must be dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_set_t* cardano_set_new(cardano_set_compare_item_t compare, cardano_set_hash_func_t hash);

/**
 * \brief Creates a new set from an existing array.
 *
 * This function constructs a new set containing all unique elements from the given array.
 *
 * \param[in] array The source array from which to create the set. The function expects
 *                  a valid pointer to an initialized `cardano_array_t` instance. The caller
 *                  retains ownership of this array and must ensure it is properly deallocated
 *                  using `cardano_array_unref`.
 * \param[in] compare The comparison function used to determine the equality of two elements.
 *                    Elements are considered equal if the comparison function returns 0.
 *
 * \param[in] hash The hash function used to compute the hash value of each element for
 *                 placement within the set.
 *
 * \return A pointer to the newly created `cardano_set_t` instance if the operation is
 *         successful. Returns `NULL` if the set cannot be created due to invalid input
 *         parameters or memory allocation failure. The caller is responsible for calling
 *         `cardano_set_unref` on the returned set to ensure proper deallocation.
 *
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_set_t* cardano_set_from_array(cardano_array_t* array, cardano_set_compare_item_t compare, cardano_set_hash_func_t hash);

/**
 * \brief Decrements the set's reference count.
 *
 * If the reference count reaches zero, the set memory is deallocated.
 *
 * \param[in] set Pointer to the set whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_set_unref(cardano_set_t** set);

/**
 * \brief Increments the set's reference count.
 *
 * Ensures that the set remains allocated until the last reference is released.
 *
 * \param[in] set Set whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_set_ref(cardano_set_t* set);

/**
 * \brief Retrieves the set's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param[in] set Target set.
 * \return Current reference count of the set.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_set_refcount(const cardano_set_t* set);

/**
 * \brief Fetches a direct pointer to the set's data.
 *
 * \warning This returns a pointer to the internal data, not a copy. Do not manually deallocate.
 *
 * \param[in] set Target set.
 * \param[in] item Pointer to the data to be added.
 *
 * \return The new size of the set.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_set_add(cardano_set_t* set, cardano_object_t* item);

/**
 * \brief Checks if the specified item exists in the set.
 *
 * This function searches the set for the given item and determines whether it is present.
 * It uses the set's hash function to compute the item's hash value and then performs an
 * equality check to find an exact match within the set.
 *
 * \param[in] set A pointer to the set in which to search for the item.
 * \param[in] item A pointer to the object being searched for in the set.
 * \return Returns `true` if the item is found in the set, otherwise returns `false`.
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_set_has(cardano_set_t* set, cardano_object_t* item);

/**
 * \brief Removes a specified item from the set.
 *
 * This function attempts to find and remove the specified item from the set.
 * If the item is found, it is removed. The function uses the set's hash function
 * for efficient location of the item.
 *
 * \param[in] set A pointer to the set from which to remove the item.
 * \param[in] item A pointer to the object to be removed from the set.
 * \return Returns `true` if the item was successfully removed, otherwise returns `false`
 *         if the item was not found in the set.
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_set_delete(cardano_set_t* set, cardano_object_t* item);

/**
 * \brief Retrieves an array containing all the entries in the set.
 *
 * This function creates a new cardano_array_t and populates it with all the objects
 * currently held in the set.
 *
 * \param[in] set A pointer to the set from which to retrieve the entries.
 * \return A pointer to a `cardano_array_t` containing all set entries or NULL if the operation
 *         fails due to memory allocation errors.
 *
 * \note The order of the entries in the returned array is not specified and may vary.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_array_t* cardano_get_entries(cardano_set_t* set);

/**
 * \brief Fetches the current size (used space) of the set.
 *
 * \param[in] set Target set.
 * \return Used space in the set. Returns 0 if set is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_set_get_size(const cardano_set_t* set);

/**
 * Clears the contents of the specified set.
 *
 * This function removes all elements from the set, leaving it empty.
 *
 * \param[in,out] set The set to clear.
 */
CARDANO_EXPORT void cardano_set_clear(cardano_set_t* set);

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
cardano_object_t* cardano_set_find(const cardano_set_t* array, cardano_set_unary_predicate_t predicate, const void* context);

/**
 * \brief Sets the last error message for a given object.
 *
 * This function records an error message in the object's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_set_get_last_error.
 *
 * \param[in,out] set A pointer to the \ref cardano_set_t instance whose last error
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
CARDANO_EXPORT void cardano_set_set_last_error(cardano_set_t* set, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific object.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_set_set_last_error for the given
 * object. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in,out] set A pointer to the \ref cardano_set_t instance whose last error
 *               message is to be retrieved. If the object is \c NULL, the function
 *               returns a generic error message indicating the null object.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified object. If the object is \c NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_set_set_last_error for the same object, or until
 *       the object is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_set_get_last_error(const cardano_set_t* set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SET_H
