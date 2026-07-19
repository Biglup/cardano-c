/**
 * \file builder_proposals.cpp
 *
 * \author angel.castillo
 * \date   Jul 19, 2026
 *
 * \section LICENSE
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

#include "../../../src/transaction_builder/internals/builder_proposals.h"

#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";

static const char* REWARD_ADDRESS            = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
static const char* ANCHOR_CBOR               = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* GOVERNANCE_ACTION_ID_CBOR = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* HASH_HEX                  = "00000000000000000000000000000000000000000000000000000000";
static const char* WITHDRAWAL_MAP_CBOR       = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the protocol parameters.
 * @return A new instance of the protocol parameters.
 */
static cardano_protocol_parameters_t*
init_protocol_parameters()
{
  cardano_protocol_parameters_t* params = NULL;

  cardano_error_t result = cardano_protocol_parameters_new(&params);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(COSTMDLS_ALL_CBOR, strlen(COSTMDLS_ALL_CBOR));

  cardano_costmdls_t* costmdls = NULL;
  result                       = cardano_costmdls_from_cbor(reader, &costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_cost_models(params, costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
  cardano_costmdls_unref(&costmdls);

  return params;
}

/**
 * Gets a borrowed reference to the redeemer list of the transaction witness set.
 * @param state The builder state holding the transaction.
 * @return The redeemer list of the witness set, or NULL when it was not created.
 */
static cardano_redeemer_list_t*
get_witness_redeemers(cardano_builder_state_t* state)
{
  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);
  cardano_redeemer_list_unref(&redeemers);

  return redeemers;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_builder_propose_parameter_change, doesNotLeakRedeemerListWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  cardano_anchor_t*      anchor = NULL;
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader                                    = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_governance_action_id_t* action_id = NULL;
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_blake2b_hash_t* policy_hash = NULL;
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_hash), CARDANO_SUCCESS);

  cardano_protocol_param_update_t* pparam_update = NULL;
  EXPECT_EQ(cardano_protocol_param_update_new(&pparam_update), CARDANO_SUCCESS);

  const char* error_message = NULL;

  EXPECT_EQ(cardano_builder_propose_parameter_change(&state, reward_address, anchor, pparam_update, action_id, policy_hash, &error_message), CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemers = get_witness_redeemers(&state);

  EXPECT_EQ(cardano_redeemer_list_get_length(redeemers), 1U);
  EXPECT_EQ(cardano_redeemer_list_refcount(redeemers), 1U);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_propose_parameter_change(&state, reward_address, anchor, pparam_update, action_id, policy_hash, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(cardano_redeemer_list_refcount(redeemers), 1U);
      EXPECT_EQ(cardano_redeemer_list_get_length(redeemers), 1U);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_protocol_param_update_unref(&pparam_update);
}

TEST(cardano_builder_propose_parameter_change, doesNotAttachRedeemerWhenProposalCannotBeAdded)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  cardano_anchor_t*      anchor = NULL;
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader                                    = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_governance_action_id_t* action_id = NULL;
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_blake2b_hash_t* policy_hash = NULL;
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_hash), CARDANO_SUCCESS);

  cardano_protocol_param_update_t* pparam_update = NULL;
  EXPECT_EQ(cardano_protocol_param_update_new(&pparam_update), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_builder_state_t state = {};

    EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_propose_parameter_change(&state, reward_address, anchor, pparam_update, action_id, policy_hash, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_redeemer_list_get_length(get_witness_redeemers(&state)), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_redeemer_list_get_length(get_witness_redeemers(&state)), 0U);
    }

    cardano_builder_state_release(&state);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_protocol_param_update_unref(&pparam_update);
}

TEST(cardano_builder_propose_treasury_withdrawals, doesNotAttachRedeemerWhenProposalCannotBeAdded)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  cardano_anchor_t*      anchor = NULL;
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader                                = cardano_cbor_reader_from_hex(WITHDRAWAL_MAP_CBOR, strlen(WITHDRAWAL_MAP_CBOR));
  cardano_withdrawal_map_t* withdrawals = NULL;
  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &withdrawals), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_blake2b_hash_t* policy_hash = NULL;
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_hash), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_builder_state_t state = {};

    EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_propose_treasury_withdrawals(&state, reward_address, anchor, withdrawals, policy_hash, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_redeemer_list_get_length(get_witness_redeemers(&state)), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_redeemer_list_get_length(get_witness_redeemers(&state)), 0U);
    }

    cardano_builder_state_release(&state);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_blake2b_hash_unref(&policy_hash);
}
