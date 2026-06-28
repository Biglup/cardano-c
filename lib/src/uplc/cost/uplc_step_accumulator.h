/**
 * \file uplc_step_accumulator.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_STEP_ACCUMULATOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_STEP_ACCUMULATOR_H

/* INCLUDES ******************************************************************/

#include "../machine/uplc_budget.h"
#include "uplc_machine_costs.h"
#include "uplc_step_kind.h"

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Accumulates step counts per kind and flushes them into a budget.
 *
 * Step costs are not charged one transition at a time; each step bumps a
 * per-kind counter, and once the total reaches \c slippage the accumulated
 * counts are multiplied by the per-step cost and added to \c spent in one batch
 * (and the counters reset). A final flush at machine end charges whatever is
 * left. The end result equals, for every kind, count_of_kind * per_step_cost,
 * which is what makes the batching budget-exact.
 *
 * Overflow posture: \c spent is signed 64-bit and is meant to grow past the
 * initial budget (that is how exhaustion is detected). All additions and the
 * count-times-cost product saturate at \c INT64_MAX rather than wrapping, so a
 * malicious step count or cost can only ever drive the budget further into
 * exhaustion, never wrap it back to a passing value.
 */
typedef struct cardano_uplc_step_accumulator_t
{
    /** \brief The per-step cost table used when flushing. */
    cardano_uplc_machine_costs_t costs;
    /** \brief Total budget charged so far (startup + flushed steps + explicit). */
    cardano_uplc_budget_t spent;
    /** \brief Pending occurrence count for each step kind, indexed by kind. */
    uint64_t counts[CARDANO_UPLC_STEP_KIND_COUNT];
    /** \brief Total pending step count across all kinds, the flush trigger. */
    uint64_t pending;
    /** \brief Flush threshold: flush when \c pending reaches this value. */
    uint64_t slippage;
} cardano_uplc_step_accumulator_t;

/**
 * \brief Initializes a step accumulator with a cost table and slippage.
 *
 * Zeroes the spent budget and all pending counts and copies \p costs into the
 * accumulator. The slippage threshold is the default value (200).
 *
 * \param[out] acc The accumulator to initialize. Must not be NULL.
 * \param[in] costs The per-step cost table to charge against. Must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p acc or \p costs is NULL.
 */
cardano_error_t
cardano_uplc_step_accumulator_init(
  cardano_uplc_step_accumulator_t*    acc,
  const cardano_uplc_machine_costs_t* costs);

/**
 * \brief Charges the one-off startup cost into the spent budget.
 *
 * Call once before the evaluation loop. Adds \c costs.startup to \c spent with
 * saturating addition.
 *
 * \param[in,out] acc The accumulator to charge. Must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p acc is NULL.
 */
cardano_error_t
cardano_uplc_step_accumulator_charge_startup(cardano_uplc_step_accumulator_t* acc);

/**
 * \brief Records one CEK step of the given kind, flushing at the threshold.
 *
 * Increments the per-kind counter and the total pending count. When the total
 * reaches the slippage threshold, every pending count is multiplied by its
 * per-step cost, the products are added to the spent budget, and the counters
 * reset.
 *
 * \param[in,out] acc The accumulator. Must not be NULL.
 * \param[in] kind The kind of step taken.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p acc is NULL.
 */
cardano_error_t
cardano_uplc_step_accumulator_step(
  cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_step_kind_t         kind);

/**
 * \brief Adds an explicit budget delta to the spent budget.
 *
 * Used by builtins, which compute their own cost rather than a per-step cost.
 * The delta is added to \c spent with saturating addition.
 *
 * \param[in,out] acc The accumulator. Must not be NULL.
 * \param[in] delta The budget to charge.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p acc is NULL.
 */
cardano_error_t
cardano_uplc_step_accumulator_charge(
  cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_budget_t            delta);

/**
 * \brief Flushes any remaining pending step counts into the spent budget.
 *
 * Call once at machine end. After this returns, \c spent equals the exact total
 * of every step's cost. Calling it again with no pending steps is a no-op.
 *
 * \param[in,out] acc The accumulator. Must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p acc is NULL.
 */
cardano_error_t
cardano_uplc_step_accumulator_flush(cardano_uplc_step_accumulator_t* acc);

/**
 * \brief Reads the total budget spent so far.
 *
 * Reflects the spent budget at the moment of the call; pending un-flushed steps
 * are not included, so call \ref cardano_uplc_step_accumulator_flush first for
 * the final total.
 *
 * \param[in] acc The accumulator. Must not be NULL.
 *
 * \return The spent budget, or a zero budget if \p acc is NULL.
 */
cardano_uplc_budget_t
cardano_uplc_step_accumulator_spent(const cardano_uplc_step_accumulator_t* acc);

/**
 * \brief Tests whether an initial budget has been exhausted.
 *
 * The remaining budget is \p initial minus the spent budget; the budget is
 * exhausted when either component of the remainder is negative. Pending
 * un-flushed steps are not considered.
 *
 * \param[in] acc The accumulator. Must not be NULL.
 * \param[in] initial The initial budget the script was given.
 *
 * \return \c true if remaining cpu or mem is negative, \c false otherwise (and
 *         \c false if \p acc is NULL).
 */
bool
cardano_uplc_step_accumulator_is_exhausted(
  const cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_budget_t                  initial);

/**
 * \brief Computes the remaining budget for a given initial budget.
 *
 * Returns \p initial minus the spent budget, component-wise, with saturating
 * subtraction. Pending un-flushed steps are not considered.
 *
 * \param[in] acc The accumulator. Must not be NULL.
 * \param[in] initial The initial budget the script was given.
 *
 * \return The remaining budget, or \p initial if \p acc is NULL.
 */
cardano_uplc_budget_t
cardano_uplc_step_accumulator_remaining(
  const cardano_uplc_step_accumulator_t* acc,
  cardano_uplc_budget_t                  initial);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_STEP_ACCUMULATOR_H */
