/**
 * \file scripts_needed.h
 *
 * \author angel.castillo
 * \date   Sep 23, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPTS_NEEDED_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPTS_NEEDED_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction/transaction.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Retrieves the script hash of the payment credential of a Cardano address.
 *
 * This function extracts the script hash from the payment credential of the specified Cardano address, if the
 * address is a base, pointer or enterprise address whose payment credential is a script hash.
 *
 * \param[in] address A pointer to an initialized \ref cardano_address_t object representing the Cardano address
 *                    from which the payment script hash will be retrieved. This parameter is required and must not be NULL.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object containing the script hash of the payment credential,
 *         or NULL if the payment credential of the address is not a script hash or if an error occurs. The caller is
 *         responsible for managing the lifecycle of this object. Specifically, the caller must release it by calling
 *         \ref cardano_blake2b_hash_unref once it is no longer needed.
 */
cardano_blake2b_hash_t*
_cardano_get_payment_script_hash(cardano_address_t* address);

/**
 * \brief Collects the hashes of the scripts a Cardano transaction body needs to run.
 *
 * A script is needed when the body spends an input locked by it, mints or burns under its policy, withdraws from
 * a reward account it controls, carries a certificate whose credential it is, casts a vote with it as the voter,
 * carries a proposal it guards or lists it among its guards. This mirrors how the ledger derives the scripts a
 * transaction needs:
 *
 * - The payment credential of every spent input whose address is a script address. Every spent input is resolved
 *   through \p resolved_inputs.
 * - The policy id of every entry of the mint field.
 * - The credential of every withdrawal whose reward address is a script address.
 * - The script credential that must authorize each certificate: the stake credential of the stake deregistration,
 *   delegation, Conway registration and unregistration certificates (the legacy stake registration certificate, which
 *   the ledger does not require a witness for, yields none), the DRep credential of the DRep certificates and the cold
 *   credential of the committee resignation and committee hot key authorization certificates.
 * - The credential of every voter that is a script, a DRep script or a constitutional committee hot script.
 * - The guardrails script hash of every proposal procedure whose governance action carries one, a parameter change
 *   or a treasury withdrawals action.
 * - Every guard of the body that is a script hash.
 *
 * Reference inputs never make a script needed.
 *
 * \param[in] body A pointer to an initialized \ref cardano_transaction_body_t object whose needed scripts are collected.
 *                 This parameter is required and must not be NULL.
 * \param[in] resolved_inputs A pointer to an initialized \ref cardano_utxo_list_t object containing the resolved UTXOs of
 *                            the inputs spent by the body. If this parameter is NULL, the spent inputs are ignored.
 * \param[out] script_hashes On successful execution, this will point to a newly created \ref cardano_blake2b_hash_set_t
 *                           object containing each needed script hash once. The caller is responsible for managing the
 *                           lifecycle of this object. Specifically, the caller must release it by calling
 *                           \ref cardano_blake2b_hash_set_unref once it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the needed script
 *         hashes were collected, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if a spent input is not among \p resolved_inputs, or
 *         an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if `body` or
 *         `script_hashes` is NULL.
 */
cardano_error_t
_cardano_get_script_hashes_needed(
  cardano_transaction_body_t*  body,
  cardano_utxo_list_t*         resolved_inputs,
  cardano_blake2b_hash_set_t** script_hashes);

/**
 * \brief Finds the Plutus languages of the scripts a Cardano transaction needs among the scripts it is provided with.
 *
 * Every needed script hash is looked up among the provided scripts: the native, PlutusV1, PlutusV2 and PlutusV3 scripts
 * of the witness set of \p tx and the reference scripts of the UTXOs of \p reference_inputs, \p pre_selected_utxo and
 * \p selection. The language of every matched Plutus script is reported, while a needed hash matched by a native
 * script reports no language. A needed hash that no provided script matches is reported through \p has_missing_scripts,
 * and its language remains unknown.
 *
 * \param[in] tx A pointer to an initialized \ref cardano_transaction_t object whose witness set provides scripts.
 *               This parameter is required and must not be NULL.
 * \param[in] needed_scripts A pointer to an initialized \ref cardano_blake2b_hash_set_t object holding the needed script
 *                           hashes, as collected by \ref _cardano_get_script_hashes_needed. This parameter is required
 *                           and must not be NULL.
 * \param[in] reference_inputs The resolved reference inputs whose reference scripts are provided, or NULL when there are none.
 * \param[in] pre_selected_utxo The pre selected UTXOs whose reference scripts are provided, or NULL when there are none.
 * \param[in] selection The UTXOs chosen by coin selection whose reference scripts are provided, or NULL when there are none.
 * \param[out] has_plutus_v1 Set to true if a needed script is a PlutusV1 script. This parameter must not be NULL.
 * \param[out] has_plutus_v2 Set to true if a needed script is a PlutusV2 script. This parameter must not be NULL.
 * \param[out] has_plutus_v3 Set to true if a needed script is a PlutusV3 script. This parameter must not be NULL.
 * \param[out] has_plutus_v4 Set to true if a needed script is a PlutusV4 script. This parameter must not be NULL.
 * \param[out] has_missing_scripts Set to true if a needed script is not among the provided scripts. This parameter must
 *                                 not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the provided scripts
 *         were inspected, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if a required parameter is NULL.
 */
cardano_error_t
_cardano_get_plutus_languages_used(
  cardano_transaction_t*      tx,
  cardano_blake2b_hash_set_t* needed_scripts,
  cardano_utxo_list_t*        reference_inputs,
  cardano_utxo_list_t*        pre_selected_utxo,
  cardano_utxo_list_t*        selection,
  bool*                       has_plutus_v1,
  bool*                       has_plutus_v2,
  bool*                       has_plutus_v3,
  bool*                       has_plutus_v4,
  bool*                       has_missing_scripts);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPTS_NEEDED_H
