/**
 * \file pool_retirement_cert.h
 *
 * \author angel.castillo
 * \date   Jul 23, 2024
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

#ifndef POOL_RETIREMENT_CERT_H
#define POOL_RETIREMENT_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This certificate is used to retire a stake pool. It includes an epoch number indicating when the pool will be retired.
 */
typedef struct cardano_pool_retirement_cert_t cardano_pool_retirement_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new pool retirement certificate.
 *
 * This function allocates and initializes a new \ref cardano_pool_retirement_cert_t object, which signifies
 * the intent to retire a stake pool at a specified epoch. The retirement certificate includes the stake pool's
 * operator key hash and the epoch at which the pool is scheduled to retire.
 *
 * \param[in] pool_key_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the hash of the pool's operator key.
 * \param[in] epoch The epoch number at which the pool is scheduled to retire.
 * \param[out] pool_retirement_cert On successful execution, this will point to the newly created \ref cardano_pool_retirement_cert_t object.
 *                                           The caller is responsible for releasing this resource using \ref cardano_pool_retirement_cert_unref when it
 *                                           is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the pool retirement certificate was
 *         successfully created, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* pool_key_hash = ...;  // Assume pool_key_hash is already initialized
 * uint64_t epoch = 250;  // Specify the epoch for retirement
 * cardano_pool_retirement_cert_t* retirement_cert = NULL;
 *
 * cardano_error_t result = cardano_pool_retirement_cert_new(pool_key_hash, epoch, &retirement_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The retirement certificate can now be used for scheduling pool retirement
 *   // Remember to free the retirement certificate when done
 *   cardano_pool_retirement_cert_unref(&retirement_cert);
 * }
 * else
 * {
 *   printf("Failed to create pool retirement certificate: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_retirement_cert_new(
  cardano_blake2b_hash_t*          pool_key_hash,
  uint64_t                         epoch,
  cardano_pool_retirement_cert_t** pool_retirement_cert);

/**
 * \brief Creates a \ref cardano_pool_retirement_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_pool_retirement_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a pool_retirement_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] pool_retirement_cert A pointer to a pointer of \ref cardano_pool_retirement_cert_t that will be set to the address
 *                        of the newly created pool_retirement_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_pool_retirement_cert_t object by calling
 *       \ref cardano_pool_retirement_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;
 *
 * cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_retirement_cert
 *
 *   // Once done, ensure to clean up and release the pool_retirement_cert
 *   cardano_pool_retirement_cert_unref(&pool_retirement_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode pool_retirement_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_retirement_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_retirement_cert_t** pool_retirement_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_pool_retirement_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] pool_retirement_cert A constant pointer to the \ref cardano_pool_retirement_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p pool_retirement_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_retirement_cert_t* pool_retirement_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_pool_retirement_cert_to_cbor(pool_retirement_cert, writer);
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
 * cardano_pool_retirement_cert_unref(&pool_retirement_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_retirement_cert_to_cbor(
  const cardano_pool_retirement_cert_t* pool_retirement_cert,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Retrieves the pool key hash from a pool retirement certificate.
 *
 * This function extracts the pool key hash from the specified \ref cardano_pool_retirement_cert_t object.
 * The pool key hash is the unique identifier for the stake pool.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_pool_retirement_cert_t object.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object containing the pool key hash. If the certificate
 *         is NULL or if the pool key hash is not set, this function returns NULL. This function returns a reference
 *         to the internal hash object, which should not be modified or freed by the caller. Use \ref cardano_blake2b_hash_unref
 *         to safely reduce the reference count when the hash is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_pool_retirement_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* pool_key_hash = cardano_pool_retirement_cert_get_pool_key_hash(certificate);
 *
 * if (pool_key_hash != NULL)
 * {
 *   // Process the pool key hash
 *   // No need to unref this specific reference since it is managed internally
 * }
 * else
 * {
 *   printf("Failed to retrieve the pool key hash or certificate is not valid.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_pool_retirement_cert_get_pool_key_hash(cardano_pool_retirement_cert_t* certificate);

/**
 * \brief Sets the pool key hash for a pool retirement certificate.
 *
 * This function assigns a new pool key hash to a given \ref cardano_pool_retirement_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_pool_retirement_cert_t object to which the key hash will be set.
 * \param[in] hash A pointer to a constant \ref cardano_blake2b_hash_t object representing the new pool key hash.
 *                 This function increments the reference count of the hash object; it is the caller's responsibility to manage
 *                 the lifecycle of the hash and decrement the reference count when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool key hash was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_retirement_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* new_pool_key_hash = ...; // Assume new_pool_key_hash is already initialized
 *
 * cardano_error_t result = cardano_pool_retirement_cert_set_pool_key_hash(certificate, new_pool_key_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool key hash set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the pool key hash.\n");
 * }
 *
 * // Remember to manage the reference count of new_pool_key_hash appropriately
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_retirement_cert_set_pool_key_hash(cardano_pool_retirement_cert_t* certificate, cardano_blake2b_hash_t* hash);

/**
 * \brief Retrieves the retirement epoch for a pool retirement certificate.
 *
 * This function extracts the epoch number from a given \ref cardano_pool_retirement_cert_t object.
 * The epoch number specifies when the retirement of the stake pool will take effect on the Cardano blockchain.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_pool_retirement_cert_t object.
 *
 * \return The epoch number as a uint64_t at which the stake pool is scheduled to retire. If the certificate
 *         is NULL or does not have an epoch set, this function returns 0 as a default value, which should be
 *         handled appropriately by the caller.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_pool_retirement_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t retirement_epoch = cardano_pool_retirement_cert_get_epoch(certificate);
 *
 * printf("Stake pool scheduled to retire at epoch: %lu\n", retirement_epoch);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_pool_retirement_cert_get_epoch(const cardano_pool_retirement_cert_t* certificate);

/**
 * \brief Sets the retirement epoch for a pool retirement certificate.
 *
 * This function assigns a new retirement epoch to a given \ref cardano_pool_retirement_cert_t object.
 * The epoch number specifies when the retirement of the stake pool will take effect on the Cardano blockchain.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_pool_retirement_cert_t object where the retirement epoch will be set.
 * \param[in] epoch The epoch number at which the stake pool is scheduled to retire.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the epoch number was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_retirement_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t retirement_epoch = 250; // Example epoch number for retirement
 *
 * cardano_error_t result = cardano_pool_retirement_cert_set_epoch(certificate, retirement_epoch);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Retirement epoch set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the retirement epoch.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_retirement_cert_set_epoch(cardano_pool_retirement_cert_t* certificate, uint64_t epoch);

/**
 * \brief Decrements the reference count of a cardano_pool_retirement_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_pool_retirement_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the pool_retirement_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] pool_retirement_cert A pointer to the pointer of the pool_retirement_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_retirement_cert_t* pool_retirement_cert = cardano_pool_retirement_cert_new(major, minor);
 *
 * // Perform operations with the pool_retirement_cert...
 *
 * cardano_pool_retirement_cert_unref(&pool_retirement_cert);
 * // At this point, pool_retirement_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_pool_retirement_cert_unref, the pointer to the \ref cardano_pool_retirement_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_pool_retirement_cert_unref(cardano_pool_retirement_cert_t** pool_retirement_cert);

/**
 * \brief Increases the reference count of the cardano_pool_retirement_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_pool_retirement_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_pool_retirement_cert_unref.
 *
 * \param pool_retirement_cert A pointer to the cardano_pool_retirement_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_retirement_cert is a previously created pool_retirement_cert object
 *
 * cardano_pool_retirement_cert_ref(pool_retirement_cert);
 *
 * // Now pool_retirement_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_pool_retirement_cert_ref there is a corresponding
 * call to \ref cardano_pool_retirement_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_pool_retirement_cert_ref(cardano_pool_retirement_cert_t* pool_retirement_cert);

/**
 * \brief Retrieves the current reference count of the cardano_pool_retirement_cert_t object.
 *
 * This function returns the number of active references to an cardano_pool_retirement_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_pool_retirement_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param pool_retirement_cert A pointer to the cardano_pool_retirement_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_pool_retirement_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_pool_retirement_cert_ref call is matched with a
 * \ref cardano_pool_retirement_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_retirement_cert is a previously created pool_retirement_cert object
 *
 * size_t ref_count = cardano_pool_retirement_cert_refcount(pool_retirement_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_retirement_cert_refcount(const cardano_pool_retirement_cert_t* pool_retirement_cert);

/**
 * \brief Sets the last error message for a given cardano_pool_retirement_cert_t object.
 *
 * Records an error message in the pool_retirement_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] pool_retirement_cert A pointer to the \ref cardano_pool_retirement_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the pool_retirement_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_pool_retirement_cert_set_last_error(
  cardano_pool_retirement_cert_t* pool_retirement_cert,
  const char*                     message);

/**
 * \brief Retrieves the last error message recorded for a specific pool_retirement_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_pool_retirement_cert_set_last_error for the given
 * pool_retirement_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] pool_retirement_cert A pointer to the \ref cardano_pool_retirement_cert_t instance whose last error
 *                   message is to be retrieved. If the pool_retirement_cert is NULL, the function
 *                   returns a generic error message indicating the null pool_retirement_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified pool_retirement_cert. If the pool_retirement_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_pool_retirement_cert_set_last_error for the same pool_retirement_cert, or until
 *       the pool_retirement_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pool_retirement_cert_get_last_error(
  const cardano_pool_retirement_cert_t* pool_retirement_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // POOL_RETIREMENT_CERT_H