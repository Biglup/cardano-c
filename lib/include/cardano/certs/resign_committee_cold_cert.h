/**
 * \file resign_committee_cold_cert.h
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

#ifndef RESIGN_COMMITTEE_COLD_CERT_H
#define RESIGN_COMMITTEE_COLD_CERT_H

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
 * \brief This certificate is used then a committee member wants to resign early (will be marked on-chain as an expired member).
 */
typedef struct cardano_resign_committee_cold_cert_t cardano_resign_committee_cold_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new resignation certificate for a committee cold key.
 *
 * This function allocates and initializes a new instance of \ref cardano_resign_committee_cold_cert_t.
 *
 * \param[in] committee_cold_cred A pointer to an initialized \ref cardano_credential_t object representing the committee's cold credential.
 * \param[in] anchor A pointer to an initialized \ref cardano_anchor_t object associated with the resignation.
 * \param[out] resign_committee_cold_cert On successful execution, this will point to a newly created \ref cardano_resign_committee_cold_cert_t object.
 *                                        The caller is responsible for managing the lifecycle of this object, including releasing it
 *                                        when it is no longer needed using the appropriate unref function.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate was
 *         successfully created, or an error code indicating the reason for failure (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL).
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* committee_cold_cred = ...; // Assume committee_cold_cred is already initialized
 * cardano_anchor_t* anchor = ...; // Assume anchor is already initialized
 * cardano_resign_committee_cold_cert_t* resign_committee_cold_cert = NULL;
 *
 * cardano_error_t result = cardano_resign_committee_cold_cert_new(committee_cold_cred, anchor, &resign_committee_cold_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The resign committee cold certificate can now be used for transaction signing or other purposes
 *   // Remember to free the certificate when done
 *   cardano_resign_committee_cold_cert_unref(&resign_committee_cold_cert);
 * }
 * else
 * {
 *   printf("Failed to create resign committee cold certificate: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_resign_committee_cold_cert_new(
  cardano_credential_t*                  committee_cold_cred,
  cardano_anchor_t*                      anchor,
  cardano_resign_committee_cold_cert_t** resign_committee_cold_cert);

/**
 * \brief Creates a \ref cardano_resign_committee_cold_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_resign_committee_cold_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a resign_committee_cold.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] resign_committee_cold A pointer to a pointer of \ref cardano_resign_committee_cold_cert_t that will be set to the address
 *                        of the newly created resign_committee_cold object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_resign_committee_cold_cert_t object by calling
 *       \ref cardano_resign_committee_cold_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_resign_committee_cold_cert_t* resign_committee_cold = NULL;
 *
 * cardano_error_t result = cardano_resign_committee_cold_cert_from_cbor(reader, &resign_committee_cold);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the resign_committee_cold
 *
 *   // Once done, ensure to clean up and release the resign_committee_cold
 *   cardano_resign_committee_cold_cert_unref(&resign_committee_cold);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode resign_committee_cold: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_resign_committee_cold_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_resign_committee_cold_cert_t** resign_committee_cold);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_resign_committee_cold_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] resign_committee_cold A constant pointer to the \ref cardano_resign_committee_cold_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p resign_committee_cold or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_resign_committee_cold_cert_t* resign_committee_cold = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_resign_committee_cold_cert_to_cbor(resign_committee_cold, writer);
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
 * cardano_resign_committee_cold_cert_unref(&resign_committee_cold);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_resign_committee_cold_cert_to_cbor(
  const cardano_resign_committee_cold_cert_t* resign_committee_cold,
  cardano_cbor_writer_t*                      writer);

/**
 * \brief Retrieves the committee cold credential from a resignation certificate.
 *
 * This function extracts the committee's cold credential from a \ref cardano_resign_committee_cold_cert_t object.
 * The returned credential is a reference counted object, and the caller is responsible for releasing it
 * using \ref cardano_credential_unref when it is no longer needed.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_resign_committee_cold_cert_t object.
 *
 * \return A pointer to the \ref cardano_credential_t object containing the committee's cold credential.
 *         If the certificate is NULL or if the credential is not set, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_resign_committee_cold_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = cardano_resign_committee_cold_cert_get_credential(certificate);
 *
 * if (credential != NULL)
 * {
 *   // Process the credential
 *   cardano_credential_unref(&credential); // Remember to unref the credential when done
 * }
 * else
 * {
 *   printf("No credential is set for this certificate.\n");
 * }
 * \endcode
 *
 * \note This function increments the reference count of the returned credential. The caller must ensure to unref it to avoid memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_resign_committee_cold_cert_get_credential(cardano_resign_committee_cold_cert_t* certificate);

/**
 * \brief Sets the committee cold credential in a resignation certificate.
 *
 * This function assigns a new committee cold credential to a given \ref cardano_resign_committee_cold_cert_t object.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_resign_committee_cold_cert_t object to which the credential will be set.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the committee's cold credential.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the committee cold credential was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_resign_committee_cold_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_resign_committee_cold_cert_set_credential(certificate, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The credential is now set for the certificate
 * }
 * else
 * {
 *   printf("Failed to set the committee's cold credential.\n");
 * }
 * // Both objects need to be managed and eventually unreferenced by the caller
 * cardano_credential_unref(&credential);
 * cardano_resign_committee_cold_cert_unref(&certificate);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_resign_committee_cold_cert_set_credential(cardano_resign_committee_cold_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Retrieves the anchor associated with a resignation committee cold certificate.
 *
 * This function returns the anchor used in a \ref cardano_resign_committee_cold_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_resign_committee_cold_cert_t object.
 *
 * \return A pointer to a \ref cardano_anchor_t object representing the anchor. If the certificate is NULL or does not have an anchor set,
 *         this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_resign_committee_cold_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_anchor_t* anchor = cardano_resign_committee_cold_cert_get_anchor(certificate);
 *
 * if (anchor != NULL)
 * {
 *   // Process the anchor information
 * }
 * else
 * {
 *   printf("No anchor is set for this certificate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_anchor_t* cardano_resign_committee_cold_cert_get_anchor(cardano_resign_committee_cold_cert_t* certificate);

/**
 * \brief Sets the anchor for a resignation committee cold certificate.
 *
 * This function assigns a new anchor to a given \ref cardano_resign_committee_cold_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_resign_committee_cold_cert_t object to which the anchor will be set.
 * \param[in] anchor A pointer to an initialized \ref cardano_anchor_t object representing the new anchor. This function
 *                    increases the reference count on the anchor, and the certificate will maintain its own reference
 *                    to the anchor.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the anchor
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_resign_committee_cold_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_anchor_t* new_anchor = ...; // Assume new_anchor is already initialized
 *
 * cardano_error_t result = cardano_resign_committee_cold_cert_set_anchor(certificate, new_anchor);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Anchor set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the anchor.\n");
 * }
 *
 * // Both certificate and new_anchor need their own unref calls when no longer needed
 * \endcode
 *
 * \note This function increments the reference count of the anchor object passed to it. It is the caller's responsibility
 *       to manage the lifecycle of both the certificate and the anchor.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_resign_committee_cold_cert_set_anchor(cardano_resign_committee_cold_cert_t* certificate, cardano_anchor_t* anchor);

/**
 * \brief Decrements the reference count of a cardano_resign_committee_cold_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_resign_committee_cold_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the resign_committee_cold is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] resign_committee_cold A pointer to the pointer of the resign_committee_cold object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_resign_committee_cold_cert_t* resign_committee_cold = cardano_resign_committee_cold_cert_new(major, minor);
 *
 * // Perform operations with the resign_committee_cold...
 *
 * cardano_resign_committee_cold_cert_unref(&resign_committee_cold);
 * // At this point, resign_committee_cold is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_resign_committee_cold_cert_unref, the pointer to the \ref cardano_resign_committee_cold_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_resign_committee_cold_cert_unref(cardano_resign_committee_cold_cert_t** resign_committee_cold);

/**
 * \brief Increases the reference count of the cardano_resign_committee_cold_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_resign_committee_cold_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_resign_committee_cold_cert_unref.
 *
 * \param resign_committee_cold A pointer to the cardano_resign_committee_cold_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming resign_committee_cold is a previously created resign_committee_cold object
 *
 * cardano_resign_committee_cold_cert_ref(resign_committee_cold);
 *
 * // Now resign_committee_cold can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_resign_committee_cold_cert_ref there is a corresponding
 * call to \ref cardano_resign_committee_cold_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_resign_committee_cold_cert_ref(cardano_resign_committee_cold_cert_t* resign_committee_cold);

/**
 * \brief Retrieves the current reference count of the cardano_resign_committee_cold_cert_t object.
 *
 * This function returns the number of active references to an cardano_resign_committee_cold_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_resign_committee_cold_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param resign_committee_cold A pointer to the cardano_resign_committee_cold_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_resign_committee_cold_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_resign_committee_cold_cert_ref call is matched with a
 * \ref cardano_resign_committee_cold_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming resign_committee_cold is a previously created resign_committee_cold object
 *
 * size_t ref_count = cardano_resign_committee_cold_cert_refcount(resign_committee_cold);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_resign_committee_cold_cert_refcount(const cardano_resign_committee_cold_cert_t* resign_committee_cold);

/**
 * \brief Sets the last error message for a given cardano_resign_committee_cold_cert_t object.
 *
 * Records an error message in the resign_committee_cold's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] resign_committee_cold A pointer to the \ref cardano_resign_committee_cold_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the resign_committee_cold's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_resign_committee_cold_cert_set_last_error(
  cardano_resign_committee_cold_cert_t* resign_committee_cold,
  const char*                           message);

/**
 * \brief Retrieves the last error message recorded for a specific resign_committee_cold.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_resign_committee_cold_cert_set_last_error for the given
 * resign_committee_cold. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] resign_committee_cold A pointer to the \ref cardano_resign_committee_cold_cert_t instance whose last error
 *                   message is to be retrieved. If the resign_committee_cold is NULL, the function
 *                   returns a generic error message indicating the null resign_committee_cold.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified resign_committee_cold. If the resign_committee_cold is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_resign_committee_cold_cert_set_last_error for the same resign_committee_cold, or until
 *       the resign_committee_cold is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_resign_committee_cold_cert_get_last_error(
  const cardano_resign_committee_cold_cert_t* resign_committee_cold);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // RESIGN_COMMITTEE_COLD_CERT_H