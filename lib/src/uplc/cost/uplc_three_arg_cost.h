/**
 * \file uplc_three_arg_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_THREE_ARG_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_THREE_ARG_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_exp_mod_cost.h"
#include "uplc_linear_cost.h"
#include "uplc_quadratic_cost.h"
#include "uplc_three_arg_kind.h"
#include "uplc_two_var_linear_cost.h"

#include <stddef.h>
#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A three-argument costing function as a tagged union.
 *
 * The active union member is selected by \c kind. Evaluate with
 * \ref cardano_uplc_three_arg_cost_eval.
 */
typedef struct cardano_uplc_three_arg_cost_t
{
    /** \brief Selects the active union member. */
    cardano_uplc_three_arg_kind_t kind;

    /** \brief The variant parameters. */
    union
    {
        /** \brief Active for CONSTANT. */
        int64_t constant;
        /** \brief Active for ADDED_SIZES, LINEAR_IN_X, LINEAR_IN_Y, LINEAR_IN_Z, LITERAL_IN_Y_OR_LINEAR_IN_Z, LINEAR_IN_MAX_YZ. */
        cardano_uplc_linear_cost_t linear;
        /** \brief Active for QUADRATIC_IN_Z. */
        cardano_uplc_quadratic_cost_t quadratic_in_z;
        /** \brief Active for EXP_MOD. */
        cardano_uplc_exp_mod_cost_t exp_mod;
        /** \brief Active for LINEAR_IN_Y_AND_Z. */
        cardano_uplc_two_var_linear_cost_t linear_in_y_and_z;
    } params;
} cardano_uplc_three_arg_cost_t;

/**
 * \brief Evaluates a non-NULL three-argument costing function at three sizes.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the first argument.
 * \param[in] y The ex-mem size of the second argument.
 * \param[in] z The ex-mem size of the third argument.
 *
 * \return The saturating cost.
 */
static inline int64_t
cardano_uplc_three_arg_eval(const cardano_uplc_three_arg_cost_t* fn, int64_t x, int64_t y, int64_t z)
{
  int64_t result = 0;

  if (fn == NULL)
  {
    return result;
  }

  switch (fn->kind)
  {
    case CARDANO_UPLC_THREE_ARG_CONSTANT:
    {
      result = fn->params.constant;
      break;
    }
    case CARDANO_UPLC_THREE_ARG_ADDED_SIZES:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, sat_add(sat_add(x, y), z));
      break;
    }
    case CARDANO_UPLC_THREE_ARG_LINEAR_IN_X:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, x);
      break;
    }
    case CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, y);
      break;
    }
    case CARDANO_UPLC_THREE_ARG_LINEAR_IN_Z:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, z);
      break;
    }
    case CARDANO_UPLC_THREE_ARG_QUADRATIC_IN_Z:
    {
      result = cardano_uplc_quadratic_cost_eval(fn->params.quadratic_in_z, z);
      break;
    }
    case CARDANO_UPLC_THREE_ARG_EXP_MOD:
    {
      cardano_uplc_exp_mod_cost_t e    = fn->params.exp_mod;
      int64_t                     yz   = sat_mul(y, z);
      int64_t                     base = sat_add(e.coeff_00, sat_mul(e.coeff_11, yz));

      base = sat_add(base, sat_mul(e.coeff_12, sat_mul(yz, z)));

      result = (x <= z) ? base : sat_add(base, base / 2);
      break;
    }
    case CARDANO_UPLC_THREE_ARG_LITERAL_IN_Y_OR_LINEAR_IN_Z:
    {
      if (y == 0)
      {
        result = cardano_uplc_linear_cost_eval(fn->params.linear, z);
      }
      else
      {
        result = y;
      }
      break;
    }
    case CARDANO_UPLC_THREE_ARG_LINEAR_IN_MAX_YZ:
    {
      result = cardano_uplc_linear_cost_eval(fn->params.linear, max_i64(y, z));
      break;
    }
    case CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y_AND_Z:
    {
      result = cardano_uplc_two_var_linear_cost_eval(fn->params.linear_in_y_and_z, y, z);
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
 * \brief Evaluates a three-argument costing function at three sizes.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] x The ex-mem size of the first argument.
 * \param[in] y The ex-mem size of the second argument.
 * \param[in] z The ex-mem size of the third argument.
 *
 * \return The saturating cost, or 0 if \p fn is NULL.
 */
int64_t
cardano_uplc_three_arg_cost_eval(const cardano_uplc_three_arg_cost_t* fn, int64_t x, int64_t y, int64_t z);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_THREE_ARG_COST_H */
