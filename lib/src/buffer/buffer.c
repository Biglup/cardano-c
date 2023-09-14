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

#include <stdlib.h>
#include <string.h>

/* STRUCTS *******************************************************************/

typedef struct cardano_buffer_t
{
    byte_t* data;
    size_t  size;
    size_t  capacity;
} cardano_buffer_t;

/* DEFINITIONS ****************************************************************/

cardano_buffer_t*
cardano_buffer_new(size_t capacity)
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
  buffer->capacity = capacity;

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

cardano_error_t
cardano_buffer_push(cardano_buffer_t* buffer, byte_t* data, size_t size)
{
  if (buffer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (buffer->size + size >= buffer->capacity)
  {
    size_t  new_capacity = buffer->capacity * 2;
    byte_t* new_data     = (byte_t*)realloc(buffer->data, new_capacity);

    if (new_data == NULL)
    {
      return CARDANO_MEMORY_ALLOCATION_FAILED;
    }

    buffer->data     = new_data;
    buffer->capacity = new_capacity;
  }

  (void)memcpy(&buffer->data[buffer->size], data, size);

  buffer->size += size;

  return CARDANO_SUCCESS;
}

byte_t*
cardano_buffer_get_data(cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return NULL;
  }

  return buffer->data;
}

size_t
cardano_buffer_get_size(cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return 0;
  }

  return buffer->size;
}

size_t
cardano_buffer_get_capacity(cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return 0;
  }

  return buffer->capacity;
}