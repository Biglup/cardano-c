/**
 * \file uplc_const_or_linear_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_CONST_OR_LINEAR_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_CONST_OR_LINEAR_COST_H

/* INCLUDES ******************************************************************/

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A diagonal cost: \c intercept + \c slope * x on the diagonal, else \c constant.
 *
 * When x equals y the linear term applies, otherwise the flat \c constant is
 * charged.
 */
typedef struct cardano_uplc_const_or_linear_cost_t
{
    /** \brief The flat cost charged off the diagonal. */
    int64_t constant;
    /** \brief The constant term of the on-diagonal linear cost. */
    int64_t intercept;
    /** \brief The slope of the on-diagonal linear cost. */
    int64_t slope;
} cardano_uplc_const_or_linear_cost_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_CONST_OR_LINEAR_COST_H */
