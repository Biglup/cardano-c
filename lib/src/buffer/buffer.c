/**
 * \file buffer.c
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

/* INCLUDES ******************************************************************/

#include "buffer.h"
#include "../endian.h"

#include <stdlib.h>
#include <string.h>

/* STRUCTS *******************************************************************/

typedef struct cardano_buffer_t
{
    byte_t* data;
    size_t  size;
    size_t  head;
    size_t  capacity;
} cardano_buffer_t;

/* STATIC FUNCTIONS ***********************************************************/

/**
 * \brief Grows the buffer if there is not enough capacity to hold the new data.
 *
 * \param buffer The buffer to grow.
 * \param size The size of the buffer.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
static cardano_error_t
grow_buffer_if_needed(cardano_buffer_t* buffer, const size_t size_of_new_data)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if ((buffer->size + size_of_new_data) >= buffer->capacity)
  {
    size_t  new_capacity = buffer->capacity * 2U;
    byte_t* new_data     = (byte_t*)realloc(buffer->data, new_capacity);

    if (new_data == NULL)
    {
      return CARDANO_MEMORY_ALLOCATION_FAILED;
    }

    buffer->data     = new_data;
    buffer->capacity = new_capacity;
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ****************************************************************/

cardano_buffer_t*
cardano_buffer_new(const size_t capacity)
{
  cardano_buffer_t* buffer = (cardano_buffer_t*)malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data = (byte_t*)malloc(capacity);

  if (buffer->data == NULL)
  {
    free(buffer);
    return NULL;
  }

  buffer->size     = 0;
  buffer->head     = 0;
  buffer->capacity = capacity;

  return buffer;
}

cardano_buffer_t*
cardano_buffer_concat(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs)
{
  if (lhs == NULL)
  {
    return NULL;
  }

  if (rhs == NULL)
  {
    return NULL;
  }

  cardano_buffer_t* buffer = (cardano_buffer_t*)malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data = (byte_t*)malloc(lhs->size + rhs->size);

  if (buffer->data == NULL)
  {
    free(buffer);
    return NULL;
  }

  (void)memcpy(buffer->data, lhs->data, lhs->size);
  (void)memcpy(&buffer->data[lhs->size], rhs->data, rhs->size);

  buffer->size     = lhs->size + rhs->size;
  buffer->head     = 0;
  buffer->capacity = buffer->size;

  return buffer;
}

void
cardano_buffer_free(cardano_buffer_t** buffer)
{
  if (buffer == NULL)
  {
    return;
  }

  byte_t* data = (*buffer)->data;
  free(*buffer);
  *buffer = NULL;

  if (data == NULL)
  {
    return;
  }

  free(data);
}

byte_t*
cardano_buffer_get_data(const cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return NULL;
  }

  return buffer->data;
}

size_t
cardano_buffer_get_size(const cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return 0;
  }

  return buffer->size;
}

size_t
cardano_buffer_get_capacity(const cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return 0;
  }

  return buffer->capacity;
}

cardano_error_t
cardano_buffer_write(cardano_buffer_t* buffer, byte_t* data, const size_t size)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, size);

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  (void)memcpy(&buffer->data[buffer->size], data, size);

  buffer->size += size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read(cardano_buffer_t* buffer, byte_t* data, const size_t bytes_to_read)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if ((buffer->head + bytes_to_read) >= buffer->size)
  {
    return CARDANO_OUT_OF_BOUNDS_MEMORY_READ;
  }

  (void)memcpy(data, &buffer->data[buffer->head], bytes_to_read);

  buffer->head += bytes_to_read;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint16_le(cardano_buffer_t* buffer, const uint16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint16_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint32_le(cardano_buffer_t* buffer, const uint32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint32_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint64_le(cardano_buffer_t* buffer, const uint64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint64_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int16_le(cardano_buffer_t* buffer, const int16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int16_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int32_le(cardano_buffer_t* buffer, const int32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int32_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int64_le(cardano_buffer_t* buffer, const int64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int64_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_float_le(cardano_buffer_t* buffer, const float value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_float_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_double_le(cardano_buffer_t* buffer, const double value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_double_le(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint16_be(cardano_buffer_t* buffer, const uint16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint16_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint32_be(cardano_buffer_t* buffer, const uint32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint32_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint64_be(cardano_buffer_t* buffer, const uint64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint64_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int16_be(cardano_buffer_t* buffer, const int16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int16_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int32_be(cardano_buffer_t* buffer, const int32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int32_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int64_be(cardano_buffer_t* buffer, const int64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int64_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_float_be(cardano_buffer_t* buffer, const float value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_float_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_double_be(cardano_buffer_t* buffer, const double value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_double_be(value, buffer->data, sizeof(value), buffer->size);

  if (write_result != CARDANO_SUCCESS)
  {
    return write_result;
  }

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint16_le(cardano_buffer_t* buffer, uint16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint16_t);

  cardano_error_t result = cardano_read_uint16_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint32_le(cardano_buffer_t* buffer, uint32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint32_t);

  cardano_error_t result = cardano_read_uint32_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint64_le(cardano_buffer_t* buffer, uint64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint64_t);

  cardano_error_t result = cardano_read_uint64_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int16_le(cardano_buffer_t* buffer, int16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int16_t);

  cardano_error_t result = cardano_read_int16_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int32_le(cardano_buffer_t* buffer, int32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int32_t);

  cardano_error_t result = cardano_read_int32_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int64_le(cardano_buffer_t* buffer, int64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int64_t);

  cardano_error_t result = cardano_read_int64_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_float_le(cardano_buffer_t* buffer, float* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(float);

  cardano_error_t result = cardano_read_float_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_double_le(cardano_buffer_t* buffer, double* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(double);

  cardano_error_t result = cardano_read_double_le(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint16_be(cardano_buffer_t* buffer, uint16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint16_t);

  cardano_error_t result = cardano_read_uint16_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint32_be(cardano_buffer_t* buffer, uint32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint32_t);

  cardano_error_t result = cardano_read_uint32_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint64_be(cardano_buffer_t* buffer, uint64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint64_t);

  cardano_error_t result = cardano_read_uint64_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int16_be(cardano_buffer_t* buffer, int16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int16_t);

  cardano_error_t result = cardano_read_int16_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int32_be(cardano_buffer_t* buffer, int32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int32_t);

  cardano_error_t result = cardano_read_int32_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int64_be(cardano_buffer_t* buffer, int64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int64_t);

  cardano_error_t result = cardano_read_int64_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_float_be(cardano_buffer_t* buffer, float* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(float);

  cardano_error_t result = cardano_read_float_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_double_be(cardano_buffer_t* buffer, double* value)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(double);

  cardano_error_t result = cardano_read_double_be(value, buffer->data, type_size, buffer->head);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}
