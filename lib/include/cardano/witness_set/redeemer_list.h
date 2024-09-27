/**
 * \file redeemer_list.h
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_LIST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_LIST_H

/* INCLUDES ******************************************************************/

#include "redeemer_tag.h"
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
 * The Redeemer is an argument provided to a Plutus smart contract (script) when
 * you are attempting to redeem a UTxO that's protected by that script.
 */
typedef struct cardano_redeemer_t cardano_redeemer_t;

/**
 * \brief Represents a list of redeemers.
 */
typedef struct cardano_redeemer_list_t cardano_redeemer_list_t;

/**
 * \brief Creates and initializes a new instance of a redeemer_list.
 *
 * This function allocates and initializes a new instance of \ref cardano_redeemer_list_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] redeemer_list On successful initialization, this will point to a newly created
 *             \ref cardano_redeemer_list_t object. This object represents a "strong reference"
 *             to the redeemer_list, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the redeemer_list is no longer needed, the caller must release it
 *             by calling \ref cardano_redeemer_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the redeemer_list was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer_list = NULL;
 *
 * // Attempt to create a new redeemer_list
 * cardano_error_t result = cardano_redeemer_list_new(&redeemer_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the redeemer_list
 *
 *   // Once done, ensure to clean up and release the redeemer_list
 *   cardano_redeemer_list_unref(&redeemer_list);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_redeemer_list_new(cardano_redeemer_list_t** redeemer_list);

/**
 * \brief Creates a redeemer list from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_redeemer_list_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a redeemer.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded redeemer data.
 * \param[out] redeemers A pointer to a pointer of \ref cardano_redeemer_list_t that will be set to the address
 *                        of the newly created redeemer object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the redeemer was successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the redeemers and invalidate any existing signatures.
 *         To prevent this, when the redeemers list object is created using \ref cardano_redeemer_list_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_redeemer_list_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_redeemer_list_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_redeemer_list_t object by calling
 *       \ref cardano_redeemer_list_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_redeemer_list_t* redeemer = NULL;
 *
 * cardano_error_t result = cardano_redeemer_list_from_cbor(reader, &redeemer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the redeemer
 *
 *   // Once done, ensure to clean up and release the redeemer
 *   cardano_redeemer_list_unref(&redeemer);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode redeemer: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_redeemer_list_from_cbor(cardano_cbor_reader_t* reader, cardano_redeemer_list_t** redeemers);

/**
 * \brief Serializes a redeemer into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_redeemer_list_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] redeemers A constant pointer to the \ref cardano_redeemer_list_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p redeemer or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the redeemers and invalidate any existing signatures.
 *         To prevent this, when the redeemers list object is created using \ref cardano_redeemer_list_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_redeemer_list_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_redeemer_list_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_redeemer_list_to_cbor(redeemer, writer);
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
 * cardano_redeemer_list_unref(&redeemer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_list_to_cbor(
  const cardano_redeemer_list_t* redeemers,
  cardano_cbor_writer_t*         writer);

/**
 * \brief Retrieves the length of a redeemer list.
 *
 * This function retrieves the number of elements in the provided \ref cardano_redeemer_list_t object.
 *
 * \param[in] redeemer_list A constant pointer to the \ref cardano_redeemer_list_t object whose length is to be retrieved.
 *
 * \return The number of elements in the redeemer_list. Return 0 if the redeemer_list is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer_list = cardano_redeemer_list_new();
 *
 * // Populate redeemer_list with elements
 *
 * size_t length = cardano_redeemer_list_get_length(redeemer_list);
 * printf("Length of the redeemer_list: %zu\n", length);
 *
 * // Clean up the redeemer_list object once done
 * cardano_redeemer_list_unref(&redeemer_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_redeemer_list_get_length(const cardano_redeemer_list_t* redeemer_list);

/**
 * \brief Retrieves an element from a redeemer list by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_redeemer_list_t object
 * and stores it in the output parameter.
 *
 * \param[in] redeemer_list A constant pointer to the \ref cardano_redeemer_list_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the redeemer list. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_redeemer_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_redeemer_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer_list = cardano_redeemer_list_new();
 *
 * // Populate redeemer_list with elements
 *
 * cardano_redeemer_t* element = NULL;
 * cardano_error_t result = cardano_redeemer_list_get(redeemer_list, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_redeemer_unref(&element);
 * }
 *
 * // Clean up the redeemer_list object once done
 * cardano_redeemer_list_unref(&redeemer_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_list_get(const cardano_redeemer_list_t* redeemer_list, size_t index, cardano_redeemer_t** element);

/**
 * \brief Adds an element to a redeemer list.
 *
 * This function adds the specified element to the end of the provided \ref cardano_redeemer_list_t object.
 *
 * \param[in] redeemer_list A constant pointer to the \ref cardano_redeemer_list_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_redeemer_t object that is to be added to the redeemer_list.
 *                    The element will be referenced by the redeemer_list after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the redeemer_list, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer_list = cardano_redeemer_list_new();
 *
 * // Create and initialize a new redeemer element
 * cardano_redeemer_t* element = { ... };
 *
 * // Add the element to the redeemer_list
 * cardano_error_t result = cardano_redeemer_list_add(redeemer_list, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the redeemer_list object once done
 * cardano_redeemer_list_unref(&redeemer_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_list_add(cardano_redeemer_list_t* redeemer_list, cardano_redeemer_t* element);

/**
 * \brief Sets the execution units (memory and steps) for a specific redeemer in the redeemer list.
 *
 * This function sets the execution units (memory and steps) for a given redeemer identified by its tag and index within the \ref cardano_redeemer_list_t.
 *
 * \param[in,out] redeemer_list A pointer to an initialized \ref cardano_redeemer_list_t object where the execution units will be set.
 *                              This parameter must not be NULL.
 * \param[in] tag The \ref cardano_redeemer_tag_t representing the type of the redeemer (e.g., spending, minting).
 * \param[in] index The index of the redeemer for which the execution units will be set.
 * \param[in] mem The amount of memory required for the redeemer execution.
 * \param[in] steps The number of steps required for the redeemer execution.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the execution units were successfully set,
 *         or an appropriate error code indicating the failure reason (e.g., \ref CARDANO_ERROR_INDEX_OUT_OF_BOUNDS if the specified index is invalid).
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer_list = ...; // Assume redeemer_list is initialized
 * cardano_redeemer_tag_t tag = CARDANO_REDEEMER_TAG_SPEND;
 * uint64_t index = 0;
 * uint64_t mem = 5000;
 * uint64_t steps = 10000;
 *
 * cardano_error_t result = cardano_redeemer_list_set_ex_units(redeemer_list, tag, index, mem, steps);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Execution units set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set execution units: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_list_set_ex_units(
  cardano_redeemer_list_t* redeemer_list,
  cardano_redeemer_tag_t   tag,
  uint64_t                 index,
  uint64_t                 mem,
  uint64_t                 steps);

/**
 * \brief Deep clones a redeemer list.
 *
 * This function creates a deep copy of the specified \ref cardano_redeemer_list_t object. The cloned list contains copies of all the redeemers
 * and their associated data, ensuring that modifications to the original list do not affect the cloned list and vice versa.
 *
 * \param[in] redeemer_list A pointer to an initialized \ref cardano_redeemer_list_t object that will be cloned.
 *                          This parameter must not be NULL.
 * \param[out] cloned_redeemer_list On success, this will point to a newly created \ref cardano_redeemer_list_t object that is a deep copy of the original list.
 *                                  The caller is responsible for managing the lifecycle of the cloned list and must call \ref cardano_redeemer_list_unref to release it when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the redeemer list was successfully cloned,
 *         or an appropriate error code indicating the failure reason (e.g., \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if memory allocation fails).
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer_list = ...; // Assume redeemer_list is initialized
 * cardano_redeemer_list_t* cloned_redeemer_list = NULL;
 *
 * cardano_error_t result = cardano_redeemer_list_clone(redeemer_list, &cloned_redeemer_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Redeemer list successfully cloned.\n");
 *   // Use the cloned list
 *
 *   // Free the cloned list when no longer needed
 *   cardano_redeemer_list_unref(&cloned_redeemer_list);
 * }
 * else
 * {
 *   printf("Failed to clone redeemer list: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_list_clone(
  cardano_redeemer_list_t*  redeemer_list,
  cardano_redeemer_list_t** cloned_redeemer_list);

/**
 * \brief Clears the cached CBOR representation from a redeemer list.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_redeemer_list_t object.
 * It is useful when you have modified the redeemer list after it was created from CBOR using
 * \ref cardano_redeemer_list_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the redeemer list, rather than using the original cached CBOR.
 *
 * \param[in,out] redeemer_list A pointer to an initialized \ref cardano_redeemer_list_t object
 *                                 from which the CBOR cache will be cleared.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the redeemer list when
 *          serialized, which can alter the redeemer list and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume redeemer list was created using cardano_redeemer_list_from_cbor
 * cardano_redeemer_list_t* redeemer_list = ...;
 *
 * // Modify the redeemer list as needed

 * // Clear the CBOR cache to ensure serialization uses the updated redeemer list
 * cardano_redeemer_list_clear_cbor_cache(redeemer_list);
 *
 * // Serialize the redeemer list to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_redeemer_list_to_cbor(redeemer_list, writer);
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
 * cardano_redeemer_list_unref(&redeemer list);
 * \endcode
 */
CARDANO_EXPORT void cardano_redeemer_list_clear_cbor_cache(cardano_redeemer_list_t* redeemer_list);

/**
 * \brief Decrements the reference count of a redeemer_list object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_redeemer_list_t object
 * by decreasing its reference count. When the reference count reaches zero, the redeemer_list is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] redeemer_list A pointer to the pointer of the redeemer_list object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemer_list = cardano_redeemer_list_new();
 *
 * // Perform operations with the redeemer_list...
 *
 * cardano_redeemer_list_unref(&redeemer_list);
 * // At this point, redeemer_list is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_redeemer_list_unref, the pointer to the \ref cardano_redeemer_list_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_redeemer_list_unref(cardano_redeemer_list_t** redeemer_list);

/**
 * \brief Increases the reference count of the cardano_redeemer_list_t object.
 *
 * This function is used to manually increment the reference count of a redeemer_list
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_redeemer_list_unref.
 *
 * \param redeemer_list A pointer to the redeemer_list object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming redeemer_list is a previously created redeemer_list object
 *
 * cardano_redeemer_list_ref(redeemer_list);
 *
 * // Now redeemer_list can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_redeemer_list_ref there is a corresponding
 * call to \ref cardano_redeemer_list_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_redeemer_list_ref(cardano_redeemer_list_t* redeemer_list);

/**
 * \brief Retrieves the current reference count of the cardano_redeemer_list_t object.
 *
 * This function returns the number of active references to a redeemer_list object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_redeemer_list_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param redeemer_list A pointer to the redeemer_list object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified redeemer_list object. If the object
 * is properly managed (i.e., every \ref cardano_redeemer_list_ref call is matched with a
 * \ref cardano_redeemer_list_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming redeemer_list is a previously created redeemer_list object
 *
 * size_t ref_count = cardano_redeemer_list_refcount(redeemer_list);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_redeemer_list_refcount(const cardano_redeemer_list_t* redeemer_list);

/**
 * \brief Sets the last error message for a given redeemer_list object.
 *
 * Records an error message in the redeemer_list's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] redeemer_list A pointer to the \ref cardano_redeemer_list_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the redeemer_list's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_redeemer_list_set_last_error(cardano_redeemer_list_t* redeemer_list, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific redeemer_list.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_redeemer_list_set_last_error for the given
 * redeemer_list. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] redeemer_list A pointer to the \ref cardano_redeemer_list_t instance whose last error
 *                   message is to be retrieved. If the redeemer_list is NULL, the function
 *                   returns a generic error message indicating the null redeemer_list.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified redeemer_list. If the redeemer_list is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_redeemer_list_set_last_error for the same redeemer_list, or until
 *       the redeemer_list is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_redeemer_list_get_last_error(const cardano_redeemer_list_t* redeemer_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_LIST_H