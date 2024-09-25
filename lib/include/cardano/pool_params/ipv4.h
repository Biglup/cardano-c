/**
 * \file ipv4.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_IPV4_H
#define BIGLUP_LABS_INCLUDE_CARDANO_IPV4_H

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
 * \brief Represents an IPv4 address.
 *
 * Each instance of `cardano_ipv4_t` holds a single IPv4 address in network byte order (big endian).
 */
typedef struct cardano_ipv4_t cardano_ipv4_t;

/**
 * \brief Creates and initializes a new instance of an IPv4 address.
 *
 * This function allocates and initializes a new instance of \ref cardano_ipv4_t.
 * The IPv4 address is created from a byte array representing the address in network
 * byte order (big endian).
 *
 * \param[in] data A pointer to a byte array containing the 4 bytes of the IPv4 address.
 * \param[in] size The size of the byte array, which should be exactly 4 bytes.
 * \param[out] ipv4 On successful initialization, this will point to the newly created
 *            \ref cardano_ipv4_t object. The object represents a "strong reference"
 *            to the IPv4 address, meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the IPv4 address is no longer needed, the caller must
 *            release it by calling \ref cardano_ipv4_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the IPv4 address was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ipv4_t* ipv4 = NULL;
 * byte_t data[] = {192, 168, 1, 1}; // Example IPv4 address
 * size_t size = sizeof(data);
 *
 * // Attempt to create a new IPv4 object
 * cardano_error_t result = cardano_ipv4_new(data, size, &ipv4);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the ipv4
 *   // Once done, ensure to clean up and release the ipv4
 *   cardano_ipv4_unref(&ipv4);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_ipv4_new(const byte_t* data, size_t size, cardano_ipv4_t** ipv4);

/**
 * \brief Initializes a new IPv4 address from a string representation.
 *
 * This function allocates and initializes a \ref cardano_ipv4_t object from a string
 * that represents an IPv4 address in dotted-decimal notation (e.g., "192.168.1.1").
 * The function parses the string and converts it into a numerical IP address.
 *
 * \param[in] string A pointer to a character array containing the IPv4 address in
 *                   dotted-decimal notation.
 * \param[in] size The length of the string, which should be a valid length for an IPv4
 *                 address in dotted-decimal format.
 * \param[out] ipv4 On successful parsing and initialization, this will point to the newly
 *                  created \ref cardano_ipv4_t object. The object represents a "strong reference"
 *                  to the IPv4 address, meaning that it is fully initialized and ready for use.
 *                  The caller is responsible for managing the lifecycle of this object.
 *                  Specifically, once the IPv4 address is no longer needed, the caller must
 *                  release it by calling \ref cardano_ipv4_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the IPv4 address was successfully parsed and created, or an appropriate error code
 *         indicating the failure reason, such as invalid format or out of memory errors.
 *
 * Usage Example:
 * \code{.c}
 * const char* ip_string = "192.168.1.1";
 * cardano_ipv4_t* ipv4 = NULL;
 * cardano_error_t result = cardano_ipv4_from_string(ip_string, strlen(ip_string), &ipv4);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the ipv4
 *
 *   // Once done, ensure to clean up and release the ipv4
 *   cardano_ipv4_unref(&ipv4);
 * }
 * else
 * {
 *   printf("Failed to create IPv4 address: %s\n", ip_string);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_ipv4_from_string(const char* string, size_t size, cardano_ipv4_t** ipv4);

/**
 * \brief Creates a \ref cardano_ipv4_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_ipv4_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a ipv4.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] ipv4 A pointer to a pointer of \ref cardano_ipv4_t that will be set to the address
 *                        of the newly created ipv4 object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_ipv4_t object by calling
 *       \ref cardano_ipv4_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_ipv4_t* ipv4 = NULL;
 *
 * cardano_error_t result = cardano_ipv4_from_cbor(reader, &ipv4);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the ipv4
 *
 *   // Once done, ensure to clean up and release the ipv4
 *   cardano_ipv4_unref(&ipv4);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode ipv4: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_ipv4_from_cbor(cardano_cbor_reader_t* reader, cardano_ipv4_t** ipv4);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_ipv4_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] ipv4 A constant pointer to the \ref cardano_ipv4_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p ipv4 or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ipv4_t* ipv4 = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_ipv4_to_cbor(ipv4, writer);
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
 * cardano_ipv4_unref(&ipv4);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ipv4_to_cbor(
  const cardano_ipv4_t*  ipv4,
  cardano_cbor_writer_t* writer);

/**
 * \brief Retrieves the size of the byte array representation of an IPv4 address.
 *
 * This function returns the size of the byte array that would be needed to store
 * the binary representation of an IPv4 address. Since IPv4 addresses always
 * consist of four octets, this function will consistently return a size of 4 bytes.
 *
 * \param[in] ipv4 A constant pointer to an initialized \ref cardano_ipv4_t object.
 *
 * \return The size in bytes of the byte array needed to represent the IPv4 address,
 *         which is always 4 bytes for standard IPv4 addresses.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ipv4_t* ipv4 = ...; // Assume ipv4 is already initialized
 * size_t ipv4_size = cardano_ipv4_get_bytes_size(ipv4);
 * printf("Size of the IPv4 byte array: %zu bytes\n", ipv4_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ipv4_get_bytes_size(const cardano_ipv4_t* ipv4);

/**
 * \brief Retrieves a pointer to the byte array representation of an IPv4 address.
 *
 * This function provides access to the raw byte data that represents an IPv4 address
 * within a \ref cardano_ipv4_t object. The returned pointer points to an internal data structure
 * of the IPv4 object and must not be modified or freed by the caller. The data remains valid
 * as long as the IPv4 object exists and has not been deallocated.
 *
 * \param[in] ipv4 A constant pointer to an initialized \ref cardano_ipv4_t object.
 *
 * \return A constant pointer to the byte array representing the IPv4 address. The array consists
 *         of 4 bytes corresponding to the IPv4 address in network byte order. Returns NULL if the
 *         input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ipv4_t* ipv4 = ...; // Assume ipv4 is already initialized
 * const byte_t* ipv4_bytes = cardano_ipv4_get_bytes(ipv4);
 *
 * if (ipv4_bytes)
 * {
 *     printf("IPv4 Address: %u.%u.%u.%u\n", ipv4_bytes[0], ipv4_bytes[1], ipv4_bytes[2], ipv4_bytes[3]);
 * }
 * else
 * {
 *     printf("Invalid IPv4 object provided.\n");
 * }
 * \endcode
 *
 * \note This function does not transfer ownership of the byte array to the caller. The returned
 *       pointer must not be freed or modified, and it should be used only while the ipv4 object
 *       is valid.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_ipv4_get_bytes(const cardano_ipv4_t* ipv4);

/**
 * \brief Retrieves the size necessary to store the string representation of an IPv4 address.
 *
 * This function calculates the length of the string required to represent the IPv4 address
 * contained within a \ref cardano_ipv4_t object, including the null terminator. This size
 * is useful when allocating a buffer to store the string representation of the IPv4 address.
 *
 * \param[in] ipv4 A constant pointer to an initialized \ref cardano_ipv4_t object.
 *
 * \return The size in bytes required to store the IPv4 address as a null-terminated string.
 *         If the input pointer is NULL, returns 0, indicating an error or uninitialized IPv4 object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ipv4_t* ipv4 = ...; // Assume ipv4 is already initialized
 * size_t ipv4_size = cardano_ipv4_get_string_size(ipv4);
 * printf("Size of the IPv4 string: %zu chars\n", ipv4_size);
 * \endcode
 *
 * \note This function returns 0 if the ipv4 pointer is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ipv4_get_string_size(const cardano_ipv4_t* ipv4);

/**
 * \brief Retrieves the string representation of an IPv4 address.
 *
 * This function provides access to the string representation of the IPv4 address contained
 * within a \ref cardano_ipv4_t object. The string is formatted in the standard dot-decimal
 * notation used for IPv4 addresses (e.g., "192.168.1.1").
 *
 * \param[in] ipv4 A constant pointer to an initialized \ref cardano_ipv4_t object.
 *
 * \return A constant pointer to a null-terminated string representing the IPv4 address.
 *         If the input pointer is NULL, returns NULL to indicate an error or uninitialized IPv4 object.
 *         The caller should not attempt to modify or free the returned string, as it points to
 *         internal memory managed by the \ref cardano_ipv4_t object. The memory for the string
 *         will be freed when the \ref cardano_ipv4_t object is destroyed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ipv4_t* ipv4 = ...; // Assume ipv4 is already initialized
 * const char* ipv4_string = cardano_ipv4_get_string(ipv4);
 *
 * if (ipv4_string != NULL)
 * {
 *   printf("IPv4 Address: %s\n", ipv4_string);
 * }
 * else
 * {
 *   printf("Failed to retrieve IPv4 string representation.\n");
 * }
 * \endcode
 *
 * \note This function returns NULL if the ipv4 pointer is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_ipv4_get_string(const cardano_ipv4_t* ipv4);

/**
 * \brief Decrements the reference count of a cardano_ipv4_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_ipv4_t object
 * by decreasing its reference count. When the reference count reaches zero, the ipv4 is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] ipv4 A pointer to the pointer of the ipv4 object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ipv4_t* ipv4 = cardano_ipv4_new(major, minor);
 *
 * // Perform operations with the ipv4...
 *
 * cardano_ipv4_unref(&ipv4);
 * // At this point, ipv4 is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_ipv4_unref, the pointer to the \ref cardano_ipv4_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_ipv4_unref(cardano_ipv4_t** ipv4);

/**
 * \brief Increases the reference count of the cardano_ipv4_t object.
 *
 * This function is used to manually increment the reference count of an cardano_ipv4_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_ipv4_unref.
 *
 * \param ipv4 A pointer to the cardano_ipv4_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming ipv4 is a previously created ipv4 object
 *
 * cardano_ipv4_ref(ipv4);
 *
 * // Now ipv4 can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_ipv4_ref there is a corresponding
 * call to \ref cardano_ipv4_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_ipv4_ref(cardano_ipv4_t* ipv4);

/**
 * \brief Retrieves the current reference count of the cardano_ipv4_t object.
 *
 * This function returns the number of active references to an cardano_ipv4_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_ipv4_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param ipv4 A pointer to the cardano_ipv4_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_ipv4_t object. If the object
 * is properly managed (i.e., every \ref cardano_ipv4_ref call is matched with a
 * \ref cardano_ipv4_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming ipv4 is a previously created ipv4 object
 *
 * size_t ref_count = cardano_ipv4_refcount(ipv4);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ipv4_refcount(const cardano_ipv4_t* ipv4);

/**
 * \brief Sets the last error message for a given cardano_ipv4_t object.
 *
 * Records an error message in the ipv4's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] ipv4 A pointer to the \ref cardano_ipv4_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the ipv4's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_ipv4_set_last_error(
  cardano_ipv4_t* ipv4,
  const char*     message);

/**
 * \brief Retrieves the last error message recorded for a specific ipv4.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_ipv4_set_last_error for the given
 * ipv4. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] ipv4 A pointer to the \ref cardano_ipv4_t instance whose last error
 *                   message is to be retrieved. If the ipv4 is NULL, the function
 *                   returns a generic error message indicating the null ipv4.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified ipv4. If the ipv4 is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_ipv4_set_last_error for the same ipv4, or until
 *       the ipv4 is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_ipv4_get_last_error(
  const cardano_ipv4_t* ipv4);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_IPV4_H