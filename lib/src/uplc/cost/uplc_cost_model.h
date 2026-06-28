/**
 * \file uplc_cost_model.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_MODEL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_MODEL_H

/* INCLUDES ******************************************************************/

#include "uplc_builtin_costs.h"
#include "uplc_cost_model_version.h"
#include "uplc_machine_costs.h"

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A full cost model: the machine-step costs and the per-builtin costs.
 *
 * Pairs the per-step machine costs with the per-builtin costing functions.
 */
typedef struct cardano_uplc_cost_model_t
{
    /** \brief The per-step machine costs. */
    cardano_uplc_machine_costs_t machine;
    /** \brief The per-builtin costing functions. */
    cardano_uplc_builtin_costs_t builtins;
} cardano_uplc_cost_model_t;

/**
 * \brief The number of flat protocol parameters in the Plutus V1 cost model.
 *
 * The V1 parameter list as ordered by the ledger has this many entries.
 */
#define CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1 ((size_t)166)

/**
 * \brief The number of flat protocol parameters in the Plutus V2 cost model.
 *
 * The V2 parameter list as ordered by the ledger has this many entries.
 */
#define CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V2 ((size_t)175)

/**
 * \brief The number of flat protocol parameters in the Plutus V3 cost model.
 *
 * The V3 list runs through the bitwise-builtin tail, the count at which the
 * machine reproduces the default V3 builtin costs.
 */
#define CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3 ((size_t)297)

/**
 * \brief Builds a full cost model from a flat ordered protocol parameter list.
 *
 * Maps the flat int64 list positionally into the machine-step costs and the
 * per-builtin costing functions, using the per-version ledger parameter
 * ordering. The shapes are fixed per version; only the coefficients come from
 * \p params.
 * Parameters absent from a version's list keep the prohibitive sentinel cost.
 * When given the version's default parameter vector this reproduces
 * \ref cardano_uplc_builtin_costs_v1, \c _v2 or \c _v3 exactly.
 *
 * \param[in] version The cost-model version selecting the parameter ordering and
 *            the per-builtin shapes.
 * \param[in] params The flat ordered parameter list. Must not be NULL.
 * \param[in] count The number of parameters in \p params. Must equal the version
 *            count (\ref CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, \c _V2 or \c _V3).
 * \param[out] out On success, set to the built cost model; left untouched on
 *             failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p params or \p out is NULL, or \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p version is unknown or \p count is not the version's parameter count.
 */
cardano_error_t
cardano_uplc_cost_model_from_params(
  cardano_uplc_cost_model_version_t version,
  const int64_t*                    params,
  size_t                            count,
  cardano_uplc_cost_model_t*        out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_COST_MODEL_H */
