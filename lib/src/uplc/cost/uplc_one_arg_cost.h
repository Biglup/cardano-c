/**
 * \file uplc_one_arg_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_ONE_ARG_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_ONE_ARG_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_linear_cost.h"
#include "uplc_one_arg_kind.h"
#include "uplc_quadratic_cost.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A one-argument costing function as a tagged union.
 *
 * The active union member is selected by \c kind. Evaluate with
 * \ref cardano_uplc_one_arg_cost_eval.
 */
typedef struct cardano_uplc_one_arg_cost_t
{
    /** \brief Selects the active union member. */
    cardano_uplc_one_arg_kind_t kind;

    /** \brief The variant parameters. */
    // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    union
    {
        /** \brief Active when kind is CONSTANT. */
        int64_t constant;
        /** \brief Active when kind is LINEAR. */
        cardano_uplc_linear_cost_t linear;
        /** \brief Active when kind is QUADRATIC. */
        cardano_uplc_quadratic_cost_t quadratic;
        // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    } params;
} cardano_uplc_one_arg_cost_t;

/**
 * \brief Evaluates a non-NULL one-argument costing function at a size.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the single argument.
 *
 * \return The saturating cost.
 */
static inline int64_t
cardano_uplc_one_arg_eval(const cardano_uplc_one_arg_cost_t* fn, int64_t x)
{
  int64_t result = 0;

  switch (fn->kind)
  {
    case CARDANO_UPLC_ONE_ARG_CONSTANT:
    {
      result = fn->params.constant;
      break;
    }
    case CARDANO_UPLC_ONE_ARG_LINEAR:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, x);
      break;
    }
    case CARDANO_UPLC_ONE_ARG_QUADRATIC:
    {
      result = cardano_uplc_quadratic_cost_eval(fn->params.quadratic, x);
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
 * \brief Evaluates a one-argument costing function at a size.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the single argument.
 *
 * \return The saturating cost, or 0 if \p fn is NULL.
 */
int64_t
cardano_uplc_one_arg_cost_eval(const cardano_uplc_one_arg_cost_t* fn, int64_t x);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_ONE_ARG_COST_H */
