/**
 * \file relay.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RELAY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RELAY_H

/* INCLUDES ******************************************************************/

#include "relay_type.h"
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
 * \brief A relay is a type of node that acts as intermediaries between core nodes
 * (which produce blocks) and the wider internet. They help in passing along
 * transactions and blocks, ensuring that data is propagated throughout the
 * network.
 */
typedef struct cardano_relay_t cardano_relay_t;

/**
 * \brief This relay points to a multi host name via a DNS (A SRV DNS record) name.
 */
typedef struct cardano_multi_host_name_relay_t cardano_multi_host_name_relay_t;

/**
 * \brief This relay points to a single host via its ipv4/ipv6 address and a given port.
 */
typedef struct cardano_single_host_addr_relay_t cardano_single_host_addr_relay_t;

/**
 * \brief This relay points to a single host via a DNS (pointing to an A or AAAA DNS record) name and a given port.
 */
typedef struct cardano_single_host_name_relay_t cardano_single_host_name_relay_t;

/**
 * \brief Creates a new relay object that points to a single host address.
 *
 * This function allocates and initializes a new instance of a \ref cardano_relay_t,
 * which is configured to point to a single host via its IP address and a specified port.
 * This type of relay is used for direct, single-host communication in the Cardano network.
 *
 * \param[in] single_host_addr_relay A pointer to an initialized \ref cardano_single_host_addr_relay_t object
 *                                   that specifies the IP address and port of the host.
 * \param[out] relay On successful initialization, this will point to the newly created
 *                   \ref cardano_relay_t object. This object represents a "strong reference"
 *                   to the relay, meaning that it is fully initialized and ready for use.
 *                   The caller is responsible for managing the lifecycle of this object.
 *                   Specifically, once the relay is no longer needed, the caller must release it
 *                   by calling \ref cardano_relay_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relay was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_addr_relay_t* single_host_addr_relay = ...; // Assume single_host_addr_relay is initialized
 * cardano_relay_t* relay = NULL;
 *
 * cardano_error_t result = cardano_relay_new_single_host_addr(single_host_addr_relay, &relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relay
 *
 *   // Once done, ensure to clean up and release the relay
 *   cardano_relay_unref(&relay);
 * }
 *
 * cardano_single_host_addr_relay_unref(&single_host_addr_relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_relay_new_single_host_addr(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_relay_t**                 relay);

/**
 * \brief Creates a new relay object that points to a single host name.
 *
 * This function allocates and initializes a new instance of \ref cardano_relay_t,
 * configured to point to a single host via its DNS name and a specified port.
 * This relay is useful for Cardano network communication where DNS resolution
 * is preferred over direct IP address connections.
 *
 * \param[in] single_host_name A pointer to an initialized \ref cardano_single_host_name_relay_t object
 *                             that specifies the DNS name and port of the host.
 * \param[out] relay On successful initialization, this will point to the newly created
 *                   \ref cardano_relay_t object. This object represents a "strong reference"
 *                   to the relay, indicating that it is fully initialized and ready for use.
 *                   The caller is responsible for managing the lifecycle of this object.
 *                   Specifically, once the relay is no longer needed, the caller must release it
 *                   by calling \ref cardano_relay_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relay was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_single_host_name_relay_t* single_host_name = ...; // Assume single_host_name is initialized
 * cardano_relay_t* relay = NULL;
 *
 * cardano_error_t result = cardano_relay_new_single_host_name(single_host_name, &relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relay
 *
 *   // Once done, ensure to clean up and release the relay
 *   cardano_relay_unref(&relay);
 * }
 *
 * cardano_single_host_name_relay_unref(&single_host_name);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_relay_new_single_host_name(
  cardano_single_host_name_relay_t* single_host_name,
  cardano_relay_t**                 relay);

/**
 * \brief Creates a new relay object that points to multiple host names.
 *
 * This function allocates and initializes a new instance of \ref cardano_relay_t,
 * configured to point to multiple hosts via a DNS SRV record. This type of relay
 * allows for the resolution of multiple addresses and is useful in environments where
 * high availability and load balancing are required.
 *
 * \param[in] multi_host_name_relay A pointer to an initialized \ref cardano_multi_host_name_relay_t object
 *                                  that specifies the DNS SRV record.
 * \param[out] relay On successful initialization, this will point to the newly created
 *                   \ref cardano_relay_t object. This object represents a "strong reference"
 *                   to the relay, indicating that it is fully initialized and ready for use.
 *                   The caller is responsible for managing the lifecycle of this object.
 *                   Specifically, once the relay is no longer needed, the caller must release it
 *                   by calling \ref cardano_relay_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relay was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_host_name_relay_t* multi_host_name_relay = ...; // Assume multi_host_name_relay is initialized
 * cardano_relay_t* relay = NULL;
 *
 * cardano_error_t result = cardano_relay_new_multi_host_name(multi_host_name_relay, &relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relay
 *
 *   // Once done, ensure to clean up and release the relay
 *   cardano_relay_unref(&relay);
 * }
 *
 * cardano_multi_host_name_relay_unref(&multi_host_name_relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_relay_new_multi_host_name(
  cardano_multi_host_name_relay_t* multi_host_name_relay,
  cardano_relay_t**                relay);

/**
 * \brief Creates a \ref cardano_relay_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_relay_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a relay.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] relay A pointer to a pointer of \ref cardano_relay_t that will be set to the address
 *                        of the newly created relay object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_relay_t object by calling
 *       \ref cardano_relay_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_relay_t* relay = NULL;
 *
 * cardano_error_t result = cardano_relay_from_cbor(reader, &relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relay
 *
 *   // Once done, ensure to clean up and release the relay
 *   cardano_relay_unref(&relay);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode relay: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_relay_from_cbor(cardano_cbor_reader_t* reader, cardano_relay_t** relay);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_relay_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] relay A constant pointer to the \ref cardano_relay_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p relay or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relay_t* relay = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_relay_to_cbor(relay, writer);
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
 * cardano_relay_unref(&relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relay_to_cbor(
  const cardano_relay_t* relay,
  cardano_cbor_writer_t* writer);

/**
 * \brief Retrieves the type of a relay.
 *
 * This function provides the type of a \ref cardano_relay_t object, which indicates
 * the specific configuration of the relay (e.g., single host address, single host name,
 * or multi host name).
 *
 * \param[in] relay A constant pointer to an initialized \ref cardano_relay_t object.
 * \param[out] type A pointer to a \ref cardano_relay_type_t variable where the type of the relay
 *                  will be stored upon successful execution.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the type was successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_relay_type_t type;
 *
 * cardano_error_t result = cardano_relay_type(relay, &type);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process depending on the type
 *   switch (type) {
 *     case CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS:
 *       // Handle single host address relay
 *       break;
 *     case CARDANO_RELAY_TYPE_SINGLE_HOST_NAME:
 *       // Handle single host name relay
 *       break;
 *     case CARDANO_RELAY_TYPE_MULTI_HOST_NAME:
 *       // Handle multi host name relay
 *       break;
 *   }
 * }
 * else
 * {
 *   printf("Failed to retrieve relay type.\n");
 * }
 *
 * cardano_relay_unref(&relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relay_get_type(
  const cardano_relay_t* relay,
  cardano_relay_type_t*  type);

/**
 * \brief Converts a relay to a single host address relay if possible and provides a new reference.
 *
 * This function attempts to convert a generic \ref cardano_relay_t object to a more specific
 * \ref cardano_single_host_addr_relay_t object. This operation will only succeed if the relay
 * is of the single host address type. On success, it returns a new reference to the single
 * host address relay, which must be managed separately from the original relay.
 *
 * \param[in] relay A pointer to an initialized \ref cardano_relay_t object.
 * \param[out] single_host_addr A pointer to a pointer of \ref cardano_single_host_addr_relay_t which will point
 *                              to the single host address relay representation if the conversion is successful.
 *                              This parameter is set only if the operation succeeds and will hold a new reference.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relay was successfully converted, or an appropriate error code indicating the failure reason.
 *         Common failure reasons include incorrect relay type or NULL pointer inputs.
 *
 * \note The caller is responsible for managing the lifecycle of both the original and the newly converted
 *       relay objects. Both must be released with appropriate unref calls when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_single_host_addr_relay_t* single_host_addr;
 *
 * cardano_error_t result = cardano_relay_to_single_host_addr(relay, &single_host_addr);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the single_host_addr
 *   // When done, release the newly created single_host_addr
 *   cardano_single_host_addr_relay_unref(single_host_addr);
 * }
 * else
 * {
 *   printf("Failed to convert relay to single host address.\n");
 * }
 * // Original relay must also be unref when it is no longer needed
 * cardano_relay_unref(&relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relay_to_single_host_addr(
  cardano_relay_t*                   relay,
  cardano_single_host_addr_relay_t** single_host_addr);

/**
 * \brief Converts a relay to a single host name relay if possible and provides a new reference.
 *
 * This function attempts to convert a generic \ref cardano_relay_t object to a more specific
 * \ref cardano_single_host_name_relay_t object. This operation will only succeed if the relay
 * is of the single host name type. On success, it returns a new reference to the single
 * host name relay, which must be managed separately from the original relay.
 *
 * \param[in] relay A pointer to an initialized \ref cardano_relay_t object.
 * \param[out] single_host_name A pointer to a pointer of \ref cardano_single_host_name_relay_t which will point
 *                              to the single host name relay representation if the conversion is successful.
 *                              This parameter is set only if the operation succeeds and will hold a new reference.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relay was successfully converted, or an appropriate error code indicating the failure reason.
 *         Common failure reasons include incorrect relay type or NULL pointer inputs.
 *
 * \note The caller is responsible for managing the lifecycle of both the original and the newly converted
 *       relay objects. Both must be released with appropriate unref calls when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_single_host_name_relay_t* single_host_name;
 *
 * cardano_error_t result = cardano_relay_to_single_host_name(relay, &single_host_name);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the single_host_name
 *   // When done, release the newly created single_host_name
 *   cardano_single_host_name_relay_unref(single_host_name);
 * }
 * else
 * {
 *   printf("Failed to convert relay to single host name.\n");
 * }
 * // Original relay must also be unref when it is no longer needed
 * cardano_relay_unref(&relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relay_to_single_host_name(
  cardano_relay_t*                   relay,
  cardano_single_host_name_relay_t** single_host_name);

/**
 * \brief Converts a relay to a multi host name relay if possible and provides a new reference.
 *
 * This function attempts to convert a generic \ref cardano_relay_t object to a more specific
 * \ref cardano_multi_host_name_relay_t object. This operation will only succeed if the relay
 * is of the multi host name type. On success, it returns a new reference to the multi
 * host name relay, which must be managed separately from the original relay.
 *
 * \param[in] relay A pointer to an initialized \ref cardano_relay_t object.
 * \param[out] multi_host_name A pointer to a pointer of \ref cardano_multi_host_name_relay_t which will point
 *                             to the multi host name relay representation if the conversion is successful.
 *                             This parameter is set only if the operation succeeds and will hold a new reference.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relay was successfully converted, or an appropriate error code indicating the failure reason.
 *         Common failure reasons include incorrect relay type or NULL pointer inputs.
 *
 * \note The caller is responsible for managing the lifecycle of both the original and the newly converted
 *       relay objects. Both must be released with appropriate unref calls when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relay_t* relay = ...; // Assume relay is already initialized
 * cardano_multi_host_name_relay_t* multi_host_name;
 *
 * cardano_error_t result = cardano_relay_to_multi_host_name(relay, &multi_host_name);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the multi_host_name
 *   // When done, release the newly created multi_host_name
 *   cardano_multi_host_name_relay_unref(multi_host_name);
 * }
 * else
 * {
 *   printf("Failed to convert relay to multi host name.\n");
 * }
 * // Original relay must also be unref when it is no longer needed
 * cardano_relay_unref(&relay);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relay_to_multi_host_name(
  cardano_relay_t*                  relay,
  cardano_multi_host_name_relay_t** multi_host_name);

/**
 * \brief Decrements the reference count of a cardano_relay_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_relay_t object
 * by decreasing its reference count. When the reference count reaches zero, the relay is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] relay A pointer to the pointer of the relay object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relay_t* relay = cardano_relay_new(major, minor);
 *
 * // Perform operations with the relay...
 *
 * cardano_relay_unref(&relay);
 * // At this point, relay is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_relay_unref, the pointer to the \ref cardano_relay_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_relay_unref(cardano_relay_t** relay);

/**
 * \brief Increases the reference count of the cardano_relay_t object.
 *
 * This function is used to manually increment the reference count of an cardano_relay_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_relay_unref.
 *
 * \param relay A pointer to the cardano_relay_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming relay is a previously created relay object
 *
 * cardano_relay_ref(relay);
 *
 * // Now relay can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_relay_ref there is a corresponding
 * call to \ref cardano_relay_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_relay_ref(cardano_relay_t* relay);

/**
 * \brief Retrieves the current reference count of the cardano_relay_t object.
 *
 * This function returns the number of active references to an cardano_relay_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_relay_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param relay A pointer to the cardano_relay_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_relay_t object. If the object
 * is properly managed (i.e., every \ref cardano_relay_ref call is matched with a
 * \ref cardano_relay_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming relay is a previously created relay object
 *
 * size_t ref_count = cardano_relay_refcount(relay);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_relay_refcount(const cardano_relay_t* relay);

/**
 * \brief Sets the last error message for a given cardano_relay_t object.
 *
 * Records an error message in the relay's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] relay A pointer to the \ref cardano_relay_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the relay's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_relay_set_last_error(
  cardano_relay_t* relay,
  const char*      message);

/**
 * \brief Retrieves the last error message recorded for a specific relay.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_relay_set_last_error for the given
 * relay. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] relay A pointer to the \ref cardano_relay_t instance whose last error
 *                   message is to be retrieved. If the relay is NULL, the function
 *                   returns a generic error message indicating the null relay.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified relay. If the relay is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_relay_set_last_error for the same relay, or until
 *       the relay is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_relay_get_last_error(
  const cardano_relay_t* relay);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RELAY_H