/**
 * \file uplc_eval_status.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_EVAL_STATUS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_EVAL_STATUS_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The outcome of an evaluation that the host successfully ran.
 *
 * These three values describe what the script did, not whether the host could
 * run it: a host failure (null argument, allocation failure) is reported through
 * the \ref cardano_error_t return of \ref cardano_uplc_evaluate, while a script
 * that reaches the error term or exhausts its budget is a successful host call
 * that reports its outcome here.
 */
typedef enum
{
  /** \brief The machine reached Done; \c result holds the discharged term. */
  CARDANO_UPLC_EVAL_SUCCESS = 0,
  /**
   * \brief The script failed: it reached the error term, applied a non-function,
   *        looked up an out-of-range variable, matched a missing case branch, or
   *        reached an unimplemented builtin. \c result is NULL.
   */
  CARDANO_UPLC_EVAL_ERROR_TERM = 1,
  /** \brief The script exhausted its CPU or memory budget. \c result is NULL. */
  CARDANO_UPLC_EVAL_OUT_OF_BUDGET = 2,
  /**
   * \brief A saturated builtin this VM build does not implement yet was reached.
   *        \c result is NULL.
   *
   * This is NOT a script failure and NOT a \ref cardano_error_t host failure: it
   * means the VM build does not run that builtin yet, distinct from
   * \ref CARDANO_UPLC_EVAL_ERROR_TERM (the script genuinely failed). The
   * conformance runner uses it to skip exactly the cases that hit an
   * unimplemented builtin.
   */
  CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN = 3
} cardano_uplc_eval_status_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_EVAL_STATUS_H */
