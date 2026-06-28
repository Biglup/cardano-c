/**
 * \file cost_model.cpp
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

/* INCLUDES ******************************************************************/

#include <cardano/error.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/builtins/uplc_builtin_semantics.h"
#include "../../src/uplc/cost/uplc_builtin_costs.h"
#include "../../src/uplc/cost/uplc_cost_model.h"
#include "../../src/uplc/cost/uplc_cost_sat.h"
#include "../../src/uplc/cost/uplc_ex_mem.h"
#include "../../src/uplc/cost/uplc_four_arg_cost.h"
#include "../../src/uplc/cost/uplc_machine_costs.h"
#include "../../src/uplc/cost/uplc_one_arg_cost.h"
#include "../../src/uplc/cost/uplc_six_arg_cost.h"
#include "../../src/uplc/cost/uplc_step_accumulator.h"
#include "../../src/uplc/cost/uplc_three_arg_cost.h"
#include "../../src/uplc/cost/uplc_two_arg_cost.h"
#include "../../src/uplc/machine/uplc_value.h"
#include "../../src/uplc/data/uplc_data.h"

#include <cardano/common/bigint.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include <cstdint>
#include <cstring>
#include <gmock/gmock.h>
#include <vector>

/* CONSTANTS *****************************************************************/

static const cardano_uplc_step_kind_t ALL_KINDS[CARDANO_UPLC_STEP_KIND_COUNT] = {
  CARDANO_UPLC_STEP_KIND_CONSTANT,
  CARDANO_UPLC_STEP_KIND_VAR,
  CARDANO_UPLC_STEP_KIND_LAMBDA,
  CARDANO_UPLC_STEP_KIND_APPLY,
  CARDANO_UPLC_STEP_KIND_DELAY,
  CARDANO_UPLC_STEP_KIND_FORCE,
  CARDANO_UPLC_STEP_KIND_BUILTIN,
  CARDANO_UPLC_STEP_KIND_CONSTR,
  CARDANO_UPLC_STEP_KIND_CASE
};

/* STATIC HELPERS ************************************************************/

// Charges a sequence of step kinds, flushes, and asserts the total spent equals
// the reference batched total sum(count_k * cost_k) for cpu and mem.
static void
check_sequence_matches_reference(
  cardano_uplc_cost_model_version_t              version,
  const std::vector<cardano_uplc_step_kind_t>&   sequence)
{
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(version);
  cardano_uplc_step_accumulator_t acc;

  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  std::vector<uint64_t> counts(CARDANO_UPLC_STEP_KIND_COUNT, 0U);

  for (size_t i = 0U; i < sequence.size(); ++i)
  {
    ASSERT_EQ(cardano_uplc_step_accumulator_step(&acc, sequence[i]), CARDANO_SUCCESS);
    counts[static_cast<size_t>(sequence[i])] += 1U;
  }

  ASSERT_EQ(cardano_uplc_step_accumulator_flush(&acc), CARDANO_SUCCESS);

  int64_t expected_cpu = 0;
  int64_t expected_mem = 0;

  for (size_t i = 0U; i < CARDANO_UPLC_STEP_KIND_COUNT; ++i)
  {
    cardano_uplc_budget_t cost = cardano_uplc_machine_costs_get(&costs, ALL_KINDS[i]);
    expected_cpu += cost.cpu * static_cast<int64_t>(counts[i]);
    expected_mem += cost.mem * static_cast<int64_t>(counts[i]);
  }

  cardano_uplc_budget_t spent = cardano_uplc_step_accumulator_spent(&acc);

  EXPECT_EQ(spent.cpu, expected_cpu);
  EXPECT_EQ(spent.mem, expected_mem);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_cost_model, default_v3_has_exact_reference_values)
{
  // Arrange
  cardano_uplc_machine_costs_t costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);

  // Assert
  EXPECT_EQ(costs.startup.mem, 100);
  EXPECT_EQ(costs.startup.cpu, 100);
  EXPECT_EQ(costs.constant.mem, 100);
  EXPECT_EQ(costs.constant.cpu, 16000);
  EXPECT_EQ(costs.var_step.mem, 100);
  EXPECT_EQ(costs.var_step.cpu, 16000);
  EXPECT_EQ(costs.lambda.mem, 100);
  EXPECT_EQ(costs.lambda.cpu, 16000);
  EXPECT_EQ(costs.apply.mem, 100);
  EXPECT_EQ(costs.apply.cpu, 16000);
  EXPECT_EQ(costs.delay.mem, 100);
  EXPECT_EQ(costs.delay.cpu, 16000);
  EXPECT_EQ(costs.force.mem, 100);
  EXPECT_EQ(costs.force.cpu, 16000);
  EXPECT_EQ(costs.builtin.mem, 100);
  EXPECT_EQ(costs.builtin.cpu, 16000);
  EXPECT_EQ(costs.constr.mem, 100);
  EXPECT_EQ(costs.constr.cpu, 16000);
  EXPECT_EQ(costs.case_step.mem, 100);
  EXPECT_EQ(costs.case_step.cpu, 16000);
}

TEST(cardano_uplc_cost_model, get_returns_each_field)
{
  // Arrange
  cardano_uplc_machine_costs_t costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);

  // Act / Assert
  for (size_t i = 0U; i < CARDANO_UPLC_STEP_KIND_COUNT; ++i)
  {
    cardano_uplc_budget_t cost = cardano_uplc_machine_costs_get(&costs, ALL_KINDS[i]);
    EXPECT_EQ(cost.mem, 100);
    EXPECT_EQ(cost.cpu, 16000);
  }
}

TEST(cardano_uplc_cost_model, get_handles_null_and_invalid_kind)
{
  // Arrange
  cardano_uplc_machine_costs_t costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);

  // Act
  cardano_uplc_budget_t null_cost    = cardano_uplc_machine_costs_get(nullptr, CARDANO_UPLC_STEP_KIND_CONSTANT);
  cardano_uplc_budget_t invalid_cost = cardano_uplc_machine_costs_get(&costs, static_cast<cardano_uplc_step_kind_t>(99));

  // Assert
  EXPECT_EQ(null_cost.cpu, 0);
  EXPECT_EQ(null_cost.mem, 0);
  EXPECT_EQ(invalid_cost.cpu, 0);
  EXPECT_EQ(invalid_cost.mem, 0);
}

TEST(cardano_uplc_cost_model, v1_v2_constr_case_are_prohibitive)
{
  // Arrange
  cardano_uplc_machine_costs_t v1 = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V1);
  cardano_uplc_machine_costs_t v2 = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V2);

  // Assert
  EXPECT_EQ(v1.constr.cpu, 30000000000);
  EXPECT_EQ(v1.constr.mem, 30000000000);
  EXPECT_EQ(v1.case_step.cpu, 30000000000);
  EXPECT_EQ(v1.case_step.mem, 30000000000);

  EXPECT_EQ(v2.constr.cpu, 30000000000);
  EXPECT_EQ(v2.constr.mem, 30000000000);
  EXPECT_EQ(v2.case_step.cpu, 30000000000);
  EXPECT_EQ(v2.case_step.mem, 30000000000);

  // V1/V2 still charge the normal cost for the other steps.
  EXPECT_EQ(v1.apply.cpu, 16000);
  EXPECT_EQ(v1.apply.mem, 100);
}

TEST(cardano_uplc_cost_model, unknown_version_defaults_to_v3)
{
  // Arrange
  cardano_uplc_machine_costs_t costs = cardano_uplc_machine_costs_default(static_cast<cardano_uplc_cost_model_version_t>(99));

  // Assert
  EXPECT_EQ(costs.constr.cpu, 16000);
  EXPECT_EQ(costs.case_step.cpu, 16000);
}

TEST(cardano_uplc_cost_model, init_zeroes_spent_and_sets_slippage)
{
  // Arrange
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;

  // Act
  cardano_error_t result = cardano_uplc_step_accumulator_init(&acc, &costs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(acc.spent.cpu, 0);
  EXPECT_EQ(acc.spent.mem, 0);
  EXPECT_EQ(acc.pending, 0U);
  EXPECT_EQ(acc.slippage, 200U);
}

TEST(cardano_uplc_cost_model, startup_charged_once)
{
  // Arrange
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_uplc_step_accumulator_charge_startup(&acc), CARDANO_SUCCESS);

  // Assert
  cardano_uplc_budget_t spent = cardano_uplc_step_accumulator_spent(&acc);
  EXPECT_EQ(spent.cpu, 100);
  EXPECT_EQ(spent.mem, 100);
}

TEST(cardano_uplc_cost_model, explicit_charge_adds_to_spent)
{
  // Arrange
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  cardano_uplc_budget_t delta_a = { 1234, 56 };
  cardano_uplc_budget_t delta_b = { 1, 2 };

  // Act
  ASSERT_EQ(cardano_uplc_step_accumulator_charge(&acc, delta_a), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_step_accumulator_charge(&acc, delta_b), CARDANO_SUCCESS);

  // Assert
  cardano_uplc_budget_t spent = cardano_uplc_step_accumulator_spent(&acc);
  EXPECT_EQ(spent.cpu, 1235);
  EXPECT_EQ(spent.mem, 58);
}

TEST(cardano_uplc_cost_model, slippage_short_sequence_only_flushes_at_end)
{
  // Arrange: fewer than the slippage threshold (200) steps; nothing flushes
  // until the explicit final flush.
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  // Act
  for (size_t i = 0U; i < 5U; ++i)
  {
    ASSERT_EQ(cardano_uplc_step_accumulator_step(&acc, CARDANO_UPLC_STEP_KIND_APPLY), CARDANO_SUCCESS);
  }

  cardano_uplc_budget_t before_flush = cardano_uplc_step_accumulator_spent(&acc);

  ASSERT_EQ(cardano_uplc_step_accumulator_flush(&acc), CARDANO_SUCCESS);

  cardano_uplc_budget_t after_flush = cardano_uplc_step_accumulator_spent(&acc);

  // Assert
  EXPECT_EQ(before_flush.cpu, 0);
  EXPECT_EQ(before_flush.mem, 0);
  EXPECT_EQ(after_flush.cpu, 5 * 16000);
  EXPECT_EQ(after_flush.mem, 5 * 100);
}

TEST(cardano_uplc_cost_model, slippage_crossing_threshold_equals_full_total)
{
  // A run of exactly the threshold (200) flushes once mid-run; the running and
  // the deferred total must agree.
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  // Act: 199 steps, no flush yet.
  for (size_t i = 0U; i < 199U; ++i)
  {
    ASSERT_EQ(cardano_uplc_step_accumulator_step(&acc, CARDANO_UPLC_STEP_KIND_VAR), CARDANO_SUCCESS);
  }
  EXPECT_EQ(cardano_uplc_step_accumulator_spent(&acc).cpu, 0);

  // The 200th step reaches the threshold and flushes all 200.
  ASSERT_EQ(cardano_uplc_step_accumulator_step(&acc, CARDANO_UPLC_STEP_KIND_VAR), CARDANO_SUCCESS);

  // Assert
  cardano_uplc_budget_t spent = cardano_uplc_step_accumulator_spent(&acc);
  EXPECT_EQ(spent.cpu, 200 * 16000);
  EXPECT_EQ(spent.mem, 200 * 100);

  ASSERT_EQ(cardano_uplc_step_accumulator_flush(&acc), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_step_accumulator_spent(&acc).cpu, 200 * 16000);
}

TEST(cardano_uplc_cost_model, mixed_sequence_crossing_threshold_many_times)
{
  // A long, mixed sequence that crosses the threshold repeatedly; total must
  // equal sum(count_k * cost_k).
  std::vector<cardano_uplc_step_kind_t> sequence;

  for (size_t i = 0U; i < 1003U; ++i)
  {
    sequence.push_back(ALL_KINDS[i % CARDANO_UPLC_STEP_KIND_COUNT]);
  }

  check_sequence_matches_reference(CARDANO_UPLC_COST_MODEL_VERSION_V3, sequence);
}

TEST(cardano_uplc_cost_model, single_kind_below_and_above_threshold)
{
  std::vector<cardano_uplc_step_kind_t> below(150U, CARDANO_UPLC_STEP_KIND_FORCE);
  std::vector<cardano_uplc_step_kind_t> above(450U, CARDANO_UPLC_STEP_KIND_FORCE);

  check_sequence_matches_reference(CARDANO_UPLC_COST_MODEL_VERSION_V3, below);
  check_sequence_matches_reference(CARDANO_UPLC_COST_MODEL_VERSION_V3, above);
}

TEST(cardano_uplc_cost_model, empty_sequence_spends_nothing)
{
  std::vector<cardano_uplc_step_kind_t> empty;
  check_sequence_matches_reference(CARDANO_UPLC_COST_MODEL_VERSION_V3, empty);
}

TEST(cardano_uplc_cost_model, double_flush_is_noop)
{
  // Arrange
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  ASSERT_EQ(cardano_uplc_step_accumulator_step(&acc, CARDANO_UPLC_STEP_KIND_LAMBDA), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_uplc_step_accumulator_flush(&acc), CARDANO_SUCCESS);
  cardano_uplc_budget_t after_first = cardano_uplc_step_accumulator_spent(&acc);
  ASSERT_EQ(cardano_uplc_step_accumulator_flush(&acc), CARDANO_SUCCESS);
  cardano_uplc_budget_t after_second = cardano_uplc_step_accumulator_spent(&acc);

  // Assert
  EXPECT_EQ(after_first.cpu, 16000);
  EXPECT_EQ(after_second.cpu, 16000);
  EXPECT_EQ(after_second.mem, 100);
}

TEST(cardano_uplc_cost_model, invalid_step_kind_is_ignored)
{
  // Arrange
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_uplc_step_accumulator_step(&acc, static_cast<cardano_uplc_step_kind_t>(50)), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_step_accumulator_flush(&acc), CARDANO_SUCCESS);

  // Assert
  cardano_uplc_budget_t spent = cardano_uplc_step_accumulator_spent(&acc);
  EXPECT_EQ(spent.cpu, 0);
  EXPECT_EQ(spent.mem, 0);
  EXPECT_EQ(acc.pending, 0U);
}

TEST(cardano_uplc_cost_model, exhaustion_remaining_and_predicate)
{
  // Arrange
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  cardano_uplc_budget_t spend = { 1000, 50 };
  ASSERT_EQ(cardano_uplc_step_accumulator_charge(&acc, spend), CARDANO_SUCCESS);

  cardano_uplc_budget_t initial_ok    = { 1000, 50 };
  cardano_uplc_budget_t initial_over  = { 2000, 100 };
  cardano_uplc_budget_t initial_low_cpu = { 999, 50 };
  cardano_uplc_budget_t initial_low_mem = { 1000, 49 };

  // Act / Assert: exactly at the limit is not exhausted.
  cardano_uplc_budget_t remaining_ok = cardano_uplc_step_accumulator_remaining(&acc, initial_ok);
  EXPECT_EQ(remaining_ok.cpu, 0);
  EXPECT_EQ(remaining_ok.mem, 0);
  EXPECT_FALSE(cardano_uplc_step_accumulator_is_exhausted(&acc, initial_ok));

  // Comfortably under budget.
  cardano_uplc_budget_t remaining_over = cardano_uplc_step_accumulator_remaining(&acc, initial_over);
  EXPECT_EQ(remaining_over.cpu, 1000);
  EXPECT_EQ(remaining_over.mem, 50);
  EXPECT_FALSE(cardano_uplc_step_accumulator_is_exhausted(&acc, initial_over));

  // Either dimension going negative exhausts the budget.
  EXPECT_TRUE(cardano_uplc_step_accumulator_is_exhausted(&acc, initial_low_cpu));
  EXPECT_TRUE(cardano_uplc_step_accumulator_is_exhausted(&acc, initial_low_mem));
}

TEST(cardano_uplc_cost_model, overflow_saturates_rather_than_wraps)
{
  // Arrange: a huge occurrence count multiplied by a positive cost must clamp at
  // INT64_MAX, never wrap to a small or negative value.
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  // Drive spent close to the ceiling, then add more.
  cardano_uplc_budget_t near_max = { INT64_MAX - 5, INT64_MAX - 5 };
  ASSERT_EQ(cardano_uplc_step_accumulator_charge(&acc, near_max), CARDANO_SUCCESS);

  cardano_uplc_budget_t more = { 1000, 1000 };
  ASSERT_EQ(cardano_uplc_step_accumulator_charge(&acc, more), CARDANO_SUCCESS);

  // Assert
  cardano_uplc_budget_t spent = cardano_uplc_step_accumulator_spent(&acc);
  EXPECT_EQ(spent.cpu, INT64_MAX);
  EXPECT_EQ(spent.mem, INT64_MAX);

  // A budget at INT64_MAX spent against any finite initial is exhausted.
  cardano_uplc_budget_t initial = { 1000000, 1000000 };
  EXPECT_TRUE(cardano_uplc_step_accumulator_is_exhausted(&acc, initial));
}

TEST(cardano_uplc_cost_model, sentinel_step_flush_saturates)
{
  // V1 constr cost is the 30000000000 sentinel; a single constr step under V1,
  // multiplied through the flush, stays exact at that value.
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V1);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  ASSERT_EQ(cardano_uplc_step_accumulator_step(&acc, CARDANO_UPLC_STEP_KIND_CONSTR), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_step_accumulator_flush(&acc), CARDANO_SUCCESS);

  cardano_uplc_budget_t spent = cardano_uplc_step_accumulator_spent(&acc);
  EXPECT_EQ(spent.cpu, 30000000000);
  EXPECT_EQ(spent.mem, 30000000000);
}

TEST(cardano_uplc_cost_model, null_pointer_paths)
{
  cardano_uplc_machine_costs_t    costs = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  cardano_uplc_step_accumulator_t acc;
  ASSERT_EQ(cardano_uplc_step_accumulator_init(&acc, &costs), CARDANO_SUCCESS);

  cardano_uplc_budget_t delta = { 1, 1 };
  cardano_uplc_budget_t initial = { 10, 10 };

  EXPECT_EQ(cardano_uplc_step_accumulator_init(nullptr, &costs), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_step_accumulator_init(&acc, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_step_accumulator_charge_startup(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_step_accumulator_step(nullptr, CARDANO_UPLC_STEP_KIND_APPLY), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_step_accumulator_charge(nullptr, delta), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_step_accumulator_flush(nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_uplc_budget_t null_spent = cardano_uplc_step_accumulator_spent(nullptr);
  EXPECT_EQ(null_spent.cpu, 0);
  EXPECT_EQ(null_spent.mem, 0);

  cardano_uplc_budget_t null_remaining = cardano_uplc_step_accumulator_remaining(nullptr, initial);
  EXPECT_EQ(null_remaining.cpu, 10);
  EXPECT_EQ(null_remaining.mem, 10);

  EXPECT_FALSE(cardano_uplc_step_accumulator_is_exhausted(nullptr, initial));
}

/* COSTING-FUNCTION SHAPE TESTS **********************************************/

static const int64_t I64_MAX = INT64_MAX;
static const int64_t I64_MIN = INT64_MIN;

TEST(cardano_uplc_cost_sat, add_no_overflow)
{
  EXPECT_EQ(cardano_uplc_cost_sat_add(3, 4), 7);
  EXPECT_EQ(cardano_uplc_cost_sat_add(-5, 2), -3);
  EXPECT_EQ(cardano_uplc_cost_sat_add(0, 0), 0);
}

TEST(cardano_uplc_cost_sat, add_saturates_high)
{
  EXPECT_EQ(cardano_uplc_cost_sat_add(I64_MAX, 1), I64_MAX);
  EXPECT_EQ(cardano_uplc_cost_sat_add(I64_MAX, I64_MAX), I64_MAX);
  EXPECT_EQ(cardano_uplc_cost_sat_add(I64_MAX - 2, 2), I64_MAX);
}

TEST(cardano_uplc_cost_sat, add_saturates_low)
{
  EXPECT_EQ(cardano_uplc_cost_sat_add(I64_MIN, -1), I64_MIN);
  EXPECT_EQ(cardano_uplc_cost_sat_add(I64_MIN, I64_MIN), I64_MIN);
  EXPECT_EQ(cardano_uplc_cost_sat_add(I64_MIN + 2, -2), I64_MIN);
}

TEST(cardano_uplc_cost_sat, mul_no_overflow)
{
  EXPECT_EQ(cardano_uplc_cost_sat_mul(6, 7), 42);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(-6, 7), -42);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(6, -7), -42);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(-6, -7), 42);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(0, I64_MAX), 0);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(I64_MAX, 0), 0);
}

TEST(cardano_uplc_cost_sat, mul_saturates_high)
{
  EXPECT_EQ(cardano_uplc_cost_sat_mul(I64_MAX, 2), I64_MAX);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(2, I64_MAX), I64_MAX);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(I64_MIN, -1), I64_MAX);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(-1, I64_MIN), I64_MAX);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(I64_MIN, -2), I64_MAX);
}

TEST(cardano_uplc_cost_sat, mul_saturates_low)
{
  EXPECT_EQ(cardano_uplc_cost_sat_mul(I64_MAX, -2), I64_MIN);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(-2, I64_MAX), I64_MIN);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(I64_MIN, 2), I64_MIN);
  EXPECT_EQ(cardano_uplc_cost_sat_mul(2, I64_MIN), I64_MIN);
}

TEST(cardano_uplc_one_arg_cost, constant)
{
  cardano_uplc_one_arg_cost_t fn;
  fn.kind            = CARDANO_UPLC_ONE_ARG_CONSTANT;
  fn.params.constant = 42;

  EXPECT_EQ(cardano_uplc_one_arg_cost_eval(&fn, 100), 42);
  EXPECT_EQ(cardano_uplc_one_arg_cost_eval(&fn, 0), 42);
}

TEST(cardano_uplc_one_arg_cost, linear)
{
  cardano_uplc_one_arg_cost_t fn;
  fn.kind                   = CARDANO_UPLC_ONE_ARG_LINEAR;
  fn.params.linear.intercept = 10;
  fn.params.linear.slope     = 3;

  // 10 + 3*3 = 19
  EXPECT_EQ(cardano_uplc_one_arg_cost_eval(&fn, 3), 19);
}

TEST(cardano_uplc_one_arg_cost, quadratic)
{
  cardano_uplc_one_arg_cost_t fn;
  fn.kind                     = CARDANO_UPLC_ONE_ARG_QUADRATIC;
  fn.params.quadratic.coeff_0 = 100;
  fn.params.quadratic.coeff_1 = 10;
  fn.params.quadratic.coeff_2 = 1;

  // 100 + 10*5 + 1*25 = 175
  EXPECT_EQ(cardano_uplc_one_arg_cost_eval(&fn, 5), 175);
}

TEST(cardano_uplc_one_arg_cost, linear_saturates)
{
  cardano_uplc_one_arg_cost_t fn;
  fn.kind                    = CARDANO_UPLC_ONE_ARG_LINEAR;
  fn.params.linear.intercept = 1;
  fn.params.linear.slope     = I64_MAX;

  EXPECT_EQ(cardano_uplc_one_arg_cost_eval(&fn, 2), I64_MAX);
}

TEST(cardano_uplc_one_arg_cost, null_returns_zero)
{
  EXPECT_EQ(cardano_uplc_one_arg_cost_eval(nullptr, 5), 0);
}

static cardano_uplc_two_arg_cost_t
make_two(cardano_uplc_two_arg_kind_t kind)
{
  cardano_uplc_two_arg_cost_t fn;
  memset(&fn, 0, sizeof(fn));
  fn.kind = kind;
  return fn;
}

TEST(cardano_uplc_two_arg_cost, constant)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_CONSTANT);
  fn.params.constant             = 7;

  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 9), 7);
}

TEST(cardano_uplc_two_arg_cost, linear_in_x)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X);
  fn.params.linear.intercept     = 100;
  fn.params.linear.slope         = 2;

  // 100 + 2*5 = 110, ignores y
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 9999), 110);
}

TEST(cardano_uplc_two_arg_cost, linear_in_y)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y);
  fn.params.linear.intercept     = 100;
  fn.params.linear.slope         = 2;

  // 100 + 2*9 = 118, ignores x
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 9999, 9), 118);
}

TEST(cardano_uplc_two_arg_cost, linear_in_x_and_y)
{
  cardano_uplc_two_arg_cost_t fn       = make_two(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X_AND_Y);
  fn.params.linear_in_x_and_y.intercept = 618401;
  fn.params.linear_in_x_and_y.slope1    = 1998;
  fn.params.linear_in_x_and_y.slope2    = 28258;

  // 618401 + 1998*3 + 28258*5 = 618401 + 5994 + 141290 = 765685
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 5), 765685);
}

TEST(cardano_uplc_two_arg_cost, with_interaction)
{
  cardano_uplc_two_arg_cost_t fn   = make_two(CARDANO_UPLC_TWO_ARG_WITH_INTERACTION);
  fn.params.with_interaction.c00   = 1000;
  fn.params.with_interaction.c10   = 172116;
  fn.params.with_interaction.c01   = 183150;
  fn.params.with_interaction.c11   = 6;

  // 1000 + 172116*2 + 183150*3 + 6*2*3 = 1000 + 344232 + 549450 + 36 = 894718
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 2, 3), 894718);
}

TEST(cardano_uplc_two_arg_cost, added_sizes)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_ADDED_SIZES);
  fn.params.linear.intercept     = 1000;
  fn.params.linear.slope         = 173;

  // 1000 + 173*(3+5) = 1000 + 1384 = 2384
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 5), 2384);
}

TEST(cardano_uplc_two_arg_cost, subtracted_sizes_above_minimum)
{
  cardano_uplc_two_arg_cost_t fn          = make_two(CARDANO_UPLC_TWO_ARG_SUBTRACTED_SIZES);
  fn.params.subtracted_sizes.intercept    = 0;
  fn.params.subtracted_sizes.slope        = 1;
  fn.params.subtracted_sizes.minimum      = 0;

  // max(0, 10-5)=5, 0 + 1*5 = 5
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 10, 5), 5);
}

TEST(cardano_uplc_two_arg_cost, subtracted_sizes_floored_by_minimum)
{
  cardano_uplc_two_arg_cost_t fn          = make_two(CARDANO_UPLC_TWO_ARG_SUBTRACTED_SIZES);
  fn.params.subtracted_sizes.intercept    = 0;
  fn.params.subtracted_sizes.slope        = 1;
  fn.params.subtracted_sizes.minimum      = 1;

  // max(1, 5-10)=1, 0 + 1*1 = 1
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 10), 1);
}

TEST(cardano_uplc_two_arg_cost, multiplied_sizes)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_MULTIPLIED_SIZES);
  fn.params.linear.intercept     = 90434;
  fn.params.linear.slope         = 519;

  // 90434 + 519*(3*5) = 90434 + 7785 = 98219
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 5), 98219);
}

TEST(cardano_uplc_two_arg_cost, min_size)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_MIN_SIZE);
  fn.params.linear.intercept     = 51775;
  fn.params.linear.slope         = 558;

  // 51775 + 558*min(7,3) = 51775 + 1674 = 53449
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 7, 3), 53449);
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 7), 53449);
}

TEST(cardano_uplc_two_arg_cost, max_size)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_MAX_SIZE);
  fn.params.linear.intercept     = 100788;
  fn.params.linear.slope         = 420;

  // 100788 + 420*max(7,3) = 100788 + 2940 = 103728
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 7, 3), 103728);
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 7), 103728);
}

TEST(cardano_uplc_two_arg_cost, linear_on_diagonal)
{
  cardano_uplc_two_arg_cost_t fn             = make_two(CARDANO_UPLC_TWO_ARG_LINEAR_ON_DIAGONAL);
  fn.params.linear_on_diagonal.constant      = 24548;
  fn.params.linear_on_diagonal.intercept     = 29498;
  fn.params.linear_on_diagonal.slope         = 38;

  // x==y: 29498 + 38*5 = 29688
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 5), 29688);
  // x!=y: constant
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 6), 24548);
}

TEST(cardano_uplc_two_arg_cost, const_above_diagonal_linear_model)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL);
  fn.params.const_diagonal.constant                  = 213283;
  fn.params.const_diagonal.kind                      = CARDANO_UPLC_DIAG_MODEL_LINEAR_IN_X_AND_Y;
  fn.params.const_diagonal.model.linear_in_x_and_y.intercept = 618401;
  fn.params.const_diagonal.model.linear_in_x_and_y.slope1    = 1998;
  fn.params.const_diagonal.model.linear_in_x_and_y.slope2    = 28258;

  // x<y: constant
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 5), 213283);
  // x>=y: 618401 + 1998*5 + 28258*3 = 618401 + 9990 + 84774 = 713165
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 3), 713165);
}

TEST(cardano_uplc_two_arg_cost, const_below_diagonal_multiplied_model)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_CONST_BELOW_DIAGONAL);
  fn.params.const_diagonal.constant                = 196500;
  fn.params.const_diagonal.kind                    = CARDANO_UPLC_DIAG_MODEL_MULTIPLIED_SIZES;
  fn.params.const_diagonal.model.multiplied_sizes.intercept = 453240;
  fn.params.const_diagonal.model.multiplied_sizes.slope     = 220;

  // x>y: constant
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 7, 3), 196500);
  // x<=y: 453240 + 220*(3*5) = 453240 + 3300 = 456540
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 5), 456540);
}

TEST(cardano_uplc_two_arg_cost, above_and_below_diagonal_quadratic_model)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_ABOVE_AND_BELOW_DIAGONAL);
  fn.params.const_diagonal.constant            = 0;
  fn.params.const_diagonal.kind                = CARDANO_UPLC_DIAG_MODEL_QUADRATIC;
  fn.params.const_diagonal.model.quadratic.minimum = 0;
  fn.params.const_diagonal.model.quadratic.coeff_00 = 1;
  fn.params.const_diagonal.model.quadratic.coeff_10 = 2;
  fn.params.const_diagonal.model.quadratic.coeff_01 = 3;
  fn.params.const_diagonal.model.quadratic.coeff_20 = 0;
  fn.params.const_diagonal.model.quadratic.coeff_11 = 0;
  fn.params.const_diagonal.model.quadratic.coeff_02 = 0;

  // evaluated at (max,min): x=3,y=5 -> (5,3): 1 + 2*5 + 3*3 = 1 + 10 + 9 = 20
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 5), 20);
  // x=5,y=3 -> (5,3): same 20
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 3), 20);
}

TEST(cardano_uplc_two_arg_cost, quadratic_in_y)
{
  cardano_uplc_two_arg_cost_t fn   = make_two(CARDANO_UPLC_TWO_ARG_QUADRATIC_IN_Y);
  fn.params.quadratic_in_y.coeff_0 = 1006041;
  fn.params.quadratic_in_y.coeff_1 = 43623;
  fn.params.quadratic_in_y.coeff_2 = 251;

  // y=2: 1006041 + 43623*2 + 251*4 = 1006041 + 87246 + 1004 = 1094291
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 0, 2), 1094291);
}

TEST(cardano_uplc_two_arg_cost, quadratic_in_x_and_y_floored)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_QUADRATIC_IN_X_AND_Y);
  fn.params.quadratic_in_x_and_y.minimum  = 1000000;
  fn.params.quadratic_in_x_and_y.coeff_00 = 1;
  fn.params.quadratic_in_x_and_y.coeff_10 = 2;
  fn.params.quadratic_in_x_and_y.coeff_01 = 3;
  fn.params.quadratic_in_x_and_y.coeff_20 = 4;
  fn.params.quadratic_in_x_and_y.coeff_11 = 5;
  fn.params.quadratic_in_x_and_y.coeff_02 = 6;

  // raw = 1 + 2*2 + 3*3 + 4*4 + 5*6 + 6*9 = 1+4+9+16+30+54 = 114; floored to minimum 1000000
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 2, 3), 1000000);
}

TEST(cardano_uplc_two_arg_cost, const_above_diagonal_into_quadratic)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL_INTO_QUADRATIC);
  fn.params.const_above_into_quadratic.constant            = 85848;
  fn.params.const_above_into_quadratic.quadratic.minimum   = 85848;
  fn.params.const_above_into_quadratic.quadratic.coeff_00  = 123203;
  fn.params.const_above_into_quadratic.quadratic.coeff_10  = 1716;
  fn.params.const_above_into_quadratic.quadratic.coeff_01  = 7305;
  fn.params.const_above_into_quadratic.quadratic.coeff_20  = 57;
  fn.params.const_above_into_quadratic.quadratic.coeff_11  = 549;
  fn.params.const_above_into_quadratic.quadratic.coeff_02  = -900;

  // x<y: constant
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 3, 5), 85848);
  // x>=y: 123203 + 1716*5 + 7305*3 + 57*25 + 549*15 + (-900)*9
  //     = 123203 + 8580 + 21915 + 1425 + 8235 - 8100 = 155258
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 5, 3), 155258);
}

TEST(cardano_uplc_two_arg_cost, drop_list)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_DROP_LIST);
  fn.params.linear.intercept     = 116711;
  fn.params.linear.slope         = 1957;

  // returns intercept only, regardless of sizes
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, 4, 9), 116711);
}

TEST(cardano_uplc_two_arg_cost, saturates)
{
  cardano_uplc_two_arg_cost_t fn = make_two(CARDANO_UPLC_TWO_ARG_MULTIPLIED_SIZES);
  fn.params.linear.intercept     = 1;
  fn.params.linear.slope         = 1;

  // 1 + 1*(I64_MAX * 2 -> sat I64_MAX) -> sat I64_MAX
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(&fn, I64_MAX, 2), I64_MAX);
}

TEST(cardano_uplc_two_arg_cost, null_returns_zero)
{
  EXPECT_EQ(cardano_uplc_two_arg_cost_eval(nullptr, 1, 2), 0);
}

static cardano_uplc_three_arg_cost_t
make_three(cardano_uplc_three_arg_kind_t kind)
{
  cardano_uplc_three_arg_cost_t fn;
  memset(&fn, 0, sizeof(fn));
  fn.kind = kind;
  return fn;
}

TEST(cardano_uplc_three_arg_cost, constant)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_CONSTANT);
  fn.params.constant               = 132994;

  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 1, 2, 3), 132994);
}

TEST(cardano_uplc_three_arg_cost, added_sizes)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_ADDED_SIZES);
  fn.params.linear.intercept       = 100;
  fn.params.linear.slope           = 2;

  // (1+2+3)*2 + 100 = 12 + 100 = 112
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 1, 2, 3), 112);
}

TEST(cardano_uplc_three_arg_cost, linear_in_x)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_LINEAR_IN_X);
  fn.params.linear.intercept       = 0;
  fn.params.linear.slope           = 1;

  // 1*7 + 0 = 7, ignores y,z
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 7, 99, 99), 7);
}

TEST(cardano_uplc_three_arg_cost, linear_in_y)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y);
  fn.params.linear.intercept       = 281145;
  fn.params.linear.slope           = 18848;

  // 281145 + 18848*2 = 281145 + 37696 = 318841
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 2, 99), 318841);
}

TEST(cardano_uplc_three_arg_cost, linear_in_z)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Z);
  fn.params.linear.intercept       = 219951;
  fn.params.linear.slope           = 9444;

  // 219951 + 9444*4 = 219951 + 37776 = 257727
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 99, 4), 257727);
}

TEST(cardano_uplc_three_arg_cost, quadratic_in_z)
{
  cardano_uplc_three_arg_cost_t fn   = make_three(CARDANO_UPLC_THREE_ARG_QUADRATIC_IN_Z);
  fn.params.quadratic_in_z.coeff_0   = 1293828;
  fn.params.quadratic_in_z.coeff_1   = 28716;
  fn.params.quadratic_in_z.coeff_2   = 63;

  // z=3: 1293828 + 28716*3 + 63*9 = 1293828 + 86148 + 567 = 1380543
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 99, 3), 1380543);
}

TEST(cardano_uplc_three_arg_cost, exp_mod_x_le_z)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_EXP_MOD);
  fn.params.exp_mod.coeff_00       = 607153;
  fn.params.exp_mod.coeff_11       = 231697;
  fn.params.exp_mod.coeff_12       = 53144;

  // y=2,z=3: base = 607153 + 231697*(2*3) + 53144*(2*3*3)
  //               = 607153 + 1390182 + 956592 = 2953927
  // x<=z -> base
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 1, 2, 3), 2953927);
}

TEST(cardano_uplc_three_arg_cost, exp_mod_x_gt_z)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_EXP_MOD);
  fn.params.exp_mod.coeff_00       = 607153;
  fn.params.exp_mod.coeff_11       = 231697;
  fn.params.exp_mod.coeff_12       = 53144;

  // base = 2953927; x>z -> base + base/2 = 2953927 + 1476963 = 4430890
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 10, 2, 3), 4430890);
}

TEST(cardano_uplc_three_arg_cost, literal_in_y_or_linear_in_z)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_LITERAL_IN_Y_OR_LINEAR_IN_Z);
  fn.params.linear.intercept       = 0;
  fn.params.linear.slope           = 1;

  // y==0 -> 0 + 1*z = z
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 0, 7), 7);
  // y!=0 -> y
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 42, 7), 42);
}

TEST(cardano_uplc_three_arg_cost, linear_in_max_yz)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_LINEAR_IN_MAX_YZ);
  fn.params.linear.intercept       = 0;
  fn.params.linear.slope           = 1;

  // max(2,9) = 9
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 2, 9), 9);
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 9, 2), 9);
}

TEST(cardano_uplc_three_arg_cost, linear_in_y_and_z)
{
  cardano_uplc_three_arg_cost_t fn        = make_three(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y_AND_Z);
  fn.params.linear_in_y_and_z.intercept   = 100181;
  fn.params.linear_in_y_and_z.slope1      = 726;
  fn.params.linear_in_y_and_z.slope2      = 719;

  // 100181 + 726*2 + 719*3 = 100181 + 1452 + 2157 = 103790
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 99, 2, 3), 103790);
}

TEST(cardano_uplc_three_arg_cost, exp_mod_saturates)
{
  cardano_uplc_three_arg_cost_t fn = make_three(CARDANO_UPLC_THREE_ARG_EXP_MOD);
  fn.params.exp_mod.coeff_00       = 0;
  fn.params.exp_mod.coeff_11       = I64_MAX;
  fn.params.exp_mod.coeff_12       = 0;

  // y*z = 4, coeff_11 * 4 saturates to I64_MAX
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(&fn, 1, 2, 2), I64_MAX);
}

TEST(cardano_uplc_three_arg_cost, null_returns_zero)
{
  EXPECT_EQ(cardano_uplc_three_arg_cost_eval(nullptr, 1, 2, 3), 0);
}

TEST(cardano_uplc_four_arg_cost, constant)
{
  cardano_uplc_four_arg_cost_t fn;
  memset(&fn, 0, sizeof(fn));
  fn.kind            = CARDANO_UPLC_FOUR_ARG_CONSTANT;
  fn.params.constant = 1000000;

  EXPECT_EQ(cardano_uplc_four_arg_cost_eval(&fn, 1, 2, 3, 4), 1000000);
}

TEST(cardano_uplc_four_arg_cost, linear_in_u)
{
  cardano_uplc_four_arg_cost_t fn;
  memset(&fn, 0, sizeof(fn));
  fn.kind                        = CARDANO_UPLC_FOUR_ARG_LINEAR_IN_U;
  fn.params.linear_in_u.intercept = 356924;
  fn.params.linear_in_u.slope     = 18413;

  // 356924 + 18413*5 = 356924 + 92065 = 448989, ignores x,y,z
  EXPECT_EQ(cardano_uplc_four_arg_cost_eval(&fn, 99, 99, 99, 5), 448989);
}

TEST(cardano_uplc_four_arg_cost, null_returns_zero)
{
  EXPECT_EQ(cardano_uplc_four_arg_cost_eval(nullptr, 1, 2, 3, 4), 0);
}

TEST(cardano_uplc_six_arg_cost, constant)
{
  cardano_uplc_six_arg_cost_t fn;
  memset(&fn, 0, sizeof(fn));
  fn.kind            = CARDANO_UPLC_SIX_ARG_CONSTANT;
  fn.params.constant = 254006273;

  EXPECT_EQ(cardano_uplc_six_arg_cost_eval(&fn, 1, 2, 3, 4, 5, 6), 254006273);
}

TEST(cardano_uplc_six_arg_cost, null_returns_zero)
{
  EXPECT_EQ(cardano_uplc_six_arg_cost_eval(nullptr, 1, 2, 3, 4, 5, 6), 0);
}

/* EX-MEM SIZING ************************************************************/

static cardano_bigint_t*
big_from_string(const char* s)
{
  cardano_bigint_t* bigint = nullptr;
  EXPECT_EQ(cardano_bigint_from_string(s, strlen(s), 10, &bigint), CARDANO_SUCCESS);
  return bigint;
}

static const cardano_uplc_constant_t*
new_int_const(cardano_uplc_arena_t* arena, const char* decimal)
{
  cardano_bigint_t*        bigint   = big_from_string(decimal);
  cardano_uplc_constant_t* constant = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, bigint, &constant), CARDANO_SUCCESS);
  cardano_bigint_unref(&bigint);
  return constant;
}

static const cardano_uplc_constant_t*
new_bytes_const(cardano_uplc_arena_t* arena, size_t len)
{
  std::vector<uint8_t> bytes(len, 0xABU);
  cardano_buffer_t*    buffer   = cardano_buffer_new_from(bytes.data(), len);
  cardano_uplc_constant_t* constant = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_byte_string(arena, buffer, &constant), CARDANO_SUCCESS);
  cardano_buffer_unref(&buffer);
  return constant;
}

static const cardano_uplc_constant_t*
new_string_const(cardano_uplc_arena_t* arena, const char* utf8)
{
  size_t                   len      = strlen(utf8);
  cardano_buffer_t*        buffer   = cardano_buffer_new_from(reinterpret_cast<const uint8_t*>(utf8), len);
  cardano_uplc_constant_t* constant = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_string(arena, buffer, &constant), CARDANO_SUCCESS);
  cardano_buffer_unref(&buffer);
  return constant;
}

static int64_t
const_ex_mem(cardano_uplc_arena_t* arena, const cardano_uplc_constant_t* c, bool utf8)
{
  (void)arena;
  return cardano_uplc_constant_ex_mem(c, utf8);
}

TEST(cardano_uplc_integer_ex_mem, reference_values)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "0"), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "1"), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "42"), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "9223372036854775808"), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "18446744073709551615"), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "18446744073709551616"), false), 2);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "999999999999999999999999999999"), false), 2);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "170141183460469231731687303715884105726"), false), 2);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "170141183460469231731687303715884105728"), false), 2);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "340282366920938463463374607431768211458"), false), 3);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "999999999999999999999999999999999999999999"), false), 3);
  EXPECT_EQ(
    cardano_uplc_constant_ex_mem(
      new_int_const(arena, "999999999999999999999999999999999999999999999999999999999999999999999999999999999999"),
      false),
    5);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_integer_ex_mem, negatives_use_abs)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "-1"), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "-18446744073709551616"), false), 2);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_int_const(arena, "-340282366920938463463374607431768211458"), false), 3);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_integer_ex_mem, null_is_one)
{
  EXPECT_EQ(cardano_uplc_integer_ex_mem(nullptr), 1);
}

TEST(cardano_uplc_byte_string_ex_mem, reference_values)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_bytes_const(arena, 1U), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_bytes_const(arena, 8U), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_bytes_const(arena, 9U), false), 2);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_bytes_const(arena, 16U), false), 2);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_bytes_const(arena, 17U), false), 3);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_byte_string_ex_mem, direct)
{
  EXPECT_EQ(cardano_uplc_byte_string_ex_mem(0U), 1);
  EXPECT_EQ(cardano_uplc_byte_string_ex_mem(8U), 1);
  EXPECT_EQ(cardano_uplc_byte_string_ex_mem(9U), 2);
}

TEST(cardano_uplc_string_ex_mem, char_count_semantics)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  // "abcd": 4 ascii code points -> 4 under char count.
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_string_const(arena, "abcd"), false), 4);
  // "e" with combining acute is 2 utf-8 bytes but 1 code point -> 1 under char count.
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_string_const(arena, "\xC3\xA9"), false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_string_const(arena, ""), false), 0);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_string_ex_mem, utf8_byte_semantics)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  // "abcd": 4 bytes -> (4-1)/4+1 = 1.
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_string_const(arena, "abcd"), true), 1);
  // 2 bytes -> (2-1)/4+1 = 1.
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_string_const(arena, "\xC3\xA9"), true), 1);
  // empty -> 0.
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_string_const(arena, ""), true), 0);
  // 9 bytes -> (9-1)/4+1 = 3.
  EXPECT_EQ(cardano_uplc_constant_ex_mem(new_string_const(arena, "123456789"), true), 3);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_string_ex_mem, direct_null_bytes)
{
  EXPECT_EQ(cardano_uplc_string_ex_mem(nullptr, 0U, false), 0);
  EXPECT_EQ(cardano_uplc_string_ex_mem(nullptr, 0U, true), 0);
}

TEST(cardano_uplc_constant_ex_mem, bool_and_unit)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_constant_t* b = nullptr;
  cardano_uplc_constant_t* u = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_bool(arena, true, &b), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_constant_new_unit(arena, &u), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_constant_ex_mem(b, false), 1);
  EXPECT_EQ(cardano_uplc_constant_ex_mem(u, false), 1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_constant_ex_mem, null_is_zero)
{
  EXPECT_EQ(cardano_uplc_constant_ex_mem(nullptr, false), 0);
}

TEST(cardano_uplc_constant_ex_mem, nested_pair_and_list_matches_aiken)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(8192U, &arena), CARDANO_SUCCESS);

  // Mirrors the aiken to_ex_mem_counts_nested_constants_iteratively test:
  // Pair(Integer(2^64), List[String("abcd"), String("e-acute"), ByteString(9 bytes)]).
  cardano_uplc_type_t* int_type = nullptr;
  cardano_uplc_type_t* str_type = nullptr;
  ASSERT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &int_type), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_STRING, nullptr, nullptr, &str_type), CARDANO_SUCCESS);

  const cardano_uplc_constant_t* fst = new_int_const(arena, "18446744073709551616");

  const cardano_uplc_constant_t* items[3];
  items[0] = new_string_const(arena, "abcd");
  items[1] = new_string_const(arena, "\xC3\xA9");
  items[2] = new_bytes_const(arena, 9U);

  cardano_uplc_constant_t* list = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_list(arena, str_type, items, 3U, &list), CARDANO_SUCCESS);

  cardano_uplc_constant_t* pair = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_pair(arena, fst, list, &pair), CARDANO_SUCCESS);

  // C semantics (char count): 2 + (4 + 1 + 2) = 9.
  EXPECT_EQ(const_ex_mem(arena, pair, false), 9);
  // D semantics (utf-8 bytes): 2 + (1 + 1 + 2) = 6.
  EXPECT_EQ(const_ex_mem(arena, pair, true), 6);

  cardano_uplc_arena_free(&arena);
}

static cardano_plutus_data_t*
data_int(int64_t v)
{
  cardano_plutus_data_t* d = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(v, &d), CARDANO_SUCCESS);
  return d;
}

static cardano_plutus_data_t*
data_int_str(const char* decimal)
{
  cardano_bigint_t*      bigint = big_from_string(decimal);
  cardano_plutus_data_t* d      = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer(bigint, &d), CARDANO_SUCCESS);
  cardano_bigint_unref(&bigint);
  return d;
}

static cardano_plutus_data_t*
data_bytes(size_t len)
{
  std::vector<uint8_t>   bytes(len, 0x11U);
  cardano_plutus_data_t* d = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_bytes(bytes.data(), len, &d), CARDANO_SUCCESS);
  return d;
}

static int64_t
data_ex_mem_of(const cardano_plutus_data_t* data)
{
  cardano_uplc_arena_t* arena = nullptr;
  cardano_uplc_data_t*  node  = nullptr;
  int64_t               total = 0;

  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  if (data == nullptr)
  {
    total = cardano_uplc_data_ex_mem(nullptr);
  }
  else
  {
    EXPECT_EQ(cardano_uplc_data_from_plutus_data(arena, data, &node), CARDANO_SUCCESS);
    total = cardano_uplc_data_ex_mem(node);
  }

  cardano_uplc_arena_free(&arena);

  return total;
}

TEST(cardano_uplc_data_ex_mem, constr_with_int_and_bytes)
{
  // Constr[Integer(2^64)=2, Bytes(9)=2]: 4 + (4+2) + (4+2) = 16.
  cardano_plutus_list_t* fields = nullptr;
  ASSERT_EQ(cardano_plutus_list_new(&fields), CARDANO_SUCCESS);

  cardano_plutus_data_t* f0 = data_int_str("18446744073709551616");
  cardano_plutus_data_t* f1 = data_bytes(9U);
  ASSERT_EQ(cardano_plutus_list_add(fields, f0), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_add(fields, f1), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&f0);
  cardano_plutus_data_unref(&f1);

  cardano_constr_plutus_data_t* constr = nullptr;
  ASSERT_EQ(cardano_constr_plutus_data_new(0U, fields, &constr), CARDANO_SUCCESS);
  cardano_plutus_list_unref(&fields);

  cardano_plutus_data_t* data = nullptr;
  ASSERT_EQ(cardano_plutus_data_new_constr(constr, &data), CARDANO_SUCCESS);
  cardano_constr_plutus_data_unref(&constr);

  EXPECT_EQ(data_ex_mem_of(data), 16);

  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_data_ex_mem, map_with_one_pair)
{
  // Map[(Integer(1)=1) -> (Bytes(8)=1)]: 4 + (4+1) + (4+1) = 14.
  cardano_plutus_map_t* map = nullptr;
  ASSERT_EQ(cardano_plutus_map_new(&map), CARDANO_SUCCESS);

  cardano_plutus_data_t* key   = data_int(1);
  cardano_plutus_data_t* value = data_bytes(8U);
  ASSERT_EQ(cardano_plutus_map_insert(map, key, value), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&value);

  cardano_plutus_data_t* data = nullptr;
  ASSERT_EQ(cardano_plutus_data_new_map(map, &data), CARDANO_SUCCESS);
  cardano_plutus_map_unref(&map);

  EXPECT_EQ(data_ex_mem_of(data), 14);

  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_data_ex_mem, list_of_two_integers)
{
  // List[Integer(1)=1, Integer(2^64)=2]: 4 + (4+1) + (4+2) = 15.
  cardano_plutus_list_t* list = nullptr;
  ASSERT_EQ(cardano_plutus_list_new(&list), CARDANO_SUCCESS);

  cardano_plutus_data_t* e0 = data_int(1);
  cardano_plutus_data_t* e1 = data_int_str("18446744073709551616");
  ASSERT_EQ(cardano_plutus_list_add(list, e0), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_add(list, e1), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&e0);
  cardano_plutus_data_unref(&e1);

  cardano_plutus_data_t* data = nullptr;
  ASSERT_EQ(cardano_plutus_data_new_list(list, &data), CARDANO_SUCCESS);
  cardano_plutus_list_unref(&list);

  EXPECT_EQ(data_ex_mem_of(data), 15);

  cardano_plutus_data_unref(&data);
}

TEST(cardano_uplc_data_ex_mem, single_leaf_and_null)
{
  cardano_plutus_data_t* i = data_int(0);
  // single Integer(0) leaf: 4 + 1 = 5.
  EXPECT_EQ(data_ex_mem_of(i), 5);
  cardano_plutus_data_unref(&i);

  cardano_plutus_data_t* b = data_bytes(1U);
  // single 1-byte Bytes leaf: 4 + 1 = 5.
  EXPECT_EQ(data_ex_mem_of(b), 5);
  cardano_plutus_data_unref(&b);

  EXPECT_EQ(data_ex_mem_of(nullptr), 0);
}

TEST(cardano_uplc_constant_ex_mem, data_constant)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_plutus_data_t*   data     = data_int(0);
  cardano_uplc_constant_t* constant = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_data(arena, data, &constant), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&data);

  EXPECT_EQ(cardano_uplc_constant_ex_mem(constant, false), 5);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_ex_mem, constant_value_delegates)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  const cardano_uplc_constant_t* c     = new_int_const(arena, "18446744073709551616");
  cardano_uplc_value_t*          value = nullptr;
  ASSERT_EQ(cardano_uplc_value_new_constant(arena, c, &value), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_value_ex_mem(value, false), 2);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_ex_mem, delay_lambda_builtin_constr_are_one)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_term_t* body = nullptr;
  ASSERT_EQ(cardano_uplc_term_new_error(arena, &body), CARDANO_SUCCESS);

  cardano_uplc_value_t* delay   = nullptr;
  cardano_uplc_value_t* lambda  = nullptr;
  cardano_uplc_value_t* builtin = nullptr;
  cardano_uplc_value_t* constr  = nullptr;
  ASSERT_EQ(cardano_uplc_value_new_delay(arena, body, nullptr, &delay), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_value_new_lambda(arena, body, nullptr, &lambda), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_value_new_builtin(arena, CARDANO_UPLC_BUILTIN_ADD_INTEGER, 0U, nullptr, 0U, &builtin), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_value_new_constr(arena, 0U, nullptr, 0U, &constr), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_uplc_value_ex_mem(delay, false), 1);
  EXPECT_EQ(cardano_uplc_value_ex_mem(lambda, false), 1);
  EXPECT_EQ(cardano_uplc_value_ex_mem(builtin, false), 1);
  EXPECT_EQ(cardano_uplc_value_ex_mem(constr, false), 1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_value_ex_mem, null_is_zero)
{
  EXPECT_EQ(cardano_uplc_value_ex_mem(nullptr, false), 0);
}

/* BUILTIN COSTS ************************************************************/

// The mainnet 2024-09-29 V1 flat parameter vector that must reproduce
// cardano_uplc_builtin_costs_v1, transcribed from aiken cost_model.rs.
static const int64_t V1_DEFAULT_PARAMS[] = {
  100788, 420, 1, 1, 1000, 173, 0, 1, 1000, 59957, 4, 1, 11183, 32, 201305, 8356, 4,
  16000, 100, 16000, 100, 16000, 100, 16000, 100, 16000, 100, 16000, 100, 100, 100,
  16000, 100, 94375, 32, 132994, 32, 61462, 4, 72010, 178, 0, 1, 22151, 32, 91189, 769,
  4, 2, 85848, 228465, 122, 0, 1, 1, 1000, 42921, 4, 2, 24548, 29498, 38, 1, 898148,
  27279, 1, 51775, 558, 1, 39184, 1000, 60594, 1, 141895, 32, 83150, 32, 15299, 32,
  76049, 1, 13169, 4, 22100, 10, 28999, 74, 1, 28999, 74, 1, 43285, 552, 1, 44749, 541,
  1, 33852, 32, 68246, 32, 72362, 32, 7243, 32, 7391, 32, 11546, 32, 85848, 228465, 122,
  0, 1, 1, 90434, 519, 0, 1, 74433, 32, 85848, 228465, 122, 0, 1, 1, 85848, 228465, 122,
  0, 1, 1, 270652, 22588, 4, 1457325, 64566, 4, 20467, 1, 4, 0, 141992, 32, 100788, 420,
  1, 1, 81663, 32, 59498, 32, 20142, 32, 24588, 32, 20744, 32, 25933, 32, 24623, 32,
  53384111, 14333, 10
};

// The mainnet 2024-09-29 V2 flat parameter vector that must reproduce
// cardano_uplc_builtin_costs_v2.
static const int64_t V2_DEFAULT_PARAMS[] = {
  100788, 420, 1, 1, 1000, 173, 0, 1, 1000, 59957, 4, 1, 11183, 32, 201305, 8356, 4,
  16000, 100, 16000, 100, 16000, 100, 16000, 100, 16000, 100, 16000, 100, 100, 100,
  16000, 100, 94375, 32, 132994, 32, 61462, 4, 72010, 178, 0, 1, 22151, 32, 91189, 769,
  4, 2, 85848, 228465, 122, 0, 1, 1, 1000, 42921, 4, 2, 24548, 29498, 38, 1, 898148,
  27279, 1, 51775, 558, 1, 39184, 1000, 60594, 1, 141895, 32, 83150, 32, 15299, 32,
  76049, 1, 13169, 4, 22100, 10, 28999, 74, 1, 28999, 74, 1, 43285, 552, 1, 44749, 541,
  1, 33852, 32, 68246, 32, 72362, 32, 7243, 32, 7391, 32, 11546, 32, 85848, 228465, 122,
  0, 1, 1, 90434, 519, 0, 1, 74433, 32, 85848, 228465, 122, 0, 1, 1, 85848, 228465, 122,
  0, 1, 1, 955506, 213312, 0, 2, 270652, 22588, 4, 1457325, 64566, 4, 20467, 1, 4, 0,
  141992, 32, 100788, 420, 1, 1, 81663, 32, 59498, 32, 20142, 32, 24588, 32, 20744, 32,
  25933, 32, 24623, 32, 43053543, 10, 53384111, 14333, 10, 43574283, 26308, 10
};

// The preprod 2024-11-22 V3 flat parameter vector that must reproduce
// cardano_uplc_builtin_costs_v3.
static const int64_t V3_DEFAULT_PARAMS[] = {
  100788, 420, 1, 1, 1000, 173, 0, 1, 1000, 59957, 4, 1, 11183, 32, 201305, 8356, 4,
  16000, 100, 16000, 100, 16000, 100, 16000, 100, 16000, 100, 16000, 100, 100, 100,
  16000, 100, 94375, 32, 132994, 32, 61462, 4, 72010, 178, 0, 1, 22151, 32, 91189, 769,
  4, 2, 85848, 123203, 7305, -900, 1716, 549, 57, 85848, 0, 1, 1, 1000, 42921, 4, 2,
  24548, 29498, 38, 1, 898148, 27279, 1, 51775, 558, 1, 39184, 1000, 60594, 1, 141895,
  32, 83150, 32, 15299, 32, 76049, 1, 13169, 4, 22100, 10, 28999, 74, 1, 28999, 74, 1,
  43285, 552, 1, 44749, 541, 1, 33852, 32, 68246, 32, 72362, 32, 7243, 32, 7391, 32,
  11546, 32, 85848, 123203, 7305, -900, 1716, 549, 57, 85848, 0, 1, 90434, 519, 0, 1,
  74433, 32, 85848, 123203, 7305, -900, 1716, 549, 57, 85848, 0, 1, 1, 85848, 123203,
  7305, -900, 1716, 549, 57, 85848, 0, 1, 955506, 213312, 0, 2, 270652, 22588, 4,
  1457325, 64566, 4, 20467, 1, 4, 0, 141992, 32, 100788, 420, 1, 1, 81663, 32, 59498, 32,
  20142, 32, 24588, 32, 20744, 32, 25933, 32, 24623, 32, 43053543, 10, 53384111, 14333,
  10, 43574283, 26308, 10, 16000, 100, 16000, 100, 962335, 18, 2780678, 6, 442008, 1,
  52538055, 3756, 18, 267929, 18, 76433006, 8868, 18, 52948122, 18, 1995836, 36, 3227919,
  12, 901022, 1, 166917843, 4307, 36, 284546, 36, 158221314, 26549, 36, 74698472, 36,
  333849714, 1, 254006273, 72, 2174038, 72, 2261318, 64571, 4, 207616, 8310, 4, 1293828,
  28716, 63, 0, 1, 1006041, 43623, 251, 0, 1, 100181, 726, 719, 0, 1, 100181, 726, 719,
  0, 1, 100181, 726, 719, 0, 1, 107878, 680, 0, 1, 95336, 1, 281145, 18848, 0, 1, 180194,
  159, 1, 1, 158519, 8942, 0, 1, 159378, 8813, 0, 1, 107490, 3298, 1, 106057, 655, 1,
  1964219, 24520, 3
};

// Builds a constant integer value in an arena, for builtin-cost dispatch tests.
static cardano_uplc_value_t*
make_int_value(cardano_uplc_arena_t* arena, int64_t n)
{
  cardano_bigint_t* big = nullptr;
  EXPECT_EQ(cardano_bigint_from_int(n, &big), CARDANO_SUCCESS);

  cardano_uplc_constant_t* constant = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, big, &constant), CARDANO_SUCCESS);
  cardano_bigint_unref(&big);

  cardano_uplc_value_t* value = nullptr;
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

// Builds a constant byte-string value of the given length in an arena.
static cardano_uplc_value_t*
make_bytes_value(cardano_uplc_arena_t* arena, size_t length)
{
  cardano_buffer_t* buffer = cardano_buffer_new(length == 0U ? 1U : length);
  EXPECT_NE(buffer, nullptr);

  for (size_t i = 0U; i < length; ++i)
  {
    byte_t b = (byte_t)(i & 0xFFU);
    EXPECT_EQ(cardano_buffer_write(buffer, &b, 1U), CARDANO_SUCCESS);
  }

  cardano_uplc_constant_t* constant = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_byte_string(arena, buffer, &constant), CARDANO_SUCCESS);
  cardano_buffer_unref(&buffer);

  cardano_uplc_value_t* value = nullptr;
  EXPECT_EQ(cardano_uplc_value_new_constant(arena, constant, &value), CARDANO_SUCCESS);

  return value;
}

TEST(cardano_uplc_cost_model_from_params, v1_round_trip_reproduces_default)
{
  cardano_uplc_cost_model_t model;
  cardano_uplc_builtin_costs_t expected = cardano_uplc_builtin_costs_v1();

  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(
      CARDANO_UPLC_COST_MODEL_VERSION_V1,
      V1_DEFAULT_PARAMS,
      CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1,
      &model),
    CARDANO_SUCCESS);

  EXPECT_EQ(std::memcmp(&model.builtins, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_cost_model_from_params, v2_round_trip_reproduces_default)
{
  cardano_uplc_cost_model_t model;
  cardano_uplc_builtin_costs_t expected = cardano_uplc_builtin_costs_v2();

  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(
      CARDANO_UPLC_COST_MODEL_VERSION_V2,
      V2_DEFAULT_PARAMS,
      CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V2,
      &model),
    CARDANO_SUCCESS);

  EXPECT_EQ(std::memcmp(&model.builtins, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_cost_model_from_params, v3_round_trip_reproduces_default)
{
  cardano_uplc_cost_model_t model;
  cardano_uplc_builtin_costs_t expected = cardano_uplc_builtin_costs_v3();

  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(
      CARDANO_UPLC_COST_MODEL_VERSION_V3,
      V3_DEFAULT_PARAMS,
      CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
      &model),
    CARDANO_SUCCESS);

  EXPECT_EQ(std::memcmp(&model.builtins, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_cost_model_from_params, v3_machine_costs_match_default)
{
  cardano_uplc_cost_model_t model;

  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(
      CARDANO_UPLC_COST_MODEL_VERSION_V3,
      V3_DEFAULT_PARAMS,
      CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
      &model),
    CARDANO_SUCCESS);

  cardano_uplc_machine_costs_t expected = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);

  EXPECT_EQ(std::memcmp(&model.machine, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_cost_model_from_params, wrong_count_is_error)
{
  cardano_uplc_cost_model_t model;

  EXPECT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V1, V1_DEFAULT_PARAMS, 10U, &model),
    CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V2, V2_DEFAULT_PARAMS, 10U, &model),
    CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V3, V3_DEFAULT_PARAMS, 10U, &model),
    CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_uplc_cost_model_from_params, null_pointers_are_error)
{
  cardano_uplc_cost_model_t model;

  EXPECT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V1, nullptr, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, &model),
    CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V1, V1_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_cost_model_from_params, unknown_version_is_error)
{
  cardano_uplc_cost_model_t model;

  EXPECT_EQ(
    cardano_uplc_cost_model_from_params((cardano_uplc_cost_model_version_t)99, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3, &model),
    CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_uplc_builtin_cost, v3_add_integer_small_args)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  const cardano_uplc_value_t* args[2];
  args[0] = make_int_value(arena, 1);
  args[1] = make_int_value(arena, 2);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_ADD_INTEGER, args, 2U, false);

  // add_integer V3: cpu MaxSize(100788, 420).cost(1,1) = 100788 + 420*max(1,1).
  // mem MaxSize(1, 1).cost(1,1) = 1 + 1*1.
  EXPECT_EQ(budget.cpu, 100788 + 420);
  EXPECT_EQ(budget.mem, 1 + 1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, v3_sha2_256_over_bytes)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // A 16-byte string has ex-mem ((16 - 1) / 8) + 1 = 2.
  const cardano_uplc_value_t* args[1];
  args[0] = make_bytes_value(arena, 16U);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_SHA2_256, args, 1U, false);

  // sha2_256 V3: cpu LinearCost(270652, 22588).cost(2) = 270652 + 22588*2.
  // mem ConstantCost(4).
  EXPECT_EQ(budget.cpu, 270652 + (22588 * 2));
  EXPECT_EQ(budget.mem, 4);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, v3_cons_byte_string_linear_in_y)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // cons_byte_string(byte, bytestring): arg0 ex-mem 1, arg1 32-byte ex-mem 4.
  const cardano_uplc_value_t* args[2];
  args[0] = make_int_value(arena, 65);
  args[1] = make_bytes_value(arena, 32U);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING, args, 2U, false);

  // cpu LinearInY(72010, 178).cost(_, 4) = 72010 + 178*4.
  // mem AddedSizes(0, 1).cost(1, 4) = 0 + 1*(1 + 4).
  EXPECT_EQ(budget.cpu, 72010 + (178 * 4));
  EXPECT_EQ(budget.mem, 1 + 4);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, v3_slice_byte_string_three_args)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // slice_byte_string(start, len, bytestring): only arg2 size matters (LinearInZ).
  const cardano_uplc_value_t* args[3];
  args[0] = make_int_value(arena, 0);
  args[1] = make_int_value(arena, 8);
  args[2] = make_bytes_value(arena, 24U); // ex-mem ((24 - 1) / 8) + 1 = 3.

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING, args, 3U, false);

  // cpu LinearInZ(20467, 1).cost(_, _, 3) = 20467 + 1*3.
  // mem LinearInZ(4, 0).cost(_, _, 3) = 4 + 0*3.
  EXPECT_EQ(budget.cpu, 20467 + 3);
  EXPECT_EQ(budget.mem, 4);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, replicate_byte_uses_literal_width)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // replicate_byte(count, byte): arg0 fed as cost_as_size(count), not its ex-mem.
  // count 16 -> size ((16 - 1) / 8) + 1 = 2.
  const cardano_uplc_value_t* args[2];
  args[0] = make_int_value(arena, 16);
  args[1] = make_int_value(arena, 255);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_REPLICATE_BYTE, args, 2U, false);

  // cpu LinearInX(180194, 159).cost(2, _) = 180194 + 159*2.
  // mem LinearInX(1, 1).cost(2, _) = 1 + 1*2.
  EXPECT_EQ(budget.cpu, 180194 + (159 * 2));
  EXPECT_EQ(budget.mem, 1 + 2);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, shift_byte_string_uses_abs_literal)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // shift_byte_string(bytestring, shift): arg1 fed as abs(shift), mem LinearInX over arg0.
  const cardano_uplc_value_t* args[2];
  args[0] = make_bytes_value(arena, 8U); // ex-mem 1
  args[1] = make_int_value(arena, -5);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING, args, 2U, false);

  // cpu LinearInX(158519, 8942).cost(1, _) = 158519 + 8942*1.
  // mem LinearInX(0, 1).cost(1, _) = 0 + 1*1.
  EXPECT_EQ(budget.cpu, 158519 + 8942);
  EXPECT_EQ(budget.mem, 1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, v1_serialise_data_is_unavailable_sentinel)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v1();

  const cardano_uplc_value_t* args[1];
  args[0] = make_bytes_value(arena, 8U);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_SERIALISE_DATA, args, 1U, false);

  EXPECT_EQ(budget.cpu, 30000000000);
  EXPECT_EQ(budget.mem, 30000000000);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, v3_exp_mod_costs_exp_mod_shape)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // exp_mod_integer(base, exponent, modulus): each one-word integer has ex-mem 1,
  // so x = y = z = 1 and the base branch (x <= z) applies.
  const cardano_uplc_value_t* args[3];
  args[0] = make_int_value(arena, 2);
  args[1] = make_int_value(arena, 3);
  args[2] = make_int_value(arena, 5);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER, args, 3U, false);

  // cpu ExpMod(607153, 231697, 53144): yz = 1, base = 607153 + 231697 + 53144.
  // mem LinearInZ(0, 1).cost(z = 1) = 1.
  EXPECT_EQ(budget.cpu, 607153 + 231697 + 53144);
  EXPECT_EQ(budget.mem, 1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, null_costs_is_zero_budget)
{
  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(nullptr, CARDANO_UPLC_BUILTIN_ADD_INTEGER, nullptr, 0U, false);
  EXPECT_EQ(budget.cpu, 0);
  EXPECT_EQ(budget.mem, 0);
}

TEST(cardano_uplc_builtin_cost, out_of_range_builtin_is_zero_budget)
{
  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();
  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, (cardano_uplc_builtin_t)CARDANO_UPLC_BUILTIN_COUNT, nullptr, 0U, false);
  EXPECT_EQ(budget.cpu, 0);
  EXPECT_EQ(budget.mem, 0);
}

TEST(cardano_uplc_builtin_cost, v3_choose_data_six_args)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  const cardano_uplc_value_t* args[6];
  for (size_t i = 0U; i < 6U; ++i)
  {
    args[i] = make_int_value(arena, (int64_t)i);
  }

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_CHOOSE_DATA, args, 6U, false);

  EXPECT_EQ(budget.cpu, 94375);
  EXPECT_EQ(budget.mem, 32);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, write_bits_uses_list_length)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // write_bits(bytestring, indexList, bit): arg1 fed as the list length.
  // A non-list arg1 yields length 0, so cpu LinearInY(281145, 18848).cost(_, 0, _).
  const cardano_uplc_value_t* args[3];
  args[0] = make_bytes_value(arena, 8U);
  args[1] = make_int_value(arena, 0);
  args[2] = make_int_value(arena, 1);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_WRITE_BITS, args, 3U, false);

  // cpu 281145 + 18848*0. mem LinearInX(0, 1).cost(1, _, _) = 0 + 1*1.
  EXPECT_EQ(budget.cpu, 281145);
  EXPECT_EQ(budget.mem, 1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, write_bits_real_list_length)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  cardano_uplc_type_t* int_type = nullptr;
  ASSERT_EQ(cardano_uplc_type_new(arena, CARDANO_UPLC_TYPE_INTEGER, nullptr, nullptr, &int_type), CARDANO_SUCCESS);

  const cardano_uplc_constant_t** items =
    (const cardano_uplc_constant_t**)cardano_uplc_arena_alloc(arena, sizeof(const cardano_uplc_constant_t*) * 3U, 8U);
  ASSERT_NE(items, nullptr);

  for (size_t i = 0U; i < 3U; ++i)
  {
    cardano_bigint_t* big = nullptr;
    ASSERT_EQ(cardano_bigint_from_int((int64_t)i, &big), CARDANO_SUCCESS);
    cardano_uplc_constant_t* item = nullptr;
    ASSERT_EQ(cardano_uplc_constant_new_integer(arena, big, &item), CARDANO_SUCCESS);
    cardano_bigint_unref(&big);
    items[i] = item;
  }

  cardano_uplc_constant_t* list = nullptr;
  ASSERT_EQ(cardano_uplc_constant_new_list(arena, int_type, items, 3U, &list), CARDANO_SUCCESS);

  cardano_uplc_value_t* list_value = nullptr;
  ASSERT_EQ(cardano_uplc_value_new_constant(arena, list, &list_value), CARDANO_SUCCESS);

  const cardano_uplc_value_t* args[3];
  args[0] = make_bytes_value(arena, 8U);
  args[1] = list_value;
  args[2] = make_int_value(arena, 1);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_WRITE_BITS, args, 3U, false);

  // cpu LinearInY(281145, 18848).cost(_, 3, _) = 281145 + 18848*3.
  EXPECT_EQ(budget.cpu, 281145 + (18848 * 3));
  EXPECT_EQ(budget.mem, 1);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, v3_insert_coin_arity_four_linear_in_value_depth)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // insertCoin costs cpu/mem linearly in the value-max-depth of the value arg
  // (args[3]). With non-value placeholder args the depth is 0, so cpu and mem
  // collapse to their intercepts: cpu 356924, mem 45.
  const cardano_uplc_value_t* args[4];
  for (size_t i = 0U; i < 4U; ++i)
  {
    args[i] = make_int_value(arena, (int64_t)i);
  }

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_INSERT_COIN, args, 4U, false);

  EXPECT_EQ(budget.cpu, 356924);
  EXPECT_EQ(budget.mem, 45);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_builtin_cost, integer_to_byte_string_literal_width)
{
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_builtin_costs_t costs = cardano_uplc_builtin_costs_v3();

  // integer_to_byte_string(endianness, width, value): arg1 fed as cost_as_size(width).
  // width 0 -> size 0. cpu QuadraticInZ over arg2 ex-mem; mem LiteralInYorLinearInZ.
  const cardano_uplc_value_t* args[3];
  args[0] = make_int_value(arena, 0);
  args[1] = make_int_value(arena, 0);
  args[2] = make_int_value(arena, 7);

  cardano_uplc_budget_t budget = cardano_uplc_builtin_cost(&costs, CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING, args, 3U, false);

  // arg2 = 7 has ex-mem 1 (single 64-bit word). cpu 1293828 + 28716*1 + 63*1*1.
  // mem LiteralInYorLinearInZ: y (= size 0) so LinearInZ(0, 1).cost(1) = 0 + 1*1.
  EXPECT_EQ(budget.cpu, 1293828 + 28716 + 63);
  EXPECT_EQ(budget.mem, 1);

  cardano_uplc_arena_free(&arena);
}

/* BUILTIN SEMANTICS ********************************************************/

TEST(cardano_uplc_builtin_semantics_for_language_and_protocol, v1_boundaries)
{
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V1, 8U), CARDANO_UPLC_SEMANTICS_A);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V1, 9U), CARDANO_UPLC_SEMANTICS_B);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V1, 10U), CARDANO_UPLC_SEMANTICS_B);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V1, 11U), CARDANO_UPLC_SEMANTICS_D);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V1, 12U), CARDANO_UPLC_SEMANTICS_D);
}

TEST(cardano_uplc_builtin_semantics_for_language_and_protocol, v2_boundaries)
{
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V2, 8U), CARDANO_UPLC_SEMANTICS_A);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V2, 9U), CARDANO_UPLC_SEMANTICS_B);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V2, 10U), CARDANO_UPLC_SEMANTICS_B);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V2, 11U), CARDANO_UPLC_SEMANTICS_D);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V2, 12U), CARDANO_UPLC_SEMANTICS_D);
}

TEST(cardano_uplc_builtin_semantics_for_language_and_protocol, v3_boundaries)
{
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V3, 0U), CARDANO_UPLC_SEMANTICS_C);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V3, 9U), CARDANO_UPLC_SEMANTICS_C);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V3, 10U), CARDANO_UPLC_SEMANTICS_C);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V3, 11U), CARDANO_UPLC_SEMANTICS_E);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V3, 12U), CARDANO_UPLC_SEMANTICS_E);
}

TEST(cardano_uplc_builtin_semantics_for_language_and_protocol, v4_follows_v3)
{
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V4, 10U), CARDANO_UPLC_SEMANTICS_C);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language_and_protocol(CARDANO_UPLC_LANG_VERSION_V4, 11U), CARDANO_UPLC_SEMANTICS_E);
}

TEST(cardano_uplc_builtin_semantics_for_language, no_protocol)
{
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language(CARDANO_UPLC_LANG_VERSION_V1), CARDANO_UPLC_SEMANTICS_B);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language(CARDANO_UPLC_LANG_VERSION_V2), CARDANO_UPLC_SEMANTICS_B);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language(CARDANO_UPLC_LANG_VERSION_V3), CARDANO_UPLC_SEMANTICS_C);
  EXPECT_EQ(cardano_uplc_builtin_semantics_for_language(CARDANO_UPLC_LANG_VERSION_V4), CARDANO_UPLC_SEMANTICS_C);
}

TEST(cardano_uplc_semantics_costs_strings_by_utf8_bytes, per_variant)
{
  EXPECT_FALSE(cardano_uplc_semantics_costs_strings_by_utf8_bytes(CARDANO_UPLC_SEMANTICS_A));
  EXPECT_FALSE(cardano_uplc_semantics_costs_strings_by_utf8_bytes(CARDANO_UPLC_SEMANTICS_B));
  EXPECT_FALSE(cardano_uplc_semantics_costs_strings_by_utf8_bytes(CARDANO_UPLC_SEMANTICS_C));
  EXPECT_TRUE(cardano_uplc_semantics_costs_strings_by_utf8_bytes(CARDANO_UPLC_SEMANTICS_D));
  EXPECT_TRUE(cardano_uplc_semantics_costs_strings_by_utf8_bytes(CARDANO_UPLC_SEMANTICS_E));
}

TEST(cardano_uplc_semantics_cons_byte_string_range_checks, per_variant)
{
  EXPECT_FALSE(cardano_uplc_semantics_cons_byte_string_range_checks(CARDANO_UPLC_SEMANTICS_A));
  EXPECT_FALSE(cardano_uplc_semantics_cons_byte_string_range_checks(CARDANO_UPLC_SEMANTICS_B));
  EXPECT_TRUE(cardano_uplc_semantics_cons_byte_string_range_checks(CARDANO_UPLC_SEMANTICS_C));
  EXPECT_FALSE(cardano_uplc_semantics_cons_byte_string_range_checks(CARDANO_UPLC_SEMANTICS_D));
  EXPECT_TRUE(cardano_uplc_semantics_cons_byte_string_range_checks(CARDANO_UPLC_SEMANTICS_E));
}
