/**
 * \file uplc_four_arg_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_FOUR_ARG_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_FOUR_ARG_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_four_arg_kind.h"
#include "uplc_linear_cost.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A four-argument costing function as a tagged union.
 *
 * The active union member is selected by \c kind. Evaluate with
 * \ref cardano_uplc_four_arg_cost_eval.
 */
typedef struct cardano_uplc_four_arg_cost_t
{
  /** \brief Selects the active union member. */
  cardano_uplc_four_arg_kind_t kind;
  /** \brief The variant parameters. */
  union
  {
    /** \brief Active for CONSTANT. */
    int64_t constant;
    /** \brief Active for LINEAR_IN_U. */
    cardano_uplc_linear_cost_t linear_in_u;
  } params;
} cardano_uplc_four_arg_cost_t;

/**
 * \brief Evaluates a non-NULL four-argument costing function at four sizes.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the first argument.
 * \param[in] y The ex-mem size of the second argument.
 * \param[in] z The ex-mem size of the third argument.
 * \param[in] u The ex-mem size of the fourth argument.
 *
 * \return The saturating cost.
 */
static inline int64_t
cardano_uplc_four_arg_eval(const cardano_uplc_four_arg_cost_t* fn, int64_t x, int64_t y, int64_t z, int64_t u)
{
  int64_t result = 0;

  (void)x;
  (void)y;
  (void)z;

  switch (fn->kind)
  {
    case CARDANO_UPLC_FOUR_ARG_CONSTANT:
    {
      result = fn->params.constant;
      break;
    }
    case CARDANO_UPLC_FOUR_ARG_LINEAR_IN_U:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear_in_u, u);
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

/**
 * \brief Evaluates a four-argument costing function at four sizes.
 *
 * Only the fourth argument can affect the cost in the shapes that occur; the
 * first three are accepted to keep the evaluator total over the arity.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the first argument.
 * \param[in] y The ex-mem size of the second argument.
 * \param[in] z The ex-mem size of the third argument.
 * \param[in] u The ex-mem size of the fourth argument.
 *
 * \return The saturating cost, or 0 if \p fn is NULL.
 */
int64_t
cardano_uplc_four_arg_cost_eval(const cardano_uplc_four_arg_cost_t* fn, int64_t x, int64_t y, int64_t z, int64_t u);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_FOUR_ARG_COST_H */
