/**
 * \file uplc_step_kind.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_STEP_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_STEP_KIND_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The number of CEK step kinds that carry a per-step cost.
 *
 * Startup is charged once outside the step loop and is not part of this set, so
 * it is not counted here. The nine kinds run from constant through case.
 */
#define CARDANO_UPLC_STEP_KIND_COUNT ((size_t)9)

/**
 * \brief The kind of CEK machine step that incurs a per-step cost.
 *
 * The numeric values are the index into the per-kind occurrence array used by
 * the slippage accumulator and must stay in this order. Startup is not a
 * member: it is charged once by \ref cardano_uplc_step_accumulator_charge_startup.
 */
typedef enum
{
  /** \brief Computing a constant term. */
  CARDANO_UPLC_STEP_KIND_CONSTANT = 0,
  /** \brief Computing a variable term. */
  CARDANO_UPLC_STEP_KIND_VAR = 1,
  /** \brief Computing a lambda term. */
  CARDANO_UPLC_STEP_KIND_LAMBDA = 2,
  /** \brief Computing an apply term. */
  CARDANO_UPLC_STEP_KIND_APPLY = 3,
  /** \brief Computing a delay term. */
  CARDANO_UPLC_STEP_KIND_DELAY = 4,
  /** \brief Computing a force term. */
  CARDANO_UPLC_STEP_KIND_FORCE = 5,
  /** \brief Computing a builtin term (not the builtin call itself). */
  CARDANO_UPLC_STEP_KIND_BUILTIN = 6,
  /** \brief Computing a constr term. */
  CARDANO_UPLC_STEP_KIND_CONSTR = 7,
  /** \brief Computing a case term. */
  CARDANO_UPLC_STEP_KIND_CASE = 8
} cardano_uplc_step_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_STEP_KIND_H */
