/**
 * \file utils.c
 *
 * \author luisd.bianchi
 * \date   Sep 30, 2024
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

#include "utils.h"

#include <cardano/export.h>

#include <assert.h>
#include <string.h>
#include <time.h>

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* DEFINITIONS ***************************************************************/

void
cardano_utils_safe_memcpy(void* dest, const size_t dest_size, const void* src, const size_t src_size)
{
  if ((dest == NULL) || (src == NULL) || (dest_size == 0U) || (src_size == 0U))
  {
    return;
  }

  size_t copy_size = (src_size < dest_size) ? src_size : dest_size;

  CARDANO_UNUSED(memcpy(dest, src, copy_size)); // nosemgrep
}

size_t
cardano_utils_safe_strlen(const char* str, const size_t max_length)
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

void
cardano_utils_set_error_message(cardano_provider_impl_t* provider_impl, const char* message)
{
  assert(provider_impl != NULL);

  size_t message_size = cardano_utils_safe_strlen(message, 1023);

  CARDANO_UNUSED(memcpy(provider_impl->error_message, message, message_size));
  provider_impl->error_message[message_size] = '\0';
}

uint64_t
cardano_utils_get_time()
{
  return (uint64_t)time(NULL);
}

uint64_t
cardano_utils_get_elapsed_time_since(const uint64_t start)
{
  const uint64_t current_time = cardano_utils_get_time();

  return ((current_time >= start) ? (current_time - start) : 0U);
}

void
cardano_utils_sleep(const uint64_t milliseconds)
{
#ifdef _WIN32
  Sleep((DWORD)milliseconds); // Windows Sleep function takes milliseconds
#else
  usleep(milliseconds * 1000U); // POSIX usleep takes microseconds
#endif
}
