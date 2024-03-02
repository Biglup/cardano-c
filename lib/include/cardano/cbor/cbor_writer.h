/**
 * \file cbor_writer.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * \section LICENSE
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

#ifndef CARDANO_CBOR_WRITER_H
#define CARDANO_CBOR_WRITER_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_tag.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A simple writer for Concise Binary Object Representation (CBOR) encoded data.
 */
typedef struct cardano_cbor_writer_t cardano_cbor_writer_t;

/**
 * \brief Creates a new instance of a cardano_cbor_writer_t object and sets its properties.
 *
 * \return A strong reference to the new cardano_cbor_writer_t object. The caller must call
 * \ref cardano_cbor_writer_unref to dispose of the object.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_writer_t* cardano_cbor_writer_new(void);

/**
 * \brief Decreases the reference count of the cardano_cbor_writer_t object. When its reference count drops
 * to 0, the object is finalized (i.e. its memory is freed).
 *
 * \param cbor_writer A pointer to the cbor writer object reference.
 */
CARDANO_EXPORT void cardano_cbor_writer_unref(cardano_cbor_writer_t** cbor_writer);

/**
 * \brief Increases the reference count of the cardano_cbor_writer_t object.
 *
 * \param cbor_writer the cbor writer object.
 */
CARDANO_EXPORT void cardano_cbor_writer_ref(cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Get the cbor_writer's reference count
 *
 * \rst
 * .. warning:: This does *not* account for transitive references.
 * \endrst
 *
 * \param cbor_writer the cbor writer object.
 * \return the reference count.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cbor_writer_refcount(const cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Provides CPP-like move construct
 *
 * Decreases the reference count by one, but does not deallocate the item even
 * if its refcount reaches zero. This is useful for passing intermediate values
 * to functions that increase reference count. Should only be used with
 * functions that `ref` their arguments.
 *
 * \rst
 * .. warning:: If the object is moved without correctly increasing the reference
 *  count afterwards, the memory will be leaked.
 * \endrst
 *
 * \param object Reference to an object
 * \return the object with reference count decreased by one.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_writer_t* cardano_cbor_writer_move(cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Writes the provided value as a tagged bignum encoding, as described in RFC7049 section 2.4.2.
 *
 * \param writer[in] The CBOR writer instance.
 * \param value[in]  The value to write.
 *
 * \return A \c cardano_error_t indicating the result of the operation: \c CARDANO_SUCCESS on success,
 *         or an appropriate error code indicating the failure reason. Refer to \c cardano_error_t documentation
 *         for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_big_integer(cardano_cbor_writer_t* writer, uint64_t value);

/**
 * \brief Writes a boolean value (major type 7).
 *
 * \param writer[in] The CBOR writer instance.
 * \param value The value to write.
 *
 * \return A \c cardano_error_t indicating the result of the operation: \c CARDANO_SUCCESS on success,
 *         or an appropriate error code indicating the failure reason. Refer to \c cardano_error_t documentation
 *         for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_bool(cardano_cbor_writer_t* writer, bool value);

/**
 * \brief Writes a buffer as a byte string encoding (major type 2) into a CBOR stream.
 *
 * This function takes a byte buffer and writes it to the specified CBOR writer as a byte string,
 * following the CBOR encoding rules for byte strings (major type 2). This is used to serialize
 * raw binary data in CBOR format.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               where the byte string will be written. This writer must have been previously
 *               initialized and be in a valid state for writing.
 *
 * \param data A pointer to the byte buffer containing the data to be written as a CBOR byte string.
 *             The data in this buffer will be copied exactly as it is into the CBOR stream, encoded
 *             as a byte string.
 *
 * \param size The size of the data buffer in bytes. This specifies how many bytes from \c data
 *             should be written to the CBOR stream as part of the byte string. The length of the
 *             resulting CBOR byte string will match this size.
 *
 * \return A \c cardano_error_t indicating the result of the operation. Returns \c CARDANO_SUCCESS
 *         on successful writing of the byte string to the CBOR stream. If the operation fails,
 *         an appropriate error code is returned indicating the reason for failure. Refer to the
 *         \c cardano_error_t documentation for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_byte_string(cardano_cbor_writer_t* writer, byte_t* data, size_t size);

/**
 * \brief Writes a given text string as a UTF-8 encoded text string (major type 3) into a CBOR stream.
 *
 * This function serializes a text string, provided as a UTF-8 encoded `char` array, into the CBOR format
 * as a text string (major type 3).
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure that represents the target CBOR stream
 *               where the text string will be written. The writer must have been previously initialized
 *               and be in a state ready for writing data.
 *
 * \param data A pointer to the `char` array containing the UTF-8 encoded text string to be written.
 *
 * \param size The number of bytes (characters) to write from the \c data array to the CBOR stream. This size
 *             should reflect the length of the text string in bytes and does not include a null terminator.
 *
 * \return A \c cardano_error_t value indicating the outcome of the write operation. The function returns
 *         \c CARDANO_SUCCESS if the text string is successfully encoded and written to the CBOR stream.
 *         If the operation encounters an error, such as an invalid parameter or a problem with the
 *         CBOR stream, the function returns an error code that identifies the failure reason. For a detailed
 *         explanation of possible error codes, refer to the documentation on \c cardano_error_t.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_text_string(cardano_cbor_writer_t* writer, const char* data, size_t size);

/**
 * \brief Writes a pre-encoded CBOR data item to the CBOR stream.
 *
 * This function is designed to directly write a CBOR-encoded data item into the CBOR writer stream,
 * bypassing the typical encoding process. It is useful for cases where the data to be written is
 * already in CBOR format and needs to be inserted into the stream as is.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               where the pre-encoded data item will be written. The writer must have been
 *               previously initialized and in a valid state for writing.
 *
 * \param data A pointer to the byte buffer containing the pre-encoded CBOR data item. This buffer
 *             is written directly to the CBOR stream without modification, assuming it is correctly
 *             encoded per CBOR specifications.
 *
 * \param size The size of the pre-encoded data buffer in bytes, indicating how many bytes from
 *             \c data should be written to the CBOR stream. It is the caller's responsibility to
 *             ensure that the data conforms to valid CBOR encoding rules for the intended data item.
 *
 * \return A \c cardano_error_t indicating the result of the operation. Returns \c CARDANO_SUCCESS
 *         if the pre-encoded data item was successfully written to the CBOR stream. If the operation
 *         fails, an appropriate error code is returned to indicate the reason for failure. For
 *         detailed explanations of possible error codes, refer to the \c cardano_error_t documentation.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_encoded(cardano_cbor_writer_t* writer, byte_t* data, size_t size);

/**
 * \brief Writes the start marker for a CBOR array (major type 4) to the CBOR stream.
 *
 * This function initiates the definition of a CBOR array in the output stream managed by the given CBOR writer.
 * It supports both definite-length arrays, where the number of elements in the array is known and specified in advance,
 * and indefinite-length arrays, where the number of elements is not known at the start of the array encoding.
 *
 * For a definite-length array, the `length` parameter should be set to the number of elements that will be included
 * in the array. For an indefinite-length array, `length` should be set to 0, and the array must be explicitly closed
 * using `cardano_cbor_writer_write_end_array` after all elements have been written.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               where the array start marker will be written. This writer must have been previously
 *               initialized and be in a valid state for writing.
 *
 * \param length The number of elements in the array for a definite-length array. Set to 0 to start
 *               an indefinite-length array. In the case of an indefinite-length array, elements can be
 *               written until `cardano_cbor_writer_write_end_array` is called to mark the end of the array.
 *
 * \return A \c cardano_error_t indicating the result of the operation. \c CARDANO_SUCCESS is returned upon
 *         successful writing of the array start marker. If the operation fails, an appropriate error code is
 *         returned indicating the reason for the failure. Refer to the \c cardano_error_t documentation
 *         for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_start_array(cardano_cbor_writer_t* writer, size_t size);

/**
 * \brief Writes the end marker for an indefinite-length array (major type 4) in CBOR format.
 *
 * This function is used to conclude the writing of an indefinite-length array in CBOR encoding.
 * It should be called only after starting an indefinite-length array with
 * cardano_cbor_writer_write_start_array() where the length parameter was set to 0. For definite-length
 * arrays, this function is not needed as the length is predefined. This operation inserts an
 * appropriate end-of-array marker into the CBOR stream to signify the end of the array structure.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               where the end-of-array marker will be written. This writer must have been previously
 *               initialized and must be in a valid state for writing.
 *
 * \return A \c cardano_error_t indicating the result of the operation. Returns \c CARDANO_SUCCESS
 *         on successful insertion of the end-of-array marker into the CBOR stream. If the operation
 *         fails, an appropriate error code is returned indicating the reason for failure. Refer to
 *         the \c cardano_error_t documentation for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_end_array(cardano_cbor_writer_t* writer);

/**
 * \brief Initiates the writing of a CBOR map (major type 5), supporting both definite and indefinite lengths.
 *
 * This function marks the beginning of a map in CBOR encoding. CBOR maps consist of key-value pairs.
 * The function supports both definite-length maps, where the number of key-value pairs is known in advance,
 * and indefinite-length maps, for which the total number of pairs is not predetermined. For definite-length
 * maps, the caller must specify the exact number of pairs to be written. For indefinite-length maps,
 * the caller should pass 0 as the length, and then use cardano_cbor_writer_write_end_map() to denote
 * the end of the map.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure, representing the CBOR stream
 *               to which the map's start will be written. This writer should have been previously
 *               initialized and be in a valid state for writing.
 *
 * \param length Specifies the number of key-value pairs for definite-length maps, or 0 to start
 *               an indefinite-length map. In the case of indefinite-length maps, the end of the map
 *               must be explicitly marked using cardano_cbor_writer_write_end_map().
 *
 * \return A \c cardano_error_t indicating the outcome of the operation: \c CARDANO_SUCCESS on successful
 *         initiation of the map in the CBOR stream, or an error code indicating the reason for failure
 *         for unsuccessful attempts. Consult the \c cardano_error_t documentation for detailed explanations
 *         of possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_start_map(cardano_cbor_writer_t* writer, size_t size);

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
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               where the end-of-map marker will be written. The writer must have been previously
 *               initialized and must be in a valid state for writing.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS
 *         upon successfully marking the end of the map in the CBOR stream. If the operation fails, an
 *         appropriate error code is returned indicating the reason for failure. Refer to the
 *         \c cardano_error_t documentation for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_end_map(cardano_cbor_writer_t* writer);

/**
 * \brief Writes a value as an unsigned integer encoding (major type 0).
 *
 * This function encodes and writes an unsigned integer to the CBOR stream. In CBOR encoding,
 * unsigned integers are represented as major type 0. The function handles the encoding according
 * to the value's size, ensuring optimal CBOR representation. It is suitable for encoding integers
 * in the range of 0 to 2^64-1.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               where the integer will be written. The writer must have been previously
 *               initialized and must be in a valid state for writing.
 *
 * \param value The unsigned integer value to be written to the CBOR stream. The function
 *              supports encoding values in the full unsigned 64-bit integer range.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation: \c CARDANO_SUCCESS on success,
 *         or an appropriate error code indicating the reason for failure. Refer to the
 *         \c cardano_error_t documentation for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_unsigned_int(cardano_cbor_writer_t* writer, uint64_t value);

/**
 * \brief Writes a signed integer value using CBOR encoding (major types 0, 1).
 *
 * This function encodes a signed integer value into CBOR format, automatically handling
 * the selection between major type 0 (positive integers) and major type 1 (negative integers)
 * based on the sign of the input value. Positive values, including zero, are encoded as
 * major type 0, while negative values are encoded as major type 1. This allows for
 * efficient representation of integers within the CBOR stream.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure that represents the CBOR stream
 *               where the integer value will be written. The writer must have been previously
 *               initialized and be in a ready state for writing.
 *
 * \param value The signed integer value to be written to the CBOR stream. The function will
 *              correctly handle both positive and negative values, encoding them as per
 *              CBOR's specification for integer representation.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS
 *         if the integer value is successfully written to the CBOR stream. If the operation fails,
 *         an error code is returned that indicates the reason for failure. Consult the
 *         \c cardano_error_t documentation for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_signed_int(cardano_cbor_writer_t* writer, int64_t value);

/**
 * \brief Writes a null value to the CBOR stream (major type 7).
 *
 * This function encodes a null value into the CBOR stream being written to, following the CBOR
 * specification for null values (major type 7, additional information 22).
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               where the null value will be encoded. This writer must have been previously
 *               initialized and must be in a state ready for writing. The function ensures that
 *               the null value is correctly inserted into the ongoing stream without altering
 *               the stream's integrity.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS
 *         upon successfully encoding the null value into the stream. If the operation encounters
 *         an issue, an error code is returned to indicate the specific failure reason. For a
 *         comprehensive understanding of possible error codes, refer to the \c cardano_error_t
 *         documentation.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_null(cardano_cbor_writer_t* writer);

/**
 * \brief Writes an undefined value to the CBOR stream (major type 7).
 *
 * In CBOR encoding, the "undefined" value is used to represent data that is explicitly unspecified.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure representing the CBOR stream
 *               to which the undefined value will be written. The writer should have been previously
 *               initialized and must be in a valid state for writing. Using this function correctly
 *               maintains the integrity of the CBOR stream by ensuring the undefined value is
 *               properly encoded according to CBOR standards.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. If the undefined value is
 *         successfully encoded into the CBOR stream, \c CARDANO_SUCCESS is returned. Should there be
 *         a failure in writing the value, an appropriate error code is returned, providing insight
 *         into the failure reason. Refer to the \c cardano_error_t documentation for an exhaustive
 *         list of potential error codes and their meanings.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_undefined(cardano_cbor_writer_t* writer);

/**
 * \brief Assigns a semantic tag (major type 6) to the next data item in the CBOR stream.
 *
 * Semantic tags in CBOR are used to give additional meaning or context to the succeeding data item,
 * enabling the representation of more complex data structures and types, such as dates, times, and
 * various domain-specific data types. This function allows for the explicit tagging of the next data
 * item written to the CBOR stream, following the tag with the data item itself to correctly encode
 * the intended meaning as per the CBOR standard.
 *
 * \param writer A pointer to the \c cardano_cbor_writer_t structure that represents the CBOR stream
 *               where the tag will be applied. The writer should have been previously initialized
 *               and must be in a valid state for writing. Proper use of this function ensures that
 *               the tag is correctly prefixed to the subsequent data item in the CBOR encoding sequence.
 *
 * \param tag The semantic tag to be applied. This is a numeric identifier that specifies the type
 *            or meaning of the next data item according to predefined or application-specific CBOR tags.
 *            The use of tags is defined within the CBOR specification and can be extended to include
 *            custom semantic meanings in application-specific contexts.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. If the tag is successfully
 *         written to the stream, \c CARDANO_SUCCESS is returned. If the operation fails, an error code
 *         is returned that provides details on the failure reason. For comprehensive information on
 *         possible error codes and their interpretations, refer to the \c cardano_error_t documentation.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_tag(cardano_cbor_writer_t* writer, cbor_tag_t tag);

/**
 * \brief Encodes the data from the internal context of the writer object into CBOR format and outputs it
 * to a provided buffer.
 *
 * This function encodes data that has been prepared or written to the writer's internal context into the
 * CBOR format. The encoded data is then written to an output buffer provided by the caller.
 *
 * \param writer[in] A pointer to the \c cardano_cbor_writer_t structure representing the context and state
 *               for the CBOR encoding operation.
 *
 * \param data[in] A pointer of a byte array where the encoded data will be written.
 *
 * \param size[in] The size of the data byte array.
 *
 * \param written[out] A pointer to a size_t variable where the function will store the total number of bytes
 *                     written into the encoded data buffer.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. \c CARDANO_SUCCESS is returned if the
 *         data is successfully encoded into CBOR format and stored in the provided buffer. If the operation
 *         fails, an error code is returned that indicates the specific reason for failure. For detailed
 *         information on possible error codes and their meanings, consult the \c cardano_error_t documentation.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_encode(cardano_cbor_writer_t* writer, byte_t* data, size_t size, size_t* written);

/**
 * \brief Creates a new hex string with the writer encoded data.
 *
 * \param writer[in] Source writer.
 *
 * \return The newly null terminated char string with the hex representation or NULL on memory allocation failure.
 * The caller assumes ownership of the returned char* string and is responsible for its lifecycle.
 * It must be freed when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT char* cardano_cbor_writer_encode_hex(cardano_cbor_writer_t* writer);

/**
 * \brief Resets the writer to have no data.
 *
 * \return A \c cardano_error_t indicating the outcome of the operation. \c CARDANO_SUCCESS is returned if the
 *         data is successfully encoded into CBOR format and stored in the provided buffer. If the operation
 *         fails, an error code is returned that indicates the specific reason for failure. For detailed
 *         information on possible error codes and their meanings, consult the \c cardano_error_t documentation.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_reset(cardano_cbor_writer_t* writer);

/**
 * \brief Sets the last error message for a given object.
 *
 * This function records an error message in the object's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_object_get_last_error.
 *
 * \param object A pointer to the cardano_cbor_writer_t instance whose last error
 *               message is to be set. If the object is NULL, the function
 *               has no effect.
 * \param message A null-terminated string containing the error message to be
 *                recorded. If the message is NULL, the object's last_error
 *                will be set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters due to the fixed size
 *       of the last_error buffer (1024 characters), including the null
 *       terminator. Messages longer than this limit will be truncated.
 */
CARDANO_EXPORT void cardano_cbor_writer_set_last_error(cardano_cbor_writer_t* writer, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific object.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by cardano_object_set_last_error for the given
 * object. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param object A pointer to the cardano_cbor_writer_t instance whose last error
 *               message is to be retrieved. If the object is NULL, the function
 *               returns a generic error message indicating the null object.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified object. If the object is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to cardano_object_set_last_error for the same object, or until
 *       the object is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cbor_writer_get_last_error(const cardano_cbor_writer_t* writer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_WRITER_H