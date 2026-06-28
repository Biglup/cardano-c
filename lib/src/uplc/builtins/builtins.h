/**
 * \file builtins.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_BUILTINS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_BUILTINS_H

/* INCLUDES ******************************************************************/

#include "../cost/uplc_builtin_costs.h"
#include "../cost/uplc_step_accumulator.h"
#include "../data/uplc_data.h"
#include "../machine/uplc_value.h"
#include "uplc_builtin_outcome.h"
#include "uplc_builtin_semantics.h"
#include "uplc_byte_view.h"

#include "../arena/uplc_arena.h"
#include "uplc_builtin.h"
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Runs a saturated builtin: cost-before-execute, then the body.
 *
 * Precondition: the builtin is ready, i.e. \p arg_count equals its arity and the
 * forces it accumulated equal its force count. An implemented builtin first
 * computes its cpu/mem cost from the ex-mem sizes of \p args
 * (\ref cardano_uplc_builtin_cost), spends that cost on \p acc, and only then
 * runs its body; this ordering is observable: the cost is spent before the body
 * is called. An unimplemented builtin spends nothing and returns
 * \ref CARDANO_UPLC_BUILTIN_OUTCOME_UNSUPPORTED (its case is skipped, so leaving
 * the budget untouched cannot corrupt the run).
 *
 * The dispatch is an exhaustive switch over \ref cardano_uplc_builtin_t; every
 * arm currently returns the unsupported outcome. As bodies land they replace
 * their arm with compute-cost, spend, run.
 *
 * \param[in] arena The arena every result value is allocated from. Must not be NULL.
 * \param[in,out] acc The step accumulator the builtin cost is spent on. Must not
 *            be NULL.
 * \param[in] costs The per-builtin costing functions. Must not be NULL.
 * \param[in] semantics The builtin semantics variant selecting the string ex-mem
 *            measure and the body's range-check behaviour.
 * \param[in] func The saturated builtin to run.
 * \param[in] args The saturated argument values, in application order. May be NULL
 *            only when \p arg_count is 0.
 * \param[in] arg_count The number of arguments applied.
 * \param[out] out_result On \ref CARDANO_UPLC_BUILTIN_OUTCOME_OK, the result value;
 *             left untouched otherwise.
 * \param[out] host_error On a host failure (allocation), the error code; set to
 *             \ref CARDANO_SUCCESS otherwise.
 *
 * \return The script-visible outcome of the run.
 */
cardano_uplc_int_builtin_outcome_t
cardano_uplc_int_builtin_run(
  struct cardano_uplc_arena_t*        arena,
  cardano_uplc_step_accumulator_t*    acc,
  const cardano_uplc_builtin_costs_t* costs,
  cardano_uplc_builtin_semantics_t    semantics,
  cardano_uplc_builtin_t              func,
  const cardano_uplc_value_t* const*  args,
  size_t                              arg_count,
  const cardano_uplc_value_t**        out_result,
  cardano_error_t*                    host_error);

/* ARGUMENT-UNWRAP HELPERS **************************************************/

/**
 * \brief Reads the integer payload of a constant value.
 *
 * The value must be a constant of integer kind. Any other value (a non-constant,
 * or a constant of a different kind) is a type mismatch, a Plutus script error.
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 * \param[out] out On success, set to the borrowed bigint; left untouched on a
 *             mismatch.
 *
 * The machine stores a small integer inline as an \c int64_t with no
 * \ref cardano_bigint_t allocated, so this accessor reports a mismatch for such a
 * value unless a bigint view has already been materialized for it. The builtin
 * bodies read integers through the internal inline-aware path instead; this entry
 * remains for callers that already hold a bigint-backed integer constant.
 *
 * \return \c true on success, \c false on a type mismatch (script error) or when the
 *         value is a small integer with no materialized bigint.
 */
bool
cardano_uplc_builtin_as_integer(const cardano_uplc_value_t* value, const cardano_bigint_t** out);

/**
 * \brief Reads the byte-string payload of a constant value as an arena byte view.
 *
 * A non-byte-string value is a type mismatch (script error). The view borrows
 * the constant's arena bytes.
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 * \param[out] out On success, set to the borrowed \c (data, size) view; left
 *             untouched on a mismatch.
 *
 * \return \c true on success, \c false on a type mismatch (script error).
 */
bool
cardano_uplc_builtin_as_byte_string(const cardano_uplc_value_t* value, cardano_uplc_byte_view_t* out);

/**
 * \brief Reads the text-string payload of a constant value as an arena byte view.
 *
 * A non-string value is a type mismatch (script error). The view borrows the
 * constant's arena bytes.
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 * \param[out] out On success, set to the borrowed \c (data, size) view; left
 *             untouched on a mismatch.
 *
 * \return \c true on success, \c false on a type mismatch (script error).
 */
bool
cardano_uplc_builtin_as_string(const cardano_uplc_value_t* value, cardano_uplc_byte_view_t* out);

/**
 * \brief Reads the boolean payload of a constant value.
 *
 * A non-boolean value is a type mismatch (script error).
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 * \param[out] out On success, set to the boolean; left untouched on a mismatch.
 *
 * \return \c true on success, \c false on a type mismatch (script error).
 */
bool
cardano_uplc_builtin_as_bool(const cardano_uplc_value_t* value, bool* out);

/**
 * \brief Reads the unit payload of a constant value.
 *
 * A non-unit value is a type mismatch (script error). There is no payload;
 * success is the only information returned.
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 *
 * \return \c true on success, \c false on a type mismatch (script error).
 */
bool
cardano_uplc_builtin_as_unit(const cardano_uplc_value_t* value);

/**
 * \brief Reads the arena Plutus-data payload of a constant value.
 *
 * A non-data value is a type mismatch (script error).
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 * \param[out] out On success, set to the borrowed arena data node; left untouched on
 *             a mismatch.
 *
 * \return \c true on success, \c false on a type mismatch (script error).
 */
bool
cardano_uplc_builtin_as_data(const cardano_uplc_value_t* value, const cardano_uplc_data_t** out);

/**
 * \brief Reads the list payload of a constant value.
 *
 * The value must be a constant of list kind. A non-list value is a list type
 * mismatch (script error). The element type and the item array are borrowed from
 * the constant.
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 * \param[out] element_type On success, the borrowed element type; left untouched
 *             on a mismatch. May be NULL if the caller does not need it.
 * \param[out] items On success, the borrowed item array (NULL when \p count is 0);
 *             left untouched on a mismatch. May be NULL if not needed.
 * \param[out] count On success, the number of items; left untouched on a mismatch.
 *             May be NULL if not needed.
 *
 * \return \c true on success, \c false on a type mismatch (script error).
 */
bool
cardano_uplc_builtin_as_list(
  const cardano_uplc_value_t*            value,
  const cardano_uplc_type_t**            element_type,
  const cardano_uplc_constant_t* const** items,
  size_t*                                count);

/**
 * \brief Reads the pair payload of a constant value.
 *
 * The value must be a constant of pair kind. A non-pair value is a pair type
 * mismatch (script error). Both components are borrowed from the constant.
 *
 * \param[in] value The value to unwrap. Must not be NULL.
 * \param[out] fst On success, the borrowed first component; left untouched on a
 *             mismatch. May be NULL if not needed.
 * \param[out] snd On success, the borrowed second component; left untouched on a
 *             mismatch. May be NULL if not needed.
 *
 * \return \c true on success, \c false on a type mismatch (script error).
 */
bool
cardano_uplc_builtin_as_pair(
  const cardano_uplc_value_t*     value,
  const cardano_uplc_constant_t** fst,
  const cardano_uplc_constant_t** snd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_BUILTINS_H */
