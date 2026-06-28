/**
 * \file uplc_two_arg_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_ARG_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_ARG_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Discriminator for a two-argument costing function.
 *
 * The \c DROP_LIST arm is the V4 dropList shape, which charges the linear
 * intercept only.
 */
typedef enum
{
  /** \brief A fixed cost independent of both arguments. */
  CARDANO_UPLC_TWO_ARG_CONSTANT = 0,
  /** \brief Linear in x. */
  CARDANO_UPLC_TWO_ARG_LINEAR_IN_X = 1,
  /** \brief Linear in y. */
  CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y = 2,
  /** \brief Linear in x and y with separate slopes. */
  CARDANO_UPLC_TWO_ARG_LINEAR_IN_X_AND_Y = 3,
  /** \brief Bilinear with an x*y interaction term. */
  CARDANO_UPLC_TWO_ARG_WITH_INTERACTION = 4,
  /** \brief Linear in x + y. */
  CARDANO_UPLC_TWO_ARG_ADDED_SIZES = 5,
  /** \brief Linear in the floored difference x - y. */
  CARDANO_UPLC_TWO_ARG_SUBTRACTED_SIZES = 6,
  /** \brief Linear in x * y. */
  CARDANO_UPLC_TWO_ARG_MULTIPLIED_SIZES = 7,
  /** \brief Linear in min(x, y). */
  CARDANO_UPLC_TWO_ARG_MIN_SIZE = 8,
  /** \brief Linear in max(x, y). */
  CARDANO_UPLC_TWO_ARG_MAX_SIZE = 9,
  /** \brief Linear on the diagonal, constant off it. */
  CARDANO_UPLC_TWO_ARG_LINEAR_ON_DIAGONAL = 10,
  /** \brief Constant when x < y, nested model otherwise. */
  CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL = 11,
  /** \brief Nested model evaluated at (max(x,y), min(x,y)). */
  CARDANO_UPLC_TWO_ARG_ABOVE_AND_BELOW_DIAGONAL = 12,
  /** \brief Constant when x > y, nested model otherwise. */
  CARDANO_UPLC_TWO_ARG_CONST_BELOW_DIAGONAL = 13,
  /** \brief Quadratic in y. */
  CARDANO_UPLC_TWO_ARG_QUADRATIC_IN_Y = 14,
  /** \brief Two-variable quadratic in x and y. */
  CARDANO_UPLC_TWO_ARG_QUADRATIC_IN_X_AND_Y = 15,
  /** \brief Constant when x < y, two-variable quadratic otherwise. */
  CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL_INTO_QUADRATIC = 16,
  /** \brief Charges the linear intercept only (V4 dropList). */
  CARDANO_UPLC_TWO_ARG_DROP_LIST = 17
} cardano_uplc_two_arg_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_TWO_ARG_KIND_H */
