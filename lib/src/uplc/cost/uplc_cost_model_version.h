/**
 * \file uplc_cost_model_version.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_MODEL_VERSION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_MODEL_VERSION_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The Plutus language version that selects a cost-model parameter ordering
 *        and its default builtin costs.
 *
 * Every version prices the machine steps alike, including constr and case. The
 * versions differ in their builtin costs and parameter lists; V4 reads the same
 * parameter list as V3. The resulting cost model is a plain value so a caller may
 * replace it wholesale with one built from ledger parameters.
 */
typedef enum
{
  /** \brief Plutus V1. */
  CARDANO_UPLC_COST_MODEL_VERSION_V1 = 0,
  /** \brief Plutus V2. */
  CARDANO_UPLC_COST_MODEL_VERSION_V2 = 1,
  /** \brief Plutus V3. */
  CARDANO_UPLC_COST_MODEL_VERSION_V3 = 2,
  /** \brief Plutus V4: the V3 parameter list. */
  CARDANO_UPLC_COST_MODEL_VERSION_V4 = 3
} cardano_uplc_cost_model_version_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_MODEL_VERSION_H */
