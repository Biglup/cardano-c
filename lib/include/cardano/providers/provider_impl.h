/**
 * \file provider_impl.h
 *
 * \author angel.castillo
 * \date   Sep 27, 2024
 *
 * Copyright 2024 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_IMPL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_IMPL_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/assets/asset_id.h>
#include <cardano/common/network_magic.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/witness_set/redeemer_list.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FORWARD DECLARATIONS *******************************************************/

/**
 * \brief Opaque structure representing the implementation details of a Cardano blockchain data provider.
 *
 * This structure encapsulates the internal state and implementation details of a Cardano blockchain data provider.
 *
 * \note Users should interact with Cardano providers through the provided \ref cardano_provider_t API functions and should
 *       not attempt to manipulate this structure directly.
 */
typedef struct cardano_provider_impl_t cardano_provider_impl_t;

/* CALLBACKS *****************************************************************/

/**
 * \brief Function pointer type for retrieving protocol parameters.
 *
 * This function retrieves the protocol parameters using the given provider implementation.
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[out] parameters    Pointer to store the retrieved protocol parameters.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_get_parameters_func_t)(
  cardano_provider_impl_t*        provider_impl,
  cardano_protocol_parameters_t** parameters);

/**
 * \brief Function pointer type for retrieving unspent outputs for an address.
 *
 * Retrieves a list of unspent transaction outputs (UTXOs) associated with the specified address.
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[in]  address       Address for which to retrieve unspent outputs.
 * \param[out] utxo_list     Pointer to store the list of unspent outputs.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_get_unspent_outputs_func_t)(
  cardano_provider_impl_t* provider_impl,
  cardano_address_t*       address,
  cardano_utxo_list_t**    utxo_list);

/**
 * \brief Function pointer type for retrieving staking rewards for an address.
 *
 * This function retrieves the current staking rewards associated with the specified address.
 * It uses the provided provider implementation instance to access blockchain data and
 * obtain the reward balance for the address. Staking rewards are accumulated for addresses
 * that delegate their stake to a stake pool.
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 *                           Must not be `NULL`.
 * \param[in]  address       Pointer to a \ref cardano_address_t representing the address
 *                           for which to retrieve staking rewards. Must not be `NULL`.
 * \param[out] rewards       Pointer to a `uint64_t` variable where the function will store
 *                           the amount of rewards (in Lovelace). Must not be `NULL`.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 */
typedef cardano_error_t (*cardano_get_rewards_balance_func_t)(
  cardano_provider_impl_t*  provider_impl,
  cardano_reward_address_t* address,
  uint64_t*                 rewards);

/**
 * \brief Function pointer type for retrieving unspent outputs for an address and specific asset.
 *
 * Retrieves a list of unspent transaction outputs (UTXOs) associated with the specified address
 * that contain the specified asset.
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[in]  address       Address for which to retrieve unspent outputs.
 * \param[in]  asset_id      Asset identifier to filter UTXOs.
 * \param[out] utxo_list     Pointer to store the list of unspent outputs containing the asset.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_get_unspent_outputs_with_asset_func_t)(
  cardano_provider_impl_t* provider_impl,
  cardano_address_t*       address,
  cardano_asset_id_t*      asset_id,
  cardano_utxo_list_t**    utxo_list);

/**
 * \brief Function pointer type for retrieving an unspent output containing a specific NFT.
 *
 * Retrieves an unspent transaction output (UTXO) that contains the specified NFT (Non-Fungible Token).
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[in]  asset_id      Asset identifier of the NFT.
 * \param[out] utxo          Pointer to store the retrieved unspent output containing the NFT.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_get_unspent_output_by_nft_func_t)(
  cardano_provider_impl_t* provider_impl,
  cardano_asset_id_t*      asset_id,
  cardano_utxo_t**         utxo);

/**
 * \brief Function pointer type for resolving unspent outputs for given transaction inputs.
 *
 * Resolves a list of unspent transaction outputs (UTXOs) corresponding to the provided transaction inputs.
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[in]  tx_ins        Set of transaction inputs to resolve.
 * \param[out] utxo_list     Pointer to store the list of resolved unspent outputs.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_resolve_unspent_outputs_func_t)(
  cardano_provider_impl_t*         provider_impl,
  cardano_transaction_input_set_t* tx_ins,
  cardano_utxo_list_t**            utxo_list);

/**
 * \brief Function pointer type for resolving a datum from its hash.
 *
 * Retrieves the Plutus datum associated with the given datum hash.
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[in]  datum_hash    Hash of the datum to resolve.
 * \param[out] datum         Pointer to store the retrieved Plutus datum.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_resolve_datum_func_t)(
  cardano_provider_impl_t* provider_impl,
  cardano_blake2b_hash_t*  datum_hash,
  cardano_plutus_data_t**  datum);

/**
 * \brief Function pointer type for confirming a transaction's inclusion in the blockchain.
 *
 * Waits for the specified transaction to be confirmed within a given timeout period.
 *
 * \param[in] provider_impl Pointer to the provider implementation instance.
 * \param[in] tx_id         Transaction ID to confirm.
 * \param[in] timeout_ms    Timeout in milliseconds to wait for confirmation.
 * \param[out] confirmed    Pointer to store the confirmation status of the transaction.
 *
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_confirm_transaction_func_t)(
  cardano_provider_impl_t* provider_impl,
  cardano_blake2b_hash_t*  tx_id,
  uint64_t                 timeout_ms,
  bool*                    confirmed);

/**
 * \brief Function pointer type for submitting a transaction to the blockchain.
 *
 * Submits the given transaction to the network and returns its transaction ID.
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[in]  tx            Transaction to submit.
 * \param[out] tx_id         Pointer to store the transaction ID after submission.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_submit_transaction_func_t)(
  cardano_provider_impl_t* provider_impl,
  cardano_transaction_t*   tx,
  cardano_blake2b_hash_t** tx_id);

/**
 * \brief Function pointer type for evaluating a transaction's execution units.
 *
 * Evaluates the execution units required by the transaction, considering any additional UTXOs and redeemers.
 *
 * \param[in] provider_impl    Pointer to the provider implementation instance.
 * \param[in] tx               Transaction to evaluate.
 * \param[in] additional_utxos Additional UTXOs required for evaluation (optional).
 * \param[in] redeemers        Redeemers to be evaluated with the transaction.
 * \return Error code indicating success or failure of the operation.
 */
typedef cardano_error_t (*cardano_evaluate_transaction_func_t)(
  cardano_provider_impl_t*  provider_impl,
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      additional_utxos,
  cardano_redeemer_list_t** redeemers);

/* STRUCTURES ****************************************************************/

/**
 * \brief Implementation of the Cardano provider interface.
 *
 * This structure contains the context and function pointers required to interact with
 * the Cardano blockchain. It serves as the implementation of the provider interface,
 * encapsulating the necessary state and behaviors.
 */
typedef struct cardano_provider_impl_t
{
    /**
     * \brief Name of the provider implementation.
     */
    char name[256];

    /**
     * \brief Error message buffer for provider-specific error messages.
     */
    char error_message[1024];

    /**
     * \brief Cardano network magic number this provider is connected to.
     */
    cardano_network_magic_t network_magic;

    /**
     * \brief Opaque pointer to the implementation-specific context.
     *
     * This pointer holds the state or context required by the provider implementation.
     * Users should not access or modify this directly.
     */
    cardano_object_t* context;

    /**
     * \brief Function to retrieve protocol parameters.
     *
     * \see cardano_get_parameters_func_t
     */
    cardano_get_parameters_func_t get_parameters;

    /**
     * \brief Function to retrieve unspent outputs for an address.
     *
     * \see cardano_get_unspent_outputs_func_t
     */
    cardano_get_unspent_outputs_func_t get_unspent_outputs;

    /**
     * \brief Function to retrieve rewards for an address.
     *
     * \see cardano_get_rewards_balance_func_t
     */
    cardano_get_rewards_balance_func_t get_rewards_balance;

    /**
     * \brief Function to retrieve unspent outputs for an address and asset.
     *
     * \see cardano_get_unspent_outputs_with_asset_func_t
     */
    cardano_get_unspent_outputs_with_asset_func_t get_unspent_outputs_with_asset;

    /**
     * \brief Function to retrieve an unspent output for a given NFT.
     *
     * \see cardano_get_unspent_output_by_nft_func_t
     */
    cardano_get_unspent_output_by_nft_func_t get_unspent_output_by_nft;

    /**
     * \brief Function to resolve unspent outputs for transaction inputs.
     *
     * \see cardano_resolve_unspent_outputs_func_t
     */
    cardano_resolve_unspent_outputs_func_t resolve_unspent_outputs;

    /**
     * \brief Function to resolve a datum for a given datum hash.
     *
     * \see cardano_resolve_datum_func_t
     */
    cardano_resolve_datum_func_t resolve_datum;

    /**
     * \brief Function to await transaction confirmation.
     *
     * \see cardano_confirm_transaction_func_t
     */
    cardano_confirm_transaction_func_t await_transaction_confirmation;

    /**
     * \brief Function to submit a transaction to the blockchain.
     *
     * \see cardano_submit_transaction_func_t
     */
    cardano_submit_transaction_func_t post_transaction_to_chain;

    /**
     * \brief Function to evaluate a transaction.
     *
     * \see cardano_evaluate_transaction_func_t
     */
    cardano_evaluate_transaction_func_t evaluate_transaction;
} cardano_provider_impl_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_IMPL_H