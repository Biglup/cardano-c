/**
 * \file large_first_coin_selector.cpp
 *
 * \author angel.castillo
 * \date   Oct 14, 2024
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

#include "../../../src/allocators.h"
#include "../../allocators_helpers.h"

#include <cardano/common/utxo.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>

#include <gmock/gmock.h>

extern "C" {
#include "../../../src/transaction_builder/coin_selection/internals/large_first_helpers.h"
}

/* CONSTANTS *****************************************************************/

static const char* CBOR                  = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
static const char* CBOR_DIFFERENT_INPUT  = "82825820bb217abaca60fc0ca78c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
static const char* CBOR_DIFFERENT_OUTPUT = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* CBOR_DIFFERENT_VAL1   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c20a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* CBOR_DIFFERENT_VAL2   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* CBOR_DIFFERENT_VAL3   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c22a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* VALUE                 = "821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c08581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* ASSET_NAME_CBOR_1     = "49736b7977616c6b6571";
static const char* POLICY_ID_HEX_1       = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";

/* STATIC FUNCTIONS **********************************************************/

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

static cardano_value_t*
new_default_value(const char* cbor)
{
  cardano_value_t*       value_obj = NULL;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_value_from_cbor(reader, &value_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return value_obj;
};

static cardano_utxo_list_t*
new_default_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = new_default_utxo(CBOR_DIFFERENT_INPUT);
  cardano_utxo_t* gai2 = new_default_utxo(CBOR_DIFFERENT_OUTPUT);

  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);

  return list;
};

static cardano_utxo_list_t*
new_utxo_list_diff_vals()
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
new_utxo_small_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai2 = new_default_utxo(CBOR_DIFFERENT_VAL2);

  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai2);

  return list;
};

static cardano_utxo_list_t*
new_utxo_list_same_utxo()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t* gai2 = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t* gai3 = new_default_utxo(CBOR_DIFFERENT_VAL2);

  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai3), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);
  cardano_utxo_unref(&gai3);

  return list;
};

static cardano_asset_name_t*
new_default_asset_name(const char* name)
{
  cardano_asset_name_t*  asset_name = NULL;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(name, strlen(name));
  cardano_error_t        result     = cardano_asset_name_from_cbor(reader, &asset_name);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return asset_name;
};

static cardano_blake2b_hash_t*
new_default_blake2b_hash(const char* hash)
{
  cardano_blake2b_hash_t* blake2b_hash = NULL;
  cardano_error_t         result       = cardano_blake2b_hash_from_hex(hash, strlen(hash), &blake2b_hash);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return blake2b_hash;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_large_first_coin_selector_new, createsALargeFirstCoinSelector)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;

  // Act
  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(large_first_coin_selector, nullptr);

  // Cleanup
  cardano_coin_selector_unref(&large_first_coin_selector);
}

TEST(cardano_large_first_coin_selector_new, returnsErrorIfLargeFirstCoinSelectorIsNull)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;

  // Act
  cardano_error_t error = cardano_large_first_coin_selector_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(large_first_coin_selector, (cardano_coin_selector_t*)nullptr);
}

TEST(cardano_large_first_coin_selector_select, selectsTheLargestFirst)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;
  cardano_utxo_list_t*     selection                 = nullptr;
  cardano_utxo_list_t*     remaining_utxo            = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo         = nullptr;
  cardano_value_t*         target                    = nullptr;

  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_value_new(1000, nullptr, &target);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  // Act
  error = cardano_coin_selector_select(large_first_coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(selection, nullptr);
  ASSERT_NE(remaining_utxo, nullptr);

  cardano_utxo_t* utxo = nullptr;
  error                = cardano_utxo_list_get(selection, 0, &utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);

  ASSERT_NE(output, nullptr);

  cardano_value_t* value = cardano_transaction_output_get_value(output);

  ASSERT_NE(value, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value), 4027026466);
  EXPECT_EQ(cardano_utxo_list_get_length(selection), 1);
  EXPECT_EQ(cardano_utxo_list_get_length(remaining_utxo), 2);

  cardano_utxo_t* utxo1 = nullptr;
  error                 = cardano_utxo_list_get(remaining_utxo, 0, &utxo1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output1 = cardano_utxo_get_output(utxo1);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value1 = cardano_transaction_output_get_value(output1);

  ASSERT_NE(value1, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value1), 4027026465);

  cardano_utxo_t* utxo2 = nullptr;

  error = cardano_utxo_list_get(remaining_utxo, 1, &utxo2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output2 = cardano_utxo_get_output(utxo2);

  ASSERT_NE(output2, nullptr);

  cardano_value_t* value2 = cardano_transaction_output_get_value(output2);

  ASSERT_NE(value2, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value2), 4027026464);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_utxo_list_unref(&available_utxo);
  cardano_value_unref(&target);
  cardano_utxo_unref(&utxo1);
  cardano_transaction_output_unref(&output1);
  cardano_value_unref(&value1);
  cardano_utxo_unref(&utxo2);
  cardano_transaction_output_unref(&output2);
  cardano_value_unref(&value2);

  cardano_coin_selector_unref(&large_first_coin_selector);
}

TEST(cardano_large_first_coin_selector_select, selectsTheLargestFirstButAlsoIncludesPreselected)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;
  cardano_utxo_list_t*     selection                 = nullptr;
  cardano_utxo_list_t*     remaining_utxo            = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo         = new_utxo_small_list();
  cardano_value_t*         target                    = nullptr;

  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_value_new(4027026467, nullptr, &target);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  // Act
  error = cardano_coin_selector_select(large_first_coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(selection, nullptr);
  ASSERT_NE(remaining_utxo, nullptr);

  EXPECT_EQ(cardano_utxo_list_get_length(selection), 2);
  EXPECT_EQ(cardano_utxo_list_get_length(remaining_utxo), 1);

  cardano_utxo_t* utxo = nullptr;
  error                = cardano_utxo_list_get(selection, 0, &utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);

  ASSERT_NE(output, nullptr);

  cardano_value_t* value = cardano_transaction_output_get_value(output);

  ASSERT_NE(value, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value), 4027026465);

  cardano_utxo_t* utxo1 = nullptr;

  error = cardano_utxo_list_get(selection, 1, &utxo1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output1 = cardano_utxo_get_output(utxo1);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value1 = cardano_transaction_output_get_value(output1);

  ASSERT_NE(value1, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value1), 4027026466);

  cardano_utxo_t* utxo2 = nullptr;

  error = cardano_utxo_list_get(remaining_utxo, 0, &utxo2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output2 = cardano_utxo_get_output(utxo2);

  ASSERT_NE(output2, nullptr);

  cardano_value_t* value2 = cardano_transaction_output_get_value(output2);

  ASSERT_NE(value2, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value2), 4027026464);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_utxo_list_unref(&available_utxo);
  cardano_value_unref(&target);
  cardano_utxo_unref(&utxo1);
  cardano_transaction_output_unref(&output1);
  cardano_value_unref(&value1);
  cardano_utxo_unref(&utxo2);
  cardano_transaction_output_unref(&output2);
  cardano_value_unref(&value2);
  cardano_coin_selector_unref(&large_first_coin_selector);
  cardano_utxo_list_unref(&pre_selected_utxo);
}

TEST(cardano_large_first_coin_selector_select, selectsTheLargestFirstAssets)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;
  cardano_utxo_list_t*     selection                 = nullptr;
  cardano_utxo_list_t*     remaining_utxo            = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo         = nullptr;
  cardano_value_t*         target                    = new_default_value(VALUE);

  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  // Act
  error = cardano_coin_selector_select(large_first_coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_utxo_list_get_length(selection), 2);
  EXPECT_EQ(cardano_utxo_list_get_length(remaining_utxo), 1);

  ASSERT_NE(selection, nullptr);
  ASSERT_NE(remaining_utxo, nullptr);

  cardano_utxo_t* utxo = nullptr;
  error                = cardano_utxo_list_get(selection, 0, &utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);

  ASSERT_NE(output, nullptr);

  cardano_value_t* value = cardano_transaction_output_get_value(output);

  ASSERT_NE(value, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value), 4027026466);

  cardano_utxo_t* utxo1 = nullptr;
  error                 = cardano_utxo_list_get(selection, 1, &utxo1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output1 = cardano_utxo_get_output(utxo1);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value1 = cardano_transaction_output_get_value(output1);

  ASSERT_NE(value1, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value1), 4027026465);

  cardano_utxo_t* utxo2 = nullptr;
  error                 = cardano_utxo_list_get(remaining_utxo, 0, &utxo2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output2 = cardano_utxo_get_output(utxo2);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value2 = cardano_transaction_output_get_value(output2);

  ASSERT_NE(value2, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value2), 4027026464);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_utxo_list_unref(&available_utxo);
  cardano_value_unref(&target);
  cardano_utxo_unref(&utxo1);
  cardano_transaction_output_unref(&output1);
  cardano_value_unref(&value1);
  cardano_utxo_unref(&utxo2);
  cardano_transaction_output_unref(&output2);
  cardano_value_unref(&value2);

  cardano_coin_selector_unref(&large_first_coin_selector);
}

TEST(cardano_large_first_coin_selector_select, selectsTheLargestFirstAssetsWithPreSelected)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;
  cardano_utxo_list_t*     selection                 = nullptr;
  cardano_utxo_list_t*     remaining_utxo            = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo         = new_utxo_small_list();
  cardano_value_t*         target                    = new_default_value(VALUE);

  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  // Act
  error = cardano_coin_selector_select(large_first_coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_utxo_list_get_length(selection), 2);
  EXPECT_EQ(cardano_utxo_list_get_length(remaining_utxo), 1);

  ASSERT_NE(selection, nullptr);
  ASSERT_NE(remaining_utxo, nullptr);

  cardano_utxo_t* utxo = nullptr;
  error                = cardano_utxo_list_get(selection, 0, &utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);

  ASSERT_NE(output, nullptr);

  cardano_value_t* value = cardano_transaction_output_get_value(output);

  ASSERT_NE(value, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value), 4027026465);

  cardano_utxo_t* utxo1 = nullptr;
  error                 = cardano_utxo_list_get(selection, 1, &utxo1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output1 = cardano_utxo_get_output(utxo1);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value1 = cardano_transaction_output_get_value(output1);

  ASSERT_NE(value1, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value1), 4027026464);

  cardano_utxo_t* utxo2 = nullptr;
  error                 = cardano_utxo_list_get(remaining_utxo, 0, &utxo2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output2 = cardano_utxo_get_output(utxo2);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value2 = cardano_transaction_output_get_value(output2);

  ASSERT_NE(value2, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value2), 4027026466);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected_utxo);
  cardano_value_unref(&target);
  cardano_utxo_unref(&utxo1);
  cardano_transaction_output_unref(&output1);
  cardano_value_unref(&value1);
  cardano_utxo_unref(&utxo2);
  cardano_transaction_output_unref(&output2);
  cardano_value_unref(&value2);

  cardano_coin_selector_unref(&large_first_coin_selector);
}

TEST(cardano_large_first_coin_selector_select, selectsTheLargestSameValues)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;
  cardano_utxo_list_t*     selection                 = nullptr;
  cardano_utxo_list_t*     remaining_utxo            = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo         = new_utxo_small_list();
  cardano_value_t*         target                    = new_default_value(VALUE);

  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = new_utxo_list_same_utxo();

  // Act
  error = cardano_coin_selector_select(large_first_coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_utxo_list_get_length(selection), 2);
  EXPECT_EQ(cardano_utxo_list_get_length(remaining_utxo), 1);

  ASSERT_NE(selection, nullptr);
  ASSERT_NE(remaining_utxo, nullptr);

  cardano_utxo_t* utxo = nullptr;
  error                = cardano_utxo_list_get(selection, 0, &utxo);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);

  ASSERT_NE(output, nullptr);

  cardano_value_t* value = cardano_transaction_output_get_value(output);

  ASSERT_NE(value, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value), 4027026465);

  cardano_utxo_t* utxo1 = nullptr;
  error                 = cardano_utxo_list_get(selection, 1, &utxo1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output1 = cardano_utxo_get_output(utxo1);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value1 = cardano_transaction_output_get_value(output1);

  ASSERT_NE(value1, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value1), 4027026465);

  cardano_utxo_t* utxo2 = nullptr;
  error                 = cardano_utxo_list_get(remaining_utxo, 0, &utxo2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_transaction_output_t* output2 = cardano_utxo_get_output(utxo2);

  ASSERT_NE(output1, nullptr);

  cardano_value_t* value2 = cardano_transaction_output_get_value(output2);

  ASSERT_NE(value2, nullptr);

  EXPECT_EQ(cardano_value_get_coin(value2), 4027026465);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected_utxo);
  cardano_value_unref(&target);
  cardano_utxo_unref(&utxo1);
  cardano_transaction_output_unref(&output1);
  cardano_value_unref(&value1);
  cardano_utxo_unref(&utxo2);
  cardano_transaction_output_unref(&output2);
  cardano_value_unref(&value2);

  cardano_coin_selector_unref(&large_first_coin_selector);
}

TEST(cardano_large_first_coin_selector_select, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;
  cardano_utxo_list_t*     selection                 = nullptr;
  cardano_utxo_list_t*     remaining_utxo            = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo         = new_utxo_small_list();
  cardano_value_t*         target                    = new_default_value(VALUE);

  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_coin_selector_select(large_first_coin_selector, nullptr, nullptr, nullptr, &selection, &remaining_utxo);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&target);
  cardano_coin_selector_unref(&large_first_coin_selector);
  cardano_utxo_list_unref(&pre_selected_utxo);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
}

TEST(cardano_large_first_coin_selector_select, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_coin_selector_t* large_first_coin_selector = nullptr;
  cardano_utxo_list_t*     selection                 = nullptr;
  cardano_utxo_list_t*     remaining_utxo            = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo         = new_utxo_small_list();
  cardano_value_t*         target                    = new_default_value(VALUE);

  cardano_error_t error = cardano_large_first_coin_selector_new(&large_first_coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  for (int i = 0; i < 57; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    error = cardano_coin_selector_select(large_first_coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

    EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_value_unref(&target);
  cardano_coin_selector_unref(&large_first_coin_selector);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected_utxo);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
}

/* INTERNALS *****************************************************************/

TEST(_cardano_large_fist_get_amount, returnsAmountForAssetId)
{
  // Arrange
  cardano_value_t*    value    = new_default_value(VALUE);
  cardano_asset_id_t* asset_id = nullptr;

  ASSERT_EQ(cardano_asset_id_new_lovelace(&asset_id), CARDANO_SUCCESS);

  // Act
  int64_t amount = _cardano_large_fist_get_amount(value, asset_id);

  // Assert
  EXPECT_EQ(amount, 4027026465);

  // Cleanup
  cardano_value_unref(&value);
  cardano_asset_id_unref(&asset_id);
}

TEST(_cardano_large_fist_get_amount, returnsZeroIfGivenNull)
{
  // Act
  int64_t amount = _cardano_large_fist_get_amount(nullptr, nullptr);

  // Assert
  EXPECT_EQ(amount, 0);
}

TEST(_cardano_large_fist_get_amount, returnsZeroIfMemoryallocationFails)
{
  // Arrange
  cardano_value_t*        value    = new_default_value(VALUE);
  cardano_asset_id_t*     asset_id = nullptr;
  cardano_blake2b_hash_t* hash     = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_t*   name     = new_default_asset_name(ASSET_NAME_CBOR_1);

  ASSERT_EQ(cardano_asset_id_new(hash, name, &asset_id), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  int64_t amount = _cardano_large_fist_get_amount(value, asset_id);

  // Assert
  EXPECT_EQ(amount, 0);

  // Cleanup
  cardano_value_unref(&value);
  cardano_asset_id_unref(&asset_id);
  cardano_set_allocators(malloc, realloc, free);
  cardano_blake2b_hash_unref(&hash);
  cardano_asset_name_unref(&name);
}

TEST(_cardano_large_fist_value_gte, returnsTrueIfValueIsGreaterOrEqual)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(VALUE);
  cardano_value_t* value2 = new_default_value(VALUE);

  // Act
  bool result = false;

  ASSERT_EQ(_cardano_large_fist_value_gte(value1, value2, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(_cardano_large_fist_value_gte, returnsFalseIfValueIsLessThan)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("16");
  cardano_value_t* value2 = new_default_value(VALUE);

  // Act
  bool result = false;

  ASSERT_EQ(_cardano_large_fist_value_gte(value1, value2, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(_cardano_large_fist_value_gte, returnsFalseIfValueIsLessThan2)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("1AF0078C29");
  cardano_value_t* value2 = new_default_value(VALUE);

  // Act
  bool result = false;

  ASSERT_EQ(_cardano_large_fist_value_gte(value1, value2, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(_cardano_large_fist_value_gte, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(VALUE);
  cardano_value_t* value2 = new_default_value(VALUE);

  // Act
  bool result = false;

  ASSERT_EQ(_cardano_large_fist_value_gte(value1, nullptr, &result), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_value_gte(nullptr, value2, &result), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_value_gte(value1, value2, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(_cardano_large_fist_value_gte, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(VALUE);
  cardano_value_t* value2 = new_default_value(VALUE);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  bool result = false;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_value_gte(value1, value2, &result), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cardano_large_fist_check_preselected, returnsTrueIfPreselectedSatisfiesTarget)
{
  // Arrange
  cardano_utxo_list_t* pre_selected_utxo = new_utxo_small_list();
  cardano_value_t*     target            = new_default_value("10");
  cardano_value_t*     accumulated_value = nullptr;
  // Act
  bool result = false;

  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_value_unref(&target);
  cardano_utxo_list_unref(&pre_selected_utxo);
  cardano_value_unref(&accumulated_value);
}

TEST(_cardano_large_fist_check_preselected, returnsFalseIfPreselectedDoesNotSatisfiesTarget)
{
  // Arrange
  cardano_utxo_list_t* pre_selected_utxo = new_utxo_small_list();
  cardano_value_t*     target            = new_default_value(VALUE);
  cardano_value_t*     accumulated_value = nullptr;

  // Act
  bool result = false;

  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_SUCCESS);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&target);
  cardano_utxo_list_unref(&pre_selected_utxo);
  cardano_value_unref(&accumulated_value);
}

TEST(_cardano_large_fist_check_preselected, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_utxo_list_t* pre_selected_utxo = new_utxo_small_list();
  cardano_value_t*     target            = new_default_value(VALUE);
  cardano_value_t*     accumulated_value = nullptr;

  // Act
  bool result = false;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_four_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_five_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, &result), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_value_unref(&target);
  cardano_utxo_list_unref(&pre_selected_utxo);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cardano_large_fist_check_preselected, returnErrorIfGivenNull)
{
  // Arrange
  cardano_utxo_list_t* pre_selected_utxo = new_utxo_small_list();
  cardano_value_t*     target            = new_default_value(VALUE);
  cardano_value_t*     accumulated_value = nullptr;

  // Act
  bool result = false;

  ASSERT_EQ(_cardano_large_fist_check_preselected(nullptr, target, &accumulated_value, &result), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, nullptr, &accumulated_value, &result), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, nullptr, &result), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_check_preselected(pre_selected_utxo, target, &accumulated_value, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&target);
  cardano_utxo_list_unref(&pre_selected_utxo);
}

TEST(_cardano_large_fist_select_utxos, returnErrorIfGivenNull)
{
  // Arrange
  cardano_utxo_list_t* selection      = new_utxo_small_list();
  cardano_asset_id_t*  asset_id       = nullptr;
  cardano_value_t*     accum_val      = nullptr;
  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  ASSERT_EQ(cardano_asset_id_new_lovelace(&asset_id), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(_cardano_large_fist_select_utxos(nullptr, 0, available_utxo, selection, &accum_val), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 0, nullptr, selection, &accum_val), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 0, available_utxo, nullptr, &accum_val), CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 0, available_utxo, selection, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_utxo_list_unref(&selection);
  cardano_asset_id_unref(&asset_id);
  cardano_value_unref(&accum_val);
  cardano_utxo_list_unref(&available_utxo);
}

TEST(_cardano_large_fist_select_utxos, returnErrorIfUnsufficientBalance)
{
  // Arrange
  cardano_utxo_list_t* selection      = new_utxo_small_list();
  cardano_asset_id_t*  asset_id       = nullptr;
  cardano_value_t*     accum_val      = nullptr;
  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  ASSERT_EQ(cardano_asset_id_new_lovelace(&asset_id), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 99999999999, available_utxo, selection, &accum_val), CARDANO_ERROR_BALANCE_INSUFFICIENT);

  // Cleanup
  cardano_utxo_list_unref(&selection);
  cardano_asset_id_unref(&asset_id);
  cardano_value_unref(&accum_val);
  cardano_utxo_list_unref(&available_utxo);
}

TEST(_cardano_large_fist_select_utxos, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_utxo_list_t* selection      = new_utxo_small_list();
  cardano_asset_id_t*  asset_id       = nullptr;
  cardano_value_t*     accum_val      = nullptr;
  cardano_utxo_list_t* available_utxo = new_utxo_list_diff_vals();

  ASSERT_EQ(cardano_asset_id_new_lovelace(&asset_id), CARDANO_SUCCESS);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 0, available_utxo, selection, &accum_val), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 0, available_utxo, selection, &accum_val), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 0, available_utxo, selection, &accum_val), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);
  ASSERT_EQ(_cardano_large_fist_select_utxos(asset_id, 0, available_utxo, selection, &accum_val), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_utxo_list_unref(&selection);
  cardano_asset_id_unref(&asset_id);
  cardano_value_unref(&accum_val);
  cardano_utxo_list_unref(&available_utxo);
  cardano_set_allocators(malloc, realloc, free);
}