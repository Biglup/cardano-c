/**
 * \file uplc_four_arg_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_FOUR_ARG_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_FOUR_ARG_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Discriminator for a four-argument costing function.
 *
 * The \c LINEAR_IN_U arm is the V4 four-argument linear-in-u shape.
 */
typedef enum
{
  /** \brief A fixed cost independent of all four arguments. */
  CARDANO_UPLC_FOUR_ARG_CONSTANT = 0,
  /** \brief Linear in the fourth argument. */
  CARDANO_UPLC_FOUR_ARG_LINEAR_IN_U = 1
} cardano_uplc_four_arg_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_FOUR_ARG_KIND_H */
