/**
 * \file endian.h
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ENDIAN_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ENDIAN_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Gets whether the scalar types are little-endian. That is, the least significant byte
 * is stored in the smallest address. For example, 0x1234 is stored 0x34 0x12.
 *
 * \return \c true if the system is little endian; otherwise; \c false.
 */
bool cardano_is_little_endian(void);

/**
 * \brief Gets whether that scalar types are big-endian, that is, the most significant byte is stored
 * in the smallest address. For example, 0x1234 is stored 0x12 0x34.
 *
 * \return \c true if the system is big endian; otherwise; \c false.
 */
bool cardano_is_big_endian(void);

/**
 * \brief Writes an uint16_t value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_uint16_le(uint16_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an uint32_t value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_uint32_le(uint32_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an uint64_t value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_uint64_le(uint64_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an int16_t value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_int16_le(int16_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an int32_t value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_int32_le(int32_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an int64_t value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_int64_le(int64_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an float value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_float_le(float value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an double value as little-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_double_le(double value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an uint16_t value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_uint16_be(uint16_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an uint32_t value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_uint32_be(uint32_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an uint64_t value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_uint64_be(uint64_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an int16_t value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_int16_be(int16_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an int32_t value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_int32_be(int32_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an int64_t value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_int64_be(int64_t value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an float value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_float_be(float value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Writes an double value as big-endian into the given buffer.
 *
 * \param[in] value The value to be written into the buffer.
 * \param[out] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to write.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_write_double_be(double value, byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a uint16_t value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_uint16_le(uint16_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a uint32_t value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_uint32_le(uint32_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a uint64_t value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_uint64_le(uint64_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a int16_t value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_int16_le(int16_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a int32_t value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_int32_le(int32_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a int64_t value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_int64_le(int64_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a float value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_float_le(float* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a double value as little-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_double_le(double* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a uint16_t value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_uint16_be(uint16_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a uint32_t value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_uint32_be(uint32_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a uint64_t value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_uint64_be(uint64_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a int16_t value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_int16_be(int16_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a int32_t value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_int32_be(int32_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a int64_t value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_int64_be(int64_t* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a float value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_float_be(float* value, const byte_t* buffer, size_t size, size_t offset);

/**
 * \brief Reads a double value as big-endian from the given buffer.
 *
 * \param[out] value A pointer to variable where the result will be written.
 * \param[in] buffer The buffer where the value will be written to.
 * \param[in] size The sizeof the buffer in bytes.
 * \param[in] offset The number of bytes to skip before starting to read.
 *
 * \return \ref cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \ref cardano_error_t otherwise.
 */
cardano_error_t cardano_read_double_be(double* value, const byte_t* buffer, size_t size, size_t offset);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ENDIAN_H