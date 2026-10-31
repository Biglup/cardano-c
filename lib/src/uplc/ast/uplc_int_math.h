/**
 * \file uplc_int_math.h
 *
 * \author angel.castillo
 * \date   Jun 27, 2026
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

#ifndef CARDANO_UPLC_INT_MATH_H
#define CARDANO_UPLC_INT_MATH_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief Adds signed integers, returning true on overflow without evaluating an overflowing expression.
 * \param[in] a First operand.
 * \param[in] b Second operand.
 * \param[out] result Result, valid only when false is returned.
 * \return Whether the mathematical result exceeds the int64 range.
 */
static inline bool
cardano_uplc_i64_add_overflow(int64_t a, int64_t b, int64_t* result)
{
#if defined(_MSC_VER) && !defined(__clang__)
  if (((b > 0) && (a > INT64_MAX - b)) || ((b < 0) && (a < INT64_MIN - b)))
    return true;
  *result = a + b;
  return false;
#else
  return __builtin_add_overflow(a, b, result);
#endif
}

/**
 * \brief Subtracts signed integers with overflow detection.
 * \param[in] a Minuend.
 * \param[in] b Subtrahend.
 * \param[out] result Result, valid only when false is returned.
 * \return Whether the mathematical result exceeds the int64 range.
 */
static inline bool
cardano_uplc_i64_sub_overflow(int64_t a, int64_t b, int64_t* result)
{
#if defined(_MSC_VER) && !defined(__clang__)
  if (((b > 0) && (a < INT64_MIN + b)) || ((b < 0) && (a > INT64_MAX + b)))
    return true;
  *result = a - b;
  return false;
#else
  return __builtin_sub_overflow(a, b, result);
#endif
}

/**
 * \brief Multiplies signed integers with overflow detection.
 * \param[in] a First operand.
 * \param[in] b Second operand.
 * \param[out] result Result, valid only when false is returned.
 * \return Whether the mathematical result exceeds the int64 range.
 */
static inline bool
cardano_uplc_i64_mul_overflow(int64_t a, int64_t b, int64_t* result)
{
#if defined(_MSC_VER) && !defined(__clang__)
  if (a > 0)
  {
    if (((b > 0) && (a > INT64_MAX / b)) || ((b < 0) && (b < INT64_MIN / a)))
      return true;
  }
  else if (a < 0)
  {
    if (((b > 0) && (a < INT64_MIN / b)) || ((b < 0) && (a < INT64_MAX / b)))
      return true;
  }
  *result = a * b;
  return false;
#else
  return __builtin_mul_overflow(a, b, result);
#endif
}

#endif
