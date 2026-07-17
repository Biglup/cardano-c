/**
 * \file sub_transaction_set.h
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_SET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_SET_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents the set of sub transactions carried by a transaction body (the
 * sub transactions transaction body field).
 *
 * Sub transactions let several parties compose intents that are validated and applied atomically
 * as part of a single top-level transaction.
 *
 * On the wire this field is a non-empty array of sub transactions, optionally wrapped in CBOR
 * tag 258. The ledger models the field as an ordered map keyed by sub transaction id, but the ids
 * are never serialized; each id is derived by hashing the sub transaction body (blake2b-256).
 * The set preserves insertion order and rejects two sub transactions with the same id.
 */
typedef struct cardano_sub_transaction_set_t cardano_sub_transaction_set_t;

/**
 * \brief Creates and initializes a new instance of a sub_transaction_set.
 *
 * This function allocates and initializes a new instance of \ref cardano_sub_transaction_set_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] sub_transaction_set On successful initialization, this will point to a newly created
 *             \ref cardano_sub_transaction_set_t object. This object represents a "strong reference"
 *             to the sub_transaction_set, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the sub_transaction_set is no longer needed, the caller must release it
 *             by calling \ref cardano_sub_transaction_set_unref.
 *
 * \return \ref CARDANO_SUCCESS if the sub_transaction_set was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = NULL;
 *
 * // Attempt to create a new sub_transaction_set
 * cardano_error_t result = cardano_sub_transaction_set_new(&sub_transaction_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the sub_transaction_set
 *
 *   // Once done, ensure to clean up and release the sub_transaction_set
 *   cardano_sub_transaction_set_unref(&sub_transaction_set);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_set_new(cardano_sub_transaction_set_t** sub_transaction_set);

/**
 * \brief Creates a sub transaction set from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_sub_transaction_set_t object.
 * The set accepts both wire forms, with or without the set tag (258): an array of sub transactions. An empty set is a decoding error,
 * as are two sub transactions with the same id (the blake2b-256 hash of the sub transaction body).
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded sub transaction set data.
 * \param[out] sub_transaction_set A pointer to a pointer of \ref cardano_sub_transaction_set_t that will be set to the address
 *                        of the newly created sub_transaction_set object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the sub_transaction_set was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_sub_transaction_set_t object by calling
 *       \ref cardano_sub_transaction_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_sub_transaction_set_t* sub_transaction_set = NULL;
 *
 * cardano_error_t result = cardano_sub_transaction_set_from_cbor(reader, &sub_transaction_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the sub_transaction_set
 *
 *   // Once done, ensure to clean up and release the sub_transaction_set
 *   cardano_sub_transaction_set_unref(&sub_transaction_set);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode sub_transaction_set: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_set_from_cbor(cardano_cbor_reader_t* reader, cardano_sub_transaction_set_t** sub_transaction_set);

/**
 * \brief Serializes a sub transaction set into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_sub_transaction_set_t object using a \ref cardano_cbor_writer_t.
 * The set is written as an array of sub transactions in insertion order. The array is wrapped in CBOR tag 258
 * unless the set was decoded from an untagged array, in which case the untagged form is preserved so that the
 * set round-trips byte-exactly.
 *
 * \param[in] sub_transaction_set A constant pointer to the \ref cardano_sub_transaction_set_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p sub_transaction_set or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_sub_transaction_set_to_cbor(sub_transaction_set, writer);
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
 * cardano_sub_transaction_set_unref(&sub_transaction_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_to_cbor(
  const cardano_sub_transaction_set_t* sub_transaction_set,
  cardano_cbor_writer_t*               writer);

/**
 * \brief Retrieves the length of a sub transaction set.
 *
 * This function retrieves the number of elements in the provided \ref cardano_sub_transaction_set_t object.
 *
 * \param[in] sub_transaction_set A constant pointer to the \ref cardano_sub_transaction_set_t object whose length is to be retrieved.
 *
 * \return The number of elements in the sub_transaction_set. Return 0 if the sub_transaction_set is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = NULL;
 * cardano_error_t result = cardano_sub_transaction_set_new(&sub_transaction_set);
 *
 * // Populate sub_transaction_set with elements
 *
 * size_t length = cardano_sub_transaction_set_get_length(sub_transaction_set);
 * printf("Length of the sub_transaction_set: %zu\n", length);
 *
 * // Clean up the sub_transaction_set object once done
 * cardano_sub_transaction_set_unref(&sub_transaction_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_sub_transaction_set_get_length(const cardano_sub_transaction_set_t* sub_transaction_set);

/**
 * \brief Retrieves an element from a sub transaction set by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_sub_transaction_set_t object
 * and stores it in the output parameter. Elements keep the order in which they were decoded or added.
 *
 * \param[in] sub_transaction_set A constant pointer to the \ref cardano_sub_transaction_set_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the sub transaction set. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_sub_transaction_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_sub_transaction_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = NULL;
 * cardano_error_t result = cardano_sub_transaction_set_new(&sub_transaction_set);
 *
 * // Populate sub_transaction_set with elements
 *
 * cardano_sub_transaction_t* element = NULL;
 * result = cardano_sub_transaction_set_get(sub_transaction_set, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_sub_transaction_unref(&element);
 * }
 *
 * // Clean up the sub_transaction_set object once done
 * cardano_sub_transaction_set_unref(&sub_transaction_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_get(const cardano_sub_transaction_set_t* sub_transaction_set, size_t index, cardano_sub_transaction_t** element);

/**
 * \brief Adds an element to a sub transaction set.
 *
 * This function adds the specified element to the end of the provided \ref cardano_sub_transaction_set_t object.
 * Insertion order is preserved. Two sub transactions with the same id (the blake2b-256 hash of the sub
 * transaction body) are rejected.
 *
 * \param[in] sub_transaction_set A constant pointer to the \ref cardano_sub_transaction_set_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_sub_transaction_t object that is to be added to the sub_transaction_set.
 *                    The element will be referenced by the sub_transaction_set after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the sub_transaction_set,
 *         \ref CARDANO_ERROR_DUPLICATED_KEY if a sub transaction with the same id is already present in the set,
 *         or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = NULL;
 * cardano_error_t result = cardano_sub_transaction_set_new(&sub_transaction_set);
 *
 * // Create and initialize a new sub transaction element
 * cardano_sub_transaction_t* element = { ... };
 *
 * // Add the element to the sub_transaction_set
 * result = cardano_sub_transaction_set_add(sub_transaction_set, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the sub_transaction_set object once done
 * cardano_sub_transaction_set_unref(&sub_transaction_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_add(cardano_sub_transaction_set_t* sub_transaction_set, cardano_sub_transaction_t* element);

/**
 * \brief Finds a sub transaction in the set by its id.
 *
 * This function searches the provided \ref cardano_sub_transaction_set_t object for the sub transaction whose
 * id (the blake2b-256 hash of the sub transaction body) equals the given hash. The ids are never part of the
 * wire encoding; they are derived from the sub transaction bodies.
 *
 * \param[in] sub_transaction_set A constant pointer to the \ref cardano_sub_transaction_set_t object to be searched.
 * \param[in] id A constant pointer to the \ref cardano_blake2b_hash_t object with the sub transaction id to look up.
 * \param[out] element Pointer to a variable where the found element will be stored.
 *                     This variable will point to the found \ref cardano_sub_transaction_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_sub_transaction_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if a sub transaction with the given id was found,
 *         \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if no sub transaction in the set has the given id,
 *         or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = ...; // Assume sub_transaction_set is already initialized
 * cardano_blake2b_hash_t* id = ...; // Assume id is already initialized
 *
 * cardano_sub_transaction_t* element = NULL;
 * cardano_error_t result = cardano_sub_transaction_set_find_by_id(sub_transaction_set, id, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the found sub transaction
 *   cardano_sub_transaction_unref(&element);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("No sub transaction with the given id.\n");
 * }
 *
 * // Clean up the sub_transaction_set object once done
 * cardano_sub_transaction_set_unref(&sub_transaction_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_find_by_id(
  const cardano_sub_transaction_set_t* sub_transaction_set,
  const cardano_blake2b_hash_t*        id,
  cardano_sub_transaction_t**          element);

/**
 * \brief Determines if the sub transaction set uses tags in its encoding.
 *
 * This function checks whether the given \ref cardano_sub_transaction_set_t object
 * uses the Conway era encoding for sets, which are collections identified by a tag.
 *
 * \param[in] sub_transaction_set A pointer to an initialized \ref cardano_sub_transaction_set_t object.
 *
 * \return A boolean value:
 *         - \c true if the sub transaction set uses the Conway era encoding for sets (tagged sets).
 *         - \c false if the sub transaction set uses the older encoding (arrays without tags).
 *
 * \note This function helps in determining the serialization format of the sub transaction set,
 *       which may be necessary for compatibility with other transaction encoders/decoders.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = ...; // Assume sub_transaction_set is already initialized
 * bool is_tagged_set = cardano_sub_transaction_set_is_tagged(sub_transaction_set);
 *
 * if (is_tagged_set)
 * {
 *   printf("The sub transaction set uses Conway era tag for sets.\n");
 * }
 * else
 * {
 *   printf("The sub transaction set uses pre-Conway era encoding.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_sub_transaction_set_is_tagged(const cardano_sub_transaction_set_t* sub_transaction_set);

/**
 * \brief Enables or disables tagged encoding (Conway era feature) for the sub transaction set.
 *
 * This function sets whether the specified \ref cardano_sub_transaction_set_t object should use tagged encoding
 * (introduced in the Conway era) when serializing the set in CBOR. If \p use_tag is set to \c true, the set will be
 * encoded using tagged sets. Otherwise, it will use the older array-based encoding method.
 *
 * \param[in,out] sub_transaction_set A pointer to an initialized \ref cardano_sub_transaction_set_t object.
 * \param[in] use_tag A boolean value that determines whether to use tagged encoding (\c true) or legacy array encoding (\c false).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the operation was successful,
 *         or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = ...; // Assume sub_transaction_set is already initialized
 * cardano_error_t result = cardano_sub_transaction_set_set_use_tag(sub_transaction_set, true);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("The sub transaction set is now set to use tagged encoding.\n");
 * }
 * else
 * {
 *   printf("Failed to set tagged encoding: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_set_use_tag(cardano_sub_transaction_set_t* sub_transaction_set, bool use_tag);

/**
 * \brief Decrements the reference count of a sub_transaction_set object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_sub_transaction_set_t object
 * by decreasing its reference count. When the reference count reaches zero, the sub_transaction_set is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] sub_transaction_set A pointer to the pointer of the sub_transaction_set object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_set_t* sub_transaction_set = NULL;
 * cardano_error_t result = cardano_sub_transaction_set_new(&sub_transaction_set);
 *
 * // Perform operations with the sub_transaction_set...
 *
 * cardano_sub_transaction_set_unref(&sub_transaction_set);
 * // At this point, sub_transaction_set is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_sub_transaction_set_unref, the pointer to the \ref cardano_sub_transaction_set_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_sub_transaction_set_unref(cardano_sub_transaction_set_t** sub_transaction_set);

/**
 * \brief Increases the reference count of the cardano_sub_transaction_set_t object.
 *
 * This function is used to manually increment the reference count of a sub_transaction_set
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_sub_transaction_set_unref.
 *
 * \param sub_transaction_set A pointer to the sub_transaction_set object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction_set is a previously created sub_transaction_set object
 *
 * cardano_sub_transaction_set_ref(sub_transaction_set);
 *
 * // Now sub_transaction_set can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_sub_transaction_set_ref there is a corresponding
 * call to \ref cardano_sub_transaction_set_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_sub_transaction_set_ref(cardano_sub_transaction_set_t* sub_transaction_set);

/**
 * \brief Retrieves the current reference count of the cardano_sub_transaction_set_t object.
 *
 * This function returns the number of active references to a sub_transaction_set object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_sub_transaction_set_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param sub_transaction_set A pointer to the sub_transaction_set object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified sub_transaction_set object. If the object
 * is properly managed (i.e., every \ref cardano_sub_transaction_set_ref call is matched with a
 * \ref cardano_sub_transaction_set_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction_set is a previously created sub_transaction_set object
 *
 * size_t ref_count = cardano_sub_transaction_set_refcount(sub_transaction_set);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_sub_transaction_set_refcount(const cardano_sub_transaction_set_t* sub_transaction_set);

/**
 * \brief Sets the last error message for a given sub_transaction_set object.
 *
 * Records an error message in the sub_transaction_set's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] sub_transaction_set A pointer to the \ref cardano_sub_transaction_set_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the sub_transaction_set's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_sub_transaction_set_set_last_error(cardano_sub_transaction_set_t* sub_transaction_set, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific sub_transaction_set.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_sub_transaction_set_set_last_error for the given
 * sub_transaction_set. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] sub_transaction_set A pointer to the \ref cardano_sub_transaction_set_t instance whose last error
 *                   message is to be retrieved. If the sub_transaction_set is NULL, the function
 *                   returns a generic error message indicating the null sub_transaction_set.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified sub_transaction_set. If the sub_transaction_set is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_sub_transaction_set_set_last_error for the same sub_transaction_set, or until
 *       the sub_transaction_set is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_sub_transaction_set_get_last_error(const cardano_sub_transaction_set_t* sub_transaction_set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_SET_H
