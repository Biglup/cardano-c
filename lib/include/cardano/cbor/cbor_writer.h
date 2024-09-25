/**
 * \file cbor_writer.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * Copyright 2023 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_WRITER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_WRITER_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
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
 * \brief A simple writer for Concise Binary Object Representation (CBOR) encoded data.
 *
 * This structure represents a writer that facilitates encoding data into the CBOR format.
 * It abstracts the complexities involved in CBOR encoding, providing a simple interface
 * for creating CBOR data streams. The writer maintains internal state to track the encoding
 * process, ensuring efficient and accurate representation of various data types as per
 * CBOR standards (RFC 7049).
 */
typedef struct cardano_cbor_writer_t cardano_cbor_writer_t;

/**
 * \brief Creates and initializes a new instance of a CBOR writer.
 *
 * This function allocates and returns a new instance of \ref cardano_cbor_writer_t,
 * setting up its internal state for encoding CBOR data.
 *
 * \return A pointer to the newly created \ref cardano_cbor_writer_t object. This object
 * represents a "strong reference" to the CBOR writer, meaning that it is fully initialized
 * and ready for use. The caller is responsible for managing the lifecycle of this object.
 * Specifically, once the CBOR writer is no longer needed, the caller must release it by
 * calling \ref cardano_cbor_writer_unref.
 *
 * Usage Example:
 * \code{.c}
 * // Create a new CBOR writer
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   // Use the writer for CBOR encoding tasks
 *
 *   // Once done, ensure to clean up and release the writer
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_writer_t* cardano_cbor_writer_new(void);

/**
 * \brief Decrements the reference count of a CBOR writer object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_cbor_writer_t object
 * by decreasing its reference count. When the reference count reaches zero, the CBOR writer is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] cbor_writer A pointer to the pointer of the CBOR writer object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * // Perform operations with the writer...
 *
 * cardano_cbor_writer_unref(&writer);
 * // At this point, writer is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_cbor_writer_unref, the pointer to the \ref cardano_cbor_writer_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_cbor_writer_unref(cardano_cbor_writer_t** cbor_writer);

/**
 * \brief Increases the reference count of the cardano_cbor_writer_t object.
 *
 * This function is used to manually increment the reference count of a CBOR writer
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_cbor_writer_unref.
 *
 * \param cbor_writer A pointer to the CBOR writer object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming writer is a previously created CBOR writer object
 *
 * cardano_cbor_writer_ref(writer);
 *
 * // Now writer can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_cbor_writer_ref there is a corresponding
 * call to \ref cardano_cbor_writer_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_cbor_writer_ref(cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Retrieves the current reference count of the cardano_cbor_writer_t object.
 *
 * This function returns the number of active references to a CBOR writer object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_cbor_writer_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param cbor_writer A pointer to the CBOR writer object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified CBOR writer object. If the object
 * is properly managed (i.e., every \ref cardano_cbor_writer_ref call is matched with a
 * \ref cardano_cbor_writer_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming writer is a previously created CBOR writer object
 *
 * size_t ref_count = cardano_cbor_writer_refcount(writer);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cbor_writer_refcount(const cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Encodes and writes a big integer (bignum) in CBOR format.
 *
 * This function writes a provided big integer value as a bignum, following the
 * encoding format specified in RFC 7049, section 2.4.2. Bignums are used to represent
 * integers that are too large to be represented directly in the available integer types
 * of CBOR. The function applies the appropriate tag (2 for unsigned bignum) before encoding
 * the integer value, ensuring its correct interpretation as a bignum in the resulting CBOR data.
 *
 * \param[in] writer The \ref cardano_cbor_writer_t instance to which the bignum will be written.
 * \param[in] bigint The \ref cardano_bigint_t object representing the big integer value to be encoded as a bignum.
 *
 * \return Returns \ref CARDANO_SUCCESS if the bignum value was successfully encoded and written to the
 *         CBOR stream. If the operation encounters an error, such as invalid parameters or issues with
 *         writing to the stream, an appropriate error code is returned indicating the reason for the failure.
 *         Consult the \ref cardano_error_t documentation for details on possible error codes and their meanings.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   // Example large value to encode as bignum
 *   cardano_bigint_t* bigint = NULL;
 *   cardano_bigint_from_string("123456789123456789123456789", strln("123456789123456789123456789"), 10, &bigint);
 *
 *   cardano_error_t result = cardano_cbor_writer_write_bigint(writer, bigint);
 *
 *   ...
 *
 *   cardano_bigint_unref(&bigint);
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_bigint(cardano_cbor_writer_t* writer, const cardano_bigint_t* bigint);

/**
 * \brief Encodes and writes a boolean value in CBOR format as per RFC 7049, section 2.3.
 *
 * This function is responsible for encoding a boolean value (`true` or `false`) and writing it
 * to the CBOR stream managed by the specified \ref cardano_cbor_writer_t instance. Boolean values
 * are encoded in CBOR as major type 7, with specific additional information values to denote
 * `true` (21) or `false` (20).
 *
 * \param[in] writer The \ref cardano_cbor_writer_t instance to which the boolean value will be written.
 * \param[in] value The boolean value to write. A value of \c true will be encoded as a CBOR `true` value,
 *                  and \c false as a CBOR `false` value.
 *
 * \return Returns \ref CARDANO_SUCCESS if the boolean value was successfully encoded and written to the
 *         CBOR stream. If the operation encounters an error, such as invalid parameters or issues with
 *         writing to the stream, an appropriate error code is returned indicating the reason for the failure.
 *         Consult the \ref cardano_error_t documentation for details on possible error codes and their meanings.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_cbor_writer_write_bool(writer, true);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *   // The boolean value 'true' was successfully written as CBOR to the writer
 *   }
 *   else
 *   {
 *   // Handle error
 *   }
 *
     cardano_cbor_writer_unref(&writer);
 * }
 *
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_bool(cardano_cbor_writer_t* writer, bool value);

/**
 * \brief Encodes and writes a byte buffer as a CBOR byte string (major type 2).
 *
 * This function takes a byte buffer and writes it to the specified CBOR writer as a byte string,
 * following the CBOR encoding rules for byte strings (major type 2). This is used to serialize
 * raw binary data in CBOR format.
 *
 * \param[in] writer The \ref cardano_cbor_writer_t instance to which the byte string will be written.
 * \param[in] data A pointer to the byte buffer containing the data to be written as a CBOR byte string.
 *                 The data in this buffer will be copied exactly as it is into the CBOR stream, encoded
 *                 as a byte string.
 * \param[in] size The number of bytes in the \c data buffer that will be written as part of the CBOR
 *                 byte string. This value determines the length of the resulting byte string in the
 *                 CBOR stream.
 *
 * \return Returns \ref CARDANO_SUCCESS if the byte buffer was successfully serialized and written to the
 *         CBOR stream as a byte string. If the function fails to perform the serialization or write operation,
 *         it returns an error code that specifies the nature of the failure. For detailed information on
 *         error codes and their meanings, refer to the \ref cardano_error_t documentation.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 * byte_t data[] = {0xde, 0xad, 0xbe, 0xef}; // Example binary data
 * size_t data_size = sizeof(data);
 *
 * cardano_error_t result = cardano_cbor_writer_write_bytestring(writer, data, data_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The binary data was successfully written as a CBOR byte string
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_cbor_writer_unref(&writer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_bytestring(cardano_cbor_writer_t* writer, const byte_t* data, size_t size);

/**
 * \brief Encodes and writes a UTF-8 encoded text string as a CBOR text string (major type 3).
 *
 * This function serializes a given text string, provided as a UTF-8 encoded `char` array, and writes it
 * into the specified CBOR stream. Text strings are encoded in CBOR as major type 3, following the format
 * outlined in the CBOR specification (RFC 7049).
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t structure that represents the CBOR stream
 *                   where the text string will be written.
 * \param[in] data A pointer to the `char` array containing the UTF-8 encoded text string to be serialized
 *                 and written to the CBOR stream.
 * \param[in] size The length of the text string in bytes (characters), excluding any null terminator.
 *                 This specifies the exact number of bytes to be written from the \c data array to the
 *                 CBOR stream as the text string content.
 *
 * \return Returns \ref CARDANO_SUCCESS if the text string was successfully encoded and written to the
 *         CBOR stream. If the function encounters an error, such as an issue with the parameters provided
 *         or a problem with the CBOR stream, an appropriate error code is returned, indicating the reason
 *         for the failure. Refer to the \ref cardano_error_t documentation for an exhaustive list of error
 *         codes and their respective meanings.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   const char* text = "Hello, CBOR!";
 *   size_t text_length = strlen(text);
 *   cardano_error_t result = cardano_cbor_writer_write_textstring(writer, text, text_length);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Text string successfully written to CBOR stream
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_textstring(cardano_cbor_writer_t* writer, const char* data, size_t size);

/**
 * \brief Writes a buffer containing a pre-encoded CBOR data item into a CBOR stream.
 *
 * This function facilitates the insertion of a CBOR data item that has already been encoded
 * into a CBOR stream. It is particularly useful in scenarios where the data item to be added
 * to the stream exists in its final encoded form and needs to be included without alteration.
 * The function ensures that the pre-encoded data is integrated into the stream seamlessly,
 * maintaining the integrity and structure of the CBOR encoding.
 *
 * \param writer A pointer to the \ref cardano_cbor_writer_t structure representing the CBOR stream
 *               to which the pre-encoded data item is to be added.
 *
 * \param data A pointer to the byte buffer that contains the pre-encoded CBOR data item. This
 *             buffer should contain the data item in a format that complies with CBOR encoding
 *             specifications. The function does not modify the content of this buffer; it simply
 *             adds it to the CBOR stream as-is.
 *
 * \param size The size, in bytes, of the pre-encoded data buffer. This value specifies the number
 *             of bytes to be written from the \c data buffer to the CBOR stream. It is the caller's
 *             responsibility to ensure that this size accurately reflects the length of the pre-encoded
 *             data item and that the data itself adheres to the correct CBOR encoding standards.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. If the pre-encoded data
 *         item is successfully added to the CBOR stream, the function returns \ref CARDANO_SUCCESS. If
 *         an error occurs during the operation, the function returns an error code identifying the
 *         specific failure reason. Consult the documentation for \ref cardano_error_t for a detailed
 *         list of potential error codes and their respective meanings.
 *
 * Example usage:
 * \code{.c}
 * // Assume writer is a previously initialized CBOR writer object
 * byte_t pre_encoded_data[] = { ... }; // Pre-encoded CBOR data
 * size_t data_size = sizeof(pre_encoded_data);
 *
 * cardano_error_t result = cardano_cbor_writer_write_encoded(writer, pre_encoded_data, data_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Pre-encoded data was successfully added to the CBOR stream
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_encoded(cardano_cbor_writer_t* writer, const byte_t* data, size_t size);

/**
 * \brief Initiates the writing of an array in the CBOR stream, supporting both definite and indefinite lengths.
 *
 * This function marks the start of an array within the CBOR data stream being constructed. CBOR arrays can be of
 * definite length, where the total number of elements is known ahead of time, or of indefinite length, where the
 * total number of elements is not predetermined. This function allows the caller to specify which type of array
 * is being started by setting the \c size parameter accordingly.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t instance that is being used to construct the CBOR stream.
 * \param[in] size   The total number of elements expected in the array for definite-length arrays, or -1 for starting an
 *                   indefinite-length array. In the case of an indefinite-length array, the writing process is open-ended
 *                   until the \ref cardano_cbor_writer_write_end_array function is called to signal the end of the array.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the array
 *         start marker is successfully written to the stream. If an error occurs (e.g., due to an invalid \c writer pointer,
 *         or if \c writer is not in a ready state), an error code corresponding to the specific failure reason is returned.
 *         Consult the \ref cardano_error_t enumeration for a detailed list of error codes.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   // Start a definite-length array with 3 elements
 *   cardano_error_t result = cardano_cbor_writer_write_start_array(writer, 3);
 *
 *   if (result != CARDANO_SUCCESS)
 *   {
 *       // Handle error
 *   }
 *
 *   // Alternatively, start an indefinite-length array
 *   result = cardano_cbor_writer_write_start_array(writer, -1);
 *
 *   if (result != CARDANO_SUCCESS)
 *   {
 *     // Handle error
 *   }
 *
 *   // Proceed with writing array elements...
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 *
 * \note For indefinite-length arrays, you must call \ref cardano_cbor_writer_write_end_array after writing all
 *       array elements to properly close the array in the CBOR stream.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_start_array(cardano_cbor_writer_t* writer, int64_t size);

/**
 * \brief Concludes the encoding of an indefinite-length array in the CBOR stream.
 *
 * This function marks the end of an indefinite-length array that was initiated with
 * \ref cardano_cbor_writer_write_start_array using a length parameter of 0. It writes an
 * end-of-array marker to the CBOR stream, signaling the closure of the array structure.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t instance representing the CBOR stream
 *                   to which the end-of-array marker is to be added. It is imperative that this function
 *                   is called only after a corresponding call to \ref cardano_cbor_writer_write_start_array
 *                   with a length of 0.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. On successful addition
 *         of the end-of-array marker, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 *         error, an error code specifying the nature of the issue is returned. Refer to the \ref cardano_error_t
 *         enumeration for an exhaustive list of error codes and their corresponding meanings.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_cbor_writer_write_start_array(writer, 0); // Start indefinite-length array
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Write array elements...
 *     result = cardano_cbor_writer_write_end_array(writer); // End the indefinite-length array
 *
 *     if (result != CARDANO_SUCCESS)
 *     {
 *       // Handle error
 *     }
 *   }
 *
 *     cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 *
 * \note The function is specifically designed for arrays of indefinite length. Attempting to use this
 *       function without a preceding call to \ref cardano_cbor_writer_write_start_array with a length
 *       parameter of 0 may result in incorrect CBOR stream formation and should be avoided.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_end_array(cardano_cbor_writer_t* writer);

/**
 * \brief Starts the writing of a map (key-value pairs) in the CBOR stream.
 *
 * This function is used to begin the encoding of a CBOR map, which consists of key-value pairs. It supports
 * the creation of both definite-length maps, where the number of key-value pairs is known upfront, and
 * indefinite-length maps, where the number of pairs is determined as the map is being written. The
 * definite-length map is initiated by specifying the exact number of pairs, while the indefinite-length
 * map is started by passing -1 as the length parameter.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t structure that represents the CBOR stream
 *                   where the map will be encoded.
 * \param[in] size   The number of key-value pairs for a definite-length map, or -1 to indicate the start
 *                   of an indefinite-length map. For indefinite-length maps, the end must be explicitly
 *                   marked using \ref cardano_cbor_writer_write_end_map to complete the map encoding.
 *
 * \return A \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the
 *         map's initiation is successfully written to the stream. If the operation encounters an error, an
 *         error code is returned specifying the failure reason. Refer to the \ref cardano_error_t enumeration
 *         for details on the possible error codes and their meanings.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   // Start a definite-length map with 2 key-value pairs
 *   cardano_error_t result = cardano_cbor_writer_write_start_map(writer, 2);
 *
 *   if (result != CARDANO_SUCCESS)
 *   {
 *     // Handle error
 *   }
 *
 *   // Alternatively, start an indefinite-length map
 *   result = cardano_cbor_writer_write_start_map(writer, -1);
 *   if (result != CARDANO_SUCCESS)
 *   {
 *     // Handle error
 *   }
 *
 *   // Proceed with writing key-value pairs...
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 *
 * \note For indefinite-length maps, you must call \ref cardano_cbor_writer_write_end_map after adding
 *       all key-value pairs to correctly close the map structure in the CBOR stream.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_start_map(cardano_cbor_writer_t* writer, int64_t size);

/**
 * \brief Concludes the writing of an indefinite-length map (major type 5) in CBOR format.
 *
 * This function is used to signify the end of an indefinite-length map in a CBOR stream. It is specifically
 * applicable when a map was started with cardano_cbor_writer_write_start_map() with a length parameter of 0,
 * indicating an indefinite-length map. Definite-length maps, where the total number of key-value pairs is known
 * and specified at the start, do not require this function to mark their end, as their boundaries are implicitly
 * defined by the length parameter.
 *
 * It is important to ensure that all key-value pairs intended for the map have been written before calling this
 * function, as it marks the map's closure in the CBOR encoding. After calling this function, new data items written
 * to the stream will not be considered part of the map.
 *
 * \param writer A pointer to the \ref cardano_cbor_writer_t structure representing the CBOR stream
 *               where the end-of-map marker will be written. The writer must have been previously
 *               initialized and must be in a valid state for writing.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         upon successfully marking the end of the map in the CBOR stream. If the operation fails, an
 *         appropriate error code is returned indicating the reason for failure. Refer to the
 *         \ref cardano_error_t documentation for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_end_map(cardano_cbor_writer_t* writer);

/**
 * \brief Encodes an unsigned integer as a CBOR data item and writes it to the CBOR stream.
 *
 * This function is designed to encode an unsigned integer value, following CBOR's major type 0
 * specification, and then write this encoded value to the specified CBOR stream. It automatically
 * optimizes the encoding based on the integer's value to ensure efficient representation within the
 * CBOR format. This function supports encoding of the full range of unsigned 64-bit integers, from 0
 * up to and including 2^64-1.
 *
 * \param[in] writer A pointer to a \ref cardano_cbor_writer_t instance representing the CBOR stream to
 *                   which the encoded unsigned integer will be written.
 * \param[in] value The unsigned 64-bit integer value to be encoded and written to the CBOR stream. This
 *                  value is encoded according to the CBOR major type 0 rules.
 *
 * \return Returns \ref CARDANO_SUCCESS if the unsigned integer is successfully encoded and written to
 *         the stream. If an error occurs during the process, a specific error code from the \ref cardano_error_t
 *         enumeration is returned to indicate the failure reason.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   uint64_t value = 12345; // Example unsigned integer
 *   cardano_error_t result = cardano_cbor_writer_write_uint(writer, value);
 *
 *   if (result != CARDANO_SUCCESS)
 *   {
 *     // Handle error
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_uint(cardano_cbor_writer_t* writer, uint64_t value);

/**
 * \brief Encodes and writes a signed integer value to the CBOR stream, using CBOR major types 0 or 1.
 *
 * This function takes a signed integer value as input and encodes it into the CBOR format, automatically
 * choosing between major type 0 for positive integers (including zero) and major type 1 for negative integers.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t instance tasked with constructing the CBOR stream.
 * \param[in] value The signed 64-bit integer to be encoded and written to the stream. The function handles both
 *                  positive and negative integers, automatically applying the correct CBOR major type based on
 *                  the value's sign.
 *
 * \return Returns \ref CARDANO_SUCCESS upon successful encoding and writing of the signed integer value to the
 *         CBOR stream. In the event of an error, a specific error code from the \ref cardano_error_t enumeration
 *         is returned to detail the cause of the problem.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   int64_t value = -42; // Example negative integer
 *   cardano_error_t result = cardano_cbor_writer_write_signed_int(writer, value);
 *
 *   if (result != CARDANO_SUCCESS)
 *   {
 *     // Handle error
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_signed_int(cardano_cbor_writer_t* writer, int64_t value);

/**
 * \brief Encodes a null value and writes it to the CBOR stream.
 *
 * This function is responsible for encoding a null value as per the CBOR specification
 * (major type 7, additional information 22) and writing it to the specified CBOR stream.
 * Null values are used to represent the absence of any meaningful value in CBOR data.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t structure representing the CBOR stream
 *                   where the null value will be written.
 *
 * \return Returns \ref CARDANO_SUCCESS if the null value is successfully encoded and written to
 *         the stream. If an error occurs, an error code from the \ref cardano_error_t enumeration
 *         is returned, indicating the failure reason.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_cbor_writer_write_null(writer);
 *
 *   if (result != CARDANO_SUCCESS)
 *   {
 *     // Handle error
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 * \endcode
 *
 * \note
 *   It is crucial that the writer is correctly initialized and in a state that permits
 *   the addition of new encoded data to ensure the successful writing of the null value.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_null(cardano_cbor_writer_t* writer);

/**
 * \brief Writes an undefined value to the CBOR stream (major type 7).
 *
 * In CBOR encoding, the "undefined" value is used to represent data that is explicitly unspecified.
 *
 * \param writer A pointer to the \ref cardano_cbor_writer_t structure representing the CBOR stream
 *               to which the undefined value will be written. The writer should have been previously
 *               initialized and must be in a valid state for writing. Using this function correctly
 *               maintains the integrity of the CBOR stream by ensuring the undefined value is
 *               properly encoded according to CBOR standards.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. If the undefined value is
 *         successfully encoded into the CBOR stream, \ref CARDANO_SUCCESS is returned. Should there be
 *         a failure in writing the value, an appropriate error code is returned, providing insight
 *         into the failure reason. Refer to the \ref cardano_error_t documentation for an exhaustive
 *         list of potential error codes and their meanings.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_undefined(cardano_cbor_writer_t* writer);

/**
 * \brief Assigns a semantic tag to the next data item in the CBOR stream.
 *
 * This function is used to give additional meaning or context to the succeeding data item by assigning a semantic tag to it,
 * according to the CBOR specification. It's useful for indicating the type or purpose of the data that follows the tag,
 * such as date-time strings, big numbers, or other specific data types.
 *
 * \param[in] writer The CBOR writer instance to which the tag will be written.
 *
 * \param[in] tag The semantic tag to assign to the next data item. It's an integer that represents the specific meaning
 * or data type according to the predefined set of CBOR tags or potentially application-defined tags.
 *
 * \return A \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS if the tag is successfully
 * written to the CBOR stream. If the operation fails, an appropriate error code is returned to indicate the failure reason.
 * See the documentation for \ref cardano_error_t for a list of error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_tag(cardano_cbor_writer_t* writer, cardano_cbor_tag_t tag);

/**
 * \brief Calculates the required buffer size for the encoded data.
 *
 * This function determines the size of the buffer needed to store the  data encoded by the writer.
 * It is intended to be used before calling `cardano_cbor_writer_encode` to allocate a buffer of appropriate size.
 *
 * \param[in] writer The source writer whose encoded data size is being calculated.
 *
 * \return The size of the buffer required to store the writer's encoded data. Returns 0 if the writer is NULL
 * or contains no data.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * // Assume writer has been used to encode some CBOR data here
 *
 * size_t required_size = cardano_cbor_writer_get_encode_size(writer);
 *
 * // Now required_size can be used to allocate a buffer of appropriate size
 * byte_t* data = (byte_t*)malloc(required_size);
 *
 * if (byte_t != NULL)
 * {
 *   if (cardano_cbor_writer_encode(writer, data, required_size) == CARDANO_SUCCESS)
 *   {
 *     // Operate on the encoded data
 *   }
 *
 *   free(data); // Free the allocated string after use
 * }
 *
 * cardano_cbor_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cbor_writer_get_encode_size(cardano_cbor_writer_t* writer);

/**
 * \brief Encodes data from the writer's context into CBOR format and outputs it to a provided buffer.
 *
 * This function encodes data prepared in the writer's internal context into CBOR format and writes it into an output
 * buffer specified by the caller.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t structure representing the context and state for the CBOR encoding
 *                   operation.
 * \param[in] data A pointer to a byte array where the encoded data will be written. The function will fill this buffer with
 *                 the CBOR-encoded data.
 * \param[in] size The size of the data byte array, indicating how much space is available for writing the encoded data.
 *
 * \return A cardano_error_t indicating the outcome of the operation. Returns CARDANO_SUCCESS if the data is successfully
 *         encoded into CBOR format and stored in the provided buffer. If the operation fails, an error code is returned indicating
 *         the specific reason for failure. Detailed information on possible error codes and their meanings can be found in the
 *         cardano_error_t documentation.
 *
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * byte_t buffer[256]; // Pre-allocated buffer with sufficient space for the encoded data
 *
 * cardano_error_t result = cardano_cbor_writer_encode(writer, buffer, sizeof(buffer));
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Encoded CBOR data successfully. Bytes written: %zu\n", sizeof(buffer));
 * }
 * else
 * {
 *   printf("Failed to encode CBOR data. Error code: %d\n", result);
 * }
 *
 * cardano_cbor_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_encode(cardano_cbor_writer_t* writer, byte_t* data, size_t size);

/**
 * \brief Encodes data from the writer's context into CBOR format and outputs it into a new buffer.
 *
 * This function encodes data prepared in the writer's internal context into CBOR format and writes it into a
 * buffer. The function will create a new buffer instance to store the encoded data.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t structure representing the context and state for the CBOR encoding
 *                   operation.
 * \param[out] buffer A pointer to a \ref cardano_buffer_t object where the encoded data will be stored. The function will allocate
 *                    and fill this buffer with the CBOR-encoded data. It is best to initialize this pointer as NULL. The caller is
 *                    responsible for managing the lifecycle of the buffer, specifically releasing it by calling \ref cardano_buffer_unref
 *                    when it is no longer needed.
 *
 * \return A cardano_error_t indicating the outcome of the operation. Returns CARDANO_SUCCESS if the data is successfully
 *         encoded into CBOR format and stored in the provided buffer. If the operation fails, an error code is returned indicating
 *         the specific reason for failure. Detailed information on possible error codes and their meanings can be found in the
 *         cardano_error_t documentation.
 *
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 * cardano_buffer_t* buffer = NULL; // Buffer will be allocated by the function
 *
 * cardano_error_t result = cardano_cbor_writer_encode_in_buffer(writer, &buffer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Encoded CBOR data successfully. Bytes written: %zu\n", cardano_buffer_get_size(buffer));
 * }
 * else
 * {
 *   printf("Failed to encode CBOR data. Error code: %d\n", result);
 * }
 *
 * cardano_buffer_unref(&buffer); // Properly dispose of the buffer after use
 * cardano_cbor_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_encode_in_buffer(cardano_cbor_writer_t* writer, cardano_buffer_t** buffer);

/**
 * \brief Calculates the required buffer size for the hex string representation of the encoded data.
 *
 * This function determines the size of the buffer needed to store the hexadecimal string representation
 * of the data encoded by the writer. This size includes space for the null terminator. It is intended to
 * be used before calling `cardano_cbor_writer_encode_hex` to allocate a buffer of appropriate size.
 *
 * \param[in] writer The source writer whose encoded data's hex string size is being calculated.
 *
 * \return The size of the buffer required to store the hex string representation of the writer's
 * encoded data, including the null terminator. Returns 0 if the writer is NULL or contains no data.
 *
 * Example usage:
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * // Assume writer has been used to encode some CBOR data here
 *
 * // Calculate the required size for the hex string, including the null terminator
 * size_t required_size = cardano_cbor_writer_get_hex_size(writer);
 *
 * // Now required_size can be used to allocate a buffer of appropriate size
 * char* hex_string = (char*)malloc(required_size);
 *
 * if (hex_string != NULL)
 * {
 *   if (cardano_cbor_writer_encode_hex(writer, hex_string, required_size) == CARDANO_SUCCESS)
 *   {
 *     printf("Encoded CBOR data as hex: %s\n", hex_string);
 *   }
 *
 *   free(hex_string); // Free the allocated string after use
 * }
 *
 * cardano_cbor_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cbor_writer_get_hex_size(const cardano_cbor_writer_t* writer);

/**
 * \brief Writes the encoded data from the writer into a provided buffer as a hex string.
 *
 * This function converts the encoded data prepared in the writer's internal context into a
 * hexadecimal string representation and writes it into the provided buffer. This approach
 * is useful for debugging, logging, or any scenario where viewing the encoded data as a
 * hex string is beneficial. The function requires the caller to allocate a buffer of
 * appropriate size beforehand, based on the size obtained from \ref cardano_cbor_writer_get_hex_size.
 *
 * \param[in] writer The source writer whose data will be encoded into a hex string.
 * \param[out] dest The destination buffer where the hex string will be written.
 * \param[in] dest_size The size of the destination buffer, including space for the null terminator.
 * The required size can be obtained using \ref cardano_cbor_writer_get_hex_size.
 *
 * \return A `cardano_error_t` indicating the result of the operation: `CARDANO_SUCCESS` on success,
 * or an appropriate error code indicating the failure reason. Refer to \ref cardano_error_t documentation
 * for details on possible error codes.
 *
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * // Assume writer has been used to encode some CBOR data here
 *
 * size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
 * char* hex_string = (char*)malloc(hex_size);
 * cardano_error_t result = cardano_cbor_writer_encode_hex(writer, hex_string, hex_size);
 *
 * if (hex_string != NULL && result == CARDANO_SUCCESS)
 * {
 *   printf("Encoded CBOR data as hex: %s\n", hex_string);
 * }
 * else
 * {
 *   printf("Failed to encode CBOR data as hex or allocate memory.\n");
 * }
 *
 * free(hex_string); // Free the allocated string after use
 *
 * cardano_cbor_writer_unref(&writer); // Properly dispose of the writer after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_encode_hex(const cardano_cbor_writer_t* writer, char* dest, size_t dest_size);

/**
 * \brief Resets the writer, clearing all written data.
 *
 * This function resets the internal state of the CBOR writer, effectively removing any data that has been written to it.
 * This is useful for reusing a writer instance without needing to create a new one, especially when working with
 * sequences of data encoding tasks.
 *
 * \param[in] writer The CBOR writer instance to reset.
 *
 * \return A cardano_error_t indicating the outcome of the operation. CARDANO_SUCCESS is returned if the writer is
 *         successfully reset. If the operation fails, an error code is returned that indicates the specific reason for
 *         failure. For detailed information on possible error codes and their meanings, consult the cardano_error_t
 *         documentation.
 *
 * \code{.c}
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * // Write some data to the writer here
 *
 * cardano_error_t reset_result = cardano_cbor_writer_reset(writer);
 *
 * if (reset_result == CARDANO_SUCCESS)
 * {
 *     printf("Writer reset successfully.\n");
 * }
 * else
 * {
 *     printf("Failed to reset writer. Error code: %d\n", reset_result);
 * }
 * // The writer can now be reused for new data without creating a new instance
 *
 * cardano_cbor_writer_unref(&writer); // Clean up when done using the writer
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_reset(cardano_cbor_writer_t* writer);

/**
 * \brief Sets the last error message for a given CBOR writer object.
 *
 * Records an error message in the CBOR writer's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t instance whose last error message is
 *                   to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the writer's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_cbor_writer_set_last_error(cardano_cbor_writer_t* writer, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific writer.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_cbor_writer_set_last_error for the given
 * writer. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] writer A pointer to the \ref cardano_cbor_writer_t instance whose last error
 *                   message is to be retrieved. If the writer is NULL, the function
 *                   returns a generic error message indicating the null writer.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified writer. If the writer is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_cbor_writer_set_last_error for the same writer, or until
 *       the writer is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cbor_writer_get_last_error(const cardano_cbor_writer_t* writer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_WRITER_H