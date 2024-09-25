/**
 * \file unregister_drep_cert.h
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

#ifndef UNREGISTER_DREP_CERT_H
#define UNREGISTER_DREP_CERT_H

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
 * \brief This certificate unregister an individual as a DRep.
 *
 * Note that a DRep is retired immediately upon the chain accepting a retirement certificate, and
 * the deposit is returned as part of the transaction that submits the retirement certificate
 * (the same way that stake credential registration deposits are returned).
 */
typedef struct cardano_unregister_drep_cert_t cardano_unregister_drep_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new unregistration certificate for a DRep in the Cardano blockchain.
 *
 * This function allocates and initializes a new \ref cardano_unregister_drep_cert_t object, which is used
 * to unregister a DRep. The certificate includes the stake credential associated with the DRep and
 * a deposit amount that might be refunded upon successful unregistration.
 *
 * \param[in] credential A pointer to a \ref cardano_credential_t object representing the stake credential
 *                       of the DRep. This credential must have been previously registered.
 * \param[in] deposit The deposit amount that was originally paid during the registration of the DRep.
 *                    This deposit may be refunded upon successful unregistration.
 * \param[out] unregister_drep On successful execution, this will point to a newly created
 *                             \ref cardano_unregister_drep_cert_t object. The caller is responsible for
 *                             managing the lifecycle of this object, which includes freeing it when it's
 *                             no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         unregistration certificate was successfully created, or an appropriate error code indicating
 *         the failure reason (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 * uint64_t deposit = 1000000; // Example deposit amount
 *
 * cardano_unregister_drep_cert_t* unregister_cert = NULL;
 * cardano_error_t result = cardano_unregister_drep_cert_new(credential, deposit, &unregister_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The unregistration certificate can now be used to unregister the DRep
 *   // Remember to free the unregister_cert when done
 *   cardano_unregister_drep_cert_unref(&unregister_cert);
 * }
 * else
 * {
 *   printf("Failed to create DRep unregistration certificate: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregister_drep_cert_new(
  cardano_credential_t*            credential,
  uint64_t                         deposit,
  cardano_unregister_drep_cert_t** unregister_drep);

/**
 * \brief Creates a \ref cardano_unregister_drep_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_unregister_drep_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a unregister_drep_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] unregister_drep_cert A pointer to a pointer of \ref cardano_unregister_drep_cert_t that will be set to the address
 *                        of the newly created unregister_drep_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_unregister_drep_cert_t object by calling
 *       \ref cardano_unregister_drep_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_unregister_drep_cert_t* unregister_drep_cert = NULL;
 *
 * cardano_error_t result = cardano_unregister_drep_cert_from_cbor(reader, &unregister_drep_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the unregister_drep_cert
 *
 *   // Once done, ensure to clean up and release the unregister_drep_cert
 *   cardano_unregister_drep_cert_unref(&unregister_drep_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode unregister_drep_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregister_drep_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_unregister_drep_cert_t** unregister_drep_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_unregister_drep_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] unregister_drep_cert A constant pointer to the \ref cardano_unregister_drep_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p unregister_drep_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregister_drep_cert_t* unregister_drep_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_unregister_drep_cert_to_cbor(unregister_drep_cert, writer);
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
 * cardano_unregister_drep_cert_unref(&unregister_drep_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_unregister_drep_cert_to_cbor(
  const cardano_unregister_drep_cert_t* unregister_drep_cert,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Retrieves the stake credential associated with a DRep unregistration certificate.
 *
 * This function extracts the stake credential from a given \ref cardano_unregister_drep_cert_t object.
 * The credential is used to identify the DRep that is being unregistered.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_unregister_drep_cert_t object.
 *
 * \return A pointer to the \ref cardano_credential_t object containing the stake credential. If the
 *         certificate is NULL or if the credential is not set, this function returns NULL. This returned
 *         object is a new reference and must be released with \ref cardano_credential_unref by the caller when
 *         it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_unregister_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_credential_t* credential = cardano_unregister_drep_cert_get_credential(certificate);
 *
 * if (credential != NULL)
 * {
 *   // Process the credential
 *   cardano_credential_unref(&credential);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_unregister_drep_cert_get_credential(cardano_unregister_drep_cert_t* certificate);

/**
 * \brief Sets the stake credential for a DRep unregistration certificate.
 *
 * This function assigns a new stake credential to a specified \ref cardano_unregister_drep_cert_t object.
 * The credential is crucial for identifying the DRep that is to be unregistered.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_unregister_drep_cert_t object.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object. This function increments
 *                       the reference count of the credential, and it will be decremented when the certificate
 *                       no longer uses it or when the certificate is freed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         credential was successfully set, or an appropriate error code indicating the failure reason, such as
 *         \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregister_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_unregister_drep_cert_set_credential(certificate, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Credential set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the credential.\n");
 * }
 *
 * // Cleanup resources as necessary
 * cardano_credential_unref(&credential); // If no longer needed elsewhere
 * cardano_unregister_drep_cert_unref(&certificate); // After use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregister_drep_cert_set_credential(cardano_unregister_drep_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Retrieves the deposit amount from an unregistration DRep certificate.
 *
 * This function gets the deposit amount that was associated with the DRep unregistration. This amount
 * represents the funds that were originally locked during the registration and are now subject to refund
 * upon successful unregistration.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_unregister_drep_cert_t object.
 *
 * \return The deposit amount as a uint64_t. Returns 0 if the certificate pointer is NULL or if no deposit
 *         was recorded.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_unregister_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = cardano_unregister_drep_cert_get_deposit(certificate);
 * printf("Deposit amount: %llu ADA\n", deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_unregister_drep_cert_get_deposit(const cardano_unregister_drep_cert_t* certificate);

/**
 * \brief Sets the deposit amount for an unregistration DRep certificate.
 *
 * This function sets the deposit amount in the specified \ref cardano_unregister_drep_cert_t object.
 * The deposit is the amount of funds that will be locked until the unregistration process is completed.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_unregister_drep_cert_t object.
 * \param[in] deposit The deposit amount to be set, expressed in lovelace (1 ADA = 1,000,000 lovelace).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the deposit was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the certificate pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregister_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = 1500000; // 1.5 ADA in lovelace
 *
 * cardano_error_t result = cardano_unregister_drep_cert_set_deposit(certificate, deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Deposit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set deposit: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unregister_drep_cert_set_deposit(cardano_unregister_drep_cert_t* certificate, uint64_t deposit);

/**
 * \brief Decrements the reference count of a cardano_unregister_drep_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_unregister_drep_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the unregister_drep_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] unregister_drep_cert A pointer to the pointer of the unregister_drep_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unregister_drep_cert_t* unregister_drep_cert = cardano_unregister_drep_cert_new(major, minor);
 *
 * // Perform operations with the unregister_drep_cert...
 *
 * cardano_unregister_drep_cert_unref(&unregister_drep_cert);
 * // At this point, unregister_drep_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_unregister_drep_cert_unref, the pointer to the \ref cardano_unregister_drep_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_unregister_drep_cert_unref(cardano_unregister_drep_cert_t** unregister_drep_cert);

/**
 * \brief Increases the reference count of the cardano_unregister_drep_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_unregister_drep_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_unregister_drep_cert_unref.
 *
 * \param unregister_drep_cert A pointer to the cardano_unregister_drep_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming unregister_drep_cert is a previously created unregister_drep_cert object
 *
 * cardano_unregister_drep_cert_ref(unregister_drep_cert);
 *
 * // Now unregister_drep_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_unregister_drep_cert_ref there is a corresponding
 * call to \ref cardano_unregister_drep_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_unregister_drep_cert_ref(cardano_unregister_drep_cert_t* unregister_drep_cert);

/**
 * \brief Retrieves the current reference count of the cardano_unregister_drep_cert_t object.
 *
 * This function returns the number of active references to an cardano_unregister_drep_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_unregister_drep_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param unregister_drep_cert A pointer to the cardano_unregister_drep_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_unregister_drep_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_unregister_drep_cert_ref call is matched with a
 * \ref cardano_unregister_drep_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming unregister_drep_cert is a previously created unregister_drep_cert object
 *
 * size_t ref_count = cardano_unregister_drep_cert_refcount(unregister_drep_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_unregister_drep_cert_refcount(const cardano_unregister_drep_cert_t* unregister_drep_cert);

/**
 * \brief Sets the last error message for a given cardano_unregister_drep_cert_t object.
 *
 * Records an error message in the unregister_drep_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] unregister_drep_cert A pointer to the \ref cardano_unregister_drep_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the unregister_drep_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_unregister_drep_cert_set_last_error(
  cardano_unregister_drep_cert_t* unregister_drep_cert,
  const char*                     message);

/**
 * \brief Retrieves the last error message recorded for a specific unregister_drep_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_unregister_drep_cert_set_last_error for the given
 * unregister_drep_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] unregister_drep_cert A pointer to the \ref cardano_unregister_drep_cert_t instance whose last error
 *                   message is to be retrieved. If the unregister_drep_cert is NULL, the function
 *                   returns a generic error message indicating the null unregister_drep_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified unregister_drep_cert. If the unregister_drep_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_unregister_drep_cert_set_last_error for the same unregister_drep_cert, or until
 *       the unregister_drep_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_unregister_drep_cert_get_last_error(
  const cardano_unregister_drep_cert_t* unregister_drep_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // UNREGISTER_DREP_CERT_H