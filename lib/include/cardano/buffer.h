/**
 * \file buffer.h
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

#ifndef CARDANO_BUFFER_H
#define CARDANO_BUFFER_H

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
 * \brief A dynamic, reference-counted buffer with configurable exponential growth.
 *
 * \remarks The buffer employs an exponential growth strategy, increasing its capacity by a factor of 1.5 by default
 * when the buffer becomes full. This default growth factor is based a
 * <a href="http://groups.google.com/group/comp.lang.c++.moderated/msg/ba558b4924758e2e">recommendation from Andrew Koenig's</a>
 * (growth factor should be less than (1+sqrt(5))/2 (~1.6)).
 *
 * \note The growth factor of the buffer can be configured at compilation time using the environment variable `BUFFER_GROW_FACTOR`
 **/
typedef struct cardano_buffer_t cardano_buffer_t;

/**
 * \brief Creates a new dynamic buffer with the specified initial capacity.
 *
 * \param capacity[in]   Initial capacity of the buffer. The capacity must be greater than 0.
 * If 0 is provided, this function will return NULL.
 *
 * \return The newly created buffer or NULL on memory allocation failure.
 * The caller assumes ownership of the returned buffer and is responsible for its lifecycle.
 * It must be dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_new(size_t capacity);

/**
 * \brief Creates a new dynamic buffer with a copy of the given data.
 *
 * \param array[in] A pointer to the data to initialize the buffer with.
 * \param size[in] The size of the data.
 *
 * \return The newly created buffer or NULL on memory allocation failure.
 * The caller assumes ownership of the returned buffer and is responsible for its lifecycle.
 * It must be dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_new_from(const byte_t* array, size_t size);

/**
 * \brief Concatenates two buffers into a new one.
 *
 * Creates a new buffer containing the combined data of the provided buffers.
 *
 * \param lhs[in]   The first buffer.
 * \param rhs[in]   The second buffer.
 *
 * \return A new buffer containing the concatenated data or NULL on memory allocation failure.
 * The caller assumes ownership of the returned buffer and must manage its lifecycle,
 * ensuring it is dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_concat(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs);

/**
 * \brief Slices a buffer.
 *
 * Extracts a portion of the buffer between the given indices.
 *
 * \param buffer[in]   Source buffer.
 * \param start[in]    Start index of the slice (inclusive).
 * \param end[in]      End index of the slice (exclusive).
 *
 * \return A new buffer containing the slice or NULL for invalid input or memory allocation failure.
 * The caller assumes ownership of the returned buffer and must manage its lifecycle,
 * ensuring it is dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_slice(const cardano_buffer_t* buffer, size_t start, size_t end);

/**
 * \brief Creates a new buffer from a hex string.
 *
 * \param hex_string[in] A pointer to the hex string.
 * \param size[in] The size of the hex string in bytes.
 *
 * \return The newly created buffer or NULL on memory allocation failure.
 * The caller assumes ownership of the returned buffer and is responsible for its lifecycle.
 * It must be dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_from_hex(const char* hex_string, size_t size);

/**
 * \brief Creates a new hex string from a buffer instance.
 *
 * \param buffer[in] Source buffer.
 *
 * \return The newly null terminated char string with the hex representation or NULL on memory allocation failure.
 * The caller assumes ownership of the returned char* string and is responsible for its lifecycle.
 * It must be freed when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT char* cardano_buffer_to_hex(const cardano_buffer_t* buffer);

/**
 * \brief Decrements the buffer's reference count.
 *
 * If the reference count reaches zero, the buffer memory is deallocated.
 *
 * \param buffer[in] Pointer to the buffer whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_buffer_unref(cardano_buffer_t** buffer);

/**
 * \brief Increments the buffer's reference count.
 *
 * Ensures that the buffer remains allocated until the last reference is released.
 *
 * \param buffer[in] Buffer whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_buffer_ref(cardano_buffer_t* buffer);

/**
 * \brief Retrieves the buffer's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param buffer[in] Target buffer.
 * \return Current reference count of the buffer.
 */
CARDANO_EXPORT size_t cardano_buffer_refcount(const cardano_buffer_t* buffer);

/**
 * \brief Moves a buffer, decrementing its reference count without deallocating.
 *
 * Useful for transferring buffer ownership to functions that will increase the reference count.
 *
 * \warning Memory will leak if the reference count isn't properly managed after a move.
 *
 * \param buffer[in] Buffer to be moved.
 * \return The buffer with its reference count decremented.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_move(cardano_buffer_t* buffer);

/**
 * \brief Fetches a direct pointer to the buffer's data.
 *
 * \warning This returns a pointer to the internal data, not a copy. Do not manually deallocate.
 *
 * \param buffer[in] Target buffer.
 * \return Pointer to the buffer's internal data or NULL if the buffer is empty.
 */
CARDANO_NODISCARD
CARDANO_EXPORT byte_t* cardano_buffer_get_data(const cardano_buffer_t* buffer);

/**
 * \brief Fetches the current size (used space) of the buffer.
 *
 * \param buffer[in] Target buffer.
 * \return Used space in the buffer. Returns 0 if buffer is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_buffer_get_size(const cardano_buffer_t* buffer);

/**
 * \brief Fetches the buffer's total capacity.
 *
 * \param buffer[in] Target buffer.
 * \return Total capacity of the buffer. Returns 0 if buffer is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_buffer_get_capacity(const cardano_buffer_t* buffer);

/**
 * \brief Appends data to the buffer.
 *
 * If required, the buffer's capacity is automatically expanded (typically doubled).
 *
 * \param buffer[in] Target buffer.
 * \param data[in]   Data to append.
 * \param size[in]   Size of the data.
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write(cardano_buffer_t* buffer, const byte_t* data, size_t size);

/**
 * \brief Reads data from the buffer into a provided array.
 *
 * If the buffer contains insufficient data, an error is returned.
 *
 * \param buffer[in]        Source buffer.
 * \param data [out]        Output array where data will be copied.
 * \param bytes_to_read[in] Number of bytes to read.
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read(cardano_buffer_t* buffer, byte_t* data, size_t bytes_to_read);

/**
 * \brief Writes a 16-bit unsigned integer value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   16-bit unsigned integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint16_le(cardano_buffer_t* buffer, uint16_t value);

/**
 * \brief Writes a 32-bit unsigned integer value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   32-bit unsigned integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint32_le(cardano_buffer_t* buffer, uint32_t value);

/**
 * \brief Writes a 64-bit unsigned integer value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   64-bit unsigned integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint64_le(cardano_buffer_t* buffer, uint64_t value);

/**
 * \brief Writes a 16-bit integer value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   16-bit integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int16_le(cardano_buffer_t* buffer, int16_t value);

/**
 * \brief Writes a 32-bit integer value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   32-bit integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int32_le(cardano_buffer_t* buffer, int32_t value);

/**
 * \brief Writes a 64-bit integer value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   64-bit integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int64_le(cardano_buffer_t* buffer, int64_t value);

/**
 * \brief Writes a float value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   float to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_float_le(cardano_buffer_t* buffer, float value);

/**
 * \brief Writes a double value in little-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   double to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_double_le(cardano_buffer_t* buffer, double value);

/**
 * \brief Writes a 16-bit unsigned integer value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   16-bit unsigned integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint16_be(cardano_buffer_t* buffer, uint16_t value);

/**
 * \brief Writes a 32-bit unsigned integer value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   32-bit unsigned integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint32_be(cardano_buffer_t* buffer, uint32_t value);

/**
 * \brief Writes a 64-bit unsigned integer value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   64-bit unsigned integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint64_be(cardano_buffer_t* buffer, uint64_t value);

/**
 * \brief Writes a 16-bit integer value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   16-bit integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int16_be(cardano_buffer_t* buffer, int16_t value);

/**
 * \brief Writes a 32-bit integer value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   32-bit integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int32_be(cardano_buffer_t* buffer, int32_t value);

/**
 * \brief Writes a 64-bit integer value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   64-bit integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int64_be(cardano_buffer_t* buffer, int64_t value);

/**
 * \brief Writes a float value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   float integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_float_be(cardano_buffer_t* buffer, float value);

/**
 * \brief Writes a double value in big-endian format to the buffer.
 *
 * \param buffer[in]  Target buffer for writing.
 * \param value[in]   double integer to be written.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_double_be(cardano_buffer_t* buffer, double value);

/**
 * \brief Reads a 16-bit unsigned integer value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 16-bit unsigned integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint16_le(cardano_buffer_t* buffer, uint16_t* value);

/**
 * \brief Reads a 32-bit unsigned integer value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 32-bit unsigned integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint32_le(cardano_buffer_t* buffer, uint32_t* value);

/**
 * \brief Reads a 64-bit unsigned integer value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 64-bit unsigned integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint64_le(cardano_buffer_t* buffer, uint64_t* value);

/**
 * \brief Reads a 16-bit integer value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 16-bit integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int16_le(cardano_buffer_t* buffer, int16_t* value);

/**
 * \brief Reads a 32-bit integer value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 32-bit integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int32_le(cardano_buffer_t* buffer, int32_t* value);

/**
 * \brief Reads a 64-bit integer value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 64-bit integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int64_le(cardano_buffer_t* buffer, int64_t* value);

/**
 * \brief Reads a float value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a float where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_float_le(cardano_buffer_t* buffer, float* value);

/**
 * \brief Reads a double value in little-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a double where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_double_le(cardano_buffer_t* buffer, double* value);

/**
 * \brief Reads a 16-bit unsigned integer value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 16-bit unsigned integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint16_be(cardano_buffer_t* buffer, uint16_t* value);

/**
 * \brief Reads a 32-bit unsigned integer value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 32-bit unsigned integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint32_be(cardano_buffer_t* buffer, uint32_t* value);

/**
 * \brief Reads a 64-bit unsigned integer value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 64-bit unsigned integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint64_be(cardano_buffer_t* buffer, uint64_t* value);

/**
 * \brief Reads a 16-bit integer value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 16-bit integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int16_be(cardano_buffer_t* buffer, int16_t* value);

/**
 * \brief Reads a 32-bit integer value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 32-bit integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int32_be(cardano_buffer_t* buffer, int32_t* value);

/**
 * \brief Reads a 64-bit integer value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a 64-bit integer where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int64_be(cardano_buffer_t* buffer, int64_t* value);

/**
 * \brief Reads a float value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a float where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_float_be(cardano_buffer_t* buffer, float* value);

/**
 * \brief Reads a double value in big-endian format from the buffer.
 *
 * \param buffer[in]   Source buffer for reading.
 * \param value[out]   Pointer to a double where the result will be stored.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_double_be(cardano_buffer_t* buffer, double* value);

/**
 * \brief Sets the last error message for a given object.
 *
 * This function records an error message in the object's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_object_get_last_error.
 *
 * \param object A pointer to the cardano_buffer_t instance whose last error
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
CARDANO_EXPORT void cardano_buffer_set_last_error(cardano_buffer_t* buffer, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific object.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by cardano_object_set_last_error for the given
 * object. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param object A pointer to the cardano_buffer_t instance whose last error
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
CARDANO_EXPORT const char* cardano_buffer_get_last_error(const cardano_buffer_t* buffer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_BUFFER_H
