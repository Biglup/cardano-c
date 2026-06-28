/**
 * \file uplc_machine_costs.c
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

/* INCLUDES ******************************************************************/

#include "uplc_machine_costs.h"

#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Memory cost shared by startup.
 */
static const int64_t CARDANO_UPLC_STARTUP_MEM = 100;

/**
 * \brief CPU cost of startup.
 */
static const int64_t CARDANO_UPLC_STARTUP_CPU = 100;

/**
 * \brief Memory cost of a normal machine step.
 */
static const int64_t CARDANO_UPLC_STEP_MEM = 100;

/**
 * \brief CPU cost of a normal machine step.
 */
static const int64_t CARDANO_UPLC_STEP_CPU = 16000;

/**
 * \brief Sentinel cost for a step kind unavailable in a language version.
 *
 * Applied to constr and case under V1/V2 so any program reaching them is driven
 * out of budget.
 */
static const int64_t CARDANO_UPLC_UNAVAILABLE_STEP_COST = 30000000000;

/* DEFINITIONS ***************************************************************/

cardano_uplc_machine_costs_t
cardano_uplc_machine_costs_default(cardano_uplc_cost_model_version_t version)
{
  cardano_uplc_machine_costs_t costs;

  const cardano_uplc_budget_t startup = { CARDANO_UPLC_STARTUP_CPU, CARDANO_UPLC_STARTUP_MEM };
  const cardano_uplc_budget_t step    = { CARDANO_UPLC_STEP_CPU, CARDANO_UPLC_STEP_MEM };
  const cardano_uplc_budget_t unavail = { CARDANO_UPLC_UNAVAILABLE_STEP_COST, CARDANO_UPLC_UNAVAILABLE_STEP_COST };

  costs.startup   = startup;
  costs.constant  = step;
  costs.var_step  = step;
  costs.lambda    = step;
  costs.apply     = step;
  costs.delay     = step;
  costs.force     = step;
  costs.builtin   = step;
  costs.constr    = step;
  costs.case_step = step;

  switch (version)
  {
    case CARDANO_UPLC_COST_MODEL_VERSION_V1:
    case CARDANO_UPLC_COST_MODEL_VERSION_V2:
    {
      costs.constr    = unavail;
      costs.case_step = unavail;
      break;
    }
    case CARDANO_UPLC_COST_MODEL_VERSION_V3:
    {
      break;
    }
    default:
    {
      break;
    }
  }

  return costs;
}

cardano_uplc_budget_t
cardano_uplc_machine_costs_get(
  const cardano_uplc_machine_costs_t* costs,
  cardano_uplc_step_kind_t            kind)
{
  cardano_uplc_budget_t result = { 0, 0 };

  if (costs == NULL)
  {
    return result;
  }

  switch (kind)
  {
    case CARDANO_UPLC_STEP_KIND_CONSTANT:
    {
      result = costs->constant;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_VAR:
    {
      result = costs->var_step;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_LAMBDA:
    {
      result = costs->lambda;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_APPLY:
    {
      result = costs->apply;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_DELAY:
    {
      result = costs->delay;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_FORCE:
    {
      result = costs->force;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_BUILTIN:
    {
      result = costs->builtin;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_CONSTR:
    {
      result = costs->constr;
      break;
    }
    case CARDANO_UPLC_STEP_KIND_CASE:
    {
      result = costs->case_step;
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}
