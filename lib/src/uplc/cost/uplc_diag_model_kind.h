/**
 * \file uplc_diag_model_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_DIAG_MODEL_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_DIAG_MODEL_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Selects which nested model a const-above/below-diagonal arm evaluates.
 *
 * Off the diagonal the cost model wraps an arbitrary two-argument model; only
 * three concrete shapes occur in the default V1-V4 cost tables, so the nested
 * model is a small closed union rather than a boxed recursive type. The arms
 * cover linear-in-x-and-y (used by valueContains), multiplied-sizes (used by
 * V1/V2 divide/mod/quotient/remainder) and the two-variable quadratic.
 */
typedef enum
{
  /** \brief Off-diagonal model is a two-variable linear cost. */
  CARDANO_UPLC_DIAG_MODEL_LINEAR_IN_X_AND_Y = 0,
  /** \brief Off-diagonal model is a multiplied-sizes linear cost over x*y. */
  CARDANO_UPLC_DIAG_MODEL_MULTIPLIED_SIZES = 1,
  /** \brief Off-diagonal model is a two-variable quadratic cost. */
  CARDANO_UPLC_DIAG_MODEL_QUADRATIC = 2
} cardano_uplc_diag_model_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_DIAG_MODEL_KIND_H */
