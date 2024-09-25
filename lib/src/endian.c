/**
 * \file endian.c
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

/* INCLUDES ******************************************************************/

#include "endian.h"
#include "string_safe.h"

/* CONSTANTS *****************************************************************/

static const byte_t SIZE_IN_BYTES_16_BITS = 2;
static const byte_t SIZE_IN_BYTES_32_BITS = 4;
static const byte_t SIZE_IN_BYTES_64_BITS = 8;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Copies count bytes from the memory pointed to by src to the memory pointed to by dest in reverse order.
 * Both objects are reinterpreted as arrays of unsigned char.
 *
 * \param dest Pointer to the destination memory.
 * \param dest_size The max capacity of the destination memory.
 * \param src Pointer to the source memory.
 * \param size The number of bytes to copy.
 */
static void
reverse_memcpy(byte_t* dest, const size_t dest_size, const byte_t* src, const size_t size)
{
  const size_t max_size = (size > dest_size) ? dest_size : size;

  for (size_t i = 0; i < max_size; ++i)
  {
    dest[i] = src[max_size - i - (size_t)1];
  }
}

/**
 * Writes the source buffer into the destination buffer adjusting for endianess.
 *
 * \param[in] src The source buffer.
 * \param[in] src_size The size of the source buffer.
 * \param[out] dest The destination buffer.
 * \param[in] dest_size The size of the destination buffer.
 * \param[in] offset The number of bytes to skip before starting to write.
 * \param[in] is_native_endian Whether the current system endianess matches the desired endianess.
 *
 * \return \c cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \c cardano_error_t otherwise.
 */
static cardano_error_t
write_bytes(
  const byte_t* src,
  const size_t  src_size,
  byte_t*       dest,
  const size_t  dest_size,
  const size_t  offset,
  const bool    is_native_endian)
{
  if ((dest_size - offset) < src_size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if (is_native_endian)
  {
    cardano_safe_memcpy(&dest[offset], dest_size - offset, src, src_size);
  }
  else
  {
    reverse_memcpy(&dest[offset], dest_size - offset, src, src_size);
  }

  return CARDANO_SUCCESS;
}

/**
 * Read the source buffer into the destination buffer adjusting for endianess.
 *
 * \param[in] src The source buffer.
 * \param[in] src_size The size of the source buffer.
 * \param[out] dest The destination buffer.
 * \param[in] dest_size The size of the destination buffer.
 * \param[in] offset The number of bytes to skip before starting to read.
 * \param[in] is_native_endian Whether the current system endianess matches the desired endianess.
 *
 * \return \c cardano_error_t Returns \c CARDANO_SUCCESS on success, member of \c cardano_error_t otherwise.
 */
static cardano_error_t
read_bytes(
  const byte_t* src,
  const size_t  src_size,
  byte_t*       dest,
  const size_t  dest_size,
  const size_t  offset,
  const bool    is_native_endian)
{
  if ((src_size - offset) < dest_size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if (is_native_endian)
  {
    cardano_safe_memcpy(dest, dest_size, &src[offset], src_size - offset);
  }
  else
  {
    reverse_memcpy(dest, dest_size, &src[offset], src_size - offset);
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ***************************************************************/

bool
cardano_is_little_endian(void)
{
  int16_t  number  = 0x1;
  uint8_t* num_ptr = (uint8_t*)&number;

  return num_ptr[0] == (uint8_t)1;
}

bool
cardano_is_big_endian(void)
{
  return !cardano_is_little_endian();
}

cardano_error_t
cardano_write_uint16_le(const uint16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_uint32_le(const uint32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_uint64_le(const uint64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_int16_le(const int16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_int32_le(const int32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_int64_le(const int64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_float_le(const float value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_double_le(const double value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_uint16_be(const uint16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_uint32_be(const uint32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_uint64_be(const uint64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_int16_be(const int16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_int32_be(const int32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_int64_be(const int64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_float_be(const float value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_double_be(const double value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write_bytes(
    (const byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_uint16_le(uint16_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_16_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_uint32_le(uint32_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_32_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_uint64_le(uint64_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_int16_le(int16_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_16_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_int32_le(int32_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_32_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_int64_le(int64_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_float_le(float* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    sizeof(float),
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_double_le(double* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    sizeof(double),
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_uint16_be(uint16_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_16_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_uint32_be(uint32_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_32_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_uint64_be(uint64_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_int16_be(int16_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_16_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_int32_be(int32_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_32_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_int64_be(int64_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_float_be(float* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    sizeof(float),
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_double_be(double* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read_bytes(
    buffer,
    size,
    (byte_t*)value,
    sizeof(double),
    offset,
    cardano_is_big_endian());
}
