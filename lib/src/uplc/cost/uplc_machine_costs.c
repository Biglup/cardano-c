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

#include <cardano/export.h>

#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Memory cost shared by startup.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_STARTUP_MEM = 100;

/**
 * \brief CPU cost of startup.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_STARTUP_CPU = 100;

/**
 * \brief Memory cost of a normal machine step.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_STEP_MEM = 100;

/**
 * \brief CPU cost of a normal machine step.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_STEP_CPU = 16000;

/* DEFINITIONS ***************************************************************/

cardano_uplc_machine_costs_t
cardano_uplc_machine_costs_default(cardano_uplc_cost_model_version_t version)
{
  cardano_uplc_machine_costs_t costs;

  const cardano_uplc_budget_t startup = { CARDANO_UPLC_STARTUP_CPU, CARDANO_UPLC_STARTUP_MEM };
  const cardano_uplc_budget_t step    = { CARDANO_UPLC_STEP_CPU, CARDANO_UPLC_STEP_MEM };

  CARDANO_UNUSED(version);

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
