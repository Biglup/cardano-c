/**
 * \file bip32_public_key.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_PUBLIC_KEY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_PUBLIC_KEY_H

/* INCLUDES ******************************************************************/

#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a BIP32 public key.
 *
 * This structure encapsulates a public key according to the BIP32 standard, which
 * is utilized in hierarchical deterministic (HD) wallets.
 */
typedef struct cardano_bip32_public_key_t cardano_bip32_public_key_t;

/**
 * \brief Creates a BIP32 public key object from a raw byte array.
 *
 * This function takes an array of bytes representing a BIP32 public key and constructs
 * a new `cardano_bip32_public_key_t` object.
 *
 * \param[in] key_bytes A pointer to the byte array containing the binary representation of the BIP32 public key.
 * \param[in] key_length The length of the byte array pointed to by `key`.
 * \param[out] key A pointer to a pointer that will be set to point to the newly created `cardano_bip32_public_key_t` object.
 *            The caller is responsible for freeing this object using the appropriate API function to prevent memory leaks.
 *
 * \return A `cardano_error_t` indicating the result of the operation. On success, the function
 *         returns `CARDANO_SUCCESS`, and the `key` parameter is updated to point to the newly created
 *         public key object. On failure, a non-zero error code is returned, and the value pointed to by
 *         `key` is set to `NULL`, indicating that the public key object was not successfully created.
 *
 * Example Usage:
 * \code
 * const byte_t raw_key[] = { ... }; // Byte array containing the BIP32 public key
 * size_t raw_key_length = sizeof(raw_key);
 * cardano_bip32_public_key_t* bip32_key = NULL;
 *
 * cardano_error_t result = cardano_bip32_public_key_from_bytes(raw_key, raw_key_length, &bip32_key);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The `bip32_key` can now be used for operations like address generation
 * }
 *
 * // Remember to free the BIP32 public key object when done
 * cardano_bip32_public_key_unref(&bip32_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_public_key_from_bytes(
  const byte_t*                key_bytes,
  size_t                       key_length,
  cardano_bip32_public_key_t** key);

/**
 * \brief Creates a BIP32 public key object from a hexadecimal string.
 *
 * This function parses a hexadecimal string representing a BIP32 public key and constructs
 * a `cardano_bip32_public_key_t` object from it.
 *
 * \param[in] hex A pointer to the null-terminated string containing the hexadecimal representation
 *            of the BIP32 public key.
 * \param[in] hex_length The length of the hexadecimal string, excluding the null terminator.
 * \param[out] key A pointer to a pointer to `cardano_bip32_public_key_t` where the address of
 *            the newly created public key object will be stored. It is the caller's responsibility
 *            to manage the lifecycle of the created object, including its deallocation through
 *            the appropriate API function to prevent memory leaks.
 *
 * \return A `cardano_error_t` indicating the result of the operation. On success, the function
 *         returns `CARDANO_SUCCESS`, and the `key` parameter is updated to point to the newly created
 *         public key object. On failure, a non-zero error code is returned, and the value pointed to by
 *         `key` is set to `NULL`, indicating that the public key object was not successfully created.
 *
 * Example Usage:
 * \code
 * const char* hex_key = "a1b2c3d4..."; // Hexadecimal string of the public key
 * size_t hex_key_len = strlen(hex_key);
 * cardano_bip32_public_key_t* public_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_public_key_from_hex(hex_key, hex_key_len, &public_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `public_key` can now be used for cryptographic operations
 * }
 *
 * // Clean up
 * cardano_bip32_public_key_unref(&public_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_public_key_from_hex(
  const char*                  hex,
  size_t                       hex_length,
  cardano_bip32_public_key_t** key);

/**
 * \brief Decrements the reference count of a BIP32 public key object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_bip32_public_key_t object
 * by decreasing its reference count. When the reference count reaches zero, the BIP32 public key is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] public_key A pointer to the pointer of the BIP32 public key object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bip32_public_key_t* key = cardano_bip32_public_key_from_bytes(...);
 *
 * // Perform operations with the key...
 *
 * cardano_bip32_public_key_unref(&key);
 * // At this point, key is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_bip32_public_key_unref, the pointer to the \ref cardano_bip32_public_key_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_bip32_public_key_unref(cardano_bip32_public_key_t** public_key);

/**
 * \brief Increases the reference count of the cardano_bip32_public_key_t object.
 *
 * This function is used to manually increment the reference count of a BIP32 public key
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_bip32_public_key_unref.
 *
 * \param public_key A pointer to the BIP32 public key object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming key is a previously created BIP32 public key object
 *
 * cardano_bip32_public_key_ref(key);
 *
 * // Now key can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_bip32_public_key_ref there is a corresponding
 * call to \ref cardano_bip32_public_key_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_bip32_public_key_ref(cardano_bip32_public_key_t* public_key);

/**
 * \brief Retrieves the current reference count of the cardano_bip32_public_key_t object.
 *
 * This function returns the number of active references to a BIP32 public key object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_bip32_public_key_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param public_key A pointer to the BIP32 public key object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified BIP32 public key object. If the object
 * is properly managed (i.e., every \ref cardano_bip32_public_key_ref call is matched with a
 * \ref cardano_bip32_public_key_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming key is a previously created BIP32 public key object
 *
 * size_t ref_count = cardano_bip32_public_key_refcount(key);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bip32_public_key_refcount(const cardano_bip32_public_key_t* public_key);

/**
 * \brief Derives a BIP32 public key based on a sequence of indices.
 *
 * This function takes an existing BIP32 public key and derives a new public key
 * by following a path specified by an array of indices. This path derivation
 * adheres to the BIP32 specification for hierarchical deterministic wallets.
 *
 * \param[in] bip32_public_key A pointer to the initial `cardano_bip32_public_key_t` from which
 *                             the derivation process starts. This key acts as the root of the
 *                             derivation path.
 * \param[in] indices An array of uint32_t values representing the derivation path. Each
 *                    value in the array specifies an index in the hierarchy where the
 *                    child key should be derived from the current key.
 * \param[in] indices_count The number of indices in the `indices` array, representing
 *                          the depth of the derivation path.
 * \param[out] derived_bip32_public_key A pointer to a pointer to `cardano_bip32_public_key_t` that will
 *                                      be updated to point to the newly derived public key object.
 *                                      The caller is responsible for freeing this object using the
 *                                      appropriate API function to prevent memory leaks.
 *
 * \return cardano_error_t Returns an error code indicating the result of the operation.
 *         On success, the function returns `CARDANO_SUCCESS`, and `public_key` is set to
 *         point to the newly derived public key object. On failure, a non-zero error code
 *         is returned, and the value pointed to by `public_key` is set to `NULL`, indicating
 *         that the derivation failed and no new key was generated.
 *
 * Example Usage:
 * \code
 * uint32_t indices[] = { 44, 1815, 0, 0, 0 }; // Example BIP44 derivation path
 * size_t indices_count = sizeof(indices) / sizeof(uint32_t);
 * cardano_bip32_public_key_t* initial_public_key = ...; // Obtain or generate the initial BIP32 public key
 * cardano_bip32_public_key_t* derived_public_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_public_key_derive(initial_public_key, indices, indices_count, &derived_public_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `derived_public_key` can now be used for further operations, such as
 *   // generating addresses or verifying transactions.
 * }
 *
 * // Clean up
 * cardano_bip32_public_key_unref(&derived_public_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_public_key_derive(
  const cardano_bip32_public_key_t* bip32_public_key,
  const uint32_t*                   indices,
  size_t                            indices_count,
  cardano_bip32_public_key_t**      derived_bip32_public_key);

/**
 * \brief Converts a BIP32 public key to an Ed25519 public key.
 *
 * This function allows for the conversion of a public key from the BIP32 hierarchical
 * deterministic wallet format to an Ed25519 public key format.
 *
 * \param[in] public_key A pointer to the `cardano_bip32_public_key_t` object representing the
 *                   BIP32 public key to be converted.
 * \param[out] ed25519_public_key A pointer to a pointer to `cardano_ed25519_public_key_t` that
 *                           will be updated to point to the newly created Ed25519 public key
 *                           object. The caller is responsible for managing the lifecycle of
 *                           this object, including its deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns an error code indicating the outcome of the operation.
 *         On success, the function returns `CARDANO_SUCCESS`, and `ed25519_public_key` is
 *         updated to point to the new Ed25519 public key object. On failure, a non-zero
 *         error code is returned, and the value pointed to by `ed25519_public_key` is set
 *         to `NULL`, indicating that the conversion was unsuccessful.
 *
 * Example Usage:
 * \code
 * cardano_bip32_public_key_t* bip32_public_key = ...; // Assume this is initialized
 * cardano_ed25519_public_key_t* ed25519_public_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_public_key_to_ed25519_key(bip32_public_key, &ed25519_public_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // `ed25519_public_key` can now be used for cryptographic operations requiring Ed25519 keys
 * }
 *
 * // Remember to free the Ed25519 public key object when done
 * cardano_ed25519_public_key_unref(&ed25519_public_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_public_key_to_ed25519_key(
  const cardano_bip32_public_key_t* public_key,
  cardano_ed25519_public_key_t**    ed25519_public_key);

/**
 * \brief Retrieves the size in bytes of a BIP32 public key.
 *
 * This function calculates and returns the size of the given BIP32 public key object
 * when represented as a byte array.
 *
 * \param[in] public_key A pointer to the `cardano_bip32_public_key_t` object whose byte size
 *                   is to be determined.
 *
 * \return The size of the BIP32 public key in bytes. If the provided `public_key` is invalid
 *         or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * cardano_bip32_public_key_t* bip32_public_key = ...; // Previously created or loaded BIP32 public key
 * size_t key_size = cardano_bip32_public_key_get_bytes_size(bip32_public_key);
 *
 * // Use key_size to allocate memory for the key's byte representation
 * byte_t* key_bytes = (byte_t*)malloc(key_size);
 * // Ensure to check for malloc failure and handle `key_bytes` appropriately
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bip32_public_key_get_bytes_size(const cardano_bip32_public_key_t* public_key);

/**
 * \brief Retrieves a direct pointer to the internal data of a BIP32 public key object.
 *
 * This function provides access to the internal storage of the BIP32 public key object, allowing for read-only operations on its contents.
 * It is intended for situations where direct access to the data is necessary for performance or interoperability reasons.
 *
 * \warning The returned pointer provides raw, direct access to the BIP32 public key object's internal data. It must not be used to modify
 * the buffer contents outside the API's control, nor should it be deallocated using free or similar memory management functions.
 * The lifecycle of the data pointed to by the returned pointer is managed by the BIP32 public key object's reference counting mechanism; therefore,
 * the data remains valid as long as the BIP32 public key object exists and has not been deallocated.
 *
 * \param[in] bip32_public_key The BIP32 public key instance from which to retrieve the internal data pointer. The BIP32 public key must have been previously
 * created and not yet deallocated.
 *
 * \return Returns a pointer to the internal data of the BIP32 public key. If the BIP32 public key instance is invalid,
 * returns NULL. The returned pointer provides direct access to the BIP32 public key's contents and must be used in accordance with the
 * warning above to avoid unintended consequences.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_bip32_public_key_get_data(const cardano_bip32_public_key_t* bip32_public_key);

/**
 * \brief Serializes a BIP32 public key into a provided byte array.
 *
 * Converts the given BIP32 public key object into its byte representation and stores it
 * in the provided buffer.
 *
 * \param[in] public_key A pointer to the `cardano_bip32_public_key_t` object representing the
 *                       BIP32 public key to be serialized.
 * \param[out] out_key_bytes A pointer to the buffer where the byte representation of the public key will
 *                           be stored. The buffer must be pre-allocated by the caller and have a size
 *                           sufficient to hold the entire serialized public key.
 * \param[in] out_key_length The size of the buffer pointed to by `key`, indicating the maximum
 *                           number of bytes that can be written into the buffer. To ensure
 *                           successful serialization, this size should be at least as large as
 *                           the size returned by `cardano_bip32_public_key_get_bytes_size` for
 *                           the given `public_key`.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the public key was successfully
 *         serialized into the byte array. If the function fails, it returns a non-zero
 *         error code. Possible failure reasons include invalid input parameters (such as
 *         a NULL `public_key`), a `key` buffer that is too small (`key_length` insufficient),
 *         or other internal errors encountered during the serialization process.
 *
 * Example Usage:
 * \code
 * // Assuming 'bip32_public_key' is a previously initialized BIP32 public key
 * const size_t required_size = cardano_bip32_public_key_get_bytes_size(bip32_public_key);
 * byte_t* buffer = malloc(required_size);
 *
 * if (buffer != NULL)
 * {
 *   cardano_error_t result = cardano_bip32_public_key_to_bytes(bip32_public_key, buffer, required_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // The buffer now contains the serialized public key
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
CARDANO_EXPORT cardano_error_t cardano_bip32_public_key_to_bytes(
  const cardano_bip32_public_key_t* public_key,
  byte_t*                           out_key_bytes,
  size_t                            out_key_length);

/**
 * \brief Determines the required buffer size for the hexadecimal representation of a BIP32 public key.
 *
 * This function calculates the length of the string that would be required to store the
 * hexadecimal representation of a given BIP32 public key, including the null-terminator.
 *
 * \param[in] public_key A pointer to the `cardano_bip32_public_key_t` object whose hexadecimal
 *                   size is being queried.
 *
 * \return The size of the buffer (in bytes) needed to store the hexadecimal string representation
 *         of the BIP32 public key, including space for the null-terminator. If the provided
 *         `public_key` is invalid or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * // Assuming 'bip32_public_key' is a previously initialized BIP32 public key
 * size_t hex_size = cardano_bip32_public_key_get_hex_size(bip32_public_key);
 *
 *  char* hex_buffer = (char*)malloc(hex_size);
 *
 *  if (hex_buffer != NULL)
 *  {
 *    // Buffer is ready for use with cardano_bip32_public_key_to_hex
 *  }
 *
 *  // Remember to free the allocated buffer
 *  free(hex_buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bip32_public_key_get_hex_size(const cardano_bip32_public_key_t* public_key);

/**
 * \brief Serializes a BIP32 public key into its hexadecimal string representation.
 *
 * Converts the given BIP32 public key object into a hexadecimal string and stores it
 * in the provided buffer. The caller is responsible for ensuring that the buffer is sufficiently
 * large to hold the entire  hexadecimal string, including the null terminator. The required size
 * can be determined by calling \ref cardano_bip32_public_key_get_hex_size.
 *
 * \param[in] public_key A pointer to the `cardano_bip32_public_key_t` object representing the
 *                   BIP32 public key to be converted to hexadecimal format.
 * \param[out] hex_key A pointer to the buffer where the hexadecimal string representation of the
 *                public key will be stored. The buffer must be pre-allocated and should be
 *                large enough to hold the hexadecimal string plus a null terminator.
 * \param[in] key_length The length of the buffer pointed to by `hex_key`, indicating the maximum
 *                   number of characters (bytes) that can be written into the buffer,
 *                   including space for the null terminator.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the public key was successfully
 *         serialized into the hexadecimal format and stored in the provided buffer.
 *         If the function fails, it returns a non-zero error code. Failure can occur
 *         for several reasons, including but not limited to an invalid `public_key`,
 *         an insufficiently sized buffer (`key_length` too small), or other internal errors
 *         encountered during the serialization process.
 *
 * Example Usage:
 * \code
 * cardano_bip32_public_key_t* bip32_public_key = ...; // Previously initialized BIP32 public key
 * size_t hex_size = cardano_bip32_public_key_get_hex_size(bip32_public_key);
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * if (cardano_blake2b_hash_to_hex(bip32_public_key, hex_buffer, hex_size) == CARDANO_SUCCESS)
 * {
 *  // hex_buffer now contains the hexadecimal representation of the key
 * }
 *
 * free(hex_buffer); // Always ensure to free allocated memory
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_public_key_to_hex(
  const cardano_bip32_public_key_t* public_key,
  char*                             hex_key,
  size_t                            key_length);

/**
 * \brief Converts a BIP32 public key into an Ed25519 key hash.
 *
 * This function takes a BIP32 public key and computes its hash using the BLAKE2b
 * hashing algorithm.
 *
 * \param[in] public_key A pointer to the `cardano_bip32_public_key_t` object representing
 *                   the BIP32 public key to be hashed.
 * \param[out] hash A pointer to a pointer that will be set to point to the newly created
 *             `cardano_blake2b_hash_t` object, containing the hash of the BIP32
 *             public key. It is the responsibility of the caller to manage the lifecycle
 *             of this object, including its deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the hash was successfully created
 *         from the BIP32 public key. On failure, a non-zero error code is returned,
 *         indicating an issue such as an invalid public key or failure in the hashing
 *         process. If the function fails, the value pointed to by `hash` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * cardano_bip32_public_key_t* bip32_public_key = ...; // Assume the BIP32 public key is initialized
 * cardano_blake2b_hash_t* hash = NULL;
 *
 * cardano_error_t error = cardano_bip32_public_key_to_hash(bip32_public_key, &hash);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `hash` can now be used for further cryptographic operations
 * }
 *
 * // Clean up
 * cardano_ed25519_key_hash_unref(&hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_public_key_to_hash(
  const cardano_bip32_public_key_t* public_key,
  cardano_blake2b_hash_t**          hash);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_PUBLIC_KEY_H
