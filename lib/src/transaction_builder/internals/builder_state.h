/**
 * \file builder_state.h
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_STATE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_STATE_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>
#include <cardano/transaction_builder/balancing/input_to_redeemer_map.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/typedefs.h>

#include "blake2b_hash_to_redeemer_map.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Holds everything a transaction builder tracks while assembling a transaction.
 *
 * The `cardano_builder_state_t` structure carries the transaction under construction together
 * with the protocol parameters, selection strategies, addresses, UTXO lists, script language
 * flags and redeemer bookkeeping that the builder consults when balancing and finalizing the
 * transaction.
 */
typedef struct cardano_builder_state_t
{
    /** \brief The transaction being incrementally assembled by the builder. */
    cardano_transaction_t* transaction;

    /** \brief The protocol parameters used for fee, deposit and size computations. */
    cardano_protocol_parameters_t* params;

    /** \brief The slot timing configuration used to convert between Unix time and slots. */
    cardano_slot_config_t slot_config;

    /** \brief The coin selection strategy used to pick inputs during balancing. */
    cardano_coin_selector_t* coin_selector;

    /** \brief The evaluator used to compute execution units for Plutus scripts. */
    cardano_tx_evaluator_t* tx_evaluator;

    /** \brief The address that receives any change produced while balancing. */
    cardano_address_t* change_address;

    /** \brief The address that receives any collateral change output. */
    cardano_address_t* collateral_address;

    /** \brief The UTXO set available to the coin selector for input selection. */
    cardano_utxo_list_t* available_utxos;

    /** \brief The UTXO set available for collateral selection. */
    cardano_utxo_list_t* collateral_utxos;

    /** \brief The inputs explicitly added by the caller that must be spent by the transaction. */
    cardano_utxo_list_t* pre_selected_inputs;

    /** \brief The reference inputs added to the transaction. */
    cardano_utxo_list_t* reference_inputs;

    /** \brief Whether the transaction uses at least one Plutus V1 script. */
    bool has_plutus_v1;

    /** \brief Whether the transaction uses at least one Plutus V2 script. */
    bool has_plutus_v2;

    /** \brief Whether the transaction uses at least one Plutus V3 script. */
    bool has_plutus_v3;

    /** \brief The number of additional signatures accounted for when computing fees. */
    size_t additional_signature_count;

    /** \brief Maps transaction inputs to their spend redeemers. */
    cardano_input_to_redeemer_map_t* input_to_redeemer_map;

    /** \brief Maps reward credential hashes to their withdrawal redeemers. */
    cardano_blake2b_hash_to_redeemer_map_t* withdrawals_to_redeemer_map;

    /** \brief Maps minting policy hashes to their mint redeemers. */
    cardano_blake2b_hash_to_redeemer_map_t* mints_to_redeemer_map;

    /** \brief Maps voter credential hashes to their voting redeemers. */
    cardano_blake2b_hash_to_redeemer_map_t* votes_to_redeemer_map;

    /** \brief The redeemers whose indices are resolved when the transaction is balanced. */
    cardano_deferred_redeemer_list_t* deferred_redeemers;
} cardano_builder_state_t;

/**
 * \brief Initializes a builder state with its default contents.
 *
 * This function initializes every field of the given state, takes a reference on the provided
 * protocol parameters, copies the slot configuration and creates the objects the builder starts
 * from: a random improve coin selector, an empty transaction, the pre selected and reference
 * input lists, the redeemer maps, the deferred redeemer list and a native transaction evaluator
 * configured from the protocol parameters cost models and protocol version.
 *
 * \param[out] state A pointer to the \ref cardano_builder_state_t to initialize. This parameter
 *                   must not be NULL.
 * \param[in] params A pointer to the \ref cardano_protocol_parameters_t used by the builder. The
 *                   state holds a reference to this object until it is released. This parameter
 *                   must not be NULL.
 * \param[in] slot_config A pointer to the \ref cardano_slot_config_t copied into the state. This
 *                        parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the state was fully initialized, or an appropriate error code
 *         indicating the failure reason. On failure the state may be partially initialized and
 *         the caller must release it with \ref cardano_builder_state_release.
 */
cardano_error_t
cardano_builder_state_init(
  cardano_builder_state_t*       state,
  cardano_protocol_parameters_t* params,
  const cardano_slot_config_t*   slot_config);

/**
 * \brief Releases every object held by a builder state.
 *
 * This function releases the references the state holds on the transaction, protocol parameters,
 * coin selector, transaction evaluator, addresses, UTXO lists, redeemer maps and deferred
 * redeemer list, setting each pointer field to NULL. It does not free the state itself. Calling
 * it on an already released state or with NULL has no effect.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t to release.
 */
void
cardano_builder_state_release(cardano_builder_state_t* state);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_STATE_H
