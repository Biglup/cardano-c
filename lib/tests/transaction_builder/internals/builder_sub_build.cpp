/**
 * \file builder_sub_build.cpp
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#include "../../../src/transaction_builder/internals/builder_sub_build.h"

#include <cardano/common/utxo.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/transaction_body.h>

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR     = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* UTXO_WITH_ASSETS_CBOR = "82825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* UTXO_CBOR             = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* OUTPUT_CBOR           = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* HASH_HEX              = "0000000000000000000000000000000000000000000000000000000000000000";
static const char* EMPTY_BODY_CBOR       = "a200d90102800180";
static const char* SCALAR_BODY_CBOR      = "a700d90102800180031907d0081903e80f0115192710161a000f4240";

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
 * Creates a UTXO from its CBOR representation.
 * @param cbor The CBOR hex string of the UTXO.
 * @return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_utxo(const char* cbor)
{
  cardano_utxo_t*        utxo   = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  EXPECT_EQ(cardano_utxo_from_cbor(reader, &utxo), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo;
}

/**
 * Creates a transaction output from its CBOR representation.
 * @param cbor The CBOR hex string of the transaction output.
 * @return A new instance of the transaction output.
 */
static cardano_transaction_output_t*
new_output(const char* cbor)
{
  cardano_transaction_output_t* output = NULL;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  EXPECT_EQ(cardano_transaction_output_from_cbor(reader, &output), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return output;
}

/**
 * Gets a borrowed reference to the body of the transaction held by the state.
 * @param state The builder state holding the transaction.
 * @return The body of the transaction.
 */
static cardano_transaction_body_t*
get_body(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  return body;
}

/**
 * Gets a borrowed reference to the body of a sub transaction.
 * @param sub_tx The sub transaction.
 * @return The body of the sub transaction.
 */
static cardano_sub_transaction_body_t*
get_sub_body(cardano_sub_transaction_t* sub_tx)
{
  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  return body;
}

/**
 * Encodes the body of a sub transaction to a CBOR hex string.
 * @param sub_tx The sub transaction.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_sub_body(cardano_sub_transaction_t* sub_tx)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_sub_transaction_body_to_cbor(get_sub_body(sub_tx), writer), CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        body_hex = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, body_hex, hex_size), CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return body_hex;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_builder_build_sub_transaction, projectsAnEmptyTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx        = NULL;
  const char*                error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

  char* body_hex = encode_sub_body(sub_tx);

  cardano_sub_transaction_body_t* sub_body = get_sub_body(sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(error_message, nullptr);
  EXPECT_STREQ(body_hex, EMPTY_BODY_CBOR);
  EXPECT_EQ(cardano_sub_transaction_body_get_invalid_before(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_invalid_after(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_network_id(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_treasury_value(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_donation(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_mint(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_certificates(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_withdrawals(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_aux_data_hash(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_script_data_hash(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_reference_inputs(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_voting_procedures(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_proposal_procedures(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_guards(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_required_top_level_guards(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_direct_deposits(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_account_balance_intervals(sub_body), nullptr);
  EXPECT_EQ(cardano_sub_transaction_get_auxiliary_data(sub_tx), nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_builder_build_sub_transaction, projectsThePreSelectedInputsInCanonicalOrder)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo1 = new_utxo(UTXO_CBOR);
  cardano_utxo_t* utxo2 = new_utxo(UTXO_WITH_ASSETS_CBOR);

  EXPECT_EQ(cardano_utxo_list_add(state.pre_selected_inputs, utxo1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(state.pre_selected_inputs, utxo2), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx        = NULL;
  const char*                error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(get_sub_body(sub_tx));
  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_input_t* first  = NULL;
  cardano_transaction_input_t* second = NULL;

  EXPECT_EQ(cardano_transaction_input_set_get(inputs, 0, &first), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_get(inputs, 1, &second), CARDANO_SUCCESS);

  cardano_transaction_input_t* input1 = cardano_utxo_get_input(utxo1);
  cardano_transaction_input_t* input2 = cardano_utxo_get_input(utxo2);

  cardano_transaction_input_set_t* state_inputs = cardano_transaction_body_get_inputs(get_body(&state));
  cardano_transaction_input_set_unref(&state_inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 2U);
  EXPECT_TRUE(cardano_transaction_input_equals(first, input2));
  EXPECT_TRUE(cardano_transaction_input_equals(second, input1));
  EXPECT_EQ(cardano_transaction_input_set_get_length(state_inputs), 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_transaction_input_unref(&first);
  cardano_transaction_input_unref(&second);
  cardano_transaction_input_unref(&input1);
  cardano_transaction_input_unref(&input2);
}

TEST(cardano_builder_build_sub_transaction, projectsTheOutputs)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_transaction_output_t* output = new_output(OUTPUT_CBOR);

  cardano_transaction_output_list_t* state_outputs = cardano_transaction_body_get_outputs(get_body(&state));
  cardano_transaction_output_list_unref(&state_outputs);

  EXPECT_EQ(cardano_transaction_output_list_add(state_outputs, output), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx        = NULL;
  const char*                error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(get_sub_body(sub_tx));
  cardano_transaction_output_list_unref(&outputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(outputs, state_outputs);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 1U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_output_unref(&output);
}

TEST(cardano_builder_build_sub_transaction, projectsTheScalarFields)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const uint64_t             invalid_before = 1000U;
  const uint64_t             invalid_after  = 2000U;
  const cardano_network_id_t network_id     = CARDANO_NETWORK_ID_MAIN_NET;
  const uint64_t             treasury_value = 10000U;
  const uint64_t             donation       = 1000000U;

  EXPECT_EQ(cardano_transaction_body_set_invalid_before(get_body(&state), &invalid_before), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_invalid_after(get_body(&state), &invalid_after), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_network_id(get_body(&state), &network_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_treasury_value(get_body(&state), &treasury_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_donation(get_body(&state), &donation), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx        = NULL;
  const char*                error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

  char* body_hex = encode_sub_body(sub_tx);

  cardano_sub_transaction_body_t* sub_body = get_sub_body(sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(*cardano_sub_transaction_body_get_invalid_before(sub_body), invalid_before);
  EXPECT_EQ(*cardano_sub_transaction_body_get_invalid_after(sub_body), invalid_after);
  EXPECT_EQ(*cardano_sub_transaction_body_get_network_id(sub_body), network_id);
  EXPECT_EQ(*cardano_sub_transaction_body_get_treasury_value(sub_body), treasury_value);
  EXPECT_EQ(*cardano_sub_transaction_body_get_donation(sub_body), donation);
  EXPECT_NE(cardano_sub_transaction_body_get_donation(sub_body), cardano_transaction_body_get_donation(get_body(&state)));
  EXPECT_STREQ(body_hex, SCALAR_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_builder_build_sub_transaction, projectsTheObjectFields)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_multi_asset_t*                   mint                      = NULL;
  cardano_certificate_set_t*               certificates              = NULL;
  cardano_withdrawal_map_t*                withdrawals               = NULL;
  cardano_blake2b_hash_t*                  aux_data_hash             = NULL;
  cardano_blake2b_hash_t*                  script_data_hash          = NULL;
  cardano_transaction_input_set_t*         reference_inputs          = NULL;
  cardano_voting_procedures_t*             voting_procedures         = NULL;
  cardano_proposal_procedure_set_t*        proposal_procedures       = NULL;
  cardano_guard_set_t*                     guards                    = NULL;
  cardano_required_guards_map_t*           required_top_level_guards = NULL;
  cardano_direct_deposit_map_t*            direct_deposits           = NULL;
  cardano_account_balance_intervals_map_t* account_balance_intervals = NULL;

  EXPECT_EQ(cardano_multi_asset_new(&mint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_new(&withdrawals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &aux_data_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &script_data_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_new(&reference_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedures_new(&voting_procedures), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_new(&proposal_procedures), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_new(&guards), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_required_guards_map_new(&required_top_level_guards), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_new(&direct_deposits), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_new(&account_balance_intervals), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_transaction_body_set_mint(get_body(&state), mint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_certificates(get_body(&state), certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_withdrawals(get_body(&state), withdrawals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_aux_data_hash(get_body(&state), aux_data_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_script_data_hash(get_body(&state), script_data_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_reference_inputs(get_body(&state), reference_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_voting_procedures(get_body(&state), voting_procedures), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_proposal_procedure(get_body(&state), proposal_procedures), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_guards(get_body(&state), guards), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_required_top_level_guards(get_body(&state), required_top_level_guards), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_direct_deposits(get_body(&state), direct_deposits), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_account_balance_intervals(get_body(&state), account_balance_intervals), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx        = NULL;
  const char*                error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

  cardano_sub_transaction_body_t* sub_body = get_sub_body(sub_tx);

  cardano_multi_asset_t*                   sub_mint                      = cardano_sub_transaction_body_get_mint(sub_body);
  cardano_certificate_set_t*               sub_certificates              = cardano_sub_transaction_body_get_certificates(sub_body);
  cardano_withdrawal_map_t*                sub_withdrawals               = cardano_sub_transaction_body_get_withdrawals(sub_body);
  cardano_blake2b_hash_t*                  sub_aux_data_hash             = cardano_sub_transaction_body_get_aux_data_hash(sub_body);
  cardano_blake2b_hash_t*                  sub_script_data_hash          = cardano_sub_transaction_body_get_script_data_hash(sub_body);
  cardano_transaction_input_set_t*         sub_reference_inputs          = cardano_sub_transaction_body_get_reference_inputs(sub_body);
  cardano_voting_procedures_t*             sub_voting_procedures         = cardano_sub_transaction_body_get_voting_procedures(sub_body);
  cardano_proposal_procedure_set_t*        sub_proposal_procedures       = cardano_sub_transaction_body_get_proposal_procedures(sub_body);
  cardano_guard_set_t*                     sub_guards                    = cardano_sub_transaction_body_get_guards(sub_body);
  cardano_required_guards_map_t*           sub_required_top_level_guards = cardano_sub_transaction_body_get_required_top_level_guards(sub_body);
  cardano_direct_deposit_map_t*            sub_direct_deposits           = cardano_sub_transaction_body_get_direct_deposits(sub_body);
  cardano_account_balance_intervals_map_t* sub_account_balance_intervals = cardano_sub_transaction_body_get_account_balance_intervals(sub_body);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(error_message, nullptr);
  EXPECT_EQ(sub_mint, mint);
  EXPECT_EQ(sub_certificates, certificates);
  EXPECT_EQ(sub_withdrawals, withdrawals);
  EXPECT_EQ(sub_aux_data_hash, aux_data_hash);
  EXPECT_EQ(sub_script_data_hash, script_data_hash);
  EXPECT_EQ(sub_reference_inputs, reference_inputs);
  EXPECT_EQ(sub_voting_procedures, voting_procedures);
  EXPECT_EQ(sub_proposal_procedures, proposal_procedures);
  EXPECT_EQ(sub_guards, guards);
  EXPECT_EQ(sub_required_top_level_guards, required_top_level_guards);
  EXPECT_EQ(sub_direct_deposits, direct_deposits);
  EXPECT_EQ(sub_account_balance_intervals, account_balance_intervals);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);

  cardano_multi_asset_unref(&mint);
  cardano_certificate_set_unref(&certificates);
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_blake2b_hash_unref(&aux_data_hash);
  cardano_blake2b_hash_unref(&script_data_hash);
  cardano_transaction_input_set_unref(&reference_inputs);
  cardano_voting_procedures_unref(&voting_procedures);
  cardano_proposal_procedure_set_unref(&proposal_procedures);
  cardano_guard_set_unref(&guards);
  cardano_required_guards_map_unref(&required_top_level_guards);
  cardano_direct_deposit_map_unref(&direct_deposits);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals);

  cardano_multi_asset_unref(&sub_mint);
  cardano_certificate_set_unref(&sub_certificates);
  cardano_withdrawal_map_unref(&sub_withdrawals);
  cardano_blake2b_hash_unref(&sub_aux_data_hash);
  cardano_blake2b_hash_unref(&sub_script_data_hash);
  cardano_transaction_input_set_unref(&sub_reference_inputs);
  cardano_voting_procedures_unref(&sub_voting_procedures);
  cardano_proposal_procedure_set_unref(&sub_proposal_procedures);
  cardano_guard_set_unref(&sub_guards);
  cardano_required_guards_map_unref(&sub_required_top_level_guards);
  cardano_direct_deposit_map_unref(&sub_direct_deposits);
  cardano_account_balance_intervals_map_unref(&sub_account_balance_intervals);
}

TEST(cardano_builder_build_sub_transaction, sharesTheWitnessSetAndTheAuxiliaryData)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_auxiliary_data_t* auxiliary_data = NULL;

  EXPECT_EQ(cardano_auxiliary_data_new(&auxiliary_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_set_auxiliary_data(state.transaction, auxiliary_data), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx        = NULL;
  const char*                error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

  cardano_witness_set_t*    witness_set        = cardano_transaction_get_witness_set(state.transaction);
  cardano_witness_set_t*    sub_witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_auxiliary_data_t* sub_auxiliary_data = cardano_sub_transaction_get_auxiliary_data(sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(sub_witness_set, nullptr);
  EXPECT_EQ(sub_witness_set, witness_set);
  EXPECT_EQ(sub_auxiliary_data, auxiliary_data);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_witness_set_unref(&witness_set);
  cardano_witness_set_unref(&sub_witness_set);
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_auxiliary_data_unref(&sub_auxiliary_data);
}

TEST(cardano_builder_build_sub_transaction, leavesOutTheFieldsThatOnlyExistOnATopLevelTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const uint64_t                           total_collateral   = 5000000U;
  cardano_transaction_input_set_t*         collateral         = NULL;
  cardano_transaction_output_t*            collateral_return  = new_output(OUTPUT_CBOR);
  cardano_account_balance_intervals_map_t* starting_intervals = NULL;

  EXPECT_EQ(cardano_transaction_input_set_new(&collateral), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_new(&starting_intervals), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_transaction_body_set_fee(get_body(&state), 200000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_collateral(get_body(&state), collateral), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_collateral_return(get_body(&state), collateral_return), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_total_collateral(get_body(&state), &total_collateral), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_starting_account_balance_intervals(get_body(&state), starting_intervals), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx        = NULL;
  const char*                error_message = NULL;

  // Act
  const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

  char* body_hex = encode_sub_body(sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(body_hex, EMPTY_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_input_set_unref(&collateral);
  cardano_transaction_output_unref(&collateral_return);
  cardano_account_balance_intervals_map_unref(&starting_intervals);
  free(body_hex);
}

TEST(cardano_builder_build_sub_transaction, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  const uint64_t             invalid_before = 1000U;
  const uint64_t             invalid_after  = 2000U;
  const cardano_network_id_t network_id     = CARDANO_NETWORK_ID_MAIN_NET;
  const uint64_t             treasury_value = 10000U;
  const uint64_t             donation       = 1000000U;
  cardano_utxo_t*            utxo           = new_utxo(UTXO_CBOR);

  EXPECT_EQ(cardano_utxo_list_add(state.pre_selected_inputs, utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_invalid_before(get_body(&state), &invalid_before), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_invalid_after(get_body(&state), &invalid_after), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_network_id(get_body(&state), &network_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_treasury_value(get_body(&state), &treasury_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_donation(get_body(&state), &donation), CARDANO_SUCCESS);

  // Act & Assert
  bool succeeded = false;
  int  failures  = 0;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_transaction_t* sub_tx        = NULL;
    const char*                error_message = NULL;

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_build_sub_transaction(&state, &sub_tx, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_NE(sub_tx, nullptr);
      EXPECT_EQ(error_message, nullptr);
    }
    else
    {
      ++failures;

      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
  }

  EXPECT_TRUE(succeeded);
  EXPECT_GT(failures, 5);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}
