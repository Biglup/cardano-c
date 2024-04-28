/**
 * \file string_safe.c
 *
 * \author luisd.bianchi
 * \date   Apr 28, 2024
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

#include "string_safe.h"

#include <cardano/export.h>

#include <string.h>

/* DEFINITIONS ***************************************************************/

void
cardano_safe_memcpy(void* dest, const size_t dest_size, const void* src, const size_t count)
{
  if ((dest == NULL) || (src == NULL) || (dest_size == 0U) || (count == 0U))
  {
    return;
  }

  size_t copy_size = (count < dest_size) ? count : dest_size;

  CARDANO_UNUSED(memcpy(dest, src, copy_size)); // nosemgrep
}

size_t
cardano_safe_strlen(const char* str, const size_t max_length)
{
  if (str == NULL)
  {
    return 0U;
  }

  size_t index = 0U;

  while ((index < max_length) && (str[index] != '\0'))
  {
    ++index;
  }

  return index;
}