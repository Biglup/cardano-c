/**
 * \file update_drep_cert.h
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

#ifndef UPDATE_DREP_CERT_H
#define UPDATE_DREP_CERT_H

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
 * \brief Updates the DRep anchored metadata.
 */
typedef struct cardano_update_drep_cert_t cardano_update_drep_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new update drep certificate.
 *
 * This function allocates and initializes a new update drep certificate, which is used to propose updates to the drep.
 *
 * \param[in] credential A pointer to an initialized cardano_credential_t object representing the credential.
 * \param[in] anchor A pointer to an initialized cardano_anchor_t object that specifies the context or the anchor point for this update.
 * \param[out] update_drep_cert A pointer to a pointer where the newly created cardano_update_drep_cert_t object will be stored.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the certificate was successfully created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...; // Assume initialized
 * cardano_anchor_t* anchor = ...; // Assume initialized
 * cardano_update_drep_cert_t* update_drep_cert = NULL;
 * cardano_error_t result = cardano_update_drep_cert_new(credential, anchor, &update_drep_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the update_drep_cert
 *   // Free resources when done
 *   cardano_update_drep_cert_unref(&update_drep_cert);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_drep_cert_new(
  cardano_credential_t*        credential,
  cardano_anchor_t*            anchor,
  cardano_update_drep_cert_t** update_drep_cert);

/**
 * \brief Creates a \ref cardano_update_drep_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_update_drep_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a update_drep_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] update_drep_cert A pointer to a pointer of \ref cardano_update_drep_cert_t that will be set to the address
 *                        of the newly created update_drep_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_update_drep_cert_t object by calling
 *       \ref cardano_update_drep_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_update_drep_cert_t* update_drep_cert = NULL;
 *
 * cardano_error_t result = cardano_update_drep_cert_from_cbor(reader, &update_drep_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the update_drep_cert
 *
 *   // Once done, ensure to clean up and release the update_drep_cert
 *   cardano_update_drep_cert_unref(&update_drep_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode update_drep_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_drep_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_update_drep_cert_t** update_drep_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_update_drep_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] update_drep_cert A constant pointer to the \ref cardano_update_drep_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p update_drep_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_drep_cert_t* update_drep_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_update_drep_cert_to_cbor(update_drep_cert, writer);
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
 * cardano_update_drep_cert_unref(&update_drep_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_update_drep_cert_to_cbor(
  const cardano_update_drep_cert_t* update_drep_cert,
  cardano_cbor_writer_t*            writer);

/**
 * \brief Sets the drep credential in the certificate.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_update_drep_cert_t object to which the credential will be set.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the credential.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the credential was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_update_drep_cert_set_credential(certificate, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The credential is now set for the certificate
 * }
 * else
 * {
 *   printf("Failed to set the credential.\n");
 * }
 * // Both objects need to be managed and eventually unreferenced by the caller
 * cardano_credential_unref(&credential);
 * cardano_update_drep_cert_unref(&certificate);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_update_drep_cert_set_credential(cardano_update_drep_cert_t* certificate, cardano_credential_t* credential);

/**
 * \brief Gets the credential from an update DRep certificate.
 *
 * This function retrieves the credential from a given update DRep certificate. The caller is responsible for
 * unreferencing the credential object after use.
 *
 * \param[in] certificate A pointer to an initialized cardano_update_drep_cert_t object representing the update DRep certificate.
 *
 * \return A pointer to the retrieved cardano_credential_t object. This will be a new reference.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_drep_cert_t* certificate = ...; // Assume initialized
 * cardano_credential_t* credential = cardano_update_drep_cert_get_credential(certificate);
 *
 * if (credential != NULL)
 * {
 *   // Use the credential
 *
 *   // When done, release the credential
 *   cardano_credential_unref(credential);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t*
cardano_update_drep_cert_get_credential(cardano_update_drep_cert_t* certificate);

/**
 * \brief Retrieves the anchor associated with the certificate.
 *
 * This function returns the anchor used in a \ref cardano_update_drep_cert_t object.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_update_drep_cert_t object.
 *
 * \return A pointer to a \ref cardano_anchor_t object representing the anchor. If the certificate is NULL or does not have an anchor set,
 *         this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_update_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * const cardano_anchor_t* anchor = cardano_update_drep_cert_get_anchor(certificate);
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
CARDANO_EXPORT cardano_anchor_t* cardano_update_drep_cert_get_anchor(cardano_update_drep_cert_t* certificate);

/**
 * \brief Sets the anchor for the certificate.
 *
 * This function assigns a new anchor to a given \ref cardano_update_drep_cert_t object.
 *
 * \param[in] certificate A pointer to an initialized \ref cardano_update_drep_cert_t object to which the anchor will be set.
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
 * cardano_update_drep_cert_t* certificate = ...; // Assume certificate is already initialized
 * cardano_anchor_t* new_anchor = ...; // Assume new_anchor is already initialized
 *
 * cardano_error_t result = cardano_update_drep_cert_set_anchor(certificate, new_anchor);
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
cardano_update_drep_cert_set_anchor(cardano_update_drep_cert_t* certificate, cardano_anchor_t* anchor);

/**
 * \brief Decrements the reference count of a cardano_update_drep_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_update_drep_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the update_drep_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] update_drep_cert A pointer to the pointer of the update_drep_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_update_drep_cert_t* update_drep_cert = cardano_update_drep_cert_new(major, minor);
 *
 * // Perform operations with the update_drep_cert...
 *
 * cardano_update_drep_cert_unref(&update_drep_cert);
 * // At this point, update_drep_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_update_drep_cert_unref, the pointer to the \ref cardano_update_drep_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_update_drep_cert_unref(cardano_update_drep_cert_t** update_drep_cert);

/**
 * \brief Increases the reference count of the cardano_update_drep_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_update_drep_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_update_drep_cert_unref.
 *
 * \param update_drep_cert A pointer to the cardano_update_drep_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming update_drep_cert is a previously created update_drep_cert object
 *
 * cardano_update_drep_cert_ref(update_drep_cert);
 *
 * // Now update_drep_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_update_drep_cert_ref there is a corresponding
 * call to \ref cardano_update_drep_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_update_drep_cert_ref(cardano_update_drep_cert_t* update_drep_cert);

/**
 * \brief Retrieves the current reference count of the cardano_update_drep_cert_t object.
 *
 * This function returns the number of active references to an cardano_update_drep_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_update_drep_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param update_drep_cert A pointer to the cardano_update_drep_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_update_drep_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_update_drep_cert_ref call is matched with a
 * \ref cardano_update_drep_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming update_drep_cert is a previously created update_drep_cert object
 *
 * size_t ref_count = cardano_update_drep_cert_refcount(update_drep_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_update_drep_cert_refcount(const cardano_update_drep_cert_t* update_drep_cert);

/**
 * \brief Sets the last error message for a given cardano_update_drep_cert_t object.
 *
 * Records an error message in the update_drep_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] update_drep_cert A pointer to the \ref cardano_update_drep_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the update_drep_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_update_drep_cert_set_last_error(
  cardano_update_drep_cert_t* update_drep_cert,
  const char*                 message);

/**
 * \brief Retrieves the last error message recorded for a specific update_drep_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_update_drep_cert_set_last_error for the given
 * update_drep_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] update_drep_cert A pointer to the \ref cardano_update_drep_cert_t instance whose last error
 *                   message is to be retrieved. If the update_drep_cert is NULL, the function
 *                   returns a generic error message indicating the null update_drep_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified update_drep_cert. If the update_drep_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_update_drep_cert_set_last_error for the same update_drep_cert, or until
 *       the update_drep_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_update_drep_cert_get_last_error(
  const cardano_update_drep_cert_t* update_drep_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // UPDATE_DREP_CERT_H