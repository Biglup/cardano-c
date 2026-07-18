/**
 * \file builder_redeemers.cpp
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
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>

#include "../../../src/transaction_builder/internals/builder_redeemers.h"

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

/**
 * Creates a new blake2b hash filled with zero bytes.
 * @return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
new_default_hash()
{
  static const byte_t bytes[28] = { 0 };

  cardano_blake2b_hash_t* hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_bytes(bytes, sizeof(bytes), &hash), CARDANO_SUCCESS);

  return hash;
}

/**
 * Deferred redeemer callback that never produces a payload.
 * @return Always returns CARDANO_SUCCESS.
 */
static cardano_error_t
noop_deferred_redeemer(
  void*                   user_context,
  cardano_transaction_t*  draft_tx,
  cardano_utxo_list_t*    resolved_inputs,
  cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(user_context);
  CARDANO_UNUSED(draft_tx);
  CARDANO_UNUSED(resolved_inputs);
  CARDANO_UNUSED(redeemer);

  return CARDANO_SUCCESS;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_builder_add_redeemer, rejectsUnsupportedTags)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state.transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_blake2b_hash_t* hash = new_default_hash();
  cardano_plutus_data_t*  data = NULL;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(0, &data), CARDANO_SUCCESS);

  const char* error_message = NULL;

  // Act
  cardano_error_t spend_result = cardano_builder_add_redeemer(witnesses, hash, data, CARDANO_REDEEMER_TAG_SPEND, &state, &error_message);
  cardano_error_t cert_result  = cardano_builder_add_redeemer(witnesses, hash, data, CARDANO_REDEEMER_TAG_CERTIFYING, &state, &error_message);

  // Assert
  EXPECT_EQ(spend_result, CARDANO_ERROR_ILLEGAL_STATE);
  EXPECT_EQ(cert_result, CARDANO_ERROR_ILLEGAL_STATE);
  EXPECT_STREQ(error_message, "Invalid redeemer tag.");

  // Cleanup
  cardano_plutus_data_unref(&data);
  cardano_blake2b_hash_unref(&hash);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_add_redeemer, doesNothingWhenDataIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state.transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_blake2b_hash_t* hash = new_default_hash();

  const char* error_message = NULL;

  // Act
  cardano_error_t result = cardano_builder_add_redeemer(witnesses, hash, NULL, CARDANO_REDEEMER_TAG_MINT, &state, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(error_message, (const char*)NULL);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get_length(state.mints_to_redeemer_map), 0U);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_builder_create_placeholder_plutus_data, createsAnEmptyConstr)
{
  // Arrange
  cardano_plutus_data_t* data = NULL;

  // Act
  cardano_error_t result = cardano_builder_create_placeholder_plutus_data(&data);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_data_kind_t kind;

  EXPECT_EQ(cardano_plutus_data_get_kind(data, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_CONSTR);

  cardano_constr_plutus_data_t* constr = NULL;

  EXPECT_EQ(cardano_plutus_data_to_constr(data, &constr), CARDANO_SUCCESS);

  uint64_t alternative = 99U;

  EXPECT_EQ(cardano_constr_plutus_data_get_alternative(constr, &alternative), CARDANO_SUCCESS);
  EXPECT_EQ(alternative, 0U);

  cardano_plutus_list_t* fields = NULL;

  EXPECT_EQ(cardano_constr_plutus_data_get_data(constr, &fields), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get_length(fields), 0U);

  // Cleanup
  cardano_plutus_list_unref(&fields);
  cardano_constr_plutus_data_unref(&constr);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_builder_register_deferred_from_map, reportsMissingRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash = new_default_hash();

  const char* error_message = NULL;

  // Act
  cardano_error_t result = cardano_builder_register_deferred_from_map(&state, state.mints_to_redeemer_map, hash, noop_deferred_redeemer, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_STREQ(error_message, "Failed to register the deferred redeemer.");
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(state.deferred_redeemers), 0U);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
}
