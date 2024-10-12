/**
 * \file vkey_witness_set.h
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VKEY_WITNESS_SET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VKEY_WITNESS_SET_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>
#include <cardano/witness_set/vkey_witness.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief vkey Witness (Verification Key Witness) is a component of a transaction that
 * provides cryptographic proof that proves that the creator of the transaction
 * has access to the private keys controlling the UTxOs being spent.
 */
typedef struct cardano_vkey_witness_t cardano_vkey_witness_t;

/**
 * \brief Represents a set of vkey_witnesss.
 */
typedef struct cardano_vkey_witness_set_t cardano_vkey_witness_set_t;

/**
 * \brief Creates and initializes a new instance of a vkey_witness_set.
 *
 * This function allocates and initializes a new instance of \ref cardano_vkey_witness_set_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] vkey_witness_set On successful initialization, this will point to a newly created
 *             \ref cardano_vkey_witness_set_t object. This object represents a "strong reference"
 *             to the vkey_witness_set, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the vkey_witness_set is no longer needed, the caller must release it
 *             by calling \ref cardano_vkey_witness_set_unref.
 *
 * \return \ref CARDANO_SUCCESS if the vkey_witness_set was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness_set = NULL;
 *
 * // Attempt to create a new vkey_witness_set
 * cardano_error_t result = cardano_vkey_witness_set_new(&vkey_witness_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the vkey_witness_set
 *
 *   // Once done, ensure to clean up and release the vkey_witness_set
 *   cardano_vkey_witness_set_unref(&vkey_witness_set);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_vkey_witness_set_new(cardano_vkey_witness_set_t** vkey_witness_set);

/**
 * \brief Creates a vkey_witness set from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_vkey_witness_set_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a vkey_witness.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded vkey_witness data.
 * \param[out] vkey_witnesss A pointer to a pointer of \ref cardano_vkey_witness_set_t that will be set to the address
 *                        of the newly created vkey_witness object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the vkey_witness was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_vkey_witness_set_t object by calling
 *       \ref cardano_vkey_witness_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_vkey_witness_set_t* vkey_witness = NULL;
 *
 * cardano_error_t result = cardano_vkey_witness_set_from_cbor(reader, &vkey_witness);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the vkey_witness
 *
 *   // Once done, ensure to clean up and release the vkey_witness
 *   cardano_vkey_witness_set_unref(&vkey_witness);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode vkey_witness: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_vkey_witness_set_from_cbor(cardano_cbor_reader_t* reader, cardano_vkey_witness_set_t** vkey_witnesss);

/**
 * \brief Serializes a vkey_witness into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_vkey_witness_set_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] vkey_witnesss A constant pointer to the \ref cardano_vkey_witness_set_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p vkey_witness or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_vkey_witness_set_to_cbor(vkey_witness, writer);
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
 * cardano_vkey_witness_set_unref(&vkey_witness);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_set_to_cbor(
  const cardano_vkey_witness_set_t* vkey_witnesss,
  cardano_cbor_writer_t*            writer);

/**
 * \brief Retrieves the length of a vkey_witness list.
 *
 * This function retrieves the number of elements in the provided \ref cardano_vkey_witness_set_t object.
 *
 * \param[in] vkey_witness_set A constant pointer to the \ref cardano_vkey_witness_set_t object whose length is to be retrieved.
 *
 * \return The number of elements in the vkey_witness_set. Return 0 if the vkey_witness_set is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness_set = cardano_vkey_witness_set_new();
 *
 * // Populate vkey_witness_set with elements
 *
 * size_t length = cardano_vkey_witness_set_get_length(vkey_witness_set);
 * printf("Length of the vkey_witness_set: %zu\n", length);
 *
 * // Clean up the vkey_witness_set object once done
 * cardano_vkey_witness_set_unref(&vkey_witness_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_vkey_witness_set_get_length(const cardano_vkey_witness_set_t* vkey_witness_set);

/**
 * \brief Retrieves an element from a vkey_witness list by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_vkey_witness_set_t object
 * and stores it in the output parameter.
 *
 * \param[in] vkey_witness_set A constant pointer to the \ref cardano_vkey_witness_set_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the vkey_witness list. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_vkey_witness_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_vkey_witness_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness_set = cardano_vkey_witness_set_new();
 *
 * // Populate vkey_witness_set with elements
 *
 * cardano_vkey_witness_t* element = NULL;
 * cardano_error_t result = cardano_vkey_witness_set_get(vkey_witness_set, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_vkey_witness_unref(&element);
 * }
 *
 * // Clean up the vkey_witness_set object once done
 * cardano_vkey_witness_set_unref(&vkey_witness_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_set_get(const cardano_vkey_witness_set_t* vkey_witness_set, size_t index, cardano_vkey_witness_t** element);

/**
 * \brief Adds an element to a vkey_witness list, replacing existing signatures for duplicate public keys.
 *
 * This function adds the specified element to the end of the provided \ref cardano_vkey_witness_set_t object.
 * If an element with the same public key is already present in the set, the signature for that public key will be replaced
 * with the new signature provided by the added \ref cardano_vkey_witness_t element.
 *
 * \param[in] vkey_witness_set A pointer to the \ref cardano_vkey_witness_set_t object to which
 *                             the element is to be added.
 * \param[in] element Pointer to the \ref cardano_vkey_witness_t object that is to be added to the vkey_witness_set.
 *                    The element will be referenced by the vkey_witness_set after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the vkey_witness_set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness_set = cardano_vkey_witness_set_new();
 *
 * // Create and initialize a new vkey_witness element
 * cardano_vkey_witness_t* element = { ... };
 *
 * // Add the element to the vkey_witness_set
 * cardano_error_t result = cardano_vkey_witness_set_add(vkey_witness_set, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the vkey_witness_set object once done
 * cardano_vkey_witness_set_unref(&vkey_witness_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_set_add(cardano_vkey_witness_set_t* vkey_witness_set, cardano_vkey_witness_t* element);

/**
 * \brief Merges new vkey_witnesses into an existing vkey_witness_set.
 *
 * This function applies the elements from the \c new_vkey_witnesses set into the \c vkey_witness_set.
 * If a witness in the new set has the same public key as one already in the original set, the signature in the
 * original witness will be replaced with the new signature from the \c new_vkey_witnesses set.
 *
 * \param[in] vkey_witness_set A pointer to the \ref cardano_vkey_witness_set_t object where new vkey_witnesses will be applied.
 * \param[in] new_vkey_witnesses A pointer to the \ref cardano_vkey_witness_set_t object containing the new witnesses to apply.
 *
 * \return \ref CARDANO_SUCCESS if the merge was successful, or an appropriate error code indicating failure.
 *
 * \note This function ensures that any existing witness signatures for the same public key in the original set
 * are replaced with new ones from the \c new_vkey_witnesses set.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* original_witnesses = cardano_vkey_witness_set_new();
 * cardano_vkey_witness_set_t* new_witnesses = cardano_vkey_witness_set_new();
 *
 * // Populate new_witnesses and original_witnesses
 *
 * // Apply the new witnesses to the original set
 * cardano_error_t result = cardano_vkey_witness_set_apply(original_witnesses, new_witnesses);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The new witnesses were successfully applied to the original set
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_set_apply(cardano_vkey_witness_set_t* vkey_witness_set, cardano_vkey_witness_set_t* new_vkey_witnesses);

/**
 * \brief Checks if the vkey witness set uses tagged encoding (Conway era feature).
 *
 * This function determines whether the specified \ref cardano_vkey_witness_set_t object uses tagged encoding for sets,
 * introduced in the Conway era of the Cardano blockchain. Tagged sets are a new way of encoding sets in CBOR, which
 * differs from the older array-based representation used in previous eras.
 *
 * \param[in] vkey_witness_set A pointer to an initialized \ref cardano_vkey_witness_set_t object.
 *
 * \return \c true if the vkey witness set uses tagged encoding for sets; \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness_set = ...; // Assume vkey_witness_set is already initialized
 *
 * bool uses_tag = cardano_vkey_witness_set_get_use_tag(vkey_witness_set);
 * if (uses_tag)
 * {
 *   printf("The vkey witness set uses tagged encoding.\n");
 * }
 * else
 * {
 *   printf("The vkey witness set uses legacy array encoding.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_vkey_witness_set_get_use_tag(const cardano_vkey_witness_set_t* vkey_witness_set);

/**
 * \brief Enables or disables tagged encoding (Conway era feature) for the vkey witness set.
 *
 * This function sets whether the specified \ref cardano_vkey_witness_set_t object should use tagged encoding
 * (introduced in the Conway era) when serializing sets in CBOR. If \p use_tag is set to \c true, the vkey witness set will be encoded
 * using tagged sets. Otherwise, it will use the older array-based encoding method.
 *
 * \param[in,out] vkey_witness_set A pointer to an initialized \ref cardano_vkey_witness_set_t object.
 * \param[in] use_tag A boolean value that determines whether to use tagged encoding (\c true) or legacy array encoding (\c false).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the operation was successful,
 *         or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness = ...; // Assume vkey_witness is already initialized
 * cardano_error_t result = cardano_vkey_witness_set_set_use_tag(vkey_witness, true);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("The vkey witness set is now set to use tagged encoding.\n");
 * }
 * else
 * {
 *   printf("Failed to set tagged encoding: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_set_set_use_tag(cardano_vkey_witness_set_t* vkey_witness_set, bool use_tag);

/**
 * \brief Decrements the reference count of a vkey_witness_set object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_vkey_witness_set_t object
 * by decreasing its reference count. When the reference count reaches zero, the vkey_witness_set is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] vkey_witness_set A pointer to the pointer of the vkey_witness_set object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_set_t* vkey_witness_set = cardano_vkey_witness_set_new();
 *
 * // Perform operations with the vkey_witness_set...
 *
 * cardano_vkey_witness_set_unref(&vkey_witness_set);
 * // At this point, vkey_witness_set is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_vkey_witness_set_unref, the pointer to the \ref cardano_vkey_witness_set_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_vkey_witness_set_unref(cardano_vkey_witness_set_t** vkey_witness_set);

/**
 * \brief Increases the reference count of the cardano_vkey_witness_set_t object.
 *
 * This function is used to manually increment the reference count of a vkey_witness_set
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_vkey_witness_set_unref.
 *
 * \param vkey_witness_set A pointer to the vkey_witness_set object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming vkey_witness_set is a previously created vkey_witness_set object
 *
 * cardano_vkey_witness_set_ref(vkey_witness_set);
 *
 * // Now vkey_witness_set can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_vkey_witness_set_ref there is a corresponding
 * call to \ref cardano_vkey_witness_set_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_vkey_witness_set_ref(cardano_vkey_witness_set_t* vkey_witness_set);

/**
 * \brief Retrieves the current reference count of the cardano_vkey_witness_set_t object.
 *
 * This function returns the number of active references to a vkey_witness_set object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_vkey_witness_set_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param vkey_witness_set A pointer to the vkey_witness_set object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified vkey_witness_set object. If the object
 * is properly managed (i.e., every \ref cardano_vkey_witness_set_ref call is matched with a
 * \ref cardano_vkey_witness_set_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming vkey_witness_set is a previously created vkey_witness_set object
 *
 * size_t ref_count = cardano_vkey_witness_set_refcount(vkey_witness_set);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_vkey_witness_set_refcount(const cardano_vkey_witness_set_t* vkey_witness_set);

/**
 * \brief Sets the last error message for a given vkey_witness_set object.
 *
 * Records an error message in the vkey_witness_set's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] vkey_witness_set A pointer to the \ref cardano_vkey_witness_set_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the vkey_witness_set's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_vkey_witness_set_set_last_error(cardano_vkey_witness_set_t* vkey_witness_set, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific vkey_witness_set.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_vkey_witness_set_set_last_error for the given
 * vkey_witness_set. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] vkey_witness_set A pointer to the \ref cardano_vkey_witness_set_t instance whose last error
 *                   message is to be retrieved. If the vkey_witness_set is NULL, the function
 *                   returns a generic error message indicating the null vkey_witness_set.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified vkey_witness_set. If the vkey_witness_set is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_vkey_witness_set_set_last_error for the same vkey_witness_set, or until
 *       the vkey_witness_set is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_vkey_witness_set_get_last_error(const cardano_vkey_witness_set_t* vkey_witness_set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VKEY_WITNESS_SET_H