/**
 * \file guard_set.h
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_GUARD_SET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_GUARD_SET_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents the set of credentials that must authorize a transaction (the guards
 * transaction body field).
 *
 * Guards generalize the required signers field: in addition to key hash credentials, a guard
 * can be a script hash credential whose script must be executed to authorize the transaction.
 *
 * On the wire this set has two forms: a set of bare key hashes (the historical required
 * signers encoding) and an ordered set of credentials. The set preserves insertion order and
 * rejects duplicates, since redeemers for script hash guards are indexed by the position of
 * the credential within this set.
 */
typedef struct cardano_guard_set_t cardano_guard_set_t;

/**
 * \brief Creates and initializes a new instance of a guard_set.
 *
 * This function allocates and initializes a new instance of \ref cardano_guard_set_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] guard_set On successful initialization, this will point to a newly created
 *             \ref cardano_guard_set_t object. This object represents a "strong reference"
 *             to the guard_set, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the guard_set is no longer needed, the caller must release it
 *             by calling \ref cardano_guard_set_unref.
 *
 * \return \ref CARDANO_SUCCESS if the guard_set was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = NULL;
 *
 * // Attempt to create a new guard_set
 * cardano_error_t result = cardano_guard_set_new(&guard_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the guard_set
 *
 *   // Once done, ensure to clean up and release the guard_set
 *   cardano_guard_set_unref(&guard_set);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_guard_set_new(cardano_guard_set_t** guard_set);

/**
 * \brief Creates a guard set from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_guard_set_t object.
 * The set accepts both wire forms, with or without the set tag (258): a set of bare 28 byte key hashes (each is wrapped as a
 * key hash credential) and an ordered set of credentials. The form is determined by the first element of the set; mixing
 * both forms within one set is a decoding error, as is an empty set or a duplicated element.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded guard set data.
 * \param[out] guard_set A pointer to a pointer of \ref cardano_guard_set_t that will be set to the address
 *                        of the newly created guard_set object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the guard_set was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_guard_set_t object by calling
 *       \ref cardano_guard_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_guard_set_t* guard_set = NULL;
 *
 * cardano_error_t result = cardano_guard_set_from_cbor(reader, &guard_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the guard_set
 *
 *   // Once done, ensure to clean up and release the guard_set
 *   cardano_guard_set_unref(&guard_set);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode guard_set: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_guard_set_from_cbor(cardano_cbor_reader_t* reader, cardano_guard_set_t** guard_set);

/**
 * \brief Serializes a guard set into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_guard_set_t object using a \ref cardano_cbor_writer_t.
 * The wire form is chosen by content: if every member is a key hash credential, the set is emitted as a set
 * of bare key hashes (the historical required signers encoding); if at least one member is a script hash
 * credential, the set is emitted as an ordered set of credentials.
 *
 * \param[in] guard_set A constant pointer to the \ref cardano_guard_set_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p guard_set or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_guard_set_to_cbor(guard_set, writer);
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
 * cardano_guard_set_unref(&guard_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_guard_set_to_cbor(
  const cardano_guard_set_t* guard_set,
  cardano_cbor_writer_t*     writer);

/**
 * \brief Retrieves the length of a guard set.
 *
 * This function retrieves the number of elements in the provided \ref cardano_guard_set_t object.
 *
 * \param[in] guard_set A constant pointer to the \ref cardano_guard_set_t object whose length is to be retrieved.
 *
 * \return The number of elements in the guard_set. Return 0 if the guard_set is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = NULL;
 * cardano_error_t result = cardano_guard_set_new(&guard_set);
 *
 * // Populate guard_set with elements
 *
 * size_t length = cardano_guard_set_get_length(guard_set);
 * printf("Length of the guard_set: %zu\n", length);
 *
 * // Clean up the guard_set object once done
 * cardano_guard_set_unref(&guard_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_guard_set_get_length(const cardano_guard_set_t* guard_set);

/**
 * \brief Retrieves an element from a guard set by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_guard_set_t object
 * and stores it in the output parameter. Elements keep the order in which they were decoded or added.
 *
 * \param[in] guard_set A constant pointer to the \ref cardano_guard_set_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the guard set. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_credential_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_credential_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = NULL;
 * cardano_error_t result = cardano_guard_set_new(&guard_set);
 *
 * // Populate guard_set with elements
 *
 * cardano_credential_t* element = NULL;
 * result = cardano_guard_set_get(guard_set, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_credential_unref(&element);
 * }
 *
 * // Clean up the guard_set object once done
 * cardano_guard_set_unref(&guard_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_guard_set_get(const cardano_guard_set_t* guard_set, size_t index, cardano_credential_t** element);

/**
 * \brief Adds an element to a guard set.
 *
 * This function adds the specified element to the end of the provided \ref cardano_guard_set_t object.
 * Insertion order is preserved, since redeemers for script hash guards are indexed by the position of
 * the credential within this set. Duplicated elements are rejected.
 *
 * \param[in] guard_set A constant pointer to the \ref cardano_guard_set_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_credential_t object that is to be added to the guard_set.
 *                    The element will be referenced by the guard_set after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the guard_set,
 *         \ref CARDANO_ERROR_DUPLICATED_KEY if the element is already present in the set, or an
 *         appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = NULL;
 * cardano_error_t result = cardano_guard_set_new(&guard_set);
 *
 * // Create and initialize a new credential element
 * cardano_credential_t* element = { ... };
 *
 * // Add the element to the guard_set
 * result = cardano_guard_set_add(guard_set, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the guard_set object once done
 * cardano_guard_set_unref(&guard_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_guard_set_add(cardano_guard_set_t* guard_set, cardano_credential_t* element);

/**
 * \brief Determines if the guard set uses tags in its encoding.
 *
 * This function checks whether the given \ref cardano_guard_set_t object
 * uses the Conway era encoding for sets, which are collections identified by a tag.
 *
 * \param[in] guard_set A pointer to an initialized \ref cardano_guard_set_t object.
 *
 * \return A boolean value:
 *         - \c true if the guard set uses the Conway era encoding for sets (tagged sets).
 *         - \c false if the guard set uses the older encoding (arrays without tags).
 *
 * \note This function helps in determining the serialization format of the guard set,
 *       which may be necessary for compatibility with other transaction encoders/decoders.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = ...; // Assume guard_set is already initialized
 * bool is_tagged_set = cardano_guard_set_is_tagged(guard_set);
 *
 * if (is_tagged_set)
 * {
 *   printf("The guard set uses Conway era tag for sets.\n");
 * }
 * else
 * {
 *   printf("The guard set uses pre-Conway era encoding.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_guard_set_is_tagged(const cardano_guard_set_t* guard_set);

/**
 * \brief Enables or disables tagged encoding (Conway era feature) for the guard set.
 *
 * This function sets whether the specified \ref cardano_guard_set_t object should use tagged encoding
 * (introduced in the Conway era) when serializing the set in CBOR. If \p use_tag is set to \c true, the set will be
 * encoded using tagged sets. Otherwise, it will use the older array-based encoding method.
 *
 * \param[in,out] guard_set A pointer to an initialized \ref cardano_guard_set_t object.
 * \param[in] use_tag A boolean value that determines whether to use tagged encoding (\c true) or legacy array encoding (\c false).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the operation was successful,
 *         or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = ...; // Assume guard_set is already initialized
 * cardano_error_t result = cardano_guard_set_set_use_tag(guard_set, true);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("The guard set is now set to use tagged encoding.\n");
 * }
 * else
 * {
 *   printf("Failed to set tagged encoding: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_guard_set_set_use_tag(cardano_guard_set_t* guard_set, bool use_tag);

/**
 * \brief Decrements the reference count of a guard_set object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_guard_set_t object
 * by decreasing its reference count. When the reference count reaches zero, the guard_set is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] guard_set A pointer to the pointer of the guard_set object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_guard_set_t* guard_set = NULL;
 * cardano_error_t result = cardano_guard_set_new(&guard_set);
 *
 * // Perform operations with the guard_set...
 *
 * cardano_guard_set_unref(&guard_set);
 * // At this point, guard_set is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_guard_set_unref, the pointer to the \ref cardano_guard_set_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_guard_set_unref(cardano_guard_set_t** guard_set);

/**
 * \brief Increases the reference count of the cardano_guard_set_t object.
 *
 * This function is used to manually increment the reference count of a guard_set
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_guard_set_unref.
 *
 * \param guard_set A pointer to the guard_set object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming guard_set is a previously created guard_set object
 *
 * cardano_guard_set_ref(guard_set);
 *
 * // Now guard_set can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_guard_set_ref there is a corresponding
 * call to \ref cardano_guard_set_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_guard_set_ref(cardano_guard_set_t* guard_set);

/**
 * \brief Retrieves the current reference count of the cardano_guard_set_t object.
 *
 * This function returns the number of active references to a guard_set object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_guard_set_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param guard_set A pointer to the guard_set object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified guard_set object. If the object
 * is properly managed (i.e., every \ref cardano_guard_set_ref call is matched with a
 * \ref cardano_guard_set_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming guard_set is a previously created guard_set object
 *
 * size_t ref_count = cardano_guard_set_refcount(guard_set);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_guard_set_refcount(const cardano_guard_set_t* guard_set);

/**
 * \brief Sets the last error message for a given guard_set object.
 *
 * Records an error message in the guard_set's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] guard_set A pointer to the \ref cardano_guard_set_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the guard_set's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_guard_set_set_last_error(cardano_guard_set_t* guard_set, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific guard_set.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_guard_set_set_last_error for the given
 * guard_set. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] guard_set A pointer to the \ref cardano_guard_set_t instance whose last error
 *                   message is to be retrieved. If the guard_set is NULL, the function
 *                   returns a generic error message indicating the null guard_set.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified guard_set. If the guard_set is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_guard_set_set_last_error for the same guard_set, or until
 *       the guard_set is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_guard_set_get_last_error(const cardano_guard_set_t* guard_set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_GUARD_SET_H
