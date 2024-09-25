/**
 * \file asset_name.h
 *
 * \author angel.castillo
 * \date   Mar 07, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ASSET_NAME_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ASSET_NAME_H

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
 * \brief Represents an asset name.
 */
typedef struct cardano_asset_name_t cardano_asset_name_t;

/**
 * \brief Creates and initializes a new instance of a Cardano asset name from byte data.
 *
 * This function allocates and initializes a new instance of a \ref cardano_asset_name_t object,
 * which represents the name of a Cardano native asset. Native asset names in Cardano are arbitrary
 * byte strings, typically used to uniquely identify assets within a policy.
 *
 * \param[in] data A pointer to an array of bytes representing the asset name. This array should contain
 *                 the raw byte data of the asset name and must not be NULL.
 * \param[in] size The size of the byte array. This value specifies how many bytes from the `data` array
 *                 should be used for the asset name. The size must not exceed the maximum allowed length
 *                 for asset names in Cardano.
 * \param[out] asset_name On successful initialization, this will point to a newly created \ref cardano_asset_name_t object.
 *             This object represents a "strong reference" to the asset name, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the asset name is no longer needed, the caller must release it
 *             by calling \ref cardano_asset_name_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the asset name was successfully created from the provided bytes, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input data pointer is NULL,
 *         or \ref CARDANO_ERROR_INVALID_ARGUMENT if the size exceeds the maximum length or other constraints.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t* name_data = (const byte_t*)"example_asset";
 * size_t name_size = strlen("example_asset");
 * cardano_asset_name_t* asset_name = NULL;
 *
 * cardano_error_t result = cardano_asset_name_from_bytes(name_data, name_size, &asset_name);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the asset_name
 *   // Once done, ensure to clean up and release the asset_name
 *   cardano_asset_name_unref(&asset_name);
 * }
 * else
 * {
 *   printf("Failed to create asset name: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_name_from_bytes(
  const byte_t*          data,
  size_t                 size,
  cardano_asset_name_t** asset_name);

/**
 * \brief Creates and initializes a new instance of a Cardano asset name from a hexadecimal string.
 *
 * This function allocates and initializes a new instance of a \ref cardano_asset_name_t object,
 * representing an asset name using a hexadecimal string. Asset names in Cardano are arbitrary byte strings,
 * and this function specifically interprets the input as a hex-encoded representation of those bytes.
 *
 * \param[in] hex A pointer to a null-terminated string containing the hexadecimal representation of the asset name.
 *                The string must not be NULL, must be properly hex-encoded, and must not contain non-hex characters.
 * \param[in] size The number of characters in the hex string. This value must be even, as each pair of characters represents one byte.
 * \param[out] asset_name On successful initialization, this will point to a newly created \ref cardano_asset_name_t object.
 *             This object represents a "strong reference" to the asset name, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the asset name is no longer needed, the caller must release it
 *             by calling \ref cardano_asset_name_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the asset name was successfully created from the provided hex string, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input hex pointer is NULL,
 *         or \ref CARDANO_ERROR_INVALID_ARGUMENT if the size is incorrect or the hex string is not valid.
 *
 * Usage Example:
 * \code{.c}
 * const char* hex_string = "6578616d706c65"; // Hex for "example"
 * size_t hex_size = strlen(hex_string); // Should be an even number
 * cardano_asset_name_t* asset_name = NULL;
 *
 * cardano_error_t result = cardano_asset_name_from_hex(hex_string, hex_size, &asset_name);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the asset_name
 *   // Once done, ensure to clean up and release the asset_name
 *   cardano_asset_name_unref(&asset_name);
 * }
 * else
 * {
 *   printf("Failed to create asset name from hex: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_name_from_hex(
  const char*            hex,
  size_t                 size,
  cardano_asset_name_t** asset_name);

/**
 * \brief Creates and initializes a new instance of a Cardano asset name from a UTF-8 string.
 *
 * This function allocates and initializes a new instance of a \ref cardano_asset_name_t object,
 * which represents the name of a Cardano native asset. Asset names in Cardano are arbitrary byte strings,
 * but this function specifically interprets the input as a UTF-8 encoded string, allowing for easier
 * creation of asset names from human-readable text.
 *
 * \param[in] string A pointer to a null-terminated UTF-8 encoded string representing the asset name.
 *                   This string must not be NULL and should be properly encoded in UTF-8.
 * \param[in] size The number of bytes in the string, not including the null terminator. This allows
 *                 for strings containing null bytes to be handled correctly.
 * \param[out] asset_name On successful initialization, this will point to a newly created \ref cardano_asset_name_t object.
 *             This object represents a "strong reference" to the asset name, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the asset name is no longer needed, the caller must release it
 *             by calling \ref cardano_asset_name_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the asset name was successfully created from the provided string, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input string pointer is NULL,
 *         or \ref CARDANO_ERROR_INVALID_ARGUMENT if the size is incorrect or other constraints are not met.
 *
 * Usage Example:
 * \code{.c}
 * const char* name_string = "example_asset";
 * size_t name_length = strlen(name_string); // Not including the null terminator
 * cardano_asset_name_t* asset_name = NULL;
 *
 * cardano_error_t result = cardano_asset_name_from_string(name_string, name_length, &asset_name);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the asset_name
 *   // Once done, ensure to clean up and release the asset_name
 *   cardano_asset_name_unref(&asset_name);
 * }
 * else
 * {
 *   printf("Failed to create asset name: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_name_from_string(
  const char*            string,
  size_t                 size,
  cardano_asset_name_t** asset_name);

/**
 * \brief Creates a \ref cardano_asset_name_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_asset_name_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a asset_name.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] asset_name A pointer to a pointer of \ref cardano_asset_name_t that will be set to the address
 *                        of the newly created asset_name object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the asset name were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_asset_name_t object by calling
 *       \ref cardano_asset_name_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_asset_name_t* asset_name = NULL;
 *
 * cardano_error_t result = cardano_asset_name_from_cbor(reader, &asset_name);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the asset_name
 *
 *   // Once done, ensure to clean up and release the asset_name
 *   cardano_asset_name_unref(&asset_name);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode asset_name: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_name_from_cbor(cardano_cbor_reader_t* reader, cardano_asset_name_t** asset_name);

/**
 * \brief Serializes asset name into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_asset_name_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] asset_name A constant pointer to the \ref cardano_asset_name_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p asset_name or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_name_t* asset_name = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_asset_name_to_cbor(asset_name, writer);
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
 * cardano_asset_name_unref(&asset_name);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_asset_name_to_cbor(
  const cardano_asset_name_t* asset_name,
  cardano_cbor_writer_t*      writer);

/**
 * \brief Retrieves the string representation of a Cardano asset name.
 *
 * This function provides access to the string representation of a \ref cardano_asset_name_t object.
 * The asset name is returned as a UTF-8 encoded null-terminated string. This string points to an internal
 * buffer of the \ref cardano_asset_name_t object and must not be modified or freed by the caller. The string
 * remains valid as long as the asset name object is not modified or freed.
 *
 * \param[in] asset_name A pointer an initialized \ref cardano_asset_name_t object.
 *                       The pointer must not be NULL and should point to a valid asset name object.
 *
 * \return A pointer to a null-terminated UTF-8 string representing the asset name. If the input is NULL
 *         or an error occurs, NULL is returned instead. The caller must not free this string as it is
 *         managed internally by the asset name object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_name_t* asset_name = ...; // Assume asset_name is already initialized
 * const char* name_str = cardano_asset_name_get_string(asset_name);
 *
 * if (name_str != NULL)
 * {
 *   printf("Asset Name: %s\n", name_str);
 * }
 * else
 * {
 *   printf("Failed to retrieve the asset name string.\n");
 * }
 * // No need to free name_str, it is managed internally
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char*
cardano_asset_name_get_string(const cardano_asset_name_t* asset_name);

/**
 * \brief Retrieves the size of the string representation of a Cardano asset name, including the null terminator.
 *
 * This function calculates the length of the string representation of a \ref cardano_asset_name_t object,
 * including the null terminator. This is useful for understanding the buffer size required to store the
 * string representation of the asset name.
 *
 * \param[in] asset_name A pointer to an initialized \ref cardano_asset_name_t object.
 *                       The pointer must not be NULL and should point to a valid asset name object.
 *
 * \return The size of the string in bytes, including the null terminator. If the input is NULL or an error occurs,
 *         the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_name_t* asset_name = ...; // Assume asset_name is already initialized
 * size_t name_size = cardano_asset_name_get_string_size(asset_name);
 *
 * if (name_size > 0)
 * {
 *   printf("Size of the asset name string including null terminator: %zu\n", name_size);
 * }
 * else
 * {
 *   printf("Failed to retrieve the size of the asset name string.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_asset_name_get_string_size(const cardano_asset_name_t* asset_name);

/**
 * \brief Retrieves the byte representation of a Cardano asset name.
 *
 * This function provides access to the underlying byte array of a \ref cardano_asset_name_t object.
 * The byte array represents the asset name as it is stored and used within the Cardano ecosystem.
 *
 * \param[in] asset_name A pointer of an initialized \ref cardano_asset_name_t object.
 *                       The pointer must not be NULL and should point to a valid asset name object.
 *
 * \return A pointer to the constant byte array representing the asset name. This pointer points to an
 *         internal buffer and must not be modified or freed by the caller. If the input is NULL or an
 *         error occurs, the function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_name_t* asset_name = ...; // Assume asset_name is already initialized
 * const byte_t* name_bytes = cardano_asset_name_get_bytes(asset_name);
 *
 * if (name_bytes != NULL)
 * {
 *   // Process the bytes of the asset name
 *   // Note: The length of the byte array should be known or determined separately
 * }
 * else
 * {
 *   printf("Failed to retrieve the bytes of the asset name.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t*
cardano_asset_name_get_bytes(const cardano_asset_name_t* asset_name);

/**
 * \brief Retrieves the size of the byte array for a Cardano asset name.
 *
 * This function returns the size of the byte array representing the asset name contained within
 * a \ref cardano_asset_name_t object.
 *
 * \param[in] asset_name A pointer of an initialized \ref cardano_asset_name_t object.
 *                       The pointer must not be NULL and should point to a valid asset name object.
 *
 * \return The size of the byte array representing the asset name. If the input is NULL or an
 *         error occurs, the function returns 0. This size does not include a null terminator, as
 *         asset names can contain arbitrary binary data.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_name_t* asset_name = ...; // Assume asset_name is already initialized
 * size_t name_size = cardano_asset_name_get_bytes_size(asset_name);
 *
 * if (name_size > 0)
 * {
 *   const byte_t* name_bytes = cardano_asset_name_get_bytes(asset_name);
 *   // Process the bytes knowing the exact size of the asset name
 * }
 * else
 * {
 *   printf("Failed to retrieve the size of the asset name or asset name is empty.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_asset_name_get_bytes_size(const cardano_asset_name_t* asset_name);

/**
 * \brief Retrieves the hexadecimal string representation of a Cardano asset name.
 *
 * This function returns the hexadecimal string representation of a \ref cardano_asset_name_t object.
 * The string encodes the asset name, which consists of the concatenated bytes of the policy ID and asset name.
 *
 * \param[in] asset_name A pointer to an initialized \ref cardano_asset_name_t object.
 *                     The pointer must not be NULL and should point to a valid asset name object.
 *
 * \return A pointer to a null-terminated string representing the asset name in hexadecimal format.
 *         This string is owned by the \ref cardano_asset_name_t object and must not be modified or freed by the caller.
 *         If the input is NULL or an error occurs, the function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_asset_name_t* asset_name = ...; // Assume asset_name is already initialized
 * const char* hex_string = cardano_asset_name_get_hex(asset_name);
 *
 * if (hex_string != NULL)
 * {
 *   // Process the hexadecimal string of the asset name
 *   printf("Asset ID in hex: %s\n", hex_string);
 * }
 * else
 * {
 *   printf("Failed to retrieve the hexadecimal string of the asset name.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char*
cardano_asset_name_get_hex(const cardano_asset_name_t* asset_name);

/**
 * \brief Retrieves the size of the hexadecimal string for a Cardano asset name.
 *
 * This function returns the size of the hexadecimal string representation of a \ref cardano_asset_name_t object.
 * The size includes the number of characters in the hexadecimal string, not including the null terminator.
 *
 * \param[in] asset_name A pointer to an initialized \ref cardano_asset_name_t object.
 *                     The pointer must not be NULL and should point to a valid asset name object.
 *
 * \return The size of the hexadecimal string representing the asset name. If the input is NULL or an
 *         error occurs, the function returns 0. The size does not include a null terminator.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_asset_name_t* asset_name = ...; // Assume asset_name is already initialized
 * size_t hex_size = cardano_asset_name_get_hex_size(asset_name);
 *
 * if (hex_size > 0)
 * {
 *   const char* hex_string = cardano_asset_name_get_hex(asset_name);
 *   // Process the hex string knowing the exact size of the asset name
 *   printf("Hexadecimal string size: %zu\n", hex_size);
 * }
 * else
 * {
 *   printf("Failed to retrieve the size of the asset name in hex or asset name is empty.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_asset_name_get_hex_size(const cardano_asset_name_t* asset_name);

/**
 * \brief Decrements the reference count of a cardano_asset_name_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_asset_name_t object
 * by decreasing its reference count. When the reference count reaches zero, the asset_name is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] asset_name A pointer to the pointer of the asset_name object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_name_t* asset_name = cardano_asset_name_new(major, minor);
 *
 * // Perform operations with the asset_name...
 *
 * cardano_asset_name_unref(&asset_name);
 * // At this point, asset_name is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_asset_name_unref, the pointer to the \ref cardano_asset_name_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_asset_name_unref(cardano_asset_name_t** asset_name);

/**
 * \brief Increases the reference count of the cardano_asset_name_t object.
 *
 * This function is used to manually increment the reference count of an cardano_asset_name_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_asset_name_unref.
 *
 * \param asset_name A pointer to the cardano_asset_name_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming asset_name is a previously created asset_name object
 *
 * cardano_asset_name_ref(asset_name);
 *
 * // Now asset_name can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_asset_name_ref there is a corresponding
 * call to \ref cardano_asset_name_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_asset_name_ref(cardano_asset_name_t* asset_name);

/**
 * \brief Retrieves the current reference count of the cardano_asset_name_t object.
 *
 * This function returns the number of active references to an cardano_asset_name_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_asset_name_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param asset_name A pointer to the cardano_asset_name_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_asset_name_t object. If the object
 * is properly managed (i.e., every \ref cardano_asset_name_ref call is matched with a
 * \ref cardano_asset_name_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming asset_name is a previously created asset_name object
 *
 * size_t ref_count = cardano_asset_name_refcount(asset_name);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_asset_name_refcount(const cardano_asset_name_t* asset_name);

/**
 * \brief Sets the last error message for a given cardano_asset_name_t object.
 *
 * Records an error message in the asset_name's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] asset_name A pointer to the \ref cardano_asset_name_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the asset_name's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_asset_name_set_last_error(
  cardano_asset_name_t* asset_name,
  const char*           message);

/**
 * \brief Retrieves the last error message recorded for a specific asset_name.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_asset_name_set_last_error for the given
 * asset_name. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] asset_name A pointer to the \ref cardano_asset_name_t instance whose last error
 *                   message is to be retrieved. If the asset_name is NULL, the function
 *                   returns a generic error message indicating the null asset_name.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified asset_name. If the asset_name is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_asset_name_set_last_error for the same asset_name, or until
 *       the asset_name is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_asset_name_get_last_error(
  const cardano_asset_name_t* asset_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ASSET_NAME_H