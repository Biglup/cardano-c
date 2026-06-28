/**
 * \file uplc_machine_costs.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_MACHINE_COSTS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_MACHINE_COSTS_H

/* INCLUDES ******************************************************************/

#include "../machine/uplc_budget.h"
#include "uplc_cost_model_version.h"
#include "uplc_step_kind.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The CPU/memory cost of each CEK machine step.
 *
 * Holds the one-off startup cost and a budget for each of the nine step kinds,
 * indexable by \ref cardano_uplc_step_kind_t through
 * \ref cardano_uplc_machine_costs_get.
 */
typedef struct cardano_uplc_machine_costs_t
{
    /** \brief Charged once before the evaluation loop starts. */
    cardano_uplc_budget_t startup;
    /** \brief Cost of one constant step. */
    cardano_uplc_budget_t constant;
    /** \brief Cost of one variable step. */
    cardano_uplc_budget_t var_step;
    /** \brief Cost of one lambda step. */
    cardano_uplc_budget_t lambda;
    /** \brief Cost of one apply step. */
    cardano_uplc_budget_t apply;
    /** \brief Cost of one delay step. */
    cardano_uplc_budget_t delay;
    /** \brief Cost of one force step. */
    cardano_uplc_budget_t force;
    /** \brief Cost of one builtin step. */
    cardano_uplc_budget_t builtin;
    /** \brief Cost of one constr step (sentinel under V1/V2). */
    cardano_uplc_budget_t constr;
    /** \brief Cost of one case step (sentinel under V1/V2). */
    cardano_uplc_budget_t case_step;
} cardano_uplc_machine_costs_t;

/**
 * \brief Returns the default machine-cost table for a language version.
 *
 * For V1 and V2 the constr and case costs are a prohibitive sentinel (those step
 * kinds are unavailable); for V3 they are the normal 100/16000. All other steps
 * are 100/16000 and startup is 100/100 in every version. An unrecognized version
 * is treated as V3.
 *
 * \param[in] version The language version selecting the table.
 *
 * \return The machine-cost table by value.
 */
cardano_uplc_machine_costs_t
cardano_uplc_machine_costs_default(cardano_uplc_cost_model_version_t version);

/**
 * \brief Returns the per-step budget for a given step kind.
 *
 * \param[in] costs The cost table to read. Must not be NULL.
 * \param[in] kind The step kind to look up.
 *
 * \return The budget for \p kind, or a zero budget if \p costs is NULL or
 *         \p kind is not a valid step kind.
 */
cardano_uplc_budget_t
cardano_uplc_machine_costs_get(
  const cardano_uplc_machine_costs_t* costs,
  cardano_uplc_step_kind_t            kind);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_MACHINE_COSTS_H */
