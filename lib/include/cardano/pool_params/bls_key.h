/**
 * \file bls_key.h
 *
 * \author angel.castillo
 * \date   Sep 05, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BLS_KEY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BLS_KEY_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/json/json_writer.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A stake pool BLS key.
 *
 * A stake pool may register a BLS12-381 public key so that it can take part in Leios voting.
 * The key is registered together with a proof of possession, a signature that proves the pool
 * operator controls the matching private key. The public key is 96 bytes long and the proof
 * of possession is 48 bytes long.
 */
typedef struct cardano_bls_key_t cardano_bls_key_t;

/**
 * \brief Creates and initializes a new instance of a BLS key.
 *
 * This function allocates and initializes a new instance of \ref cardano_bls_key_t from the raw
 * bytes of the public key and its proof of possession.
 *
 * \param[in] public_key A pointer to a byte array containing the 96 bytes of the BLS public key.
 * \param[in] public_key_size The size of the public key byte array, which must be exactly 96 bytes.
 * \param[in] possession_proof A pointer to a byte array containing the 48 bytes of the proof of possession.
 * \param[in] possession_proof_size The size of the proof of possession byte array, which must be exactly 48 bytes.
 * \param[out] bls_key On successful initialization, this will point to the newly created
 *             \ref cardano_bls_key_t object. The object represents a "strong reference"
 *             to the BLS key, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the BLS key is no longer needed, the caller must
 *             release it by calling \ref cardano_bls_key_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the BLS key was successfully created, \ref CARDANO_ERROR_INVALID_ARGUMENT if either byte
 *         array has the wrong size, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t public_key[96] = { ... };
 * const byte_t possession_proof[48] = { ... };
 * cardano_bls_key_t* bls_key = NULL;
 *
 * cardano_error_t result = cardano_bls_key_new(public_key, sizeof(public_key), possession_proof, sizeof(possession_proof), &bls_key);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bls_key
 *
 *   // Once done, ensure to clean up and release the bls_key
 *   cardano_bls_key_unref(&bls_key);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bls_key_new(
  const byte_t*       public_key,
  size_t              public_key_size,
  const byte_t*       possession_proof,
  size_t              possession_proof_size,
  cardano_bls_key_t** bls_key);

/**
 * \brief Creates and initializes a new instance of a BLS key from hexadecimal strings.
 *
 * This function allocates and initializes a new instance of \ref cardano_bls_key_t from the
 * hexadecimal representation of the public key and its proof of possession.
 *
 * \param[in] public_key_hex A pointer to a character array containing the hexadecimal representation
 *                           of the 96 byte BLS public key.
 * \param[in] public_key_hex_size The length of the public key hexadecimal string, which must be exactly 192 characters.
 * \param[in] possession_proof_hex A pointer to a character array containing the hexadecimal representation
 *                                 of the 48 byte proof of possession.
 * \param[in] possession_proof_hex_size The length of the proof of possession hexadecimal string, which must be
 *                                      exactly 96 characters.
 * \param[out] bls_key On successful initialization, this will point to the newly created
 *             \ref cardano_bls_key_t object. The object represents a "strong reference"
 *             to the BLS key, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the BLS key is no longer needed, the caller must
 *             release it by calling \ref cardano_bls_key_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the BLS key was successfully created, \ref CARDANO_ERROR_INVALID_ARGUMENT if either string
 *         has the wrong length or is not valid hexadecimal, or an appropriate error code indicating
 *         the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* public_key_hex = "...";       // 192 hexadecimal characters
 * const char* possession_proof_hex = "..."; // 96 hexadecimal characters
 * cardano_bls_key_t* bls_key = NULL;
 *
 * cardano_error_t result = cardano_bls_key_new_from_hex(
 *   public_key_hex,
 *   strlen(public_key_hex),
 *   possession_proof_hex,
 *   strlen(possession_proof_hex),
 *   &bls_key);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bls_key
 *
 *   // Once done, ensure to clean up and release the bls_key
 *   cardano_bls_key_unref(&bls_key);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bls_key_new_from_hex(
  const char*         public_key_hex,
  size_t              public_key_hex_size,
  const char*         possession_proof_hex,
  size_t              possession_proof_hex_size,
  cardano_bls_key_t** bls_key);

/**
 * \brief Creates a \ref cardano_bls_key_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_bls_key_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a BLS key,
 * an array of two byte strings holding the public key and the proof of possession.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] bls_key A pointer to a pointer of \ref cardano_bls_key_t that will be set to the address
 *                     of the newly created BLS key object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_bls_key_t object by calling
 *       \ref cardano_bls_key_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_bls_key_t* bls_key = NULL;
 *
 * cardano_error_t result = cardano_bls_key_from_cbor(reader, &bls_key);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bls_key
 *
 *   // Once done, ensure to clean up and release the bls_key
 *   cardano_bls_key_unref(&bls_key);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode bls_key: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bls_key_from_cbor(cardano_cbor_reader_t* reader, cardano_bls_key_t** bls_key);

/**
 * \brief Serializes a BLS key into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_bls_key_t object using a \ref cardano_cbor_writer_t.
 * The key is written as an array of two byte strings holding the public key and the proof of possession.
 *
 * \param[in] bls_key A constant pointer to the \ref cardano_bls_key_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p bls_key or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bls_key_t* bls_key = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_bls_key_to_cbor(bls_key, writer);
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
 * cardano_bls_key_unref(&bls_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bls_key_to_cbor(
  const cardano_bls_key_t* bls_key,
  cardano_cbor_writer_t*   writer);

/**
 * \brief Serializes a BLS key to CIP-116 JSON.
 *
 * The function writes the full JSON object, including the surrounding braces.
 * Keys are written in the order: "public_key", "possession_proof". Both values are
 * written as lowercase hexadecimal strings.
 *
 * \param[in]  bls_key Pointer to a valid \ref cardano_bls_key_t.
 * \param[in]  writer  Pointer to a valid \ref cardano_json_writer_t.
 *
 * \return CARDANO_SUCCESS                On success.
 * CARDANO_ERROR_POINTER_IS_NULL          If \p bls_key or \p writer is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bls_key_to_cip116_json(
  const cardano_bls_key_t* bls_key,
  cardano_json_writer_t*   writer);

/**
 * \brief Retrieves the size of the BLS public key.
 *
 * This function returns the size of the byte array holding the BLS public key. Since BLS
 * public keys always have the same length, this function will consistently return 96.
 *
 * \param[in] bls_key A constant pointer to an initialized \ref cardano_bls_key_t object.
 *
 * \return The size in bytes of the public key, which is always 96. Returns 0 if the input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bls_key_t* bls_key = ...; // Assume bls_key is already initialized
 * size_t public_key_size = cardano_bls_key_get_public_key_size(bls_key);
 * printf("Size of the public key: %zu bytes\n", public_key_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bls_key_get_public_key_size(const cardano_bls_key_t* bls_key);

/**
 * \brief Retrieves a pointer to the bytes of the BLS public key.
 *
 * This function provides access to the raw bytes of the public key stored within a
 * \ref cardano_bls_key_t object. The returned pointer points to an internal data structure
 * of the BLS key object and must not be modified or freed by the caller. The data remains valid
 * as long as the BLS key object exists and has not been deallocated.
 *
 * \param[in] bls_key A constant pointer to an initialized \ref cardano_bls_key_t object.
 *
 * \return A constant pointer to the 96 bytes of the public key. Returns NULL if the input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bls_key_t* bls_key = ...; // Assume bls_key is already initialized
 * const byte_t* public_key = cardano_bls_key_get_public_key_bytes(bls_key);
 * size_t public_key_size = cardano_bls_key_get_public_key_size(bls_key);
 *
 * if (public_key != NULL)
 * {
 *   // Use the public key bytes
 * }
 * \endcode
 *
 * \note This function does not transfer ownership of the byte array to the caller. The returned
 *       pointer must not be freed or modified, and it should be used only while the BLS key object
 *       is valid.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_bls_key_get_public_key_bytes(const cardano_bls_key_t* bls_key);

/**
 * \brief Retrieves the size of the BLS proof of possession.
 *
 * This function returns the size of the byte array holding the proof of possession. Since BLS
 * proofs of possession always have the same length, this function will consistently return 48.
 *
 * \param[in] bls_key A constant pointer to an initialized \ref cardano_bls_key_t object.
 *
 * \return The size in bytes of the proof of possession, which is always 48. Returns 0 if the input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bls_key_t* bls_key = ...; // Assume bls_key is already initialized
 * size_t possession_proof_size = cardano_bls_key_get_possession_proof_size(bls_key);
 * printf("Size of the proof of possession: %zu bytes\n", possession_proof_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bls_key_get_possession_proof_size(const cardano_bls_key_t* bls_key);

/**
 * \brief Retrieves a pointer to the bytes of the BLS proof of possession.
 *
 * This function provides access to the raw bytes of the proof of possession stored within a
 * \ref cardano_bls_key_t object. The returned pointer points to an internal data structure
 * of the BLS key object and must not be modified or freed by the caller. The data remains valid
 * as long as the BLS key object exists and has not been deallocated.
 *
 * \param[in] bls_key A constant pointer to an initialized \ref cardano_bls_key_t object.
 *
 * \return A constant pointer to the 48 bytes of the proof of possession. Returns NULL if the input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bls_key_t* bls_key = ...; // Assume bls_key is already initialized
 * const byte_t* possession_proof = cardano_bls_key_get_possession_proof_bytes(bls_key);
 * size_t possession_proof_size = cardano_bls_key_get_possession_proof_size(bls_key);
 *
 * if (possession_proof != NULL)
 * {
 *   // Use the proof of possession bytes
 * }
 * \endcode
 *
 * \note This function does not transfer ownership of the byte array to the caller. The returned
 *       pointer must not be freed or modified, and it should be used only while the BLS key object
 *       is valid.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_bls_key_get_possession_proof_bytes(const cardano_bls_key_t* bls_key);

/**
 * \brief Decrements the reference count of a cardano_bls_key_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_bls_key_t object
 * by decreasing its reference count. When the reference count reaches zero, the BLS key is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] bls_key A pointer to the pointer of the BLS key object. This double
 *                        indirection allows the function to set the caller's pointer to
 *                        NULL, avoiding dangling pointer issues after the object has been
 *                        freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bls_key_t* bls_key = ...; // Assume bls_key is already initialized
 *
 * // Perform operations with the bls_key...
 *
 * cardano_bls_key_unref(&bls_key);
 * // At this point, bls_key is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_bls_key_unref, the pointer to the \ref cardano_bls_key_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_bls_key_unref(cardano_bls_key_t** bls_key);

/**
 * \brief Increases the reference count of the cardano_bls_key_t object.
 *
 * This function is used to manually increment the reference count of a cardano_bls_key_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_bls_key_unref.
 *
 * \param bls_key A pointer to the cardano_bls_key_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming bls_key is a previously created BLS key object
 *
 * cardano_bls_key_ref(bls_key);
 *
 * // Now bls_key can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_bls_key_ref there is a corresponding
 * call to \ref cardano_bls_key_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_bls_key_ref(cardano_bls_key_t* bls_key);

/**
 * \brief Retrieves the current reference count of the cardano_bls_key_t object.
 *
 * This function returns the number of active references to a cardano_bls_key_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_bls_key_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param bls_key A pointer to the cardano_bls_key_t object whose reference count is queried.
 *                The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_bls_key_t object. If the object
 * is properly managed (i.e., every \ref cardano_bls_key_ref call is matched with a
 * \ref cardano_bls_key_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming bls_key is a previously created BLS key object
 *
 * size_t ref_count = cardano_bls_key_refcount(bls_key);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bls_key_refcount(const cardano_bls_key_t* bls_key);

/**
 * \brief Sets the last error message for a given cardano_bls_key_t object.
 *
 * Records an error message in the BLS key's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] bls_key A pointer to the \ref cardano_bls_key_t instance whose last error message is
 *                    to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the BLS key's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_bls_key_set_last_error(
  cardano_bls_key_t* bls_key,
  const char*        message);

/**
 * \brief Retrieves the last error message recorded for a specific BLS key.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_bls_key_set_last_error for the given
 * BLS key. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] bls_key A pointer to the \ref cardano_bls_key_t instance whose last error
 *                    message is to be retrieved. If the BLS key is NULL, the function
 *                    returns a generic error message indicating the null BLS key.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified BLS key. If the BLS key is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_bls_key_set_last_error for the same BLS key, or until
 *       the BLS key is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_bls_key_get_last_error(
  const cardano_bls_key_t* bls_key);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BLS_KEY_H
