/**
 * \file single_host_name_relay.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SINGLE_HOST_NAME_RELAY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SINGLE_HOST_NAME_RELAY_H

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
 * \brief This relay points to a single host via a DNS (pointing to an A or AAAA DNS record) name and a given port.
 */
typedef struct cardano_single_host_name_relay_t cardano_single_host_name_relay_t;

/**
 * \brief Creates and initializes a new instance of a single host name relay.
 *
 * This function allocates and initializes a new instance of \ref cardano_single_host_name_relay_t,
 * representing a relay that points to a single host via its DNS name and an optional port number.
 *
 * \param[in] port A pointer to the port number for the relay. This can be NULL if the port is not specified.
 * \param[in] dns A pointer to a character array containing the DNS name of the host.
 * \param[in] str_size The length of the DNS name string, excluding the null terminator.
 * \param[out] single_host_name_relay On successful initialization, this will point to the newly created
 *                                    \ref cardano_single_host_name_relay_t object. The caller is responsible
 *                                    for managing the lifecycle of this object. Specifically, once the relay
 *                                    is no longer needed, the caller must release it by calling \ref cardano_single_host_name_relay_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relay was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* dns_name = "relay.example.com";
 * size_t dns_size = strlen(dns_name);
 * uint16_t port = 3001;
 * cardano_single_host_name_relay_t* relay = NULL;
 *
 * // Attempt to create a new single host name relay
 * cardano_error_t result = cardano_single_host_name_relay_new(&port, dns_name, dns_size, &relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relay
 *
 *   // Once done, ensure to clean up and release the relay
 *   cardano_single_host_name_relay_unref(&relay);
 * }
 * else
 * {
 *   printf("Failed to create single host name relay.\n");
 * }
 * \endcode
 *
 * \note If the port is not specified, the \p port parameter can be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_single_host_name_relay_new(
  const uint16_t*                    port,
  const char*                        dns,
  size_t                             str_size,
  cardano_single_host_name_relay_t** single_host_name_relay);

/**
 * \brief Creates a \ref cardano_single_host_name_relay_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_single_host_name_relay_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a single_host_name_relay.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] single_host_name_relay A pointer to a pointer of \ref cardano_single_host_name_relay_t that will be set to the address
 *                        of the newly created single_host_name_relay object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_single_host_name_relay_t object by calling
 *       \ref cardano_single_host_name_relay_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_single_host_name_relay_t* single_host_name_relay = NULL;
 *
 * cardano_error_t result = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the single_host_name_relay
 *
 *   // Once done, ensure to clean up and release the single_host_name_relay
 *   cardano_single_host_name_relay_unref(&single_host_name_relay);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode single_host_name_relay: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_single_host_name_relay_from_cbor(cardano_cbor_reader_t* reader, cardano_single_host_name_relay_t** single_host_name_relay);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_single_host_name_relay_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] single_host_name_relay A constant pointer to the \ref cardano_single_host_name_relay_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p single_host_name_relay or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* single_host_name_relay = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_single_host_name_relay_to_cbor(single_host_name_relay, writer);
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
 * cardano_single_host_name_relay_unref(&single_host_name_relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_name_relay_to_cbor(
  const cardano_single_host_name_relay_t* single_host_name_relay,
  cardano_cbor_writer_t*                  writer);

/**
 * \brief Retrieves the port number for a single host name relay.
 *
 * This function retrieves the port number from a given \ref cardano_single_host_name_relay_t object.
 *
 * \param[in] relay A constant pointer to an initialized \ref cardano_single_host_name_relay_t object.
 *
 * \return A constant pointer to the port number of the relay. If the relay does not have a port set,
 *         the function returns NULL. The returned pointer points to the internal state of the relay
 *         object and should not be freed or modified by the caller.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the port number was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * const uint16_t* port = cardano_single_host_name_relay_get_port(relay);
 *
 * if (port != NULL)
 * {
 *   printf("Relay port: %lu\n", *port);
 * }
 * else
 * {
 *   printf("No port is set for this relay.\n");
 * }
 * \endcode
 *
 * \note The returned pointer points to the internal state of the relay object and should not be freed or modified by the caller.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint16_t* cardano_single_host_name_relay_get_port(
  const cardano_single_host_name_relay_t* relay);

/**
 * \brief Sets the port number for a single host name relay.
 *
 * This function sets the port number for a given \ref cardano_single_host_name_relay_t object.
 *
 * \param[in] relay A pointer to an initialized \ref cardano_single_host_name_relay_t object.
 * \param[in] port A pointer to the port number to be set for the relay. If \p port is NULL, the port will be unset for the relay.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the port number was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * uint16_t port = 8080;
 *
 * cardano_error_t result = cardano_single_host_name_relay_set_port(relay, &port);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Port number set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the port number.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_name_relay_set_port(
  cardano_single_host_name_relay_t* relay,
  const uint16_t*                   port);

/**
 * \brief Retrieves the size of the DNS name string of a \c cardano_single_host_name_relay_t object.
 *
 * This function returns the size in bytes of the DNS name string stored within a
 * \ref cardano_single_host_name_relay_t object. This size includes the null-terminating character
 * for the string, thus providing the actual length of the DNS name plus one.
 *
 * \param[in] single_host_name_relay A constant pointer to an initialized \ref cardano_single_host_name_relay_t object.
 *
 * \return The size in bytes of the DNS name string, including the null terminator.
 *         If the input pointer is NULL, returns 0 to indicate an error or uninitialized relay object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * size_t dns_size = cardano_single_host_name_relay_get_dns_size(relay);
 * printf("Size of the DNS name string: %zu bytes\n", dns_size);
 * \endcode
 *
 * \note This function returns 0 if the \p single_host_name_relay pointer is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_single_host_name_relay_get_dns_size(
  const cardano_single_host_name_relay_t* single_host_name_relay);

/**
 * \brief Retrieves the DNS name string from a \c cardano_single_host_name_relay_t object.
 *
 * This function provides access to the DNS name string contained within a \ref cardano_single_host_name_relay_t object.
 * The string represents the domain name used for DNS-based relay discovery and connection in the Cardano network.
 *
 * \param[in] single_host_name_relay A constant pointer to an initialized \ref cardano_single_host_name_relay_t object.
 *
 * \return A constant pointer to a null-terminated string representing the DNS name.
 *         If the input pointer is NULL, returns NULL to indicate an error or uninitialized relay object.
 *         The returned string points to internal memory within the \c cardano_single_host_name_relay_t object and must not be modified
 *         or freed by the caller. The memory will remain valid as long as the \c cardano_single_host_name_relay_t object is not
 *         destroyed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * const char* dns_name = cardano_single_host_name_relay_get_dns(relay);
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
 * \note This function returns NULL if the \p single_host_name_relay pointer is NULL, indicating either an error or
 *       an uninitialized state.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_single_host_name_relay_get_dns(
  const cardano_single_host_name_relay_t* single_host_name_relay);

/**
 * \brief Sets the DNS name for a \c cardano_single_host_name_relay_t object.
 *
 * This function updates the DNS name in a \ref cardano_single_host_name_relay_t object with the provided DNS name string.
 * The DNS name is used for DNS-based relay discovery and connection in the Cardano network. The function ensures the
 * new DNS name is properly stored within the relay object, replacing any previous value.
 *
 * \param[in] dns A constant pointer to a character array containing the new DNS name in null-terminated string format.
 * \param[in] dns_size The size of the DNS name string, excluding the null terminator.
 * \param[out] single_host_name_relay A pointer to an initialized \ref cardano_single_host_name_relay_t object which will
 *                                    receive the updated DNS name.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the DNS name
 *         was successfully updated, or an appropriate error code indicating the failure reason, such as invalid format,
 *         null pointers, or out of memory errors.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* relay = ...; // Assume relay is already initialized
 * const char* new_dns = "new.example.com";
 * size_t dns_size = strlen(new_dns);
 * cardano_error_t result = cardano_single_host_name_relay_set_dns(new_dns, dns_size, relay);
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
CARDANO_EXPORT cardano_error_t cardano_single_host_name_relay_set_dns(
  const char*                       dns,
  size_t                            dns_size,
  cardano_single_host_name_relay_t* single_host_name_relay);

/**
 * \brief Decrements the reference count of a cardano_single_host_name_relay_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_single_host_name_relay_t object
 * by decreasing its reference count. When the reference count reaches zero, the single_host_name_relay is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] single_host_name_relay A pointer to the pointer of the single_host_name_relay object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* single_host_name_relay = cardano_single_host_name_relay_new(major, minor);
 *
 * // Perform operations with the single_host_name_relay...
 *
 * cardano_single_host_name_relay_unref(&single_host_name_relay);
 * // At this point, single_host_name_relay is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_single_host_name_relay_unref, the pointer to the \ref cardano_single_host_name_relay_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_single_host_name_relay_unref(cardano_single_host_name_relay_t** single_host_name_relay);

/**
 * \brief Increases the reference count of the cardano_single_host_name_relay_t object.
 *
 * This function is used to manually increment the reference count of an cardano_single_host_name_relay_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_single_host_name_relay_unref.
 *
 * \param single_host_name_relay A pointer to the cardano_single_host_name_relay_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming single_host_name_relay is a previously created single_host_name_relay object
 *
 * cardano_single_host_name_relay_ref(single_host_name_relay);
 *
 * // Now single_host_name_relay can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_single_host_name_relay_ref there is a corresponding
 * call to \ref cardano_single_host_name_relay_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_single_host_name_relay_ref(cardano_single_host_name_relay_t* single_host_name_relay);

/**
 * \brief Retrieves the current reference count of the cardano_single_host_name_relay_t object.
 *
 * This function returns the number of active references to an cardano_single_host_name_relay_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_single_host_name_relay_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param single_host_name_relay A pointer to the cardano_single_host_name_relay_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_single_host_name_relay_t object. If the object
 * is properly managed (i.e., every \ref cardano_single_host_name_relay_ref call is matched with a
 * \ref cardano_single_host_name_relay_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming single_host_name_relay is a previously created single_host_name_relay object
 *
 * size_t ref_count = cardano_single_host_name_relay_refcount(single_host_name_relay);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_single_host_name_relay_refcount(const cardano_single_host_name_relay_t* single_host_name_relay);

/**
 * \brief Sets the last error message for a given cardano_single_host_name_relay_t object.
 *
 * Records an error message in the single_host_name_relay's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] single_host_name_relay A pointer to the \ref cardano_single_host_name_relay_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the single_host_name_relay's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_single_host_name_relay_set_last_error(
  cardano_single_host_name_relay_t* single_host_name_relay,
  const char*                       message);

/**
 * \brief Retrieves the last error message recorded for a specific single_host_name_relay.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_single_host_name_relay_set_last_error for the given
 * single_host_name_relay. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] single_host_name_relay A pointer to the \ref cardano_single_host_name_relay_t instance whose last error
 *                   message is to be retrieved. If the single_host_name_relay is NULL, the function
 *                   returns a generic error message indicating the null single_host_name_relay.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified single_host_name_relay. If the single_host_name_relay is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_single_host_name_relay_set_last_error for the same single_host_name_relay, or until
 *       the single_host_name_relay is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_single_host_name_relay_get_last_error(
  const cardano_single_host_name_relay_t* single_host_name_relay);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SINGLE_HOST_NAME_RELAY_H