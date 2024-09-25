/**
 * \file plutus_v1_script.h
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_V1_SCRIPT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_V1_SCRIPT_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Plutus' scripts are pieces of code that implement pure functions with True or False outputs. These functions take
 * several inputs such as Datum, Redeemer and the transaction context to decide whether an output can be spent or not.
 *
 * V1 was the initial version of Plutus, introduced in the Alonzo hard fork.
 */
typedef struct cardano_plutus_v1_script_t cardano_plutus_v1_script_t;

/**
 * \brief Creates and initializes a new instance of a plutus script from a byte array.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_v1_script_t object
 * from a given byte array. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] bytes A pointer to the byte array from which the plutus_v1_script will be created.
 * \param[in] size The size of the byte array.
 * \param[out] plutus_v1_script On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_v1_script_t object. This object represents a "strong reference"
 *             to the plutus script, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus script is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_v1_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t bytes[] = {0x01, 0x02, 0x03, 0x04 ... }; // Example byte array
 * size_t size = sizeof(bytes); // Calculate the size of the byte array
 * cardano_plutus_v1_script_t* plutus_v1_script = NULL;
 *
 * // Attempt to create a new plutus script from a byte array
 * cardano_error_t result = cardano_plutus_v1_script_new_bytes(bytes, size, &plutus_v1_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus script
 *
 *   // Once done, ensure to clean up and release the plutus script
 *   cardano_plutus_v1_script_unref(&plutus_v1_script);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_v1_script_new_bytes(
  const byte_t*                bytes,
  size_t                       size,
  cardano_plutus_v1_script_t** plutus_v1_script);

/**
 * \brief Creates and initializes a new instance of a plutus script from a hexadecimal string.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_v1_script_t object
 * from a given hexadecimal string. It returns an error code to indicate the success or
 * failure of the operation.
 *
 * \param[in] hex A pointer to the hexadecimal string from which the plutus script will be created.
 * \param[in] size The size of the hexadecimal string.
 * \param[out] plutus_v1_script On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_v1_script_t object. This object represents a "strong reference"
 *             to the plutus script, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus script is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_v1_script_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus script was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* hex_string = "0123456789abcdef..."; // Example hexadecimal string
 * size_t size = strlen(hex_string); // Calculate the size of the string
 * cardano_plutus_v1_script_t* plutus_v1_script = NULL;
 *
 * // Attempt to create a new plutus script from a hexadecimal string
 * cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(hex_string, size, &plutus_v1_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus script
 *
 *   // Once done, ensure to clean up and release the plutus script
 *   cardano_plutus_v1_script_unref(&plutus_v1_script);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_v1_script_new_bytes_from_hex(
  const char*                  hex,
  size_t                       size,
  cardano_plutus_v1_script_t** plutus_v1_script);

/**
 * \brief Creates a plutus_v1_script from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_plutus_v1_script_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a plutus_v1_script.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded plutus_v1_script data.
 * \param[out] plutus_v1_script A pointer to a pointer of \ref cardano_plutus_v1_script_t that will be set to the address
 *                        of the newly created plutus_v1_script object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the plutus_v1_script was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_plutus_v1_script_t object by calling
 *       \ref cardano_plutus_v1_script_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_plutus_v1_script_t* plutus_v1_script = NULL;
 *
 * cardano_error_t result = cardano_plutus_v1_script_from_cbor(reader, &plutus_v1_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_v1_script
 *
 *   // Once done, ensure to clean up and release the plutus_v1_script
 *   cardano_plutus_v1_script_unref(&plutus_v1_script);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode plutus_v1_script: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_v1_script_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_v1_script_t** plutus_v1_script);

/**
 * \brief Serializes a plutus_v1_script into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_plutus_v1_script_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] plutus_v1_script A constant pointer to the \ref cardano_plutus_v1_script_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p plutus_v1_script or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v1_script_t* plutus_v1_script = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_plutus_v1_script_to_cbor(plutus_v1_script, writer);
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
 * cardano_plutus_v1_script_unref(&plutus_v1_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_v1_script_to_cbor(
  const cardano_plutus_v1_script_t* plutus_v1_script,
  cardano_cbor_writer_t*            writer);

/**
 * \brief Gets the raw bytes of a compiled Plutus v1 script. If you need "cborBytes" for cardano-cli use \ref cardano_plutus_v1_script_to_cbor instead.
 *
 * This function converts a \ref cardano_plutus_v1_script_t object to its raw byte representation.
 * The resulting bytes are stored in a \ref cardano_buffer_t object.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] plutus_v1_script A constant pointer to the \ref cardano_plutus_v1_script_t object to be converted.
 * \param[out] compiled_script On successful conversion, this will point to a newly created
 *                           \ref cardano_buffer_t object containing the extracted bytes.
 *                           This object represents a "strong reference" to the compiled_script bytes,
 *                           meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the bounded bytes are no longer needed, the caller must release it
 *                           by calling \ref cardano_buffer_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v1_script_t* plutus_v1_script = ...; // Assume plutus_v1_script is initialized
 * cardano_buffer_t* compiled_script = NULL;
 * cardano_error_t result = cardano_plutus_v1_script_to_raw_bytes(plutus_v1_script, &compiled_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bounded bytes
 *   printf("Extracted bounded bytes: ");
 *
 *   for (size_t i = 0; i < cardano_buffer_get_size(compiled_script); ++i)
 *   {
 *     printf("%02X ", cardano_buffer_get_data(compiled_script)[i]);
 *   }
 *
 *   printf("\n");
 *
 *   // Once done, ensure to clean up and release the bounded bytes
 *   cardano_buffer_unref(&compiled_script);
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to extract bounded bytes: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the plutus_v1_script object once done
 * cardano_plutus_v1_script_unref(&plutus_v1_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_v1_script_to_raw_bytes(
  cardano_plutus_v1_script_t* plutus_v1_script,
  cardano_buffer_t**          compiled_script);

/**
 * \brief Retrieves the hash associated with a plutus_v1_script.
 *
 * This function computes the hash of a \ref cardano_plutus_v1_script_t object.
 * It returns a new reference to a \ref cardano_blake2b_hash_t object representing the hash.
 * It the caller's responsibility to release it by calling \ref cardano_blake2b_hash_unref when
 * it is no longer needed.
 *
 * \param plutus_v1_script A constant pointer to the \ref cardano_plutus_v1_script_t object from which
 *                   the hash is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_blake2b_hash_t object containing the hash.
 *         If the input plutus_v1_script is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_blake2b_hash_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v1_script_t* original_plutus_v1_script = cardano_plutus_v1_script_new(...);
 * cardano_blake2b_hash_t* hash_plutus_v1_script = cardano_plutus_v1_script_get_hash(original_plutus_v1_script);
 *
 * if (hash_plutus_v1_script)
 * {
 *   // Use the hash plutus_v1_script
 *
 *   // Once done, ensure to clean up and release the hash plutus_v1_script
 *   cardano_blake2b_hash_unref(&hash_plutus_v1_script);
 * }
 * // Release the original plutus_v1_script after use
 * cardano_plutus_v1_script_unref(&original_plutus_v1_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_plutus_v1_script_get_hash(const cardano_plutus_v1_script_t* plutus_v1_script);

/**
 * \brief Checks if two Plutus v1 script objects are equal.
 *
 * This function compares two \ref cardano_plutus_v1_script_t objects to determine if they are equal.
 * Two Plutus v1 scripts are considered equal if they have the same content.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_plutus_v1_script_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_plutus_v1_script_t object to be compared.
 *
 * \return \c true if the two Plutus v1 scripts are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v1_script_t* script1 = ...;  // Assume script1 is initialized here
 * cardano_plutus_v1_script_t* script2 = ...;  // Assume script2 is initialized here
 *
 * if (cardano_plutus_v1_script_equals(script1, script2))
 * {
 *   // The two Plutus v1 scripts are equal
 * }
 * else
 * {
 *   // The two Plutus v1 scripts are not equal
 * }
 *
 * // Clean up the script objects once done
 * cardano_plutus_v1_script_unref(&script1);
 * cardano_plutus_v1_script_unref(&script2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_plutus_v1_script_equals(const cardano_plutus_v1_script_t* lhs, const cardano_plutus_v1_script_t* rhs);

/**
 * \brief Decrements the reference count of a plutus_v1_script object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_plutus_v1_script_t object
 * by decreasing its reference count. When the reference count reaches zero, the plutus_v1_script is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] plutus_v1_script A pointer to the pointer of the plutus_v1_script object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v1_script_t* plutus_v1_script = cardano_plutus_v1_script_new();
 *
 * // Perform operations with the plutus_v1_script...
 *
 * cardano_plutus_v1_script_unref(&plutus_v1_script);
 * // At this point, plutus_v1_script is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_plutus_v1_script_unref, the pointer to the \ref cardano_plutus_v1_script_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_plutus_v1_script_unref(cardano_plutus_v1_script_t** plutus_v1_script);

/**
 * \brief Increases the reference count of the cardano_plutus_v1_script_t object.
 *
 * This function is used to manually increment the reference count of a plutus_v1_script
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_plutus_v1_script_unref.
 *
 * \param plutus_v1_script A pointer to the plutus_v1_script object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_v1_script is a previously created plutus_v1_script object
 *
 * cardano_plutus_v1_script_ref(plutus_v1_script);
 *
 * // Now plutus_v1_script can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_plutus_v1_script_ref there is a corresponding
 * call to \ref cardano_plutus_v1_script_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_plutus_v1_script_ref(cardano_plutus_v1_script_t* plutus_v1_script);

/**
 * \brief Retrieves the current reference count of the cardano_plutus_v1_script_t object.
 *
 * This function returns the number of active references to a plutus_v1_script object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_plutus_v1_script_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param plutus_v1_script A pointer to the plutus_v1_script object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified plutus_v1_script object. If the object
 * is properly managed (i.e., every \ref cardano_plutus_v1_script_ref call is matched with a
 * \ref cardano_plutus_v1_script_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_v1_script is a previously created plutus_v1_script object
 *
 * size_t ref_count = cardano_plutus_v1_script_refcount(plutus_v1_script);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_plutus_v1_script_refcount(const cardano_plutus_v1_script_t* plutus_v1_script);

/**
 * \brief Sets the last error message for a given plutus_v1_script object.
 *
 * Records an error message in the plutus_v1_script's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] plutus_v1_script A pointer to the \ref cardano_plutus_v1_script_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the plutus_v1_script's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_plutus_v1_script_set_last_error(cardano_plutus_v1_script_t* plutus_v1_script, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific plutus_v1_script.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_plutus_v1_script_set_last_error for the given
 * plutus_v1_script. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] plutus_v1_script A pointer to the \ref cardano_plutus_v1_script_t instance whose last error
 *                   message is to be retrieved. If the plutus_v1_script is NULL, the function
 *                   returns a generic error message indicating the null plutus_v1_script.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified plutus_v1_script. If the plutus_v1_script is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_plutus_v1_script_set_last_error for the same plutus_v1_script, or until
 *       the plutus_v1_script is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_plutus_v1_script_get_last_error(const cardano_plutus_v1_script_t* plutus_v1_script);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_V1_SCRIPT_H