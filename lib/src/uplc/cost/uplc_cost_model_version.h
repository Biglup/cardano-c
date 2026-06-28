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
 * \brief The Plutus language version that selects a default machine-cost table.
 *
 * V1 and V2 share one table in which constr and case carry a prohibitive
 * sentinel cost because those step kinds are not available; V3 enables them at
 * the normal cost. The table is a plain value so a caller may replace it
 * wholesale with one built from ledger parameters.
 */
typedef enum
{
  /** \brief Plutus V1: constr/case unavailable (sentinel cost). */
  CARDANO_UPLC_COST_MODEL_VERSION_V1 = 0,
  /** \brief Plutus V2: constr/case unavailable (sentinel cost). */
  CARDANO_UPLC_COST_MODEL_VERSION_V2 = 1,
  /** \brief Plutus V3: constr/case available at the normal cost. */
  CARDANO_UPLC_COST_MODEL_VERSION_V3 = 2
} cardano_uplc_cost_model_version_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_MODEL_VERSION_H */
