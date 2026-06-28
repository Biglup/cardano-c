/**
 * \file uplc_cost_sat.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_SAT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_SAT_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Adds two signed 64-bit values, saturating at the int64 bounds.
 *
 * \param[in] a The first operand.
 * \param[in] b The second operand.
 *
 * \return \p a + \p b, clamped to [INT64_MIN, INT64_MAX] on overflow.
 */
static inline int64_t
sat_add(int64_t a, int64_t b)
{
  int64_t result = 0;

  if (__builtin_add_overflow(a, b, &result))
  {
    result = (a >= 0) ? INT64_MAX : INT64_MIN;
  }

  return result;
}

/**
 * \brief Subtracts two signed 64-bit values, saturating at the int64 bounds.
 *
 * \param[in] a The minuend.
 * \param[in] b The subtrahend.
 *
 * \return \p a - \p b, clamped to [INT64_MIN, INT64_MAX] on overflow.
 */
static inline int64_t
sat_sub(int64_t a, int64_t b)
{
  int64_t result = 0;

  if (__builtin_sub_overflow(a, b, &result))
  {
    result = (a >= 0) ? INT64_MAX : INT64_MIN;
  }

  return result;
}

/**
 * \brief Multiplies two signed 64-bit values, saturating at the int64 bounds.
 *
 * The result clamps to \c INT64_MAX or \c INT64_MIN on overflow rather than
 * wrapping.
 *
 * \param[in] a The first operand.
 * \param[in] b The second operand.
 *
 * \return \p a * \p b, clamped to [INT64_MIN, INT64_MAX] on overflow.
 */
static inline int64_t
sat_mul(int64_t a, int64_t b)
{
  int64_t result = 0;

  if (__builtin_mul_overflow(a, b, &result))
  {
    result = ((a > 0) == (b > 0)) ? INT64_MAX : INT64_MIN;
  }

  return result;
}

/**
 * \brief Returns the smaller of two signed 64-bit values.
 *
 * \param[in] a The first value.
 * \param[in] b The second value.
 *
 * \return The minimum of \p a and \p b.
 */
static inline int64_t
min_i64(int64_t a, int64_t b)
{
  return (a < b) ? a : b;
}

/**
 * \brief Returns the larger of two signed 64-bit values.
 *
 * \param[in] a The first value.
 * \param[in] b The second value.
 *
 * \return The maximum of \p a and \p b.
 */
static inline int64_t
max_i64(int64_t a, int64_t b)
{
  return (a > b) ? a : b;
}

/**
 * \brief Adds two signed 64-bit costs, saturating at the int64 bounds.
 *
 * Used wherever a cost is summed so that an intermediate that exceeds the int64
 * range clamps to \c INT64_MAX (or \c INT64_MIN) rather than wrapping.
 *
 * \param[in] a The first operand.
 * \param[in] b The second operand.
 *
 * \return \p a + \p b, clamped to [INT64_MIN, INT64_MAX] on overflow.
 */
int64_t
cardano_uplc_cost_sat_add(int64_t a, int64_t b);

/**
 * \brief Multiplies two signed 64-bit costs, saturating at the int64 bounds.
 *
 * Used wherever sizes or coefficients are multiplied so that a large product
 * clamps to \c INT64_MAX (or \c INT64_MIN) rather than wrapping.
 *
 * \param[in] a The first operand.
 * \param[in] b The second operand.
 *
 * \return \p a * \p b, clamped to [INT64_MIN, INT64_MAX] on overflow.
 */
int64_t
cardano_uplc_cost_sat_mul(int64_t a, int64_t b);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_SAT_H */
