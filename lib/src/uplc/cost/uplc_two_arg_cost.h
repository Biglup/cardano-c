/**
 * \file uplc_two_arg_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_ARG_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_ARG_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_const_or_linear_cost.h"
#include "uplc_const_or_two_arg_cost.h"
#include "uplc_linear_cost.h"
#include "uplc_quadratic_cost.h"
#include "uplc_subtracted_sizes_cost.h"
#include "uplc_two_arg_kind.h"
#include "uplc_two_var_linear_cost.h"
#include "uplc_two_var_quadratic_cost.h"
#include "uplc_with_interaction_cost.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A two-argument costing function as a tagged union.
 *
 * The active union member is selected by \c kind. Evaluate with
 * \ref cardano_uplc_two_arg_cost_eval.
 */
typedef struct cardano_uplc_two_arg_cost_t
{
  /** \brief Selects the active union member. */
  cardano_uplc_two_arg_kind_t kind;
  /** \brief The variant parameters. */
  union
  {
    /** \brief Active for CONSTANT. */
    int64_t constant;
    /** \brief Active for LINEAR_IN_X, LINEAR_IN_Y, ADDED_SIZES, MULTIPLIED_SIZES, MIN_SIZE, MAX_SIZE, DROP_LIST. */
    cardano_uplc_linear_cost_t linear;
    /** \brief Active for LINEAR_IN_X_AND_Y. */
    cardano_uplc_two_var_linear_cost_t linear_in_x_and_y;
    /** \brief Active for WITH_INTERACTION. */
    cardano_uplc_with_interaction_cost_t with_interaction;
    /** \brief Active for SUBTRACTED_SIZES. */
    cardano_uplc_subtracted_sizes_cost_t subtracted_sizes;
    /** \brief Active for LINEAR_ON_DIAGONAL. */
    cardano_uplc_const_or_linear_cost_t linear_on_diagonal;
    /** \brief Active for CONST_ABOVE_DIAGONAL, ABOVE_AND_BELOW_DIAGONAL, CONST_BELOW_DIAGONAL. */
    cardano_uplc_const_or_two_arg_cost_t const_diagonal;
    /** \brief Active for QUADRATIC_IN_Y. */
    cardano_uplc_quadratic_cost_t quadratic_in_y;
    /** \brief Active for QUADRATIC_IN_X_AND_Y. */
    cardano_uplc_two_var_quadratic_cost_t quadratic_in_x_and_y;
    /** \brief Active for CONST_ABOVE_DIAGONAL_INTO_QUADRATIC. */
    struct
    {
      /** \brief The flat cost charged when x < y. */
      int64_t constant;
      /** \brief The quadratic evaluated when x >= y. */
      cardano_uplc_two_var_quadratic_cost_t quadratic;
    } const_above_into_quadratic;
  } params;
} cardano_uplc_two_arg_cost_t;

/**
 * \brief Evaluates a non-NULL two-argument costing function at two sizes.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the first argument.
 * \param[in] y The ex-mem size of the second argument.
 *
 * \return The saturating cost.
 */
static inline int64_t
cardano_uplc_two_arg_eval(const cardano_uplc_two_arg_cost_t* fn, int64_t x, int64_t y)
{
  int64_t result = 0;

  switch (fn->kind)
  {
    case CARDANO_UPLC_TWO_ARG_CONSTANT:
    {
      result = fn->params.constant;
      break;
    }
    case CARDANO_UPLC_TWO_ARG_LINEAR_IN_X:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, x);
      break;
    }
    case CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, y);
      break;
    }
    case CARDANO_UPLC_TWO_ARG_LINEAR_IN_X_AND_Y:
    {
      result = cardano_uplc_two_var_linear_cost_eval(fn->params.linear_in_x_and_y, x, y);
      break;
    }
    case CARDANO_UPLC_TWO_ARG_WITH_INTERACTION:
    {
      cardano_uplc_with_interaction_cost_t w = fn->params.with_interaction;

      result = sat_add(w.c00, sat_mul(w.c10, x));
      result = sat_add(result, sat_mul(w.c01, y));
      result = sat_add(result, sat_mul(w.c11, sat_mul(x, y)));
      break;
    }
    case CARDANO_UPLC_TWO_ARG_ADDED_SIZES:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, sat_add(x, y));
      break;
    }
    case CARDANO_UPLC_TWO_ARG_SUBTRACTED_SIZES:
    {
      cardano_uplc_subtracted_sizes_cost_t s   = fn->params.subtracted_sizes;
      int64_t                              arg = max_i64(s.minimum, sat_sub(x, y));

      result = sat_add(s.intercept, sat_mul(s.slope, arg));
      break;
    }
    case CARDANO_UPLC_TWO_ARG_MULTIPLIED_SIZES:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, sat_mul(x, y));
      break;
    }
    case CARDANO_UPLC_TWO_ARG_MIN_SIZE:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, min_i64(x, y));
      break;
    }
    case CARDANO_UPLC_TWO_ARG_MAX_SIZE:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, max_i64(x, y));
      break;
    }
    case CARDANO_UPLC_TWO_ARG_LINEAR_ON_DIAGONAL:
    {
      cardano_uplc_const_or_linear_cost_t d = fn->params.linear_on_diagonal;

      if (x == y)
      {
        result = sat_add(d.intercept, sat_mul(d.slope, x));
      }
      else
      {
        result = d.constant;
      }
      break;
    }
    case CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL:
    {
      cardano_uplc_const_or_two_arg_cost_t c = fn->params.const_diagonal;

      result = (x < y) ? c.constant : cardano_uplc_const_or_two_arg_cost_eval(c, x, y);
      break;
    }
    case CARDANO_UPLC_TWO_ARG_ABOVE_AND_BELOW_DIAGONAL:
    {
      cardano_uplc_const_or_two_arg_cost_t c = fn->params.const_diagonal;

      result = cardano_uplc_const_or_two_arg_cost_eval(c, max_i64(x, y), min_i64(x, y));
      break;
    }
    case CARDANO_UPLC_TWO_ARG_CONST_BELOW_DIAGONAL:
    {
      cardano_uplc_const_or_two_arg_cost_t c = fn->params.const_diagonal;

      result = (x > y) ? c.constant : cardano_uplc_const_or_two_arg_cost_eval(c, x, y);
      break;
    }
    case CARDANO_UPLC_TWO_ARG_QUADRATIC_IN_Y:
    {
      result = cardano_uplc_quadratic_cost_eval(fn->params.quadratic_in_y, y);
      break;
    }
    case CARDANO_UPLC_TWO_ARG_QUADRATIC_IN_X_AND_Y:
    {
      result = cardano_uplc_two_var_quadratic_cost_eval(fn->params.quadratic_in_x_and_y, x, y);
      break;
    }
    case CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL_INTO_QUADRATIC:
    {
      if (x < y)
      {
        result = fn->params.const_above_into_quadratic.constant;
      }
      else
      {
        result = cardano_uplc_two_var_quadratic_cost_eval(fn->params.const_above_into_quadratic.quadratic, x, y);
      }
      break;
    }
    case CARDANO_UPLC_TWO_ARG_DROP_LIST:
    {
      result = fn->params.linear.intercept;
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
 * \brief Evaluates a two-argument costing function at two sizes.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the first argument.
 * \param[in] y The ex-mem size of the second argument.
 *
 * \return The saturating cost, or 0 if \p fn is NULL.
 */
int64_t
cardano_uplc_two_arg_cost_eval(const cardano_uplc_two_arg_cost_t* fn, int64_t x, int64_t y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_ARG_COST_H */
