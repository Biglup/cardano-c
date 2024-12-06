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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DEFINITIONS ***************************************************************/

void
cardano_safe_memcpy(void* dest, const size_t dest_size, const void* src, const size_t src_size)
{
  if ((dest == NULL) || (src == NULL) || (dest_size == 0U) || (src_size == 0U))
  {
    return;
  }

  size_t copy_size = (src_size < dest_size) ? src_size : dest_size;

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

size_t
cardano_safe_int64_to_string(const int64_t value, char* buffer, size_t buffer_size)
{
  if ((buffer == NULL) || (buffer_size == 0U))
  {
    return 0U;
  }

  int written = snprintf(buffer, buffer_size, "%lld", (long long)value);

  if ((written < 0) || ((size_t)written >= buffer_size))
  {
    return 0U;
  }

  return (size_t)written;
}

cardano_error_t
cardano_safe_string_to_int64(const char* str, size_t string_size, int64_t* value)
{
  if ((str == NULL) || (value == NULL) || (string_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (string_size >= 32U)
  {
    return CARDANO_ERROR_INTEGER_OVERFLOW;
  }

  char temp[32] = { 0 };
  cardano_safe_memcpy(temp, 32, str, string_size);
  temp[string_size] = '\0';

  char* endptr = NULL;
  errno        = 0;

  int64_t result = strtoll(temp, &endptr, 10);

  if (errno == ERANGE)
  {
    return CARDANO_ERROR_INTEGER_OVERFLOW;
  }

  if (endptr == temp)
  {
    return CARDANO_ERROR_DECODING;
  }

  if (*endptr != '\0')
  {
    return CARDANO_ERROR_DECODING;
  }

  *value = result;

  return CARDANO_SUCCESS;
}

size_t
cardano_safe_uint64_to_string(const uint64_t value, char* buffer, size_t buffer_size)
{
  if ((buffer == NULL) || (buffer_size == 0U))
  {
    return 0U;
  }

  int written = snprintf(buffer, buffer_size, "%llu", (unsigned long long)value);

  if ((written < 0) || ((size_t)written >= buffer_size))
  {
    return 0U;
  }

  return (size_t)written;
}

cardano_error_t
cardano_safe_string_to_uint64(const char* str, const size_t string_size, uint64_t* value)
{
  if ((str == NULL) || (value == NULL) || (string_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (string_size >= 32U)
  {
    return CARDANO_ERROR_INTEGER_OVERFLOW;
  }

  char temp[32] = { 0 };
  cardano_safe_memcpy(temp, 32, str, string_size);
  temp[string_size] = '\0';

  char* endptr = NULL;
  errno        = 0;

  unsigned long long result = strtoull(temp, &endptr, 10);

  // cppcheck-suppress misra-c2012-22.10; Reason: False positive, strtoull sets errno.
  if ((errno == ERANGE) || (result > UINT64_MAX))
  {
    return CARDANO_ERROR_INTEGER_OVERFLOW;
  }

  if (endptr == temp)
  {
    return CARDANO_ERROR_DECODING;
  }

  if (*endptr != '\0')
  {
    return CARDANO_ERROR_DECODING;
  }

  *value = (uint64_t)result;

  return CARDANO_SUCCESS;
}

size_t
cardano_safe_double_to_string(double value, char* buffer, const size_t buffer_size)
{
  if ((buffer == NULL) || (buffer_size == 0U))
  {
    return 0U;
  }

  if (isnan(value) || isinf(value))
  {
    return 0U;
  }

  int written = snprintf(buffer, buffer_size, "%.17g", value);

  if ((written < 0) || ((size_t)written >= buffer_size))
  {
    return 0U;
  }

  return (size_t)written;
}

cardano_error_t
cardano_safe_string_to_double(const char* str, const size_t string_size, double* value)
{
  if ((str == NULL) || (value == NULL) || (string_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (string_size >= 128U)
  {
    return CARDANO_ERROR_DECODING;
  }

  char temp[128] = { 0 };
  cardano_safe_memcpy(temp, 128, str, string_size);
  temp[string_size] = '\0';

  char* endptr = NULL;
  errno        = 0;

  double result = strtod(temp, &endptr);

  if (errno == ERANGE)
  {
    return CARDANO_ERROR_DECODING;
  }

  if (endptr == temp)
  {
    return CARDANO_ERROR_DECODING;
  }

  if (*endptr != '\0')
  {
    return CARDANO_ERROR_DECODING;
  }

  *value = result;

  return CARDANO_SUCCESS;
}