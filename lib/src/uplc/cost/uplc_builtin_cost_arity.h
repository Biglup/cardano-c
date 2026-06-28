/**
 * \file uplc_builtin_cost_arity.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COST_ARITY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COST_ARITY_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The arity of a builtin's costing function, selecting its cost shape.
 *
 * Each builtin takes its cpu and mem cost from a costing function of a fixed
 * arity (one of 1, 2, 3, 4 or 6). The arity is a property of the builtin and
 * never changes between versions; it selects which member of the cpu/mem unions
 * in \ref cardano_uplc_builtin_cost_t is active.
 */
typedef enum
{
  /** \brief A one-argument costing function. */
  CARDANO_UPLC_BUILTIN_COST_ARITY_ONE = 0,
  /** \brief A two-argument costing function. */
  CARDANO_UPLC_BUILTIN_COST_ARITY_TWO = 1,
  /** \brief A three-argument costing function. */
  CARDANO_UPLC_BUILTIN_COST_ARITY_THREE = 2,
  /** \brief A four-argument costing function. */
  CARDANO_UPLC_BUILTIN_COST_ARITY_FOUR = 3,
  /** \brief A six-argument costing function. */
  CARDANO_UPLC_BUILTIN_COST_ARITY_SIX = 4
} cardano_uplc_builtin_cost_arity_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_BUILTIN_COST_ARITY_H */
