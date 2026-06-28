/**
 * \file uplc_two_var_quadratic_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_VAR_QUADRATIC_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_VAR_QUADRATIC_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_cost_sat.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A two-variable quadratic floored by a minimum.
 *
 * Evaluates max(\c minimum, \c coeff_00 + \c coeff_10 * x + \c coeff_01 * y +
 * \c coeff_20 * x * x + \c coeff_11 * x * y + \c coeff_02 * y * y).
 */
typedef struct cardano_uplc_two_var_quadratic_cost_t
{
  /** \brief The lower bound on the result. */
  int64_t minimum;
  /** \brief The constant term. */
  int64_t coeff_00;
  /** \brief The coefficient of x. */
  int64_t coeff_10;
  /** \brief The coefficient of y. */
  int64_t coeff_01;
  /** \brief The coefficient of x*x. */
  int64_t coeff_20;
  /** \brief The coefficient of x*y. */
  int64_t coeff_11;
  /** \brief The coefficient of y*y. */
  int64_t coeff_02;
} cardano_uplc_two_var_quadratic_cost_t;

/**
 * \brief Evaluates a two-variable quadratic floored by its minimum, saturating.
 *
 * Computes c00 + c10*x + c01*y + c20*x*x + c11*x*y + c02*y*y, then takes the
 * maximum with the configured minimum.
 *
 * \param[in] q The two-variable quadratic parameters.
 * \param[in] x The first size argument.
 * \param[in] y The second size argument.
 *
 * \return max(minimum, quadratic), saturating.
 */
static inline int64_t
cardano_uplc_two_var_quadratic_cost_eval(cardano_uplc_two_var_quadratic_cost_t q, int64_t x, int64_t y)
{
  int64_t result = sat_add(q.coeff_00, sat_mul(q.coeff_10, x));
  result         = sat_add(result, sat_mul(q.coeff_01, y));
  result         = sat_add(result, sat_mul(q.coeff_20, sat_mul(x, x)));
  result         = sat_add(result, sat_mul(q.coeff_11, sat_mul(x, y)));
  result         = sat_add(result, sat_mul(q.coeff_02, sat_mul(y, y)));

  return max_i64(q.minimum, result);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_VAR_QUADRATIC_COST_H */
