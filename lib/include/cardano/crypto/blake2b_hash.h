/**
 * \file blake2b_hash.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a BLAKE2b hash.
 *
 * This structure encapsulates the result of a BLAKE2b hashing operation, abstracting
 * the underlying hash data. It is used throughout the Cardano system in various contexts,
 * such as transaction identification, address generation, and cryptographic verification.
 */
typedef struct cardano_blake2b_hash_t cardano_blake2b_hash_t;

/**
 * \brief Computes a BLAKE2b hash for the given data.
 *
 * This function calculates the BLAKE2b hash of the specified input data and returns
 * the hash result as a newly allocated `cardano_blake2b_hash_t` instance.
 *
 * \param[in] data A pointer to the byte array containing the input data to hash.
 * \param[in] data_length The length of the input data array in bytes.
 * \param[in] hash_length The desired length of the resulting hash in bytes. Valid
 * values depend on the specific BLAKE2b variant being used (e.g., 28 for BLAKE2b-224,
 * 32 for BLAKE2b-256, 64 for BLAKE2b-512).
 * \param[out] hash A pointer to a pointer to `cardano_blake2b_hash_t` that will be
 * allocated and set to point to the newly created hash object on successful
 * completion of the function.
 *
 * \return \ref cardano_error_t indicating the success or failure of the hash computation.
 * Returns `CARDANO_SUCCESS` on success, or an error code signifying the type of
 * failure encountered during the hash computation.
 *
 * Example Usage:
 * \code
 * byte_t data[] = { ... };
 * size_t data_length = sizeof(data);
 * cardano_blake2b_hash_t* hash = NULL;
 * cardano_error_t result = cardano_blake2b_compute_hash(data, data_length, CARDANO_BLAKE2B_HASH_SIZE_256, &hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the hash
 *   ...
 *   cardano_blake2b_hash_unref(&hash);
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_compute_hash(
  const byte_t*            data,
  size_t                   data_length,
  size_t                   hash_length,
  cardano_blake2b_hash_t** hash);

/**
 * \brief Creates a BLAKE2b hash object from raw byte data.
 *
 * This function takes a buffer of hash bytes and constructs a new BLAKE2b hash object.
 *
 * \param[in] data Pointer to the raw byte data of the hash.
 * \param[in] data_length The length of the data in bytes.
 * \param[out] hash A pointer to a pointer to `cardano_blake2b_hash_t` where the address of
 *             the newly created hash object will be stored. The caller is responsible
 *             for managing the lifecycle of the created hash object, including its
 *             proper deallocation through the corresponding unreference or destruction
 *             function to prevent memory leaks.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS and `hash` is set to point
 *         to the newly created hash object. On failure, a non-zero error code is
 *         returned and the value pointed to by `hash` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * byte_t data[] = { ... };
 * size_t data_length = sizeof(data) / sizeof(byte_t);
 * cardano_blake2b_hash_t* hash = NULL;
 *
 * cardano_error_t error = cardano_blake2b_hash_from_bytes(data, data_length, &hash);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Use the hash object for further operations
 * }
 *
 * // Clean up
 * cardano_blake2b_hash_unref(hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_from_bytes(
  const byte_t*            data,
  size_t                   data_length,
  cardano_blake2b_hash_t** hash);

/**
 * \brief Creates a BLAKE2b hash object from hexadecimal string data.
 *
 * This function takes a hexadecimal string representing hash data and constructs
 * a new BLAKE2b hash object. The function is designed to parse the hexadecimal
 * string input and convert it into the corresponding binary representation before
 * creating the hash object.
 *
 * \param[in] hex Pointer to the hexadecimal string to be converted into a hash object.
 * \param[in] hex_length The length of the hexadecimal string in characters.
 * \param[out] hash A pointer to a pointer to `cardano_blake2b_hash_t` where the address of
 *             the newly created hash object will be stored. The caller is responsible
 *             for managing the lifecycle of the created hash object, including its
 *             proper deallocation through the corresponding unreference or destruction
 *             function to prevent memory leaks.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS, and `hash` is set to
 *         point to the newly created hash object. On failure, a non-zero error code is
 *         returned, and the value pointed to by `hash` is set to `NULL`.
 *
 * Example Usage:
 * \code
 * const char* hex_data = "abc123..."; // Hexadecimal representation of the data
 * size_t hex_length = strlen(hex_data);
 * cardano_blake2b_hash_t* hash = NULL;
 *
 * cardano_error_t error = cardano_blake2b_hash_from_hex(hex_data, hex_length, &hash);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The hash object can now be used for further operations
 * }
 *
 * // Clean up
 * cardano_blake2b_hash_unref(hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_from_hex(
  const char*              hex,
  size_t                   hex_length,
  cardano_blake2b_hash_t** hash);

/**
 * \brief Creates a \ref cardano_blake2b_hash_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_blake2b_hash_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a blake2b_hash.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] blake2b_hash A pointer to a pointer of \ref cardano_blake2b_hash_t that will be set to the address
 *                        of the newly created blake2b_hash object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_blake2b_hash_t object by calling
 *       \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_blake2b_hash_t* blake2b_hash = NULL;
 *
 * cardano_error_t result = cardano_blake2b_hash_from_cbor(reader, &blake2b_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the blake2b_hash
 *
 *   // Once done, ensure to clean up and release the blake2b_hash
 *   cardano_blake2b_hash_unref(&blake2b_hash);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode blake2b_hash: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_blake2b_hash_from_cbor(cardano_cbor_reader_t* reader, cardano_blake2b_hash_t** blake2b_hash);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_blake2b_hash_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] blake2b_hash A constant pointer to the \ref cardano_blake2b_hash_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p blake2b_hash or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* blake2b_hash = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_blake2b_hash_to_cbor(blake2b_hash, writer);
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
 * cardano_blake2b_hash_unref(&blake2b_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_cbor(
  const cardano_blake2b_hash_t* blake2b_hash,
  cardano_cbor_writer_t*        writer);

/**
 * \brief Compares two Blake2b hash objects for equality.
 *
 * This function compares two Blake2b hash objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first Blake2b hash object.
 * \param[in] rhs Pointer to the second Blake2b hash object.
 *
 * \return \c true if the hash objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* hash1 = NULL;
 * cardano_blake2b_hash_t* hash2 = NULL;
 *
 * // Assume hash1 and hash2 are initialized properly
 *
 * bool are_equal = cardano_blake2b_hash_equals(hash1, hash2);
 *
 * if (are_equal)
 * {
 *   printf("The hash objects are equal.\n");
 * }
 * else
 * {
 *   printf("The hash objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_blake2b_hash_unref(&hash1);
 * cardano_blake2b_hash_unref(&hash2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_blake2b_hash_equals(
  const cardano_blake2b_hash_t* lhs,
  const cardano_blake2b_hash_t* rhs);

/**
 * \brief Compares two Blake2b hash objects.
 *
 * This function compares two Blake2b hash objects and returns an integer indicating
 * their relative order.
 *
 * \param[in] lhs Pointer to the first Blake2b hash object.
 * \param[in] rhs Pointer to the second Blake2b hash object.
 *
 * \return A negative value if lhs is less than rhs, zero if they are equal, and a positive value if lhs is greater than rhs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* hash1 = NULL;
 * cardano_blake2b_hash_t* hash2 = NULL;
 *
 * // Assume hash1 and hash2 are initialized properly
 *
 * int32_t comparison = cardano_blake2b_hash_compare(hash1, hash2);
 * if (comparison < 0)
 * {
 *   printf("hash1 is less than hash2.\n");
 * }
 * else if (comparison == 0)
 * {
 *   printf("hash1 is equal to hash2.\n");
 * }
 * else
 * {
 *   printf("hash1 is greater than hash2.\n");
 * }
 *
 * // Clean up
 * cardano_blake2b_hash_unref(&hash1);
 * cardano_blake2b_hash_unref(&hash2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT int32_t cardano_blake2b_hash_compare(
  const cardano_blake2b_hash_t* lhs,
  const cardano_blake2b_hash_t* rhs);

/**
 * \brief Decrements the reference count of a BLAKE2b hash object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_blake2b_hash_t object
 * by decreasing its reference count. When the reference count reaches zero, the BLAKE2b hash is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] hash A pointer to the pointer of the BLAKE2b hash object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_error_t result = cardano_blake2b_compute_hash(data, data_length, CARDANO_BLAKE2B_HASH_SIZE_256, &hash);
 *
 * // Perform operations with the hash...
 *
 * cardano_blake2b_hash_unref(&hash);
 * // At this point, hash is NULL and cannot be used.
 * \endcode
 */
CARDANO_EXPORT void cardano_blake2b_hash_unref(cardano_blake2b_hash_t** hash);

/**
 * \brief Increases the reference count of the cardano_blake2b_hash_t object.
 *
 * This function is used to manually increment the reference count of a BLAKE2b hash
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_blake2b_hash_unref.
 *
 * \param[in,out] hash A pointer to the BLAKE2b hash object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming hash is a previously created BLAKE2b hash object
 *
 * cardano_blake2b_hash_ref(hash);
 *
 * // Now hash can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_blake2b_hash_ref there is a corresponding
 * call to \ref cardano_blake2b_hash_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_blake2b_hash_ref(cardano_blake2b_hash_t* hash);

/**
 * \brief Retrieves the current reference count of the cardano_blake2b_hash_t object.
 *
 * This function returns the number of active references to a BLAKE2b hash object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_blake2b_hash_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param[in,out] hash A pointer to the BLAKE2b hash object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified BLAKE2b hash object. If the object
 * is properly managed (i.e., every \ref cardano_blake2b_hash_ref call is matched with a
 * \ref cardano_blake2b_hash_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming hash is a previously created BLAKE2b hash object
 *
 * size_t ref_count = cardano_blake2b_hash_refcount(hash);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_blake2b_hash_refcount(const cardano_blake2b_hash_t* hash);

/**
 * \brief Retrieves a direct pointer to the internal data of a blake2b hash object.
 *
 * This function provides access to the internal storage of the blake2b hash object, allowing for read-only operations on its contents.
 * It is intended for situations where direct access to the data is necessary for performance or interoperability reasons.
 *
 * \warning The returned pointer provides raw, direct access to the blake2b hash object's internal data. It must not be used to modify
 * the buffer contents outside the API's control, nor should it be deallocated using free or similar memory management functions.
 * The lifecycle of the data pointed to by the returned pointer is managed by the blake2b hash object's reference counting mechanism; therefore,
 * the data remains valid as long as the blake2b hash object exists and has not been deallocated.
 *
 * \param[in] blake2b_hash The blake2b hash instance from which to retrieve the internal data pointer. The blake2b hash must have been previously
 * created and not yet deallocated.
 *
 * \return Returns a pointer to the internal data of the blake2b hash. If the blake2b hash instance is invalid,
 * returns NULL. The returned pointer provides direct access to the blake2b hash's contents and must be used in accordance with the
 * warning above to avoid unintended consequences.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_blake2b_hash_get_data(const cardano_blake2b_hash_t* blake2b_hash);

/**
 * \brief Retrieves the size of the BLAKE2b hash object in bytes.
 *
 * \param[in] blake2b_hash A pointer to the `cardano_blake2b_hash_t` object whose size is to be retrieved.
 *
 * \return The size of the BLAKE2b hash object in bytes. If the provided hash object
 *         is invalid or NULL, the function will return 0.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_blake2b_hash_get_bytes_size(const cardano_blake2b_hash_t* blake2b_hash);

/**
 * \brief Converts a BLAKE2b hash object into a raw byte array representation.
 *
 * This function exports the contents of a given BLAKE2b hash object into a user-provided
 * byte array.
 *
 * \param[in] blake2b_hash A pointer to the `cardano_blake2b_hash_t` object to be exported.
 * \param[out] hash A pointer to a byte array where the hash bytes will be stored. This array
 *             must be pre-allocated by the caller and be large enough to hold the entire
 *             hash value.
 * \param[in] hash_length The length of the `hash` array in bytes. This must be equal to or
 *                 larger than the size of the hash object to avoid buffer overflow.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS and the `hash` array
 *         contains the byte representation of the hash object. On failure, a non-zero
 *         error code is returned. Possible failure reasons include invalid input parameters
 *         or a mismatch between `hash_length` and the actual size of the hash object.
 *
 * Example Usage:
 * \code
 * cardano_blake2b_hash_t* hash_obj = ...; // Assume hash_obj is previously created and valid
 * const size_t hash_size = cardano_blake2b_hash_get_bytes_size(hash_obj);
 * byte_t* hash_bytes = malloc(hash_size); // Allocate enough space for the hash
 *
 * if (hash_bytes != NULL)
 * {
 *   cardano_error_t error = cardano_blake2b_hash_to_bytes(hash_obj, hash_bytes, hash_size);
 *
 *   if (error == CARDANO_SUCCESS)
 *   {
 *     // The hash_bytes array now contains the hash object's bytes
 *   }
 *
 *   // Remember to free the allocated memory when done
 *   free(hash_bytes);
 * }
 * \endcode
 *
 * Note: It is the caller's responsibility to ensure that the `hash` array is correctly
 *       allocated and to free any allocated resources when no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_bytes(
  const cardano_blake2b_hash_t* blake2b_hash,
  byte_t*                       hash,
  size_t                        hash_length);

/**
 * \brief Retrieves the size of the hexadecimal representation of a BLAKE2b hash object.
 *
 * This function calculates the length of the string that would represent the BLAKE2b
 * hash object in hexadecimal form. This is useful for allocating the correct amount of
 * memory to store the hexadecimal representation of the hash.
 *
 * \param[in] blake2b_hash A pointer to the `cardano_blake2b_hash_t` object whose hexadecimal
 *                     size is to be retrieved.
 *
 * \return The size of the buffer needed to store the hexadecimal representation of the hash
 *         object, including the terminating null byte. If the provided hash object is invalid
 *         or NULL, the function will return 0.
 *
 * Example Usage:
 * \code
 * cardano_blake2b_hash_t* hash_obj = ...; // Assume hash_obj is previously created
 * size_t hex_size = cardano_blake2b_hash_get_hex_size(hash_obj);
 * char* hex_buffer = (char*)malloc(hex_size);
 *
 * if (cardano_blake2b_hash_to_hex(hash_obj, hex_buffer, hex_size) == CARDANO_SUCCESS)
 * {
 *   // hex_buffer now contains the hexadecimal representation of the hash
 * }
 *
 * free(hex_buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_blake2b_hash_get_hex_size(const cardano_blake2b_hash_t* blake2b_hash);

/**
 * \brief Converts a BLAKE2b hash object into its hexadecimal string representation.
 *
 * This function exports the hexadecimal string representation of the provided
 * BLAKE2b hash object into a user-supplied buffer.
 *
 * \param[in] blake2b_hash A pointer to the `cardano_blake2b_hash_t` object to be converted.
 * \param[out] hex_hash A pointer to the buffer where the hexadecimal string representation
 *                 of the hash will be stored. The buffer must be large enough to hold
 *                 the hexadecimal representation of the hash, including the terminating
 *                 null byte.
 * \param[in] hash_length The size of the buffer pointed to by `hex_hash`. This value should be
 *                 at least twice the size of the hash object in bytes plus one for the
 *                 terminating null byte, to accommodate the hexadecimal representation.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 *         On success, the function returns \ref CARDANO_SUCCESS, and the buffer pointed to
 *         by `hex_hash` is filled with the hexadecimal representation of the hash object.
 *         On failure, a non-zero error code is returned, which could indicate an invalid
 *         hash object, insufficient buffer size, or other errors.
 *
 * Example Usage:
 * \code
 * cardano_blake2b_hash_t* hash_obj = ...; // Assume hash_obj is previously created
 * size_t hex_size = cardano_blake2b_hash_get_hex_size(hash_obj);
 * byte_t* hex_buffer = (byte_t*)malloc(hex_size);
 *
 * if (cardano_blake2b_hash_to_hex(hash_obj, hex_buffer, hex_size) == CARDANO_SUCCESS)
 * {
 *  // hex_buffer now contains the hexadecimal representation of the hash
 * }
 *
 * free(hex_buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_blake2b_hash_to_hex(
  const cardano_blake2b_hash_t* blake2b_hash,
  char*                         hex_hash,
  size_t                        hash_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BLAKE2B_HASH_H
