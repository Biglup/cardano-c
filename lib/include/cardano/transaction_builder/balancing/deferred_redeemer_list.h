/**
 * \file deferred_redeemer_list.h
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_DEFERRED_REDEEMER_LIST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_DEFERRED_REDEEMER_LIST_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction/transaction.h>
#include <cardano/typedefs.h>
#include <cardano/witness_set/redeemer.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Produces the redeemer payload for a script purpose from the balanced draft transaction.
 *
 * Deferred redeemer callbacks are the mechanism for building redeemers that depend on the final
 * canonical layout of the transaction (input indices, change outputs, the fee), which only exists
 * after balancing. See \ref cardano_tx_builder_add_input_with_deferred_redeemer.
 *
 * The callback receives the balanced draft transaction (canonical input order, outputs including
 * change, fee) and the final selected inputs as fully resolved UTXOs in canonical order. The
 * \ref cardano_transaction_find_input_index family of functions can be used to look up canonical
 * positions.
 *
 * \warning Callbacks are invoked on EVERY balancing iteration and MUST be pure, deterministic
 * functions of their arguments: no captured mutable state, no side effects, no randomness. A
 * callback that violates this contract can prevent the balancing loop from converging.
 *
 * \param[in]  user_context    The opaque pointer given at registration time.
 * \param[in]  draft_tx        The balanced draft transaction. Borrowed; must not be modified.
 * \param[in]  resolved_inputs The final selected inputs as resolved UTXOs. Borrowed.
 * \param[out] redeemer        The produced redeemer payload. The caller takes ownership.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code (which aborts balancing).
 */
typedef cardano_error_t (*cardano_deferred_redeemer_fn_t)(
  void*                   user_context,
  cardano_transaction_t*  draft_tx,
  cardano_utxo_list_t*    resolved_inputs,
  cardano_plutus_data_t** redeemer);

/**
 * \brief A list of pending deferred redeemers: witness-set redeemer objects paired with the
 * callback that produces their payload once the final transaction layout is known.
 */
typedef struct cardano_deferred_redeemer_list_t cardano_deferred_redeemer_list_t;

/**
 * \brief Creates a new empty deferred redeemer list.
 *
 * \param[out] list A pointer to store the address of the newly created list.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_deferred_redeemer_list_new(cardano_deferred_redeemer_list_t** list);

/**
 * \brief Registers a deferred redeemer.
 *
 * The list takes a reference on the redeemer object. Because the transaction builder shares the
 * same redeemer object between the witness set and its bookkeeping maps, resolving the deferred
 * payload updates the witness set in place.
 *
 * \param[in] list         The list to add to.
 * \param[in] redeemer     The witness-set redeemer object whose payload is produced by the callback.
 * \param[in] callback     The callback producing the payload. See \ref cardano_deferred_redeemer_fn_t.
 * \param[in] user_context An opaque pointer forwarded to the callback. Can be NULL. The caller
 *                         must keep it valid until the transaction is built.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_deferred_redeemer_list_add(
  cardano_deferred_redeemer_list_t* list,
  cardano_redeemer_t*               redeemer,
  cardano_deferred_redeemer_fn_t    callback,
  void*                             user_context);

/**
 * \brief Returns the number of registered deferred redeemers.
 *
 * \param[in] list The list to inspect.
 *
 * \return The number of entries, or zero if the list is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_deferred_redeemer_list_get_length(const cardano_deferred_redeemer_list_t* list);

/**
 * \brief Resolves every deferred redeemer against a balanced draft transaction.
 *
 * Invokes each registered callback with the draft transaction and the resolved inputs, and
 * replaces the corresponding redeemer's payload with the produced plutus data. This is called
 * by \ref cardano_balance_transaction on every balancing iteration, after the canonical input
 * order and the change outputs are final and before evaluation and fee computation.
 *
 * \param[in] list            The list of deferred redeemers. Can be NULL (no-op).
 * \param[in] draft_tx        The balanced draft transaction.
 * \param[in] resolved_inputs The final selected inputs as resolved UTXOs.
 *
 * \return \ref CARDANO_SUCCESS on success, or the first error returned by a callback.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_deferred_redeemer_list_resolve(
  cardano_deferred_redeemer_list_t* list,
  cardano_transaction_t*            draft_tx,
  cardano_utxo_list_t*              resolved_inputs);

/**
 * \brief Increments the reference count of the deferred redeemer list.
 *
 * \param[in] list The list whose reference count to increment.
 */
CARDANO_EXPORT void cardano_deferred_redeemer_list_ref(cardano_deferred_redeemer_list_t* list);

/**
 * \brief Decrements the reference count of the deferred redeemer list; frees it when it reaches zero.
 *
 * \param[in,out] list A pointer to the list pointer. Set to NULL.
 */
CARDANO_EXPORT void cardano_deferred_redeemer_list_unref(cardano_deferred_redeemer_list_t** list);

/**
 * \brief Returns the current reference count of the deferred redeemer list.
 *
 * \param[in] list The list to inspect.
 *
 * \return The reference count, or zero if the list is NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_deferred_redeemer_list_refcount(const cardano_deferred_redeemer_list_t* list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_DEFERRED_REDEEMER_LIST_H
