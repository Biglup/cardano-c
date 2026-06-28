/**
 * \file uplc_selected_cost_model.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SELECTED_COST_MODEL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SELECTED_COST_MODEL_H

/* INCLUDES ******************************************************************/

#include "../ast/uplc_lang_version.h"
#include "../builtins/uplc_builtin_semantics.h"
#include "uplc_cost_model.h"
#include "uplc_cost_model_version.h"

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A selected cost model paired with its builtin semantics variant.
 *
 * The phase-2 driver resolves one of these per Plutus script from the script's
 * language version, the protocol major version and the ledger cost model
 * parameters, then hands both halves to the CEK machine: the structured
 * \c model drives the per-step and per-builtin costs and the \c semantics variant
 * drives the version-dependent builtin behaviours (utf-8 string costing and the
 * \c consByteString range check). Pairing them keeps the two selections, which are
 * made from the same inputs, from drifting apart.
 */
typedef struct cardano_uplc_selected_cost_model_t
{
    /** \brief The structured cost model built for the language version. */
    cardano_uplc_cost_model_t model;
    /** \brief The builtin semantics variant for the language and protocol. */
    cardano_uplc_builtin_semantics_t semantics;
} cardano_uplc_selected_cost_model_t;

/**
 * \brief Maps a Plutus language version to its cost-model parameter ordering.
 *
 * V1 and V2 map to their own orderings; V3 and V4 share the V3 ordering, as
 * there is no separate V4 parameter list yet. This is the version that selects
 * the parameter count and per-builtin shapes consumed by
 * \ref cardano_uplc_cost_model_from_params.
 *
 * \param[in] lang_version The language version to map.
 *
 * \return The matching cost-model version. An unrecognized language version maps
 *         to \ref CARDANO_UPLC_COST_MODEL_VERSION_V3.
 */
cardano_uplc_cost_model_version_t
cardano_uplc_cost_model_version_for_language(cardano_uplc_lang_version_t lang_version);

/**
 * \brief Selects the cost model and builtin semantics for a script.
 *
 * Builds the structured cost model from the flat ordered ledger parameter list for
 * the language version's parameter ordering, and selects the builtin semantics
 * variant from the language version and protocol major version
 * (\ref cardano_uplc_builtin_semantics_for_language_and_protocol). Both selections
 * are made from the same inputs and returned together so the driver and the CEK
 * machine read one consistent pair. Consensus-critical: a wrong semantics variant
 * or cost model changes the validation outcome.
 *
 * \param[in] lang_version The script's language version, selecting the parameter
 *            ordering and (with \p protocol_major) the semantics variant.
 * \param[in] protocol_major The protocol major version, selecting the semantics
 *            variant at the Chang and Van-Rossem boundaries.
 * \param[in] params The flat ordered cost-model parameter list. Must not be NULL.
 * \param[in] count The number of parameters in \p params. Must equal the parameter
 *            count for the language version's cost-model ordering.
 * \param[out] out On success, set to the selected cost model and semantics; left
 *             untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p params or \p out is NULL, or \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p count is not the language version's parameter count.
 */
cardano_error_t
cardano_uplc_select_cost_model(
  cardano_uplc_lang_version_t         lang_version,
  uint64_t                            protocol_major,
  const int64_t*                      params,
  size_t                              count,
  cardano_uplc_selected_cost_model_t* out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_SELECTED_COST_MODEL_H */
