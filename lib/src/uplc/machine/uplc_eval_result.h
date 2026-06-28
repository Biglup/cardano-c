/**
 * \file uplc_eval_result.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_EVAL_RESULT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_EVAL_RESULT_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>
#include "uplc_budget.h"
#include "uplc_eval_status.h"
#include "../ast/uplc_term.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The result of a successful host call to \ref cardano_uplc_evaluate.
 *
 * The \c status names the script outcome. \c spent is the budget the evaluation
 * consumed (the initial budget minus what remained), reported on every outcome
 * including failure and exhaustion. \c result is the discharged result term on
 * \ref CARDANO_UPLC_EVAL_SUCCESS and NULL on every other outcome; it is allocated
 * in the arena the caller passed to \ref cardano_uplc_evaluate and stays valid as
 * long as that arena is alive.
 */
typedef struct cardano_uplc_eval_result_t
{
  /** \brief The script outcome. */
  cardano_uplc_eval_status_t status;
  /** \brief The budget consumed by the evaluation. */
  cardano_uplc_budget_t spent;
  /** \brief The discharged result term on success, NULL otherwise. */
  const cardano_uplc_term_t* result;
} cardano_uplc_eval_result_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_EVAL_RESULT_H */
