/**
 * \file bip32_private_key.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_PRIVATE_KEY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_PRIVATE_KEY_H

/* INCLUDES ******************************************************************/

#include <cardano/crypto/bip32_public_key.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a BIP32 hierarchical deterministic (HD) private key.
 *
 * This structure encapsulates a private key following the BIP32 standard, which can be used for
 * generating a deterministic series of private keys from a single master seed. BIP32 private keys
 * allow for the structured management of cryptocurrency wallets through a tree-like hierarchy of
 * key derivation paths. This enables a single seed to give rise to a practically unlimited number
 * of child keys, each of which can be used independently.
 */
typedef struct cardano_bip32_private_key_t cardano_bip32_private_key_t;

/**
 * \brief Creates a BIP32 private key object from a raw byte array.
 *
 * This function takes an array of bytes representing a BIP32 private key and constructs
 * a new `cardano_bip32_private_key_t` object.
 *
 * \param[in] key_bytes A pointer to the byte array containing the binary representation of the BIP32 private key.
 * \param[in] key_length The length of the byte array pointed to by `key`.
 * \param[out] key A pointer to a pointer that will be set to point to the newly created `cardano_bip32_private_key_t` object.
 *            The caller is responsible for freeing this object using the appropriate API function to prevent memory leaks.
 *
 * \return A \c cardano_error_t indicating the result of the operation. On success, the function
 *         returns \ref CARDANO_SUCCESS, and the `key` parameter is updated to point to the newly created
 *         private key object. On failure, a non-zero error code is returned, and the value pointed to by
 *         `key` is set to `NULL`, indicating that the private key object was not successfully created.
 *
 * Example Usage:
 * \code
 * const byte_t raw_key[] = { ... }; // Byte array containing the BIP32 private key
 * size_t raw_key_length = sizeof(raw_key);
 * cardano_bip32_private_key_t* bip32_key = NULL;
 *
 * cardano_error_t result = cardano_bip32_private_key_from_bytes(raw_key, raw_key_length, &bip32_key);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The `bip32_key` can now be used for operations like address generation
 * }
 *
 * // Remember to free the BIP32 private key object when done
 * cardano_bip32_private_key_unref(&bip32_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_from_bytes(
  const byte_t*                 key_bytes,
  size_t                        key_length,
  cardano_bip32_private_key_t** key);

/**
 * \brief Generates a BIP32 private key from BIP39 entropy.
 *
 * This function creates a BIP32 hierarchical deterministic (HD) private key using
 * the provided BIP39 entropy. The entropy typically comes from a mnemonic seed phrase,
 * which is converted into a binary format. Additionally, a password can be supplied for
 * use in the key generation process, adding an extra layer of security. The resulting
 * BIP32 private key can be used to derive wallet addresses and other private keys within
 * a hierarchical structure.
 *
 * \param[in] password A pointer to the byte array containing the password or passphrase.
 *                 This can be an empty string if no password is used.
 * \param[in] password_length The length of the password in bytes.
 * \param[in] entropy A pointer to the byte array containing the entropy derived from a BIP39 mnemonic.
 * \param[in] entropy_length The length of the entropy in bytes.
 * \param[out] key A pointer to a pointer that will be set to point to the newly created
 *            `cardano_bip32_private_key_t` object. It is the responsibility of the caller
 *            to manage the lifecycle of this object, including its deallocation, to
 *            prevent memory leaks.
 *
 * \return cardano_error_t Returns \ref CARDANO_SUCCESS if the BIP32 private key was successfully
 *         created. On failure, a non-zero error code is returned, indicating an issue with
 *         the inputs or an internal error in the key generation process. If the function
 *         fails, the value pointed to by `key` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * const byte_t password[] = "optional password";
 * size_t password_length = sizeof(password);
 * const byte_t entropy[] = { ... }; // Entropy derived from a BIP39 mnemonic
 * size_t entropy_length = sizeof(entropy);
 * cardano_bip32_private_key_t* bip32_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_private_key_from_bip39_entropy(password, password_length, entropy, entropy_length, &bip32_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *     // The `bip32_key` can now be used
 * }
 *
 * // Clean up
 * cardano_bip32_private_key_unref(&bip32_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_from_bip39_entropy(
  const byte_t*                 password,
  size_t                        password_length,
  const byte_t*                 entropy,
  size_t                        entropy_length,
  cardano_bip32_private_key_t** key);

/**
 * \brief Creates a BIP32 private key object from a hexadecimal string.
 *
 * This function parses a hexadecimal string representing a BIP32 private key and constructs
 * a `cardano_bip32_private_key_t` object from it.
 *
 * \param[in] hex A pointer to the null-terminated string containing the hexadecimal representation
 *            of the BIP32 private key.
 * \param[in] hex_len The length of the hexadecimal string, excluding the null terminator.
 * \param[out] key A pointer to a pointer to `cardano_bip32_private_key_t` where the address of
 *            the newly created private key object will be stored. It is the caller's responsibility
 *            to manage the lifecycle of the created object, including its deallocation through
 *            the appropriate API function to prevent memory leaks.
 *
 * \return A \c cardano_error_t indicating the result of the operation. On success, the function
 *         returns \ref CARDANO_SUCCESS, and the `key` parameter is updated to point to the newly created
 *         private key object. On failure, a non-zero error code is returned, and the value pointed to by
 *         `key` is set to `NULL`, indicating that the private key object was not successfully created.
 *
 * Example Usage:
 * \code
 * const char* hex_key = "a1b2c3d4..."; // Hexadecimal string of the private key
 * size_t hex_key_len = strlen(hex_key);
 * cardano_bip32_private_key_t* private_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_private_key_from_hex(hex_key, hex_key_len, &private_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `private_key` can now be used for cryptographic operations
 * }
 *
 * // Clean up
 * cardano_bip32_private_key_unref(&private_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_from_hex(
  const char*                   hex,
  size_t                        hex_len,
  cardano_bip32_private_key_t** key);

/**
 * \brief Decrements the reference count of a BIP32 private key object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_bip32_private_key_t object
 * by decreasing its reference count. When the reference count reaches zero, the BIP32 private key is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] private_key A pointer to the pointer of the BIP32 private key object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_bip32_private_key_t* key = cardano_bip32_private_key_new();
 *
 * // Perform operations with the key...
 *
 * cardano_bip32_private_key_unref(&key);
 * // At this point, key is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_bip32_private_key_unref, the pointer to the \ref cardano_bip32_private_key_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_bip32_private_key_unref(cardano_bip32_private_key_t** private_key);

/**
 * \brief Increases the reference count of the cardano_bip32_private_key_t object.
 *
 * This function is used to manually increment the reference count of a BIP32 private key
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_bip32_private_key_unref.
 *
 * \param[in] private_key A pointer to the BIP32 private key object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming key is a previously created BIP32 private key object
 *
 * cardano_bip32_private_key_ref(key);
 *
 * // Now key can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_bip32_private_key_ref there is a corresponding
 * call to \ref cardano_bip32_private_key_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_bip32_private_key_ref(cardano_bip32_private_key_t* private_key);

/**
 * \brief Retrieves the current reference count of the cardano_bip32_private_key_t object.
 *
 * This function returns the number of active references to a BIP32 private key object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_bip32_private_key_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param[in] private_key A pointer to the BIP32 private key object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified BIP32 private key object. If the object
 * is properly managed (i.e., every \ref cardano_bip32_private_key_ref call is matched with a
 * \ref cardano_bip32_private_key_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming key is a previously created BIP32 private key object
 *
 * size_t ref_count = cardano_bip32_private_key_refcount(key);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bip32_private_key_refcount(const cardano_bip32_private_key_t* private_key);

/**
 * \brief Derives a child BIP32 private key from a parent BIP32 private key using a specified derivation path.
 *
 * This function takes a BIP32 private key and an array of indices representing the derivation path
 * to produce a child private key. The derivation path allows for a structured hierarchy of keys,
 * enabling the generation of multiple keys from a single seed, in accordance with the BIP32
 * specification.
 *
 * \param[in] private_key A pointer to the initial `cardano_bip32_private_key_t` from which
 *                        the derivation process starts. This key acts as the root of the
 *                        derivation path.
 * \param[in] indices An array of uint32_t values representing the derivation path. Each value
 *                    specifies an index at which a child key should be derived from the current key
 *                    in the path. Hardened keys are indicated by indices >= 2^31.
 * \param[in] indices_count The number of indices in the `indices` array, indicating the depth of
 *                          the derivation path.
 * \param[out] derived_private_key A pointer to a pointer to `cardano_bip32_private_key_t` that will be updated
 *                         to point to the newly derived private key object. The caller is responsible
 *                         for managing the lifecycle of this object, including its deallocation, to
 *                         prevent memory leaks.
 *
 * \return cardano_error_t Returns \ref CARDANO_SUCCESS if the child private key was successfully derived.
 *         On failure, a non-zero error code is returned, indicating an issue with the inputs,
 *         the inability to derive a key at the specified path, or an internal error during the
 *         derivation process. If the function fails, the value pointed to by `derived_private_key` is
 *         set to `NULL`.
 *
 * Example Usage:
 * \code
 * uint32_t derivation_path[] = { ... };
 * size_t path_length = sizeof(derivation_path) / sizeof(uint32_t);
 * cardano_bip32_private_key_t* parent_key = ...; // Assume parent_key is previously initialized
 * cardano_bip32_private_key_t* child_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_private_key_derive(parent_key, derivation_path, path_length, &child_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `child_key` can now be used for transactions, address generation, etc.
 * }
 *
 * // Clean up
 * cardano_bip32_private_key_unref(&child_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_derive(
  const cardano_bip32_private_key_t* private_key,
  const uint32_t*                    indices,
  size_t                             indices_count,
  cardano_bip32_private_key_t**      derived_private_key);

/**
 * \brief Converts a BIP32 private key to an Ed25519 private key.
 *
 * This function allows for the conversion of a private key from the BIP32 hierarchical
 * deterministic wallet format to an Ed25519 private key format.
 *
 * \param[in] private_key A pointer to the `cardano_bip32_private_key_t` object representing the
 *                   BIP32 private key to be converted.
 * \param[out] ed25519_private_key A pointer to a pointer to `cardano_ed25519_private_key_t` that
 *                           will be updated to point to the newly created Ed25519 private key
 *                           object. The caller is responsible for managing the lifecycle of
 *                           this object, including its deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns an error code indicating the outcome of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS, and `ed25519_private_key` is
 *         updated to point to the new Ed25519 private key object. On failure, a non-zero
 *         error code is returned, and the value pointed to by `ed25519_private_key` is set
 *         to `NULL`, indicating that the conversion was unsuccessful.
 *
 * Example Usage:
 * \code
 * cardano_bip32_private_key_t* bip32_private_key = ...; // Assume this is initialized
 * cardano_ed25519_private_key_t* ed25519_private_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_private_key_to_ed25519_key(bip32_private_key, &ed25519_private_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // `ed25519_private_key` can now be used for cryptographic operations requiring Ed25519 keys
 * }
 *
 * // Remember to free the Ed25519 private key object when done
 * cardano_ed25519_private_key_unref(&ed25519_private_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_to_ed25519_key(
  const cardano_bip32_private_key_t* private_key,
  cardano_ed25519_private_key_t**    ed25519_private_key);

/**
 * \brief Extracts the public key from a BIP32 private key.
 *
 * This function derives the corresponding public key from a given BIP32 private key.
 * The derived public key is essential for various operations within the Cardano ecosystem,
 * such as generating wallet addresses or verifying signatures, where the private key itself
 * must remain secret. The public key is derived without compromising the private key.
 *
 * \param[in] private_key A pointer to the `cardano_bip32_private_key_t` object from which
 *                    the public key is to be derived. This private key must have been
 *                    properly initialized and represent a valid BIP32 private key.
 * \param[out] ed25519_public_key A pointer to a pointer to `cardano_bip32_public_key_t` that will
 *                           be updated to point to the newly created public key object. It is
 *                           the caller's responsibility to manage the lifecycle of this object,
 *                           including its deallocation, to prevent memory leaks.
 *
 * \return cardano_error_t Returns \ref CARDANO_SUCCESS if the public key was successfully derived
 *         from the private key. On failure, a non-zero error code is returned, indicating
 *         an issue with the input private key or an internal error in the public key derivation
 *         process. If the function fails, the value pointed to by `ed25519_public_key` is set
 *         to `NULL`.
 *
 * Example Usage:
 * \code
 * cardano_bip32_private_key_t* bip32_private_key = ...; // Previously initialized BIP32 private key
 * cardano_bip32_public_key_t* bip32_public_key = NULL;
 *
 * cardano_error_t error = cardano_bip32_private_key_get_public_key(bip32_private_key, &bip32_public_key);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The `bip32_public_key` can now be used for address generation, transaction verification, etc.
 * }
 *
 * // Clean up
 * cardano_bip32_public_key_unref(&bip32_public_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_get_public_key(
  const cardano_bip32_private_key_t* private_key,
  cardano_bip32_public_key_t**       ed25519_public_key);

/**
 * \brief Retrieves the size in bytes of a BIP32 private key.
 *
 * This function calculates and returns the size of the given BIP32 private key object
 * when represented as a byte array.
 *
 * \param[in] private_key A pointer to the `cardano_bip32_private_key_t` object whose byte size
 *                   is to be determined.
 *
 * \return The size of the BIP32 private key in bytes. If the provided `private_key` is invalid
 *         or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * cardano_bip32_private_key_t* bip32_private_key = ...; // Previously created or loaded BIP32 private key
 * size_t key_size = cardano_bip32_private_key_get_bytes_size(bip32_private_key);
 *
 * // Use key_size to allocate memory for the key's byte representation
 * byte_t* key_bytes = (byte_t*)malloc(key_size);
 * // Ensure to check for malloc failure and handle `key_bytes` appropriately
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bip32_private_key_get_bytes_size(const cardano_bip32_private_key_t* private_key);

/**
 * \brief Retrieves a direct pointer to the internal data of a BIP32 private key object.
 *
 * This function provides access to the internal storage of the BIP32 private key object, allowing for read-only operations on its contents.
 * It is intended for situations where direct access to the data is necessary for performance or interoperability reasons.
 *
 * \warning The returned pointer provides raw, direct access to the BIP32 private key object's internal data. It must not be used to modify
 * the buffer contents outside the API's control, nor should it be deallocated using free or similar memory management functions.
 * The lifecycle of the data pointed to by the returned pointer is managed by the BIP32 private key object's reference counting mechanism; therefore,
 * the data remains valid as long as the BIP32 private key object exists and has not been deallocated.
 *
 * \param[in] bip32_private_key The BIP32 private key instance from which to retrieve the internal data pointer. The BIP32 private key must have been previously
 * created and not yet deallocated.
 *
 * \return Returns a pointer to the internal data of the BIP32 private key. If the BIP32 private key instance is invalid,
 * returns NULL. The returned pointer provides direct access to the BIP32 private key's contents and must be used in accordance with the
 * warning above to avoid unintended consequences.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_bip32_private_key_get_data(const cardano_bip32_private_key_t* bip32_private_key);

/**
 * \brief Serializes a BIP32 private key into a provided byte array.
 *
 * Converts the given BIP32 private key object into its byte representation and stores it
 * in the provided buffer.
 *
 * \param[in] private_key A pointer to the `cardano_bip32_private_key_t` object representing the
 *                        BIP32 private key to be serialized.
 * \param[out] out_key_bytes A pointer to the buffer where the byte representation of the private key will
 *                           be stored. The buffer must be pre-allocated by the caller and have a size
 *                           sufficient to hold the entire serialized private key.
 * \param[in] out_key_length The size of the buffer pointed to by `key`, indicating the maximum
 *                           number of bytes that can be written into the buffer. To ensure
 *                           successful serialization, this size should be at least as large as
 *                           the size returned by `cardano_bip32_private_key_get_bytes_size` for
 *                           the given `private_key`.
 *
 * \return cardano_error_t Returns \ref CARDANO_SUCCESS if the private key was successfully
 *         serialized into the byte array. If the function fails, it returns a non-zero
 *         error code. Possible failure reasons include invalid input parameters (such as
 *         a NULL `private_key`), a `key` buffer that is too small (`key_length` insufficient),
 *         or other internal errors encountered during the serialization process.
 *
 * Example Usage:
 * \code
 * // Assuming 'bip32_private_key' is a previously initialized BIP32 private key
 * const size_t required_size = cardano_bip32_private_key_get_bytes_size(bip32_private_key);
 * byte_t* buffer = malloc(required_size);
 *
 * if (buffer != NULL)
 * {
 *   cardano_error_t result = cardano_bip32_private_key_to_bytes(bip32_private_key, buffer, required_size);
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
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_to_bytes(
  const cardano_bip32_private_key_t* private_key,
  byte_t*                            out_key_bytes,
  size_t                             out_key_length);

/**
 * \brief Determines the required buffer size for the hexadecimal representation of a BIP32 private key.
 *
 * This function calculates the length of the string that would be required to store the
 * hexadecimal representation of a given BIP32 private key, including the null-terminator.
 *
 * \param[in] private_key A pointer to the `cardano_bip32_private_key_t` object whose hexadecimal
 *                   size is being queried.
 *
 * \return The size of the buffer (in bytes) needed to store the hexadecimal string representation
 *         of the BIP32 private key, including space for the null-terminator. If the provided
 *         `private_key` is invalid or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * // Assuming 'bip32_private_key' is a previously initialized BIP32 private key
 * size_t hex_size = cardano_bip32_private_key_get_hex_size(bip32_private_key);
 *
 *  char* hex_buffer = (char*)malloc(hex_size);
 *
 *  if (hex_buffer != NULL)
 *  {
 *    // Buffer is ready for use with cardano_bip32_private_key_to_hex
 *  }
 *
 *  // Remember to free the allocated buffer
 *  free(hex_buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_bip32_private_key_get_hex_size(const cardano_bip32_private_key_t* private_key);

/**
 * \brief Serializes a BIP32 private key into its hexadecimal string representation.
 *
 * Converts the given BIP32 private key object into a hexadecimal string and stores it
 * in the provided buffer. The caller is responsible for ensuring that the buffer is sufficiently
 * large to hold the entire  hexadecimal string, including the null terminator. The required size
 * can be determined by calling \ref cardano_bip32_private_key_get_hex_size.
 *
 * \param[in] private_key A pointer to the `cardano_bip32_private_key_t` object representing the
 *                   BIP32 private key to be converted to hexadecimal format.
 * \param[out] hex_key A pointer to the buffer where the hexadecimal string representation of the
 *                private key will be stored. The buffer must be pre-allocated and should be
 *                large enough to hold the hexadecimal string plus a null terminator.
 * \param[in] key_length The length of the buffer pointed to by `hex_key`, indicating the maximum
 *                   number of characters (bytes) that can be written into the buffer,
 *                   including space for the null terminator.
 *
 * \return cardano_error_t Returns \ref CARDANO_SUCCESS if the private key was successfully
 *         serialized into the hexadecimal format and stored in the provided buffer.
 *         If the function fails, it returns a non-zero error code. Failure can occur
 *         for several reasons, including but not limited to an invalid `private_key`,
 *         an insufficiently sized buffer (`key_length` too small), or other internal errors
 *         encountered during the serialization process.
 *
 * Example Usage:
 * \code
 * cardano_bip32_private_key_t* bip32_private_key = ...; // Previously initialized BIP32 private key
 * size_t hex_size = cardano_bip32_private_key_get_hex_size(bip32_private_key);
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * if (cardano_blake2b_hash_to_hex(bip32_private_key, hex_buffer, hex_size) == CARDANO_SUCCESS)
 * {
 *    // hex_buffer now contains the hexadecimal representation of the key
 * }
 *
 * free(hex_buffer); // Always ensure to free allocated memory
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip32_private_key_to_hex(
  const cardano_bip32_private_key_t* private_key,
  char*                              hex_key,
  size_t                             key_length);

/**
 * \brief Hardens a given BIP32 index.
 *
 * This function converts a given index into its hardened form according to BIP32 specifications.
 * In BIP32, an index is hardened by adding 2^31 to it, which sets the highest bit of a 32-bit integer.
 * Hardened indices prevent the derivation of child private keys from parent public keys.
 *
 * Hardening ensures that even if an attacker gains access to a child private key and the parent public
 * key, they cannot derive the parent private key or any sibling private keys.
 *
 * \param[in] index The index to be hardened. This index should be within the range of valid BIP32 indices
 *                 (0 through 2^31 - 1) for non-hardened indices. Indices outside this range may already
 *                 be considered hardened.
 *
 * \return The hardened index, which is the input index with 2^31 added to it. If the input index is already
 *         hardened (i.e., it has the highest bit set), the function returns the input index unchanged.
 *
 * Example Usage:
 * \code
 * uint32_t index = 44; // Example index for a BIP32 derivation path
 * uint32_t hardened_index = cardano_bip32_harden(index);
 * // hardenedIndex now equals 44 + 2^31, indicating it's a hardened index
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint32_t cardano_bip32_harden(uint32_t index);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_PRIVATE_KEY_H
