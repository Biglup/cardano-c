/**
 * \file constitution.h
 *
 * \author angel.castillo
 * \date   Aug 14, 2024
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

#ifndef CONSTITUTION_H
#define CONSTITUTION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/anchor.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The Cardano Constitution is a text document that defines Cardano's shared values and guiding principles.
 * At this stage, the Constitution is an informational document that unambiguously captures the core values
 * of Cardano and acts to ensure its long-term sustainability. At a later stage, we can imagine the Constitution
 * perhaps evolving into a smart-contract based set of rules that drives the entire governance framework.
 *
 * For now, however, the Constitution will remain an off-chain document whose hash digest value will be recorded
 * on-chain.
 */
typedef struct cardano_constitution_t cardano_constitution_t;

/**
 * \brief Creates and initializes a new instance of the Cardano Constitution.
 *
 * This function allocates and initializes a new instance of a \ref cardano_constitution_t object,
 * which represents the Cardano Constitution, a guiding document that defines Cardano's shared values and principles.
 * It ensures the long-term sustainability of the Cardano ecosystem by capturing these core values in a form
 * that is potentially verifiable on-chain through its hash.
 *
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object that links to the off-chain content of the constitution.
 * \param[in] script_hash An optional pointer to a \ref cardano_blake2b_hash_t object representing the hash of the constitution's script.
 *                        This parameter can be NULL if no hash is to be associated.
 * \param[out] constitution On successful initialization, this will point to a newly created
 *             \ref cardano_constitution_t object. This object represents a "strong reference"
 *             to the constitution, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the constitution is no longer needed, the caller must release it
 *             by calling \ref cardano_constitution_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the constitution was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Assume anchor is already initialized
 * cardano_blake2b_hash_t* script_hash = cardano_blake2b_hash_new(...); // Optionally initialized
 * cardano_constitution_t* constitution = NULL;
 * cardano_error_t result = cardano_constitution_new(anchor, script_hash, &constitution);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the constitution
 *   // Free resources when done
 *   cardano_constitution_unref(&constitution);
 * }
 * else
 * {
 *   printf("Failed to create the constitution: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup the anchor and optionally the script hash
 * cardano_anchor_unref(&anchor);
 *
 * if (script_hash)
 * {
 *   cardano_blake2b_hash_unref(&script_hash);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constitution_new(
  cardano_anchor_t*        anchor,
  cardano_blake2b_hash_t*  script_hash,
  cardano_constitution_t** constitution);

/**
 * \brief Creates a \ref cardano_constitution_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_constitution_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a constitution.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] constitution A pointer to a pointer of \ref cardano_constitution_t that will be set to the address
 *                        of the newly created constitution object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_constitution_t object by calling
 *       \ref cardano_constitution_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_constitution_t* constitution = NULL;
 *
 * cardano_error_t result = cardano_constitution_from_cbor(reader, &constitution);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the constitution
 *
 *   // Once done, ensure to clean up and release the constitution
 *   cardano_constitution_unref(&constitution);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode constitution: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constitution_from_cbor(cardano_cbor_reader_t* reader, cardano_constitution_t** constitution);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_constitution_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] constitution A constant pointer to the \ref cardano_constitution_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p constitution or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constitution_t* constitution = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_constitution_to_cbor(constitution, writer);
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
 * cardano_constitution_unref(&constitution);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_constitution_to_cbor(
  const cardano_constitution_t* constitution,
  cardano_cbor_writer_t*        writer);

/**
 * \brief Sets the anchor in the constitution.
 *
 * This function updates the anchor of a \ref cardano_constitution_t object. The anchor is used to link to the off-chain content of the constitution.
 *
 * \param[in,out] constitution A pointer to an initialized \ref cardano_constitution_t object to which the anchor will be set.
 * \param[in] anchor A pointer to an initialized \ref cardano_anchor_t object representing the new anchor.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the anchor was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constitution_t* constitution = ...; // Assume constitution is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Assume anchor is already initialized
 *
 * cardano_error_t result = cardano_constitution_set_anchor(constitution, anchor);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The anchor is now set for the constitution
 * }
 * else
 * {
 *   printf("Failed to set the anchor.\n");
 * }
 * // Clean up the constitution and anchor after use
 * cardano_constitution_unref(&constitution);
 * cardano_anchor_unref(&anchor);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constitution_set_anchor(cardano_constitution_t* constitution, cardano_anchor_t* anchor);

/**
 * \brief Gets the anchor from a constitution.
 *
 * This function retrieves the anchor from a given \ref cardano_constitution_t object. The anchor links to the off-chain content of the constitution.
 *
 * \param[in] constitution A pointer to an initialized \ref cardano_constitution_t object from which the anchor is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_anchor_t object representing the anchor. This will be a new reference,
 *         and the caller is responsible for releasing it with \ref cardano_anchor_unref when it is no longer needed.
 *         If the constitution does not have an anchor set, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constitution_t* constitution = ...; // Assume initialized
 * cardano_anchor_t* anchor = cardano_constitution_get_anchor(constitution);
 *
 * if (anchor != NULL)
 * {
 *   // Use the anchor
 *
 *   // Once done, ensure to clean up and release the anchor
 *   cardano_anchor_unref(&anchor);
 * }
 * else
 * {
 *   printf("ERROR: No anchor set for this constitution.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_anchor_t*
cardano_constitution_get_anchor(cardano_constitution_t* constitution);

/**
 * \brief Sets the script hash in the constitution.
 *
 * This function updates the script hash of a \ref cardano_constitution_t object.
 * The script hash is a \ref cardano_blake2b_hash_t object representing the hash of the off-chain constitution content.
 *
 * \param[in,out] constitution A pointer to an initialized \ref cardano_constitution_t object to which the script hash will be set.
 * \param[in] script_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the new script hash. This parameter
 *            can be NULL if the script hash is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script hash was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         constitution pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constitution_t* constitution = ...; // Assume constitution is already initialized
 * cardano_blake2b_hash_t* script_hash = cardano_blake2b_hash_new(...); // Assume script_hash is initialized
 *
 * cardano_error_t result = cardano_constitution_set_script_hash(constitution, script_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The script hash is now set for the constitution
 * }
 * else
 * {
 *   printf("Failed to set the script hash.\n");
 * }
 *
 * // Free resources when done
 * cardano_blake2b_hash_unref(script_hash);
 * cardano_constitution_unref(&constitution);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constitution_set_script_hash(cardano_constitution_t* constitution, cardano_blake2b_hash_t* script_hash);

/**
 * \brief Retrieves the script hash from a constitution.
 *
 * This function retrieves the script hash from a given \ref cardano_constitution_t object. The script hash
 * is represented as a \ref cardano_blake2b_hash_t object.
 *
 * \param[in] constitution A pointer to an initialized \ref cardano_constitution_t object from which the script hash is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_blake2b_hash_t object representing the script hash.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_blake2b_hash_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constitution_t* constitution = ...; // Assume initialized
 * cardano_blake2b_hash_t* script_hash = cardano_constitution_get_script_hash(constitution);
 *
 * if (script_hash != NULL)
 * {
 *   printf("Script Hash: %s\n", cardano_blake2b_hash_to_string(script_hash));
 *   // Use the script hash data
 *
 *   // Once done, ensure to clean up and release the script hash
 *   cardano_blake2b_hash_unref(&script_hash);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t*
cardano_constitution_get_script_hash(cardano_constitution_t* constitution);

/**
 * \brief Decrements the reference count of a cardano_constitution_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_constitution_t object
 * by decreasing its reference count. When the reference count reaches zero, the constitution is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] constitution A pointer to the pointer of the constitution object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constitution_t* constitution = cardano_constitution_new(major, minor);
 *
 * // Perform operations with the constitution...
 *
 * cardano_constitution_unref(&constitution);
 * // At this point, constitution is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_constitution_unref, the pointer to the \ref cardano_constitution_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_constitution_unref(cardano_constitution_t** constitution);

/**
 * \brief Increases the reference count of the cardano_constitution_t object.
 *
 * This function is used to manually increment the reference count of an cardano_constitution_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_constitution_unref.
 *
 * \param constitution A pointer to the cardano_constitution_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming constitution is a previously created constitution object
 *
 * cardano_constitution_ref(constitution);
 *
 * // Now constitution can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_constitution_ref there is a corresponding
 * call to \ref cardano_constitution_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_constitution_ref(cardano_constitution_t* constitution);

/**
 * \brief Retrieves the current reference count of the cardano_constitution_t object.
 *
 * This function returns the number of active references to an cardano_constitution_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_constitution_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param constitution A pointer to the cardano_constitution_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_constitution_t object. If the object
 * is properly managed (i.e., every \ref cardano_constitution_ref call is matched with a
 * \ref cardano_constitution_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming constitution is a previously created constitution object
 *
 * size_t ref_count = cardano_constitution_refcount(constitution);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_constitution_refcount(const cardano_constitution_t* constitution);

/**
 * \brief Sets the last error message for a given cardano_constitution_t object.
 *
 * Records an error message in the constitution's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] constitution A pointer to the \ref cardano_constitution_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the constitution's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_constitution_set_last_error(
  cardano_constitution_t* constitution,
  const char*             message);

/**
 * \brief Retrieves the last error message recorded for a specific constitution.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_constitution_set_last_error for the given
 * constitution. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] constitution A pointer to the \ref cardano_constitution_t instance whose last error
 *                   message is to be retrieved. If the constitution is NULL, the function
 *                   returns a generic error message indicating the null constitution.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified constitution. If the constitution is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_constitution_set_last_error for the same constitution, or until
 *       the constitution is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_constitution_get_last_error(
  const cardano_constitution_t* constitution);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CONSTITUTION_H