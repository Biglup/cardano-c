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
 * Builtins introduced after V1 carry the prohibitive sentinel cost.
 *
 * \return The V1 builtin costs by value.
 */
cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v1(void);

/**
 * \brief Returns the default Plutus V2 per-builtin costing functions.
 *
 * \return The V2 builtin costs by value.
 */
cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v2(void);

/**
 * \brief Returns the default Plutus V3 per-builtin costing functions.
 *
 * \return The V3 builtin costs by value.
 */
cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v3(void);

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
