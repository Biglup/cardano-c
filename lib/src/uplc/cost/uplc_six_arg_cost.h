/**
 * \file uplc_six_arg_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SIX_ARG_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SIX_ARG_COST_H

/* INCLUDES ******************************************************************/

#include "uplc_six_arg_kind.h"

#include <stddef.h>
#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A six-argument costing function as a tagged union.
 *
 * The active union member is selected by \c kind. Evaluate with
 * \ref cardano_uplc_six_arg_cost_eval.
 */
typedef struct cardano_uplc_six_arg_cost_t
{
    /** \brief Selects the active union member. */
    cardano_uplc_six_arg_kind_t kind;

    /** \brief The variant parameters. */
    // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    union
    {
        /** \brief Active for CONSTANT. */
        int64_t constant;
        // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    } params;
} cardano_uplc_six_arg_cost_t;

/**
 * \brief Evaluates a six-argument costing function for a non-NULL pointer.
 *
 * The six sizes are accepted for symmetry; the only shape is the constant cost.
 *
 * \param[in] fn The costing function, or NULL.
 * \param[in] a The ex-mem size of the first argument.
 * \param[in] b The ex-mem size of the second argument.
 * \param[in] c The ex-mem size of the third argument.
 * \param[in] d The ex-mem size of the fourth argument.
 * \param[in] e The ex-mem size of the fifth argument.
 * \param[in] f The ex-mem size of the sixth argument.
 *
 * \return The saturating cost, or 0 if \p fn is NULL.
 */
static inline int64_t
cardano_uplc_six_arg_eval(
  const cardano_uplc_six_arg_cost_t* fn,
  int64_t                            a,
  int64_t                            b,
  int64_t                            c,
  int64_t                            d,
  int64_t                            e,
  int64_t                            f)
{
  int64_t result = 0;

  (void)a;
  (void)b;
  (void)c;
  (void)d;
  (void)e;
  (void)f;

  if (fn == NULL)
  {
    return result;
  }

  switch (fn->kind)
  {
    case CARDANO_UPLC_SIX_ARG_CONSTANT:
    {
      result = fn->params.constant;
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
 * \brief Evaluates a six-argument costing function.
 *
 * The six sizes are accepted for symmetry; the only shape is the constant cost.
 *
 * \param[in] fn The costing function. Must not be NULL.
 * \param[in] a The ex-mem size of the first argument.
 * \param[in] b The ex-mem size of the second argument.
 * \param[in] c The ex-mem size of the third argument.
 * \param[in] d The ex-mem size of the fourth argument.
 * \param[in] e The ex-mem size of the fifth argument.
 * \param[in] f The ex-mem size of the sixth argument.
 *
 * \return The saturating cost, or 0 if \p fn is NULL.
 */
int64_t
cardano_uplc_six_arg_cost_eval(
  const cardano_uplc_six_arg_cost_t* fn,
  int64_t                            a,
  int64_t                            b,
  int64_t                            c,
  int64_t                            d,
  int64_t                            e,
  int64_t                            f);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SIX_ARG_COST_H */
