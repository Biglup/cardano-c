/**
 * \file ledger_order.cpp
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

#include <cardano/error.h>

#include <cardano/address/reward_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/common/credential.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>

#include "../../src/common/ledger_order.h"

#include <cstring>
#include <gmock/gmock.h>
#include <vector>

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a voter from its CBOR hex.
 * \return A new voter instance.
 */
static cardano_voter_t*
new_voter(const char* cbor)
{
  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  EXPECT_EQ(cardano_voter_from_cbor(reader, &voter), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
}

/**
 * Creates a reward address from its header byte followed by a hash made of a repeated byte.
 * \return A new reward address instance.
 */
static cardano_reward_address_t*
new_reward_address(const byte_t header, const byte_t fill)
{
  byte_t bytes[29] = { 0 };

  bytes[0] = header;
  memset(&bytes[1], fill, sizeof(bytes) - 1U);

  cardano_reward_address_t* address = NULL;

  EXPECT_EQ(cardano_reward_address_from_bytes(bytes, sizeof(bytes), &address), CARDANO_SUCCESS);

  return address;
}

/**
 * Creates a list holding the given voters, in the given order.
 * \return A new voter list instance.
 */
static cardano_voter_list_t*
new_voter_list(const std::vector<cardano_voter_t*>& voters)
{
  cardano_voter_list_t* list = NULL;

  EXPECT_EQ(cardano_voter_list_new(&list), CARDANO_SUCCESS);

  for (cardano_voter_t* voter: voters)
  {
    EXPECT_EQ(cardano_voter_list_add(list, voter), CARDANO_SUCCESS);
  }

  return list;
}

/**
 * Creates a withdrawal map holding the given reward accounts.
 * \return A new withdrawal map instance.
 */
static cardano_withdrawal_map_t*
new_withdrawals(const std::vector<cardano_reward_address_t*>& accounts)
{
  cardano_withdrawal_map_t* withdrawals = NULL;

  EXPECT_EQ(cardano_withdrawal_map_new(&withdrawals), CARDANO_SUCCESS);

  for (cardano_reward_address_t* account: accounts)
  {
    EXPECT_EQ(cardano_withdrawal_map_insert(withdrawals, account, 1U), CARDANO_SUCCESS);
  }

  return withdrawals;
}

/* UNIT TESTS ****************************************************************/

TEST(_cardano_ledger_order_credential_key, putsScriptCredentialsBeforeKeyCredentials)
{
  // Arrange
  cardano_voter_t*      key_voter    = new_voter("8202581c01010101010101010101010101010101010101010101010101010101");
  cardano_voter_t*      script_voter = new_voter("8203581c02020202020202020202020202020202020202020202020202020202");
  cardano_credential_t* key_cred     = cardano_voter_get_credential(key_voter);
  cardano_credential_t* script_cred  = cardano_voter_get_credential(script_voter);

  byte_t key_key[CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE]    = { 0 };
  byte_t script_key[CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE] = { 0 };

  // Act
  EXPECT_EQ(_cardano_ledger_order_credential_key(key_cred, key_key), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_credential_key(script_cred, script_key), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(key_key[0], 1U);
  EXPECT_EQ(key_key[1], 0x01U);
  EXPECT_EQ(key_key[CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE - 1], 0x01U);
  EXPECT_EQ(script_key[0], 0U);
  EXPECT_EQ(script_key[1], 0x02U);
  EXPECT_LT(memcmp(script_key, key_key, sizeof(key_key)), 0);

  // Cleanup
  cardano_credential_unref(&key_cred);
  cardano_credential_unref(&script_cred);
  cardano_voter_unref(&key_voter);
  cardano_voter_unref(&script_voter);
}

TEST(_cardano_ledger_order_credential_key, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_voter_t*      voter                                         = new_voter("8202581c01010101010101010101010101010101010101010101010101010101");
  cardano_credential_t* credential                                    = cardano_voter_get_credential(voter);
  byte_t                key[CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE] = { 0 };

  // Act & Assert
  EXPECT_EQ(_cardano_ledger_order_credential_key(NULL, key), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_ledger_order_credential_key(credential, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_voter_unref(&voter);
}

TEST(_cardano_ledger_order_voter_key, ranksRoleThenCredentialKind)
{
  // Arrange
  const char* voters[] = {
    "8201581cffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
    "8200581c00000000000000000000000000000000000000000000000000000000",
    "8203581cffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
    "8202581c00000000000000000000000000000000000000000000000000000000",
    "8204581c00000000000000000000000000000000000000000000000000000000",
  };
  const byte_t expected_role[] = { 0U, 0U, 1U, 1U, 2U };
  const byte_t expected_kind[] = { 0U, 1U, 0U, 1U, 1U };

  byte_t previous[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

  for (size_t i = 0U; i < 5U; ++i)
  {
    cardano_voter_t* voter                              = new_voter(voters[i]);
    byte_t           key[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

    // Act
    EXPECT_EQ(_cardano_ledger_order_voter_key(voter, key), CARDANO_SUCCESS);

    // Assert
    EXPECT_EQ(key[0], expected_role[i]);
    EXPECT_EQ(key[1], expected_kind[i]);

    if (i > 0U)
    {
      EXPECT_LT(memcmp(previous, key, sizeof(key)), 0);
    }

    memcpy(previous, key, sizeof(key));

    // Cleanup
    cardano_voter_unref(&voter);
  }
}

TEST(_cardano_ledger_order_voter_key, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_voter_t* voter                              = new_voter("8202581c01010101010101010101010101010101010101010101010101010101");
  byte_t           key[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

  // Act & Assert
  EXPECT_EQ(_cardano_ledger_order_voter_key(NULL, key), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_ledger_order_voter_key(voter, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voter_unref(&voter);
}

TEST(_cardano_ledger_order_reward_address_key, ranksNetworkThenCredentialKind)
{
  // Arrange
  const byte_t headers[]       = { 0xf0U, 0xe0U, 0xf1U, 0xe1U };
  const byte_t fills[]         = { 0xffU, 0x00U, 0xffU, 0x00U };
  const byte_t expected_net[]  = { 0U, 0U, 1U, 1U };
  const byte_t expected_kind[] = { 0U, 1U, 0U, 1U };

  byte_t previous[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

  for (size_t i = 0U; i < 4U; ++i)
  {
    cardano_reward_address_t* address                            = new_reward_address(headers[i], fills[i]);
    byte_t                    key[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

    // Act
    EXPECT_EQ(_cardano_ledger_order_reward_address_key(address, key), CARDANO_SUCCESS);

    // Assert
    EXPECT_EQ(key[0], expected_net[i]);
    EXPECT_EQ(key[1], expected_kind[i]);
    EXPECT_EQ(key[2], fills[i]);

    if (i > 0U)
    {
      EXPECT_LT(memcmp(previous, key, sizeof(key)), 0);
    }

    memcpy(previous, key, sizeof(key));

    // Cleanup
    cardano_reward_address_unref(&address);
  }
}

TEST(_cardano_ledger_order_reward_address_key, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_reward_address_t* address                            = new_reward_address(0xe0U, 0x01U);
  byte_t                    key[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };

  // Act & Assert
  EXPECT_EQ(_cardano_ledger_order_reward_address_key(NULL, key), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_ledger_order_reward_address_key(address, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&address);
}

TEST(_cardano_ledger_order_voter_index, countsTheVotersThatSortFirst)
{
  // Arrange
  cardano_voter_t*      pool          = new_voter("8204581c00000000000000000000000000000000000000000000000000000000");
  cardano_voter_t*      drep_key      = new_voter("8202581c01010101010101010101010101010101010101010101010101010101");
  cardano_voter_t*      drep_script   = new_voter("8203581c02020202020202020202020202020202020202020202020202020202");
  cardano_voter_t*      committee_key = new_voter("8200581c03030303030303030303030303030303030303030303030303030303");
  cardano_voter_t*      committee_scr = new_voter("8201581c04040404040404040404040404040404040404040404040404040404");
  cardano_voter_list_t* voters        = new_voter_list({ pool, drep_key, drep_script, committee_key, committee_scr });
  uint64_t              indices[]     = { 99U, 99U, 99U, 99U, 99U };

  // Act
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, committee_scr, &indices[0]), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, committee_key, &indices[1]), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, drep_script, &indices[2]), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, drep_key, &indices[3]), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, pool, &indices[4]), CARDANO_SUCCESS);

  // Assert
  for (uint64_t i = 0U; i < 5U; ++i)
  {
    EXPECT_EQ(indices[i], i);
  }

  // Cleanup
  cardano_voter_list_unref(&voters);
  cardano_voter_unref(&pool);
  cardano_voter_unref(&drep_key);
  cardano_voter_unref(&drep_script);
  cardano_voter_unref(&committee_key);
  cardano_voter_unref(&committee_scr);
}

TEST(_cardano_ledger_order_voter_index, returnsNotFoundIfTheVoterIsMissing)
{
  // Arrange
  cardano_voter_t*      present = new_voter("8202581c01010101010101010101010101010101010101010101010101010101");
  cardano_voter_t*      missing = new_voter("8203581c01010101010101010101010101010101010101010101010101010101");
  cardano_voter_list_t* voters  = new_voter_list({ present });
  uint64_t              index   = 99U;

  // Act & Assert
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, missing, &index), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(index, 99U);

  // Cleanup
  cardano_voter_list_unref(&voters);
  cardano_voter_unref(&present);
  cardano_voter_unref(&missing);
}

TEST(_cardano_ledger_order_voter_index, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_voter_t*      voter  = new_voter("8202581c01010101010101010101010101010101010101010101010101010101");
  cardano_voter_list_t* voters = new_voter_list({ voter });
  uint64_t              index  = 0U;

  // Act & Assert
  EXPECT_EQ(_cardano_ledger_order_voter_index(NULL, voter, &index), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, NULL, &index), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_ledger_order_voter_index(voters, voter, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voter_list_unref(&voters);
  cardano_voter_unref(&voter);
}

TEST(_cardano_ledger_order_withdrawal_index, countsTheAccountsThatSortFirst)
{
  // Arrange
  cardano_reward_address_t* key_mainnet    = new_reward_address(0xe1U, 0x01U);
  cardano_reward_address_t* key_testnet    = new_reward_address(0xe0U, 0x02U);
  cardano_reward_address_t* script_mainnet = new_reward_address(0xf1U, 0x03U);
  cardano_reward_address_t* script_testnet = new_reward_address(0xf0U, 0x04U);
  cardano_withdrawal_map_t* withdrawals    = new_withdrawals({ key_mainnet, key_testnet, script_mainnet, script_testnet });
  uint64_t                  indices[]      = { 99U, 99U, 99U, 99U };

  // Act
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(withdrawals, script_testnet, &indices[0]), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(withdrawals, key_testnet, &indices[1]), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(withdrawals, script_mainnet, &indices[2]), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(withdrawals, key_mainnet, &indices[3]), CARDANO_SUCCESS);

  // Assert
  for (uint64_t i = 0U; i < 4U; ++i)
  {
    EXPECT_EQ(indices[i], i);
  }

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_reward_address_unref(&key_mainnet);
  cardano_reward_address_unref(&key_testnet);
  cardano_reward_address_unref(&script_mainnet);
  cardano_reward_address_unref(&script_testnet);
}

TEST(_cardano_ledger_order_withdrawal_index, returnsNotFoundIfTheAccountDoesNotWithdraw)
{
  // Arrange
  cardano_reward_address_t* present     = new_reward_address(0xe0U, 0x01U);
  cardano_reward_address_t* missing     = new_reward_address(0xe1U, 0x01U);
  cardano_withdrawal_map_t* withdrawals = new_withdrawals({ present });
  uint64_t                  index       = 99U;

  // Act & Assert
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(withdrawals, missing, &index), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(index, 99U);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_reward_address_unref(&present);
  cardano_reward_address_unref(&missing);
}

TEST(_cardano_ledger_order_withdrawal_index, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_reward_address_t* address     = new_reward_address(0xe0U, 0x01U);
  cardano_withdrawal_map_t* withdrawals = new_withdrawals({ address });
  uint64_t                  index       = 0U;

  // Act & Assert
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(NULL, address, &index), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(withdrawals, NULL, &index), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_ledger_order_withdrawal_index(withdrawals, address, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_reward_address_unref(&address);
}
