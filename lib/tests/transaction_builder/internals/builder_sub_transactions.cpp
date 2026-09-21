/**
 * \file builder_sub_transactions.cpp
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

#include "../../../src/transaction_builder/internals/builder_inputs.h"
#include "../../../src/transaction_builder/internals/builder_sub_transactions.h"

#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_builder/sub_transaction_builder.h>

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

#include <string>

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";

static const char* UTXO_CBOR                   = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* UTXO_WITH_ASSETS_CBOR       = "82825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* OTHER_UTXO_CBOR             = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a026679b8a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* UTXO_WITH_REF_SCRIPT_NATIVE = "82825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d818582282008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* UNTAGGED_SUB_TX_CBOR        = "83a20081825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010180a0f6";

static const char* DUPLICATED_ID_ERROR         = "Sub transaction is already part of the transaction.";
static const char* SPENT_BY_TRANSACTION_ERROR  = "Sub transaction spends an input that is already spent by the transaction.";
static const char* SPENT_BY_SUB_TX_ERROR       = "Sub transaction spends an input that is already spent by another sub transaction.";
static const char* UNRESOLVED_INPUT_ERROR      = "Sub transaction spends an input that is not in the resolved UTXOs.";
static const char* INPUT_SPENT_BY_SUB_TX_ERROR = "Input is already spent by a sub transaction";

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
 * Creates a UTXO list with the given UTXOs.
 * @param first The first UTXO of the list, or NULL for an empty list.
 * @param second The second UTXO of the list, or NULL when the list has less than two elements.
 * @return A new instance of the UTXO list.
 */
static cardano_utxo_list_t*
create_utxo_list(cardano_utxo_t* first, cardano_utxo_t* second)
{
  cardano_utxo_list_t* list = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&list), CARDANO_SUCCESS);

  if (first != NULL)
  {
    EXPECT_EQ(cardano_utxo_list_add(list, first), CARDANO_SUCCESS);
  }

  if (second != NULL)
  {
    EXPECT_EQ(cardano_utxo_list_add(list, second), CARDANO_SUCCESS);
  }

  return list;
}

/**
 * Builds a sub transaction that spends and references the given UTXOs.
 * @param params The protocol parameters.
 * @param inputs The UTXOs the sub transaction spends.
 * @param reference_inputs The UTXOs the sub transaction references, or NULL when it has none.
 * @return A new instance of the sub transaction.
 */
static cardano_sub_transaction_t*
create_sub_transaction(
  cardano_protocol_parameters_t* params,
  cardano_utxo_list_t*           inputs,
  cardano_utxo_list_t*           reference_inputs)
{
  cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  for (size_t i = 0U; i < cardano_utxo_list_get_length(inputs); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    EXPECT_EQ(cardano_utxo_list_get(inputs, i, &utxo), CARDANO_SUCCESS);

    cardano_sub_tx_builder_add_input(builder, utxo);
    cardano_utxo_unref(&utxo);
  }

  for (size_t i = 0U; i < cardano_utxo_list_get_length(reference_inputs); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    EXPECT_EQ(cardano_utxo_list_get(reference_inputs, i, &utxo), CARDANO_SUCCESS);

    cardano_sub_tx_builder_add_reference_input(builder, utxo);
    cardano_utxo_unref(&utxo);
  }

  cardano_sub_transaction_t* sub_transaction = NULL;

  EXPECT_EQ(cardano_sub_tx_builder_build(builder, &sub_transaction), CARDANO_SUCCESS);

  cardano_sub_tx_builder_unref(&builder);

  return sub_transaction;
}

/**
 * Creates a sub transaction from its CBOR representation.
 * @param cbor The CBOR hex string of the sub transaction.
 * @return A new instance of the sub transaction.
 */
static cardano_sub_transaction_t*
create_sub_transaction_from_cbor(const char* cbor)
{
  cardano_sub_transaction_t* sub_transaction = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  EXPECT_EQ(cardano_sub_transaction_from_cbor(reader, &sub_transaction), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return sub_transaction;
}

/**
 * Gets a borrowed reference to the sub transaction set of the transaction body.
 * @param state The builder state holding the transaction.
 * @return The sub transaction set of the body, or NULL when it was not created.
 */
static cardano_sub_transaction_set_t*
get_sub_transactions(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  return sub_transactions;
}

/**
 * Gets a borrowed reference to the sub transaction stored at the given position of the set.
 * @param sub_transactions The sub transaction set.
 * @param index The position of the sub transaction.
 * @return The sub transaction at that position, or NULL when the position is out of bounds.
 */
static cardano_sub_transaction_t*
get_sub_transaction_at(cardano_sub_transaction_set_t* sub_transactions, const size_t index)
{
  cardano_sub_transaction_t* sub_transaction = NULL;

  if (cardano_sub_transaction_set_get(sub_transactions, index, &sub_transaction) != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_sub_transaction_unref(&sub_transaction);

  return sub_transaction;
}

/**
 * Gets a borrowed reference to the UTXO stored at the given position of the list.
 * @param utxos The UTXO list.
 * @param index The position of the UTXO.
 * @return The UTXO at that position, or NULL when the position is out of bounds.
 */
static cardano_utxo_t*
get_utxo_at(cardano_utxo_list_t* utxos, const size_t index)
{
  cardano_utxo_t* utxo = NULL;

  if (cardano_utxo_list_get(utxos, index, &utxo) != CARDANO_SUCCESS)
  {
    return NULL;
  }

  cardano_utxo_unref(&utxo);

  return utxo;
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
 * Gets a borrowed reference to the transaction input of a UTXO.
 * @param utxo The UTXO.
 * @return The transaction input that points to the UTXO.
 */
static cardano_transaction_input_t*
get_input(cardano_utxo_t* utxo)
{
  cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&input);

  return input;
}

/**
 * Serializes a sub transaction to its CBOR hex representation.
 * @param sub_transaction The sub transaction to serialize.
 * @return The CBOR hex string of the sub transaction.
 */
static std::string
sub_transaction_to_hex(cardano_sub_transaction_t* sub_transaction)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_sub_transaction_to_cbor(sub_transaction, writer), CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  std::string  hex(hex_size, '\0');

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, &hex[0], hex_size), CARDANO_SUCCESS);

  hex.resize(hex_size - 1U);

  cardano_cbor_writer_unref(&writer);

  return hex;
}

/**
 * Serializes the body of the transaction under construction to its CBOR hex representation.
 * @param state The builder state holding the transaction.
 * @return The CBOR hex string of the transaction body.
 */
static std::string
body_to_hex(cardano_builder_state_t* state)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_transaction_body_to_cbor(body, writer), CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  std::string  hex(hex_size, '\0');

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, &hex[0], hex_size), CARDANO_SUCCESS);

  hex.resize(hex_size - 1U);

  cardano_cbor_writer_unref(&writer);

  return hex;
}

/**
 * Gets the id of a sub transaction as a hex string.
 * @param sub_transaction The sub transaction.
 * @return The hex string of the sub transaction id.
 */
static std::string
sub_transaction_id_to_hex(cardano_sub_transaction_t* sub_transaction)
{
  cardano_blake2b_hash_t* id = cardano_sub_transaction_get_id(sub_transaction);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(id);
  std::string  hex(hex_size, '\0');

  EXPECT_EQ(cardano_blake2b_hash_to_hex(id, &hex[0], hex_size), CARDANO_SUCCESS);

  hex.resize(hex_size - 1U);

  cardano_blake2b_hash_unref(&id);

  return hex;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_builder_add_sub_transaction, canAddSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(utxo, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction(params, resolved_utxos, NULL);
  const char*                error_message   = NULL;

  EXPECT_EQ(get_sub_transactions(&state), nullptr);

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(error_message, nullptr);

  cardano_sub_transaction_set_t* sub_transactions = get_sub_transactions(&state);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 1U);
  EXPECT_EQ(get_sub_transaction_at(sub_transactions, 0U), sub_transaction);

  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 1U);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_inputs, 0U), utxo);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 0U);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 0U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_builder_add_sub_transaction, canAddSeveralSubTransactions)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo1            = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            utxo2            = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_t*            utxo3            = create_utxo(OTHER_UTXO_CBOR);
  cardano_utxo_t*            reference_utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_list_t*       inputs1          = create_utxo_list(utxo1, utxo2);
  cardano_utxo_list_t*       inputs2          = create_utxo_list(utxo3, NULL);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(reference_utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos2  = create_utxo_list(reference_utxo, utxo3);
  cardano_sub_transaction_t* sub_transaction1 = create_sub_transaction(params, inputs1, NULL);
  cardano_sub_transaction_t* sub_transaction2 = create_sub_transaction(params, inputs2, reference_inputs);
  const char*                error_message    = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction1, inputs1, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction2, resolved_utxos2, &error_message), CARDANO_SUCCESS);

  // Assert
  cardano_sub_transaction_set_t* sub_transactions = get_sub_transactions(&state);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 2U);
  EXPECT_EQ(get_sub_transaction_at(sub_transactions, 0U), sub_transaction1);
  EXPECT_EQ(get_sub_transaction_at(sub_transactions, 1U), sub_transaction2);

  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 3U);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_inputs, 0U), utxo2);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_inputs, 1U), utxo1);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_inputs, 2U), utxo3);

  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 1U);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_reference_inputs, 0U), reference_utxo);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction1);
  cardano_sub_transaction_unref(&sub_transaction2);
  cardano_utxo_list_unref(&inputs1);
  cardano_utxo_list_unref(&inputs2);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_utxos2);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_utxo_unref(&reference_utxo);
}

TEST(cardano_builder_add_sub_transaction, holdsAReferenceToTheSubTransactionAndTheResolvedUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo             = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            reference_utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_list_t*       inputs           = create_utxo_list(utxo, NULL);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(reference_utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos   = create_utxo_list(utxo, reference_utxo);
  cardano_sub_transaction_t* sub_transaction  = create_sub_transaction(params, inputs, reference_inputs);
  const char*                error_message    = NULL;

  cardano_utxo_list_unref(&inputs);
  cardano_utxo_list_unref(&reference_inputs);

  EXPECT_EQ(cardano_sub_transaction_refcount(sub_transaction), 1U);
  EXPECT_EQ(cardano_utxo_refcount(utxo), 2U);
  EXPECT_EQ(cardano_utxo_refcount(reference_utxo), 2U);
  EXPECT_EQ(cardano_utxo_list_refcount(resolved_utxos), 1U);

  // Act
  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_sub_transaction_refcount(sub_transaction), 2U);
  EXPECT_EQ(cardano_utxo_refcount(utxo), 3U);
  EXPECT_EQ(cardano_utxo_refcount(reference_utxo), 3U);
  EXPECT_EQ(cardano_utxo_list_refcount(resolved_utxos), 1U);
  EXPECT_EQ(cardano_utxo_list_refcount(state.sub_transaction_inputs), 1U);
  EXPECT_EQ(cardano_utxo_list_refcount(state.sub_transaction_reference_inputs), 1U);

  cardano_builder_state_release(&state);

  EXPECT_EQ(state.sub_transaction_inputs, (cardano_utxo_list_t*)nullptr);
  EXPECT_EQ(state.sub_transaction_reference_inputs, (cardano_utxo_list_t*)nullptr);
  EXPECT_EQ(cardano_sub_transaction_refcount(sub_transaction), 1U);
  EXPECT_EQ(cardano_utxo_refcount(utxo), 2U);
  EXPECT_EQ(cardano_utxo_refcount(reference_utxo), 2U);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&reference_utxo);
}

TEST(cardano_builder_add_sub_transaction, preservesTheBytesAndTheIdOfTheSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(utxo, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction_from_cbor(UNTAGGED_SUB_TX_CBOR);
  const std::string          id              = sub_transaction_id_to_hex(sub_transaction);
  const char*                error_message   = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message), CARDANO_SUCCESS);

  // Assert
  cardano_sub_transaction_t* embedded = get_sub_transaction_at(get_sub_transactions(&state), 0U);

  EXPECT_EQ(sub_transaction_to_hex(embedded), UNTAGGED_SUB_TX_CBOR);
  EXPECT_EQ(sub_transaction_id_to_hex(embedded), id);
  EXPECT_NE(body_to_hex(&state).find(UNTAGGED_SUB_TX_CBOR), std::string::npos);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_builder_add_sub_transaction, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_list_t* resolved_utxos = create_utxo_list(NULL, NULL);
  const char*          error_message  = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, nullptr, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Sub transaction is NULL.");
  EXPECT_EQ(get_sub_transactions(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_builder_add_sub_transaction, returnsErrorIfResolvedUtxosIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_transaction = create_sub_transaction_from_cbor(UNTAGGED_SUB_TX_CBOR);
  const char*                error_message   = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, nullptr, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(error_message, "Resolved UTXOs list is NULL.");
  EXPECT_EQ(get_sub_transactions(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_builder_add_sub_transaction, returnsErrorIfSubTransactionWasAlreadyAdded)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(utxo, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction(params, resolved_utxos, NULL);
  cardano_sub_transaction_t* same_id         = create_sub_transaction(params, resolved_utxos, NULL);
  const char*                error_message   = NULL;

  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, same_id, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, DUPLICATED_ID_ERROR);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(&state)), 1U);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_unref(&same_id);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_builder_add_sub_transaction, returnsErrorIfInputIsSpentByAnotherSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            shared_utxo      = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            other_utxo       = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_list_t*       inputs1          = create_utxo_list(shared_utxo, NULL);
  cardano_utxo_list_t*       inputs2          = create_utxo_list(other_utxo, shared_utxo);
  cardano_sub_transaction_t* sub_transaction1 = create_sub_transaction(params, inputs1, NULL);
  cardano_sub_transaction_t* sub_transaction2 = create_sub_transaction(params, inputs2, NULL);
  const char*                error_message    = NULL;

  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction1, inputs1, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction2, inputs2, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, SPENT_BY_SUB_TX_ERROR);

  cardano_sub_transaction_set_t* sub_transactions = get_sub_transactions(&state);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 1U);
  EXPECT_EQ(get_sub_transaction_at(sub_transactions, 0U), sub_transaction1);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 1U);
  EXPECT_FALSE(cardano_builder_is_input_spent_by_sub_transaction(&state, get_input(other_utxo)));

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction1);
  cardano_sub_transaction_unref(&sub_transaction2);
  cardano_utxo_list_unref(&inputs1);
  cardano_utxo_list_unref(&inputs2);
  cardano_utxo_unref(&shared_utxo);
  cardano_utxo_unref(&other_utxo);
}

TEST(cardano_builder_add_sub_transaction, returnsErrorIfInputIsSpentByTheTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(utxo, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction(params, resolved_utxos, NULL);
  const char*                error_message   = NULL;

  EXPECT_EQ(cardano_builder_add_input(&state, utxo, NULL, NULL, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, SPENT_BY_TRANSACTION_ERROR);
  EXPECT_EQ(get_sub_transactions(&state), nullptr);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 0U);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_builder_add_sub_transaction, returnsErrorIfSpendInputIsNotResolved)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo1           = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            utxo2           = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_list_t*       inputs          = create_utxo_list(utxo1, utxo2);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(utxo2, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction(params, inputs, NULL);
  const char*                error_message   = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_STREQ(error_message, UNRESOLVED_INPUT_ERROR);
  EXPECT_EQ(get_sub_transactions(&state), nullptr);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 0U);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 0U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&inputs);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
}

TEST(cardano_builder_add_sub_transaction, returnsErrorIfResolvedUtxosIsEmpty)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(NULL, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction_from_cbor(UNTAGGED_SUB_TX_CBOR);
  const char*                error_message   = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_STREQ(error_message, UNRESOLVED_INPUT_ERROR);
  EXPECT_EQ(get_sub_transactions(&state), nullptr);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_builder_add_sub_transaction, recordsOnlyTheResolvedReferenceInputs)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo             = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            resolved_ref     = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_t*            unresolved_ref   = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_list_t*       inputs           = create_utxo_list(utxo, NULL);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(unresolved_ref, resolved_ref);
  cardano_utxo_list_t*       resolved_utxos   = create_utxo_list(resolved_ref, utxo);
  cardano_sub_transaction_t* sub_transaction  = create_sub_transaction(params, inputs, reference_inputs);
  const char*                error_message    = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 1U);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_inputs, 0U), utxo);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 1U);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_reference_inputs, 0U), resolved_ref);
  EXPECT_EQ(cardano_utxo_list_get_length(state.reference_inputs), 0U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&resolved_ref);
  cardano_utxo_unref(&unresolved_ref);
}

TEST(cardano_builder_add_sub_transaction, ignoresResolvedUtxosThatAreNotInputsOfTheSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            unrelated_utxo  = create_utxo(OTHER_UTXO_CBOR);
  cardano_utxo_list_t*       inputs          = create_utxo_list(utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(unrelated_utxo, utxo);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction(params, inputs, NULL);
  const char*                error_message   = NULL;

  // Act
  const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 1U);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_inputs, 0U), utxo);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 0U);
  EXPECT_FALSE(cardano_builder_is_input_spent_by_sub_transaction(&state, get_input(unrelated_utxo)));

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&inputs);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&unrelated_utxo);
}

TEST(cardano_builder_add_sub_transaction, recordsAReferenceInputOncePerSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo1            = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            utxo2            = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_t*            reference_utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_list_t*       inputs1          = create_utxo_list(utxo1, NULL);
  cardano_utxo_list_t*       inputs2          = create_utxo_list(utxo2, NULL);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(reference_utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos1  = create_utxo_list(utxo1, reference_utxo);
  cardano_utxo_list_t*       resolved_utxos2  = create_utxo_list(utxo2, reference_utxo);
  cardano_sub_transaction_t* sub_transaction1 = create_sub_transaction(params, inputs1, reference_inputs);
  cardano_sub_transaction_t* sub_transaction2 = create_sub_transaction(params, inputs2, reference_inputs);
  const char*                error_message    = NULL;

  // Act
  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction1, resolved_utxos1, &error_message), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction2, resolved_utxos2, &error_message), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 2U);
  EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 2U);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_reference_inputs, 0U), reference_utxo);
  EXPECT_EQ(get_utxo_at(state.sub_transaction_reference_inputs, 1U), reference_utxo);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction1);
  cardano_sub_transaction_unref(&sub_transaction2);
  cardano_utxo_list_unref(&inputs1);
  cardano_utxo_list_unref(&inputs2);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_utxos1);
  cardano_utxo_list_unref(&resolved_utxos2);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&reference_utxo);
}

TEST(cardano_builder_add_sub_transaction, leavesTheStateUnchangedWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo             = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            reference_utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_list_t*       inputs           = create_utxo_list(utxo, NULL);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(reference_utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos   = create_utxo_list(utxo, reference_utxo);
  cardano_sub_transaction_t* sub_transaction  = create_sub_transaction(params, inputs, reference_inputs);
  const char*                error_message    = NULL;

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    error_message = NULL;

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(&state)), 1U);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 1U);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 1U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_NE(error_message, nullptr);
      EXPECT_EQ(get_sub_transactions(&state), nullptr);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 0U);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 0U);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&reference_utxo);
}

TEST(cardano_builder_add_sub_transaction, keepsThePreviousSubTransactionsWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo1            = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            utxo2            = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_t*            reference_utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_list_t*       inputs1          = create_utxo_list(utxo1, NULL);
  cardano_utxo_list_t*       inputs2          = create_utxo_list(utxo2, NULL);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(reference_utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos1  = create_utxo_list(utxo1, reference_utxo);
  cardano_utxo_list_t*       resolved_utxos2  = create_utxo_list(utxo2, reference_utxo);
  cardano_sub_transaction_t* sub_transaction1 = create_sub_transaction(params, inputs1, reference_inputs);
  cardano_sub_transaction_t* sub_transaction2 = create_sub_transaction(params, inputs2, reference_inputs);
  const char*                error_message    = NULL;

  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction1, resolved_utxos1, &error_message), CARDANO_SUCCESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_builder_add_sub_transaction(&state, sub_transaction2, resolved_utxos2, &error_message);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_sub_transaction_set_t* sub_transactions = get_sub_transactions(&state);

    EXPECT_EQ(get_sub_transaction_at(sub_transactions, 0U), sub_transaction1);
    EXPECT_EQ(get_utxo_at(state.sub_transaction_inputs, 0U), utxo1);
    EXPECT_EQ(get_utxo_at(state.sub_transaction_reference_inputs, 0U), reference_utxo);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 2U);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 2U);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 2U);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 1U);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_inputs), 1U);
      EXPECT_EQ(cardano_utxo_list_get_length(state.sub_transaction_reference_inputs), 1U);
    }
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction1);
  cardano_sub_transaction_unref(&sub_transaction2);
  cardano_utxo_list_unref(&inputs1);
  cardano_utxo_list_unref(&inputs2);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_utxos1);
  cardano_utxo_list_unref(&resolved_utxos2);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&reference_utxo);
}

TEST(cardano_builder_is_input_spent_by_sub_transaction, returnsTrueForTheInputsOfASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo1            = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            utxo2            = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_t*            reference_utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_list_t*       inputs           = create_utxo_list(utxo1, utxo2);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(reference_utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos   = create_utxo_list(utxo1, utxo2);
  cardano_sub_transaction_t* sub_transaction  = create_sub_transaction(params, inputs, reference_inputs);
  const char*                error_message    = NULL;

  EXPECT_EQ(cardano_utxo_list_add(resolved_utxos, reference_utxo), CARDANO_SUCCESS);
  EXPECT_FALSE(cardano_builder_is_input_spent_by_sub_transaction(&state, get_input(utxo1)));

  // Act
  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(cardano_builder_is_input_spent_by_sub_transaction(&state, get_input(utxo1)));
  EXPECT_TRUE(cardano_builder_is_input_spent_by_sub_transaction(&state, get_input(utxo2)));
  EXPECT_FALSE(cardano_builder_is_input_spent_by_sub_transaction(&state, get_input(reference_utxo)));

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&reference_utxo);
}

TEST(cardano_builder_add_input, returnsErrorIfInputIsSpentByASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(utxo, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction(params, resolved_utxos, NULL);
  const char*                error_message   = NULL;

  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input(&state, utxo, NULL, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, INPUT_SPENT_BY_SUB_TX_ERROR);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 0U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_builder_add_input, canAddAnInputThatIsOnlyReferencedByASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo             = create_utxo(UTXO_CBOR);
  cardano_utxo_t*            referenced_utxo  = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_list_t*       inputs           = create_utxo_list(utxo, NULL);
  cardano_utxo_list_t*       reference_inputs = create_utxo_list(referenced_utxo, NULL);
  cardano_utxo_list_t*       resolved_utxos   = create_utxo_list(utxo, referenced_utxo);
  cardano_sub_transaction_t* sub_transaction  = create_sub_transaction(params, inputs, reference_inputs);
  const char*                error_message    = NULL;

  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input(&state, referenced_utxo, NULL, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 1U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&referenced_utxo);
}

TEST(cardano_builder_add_input_with_deferred_redeemer, returnsErrorIfInputIsSpentByASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_builder_state_t        state  = {};

  EXPECT_EQ(cardano_builder_state_init(&state, params, &CARDANO_MAINNET_SLOT_CONFIG), CARDANO_SUCCESS);

  cardano_utxo_t*            utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_list_t*       resolved_utxos  = create_utxo_list(utxo, NULL);
  cardano_sub_transaction_t* sub_transaction = create_sub_transaction(params, resolved_utxos, NULL);
  const char*                error_message   = NULL;

  EXPECT_EQ(cardano_builder_add_sub_transaction(&state, sub_transaction, resolved_utxos, &error_message), CARDANO_SUCCESS);

  // Act
  const cardano_error_t result = cardano_builder_add_input_with_deferred_redeemer(&state, utxo, deferred_redeemer_callback, NULL, NULL, &error_message);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(error_message, INPUT_SPENT_BY_SUB_TX_ERROR);
  EXPECT_EQ(cardano_utxo_list_get_length(state.pre_selected_inputs), 0U);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(state.deferred_redeemers), 0U);

  // Cleanup
  cardano_builder_state_release(&state);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}
