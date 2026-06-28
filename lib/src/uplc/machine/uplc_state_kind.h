/**
 * \file uplc_state_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_STATE_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_STATE_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The phase of a single CEK step.
 *
 * The machine alternates between computing a term in an environment and
 * returning a computed value into the continuation, ending when the value is
 * returned into the empty frame.
 */
typedef enum
{
  /** \brief Evaluate a term in an environment under a continuation. */
  CARDANO_UPLC_STATE_COMPUTE = 0,
  /** \brief Apply the continuation to an already-computed value. */
  CARDANO_UPLC_STATE_RETURN = 1,
  /** \brief Evaluation is complete; the result term is available. */
  CARDANO_UPLC_STATE_DONE = 2
} cardano_uplc_state_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_STATE_KIND_H */
