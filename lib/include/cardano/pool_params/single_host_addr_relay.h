/**
 * \file single_host_addr_relay.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SINGLE_HOST_ADDR_RELAY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SINGLE_HOST_ADDR_RELAY_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/pool_params/ipv4.h>
#include <cardano/pool_params/ipv6.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This relay points to a single host via its ipv4/ipv6 address and a given port.
 */
typedef struct cardano_single_host_addr_relay_t cardano_single_host_addr_relay_t;

/**
 * \brief Creates and initializes a new instance of a single host address relay.
 *
 * This function allocates and initializes a new instance of \ref cardano_single_host_addr_relay_t.
 * It allows for the creation of a relay that points to a single host, which can be specified by
 * either an IPv4 or IPv6 address, or both. The port can also be specified, though it is optional.
 *
 * \param[in] port A pointer to the port number on which the host is listening. If NULL, no port
 *                 is set, and it should be handled accordingly by the consuming function.
 * \param[in] ipv4 A pointer to an initialized \ref cardano_ipv4_t object representing the host's IPv4 address.
 *                 If NULL, the IPv4 address is not set for the relay.
 * \param[in] ipv6 A pointer to an initialized \ref cardano_ipv6_t object representing the host's IPv6 address.
 *                 If NULL, the IPv6 address is not set for the relay.
 * \param[out] single_host_addr_relay On successful initialization, this pointer will point to the newly created
 *                                    \ref cardano_single_host_addr_relay_t object. This object represents a "strong reference"
 *                                    to the relay, meaning it is fully initialized and ready for use.
 *                                    The caller is responsible for managing the lifecycle of this object.
 *                                    Specifically, once the relay is no longer needed, the caller must release it
 *                                    by calling \ref cardano_single_host_addr_relay_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the single host address relay was successfully created, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the single_host_addr_relay pointer
 *         is NULL or other input validation errors.
 *
 * Usage Example:
 * \code{.c}
 * uint16_t port_number = 8080;
 * cardano_ipv4_t* ipv4_address = ...; // Assume ipv4_address is already initialized
 * cardano_ipv6_t* ipv6_address = ...; // Assume ipv6_address is already initialized
 * cardano_single_host_addr_relay_t* relay = NULL;
 *
 * cardano_error_t result = cardano_single_host_addr_relay_new(&port_number, ipv4_address, ipv6_address, &relay);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relay
 *
 *   // Once done, ensure to clean up and release the relay
 *   cardano_single_host_addr_relay_unref(&relay);
 * }
 * else
 * {
 *   printf("Failed to create the single host address relay.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_single_host_addr_relay_new(
  const uint16_t*                    port,
  cardano_ipv4_t*                    ipv4,
  cardano_ipv6_t*                    ipv6,
  cardano_single_host_addr_relay_t** single_host_addr_relay);

/**
 * \brief Creates a \ref cardano_single_host_addr_relay_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_single_host_addr_relay_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a single_host_addr_relay.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] single_host_addr_relay A pointer to a pointer of \ref cardano_single_host_addr_relay_t that will be set to the address
 *                        of the newly created single_host_addr_relay object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_single_host_addr_relay_t object by calling
 *       \ref cardano_single_host_addr_relay_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_single_host_addr_relay_t* single_host_addr_relay = NULL;
 *
 * cardano_error_t result = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the single_host_addr_relay
 *
 *   // Once done, ensure to clean up and release the single_host_addr_relay
 *   cardano_single_host_addr_relay_unref(&single_host_addr_relay);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode single_host_addr_relay: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_single_host_addr_relay_from_cbor(cardano_cbor_reader_t* reader, cardano_single_host_addr_relay_t** single_host_addr_relay);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_single_host_addr_relay_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] single_host_addr_relay A constant pointer to the \ref cardano_single_host_addr_relay_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p single_host_addr_relay or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_addr_relay_t* single_host_addr_relay = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_single_host_addr_relay_to_cbor(single_host_addr_relay, writer);
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
 * cardano_single_host_addr_relay_unref(&single_host_addr_relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_addr_relay_to_cbor(
  const cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_cbor_writer_t*                  writer);

/**
 * \brief Retrieves the port number for a single host address relay.
 *
 * This function retrieves the port number from a given \ref cardano_single_host_addr_relay_t object.
 *
 * \param[in] relay A constant pointer to an initialized \ref cardano_single_host_addr_relay_t object.
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
 * single_host_addr_relay* relay = ...; // Assume relay is already initialized
 * const uint16_t* port = cardano_single_host_addr_relay_get_port(relay);
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
CARDANO_EXPORT const uint16_t* cardano_single_host_addr_relay_get_port(
  const cardano_single_host_addr_relay_t* relay);

/**
 * \brief Sets the port number for a single host address relay.
 *
 * This function assigns a port number to a \ref cardano_single_host_addr_relay_t object. If the port
 * pointer is NULL, the function will remove any existing port number, effectively unsetting it.
 *
 * \param[in] single_host_addr_relay A pointer to an initialized \ref cardano_single_host_addr_relay_t object.
 * \param[in] port A pointer to a uint64_t that contains the new port number to be set. If the pointer is NULL,
 *                 the port number is unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the port number was successfully set or unset. Returns an error code such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the single_host_addr_relay pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_addr_relay_t* relay = ...; // Assume relay is already initialized
 * uint16_t new_port = 8080;
 *
 * cardano_error_t result = cardano_single_host_addr_relay_set_port(relay, &new_port);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Port number set to %lu.\n", new_port);
 * }
 * else
 * {
 *   printf("Failed to set the port number.\n");
 * }
 *
 * // To unset the port number
 * result = cardano_single_host_addr_relay_set_port(relay, NULL);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Port number has been unset.\n");
 * }
 * \endcode
 *
 * \note Passing a NULL for the port parameter will unset the port number for the relay.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_addr_relay_set_port(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  const uint16_t*                   port);

/**
 * \brief Retrieves the IPv4 address from a single host address relay.
 *
 * This function fetches the IPv4 address associated with a given \ref cardano_single_host_addr_relay_t object.
 * The function returns a new reference to the IPv4 object, which must be managed by the caller.
 *
 * \param[in] single_host_addr_relay A constant pointer to an initialized \ref cardano_single_host_addr_relay_t object.
 * \param[out] ipv4 On successful retrieval, this will point to the IPv4 address associated with the relay.
 *                  The function increments the reference count on the IPv4 object before returning it,
 *                  and the caller is responsible for releasing this reference when it is no longer needed
 *                  by calling \ref cardano_ipv4_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the IPv4 address was successfully retrieved, or an error code such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_single_host_addr_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_ipv4_t* ipv4 = NULL;
 *
 * cardano_error_t result = cardano_single_host_addr_relay_get_ipv4(relay, &ipv4);
 * if (result == CARDANO_SUCCESS && ipv4 != NULL)
 * {
 *   printf("IPv4 Address retrieved.\n");
 *   // Use the ipv4 address
 *
 *   // Once done, ensure to clean up and release the ipv4 reference
 *   cardano_ipv4_unref(&ipv4);
 * }
 * else
 * {
 *   printf("Failed to retrieve IPv4 address or address not set.\n");
 * }
 * \endcode
 *
 * \note If the relay does not have an IPv4 address set, the output parameter ipv4 will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_addr_relay_get_ipv4(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv4_t**                  ipv4);

/**
 * \brief Sets the IPv4 address for a single host address relay.
 *
 * This function assigns an IPv4 address to a \ref cardano_single_host_addr_relay_t object.
 * If the relay already has an IPv4 address assigned, it will be replaced by the new one provided.
 *
 * \param[in,out] single_host_addr_relay A pointer to an initialized \ref cardano_single_host_addr_relay_t object to which the IPv4 address will be set.
 * \param[in] ipv4 A pointer to an initialized \ref cardano_ipv4_t object representing the IPv4 address to be set. This function increases the reference count on the IPv4 object, and it is the caller's responsibility to manage the lifecycle of the passed ipv4 object, including releasing its reference when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the IPv4 address was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input pointer to the relay or IPv4 address is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_addr_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_ipv4_t* ipv4 = ...; // Assume ipv4 is already initialized
 *
 * cardano_error_t result = cardano_single_host_addr_relay_set_ipv4(relay, ipv4);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("IPv4 Address successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set IPv4 address.\n");
 * }
 *
 * // Ensure to manage the lifecycle of ipv4 if not used elsewhere
 * cardano_ipv4_unref(&ipv4);
 * \endcode
 *
 * \note This function increments the reference count of the ipv4 object passed. The caller must unreference
 * the ipv4 object when it is no longer needed to prevent memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_addr_relay_set_ipv4(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv4_t*                   ipv4);

/**
 * \brief Retrieves the IPv6 address from a single host address relay.
 *
 * This function fetches the IPv6 address associated with a given \ref cardano_single_host_addr_relay_t object.
 * The function returns a new reference to the IPv6 object, which must be managed by the caller.
 *
 * \param[in] single_host_addr_relay A constant pointer to an initialized \ref cardano_single_host_addr_relay_t object.
 * \param[out] ipv6 On successful retrieval, this will point to the IPv6 address associated with the relay.
 *                  The function increments the reference count on the IPv6 object before returning it,
 *                  and the caller is responsible for releasing this reference when it is no longer needed
 *                  by calling \ref cardano_ipv6_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the IPv6 address was successfully retrieved, or an error code such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_single_host_addr_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_ipv6_t* ipv6 = NULL;
 *
 * cardano_error_t result = cardano_single_host_addr_relay_get_ipv6(relay, &ipv6);
 * if (result == CARDANO_SUCCESS && ipv6 != NULL)
 * {
 *   printf("IPv6 Address retrieved.\n");
 *   // Use the ipv6 address
 *
 *   // Once done, ensure to clean up and release the ipv6 reference
 *   cardano_ipv6_unref(&ipv6);
 * }
 * else
 * {
 *   printf("Failed to retrieve IPv6 address or address not set.\n");
 * }
 * \endcode
 *
 * \note If the relay does not have an IPv6 address set, the output parameter ipv6 will be set to NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_addr_relay_get_ipv6(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv6_t**                  ipv6);

/**
 * \brief Sets the IPv6 address for a single host address relay.
 *
 * This function assigns an IPv6 address to a \ref cardano_single_host_addr_relay_t object.
 * If the relay already has an IPv6 address assigned, it will be replaced by the new one provided.
 *
 * \param[in,out] single_host_addr_relay A pointer to an initialized \ref cardano_single_host_addr_relay_t object to which the IPv6 address will be set.
 * \param[in] ipv6 A pointer to an initialized \ref cardano_ipv6_t object representing the IPv6 address to be set. This function increases the reference count on the IPv6 object, and it is the caller's responsibility to manage the lifecycle of the passed ipv6 object, including releasing its reference when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the IPv6 address was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input pointer to the relay or IPv6 address is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_addr_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_ipv6_t* ipv6 = ...; // Assume ipv6 is already initialized
 *
 * cardano_error_t result = cardano_single_host_addr_relay_set_ipv6(relay, ipv6);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("IPv6 Address successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set IPv6 address.\n");
 * }
 *
 * // Ensure to manage the lifecycle of ipv6 if not used elsewhere
 * cardano_ipv6_unref(&ipv6);
 * \endcode
 *
 * \note This function increments the reference count of the ipv6 object passed. The caller must unreference
 * the ipv6 object when it is no longer needed to prevent memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_single_host_addr_relay_set_ipv6(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv6_t*                   ipv6);

/**
 * \brief Decrements the reference count of a cardano_single_host_addr_relay_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_single_host_addr_relay_t object
 * by decreasing its reference count. When the reference count reaches zero, the single_host_addr_relay is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] single_host_addr_relay A pointer to the pointer of the single_host_addr_relay object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_addr_relay_t* single_host_addr_relay = cardano_single_host_addr_relay_new(major, minor);
 *
 * // Perform operations with the single_host_addr_relay...
 *
 * cardano_single_host_addr_relay_unref(&single_host_addr_relay);
 * // At this point, single_host_addr_relay is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_single_host_addr_relay_unref, the pointer to the \ref cardano_single_host_addr_relay_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_single_host_addr_relay_unref(cardano_single_host_addr_relay_t** single_host_addr_relay);

/**
 * \brief Increases the reference count of the cardano_single_host_addr_relay_t object.
 *
 * This function is used to manually increment the reference count of an cardano_single_host_addr_relay_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_single_host_addr_relay_unref.
 *
 * \param single_host_addr_relay A pointer to the cardano_single_host_addr_relay_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming single_host_addr_relay is a previously created single_host_addr_relay object
 *
 * cardano_single_host_addr_relay_ref(single_host_addr_relay);
 *
 * // Now single_host_addr_relay can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_single_host_addr_relay_ref there is a corresponding
 * call to \ref cardano_single_host_addr_relay_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_single_host_addr_relay_ref(cardano_single_host_addr_relay_t* single_host_addr_relay);

/**
 * \brief Retrieves the current reference count of the cardano_single_host_addr_relay_t object.
 *
 * This function returns the number of active references to an cardano_single_host_addr_relay_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_single_host_addr_relay_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param single_host_addr_relay A pointer to the cardano_single_host_addr_relay_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_single_host_addr_relay_t object. If the object
 * is properly managed (i.e., every \ref cardano_single_host_addr_relay_ref call is matched with a
 * \ref cardano_single_host_addr_relay_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming single_host_addr_relay is a previously created single_host_addr_relay object
 *
 * size_t ref_count = cardano_single_host_addr_relay_refcount(single_host_addr_relay);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_single_host_addr_relay_refcount(const cardano_single_host_addr_relay_t* single_host_addr_relay);

/**
 * \brief Sets the last error message for a given cardano_single_host_addr_relay_t object.
 *
 * Records an error message in the single_host_addr_relay's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] single_host_addr_relay A pointer to the \ref cardano_single_host_addr_relay_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the single_host_addr_relay's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_single_host_addr_relay_set_last_error(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  const char*                       message);

/**
 * \brief Retrieves the last error message recorded for a specific single_host_addr_relay.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_single_host_addr_relay_set_last_error for the given
 * single_host_addr_relay. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] single_host_addr_relay A pointer to the \ref cardano_single_host_addr_relay_t instance whose last error
 *                   message is to be retrieved. If the single_host_addr_relay is NULL, the function
 *                   returns a generic error message indicating the null single_host_addr_relay.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified single_host_addr_relay. If the single_host_addr_relay is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_single_host_addr_relay_set_last_error for the same single_host_addr_relay, or until
 *       the single_host_addr_relay is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_single_host_addr_relay_get_last_error(
  const cardano_single_host_addr_relay_t* single_host_addr_relay);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SINGLE_HOST_ADDR_RELAY_H