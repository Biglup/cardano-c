/**
 * \file multi_host_name_relay.h
 *
 * \author angel.castillo
 * \date   Jun 26, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_MULTI_HOST_NAME_RELAY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_MULTI_HOST_NAME_RELAY_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This relay points to a multi host name via a DNS (A SRV DNS record) name.
 */
typedef struct cardano_multi_host_name_relay_t cardano_multi_host_name_relay_t;

/**
 * \brief Creates and initializes a new \ref cardano_multi_host_name_relay_t object.
 *
 * This function allocates and initializes a new instance of \ref cardano_multi_host_name_relay_t,
 * which represents a multi-host name relay in the Cardano network. A multi-host name relay
 * provide a DNS name that resolves to multiple IP addresses, facilitating load balancing and
 * failover for stake pool relays.
 *
 * \param[in] dns A pointer to a character array containing the DNS name for the multi-host relay.
 * \param[in] str_size The length of the DNS string.
 * \param[out] multi_host_name_relay On successful initialization, this will point to the newly created
 *            \ref cardano_multi_host_name_relay_t object. This object represents a "strong reference"
 *            to the multi-host name relay, meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the multi-host name relay is no longer needed, the caller must
 *            release it by calling \ref cardano_multi_host_name_relay_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the multi-host name relay was successfully created, or an appropriate error code
 *         indicating the failure reason, such as invalid DNS name format or out of memory errors.
 *
 * Usage Example:
 * \code{.c}
 * const char* dns_name = "relay1.example.com";
 * cardano_multi_host_name_relay_t* relay = NULL;
 * cardano_error_t result = cardano_multi_host_name_relay_new(dns_name, strlen(dns_name), &relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the multi-host name relay
 *
 *   // Once done, ensure to clean up and release the relay
 *   cardano_multi_host_name_relay_unref(&relay);
 * }
 * else
 * {
 *   printf("Failed to create multi-host name relay: %s\n", dns_name);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_multi_host_name_relay_new(
  const char*                       dns,
  size_t                            str_size,
  cardano_multi_host_name_relay_t** multi_host_name_relay);

/**
 * \brief Creates a \ref cardano_multi_host_name_relay_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_multi_host_name_relay_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a multi_host_name_relay.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] multi_host_name_relay A pointer to a pointer of \ref cardano_multi_host_name_relay_t that will be set to the address
 *                        of the newly created multi_host_name_relay object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_multi_host_name_relay_t object by calling
 *       \ref cardano_multi_host_name_relay_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_multi_host_name_relay_t* multi_host_name_relay = NULL;
 *
 * cardano_error_t result = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name_relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the multi_host_name_relay
 *
 *   // Once done, ensure to clean up and release the multi_host_name_relay
 *   cardano_multi_host_name_relay_unref(&multi_host_name_relay);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode multi_host_name_relay: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_multi_host_name_relay_from_cbor(cardano_cbor_reader_t* reader, cardano_multi_host_name_relay_t** multi_host_name_relay);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_multi_host_name_relay_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] multi_host_name_relay A constant pointer to the \ref cardano_multi_host_name_relay_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p multi_host_name_relay or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_host_name_relay_t* multi_host_name_relay = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_multi_host_name_relay_to_cbor(multi_host_name_relay, writer);
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
 * cardano_multi_host_name_relay_unref(&multi_host_name_relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_host_name_relay_to_cbor(
  const cardano_multi_host_name_relay_t* multi_host_name_relay,
  cardano_cbor_writer_t*                 writer);

/**
 * \brief Retrieves the size of the DNS name string of a \c cardano_multi_host_name_relay_t object.
 *
 * This function returns the size in bytes of the DNS name string stored within a
 * \ref cardano_multi_host_name_relay_t object. This size includes the null-terminating character
 * for the string, thus providing the actual length of the DNS name plus one.
 *
 * \param[in] multi_host_name_relay A constant pointer to an initialized \ref cardano_multi_host_name_relay_t object.
 *
 * \return The size in bytes of the DNS name string, including the null terminator.
 *         If the input pointer is NULL, returns 0 to indicate an error or uninitialized relay object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * size_t dns_size = cardano_multi_host_name_relay_get_dns_size(relay);
 * printf("Size of the DNS name string: %zu bytes\n", dns_size);
 * \endcode
 *
 * \note This function returns 0 if the \p multi_host_name_relay pointer is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_multi_host_name_relay_get_dns_size(
  const cardano_multi_host_name_relay_t* multi_host_name_relay);

/**
 * \brief Retrieves the DNS name string from a \c cardano_multi_host_name_relay_t object.
 *
 * This function provides access to the DNS name string contained within a \ref cardano_multi_host_name_relay_t object.
 * The string represents the domain name used for DNS-based relay discovery and connection in the Cardano network.
 *
 * \param[in] multi_host_name_relay A constant pointer to an initialized \ref cardano_multi_host_name_relay_t object.
 *
 * \return A constant pointer to a null-terminated string representing the DNS name.
 *         If the input pointer is NULL, returns NULL to indicate an error or uninitialized relay object.
 *         The returned string points to internal memory within the \c cardano_multi_host_name_relay_t object and must not be modified
 *         or freed by the caller. The memory will remain valid as long as the \c cardano_multi_host_name_relay_t object is not
 *         destroyed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * const char* dns_name = cardano_multi_host_name_relay_get_dns(relay);
 *
 * if (dns_name != NULL)
 * {
 *   printf("DNS Name: %s\n", dns_name);
 * }
 * else
 * {
 *   printf("Failed to retrieve DNS name.\n");
 * }
 * \endcode
 *
 * \note This function returns NULL if the \p multi_host_name_relay pointer is NULL, indicating either an error or
 *       an uninitialized state.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_multi_host_name_relay_get_dns(
  const cardano_multi_host_name_relay_t* multi_host_name_relay);

/**
 * \brief Sets the DNS name for a \c cardano_multi_host_name_relay_t object.
 *
 * This function updates the DNS name in a \ref cardano_multi_host_name_relay_t object with the provided DNS name string.
 * The DNS name is used for DNS-based relay discovery and connection in the Cardano network. The function ensures the
 * new DNS name is properly stored within the relay object, replacing any previous value.
 *
 * \param[in] dns A constant pointer to a character array containing the new DNS name in null-terminated string format.
 * \param[in] dns_size The size of the DNS name string, excluding the null terminator.
 * \param[out] multi_host_name_relay A pointer to an initialized \ref cardano_multi_host_name_relay_t object which will
 *                                   receive the updated DNS name.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the DNS name
 *         was successfully updated, or an appropriate error code indicating the failure reason, such as invalid format,
 *         null pointers, or out of memory errors.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * const char* new_dns = "new.example.com";
 * size_t dns_size = strlen(new_dns);
 * cardano_error_t result = cardano_multi_host_name_relay_set_dns(new_dns, dns_size, relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DNS name updated successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to update DNS name.\n");
 * }
 * \endcode
 *
 * \note This function does not take ownership of the dns string; the caller must ensure it remains valid for the duration
 *       of the call and manages its memory appropriately.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_host_name_relay_set_dns(
  const char*                      dns,
  size_t                           dns_size,
  cardano_multi_host_name_relay_t* multi_host_name_relay);

/**
 * \brief Decrements the reference count of a cardano_multi_host_name_relay_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_multi_host_name_relay_t object
 * by decreasing its reference count. When the reference count reaches zero, the multi_host_name_relay is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] multi_host_name_relay A pointer to the pointer of the multi_host_name_relay object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_host_name_relay_t* multi_host_name_relay = cardano_multi_host_name_relay_new(major, minor);
 *
 * // Perform operations with the multi_host_name_relay...
 *
 * cardano_multi_host_name_relay_unref(&multi_host_name_relay);
 * // At this point, multi_host_name_relay is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_multi_host_name_relay_unref, the pointer to the \ref cardano_multi_host_name_relay_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_multi_host_name_relay_unref(cardano_multi_host_name_relay_t** multi_host_name_relay);

/**
 * \brief Increases the reference count of the cardano_multi_host_name_relay_t object.
 *
 * This function is used to manually increment the reference count of an cardano_multi_host_name_relay_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_multi_host_name_relay_unref.
 *
 * \param multi_host_name_relay A pointer to the cardano_multi_host_name_relay_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming multi_host_name_relay is a previously created multi_host_name_relay object
 *
 * cardano_multi_host_name_relay_ref(multi_host_name_relay);
 *
 * // Now multi_host_name_relay can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_multi_host_name_relay_ref there is a corresponding
 * call to \ref cardano_multi_host_name_relay_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_multi_host_name_relay_ref(cardano_multi_host_name_relay_t* multi_host_name_relay);

/**
 * \brief Retrieves the current reference count of the cardano_multi_host_name_relay_t object.
 *
 * This function returns the number of active references to an cardano_multi_host_name_relay_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_multi_host_name_relay_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param multi_host_name_relay A pointer to the cardano_multi_host_name_relay_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_multi_host_name_relay_t object. If the object
 * is properly managed (i.e., every \ref cardano_multi_host_name_relay_ref call is matched with a
 * \ref cardano_multi_host_name_relay_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming multi_host_name_relay is a previously created multi_host_name_relay object
 *
 * size_t ref_count = cardano_multi_host_name_relay_refcount(multi_host_name_relay);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_multi_host_name_relay_refcount(const cardano_multi_host_name_relay_t* multi_host_name_relay);

/**
 * \brief Sets the last error message for a given cardano_multi_host_name_relay_t object.
 *
 * Records an error message in the multi_host_name_relay's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] multi_host_name_relay A pointer to the \ref cardano_multi_host_name_relay_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the multi_host_name_relay's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_multi_host_name_relay_set_last_error(
  cardano_multi_host_name_relay_t* multi_host_name_relay,
  const char*                      message);

/**
 * \brief Retrieves the last error message recorded for a specific multi_host_name_relay.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_multi_host_name_relay_set_last_error for the given
 * multi_host_name_relay. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] multi_host_name_relay A pointer to the \ref cardano_multi_host_name_relay_t instance whose last error
 *                   message is to be retrieved. If the multi_host_name_relay is NULL, the function
 *                   returns a generic error message indicating the null multi_host_name_relay.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified multi_host_name_relay. If the multi_host_name_relay is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_multi_host_name_relay_set_last_error for the same multi_host_name_relay, or until
 *       the multi_host_name_relay is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_multi_host_name_relay_get_last_error(
  const cardano_multi_host_name_relay_t* multi_host_name_relay);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_MULTI_HOST_NAME_RELAY_H