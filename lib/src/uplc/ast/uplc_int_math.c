/**
 * \file uplc_int_math.c
 *
 * \author angel.castillo
 * \date   Oct 31, 2026
 *
 * Copyright 2026 Biglup Labs
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

#include "uplc_int_math.h"

/* DEFINITIONS ***************************************************************/

bool
cardano_uplc_i64_add_overflow(int64_t a, int64_t b, int64_t* result)
{
#if defined(_MSC_VER) && !defined(__clang__)
  if (((b > 0) && (a > ((int64_t)INT64_MAX - b))) || ((b < 0) && (a < ((int64_t)(INT64_MIN)-b))))
  {
    return true;
  }

  *result = a + b;

  return false;
#else
  return __builtin_add_overflow(a, b, result);
#endif
}

bool
cardano_uplc_i64_sub_overflow(int64_t a, int64_t b, int64_t* result)
{
#if defined(_MSC_VER) && !defined(__clang__)
  if (((b > 0) && (a < ((int64_t)(INT64_MIN) + b))) || ((b < 0) && (a > ((int64_t)INT64_MAX + b))))
  {
    return true;
  }

  *result = a - b;

  return false;
#else
  return __builtin_sub_overflow(a, b, result);
#endif
}

bool
cardano_uplc_i64_mul_overflow(int64_t a, int64_t b, int64_t* result)
{
#if defined(_MSC_VER) && !defined(__clang__)
  if (a > 0)
  {
    if (((b > 0) && (a > ((int64_t)INT64_MAX / b))) || ((b < 0) && (b < ((int64_t)(INT64_MIN) / a))))
    {
      return true;
    }
  }
  else if (a < 0)
  {
    if (((b > 0) && (a < ((int64_t)(INT64_MIN) / b))) || ((b < 0) && (a < ((int64_t)INT64_MAX / b))))
    {
      return true;
    }
  }
  else
  {
    *result = 0;

    return false;
  }

  *result = a * b;

  return false;
#else
  return __builtin_mul_overflow(a, b, result);
#endif
}
