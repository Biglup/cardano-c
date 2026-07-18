/**
 * \file builder_state.cpp
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
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

#include "../../../src/transaction_builder/internals/builder_state.h"

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";

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

/* UNIT TESTS ****************************************************************/

TEST(cardano_builder_state_init, canInitializeState)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  // Act
  cardano_error_t result = cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_THAT(state.transaction, testing::Not((cardano_transaction_t*)nullptr));
  EXPECT_EQ(state.params, params);
  EXPECT_THAT(state.coin_selector, testing::Not((cardano_coin_selector_t*)nullptr));
  EXPECT_THAT(state.tx_evaluator, testing::Not((cardano_tx_evaluator_t*)nullptr));
  EXPECT_EQ(state.change_address, (cardano_address_t*)nullptr);
  EXPECT_EQ(state.collateral_address, (cardano_address_t*)nullptr);
  EXPECT_EQ(state.available_utxos, (cardano_utxo_list_t*)nullptr);
  EXPECT_EQ(state.collateral_utxos, (cardano_utxo_list_t*)nullptr);
  EXPECT_THAT(state.pre_selected_inputs, testing::Not((cardano_utxo_list_t*)nullptr));
  EXPECT_THAT(state.reference_inputs, testing::Not((cardano_utxo_list_t*)nullptr));
  EXPECT_FALSE(state.has_plutus_v1);
  EXPECT_FALSE(state.has_plutus_v2);
  EXPECT_FALSE(state.has_plutus_v3);
  EXPECT_EQ(state.additional_signature_count, 0U);
  EXPECT_THAT(state.input_to_redeemer_map, testing::Not((cardano_input_to_redeemer_map_t*)nullptr));
  EXPECT_THAT(state.withdrawals_to_redeemer_map, testing::Not((cardano_blake2b_hash_to_redeemer_map_t*)nullptr));
  EXPECT_THAT(state.mints_to_redeemer_map, testing::Not((cardano_blake2b_hash_to_redeemer_map_t*)nullptr));
  EXPECT_THAT(state.votes_to_redeemer_map, testing::Not((cardano_blake2b_hash_to_redeemer_map_t*)nullptr));
  EXPECT_THAT(state.deferred_redeemers, testing::Not((cardano_deferred_redeemer_list_t*)nullptr));

  EXPECT_EQ(state.slot_config.zero_time, CARDANO_MAINNET_SLOT_CONFIG.zero_time);
  EXPECT_EQ(state.slot_config.zero_slot, CARDANO_MAINNET_SLOT_CONFIG.zero_slot);
  EXPECT_EQ(state.slot_config.slot_length, CARDANO_MAINNET_SLOT_CONFIG.slot_length);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_init, takesReferenceOnProtocolParameters)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_protocol_parameters_refcount(params), 1U);

  // Act
  cardano_error_t result = cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_refcount(params), 2U);

  cardano_builder_state_release(&state);

  EXPECT_EQ(cardano_protocol_parameters_refcount(params), 1U);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_init, returnsErrorIfStateIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_error_t result = cardano_builder_state_init(nullptr, params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_init, returnsErrorIfParamsIsNull)
{
  // Arrange
  cardano_builder_state_t state = {};

  // Act
  cardano_error_t result = cardano_builder_state_init(&state, nullptr, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_builder_state_init, returnsErrorIfSlotConfigIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  // Act
  cardano_error_t result = cardano_builder_state_init(&state, params, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_init, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_builder_state_release(&state);

  EXPECT_EQ(cardano_protocol_parameters_refcount(params), 1U);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_init, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_builder_state_release(&state);

  EXPECT_EQ(cardano_protocol_parameters_refcount(params), 1U);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_release, setsHeldObjectPointersToNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  // Act
  cardano_builder_state_release(&state);

  // Assert
  EXPECT_EQ(state.transaction, (cardano_transaction_t*)nullptr);
  EXPECT_EQ(state.params, (cardano_protocol_parameters_t*)nullptr);
  EXPECT_EQ(state.coin_selector, (cardano_coin_selector_t*)nullptr);
  EXPECT_EQ(state.tx_evaluator, (cardano_tx_evaluator_t*)nullptr);
  EXPECT_EQ(state.change_address, (cardano_address_t*)nullptr);
  EXPECT_EQ(state.collateral_address, (cardano_address_t*)nullptr);
  EXPECT_EQ(state.available_utxos, (cardano_utxo_list_t*)nullptr);
  EXPECT_EQ(state.collateral_utxos, (cardano_utxo_list_t*)nullptr);
  EXPECT_EQ(state.pre_selected_inputs, (cardano_utxo_list_t*)nullptr);
  EXPECT_EQ(state.reference_inputs, (cardano_utxo_list_t*)nullptr);
  EXPECT_EQ(state.input_to_redeemer_map, (cardano_input_to_redeemer_map_t*)nullptr);
  EXPECT_EQ(state.withdrawals_to_redeemer_map, (cardano_blake2b_hash_to_redeemer_map_t*)nullptr);
  EXPECT_EQ(state.mints_to_redeemer_map, (cardano_blake2b_hash_to_redeemer_map_t*)nullptr);
  EXPECT_EQ(state.votes_to_redeemer_map, (cardano_blake2b_hash_to_redeemer_map_t*)nullptr);
  EXPECT_EQ(state.deferred_redeemers, (cardano_deferred_redeemer_list_t*)nullptr);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_release, canBeCalledMoreThanOnce)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  // Act
  cardano_builder_state_release(&state);
  cardano_builder_state_release(&state);

  // Assert
  EXPECT_EQ(state.transaction, (cardano_transaction_t*)nullptr);
  EXPECT_EQ(cardano_protocol_parameters_refcount(params), 1U);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_state_release, doesNothingIfStateIsNull)
{
  // Act
  cardano_builder_state_release(nullptr);
}
