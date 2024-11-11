/**
 * \file transaction_builder.cpp
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
 *
 * \section LICENSE
 *
 * Copyright 2024 Biglup Labs
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

#include <cardano/transaction_builder/transaction_builder.h>

#include <allocators.h>
#include <cardano/transaction_body/transaction_output.h>
#include <gmock/gmock.h>
#include <tests/allocators_helpers.h>

/* TX BUILDER INTERNALS ******************************************************/

typedef struct cardano_tx_builder_t
{
    cardano_object_t               base;
    cardano_error_t                last_error;
    cardano_transaction_t*         transaction;
    cardano_protocol_parameters_t* params;
    cardano_provider_t*            provider;
    cardano_coin_selector_t*       coin_selector;
    cardano_tx_evaluator_t*        tx_evaluator;
    cardano_address_t*             change_address;
    cardano_address_t*             collateral_address;
    cardano_utxo_list_t*           available_utxos;
    cardano_utxo_list_t*           collateral_utxos;
    cardano_utxo_list_t*           pre_selected_inputs;
    cardano_utxo_list_t*           reference_inputs;
    bool                           has_plutus_v1;
    bool                           has_plutus_v2;
    bool                           has_plutus_v3;
    size_t                         additional_signature_count;
} cardano_tx_builder_t;

/* CONSTANTS *****************************************************************/

static const char* UTXO_WITH_SCRIPT_ADDRESS    = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a300583911537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_PV1    = "82825820bb247abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_PV2    = "82825820bb257abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e002a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182024e4d02000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_PV3    = "82825820bb267abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182034e4d03000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_NATIVE = "82825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d818582282008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* CBOR_DIFFERENT_VAL1         = "82825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* CBOR_DIFFERENT_VAL2         = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* CBOR_DIFFERENT_VAL3         = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a026679b8a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* OUTPUT_CBOR                 = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* PLUTUS_DATA_CBOR            = "d8799f0102030405ff";
static const char* COSTMDLS_ALL_CBOR           = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* SCRIPT_ADDRESS              = "addr1x8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shskhj42g";

/* STATIC FUNCTIONS **********************************************************/

static cardano_plutus_data_t*
create_plutus_data(const char* cbor)
{
  cardano_plutus_data_t* data = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_plutus_data_from_cbor(reader, &data);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return data;
}

static cardano_utxo_t*
create_utxo(const char* cbor)
{
  cardano_utxo_t* utxo = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo;
}

static cardano_utxo_list_t*
new_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t* gai2 = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t* gai3 = create_utxo(CBOR_DIFFERENT_VAL3);

  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai3), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);
  cardano_utxo_unref(&gai3);

  return list;
};

static cardano_protocol_parameters_t*
init_protocol_parameters()
{
  cardano_protocol_parameters_t* params = NULL;

  cardano_error_t result = cardano_protocol_parameters_new(&params);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_ex_unit_prices_t* ex_unit_prices  = NULL;
  cardano_unit_interval_t*  memory_prices   = NULL;
  cardano_unit_interval_t*  steps_prices    = NULL;
  cardano_unit_interval_t*  script_ref_cost = NULL;

  result = cardano_unit_interval_from_double(0.0577, &memory_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_unit_interval_from_double(0.0000721, &steps_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_unit_interval_from_double(15.0, &script_ref_cost);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_min_fee_a(params, 44);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_min_fee_b(params, 155381);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_execution_costs(params, ex_unit_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_ref_script_cost_per_byte(params, script_ref_cost);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_ada_per_utxo_byte(params, 4310U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_key_deposit(params, 2000000U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_pool_deposit(params, 2000000U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_drep_deposit(params, 500000000U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(COSTMDLS_ALL_CBOR, strlen(COSTMDLS_ALL_CBOR));

  cardano_costmdls_t* costmdls = NULL;
  result                       = cardano_costmdls_from_cbor(reader, &costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_cost_models(params, costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
  cardano_costmdls_unref(&costmdls);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

/**
 * \brief Allocates and initializes a new Cardano provider context.
 */
typedef struct api_context_t
{
    cardano_object_t base;
    char             key[256];
} ref_counted_string_t;

/**
 * \brief Allocates and initializes a new Cardano provider context.
 */
static cardano_provider_impl_t
cardano_provider_impl_new()
{
  cardano_provider_impl_t impl    = { 0 };
  api_context_t*          context = reinterpret_cast<api_context_t*>(malloc(sizeof(api_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    CARDANO_UNUSED(memset(context->key, 0, sizeof(context->key)));
    CARDANO_UNUSED(memccpy((void*)&context->key[0], "This is a test key", strlen("This is a test key"), sizeof(context->key)));

    impl.context = (cardano_object_t*)context;
  }

  impl.post_transaction_to_chain = [](cardano_provider_impl_t*, cardano_transaction_t*, cardano_blake2b_hash_t** tx_id) -> cardano_error_t
  {
    return cardano_blake2b_compute_hash((const byte_t*)"a", 1, 32, tx_id);
  };

  impl.await_transaction_confirmation = [](cardano_provider_impl_t*, cardano_blake2b_hash_t*, uint64_t, bool*) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.get_parameters = [](cardano_provider_impl_t*, cardano_protocol_parameters_t** param) -> cardano_error_t
  {
    return cardano_protocol_parameters_new(param);
  };

  impl.get_unspent_outputs = [](cardano_provider_impl_t*, cardano_address_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.get_unspent_outputs_with_asset = [](cardano_provider_impl_t*, cardano_address_t*, cardano_asset_id_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.get_unspent_output_by_nft = [](cardano_provider_impl_t*, cardano_asset_id_t*, cardano_utxo_t** utxo) -> cardano_error_t
  {
    static const char*     CBOR   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
    cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

    if (reader == NULL)
    {
      return CARDANO_ERROR_GENERIC;
    }

    cardano_error_t result = cardano_utxo_from_cbor(reader, utxo);

    cardano_cbor_reader_unref(&reader);

    return result;
  };

  impl.resolve_unspent_outputs = [](cardano_provider_impl_t*, cardano_transaction_input_set_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.resolve_datum = [](cardano_provider_impl_t*, cardano_blake2b_hash_t*, cardano_plutus_data_t** datum) -> cardano_error_t
  {
    return cardano_plutus_data_new_integer_from_int(0, datum);
  };

  impl.evaluate_transaction = [](cardano_provider_impl_t*, cardano_transaction_t*, cardano_utxo_list_t*, cardano_redeemer_list_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.get_rewards_balance = [](cardano_provider_impl_t*, cardano_reward_address_t*, uint64_t* balance) -> cardano_error_t
  {
    *balance = 0U;

    return CARDANO_SUCCESS;
  };

  impl.network_magic = CARDANO_NETWORK_MAGIC_MAINNET;

  return impl;
}

/**
 * \brief Allocates and initializes a new Cardano coin_selector context.
 */
static cardano_coin_selector_impl_t
cardano_empty_coin_selector_impl_new()
{
  cardano_coin_selector_impl_t impl = { 0 };

  CARDANO_UNUSED(memset(impl.name, 0, sizeof(impl.name)));
  CARDANO_UNUSED(memccpy((void*)&impl.name[0], "Empty Coin Selector", strlen("Empty Coin Selector"), sizeof(impl.name)));

  impl.select = NULL;

  return impl;
}

/**
 * \brief Allocates and initializes a new Cardano tx_evaluator context.
 */
static cardano_tx_evaluator_impl_t
cardano_empty_tx_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl = { 0 };

  CARDANO_UNUSED(memset(impl.name, 0, sizeof(impl.name)));
  CARDANO_UNUSED(memccpy((void*)&impl.name[0], "Empty Tx Evaluator", strlen("Empty Tx Evaluator"), sizeof(impl.name)));

  impl.evaluate = NULL;

  return impl;
}

/**
 * \brief creates a transaction output from a CBOR hex string.
 *
 * \param cbor the CBOR hex string.
 */
static cardano_transaction_output_t*
cardano_tx_out_new(const char* cbor)
{
  cardano_transaction_output_t* output = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &output);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return output;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_tx_builder_new, canCreateATxBuilder)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Assert
  cardano_tx_builder_set_metadata(builder, 0, (cardano_metadatum_t*)"");
  cardano_tx_builder_set_metadata_ex(builder, 0, "", 0);
  cardano_tx_builder_mint_token(builder, (cardano_blake2b_hash_t*)"", (cardano_asset_name_t*)"", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_mint_token_ex(builder, "", 0, "", 0, 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_mint_token_with_id(builder, (cardano_asset_id_t*)"", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_mint_token_with_id_ex(builder, "", 0, 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_add_mint(builder, (cardano_multi_asset_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_pad_signer_count(builder, 0);
  cardano_tx_builder_add_signer(builder, (cardano_blake2b_hash_t*)"");
  cardano_tx_builder_add_signer_ex(builder, "", 0);
  cardano_tx_builder_add_datum(builder, (cardano_plutus_data_t*)"");
  cardano_tx_builder_withdraw_rewards(builder, (cardano_reward_address_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_withdraw_rewards_ex(builder, "", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_register_reward_address(builder, (cardano_reward_address_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_register_reward_address_ex(builder, "", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_deregister_reward_address(builder, (cardano_reward_address_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_deregister_reward_address_ex(builder, "", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_delegate_stake(builder, (cardano_reward_address_t*)"", (cardano_blake2b_hash_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_delegate_stake_ex(builder, "", 0, "", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_delegate_voting_power(builder, (cardano_reward_address_t*)"", (cardano_drep_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_delegate_voting_power_ex(builder, "", 0, "", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_register_drep(builder, (cardano_drep_t*)"", (cardano_anchor_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_register_drep_ex(builder, "", 0, (cardano_anchor_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_update_drep(builder, (cardano_drep_t*)"", (cardano_anchor_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_update_drep_ex(builder, "", 0, (cardano_anchor_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_deregister_drep(builder, (cardano_drep_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_deregister_drep_ex(builder, "", 0, (cardano_plutus_data_t*)"");
  cardano_tx_builder_vote(builder, (cardano_voter_t*)"", (cardano_governance_action_id_t*)"", (cardano_voting_procedure_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_add_certificate(builder, (cardano_certificate_t*)"", (cardano_plutus_data_t*)"");
  cardano_tx_builder_add_script(builder, (cardano_script_t*)"");

  cardano_transaction_t* tx = NULL;
  ASSERT_EQ(CARDANO_ERROR_NOT_IMPLEMENTED, cardano_tx_builder_build(builder, &tx));

  // Clean up
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
}

TEST(cardano_tx_builder_new, returnsErrorOnMemoryAllocationFailure)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  for (int i = 0; i < 25; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);
    cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

    EXPECT_EQ(builder, nullptr);
  }

  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);
  cardano_provider_unref(&provider);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_coin_selector, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_coin_selector(nullptr, nullptr);
  cardano_tx_builder_set_coin_selector(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_coin_selector, canSetCoinSelector)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_coin_selector_t*       selector = NULL;

  EXPECT_EQ(cardano_coin_selector_new(cardano_empty_coin_selector_impl_new(), &selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_coin_selector(builder, selector);

  // Assert
  EXPECT_EQ(builder->coin_selector, selector);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_tx_builder_set_network_id, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_network_id(nullptr, CARDANO_NETWORK_ID_MAIN_NET);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_network_id, canSetNetworkId)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  const cardano_network_id_t* network_id = cardano_transaction_body_get_network_id(body);

  // Assert
  EXPECT_EQ(*network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_network_id, returnsErroIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_network_id, returnsErroIfMemoryAllocaitonFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_tx_evaluator, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_tx_evaluator(nullptr, nullptr);
  cardano_tx_builder_set_tx_evaluator(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_tx_evaluator, canSetTxEvaluator)
{
  // Arrange
  cardano_protocol_parameters_t* params    = init_protocol_parameters();
  cardano_provider_t*            provider  = NULL;
  cardano_tx_evaluator_t*        evaluator = NULL;

  EXPECT_EQ(cardano_tx_evaluator_new(cardano_empty_tx_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_tx_evaluator(builder, evaluator);

  // Assert
  EXPECT_EQ(builder->tx_evaluator, evaluator);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_evaluator_unref(&evaluator);
}

TEST(cardano_tx_builder_set_change_address, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address(nullptr, nullptr);
  cardano_tx_builder_set_change_address(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_change_address, canSetChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address(builder, address);

  // Assert
  EXPECT_EQ(builder->change_address, address);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_change_address_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address_ex(nullptr, "", 0);
  cardano_tx_builder_set_change_address_ex(builder, nullptr, 0);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_change_address_ex, canSetChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_STREQ(cardano_address_get_string(builder->change_address), cardano_address_get_string(address));

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_change_address_ex, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_collateral_change_address, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address(nullptr, nullptr);
  cardano_tx_builder_set_collateral_change_address(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_change_address, canSetCollateralChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address(builder, address);

  // Assert
  EXPECT_EQ(builder->collateral_address, address);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address_ex(nullptr, "", 0);
  cardano_tx_builder_set_collateral_change_address_ex(builder, nullptr, 0);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, canSetCollateralChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_STREQ(cardano_address_get_string(builder->collateral_address), cardano_address_get_string(address));

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_collateral_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_minimum_fee, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_minimum_fee(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_minimum_fee, canSetMinimumFee)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_minimum_fee(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(cardano_transaction_body_get_fee(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_minimum_fee, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_minimum_fee(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_new, returnsErrorWhenGiveNull)
{
  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(nullptr, nullptr);

  // Assert
  EXPECT_THAT(builder, testing::IsNull());
}

TEST(cardano_tx_builder_set_utxos, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_utxos(nullptr, nullptr);
  cardano_tx_builder_set_utxos(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_utxos, canSetUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_utxos(builder, utxos);

  // Assert
  EXPECT_EQ(builder->available_utxos, utxos);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_new, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Assert
  EXPECT_THAT(builder, nullptr);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_collateral_utxos, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_utxos(nullptr, nullptr);
  cardano_tx_builder_set_collateral_utxos(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_utxos, canSetCollateralUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_utxos(builder, utxos);

  // Assert
  EXPECT_EQ(builder->collateral_utxos, utxos);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_set_invalid_after, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_after, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_after(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_after(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_after(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_after_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after_ex(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_after_ex, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after_ex(builder, 1730901968);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_after(body), 139335677);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after_ex, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_after_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_after_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_before, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_before(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_before, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_before(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_before(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_before(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_before(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_before_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_set_invalid_before_ex(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before_ex, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_invalid_before_ex(builder, 1730901968);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_before(body), 139335677);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before_ex, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  cardano_tx_builder_set_invalid_before_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_before_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_reference_input, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_reference_input(nullptr, utxo);
  cardano_tx_builder_add_reference_input(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_add_reference_input, canAddReferenceInput)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo1    = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo2    = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_t*                utxo3    = create_utxo(UTXO_WITH_REF_SCRIPT_PV3);
  cardano_utxo_t*                utxo4    = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_reference_input(builder, utxo1);
  cardano_tx_builder_add_reference_input(builder, utxo2);
  cardano_tx_builder_add_reference_input(builder, utxo3);
  cardano_tx_builder_add_reference_input(builder, utxo4);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  // Assert
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 4);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_utxo_unref(&utxo4);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfReferenceInputsIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_utxo_list_unref(&builder->reference_inputs);
  builder->reference_inputs = NULL;

  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_lovelace, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_send_lovelace(nullptr, address, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_send_lovelace(builder, address, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(value), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_send_lovelace(builder, address, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_send_lovelace(builder, nullptr, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_send_lovelace(builder, address, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_lovelace_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_send_lovelace_ex(nullptr, address, strlen(address), 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_lovelace_ex, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_lovelace_ex(builder, address, strlen(address), 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(value), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_lovelace_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_lovelace_ex(builder, address, 0, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_lovelace_ex, returnsErrorIfInvalidAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = "invalid_address";

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_lovelace_ex(builder, address, strlen(address), 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_value, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_send_value(nullptr, address, value);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value(builder, address, value);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* output_value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&output_value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(output_value), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  cardano_address_t* address = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  cardano_value_t* value = nullptr;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_value_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_send_value_ex(nullptr, address, strlen(address), value);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* output_value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&output_value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(output_value), 0);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = nullptr;
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, 0, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfInvalidAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "invalid_address";
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_ref(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder, testing::Not((cardano_tx_builder_t*)nullptr));
  EXPECT_EQ(cardano_tx_builder_refcount(tx_builder), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_tx_builder_unref(&tx_builder);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_pad_signer_count, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_builder_pad_signer_count(nullptr, 0);
}

TEST(cardano_tx_builder_pad_signer_count, canSetTheSignerCount)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_pad_signer_count(builder, 10);

  // Assert
  EXPECT_EQ(builder->additional_signature_count, 10);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_builder_ref(nullptr);
}

TEST(cardano_tx_builder_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_tx_builder_t* tx_builder = nullptr;

  // Act
  cardano_tx_builder_unref(&tx_builder);
}

TEST(cardano_tx_builder_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_builder_unref((cardano_tx_builder_t**)nullptr);
}

TEST(cardano_tx_builder_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_ref(tx_builder);
  size_t ref_count = cardano_tx_builder_refcount(tx_builder);

  cardano_tx_builder_unref(&tx_builder);
  size_t updated_ref_count = cardano_tx_builder_refcount(tx_builder);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_ref(tx_builder);
  size_t ref_count = cardano_tx_builder_refcount(tx_builder);

  cardano_tx_builder_unref(&tx_builder);
  size_t updated_ref_count = cardano_tx_builder_refcount(tx_builder);

  cardano_tx_builder_unref(&tx_builder);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(tx_builder, (cardano_tx_builder_t*)nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_tx_builder_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_tx_builder_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_tx_builder_t* tx_builder = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_tx_builder_set_last_error(tx_builder, message);

  // Assert
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Object is NULL.");
}

TEST(cardano_tx_builder_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  const char* message = nullptr;

  // Act
  cardano_tx_builder_set_last_error(tx_builder, message);

  // Assert
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_build, returnsErrorIfGivenNull)
{
  // Act
  cardano_tx_builder_t*  tx_builder = nullptr;
  cardano_transaction_t* tx         = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(tx, nullptr);
}

TEST(cardano_tx_builder_build, returnsErrorIfTransactionIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_error_t result = cardano_tx_builder_build(tx_builder, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_build, returnsErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;

  tx_builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_build, returnsErrorIfChangeAddressNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_build, returnsErrorIfUtxosNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
}

TEST(cardano_tx_builder_build, canBuildTheTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_build, returnsErrorIfBalancingFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_transaction_unref(&tx_builder->transaction);
  tx_builder->transaction = NULL;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_lovelace(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_tx_builder_lock_lovelace, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace(tx_builder, nullptr, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace, canLockLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace(tx_builder, change_address, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace(tx_builder, change_address, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_lock_lovelace_ex, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_lovelace_ex(nullptr, nullptr, 0, 0, nullptr);
}

TEST(cardano_tx_builder_lock_lovelace_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace_ex(tx_builder, nullptr, 0, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace_ex, canLockLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace_ex(tx_builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace_ex(
    tx_builder,
    "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg",
    strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"),
    1000,
    nullptr);

  const cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_lock_value, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_value(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_lock_value, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value(tx_builder, nullptr, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_lock_value, canLockValue)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value(tx_builder, change_address, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_lock_value, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value(tx_builder, change_address, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_lock_value_ex, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_value_ex(nullptr, nullptr, 0, nullptr, nullptr);
}

TEST(cardano_tx_builder_lock_value_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value_ex(tx_builder, nullptr, 0, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_lock_value_ex, canLockValue)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value_ex(tx_builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_add_output, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_output(nullptr, nullptr);
}

TEST(cardano_tx_builder_add_output, returnsErrorIfOutputIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_output(tx_builder, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_output, canAddOutput)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_transaction_output_t*  output         = cardano_tx_out_new(OUTPUT_CBOR);

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_output(tx_builder, output);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_BALANCE_INSUFFICIENT);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_output_unref(&output);
}

TEST(cardano_tx_builder_add_input, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_input(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_add_input, returnsErrorIfInputIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_input(tx_builder, nullptr, nullptr, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
}

TEST(cardano_tx_builder_add_input, canAddInput)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo1          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo2          = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_t*                utxo3          = create_utxo(UTXO_WITH_REF_SCRIPT_PV3);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_set_collateral_change_address(tx_builder, change_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_reference_input(tx_builder, utxo1);
  cardano_tx_builder_add_reference_input(tx_builder, utxo2);
  cardano_tx_builder_add_reference_input(tx_builder, utxo3);
  cardano_tx_builder_add_input(tx_builder, utxo, redeemer, datum);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_input, returnsErrorIfScriptInputIsAddedWithoutRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_SCRIPT_ADDRESS);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string(SCRIPT_ADDRESS, strlen(SCRIPT_ADDRESS), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_input(tx_builder, utxo, nullptr, datum);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_input, returnsErrorOnMemoryAllocationFail)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 9; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

    cardano_tx_builder_set_change_address(tx_builder, change_address);
    cardano_tx_builder_set_utxos(tx_builder, utxos);

    reset_allocators_run_count();
    set_malloc_limit(i);

    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_input(tx_builder, utxo, redeemer, datum);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_build, doesntCrashOnMemoryAllocationFail)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo1          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo2          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo3          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 1024; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

    cardano_tx_builder_set_change_address(tx_builder, change_address);
    cardano_tx_builder_set_utxos(tx_builder, utxos);
    cardano_tx_builder_add_input(tx_builder, utxo, redeemer, datum);
    cardano_tx_builder_add_reference_input(tx_builder, utxo1);
    cardano_tx_builder_add_reference_input(tx_builder, utxo2);
    cardano_tx_builder_add_reference_input(tx_builder, utxo3);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_transaction_t* tx     = nullptr;
    cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

    cardano_tx_builder_unref(&tx_builder);

    cardano_transaction_unref(&tx);
    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}