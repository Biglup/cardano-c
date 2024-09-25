/**
 * \file genesis_key_delegation_cert.h
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

#ifndef genesis_key_delegation_cert_H
#define genesis_key_delegation_cert_H

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
 * \brief This certificate is used to delegate from a Genesis key to a set of keys. This was primarily used in the early
 * phases of the Cardano network's existence during the transition from the Byron to the Shelley era.
 */
typedef struct cardano_genesis_key_delegation_cert_t cardano_genesis_key_delegation_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new Genesis Key Delegation certificate.
 *
 * This function allocates and initializes a new instance of \ref cardano_genesis_key_delegation_cert_t.
 *
 * \param[in] genesis_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the hash of the genesis block.
 *                         This hash uniquely identifies the blockchain and ensures that the delegation occurs on the intended chain.
 * \param[in] genesis_delegate_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the hash of the delegate's public key.
 *                                  This delegate is the new recipient of the block production rights originally held by the genesis key.
 * \param[in] vrf_key_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the hash of the VRF key belonging to the delegate.
 * \param[out] cert On successful initialization, this parameter will point to the newly created \ref cardano_genesis_key_delegation_cert_t object.
 *                  The caller is responsible for managing the lifecycle of this object, including its deallocation through the appropriate unref function.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was successfully created,
 *         or an appropriate error code indicating the reason for failure, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* genesis_hash = ...; // Assume genesis_hash is already initialized
 * cardano_blake2b_hash_t* genesis_delegate_hash = ...; // Assume genesis_delegate_hash is initialized
 * cardano_blake2b_hash_t* vrf_key_hash = ...; // Assume vrf_key_hash is initialized
 * cardano_genesis_key_delegation_cert_t* cert = NULL;
 *
 * cardano_error_t result = cardano_genesis_key_delegation_cert_new(genesis_hash, genesis_delegate_hash, vrf_key_hash, &cert);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The Genesis Key Delegation certificate is now ready to be used
 *   // Remember to free the certificate when done
 *   cardano_genesis_key_delegation_cert_unref(&cert);
 * }
 * else
 * {
 *   printf("Failed to create Genesis Key Delegation certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_blake2b_hash_unref(&genesis_hash);
 * cardano_blake2b_hash_unref(&genesis_delegate_hash);
 * cardano_blake2b_hash_unref(&vrf_key_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_genesis_key_delegation_cert_new(
  cardano_blake2b_hash_t*                 genesis_hash,
  cardano_blake2b_hash_t*                 genesis_delegate_hash,
  cardano_blake2b_hash_t*                 vrf_key_hash,
  cardano_genesis_key_delegation_cert_t** cert);

/**
 * \brief Creates a \ref cardano_genesis_key_delegation_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_genesis_key_delegation_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a genesis_key_delegation_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] genesis_key_delegation_cert A pointer to a pointer of \ref cardano_genesis_key_delegation_cert_t that will be set to the address
 *                        of the newly created genesis_key_delegation_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_genesis_key_delegation_cert_t object by calling
 *       \ref cardano_genesis_key_delegation_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;
 *
 * cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the genesis_key_delegation_cert
 *
 *   // Once done, ensure to clean up and release the genesis_key_delegation_cert
 *   cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode genesis_key_delegation_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_genesis_key_delegation_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_genesis_key_delegation_cert_t** genesis_key_delegation_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_genesis_key_delegation_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] genesis_key_delegation_cert A constant pointer to the \ref cardano_genesis_key_delegation_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p genesis_key_delegation_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_genesis_key_delegation_cert_to_cbor(genesis_key_delegation_cert, writer);
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
 * cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_genesis_key_delegation_cert_to_cbor(
  const cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert,
  cardano_cbor_writer_t*                       writer);

/**
 * \brief Retrieves the genesis hash from a genesis key delegation certificate.
 *
 * This function extracts the genesis hash from the specified \ref cardano_genesis_key_delegation_cert_t object.
 * The genesis hash uniquely identifies the blockchain network and is used to confirm that the delegation occurs on the intended blockchain.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_genesis_key_delegation_cert_t object.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object containing the genesis hash. If the certificate is NULL or
 *         if the genesis hash is not set, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_genesis_key_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* genesis_hash = cardano_genesis_key_delegation_cert_get_genesis_hash(certificate);
 *
 * if (genesis_hash != NULL)
 * {
 *   // Process the genesis hash
 *   cardano_blake2b_hash_unref(&genesis_hash);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_genesis_key_delegation_cert_get_genesis_hash(
  cardano_genesis_key_delegation_cert_t* certificate);

/**
 * \brief Sets the genesis hash in a genesis key delegation certificate.
 *
 * This function assigns a new genesis hash to the specified \ref cardano_genesis_key_delegation_cert_t object.
 * The genesis hash uniquely identifies the blockchain network and is used to confirm that the delegation occurs on the intended blockchain.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_genesis_key_delegation_cert_t object.
 * \param[in] hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the genesis hash.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the genesis hash
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_genesis_key_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* hash = ...; // Assume hash is initialized
 * cardano_error_t result = cardano_genesis_key_delegation_cert_set_genesis_hash(certificate, hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Genesis hash set successfully.\n");
 *
 *  cardano_blake2b_hash_unref(&hash);
 * }
 * else
 * {
 *   printf("Failed to set genesis hash.\n");
 * }
 * \endcode
 *
 * \note This function does not take ownership of the hash; the caller is responsible for managing the lifecycle of the hash object.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_genesis_key_delegation_cert_set_genesis_hash(
  cardano_genesis_key_delegation_cert_t* certificate,
  cardano_blake2b_hash_t*                hash);

/**
 * \brief Retrieves the genesis delegate hash from a genesis key delegation certificate.
 *
 * This function extracts the genesis delegate hash from the specified \ref cardano_genesis_key_delegation_cert_t object.
 * The genesis delegate hash is the public key hash of the delegate to whom the power of the genesis key is being delegated.
 * This delegation is crucial during the transitional phase from the Byron era to the Shelley era, allowing specified delegates
 * the rights to produce blocks.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_genesis_key_delegation_cert_t object.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object containing the genesis delegate hash. If the certificate is NULL or
 *         if the genesis delegate hash is not set, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_genesis_key_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* delegate_hash = cardano_genesis_key_delegation_cert_get_genesis_delegate_hash(certificate);
 *
 * if (delegate_hash != NULL)
 * {
 *   // Process the genesis delegate hash
 *   cardano_blake2b_hash_unref(&delegate_hash);
 * }
 * \endcode
 *
 * \note This function returns a new reference that must be managed by the caller.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_genesis_key_delegation_cert_get_genesis_delegate_hash(
  cardano_genesis_key_delegation_cert_t* certificate);

/**
 * \brief Sets the genesis delegate hash for a genesis key delegation certificate.
 *
 * This function assigns a new genesis delegate hash to the specified \ref cardano_genesis_key_delegation_cert_t object.
 * The genesis delegate hash is the public key hash of the delegate to whom the power of the genesis key is being delegated,
 * crucial during the transitional phase from the Byron era to the Shelley era.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_genesis_key_delegation_cert_t object.
 * \param[in] hash A constant pointer to an initialized \ref cardano_blake2b_hash_t object containing the new genesis delegate hash.
 *                 The certificate object will hold a reference to this hash object, so it must remain valid for the duration of
 *                 the certificate's lifecycle or until a different hash is set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the genesis delegate hash
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any
 *         of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_genesis_key_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* delegate_hash = ...; // Assume delegate_hash is already initialized
 *
 * cardano_error_t result = cardano_genesis_key_delegation_cert_set_genesis_delegate_hash(certificate, delegate_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Genesis delegate hash updated successfully.\n");
 *   cardano_blake2b_hash_unref(&delegate_hash);
 * }
 * else
 * {
 *   printf("Failed to set the genesis delegate hash.\n");
 * }
 * \endcode
 *
 * \note This function does not take ownership of the hash; the caller is responsible for managing the lifecycle of the hash object.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_genesis_key_delegation_cert_set_genesis_delegate_hash(
  cardano_genesis_key_delegation_cert_t* certificate,
  cardano_blake2b_hash_t*                hash);

/**
 * \brief Retrieves the VRF key hash from a genesis key delegation certificate.
 *
 * This function extracts the VRF (Verifiable Random Function) key hash from the specified
 * \ref cardano_genesis_key_delegation_cert_t object. The VRF key hash is critical for ensuring the integrity
 * and security of the random selection process in the blockchain's consensus mechanism.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_genesis_key_delegation_cert_t object.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object containing the VRF key hash. If the certificate is NULL or
 *         if the VRF key hash is not set, this function returns NULL. The returned hash object is a new reference, and
 *         the caller is responsible for releasing it using \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_genesis_key_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_blake2b_hash_t* vrf_key_hash = cardano_genesis_key_delegation_cert_get_vrf_key_hash(certificate);
 *
 * if (vrf_key_hash != NULL)
 * {
 *   // Process the VRF key hash
 *   cardano_blake2b_hash_unref(&vrf_key_hash);
 * }
 * else
 * {
 *   printf("Failed to retrieve VRF key hash.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_genesis_key_delegation_cert_get_vrf_key_hash(
  cardano_genesis_key_delegation_cert_t* certificate);

/**
 * \brief Sets the VRF key hash for a genesis key delegation certificate.
 *
 * This function assigns a VRF (Verifiable Random Function) key hash to a given \ref cardano_genesis_key_delegation_cert_t object.
 * The VRF key hash is crucial for the secure and random selection process in the blockchain's consensus mechanism.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_genesis_key_delegation_cert_t object to which the VRF key hash will be set.
 * \param[in] hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the VRF key hash.
 *                 The certificate takes a reference to this hash, ensuring it remains valid for the duration of the certificate's usage.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the VRF key hash was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_genesis_key_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_blake2b_hash_t* vrf_key_hash = ...; // Assume vrf_key_hash is already initialized
 *
 * cardano_error_t result = cardano_genesis_key_delegation_cert_set_vrf_key_hash(certificate, vrf_key_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("VRF key hash successfully set.\n");
 *   cardano_blake2b_hash_unref(&vrf_key_hash);
 * }
 * else
 * {
 *   printf("Failed to set VRF key hash.\n");
 * }
 * \endcode
 *
 * \note This function does not take ownership of the hash; the caller is responsible for managing the lifecycle of the hash object.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_genesis_key_delegation_cert_set_vrf_key_hash(
  cardano_genesis_key_delegation_cert_t* certificate,
  cardano_blake2b_hash_t*                hash);

/**
 * \brief Decrements the reference count of a cardano_genesis_key_delegation_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_genesis_key_delegation_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the genesis_key_delegation_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] genesis_key_delegation_cert A pointer to the pointer of the genesis_key_delegation_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = cardano_genesis_key_delegation_cert_new(major, minor);
 *
 * // Perform operations with the genesis_key_delegation_cert...
 *
 * cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
 * // At this point, genesis_key_delegation_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_genesis_key_delegation_cert_unref, the pointer to the \ref cardano_genesis_key_delegation_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_genesis_key_delegation_cert_unref(
  cardano_genesis_key_delegation_cert_t** genesis_key_delegation_cert);

/**
 * \brief Increases the reference count of the cardano_genesis_key_delegation_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_genesis_key_delegation_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_genesis_key_delegation_cert_unref.
 *
 * \param genesis_key_delegation_cert A pointer to the cardano_genesis_key_delegation_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming genesis_key_delegation_cert is a previously created genesis_key_delegation_cert object
 *
 * cardano_genesis_key_delegation_cert_ref(genesis_key_delegation_cert);
 *
 * // Now genesis_key_delegation_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_genesis_key_delegation_cert_ref there is a corresponding
 * call to \ref cardano_genesis_key_delegation_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_genesis_key_delegation_cert_ref(
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert);

/**
 * \brief Retrieves the current reference count of the cardano_genesis_key_delegation_cert_t object.
 *
 * This function returns the number of active references to an cardano_genesis_key_delegation_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_genesis_key_delegation_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param genesis_key_delegation_cert A pointer to the cardano_genesis_key_delegation_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_genesis_key_delegation_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_genesis_key_delegation_cert_ref call is matched with a
 * \ref cardano_genesis_key_delegation_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming genesis_key_delegation_cert is a previously created genesis_key_delegation_cert object
 *
 * size_t ref_count = cardano_genesis_key_delegation_cert_refcount(genesis_key_delegation_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_genesis_key_delegation_cert_refcount(
  const cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert);

/**
 * \brief Sets the last error message for a given cardano_genesis_key_delegation_cert_t object.
 *
 * Records an error message in the genesis_key_delegation_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] genesis_key_delegation_cert A pointer to the \ref cardano_genesis_key_delegation_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the genesis_key_delegation_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_genesis_key_delegation_cert_set_last_error(
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert,
  const char*                            message);

/**
 * \brief Retrieves the last error message recorded for a specific genesis_key_delegation_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_genesis_key_delegation_cert_set_last_error for the given
 * genesis_key_delegation_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] genesis_key_delegation_cert A pointer to the \ref cardano_genesis_key_delegation_cert_t instance whose last error
 *                   message is to be retrieved. If the genesis_key_delegation_cert is NULL, the function
 *                   returns a generic error message indicating the null genesis_key_delegation_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified genesis_key_delegation_cert. If the genesis_key_delegation_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_genesis_key_delegation_cert_set_last_error for the same genesis_key_delegation_cert, or until
 *       the genesis_key_delegation_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_genesis_key_delegation_cert_get_last_error(
  const cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // genesis_key_delegation_cert_H