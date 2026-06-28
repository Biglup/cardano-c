/**
 * \file uplc_two_var_linear_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_VAR_LINEAR_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_VAR_LINEAR_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_cost_sat.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A two-variable linear cost: \c intercept + \c slope1 * x + \c slope2 * y.
 *
 * Reused for the two-argument linear-in-x-and-y arm and the three-argument
 * linear-in-y-and-z arm.
 */
typedef struct cardano_uplc_two_var_linear_cost_t
{
    /** \brief The constant term. */
    int64_t intercept;
    /** \brief The coefficient of the first variable. */
    int64_t slope1;
    /** \brief The coefficient of the second variable. */
    int64_t slope2;
} cardano_uplc_two_var_linear_cost_t;

/**
 * \brief Evaluates a two-variable linear cost intercept + s1*x + s2*y, saturating.
 *
 * \param[in] l The two-variable linear parameters.
 * \param[in] x The first size argument.
 * \param[in] y The second size argument.
 *
 * \return The saturating two-variable linear value.
 */
static inline int64_t
cardano_uplc_two_var_linear_cost_eval(cardano_uplc_two_var_linear_cost_t l, int64_t x, int64_t y)
{
  int64_t result = sat_add(l.intercept, sat_mul(l.slope1, x));
  result         = sat_add(result, sat_mul(l.slope2, y));

  return result;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_VAR_LINEAR_COST_H */
