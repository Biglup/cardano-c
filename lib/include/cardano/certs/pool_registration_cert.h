/**
 * \file pool_registration_cert.h
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

#ifndef POOL_REGISTRATION_CERT_H
#define POOL_REGISTRATION_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/pool_params/pool_params.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This certificate is used to register a new stake pool. It includes various details
 * about the pool such as the pledge, costs, margin, reward account, and the pool's owners and relays.
 */
typedef struct cardano_pool_registration_cert_t cardano_pool_registration_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a new pool registration certificate.
 *
 * This function allocates and initializes a new \ref cardano_pool_registration_cert_t object based on the provided pool parameters.
 * A pool registration certificate is used in the Cardano network to register a new stake pool.
 *
 * \param[in] params A pointer to an initialized \ref cardano_pool_params_t object containing the parameters of the pool being registered.
 * \param[out] cardano_pool_registration_cert On successful execution, this will point to a newly created \ref cardano_pool_registration_cert_t object.
 *                                            The caller is responsible for managing the lifecycle of this object using \ref cardano_pool_registration_cert_unref
 *                                            when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the pool registration certificate was
 *         successfully created, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* params = ...; // Assume params is already initialized
 * cardano_pool_registration_cert_t* registration_cert = NULL;
 *
 * cardano_error_t result = cardano_pool_registration_cert_new(params, &registration_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The pool registration certificate can now be used for pool registration
 *   // Remember to free the certificate when done
 *   cardano_pool_registration_cert_unref(&registration_cert);
 * }
 * else
 * {
 *   printf("Failed to create pool registration certificate: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_registration_cert_new(
  cardano_pool_params_t*             params,
  cardano_pool_registration_cert_t** cardano_pool_registration_cert);

/**
 * \brief Creates a \ref cardano_pool_registration_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_pool_registration_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a pool_registration_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] pool_registration_cert A pointer to a pointer of \ref cardano_pool_registration_cert_t that will be set to the address
 *                        of the newly created pool_registration_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_pool_registration_cert_t object by calling
 *       \ref cardano_pool_registration_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_pool_registration_cert_t* pool_registration_cert = NULL;
 *
 * cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_registration_cert
 *
 *   // Once done, ensure to clean up and release the pool_registration_cert
 *   cardano_pool_registration_cert_unref(&pool_registration_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode pool_registration_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_registration_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_registration_cert_t** pool_registration_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_pool_registration_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] pool_registration_cert A constant pointer to the \ref cardano_pool_registration_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p pool_registration_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_registration_cert_t* pool_registration_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_pool_registration_cert_to_cbor(pool_registration_cert, writer);
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
 * cardano_pool_registration_cert_unref(&pool_registration_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_registration_cert_to_cbor(
  const cardano_pool_registration_cert_t* pool_registration_cert,
  cardano_cbor_writer_t*                  writer);

/**
 * \brief Retrieves the pool parameters from a pool registration certificate.
 *
 * This function extracts the pool parameters from the specified \ref cardano_pool_registration_cert_t object.
 * These parameters define the configurations of the stake pool being registered.
 *
 * \param[in] cert A pointer to an initialized \ref cardano_pool_registration_cert_t object from which the pool parameters are to be retrieved.
 * \param[out] params On successful execution, this will point to the \ref cardano_pool_params_t object associated with the pool registration certificate.
 *                    This output provides a reference to the pool parameters, which the caller must manage using \ref cardano_pool_params_unref when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the pool parameters were successfully retrieved,
 *         or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_registration_cert_t* registration_cert = ...; // Assume registration_cert is already initialized
 * cardano_pool_params_t* params = NULL;
 *
 * cardano_error_t result = cardano_pool_registration_cert_get_params(registration_cert, &params);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the pool parameters
 *   // Remember to free the params when done
 *   cardano_pool_params_unref(&params);
 * }
 * else
 * {
 *   printf("Failed to retrieve pool parameters: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_registration_cert_get_params(
  cardano_pool_registration_cert_t* cert,
  cardano_pool_params_t**           params);

/**
 * \brief Sets the parameters for a pool registration certificate.
 *
 * This function sets the pool parameters for an existing pool registration certificate. It updates the certificate
 * to use the provided pool parameters.
 *
 * \param[in] cert A pointer to an initialized cardano_pool_registration_cert_t object representing the pool registration certificate.
 * \param[in] params A pointer to an initialized cardano_pool_params_t object representing the pool parameters to set.
 *
 * \return cardano_error_t indicating the outcome of the operation. Returns CARDANO_SUCCESS if the parameters were successfully set.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_registration_cert_t* cert = ...; // Assume initialized
 * cardano_pool_params_t* params = ...; // Assume initialized
 * cardano_error_t result = cardano_pool_registration_cert_set_params(cert, params);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The pool registration certificate now has the updated parameters
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_registration_cert_set_params(
  cardano_pool_registration_cert_t* cert,
  cardano_pool_params_t*            params);

/**
 * \brief Decrements the reference count of a cardano_pool_registration_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_pool_registration_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the pool_registration_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] pool_registration_cert A pointer to the pointer of the pool_registration_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_registration_cert_t* pool_registration_cert = cardano_pool_registration_cert_new(major, minor);
 *
 * // Perform operations with the pool_registration_cert...
 *
 * cardano_pool_registration_cert_unref(&pool_registration_cert);
 * // At this point, pool_registration_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_pool_registration_cert_unref, the pointer to the \ref cardano_pool_registration_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_pool_registration_cert_unref(cardano_pool_registration_cert_t** pool_registration_cert);

/**
 * \brief Increases the reference count of the cardano_pool_registration_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_pool_registration_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_pool_registration_cert_unref.
 *
 * \param pool_registration_cert A pointer to the cardano_pool_registration_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_registration_cert is a previously created pool_registration_cert object
 *
 * cardano_pool_registration_cert_ref(pool_registration_cert);
 *
 * // Now pool_registration_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_pool_registration_cert_ref there is a corresponding
 * call to \ref cardano_pool_registration_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_pool_registration_cert_ref(cardano_pool_registration_cert_t* pool_registration_cert);

/**
 * \brief Retrieves the current reference count of the cardano_pool_registration_cert_t object.
 *
 * This function returns the number of active references to an cardano_pool_registration_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_pool_registration_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param pool_registration_cert A pointer to the cardano_pool_registration_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_pool_registration_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_pool_registration_cert_ref call is matched with a
 * \ref cardano_pool_registration_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_registration_cert is a previously created pool_registration_cert object
 *
 * size_t ref_count = cardano_pool_registration_cert_refcount(pool_registration_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_registration_cert_refcount(const cardano_pool_registration_cert_t* pool_registration_cert);

/**
 * \brief Sets the last error message for a given cardano_pool_registration_cert_t object.
 *
 * Records an error message in the pool_registration_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] pool_registration_cert A pointer to the \ref cardano_pool_registration_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the pool_registration_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_pool_registration_cert_set_last_error(
  cardano_pool_registration_cert_t* pool_registration_cert,
  const char*                       message);

/**
 * \brief Retrieves the last error message recorded for a specific pool_registration_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_pool_registration_cert_set_last_error for the given
 * pool_registration_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] pool_registration_cert A pointer to the \ref cardano_pool_registration_cert_t instance whose last error
 *                   message is to be retrieved. If the pool_registration_cert is NULL, the function
 *                   returns a generic error message indicating the null pool_registration_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified pool_registration_cert. If the pool_registration_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_pool_registration_cert_set_last_error for the same pool_registration_cert, or until
 *       the pool_registration_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pool_registration_cert_get_last_error(
  const cardano_pool_registration_cert_t* pool_registration_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // POOL_REGISTRATION_CERT_H