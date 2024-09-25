/**
 * \file pool_metadata.h
 *
 * \author angel.castillo
 * \date   Jun 26, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_POOL_METADATA_H
#define BIGLUP_LABS_INCLUDE_CARDANO_POOL_METADATA_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The pool registration certificate can include a way to locate pool metadata. This includes the hash of
 * the metadata. This is not the metadata itself but a unique identifier that corresponds to the metadata.
 * The hash function ensures that even a small change in the metadata leads to a completely different hash,
 * securing the authenticity of the data.
 *
 * Along with the hash of the metadata, the URL where the actual metadata file (in JSON format)
 * is hosted is also included in the certificate. The combination of the URL and the hash allows wallets
 * and other services to download the metadata file and verify it against the hash.
 */
typedef struct cardano_pool_metadata_t cardano_pool_metadata_t;

/**
 * \brief Creates and initializes a new pool metadata object.
 *
 * This function allocates and initializes a new instance of \ref cardano_pool_metadata_t.
 * The pool metadata includes information such as a URL to the pool's metadata file and its associated hash.
 *
 * \param[in] url A constant pointer to a character array containing the URL where the pool metadata is hosted.
 * \param[in] url_length The length of the URL character array.
 * \param[in] hash A pointer to a \ref cardano_blake2b_hash_t object representing the BLAKE2b hash of the metadata file.
 *                 This hash is used to verify the integrity of the metadata once fetched.
 * \param[out] pool_metadata On successful initialization, this will point to the newly created
 *             \ref cardano_pool_metadata_t object. The object represents a "strong reference"
 *             to the pool metadata, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the pool metadata is no longer needed, the caller must
 *             release it by calling \ref cardano_pool_metadata_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool metadata was successfully created, or an appropriate error code
 *         indicating the failure reason, such as invalid URL format, null pointers, or out of memory errors.
 *
 * Usage Example:
 * \code{.c}
 * const char* url = "https://example.com/poolmetadata.json";
 * size_t url_length = strlen(url);
 * cardano_blake2b_hash_t* hash = ...;  // Assume hash is initialized here
 * cardano_pool_metadata_t* pool_metadata = NULL;
 *
 * cardano_error_t result = cardano_pool_metadata_new(url, url_length, hash, &pool_metadata);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool metadata
 *
 *   // Once done, ensure to clean up and release the pool metadata
 *   cardano_pool_metadata_unref(&pool_metadata);
 * }
 * else
 * {
 *   printf("Failed to create pool metadata: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_metadata_new(
  const char*               url,
  size_t                    url_length,
  cardano_blake2b_hash_t*   hash,
  cardano_pool_metadata_t** pool_metadata);

/**
 * \brief Initializes pool metadata from a URL and a hexadecimal hash string.
 *
 * This function creates a pool metadata object based on the provided URL and a BLAKE2b hash of the metadata
 * content, given as a hexadecimal string. This approach is useful when the hash of the metadata is known in
 * a string format, allowing for easy verification of metadata integrity after retrieval.
 *
 * \param[in] url A pointer to a character array containing the URL where the pool metadata is hosted.
 * \param[in] url_size The length of the URL character array.
 * \param[in] hash A pointer to a character array containing the hexadecimal representation of the BLAKE2b hash
 *                 of the pool metadata. This hash is used to ensure the integrity of the downloaded metadata.
 * \param[in] hash_size The length of the hash character array.
 * \param[out] pool_metadata On successful initialization, this will point to the newly created
 *             \ref cardano_pool_metadata_t object. This object represents a "strong reference"
 *             to the pool metadata, fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically releasing it by calling \ref cardano_pool_metadata_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool metadata was successfully created, or an appropriate error code
 *         indicating the failure reason, such as invalid URL or hash format, null pointers, or memory allocation issues.
 *
 * Usage Example:
 * \code{.c}
 * const char* url = "https://example.com/poolmetadata.json";
 * size_t url_size = strlen(url);
 * const char* hash_hex = "abc123...";  // Example BLAKE2b hash in hexadecimal
 * size_t hash_size = strlen(hash_hex);
 * cardano_pool_metadata_t* pool_metadata = NULL;
 *
 * cardano_error_t result = cardano_pool_metadata_from_hash_hex(url, url_size, hash_hex, hash_size, &pool_metadata);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool metadata
 *
 *   // Once done, ensure to clean up and release the pool metadata
 *   cardano_pool_metadata_unref(&pool_metadata);
 * }
 * else
 * {
 *   printf("Failed to create pool metadata: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_metadata_from_hash_hex(
  const char*               url,
  size_t                    url_size,
  const char*               hash,
  size_t                    hash_size,
  cardano_pool_metadata_t** pool_metadata);

/**
 * \brief Creates a \ref cardano_pool_metadata_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_pool_metadata_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a pool_metadata.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] pool_metadata A pointer to a pointer of \ref cardano_pool_metadata_t that will be set to the address
 *                        of the newly created pool_metadata object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_pool_metadata_t object by calling
 *       \ref cardano_pool_metadata_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_pool_metadata_t* pool_metadata = NULL;
 *
 * cardano_error_t result = cardano_pool_metadata_from_cbor(reader, &pool_metadata);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_metadata
 *
 *   // Once done, ensure to clean up and release the pool_metadata
 *   cardano_pool_metadata_unref(&pool_metadata);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode pool_metadata: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_metadata_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_metadata_t** pool_metadata);

/**
 * \brief Serializes a pool_metadata into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_pool_metadata_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] pool_metadata A constant pointer to the \ref cardano_pool_metadata_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p pool_metadata or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_metadata_t* pool_metadata = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_pool_metadata_to_cbor(pool_metadata, writer);
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
 * cardano_pool_metadata_unref(&pool_metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_metadata_to_cbor(
  const cardano_pool_metadata_t* pool_metadata,
  cardano_cbor_writer_t*         writer);

/**
 * \brief Retrieves the size of the URL string stored in the pool metadata object.
 *
 * This function returns the length of the URL string contained within a \ref cardano_pool_metadata_t object.
 * This length does not include the null terminator of the string. This function is useful for determining
 * the buffer size needed to store or manipulate the URL string outside of the pool metadata object.
 *
 * \param[in] pool_metadata A constant pointer to an initialized \ref cardano_pool_metadata_t object.
 *
 * \return The size of the URL string in bytes, excluding the null terminator. If the input pointer
 *         is NULL, returns 0 to indicate an error or uninitialized pool metadata object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_metadata_t* pool_metadata = ...; // Assume pool_metadata is already initialized
 * size_t url_size = cardano_pool_metadata_get_url_size(pool_metadata);
 * printf("URL size: %zu bytes\n", url_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_metadata_get_url_size(
  const cardano_pool_metadata_t* pool_metadata);

/**
 * \brief Retrieves the URL string from a pool metadata object.
 *
 * This function provides access to the URL string stored within a \ref cardano_pool_metadata_t object.
 * The URL is associated with the metadata of a Cardano stake pool and is used for various network operations
 * and identity verifications. The returned string is null-terminated.
 *
 * \param[in] pool_metadata A constant pointer to an initialized \ref cardano_pool_metadata_t object.
 *
 * \return A constant pointer to a null-terminated string representing the URL of the pool metadata.
 *         If the input pointer is NULL, or if the pool metadata object is uninitialized, returns NULL.
 *         The caller should not attempt to modify or free the returned string, as it points to internal memory
 *         managed by the \ref cardano_pool_metadata_t object. The memory for the string will be freed when
 *         the \ref cardano_pool_metadata_t object is destroyed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_metadata_t* pool_metadata = ...; // Assume pool_metadata is already initialized
 * const char* url = cardano_pool_metadata_get_url(pool_metadata);
 *
 * if (url != NULL)
 * {
 *   printf("Pool URL: %s\n", url);
 * }
 * else
 * {
 *   printf("Failed to retrieve URL from pool metadata.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pool_metadata_get_url(
  const cardano_pool_metadata_t* pool_metadata);

/**
 * \brief Sets the URL string in a pool metadata object.
 *
 * This function updates the URL string stored within a \ref cardano_pool_metadata_t object. The URL is associated with
 * the metadata of a Cardano stake pool and is used for various network operations and identity verifications. This function
 * copies the URL from the provided string to the pool metadata object.
 *
 * \param[in] url A pointer to a character array containing the new URL to be set in the pool metadata.
 * \param[in] url_size The size of the URL character array, including the null terminator.
 * \param[in,out] pool_metadata A pointer to an initialized \ref cardano_pool_metadata_t object which will be updated.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the URL was
 *         successfully updated, or an appropriate error code indicating the failure reason, such as invalid parameters
 *         or out of memory errors.
 *
 * Usage Example:
 * \code{.c}
 * const char* new_url = "https://newpoolmetadata.com";
 * cardano_pool_metadata_t* pool_metadata = ...; // Assume pool_metadata is already initialized
 * cardano_error_t result = cardano_pool_metadata_set_url(new_url, strlen(new_url) + 1, pool_metadata);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool URL updated successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to update pool URL.\n");
 * }
 * \endcode
 *
 * \note This function does not take ownership of the `url` parameter but copies its content. The caller is responsible
 *       for managing the memory of `url`. Ensure that the `pool_metadata` object is not NULL and properly initialized
 *       before calling this function.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_metadata_set_url(
  const char*              url,
  size_t                   url_size,
  cardano_pool_metadata_t* pool_metadata);

/**
 * \brief Retrieves the Blake2b hash from a pool metadata object.
 *
 * This function provides access to the Blake2b hash stored within a
 * \ref cardano_pool_metadata_t object.
 *
 * \param[in] pool_metadata A pointer to an initialized \ref cardano_pool_metadata_t object.
 * \param[out] hash On successful retrieval, this will point to a newly created
 *                  \ref cardano_blake2b_hash_t object containing the hash. The caller is responsible
 *                  for managing the lifecycle of this object, and must release it by calling
 *                  \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hash was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_metadata_t* pool_metadata = ...; // Assume pool_metadata is already initialized
 * cardano_blake2b_hash_t* hash = NULL;
 * cardano_error_t result = cardano_pool_metadata_get_hash(pool_metadata, &hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the hash
 *
 *   // Once done, ensure to clean up and release the hash
 *   cardano_blake2b_hash_unref(&hash);
 * }
 * else
 * {
 *   printf("Failed to retrieve hash from pool metadata.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_metadata_get_hash(
  cardano_pool_metadata_t* pool_metadata,
  cardano_blake2b_hash_t** hash);

/**
 * \brief Sets the Blake2b hash for a pool metadata object.
 *
 * This function sets the Blake2b hash within a \ref cardano_pool_metadata_t object.
 *
 * \param[in] pool_metadata A pointer to an initialized \ref cardano_pool_metadata_t object.
 * \param[in] hash A pointer to an initialized \ref cardano_blake2b_hash_t object containing
 *                 the hash to be set. The caller is responsible for managing the lifecycle
 *                 of the hash object, and must ensure that it remains valid for the duration
 *                 of the operation. The function does not take ownership of the hash object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hash was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_metadata_t* pool_metadata = ...; // Assume pool_metadata is already initialized
 * cardano_blake2b_hash_t* hash = ...; // Assume hash is already initialized
 * cardano_error_t result = cardano_pool_metadata_set_hash(pool_metadata, hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Hash was successfully set
 * }
 * else
 * {
 *   printf("Failed to set hash for pool metadata.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_metadata_set_hash(
  cardano_pool_metadata_t* pool_metadata,
  cardano_blake2b_hash_t*  hash);

/**
 * \brief Decrements the reference count of a cardano_pool_metadata_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_pool_metadata_t object
 * by decreasing its reference count. When the reference count reaches zero, the pool_metadata is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] pool_metadata A pointer to the pointer of the pool_metadata object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_metadata_t* pool_metadata = cardano_pool_metadata_new(major, minor);
 *
 * // Perform operations with the pool_metadata...
 *
 * cardano_pool_metadata_unref(&pool_metadata);
 * // At this point, pool_metadata is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_pool_metadata_unref, the pointer to the \ref cardano_pool_metadata_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_pool_metadata_unref(cardano_pool_metadata_t** pool_metadata);

/**
 * \brief Increases the reference count of the cardano_pool_metadata_t object.
 *
 * This function is used to manually increment the reference count of an cardano_pool_metadata_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_pool_metadata_unref.
 *
 * \param pool_metadata A pointer to the cardano_pool_metadata_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_metadata is a previously created pool_metadata object
 *
 * cardano_pool_metadata_ref(pool_metadata);
 *
 * // Now pool_metadata can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_pool_metadata_ref there is a corresponding
 * call to \ref cardano_pool_metadata_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_pool_metadata_ref(cardano_pool_metadata_t* pool_metadata);

/**
 * \brief Retrieves the current reference count of the cardano_pool_metadata_t object.
 *
 * This function returns the number of active references to an cardano_pool_metadata_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_pool_metadata_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param pool_metadata A pointer to the cardano_pool_metadata_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_pool_metadata_t object. If the object
 * is properly managed (i.e., every \ref cardano_pool_metadata_ref call is matched with a
 * \ref cardano_pool_metadata_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_metadata is a previously created pool_metadata object
 *
 * size_t ref_count = cardano_pool_metadata_refcount(pool_metadata);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_metadata_refcount(const cardano_pool_metadata_t* pool_metadata);

/**
 * \brief Sets the last error message for a given cardano_pool_metadata_t object.
 *
 * Records an error message in the pool_metadata's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] pool_metadata A pointer to the \ref cardano_pool_metadata_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the pool_metadata's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_pool_metadata_set_last_error(
  cardano_pool_metadata_t* pool_metadata,
  const char*              message);

/**
 * \brief Retrieves the last error message recorded for a specific pool_metadata.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_pool_metadata_set_last_error for the given
 * pool_metadata. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] pool_metadata A pointer to the \ref cardano_pool_metadata_t instance whose last error
 *                   message is to be retrieved. If the pool_metadata is NULL, the function
 *                   returns a generic error message indicating the null pool_metadata.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified pool_metadata. If the pool_metadata is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_pool_metadata_set_last_error for the same pool_metadata, or until
 *       the pool_metadata is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pool_metadata_get_last_error(
  const cardano_pool_metadata_t* pool_metadata);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_POOL_METADATA_H