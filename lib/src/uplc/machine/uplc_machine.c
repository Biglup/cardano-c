/**
 * \file uplc_machine.c
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

#include "../arena/uplc_arena.h"
#include "../ast/uplc_int.h"
#include "../builtins/builtins.h"
#include "../builtins/uplc_builtin.h"
#include "../builtins/uplc_builtin_semantics.h"
#include "../cost/uplc_builtin_costs.h"
#include "../cost/uplc_cost_model.h"
#include "../cost/uplc_machine_costs.h"
#include "../cost/uplc_step_accumulator.h"

#include "uplc_env.h"
#include "uplc_frame.h"
#include "uplc_frame_kind.h"
#include "uplc_machine.h"
#include "uplc_state_kind.h"
#include "uplc_value.h"
#include "uplc_value_kind.h"

#include <stddef.h>

/* MACHINE LOOP **************************************************************/

/**
 * \brief The classification of a single CEK transition.
 *
 * A transition either advances the machine, finishes it, or hits a normal Plutus
 * failure (the error term, a bad application, an out-of-range index, a missing
 * branch, or an unimplemented builtin) reported as a script error. Host failures
 * (allocation) are reported separately through a \ref cardano_error_t out-param,
 * not through this enum.
 */
typedef enum
{
  /** \brief The state was advanced; keep looping. */
  PRV_STEP_CONTINUE = 0,
  /** \brief The machine reached Done; the result value is ready to discharge. */
  PRV_STEP_DONE = 1,
  /** \brief The script failed; the outcome is the error term. */
  PRV_STEP_SCRIPT_ERROR = 2,
  /**
   * \brief A saturated builtin this VM build does not implement was reached.
   *
   * Distinct from \ref PRV_STEP_SCRIPT_ERROR: the script did not fail, the VM
   * just does not run that builtin.
   */
  PRV_STEP_UNSUPPORTED_BUILTIN = 3
} step_outcome_t;

/**
 * \brief A CEK machine state: a Compute, a Return, or a Done.
 *
 * Compute carries the term, its environment and the continuation; Return carries
 * the value and the continuation; Done carries only the final value. The active
 * fields are selected by \c kind.
 */
typedef struct
{
    cardano_uplc_state_kind_t   kind;
    const cardano_uplc_frame_t* ctx;
    const cardano_uplc_env_t*   env;
    const cardano_uplc_term_t*  term;
    const cardano_uplc_value_t* value;
} state_t;

/**
 * \brief The mutable context threaded through every transition.
 *
 * Holds the arena every interior node is allocated from, the step/budget
 * accumulator, and the initial budget used to test exhaustion. It also carries
 * the full cost model (machine-step costs plus the per-builtin costing functions)
 * and the builtin semantics variant; the step loop reads its machine-step costs
 * through the accumulator, and the saturated-builtin path reads \c cost_model.builtins
 * and \c semantics to charge builtin costs.
 */
typedef struct
{
    cardano_uplc_arena_t*            arena;
    cardano_uplc_step_accumulator_t  acc;
    cardano_uplc_budget_t            initial;
    cardano_uplc_cost_model_t        cost_model;
    cardano_uplc_builtin_semantics_t semantics;
    cardano_uplc_lang_version_t      language;
    uint64_t                         protocol_major;
} machine_t;

/**
 * \brief Charges one machine step and, on host failure, reports it.
 *
 * \param[in,out] machine The machine whose accumulator is charged.
 * \param[in] kind The step kind to charge.
 *
 * \return \ref CARDANO_SUCCESS on success, propagating any accumulator failure.
 */
static cardano_error_t
charge_step(machine_t* machine, cardano_uplc_step_kind_t kind)
{
  return cardano_uplc_step_accumulator_step(&machine->acc, kind);
}

/**
 * \brief Computes a term in an environment under a continuation.
 *
 * Implements the Compute half of the CEK transition for every term form, charging
 * the matching step and producing the next state.
 *
 * \param[in,out] machine The machine context.
 * \param[in] ctx The continuation the result returns into.
 * \param[in] env The environment the term is computed in.
 * \param[in] term The term to compute.
 * \param[out] next On \ref PRV_STEP_CONTINUE, the next state.
 * \param[out] host_error On a host failure, the error code; \ref CARDANO_SUCCESS
 *             otherwise.
 *
 * \return The transition outcome.
 */
static step_outcome_t
run_compute_state(
  machine_t*                  machine,
  const cardano_uplc_frame_t* ctx,
  const cardano_uplc_env_t*   env,
  const cardano_uplc_term_t*  term,
  state_t*                    next,
  cardano_error_t*            host_error)
{
  *host_error = CARDANO_SUCCESS;

  switch (term->kind)
  {
    case CARDANO_UPLC_TERM_VAR:
    {
      const cardano_uplc_value_t* value = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_VAR);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      if (cardano_uplc_env_lookup(env, term->as.var_index, &value) != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind  = CARDANO_UPLC_STATE_RETURN;
      next->ctx   = ctx;
      next->value = value;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_TERM_DELAY:
    {
      cardano_uplc_value_t* value = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_DELAY);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = cardano_uplc_value_new_delay(machine->arena, term->as.unary, env, &value);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind  = CARDANO_UPLC_STATE_RETURN;
      next->ctx   = ctx;
      next->value = value;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_TERM_LAMBDA:
    {
      cardano_uplc_value_t* value = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_LAMBDA);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = cardano_uplc_value_new_lambda(machine->arena, term->as.unary, env, &value);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind  = CARDANO_UPLC_STATE_RETURN;
      next->ctx   = ctx;
      next->value = value;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_TERM_APPLY:
    {
      cardano_uplc_frame_t* frame = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_APPLY);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = cardano_uplc_frame_new_await_fun_term(machine->arena, env, term->as.apply.argument, ctx, &frame);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind = CARDANO_UPLC_STATE_COMPUTE;
      next->ctx  = frame;
      next->env  = env;
      next->term = term->as.apply.function;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_TERM_CONSTANT:
    {
      cardano_uplc_value_t* value = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_CONSTANT);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = cardano_uplc_value_new_constant(machine->arena, term->as.constant, &value);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind  = CARDANO_UPLC_STATE_RETURN;
      next->ctx   = ctx;
      next->value = value;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_TERM_FORCE:
    {
      cardano_uplc_frame_t* frame = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_FORCE);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = cardano_uplc_frame_new_force(machine->arena, ctx, &frame);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind = CARDANO_UPLC_STATE_COMPUTE;
      next->ctx  = frame;
      next->env  = env;
      next->term = term->as.unary;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_TERM_ERROR:
    {
      return PRV_STEP_SCRIPT_ERROR;
    }

    case CARDANO_UPLC_TERM_BUILTIN:
    {
      cardano_uplc_value_t* value = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_BUILTIN);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = cardano_uplc_value_new_builtin(machine->arena, term->as.builtin, (size_t)0, NULL, (size_t)0, &value);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind  = CARDANO_UPLC_STATE_RETURN;
      next->ctx   = ctx;
      next->value = value;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_TERM_CONSTR:
    {
      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_CONSTR);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      if (term->as.constr.field_count == (size_t)0)
      {
        cardano_uplc_value_t* value = NULL;

        *host_error = cardano_uplc_value_new_constr(machine->arena, term->as.constr.tag, NULL, (size_t)0, &value);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        next->kind  = CARDANO_UPLC_STATE_RETURN;
        next->ctx   = ctx;
        next->value = value;

        return PRV_STEP_CONTINUE;
      }
      else
      {
        cardano_uplc_frame_t* frame = NULL;

        *host_error = cardano_uplc_frame_new_constr(
          machine->arena,
          env,
          term->as.constr.tag,
          &term->as.constr.fields[1],
          term->as.constr.field_count - (size_t)1,
          NULL,
          (size_t)0,
          ctx,
          &frame);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        next->kind = CARDANO_UPLC_STATE_COMPUTE;
        next->ctx  = frame;
        next->env  = env;
        next->term = term->as.constr.fields[0];

        return PRV_STEP_CONTINUE;
      }

      break;
    }

    case CARDANO_UPLC_TERM_CASE:
    {
      cardano_uplc_frame_t* frame = NULL;

      *host_error = charge_step(machine, CARDANO_UPLC_STEP_KIND_CASE);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = cardano_uplc_frame_new_cases(
        machine->arena,
        env,
        term->as.cases.branches,
        term->as.cases.branch_count,
        ctx,
        &frame);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind = CARDANO_UPLC_STATE_COMPUTE;
      next->ctx  = frame;
      next->env  = env;
      next->term = term->as.cases.scrutinee;

      return PRV_STEP_CONTINUE;
    }

    default:
    {
      return PRV_STEP_SCRIPT_ERROR;
    }
  }
}

/**
 * \brief Appends one argument value to a builtin's argument array.
 *
 * Copies the existing argument pointers into a fresh arena array one slot longer
 * and writes \p arg at the end, leaving the source builtin value untouched.
 *
 * \param[in] arena The arena to allocate the new array from.
 * \param[in] builtin The builtin value whose arguments are extended.
 * \param[in] arg The argument value to append.
 * \param[out] out On success, the extended array; the new count is the builtin's
 *             count plus one.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the array.
 */
static cardano_error_t
extend_builtin_args(
  cardano_uplc_arena_t*       arena,
  const cardano_uplc_value_t* builtin,
  const cardano_uplc_value_t* arg,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_value_t* const** out)
{
  const cardano_uplc_value_t** args  = NULL;
  size_t                       count = builtin->as.builtin.arg_count;
  size_t                       i     = (size_t)0;

  args = (const cardano_uplc_value_t**)cardano_uplc_arena_alloc(
    arena,
    sizeof(const cardano_uplc_value_t*) * (count + (size_t)1),
    0U);

  if (args == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (i = (size_t)0; i < count; ++i)
  {
    args[i] = builtin->as.builtin.args[i];
  }

  args[count] = arg;

  *out = args;

  return CARDANO_SUCCESS;
}

/**
 * \brief Applies a forced or applied builtin, saturating it if it is ready.
 *
 * Builds the next builtin value from \p func, \p forces and \p args and, when it
 * has all its forces and arguments, runs it through the builtin runtime. A ready
 * builtin spends its cost before its body runs (cost-before-execute, handled by
 * \ref cardano_uplc_int_builtin_run using \c machine->cost_model.builtins and
 * \c machine->semantics); the body produces the resulting value, fails the script
 * on a Plutus error (type mismatch, etc.), or reports that this VM build does not
 * implement that builtin. An unsaturated builtin is returned as a value.
 *
 * \param[in,out] machine The machine context.
 * \param[in] func The builtin function tag.
 * \param[in] forces The forces accumulated so far.
 * \param[in] args The argument values accumulated so far.
 * \param[in] arg_count The number of arguments accumulated so far.
 * \param[in] arity The builtin's arity, supplied by the caller.
 * \param[in] force_count The builtin's force count, supplied by the caller.
 * \param[out] out On \ref PRV_STEP_CONTINUE, the resulting value (the partially
 *             applied builtin when unsaturated, or the builtin result when ready).
 * \param[out] host_error On a host failure, the error code.
 *
 * \return \ref PRV_STEP_CONTINUE when the builtin is unsaturated or ran to a
 *         result, \ref PRV_STEP_SCRIPT_ERROR when a ready builtin fails the script
 *         or on a host failure, or \ref PRV_STEP_UNSUPPORTED_BUILTIN when a ready
 *         builtin is not implemented.
 */
static step_outcome_t
try_call_builtin(
  machine_t*                         machine,
  cardano_uplc_builtin_t             func,
  size_t                             forces,
  const cardano_uplc_value_t* const* args,
  size_t                             arg_count,
  size_t                             arity,
  size_t                             force_count,
  const cardano_uplc_value_t**       out,
  cardano_error_t*                   host_error)
{
  cardano_uplc_value_t* value = NULL;

  *host_error = CARDANO_SUCCESS;

  if ((arg_count == arity) && (forces == force_count))
  {
    if (!cardano_uplc_builtin_available(func, machine->language, machine->protocol_major))
    {
      return PRV_STEP_UNSUPPORTED_BUILTIN;
    }

    const cardano_uplc_value_t*        result  = NULL;
    cardano_uplc_int_builtin_outcome_t outcome = cardano_uplc_int_builtin_run(
      machine->arena,
      &machine->acc,
      &machine->cost_model.builtins,
      machine->semantics,
      func,
      args,
      arg_count,
      &result,
      host_error);

    switch (outcome)
    {
      case CARDANO_UPLC_BUILTIN_OUTCOME_OK:
      {
        *out = result;
        return PRV_STEP_CONTINUE;
      }

      case CARDANO_UPLC_BUILTIN_OUTCOME_UNSUPPORTED:
      {
        return PRV_STEP_UNSUPPORTED_BUILTIN;
      }

      case CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR:
      default:
      {
        return PRV_STEP_SCRIPT_ERROR;
      }
    }
  }

  *host_error = cardano_uplc_value_new_builtin(machine->arena, func, forces, args, arg_count, &value);

  if (*host_error != CARDANO_SUCCESS)
  {
    return PRV_STEP_SCRIPT_ERROR;
  }

  *out = value;

  return PRV_STEP_CONTINUE;
}

/**
 * \brief Applies a function value to an argument value.
 *
 * A lambda extends its captured environment and computes its body; a builtin that
 * still expects a term argument accumulates the argument and saturates if ready;
 * any other value is a non-functional application and fails as a script error.
 *
 * \param[in,out] machine The machine context.
 * \param[in] ctx The continuation the application returns into.
 * \param[in] function The function value applied.
 * \param[in] arg The argument value applied.
 * \param[out] next On \ref PRV_STEP_CONTINUE, the next state.
 * \param[out] host_error On a host failure, the error code.
 *
 * \return The transition outcome.
 */
static step_outcome_t
apply_evaluate(
  machine_t*                  machine,
  const cardano_uplc_frame_t* ctx,
  const cardano_uplc_value_t* function,
  const cardano_uplc_value_t* arg,
  state_t*                    next,
  cardano_error_t*            host_error)
{
  *host_error = CARDANO_SUCCESS;

  switch (function->kind)
  {
    case CARDANO_UPLC_VALUE_LAMBDA:
    {
      const cardano_uplc_env_t* env = NULL;

      *host_error = cardano_uplc_env_extend(machine->arena, function->as.lambda.env, arg, &env);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind = CARDANO_UPLC_STATE_COMPUTE;
      next->ctx  = ctx;
      next->env  = env;
      next->term = function->as.lambda.body;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_VALUE_BUILTIN:
    {
      const cardano_uplc_value_t* const* args        = NULL;
      const cardano_uplc_value_t*        value       = NULL;
      size_t                             arity       = (size_t)0;
      size_t                             force_count = (size_t)0;

      if ((cardano_uplc_builtin_arity(function->as.builtin.func, &arity) != CARDANO_SUCCESS) || (cardano_uplc_builtin_force_count(function->as.builtin.func, &force_count) != CARDANO_SUCCESS))
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      if ((force_count <= function->as.builtin.forces) && (arity > function->as.builtin.arg_count))
      {
        *host_error = extend_builtin_args(machine->arena, function, arg, &args);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        step_outcome_t outcome = try_call_builtin(
          machine,
          function->as.builtin.func,
          function->as.builtin.forces,
          args,
          function->as.builtin.arg_count + (size_t)1,
          arity,
          force_count,
          &value,
          host_error);

        if (outcome != PRV_STEP_CONTINUE)
        {
          return outcome;
        }

        next->kind  = CARDANO_UPLC_STATE_RETURN;
        next->ctx   = ctx;
        next->value = value;

        return PRV_STEP_CONTINUE;
      }

      return PRV_STEP_SCRIPT_ERROR;
    }

    case CARDANO_UPLC_VALUE_CONSTANT:
    case CARDANO_UPLC_VALUE_DELAY:
    case CARDANO_UPLC_VALUE_CONSTR:
    default:
    {
      return PRV_STEP_SCRIPT_ERROR;
    }
  }
}

/**
 * \brief Forces a delayed value or a builtin awaiting a force.
 *
 * A delay computes its body in its captured environment; a builtin that still
 * expects a force consumes one and saturates if ready; any other value is a
 * non-polymorphic instantiation and fails as a script error.
 *
 * \param[in,out] machine The machine context.
 * \param[in] ctx The continuation the force returns into.
 * \param[in] value The value being forced.
 * \param[out] next On \ref PRV_STEP_CONTINUE, the next state.
 * \param[out] host_error On a host failure, the error code.
 *
 * \return The transition outcome.
 */
static step_outcome_t
force_evaluate(
  machine_t*                  machine,
  const cardano_uplc_frame_t* ctx,
  const cardano_uplc_value_t* value,
  state_t*                    next,
  cardano_error_t*            host_error)
{
  *host_error = CARDANO_SUCCESS;

  switch (value->kind)
  {
    case CARDANO_UPLC_VALUE_DELAY:
    {
      next->kind = CARDANO_UPLC_STATE_COMPUTE;
      next->ctx  = ctx;
      next->env  = value->as.delay.env;
      next->term = value->as.delay.body;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_VALUE_BUILTIN:
    {
      const cardano_uplc_value_t* resolved    = NULL;
      size_t                      arity       = (size_t)0;
      size_t                      force_count = (size_t)0;
      step_outcome_t              outcome;

      if ((cardano_uplc_builtin_arity(value->as.builtin.func, &arity) != CARDANO_SUCCESS) || (cardano_uplc_builtin_force_count(value->as.builtin.func, &force_count) != CARDANO_SUCCESS))
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      if (force_count > value->as.builtin.forces)
      {
        outcome = try_call_builtin(
          machine,
          value->as.builtin.func,
          value->as.builtin.forces + (size_t)1,
          value->as.builtin.args,
          value->as.builtin.arg_count,
          arity,
          force_count,
          &resolved,
          host_error);

        if (outcome != PRV_STEP_CONTINUE)
        {
          return outcome;
        }

        next->kind  = CARDANO_UPLC_STATE_RETURN;
        next->ctx   = ctx;
        next->value = resolved;

        return PRV_STEP_CONTINUE;
      }

      return PRV_STEP_SCRIPT_ERROR;
    }

    case CARDANO_UPLC_VALUE_CONSTANT:
    case CARDANO_UPLC_VALUE_LAMBDA:
    case CARDANO_UPLC_VALUE_CONSTR:
    default:
    {
      return PRV_STEP_SCRIPT_ERROR;
    }
  }
}

/**
 * \brief Pushes constructor fields onto the continuation as await-fun-value
 *        frames so a case branch is applied to them in order.
 *
 * Wraps each field, last first, in a \ref CARDANO_UPLC_FRAME_AWAIT_FUN_VALUE frame
 * so that when the branch value returns it is applied to field 0 first.
 *
 * \param[in,out] machine The machine context.
 * \param[in] fields The resolved constructor field values.
 * \param[in] field_count The number of fields.
 * \param[in] ctx The continuation to push the frames onto.
 * \param[out] out On success, the continuation with the field frames pushed.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve a
 *         frame.
 */
static cardano_error_t
transfer_arg_stack(
  machine_t*                         machine,
  const cardano_uplc_value_t* const* fields,
  size_t                             field_count,
  const cardano_uplc_frame_t*        ctx,
  const cardano_uplc_frame_t**       out)
{
  const cardano_uplc_frame_t* current = ctx;
  size_t                      i       = field_count;

  while (i > (size_t)0)
  {
    cardano_uplc_frame_t* frame = NULL;
    cardano_error_t       error;

    --i;

    error = cardano_uplc_frame_new_await_fun_value(machine->arena, fields[i], current, &frame);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }

    current = frame;
  }

  *out = current;

  return CARDANO_SUCCESS;
}

/**
 * \brief Resolves the constant-index of a non-negative in-range integer.
 *
 * Maps a constant integer to a branch index, rejecting any integer that is
 * negative or does not fit a \c size_t.
 *
 * \param[in] integer The integer constant to convert.
 * \param[out] out On success, the branch index.
 *
 * \return \ref CARDANO_SUCCESS when the integer is a valid index, or
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT when it is negative or too large.
 */
static cardano_error_t
constant_to_index(const cardano_bigint_t* integer, size_t* out)
{
  if (cardano_bigint_signum(integer) < 0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (cardano_bigint_bit_length(integer) > (sizeof(size_t) * (size_t)8))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *out = (size_t)cardano_bigint_to_unsigned_int(integer);

  return CARDANO_SUCCESS;
}

/**
 * \brief Wraps a constant as a fresh constant value in the arena.
 *
 * \param[in,out] machine The machine context.
 * \param[in] constant The constant to wrap.
 * \param[out] out On success, the new constant value.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
static cardano_error_t
constant_value(
  machine_t*                     machine,
  const cardano_uplc_constant_t* constant,
  const cardano_uplc_value_t**   out)
{
  cardano_uplc_value_t* value = NULL;
  cardano_error_t       error = cardano_uplc_value_new_constant(machine->arena, constant, &value);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *out = value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Selects and enters a case branch when the scrutinee is a constant.
 *
 * A bool selects branch 1 when true and branch 0 when false; a unit selects
 * branch 0 of a single branch; an integer selects the branch at its non-negative
 * in-range index; a non-empty list selects branch 0 and pushes its head then
 * tail; an empty list selects branch 1; a pair selects branch 0 and pushes its
 * first then second component. Any other constant kind, or an out-of-range or
 * missing branch, is a script error.
 *
 * \param[in,out] machine The machine context.
 * \param[in] frame The cases frame holding the branches and environment.
 * \param[in] constant The constant scrutinee.
 * \param[out] next On \ref PRV_STEP_CONTINUE, the next state.
 * \param[out] host_error On a host failure, the error code.
 *
 * \return The transition outcome.
 */
static step_outcome_t
case_on_constant(
  machine_t*                     machine,
  const cardano_uplc_frame_t*    frame,
  const cardano_uplc_constant_t* constant,
  state_t*                       next,
  cardano_error_t*               host_error)
{
  const cardano_uplc_frame_t* branch_ctx = frame->as.cases.ctx;
  size_t                      tag        = (size_t)0;

  *host_error = CARDANO_SUCCESS;

  switch (constant->kind)
  {
    case CARDANO_UPLC_TYPE_BOOL:
    {
      if ((frame->as.cases.branch_count < (size_t)1) || (frame->as.cases.branch_count > (size_t)2))
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      tag = constant->as.boolean ? (size_t)1 : (size_t)0;

      if (tag >= frame->as.cases.branch_count)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      break;
    }

    case CARDANO_UPLC_TYPE_UNIT:
    {
      if (frame->as.cases.branch_count != (size_t)1)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      break;
    }

    case CARDANO_UPLC_TYPE_INTEGER:
    {
      if (cardano_uplc_constant_int_is_small(constant))
      {
        const int64_t small = cardano_uplc_constant_int_small(constant);

        if (small < 0)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        tag = (size_t)small;
      }
      else if (constant_to_index(constant->as.integer.big, &tag) != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }
      else
      {
      }

      if (tag >= frame->as.cases.branch_count)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      break;
    }

    case CARDANO_UPLC_TYPE_LIST:
    {
      if (constant->as.list.count > (size_t)0)
      {
        const cardano_uplc_value_t* fields[2];
        cardano_uplc_constant_t*    tail_constant = NULL;

        if (frame->as.cases.branch_count < (size_t)1)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        *host_error = constant_value(machine, constant->as.list.items[0], &fields[0]);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        *host_error = cardano_uplc_constant_new_list(
          machine->arena,
          constant->as.list.element_type,
          &constant->as.list.items[1],
          constant->as.list.count - (size_t)1,
          &tail_constant);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        *host_error = constant_value(machine, tail_constant, &fields[1]);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        *host_error = transfer_arg_stack(machine, fields, (size_t)2, frame->as.cases.ctx, &branch_ctx);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        tag = (size_t)0;
      }
      else
      {
        if (frame->as.cases.branch_count < (size_t)2)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        tag = (size_t)1;
      }

      break;
    }

    case CARDANO_UPLC_TYPE_PAIR:
    {
      const cardano_uplc_value_t* fields[2];

      if (frame->as.cases.branch_count != (size_t)1)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = constant_value(machine, constant->as.pair.fst, &fields[0]);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = constant_value(machine, constant->as.pair.snd, &fields[1]);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = transfer_arg_stack(machine, fields, (size_t)2, frame->as.cases.ctx, &branch_ctx);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      tag = (size_t)0;

      break;
    }

    case CARDANO_UPLC_TYPE_BYTE_STRING:
    case CARDANO_UPLC_TYPE_STRING:
    case CARDANO_UPLC_TYPE_DATA:
    case CARDANO_UPLC_TYPE_ARRAY:
    case CARDANO_UPLC_TYPE_VALUE:
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    default:
    {
      return PRV_STEP_SCRIPT_ERROR;
    }
  }

  next->kind = CARDANO_UPLC_STATE_COMPUTE;
  next->ctx  = branch_ctx;
  next->env  = frame->as.cases.env;
  next->term = frame->as.cases.branches[tag];

  return PRV_STEP_CONTINUE;
}

/**
 * \brief Returns a computed value into the current continuation.
 *
 * Implements the Return half of the CEK transition for every frame. The empty
 * frame ends the machine; the apply, force, constr and cases frames advance it.
 *
 * \param[in,out] machine The machine context.
 * \param[in] ctx The frame the value returns into.
 * \param[in] value The value being returned.
 * \param[out] next On \ref PRV_STEP_CONTINUE, the next state; on
 *             \ref PRV_STEP_DONE, the final value in \c value.
 * \param[out] host_error On a host failure, the error code.
 *
 * \return The transition outcome.
 */
static step_outcome_t
run_return_state(
  machine_t*                  machine,
  const cardano_uplc_frame_t* ctx,
  const cardano_uplc_value_t* value,
  state_t*                    next,
  cardano_error_t*            host_error)
{
  *host_error = CARDANO_SUCCESS;

  switch (ctx->kind)
  {
    case CARDANO_UPLC_FRAME_NO_FRAME:
    {
      next->kind  = CARDANO_UPLC_STATE_DONE;
      next->value = value;

      return PRV_STEP_DONE;
    }

    case CARDANO_UPLC_FRAME_AWAIT_FUN_TERM:
    {
      cardano_uplc_frame_t* frame = NULL;

      *host_error = cardano_uplc_frame_new_await_arg(machine->arena, value, ctx->as.await_fun_term.ctx, &frame);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind = CARDANO_UPLC_STATE_COMPUTE;
      next->ctx  = frame;
      next->env  = ctx->as.await_fun_term.env;
      next->term = ctx->as.await_fun_term.term;

      return PRV_STEP_CONTINUE;
    }

    case CARDANO_UPLC_FRAME_AWAIT_ARG:
    {
      return apply_evaluate(machine, ctx->as.await_arg.ctx, ctx->as.await_arg.value, value, next, host_error);
    }

    case CARDANO_UPLC_FRAME_AWAIT_FUN_VALUE:
    {
      return apply_evaluate(machine, ctx->as.await_fun_value.ctx, value, ctx->as.await_fun_value.value, next, host_error);
    }

    case CARDANO_UPLC_FRAME_FORCE:
    {
      return force_evaluate(machine, ctx->as.force.ctx, value, next, host_error);
    }

    case CARDANO_UPLC_FRAME_CONSTR:
    {
      const cardano_uplc_value_t** resolved = NULL;
      size_t                       count    = ctx->as.constr.resolved_count;
      size_t                       i        = (size_t)0;

      resolved = (const cardano_uplc_value_t**)cardano_uplc_arena_alloc(
        machine->arena,
        sizeof(const cardano_uplc_value_t*) * (count + (size_t)1),
        0U);

      if (resolved == NULL)
      {
        *host_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        return PRV_STEP_SCRIPT_ERROR;
      }

      for (i = (size_t)0; i < count; ++i)
      {
        resolved[i] = ctx->as.constr.resolved[i];
      }

      resolved[count] = value;

      if (ctx->as.constr.field_count == (size_t)0)
      {
        cardano_uplc_value_t* result = NULL;

        *host_error = cardano_uplc_value_new_constr(
          machine->arena,
          ctx->as.constr.tag,
          resolved,
          count + (size_t)1,
          &result);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        next->kind  = CARDANO_UPLC_STATE_RETURN;
        next->ctx   = ctx->as.constr.ctx;
        next->value = result;

        return PRV_STEP_CONTINUE;
      }
      else
      {
        cardano_uplc_frame_t* frame = NULL;

        *host_error = cardano_uplc_frame_new_constr(
          machine->arena,
          ctx->as.constr.env,
          ctx->as.constr.tag,
          &ctx->as.constr.fields[1],
          ctx->as.constr.field_count - (size_t)1,
          resolved,
          count + (size_t)1,
          ctx->as.constr.ctx,
          &frame);

        if (*host_error != CARDANO_SUCCESS)
        {
          return PRV_STEP_SCRIPT_ERROR;
        }

        next->kind = CARDANO_UPLC_STATE_COMPUTE;
        next->ctx  = frame;
        next->env  = ctx->as.constr.env;
        next->term = ctx->as.constr.fields[0];

        return PRV_STEP_CONTINUE;
      }

      break;
    }

    case CARDANO_UPLC_FRAME_CASES:
    {
      const cardano_uplc_frame_t* branch_ctx = NULL;

      if (value->kind == CARDANO_UPLC_VALUE_CONSTANT)
      {
        return case_on_constant(machine, ctx, value->as.constant, next, host_error);
      }

      if (value->kind != CARDANO_UPLC_VALUE_CONSTR)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      if (value->as.constr.tag >= ctx->as.cases.branch_count)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      *host_error = transfer_arg_stack(
        machine,
        value->as.constr.fields,
        value->as.constr.field_count,
        ctx->as.cases.ctx,
        &branch_ctx);

      if (*host_error != CARDANO_SUCCESS)
      {
        return PRV_STEP_SCRIPT_ERROR;
      }

      next->kind = CARDANO_UPLC_STATE_COMPUTE;
      next->ctx  = branch_ctx;
      next->env  = ctx->as.cases.env;
      next->term = ctx->as.cases.branches[value->as.constr.tag];

      return PRV_STEP_CONTINUE;
    }

    default:
    {
      return PRV_STEP_SCRIPT_ERROR;
    }
  }
}

/* DISCHARGE ****************************************************************/

/**
 * \brief Upper bound on the nesting depth the discharge traversal will follow.
 *
 * \c with_env and \c discharge_value are mutually recursive over a
 * result value that can be arbitrarily deep on attacker-controlled input, so the
 * recursion is capped to keep the worst case well inside the C stack. Past this
 * depth the traversal stops with \ref CARDANO_ERROR_ILLEGAL_STATE, a host-side
 * resource limit rather than a script outcome. The value mirrors the decoder's
 * \c FLAT_TERM_MAX_DEPTH ceiling in \c flat_decode.c.
 */
enum
{
  PRV_DISCHARGE_MAX_DEPTH = 4096
};

static cardano_error_t
discharge_value(cardano_uplc_arena_t* arena, size_t depth, const cardano_uplc_value_t* value, const cardano_uplc_term_t** out);

/**
 * \brief Re-discharges a body term, substituting environment values for the free
 *        variables it does not itself bind.
 *
 * Walks \p term counting the binders entered along the way (\p lam_cnt). A
 * variable bound by one of those binders is left as-is; a deeper variable is
 * resolved in \p env and the bound value is discharged in its place.
 *
 * \param[in] arena The arena the rebuilt term is allocated in.
 * \param[in] depth The current discharge nesting depth.
 * \param[in] lam_cnt The number of binders entered since the discharge root.
 * \param[in] env The environment the closure captured.
 * \param[in] term The term to re-discharge.
 * \param[out] out On success, the rebuilt term.
 *
 * \return \ref CARDANO_SUCCESS on success,
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve a
 *         node, or \ref CARDANO_ERROR_ILLEGAL_STATE past
 *         \ref PRV_DISCHARGE_MAX_DEPTH.
 */
static cardano_error_t
with_env(
  cardano_uplc_arena_t*       arena,
  size_t                      depth,
  uint64_t                    lam_cnt,
  const cardano_uplc_env_t*   env,
  const cardano_uplc_term_t*  term,
  const cardano_uplc_term_t** out)
{
  cardano_error_t error = CARDANO_SUCCESS;

  if (depth > (size_t)PRV_DISCHARGE_MAX_DEPTH)
  {
    return CARDANO_ERROR_ILLEGAL_STATE;
  }

  switch (term->kind)
  {
    case CARDANO_UPLC_TERM_VAR:
    {
      if (lam_cnt >= term->as.var_index)
      {
        *out = term;
        return CARDANO_SUCCESS;
      }
      else
      {
        const cardano_uplc_value_t* bound = NULL;

        if (cardano_uplc_env_lookup(env, term->as.var_index - lam_cnt, &bound) == CARDANO_SUCCESS)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          return discharge_value(arena, depth + (size_t)1, bound, out);
        }

        *out = term;
        return CARDANO_SUCCESS;
      }

      break;
    }

    case CARDANO_UPLC_TERM_LAMBDA:
    {
      const cardano_uplc_term_t* body   = NULL;
      cardano_uplc_term_t*       result = NULL;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      error = with_env(arena, depth + (size_t)1, lam_cnt + (uint64_t)1, env, term->as.unary, &body);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      error = cardano_uplc_term_new_lambda(arena, body, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_TERM_DELAY:
    {
      const cardano_uplc_term_t* body   = NULL;
      cardano_uplc_term_t*       result = NULL;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      error = with_env(arena, depth + (size_t)1, lam_cnt, env, term->as.unary, &body);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      error = cardano_uplc_term_new_delay(arena, body, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_TERM_FORCE:
    {
      const cardano_uplc_term_t* body   = NULL;
      cardano_uplc_term_t*       result = NULL;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      error = with_env(arena, depth + (size_t)1, lam_cnt, env, term->as.unary, &body);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      error = cardano_uplc_term_new_force(arena, body, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_TERM_APPLY:
    {
      const cardano_uplc_term_t* function = NULL;
      const cardano_uplc_term_t* argument = NULL;
      cardano_uplc_term_t*       result   = NULL;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      error = with_env(arena, depth + (size_t)1, lam_cnt, env, term->as.apply.function, &function);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      error = with_env(arena, depth + (size_t)1, lam_cnt, env, term->as.apply.argument, &argument);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      error = cardano_uplc_term_new_apply(arena, function, argument, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_TERM_CONSTR:
    {
      const cardano_uplc_term_t** fields = NULL;
      cardano_uplc_term_t*        result = NULL;

      if (term->as.constr.field_count > (size_t)0)
      {
        size_t i;

        fields = (const cardano_uplc_term_t**)cardano_uplc_arena_alloc(
          arena,
          sizeof(const cardano_uplc_term_t*) * term->as.constr.field_count,
          0U);

        if (fields == NULL)
        {
          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }

        for (i = (size_t)0; i < term->as.constr.field_count; ++i)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          error = with_env(arena, depth + (size_t)1, lam_cnt, env, term->as.constr.fields[i], &fields[i]);

          if (error != CARDANO_SUCCESS)
          {
            return error;
          }
        }
      }

      error = cardano_uplc_term_new_constr(arena, term->as.constr.tag, fields, term->as.constr.field_count, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_TERM_CASE:
    {
      const cardano_uplc_term_t*  scrutinee = NULL;
      const cardano_uplc_term_t** branches  = NULL;
      cardano_uplc_term_t*        result    = NULL;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      error = with_env(arena, depth + (size_t)1, lam_cnt, env, term->as.cases.scrutinee, &scrutinee);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      if (term->as.cases.branch_count > (size_t)0)
      {
        size_t i;

        branches = (const cardano_uplc_term_t**)cardano_uplc_arena_alloc(
          arena,
          sizeof(const cardano_uplc_term_t*) * term->as.cases.branch_count,
          0U);

        if (branches == NULL)
        {
          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }

        for (i = (size_t)0; i < term->as.cases.branch_count; ++i)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          error = with_env(arena, depth + (size_t)1, lam_cnt, env, term->as.cases.branches[i], &branches[i]);

          if (error != CARDANO_SUCCESS)
          {
            return error;
          }
        }
      }

      error = cardano_uplc_term_new_case(arena, scrutinee, branches, term->as.cases.branch_count, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_TERM_CONSTANT:
    case CARDANO_UPLC_TERM_BUILTIN:
    case CARDANO_UPLC_TERM_ERROR:
    default:
    {
      *out = term;
      return CARDANO_SUCCESS;
    }
  }
}

/**
 * \brief Converts a final CEK value back into a term for the result.
 *
 * A constant becomes a constant term; a delay or lambda closure rebuilds its term
 * and re-discharges its captured environment into the body; a builtin rebuilds the
 * builtin applied to its forces and arguments; a constr discharges each field.
 *
 * \param[in] arena The arena the result term is allocated in.
 * \param[in] depth The current discharge nesting depth.
 * \param[in] value The value to discharge.
 * \param[out] out On success, the discharged term.
 *
 * \return \ref CARDANO_SUCCESS on success,
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve a
 *         node, or \ref CARDANO_ERROR_ILLEGAL_STATE past
 *         \ref PRV_DISCHARGE_MAX_DEPTH.
 */
static cardano_error_t
discharge_value(
  cardano_uplc_arena_t*       arena,
  size_t                      depth,
  const cardano_uplc_value_t* value,
  const cardano_uplc_term_t** out)
{
  cardano_error_t error = CARDANO_SUCCESS;

  if (depth > (size_t)PRV_DISCHARGE_MAX_DEPTH)
  {
    return CARDANO_ERROR_ILLEGAL_STATE;
  }

  switch (value->kind)
  {
    case CARDANO_UPLC_VALUE_CONSTANT:
    {
      cardano_uplc_term_t* result = NULL;

      error = cardano_uplc_term_new_constant(arena, value->as.constant, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_VALUE_DELAY:
    {
      cardano_uplc_term_t* delay = NULL;

      error = cardano_uplc_term_new_delay(arena, value->as.delay.body, &delay);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      return with_env(arena, depth + (size_t)1, (uint64_t)0, value->as.delay.env, delay, out);
    }

    case CARDANO_UPLC_VALUE_LAMBDA:
    {
      const cardano_uplc_term_t* discharged_body = NULL;
      cardano_uplc_term_t*       result          = NULL;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      error = with_env(arena, depth + (size_t)1, (uint64_t)1, value->as.lambda.env, value->as.lambda.body, &discharged_body);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      error = cardano_uplc_term_new_lambda(arena, discharged_body, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_VALUE_BUILTIN:
    {
      cardano_uplc_term_t*       builtin_term = NULL;
      const cardano_uplc_term_t* term         = NULL;
      size_t                     i            = (size_t)0;

      error = cardano_uplc_term_new_builtin(arena, value->as.builtin.func, &builtin_term);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      term = builtin_term;

      for (i = (size_t)0; i < value->as.builtin.forces; ++i)
      {
        cardano_uplc_term_t* force_term = NULL;

        error = cardano_uplc_term_new_force(arena, term, &force_term);

        if (error != CARDANO_SUCCESS)
        {
          return error;
        }

        term = force_term;
      }

      for (i = (size_t)0; i < value->as.builtin.arg_count; ++i)
      {
        const cardano_uplc_term_t* arg_term = NULL;
        cardano_uplc_term_t*       apply    = NULL;

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        error = discharge_value(arena, depth + (size_t)1, value->as.builtin.args[i], &arg_term);

        if (error != CARDANO_SUCCESS)
        {
          return error;
        }

        error = cardano_uplc_term_new_apply(arena, term, arg_term, &apply);

        if (error != CARDANO_SUCCESS)
        {
          return error;
        }

        term = apply;
      }

      *out = term;

      return CARDANO_SUCCESS;
    }

    case CARDANO_UPLC_VALUE_CONSTR:
    {
      const cardano_uplc_term_t** fields = NULL;
      cardano_uplc_term_t*        result = NULL;

      if (value->as.constr.field_count > (size_t)0)
      {
        size_t i;

        fields = (const cardano_uplc_term_t**)cardano_uplc_arena_alloc(
          arena,
          sizeof(const cardano_uplc_term_t*) * value->as.constr.field_count,
          0U);

        if (fields == NULL)
        {
          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }

        for (i = (size_t)0; i < value->as.constr.field_count; ++i)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          error = discharge_value(arena, depth + (size_t)1, value->as.constr.fields[i], &fields[i]);

          if (error != CARDANO_SUCCESS)
          {
            return error;
          }
        }
      }

      error = cardano_uplc_term_new_constr(arena, value->as.constr.tag, fields, value->as.constr.field_count, &result);

      if (error != CARDANO_SUCCESS)
      {
        return error;
      }

      *out = result;

      return CARDANO_SUCCESS;
    }

    default:
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }
}

/* EVALUATE *****************************************************************/

/**
 * \brief Maps a machine version to the matching cost-model version.
 *
 * \param[in] version The machine version to map.
 *
 * \return The cost-model version selecting the default tables; an unrecognized
 *         machine version maps to V3.
 */
static cardano_uplc_cost_model_version_t
cost_model_version(cardano_uplc_machine_version_t version)
{
  switch (version)
  {
    case CARDANO_UPLC_MACHINE_VERSION_V1:
    {
      return CARDANO_UPLC_COST_MODEL_VERSION_V1;
    }

    case CARDANO_UPLC_MACHINE_VERSION_V2:
    {
      return CARDANO_UPLC_COST_MODEL_VERSION_V2;
    }

    case CARDANO_UPLC_MACHINE_VERSION_V3:
    default:
    {
      return CARDANO_UPLC_COST_MODEL_VERSION_V3;
    }
  }
}

/**
 * \brief Maps a machine version to the matching builtin language version.
 *
 * \param[in] version The machine version to map.
 *
 * \return The builtin language version; an unrecognized machine version maps to V3.
 */
static cardano_uplc_lang_version_t
lang_version(cardano_uplc_machine_version_t version)
{
  switch (version)
  {
    case CARDANO_UPLC_MACHINE_VERSION_V1:
    {
      return CARDANO_UPLC_LANG_VERSION_V1;
    }

    case CARDANO_UPLC_MACHINE_VERSION_V2:
    {
      return CARDANO_UPLC_LANG_VERSION_V2;
    }

    case CARDANO_UPLC_MACHINE_VERSION_V3:
    default:
    {
      return CARDANO_UPLC_LANG_VERSION_V3;
    }
  }
}

/**
 * \brief Builds the default full cost model for a machine version.
 *
 * Pairs the default machine-step costs with the default per-builtin costing
 * functions for the version. The machine-step component equals
 * \ref cardano_uplc_machine_costs_default for the version, so the step loop charges
 * the same budgets as before this model was threaded through.
 *
 * \param[in] version The machine version selecting the tables.
 *
 * \return The default full cost model for \p version.
 */
static cardano_uplc_cost_model_t
cost_model_for_version(cardano_uplc_machine_version_t version)
{
  cardano_uplc_cost_model_t         model;
  cardano_uplc_cost_model_version_t cost_version = cost_model_version(version);

  model.machine = cardano_uplc_machine_costs_default(cost_version);

  switch (cost_version)
  {
    case CARDANO_UPLC_COST_MODEL_VERSION_V1:
    {
      model.builtins = cardano_uplc_builtin_costs_v1();
      break;
    }

    case CARDANO_UPLC_COST_MODEL_VERSION_V2:
    {
      model.builtins = cardano_uplc_builtin_costs_v2();
      break;
    }

    case CARDANO_UPLC_COST_MODEL_VERSION_V3:
    default:
    {
      model.builtins = cardano_uplc_builtin_costs_v3();
      break;
    }
  }

  return model;
}

cardano_error_t
cardano_uplc_int_evaluate_with_costs(
  cardano_uplc_arena_t*            arena,
  const cardano_uplc_program_t*    program,
  const cardano_uplc_cost_model_t* cost_model,
  cardano_uplc_builtin_semantics_t semantics,
  cardano_uplc_lang_version_t      language,
  uint64_t                         protocol_major,
  cardano_uplc_budget_t            initial_budget,
  cardano_uplc_eval_result_t*      out)
{
  machine_t             machine;
  cardano_uplc_frame_t* no_frame = NULL;
  state_t               state;
  cardano_error_t       error       = CARDANO_SUCCESS;
  bool                  running     = true;
  bool                  failed      = false;
  bool                  unsupported = false;

  if ((arena == NULL) || (program == NULL) || (program->term == NULL) || (cost_model == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  machine.arena          = arena;
  machine.initial        = initial_budget;
  machine.cost_model     = *cost_model;
  machine.semantics      = semantics;
  machine.language       = language;
  machine.protocol_major = protocol_major;

  error = cardano_uplc_step_accumulator_init(&machine.acc, &machine.cost_model.machine);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  error = cardano_uplc_step_accumulator_charge_startup(&machine.acc);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  if (cardano_uplc_step_accumulator_is_exhausted(&machine.acc, machine.initial))
  {
    out->status = CARDANO_UPLC_EVAL_OUT_OF_BUDGET;
    out->spent  = cardano_uplc_step_accumulator_spent(&machine.acc);
    out->result = NULL;

    return CARDANO_SUCCESS;
  }

  error = cardano_uplc_frame_new_no_frame(arena, &no_frame);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  state.kind  = CARDANO_UPLC_STATE_COMPUTE;
  state.ctx   = no_frame;
  state.env   = NULL;
  state.term  = program->term;
  state.value = NULL;

  while (running)
  {
    state_t               next;
    step_outcome_t        outcome      = PRV_STEP_DONE;
    cardano_uplc_budget_t spent_before = machine.acc.spent;

    next.kind = CARDANO_UPLC_STATE_COMPUTE;

    switch (state.kind)
    {
      case CARDANO_UPLC_STATE_COMPUTE:
      {
        outcome = run_compute_state(&machine, state.ctx, state.env, state.term, &next, &error);
        break;
      }

      case CARDANO_UPLC_STATE_RETURN:
      {
        outcome = run_return_state(&machine, state.ctx, state.value, &next, &error);
        break;
      }

      case CARDANO_UPLC_STATE_DONE:
      default:
      {
        outcome = PRV_STEP_DONE;
        break;
      }
    }

    if ((outcome == PRV_STEP_SCRIPT_ERROR) && (error != CARDANO_SUCCESS))
    {
      return error;
    }

    if (((machine.acc.spent.cpu != spent_before.cpu) || (machine.acc.spent.mem != spent_before.mem)) && cardano_uplc_step_accumulator_is_exhausted(&machine.acc, machine.initial))
    {
      out->status = CARDANO_UPLC_EVAL_OUT_OF_BUDGET;
      out->spent  = cardano_uplc_step_accumulator_spent(&machine.acc);
      out->result = NULL;

      return CARDANO_SUCCESS;
    }

    if (outcome == PRV_STEP_UNSUPPORTED_BUILTIN)
    {
      unsupported = true;
      running     = false;
    }
    else if (outcome == PRV_STEP_SCRIPT_ERROR)
    {
      failed  = true;
      running = false;
    }
    else if (outcome == PRV_STEP_DONE)
    {
      state   = next;
      running = false;
    }
    else
    {
      state = next;
    }
  }

  error = cardano_uplc_step_accumulator_flush(&machine.acc);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  if (cardano_uplc_step_accumulator_is_exhausted(&machine.acc, machine.initial))
  {
    out->status = CARDANO_UPLC_EVAL_OUT_OF_BUDGET;
    out->spent  = cardano_uplc_step_accumulator_spent(&machine.acc);
    out->result = NULL;

    return CARDANO_SUCCESS;
  }

  if (unsupported)
  {
    out->status = CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN;
    out->spent  = cardano_uplc_step_accumulator_spent(&machine.acc);
    out->result = NULL;

    return CARDANO_SUCCESS;
  }

  if (failed)
  {
    out->status = CARDANO_UPLC_EVAL_ERROR_TERM;
    out->spent  = cardano_uplc_step_accumulator_spent(&machine.acc);
    out->result = NULL;

    return CARDANO_SUCCESS;
  }

  {
    const cardano_uplc_term_t* result = NULL;

    error = discharge_value(arena, (size_t)0, state.value, &result);

    if (error != CARDANO_SUCCESS)
    {
      return error;
    }

    out->status = CARDANO_UPLC_EVAL_SUCCESS;
    out->spent  = cardano_uplc_step_accumulator_spent(&machine.acc);
    out->result = result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_evaluate(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_program_t*  program,
  cardano_uplc_machine_version_t version,
  cardano_uplc_budget_t          initial_budget,
  cardano_uplc_eval_result_t*    out)
{
  cardano_uplc_cost_model_t        cost_model = cost_model_for_version(version);
  cardano_uplc_builtin_semantics_t semantics  = cardano_uplc_builtin_semantics_for_language(lang_version(version));

  // This generic entry point does not carry a protocol version; gate by language
  // only by assuming the newest protocol, where every builtin of the language is
  // available. The transaction evaluator threads the real protocol version below.
  return cardano_uplc_int_evaluate_with_costs(arena, program, &cost_model, semantics, lang_version(version), 11U, initial_budget, out);
}
