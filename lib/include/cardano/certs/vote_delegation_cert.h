/**
 * \file vote_delegation_cert.h
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

#ifndef VOTE_DELEGATION_CERT_H
#define VOTE_DELEGATION_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/common/drep.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This certificate is used when an individual wants to delegate their voting rights to any other DRep.
 */
typedef struct cardano_vote_delegation_cert_t cardano_vote_delegation_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new vote delegation certificate.
 *
 * This function allocates and initializes a new vote delegation certificate, which is used to delegate the voting power
 * of a stakeholder to a drep.
 *
 * \param[in] credential A pointer to an initialized cardano_credential_t object representing the stakeholder's credential.
 * \param[in] drep A pointer to an initialized cardano_drep_t object representing the delegation representative.
 * \param[out] vote_delegation A pointer to a pointer where the newly created cardano_vote_delegation_cert_t object will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was successfully created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...; // Assume initialized
 * cardano_drep_t* drep = ...; // Assume initialized
 * cardano_vote_delegation_cert_t* vote_delegation = NULL;
 * cardano_error_t result = cardano_vote_delegation_cert_new(credential, drep, &vote_delegation);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     // Use the vote_delegation
 *     // Free resources when done
 *     cardano_vote_delegation_cert_unref(&vote_delegation);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_vote_delegation_cert_new(
  cardano_credential_t*            credential,
  cardano_drep_t*                  drep,
  cardano_vote_delegation_cert_t** vote_delegation);

/**
 * \brief Creates a \ref cardano_vote_delegation_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_vote_delegation_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a vote_delegation_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] vote_delegation_cert A pointer to a pointer of \ref cardano_vote_delegation_cert_t that will be set to the address
 *                        of the newly created vote_delegation_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_vote_delegation_cert_t object by calling
 *       \ref cardano_vote_delegation_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_vote_delegation_cert_t* vote_delegation_cert = NULL;
 *
 * cardano_error_t result = cardano_vote_delegation_cert_from_cbor(reader, &vote_delegation_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the vote_delegation_cert
 *
 *   // Once done, ensure to clean up and release the vote_delegation_cert
 *   cardano_vote_delegation_cert_unref(&vote_delegation_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode vote_delegation_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_vote_delegation_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_vote_delegation_cert_t** vote_delegation_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_vote_delegation_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] vote_delegation_cert A constant pointer to the \ref cardano_vote_delegation_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p vote_delegation_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vote_delegation_cert_t* vote_delegation_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_vote_delegation_cert_to_cbor(vote_delegation_cert, writer);
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
 * cardano_vote_delegation_cert_unref(&vote_delegation_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_vote_delegation_cert_to_cbor(
  const cardano_vote_delegation_cert_t* vote_delegation_cert,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Retrieves the stake credential from a vote delegation certificate.
 *
 * This function extracts the stake credential from a given \ref cardano_vote_delegation_cert_t object.
 * The credential identifies the stakeholder in the context of the Cardano network.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_vote_delegation_cert_t object.
 *
 * \return A pointer to a \ref cardano_credential_t object containing the stake credential. If the input certificate is NULL,
 *         or if the credential is not set, this function returns NULL.
 *
 * \note The returned \ref cardano_credential_t object is a new reference and must be managed by the caller.
 *       It should be released using \ref cardano_credential_unref when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_vote_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = cardano_vote_delegation_cert_get_credential(certificate);
 *
 * if (credential != NULL)
 * {
 *   // Process the credential
 *   // Remember to release the credential reference when done
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   printf("Failed to retrieve credential or certificate is not initialized properly.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_vote_delegation_cert_get_credential(cardano_vote_delegation_cert_t* certificate);

/**
 * \brief Sets the stake credential for a vote delegation certificate.
 *
 * This function assigns a stake credential to a given \ref cardano_vote_delegation_cert_t object.
 * The credential identifies the stakeholder in the context of the Cardano network.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_vote_delegation_cert_t object
 *                            where the stake credential will be set.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the stake credential.
 *                       This function increments the reference count of the credential, and the caller remains responsible
 *                       for releasing their own reference.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the stake credential
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vote_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_vote_delegation_cert_set_credential(certificate, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Credential successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set the credential: %s\n", cardano_error_to_string(result));
 * }
 *
 * \endcode
 *
 * \note After setting, the certificate holds its own reference to the credential. The caller
 *       must manage and release its own references to the credential object.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_vote_delegation_cert_set_credential(cardano_vote_delegation_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Retrieves the DRep associated with a vote delegation certificate.
 *
 * This function extracts the DRep from the specified \ref cardano_vote_delegation_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_vote_delegation_cert_t object.
 *
 * \return A pointer to a new \ref cardano_drep_t object containing the DRep. The caller is responsible
 *         for managing the lifecycle of this returned DRep object, including releasing it using \ref cardano_drep_unref
 *         when it is no longer needed. If the certificate is NULL or if the DRep is not set, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_vote_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_drep_t* drep = cardano_vote_delegation_cert_get_drep(certificate);
 *
 * if (drep != NULL)
 * {
 *   // Process the DRep
 *   // Remember to free the DRep when done
 *   cardano_drep_unref(&drep);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_drep_t* cardano_vote_delegation_cert_get_drep(cardano_vote_delegation_cert_t* certificate);

/**
 * \brief Sets the DRep for a vote delegation certificate.
 *
 * This function assigns a DRep to the specified \ref cardano_vote_delegation_cert_t object.
 * The DRep is essential for representing delegate participation in the governance process within the Cardano network.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_vote_delegation_cert_t object which will receive the DRep.
 * \param[in] drep A pointer to an initialized \ref cardano_drep_t object to be set on the certificate. This function increments the reference
 *                 count of the DRep, and the caller is still responsible for releasing their reference to the DRep object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the DRep was successfully set,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vote_delegation_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_drep_t* drep = ...; // Assume drep is already initialized
 *
 * cardano_error_t result = cardano_vote_delegation_cert_set_drep(certificate, drep);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The DRep is now set for the certificate
 * }
 * else
 * {
 *   printf("Failed to set the DRep.\n");
 * }
 *
 * // Both certificate and drep need their references managed appropriately
 * cardano_drep_unref(&drep);
 * \endcode
 *
 * \note This function increments the reference count of the drep argument, the caller must manage their own reference to it.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_vote_delegation_cert_set_drep(cardano_vote_delegation_cert_t* certificate, cardano_drep_t* drep);

/**
 * \brief Decrements the reference count of a cardano_vote_delegation_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_vote_delegation_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the vote_delegation_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] vote_delegation_cert A pointer to the pointer of the vote_delegation_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vote_delegation_cert_t* vote_delegation_cert = cardano_vote_delegation_cert_new(major, minor);
 *
 * // Perform operations with the vote_delegation_cert...
 *
 * cardano_vote_delegation_cert_unref(&vote_delegation_cert);
 * // At this point, vote_delegation_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_vote_delegation_cert_unref, the pointer to the \ref cardano_vote_delegation_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_vote_delegation_cert_unref(cardano_vote_delegation_cert_t** vote_delegation_cert);

/**
 * \brief Increases the reference count of the cardano_vote_delegation_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_vote_delegation_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_vote_delegation_cert_unref.
 *
 * \param vote_delegation_cert A pointer to the cardano_vote_delegation_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming vote_delegation_cert is a previously created vote_delegation_cert object
 *
 * cardano_vote_delegation_cert_ref(vote_delegation_cert);
 *
 * // Now vote_delegation_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_vote_delegation_cert_ref there is a corresponding
 * call to \ref cardano_vote_delegation_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_vote_delegation_cert_ref(cardano_vote_delegation_cert_t* vote_delegation_cert);

/**
 * \brief Retrieves the current reference count of the cardano_vote_delegation_cert_t object.
 *
 * This function returns the number of active references to an cardano_vote_delegation_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_vote_delegation_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param vote_delegation_cert A pointer to the cardano_vote_delegation_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_vote_delegation_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_vote_delegation_cert_ref call is matched with a
 * \ref cardano_vote_delegation_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming vote_delegation_cert is a previously created vote_delegation_cert object
 *
 * size_t ref_count = cardano_vote_delegation_cert_refcount(vote_delegation_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_vote_delegation_cert_refcount(const cardano_vote_delegation_cert_t* vote_delegation_cert);

/**
 * \brief Sets the last error message for a given cardano_vote_delegation_cert_t object.
 *
 * Records an error message in the vote_delegation_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] vote_delegation_cert A pointer to the \ref cardano_vote_delegation_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the vote_delegation_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_vote_delegation_cert_set_last_error(
  cardano_vote_delegation_cert_t* vote_delegation_cert,
  const char*                     message);

/**
 * \brief Retrieves the last error message recorded for a specific vote_delegation_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_vote_delegation_cert_set_last_error for the given
 * vote_delegation_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] vote_delegation_cert A pointer to the \ref cardano_vote_delegation_cert_t instance whose last error
 *                   message is to be retrieved. If the vote_delegation_cert is NULL, the function
 *                   returns a generic error message indicating the null vote_delegation_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified vote_delegation_cert. If the vote_delegation_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_vote_delegation_cert_set_last_error for the same vote_delegation_cert, or until
 *       the vote_delegation_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_vote_delegation_cert_get_last_error(
  const cardano_vote_delegation_cert_t* vote_delegation_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // VOTE_DELEGATION_CERT_H