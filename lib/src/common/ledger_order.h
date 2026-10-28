/**
 * \file ledger_order.h
 *
 * \author angel.castillo
 * \date   Oct 28, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_LEDGER_ORDER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_LEDGER_ORDER_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/common/credential.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>
#include <cardano/voting_procedures/voting_procedures.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Sizes of the ledger order keys.
 *
 * A credential key is a kind byte followed by the credential hash. A voter key
 * and a reward account key put a rank byte (the voter role or the network) in
 * front of the credential key.
 */
enum
{
  CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE = 1 + CARDANO_BLAKE2B_HASH_SIZE_224,
  CARDANO_LEDGER_ORDER_KEY_SIZE            = 1 + CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE
};

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Writes the key that orders a credential the way the ledger does.
 *
 * The ledger derives \c Ord for \c Credential, so script credentials sort before
 * key credentials and credentials of the same kind sort by hash. Two keys compare
 * in that order under \c memcmp.
 *
 * \param[in] credential The credential.
 * \param[out] key Receives \ref CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE bytes.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p credential or \p key is NULL.
 */
cardano_error_t
_cardano_ledger_order_credential_key(const cardano_credential_t* credential, byte_t* key);

/**
 * \brief Writes the key that orders a voter the way the ledger does.
 *
 * The ledger derives \c Ord for \c Voter, so committee voters sort first, then
 * DReps, then stake pools, and voters of the same role sort by credential. Two
 * keys compare in that order under \c memcmp. A voting redeemer index is the
 * position of the voter in this order.
 *
 * \param[in] voter The voter.
 * \param[out] key Receives \ref CARDANO_LEDGER_ORDER_KEY_SIZE bytes.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p voter or \p key is NULL.
 */
cardano_error_t
_cardano_ledger_order_voter_key(cardano_voter_t* voter, byte_t* key);

/**
 * \brief Writes the key that orders a reward account the way the ledger does.
 *
 * The ledger derives \c Ord for \c AccountAddress, so accounts sort by network,
 * test networks first, then by credential. Two keys compare in that order under
 * \c memcmp. A reward redeemer index is the position of the account in this
 * order.
 *
 * \param[in] address The reward account.
 * \param[out] key Receives \ref CARDANO_LEDGER_ORDER_KEY_SIZE bytes.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if \p address or \p key is NULL.
 */
cardano_error_t
_cardano_ledger_order_reward_address_key(cardano_reward_address_t* address, byte_t* key);

/**
 * \brief Finds the position of a voter among a list of voters in ledger order.
 *
 * The position is the number of voters in \p voters that sort before \p voter.
 *
 * \param[in] voters The voters to search.
 * \param[in] voter The voter to find.
 * \param[out] index On success, the position of \p voter.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if
 *         \p voter is not in \p voters.
 */
cardano_error_t
_cardano_ledger_order_voter_index(cardano_voter_list_t* voters, cardano_voter_t* voter, uint64_t* index);

/**
 * \brief Finds the position of a reward account among withdrawals in ledger order.
 *
 * The position is the number of accounts in \p withdrawals that sort before
 * \p address.
 *
 * \param[in] withdrawals The withdrawals to search.
 * \param[in] address The reward account to find.
 * \param[out] index On success, the position of \p address.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if
 *         \p address does not withdraw in \p withdrawals.
 */
cardano_error_t
_cardano_ledger_order_withdrawal_index(cardano_withdrawal_map_t* withdrawals, cardano_reward_address_t* address, uint64_t* index);

/**
 * \brief Finds the reward account a reward redeemer points to.
 *
 * The ledger resolves a reward redeemer index against its map of withdrawals,
 * which orders accounts by network, test networks first, then script
 * credentials before key credentials, then by hash. The withdrawal map of the
 * library is ordered by address bytes, which puts key credentials first, so the
 * position in the library map is not the redeemer index.
 *
 * \param[in] withdrawals The withdrawals of the transaction level. May be NULL.
 * \param[in] index The redeemer index.
 * \param[out] address On success, the reward account at \p index in ledger order,
 *             owned by the caller.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p address is NULL, \ref CARDANO_ERROR_INDEX_OUT_OF_BOUNDS if \p index
 *         is past the withdrawals, or another error code otherwise.
 */
cardano_error_t
_cardano_ledger_order_withdrawal_at_index(
  cardano_withdrawal_map_t*  withdrawals,
  uint64_t                   index,
  cardano_reward_address_t** address);

/**
 * \brief Finds the voter a voting redeemer points to.
 *
 * The ledger resolves a voting redeemer index against its map of voting
 * procedures, which orders committee voters first, then DReps, then stake
 * pools, each with script credentials before key credentials. The library
 * orders key credentials first within a role, so the position in the library
 * map is not the redeemer index.
 *
 * \param[in] procedures The voting procedures of the transaction level. May be NULL.
 * \param[in] index The redeemer index.
 * \param[out] voter On success, the voter at \p index in ledger order, owned by
 *             the caller.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p voter is NULL, \ref CARDANO_ERROR_INDEX_OUT_OF_BOUNDS if \p index is
 *         past the voters, or another error code otherwise.
 */
cardano_error_t
_cardano_ledger_order_voter_at_index(
  cardano_voting_procedures_t* procedures,
  uint64_t                     index,
  cardano_voter_t**            voter);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_LEDGER_ORDER_H */
