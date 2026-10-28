/**
 * \file ledger_order.c
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

/* INCLUDES ******************************************************************/

#include "ledger_order.h"

#include <cardano/common/network_id.h>
#include <cardano/voting_procedures/voter_type.h>

#include "../string_safe.h"

#include <stdbool.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Ranks of the constructors the ledger orders by, in declaration order.
 *
 * \c Credential declares \c ScriptHashObj before \c KeyHashObj, \c Network
 * declares \c Testnet before \c Mainnet, and \c Voter declares
 * \c CommitteeVoter, \c DRepVoter and \c StakePoolVoter in that order.
 */
static const byte_t LEDGER_SCRIPT_CREDENTIAL = 0U;
static const byte_t LEDGER_KEY_CREDENTIAL    = 1U;
static const byte_t LEDGER_TESTNET           = 0U;
static const byte_t LEDGER_MAINNET           = 1U;
static const byte_t LEDGER_COMMITTEE_VOTER   = 0U;
static const byte_t LEDGER_DREP_VOTER        = 1U;
static const byte_t LEDGER_STAKE_POOL_VOTER  = 2U;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Returns the rank of the role of a voter kind in the ledger voter order.
 *
 * \param[in] type The voter kind.
 *
 * \return The rank of the role of \p type.
 */
static byte_t
voter_role_rank(const cardano_voter_type_t type)
{
  byte_t rank = LEDGER_STAKE_POOL_VOTER;

  switch (type)
  {
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH:
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH:
    {
      rank = LEDGER_COMMITTEE_VOTER;
      break;
    }
    case CARDANO_VOTER_TYPE_DREP_KEY_HASH:
    case CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH:
    {
      rank = LEDGER_DREP_VOTER;
      break;
    }
    case CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH:
    default:
    {
      rank = LEDGER_STAKE_POOL_VOTER;
      break;
    }
  }

  return rank;
}

/**
 * \brief Writes a rank byte followed by the credential key of a credential.
 *
 * \param[in] rank The rank byte.
 * \param[in] credential The credential.
 * \param[out] key Receives \ref CARDANO_LEDGER_ORDER_KEY_SIZE bytes.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code if the credential key
 *         cannot be written.
 */
static cardano_error_t
ranked_credential_key(const byte_t rank, const cardano_credential_t* credential, byte_t* key)
{
  key[0] = rank;

  return _cardano_ledger_order_credential_key(credential, &key[1]);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
_cardano_ledger_order_credential_key(const cardano_credential_t* credential, byte_t* key)
{
  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_type_t type   = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  cardano_error_t           result = cardano_credential_get_type(credential, &type);

  if (result == CARDANO_SUCCESS)
  {
    key[0] = (type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH) ? LEDGER_SCRIPT_CREDENTIAL : LEDGER_KEY_CREDENTIAL;

    cardano_safe_memcpy(
      &key[1],
      (size_t)CARDANO_BLAKE2B_HASH_SIZE_224,
      cardano_credential_get_hash_bytes(credential),
      cardano_credential_get_hash_bytes_size(credential));
  }

  return result;
}

cardano_error_t
_cardano_ledger_order_voter_key(cardano_voter_t* voter, byte_t* key)
{
  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voter_type_t  type       = CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH;
  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = cardano_voter_get_type(voter, &type);

  if (result == CARDANO_SUCCESS)
  {
    credential = cardano_voter_get_credential(voter);
    result     = ranked_credential_key(voter_role_rank(type), credential, key);
  }

  cardano_credential_unref(&credential);

  return result;
}

cardano_error_t
_cardano_ledger_order_reward_address_key(cardano_reward_address_t* address, byte_t* key)
{
  if (key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_network_id_t  network    = CARDANO_NETWORK_ID_MAIN_NET;
  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = cardano_reward_address_get_network_id(address, &network);

  if (result == CARDANO_SUCCESS)
  {
    credential = cardano_reward_address_get_credential(address);
    result     = ranked_credential_key((network == CARDANO_NETWORK_ID_MAIN_NET) ? LEDGER_MAINNET : LEDGER_TESTNET, credential, key);
  }

  cardano_credential_unref(&credential);

  return result;
}

cardano_error_t
_cardano_ledger_order_voter_index(cardano_voter_list_t* voters, cardano_voter_t* voter, uint64_t* index)
{
  if ((voters == NULL) || (voter == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t          target[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };
  uint64_t        rank                                  = 0U;
  bool            found                                 = false;
  const size_t    count                                 = cardano_voter_list_get_length(voters);
  cardano_error_t result                                = _cardano_ledger_order_voter_key(voter, target);

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_voter_t* entry                                    = NULL;
    byte_t           entry_key[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

    result = cardano_voter_list_get(voters, i, &entry);

    if (result == CARDANO_SUCCESS)
    {
      result = _cardano_ledger_order_voter_key(entry, entry_key);
    }

    if (result == CARDANO_SUCCESS)
    {
      const int32_t order = (int32_t)memcmp(entry_key, target, sizeof(target));

      rank  += (order < 0) ? 1U : 0U;
      found = found || (order == 0);
    }

    cardano_voter_unref(&entry);
  }

  if ((result == CARDANO_SUCCESS) && !found)
  {
    result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    *index = rank;
  }

  return result;
}

cardano_error_t
_cardano_ledger_order_withdrawal_index(cardano_withdrawal_map_t* withdrawals, cardano_reward_address_t* address, uint64_t* index)
{
  if ((withdrawals == NULL) || (address == NULL) || (index == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  byte_t          target[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };
  uint64_t        rank                                  = 0U;
  bool            found                                 = false;
  const size_t    count                                 = cardano_withdrawal_map_get_length(withdrawals);
  cardano_error_t result                                = _cardano_ledger_order_reward_address_key(address, target);

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_reward_address_t* entry                                    = NULL;
    byte_t                    entry_key[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

    result = cardano_withdrawal_map_get_key_at(withdrawals, i, &entry);

    if (result == CARDANO_SUCCESS)
    {
      result = _cardano_ledger_order_reward_address_key(entry, entry_key);
    }

    if (result == CARDANO_SUCCESS)
    {
      const int32_t order = (int32_t)memcmp(entry_key, target, sizeof(target));

      rank  += (order < 0) ? 1U : 0U;
      found = found || (order == 0);
    }

    cardano_reward_address_unref(&entry);
  }

  if ((result == CARDANO_SUCCESS) && !found)
  {
    result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    *index = rank;
  }

  return result;
}

cardano_error_t
_cardano_ledger_order_withdrawal_at_index(
  cardano_withdrawal_map_t*  withdrawals,
  const uint64_t             index,
  cardano_reward_address_t** address)
{
  if (address == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t              count  = (withdrawals != NULL) ? cardano_withdrawal_map_get_length(withdrawals) : 0U;
  cardano_reward_address_t* found  = NULL;
  cardano_error_t           result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (found == NULL) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_reward_address_t* candidate = NULL;
    uint64_t                  rank      = 0U;

    result = cardano_withdrawal_map_get_key_at(withdrawals, i, &candidate);

    if (result == CARDANO_SUCCESS)
    {
      result = _cardano_ledger_order_withdrawal_index(withdrawals, candidate, &rank);
    }

    if ((result == CARDANO_SUCCESS) && (rank == index))
    {
      cardano_reward_address_ref(candidate);
      found = candidate;
    }

    cardano_reward_address_unref(&candidate);
  }

  if ((result == CARDANO_SUCCESS) && (found == NULL))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    *address = found;
  }

  return result;
}

cardano_error_t
_cardano_ledger_order_voter_at_index(
  cardano_voting_procedures_t* procedures,
  const uint64_t               index,
  cardano_voter_t**            voter)
{
  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voter_list_t* voters = NULL;
  cardano_voter_t*      found  = NULL;
  cardano_error_t       result = (procedures != NULL) ? cardano_voting_procedures_get_voters(procedures, &voters) : CARDANO_SUCCESS;
  const size_t          count  = (voters != NULL) ? cardano_voter_list_get_length(voters) : 0U;

  for (size_t i = 0U; (i < count) && (found == NULL) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_voter_t* candidate = NULL;
    uint64_t         rank      = 0U;

    result = cardano_voter_list_get(voters, i, &candidate);

    if (result == CARDANO_SUCCESS)
    {
      result = _cardano_ledger_order_voter_index(voters, candidate, &rank);
    }

    if ((result == CARDANO_SUCCESS) && (rank == index))
    {
      cardano_voter_ref(candidate);
      found = candidate;
    }

    cardano_voter_unref(&candidate);
  }

  if ((result == CARDANO_SUCCESS) && (found == NULL))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    *voter = found;
  }

  cardano_voter_list_unref(&voters);

  return result;
}
