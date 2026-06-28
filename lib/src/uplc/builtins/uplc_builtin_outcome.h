/**
 * \file uplc_builtin_outcome.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_OUTCOME_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_OUTCOME_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The outcome of running a saturated builtin.
 *
 * The builtin runtime distinguishes three script-visible results plus a host
 * failure that is reported separately through a \ref cardano_error_t out-param.
 * A successful run yields a value; a type mismatch (or any other Plutus failure a
 * body raises) is a script error that fails the script; an unimplemented builtin
 * is reported as unsupported so the caller can tell "this VM build does not run
 * this builtin yet" apart from "the script failed".
 */
typedef enum
{
  /** \brief The body ran and produced a result value. */
  CARDANO_UPLC_BUILTIN_OUTCOME_OK = 0,
  /** \brief The body raised a Plutus failure (type mismatch, etc.); the script fails. */
  CARDANO_UPLC_BUILTIN_OUTCOME_SCRIPT_ERROR = 1,
  /** \brief The builtin body is not implemented in this VM build yet. */
  CARDANO_UPLC_BUILTIN_OUTCOME_UNSUPPORTED = 2
} cardano_uplc_int_builtin_outcome_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_OUTCOME_H */
