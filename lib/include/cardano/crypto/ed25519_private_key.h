/**
 * \file ed25519_private_key.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ED25519_PRIVATE_KEY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ED25519_PRIVATE_KEY_H

/* INCLUDES ******************************************************************/

#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents an Ed25519 private key within the Cardano ecosystem.
 *
 * The `cardano_ed25519_private_key_t` structure provides an abstraction over the raw
 * private key data, facilitating secure handling, storage, and usage in various cryptographic
 * functions. It enables the signing of messages, which can then be verified by others using
 * the corresponding public key, without exposing the private key itself.
 */
typedef struct cardano_ed25519_private_key_t cardano_ed25519_private_key_t;

/**
 * \brief Creates an Ed25519 private key object from a raw byte array.
 *
 * This function constructs an Ed25519 private key from a provided array of bytes, assuming
 * the bytes represent a "normal" Ed25519 private key. "Normal" in this context refers to a
 * key derived directly from a seed, requiring 32 bytes of data.
 *
 * \param[in] key_bytes A pointer to the byte array containing the binary representation of
 *            the Ed25519 private key.
 * \param[in] key_length The length of the byte array pointed to by `key_bytes`. For a
 *            "normal" Ed25519 private key, this should be 32 bytes.
 * \param[out] private_key A pointer to a pointer that will be updated to point to the newly
 *            created `cardano_ed25519_private_key_t` object. The caller is responsible for
 *            managing the lifecycle of this object.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key was successfully created
 *         from the byte array. On failure, a non-zero error code is returned, indicating an issue
 *         with the input parameters or an internal error in the key creation process. If the function
 *         fails, the value pointed to by `private_key` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * const byte_t raw_private_key[] = { ... }; // Byte array with the binary representation of the private key seed
 * size_t key_length = sizeof(raw_private_key);
 * cardano_ed25519_private_key_t* ed25519_key = NULL;
 *
 * cardano_error_t error = cardano_ed25519_private_key_from_normal_bytes(raw_private_key, key_length, &ed25519_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `ed25519_key` is now ready for cryptographic operations, such as signing messages.
 * }
 *
 * // Clean up
 * cardano_ed25519_private_key_unref(&ed25519_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_from_normal_bytes(
  const byte_t*                   key_bytes,
  size_t                          key_length,
  cardano_ed25519_private_key_t** private_key);

/**
 * \brief Creates an extended Ed25519 private key object from a raw byte array of an extended key.
 *
 * This function constructs an extended Ed25519 private key from a provided array of bytes,
 * allowing the construction of a private key that includes both the private scalar and additional
 * information used for signing.
 *
 * \param[in] hex A pointer to the byte array containing the binary representation of the extended
 *            Ed25519 private key. The array should include both the 32-byte private scalar
 *            and the 32-byte chain code or initialization vector, totaling 64 bytes.
 * \param[in] key_length The length of the byte array pointed to by `hex`. For an extended Ed25519
 *            private key, this should be 64 bytes to include both components of the extended key.
 * \param[out] private_key A pointer to a pointer that will be updated to point to the newly created
 *            `cardano_ed25519_private_key_t` object. The caller is responsible for managing
 *            the lifecycle of this object, including its deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key was successfully created
 *         from the byte array. On failure, a non-zero error code is returned, indicating an issue
 *         with the input parameters or an internal error in the key creation process. If the function
 *         fails, the value pointed to by `private_key` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * const byte_t raw_extended_key[] = { ... }; // Byte array with the binary representation of the extended private key
 * size_t key_length = sizeof(raw_extended_key);
 * cardano_ed25519_private_key_t* ed25519_key = NULL;
 *
 * cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(raw_extended_key, key_length, &ed25519_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `ed25519_key` is now ready for cryptographic operations, including advanced signing mechanisms.
 * }
 *
 * // Clean up
 * cardano_ed25519_private_key_unref(&ed25519_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_from_extended_bytes(
  const byte_t*                   hex,
  size_t                          key_length,
  cardano_ed25519_private_key_t** private_key);

/**
 * \brief Generates an Ed25519 private key from a hexadecimal string.
 *
 * This function converts a hexadecimal string into a binary format and then creates an Ed25519
 * private key from the resultant bytes.
 *
 * \param[in] hex A pointer to the null-terminated hexadecimal string representing the seed bytes
 *            for the private key. The string should only contain valid hexadecimal characters.
 * \param[in] hex_length The length of the hexadecimal string, not including the null terminator.
 *            This length should be exactly twice the size of the expected binary key (64 characters
 *            for a 32-byte key).
 * \param[out] key A pointer to a pointer that will be updated to point to the newly created
 *            `cardano_ed25519_private_key_t` object. The caller is responsible for managing
 *            the lifecycle of this object.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key was successfully created
 *         from the hexadecimal string. On failure, returns a non-zero error code indicating
 *         issues such as an invalid hexadecimal string format, incorrect length, or internal
 *         errors in processing the hexadecimal data. If the function fails, the value pointed
 *         to by `key` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * const char* hex_seed = "4fe5b2...";
 * size_t hex_length = strlen(hex_seed);
 * cardano_ed25519_private_key_t* private_key = NULL;
 *
 * cardano_error_t error = cardano_ed25519_private_key_from_normal_hex(hex_seed, hex_length, &private_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `private_key` is now ready for cryptographic operations, such as signing.
 * }
 * else
 * {
 *   // Handle error: invalid input, memory allocation failure, etc.
 * }
 *
 * // Clean up
 * cardano_ed25519_private_key_unref(&private_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_from_normal_hex(
  const char*                     hex,
  size_t                          hex_length,
  cardano_ed25519_private_key_t** key);

/**
 * \brief Generates an extended Ed25519 private key from a hexadecimal string.
 *
 * This function creates an extended Ed25519 private key from a provided hexadecimal string,
 * which should represent the full 64 bytes of the extended private key.
 *
 * \param[in] hex A pointer to the null-terminated hexadecimal string representing the bytes
 *            of the extended Ed25519 private key. This string should include both parts
 *            of the extended key, resulting in a hexadecimal string of 128 characters.
 * \param[in] hex_length The length of the hexadecimal string, not including the null terminator.
 *            For an extended Ed25519 private key, this length should be exactly 128 characters
 *            to correctly represent the 64 bytes of key data.
 * \param[out] key A pointer to a pointer that will be updated to point to the newly created
 *            `cardano_ed25519_private_key_t` object. The caller is responsible for the lifecycle
 *            management of this object, including its secure deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key is successfully created
 *         from the hexadecimal string. On failure, returns a non-zero error code indicating
 *         an issue with the input parameters, such as an incorrect length or invalid hexadecimal data.
 *         If the function fails, the value pointed to by `key` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * const char* hex = "4a2e...fe3b"; // 128-character hexadecimal string representing the extended private key
 * cardano_ed25519_private_key_t* private_key = NULL;
 *
 * cardano_error_t error = cardano_ed25519_private_key_from_extended_hex(hex, strlen(hex), &private_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `private_key` is now initialized and can be used for cryptographic operations.
 * }
 *
 * // Clean up
 * cardano_ed25519_private_key_unref(&private_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_from_extended_hex(
  const char*                     hex,
  size_t                          hex_length,
  cardano_ed25519_private_key_t** key);

/**
 * \brief Decrements the reference count of a Ed25519 private key object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_ed25519_private_key_t object
 * by decreasing its reference count. When the reference count reaches zero, the Ed25519 private key is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] private_key A pointer to the pointer of the Ed25519 private key object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ed25519_private_key_t* key = cardano_ed25519_private_key_new();
 *
 * // Perform operations with the key...
 *
 * cardano_ed25519_private_key_unref(&key);
 * // At this point, key is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_ed25519_private_key_unref, the pointer to the \ref cardano_ed25519_private_key_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_ed25519_private_key_unref(cardano_ed25519_private_key_t** private_key);

/**
 * \brief Increases the reference count of the cardano_ed25519_private_key_t object.
 *
 * This function is used to manually increment the reference count of a Ed25519 private key
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_ed25519_private_key_unref.
 *
 * \param[in,out] private_key A pointer to the Ed25519 private key object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming key is a previously created Ed25519 private key object
 *
 * cardano_ed25519_private_key_ref(key);
 *
 * // Now key can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_ed25519_private_key_ref there is a corresponding
 * call to \ref cardano_ed25519_private_key_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_ed25519_private_key_ref(cardano_ed25519_private_key_t* private_key);

/**
 * \brief Retrieves the current reference count of the cardano_ed25519_private_key_t object.
 *
 * This function returns the number of active references to a Ed25519 private key object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_ed25519_private_key_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param[in] private_key A pointer to the Ed25519 private key object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified Ed25519 private key object. If the object
 * is properly managed (i.e., every \ref cardano_ed25519_private_key_ref call is matched with a
 * \ref cardano_ed25519_private_key_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming key is a previously created Ed25519 private key object
 *
 * size_t ref_count = cardano_ed25519_private_key_refcount(key);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ed25519_private_key_refcount(const cardano_ed25519_private_key_t* private_key);

/**
 * \brief Signs a message using an Ed25519 private key.
 *
 * This function creates a digital signature for a specified message using the provided
 * Ed25519 private key. Digital signatures are used to verify the authenticity and integrity
 * of messages, ensuring that the message has not been altered in transit and was signed by
 * the holder of the corresponding private key.
 *
 * \param[in] private_key A pointer to the `cardano_ed25519_private_key_t` object representing
 *                    the Ed25519 private key used for signing the message.
 * \param[in] message A pointer to the byte array containing the message data to be signed.
 * \param[in] message_length The length of the message in bytes.
 * \param[out] signature A pointer to a pointer that will be updated to point to the newly created
 *                  `cardano_ed25519_signature_t` object containing the signature of the message.
 *                  The caller is responsible for managing the lifecycle of this object, including
 *                  its deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the signature was successfully created.
 *         On failure, a non-zero error code is returned, indicating an issue with the input
 *         parameters or an internal error during the signing process. If the function fails,
 *         the value pointed to by `signature` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * cardano_ed25519_private_key_t* private_key = ...; // Assume private_key is initialized
 * byte_t message[] = "Message to sign";
 * size_t message_length = sizeof(message) - 1; // Exclude null terminator for raw data signing
 * cardano_ed25519_signature_t* signature = NULL;
 *
 * cardano_error_t error = cardano_ed25519_private_key_sign(private_key, message, message_length, &signature);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `signature` can now be used for verification or other cryptographic operations
 * }
 *
 * // Clean up
 * cardano_ed25519_signature_unref(&signature);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_sign(const cardano_ed25519_private_key_t* private_key, const byte_t* message, size_t message_length, cardano_ed25519_signature_t** signature);

/**
 * \brief Extracts the public key from a Ed25519 private key.
 *
 * This function derives the corresponding public key from a given Ed25519 private key.
 * The derived public key is essential for various operations within the Cardano ecosystem,
 * such as generating wallet addresses or verifying signatures, where the private key itself
 * must remain secret. The public key is derived without compromising the private key.
 *
 * \param[in] private_key A pointer to the `cardano_ed25519_private_key_t` object from which
 *                    the public key is to be derived. This private key must have been
 *                    properly initialized and represent a valid Ed25519 private key.
 * \param[out] ed25519_public_key A pointer to a pointer to `cardano_ed25519_public_key_t` that will
 *                           be updated to point to the newly created public key object. It is
 *                           the caller's responsibility to manage the lifecycle of this object,
 *                           including its deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the public key was successfully derived
 *         from the private key. On failure, a non-zero error code is returned, indicating
 *         an issue with the input private key or an internal error in the public key derivation
 *         process. If the function fails, the value pointed to by `ed25519_public_key` is set
 *         to `NULL`.
 *
 * Example Usage:
 * \code
 * cardano_ed25519_private_key_t* ed25519_private_key = ...; // Previously initialized Ed25519 private key
 * cardano_ed25519_public_key_t* ed25519_public_key = NULL;
 *
 * cardano_error_t error = cardano_ed25519_private_key_get_public_key(ed25519_private_key, &ed25519_public_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `ed25519_public_key` can now be used for address generation, transaction verification, etc.
 * }
 *
 * // Clean up
 * cardano_ed25519_public_key_unref(&ed25519_public_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_get_public_key(const cardano_ed25519_private_key_t* private_key, cardano_ed25519_public_key_t** ed25519_public_key);

/**
 * \brief Retrieves the size in bytes of a Ed25519 private key.
 *
 * This function calculates and returns the size of the given Ed25519 private key object
 * when represented as a byte array.
 *
 * \param[in] private_key A pointer to the `cardano_ed25519_private_key_t` object whose byte size
 *                   is to be determined.
 *
 * \return The size of the Ed25519 private key in bytes. If the provided `private_key` is invalid
 *         or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * cardano_ed25519_private_key_t* ed25519_private_key = ...; // Previously created or loaded Ed25519 private key
 * size_t key_size = cardano_ed25519_private_key_get_bytes_size(ed25519_private_key);
 *
 * // Use key_size to allocate memory for the key's byte representation
 * byte_t* key_bytes = (byte_t*)malloc(key_size);
 * // Ensure to check for malloc failure and handle `key_bytes` appropriately
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ed25519_private_key_get_bytes_size(const cardano_ed25519_private_key_t* private_key);

/**
 * \brief Retrieves a direct pointer to the internal data of a Ed25519 private key object.
 *
 * This function provides access to the internal storage of the Ed25519 private key object, allowing for read-only operations on its contents.
 * It is intended for situations where direct access to the data is necessary for performance or interoperability reasons.
 *
 * \warning The returned pointer provides raw, direct access to the Ed25519 private key object's internal data. It must not be used to modify
 * the buffer contents outside the API's control, nor should it be deallocated using free or similar memory management functions.
 * The lifecycle of the data pointed to by the returned pointer is managed by the Ed25519 private key object's reference counting mechanism; therefore,
 * the data remains valid as long as the Ed25519 private key object exists and has not been deallocated.
 *
 * \param[in] ed25519_private_key The Ed25519 private key instance from which to retrieve the internal data pointer. The Ed25519 private key must have been previously
 * created and not yet deallocated.
 *
 * \return Returns a pointer to the internal data of the Ed25519 private key. If the Ed25519 private key instance is invalid,
 * returns NULL. The returned pointer provides direct access to the Ed25519 private key's contents and must be used in accordance with the
 * warning above to avoid unintended consequences.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_ed25519_private_key_get_data(const cardano_ed25519_private_key_t* ed25519_private_key);

/**
 * \brief Serializes a Ed25519 private key into a provided byte array.
 *
 * Converts the given Ed25519 private key object into its byte representation and stores it
 * in the provided buffer.
 *
 * \param[in] private_key A pointer to the `cardano_ed25519_private_key_t` object representing the
 *                        Ed25519 private key to be serialized.
 * \param[out] out_key_bytes A pointer to the buffer where the byte representation of the private key will
 *                           be stored. The buffer must be pre-allocated by the caller and have a size
 *                           sufficient to hold the entire serialized private key.
 * \param[in] out_key_length The size of the buffer pointed to by `key`, indicating the maximum
 *                           number of bytes that can be written into the buffer. To ensure
 *                           successful serialization, this size should be at least as large as
 *                           the size returned by `cardano_ed25519_private_key_get_bytes_size` for
 *                           the given `private_key`.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key was successfully
 *         serialized into the byte array. If the function fails, it returns a non-zero
 *         error code. Possible failure reasons include invalid input parameters (such as
 *         a NULL `private_key`), a `key` buffer that is too small (`key_length` insufficient),
 *         or other internal errors encountered during the serialization process.
 *
 * Example Usage:
 * \code
 * // Assuming 'ed25519_private_key' is a previously initialized Ed25519 private key
 * const size_t required_size = cardano_ed25519_private_key_get_bytes_size(ed25519_private_key);
 * byte_t* buffer = malloc(required_size);
 *
 * if (buffer != NULL)
 * {
 *   cardano_error_t result = cardano_ed25519_private_key_to_bytes(ed25519_private_key, buffer, required_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // The buffer now contains the serialized private key
 *   }
 *
 *   free(buffer);
 * }
 * else
 * {
 *   // Handle memory allocation failure
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_to_bytes(
  const cardano_ed25519_private_key_t* private_key,
  byte_t*                              out_key_bytes,
  size_t                               out_key_length);

/**
 * \brief Determines the required buffer size for the hexadecimal representation of a Ed25519 private key.
 *
 * This function calculates the length of the string that would be required to store the
 * hexadecimal representation of a given Ed25519 private key, including the null-terminator.
 *
 * \param[in] private_key A pointer to the `cardano_ed25519_private_key_t` object whose hexadecimal
 *                   size is being queried.
 *
 * \return The size of the buffer (in bytes) needed to store the hexadecimal string representation
 *         of the Ed25519 private key, including space for the null-terminator. If the provided
 *         `private_key` is invalid or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * // Assuming 'ed25519_private_key' is a previously initialized Ed25519 private key
 * size_t hex_size = cardano_ed25519_private_key_get_hex_size(ed25519_private_key);
 *
 *  char* hex_buffer = (char*)malloc(hex_size);
 *
 *  if (hex_buffer != NULL)
 *  {
 *    // Buffer is ready for use with cardano_ed25519_private_key_to_hex
 *  }
 *
 *  // Remember to free the allocated buffer
 *  free(hex_buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ed25519_private_key_get_hex_size(const cardano_ed25519_private_key_t* private_key);

/**
 * \brief Serializes a Ed25519 private key into its hexadecimal string representation.
 *
 * Converts the given Ed25519 private key object into a hexadecimal string and stores it
 * in the provided buffer. The caller is responsible for ensuring that the buffer is sufficiently
 * large to hold the entire  hexadecimal string, including the null terminator. The required size
 * can be determined by calling \ref cardano_ed25519_private_key_get_hex_size.
 *
 * \param[in] private_key A pointer to the `cardano_ed25519_private_key_t` object representing the
 *                   Ed25519 private key to be converted to hexadecimal format.
 * \param[out] hex_key A pointer to the buffer where the hexadecimal string representation of the
 *                private key will be stored. The buffer must be pre-allocated and should be
 *                large enough to hold the hexadecimal string plus a null terminator.
 * \param[in] key_length The length of the buffer pointed to by `hex_key`, indicating the maximum
 *                   number of characters (bytes) that can be written into the buffer,
 *                   including space for the null terminator.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key was successfully
 *         serialized into the hexadecimal format and stored in the provided buffer.
 *         If the function fails, it returns a non-zero error code. Failure can occur
 *         for several reasons, including but not limited to an invalid `private_key`,
 *         an insufficiently sized buffer (`key_length` too small), or other internal errors
 *         encountered during the serialization process.
 *
 * Example Usage:
 * \code
 * cardano_ed25519_private_key_t* ed25519_private_key = ...; // Previously initialized Ed25519 private key
 * size_t hex_size = cardano_ed25519_private_key_get_hex_size(ed25519_private_key);
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * if (cardano_blake2b_hash_to_hex(ed25519_private_key, hex_buffer, hex_size) == CARDANO_SUCCESS)
 * {
 *  // hex_buffer now contains the hexadecimal representation of the key
 * }
 *
 * free(hex_buffer); // Always ensure to free allocated memory
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ed25519_private_key_to_hex(
  const cardano_ed25519_private_key_t* private_key,
  char*                                hex_key,
  size_t                               key_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ED25519_PRIVATE_KEY_H
