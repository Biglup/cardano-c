/**
 * \file uplc_machine.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_MACHINE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_MACHINE_H

/* INCLUDES ******************************************************************/

#include "../arena/uplc_arena.h"
#include "../ast/uplc_program.h"
#include "../ast/uplc_term.h"
#include "../builtins/uplc_builtin_semantics.h"
#include "../cost/uplc_cost_model.h"
#include "uplc_budget.h"
#include "uplc_eval_result.h"
#include "uplc_eval_status.h"
#include "uplc_machine_version.h"
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Evaluates a decoded UPLC program on the CEK machine.
 *
 * Runs the Compute/Return/Done transition machine over \p program->term,
 * charging the default machine-step costs for \p version with slippage batching,
 * and reports the outcome through \p out.
 *
 * The loop is iterative, not host-recursive, so a deeply nested term cannot
 * overflow the C stack. Builtin bodies are implemented in later milestones: a
 * builtin that saturates but whose body is not implemented yet ends the
 * evaluation with \ref CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN rather than
 * producing a wrong value, so a program that relies on such a builtin is visibly
 * distinguishable from a script that genuinely failed.
 *
 * Ownership: \p program and its whole term tree are borrowed and must stay valid
 * for the call; they live in \p arena, the arena they were decoded into. The
 * machine allocates every interior value, environment cell and continuation frame
 * in \p arena, and allocates the discharged result term in \p arena as well, so
 * the result outlives the call exactly as long as \p arena does. The machine does
 * not create or free an arena of its own; the caller owns \p arena and releases it
 * with \ref cardano_uplc_arena_free when done with the result.
 *
 * Host failure versus script outcome: a \ref cardano_error_t other than
 * \ref CARDANO_SUCCESS means the host could not run the script (a null argument,
 * or an arena allocation failure). A script that fails or runs out of budget is a
 * \ref CARDANO_SUCCESS return whose \p out carries the
 * \ref CARDANO_UPLC_EVAL_ERROR_TERM or \ref CARDANO_UPLC_EVAL_OUT_OF_BUDGET status.
 *
 * \param[in] arena The arena that owns \p program and receives the result term.
 *            Must not be NULL.
 * \param[in] program The decoded program to evaluate. Must not be NULL and must
 *            live in \p arena.
 * \param[in] version The language version selecting the default step costs.
 * \param[in] initial_budget The CPU and memory ceiling for the evaluation.
 * \param[out] out On a \ref CARDANO_SUCCESS return, the script outcome, spent
 *             budget and result term; left untouched on a \ref cardano_error_t
 *             failure.
 *
 * \return \ref CARDANO_SUCCESS when the host ran the script (whatever the script
 *         outcome), \ref CARDANO_ERROR_POINTER_IS_NULL if \p arena, \p program,
 *         \p program->term, or \p out is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if \p arena cannot serve an
 *         interior allocation.
 */
cardano_error_t
cardano_uplc_evaluate(
  struct cardano_uplc_arena_t*   arena,
  const cardano_uplc_program_t*  program,
  cardano_uplc_machine_version_t version,
  cardano_uplc_budget_t          initial_budget,
  cardano_uplc_eval_result_t*    out);

/* INTERNAL DECLARATIONS *****************************************************/

/* The following entry point is module-internal (the cardano_uplc_int_ prefix
 * marks it so). It is the cost-model-aware core of the CEK evaluator and is not
 * part of the public API. */

/**
 * \brief Evaluates a decoded UPLC program with a caller-provided cost model.
 *
 * The cost-model-aware counterpart of the public \ref cardano_uplc_evaluate. It
 * takes a structured \p cost_model and a builtin \p semantics variant directly
 * rather than deriving the per-version defaults, so a caller can charge the real
 * ledger cost model the transaction was submitted under. The step loop reads
 * \p cost_model.machine through the slippage accumulator and the saturated
 * builtin path reads \p cost_model.builtins and \p semantics. The public
 * \ref cardano_uplc_evaluate is a thin wrapper that supplies the version default
 * model and semantics.
 *
 * Ownership and outcome semantics match \ref cardano_uplc_evaluate exactly: the
 * program and result live in \p arena, a script outcome is reported through
 * \p out on a \ref CARDANO_SUCCESS return, and a \ref cardano_error_t marks a host
 * failure.
 *
 * \param[in] arena The arena that owns \p program and receives the result term.
 *            Must not be NULL.
 * \param[in] program The decoded program to evaluate. Must not be NULL and must
 *            live in \p arena.
 * \param[in] cost_model The full cost model to charge against. Must not be NULL.
 * \param[in] semantics The builtin semantics variant driving the version-dependent
 *            builtin behaviours.
 * \param[in] initial_budget The CPU and memory ceiling for the evaluation.
 * \param[out] out On a \ref CARDANO_SUCCESS return, the script outcome, spent
 *             budget and result term.
 *
 * \return \ref CARDANO_SUCCESS when the host ran the script, or a
 *         \ref cardano_error_t host failure.
 */
cardano_error_t
cardano_uplc_int_evaluate_with_costs(
  struct cardano_uplc_arena_t*     arena,
  const cardano_uplc_program_t*    program,
  const cardano_uplc_cost_model_t* cost_model,
  cardano_uplc_builtin_semantics_t semantics,
  cardano_uplc_budget_t            initial_budget,
  cardano_uplc_eval_result_t*      out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_MACHINE_H */
