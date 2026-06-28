/**
 * \file uplc_frame_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_FRAME_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_FRAME_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The kind of a CEK continuation frame.
 *
 * Selects the active arm of \ref cardano_uplc_frame_t. The set is the six
 * continuation frames plus the empty frame.
 */
typedef enum
{
  /** \brief No continuation remains; returning here ends evaluation. */
  CARDANO_UPLC_FRAME_NO_FRAME = 0,
  /** \brief A function value is held, waiting to be applied to an argument value. */
  CARDANO_UPLC_FRAME_AWAIT_ARG = 1,
  /** \brief A function value is held, the argument term still has to be computed. */
  CARDANO_UPLC_FRAME_AWAIT_FUN_TERM = 2,
  /** \brief An argument value is held, the function value still has to be computed. */
  CARDANO_UPLC_FRAME_AWAIT_FUN_VALUE = 3,
  /** \brief A delayed value is being forced. */
  CARDANO_UPLC_FRAME_FORCE = 4,
  /** \brief Constructor fields are being computed one by one. */
  CARDANO_UPLC_FRAME_CONSTR = 5,
  /** \brief A scrutinee is being matched against case branches. */
  CARDANO_UPLC_FRAME_CASES = 6
} cardano_uplc_frame_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_FRAME_KIND_H */
