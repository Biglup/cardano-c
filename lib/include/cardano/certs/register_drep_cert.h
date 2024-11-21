/**
 * \file register_drep_cert.h
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

#ifndef REGISTER_DREP_CERT_H
#define REGISTER_DREP_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/anchor.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief In Voltaire, existing stake credentials will be able to delegate their stake to DReps for voting
 * purposes, in addition to the current delegation to stake pools for block production.
 * DRep delegation will mimic the existing stake delegation mechanisms (via on-chain certificates).
 *
 * This certificate register a stake key as a DRep.
 */
typedef struct cardano_register_drep_cert_t cardano_register_drep_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new DRep registration certificate.
 *
 * This function allocates and initializes a \ref cardano_register_drep_cert_t object,
 * which represents a certificate for registering a DRep in the Cardano network.
 *
 * \param[in] drep_credential A pointer to an initialized \ref cardano_credential_t object representing
 *                            the credential of the decentralized representative.
 * \param[in] deposit The deposit amount required for registration.
 * \param[in] anchor An instance of \ref cardano_anchor_t representing the anchor of the metadata linked to this DRep. This is optional.
 * \param[out] register_drep_cert On successful execution, this will point to a newly created \ref cardano_register_drep_cert_t object.
 *                                The caller is responsible for releasing this resource using \ref cardano_register_drep_cert_unref when it
 *                                is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep registration certificate was successfully created, or an appropriate error code indicating
 *         the failure reason (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* drep_credential = ...; // Assume drep_credential is already initialized
 * uint64_t deposit = 5000000; // Example deposit amount in lovelace
 * cardano_anchor_t anchor = ...; // Assume anchor is initialized
 * cardano_register_drep_cert_t* register_drep_cert = NULL;
 *
 * cardano_error_t result = cardano_register_drep_cert_new(drep_credential, deposit, anchor, &register_drep_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The DRep registration certificate can now be used
 *   // Remember to free the register_drep_cert when done
 *   cardano_register_drep_cert_unref(&register_drep_cert);
 * }
 * else
 * {
 *   printf("Failed to create DRep registration certificate: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_credential_unref(&drep_credential); // Cleanup the drep_credential
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_register_drep_cert_new(
  cardano_credential_t*          drep_credential,
  uint64_t                       deposit,
  cardano_anchor_t*              anchor,
  cardano_register_drep_cert_t** register_drep_cert);

/**
 * \brief Creates a \ref cardano_register_drep_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_register_drep_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a register_drep_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] register_drep_cert A pointer to a pointer of \ref cardano_register_drep_cert_t that will be set to the address
 *                        of the newly created register_drep_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_register_drep_cert_t object by calling
 *       \ref cardano_register_drep_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_register_drep_cert_t* register_drep_cert = NULL;
 *
 * cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the register_drep_cert
 *
 *   // Once done, ensure to clean up and release the register_drep_cert
 *   cardano_register_drep_cert_unref(&register_drep_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode register_drep_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_register_drep_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_register_drep_cert_t** register_drep_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_register_drep_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] register_drep_cert A constant pointer to the \ref cardano_register_drep_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p register_drep_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* register_drep_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_register_drep_cert_to_cbor(register_drep_cert, writer);
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
 * cardano_register_drep_cert_unref(&register_drep_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_register_drep_cert_to_cbor(
  const cardano_register_drep_cert_t* register_drep_cert,
  cardano_cbor_writer_t*              writer);

/**
 * \brief Retrieves the DRep credential from a DRep registration certificate.
 *
 * This function extracts the decentralized representative's credential from the specified
 * \ref cardano_register_drep_cert_t object. This credential is used to identify the DRep
 * within the Cardano network.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_register_drep_cert_t object.
 *
 * \return A pointer to the \ref cardano_credential_t object containing the DRep credential. If the certificate
 *         is NULL or does not contain a DRep credential, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_register_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_credential_t* drep_credential = cardano_register_drep_cert_get_credential(certificate);
 *
 * if (drep_credential != NULL)
 * {
 *   // Process the DRep credential
 *   // Note: The credential does not need to be freed; it is managed within the certificate object
 * }
 * else
 * {
 *   printf("No DRep credential is set for this certificate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_register_drep_cert_get_credential(cardano_register_drep_cert_t* certificate);

/**
 * \brief Sets the DRep credential for a DRep registration certificate.
 *
 * This function updates the DRep credential in a \ref cardano_register_drep_cert_t object with the provided
 * \ref cardano_credential_t. The DRep credential identifies the decentralized representative within the Cardano network.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_register_drep_cert_t object to which the credential will be set.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the new DRep credential.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the DRep credential
 *         was successfully updated, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_register_drep_cert_set_credential(certificate, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep credential updated successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to update DRep credential.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_register_drep_cert_set_credential(cardano_register_drep_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Retrieves the deposit amount from a DRep registration certificate.
 *
 * This function fetches the deposit amount specified in a \ref cardano_register_drep_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_register_drep_cert_t object.
 *
 * \return The deposit amount in lovelace (1 ADA = 1,000,000 lovelace). If the certificate is NULL, this function will
 *         return 0.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_register_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = cardano_register_drep_cert_get_deposit(certificate);
 *
 * if (deposit > 0)
 * {
 *   printf("Deposit amount: %lu lovelace\n", deposit);
 * }
 * else
 * {
 *   printf("Failed to retrieve deposit amount.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_register_drep_cert_get_deposit(const cardano_register_drep_cert_t* certificate);

/**
 * \brief Sets the deposit amount in a DRep registration certificate.
 *
 * This function assigns a new deposit amount to a \ref cardano_register_drep_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_register_drep_cert_t object which will have its deposit updated.
 * \param[in] deposit The deposit amount in lovelace (1 ADA = 1,000,000 lovelace) to be set in the certificate.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the deposit was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         certificate pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = 1000000; // Set deposit to 1 ADA
 *
 * cardano_error_t result = cardano_register_drep_cert_set_deposit(certificate, deposit);
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
cardano_register_drep_cert_set_deposit(cardano_register_drep_cert_t* certificate, uint64_t deposit);

/**
 * \brief Retrieves the anchor from a DRep registration certificate.
 *
 * This function extracts the anchor from the specified \ref cardano_register_drep_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_register_drep_cert_t object.
 *
 * \return A pointer to the \ref cardano_anchor_t object containing the anchor. If the certificate is NULL or
 *         if the anchor is not set, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_register_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_anchor_t* anchor = cardano_register_drep_cert_get_anchor(certificate);
 *
 * if (anchor != NULL)
 * {
 *   // Process the anchor
 *   printf("Anchor retrieved successfully.\n");
 * }
 * else
 * {
 *   printf("No anchor set for this certificate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_anchor_t* cardano_register_drep_cert_get_anchor(cardano_register_drep_cert_t* certificate);

/**
 * \brief Sets the anchor for a DRep registration certificate.
 *
 * This function updates the anchor in the specified \ref cardano_register_drep_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_register_drep_cert_t object
 *                        which will receive the updated anchor.
 * \param[in] anchor A pointer to an initialized \ref cardano_anchor_t object that represents
 *                   the new anchor. This function copies the anchor into the certificate.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the anchor was successfully updated, or an appropriate error code indicating the
 *         failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_anchor_t* anchor = ...; // Assume anchor is already initialized
 *
 * cardano_error_t result = cardano_register_drep_cert_set_anchor(certificate, anchor);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Anchor set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the anchor.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_register_drep_cert_set_anchor(cardano_register_drep_cert_t* certificate, cardano_anchor_t* anchor);

/**
 * \brief Decrements the reference count of a cardano_register_drep_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_register_drep_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the register_drep_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] register_drep_cert A pointer to the pointer of the register_drep_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_register_drep_cert_t* register_drep_cert = cardano_register_drep_cert_new(major, minor);
 *
 * // Perform operations with the register_drep_cert...
 *
 * cardano_register_drep_cert_unref(&register_drep_cert);
 * // At this point, register_drep_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_register_drep_cert_unref, the pointer to the \ref cardano_register_drep_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_register_drep_cert_unref(cardano_register_drep_cert_t** register_drep_cert);

/**
 * \brief Increases the reference count of the cardano_register_drep_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_register_drep_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_register_drep_cert_unref.
 *
 * \param register_drep_cert A pointer to the cardano_register_drep_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming register_drep_cert is a previously created register_drep_cert object
 *
 * cardano_register_drep_cert_ref(register_drep_cert);
 *
 * // Now register_drep_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_register_drep_cert_ref there is a corresponding
 * call to \ref cardano_register_drep_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_register_drep_cert_ref(cardano_register_drep_cert_t* register_drep_cert);

/**
 * \brief Retrieves the current reference count of the cardano_register_drep_cert_t object.
 *
 * This function returns the number of active references to an cardano_register_drep_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_register_drep_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param register_drep_cert A pointer to the cardano_register_drep_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_register_drep_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_register_drep_cert_ref call is matched with a
 * \ref cardano_register_drep_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming register_drep_cert is a previously created register_drep_cert object
 *
 * size_t ref_count = cardano_register_drep_cert_refcount(register_drep_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_register_drep_cert_refcount(const cardano_register_drep_cert_t* register_drep_cert);

/**
 * \brief Sets the last error message for a given cardano_register_drep_cert_t object.
 *
 * Records an error message in the register_drep_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] register_drep_cert A pointer to the \ref cardano_register_drep_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the register_drep_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_register_drep_cert_set_last_error(
  cardano_register_drep_cert_t* register_drep_cert,
  const char*                   message);

/**
 * \brief Retrieves the last error message recorded for a specific register_drep_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_register_drep_cert_set_last_error for the given
 * register_drep_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] register_drep_cert A pointer to the \ref cardano_register_drep_cert_t instance whose last error
 *                   message is to be retrieved. If the register_drep_cert is NULL, the function
 *                   returns a generic error message indicating the null register_drep_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified register_drep_cert. If the register_drep_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_register_drep_cert_set_last_error for the same register_drep_cert, or until
 *       the register_drep_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_register_drep_cert_get_last_error(
  const cardano_register_drep_cert_t* register_drep_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REGISTER_DREP_CERT_H