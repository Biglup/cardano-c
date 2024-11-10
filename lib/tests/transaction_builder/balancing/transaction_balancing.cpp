/**
 * \file transaction_balancing.cpp
 *
 * \author angel.castillo
 * \date   Nov 04, 2024
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

#include "../../allocators_helpers.h"

#include <cardano/common/utxo.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>

#include <allocators.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/witness_set/redeemer.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* BALANCED_TX_CBOR    = "84a300d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010182a2005839005cf6c91279a859a072601779fb33bb07c34e1d641d45df51ff63b967f15db05f56035465bf8900a09bdaa16c3d8b8244fea686524408dd8001821a00e4e1c0a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0d294e28021a00029eb9a0f5f6";
static const char* UNBALANCED_TX_CBOR  = "84a300d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010182a2005839005cf6c91279a859a072601779fb33bb07c34e1d641d45df51ff63b967f15db05f56035465bf8900a09bdaa16c3d8b8244fea686524408dd8001821a00e4e1c0a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0d294e28021a00000000a0f5f6";
static const char* COMPLEX_TX_CBOR     = "84b000818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e8049182008200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d083078200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a83088200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01483088200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f186482018200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f82008200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f81581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d58304581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01901f483028200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db784108200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05f683118200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0584108200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05f683118200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05840b8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db70a840c8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d08200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0a850d8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db78200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0a82018200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d3910825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5001481841864581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d08106827468747470733a2f2f74657374696e672e7468697358203e33018e8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80da700818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501868205186482041901f48200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548201818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548202818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54830301818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f540281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b4500481187b0582840100d87a9f187bff82190bb8191b58840201d87a9f187bff821913881907d006815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b450f5a6011904d2026373747203821904d2637374720445627974657305a2667374726b6579187b81676c6973746b65796873747276616c75650626";
static const char* CBOR_DIFFERENT_VAL1 = "82825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* CBOR_DIFFERENT_VAL2 = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* CBOR_DIFFERENT_VAL3 = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a026679b8a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";

/* STATIC FUNCTIONS **********************************************************/

static cardano_transaction_t*
new_default_transaction(const char* cbor)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction;
};

static cardano_transaction_t*
new_transaction_without_inputs(const char* cbor, const int64_t target_coin)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  cardano_transaction_input_set_t* inputs = NULL;
  result                                  = cardano_transaction_input_set_new(&inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  result = cardano_transaction_body_set_inputs(body, inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  result                               = cardano_transaction_output_list_get(outputs, 0, &output);
  cardano_transaction_output_unref(&output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  result                 = cardano_value_set_coin(value, target_coin);
  cardano_value_unref(&value);
  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_t* new_outputs = NULL;
  result                                         = cardano_transaction_output_list_new(&new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_output_list_add(new_outputs, output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_body_set_outputs(body, new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_unref(&new_outputs);

  return transaction;
};

static cardano_transaction_t*
new_transaction_without_inputs_no_assets(const char* cbor, const int64_t target_coin)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  cardano_transaction_input_set_t* inputs = NULL;
  result                                  = cardano_transaction_input_set_new(&inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  result = cardano_transaction_body_set_inputs(body, inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  result                               = cardano_transaction_output_list_get(outputs, 0, &output);
  cardano_transaction_output_unref(&output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_value_t* value = cardano_value_new_zero();
  result                 = cardano_value_set_coin(value, target_coin);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_output_set_value(output, value);
  cardano_value_unref(&value);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_t* new_outputs = NULL;
  result                                         = cardano_transaction_output_list_new(&new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_output_list_add(new_outputs, output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_body_set_outputs(body, new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_unref(&new_outputs);

  EXPECT_THAT(cardano_transaction_body_set_fee(body, 0), CARDANO_SUCCESS);

  return transaction;
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

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

static cardano_utxo_t*
new_default_utxo(const char* utxo)
{
  cardano_utxo_t*        utxo_obj = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(utxo, strlen(utxo));

  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo_obj;
};

static cardano_utxo_list_t*
new_default_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = new_default_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t* gai2 = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t* gai3 = new_default_utxo(CBOR_DIFFERENT_VAL3);

  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai3), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);
  cardano_utxo_unref(&gai3);

  return list;
};

static cardano_utxo_list_t*
new_empty_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  return list;
};

static cardano_tx_evaluator_impl_t
cardano_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl = { 0 };

  impl.evaluate = [](cardano_tx_evaluator_impl_t*, cardano_transaction_t* tx, cardano_utxo_list_t*, cardano_redeemer_list_t** output) -> cardano_error_t
  {
    cardano_witness_set_t* witness = cardano_transaction_get_witness_set(tx);
    cardano_witness_set_unref(&witness);

    cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness);
    cardano_redeemer_list_unref(&redeemers);

    cardano_redeemer_list_t* clone = NULL;
    EXPECT_EQ(cardano_redeemer_list_clone(redeemers, &clone), CARDANO_SUCCESS);

    const size_t redeemers_count = cardano_redeemer_list_get_length(clone);

    cardano_ex_units_t* ex_units = nullptr;
    EXPECT_EQ(cardano_ex_units_new(1000000000, 5000000000, &ex_units), CARDANO_SUCCESS);

    for (size_t i = 0; i < redeemers_count; i++)
    {
      cardano_redeemer_t* redeemer = NULL;
      EXPECT_EQ(cardano_redeemer_list_get(clone, i, &redeemer), CARDANO_SUCCESS);
      cardano_redeemer_unref(&redeemer);

      EXPECT_EQ(cardano_redeemer_set_ex_units(redeemer, ex_units), CARDANO_SUCCESS);
    }

    cardano_ex_units_unref(&ex_units);

    *output = clone;

    return CARDANO_SUCCESS;
  };

  return impl;
}

static cardano_address_t*
create_address(const char* address)
{
  cardano_address_t* payment_address = NULL;

  cardano_error_t result = cardano_address_from_string(address, strlen(address), &payment_address);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return payment_address;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_balance_transaction, canBalanceATransaction)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(BALANCED_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    evaluator);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, canBalanceATransaction2)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    evaluator);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, useSuggestedFeeIfGivenAndEnough)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(BALANCED_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  EXPECT_EQ(cardano_transaction_body_set_fee(body, 5000000), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    evaluator);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, canBalanceTxWithScripts)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(COMPLEX_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    evaluator);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_is_transaction_balanced, returnsTrueIfTheTransactionIsBalanced)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsFalseIfTheTransactionIsNotBalanced)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(UNBALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_FALSE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfTxIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(NULL, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfProtocolIsNull)
{
  // Arrange
  cardano_transaction_t* tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, NULL, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfIsBalancedIsNull)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfResolvedInputsIsNull)
{
  // Arrange
  cardano_transaction_t*         tx       = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, NULL, protocol, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
}