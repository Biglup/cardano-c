/**
 * \file uplc_step_accumulator.c
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

#include "uplc_step_accumulator.h"
#include "../machine/uplc_budget.h"
#include "uplc_cost_sat.h"
#include "uplc_machine_costs.h"
#include "uplc_step_kind.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief The default slippage threshold: flush after this many steps.
 */
static const uint64_t CARDANO_UPLC_DEFAULT_SLIPPAGE = 200U;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Multiplies a signed cost by a non-negative count, saturating high.
 *
 * The count comes from an occurrence counter and is always non-negative, so the
 * product cannot underflow; only the positive overflow needs clamping.
 *
 * \param[in] cost The per-step cost (non-negative in every default table).
 * \param[in] count The number of occurrences.
 *
 * \return \p cost * \p count, clamped to INT64_MAX on overflow.
 */
static int64_t
sat_mul_count(int64_t cost, uint64_t count)
{
  int64_t result = 0;

  if ((cost == 0) || (count == 0U))
  {
    result = 0;
  }
  else if (cost > 0)
  {
    if ((uint64_t)cost > ((uint64_t)INT64_MAX / count))
    {
      result = INT64_MAX;
    }
    else
    {
      result = (int64_t)((uint64_t)cost * count);
    }
  }
  else
  {
    uint64_t magnitude = (uint64_t)(-(cost + 1)) + 1U;

    if (magnitude > ((uint64_t)INT64_MAX / count))
    {
      result = INT64_MIN;
    }
    else
    {
      result = -(int64_t)(magnitude * count);
    }
  }

  return result;
}

/**
 * \brief Flushes all pending step counts into the spent budget and resets them.
 *
 * For each step kind with a non-zero count, adds count * per-step cost (cpu and
 * mem separately) to the spent budget and zeroes the count, iterating the kinds
 * in index order. Resets the pending total.
 *
 * \param[in,out] acc The accumulator to flush. Must not be NULL.
 */
static void
flush_step_budget(cardano_uplc_step_accumulator_t* acc)
{
  size_t i = 0U;

  for (i = 0U; i < CARDANO_UPLC_STEP_KIND_COUNT; ++i)
  {
    uint64_t n = acc->counts[i];

    if (n > 0U)
    {
      cardano_uplc_budget_t cost = cardano_uplc_machine_costs_get(&acc->costs, (cardano_uplc_step_kind_t)i);

      acc->spent.cpu = sat_add(acc->spent.cpu, sat_mul_count(cost.cpu, n));
      acc->spent.mem = sat_add(acc->spent.mem, sat_mul_count(cost.mem, n));

      acc->counts[i] = 0U;
    }
  }

  acc->pending = 0U;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_step_accumulator_init(
  cardano_uplc_step_accumulator_t*    acc,
  const cardano_uplc_machine_costs_t* costs)
{
  if ((acc == NULL) || (costs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  acc->costs     = *costs;
  acc->spent.cpu = 0;
  acc->spent.mem = 0;
  acc->pending   = 0U;
  acc->slippage  = CARDANO_UPLC_DEFAULT_SLIPPAGE;

  memset(acc->counts, 0, sizeof(acc->counts));

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_step_accumulator_charge_startup(cardano_uplc_step_accumulator_t* acc)
{
  if (acc == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  acc->spent.cpu = sat_add(acc->spent.cpu, acc->costs.startup.cpu);
  acc->spent.mem = sat_add(acc->spent.mem, acc->costs.startup.mem);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_step_accumulator_step(
  cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_step_kind_t         kind)
{
  if (acc == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((size_t)kind < CARDANO_UPLC_STEP_KIND_COUNT)
  {
    ++acc->counts[(size_t)kind];
    ++acc->pending;
  }

  if (acc->pending >= acc->slippage)
  {
    flush_step_budget(acc);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_step_accumulator_charge(
  cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_budget_t            delta)
{
  if (acc == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  acc->spent.cpu = sat_add(acc->spent.cpu, delta.cpu);
  acc->spent.mem = sat_add(acc->spent.mem, delta.mem);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_step_accumulator_flush(cardano_uplc_step_accumulator_t* acc)
{
  if (acc == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  flush_step_budget(acc);

  return CARDANO_SUCCESS;
}

cardano_uplc_budget_t
cardano_uplc_step_accumulator_spent(const cardano_uplc_step_accumulator_t* acc)
{
  cardano_uplc_budget_t result = { 0, 0 };

  if (acc != NULL)
  {
    result = acc->spent;
  }

  return result;
}

cardano_uplc_budget_t
cardano_uplc_step_accumulator_remaining(
  const cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_budget_t                  initial)
{
  cardano_uplc_budget_t result = initial;

  if (acc != NULL)
  {
    result.cpu = sat_sub(initial.cpu, acc->spent.cpu);
    result.mem = sat_sub(initial.mem, acc->spent.mem);
  }

  return result;
}

bool
cardano_uplc_step_accumulator_is_exhausted(
  const cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_budget_t                  initial)
{
  cardano_uplc_budget_t remaining;

  if (acc == NULL)
  {
    return false;
  }

  remaining = cardano_uplc_step_accumulator_remaining(acc, initial);

  return (remaining.cpu < 0) || (remaining.mem < 0);
}
