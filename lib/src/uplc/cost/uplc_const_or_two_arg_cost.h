/**
 * \file uplc_const_or_two_arg_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_CONST_OR_TWO_ARG_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_CONST_OR_TWO_ARG_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_cost_sat.h"
#include "uplc_diag_model_kind.h"
#include "uplc_linear_cost.h"
#include "uplc_two_var_linear_cost.h"
#include "uplc_two_var_quadratic_cost.h"

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A const-above/below-diagonal cost: a flat constant plus a nested model.
 *
 * Off the diagonal the flat \c constant is charged; on the relevant side of the
 * diagonal the nested model selected by \c kind is evaluated over (x, y). The
 * nested model is restricted to the shapes that actually occur.
 */
typedef struct cardano_uplc_const_or_two_arg_cost_t
{
    /** \brief The flat cost charged on the constant side of the diagonal. */
    int64_t constant;
    /** \brief Selects which member of \c model is active. */
    cardano_uplc_diag_model_kind_t kind;

    /** \brief The nested off-diagonal model. */
    union
    {
        /** \brief Active when kind is LINEAR_IN_X_AND_Y. */
        cardano_uplc_two_var_linear_cost_t linear_in_x_and_y;
        /** \brief Active when kind is MULTIPLIED_SIZES. */
        cardano_uplc_linear_cost_t multiplied_sizes;
        /** \brief Active when kind is QUADRATIC. */
        cardano_uplc_two_var_quadratic_cost_t quadratic;
    } model;
} cardano_uplc_const_or_two_arg_cost_t;

/**
 * \brief Evaluates the nested off-diagonal model of a const-diagonal arm.
 *
 * Dispatches on the nested model kind and evaluates it over (x, y), restricted
 * to the shapes that occur in the default tables.
 *
 * \param[in] c The const-or-two-argument parameters.
 * \param[in] x The first size argument.
 * \param[in] y The second size argument.
 *
 * \return The saturating cost of the nested model at (x, y).
 */
static inline int64_t
cardano_uplc_const_or_two_arg_cost_eval(cardano_uplc_const_or_two_arg_cost_t c, int64_t x, int64_t y)
{
  int64_t result = 0;

  switch (c.kind)
  {
    case CARDANO_UPLC_DIAG_MODEL_LINEAR_IN_X_AND_Y:
    {
      result = cardano_uplc_two_var_linear_cost_eval(c.model.linear_in_x_and_y, x, y);
      break;
    }
    case CARDANO_UPLC_DIAG_MODEL_MULTIPLIED_SIZES:
    {
      result = cardano_uplc_linear_cost_eval(c.model.multiplied_sizes, sat_mul(x, y));
      break;
    }
    case CARDANO_UPLC_DIAG_MODEL_QUADRATIC:
    {
      result = cardano_uplc_two_var_quadratic_cost_eval(c.model.quadratic, x, y);
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_CONST_OR_TWO_ARG_COST_H */
