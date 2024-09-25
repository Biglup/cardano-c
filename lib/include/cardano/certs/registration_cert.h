/**
 * \file registration_cert.h
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

#ifndef REGISTRATION_CERT_H
#define REGISTRATION_CERT_H

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
 * \brief This certificate is used when an individual wants to register as a stakeholder.
 * It allows the holder to participate in the staking process by delegating their
 * stake or creating a stake pool.
 *
 * This certificate also provides the ability to specify the deposit amount.
 *
 * Deposit must match the expected deposit amount specified by `ppKeyDepositL` in
 * the protocol parameters.
 *
 * \remark Replaces the deprecated `StakeRegistration` in after Conway era.
 */
typedef struct cardano_registration_cert_t cardano_registration_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new registration certificate.
 *
 * This function allocates and initializes a new instance of \ref cardano_registration_cert_t,
 * which represents a registration certificate in the Cardano blockchain. This certificate is used
 * to register a stake credential, along with specifying the required deposit.
 *
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the stake credential.
 * \param[in] deposit The amount of the deposit that must match the expected deposit amount specified by the
 *                    protocol parameters (ppKeyDepositL).
 * \param[out] registration_cert On successful initialization, this will point to a newly created
 *                               \ref cardano_registration_cert_t object. The caller is responsible for managing
 *                               the lifecycle of this object. Specifically, once the registration certificate
 *                               is no longer needed, the caller must release it by calling \ref cardano_registration_cert_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the registration certificate was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 * uint64_t deposit = 5000000; // Deposit amount in lovelace
 *
 * cardano_registration_cert_t* registration_cert = NULL;
 * cardano_error_t result = cardano_registration_cert_new(credential, deposit, &registration_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The registration certificate is now ready to be used
 *   // Remember to free the certificate when done
 *   cardano_registration_cert_unref(&registration_cert);
 * }
 * else
 * {
 *   printf("Failed to create registration certificate: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_registration_cert_new(
  cardano_credential_t*         credential,
  uint64_t                      deposit,
  cardano_registration_cert_t** registration_cert);

/**
 * \brief Creates a \ref cardano_registration_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_registration_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a registration_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] registration_cert A pointer to a pointer of \ref cardano_registration_cert_t that will be set to the address
 *                        of the newly created registration_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_registration_cert_t object by calling
 *       \ref cardano_registration_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_registration_cert_t* registration_cert = NULL;
 *
 * cardano_error_t result = cardano_registration_cert_from_cbor(reader, &registration_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the registration_cert
 *
 *   // Once done, ensure to clean up and release the registration_cert
 *   cardano_registration_cert_unref(&registration_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode registration_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_registration_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_registration_cert_t** registration_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_registration_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] registration_cert A constant pointer to the \ref cardano_registration_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p registration_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_registration_cert_t* registration_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_registration_cert_to_cbor(registration_cert, writer);
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
 * cardano_registration_cert_unref(&registration_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_registration_cert_to_cbor(
  const cardano_registration_cert_t* registration_cert,
  cardano_cbor_writer_t*             writer);

/**
 * \brief Retrieves the stake credential from a registration certificate.
 *
 * This function extracts the stake credential associated with a given \ref cardano_registration_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_registration_cert_t object from which
 *                        the stake credential will be retrieved.
 *
 * \return A pointer to the \ref cardano_credential_t object containing the stake credential. If the input
 *         certificate is NULL, or if the stake credential is not set, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_registration_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_credential_t* credential = cardano_registration_cert_get_stake_credential(certificate);
 *
 * if (credential != NULL)
 * {
 *   // Process the stake credential
 * }
 * else
 * {
 *   printf("No stake credential is set in the certificate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_registration_cert_get_stake_credential(cardano_registration_cert_t* certificate);

/**
 * \brief Sets the stake credential for a registration certificate.
 *
 * This function assigns a stake credential to a given \ref cardano_registration_cert_t object.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_registration_cert_t object to which the stake credential will be set.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the stake credential.
 *                        This function increments the reference count of the credential, so it remains valid even after the caller releases its own reference.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the stake credential was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_registration_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_registration_cert_set_stake_credential(certificate, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The stake credential is now set for the certificate
 * }
 * else
 * {
 *   printf("Failed to set the stake credential: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up: Release references when they are no longer needed
 * cardano_credential_unref(&credential);
 * \endcode
 *
 * \note The caller must manage the lifecycle of both the certificate and the newly referenced credential.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_registration_cert_set_stake_credential(cardano_registration_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Retrieves the deposit amount from a registration certificate.
 *
 * This function extracts the deposit amount specified in a \ref cardano_registration_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_registration_cert_t object.
 *
 * \return The deposit amount associated with the registration certificate. If the certificate pointer is NULL,
 *         the behavior is undefined, and it may cause a crash. It is the caller's responsibility to ensure
 *         that the certificate is properly initialized.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_registration_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = cardano_registration_cert_get_deposit(certificate);
 *
 * printf("The registration deposit is: %llu ADA\n", deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_registration_cert_get_deposit(const cardano_registration_cert_t* certificate);

/**
 * \brief Sets the deposit amount for a registration certificate.
 *
 * This function updates the deposit amount in a \ref cardano_registration_cert_t object. The deposit is required when
 * registering a new stake credential and is specified by the protocol parameters.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_registration_cert_t object to which the deposit will be set.
 * \param[in] deposit The deposit amount to be set in lovelaces (1 ADA = 1,000,000 lovelaces).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the deposit was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the certificate pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_registration_cert_t* certificate = ...; // Assume certificate is already initialized
 * uint64_t deposit = 2000000; // Example deposit amount
 *
 * cardano_error_t result = cardano_registration_cert_set_deposit(certificate, deposit);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Deposit successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set deposit.\n");
 * }
 * \endcode
 *
 * \note It is the caller's responsibility to ensure that the certificate is properly initialized before calling this function.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_registration_cert_set_deposit(cardano_registration_cert_t* certificate, uint64_t deposit);

/**
 * \brief Decrements the reference count of a cardano_registration_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_registration_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the registration_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] registration_cert A pointer to the pointer of the registration_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_registration_cert_t* registration_cert = cardano_registration_cert_new(major, minor);
 *
 * // Perform operations with the registration_cert...
 *
 * cardano_registration_cert_unref(&registration_cert);
 * // At this point, registration_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_registration_cert_unref, the pointer to the \ref cardano_registration_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_registration_cert_unref(cardano_registration_cert_t** registration_cert);

/**
 * \brief Increases the reference count of the cardano_registration_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_registration_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_registration_cert_unref.
 *
 * \param registration_cert A pointer to the cardano_registration_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming registration_cert is a previously created registration_cert object
 *
 * cardano_registration_cert_ref(registration_cert);
 *
 * // Now registration_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_registration_cert_ref there is a corresponding
 * call to \ref cardano_registration_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_registration_cert_ref(cardano_registration_cert_t* registration_cert);

/**
 * \brief Retrieves the current reference count of the cardano_registration_cert_t object.
 *
 * This function returns the number of active references to an cardano_registration_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_registration_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param registration_cert A pointer to the cardano_registration_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_registration_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_registration_cert_ref call is matched with a
 * \ref cardano_registration_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming registration_cert is a previously created registration_cert object
 *
 * size_t ref_count = cardano_registration_cert_refcount(registration_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_registration_cert_refcount(const cardano_registration_cert_t* registration_cert);

/**
 * \brief Sets the last error message for a given cardano_registration_cert_t object.
 *
 * Records an error message in the registration_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] registration_cert A pointer to the \ref cardano_registration_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the registration_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_registration_cert_set_last_error(
  cardano_registration_cert_t* registration_cert,
  const char*                  message);

/**
 * \brief Retrieves the last error message recorded for a specific registration_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_registration_cert_set_last_error for the given
 * registration_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] registration_cert A pointer to the \ref cardano_registration_cert_t instance whose last error
 *                   message is to be retrieved. If the registration_cert is NULL, the function
 *                   returns a generic error message indicating the null registration_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified registration_cert. If the registration_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_registration_cert_set_last_error for the same registration_cert, or until
 *       the registration_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_registration_cert_get_last_error(
  const cardano_registration_cert_t* registration_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REGISTRATION_CERT_H