/**
 * \file certificate_set.h
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#ifndef CARDANO_CERTIFICATE_SET_H
#define CARDANO_CERTIFICATE_SET_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/certificate.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Represents a set of certificates.
 */
typedef struct cardano_certificate_set_t cardano_certificate_set_t;

/**
 * \brief Creates and initializes a new instance of a certificate_set.
 *
 * This function allocates and initializes a new instance of \ref cardano_certificate_set_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] certificate_set On successful initialization, this will point to a newly created
 *             \ref cardano_certificate_set_t object. This object represents a "strong reference"
 *             to the certificate_set, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the certificate_set is no longer needed, the caller must release it
 *             by calling \ref cardano_certificate_set_unref.
 *
 * \return \ref CARDANO_SUCCESS if the certificate_set was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_set_t* certificate_set = NULL;
 *
 * // Attempt to create a new certificate_set
 * cardano_error_t result = cardano_certificate_set_new(&certificate_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the certificate_set
 *
 *   // Once done, ensure to clean up and release the certificate_set
 *   cardano_certificate_set_unref(&certificate_set);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_set_new(cardano_certificate_set_t** certificate_set);

/**
 * \brief Creates a certificate set from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_certificate_set_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a certificate.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded certificate data.
 * \param[out] certificates A pointer to a pointer of \ref cardano_certificate_set_t that will be set to the address
 *                        of the newly created certificate object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the certificate was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_certificate_set_t object by calling
 *       \ref cardano_certificate_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_certificate_set_t* certificate = NULL;
 *
 * cardano_error_t result = cardano_certificate_set_from_cbor(reader, &certificate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the certificate
 *
 *   // Once done, ensure to clean up and release the certificate
 *   cardano_certificate_set_unref(&certificate);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode certificate: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_certificate_set_from_cbor(cardano_cbor_reader_t* reader, cardano_certificate_set_t** certificates);

/**
 * \brief Serializes a certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_certificate_set_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] certificates A constant pointer to the \ref cardano_certificate_set_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p certificate or \p writer
 *         is NULL, returns \ref CARDANO_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_set_t* certificate = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_certificate_set_to_cbor(certificate, writer);
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
 * cardano_certificate_set_unref(&certificate);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_certificate_set_to_cbor(
  const cardano_certificate_set_t* certificates,
  cardano_cbor_writer_t*           writer);

/**
 * \brief Retrieves the length of a certificate list.
 *
 * This function retrieves the number of elements in the provided \ref cardano_certificate_set_t object.
 *
 * \param[in] certificate_set A constant pointer to the \ref cardano_certificate_set_t object whose length is to be retrieved.
 *
 * \return The number of elements in the certificate_set. Return 0 if the certificate_set is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_set_t* certificate_set = cardano_certificate_set_new();
 *
 * // Populate certificate_set with elements
 *
 * size_t length = cardano_certificate_set_get_length(certificate_set);
 * printf("Length of the certificate_set: %zu\n", length);
 *
 * // Clean up the certificate_set object once done
 * cardano_certificate_set_unref(&certificate_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_certificate_set_get_length(const cardano_certificate_set_t* certificate_set);

/**
 * \brief Retrieves an element from a certificate list by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_certificate_set_t object
 * and stores it in the output parameter.
 *
 * \param[in] certificate_set A constant pointer to the \ref cardano_certificate_set_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the certificate list. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_certificate_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_certificate_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_set_t* certificate_set = cardano_certificate_set_new();
 *
 * // Populate certificate_set with elements
 *
 * cardano_certificate_t* element = NULL;
 * cardano_error_t result = cardano_certificate_set_get(certificate_set, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_certificate_unref(&element);
 * }
 *
 * // Clean up the certificate_set object once done
 * cardano_certificate_set_unref(&certificate_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_certificate_set_get(const cardano_certificate_set_t* certificate_set, size_t index, cardano_certificate_t** element);

/**
 * \brief Adds an element to a certificate list.
 *
 * This function adds the specified element to the end of the provided \ref cardano_certificate_set_t object.
 *
 * \param[in] certificate_set A constant pointer to the \ref cardano_certificate_set_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_certificate_t object that is to be added to the certificate_set.
 *                    The element will be referenced by the certificate_set after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the certificate_set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_set_t* certificate_set = cardano_certificate_set_new();
 *
 * // Create and initialize a new certificate element
 * cardano_certificate_t* element = { ... };
 *
 * // Add the element to the certificate_set
 * cardano_error_t result = cardano_certificate_set_add(certificate_set, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the certificate_set object once done
 * cardano_certificate_set_unref(&certificate_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_certificate_set_add(cardano_certificate_set_t* certificate_set, cardano_certificate_t* element);

/**
 * \brief Decrements the reference count of a certificate_set object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_certificate_set_t object
 * by decreasing its reference count. When the reference count reaches zero, the certificate_set is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] certificate_set A pointer to the pointer of the certificate_set object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_certificate_set_t* certificate_set = cardano_certificate_set_new();
 *
 * // Perform operations with the certificate_set...
 *
 * cardano_certificate_set_unref(&certificate_set);
 * // At this point, certificate_set is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_certificate_set_unref, the pointer to the \ref cardano_certificate_set_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_certificate_set_unref(cardano_certificate_set_t** certificate_set);

/**
 * \brief Increases the reference count of the cardano_certificate_set_t object.
 *
 * This function is used to manually increment the reference count of a certificate_set
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_certificate_set_unref.
 *
 * \param certificate_set A pointer to the certificate_set object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming certificate_set is a previously created certificate_set object
 *
 * cardano_certificate_set_ref(certificate_set);
 *
 * // Now certificate_set can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_certificate_set_ref there is a corresponding
 * call to \ref cardano_certificate_set_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_certificate_set_ref(cardano_certificate_set_t* certificate_set);

/**
 * \brief Retrieves the current reference count of the cardano_certificate_set_t object.
 *
 * This function returns the number of active references to a certificate_set object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_certificate_set_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param certificate_set A pointer to the certificate_set object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified certificate_set object. If the object
 * is properly managed (i.e., every \ref cardano_certificate_set_ref call is matched with a
 * \ref cardano_certificate_set_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming certificate_set is a previously created certificate_set object
 *
 * size_t ref_count = cardano_certificate_set_refcount(certificate_set);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_certificate_set_refcount(const cardano_certificate_set_t* certificate_set);

/**
 * \brief Sets the last error message for a given certificate_set object.
 *
 * Records an error message in the certificate_set's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] certificate_set A pointer to the \ref cardano_certificate_set_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the certificate_set's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_certificate_set_set_last_error(cardano_certificate_set_t* certificate_set, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific certificate_set.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_certificate_set_set_last_error for the given
 * certificate_set. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] certificate_set A pointer to the \ref cardano_certificate_set_t instance whose last error
 *                   message is to be retrieved. If the certificate_set is NULL, the function
 *                   returns a generic error message indicating the null certificate_set.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified certificate_set. If the certificate_set is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_certificate_set_set_last_error for the same certificate_set, or until
 *       the certificate_set is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_certificate_set_get_last_error(const cardano_certificate_set_t* certificate_set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CERTIFICATE_SET_H