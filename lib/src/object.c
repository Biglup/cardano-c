/**
 * \file object.c
 *
 * \author luisd.bianchi
 * \date   Mar 03, 2024
 *
 * Copyright 2024 Biglup Labs
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

#include <cardano/object.h>

#include <assert.h>
#include <string.h>

#include "./config.h"
#include "./string_safe.h"

/* STATIC FUNCTIONS **********************************************************/

/**
 * Copies a null-terminated string into a buffer safely using memcpy.
 *
 * @param dest Destination buffer where the string is copied.
 * @param src Source string to be copied.
 * @param maxDestSize Maximum size of the destination buffer.
 */
static void
safe_string_copy(char* dest, const char* src, const size_t max_dest_size)
{
  assert(dest != NULL);
  assert(src != NULL);
  assert(max_dest_size > 0U);

  const size_t src_length = cardano_safe_strlen(src, max_dest_size);
  const size_t copy_size  = (src_length < (max_dest_size - 1U)) ? src_length : (max_dest_size - 1U);

  cardano_safe_memcpy(dest, max_dest_size, src, copy_size);

  dest[copy_size] = '\0';
}

/* DEFINITIONS ****************************************************************/

void
cardano_object_unref(cardano_object_t** object)
{
  if ((object == NULL) || (*object == NULL))
  {
    return;
  }

  cardano_object_t* reference = *object;

  if (reference->ref_count > 0U)
  {
    reference->ref_count -= 1U;
  }

  if (reference->ref_count == 0U)
  {
    assert(reference->deallocator != NULL);
    reference->deallocator(reference);
    *object = NULL;
  }
}

void
cardano_object_ref(cardano_object_t* object)
{
  if (object == NULL)
  {
    return;
  }

  object->ref_count += 1U;
}

size_t
cardano_object_refcount(const cardano_object_t* object)
{
  if (object == NULL)
  {
    return 0;
  }

  return object->ref_count;
}

void
cardano_object_set_last_error(cardano_object_t* object, const char* message)
{
  if ((object != NULL) && (message != NULL))
  {
    safe_string_copy(object->last_error, message, sizeof(object->last_error));
  }
}

const char*
cardano_object_get_last_error(const cardano_object_t* object)
{
  if (object == NULL)
  {
    return "Object is NULL.";
  }

  return object->last_error;
}