/**
 * \file anchor.h
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
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

#ifndef CARDANO_ANCHOR_H
#define CARDANO_ANCHOR_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/anchor_type.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief An anchor is a pair of:
 *
 * - a URL to a JSON payload of metadata.
 * - a hash of the contents of the metadata URL.
 *
 * The on-chain rules will not check either the URL or the hash. Client applications should,
 * however, perform the usual sanity checks when fetching content from the provided URL.
 */
typedef struct cardano_anchor_t cardano_anchor_t;

/**
 * \brief Creates and initializes a new instance of a anchor.
 *
 * This function allocates and initializes a new instance of \ref cardano_anchor_t,
 * using the provided hash and anchor type. It returns an error code to indicate success
 * or failure of the operation.
 *
 * \param[in] hash A pointer to \ref cardano_blake2b_hash_t representing the hash associated
 *             with this anchor. The hash must be properly initialized before being
 *             passed to this function.
 * \param[in] type The type of anchor, as defined in \ref cardano_anchor_type_t,
 *             either a key hash or a script hash.
 * \param[out] anchor On successful initialization, this will point to a newly created
 *             \ref cardano_anchor_t object. This object represents a "strong reference"
 *             to the anchor, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the anchor is no longer needed, the caller must release it
 *             by calling \ref cardano_anchor_unref.
 *
 * \return \ref CARDANO_SUCCESS if the anchor was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* hash = { ... };  // Assume hash is initialized here
 * cardano_anchor_t* anchor = NULL;
 *
 * // Attempt to create a new anchor
 * cardano_error_t result = cardano_anchor_new(hash, CARDANO_anchor_TYPE_KEY_HASH, &anchor);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the anchor
 *
 *   // Once done, ensure to clean up and release the anchor
 *   cardano_anchor_unref(&anchor);
 * }
 *
 * cardano_blake2b_hash_unref(&hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_anchor_new(
  const char*                   url,
  size_t                        url_size,
  const cardano_blake2b_hash_t* hash,
  cardano_anchor_t**            anchor);

/**
 * \brief Creates a anchor from a hexadecimal hash string.
 *
 * This function constructs a \ref cardano_anchor_t object by interpreting the provided
 * hexadecimal string as a hash value and associating it with a specified anchor type. It
 * returns an error code indicating the success or failure of the operation.
 *
 * \param[in] hex A pointer to a character array containing the hexadecimal representation of the hash.
 * \param[in] hex_size The size of the hexadecimal string in bytes.
 * \param[in] type The type of anchor, as defined in \ref cardano_anchor_type_t,
 *                 which determines how the hash is to be treated (e.g., as a key hash or a script hash).
 * \param[out] anchor On successful initialization, this will point to a newly created
 *                 \ref cardano_anchor_t object. This object represents a "strong reference"
 *                 to the anchor, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the anchor is no longer needed, the caller must release it
 *                 by calling \ref cardano_anchor_unref.
 *
 * \return \ref CARDANO_SUCCESS if the anchor was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * \note The function assumes that the hexadecimal string is valid and correctly formatted.
 *       Malformed input may lead to incorrect or undefined behavior.
 *
 * Usage Example:
 * \code{.c}
 * const char* hash_hex = "abcdef1234567890abcdef1234567890abcdef12....";
 * size_t hex_size = strlen(hash_hex);
 * cardano_anchor_t* anchor = NULL;
 *
 * // Attempt to create a new anchor from a hexadecimal hash
 * cardano_error_t result = cardano_anchor_from_hash_hex(hash_hex, hex_size, CARDANO_anchor_TYPE_KEY_HASH, &anchor);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the anchor
 *
 *   // Once done, ensure to clean up and release the anchor
 *   cardano_anchor_unref(&anchor);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_anchor_from_hash_hex(
  const char*        url,
  size_t             url_size,
  const char*        hex,
  size_t             hex_size,
  cardano_anchor_t** anchor);

/**
 * \brief Creates a anchor from a byte array representing a hash.
 *
 * This function constructs a \ref cardano_anchor_t object by using the provided
 * byte array as a hash value and associating it with a specified anchor type. It
 * returns an error code indicating the success or failure of the operation.
 *
 * \param[in] data A pointer to the byte array containing the hash data.
 * \param[in] data_size The size of the byte array in bytes.
 * \param[in] type The type of anchor, as defined in \ref cardano_anchor_type_t,
 *                 which determines how the hash is to be treated (e.g., as a key hash or a script hash).
 * \param[out] anchor On successful initialization, this will point to a newly created
 *                 \ref cardano_anchor_t object. This object represents a "strong reference"
 *                 to the anchor, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the anchor is no longer needed, the caller must release it
 *                 by calling \ref cardano_anchor_unref.
 *
 * \return \ref CARDANO_SUCCESS if the anchor was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t hash_data[] = { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef ... };
 * size_t data_size = sizeof(hash_data);
 * cardano_anchor_t* anchor = NULL;
 *
 * // Attempt to create a new anchor from byte array hash
 * cardano_error_t result = cardano_anchor_from_hash_bytes(hash_data, data_size, CARDANO_anchor_TYPE_KEY_HASH, &anchor);
 *
 * if (result == CARDANO_SUCCESS && anchor)
 * {
 *   // Use the anchor
 *
 *   // Once done, ensure to clean up and release the anchor
 *   cardano_anchor_unref(&anchor);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_anchor_from_hash_bytes(
  const char*        url,
  size_t             url_size,
  const byte_t*      data,
  size_t             data_size,
  cardano_anchor_t** anchor);

/**
 * \brief Creates a anchor from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_anchor_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a anchor.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded anchor data.
 * \param[out] anchor A pointer to a pointer of \ref cardano_anchor_t that will be set to the address
 *                        of the newly created anchor object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the anchor was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_anchor_t object by calling
 *       \ref cardano_anchor_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_anchor_t* anchor = NULL;
 *
 * cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the anchor
 *
 *   // Once done, ensure to clean up and release the anchor
 *   cardano_anchor_unref(&anchor);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode anchor: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_anchor_from_cbor(cardano_cbor_reader_t* reader, cardano_anchor_t** anchor);

/**
 * \brief Serializes a anchor into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_anchor_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] anchor A constant pointer to the \ref cardano_anchor_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p anchor or \p writer
 *         is NULL, returns \ref CARDANO_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_anchor_t* anchor = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_anchor_to_cbor(anchor, writer);
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
 * cardano_anchor_unref(&anchor);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_anchor_to_cbor(
  const cardano_anchor_t* anchor,
  cardano_cbor_writer_t*  writer);

/**
 * \brief Retrieves the hash associated with a anchor.
 *
 * This function provides access to the hash part of a \ref cardano_anchor_t object.
 * It returns a new reference to a \ref cardano_blake2b_hash_t object representing the hash.
 * This allows the hash to be used independently of the original anchor object. The
 * reference count of the hash object is increased by one, making it the caller's responsibility
 * to release it by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \param anchor A constant pointer to the \ref cardano_anchor_t object from which
 *                   the hash is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_blake2b_hash_t object containing the hash.
 *         If the input anchor is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_blake2b_hash_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_anchor_t* original_anchor = cardano_anchor_new(...);
 * cardano_blake2b_hash_t* hash_anchor = cardano_anchor_get_hash(original_anchor);
 *
 * if (hash_anchor)
 * {
 *   // Use the hash anchor
 *
 *   // Once done, ensure to clean up and release the hash anchor
 *   cardano_blake2b_hash_unref(&hash_anchor);
 * }
 * // Release the original anchor after use
 * cardano_anchor_unref(&original_anchor);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_anchor_get_hash(const cardano_anchor_t* anchor);

/**
 * \brief Retrieves the byte array representation of the hash from a anchor.
 *
 * This function accesses the byte representation of the hash associated with a given
 * \ref cardano_anchor_t object. It provides a direct pointer to the internal byte array
 * representing the hash, which should not be modified or freed by the caller.
 *
 * \param anchor A constant pointer to the \ref cardano_anchor_t object from which
 *                   the hash bytes are to be retrieved.
 *
 * \return A pointer to a constant byte array containing the hash data. If the input anchor
 *         is NULL, returns NULL. The data remains valid as long as the anchor object is not
 *         freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_anchor_t* anchor = cardano_anchor_new(...);
 * const byte_t* hash_bytes = cardano_anchor_get_hash_bytes(anchor);
 *
 * if (hash_bytes)
 * {
 *   // Use the hash bytes for operations like comparison or display
 * }
 *
 * // Clean up the anchor object once done
 * cardano_anchor_unref(&anchor);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_anchor_get_hash_bytes(const cardano_anchor_t* anchor);

/**
 * \brief Retrieves the hexadecimal string representation of the hash from a anchor.
 *
 * This function provides access to the hexadecimal (hex) string representation of the hash
 * associated with a given \ref cardano_anchor_t object. It returns a direct pointer to the
 * internal hex string which should not be modified or freed by the caller.
 *
 * \param anchor A constant pointer to the \ref cardano_anchor_t object from which
 *                   the hex string of the hash is to be retrieved. The object must not be NULL.
 *
 * \return A pointer to a constant character array containing the hex representation of the hash.
 *         If the input anchor is NULL, returns NULL. The data remains valid as long as the
 *         anchor object is not freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_anchor_t* anchor = cardano_anchor_new(...);
 * const char* hash_hex = cardano_anchor_get_hash_hex(anchor);
 *
 * if (hash_hex)
 * {
 *   // Use the hash hex for operations like logging, display, or comparison
 * }
 *
 * // Clean up the anchor object once done
 * cardano_anchor_unref(&anchor);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_anchor_get_hash_hex(const cardano_anchor_t* anchor);

/**
 * \brief Decrements the reference count of a anchor object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_anchor_t object
 * by decreasing its reference count. When the reference count reaches zero, the anchor is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] anchor A pointer to the pointer of the anchor object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_anchor_t* anchor = cardano_anchor_new();
 *
 * // Perform operations with the anchor...
 *
 * cardano_anchor_unref(&anchor);
 * // At this point, anchor is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_anchor_unref, the pointer to the \ref cardano_anchor_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_anchor_unref(cardano_anchor_t** anchor);

/**
 * \brief Increases the reference count of the cardano_anchor_t object.
 *
 * This function is used to manually increment the reference count of a anchor
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_anchor_unref.
 *
 * \param anchor A pointer to the anchor object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming anchor is a previously created anchor object
 *
 * cardano_anchor_ref(anchor);
 *
 * // Now anchor can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_anchor_ref there is a corresponding
 * call to \ref cardano_anchor_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_anchor_ref(cardano_anchor_t* anchor);

/**
 * \brief Retrieves the current reference count of the cardano_anchor_t object.
 *
 * This function returns the number of active references to a anchor object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_anchor_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param anchor A pointer to the anchor object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified anchor object. If the object
 * is properly managed (i.e., every \ref cardano_anchor_ref call is matched with a
 * \ref cardano_anchor_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming anchor is a previously created anchor object
 *
 * size_t ref_count = cardano_anchor_refcount(anchor);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_anchor_refcount(const cardano_anchor_t* anchor);

/**
 * \brief Sets the last error message for a given anchor object.
 *
 * Records an error message in the anchor's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] anchor A pointer to the \ref cardano_anchor_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the anchor's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_anchor_set_last_error(cardano_anchor_t* anchor, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific anchor.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_anchor_set_last_error for the given
 * anchor. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] anchor A pointer to the \ref cardano_anchor_t instance whose last error
 *                   message is to be retrieved. If the anchor is NULL, the function
 *                   returns a generic error message indicating the null anchor.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified anchor. If the anchor is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_anchor_set_last_error for the same anchor, or until
 *       the anchor is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_anchor_get_last_error(const cardano_anchor_t* anchor);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_ANCHOR_H