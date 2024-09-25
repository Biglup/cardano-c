/**
 * \file credential.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CREDENTIAL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CREDENTIAL_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential_type.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a credential used in the Cardano blockchain, which can be either a key hash or a script hash.
 */
typedef struct cardano_credential_t cardano_credential_t;

/**
 * \brief Creates and initializes a new instance of a credential.
 *
 * This function allocates and initializes a new instance of \ref cardano_credential_t,
 * using the provided hash and credential type. It returns an error code to indicate success
 * or failure of the operation.
 *
 * \param[in] hash A pointer to \ref cardano_blake2b_hash_t representing the hash associated
 *             with this credential. The hash must be properly initialized before being
 *             passed to this function.
 * \param[in] type The type of credential, as defined in \ref cardano_credential_type_t,
 *             either a key hash or a script hash.
 * \param[out] credential On successful initialization, this will point to a newly created
 *             \ref cardano_credential_t object. This object represents a "strong reference"
 *             to the credential, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the credential is no longer needed, the caller must release it
 *             by calling \ref cardano_credential_unref.
 *
 * \return \ref CARDANO_SUCCESS if the credential was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* hash = { ... };  // Assume hash is initialized here
 * cardano_credential_t* credential = NULL;
 *
 * // Attempt to create a new credential
 * cardano_error_t result = cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 *
 * cardano_blake2b_hash_unref(&hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_credential_new(
  const cardano_blake2b_hash_t* hash,
  cardano_credential_type_t     type,
  cardano_credential_t**        credential);

/**
 * \brief Creates a credential from a hexadecimal hash string.
 *
 * This function constructs a \ref cardano_credential_t object by interpreting the provided
 * hexadecimal string as a hash value and associating it with a specified credential type. It
 * returns an error code indicating the success or failure of the operation.
 *
 * \param[in] hex A pointer to a character array containing the hexadecimal representation of the hash.
 * \param[in] hex_size The size of the hexadecimal string in bytes.
 * \param[in] type The type of credential, as defined in \ref cardano_credential_type_t,
 *                 which determines how the hash is to be treated (e.g., as a key hash or a script hash).
 * \param[out] credential On successful initialization, this will point to a newly created
 *                 \ref cardano_credential_t object. This object represents a "strong reference"
 *                 to the credential, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the credential is no longer needed, the caller must release it
 *                 by calling \ref cardano_credential_unref.
 *
 * \return \ref CARDANO_SUCCESS if the credential was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * \note The function assumes that the hexadecimal string is valid and correctly formatted.
 *       Malformed input may lead to incorrect or undefined behavior.
 *
 * Usage Example:
 * \code{.c}
 * const char* hash_hex = "abcdef1234567890abcdef1234567890abcdef12....";
 * size_t hex_size = strlen(hash_hex);
 * cardano_credential_t* credential = NULL;
 *
 * // Attempt to create a new credential from a hexadecimal hash
 * cardano_error_t result = cardano_credential_from_hash_hex(hash_hex, hex_size, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_credential_from_hash_hex(
  const char*               hex,
  size_t                    hex_size,
  cardano_credential_type_t type,
  cardano_credential_t**    credential);

/**
 * \brief Creates a credential from a byte array representing a hash.
 *
 * This function constructs a \ref cardano_credential_t object by using the provided
 * byte array as a hash value and associating it with a specified credential type. It
 * returns an error code indicating the success or failure of the operation.
 *
 * \param[in] data A pointer to the byte array containing the hash data.
 * \param[in] data_size The size of the byte array in bytes.
 * \param[in] type The type of credential, as defined in \ref cardano_credential_type_t,
 *                 which determines how the hash is to be treated (e.g., as a key hash or a script hash).
 * \param[out] credential On successful initialization, this will point to a newly created
 *                 \ref cardano_credential_t object. This object represents a "strong reference"
 *                 to the credential, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the credential is no longer needed, the caller must release it
 *                 by calling \ref cardano_credential_unref.
 *
 * \return \ref CARDANO_SUCCESS if the credential was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t hash_data[] = { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef ... };
 * size_t data_size = sizeof(hash_data);
 * cardano_credential_t* credential = NULL;
 *
 * // Attempt to create a new credential from byte array hash
 * cardano_error_t result = cardano_credential_from_hash_bytes(hash_data, data_size, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
 *
 * if (result == CARDANO_SUCCESS && credential)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_credential_from_hash_bytes(
  const byte_t*             data,
  size_t                    data_size,
  cardano_credential_type_t type,
  cardano_credential_t**    credential);

/**
 * \brief Creates a credential from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_credential_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a credential.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded credential data.
 * \param[out] credential A pointer to a pointer of \ref cardano_credential_t that will be set to the address
 *                        of the newly created credential object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the credential was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_credential_t object by calling
 *       \ref cardano_credential_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_credential_t* credential = NULL;
 *
 * cardano_error_t result = cardano_credential_from_cbor(reader, &credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode credential: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_credential_from_cbor(cardano_cbor_reader_t* reader, cardano_credential_t** credential);

/**
 * \brief Serializes a credential into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_credential_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] credential A constant pointer to the \ref cardano_credential_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p credential or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_credential_to_cbor(credential, writer);
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
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_credential_to_cbor(
  const cardano_credential_t* credential,
  cardano_cbor_writer_t*      writer);

/**
 * \brief Retrieves the hash associated with a credential.
 *
 * This function provides access to the hash part of a \ref cardano_credential_t object.
 * It returns a new reference to a \ref cardano_blake2b_hash_t object representing the hash.
 * This allows the hash to be used independently of the original credential object. The
 * reference count of the hash object is increased by one, making it the caller's responsibility
 * to release it by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \param credential A constant pointer to the \ref cardano_credential_t object from which
 *                   the hash is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_blake2b_hash_t object containing the hash.
 *         If the input credential is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_blake2b_hash_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* original_credential = cardano_credential_new(...);
 * cardano_blake2b_hash_t* hash_credential = cardano_credential_get_hash(original_credential);
 *
 * if (hash_credential)
 * {
 *   // Use the hash credential
 *
 *   // Once done, ensure to clean up and release the hash credential
 *   cardano_blake2b_hash_unref(&hash_credential);
 * }
 * // Release the original credential after use
 * cardano_credential_unref(&original_credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_credential_get_hash(const cardano_credential_t* credential);

/**
 * \brief Retrieves the size of the hash bytes stored in the credential.
 *
 * This function computes the size of the hash bytes stored within a \ref cardano_credential_t object.
 * It is particularly useful for determining the buffer size needed to store the hash bytes when
 * retrieving them via \ref cardano_credential_get_hash_bytes.
 *
 * \param[in] credential A constant pointer to the \ref cardano_credential_t object from which
 *                       the size of the hash bytes is to be retrieved.
 *
 * \return The size of the hash bytes. If the credential is NULL the function returns zero.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new(...);
 * size_t hash_size = cardano_credential_get_hash_bytes_size(credential);
 *
 * if (hash_size > 0)
 * {
 *   byte_t* hash_bytes = malloc(hash_size);
 *   if (hash_bytes)
 *   {
 *     // Proceed to get the hash bytes
 *   }
 * }
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_credential_get_hash_bytes_size(const cardano_credential_t* credential);

/**
 * \brief Retrieves the byte array representation of the hash from a credential.
 *
 * This function accesses the byte representation of the hash associated with a given
 * \ref cardano_credential_t object. It provides a direct pointer to the internal byte array
 * representing the hash, which should not be modified or freed by the caller.
 *
 * \param credential A constant pointer to the \ref cardano_credential_t object from which
 *                   the hash bytes are to be retrieved.
 *
 * \return A pointer to a constant byte array containing the hash data. If the input credential
 *         is NULL, returns NULL. The data remains valid as long as the credential object is not
 *         freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new(...);
 * const byte_t* hash_bytes = cardano_credential_get_hash_bytes(credential);
 *
 * if (hash_bytes)
 * {
 *   // Use the hash bytes for operations like comparison or display
 * }
 *
 * // Clean up the credential object once done
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_credential_get_hash_bytes(const cardano_credential_t* credential);

/**
 * \brief Retrieves the size needed for the hexadecimal string representation of the credential's hash.
 *
 * This function calculates the size required to store the hexadecimal string representation of the hash
 * associated with a \ref cardano_credential_t object. This size includes the space needed for the null-terminator.
 *
 * \param[in] credential A constant pointer to the \ref cardano_credential_t object whose hash size is to be determined.
 *
 * \return The size in bytes required to store the hexadecimal representation of the hash, including the null terminator.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new(...);
 * size_t hex_size = cardano_credential_get_hash_hex_size(credential);
 * char* hex_string = malloc(hex_size);
 *
 * if (hex_string)
 * {
 *   // Now use hex_string to get the hash or do other operations
 *   free(hex_string);
 * }
 *
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_credential_get_hash_hex_size(const cardano_credential_t* credential);

/**
 * \brief Retrieves the hexadecimal string representation of the hash from a credential.
 *
 * This function provides access to the hexadecimal (hex) string representation of the hash
 * associated with a given \ref cardano_credential_t object. It returns a direct pointer to the
 * internal hex string which should not be modified or freed by the caller.
 *
 * \param credential A constant pointer to the \ref cardano_credential_t object from which
 *                   the hex string of the hash is to be retrieved. The object must not be NULL.
 *
 * \return A pointer to a constant character array containing the hex representation of the hash.
 *         If the input credential is NULL, returns NULL. The data remains valid as long as the
 *         credential object is not freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new(...);
 * const char* hash_hex = cardano_credential_get_hash_hex(credential);
 *
 * if (hash_hex)
 * {
 *   // Use the hash hex for operations like logging, display, or comparison
 * }
 *
 * // Clean up the credential object once done
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_credential_get_hash_hex(const cardano_credential_t* credential);

/**
 * \brief Retrieves the type of the credential.
 *
 * This function retrieves the type of a given \ref cardano_credential_t object and stores it in the provided
 * output parameter. The credential type is defined in the \ref cardano_credential_type_t enumeration, which
 * specifies whether the credential is a key hash or a script hash.
 *
 * \param[in] credential A constant pointer to the \ref cardano_credential_t object from which
 *                       the type is to be retrieved. The object must not be NULL.
 * \param[out] type Pointer to a variable where the credential type will be stored. This variable will
 *                  be set to the value from the \ref cardano_credential_type_t enumeration.
 *
 * \return CARDANO_SUCCESS if the type was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new(...);
 * cardano_credential_type_t type;
 * cardano_error_t result = cardano_credential_get_type(credential, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (type == CARDANO_CREDENTIAL_TYPE_KEY_HASH)
 *   {
 *     // Handle key hash type credential
 *   }
 *   else if (type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH)
 *   {
 *     // Handle script hash type credential
 *   }
 * }
 *
 * // Clean up the credential object once done
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_credential_get_type(const cardano_credential_t* credential, cardano_credential_type_t* type);

/**
 * \brief Sets the type of the credential.
 *
 * This function assigns a new type to an existing \ref cardano_credential_t object. The type is specified
 * by the \ref cardano_credential_type_t enumeration, which indicates whether the credential is derived from
 * a public key hash or a script hash.
 *
 * \param[in,out] credential A pointer to the \ref cardano_credential_t object whose type is to be set.
 * \param[in] type The new type of the credential, as defined in the \ref cardano_credential_type_t enumeration.
 *
 * \return Returns \ref CARDANO_SUCCESS if the type was successfully set. Returns \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p credential pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new(...);
 * cardano_error_t result = cardano_credential_set_type(credential, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Credential type updated successfully
 * }
 *
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_credential_set_type(cardano_credential_t* credential, cardano_credential_type_t type);

/**
 * \brief Sets the hash for a credential.
 *
 * This function assigns a new hash to an existing \ref cardano_credential_t object. The hash
 * represents the identifying data for the credential.The function copies the provided hash into
 * the credential, so the original hash object may be modified or freed after this operation without
 * affecting the credential's hash.
 *
 * \param[in,out] credential A pointer to the \ref cardano_credential_t object whose hash is to be set.
 *                           This object must have been previously created and not yet freed.
 * \param[in] hash A pointer to a \ref cardano_blake2b_hash_t object containing the new hash to be set.
 *                 This parameter must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hash was successfully set. If the \p credential or \p hash is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...;
 * cardano_blake2b_hash_t* new_hash = cardano_blake2b_compute_hash(...);
 *
 * cardano_error_t result = cardano_credential_set_hash(credential, new_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     // The hash was successfully set
 * }
 *
 * // Clean up
 * cardano_credential_unref(&credential);
 * cardano_blake2b_hash_unref(&new_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_credential_set_hash(cardano_credential_t* credential, const cardano_blake2b_hash_t* hash);

/**
 * \brief Compares two credential objects for equality.
 *
 * This function compares two credential objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first credential object.
 * \param[in] rhs Pointer to the second credential object.
 *
 * \return \c true if the credential objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential1 = NULL;
 * cardano_credential_t* credential2 = NULL;
 *
 * // Assume credential1 and credential2 are initialized properly
 *
 * bool are_equal = cardano_credential_equals(credential1, credential2);
 *
 * if (are_equal)
 * {
 *   printf("The credential objects are equal.\n");
 * }
 * else
 * {
 *   printf("The credential objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_credential_unref(&credential1);
 * cardano_credential_unref(&credential2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_credential_equals(
  const cardano_credential_t* lhs,
  const cardano_credential_t* rhs);

/**
 * \brief Compares two credential objects.
 *
 * This function compares two credential objects and returns an integer indicating
 * their relative order.
 *
 * \param[in] lhs Pointer to the first credential object.
 * \param[in] rhs Pointer to the second credential object.
 *
 * \return A negative value if lhs is less than rhs, zero if they are equal, and a positive value if lhs is greater than rhs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential1 = NULL;
 * cardano_credential_t* credential2 = NULL;
 *
 * // Assume credential1 and credential2 are initialized properly
 *
 * int32_t comparison = cardano_credential_compare(credential1, credential2);
 * if (comparison < 0)
 * {
 *   printf("credential1 is less than credential2.\n");
 * }
 * else if (comparison == 0)
 * {
 *   printf("credential1 is equal to credential2.\n");
 * }
 * else
 * {
 *   printf("credential1 is greater than credential2.\n");
 * }
 *
 * // Clean up
 * cardano_credential_unref(&credential1);
 * cardano_credential_unref(&credential2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT int32_t cardano_credential_compare(
  const cardano_credential_t* lhs,
  const cardano_credential_t* rhs);

/**
 * \brief Decrements the reference count of a credential object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_credential_t object
 * by decreasing its reference count. When the reference count reaches zero, the credential is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] credential A pointer to the pointer of the credential object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new();
 *
 * // Perform operations with the credential...
 *
 * cardano_credential_unref(&credential);
 * // At this point, credential is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_credential_unref, the pointer to the \ref cardano_credential_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_credential_unref(cardano_credential_t** credential);

/**
 * \brief Increases the reference count of the cardano_credential_t object.
 *
 * This function is used to manually increment the reference count of a credential
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_credential_unref.
 *
 * \param credential A pointer to the credential object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming credential is a previously created credential object
 *
 * cardano_credential_ref(credential);
 *
 * // Now credential can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_credential_ref there is a corresponding
 * call to \ref cardano_credential_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_credential_ref(cardano_credential_t* credential);

/**
 * \brief Retrieves the current reference count of the cardano_credential_t object.
 *
 * This function returns the number of active references to a credential object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_credential_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param credential A pointer to the credential object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified credential object. If the object
 * is properly managed (i.e., every \ref cardano_credential_ref call is matched with a
 * \ref cardano_credential_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming credential is a previously created credential object
 *
 * size_t ref_count = cardano_credential_refcount(credential);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_credential_refcount(const cardano_credential_t* credential);

/**
 * \brief Sets the last error message for a given credential object.
 *
 * Records an error message in the credential's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] credential A pointer to the \ref cardano_credential_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the credential's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_credential_set_last_error(cardano_credential_t* credential, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific credential.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_credential_set_last_error for the given
 * credential. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] credential A pointer to the \ref cardano_credential_t instance whose last error
 *                   message is to be retrieved. If the credential is NULL, the function
 *                   returns a generic error message indicating the null credential.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified credential. If the credential is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_credential_set_last_error for the same credential, or until
 *       the credential is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_credential_get_last_error(const cardano_credential_t* credential);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CREDENTIAL_H