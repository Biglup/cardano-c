/**
 * \file endian.h
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CARDANO_ENDIAN_H
#define CARDANO_ENDIAN_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /**
   * \brief Gets whether the scalar types are little-endian. That is, the least significant byte
   * is stored in the smallest address. For example, 0x1234 is stored 0x34 0x12.
   *
   * \return <tt>true</tt> if the system is little endian; otherwise; <tt>false</tt>.
   */
  bool cardano_is_little_endian();

  /**
   * \brief Gets whether that scalar types are big-endian, that is, the most significant byte is stored
   * in the smallest address. For example, 0x1234 is stored 0x12 0x34.
   *
   * \return <tt>true</tt> if the system is big endian; otherwise; <tt>false</tt>.
   */
  bool cardano_is_big_endian();

  /**
   * \brief Writes an uint16_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_uint16_le(uint16_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an uint32_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_uint32_le(uint32_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an uint64_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_uint64_le(uint64_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an int16_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_int16_le(int16_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an int32_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_int32_le(int32_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an int64_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_int64_le(int64_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an float32_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_float32_le(float32_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an float64_t value as little-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_float64_le(float64_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an uint16_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_uint16_be(uint16_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an uint32_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_uint32_be(uint32_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an uint64_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_uint64_be(uint64_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an int16_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_int16_be(int16_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an int32_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_int32_be(int32_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an int64_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_int64_be(int64_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an float32_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_float32_be(float32_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Writes an float64_t value as big-endian into the given buffer.
   *
   * \param value[in] The value to be written into the buffer.
   * \param buffer[out] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to write.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_write_float64_be(float64_t value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a uint16_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_uint16_le(uint16_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a uint32_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_uint32_le(uint32_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a uint64_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_uint64_le(uint64_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a int16_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_int16_le(int16_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a int32_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_int32_le(int32_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a int64_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_int64_le(int64_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a float32_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_float32_le(float32_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a float64_t value as little-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_float64_le(float64_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a uint16_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_uint16_be(uint16_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a uint32_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_uint32_be(uint32_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a uint64_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_uint64_be(uint64_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a int16_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_int16_be(int16_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a int32_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_int32_be(int32_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a int64_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_int64_be(int64_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a float32_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_float32_be(float32_t* value, byte_t* buffer, size_t size, size_t offset);

  /**
   * \brief Reads a float64_t value as big-endian from the given buffer.
   *
   * \param value[out] A pointer to variable where the result will be written.
   * \param buffer[in] The buffer where the value will be written to.
   * \param size[in] The sizeof the buffer in bytes.
   * \param offset[in] The number of bytes to skip before starting to read.
   *
   * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
   */
  cardano_error_t cardano_read_float64_be(float64_t* value, byte_t* buffer, size_t size, size_t offset);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_ENDIAN_H