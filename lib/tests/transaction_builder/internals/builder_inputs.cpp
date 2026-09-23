/**
 * \file builder_inputs.cpp
 *
 * \author angel.castillo
 * \date   Sep 23, 2026
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

#include "../../../src/transaction_builder/internals/builder_inputs.h"

#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";

static const char* UTXO_CBOR                   = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* SAME_ID_OTHER_INDEX_UTXO    = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63302a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* OTHER_ID_SAME_INDEX_UTXO    = "82825820d4c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* UTXO_WITH_REF_SCRIPT_NATIVE = "82825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d818582282008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* PLUTUS_DATA_CBOR            = "d8799f0102030405ff";

static const char* INPUT_ALREADY_ADDED_ERROR           = "Input is already added to the transaction";
static const char* REFERENCE_INPUT_ALREADY_ADDED_ERROR = "Reference input is already added to the transaction.";

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
create_utxo(const char* cbor)
{
  cardano_utxo_t* utxo = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  EXPECT_EQ(cardano_utxo_from_cbor(reader, &utxo), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo;
}

/**
 * Creates a plutus data from its CBOR representation.
 * @param cbor The CBOR hex string of the plutus data.
 * @return A new instance of the plutus data.
 */
static cardano_plutus_data_t*
create_plutus_data(const char* cbor)
{
  cardano_plutus_data_t* data = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  EXPECT_EQ(cardano_plutus_data_from_cbor(reader, &data), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return data;
}

/**
 * Deferred redeemer callback that produces no redeemer.
 * @param user_context The user context, unused.
 * @param draft_tx The draft transaction, unused.
 * @param resolved_inputs The resolved inputs, unused.
 * @param redeemer The produced redeemer, left untouched.
 * @return Always CARDANO_SUCCESS.
 */
static cardano_error_t
deferred_redeemer_callback(void*, cardano_transaction_t*, cardano_utxo_list_t*, cardano_plutus_data_t**)
{
  return CARDANO_SUCCESS;
}

/**
 * Gets a borrowed reference to the witness set of the transaction under construction.
 * @param state The builder state holding the transaction.
 * @return The witness set of the transaction.
 */
static cardano_witness_set_t*
get_witness_set(cardano_builder_state_t* state)
{
  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witness_set);

  return witness_set;
}

/**
 * Gets the number of redeemers in the witness set of the transaction under construction.
 * @param state The builder state holding the transaction.
 * @return The number of redeemers, or zero when the witness set has no redeemer list.
 */
static size_t
get_redeemer_count(cardano_builder_state_t* state)
{
  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(get_witness_set(state));
  cardano_redeemer_list_unref(&redeemers);

  return cardano_redeemer_list_get_length(redeemers);
}

/**
 * Gets the number of datums in the witness set of the transaction under construction.
 * @param state The builder state holding the transaction.
 * @return The number of datums, or zero when the witness set has no plutus data set.
 */
static size_t
get_datum_count(cardano_builder_state_t* state)
{
  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(get_witness_set(state));
  cardano_plutus_data_set_unref(&datums);

  return cardano_plutus_data_set_get_length(datums);
}

/**
 * Gets the number of reference inputs in the body of the transaction under construction.
 * @param state The builder state holding the transaction.
 * @return The number of reference inputs, or zero when the body has no reference input set.
 */
static size_t
get_reference_input_count(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* reference_inputs = cardano_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_unref(&reference_inputs);

  return cardano_transaction_input_set_get_length(reference_inputs);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_builder_add_input, returnsErrorIfTheSameUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo          = create_utxo(UTXO_CBOR);
  const char*     error_message = NULL;

  EXPECT_EQ(cardano_builder_add_input(&state, utxo, NULL, NULL, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input(&state, utxo, NULL, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, INPUT_ALREADY_ADDED_ERROR);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_builder_add_input, returnsErrorIfAnEqualUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*        utxo          = create_utxo(UTXO_CBOR);
  cardano_utxo_t*        copy          = create_utxo(UTXO_CBOR);
  cardano_plutus_data_t* redeemer      = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t* datum         = create_plutus_data(PLUTUS_DATA_CBOR);
  const char*            error_message = NULL;

  EXPECT_EQ(cardano_builder_add_input(&state, utxo, redeemer, datum, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input(&state, copy, redeemer, datum, &error_message);

  // Assert
  EXPECT_NE(utxo, copy);
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, INPUT_ALREADY_ADDED_ERROR);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 1U);
  EXPECT_EQ(cardano_input_to_redeemer_map_get_length(state.input_to_redeemer_map), 1U);
  EXPECT_EQ(get_redeemer_count(&state), 1U);
  EXPECT_EQ(get_datum_count(&state), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&copy);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
}

TEST(cardano_builder_add_input, canAddInputsWithTheSameTransactionIdAndADifferentIndex)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo          = create_utxo(UTXO_CBOR);
  cardano_utxo_t* other         = create_utxo(SAME_ID_OTHER_INDEX_UTXO);
  const char*     error_message = NULL;

  EXPECT_EQ(cardano_builder_add_input(&state, utxo, NULL, NULL, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input(&state, other, NULL, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 2U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&other);
}

TEST(cardano_builder_add_input, canAddInputsWithTheSameIndexAndADifferentTransactionId)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo          = create_utxo(UTXO_CBOR);
  cardano_utxo_t* other         = create_utxo(OTHER_ID_SAME_INDEX_UTXO);
  const char*     error_message = NULL;

  EXPECT_EQ(cardano_builder_add_input(&state, utxo, NULL, NULL, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input(&state, other, NULL, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 2U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&other);
}

TEST(cardano_builder_add_input_with_deferred_redeemer, returnsErrorIfTheInputWasAlreadyAdded)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo          = create_utxo(UTXO_CBOR);
  cardano_utxo_t* copy          = create_utxo(UTXO_CBOR);
  const char*     error_message = NULL;

  EXPECT_EQ(cardano_builder_add_input_with_deferred_redeemer(&state, utxo, deferred_redeemer_callback, NULL, NULL, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input_with_deferred_redeemer(&state, copy, deferred_redeemer_callback, NULL, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, INPUT_ALREADY_ADDED_ERROR);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 1U);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(state.deferred_redeemers), 1U);
  EXPECT_EQ(get_redeemer_count(&state), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&copy);
}

TEST(cardano_builder_add_reference_input, returnsErrorIfTheSameUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo          = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  const char*     error_message = NULL;

  EXPECT_EQ(cardano_builder_add_reference_input(&state, utxo, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_reference_input(&state, utxo, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, REFERENCE_INPUT_ALREADY_ADDED_ERROR);
  EXPECT_EQ(cardano_utxo_list_get_length(state.reference_inputs), 1U);
  EXPECT_EQ(get_reference_input_count(&state), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_builder_add_reference_input, returnsErrorIfAnEqualUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo          = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_t* copy          = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  const char*     error_message = NULL;

  EXPECT_EQ(cardano_builder_add_reference_input(&state, utxo, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_reference_input(&state, copy, &error_message);

  // Assert
  EXPECT_NE(utxo, copy);
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, REFERENCE_INPUT_ALREADY_ADDED_ERROR);
  EXPECT_EQ(cardano_utxo_list_get_length(state.reference_inputs), 1U);
  EXPECT_EQ(get_reference_input_count(&state), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&copy);
}

TEST(cardano_builder_add_reference_input, canAddReferenceInputsWithTheSameTransactionIdAndADifferentIndex)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t* utxo          = create_utxo(UTXO_CBOR);
  cardano_utxo_t* other         = create_utxo(SAME_ID_OTHER_INDEX_UTXO);
  const char*     error_message = NULL;

  EXPECT_EQ(cardano_builder_add_reference_input(&state, utxo, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_reference_input(&state, other, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_get_length(state.reference_inputs), 2U);
  EXPECT_EQ(get_reference_input_count(&state), 2U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&other);
}
