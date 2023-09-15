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
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief A reference counted dynamic buffer.
 *
 * This buffer grows automatically as data is added.
 * Its capacity doubles when the size reaches the current capacity.
 */
typedef struct cardano_buffer_t cardano_buffer_t;

/**
 * \brief Creates and initializes an cardano_buffer_t. This is not a reference counted object.
 *
 * \param capacity The initial capacity of the buffer.
 * \return A pointer to the created buffer, or NULL if memory allocation fails.
 */
cardano_buffer_t* cardano_buffer_new(size_t capacity);

/**
 * \brief Concatenates two buffers and returns their contents in a newly created buffer.
 *
 * \param lhs The first buffer that will be added to the new array.
 * \param lhs The second buffer that will be added to the new array.
 *
 * \return A pointer to the newly created buffer, or NULL if memory allocation fails.
 */
cardano_buffer_t* cardano_buffer_concat(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs);

/**
 * \brief Returns a section of an array.
 *
 * \param buffer The buffer to get the slice from.
 * \param start  The beginning of the specified portion of the array.
 * \param end    The end of the specified portion of the array. This is exclusive of the element at the index 'end'.
 *
 * \return A new buffer with the requested slice. This method will return NULL if: buffer is NULL, start or end are of
 * bounds, end is bigger than start or start and end are equals.
 */
cardano_buffer_t* cardano_buffer_slice(const cardano_buffer_t* buffer, size_t start, size_t end);

/**
 * \brief Decreases the reference count of the cardano_buffer_t object. When its reference count drops
 * to 0, the object is finalized (i.e. its memory is freed).
 *
 * \param buffer A pointer to the cbor writer object reference.
 */
void cardano_buffer_unref(cardano_buffer_t** buffer);

/**
 * \brief Increases the reference count of the cardano_buffer_t object.
 *
 * \param buffer the cbor writer object.
 */
void cardano_buffer_ref(cardano_buffer_t* buffer);

/**
 * \brief Get the buffer's reference count
 *
 * \rst
 * .. warning:: This does *not* account for transitive references.
 * \endrst
 *
 * \param buffer the cbor writer object.
 * \return the reference count
 */
size_t cardano_buffer_refcount(const cardano_buffer_t* buffer);

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
 * \return the object with reference count decreased by one
 */
cardano_buffer_t* cardano_buffer_move(cardano_buffer_t* buffer);

/**
 * \brief Gets a pointer to the buffers data.
 *
 * \rst
 * .. warning:: This is a pointer to the inner data of the buffer and not a copy. Do not free this pointer.
 * \endrst
 *
 * \param buffer The buffer instance.
 * \return A pointer to the inner byte array. If the given buffer is NULL, this function will return NULL.
 */
byte_t* cardano_buffer_get_data(const cardano_buffer_t* buffer);

/**
 * \brief Gets buffer size.
 *
 * \param buffer The buffer instance.
 * \return The buffer size. If the given buffer is NULL, this function will return 0.
 */
size_t cardano_buffer_get_size(const cardano_buffer_t* buffer);

/**
 * \brief Gets buffer capacity.
 *
 * \param buffer The buffer instance.
 * \return The buffer capacity. If the given buffer is NULL, this function will return 0.
 */
size_t cardano_buffer_get_capacity(const cardano_buffer_t* buffer);

/**
 * \brief Adds data to the end of the buffer.
 *
 * If the buffer's size reaches its capacity, the buffer's capacity is doubled.
 *
 * \param buffer The buffer to which the data is added.
 * \param data The data to add to the buffer.
 * \param size The size of the data to be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write(cardano_buffer_t* buffer, byte_t* data, size_t size);

/**
 * \brief Reads data from the buffer. If not enough data is present in the buffer. This function will return an error.
 *
 * \param buffer[in] The buffer to which the data is added.
 * \param data[out] The array where the data will be stored.
 * \param bytes_to_read[in] The size of the data to be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read(cardano_buffer_t* buffer, byte_t* data, size_t bytes_to_read);

/**
 * \brief Writes an uint16_t value as little-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_uint16_le(cardano_buffer_t* buffer, uint16_t value);

/**
 * \brief Writes an uint32_t value as little-endian into the given buffer.
 *
 * \param value[in] The value to be written into the buffer.
 * \param buffer[in] The buffer where the value will be written to.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_uint32_le(cardano_buffer_t* buffer, uint32_t value);

/**
 * \brief Writes an uint64_t value as little-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_uint64_le(cardano_buffer_t* buffer, uint64_t value);

/**
 * \brief Writes an int16_t value as little-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_int16_le(cardano_buffer_t* buffer, int16_t value);

/**
 * \brief Writes an int32_t value as little-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_int32_le(cardano_buffer_t* buffer, int32_t value);

/**
 * \brief Writes an int64_t value as little-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_int64_le(cardano_buffer_t* buffer, int64_t value);

/**
 * \brief Writes an float value as little-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_float_le(cardano_buffer_t* buffer, float value);

/**
 * \brief Writes an double value as little-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_double_le(cardano_buffer_t* buffer, double value);

/**
 * \brief Writes an uint16_t value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_uint16_be(cardano_buffer_t* buffer, uint16_t value);

/**
 * \brief Writes an uint32_t value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_uint32_be(cardano_buffer_t* buffer, uint32_t value);

/**
 * \brief Writes an uint64_t value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_uint64_be(cardano_buffer_t* buffer, uint64_t value);

/**
 * \brief Writes an int16_t value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_int16_be(cardano_buffer_t* buffer, int16_t value);

/**
 * \brief Writes an int32_t value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_int32_be(cardano_buffer_t* buffer, int32_t value);

/**
 * \brief Writes an int64_t value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_int64_be(cardano_buffer_t* buffer, int64_t value);

/**
 * \brief Writes an float value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_float_be(cardano_buffer_t* buffer, float value);

/**
 * \brief Writes an double value as big-endian into the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[in] The value to be written into the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_write_double_be(cardano_buffer_t* buffer, double value);

/**
 * \brief Reads a uint16_t value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_uint16_le(cardano_buffer_t* buffer, uint16_t* value);

/**
 * \brief Reads a uint32_t value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_uint32_le(cardano_buffer_t* buffer, uint32_t* value);

/**
 * \brief Reads a uint64_t value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_uint64_le(cardano_buffer_t* buffer, uint64_t* value);

/**
 * \brief Reads a int16_t value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_int16_le(cardano_buffer_t* buffer, int16_t* value);

/**
 * \brief Reads a int32_t value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_int32_le(cardano_buffer_t* buffer, int32_t* value);

/**
 * \brief Reads a int64_t value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_int64_le(cardano_buffer_t* buffer, int64_t* value);

/**
 * \brief Reads a float value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_float_le(cardano_buffer_t* buffer, float* value);

/**
 * \brief Reads a double value as little-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_double_le(cardano_buffer_t* buffer, double* value);

/**
 * \brief Reads a uint16_t value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_uint16_be(cardano_buffer_t* buffer, uint16_t* value);

/**
 * \brief Reads a uint32_t value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_uint32_be(cardano_buffer_t* buffer, uint32_t* value);

/**
 * \brief Reads a uint64_t value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_uint64_be(cardano_buffer_t* buffer, uint64_t* value);

/**
 * \brief Reads a int16_t value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_int16_be(cardano_buffer_t* buffer, int16_t* value);

/**
 * \brief Reads a int32_t value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_int32_be(cardano_buffer_t* buffer, int32_t* value);

/**
 * \brief Reads a int64_t value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_int64_be(cardano_buffer_t* buffer, int64_t* value);

/**
 * \brief Reads a float value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_float_be(cardano_buffer_t* buffer, float* value);

/**
 * \brief Reads a double value as big-endian from the given buffer.
 *
 * \param buffer[in] The buffer where the value will be written to.
 * \param value[out] A pointer to variable where the result will be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_read_double_be(cardano_buffer_t* buffer, double* value);

#endif // CARDANO_buffer_H