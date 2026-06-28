/**
 * \file uplc_builtin_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_builtin_cost_arity.h"
#include "uplc_four_arg_cost.h"
#include "uplc_one_arg_cost.h"
#include "uplc_six_arg_cost.h"
#include "uplc_three_arg_cost.h"
#include "uplc_two_arg_cost.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The cpu and mem costing functions of a single builtin.
 *
 * The \c arity selects which member of \c cpu and \c mem is active; both members
 * always have the same arity for a given builtin.
 */
typedef struct cardano_uplc_builtin_cost_t
{
  /** \brief The arity selecting the active union member of \c cpu and \c mem. */
  cardano_uplc_builtin_cost_arity_t arity;

  /** \brief The cpu costing function for the builtin. */
  union
  {
    /** \brief Active when arity is ONE. */
    cardano_uplc_one_arg_cost_t one;
    /** \brief Active when arity is TWO. */
    cardano_uplc_two_arg_cost_t two;
    /** \brief Active when arity is THREE. */
    cardano_uplc_three_arg_cost_t three;
    /** \brief Active when arity is FOUR. */
    cardano_uplc_four_arg_cost_t four;
    /** \brief Active when arity is SIX. */
    cardano_uplc_six_arg_cost_t six;
  } cpu;

  /** \brief The mem costing function for the builtin. */
  union
  {
    /** \brief Active when arity is ONE. */
    cardano_uplc_one_arg_cost_t one;
    /** \brief Active when arity is TWO. */
    cardano_uplc_two_arg_cost_t two;
    /** \brief Active when arity is THREE. */
    cardano_uplc_three_arg_cost_t three;
    /** \brief Active when arity is FOUR. */
    cardano_uplc_four_arg_cost_t four;
    /** \brief Active when arity is SIX. */
    cardano_uplc_six_arg_cost_t six;
  } mem;
} cardano_uplc_builtin_cost_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COST_H */
