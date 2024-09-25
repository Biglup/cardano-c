/**
 * \file unregistration_cert.h
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

#ifndef UNREGISTRATION_CERT_H
#define UNREGISTRATION_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This certificate is used when a stakeholder no longer wants to participate in
 * staking. It revokes the stake Unregistration and the associated stake is no
 * longer counted when calculating stake pool rewards.
 *
 * Deposit must match the expected deposit amount specified by `ppKeyDepositL` in
 * the protocol parameters.
 *
 * \remark Replaces the deprecated \c cardano_stake_registration_cert_t after Conway era.
 */
typedef struct cardano_unregistration_cert_t cardano_unregistration_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new unregistration certificate.
 *
 * This function allocates and initializes a new unregistration certificate. The certificate is used to unregister a stake key,
 * removing it from the blockchain and reclaiming any deposits associated with it.
 *
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the stake credential.
 * \param[in] deposit The amount of deposit to be refunded upon successful unregistration, expressed in lovelace (1 ADA = 1,000,000 lovelace).
 * \param[out] unregistration On successful execution, this will point to a newly created \ref cardano_unregistration_cert_t object.
 *                             The caller is responsible for managing the lifecycle of this object and must release it using
 *                             \ref cardano_unregistration_cert_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an appropriate error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 * uint64_t deposit = 2000000; // 2 ADA in lovelace
 * cardano_unregistration_cert_t* unregistration = NULL;
 *
 * cardano_error_t result = cardano_unregistration_cert_new(credential, deposit, &unregistration);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The unregistration certificate can now be used for blockchain transactions
 *   // Remember to free the unregistration certificate when done
 *   cardano_unregistration_cert_unref(&unregistration);
 * }
 * else
 * {
 *   printf("Failed to create unregistration certificate: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregistration_cert_new(
  cardano_credential_t*           credential,
  uint64_t                        deposit,
  cardano_unregistration_cert_t** unregistration);

/**
 * \brief Creates a \ref cardano_unregistration_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_unregistration_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a unregistration_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] unregistration_cert A pointer to a pointer of \ref cardano_unregistration_cert_t that will be set to the address
 *                        of the newly created unregistration_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_unregistration_cert_t object by calling
 *       \ref cardano_unregistration_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_unregistration_cert_t* unregistration_cert = NULL;
 *
 * cardano_error_t result = cardano_unregistration_cert_from_cbor(reader, &unregistration_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the unregistration_cert
 *
 *   // Once done, ensure to clean up and release the unregistration_cert
 *   cardano_unregistration_cert_unref(&unregistration_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode unregistration_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregistration_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_unregistration_cert_t** unregistration_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_unregistration_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] unregistration_cert A constant pointer to the \ref cardano_unregistration_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p unregistration_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregistration_cert_t* unregistration_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_unregistration_cert_to_cbor(unregistration_cert, writer);
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
 * cardano_unregistration_cert_unref(&unregistration_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_unregistration_cert_to_cbor(
  const cardano_unregistration_cert_t* unregistration_cert,
  cardano_cbor_writer_t*               writer);

/**
 * \brief Retrieves the credential associated with an unregistration certificate.
 *
 * This function fetches the credential from a given \ref cardano_unregistration_cert_t object. The credential indicates
 * the stake key being unregistered.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_unregistration_cert_t object.
 *
 * \return A pointer to the \ref cardano_credential_t object representing the stake credential. If the input certificate
 *         pointer is NULL or the certificate does not contain a credential, this function returns NULL.
 *         Note that the returned credential is a new reference and must be released using \ref cardano_credential_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_unregistration_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = cardano_unregistration_cert_get_credential(certificate);
 *
 * if (credential != NULL)
 * {
 *   // Process the credential
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   printf("No credential found or invalid certificate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_unregistration_cert_get_credential(cardano_unregistration_cert_t* certificate);

/**
 * \brief Sets the credential for an unregistration certificate.
 *
 * This function assigns a new stake credential to a \ref cardano_unregistration_cert_t object.
 * The credential specifies the stake key to be unregistered.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_unregistration_cert_t object to which the credential will be set.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the stake credential to be unregistered.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the credential
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note This function increases the reference count of the credential object; therefore, the caller retain ownership of their respective references.
 *       It is the caller's responsibility to release their reference to the credential when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregistration_cert_t* unregistration_cert = ...; // Assume unregistration_cert is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is initialized
 *
 * cardano_error_t result = cardano_unregistration_cert_set_credential(unregistration_cert, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The credential is now set for the unregistration_cert
 * }
 * else
 * {
 *   printf("Failed to set the credential.\n");
 * }
 *
 * // Clean up resources
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregistration_cert_set_credential(cardano_unregistration_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Retrieves the deposit amount from an unregistration certificate.
 *
 * This function retrieves the deposit amount that was associated with the unregistration certificate
 * at the time of its creation.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_unregistration_cert_t object.
 *
 * \return The deposit amount in lovelaces. If the certificate is NULL, the function will return 0, which should be
 *         handled appropriately by the caller to distinguish from a genuine deposit of 0 lovelaces.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_unregistration_cert_t* unregistration_cert = ...; // Assume unregistration_cert is already initialized
 *
 * uint64_t deposit = cardano_unregistration_cert_get_deposit(unregistration_cert);
 * printf("Deposit for unregistration: %llu lovelaces\n", deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_unregistration_cert_get_deposit(const cardano_unregistration_cert_t* certificate);

/**
 * \brief Sets the deposit amount for an unregistration certificate.
 *
 * This function sets the deposit amount required for unregistration of a stake credential.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_unregistration_cert_t object to which the deposit amount will be set.
 * \param[in] deposit The deposit amount in lovelaces to set for the unregistration.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the deposit amount was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input certificate pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregistration_cert_t* unregistration_cert = ...; // Assume unregistration_cert is already initialized
 * uint64_t deposit = 2000000;
 *
 * cardano_error_t result = cardano_unregistration_cert_set_deposit(unregistration_cert, deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Deposit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregistration_cert_set_deposit(cardano_unregistration_cert_t* certificate, uint64_t deposit);

/**
 * \brief Decrements the reference count of a cardano_unregistration_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_unregistration_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the unregistration_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] unregistration_cert A pointer to the pointer of the unregistration_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregistration_cert_t* unregistration_cert = cardano_unregistration_cert_new(major, minor);
 *
 * // Perform operations with the unregistration_cert...
 *
 * cardano_unregistration_cert_unref(&unregistration_cert);
 * // At this point, unregistration_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_unregistration_cert_unref, the pointer to the \ref cardano_unregistration_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_unregistration_cert_unref(cardano_unregistration_cert_t** unregistration_cert);

/**
 * \brief Increases the reference count of the cardano_unregistration_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_unregistration_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_unregistration_cert_unref.
 *
 * \param unregistration_cert A pointer to the cardano_unregistration_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming unregistration_cert is a previously created unregistration_cert object
 *
 * cardano_unregistration_cert_ref(unregistration_cert);
 *
 * // Now unregistration_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_unregistration_cert_ref there is a corresponding
 * call to \ref cardano_unregistration_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_unregistration_cert_ref(cardano_unregistration_cert_t* unregistration_cert);

/**
 * \brief Retrieves the current reference count of the cardano_unregistration_cert_t object.
 *
 * This function returns the number of active references to an cardano_unregistration_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_unregistration_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param unregistration_cert A pointer to the cardano_unregistration_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_unregistration_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_unregistration_cert_ref call is matched with a
 * \ref cardano_unregistration_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming unregistration_cert is a previously created unregistration_cert object
 *
 * size_t ref_count = cardano_unregistration_cert_refcount(unregistration_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_unregistration_cert_refcount(const cardano_unregistration_cert_t* unregistration_cert);

/**
 * \brief Sets the last error message for a given cardano_unregistration_cert_t object.
 *
 * Records an error message in the unregistration_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] unregistration_cert A pointer to the \ref cardano_unregistration_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the unregistration_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_unregistration_cert_set_last_error(
  cardano_unregistration_cert_t* unregistration_cert,
  const char*                    message);

/**
 * \brief Retrieves the last error message recorded for a specific unregistration_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_unregistration_cert_set_last_error for the given
 * unregistration_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] unregistration_cert A pointer to the \ref cardano_unregistration_cert_t instance whose last error
 *                   message is to be retrieved. If the unregistration_cert is NULL, the function
 *                   returns a generic error message indicating the null unregistration_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified unregistration_cert. If the unregistration_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_unregistration_cert_set_last_error for the same unregistration_cert, or until
 *       the unregistration_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_unregistration_cert_get_last_error(
  const cardano_unregistration_cert_t* unregistration_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // UNREGISTRATION_CERT_H