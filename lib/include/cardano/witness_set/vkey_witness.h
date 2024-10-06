/**
 * \file vkey_witness.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VKEY_WITNESS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VKEY_WITNESS_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

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
 * \brief Creates and initializes a new instance of a Vkey Witness.
 *
 * This function allocates and initializes a new instance of a \ref cardano_vkey_witness_t object.
 *
 * \param[in] vkey A pointer to an initialized \ref cardano_ed25519_public_key_t object representing
 *                 the public verification key (VKey). This key is used to verify the cryptographic signature.
 * \param[in] signature A pointer to an initialized \ref cardano_ed25519_signature_t object containing
 *                      the cryptographic signature. This signature is produced by signing the hash
 *                      of the transaction body with the corresponding private key. The VKey is used to
 *                      verify that the signature is valid and was made by the owner of the corresponding private key.
 * \param[out] vkey_witness On successful initialization, this will point to a newly created
 *                          \ref cardano_vkey_witness_t object. This object represents a "strong reference"
 *                          to the vkey witness, meaning that it is fully initialized and ready for use.
 *                          The caller is responsible for managing the lifecycle of this object.
 *                          Specifically, once the vkey witness is no longer needed, the caller
 *                          must release it by calling \ref cardano_vkey_witness_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         vkey witness was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ed25519_public_key_t* vkey = ...; // Assume vkey is already initialized
 * cardano_ed25519_signature_t* signature = ...; // Assume signature is already initialized
 * cardano_vkey_witness_t* vkey_witness = NULL;
 *
 * cardano_error_t result = cardano_vkey_witness_new(vkey, signature, &vkey_witness);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the vkey_witness
 *   // Once done, ensure to clean up and release the vkey_witness
 *   cardano_vkey_witness_unref(&vkey_witness);
 * }
 * else
 * {
 *   printf("Failed to create the vkey witness.\n");
 * }
 *
 * // Cleanup the other inputs
 * cardano_ed25519_public_key_unref(&vkey);
 * cardano_ed25519_signature_unref(&signature);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_vkey_witness_new(
  cardano_ed25519_public_key_t* vkey,
  cardano_ed25519_signature_t*  signature,
  cardano_vkey_witness_t**      vkey_witness);

/**
 * \brief Creates a \ref cardano_vkey_witness_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_vkey_witness_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a vkey_witness.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] vkey_witness A pointer to a pointer of \ref cardano_vkey_witness_t that will be set to the address
 *                        of the newly created vkey_witness object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the vkey witness were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_vkey_witness_t object by calling
 *       \ref cardano_vkey_witness_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_vkey_witness_t* vkey_witness = NULL;
 *
 * cardano_error_t result = cardano_vkey_witness_from_cbor(reader, &vkey_witness);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the vkey_witness
 *
 *   // Once done, ensure to clean up and release the vkey_witness
 *   cardano_vkey_witness_unref(&vkey_witness);
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
cardano_vkey_witness_from_cbor(cardano_cbor_reader_t* reader, cardano_vkey_witness_t** vkey_witness);

/**
 * \brief Serializes vkey witness into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_vkey_witness_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] vkey_witness A constant pointer to the \ref cardano_vkey_witness_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p vkey_witness or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_t* vkey_witness = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_vkey_witness_to_cbor(vkey_witness, writer);
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
 * cardano_vkey_witness_unref(&vkey_witness);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_to_cbor(
  const cardano_vkey_witness_t* vkey_witness,
  cardano_cbor_writer_t*        writer);

/**
 * \brief Checks if a vkey_witness contains a specific public key.
 *
 * This function checks if the provided \ref cardano_vkey_witness_t object contains the specified Ed25519 public key.
 * It is useful for determining if a particular witness corresponds to a given public key.
 *
 * \param[in] vkey_witness A constant pointer to the \ref cardano_vkey_witness_t object to be checked.
 * \param[in] vkey A constant pointer to the \ref cardano_ed25519_public_key_t object representing the public key to search for.
 *
 * \return `true` if the public key is found in the vkey_witness; otherwise, `false`.
 *
 * \note This function only compares the public key, and not the associated signature or any other witness data.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_t* vkey_witness = { ... };
 * cardano_ed25519_public_key_t* public_key = { ... };
 *
 * // Check if the vkey_witness contains the specified public key
 * bool has_key = cardano_vkey_witness_has_public_key(vkey_witness, public_key);
 *
 * if (has_key)
 * {
 *   // The vkey_witness contains the public key
 * }
 * else
 * {
 *   // The public key is not present in the vkey_witness
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool
cardano_vkey_witness_has_public_key(const cardano_vkey_witness_t* vkey_witness, const cardano_ed25519_public_key_t* vkey);

/**
 * \brief Retrieves the verification key (VKey) from a Vkey Witness.
 *
 * This function returns the \ref cardano_ed25519_public_key_t object representing the public verification key
 * (VKey) associated with a \ref cardano_vkey_witness_t object. The VKey is used to verify the cryptographic
 * signature provided in the vkey witness.
 *
 * \param[in] vkey_witness A pointer to an initialized \ref cardano_vkey_witness_t object from
 *                              which the verification key will be retrieved.
 *
 * \return A pointer to the \ref cardano_ed25519_public_key_t object representing the verification key.
 *         The caller is responsible for managing the lifecycle of the returned VKey. Specifically, the caller
 *         must release it by calling \ref cardano_ed25519_public_key_unref when it is no longer needed.
 *         If \p vkey_witness is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_t* vkey_witness = ...; // Assume vkey_witness is initialized
 * cardano_ed25519_public_key_t* vkey = cardano_vkey_witness_get_vkey(vkey_witness);
 *
 * if (vkey != NULL)
 * {
 *   // Use the verification key
 *   // Once done, ensure to clean up and release the vkey
 *   cardano_ed25519_public_key_unref(&vkey);
 * }
 * else
 * {
 *   printf("Failed to retrieve the verification key.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_ed25519_public_key_t* cardano_vkey_witness_get_vkey(
  cardano_vkey_witness_t* vkey_witness);

/**
 * \brief Sets the verification key (VKey) for a Vkey Witness.
 *
 * This function assigns a new \ref cardano_ed25519_public_key_t object as the verification key (VKey)
 * for the given \ref cardano_vkey_witness_t object. The VKey is the public key that is used
 * to verify the cryptographic signature in the vkey witness.
 *
 * \param[in,out] vkey_witness A pointer to an initialized \ref cardano_vkey_witness_t object
 *                             to which the verification key will be assigned.
 * \param[in] vkey A pointer to an initialized \ref cardano_ed25519_public_key_t object representing the new
 *                 verification key. The caller retains ownership of this object and must ensure its lifecycle
 *                 is properly managed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the verification key
 *         was successfully set, or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         either \p vkey_witness or \p vkey is NULL.
 *
 * \note The function does not take ownership of the \p vkey, meaning the caller is still responsible for freeing the
 *       \ref cardano_ed25519_public_key_t object when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_t* vkey_witness = ...; // Assume vkey_witness is initialized
 * cardano_ed25519_public_key_t* vkey = ...; // Assume vkey is initialized
 *
 * cardano_error_t result = cardano_vkey_witness_set_vkey(vkey_witness, vkey);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("VKey set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the VKey.\n");
 * }
 *
 * // Cleanup resources for the VKey as it is still owned by the caller
 * cardano_ed25519_public_key_unref(&vkey);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_set_vkey(
  cardano_vkey_witness_t*       vkey_witness,
  cardano_ed25519_public_key_t* vkey);

/**
 * \brief Retrieves the cryptographic signature from a Vkey Witness.
 *
 * This function retrieves the \ref cardano_ed25519_signature_t object from a given \ref cardano_vkey_witness_t object.
 * The signature is produced by signing the hash of the transaction body using the corresponding private key (SKey),
 * and it can be verified using the associated public key (VKey).
 *
 * \param[in] vkey_witness A pointer to an initialized \ref cardano_vkey_witness_t object from which
 *                              the signature will be retrieved.
 *
 * \return A pointer to the \ref cardano_ed25519_signature_t object representing the signature. The returned signature is
 *         a new reference, and the caller is responsible for managing its lifecycle. Specifically, the caller must release
 *         it using \ref cardano_ed25519_signature_unref when it is no longer needed. If \p vkey_witness is NULL, this
 *         function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_t* vkey_witness = ...; // Assume vkey_witness is initialized
 * cardano_ed25519_signature_t* signature = cardano_vkey_witness_get_signature(vkey_witness);
 *
 * if (signature != NULL)
 * {
 *   // Process the signature
 *   cardano_ed25519_signature_unref(&signature); // Ensure to clean up the signature
 * }
 * else
 * {
 *   printf("Failed to retrieve the signature.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_ed25519_signature_t* cardano_vkey_witness_get_signature(
  cardano_vkey_witness_t* vkey_witness);

/**
 * \brief Sets the cryptographic signature for a Vkey Witness.
 *
 * This function assigns a new \ref cardano_ed25519_signature_t object to a given \ref cardano_vkey_witness_t object.
 * The signature is produced by signing the hash of the transaction body using the corresponding private key (SKey),
 * and it can be verified using the associated public key (VKey).
 *
 * \param[in,out] vkey_witness A pointer to an initialized \ref cardano_vkey_witness_t object to which
 *                                  the signature will be assigned.
 * \param[in] signature A pointer to an initialized \ref cardano_ed25519_signature_t object representing the signature.
 *                      This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the signature
 *         was successfully set. Returns \ref CARDANO_ERROR_POINTER_IS_NULL if either \p vkey_witness or \p signature is NULL.
 *
 * \note This function increases the reference count of the \p signature object; therefore, the caller retains ownership
 *       of their reference and must release their copy when it is no longer needed using \ref cardano_ed25519_signature_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_t* vkey_witness = ...; // Assume vkey_witness is initialized
 * cardano_ed25519_signature_t* signature = ...; // Assume signature is initialized
 *
 * cardano_error_t result = cardano_vkey_witness_set_signature(vkey_witness, signature);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Signature successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set the signature.\n");
 * }
 *
 * // Clean up
 * cardano_ed25519_signature_unref(&signature); // Caller still owns their reference to the signature
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vkey_witness_set_signature(
  cardano_vkey_witness_t*      vkey_witness,
  cardano_ed25519_signature_t* signature);

/**
 * \brief Decrements the reference count of a cardano_vkey_witness_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_vkey_witness_t object
 * by decreasing its reference count. When the reference count reaches zero, the vkey_witness is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] vkey_witness A pointer to the pointer of the vkey_witness object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vkey_witness_t* vkey_witness = cardano_vkey_witness_new(major, minor);
 *
 * // Perform operations with the vkey_witness...
 *
 * cardano_vkey_witness_unref(&vkey_witness);
 * // At this point, vkey_witness is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_vkey_witness_unref, the pointer to the \ref cardano_vkey_witness_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_vkey_witness_unref(cardano_vkey_witness_t** vkey_witness);

/**
 * \brief Increases the reference count of the cardano_vkey_witness_t object.
 *
 * This function is used to manually increment the reference count of an cardano_vkey_witness_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_vkey_witness_unref.
 *
 * \param vkey_witness A pointer to the cardano_vkey_witness_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming vkey_witness is a previously created vkey_witness object
 *
 * cardano_vkey_witness_ref(vkey_witness);
 *
 * // Now vkey_witness can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_vkey_witness_ref there is a corresponding
 * call to \ref cardano_vkey_witness_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_vkey_witness_ref(cardano_vkey_witness_t* vkey_witness);

/**
 * \brief Retrieves the current reference count of the cardano_vkey_witness_t object.
 *
 * This function returns the number of active references to an cardano_vkey_witness_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_vkey_witness_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param vkey_witness A pointer to the cardano_vkey_witness_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_vkey_witness_t object. If the object
 * is properly managed (i.e., every \ref cardano_vkey_witness_ref call is matched with a
 * \ref cardano_vkey_witness_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming vkey_witness is a previously created vkey_witness object
 *
 * size_t ref_count = cardano_vkey_witness_refcount(vkey_witness);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_vkey_witness_refcount(const cardano_vkey_witness_t* vkey_witness);

/**
 * \brief Sets the last error message for a given cardano_vkey_witness_t object.
 *
 * Records an error message in the vkey_witness's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] vkey_witness A pointer to the \ref cardano_vkey_witness_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the vkey_witness's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_vkey_witness_set_last_error(
  cardano_vkey_witness_t* vkey_witness,
  const char*             message);

/**
 * \brief Retrieves the last error message recorded for a specific vkey_witness.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_vkey_witness_set_last_error for the given
 * vkey_witness. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] vkey_witness A pointer to the \ref cardano_vkey_witness_t instance whose last error
 *                   message is to be retrieved. If the vkey_witness is NULL, the function
 *                   returns a generic error message indicating the null vkey_witness.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified vkey_witness. If the vkey_witness is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_vkey_witness_set_last_error for the same vkey_witness, or until
 *       the vkey_witness is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_vkey_witness_get_last_error(
  const cardano_vkey_witness_t* vkey_witness);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VKEY_WITNESS_H