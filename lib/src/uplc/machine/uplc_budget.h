/**
 * \file uplc_budget.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_BUDGET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_BUDGET_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A CPU and memory budget for evaluation.
 *
 * Both components are signed 64-bit and are allowed to go negative: a charge that
 * drives either below zero is how the machine detects an exhausted script. A
 * budget passed to \ref cardano_uplc_evaluate is the ceiling the script may
 * consume; the spent budget reported back is how much was actually charged.
 */
typedef struct cardano_uplc_budget_t
{
    /** \brief CPU units. */
    int64_t cpu;
    /** \brief Memory units. */
    int64_t mem;
} cardano_uplc_budget_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_BUDGET_H */
