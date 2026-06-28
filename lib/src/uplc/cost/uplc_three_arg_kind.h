/**
 * \file uplc_three_arg_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_THREE_ARG_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_THREE_ARG_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Discriminator for a three-argument costing function.
 */
typedef enum
{
  /** \brief A fixed cost independent of all arguments. */
  CARDANO_UPLC_THREE_ARG_CONSTANT = 0,
  /** \brief Linear in x + y + z. */
  CARDANO_UPLC_THREE_ARG_ADDED_SIZES = 1,
  /** \brief Linear in x. */
  CARDANO_UPLC_THREE_ARG_LINEAR_IN_X = 2,
  /** \brief Linear in y. */
  CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y = 3,
  /** \brief Linear in z. */
  CARDANO_UPLC_THREE_ARG_LINEAR_IN_Z = 4,
  /** \brief Quadratic in z. */
  CARDANO_UPLC_THREE_ARG_QUADRATIC_IN_Z = 5,
  /** \brief The exp-mod costing function. */
  CARDANO_UPLC_THREE_ARG_EXP_MOD = 6,
  /** \brief y when y != 0, else linear in z. */
  CARDANO_UPLC_THREE_ARG_LITERAL_IN_Y_OR_LINEAR_IN_Z = 7,
  /** \brief Linear in max(y, z). */
  CARDANO_UPLC_THREE_ARG_LINEAR_IN_MAX_YZ = 8,
  /** \brief Linear in y and z with separate slopes. */
  CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y_AND_Z = 9
} cardano_uplc_three_arg_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_THREE_ARG_KIND_H */
