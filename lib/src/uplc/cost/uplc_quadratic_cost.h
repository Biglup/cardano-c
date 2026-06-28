/**
 * \file uplc_quadratic_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_QUADRATIC_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_QUADRATIC_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_cost_sat.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A one-variable quadratic: \c coeff_0 + \c coeff_1 * x + \c coeff_2 * x * x.
 */
typedef struct cardano_uplc_quadratic_cost_t
{
  /** \brief The constant term. */
  int64_t coeff_0;
  /** \brief The linear coefficient. */
  int64_t coeff_1;
  /** \brief The quadratic coefficient. */
  int64_t coeff_2;
} cardano_uplc_quadratic_cost_t;

/**
 * \brief Evaluates a one-variable quadratic c0 + c1*x + c2*x*x with saturation.
 *
 * \param[in] q The quadratic cost parameters.
 * \param[in] x The size argument.
 *
 * \return The saturating quadratic value.
 */
static inline int64_t
cardano_uplc_quadratic_cost_eval(cardano_uplc_quadratic_cost_t q, int64_t x)
{
  int64_t result = sat_add(q.coeff_0, sat_mul(q.coeff_1, x));
  result         = sat_add(result, sat_mul(q.coeff_2, sat_mul(x, x)));

  return result;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_QUADRATIC_COST_H */
