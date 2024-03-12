/**
 * \file buffer.c
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

/* INCLUDES ******************************************************************/

#include "../endian.h"

#include <cardano/buffer.h>
#include <cardano/object.h>

#include <assert.h>
#include <math.h>
#include <sodium.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"

/* STRUCTS *******************************************************************/

/**
 * Represents a buffer object.
 */
typedef struct cardano_buffer_t
{
    cardano_object_t base;
    byte_t*          data;
    size_t           size;
    size_t           head;
    size_t           capacity;
    size_t           ref_count;
} cardano_buffer_t;

/* STATIC FUNCTIONS ***********************************************************/

/**
 * \brief Grows the buffer if there is not enough capacity to hold the new data.
 *
 * \param buffer The buffer to grow.
 * \param size_of_new_data The size of new data to be written into the buffer.
 *
 * \return A \c cardano_error_t indicating the result of the operation: \c CARDANO_SUCCESS on success,
 *         or an appropriate error code indicating the failure reason. Refer to \c cardano_error_t documentation
 *         for details on possible error codes.
 */
static cardano_error_t
grow_buffer_if_needed(cardano_buffer_t* buffer, const size_t size_of_new_data)
{
  assert(buffer != NULL);

  if ((buffer->size + size_of_new_data) >= buffer->capacity)
  {
    size_t  new_capacity = (size_t)ceil((float)buffer->capacity * (float)LIB_CARDANO_C_COLLECTION_GROW_FACTOR);
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

/**
 * \brief Deallocates a buffer object.
 *
 * This function is responsible for properly deallocating a buffer object (`cardano_buffer_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the buffer object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_buffer_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the buffer
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_buffer_deallocate(void* object)
{
  assert(object != NULL);

  cardano_buffer_t* buffer = (cardano_buffer_t*)object;

  if (buffer->data != NULL)
  {
    free(buffer->data);
    buffer->data = NULL;
  }

  free(buffer);
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

  buffer->size               = 0;
  buffer->head               = 0;
  buffer->capacity           = capacity;
  buffer->base.ref_count     = 1;
  buffer->base.last_error[0] = '\0';
  buffer->base.deallocator   = cardano_buffer_deallocate;

  return buffer;
}

cardano_buffer_t*
cardano_buffer_new_from(const byte_t* array, const size_t size)
{
  if (array == NULL)
  {
    return NULL;
  }

  cardano_buffer_t* buffer = (cardano_buffer_t*)malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data = (byte_t*)malloc(size);

  if (buffer->data == NULL)
  {
    free(buffer);
    return NULL;
  }

  (void)memcpy(buffer->data, array, size);

  buffer->size               = size;
  buffer->head               = 0;
  buffer->capacity           = buffer->size;
  buffer->base.ref_count     = 1;
  buffer->base.last_error[0] = '\0';
  buffer->base.deallocator   = cardano_buffer_deallocate;

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

  buffer->size               = lhs->size + rhs->size;
  buffer->head               = 0;
  buffer->capacity           = buffer->size;
  buffer->base.ref_count     = 1;
  buffer->base.last_error[0] = '\0';
  buffer->base.deallocator   = cardano_buffer_deallocate;

  return buffer;
}

cardano_buffer_t*
cardano_buffer_slice(const cardano_buffer_t* buffer, size_t start, size_t end)
{
  if (buffer == NULL)
  {
    return NULL;
  }

  if (buffer->data == NULL)
  {
    return NULL;
  }

  if (start > buffer->size)
  {
    return NULL;
  }

  if (end > buffer->size)
  {
    return NULL;
  }

  if (end < start)
  {
    return NULL;
  }

  size_t slice_size = end - start;

  if (slice_size == 0U)
  {
    return NULL;
  }

  byte_t* slice_data = (byte_t*)malloc(slice_size);

  if (slice_data == NULL)
  {
    return NULL;
  }

  (void)memcpy(slice_data, &buffer->data[start], slice_size);

  cardano_buffer_t* sliced_buffer = (cardano_buffer_t*)malloc(sizeof(cardano_buffer_t));

  if (sliced_buffer == NULL)
  {
    free(slice_data);
    return NULL;
  }

  sliced_buffer->data               = slice_data;
  sliced_buffer->size               = slice_size;
  sliced_buffer->head               = 0;
  sliced_buffer->capacity           = sliced_buffer->size;
  sliced_buffer->base.ref_count     = 1;
  sliced_buffer->base.last_error[0] = '\0';
  sliced_buffer->base.deallocator   = cardano_buffer_deallocate;

  return sliced_buffer;
}

cardano_buffer_t*
cardano_buffer_from_hex(const char* hex_string, const size_t size)
{
  if (hex_string == NULL)
  {
    return NULL;
  }

  if ((size % 2U) != 0U)
  {
    return NULL;
  }

  cardano_buffer_t* buffer = (cardano_buffer_t*)malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data               = (byte_t*)malloc(size / 2U);
  buffer->ref_count          = 1;
  buffer->size               = size / 2U;
  buffer->head               = 0;
  buffer->capacity           = buffer->size;
  buffer->base.ref_count     = 1;
  buffer->base.last_error[0] = '\0';
  buffer->base.deallocator   = cardano_buffer_deallocate;

  if (buffer->data == NULL)
  {
    free(buffer);
    return NULL;
  }

  const char* end = NULL;

  int init_result = sodium_init();

  if (init_result == -1)
  {
    cardano_buffer_unref(&buffer);
    return NULL;
  }

  int decode_result = sodium_hex2bin(buffer->data, size, hex_string, size, NULL, NULL, &end);

  if (decode_result != 0)
  {
    cardano_buffer_unref(&buffer);
    return NULL;
  }

  return buffer;
}

char*
cardano_buffer_to_hex(const cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return NULL;
  }

  if (buffer->data == NULL)
  {
    return NULL;
  }

  int init_result = sodium_init();

  if (init_result == -1)
  {
    return NULL;
  }

  static const size_t null_termination_size = 1;
  static const size_t byte_size             = 1;
  static const size_t byte_size_in_hex      = 2;

  size_t hex_string_size = (buffer->size * byte_size_in_hex) + null_termination_size;
  char*  hex_string      = (char*)calloc(hex_string_size, byte_size);

  // TODO: Make this function take a pointer to preallocated memory and write to it.
  // NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
  return sodium_bin2hex(hex_string, hex_string_size, buffer->data, buffer->size);
}

void
cardano_buffer_unref(cardano_buffer_t** buffer)
{
  if ((buffer == NULL) || (*buffer == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*buffer)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *buffer = NULL;
    return;
  }
}

void
cardano_buffer_ref(cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return;
  }

  cardano_object_ref(&buffer->base);
}

size_t
cardano_buffer_refcount(const cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&buffer->base);
}

cardano_buffer_t*
cardano_buffer_move(cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return NULL;
  }

  cardano_object_t* object = cardano_object_move(&buffer->base);
  (void)object;

  return buffer;
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
cardano_buffer_write(cardano_buffer_t* buffer, const byte_t* data, const size_t size)
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

  const cardano_error_t write_result = cardano_write_uint32_le(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_uint64_le(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_int16_le(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_int32_le(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_int64_le(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_float_le(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_double_le(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_uint16_be(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_uint32_be(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_uint64_be(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_int16_be(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_int32_be(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_int64_be(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_float_be(value, buffer->data, buffer->capacity, buffer->size);

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

  const cardano_error_t write_result = cardano_write_double_be(value, buffer->data, buffer->capacity, buffer->size);

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

void
cardano_buffer_set_last_error(cardano_buffer_t* buffer, const char* message)
{
  cardano_object_set_last_error(&buffer->base, message);
}

const char*
cardano_buffer_get_last_error(const cardano_buffer_t* buffer)
{
  return cardano_object_get_last_error(&buffer->base);
}
