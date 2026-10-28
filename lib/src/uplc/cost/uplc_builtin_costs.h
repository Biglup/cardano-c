/**
 * \file uplc_builtin_costs.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COSTS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COSTS_H

/* INCLUDES ******************************************************************/

#include "uplc_builtin_cost.h"

#include "../builtins/uplc_builtin.h"
#include "../builtins/uplc_builtin_semantics.h"
#include "../machine/uplc_budget.h"
#include <cardano/typedefs.h>

/* FORWARD DECLARATIONS ******************************************************/

#ifndef CARDANO_UPLC_VALUE_T_FORWARD_DECLARED
#define CARDANO_UPLC_VALUE_T_FORWARD_DECLARED
typedef struct cardano_uplc_value_t cardano_uplc_value_t;
#endif /* CARDANO_UPLC_VALUE_T_FORWARD_DECLARED */

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The cpu/mem costing functions of every builtin, indexed by tag.
 *
 * Holds one \ref cardano_uplc_builtin_cost_t per \ref cardano_uplc_builtin_t,
 * indexable by the builtin tag. Builtins not available in a version carry the
 * prohibitive sentinel cost.
 */
typedef struct cardano_uplc_builtin_costs_t
{
    /** \brief The per-builtin costing functions, indexed by builtin tag. */
    cardano_uplc_builtin_cost_t entries[CARDANO_UPLC_BUILTIN_COUNT];
} cardano_uplc_builtin_costs_t;

/**
 * \brief Returns the default Plutus V1 per-builtin costing functions.
 *
 * The coefficients are those of the reference builtin cost model D, which the
 * V1 and V2 languages read from protocol version 11 (van Rossem). The integer
 * division shapes are those of cost model B until
 * \ref cardano_uplc_builtin_costs_select_division_shapes reshapes them. Builtins
 * without a costing function carry the prohibitive sentinel cost.
 *
 * \return The V1 builtin costs by value.
 */
cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v1(void);

/**
 * \brief Returns the default Plutus V2 per-builtin costing functions.
 *
 * Identical to \ref cardano_uplc_builtin_costs_v1: both read builtin cost
 * model D.
 *
 * \return The V2 builtin costs by value.
 */
cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v2(void);

/**
 * \brief Returns the default Plutus V3 and V4 per-builtin costing functions.
 *
 * The coefficients are those of the reference builtin cost model E, which V3
 * reads from protocol version 11 (van Rossem) and V4 always reads. The integer
 * division shapes are those of cost model C until
 * \ref cardano_uplc_builtin_costs_select_division_shapes reshapes them.
 *
 * \return The V3 builtin costs by value.
 */
cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v3(void);

/**
 * \brief Reshapes the integer-division costing functions for a semantics variant.
 *
 * The ledger overlays its parameters on the builtin cost model of the semantics
 * variant: A, B and C read cost models A, B and C, D and F read cost model D, and
 * E and G read cost model E. The parameter lists are the same for every variant;
 * cost models D and E only interpret some division parameters with other shapes.
 * Under D, E, F and G the cpu of divideInteger and modInteger becomes
 * above-and-below-diagonal: the nested model of the const-above-diagonal shape is
 * evaluated at (max(x, y), min(x, y)) on both sides of the diagonal, and the
 * constant is no longer charged. Under D and F the mem of modInteger and
 * remainderInteger becomes linear-in-y2, which is linear in y and keeps the
 * subtracted-sizes minimum unread. quotientInteger keeps its shapes. Under A, B
 * and C nothing changes.
 *
 * Only const-above-diagonal and const-above-diagonal-into-quadratic cpu functions
 * and subtracted-sizes mem functions are reshaped, keeping their coefficients;
 * any other shape is left as is, so applying the same variant twice is harmless.
 *
 * \param[in,out] costs The builtin costs to reshape. Ignored when NULL.
 * \param[in] semantics The semantics variant selecting the cost model shapes.
 */
void
cardano_uplc_builtin_costs_select_division_shapes(
  cardano_uplc_builtin_costs_t*    costs,
  cardano_uplc_builtin_semantics_t semantics);

/**
 * \brief Computes the cpu and mem budget a builtin charges for its arguments.
 *
 * Computes each argument's ex-mem and feeds the right sizes to the builtin's cpu
 * and mem costing functions. Most builtins feed the ex-mem
 * of each argument; a few feed a literal-derived size (\c integerToByteString and
 * \c replicateByte feed a width derived from an integer literal, \c writeBits
 * feeds the update-list length, and \c shiftByteString and \c rotateByteString
 * feed the absolute shift literal). When an argument the size depends on is
 * missing or of the wrong shape, the size used is 0.
 *
 * \param[in] costs The per-builtin costs to charge against. Must not be NULL.
 * \param[in] func The builtin being applied.
 * \param[in] args The saturated argument values, in application order. May be
 *            NULL only when \p arg_count is 0.
 * \param[in] arg_count The number of arguments applied.
 * \param[in] costs_strings_by_utf8_bytes Selects the string ex-mem measure, as in
 *            \ref cardano_uplc_string_ex_mem.
 *
 * \return The cpu and mem budget the builtin charges. A NULL \p costs or an
 *         out-of-range \p func yields a zero budget.
 */
cardano_uplc_budget_t
cardano_uplc_builtin_cost(
  const cardano_uplc_builtin_costs_t* costs,
  cardano_uplc_builtin_t              func,
  const cardano_uplc_value_t* const*  args,
  size_t                              arg_count,
  bool                                costs_strings_by_utf8_bytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COSTS_H */
