/**
 * \file endian.c
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
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include <cardano/endian.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

const byte_t SIZE_IN_BYTES_16_BITS = 2;
const byte_t SIZE_IN_BYTES_32_BITS = 4;
const byte_t SIZE_IN_BYTES_64_BITS = 8;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Copies count bytes from the memory pointed to by src to the memory pointed to by dest in reverse order.
 * Both objects are reinterpreted as arrays of unsigned char.
 *
 * \param dest Pointer to the destination memory.
 * \param src Pointer to the source memory.
 * \param size The number of bytes to copy.
 */
static void
reverse_memcpy(byte_t* dest, const byte_t* src, const size_t size)
{
  for (size_t i = 0; i < size; ++i)
  {
    dest[i] = src[size - i - (size_t)1];
  }
}

/**
 * Writes the source buffer into the destination buffer adjusting for endianess.
 *
 * \param src[in] The source buffer.
 * \param srcSize[in] The size of the source buffer.
 * \param dest[out] The destination buffer.
 * \param destSize[in] The size of the destination buffer.
 * \param offset[in] The number of bytes to skip before starting to write.
 * \param isNativeEndian[in] Whether the current system endianess matches the desired endianess.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
static cardano_error_t
write(
  const byte_t* src,
  const size_t  src_size,
  byte_t*       dest,
  const size_t  dest_size,
  const size_t  offset,
  const bool    is_native_endian)
{
  if ((dest_size - offset) < src_size)
  {
    return CARDANO_INSUFFICIENT_BUFFER_SIZE;
  }

  if (is_native_endian)
  {
    memcpy((void*)(dest + offset), src, src_size);
  }
  else
  {
    reverse_memcpy((byte_t*)(dest + offset), (byte_t*)src, src_size);
  }

  return CARDANO_SUCCESS;
}

/**
 * Read the source buffer into the destination buffer adjusting for endianess.
 *
 * \param src[in] The source buffer.
 * \param srcSize[in] The size of the source buffer.
 * \param dest[out] The destination buffer.
 * \param destSize[in] The size of the destination buffer.
 * \param offset[in] The number of bytes to skip before starting to read.
 * \param isNativeEndian[in] Whether the current system endianess matches the desired endianess.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
static cardano_error_t
read(
  const byte_t* src,
  const size_t  src_size,
  byte_t*       dest,
  const size_t  dest_size,
  const size_t  offset,
  const bool    is_native_endian)
{
  if ((src_size - offset) < dest_size)
  {
    return CARDANO_INSUFFICIENT_BUFFER_SIZE;
  }

  if (is_native_endian)
  {
    memcpy((void*)(dest), src + offset, dest_size);
  }
  else
  {
    reverse_memcpy((byte_t*)dest, (byte_t*)src + offset, dest_size);
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ***************************************************************/

bool
cardano_is_little_endian()
{
  short int number  = 0x1;
  char*     num_ptr = (char*)&number;

  return num_ptr[0] == 1;
}

bool
cardano_is_big_endian()
{
  return !cardano_is_little_endian();
}

cardano_error_t
cardano_write_uint16_le(const uint16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_uint32_le(const uint32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_uint64_le(const uint64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_int16_le(const int16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_int32_le(const int32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_int64_le(const int64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_float32_le(const float32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_float64_le(const float64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_write_uint16_be(const uint16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_uint32_be(const uint32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_uint64_be(const uint64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_int16_be(const int16_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_int32_be(const int32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_int64_be(const int64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_float32_be(const float32_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_write_float64_be(const float64_t value, byte_t* buffer, const size_t size, const size_t offset)
{
  return write(
    (byte_t*)&value,
    sizeof(value),
    buffer,
    size,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_uint16_le(uint16_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read(
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
  return read(
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
  return read(
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
  return read(
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
  return read(
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
  return read(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_float32_le(float32_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_32_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_float64_le(float64_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_little_endian());
}

cardano_error_t
cardano_read_uint16_be(uint16_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read(
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
  return read(
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
  return read(
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
  return read(
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
  return read(
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
  return read(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_float32_be(float32_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_32_BITS,
    offset,
    cardano_is_big_endian());
}

cardano_error_t
cardano_read_float64_be(float64_t* value, const byte_t* buffer, const size_t size, const size_t offset)
{
  return read(
    buffer,
    size,
    (byte_t*)value,
    SIZE_IN_BYTES_64_BITS,
    offset,
    cardano_is_big_endian());
}
