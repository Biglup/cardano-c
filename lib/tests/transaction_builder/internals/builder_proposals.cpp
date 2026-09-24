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

#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";

static const char* REWARD_ADDRESS             = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
static const char* ANCHOR_CBOR                = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* GOVERNANCE_ACTION_ID_CBOR  = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* HASH_HEX                   = "00000000000000000000000000000000000000000000000000000000";
static const char* WITHDRAWAL_MAP_CBOR        = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005";
static const char* ANCHOR_URL                 = "https://storage.googleapis.com/biglup/Angel_Castillo.jsonld";
static const char* ANCHOR_HASH                = "26ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* GOVERNANCE_ACTION_ID       = "gov_action1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpzklpgpf";
static const char* HARDFORK_PROPOSALS_CBOR    = "d90102818400581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64548301825820000000000000000000000000000000000000000000000000000000000000000011820c0082783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* CREDENTIAL_SET_CBOR        = "d90102848200581c000000000000000000000000000000000000000000000000000000008200581c100000000000000000000000000000000000000000000000000000008200581c200000000000000000000000000000000000000000000000000000008200581c30000000000000000000000000000000000000000000000000000000";
static const char* COMMITTEE_MEMBERS_MAP_CBOR = "a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003";
static const char* CONSTITUTION_CBOR          = "82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";

/**
 * Zero based index, counted from the start of a propose call on a fresh builder state, of the first
 * allocation made while creating the empty plutus data of the proposing redeemer.
 */
static const int EMPTY_PLUTUS_DATA_MALLOC_INDEX = 5;

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

/**
 * Gets a borrowed reference to the proposal procedures of the transaction body.
 * @param state The builder state holding the transaction.
 * @return The proposal procedures of the body, or NULL when none was proposed.
 */
static cardano_proposal_procedure_set_t*
get_proposal_procedures(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&proposals);

  return proposals;
}

/**
 * Encodes the proposal procedures of the transaction body to a CBOR hex string.
 * @param state The builder state holding the transaction.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_proposal_procedures(cardano_builder_state_t* state)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposal_procedure_set_to_cbor(get_proposal_procedures(state), writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size      = cardano_cbor_writer_get_hex_size(writer);
  char*        proposals_hex = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, proposals_hex, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return proposals_hex;
}

/**
 * Gets the protocol version proposed by a hard fork initiation proposal of the transaction body.
 * @param state The builder state holding the transaction.
 * @param index The index of the proposal.
 * @return The proposed protocol version. The caller must release it.
 */
static cardano_protocol_version_t*
get_hardfork_proposal_version(cardano_builder_state_t* state, const size_t index)
{
  cardano_proposal_procedure_t*          proposal = NULL;
  cardano_hard_fork_initiation_action_t* action   = NULL;

  EXPECT_EQ(cardano_proposal_procedure_set_get(get_proposal_procedures(state), index, &proposal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_to_hard_fork_initiation_action(proposal, &action), CARDANO_SUCCESS);

  cardano_protocol_version_t* version = cardano_hard_fork_initiation_action_get_protocol_version(action);

  cardano_hard_fork_initiation_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal);

  return version;
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

TEST(cardano_builder_propose_parameter_change, returnsErrorIfEmptyPlutusDataCannotBeCreated)
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

  set_malloc_fail_index(EMPTY_PLUTUS_DATA_MALLOC_INDEX);
  cardano_set_allocators(fail_malloc_at_exact_index, realloc, free);

  // Act
  const cardano_error_t result = cardano_builder_propose_parameter_change(&state, reward_address, anchor, pparam_update, action_id, policy_hash, &error_message);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_STREQ(error_message, "Failed to add proposing redeemer.");
  EXPECT_EQ(get_witness_redeemers(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_protocol_param_update_unref(&pparam_update);
}

TEST(cardano_builder_propose_hardfork, returnsErrorIfMemoryAllocationFails)
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

  cardano_protocol_version_t* version = NULL;
  EXPECT_EQ(cardano_protocol_version_new(12, 0, &version), CARDANO_SUCCESS);

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

    const cardano_error_t result = cardano_builder_propose_hardfork(&state, reward_address, anchor, version, action_id, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 0U);
    }

    cardano_builder_state_release(&state);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_protocol_version_unref(&version);
}

TEST(cardano_builder_propose_hardfork_ex, proposesTheGivenMajorAndMinorProtocolVersion)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_propose_hardfork_ex(&state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12, &error_message);

  char*                       proposals_hex = encode_proposal_procedures(&state);
  cardano_protocol_version_t* version       = get_hardfork_proposal_version(&state, 0);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 1U);
  EXPECT_EQ(cardano_protocol_version_get_major(version), 12U);
  EXPECT_EQ(cardano_protocol_version_get_minor(version), 0U);
  EXPECT_STREQ(proposals_hex, HARDFORK_PROPOSALS_CBOR);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_protocol_version_unref(&version);
  free(proposals_hex);
}

TEST(cardano_builder_propose_hardfork_ex, producesTheSameProposalAsTheObjectVariant)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_builder_state_t        object_state = {};
  cardano_builder_state_t        string_state = {};

  EXPECT_EQ(cardano_builder_state_init(&object_state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_state_init(&string_state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_anchor_t* anchor = NULL;
  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);

  cardano_governance_action_id_t* action_id = NULL;
  EXPECT_EQ(cardano_governance_action_id_from_bech32(GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), &action_id), CARDANO_SUCCESS);

  cardano_protocol_version_t* version = NULL;
  EXPECT_EQ(cardano_protocol_version_new(12, 0, &version), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  EXPECT_EQ(cardano_builder_propose_hardfork(&object_state, reward_address, anchor, version, action_id, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_propose_hardfork_ex(&string_state, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12, &error_message), CARDANO_SUCCESS);

  char* object_proposals_hex = encode_proposal_procedures(&object_state);
  char* string_proposals_hex = encode_proposal_procedures(&string_state);

  // Assert
  EXPECT_STREQ(string_proposals_hex, object_proposals_hex);
  EXPECT_STREQ(object_proposals_hex, HARDFORK_PROPOSALS_CBOR);

  // Cleanup
  cardano_builder_state_release(&object_state);
  cardano_builder_state_release(&string_state);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_protocol_version_unref(&version);
  free(object_proposals_hex);
  free(string_proposals_hex);
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

TEST(cardano_builder_propose_no_confidence, returnsErrorIfMemoryAllocationFails)
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

    const cardano_error_t result = cardano_builder_propose_no_confidence(&state, reward_address, anchor, action_id, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 0U);
    }

    cardano_builder_state_release(&state);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
}

TEST(cardano_builder_propose_update_committee, returnsErrorIfMemoryAllocationFails)
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

  reader                                          = cardano_cbor_reader_from_hex(CREDENTIAL_SET_CBOR, strlen(CREDENTIAL_SET_CBOR));
  cardano_credential_set_t* members_to_be_removed = NULL;
  EXPECT_EQ(cardano_credential_set_from_cbor(reader, &members_to_be_removed), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader                                               = cardano_cbor_reader_from_hex(COMMITTEE_MEMBERS_MAP_CBOR, strlen(COMMITTEE_MEMBERS_MAP_CBOR));
  cardano_committee_members_map_t* members_to_be_added = NULL;
  EXPECT_EQ(cardano_committee_members_map_from_cbor(reader, &members_to_be_added), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_unit_interval_t* new_quorum = NULL;
  EXPECT_EQ(cardano_unit_interval_from_double(0.5, &new_quorum), CARDANO_SUCCESS);

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

    const cardano_error_t result = cardano_builder_propose_update_committee(&state, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, new_quorum, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 0U);
    }

    cardano_builder_state_release(&state);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
}

TEST(cardano_builder_propose_new_constitution, returnsErrorIfMemoryAllocationFails)
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

  reader                               = cardano_cbor_reader_from_hex(CONSTITUTION_CBOR, strlen(CONSTITUTION_CBOR));
  cardano_constitution_t* constitution = NULL;
  EXPECT_EQ(cardano_constitution_from_cbor(reader, &constitution), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

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

    const cardano_error_t result = cardano_builder_propose_new_constitution(&state, reward_address, anchor, action_id, constitution, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 0U);
    }

    cardano_builder_state_release(&state);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_builder_propose_info, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_reward_address_t* reward_address = NULL;
  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  cardano_anchor_t*      anchor = NULL;
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

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

    const cardano_error_t result = cardano_builder_propose_info(&state, reward_address, anchor, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(&state)), 0U);
    }

    cardano_builder_state_release(&state);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}
