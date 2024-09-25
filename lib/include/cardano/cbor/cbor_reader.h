/**
 * \file cbor_reader.h
 *
 * \author luisd.bianchi
 * \date   Mar 14, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader_state.h>
#include <cardano/cbor/cbor_simple_value.h>
#include <cardano/cbor/cbor_tag.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a reader for parsing Concise Binary Object Representation (CBOR) encoded data.
 */
typedef struct cardano_cbor_reader_t cardano_cbor_reader_t;

/**
 * \brief Creates a new instance of a cardano_cbor_reader_t object for reading CBOR encoded data.
 *
 * This function initializes a new CBOR reader object with a given buffer containing CBOR data.
 * The created reader is capable of parsing the CBOR data, allowing for the extraction and
 * interpretation of the encoded information.
 *
 * \param[in] cbor_data Pointer to the buffer containing the CBOR encoded data to be read.
 * \param[in] size The size of the buffer in bytes.
 *
 * \return A pointer to the newly created \ref cardano_cbor_reader_t object. This object
 * represents a "strong reference" to the CBOR reader, meaning that it is fully initialized
 * and ready for use. The caller is responsible for managing the lifecycle of this object.
 * Specifically, once the CBOR reader is no longer needed, the caller must release it by
 * calling \ref cardano_cbor_reader_unref.
 *
 *
 * Usage Example:
 * \code{.c}
 * // Create a new CBOR reader
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(data, size_size);
 *
 * if (reader)
 * {
 *   // Use the reader for CBOR decoding tasks
 *
 *   // Once done, ensure to clean up and release the reader
 *   cardano_cbor_reader_unref(&reader);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_reader_t* cardano_cbor_reader_new(const byte_t* cbor_data, size_t size);

/**
 * \brief Creates a new instance of a cardano_cbor_reader_t object for reading CBOR data from a hexadecimal string.
 *
 * \param[in] hex_string Pointer to the null-terminated string containing the hexadecimal representation of CBOR encoded data.
 * \param[in] size The size of the hex string in bytes, not including the null terminator.
 *
 * \return On success, a pointer to the newly created \ref cardano_cbor_reader_t object. This object
 * represents a "strong reference" to the CBOR reader, indicating that it is fully initialized
 * and ready to parse the decoded CBOR data. The caller is responsible for managing the lifecycle
 * of this object. Specifically, the object must be released by calling \ref cardano_cbor_reader_unref
 * once it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming `hex_data` is a null-terminated string containing hexadecimal CBOR data
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(hex_data, strlen(hex_data));
 *
 * if (reader)
 * {
 *   // Use the reader for CBOR decoding tasks
 *
 *   // Once done, ensure to clean up and release the reader
 *   cardano_cbor_reader_unref(&reader);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_reader_t* cardano_cbor_reader_from_hex(const char* hex_string, size_t size);

/**
 * \brief Decrements the reference count of a CBOR reader object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_cbor_reader_t object
 * by decreasing its reference count. When the reference count reaches zero, the CBOR reader is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] cbor_reader A pointer to the pointer of the CBOR reader object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new();
 *
 * // Perform operations with the reader...
 *
 * cardano_cbor_reader_unref(&reader);
 * // At this point, reader is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_cbor_reader_unref, the pointer to the \ref cardano_cbor_reader_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_cbor_reader_unref(cardano_cbor_reader_t** cbor_reader);

/**
 * \brief Increases the reference count of the cardano_cbor_reader_t object.
 *
 * This function is used to manually increment the reference count of a CBOR reader
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_cbor_reader_unref.
 *
 * \param cbor_reader A pointer to the CBOR reader object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming reader is a previously created CBOR reader object
 *
 * cardano_cbor_reader_ref(reader);
 *
 * // Now reader can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_cbor_reader_ref there is a corresponding
 * call to \ref cardano_cbor_reader_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_cbor_reader_ref(cardano_cbor_reader_t* cbor_reader);

/**
 * \brief Retrieves the current reference count of the cardano_cbor_reader_t object.
 *
 * This function returns the number of active references to a CBOR reader object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_cbor_reader_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param cbor_reader A pointer to the CBOR reader object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified CBOR reader object. If the object
 * is properly managed (i.e., every \ref cardano_cbor_reader_ref call is matched with a
 * \ref cardano_cbor_reader_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming reader is a previously created CBOR reader object
 *
 * size_t ref_count = cardano_cbor_reader_refcount(reader);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cbor_reader_refcount(const cardano_cbor_reader_t* cbor_reader);

/**
 * \brief Reads the next CBOR token from the reader without advancing the reader's internal position.
 *
 * This function examines the next CBOR token in the data stream, allowing the caller to inspect
 * the type and structure of the upcoming data without actually consuming it. This is particularly
 * useful for conditional parsing based on the upcoming data type or for implementing look-ahead
 * parsing strategies in more complex CBOR structures.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the next CBOR token
 * is to be examined.
 * \param[out] state Pointer to a \ref cardano_cbor_reader_state_t variable where the current state of the
 * CBOR reader will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on successful
 * examination of the next CBOR token, or an appropriate error code indicating the reason for failure.
 * Refer to \ref cardano_error_t documentation for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_state_t state;
 * cardano_error_t error = cardano_cbor_reader_peek_state(&state);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Analyze `state` to decide how to proceed
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_peek_state(cardano_cbor_reader_t* reader, cardano_cbor_reader_state_t* state);

/**
 * \brief Retrieves the total number of unread bytes remaining in the CBOR reader's buffer.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the number of unread bytes is to be retrieved.
 * \param[out] bytes_remaining A pointer to a size_t variable where the number of unread bytes
 * remaining in the buffer will be stored. This value represents how much data is available
 * for reading and processing before the end of the buffer is reached.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * size_t bytes_remaining;
 * cardano_error_t error = cardano_cbor_reader_get_bytes_remaining(&bytes_remaining);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   printf("Bytes remaining: %zu\n", bytes_remaining);
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_get_bytes_remaining(cardano_cbor_reader_t* reader, size_t* bytes_remaining);

/**
 * \brief Retrieves the remaining bytes from the CBOR reader.
 *
 * This function copies the remainder of the bytes from the CBOR reader that have not yet been parsed.
 * It's useful for scenarios where the parsing process is either partial or selective, and the caller
 * needs access to the unparsed portion of the data.
 *
 * \param[in] reader The CBOR reader instance from which the remainder bytes are to be retrieved.
 * \param[out] remainder_bytes A buffer with a copy of the remainder bytes. The caller
 * is responsible for releasing this buffer using \ref cardano_buffer_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the remainder bytes were successfully retrieved and copied into
 *         the provided buffer. If the operation fails due to an insufficient buffer size or
 *         other issues, an appropriate error code is returned. Refer to \c cardano_error_t
 *         documentation for details on possible error codes.
 *
 * Example usage:
 * \code{.c}
 * cardano_buffer_t* remainder_bytes_buffer = NULL;
 *
 * if (cardano_cbor_reader_get_remainder_bytes(reader, buffer) == CARDANO_SUCCESS)
 * {
 *   // Use remainder_bytes_buffer as needed
 *   cardano_buffer_unref(&remainder_bytes_buffer);
 * }
 *
 * cardano_cbor_reader_unref(&reader);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_get_remainder_bytes(cardano_cbor_reader_t* reader, cardano_buffer_t** remainder_bytes);

/**
 * \brief Clones a CBOR reader.
 *
 * This function creates a duplicate of the given \ref cardano_cbor_reader_t object.
 * It allocates a new \ref cardano_cbor_reader_t object that is a clone of the provided reader.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t object to be cloned.
 * \param[out] clone On successful cloning, this will point to a newly created
 *                   \ref cardano_cbor_reader_t object. This object represents a "strong reference"
 *                   to the cloned reader, meaning that it is fully initialized and ready for use.
 *                   The caller is responsible for managing the lifecycle of this object.
 *                   Specifically, once the cloned reader is no longer needed, the caller must release it
 *                   by calling \ref cardano_cbor_reader_unref.
 *
 * \return \ref CARDANO_SUCCESS if the reader was successfully cloned, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* original_reader = cardano_cbor_reader_new(...); // Assume original_reader is initialized
 * cardano_cbor_reader_t* cloned_reader = NULL;
 *
 * // Attempt to clone the original reader
 * cardano_error_t result = cardano_cbor_reader_clone(original_reader, &cloned_reader);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the cloned reader
 *
 *   // Once done, ensure to clean up and release the cloned reader
 *   cardano_cbor_reader_unref(&cloned_reader);
 * }
 *
 * // Clean up the original reader object once done
 * cardano_cbor_reader_unref(&original_reader);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_clone(cardano_cbor_reader_t* reader, cardano_cbor_reader_t** clone);

/**
 * \brief Skips the next CBOR data item and advances the reader.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the next CBOR data item
 * is to be skipped.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming `reader` is a previously initialized CBOR reader
 * cardano_error_t error = cardano_cbor_reader_skip_value(reader);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The reader has successfully skipped the next CBOR data item
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_skip_value(cardano_cbor_reader_t* reader);

/**
 * \brief Reads the next CBOR data item, returning a subarray with the encoded value.
 *
 * This function reads the next CBOR data item from the reader and returns a subarray
 * representing the encoded value as a contiguous region of memory. The subarray is
 * returned as a pointer to a \ref cardano_buffer_t object, which contains the encoded
 * data. For data items encoded with indefinite length, the returned subarray includes
 * the "break" byte that marks the end of the data item.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the next
 * CBOR data item is to be read.
 * \param[out] encoded_value A pointer to a pointer to \ref cardano_buffer_t, which will be
 * allocated and populated with the encoded value. The caller is responsible for releasing
 * this buffer using \ref cardano_buffer_unref when it is no longer needed.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * cardano_buffer_t* encoded_value = NULL;
 * cardano_error_t error = cardano_cbor_reader_read_encoded_value(reader, &encoded_value);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Use `encoded_value` for further processing
 *   cardano_buffer_unref(&encoded_value); // Clean up after use
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_encoded_value(cardano_cbor_reader_t* reader, cardano_buffer_t** encoded_value);

/**
 * \brief Reads the next data item as the start of an array (major type 4).
 *
 * This function attempts to read the next data item from the reader as the start of a CBOR array.
 * It decodes the length of the array and provides this information through the size parameter.
 * If the array is of indefinite length, the function returns a negative size value.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the start of the array is to be read.
 * \param[out] size A pointer to an int64_t variable where the size of the array will be stored.
 * If the array is of definite length, size will be set to the number of elements in the array.
 * If the array is of indefinite length, size will be set to a negative value.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * int64_t array_size = 0;
 * cardano_error_t error = cardano_cbor_reader_read_start_array(reader, &array_size);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   if (arraySize >= 0)
 *   {
 *     printf("Definite-length array with %lld elements.\n", array_size);
 *   }
 *   else
 *   {
 *     printf("Indefinite-length array.\n");
 *   }
 *   // Proceed with reading array items
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 *
 * \note The caller should check if the array is of indefinite length by examining
 * if the size is negative, and should handle array items accordingly.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_start_array(cardano_cbor_reader_t* reader, int64_t* size);

/**
 * \brief Reads the end of an array (major type 4).
 *
 * This function is used to read the end marker of an array in CBOR format. For arrays of definite length,
 * this function confirms the end of the array has been reached according to the previously read length.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the end of the array is to be read.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming the start of an array has been read, and items processed
 * cardano_error_t error = cardano_cbor_reader_read_end_array(reader);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   printf("End of array reached.\n");
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 *
 * \note This function should be called after reading all elements of an array to ensure that
 * the array is correctly terminated, especially in the case of indefinite length arrays.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_end_array(cardano_cbor_reader_t* reader);

/**
 * \brief Reads the next data item as a signed integer (major types 0,1).
 *
 * This function decodes and reads the next CBOR data item from the stream as a signed integer.
 * It handles both positive and negative integers according to CBOR's major types 0 and 1 encoding rules.
 * The decoded integer is stored in the provided `value` parameter.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the signed integer
 * is to be read.
 * \param[out] value Pointer to an `int64_t` variable where the decoded integer value will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * int64_t value;
 * cardano_error_t error = cardano_cbor_reader_read_int(reader, &value);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   printf("Decoded integer: %lld\n", (long long)value);
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_int(cardano_cbor_reader_t* reader, int64_t* value);

/**
 * \brief Reads the next data item as an unsigned integer (major type 0).
 *
 * This function decodes and reads the next CBOR data item from the stream as an unsigned integer,
 * following the CBOR major type 0 encoding rules. The decoded unsigned integer is stored in the
 * provided `value` parameter.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the unsigned integer
 * is to be read.
 * \param[out] value Pointer to a `uint64_t` variable where the decoded unsigned integer value will
 * be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t value;
 *
 * cardano_error_t error = cardano_cbor_reader_read_uint(reader, &value);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   printf("Decoded unsigned integer: %llu\n", (unsigned long long)value);
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_uint(cardano_cbor_reader_t* reader, uint64_t* value);

/**
 * \brief Decodes and reads a big integer (bignum) from CBOR format.
 *
 * This function reads and decodes a bignum from a provided CBOR reader, following the
 * encoding format specified in RFC 7049, section 2.4.2. Bignums are used to represent
 * integers that are too large to be represented directly in the available integer types
 * of CBOR. The function interprets the appropriate tag (2 for unsigned bignum) and decodes
 * the integer value, ensuring its correct representation as a bignum in the resulting bigint object.
 *
 * \param[in] reader The \ref cardano_cbor_reader_t instance from which the bignum will be read.
 * \param[out] bigint A pointer to a pointer that will be set to the address of the newly created \ref cardano_bigint_t object
 *                    representing the decoded big integer value. The caller is responsible for managing the memory of this object.
 *
 * \return Returns \ref CARDANO_SUCCESS if the bignum value was successfully decoded and read from the
 *         CBOR stream. If the operation encounters an error, such as invalid parameters or issues with
 *         reading from the stream, an appropriate error code is returned indicating the reason for the failure.
 *         Consult the \ref cardano_error_t documentation for details on possible error codes and their meanings.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, cbor_data_size);
 * if (reader)
 * {
 *   cardano_bigint_t* bigint = NULL;
 *   cardano_error_t result = cardano_cbor_reader_read_bigint(reader, &bigint);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Successfully read bignum from the reader
 *     // Use the bigint
 *   }
 *
 *   // Clean up
 *   cardano_bigint_unref(&bigint);
 *   cardano_cbor_reader_unref(&reader);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_bigint(cardano_cbor_reader_t* reader, cardano_bigint_t** bigint);

/**
 * \brief Reads the next data item as a double-precision floating point number (major type 7).
 *
 * This function decodes and reads the next CBOR data item from the stream as a double-precision floating point number.
 * It handles the floating-point numbers according to CBOR's major type 7 encoding rules.
 * The decoded double is stored in the provided `value` parameter.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the double-precision
 * floating point number is to be read.
 * \param[out] value Pointer to a `double` variable where the decoded floating point value will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * double value;
 *
 * cardano_error_t error = cardano_cbor_reader_read_double(reader, &value);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   printf("Decoded double: %f\n", value);
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_double(cardano_cbor_reader_t* reader, double* value);

/**
 * \brief Reads the next data item as a CBOR simple value (major type 7).
 *
 * This function decodes and reads the next CBOR data item from the stream as a CBOR simple value,
 * according to CBOR's major type 7 encoding rules for simple values (e.g., true, false, null, undefined).
 * The decoded simple value is stored in the provided `value` parameter.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the simple value is to be read.
 * \param[out] value Pointer to a `cardano_cbor_simple_value_t` variable where the decoded simple value will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_simple_value_t value;
 *
 * cardano_error_t error = cardano_cbor_reader_read_simple_value(reader, &value);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Process the decoded simple value
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_simple_value(cardano_cbor_reader_t* reader, cardano_cbor_simple_value_t* value);

/**
 * \brief Reads the next data item as the start of a map (major type 5).
 *
 * This function decodes and reads the next CBOR data item from the stream as the start of a map,
 * following CBOR's major type 5 encoding rules for maps. Maps are collections of key-value pairs.
 * The number of key-value pairs is stored in the provided `size` parameter. A `size` of -1 indicates
 * an indefinite-length map, while a non-negative value indicates a definite-length map with that many
 * key-value pairs.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the start of the map is to be read.
 * \param[out] size Pointer to an `int64_t` variable where the size of the map (number of key-value pairs)
 *                  will be stored. A value of -1 indicates an indefinite-length map.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 * or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * \remark Map contents are consumed as if they were arrays twice the length of the map's declared size.
 * For example, a map of size 1 containing a key of type int with a value of type string
 * must be consumed by successive calls to readInt32 and readTextString.
 *
 * Usage Example:
 * \code{.c}
 * int64_t map_size;
 *
 * cardano_error_t error = cardano_cbor_reader_read_start_map(reader, &map_size);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Process the start of the map, prepare to read key-value pairs
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_start_map(cardano_cbor_reader_t* reader, int64_t* size);

/**
 * \brief Reads the end of a map (major type 5).
 *
 * This function is used to signify the end of reading a map encoded in CBOR format. It is applicable for
 * maps that were started with \ref cardano_cbor_reader_read_start_map, especially for indefinite-length maps
 * where an explicit end marker is needed. For definite-length maps, this function can be used as a validation
 * step to ensure that the expected number of key-value pairs has been read.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the end of the map is to be read.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on successful
 * reading of the map's end marker, or an appropriate error code indicating the reason for failure. Refer
 * to \ref cardano_error_t documentation for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * // After reading all key-value pairs of the map
 * cardano_error_t error = cardano_cbor_reader_read_end_map(reader);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Successfully read the end of the map
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_end_map(cardano_cbor_reader_t* reader);

/**
 * \brief Reads the next data item as a boolean value (major type 7).
 *
 * This function reads and decodes the next data item from the CBOR stream as a boolean value.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the boolean value is to be read.
 * \param[out] value Pointer to a boolean variable where the decoded value will be stored. The
 * value is set to `true` if the CBOR data item represents true, and `false` if it represents false.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on successful
 * reading and decoding of the boolean value, or an appropriate error code indicating the reason for failure.
 * Refer to \ref cardano_error_t documentation for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * bool value;
 * cardano_error_t error = cardano_cbor_reader_read_bool(reader, &value);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Successfully read the boolean value
 *   if (value)
 *   {
 *     printf("The boolean value is true.\n");
 *   }
 *   else
 *   {
 *     printf("The boolean value is false.\n");
 *   }
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_bool(cardano_cbor_reader_t* reader, bool* value);

/**
 * \brief Reads the next data item as a null value (major type 7).
 *
 * This function is used to read a null value from the CBOR stream, indicating the absence of a value.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the null value is to be read.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS if the next
 * data item in the stream is correctly identified as a null value, or an appropriate error code indicating
 * the reason for failure. Refer to \ref cardano_error_t documentation for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * cardano_error_t error = cardano_cbor_reader_read_null(reader);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Successfully read a null value
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 *
 * \note This function advances the reader to the next item in the CBOR stream after successfully reading
 * a null value. If the current data item is not a null value, an error will be returned.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_null(cardano_cbor_reader_t* reader);

/**
 * \brief Reads the next data item as a byte string (major type 2).
 *
 * This function reads a byte string from the CBOR stream. This function is capable of handling both
 * definite-length byte strings and indefinite-length byte strings. In the case of indefinite-length
 * strings, the function concatenates all segments into a single contiguous byte string.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the byte string is to be read.
 * \param[out] byte_string A pointer to a pointer to a \ref cardano_buffer_t structure where the decoded byte
 *                         string will be stored. The caller is responsible for freeing this buffer using
 *                         \ref cardano_buffer_unref when it is no longer needed.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS if the byte string
 * is successfully read from the CBOR stream, or an appropriate error code indicating the reason for failure.
 * Refer to \ref cardano_error_t documentation for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * cardano_buffer_t* byte_string = NULL;
 *
 * cardano_error_t error = cardano_cbor_reader_read_bytestring(reader, &byte_string);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Successfully read a byte string, byte_string contains the data
 *   // Process byte_string as needed
 *   cardano_buffer_unref(&byte_string); // Clean up when done
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 *
 * \note This function allocates a new \ref cardano_buffer_t to hold the byte string's data. It is the caller's
 * responsibility to release this memory by calling \ref cardano_buffer_unref on the buffer pointer.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_bytestring(cardano_cbor_reader_t* reader, cardano_buffer_t** byte_string);

/**
 * \brief Reads the next data item as a text string (major type 3) from the CBOR stream.
 *
 * This function parses the next CBOR data item, expecting it to be a text string (major type 3),
 * and returns its value as a newly created `cardano_buffer_t` instance containing the text string
 * encoded as UTF-8. The function supports both definite-length strings and indefinite-length strings,
 * seamlessly handling the concatenation of string fragments in the case of the latter.
 *
 * \param[in] reader A pointer to the `cardano_cbor_reader_t` instance responsible for reading the CBOR data.
 * \param[out] text_string A pointer to a pointer of `cardano_buffer_t` where the text string will be stored.
 *                On successful execution, this points to a newly allocated buffer containing the text string.
 *                The caller assumes ownership of this buffer and is responsible for deallocating it using the
 *                appropriate function (e.g., `cardano_buffer_unref`) when it is no longer needed.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS if the text string
 * is successfully read from the CBOR stream, or an appropriate error code indicating the reason for failure.
 * Refer to \ref cardano_error_t documentation for details on possible error codes.
 *
 * Usage example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_buffer_t* text_string = NULL;
 *
 * if (cardano_cbor_reader_read_textstring(reader, &text_string) == CARDANO_SUCCESS)
 * {
 *   // Use the text_string for your application logic
 *
 *   // Cleanup
 *   cardano_buffer_unref(&text_string);
 * }
 * else
 * {
 *   printf("Failed to read text string from CBOR data.\n");
 * }
 *
 * cardano_cbor_reader_unref(&reader);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_textstring(cardano_cbor_reader_t* reader, cardano_buffer_t** text_string);

/**
 * \brief Reads the next data item as a semantic tag (major type 6), advancing the reader.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the tag is to be read.
 * \param[out] tag A pointer to a \ref cardano_cbor_tag_t variable where the decoded tag value will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS if the tag is
 * successfully read from the CBOR stream, or an appropriate error code indicating the reason for failure.
 * Refer to \ref cardano_error_t documentation for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_tag_t tag;
 *
 * cardano_error_t error = cardano_cbor_reader_read_tag(reader, &tag);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Successfully read a semantic tag, tag contains the value
 *   // Process the tag as needed
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_read_tag(cardano_cbor_reader_t* reader, cardano_cbor_tag_t* tag);

/**
 * \brief Peeks at the next semantic tag (major type 6) in the CBOR stream without consuming it.
 *
 * This function peeks at the semantic tag ahead in the CBOR data stream. Semantic tags provide additional context
 * to the subsequent data item, according to the CBOR specification (major type 6). This peek operation allows
 * the application to inspect the tag without advancing the reader's position, enabling decisions based on the tag
 * value before fully processing the associated data item.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance from which the tag is to be peeked.
 * \param[out] tag A pointer to a \ref cardano_cbor_tag_t variable where the tag value will be stored if a tag is present.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS if a tag is successfully
 * peeked at, or an appropriate error code indicating the reason for failure. Refer to \ref cardano_error_t
 * documentation for details on possible error codes.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_tag_t tag;
 *
 * cardano_error_t error = cardano_cbor_reader_peek_tag(reader, &tag);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Successfully peeked at the next semantic tag, tag contains the value
 *   // Decision making based on the peeked tag
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 *
 * \note This function does not modify the reader's current position, allowing subsequent operations to
 * process or skip the tag as needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_reader_peek_tag(cardano_cbor_reader_t* reader, cardano_cbor_tag_t* tag);

/**
 * \brief Sets the last error message for a given CBOR reader object.
 *
 * Records an error message in the CBOR reader's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance whose last error message is
 *                   to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the reader's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_cbor_reader_set_last_error(cardano_cbor_reader_t* reader, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific reader.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_cbor_reader_set_last_error for the given
 * reader. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance whose last error
 *                   message is to be retrieved. If the reader is NULL, the function
 *                   returns a generic error message indicating the null reader.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified reader. If the reader is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_cbor_reader_set_last_error for the same reader, or until
 *       the reader is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cbor_reader_get_last_error(const cardano_cbor_reader_t* reader);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_H