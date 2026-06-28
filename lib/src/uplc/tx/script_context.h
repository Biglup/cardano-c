/**
 * \file script_context.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_TX_SCRIPT_CONTEXT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_TX_SCRIPT_CONTEXT_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/transaction.h>
#include <cardano/witness_set/redeemer.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Converts an absolute slot to its beginning POSIX time, in milliseconds.
 *
 * Computes \c zero_time + (slot - zero_slot) * slot_length, the inclusive start of
 * the slot. A slot earlier than \c zero_slot has no representable time and is
 * rejected. This is the single implementation of the conversion used to derive the
 * validity-interval bounds of the script-context \c TxInfo from the transaction's
 * validity slots, and is consensus-critical: a wrong time changes which validity
 * range a script sees. A slot before the slot-config origin is rejected.
 *
 * \param[in] slot_config The slot configuration (\c zero_time, \c zero_slot and
 *            \c slot_length). Must not be NULL.
 * \param[in] slot The absolute slot number to convert.
 * \param[out] posix_time On success, set to the beginning POSIX time in
 *             milliseconds; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p slot_config or \p posix_time is NULL, or
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT if \p slot is earlier than
 *         \c zero_slot.
 */
cardano_error_t
cardano_uplc_int_slot_to_posix_time(
  const cardano_slot_config_t* slot_config,
  uint64_t                     slot,
  uint64_t*                    posix_time);

/**
 * \brief Builds the Plutus V1 TxInfo of a transaction as Plutus data.
 *
 * The encoding is the V1 TxInfo plutus-data: a constructor with index 0 whose
 * fields are (inputs, outputs, fee, mint, dcert, withdrawals, valid range,
 * signatories, datums, transaction id). Inputs and outputs carry the legacy
 * datum-hash-only output shape and the wrapped transaction id.
 *
 * \param[in] tx The transaction to translate. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving every spent input. Must not
 *            be NULL. A reference-input section in \p tx is rejected, matching
 *            the V1 ledger rules.
 * \param[in] slot_config The slot/time parameters used to convert the validity
 *            interval slots to POSIX time. Must not be NULL.
 * \param[out] tx_info On success, the newly created TxInfo data. The caller owns
 *             it and must release it with \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_tx_info_v1(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t**      tx_info);

/**
 * \brief Builds the Plutus V2 TxInfo of a transaction as Plutus data.
 *
 * As \ref cardano_uplc_int_build_tx_info_v1, but for the V2 shape: it adds the
 * reference inputs, keeps inline datums and reference scripts in outputs,
 * carries the redeemers as a map keyed by script purpose, and exposes the full
 * datum witnesses as a map. The constructor index is 0.
 *
 * \param[in] tx The transaction to translate. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving inputs and reference inputs.
 *            Must not be NULL.
 * \param[in] slot_config The slot/time parameters. Must not be NULL.
 * \param[out] tx_info On success, the newly created TxInfo data, owned by the
 *             caller.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_tx_info_v2(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t**      tx_info);

/**
 * \brief Builds the script purpose of a redeemer as Plutus data, in the V1/V2 shape.
 *
 * The purpose is resolved by looking up the redeemer tag and index against the
 * sorted inputs, mint policies, certificates and withdrawals of \p tx, then
 * encoded as the wrapped-transaction-id ScriptPurpose: Minting =>
 * Constr 0 [policy id], Spending => Constr 1 [out ref], Rewarding => Constr 2
 * [wrapped stake credential], Certifying => Constr 3 [certificate].
 *
 * \param[in] tx The transaction. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving the inputs. Must not be NULL.
 * \param[in] redeemer The redeemer whose purpose is requested. Must not be NULL.
 * \param[out] purpose On success, the newly created purpose data, owned by the
 *             caller.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_script_purpose_v1v2(
  cardano_transaction_t*     tx,
  const cardano_utxo_list_t* resolved_inputs,
  cardano_redeemer_t*        redeemer,
  cardano_plutus_data_t**    purpose);

/**
 * \brief Builds the complete Plutus V1 ScriptContext of a redeemer as Plutus data.
 *
 * The result is the V1/V2 ScriptContext plutus-data: a constructor with index 0
 * whose fields are the V1 TxInfo and the script purpose.
 *
 * \param[in] tx The transaction. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving the inputs. Must not be NULL.
 * \param[in] slot_config The slot/time parameters. Must not be NULL.
 * \param[in] redeemer The redeemer for which the context is built. Must not be NULL.
 * \param[out] script_context On success, the newly created ScriptContext data,
 *             owned by the caller.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_script_context_v1(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t**      script_context);

/**
 * \brief Builds the complete Plutus V2 ScriptContext of a redeemer as Plutus data.
 *
 * As \ref cardano_uplc_int_build_script_context_v1 but using the V2 TxInfo.
 *
 * \param[in] tx The transaction. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving the inputs. Must not be NULL.
 * \param[in] slot_config The slot/time parameters. Must not be NULL.
 * \param[in] redeemer The redeemer for which the context is built. Must not be NULL.
 * \param[out] script_context On success, the newly created ScriptContext data,
 *             owned by the caller.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_script_context_v2(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t**      script_context);

/**
 * \brief Builds the Plutus V3 (Conway) TxInfo of a transaction as Plutus data.
 *
 * The encoding is the V3 TxInfo plutus-data. Compared to V2, V3 uses the
 * unwrapped transaction id in
 * inputs and the top-level id, encodes the fee as a bare lovelace integer
 * (rather than a value map), encodes the mint as a signed-amount value without
 * the synthetic zero-ada entry, keeps the withdrawals keyed by the full address
 * (not a wrapped stake credential), encodes the certificates and the redeemer
 * purposes in the Conway never-registration-deposit shape, and adds the
 * Conway governance fields: votes, proposal procedures, the current treasury
 * amount and the treasury donation.
 *
 * The constructor index is 0 and the field order is: inputs, reference inputs,
 * outputs, fee, mint, certificates, withdrawals, validity range, signatories,
 * redeemers, datums, transaction id, votes, proposal procedures, current
 * treasury amount, treasury donation.
 *
 * \param[in] tx The transaction to translate. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving inputs and reference inputs.
 *            Must not be NULL.
 * \param[in] slot_config The slot/time parameters. Must not be NULL.
 * \param[out] tx_info On success, the newly created TxInfo data, owned by the
 *             caller.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_tx_info_v3(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t**      tx_info);

/**
 * \brief Builds the V3 ScriptInfo of a redeemer as Plutus data.
 *
 * The ScriptInfo supersedes the V1/V2 ScriptPurpose. It is resolved by looking
 * up the redeemer tag and index against the sorted inputs, mint policies,
 * certificates, withdrawals, votes and proposal procedures of \p tx, then
 * encoded as the never-registration-deposit ScriptInfo: Minting =>
 * Constr 0 [policy id], Spending => Constr 1 [out ref (unwrapped), Maybe datum],
 * Rewarding => Constr 2 [stake credential], Certifying => Constr 3 [index,
 * certificate], Voting => Constr 4 [voter], Proposing => Constr 5 [index,
 * proposal procedure].
 *
 * \param[in] tx The transaction. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving the inputs. Must not be NULL.
 * \param[in] redeemer The redeemer whose script info is requested. Must not be NULL.
 * \param[in] datum The resolved spending datum, or NULL. Only used (and encoded
 *            as a \c Maybe) for the Spending variant.
 * \param[out] script_info On success, the newly created script info data, owned
 *             by the caller.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_script_info_v3(
  cardano_transaction_t*  tx,
  cardano_utxo_list_t*    resolved_inputs,
  cardano_redeemer_t*     redeemer,
  cardano_plutus_data_t*  datum,
  cardano_plutus_data_t** script_info);

/**
 * \brief Builds the complete Plutus V3 ScriptContext of a redeemer as Plutus data.
 *
 * The result is the V3 ScriptContext plutus-data: a constructor with index 0
 * whose fields are the V3 TxInfo, the script's own redeemer data,
 * and the V3 ScriptInfo (which, for the Spending purpose, carries the optional
 * datum).
 *
 * \param[in] tx The transaction. Must not be NULL.
 * \param[in] resolved_inputs The UTxO set resolving the inputs. Must not be NULL.
 * \param[in] slot_config The slot/time parameters. Must not be NULL.
 * \param[in] redeemer The redeemer for which the context is built. Must not be NULL.
 * \param[in] datum The resolved spending datum, or NULL.
 * \param[out] script_context On success, the newly created ScriptContext data,
 *             owned by the caller.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code otherwise.
 */
cardano_error_t
cardano_uplc_int_build_script_context_v3(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t*       datum,
  cardano_plutus_data_t**      script_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_TX_SCRIPT_CONTEXT_H */
