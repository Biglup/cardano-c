/**
 * \file auth_committee_hot_cert.h
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

#ifndef AUTH_COMMITTEE_HOT_CERT_H
#define AUTH_COMMITTEE_HOT_CERT_H

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
 * \brief Constitutional Committee members maintain operational integrity by managing two credentials: a cold credential and a hot credential.
 *
 * One of the purposes of this credential management system is to provide multiple layers of security to help committee members prevent losing control
 * over a cold credential and to give good options for recovery if necessary.
 *
 * This certificates registers the Hot credentials of a committee member and requires a signature from the CC member Cold key.
 */
typedef struct cardano_auth_committee_hot_cert_t cardano_auth_committee_hot_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates and initializes a new instance of an authorization committee hot certificate.
 *
 * This function allocates and initializes a new instance of \ref cardano_auth_committee_hot_cert_t.
 *
 * \param[in] committee_cold_cred A \ref cardano_credential_t object representing the committee cold credential.
 * \param[in] committee_hot_cred A \ref cardano_credential_t object representing the committee hot credential.
 * \param[out] auth_committee_hot_cert On successful initialization, this will point to a newly created
 *                                             \ref cardano_auth_committee_hot_cert_t object. The caller is responsible
 *                                             for managing the lifecycle of this object. Specifically, once the authorization
 *                                             committee hot certificate is no longer needed, the caller must release it
 *                                             by calling \ref cardano_auth_committee_hot_cert_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the authorization committee hot certificate was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t committee_cold_cred = ...; // Assume committee_cold_cred is initialized
 * cardano_credential_t committee_hot_cred = ...; // Assume committee_hot_cred is initialized
 * cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
 *
 * // Attempt to create a new authorization committee hot certificate
 * cardano_error_t result = cardano_auth_committee_hot_cert_new(committee_cold_cred, committee_hot_cred, &auth_committee_hot_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the auth_committee_hot_cert
 *
 *   // Once done, ensure to clean up and release the auth_committee_hot_cert
 *   cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auth_committee_hot_cert_new(
  cardano_credential_t*               committee_cold_cred,
  cardano_credential_t*               committee_hot_cred,
  cardano_auth_committee_hot_cert_t** auth_committee_hot_cert);

/**
 * \brief Creates a \ref cardano_auth_committee_hot_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_auth_committee_hot_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a auth_committee_hot.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] auth_committee_hot A pointer to a pointer of \ref cardano_auth_committee_hot_cert_t that will be set to the address
 *                        of the newly created auth_committee_hot object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_auth_committee_hot_cert_t object by calling
 *       \ref cardano_auth_committee_hot_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_auth_committee_hot_cert_t* auth_committee_hot = NULL;
 *
 * cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the auth_committee_hot
 *
 *   // Once done, ensure to clean up and release the auth_committee_hot
 *   cardano_auth_committee_hot_cert_unref(&auth_committee_hot);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode auth_committee_hot: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auth_committee_hot_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_auth_committee_hot_cert_t** auth_committee_hot);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_auth_committee_hot_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] auth_committee_hot A constant pointer to the \ref cardano_auth_committee_hot_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p auth_committee_hot or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auth_committee_hot_cert_t* auth_committee_hot = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_auth_committee_hot_cert_to_cbor(auth_committee_hot, writer);
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
 * cardano_auth_committee_hot_cert_unref(&auth_committee_hot);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_auth_committee_hot_cert_to_cbor(
  const cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_cbor_writer_t*                   writer);

/**
 * \brief Sets the cold credential for an authorization committee hot certificate.
 *
 * This function assigns a new cold credential to a given \ref cardano_auth_committee_hot_cert_t object.
 *
 * \param[in] auth_committee_hot A pointer to an initialized \ref cardano_auth_committee_hot_cert_t object to which the cold credential will be set.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the new cold credential.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the cold credential was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auth_committee_hot_cert_t* auth_committee_hot = ...; // Assume auth_committee_hot is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_auth_committee_hot_cert_set_cold_cred(auth_committee_hot, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The cold credential is now set for the auth_committee_hot
 * }
 * else
 * {
 *   printf("Failed to set the cold credential.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_auth_committee_hot_cert_set_cold_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t*              credential);

/**
 * \brief Retrieves the cold credential from an authorization committee hot certificate.
 *
 * This function retrieves the cold credential from a given \ref cardano_auth_committee_hot_cert_t object.
 *
 * \param[in] auth_committee_hot A pointer to an initialized \ref cardano_auth_committee_hot_cert_t object.
 * \param[out] credential On successful retrieval, this will point to the cold credential of the auth_committee_hot.
 *                        The caller is responsible for managing the lifecycle of this object and must call the appropriate
 *                        unref function to release it when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the cold credential was successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note This function returns a new reference to the cold credential. Both the source auth_committee_hot object
 *       and the retrieved credential must be unreferenced appropriately.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auth_committee_hot_cert_t* auth_committee_hot = ...; // Assume auth_committee_hot is already initialized
 * cardano_credential_t* credential = NULL;
 *
 * cardano_error_t result = cardano_auth_committee_hot_cert_get_cold_cred(auth_committee_hot, &credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   printf("Failed to retrieve the cold credential.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_auth_committee_hot_cert_get_cold_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t**             credential);

/**
 * \brief Sets the hot credential for an authorization committee hot certificate.
 *
 * This function assigns a new hot credential to a given \ref cardano_auth_committee_hot_cert_t object.
 *
 * \param[in] auth_committee_hot A pointer to an initialized \ref cardano_auth_committee_hot_cert_t object.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the hot credential.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hot credential was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auth_committee_hot_cert_t* auth_committee_hot = ...; // Assume auth_committee_hot is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 *
 * cardano_error_t result = cardano_auth_committee_hot_cert_set_hot_cred(auth_committee_hot, credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The hot credential is now set for the auth_committee_hot
 * }
 * else
 * {
 *   printf("Failed to set the hot credential.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_auth_committee_hot_cert_set_hot_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t*              credential);

/**
 * \brief Retrieves the hot credential from an authorization committee hot certificate.
 *
 * This function retrieves the hot credential from a given \ref cardano_auth_committee_hot_cert_t object.
 *
 * \param[in] auth_committee_hot A pointer to an initialized \ref cardano_auth_committee_hot_cert_t object.
 * \param[out] credential On successful retrieval, this will point to the hot credential of the auth_committee_hot object.
 *                        The function returns a new reference to the credential, and the caller is responsible for managing
 *                        its lifecycle, including calling \ref cardano_credential_unref when the credential is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hot credential was successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note This function returns a new reference to the hot credential. Both the caller and the auth_committee_hot object
 *       must unreference the credential appropriately to avoid memory leaks.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auth_committee_hot_cert_t* auth_committee_hot = ...; // Assume auth_committee_hot is already initialized
 * cardano_credential_t* credential = NULL;
 *
 * cardano_error_t result = cardano_auth_committee_hot_cert_get_hot_cred(auth_committee_hot, &credential);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the hot credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   printf("Failed to retrieve the hot credential.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_auth_committee_hot_cert_get_hot_cred(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  cardano_credential_t**             credential);

/**
 * \brief Decrements the reference count of a cardano_auth_committee_hot_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_auth_committee_hot_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the auth_committee_hot is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] auth_committee_hot A pointer to the pointer of the auth_committee_hot object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auth_committee_hot_cert_t* auth_committee_hot = cardano_auth_committee_hot_cert_new(major, minor);
 *
 * // Perform operations with the auth_committee_hot...
 *
 * cardano_auth_committee_hot_cert_unref(&auth_committee_hot);
 * // At this point, auth_committee_hot is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_auth_committee_hot_cert_unref, the pointer to the \ref cardano_auth_committee_hot_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_auth_committee_hot_cert_unref(cardano_auth_committee_hot_cert_t** auth_committee_hot);

/**
 * \brief Increases the reference count of the cardano_auth_committee_hot_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_auth_committee_hot_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_auth_committee_hot_cert_unref.
 *
 * \param auth_committee_hot A pointer to the cardano_auth_committee_hot_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * cardano_auth_committee_hot_cert_ref(auth_committee_hot);
 *
 * // Now auth_committee_hot can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_auth_committee_hot_cert_ref there is a corresponding
 * call to \ref cardano_auth_committee_hot_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_auth_committee_hot_cert_ref(cardano_auth_committee_hot_cert_t* auth_committee_hot);

/**
 * \brief Retrieves the current reference count of the cardano_auth_committee_hot_cert_t object.
 *
 * This function returns the number of active references to an cardano_auth_committee_hot_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_auth_committee_hot_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param auth_committee_hot A pointer to the cardano_auth_committee_hot_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_auth_committee_hot_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_auth_committee_hot_cert_ref call is matched with a
 * \ref cardano_auth_committee_hot_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * size_t ref_count = cardano_auth_committee_hot_cert_refcount(auth_committee_hot);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_auth_committee_hot_cert_refcount(const cardano_auth_committee_hot_cert_t* auth_committee_hot);

/**
 * \brief Sets the last error message for a given cardano_auth_committee_hot_cert_t object.
 *
 * Records an error message in the auth_committee_hot's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_auth_committee_hot_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the auth_committee_hot's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_auth_committee_hot_cert_set_last_error(
  cardano_auth_committee_hot_cert_t* auth_committee_hot,
  const char*                        message);

/**
 * \brief Retrieves the last error message recorded for a specific auth_committee_hot.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_auth_committee_hot_cert_set_last_error for the given
 * auth_committee_hot. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_auth_committee_hot_cert_t instance whose last error
 *                   message is to be retrieved. If the auth_committee_hot is NULL, the function
 *                   returns a generic error message indicating the null auth_committee_hot.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified auth_committee_hot. If the auth_committee_hot is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_auth_committee_hot_cert_set_last_error for the same auth_committee_hot, or until
 *       the auth_committee_hot is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_auth_committee_hot_cert_get_last_error(
  const cardano_auth_committee_hot_cert_t* auth_committee_hot);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // AUTH_COMMITTEE_HOT_CERT_H