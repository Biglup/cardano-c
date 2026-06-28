/**
 * \file uplc_subtracted_sizes_cost.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SUBTRACTED_SIZES_COST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SUBTRACTED_SIZES_COST_H

/* INCLUDES ******************************************************************/

#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A subtracted-sizes cost: \c slope * max(\c minimum, x - y) + \c intercept.
 *
 * The \c minimum floors the difference before the linear term is applied.
 */
typedef struct cardano_uplc_subtracted_sizes_cost_t
{
    /** \brief The constant term. */
    int64_t intercept;
    /** \brief The coefficient of the floored difference. */
    int64_t slope;
    /** \brief The lower bound applied to x - y before scaling. */
    int64_t minimum;
} cardano_uplc_subtracted_sizes_cost_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SUBTRACTED_SIZES_COST_H */
