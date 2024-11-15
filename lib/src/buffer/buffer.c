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
#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <math.h>
#include <sodium.h>
#include <string.h>

#include "../config.h"

/* STRUCTS *******************************************************************/

/**
 * \struct cardano_buffer_t
 * \brief Represents a dynamically sized buffer for storing binary data.
 *
 * This structure is designed to manage a variable-sized sequence of bytes, providing mechanisms
 * for dynamically resizing the buffer as necessary. It is built on top of \c cardano_object_t,
 * inheriting reference counting and basic object management functionalities.
 */
typedef struct cardano_buffer_t
{
    cardano_object_t base;
    byte_t*          data;
    size_t           size;
    size_t           head;
    size_t           capacity;
} cardano_buffer_t;

/* STATIC FUNCTIONS ***********************************************************/

/**
 * \brief Grows the buffer if there is not enough capacity to hold the new data.
 *
 * \param[in] buffer The buffer to grow.
 * \param[in] size_of_new_data The size of new data to be written into the buffer.
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
    size_t  new_capacity = (size_t)ceil((float)((float)buffer->size + (float)size_of_new_data) * (float)LIB_CARDANO_C_COLLECTION_GROW_FACTOR);
    byte_t* new_data     = (byte_t*)_cardano_realloc(buffer->data, new_capacity);

    if (new_data == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
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
 * \param[in] object A void pointer to the buffer object to be deallocated. The function casts this
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
    _cardano_free(buffer->data);
    buffer->data = NULL;
  }

  _cardano_free(buffer);
}

/* DEFINITIONS ****************************************************************/

cardano_buffer_t*
cardano_buffer_new(const size_t capacity)
{
  cardano_buffer_t* buffer = (cardano_buffer_t*)_cardano_malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data = (byte_t*)_cardano_malloc(capacity);

  if (buffer->data == NULL)
  {
    _cardano_free(buffer);
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

  cardano_buffer_t* buffer = (cardano_buffer_t*)_cardano_malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data = (byte_t*)_cardano_malloc(size);

  if (buffer->data == NULL)
  {
    _cardano_free(buffer);
    return NULL;
  }

  cardano_safe_memcpy(buffer->data, size, array, size);

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

  cardano_buffer_t* buffer = (cardano_buffer_t*)_cardano_malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data = (byte_t*)_cardano_malloc(lhs->size + rhs->size);

  if (buffer->data == NULL)
  {
    _cardano_free(buffer);
    return NULL;
  }

  cardano_safe_memcpy(buffer->data, lhs->size + rhs->size, lhs->data, lhs->size);
  cardano_safe_memcpy(&buffer->data[lhs->size], rhs->size, rhs->data, rhs->size);

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
    return cardano_buffer_new(1U);
  }

  byte_t* slice_data = (byte_t*)_cardano_malloc(slice_size);

  if (slice_data == NULL)
  {
    return NULL;
  }

  assert(buffer->data != NULL);

  cardano_safe_memcpy(slice_data, slice_size, &buffer->data[start], slice_size);

  cardano_buffer_t* sliced_buffer = (cardano_buffer_t*)_cardano_malloc(sizeof(cardano_buffer_t));

  if (sliced_buffer == NULL)
  {
    _cardano_free(slice_data);
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

  cardano_buffer_t* buffer = (cardano_buffer_t*)_cardano_malloc(sizeof(cardano_buffer_t));

  if (buffer == NULL)
  {
    return NULL;
  }

  buffer->data               = (byte_t*)_cardano_malloc(size / 2U);
  buffer->size               = size / 2U;
  buffer->head               = 0;
  buffer->capacity           = buffer->size;
  buffer->base.ref_count     = 1;
  buffer->base.last_error[0] = '\0';
  buffer->base.deallocator   = cardano_buffer_deallocate;

  if (buffer->data == NULL)
  {
    _cardano_free(buffer);
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

size_t
cardano_buffer_get_hex_size(const cardano_buffer_t* buffer)
{
  static const size_t null_termination_size = 1;
  static const size_t byte_size_in_hex      = 2;

  if (buffer == NULL)
  {
    return 0;
  }

  return (buffer->size * byte_size_in_hex) + null_termination_size;
}

cardano_error_t
cardano_buffer_to_hex(const cardano_buffer_t* buffer, char* dest, const size_t dest_size)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (dest == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t hex_string_size = cardano_buffer_get_hex_size(buffer);

  if (dest_size < hex_string_size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  int init_result = sodium_init();

  if (init_result == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  assert(buffer->data != NULL);

  if (sodium_bin2hex(dest, dest_size, buffer->data, buffer->size) < 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_buffer_get_str_size(const cardano_buffer_t* buffer)
{
  static const size_t null_termination_size = 1;

  if (buffer == NULL)
  {
    return 0;
  }

  return buffer->size + null_termination_size;
}

cardano_error_t
cardano_buffer_to_str(const cardano_buffer_t* buffer, char* dest, const size_t dest_size)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (dest == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t string_size = cardano_buffer_get_str_size(buffer);

  if (dest_size < string_size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  assert(buffer->data != NULL);

  cardano_safe_memcpy(dest, dest_size, buffer->data, buffer->size);

  dest[buffer->size] = '\0';

  return CARDANO_SUCCESS;
}

bool
cardano_buffer_equals(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (lhs->size != rhs->size)
  {
    return false;
  }

  if (lhs->size == 0U)
  {
    return true;
  }

  assert(lhs->data != NULL);
  assert(rhs->data != NULL);

  return (memcmp(lhs->data, rhs->data, lhs->size) == 0);
}

int32_t
cardano_buffer_compare(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs)
{
  if (lhs == rhs)
  {
    return 0;
  }

  if (lhs == NULL)
  {
    return -1;
  }

  if (rhs == NULL)
  {
    return 1;
  }

  if (lhs->size < rhs->size)
  {
    return -1;
  }

  if (lhs->size > rhs->size)
  {
    return 1;
  }

  if (lhs->size == 0U)
  {
    return 0;
  }

  assert(lhs->data != NULL);
  assert(rhs->data != NULL);

  return memcmp(lhs->data, rhs->data, lhs->size);
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

byte_t*
cardano_buffer_get_data(const cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return NULL;
  }

  return buffer->data;
}

void
cardano_buffer_memzero(cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return;
  }

  if (buffer->data == NULL)
  {
    return;
  }

  sodium_memzero(buffer->data, buffer->size);
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

cardano_error_t
cardano_buffer_copy_bytes(const cardano_buffer_t* buffer, byte_t* dest, const size_t dest_size)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (dest == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (dest_size == 0U)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE;
  }

  size_t data_length = cardano_buffer_get_size(buffer);

  if (data_length > dest_size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE;
  }

  cardano_safe_memcpy(dest, dest_size, cardano_buffer_get_data(buffer), data_length);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_set_size(cardano_buffer_t* buffer, const size_t size)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size > buffer->capacity)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE;
  }

  buffer->size = size;

  return CARDANO_SUCCESS;
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
cardano_buffer_seek(cardano_buffer_t* buffer, size_t position)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (position > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  buffer->head = position;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write(cardano_buffer_t* buffer, const byte_t* data, const size_t size)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, size);

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  cardano_safe_memcpy(&buffer->data[buffer->size], buffer->capacity - buffer->size, data, size);

  buffer->size += size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read(cardano_buffer_t* buffer, byte_t* data, const size_t bytes_to_read)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((buffer->head + bytes_to_read) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_safe_memcpy(data, bytes_to_read, &buffer->data[buffer->head], bytes_to_read);

  buffer->head += bytes_to_read;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint16_le(cardano_buffer_t* buffer, const uint16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint16_le(value, buffer->data, sizeof(value), buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint32_le(cardano_buffer_t* buffer, const uint32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint32_le(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint64_le(cardano_buffer_t* buffer, const uint64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint64_le(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int16_le(cardano_buffer_t* buffer, const int16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int16_le(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int32_le(cardano_buffer_t* buffer, const int32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int32_le(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int64_le(cardano_buffer_t* buffer, const int64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int64_le(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_float_le(cardano_buffer_t* buffer, const float value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_float_le(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_double_le(cardano_buffer_t* buffer, const double value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_double_le(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint16_be(cardano_buffer_t* buffer, const uint16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint16_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint32_be(cardano_buffer_t* buffer, const uint32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint32_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_uint64_be(cardano_buffer_t* buffer, const uint64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_uint64_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int16_be(cardano_buffer_t* buffer, const int16_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int16_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int32_be(cardano_buffer_t* buffer, const int32_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int32_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_int64_be(cardano_buffer_t* buffer, const int64_t value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_int64_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_float_be(cardano_buffer_t* buffer, const float value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_float_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_write_double_be(cardano_buffer_t* buffer, const double value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t          type_size   = sizeof(value);
  const cardano_error_t grow_result = grow_buffer_if_needed(buffer, sizeof(value));

  if (grow_result != CARDANO_SUCCESS)
  {
    return grow_result;
  }

  const cardano_error_t write_result = cardano_write_double_be(value, buffer->data, buffer->capacity, buffer->size);

  CARDANO_UNUSED(write_result);
  assert(write_result == CARDANO_SUCCESS);

  buffer->size += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint16_le(cardano_buffer_t* buffer, uint16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint16_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_uint16_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint32_le(cardano_buffer_t* buffer, uint32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint32_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_uint32_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint64_le(cardano_buffer_t* buffer, uint64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint64_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_uint64_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int16_le(cardano_buffer_t* buffer, int16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int16_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_int16_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int32_le(cardano_buffer_t* buffer, int32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int32_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_int32_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int64_le(cardano_buffer_t* buffer, int64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int64_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_int64_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_float_le(cardano_buffer_t* buffer, float* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(float);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_float_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_double_le(cardano_buffer_t* buffer, double* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(double);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_double_le(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint16_be(cardano_buffer_t* buffer, uint16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint16_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_uint16_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint32_be(cardano_buffer_t* buffer, uint32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint32_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_uint32_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_uint64_be(cardano_buffer_t* buffer, uint64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(uint64_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_uint64_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int16_be(cardano_buffer_t* buffer, int16_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int16_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_int16_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int32_be(cardano_buffer_t* buffer, int32_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int32_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_int32_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_int64_be(cardano_buffer_t* buffer, int64_t* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(int64_t);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_int64_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_float_be(cardano_buffer_t* buffer, float* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(float);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_float_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

  buffer->head += type_size;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_buffer_read_double_be(cardano_buffer_t* buffer, double* value)
{
  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t type_size = sizeof(double);

  if ((buffer->head + type_size) > buffer->size)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_error_t result = cardano_read_double_be(value, buffer->data, buffer->size, buffer->head);

  CARDANO_UNUSED(result);
  assert(result == CARDANO_SUCCESS);

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
