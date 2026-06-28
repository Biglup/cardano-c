/**
 * \file uplc_value.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_VALUE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_VALUE_H

/* INCLUDES ******************************************************************/

#include "uplc_value_kind.h"

#include "../arena/uplc_arena.h"
#include "../ast/uplc_term.h"
#include "../builtins/uplc_builtin.h"
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* FORWARD DECLARATIONS ******************************************************/

#ifndef CARDANO_UPLC_VALUE_T_FORWARD_DECLARED
#define CARDANO_UPLC_VALUE_T_FORWARD_DECLARED
typedef struct cardano_uplc_value_t cardano_uplc_value_t;
#endif /* CARDANO_UPLC_VALUE_T_FORWARD_DECLARED */

#ifndef CARDANO_UPLC_ENV_T_FORWARD_DECLARED
#define CARDANO_UPLC_ENV_T_FORWARD_DECLARED
typedef struct cardano_uplc_env_t cardano_uplc_env_t;
#endif /* CARDANO_UPLC_ENV_T_FORWARD_DECLARED */

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Sentinel for an un-memoized \ref cardano_uplc_value_t ex-mem size.
 *
 * Every ex-mem size is non-negative, so a negative value unambiguously marks the
 * cache as not yet computed.
 */
#define CARDANO_UPLC_VALUE_EX_MEM_UNCOMPUTED ((int64_t)-1)

/**
 * \brief An immutable CEK runtime value, tagged by its value kind.
 *
 * The \c kind selects the active union arm; never read an arm the kind does not
 * select. Closures (\c delay, \c lambda) hold the body term and the environment
 * they were built in. A \c builtin accumulates the forces seen and the argument
 * values applied so far until it saturates. A \c constr holds its tag and the
 * already-resolved field values. Values are arena-allocated and never mutated
 * after construction; the constant an arena owns inside a \c constant value is
 * already registered with that arena.
 *
 * \c ex_mem caches the value's ex-mem size so the builtin cost path does not
 * walk the constant on every application. It starts at
 * \ref CARDANO_UPLC_VALUE_EX_MEM_UNCOMPUTED and is filled lazily on first use by
 * \ref cardano_uplc_value_ex_mem. The size depends only on the value's payload
 * and the semantics flag, both fixed for the lifetime of the value, so the cache
 * is always consistent.
 */
struct cardano_uplc_value_t
{
    cardano_uplc_value_kind_t kind;
    int64_t                   ex_mem;

    // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    union
    {
        const cardano_uplc_constant_t* constant;

        struct
        {
            const cardano_uplc_term_t* body;
            const cardano_uplc_env_t*  env;
        } delay;

        struct
        {
            const cardano_uplc_term_t* body;
            const cardano_uplc_env_t*  env;
        } lambda;

        struct
        {
            cardano_uplc_builtin_t             func;
            size_t                             forces;
            const cardano_uplc_value_t* const* args;
            size_t                             arg_count;
        } builtin;

        struct
        {
            uint64_t                           tag;
            const cardano_uplc_value_t* const* fields;
            size_t                             field_count;
        } constr;

        // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    } as;
};

/**
 * \brief Builds a constant value wrapping an arena-owned constant.
 *
 * \param[in] arena The arena to allocate the value from. Must not be NULL.
 * \param[in] constant The constant the value holds. Must not be NULL and must
 *            live in \p arena.
 * \param[out] out On success, set to the new value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_value_new_constant(
  struct cardano_uplc_arena_t*   arena,
  const cardano_uplc_constant_t* constant,
  cardano_uplc_value_t**         out);

/**
 * \brief Builds a delay value closing a body term over an environment.
 *
 * \param[in] arena The arena to allocate the value from. Must not be NULL.
 * \param[in] body The delayed body term. Must not be NULL and must live in \p arena.
 * \param[in] env The environment the delay closes over, or NULL for the empty
 *            environment.
 * \param[out] out On success, set to the new value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p body, or \p out is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_value_new_delay(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_term_t*   body,
  const cardano_uplc_env_t*    env,
  cardano_uplc_value_t**       out);

/**
 * \brief Builds a lambda closure closing a body term over an environment.
 *
 * The binder is implicit: applying the closure extends \p env with the argument
 * value before computing \p body.
 *
 * \param[in] arena The arena to allocate the value from. Must not be NULL.
 * \param[in] body The lambda body term. Must not be NULL and must live in \p arena.
 * \param[in] env The environment the closure captures, or NULL for the empty
 *            environment.
 * \param[out] out On success, set to the new value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p body, or \p out is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_value_new_lambda(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_term_t*   body,
  const cardano_uplc_env_t*    env,
  cardano_uplc_value_t**       out);

/**
 * \brief Builds a partially applied builtin value.
 *
 * The \p args array and the values it points at must already live in \p arena;
 * the constructor copies the pointers by reference, it does not copy the values.
 * A builtin with no arguments yet is allowed with \p arg_count 0 and \p args
 * NULL.
 *
 * \param[in] arena The arena to allocate the value from. Must not be NULL.
 * \param[in] func The builtin function tag.
 * \param[in] forces The number of forces applied to the builtin so far.
 * \param[in] args The argument values applied so far, or NULL when \p arg_count
 *            is 0.
 * \param[in] arg_count The number of arguments applied so far.
 * \param[out] out On success, set to the new value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p args is NULL while \p arg_count is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_value_new_builtin(
  struct cardano_uplc_arena_t*       arena,
  cardano_uplc_builtin_t             func,
  size_t                             forces,
  const cardano_uplc_value_t* const* args,
  size_t                             arg_count,
  cardano_uplc_value_t**             out);

/**
 * \brief Builds a constructor value from a tag and a resolved-field array.
 *
 * The \p fields array and the values it points at must already live in \p arena.
 * A constructor with no fields is allowed with \p field_count 0 and \p fields
 * NULL.
 *
 * \param[in] arena The arena to allocate the value from. Must not be NULL.
 * \param[in] tag The constructor tag.
 * \param[in] fields The resolved field values, or NULL when \p field_count is 0.
 * \param[in] field_count The number of fields.
 * \param[out] out On success, set to the new value; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p fields is NULL while \p field_count is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_value_new_constr(
  struct cardano_uplc_arena_t*       arena,
  uint64_t                           tag,
  const cardano_uplc_value_t* const* fields,
  size_t                             field_count,
  cardano_uplc_value_t**             out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_VALUE_H */
