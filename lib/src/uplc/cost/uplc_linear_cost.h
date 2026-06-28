/**
 * \file uplc_linear_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_LINEAR_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_LINEAR_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_cost_sat.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A linear cost: \c intercept + \c slope * x, evaluated saturating.
 *
 * The member order is intercept first, then slope.
 */
typedef struct cardano_uplc_linear_cost_t
{
  /** \brief The constant term added to the slope contribution. */
  int64_t intercept;
  /** \brief The coefficient multiplying the size argument. */
  int64_t slope;
} cardano_uplc_linear_cost_t;

/**
 * \brief Evaluates a linear cost intercept + slope * x with saturation.
 *
 * \param[in] l The linear cost parameters.
 * \param[in] x The size argument.
 *
 * \return \c l.intercept + \c l.slope * x, saturating.
 */
static inline int64_t
cardano_uplc_linear_cost_eval(cardano_uplc_linear_cost_t l, int64_t x)
{
  return sat_add(l.intercept, sat_mul(l.slope, x));
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_LINEAR_COST_H */
