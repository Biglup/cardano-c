/**
 * \file ed25519_signature.h
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ED25519_SIGNATURE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ED25519_SIGNATURE_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents an Ed25519 signature.
 */
typedef struct cardano_ed25519_signature_t cardano_ed25519_signature_t;

/**
 * \brief Creates a Ed25519 signature object from raw byte data.
 *
 * This function takes a buffer of signature bytes and constructs a new Ed25519 signature object.
 *
 * \param[in] data Pointer to the raw byte data of the signature.
 * \param[in] data_length The length of the data in bytes.
 * \param[out] signature A pointer to a pointer to `cardano_ed25519_signature_t` where the address of
 *             the newly created signature object will be stored. The caller is responsible
 *             for managing the lifecycle of the created signature object, including its
 *             proper deallocation through the corresponding unreference function to prevent memory leaks.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS and `signature` is set to point
 *         to the newly created signature object. On failure, a non-zero error code is
 *         returned and the value pointed to by `signature` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * byte_t data[] = { ... };
 * size_t data_length = sizeof(data);
 * cardano_ed25519_signature_t* signature = NULL;
 *
 * cardano_error_t error = cardano_ed25519_signature_from_bytes(data, data_length, &signature);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Use the signature object for further operations
 * }
 *
 * // Clean up
 * cardano_ed25519_signature_unref(signature);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_signature_from_bytes(
  const byte_t*                 data,
  size_t                        data_length,
  cardano_ed25519_signature_t** signature);

/**
 * \brief Creates a Ed25519 signature object from hexadecimal string data.
 *
 * This function takes a hexadecimal string representing signature data and constructs
 * a new Ed25519 signature object. The function is designed to parse the hexadecimal
 * string input and convert it into the corresponding binary representation before
 * creating the signature object.
 *
 * \param[in] hex Pointer to the hexadecimal string to be converted into a signature object.
 * \param[in] hex_length The length of the hexadecimal string in characters.
 * \param[out] signature A pointer to a pointer to `cardano_ed25519_signature_t` where the address of
 *             the newly created signature object will be stored. The caller is responsible
 *             for managing the lifecycle of the created signature object, including its
 *             proper deallocation through the corresponding unreference function to prevent memory leaks.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS, and `signature` is set to
 *         point to the newly created signature object. On failure, a non-zero error code is
 *         returned, and the value pointed to by `signature` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * const char* hex_data = "abc123..."; // Hexadecimal representation of the data
 * size_t hex_length = strlen(hex_data);
 * cardano_ed25519_signature_t* signature = NULL;
 *
 * cardano_error_t error = cardano_ed25519_signature_from_hex(hex_data, hex_length, &signature);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The signature object can now be used for further operations
 * }
 *
 * // Clean up
 * cardano_ed25519_signature_unref(signature);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_signature_from_hex(
  const char*                   hex,
  size_t                        hex_length,
  cardano_ed25519_signature_t** signature);

/**
 * \brief Decrements the reference count of a Ed25519 signature object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_ed25519_signature_t object
 * by decreasing its reference count. When the reference count reaches zero, the Ed25519 signature is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] signature A pointer to the pointer of the Ed25519 signature object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_error_t result = cardano_ed25519_signature_from_bytes(data, data_length, &signature);
 *
 * // Perform operations with the signature...
 *
 * cardano_ed25519_signature_unref(&signature);
 * // At this point, signature is NULL and cannot be used.
 * \endcode
 */
CARDANO_EXPORT void cardano_ed25519_signature_unref(cardano_ed25519_signature_t** signature);

/**
 * \brief Increases the reference count of the cardano_ed25519_signature_t object.
 *
 * This function is used to manually increment the reference count of a Ed25519 signature
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_ed25519_signature_unref.
 *
 * \param[in,out] signature A pointer to the Ed25519 signature object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming signature is a previously created Ed25519 signature object
 *
 * cardano_ed25519_signature_ref(signature);
 *
 * // Now signature can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_ed25519_signature_ref there is a corresponding
 * call to \ref cardano_ed25519_signature_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_ed25519_signature_ref(cardano_ed25519_signature_t* signature);

/**
 * \brief Retrieves the current reference count of the cardano_ed25519_signature_t object.
 *
 * This function returns the number of active references to a Ed25519 signature object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_ed25519_signature_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param[in,out] signature A pointer to the Ed25519 signature object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified Ed25519 signature object. If the object
 * is properly managed (i.e., every \ref cardano_ed25519_signature_ref call is matched with a
 * \ref cardano_ed25519_signature_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming signature is a previously created Ed25519 signature object
 *
 * size_t ref_count = cardano_ed25519_signature_refcount(signature);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ed25519_signature_refcount(const cardano_ed25519_signature_t* signature);

/**
 * \brief Retrieves a direct pointer to the internal data of a Ed25519 signature object.
 *
 * This function provides access to the internal storage of the Ed25519 signature object, allowing for read-only operations on its contents.
 * It is intended for situations where direct access to the data is necessary for performance or interoperability reasons.
 *
 * \warning The returned pointer provides raw, direct access to the Ed25519 signature object's internal data. It must not be used to modify
 * the buffer contents outside the API's control, nor should it be deallocated using free or similar memory management functions.
 * The lifecycle of the data pointed to by the returned pointer is managed by the Ed25519 signature object's reference counting mechanism; therefore,
 * the data remains valid as long as the Ed25519 signature object exists and has not been deallocated.
 *
 * \param[in] ed25519_signature The Ed25519 signature instance from which to retrieve the internal data pointer. The Ed25519 signature must have been previously
 * created and not yet deallocated.
 *
 * \return Returns a pointer to the internal data of the Ed25519 signature. If the Ed25519 signature instance is invalid,
 * returns NULL. The returned pointer provides direct access to the Ed25519 signature's contents and must be used in accordance with the
 * warning above to avoid unintended consequences.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_ed25519_signature_get_data(const cardano_ed25519_signature_t* ed25519_signature);

/**
 * \brief Retrieves the size of the Ed25519 signature object in bytes.
 *
 * \param[in] ed25519_signature A pointer to the `cardano_ed25519_signature_t` object whose size is to be retrieved.
 *
 * \return The size of the Ed25519 signature object in bytes. If the provided signature object
 *         is invalid or NULL, the function will return 0.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ed25519_signature_get_bytes_size(const cardano_ed25519_signature_t* ed25519_signature);

/**
 * \brief Converts a Ed25519 signature object into a raw byte array representation.
 *
 * This function exports the contents of a given Ed25519 signature object into a user-provided
 * byte array.
 *
 * \param[in] ed25519_signature A pointer to the `cardano_ed25519_signature_t` object to be exported.
 * \param[out] signature A pointer to a byte array where the signature bytes will be stored. This array
 *             must be pre-allocated by the caller and be large enough to hold the entire
 *             signature value.
 * \param[in] signature_length The length of the `signature` array in bytes. This must be equal to or
 *                             larger than the size of the signature object to avoid buffer overflow.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS and the `signature` array
 *         contains the byte representation of the signature object. On failure, a non-zero
 *         error code is returned. Possible failure reasons include invalid input parameters
 *         or a mismatch between `signature_length` and the actual size of the signature object.
 *
 * Example Usage:
 * \code
 * cardano_ed25519_signature_t* signature_obj = ...; // Assume signature_obj is previously created and valid
 * const size_t signature_size = cardano_ed25519_signature_get_bytes_size(signature_obj);
 * byte_t* signature_bytes = malloc(signature_size); // Allocate enough space for the signature
 *
 * if (signature_bytes != NULL)
 * {
 *   cardano_error_t error = cardano_ed25519_signature_to_bytes(signature_obj, signature_bytes, signature_size);
 *
 *   if (error == CARDANO_SUCCESS)
 *   {
 *     // The signature_bytes array now contains the signature object's bytes
 *   }
 *
 *   // Remember to free the allocated memory when done
 *   free(signature_bytes);
 * }
 * \endcode
 *
 * Note: It is the caller's responsibility to ensure that the `signature` array is correctly
 *       allocated and to free any allocated resources when no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_signature_to_bytes(
  const cardano_ed25519_signature_t* ed25519_signature,
  byte_t*                            signature,
  size_t                             signature_length);

/**
 * \brief Retrieves the size of the hexadecimal representation of a Ed25519 signature object.
 *
 * This function calculates the length of the string that would represent the BLAKE2b
 * signature object in hexadecimal form. This is useful for allocating the correct amount of
 * memory to store the hexadecimal representation of the signature.
 *
 * \param[in] ed25519_signature A pointer to the `cardano_ed25519_signature_t` object whose hexadecimal
 *                     size is to be retrieved.
 *
 * \return The size of the buffer needed to store the hexadecimal representation of the signature
 *         object, including the terminating null byte. If the provided signature object is invalid
 *         or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * cardano_ed25519_signature_t* signature_obj = ...; // Assume signature_obj is previously created
 * size_t hex_size = cardano_ed25519_signature_get_hex_size(signature_obj);
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * if (cardano_ed25519_signature_to_hex(signature_obj, hex_buffer, hex_size) == CARDANO_SUCCESS)
 * {
 *   // hex_buffer now contains the hexadecimal representation of the signature
 * }
 *
 * free(hex_buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ed25519_signature_get_hex_size(const cardano_ed25519_signature_t* ed25519_signature);

/**
 * \brief Converts a Ed25519 signature object into its hexadecimal string representation.
 *
 * This function exports the hexadecimal string representation of the provided
 * Ed25519 signature object into a user-supplied buffer.
 *
 * \param[in] ed25519_signature A pointer to the `cardano_ed25519_signature_t` object to be converted.
 * \param[out] hex_signature A pointer to the buffer where the hexadecimal string representation
 *                 of the signature will be stored. The buffer must be large enough to hold
 *                 the hexadecimal representation of the signature, including the terminating
 *                 null byte.
 * \param[in] signature_len The size of the buffer pointed to by `hex_signature`. This value should be
 *                 at least twice the size of the signature object in bytes plus one for the
 *                 terminating null byte, to accommodate the hexadecimal representation.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS, and the buffer pointed to
 *         by `hex_signature` is filled with the hexadecimal representation of the signature object.
 *         On failure, a non-zero error code is returned, which could indicate an invalid
 *         signature object, insufficient buffer size, or other errors.
 *
 * Example Usage:
 * \code
 * cardano_ed25519_signature_t* signature_obj = ...; // Assume signature_obj is previously created
 * size_t hex_size = cardano_ed25519_signature_get_hex_size(signature_obj);
 * byte_t* hex_buffer = (byte_t*)malloc(hex_size);
 *
 * if (cardano_ed25519_signature_to_hex(signature_obj, hex_buffer, hex_size) == CARDANO_SUCCESS)
 * {
 *  // hex_buffer now contains the hexadecimal representation of the signature
 * }
 *
 * free(hex_buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_signature_to_hex(
  const cardano_ed25519_signature_t* ed25519_signature,
  char*                              hex_signature,
  size_t                             signature_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ED25519_SIGNATURE_H
