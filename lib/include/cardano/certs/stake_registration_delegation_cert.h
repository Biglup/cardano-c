/**
 * \file stake_registration_delegation_cert.h
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

#ifndef STAKE_REGISTRATION_DELEGATION_CERT_H
#define STAKE_REGISTRATION_DELEGATION_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This certificate Register the stake key and delegate with a single certificate to a stake pool.
 */
typedef struct cardano_stake_registration_delegation_cert_t cardano_stake_registration_delegation_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new stake registration and delegation certificate.
 *
 * This function allocates and initializes a \ref cardano_stake_registration_delegation_cert_t object, which represents
 * a certificate that both registers a new staking credential and delegates it to a specified staking pool.
 *
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the stake credential to be registered.
 * \param[in] pool_key_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the pool's key hash to which the stake will be delegated.
 * \param[in] deposit The amount of ADA deposited for registering the stake, required as per network protocol rules.
 * \param[out] stake_registration_delegation On successful initialization, this will point to a newly created \ref cardano_stake_registration_delegation_cert_t object.
 *                                            The caller is responsible for managing the lifecycle of this object, including releasing it when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was successfully created, or an error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 * cardano_blake2b_hash_t* pool_key_hash = ...; // Assume pool_key_hash is initialized
 * uint64_t deposit = 2000000; // Example deposit amount
 *
 * cardano_stake_registration_delegation_cert_t* certificate = NULL;
 * cardano_error_t result = cardano_stake_registration_delegation_cert_new(credential, pool_key_hash, deposit, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The certificate is now ready to use
 *   // Once done, ensure to clean up and release the certificate
 *   cardano_stake_registration_delegation_cert_unref(&certificate);
 * }
 * else
 * {
 *   printf("Failed to create stake registration and delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_stake_registration_delegation_cert_new(
  cardano_credential_t*                          credential,
  cardano_blake2b_hash_t*                        pool_key_hash,
  uint64_t                                       deposit,
  cardano_stake_registration_delegation_cert_t** stake_registration_delegation);

/**
 * \brief Creates a \ref cardano_stake_registration_delegation_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_stake_registration_delegation_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a stake_registration.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] stake_registration A pointer to a pointer of \ref cardano_stake_registration_delegation_cert_t that will be set to the address
 *                        of the newly created stake_registration object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_stake_registration_delegation_cert_t object by calling
 *       \ref cardano_stake_registration_delegation_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_stake_registration_delegation_cert_t* stake_registration = NULL;
 *
 * cardano_error_t result = cardano_stake_registration_delegation_cert_from_cbor(reader, &stake_registration);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the stake_registration
 *
 *   // Once done, ensure to clean up and release the stake_registration
 *   cardano_stake_registration_delegation_cert_unref(&stake_registration);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode stake_registration: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_stake_registration_delegation_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_stake_registration_delegation_cert_t** stake_registration);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_stake_registration_delegation_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] stake_registration A constant pointer to the \ref cardano_stake_registration_delegation_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p stake_registration or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_registration_delegation_cert_t* stake_registration = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_stake_registration_delegation_cert_to_cbor(stake_registration, writer);
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
 * cardano_stake_registration_delegation_cert_unref(&stake_registration);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_stake_registration_delegation_cert_to_cbor(
  const cardano_stake_registration_delegation_cert_t* stake_registration,
  cardano_cbor_writer_t*                              writer);

/**
 * \brief Retrieves the stake credential from a stake registration and delegation certificate.
 *
 * This function extracts the stake credential from a given \ref cardano_stake_registration_delegation_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_stake_registration_delegation_cert_t object.
 *
 * \return A pointer to the \ref cardano_credential_t object containing the stake credential. If the certificate is NULL,
 *         this function returns NULL. The returned credential is reference counted and must be unreferenced by the caller
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_stake_registration_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_credential_t* credential = cardano_stake_registration_delegation_cert_get_credential(certificate);
 *
 * if (credential != NULL)
 * {
 *   // Process the credential
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   printf("Failed to retrieve the credential.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_stake_registration_delegation_cert_get_credential(cardano_stake_registration_delegation_cert_t* certificate);

/**
 * \brief Sets the stake credential for a stake registration and delegation certificate.
 *
 * This function assigns a stake credential to a given \ref cardano_stake_registration_delegation_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_stake_registration_delegation_cert_t object which will
 *                        receive the new stake credential.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object containing the stake credential to be
 *                       set. This function increases the reference count of the credential, so it remains managed and must
 *                       be properly unreferenced after it is no longer needed by the caller.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the stake credential
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_registration_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_stake_registration_delegation_cert_set_credential(certificate, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Stake credential set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the stake credential.\n");
 * }
 *
 * // Both objects should be unreferenced when no longer needed
 * cardano_credential_unref(&credential);
 * cardano_stake_registration_delegation_cert_unref(&certificate);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_stake_registration_delegation_cert_set_credential(cardano_stake_registration_delegation_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Retrieves the pool key hash from a stake registration and delegation certificate.
 *
 * This function extracts the pool key hash from the specified \ref cardano_stake_registration_delegation_cert_t object.
 * The pool key hash identifies the staking pool to which the stake rights are being delegated.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_stake_registration_delegation_cert_t object.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object containing the pool key hash. If the certificate is NULL,
 *         or if the pool key hash has not been set, this function returns NULL. The returned hash is a new reference and
 *         must be managed by the caller. It must be unreferenced when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_stake_registration_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* pool_key_hash = cardano_stake_registration_delegation_cert_get_pool_key_hash(certificate);
 *
 * if (pool_key_hash != NULL)
 * {
 *   // Process the pool key hash
 *   cardano_blake2b_hash_unref(&pool_key_hash);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_stake_registration_delegation_cert_get_pool_key_hash(cardano_stake_registration_delegation_cert_t* certificate);

/**
 * \brief Sets the pool key hash for a stake registration and delegation certificate.
 *
 * This function assigns a pool key hash to the specified \ref cardano_stake_registration_delegation_cert_t object.
 * The pool key hash identifies the staking pool to which the stake rights are being delegated.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_stake_registration_delegation_cert_t object to which the pool key hash will be set.
 * \param[in] hash A constant pointer to an initialized \ref cardano_blake2b_hash_t object representing the pool key hash.
 *                  The pool parameters object will take a reference to this hash, so it must remain valid as long as it is in use by the certificate.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the pool key hash was successfully set,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_registration_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_blake2b_hash_t* hash = ...; // Assume hash is already initialized
 *
 * cardano_error_t result = cardano_stake_registration_delegation_cert_set_pool_key_hash(certificate, hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The pool key hash has been set successfully
 * }
 * else
 * {
 *   printf("Failed to set the pool key hash.\n");
 * }
 * \endcode
 *
 * \note This function increments the reference count of the hash passed to it, as the certificate will maintain a reference to it.
 *       The caller must manage their own reference to the hash and release it when it is no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_stake_registration_delegation_cert_set_pool_key_hash(cardano_stake_registration_delegation_cert_t* certificate, cardano_blake2b_hash_t* hash);

/**
 * \brief Retrieves the deposit amount from a stake registration and delegation certificate.
 *
 * This function extracts the deposit amount from the specified \ref cardano_stake_registration_delegation_cert_t object.
 * The deposit is required when registering stake rights and delegating them to a staking pool, as specified by the protocol parameters.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_stake_registration_delegation_cert_t object.
 *
 * \return The deposit amount in lovelaces. If the certificate is NULL, returns 0.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_stake_registration_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = cardano_stake_registration_delegation_cert_get_deposit(certificate);
 * printf("Deposit amount: %llu lovelaces\n", deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_stake_registration_delegation_cert_get_deposit(const cardano_stake_registration_delegation_cert_t* certificate);

/**
 * \brief Sets the deposit amount for a stake registration and delegation certificate.
 *
 * This function assigns a new deposit amount to a given \ref cardano_stake_registration_delegation_cert_t object.
 * The deposit is a required amount of ADA that must be locked when registering stake rights and delegating them to a staking pool,
 * according to the protocol parameters.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_stake_registration_delegation_cert_t object to which the deposit will be set.
 * \param[in] deposit The deposit amount in lovelaces to be set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the deposit amount was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input certificate pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_registration_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = 2000000; // Deposit of 2 ADA
 *
 * cardano_error_t result = cardano_stake_registration_delegation_cert_set_deposit(certificate, deposit);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Deposit amount set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the deposit amount.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_stake_registration_delegation_cert_set_deposit(cardano_stake_registration_delegation_cert_t* certificate, uint64_t deposit);

/**
 * \brief Decrements the reference count of a cardano_stake_registration_delegation_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_stake_registration_delegation_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the stake_registration is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] stake_registration A pointer to the pointer of the stake_registration object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_stake_registration_delegation_cert_t* stake_registration = cardano_stake_registration_delegation_cert_new(major, minor);
 *
 * // Perform operations with the stake_registration...
 *
 * cardano_stake_registration_delegation_cert_unref(&stake_registration);
 * // At this point, stake_registration is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_stake_registration_delegation_cert_unref, the pointer to the \ref cardano_stake_registration_delegation_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_stake_registration_delegation_cert_unref(cardano_stake_registration_delegation_cert_t** stake_registration);

/**
 * \brief Increases the reference count of the cardano_stake_registration_delegation_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_stake_registration_delegation_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_stake_registration_delegation_cert_unref.
 *
 * \param stake_registration A pointer to the cardano_stake_registration_delegation_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming stake_registration is a previously created stake_registration object
 *
 * cardano_stake_registration_delegation_cert_ref(stake_registration);
 *
 * // Now stake_registration can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_stake_registration_delegation_cert_ref there is a corresponding
 * call to \ref cardano_stake_registration_delegation_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_stake_registration_delegation_cert_ref(cardano_stake_registration_delegation_cert_t* stake_registration);

/**
 * \brief Retrieves the current reference count of the cardano_stake_registration_delegation_cert_t object.
 *
 * This function returns the number of active references to an cardano_stake_registration_delegation_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_stake_registration_delegation_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param stake_registration A pointer to the cardano_stake_registration_delegation_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_stake_registration_delegation_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_stake_registration_delegation_cert_ref call is matched with a
 * \ref cardano_stake_registration_delegation_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming stake_registration is a previously created stake_registration object
 *
 * size_t ref_count = cardano_stake_registration_delegation_cert_refcount(stake_registration);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_stake_registration_delegation_cert_refcount(const cardano_stake_registration_delegation_cert_t* stake_registration);

/**
 * \brief Sets the last error message for a given cardano_stake_registration_delegation_cert_t object.
 *
 * Records an error message in the stake_registration's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] stake_registration A pointer to the \ref cardano_stake_registration_delegation_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the stake_registration's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_stake_registration_delegation_cert_set_last_error(
  cardano_stake_registration_delegation_cert_t* stake_registration,
  const char*                                   message);

/**
 * \brief Retrieves the last error message recorded for a specific stake_registration.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_stake_registration_delegation_cert_set_last_error for the given
 * stake_registration. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] stake_registration A pointer to the \ref cardano_stake_registration_delegation_cert_t instance whose last error
 *                   message is to be retrieved. If the stake_registration is NULL, the function
 *                   returns a generic error message indicating the null stake_registration.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified stake_registration. If the stake_registration is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_stake_registration_delegation_cert_set_last_error for the same stake_registration, or until
 *       the stake_registration is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_stake_registration_delegation_cert_get_last_error(
  const cardano_stake_registration_delegation_cert_t* stake_registration);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // STAKE_REGISTRATION_DELEGATION_CERT_H