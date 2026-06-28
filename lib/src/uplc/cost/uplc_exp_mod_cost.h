/**
 * \file uplc_exp_mod_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_EXP_MOD_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_EXP_MOD_COST_H

/* INCLUDES ******************************************************************/

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The exp-mod costing function over the three modExp size arguments.
 *
 * Computes base = \c coeff_00 + \c coeff_11 * y * z + \c coeff_12 * y * z * z,
 * then base when x <= z and base + base / 2 otherwise.
 */
typedef struct cardano_uplc_exp_mod_cost_t
{
  /** \brief The constant term. */
  int64_t coeff_00;
  /** \brief The coefficient of the y*z term. */
  int64_t coeff_11;
  /** \brief The coefficient of the y*z*z term. */
  int64_t coeff_12;
} cardano_uplc_exp_mod_cost_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_EXP_MOD_COST_H */
