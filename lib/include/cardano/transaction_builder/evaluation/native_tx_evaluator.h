/**
 * \file native_tx_evaluator.h
 *
 * \author angel.castillo
 * \date   Jun 20, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BUILDER_EVALUATION_NATIVE_TX_EVALUATOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BUILDER_EVALUATION_NATIVE_TX_EVALUATOR_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/slot_config.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Creates a native phase-2 transaction evaluator backed by the in-process UPLC VM.
 *
 * The native evaluator is the phase-2 driver: for every redeemer in a transaction
 * it resolves the script and (for spends) the datum, builds the version-appropriate
 * Plutus \c ScriptContext, applies the arguments, evaluates the program on the
 * CEK machine, and writes the consumed execution units back into the redeemer it
 * returns. It plugs into the existing \ref cardano_tx_evaluator_t interface
 * through a \ref cardano_tx_evaluator_impl_t, so callers use it exactly as they
 * would the provider-backed evaluator.
 *
 * The \ref cardano_tx_evaluator_evaluate signature carries no slot config, cost
 * models or protocol version, so this constructor binds them once: the slot
 * config converts validity slots to POSIX time for the \c ScriptContext, the cost
 * models and protocol major version select the builtin semantics variant per
 * script. The evaluator copies \p slot_config and takes a reference on
 * \p cost_models; both stay owned by the evaluator for its lifetime.
 *
 * \remark For each script the driver builds the structured cost model from the
 *         \p cost_models entry matching the script's language version and threads
 *         it, together with the selected builtin semantics variant, into the CEK
 *         machine's budget accounting. The returned execution units therefore
 *         reflect the supplied ledger cost model exactly: the machine charges
 *         every step and builtin against those coefficients. When \p cost_models
 *         carries no entry for a version, the per-version default coefficients for
 *         the protocol version are used instead.
 *
 * \param[in] slot_config The slot/time parameters used to convert the transaction
 *            validity interval to POSIX time. Must not be NULL.
 * \param[in] cost_models The ledger cost models keyed by Plutus language version.
 *            May be NULL, in which case the evaluator uses the per-version default
 *            semantics for the protocol version.
 * \param[in] protocol_major The protocol major version selecting the builtin
 *            semantics variant at the Chang and Van-Rossem boundaries.
 * \param[out] tx_evaluator On success, set to the newly created evaluator. The
 *             caller owns it and releases it with \ref cardano_tx_evaluator_unref.
 *             Left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p slot_config or \p tx_evaluator is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the evaluator context
 *         cannot be allocated.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_evaluator_t* evaluator = NULL;
 * cardano_error_t         result    = cardano_tx_evaluator_new_native(
 *   &CARDANO_MAINNET_SLOT_CONFIG, cost_models, 10U, &evaluator);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   cardano_redeemer_list_t* redeemers = NULL;
 *   result = cardano_tx_evaluator_evaluate(evaluator, tx, additional_utxos, &redeemers);
 *   // ... inspect redeemers' ex-units ...
 *   cardano_redeemer_list_unref(&redeemers);
 *   cardano_tx_evaluator_unref(&evaluator);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_tx_evaluator_new_native(
  const cardano_slot_config_t* slot_config,
  cardano_costmdls_t*          cost_models,
  uint64_t                     protocol_major,
  cardano_tx_evaluator_t**     tx_evaluator);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BUILDER_EVALUATION_NATIVE_TX_EVALUATOR_H */
