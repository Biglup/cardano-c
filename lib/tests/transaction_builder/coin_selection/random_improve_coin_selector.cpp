/**
 * \file random_improve_coin_selector.cpp
 *
 * \author angel.castillo
 * \date   Jul 02, 2026
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

#include <cardano/address/address.h>
#include <cardano/assets/asset_id.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>
#include <cardano/transaction_builder/fee.h>

#include <gmock/gmock.h>

#include <string>
#include <vector>

extern "C" {
#include "../../../src/transaction_builder/coin_selection/internals/random_improve_helpers.h"
}

/* STATIC FUNCTIONS **********************************************************/

static cardano_address_t*
new_address(const char* address)
{
  cardano_address_t* result = NULL;

  EXPECT_EQ(cardano_address_from_string(address, strlen(address), &result), CARDANO_SUCCESS);

  return result;
}

static cardano_protocol_parameters_t*
new_protocol_parameters()
{
  cardano_protocol_parameters_t* params = NULL;

  EXPECT_EQ(cardano_protocol_parameters_new(&params), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ada_per_utxo_byte(params, 4310U), CARDANO_SUCCESS);

  return params;
}

static cardano_utxo_t*
new_ada_utxo(const uint64_t ordinal, const int64_t coin, cardano_address_t* owner)
{
  char hex[65] = { 0 };

  EXPECT_EQ(snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal), 64);

  cardano_blake2b_hash_t* id = NULL;
  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, 64, &id), CARDANO_SUCCESS);

  cardano_transaction_input_t* input = NULL;
  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_new(owner, (uint64_t)coin, &output), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = NULL;
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);

  return utxo;
}

static const char* CHANGE_ADDRESS = "addr_test1qqydn46r6mhge0kfpqmt36m6q43knzsd9ga32n96m89px3nuzcjqw982pcftgx53fu5527z2cj2tkx2h8ux2vxsg475qypp3m9";
static const char* WALLET_ADDRESS = "addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk";

static cardano_error_t
do_select(
  cardano_coin_selector_t*            selector,
  cardano_utxo_list_t*                pre_selected_utxo,
  cardano_utxo_list_t*                available_utxo,
  cardano_value_t*                    target,
  cardano_transaction_output_list_t*  outputs_to_cover,
  cardano_address_t*                  change_address,
  cardano_protocol_parameters_t*      protocol_params,
  cardano_utxo_list_t**               selection,
  cardano_utxo_list_t**               remaining_utxo,
  cardano_transaction_output_list_t** change_outputs)
{
  cardano_coin_selection_request_t request = { 0 };

  request.pre_selected_utxo = pre_selected_utxo;
  request.available_utxo    = available_utxo;
  request.target            = target;
  request.outputs_to_cover  = outputs_to_cover;
  request.change_address    = change_address;
  request.protocol_params   = protocol_params;

  return cardano_coin_selector_select(selector, &request, selection, remaining_utxo, change_outputs);
}

/* HELPER UNIT TESTS *********************************************************/

TEST(_cardano_random_improve_partition, preservesTheSumAndProportions)
{
  const uint64_t weights[4] = { 1U, 2U, 3U, 4U };
  uint64_t       parts[4]   = { 0U };

  ASSERT_EQ(_cardano_random_improve_partition(100U, weights, 4U, parts), CARDANO_SUCCESS);

  EXPECT_EQ(parts[0] + parts[1] + parts[2] + parts[3], 100U);
  EXPECT_EQ(parts[0], 10U);
  EXPECT_EQ(parts[1], 20U);
  EXPECT_EQ(parts[2], 30U);
  EXPECT_EQ(parts[3], 40U);
}

TEST(_cardano_random_improve_partition, distributesRemaindersToLargestFractions)
{
  const uint64_t weights[3] = { 1U, 1U, 1U };
  uint64_t       parts[3]   = { 0U };

  ASSERT_EQ(_cardano_random_improve_partition(10U, weights, 3U, parts), CARDANO_SUCCESS);

  EXPECT_EQ(parts[0] + parts[1] + parts[2], 10U);

  uint64_t largest  = 0U;
  uint64_t smallest = UINT64_MAX;

  for (size_t i = 0U; i < 3U; ++i)
  {
    largest  = (parts[i] > largest) ? parts[i] : largest;
    smallest = (parts[i] < smallest) ? parts[i] : smallest;
  }

  EXPECT_LE(largest - smallest, 1U);
}

TEST(_cardano_random_improve_partition, preservesSumWithLargeValues)
{
  const uint64_t weights[3] = { 4027026466ULL, 999999999999ULL, 1ULL };
  uint64_t       parts[3]   = { 0U };

  const uint64_t target = 45000000000000000ULL;

  ASSERT_EQ(_cardano_random_improve_partition(target, weights, 3U, parts), CARDANO_SUCCESS);

  EXPECT_EQ(parts[0] + parts[1] + parts[2], target);
}

TEST(_cardano_random_improve_partition, returnsErrorIfAllWeightsAreZero)
{
  const uint64_t weights[2] = { 0U, 0U };
  uint64_t       parts[2]   = { 0U };

  EXPECT_EQ(_cardano_random_improve_partition(10U, weights, 2U, parts), CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(_cardano_random_improve_pad_coalesce, padsShortListsWithZeros)
{
  const uint64_t quantities[1] = { 1U };
  uint64_t       result[4]     = { 0U };

  ASSERT_EQ(_cardano_random_improve_pad_coalesce(quantities, 1U, 4U, result), CARDANO_SUCCESS);

  EXPECT_EQ(result[0], 0U);
  EXPECT_EQ(result[1], 0U);
  EXPECT_EQ(result[2], 0U);
  EXPECT_EQ(result[3], 1U);
}

TEST(_cardano_random_improve_pad_coalesce, coalescesLongListsPreservingSumAndOrder)
{
  const uint64_t quantities[4] = { 8U, 4U, 2U, 1U };

  uint64_t result3[3] = { 0U };
  ASSERT_EQ(_cardano_random_improve_pad_coalesce(quantities, 4U, 3U, result3), CARDANO_SUCCESS);
  EXPECT_EQ(result3[0], 3U);
  EXPECT_EQ(result3[1], 4U);
  EXPECT_EQ(result3[2], 8U);

  uint64_t result2[2] = { 0U };
  ASSERT_EQ(_cardano_random_improve_pad_coalesce(quantities, 4U, 2U, result2), CARDANO_SUCCESS);
  EXPECT_EQ(result2[0], 7U);
  EXPECT_EQ(result2[1], 8U);

  uint64_t result1[1] = { 0U };
  ASSERT_EQ(_cardano_random_improve_pad_coalesce(quantities, 4U, 1U, result1), CARDANO_SUCCESS);
  EXPECT_EQ(result1[0], 15U);
}

TEST(_cardano_random_improve_reduce_quantities, reducesFromTheSmallestFirst)
{
  uint64_t quantities[5] = { 0U, 1U, 2U, 3U, 4U };

  _cardano_random_improve_reduce_quantities(4U, quantities, 5U);

  EXPECT_EQ(quantities[0], 0U);
  EXPECT_EQ(quantities[1], 0U);
  EXPECT_EQ(quantities[2], 0U);
  EXPECT_EQ(quantities[3], 2U);
  EXPECT_EQ(quantities[4], 4U);
}

/* SELECTOR TESTS ************************************************************/

TEST(cardano_random_improve_coin_selector_new, createsASelector)
{
  cardano_coin_selector_t* selector = NULL;

  ASSERT_EQ(cardano_random_improve_coin_selector_new(&selector), CARDANO_SUCCESS);
  ASSERT_NE(selector, nullptr);
  EXPECT_STREQ(cardano_coin_selector_get_name(selector), "Random improve coin selector");

  cardano_coin_selector_unref(&selector);
}

TEST(cardano_random_improve_coin_selector_new, returnsErrorIfGivenNull)
{
  EXPECT_EQ(cardano_random_improve_coin_selector_new(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_random_improve_coin_selector_new_with_seed(42U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_random_improve_coin_selector_select, mimicsTheNumberOfUserOutputs)
{
  cardano_coin_selector_t*       selector        = NULL;
  cardano_address_t*             change_address  = new_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = new_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = new_protocol_parameters();

  ASSERT_EQ(cardano_random_improve_coin_selector_new_with_seed(42U, &selector), CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  for (uint64_t i = 1U; i <= 8U; ++i)
  {
    cardano_utxo_t* utxo = new_ada_utxo(i, 10000000, wallet_address);

    ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);
    cardano_utxo_unref(&utxo);
  }

  cardano_transaction_output_list_t* outputs = NULL;
  ASSERT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);

  for (uint64_t i = 0U; i < 2U; ++i)
  {
    cardano_transaction_output_t* output = NULL;

    ASSERT_EQ(cardano_transaction_output_new(wallet_address, 5000000U, &output), CARDANO_SUCCESS);
    ASSERT_EQ(cardano_transaction_output_list_add(outputs, output), CARDANO_SUCCESS);
    cardano_transaction_output_unref(&output);
  }

  cardano_value_t* target = cardano_value_new_from_coin(10000000);

  cardano_utxo_list_t*               selection      = NULL;
  cardano_utxo_list_t*               remaining_utxo = NULL;
  cardano_transaction_output_list_t* change_outputs = NULL;

  ASSERT_EQ(
    do_select(selector, NULL, available_utxo, target, outputs, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
    CARDANO_SUCCESS);

  EXPECT_EQ(cardano_transaction_output_list_get_length(change_outputs), 2U);

  int64_t selected_coin = 0;

  for (size_t i = 0U; i < cardano_utxo_list_get_length(selection); ++i)
  {
    cardano_utxo_t* utxo = NULL;
    ASSERT_EQ(cardano_utxo_list_get(selection, i, &utxo), CARDANO_SUCCESS);

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_value_t*              value  = cardano_transaction_output_get_value(output);

    selected_coin += cardano_value_get_coin(value);

    cardano_value_unref(&value);
    cardano_transaction_output_unref(&output);
    cardano_utxo_unref(&utxo);
  }

  int64_t change_coin = 0;

  for (size_t i = 0U; i < cardano_transaction_output_list_get_length(change_outputs); ++i)
  {
    cardano_transaction_output_t* output = NULL;
    ASSERT_EQ(cardano_transaction_output_list_get(change_outputs, i, &output), CARDANO_SUCCESS);

    cardano_value_t* value = cardano_transaction_output_get_value(output);

    change_coin += cardano_value_get_coin(value);

    uint64_t min_ada = 0U;
    ASSERT_EQ(cardano_compute_min_ada_required(output, 4310U, &min_ada), CARDANO_SUCCESS);
    EXPECT_GE(cardano_value_get_coin(value), (int64_t)min_ada);

    cardano_value_unref(&value);
    cardano_transaction_output_unref(&output);
  }

  EXPECT_EQ(selected_coin, 10000000 + change_coin);

  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_transaction_output_list_unref(&change_outputs);
  cardano_value_unref(&target);
  cardano_transaction_output_list_unref(&outputs);
  cardano_utxo_list_unref(&available_utxo);
  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_random_improve_coin_selector_select, isDeterministicForAFixedSeed)
{
  cardano_address_t*             change_address  = new_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = new_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = new_protocol_parameters();

  size_t selection_sizes[2] = { 0U };

  for (size_t run = 0U; run < 2U; ++run)
  {
    cardano_coin_selector_t* selector = NULL;

    ASSERT_EQ(cardano_random_improve_coin_selector_new_with_seed(7U, &selector), CARDANO_SUCCESS);

    cardano_utxo_list_t* available_utxo = NULL;
    ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

    for (uint64_t i = 1U; i <= 20U; ++i)
    {
      cardano_utxo_t* utxo = new_ada_utxo(i, (int64_t)(1000000U * i), wallet_address);

      ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);
      cardano_utxo_unref(&utxo);
    }

    cardano_value_t* target = cardano_value_new_from_coin(15000000);

    cardano_utxo_list_t*               selection      = NULL;
    cardano_utxo_list_t*               remaining_utxo = NULL;
    cardano_transaction_output_list_t* change_outputs = NULL;

    ASSERT_EQ(
      do_select(selector, NULL, available_utxo, target, NULL, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
      CARDANO_SUCCESS);

    selection_sizes[run] = cardano_utxo_list_get_length(selection);

    cardano_utxo_list_unref(&selection);
    cardano_utxo_list_unref(&remaining_utxo);
    cardano_transaction_output_list_unref(&change_outputs);
    cardano_value_unref(&target);
    cardano_utxo_list_unref(&available_utxo);
    cardano_coin_selector_unref(&selector);
  }

  EXPECT_EQ(selection_sizes[0], selection_sizes[1]);

  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
}

TEST(cardano_random_improve_coin_selector_select, failsWhenBalanceInsufficient)
{
  cardano_coin_selector_t*       selector        = NULL;
  cardano_address_t*             change_address  = new_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = new_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = new_protocol_parameters();

  ASSERT_EQ(cardano_random_improve_coin_selector_new_with_seed(42U, &selector), CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = new_ada_utxo(1U, 1000000, wallet_address);
  ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);
  cardano_utxo_unref(&utxo);

  cardano_value_t* target = cardano_value_new_from_coin(5000000);

  cardano_utxo_list_t*               selection      = NULL;
  cardano_utxo_list_t*               remaining_utxo = NULL;
  cardano_transaction_output_list_t* change_outputs = NULL;

  EXPECT_EQ(
    do_select(selector, NULL, available_utxo, target, NULL, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
    CARDANO_ERROR_BALANCE_INSUFFICIENT);

  cardano_value_unref(&target);
  cardano_utxo_list_unref(&available_utxo);
  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_random_improve_coin_selector_select, repeatedCallsOnTheSameSelectorAreReproducible)
{
  // The balancer invokes the selector once per fee-convergence iteration. The RNG is re-seeded on
  // every call, so identical inputs must yield identical selections across repeated invocations.
  cardano_coin_selector_t*       selector        = NULL;
  cardano_address_t*             change_address  = new_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = new_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = new_protocol_parameters();

  ASSERT_EQ(cardano_random_improve_coin_selector_new_with_seed(7U, &selector), CARDANO_SUCCESS);

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  for (uint64_t i = 1U; i <= 20U; ++i)
  {
    cardano_utxo_t* utxo = new_ada_utxo(i, (int64_t)(1000000U * i), wallet_address);

    ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);
    cardano_utxo_unref(&utxo);
  }

  cardano_value_t* target = cardano_value_new_from_coin(15000000);

  std::vector<std::string> first_run_inputs;

  for (size_t run = 0U; run < 2U; ++run)
  {
    cardano_utxo_list_t*               selection      = NULL;
    cardano_utxo_list_t*               remaining_utxo = NULL;
    cardano_transaction_output_list_t* change_outputs = NULL;

    ASSERT_EQ(
      do_select(selector, NULL, available_utxo, target, NULL, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
      CARDANO_SUCCESS);

    std::vector<std::string> run_inputs;

    for (size_t i = 0U; i < cardano_utxo_list_get_length(selection); ++i)
    {
      cardano_utxo_t* utxo = NULL;
      ASSERT_EQ(cardano_utxo_list_get(selection, i, &utxo), CARDANO_SUCCESS);

      cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
      cardano_blake2b_hash_t*      id    = cardano_transaction_input_get_id(input);

      char hex[65] = { 0 };
      ASSERT_EQ(cardano_blake2b_hash_to_hex(id, hex, sizeof(hex)), CARDANO_SUCCESS);

      run_inputs.push_back(std::string(hex));

      cardano_blake2b_hash_unref(&id);
      cardano_transaction_input_unref(&input);
      cardano_utxo_unref(&utxo);
    }

    if (run == 0U)
    {
      first_run_inputs = run_inputs;
    }
    else
    {
      EXPECT_EQ(first_run_inputs, run_inputs);
    }

    cardano_utxo_list_unref(&selection);
    cardano_utxo_list_unref(&remaining_utxo);
    cardano_transaction_output_list_unref(&change_outputs);
  }

  cardano_value_unref(&target);
  cardano_utxo_list_unref(&available_utxo);
  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

/* REFERENCE VECTORS *********************************************************/

// The following vectors are ported from the unit tests of the reference implementation
// (cardano-foundation/cardano-coin-selection, BalanceSpec.hs).

TEST(_cardano_random_improve_partition, matchesReferenceVectorsForCoinDistribution)
{
  // unit_makeChangeForCoin
  {
    const uint64_t weights[1] = { 1U };
    uint64_t       parts[1]   = { 0U };

    ASSERT_EQ(_cardano_random_improve_partition(1U, weights, 1U, parts), CARDANO_SUCCESS);
    EXPECT_EQ(parts[0], 1U);
  }
  {
    const uint64_t weights[3] = { 1U, 2U, 3U };
    uint64_t       parts[3]   = { 0U };

    ASSERT_EQ(_cardano_random_improve_partition(12U, weights, 3U, parts), CARDANO_SUCCESS);
    EXPECT_EQ(parts[0], 2U);
    EXPECT_EQ(parts[1], 4U);
    EXPECT_EQ(parts[2], 6U);
  }
  {
    const uint64_t weights[3] = { 1U, 2U, 3U };
    uint64_t       parts[3]   = { 0U };

    ASSERT_EQ(_cardano_random_improve_partition(5U, weights, 3U, parts), CARDANO_SUCCESS);
    EXPECT_EQ(parts[0], 1U);
    EXPECT_EQ(parts[1], 2U);
    EXPECT_EQ(parts[2], 2U);
  }
}

TEST(_cardano_random_improve_partition, matchesReferenceVectorsForUserSpecifiedAssets)
{
  // unit_makeChangeForUserSpecifiedAsset
  {
    const uint64_t weights[1] = { 1U };
    uint64_t       parts[1]   = { 0U };

    ASSERT_EQ(_cardano_random_improve_partition(3U, weights, 1U, parts), CARDANO_SUCCESS);
    EXPECT_EQ(parts[0], 3U);
  }
  {
    const uint64_t weights[2] = { 1U, 2U };
    uint64_t       parts[2]   = { 0U };

    ASSERT_EQ(_cardano_random_improve_partition(3U, weights, 2U, parts), CARDANO_SUCCESS);
    EXPECT_EQ(parts[0], 1U);
    EXPECT_EQ(parts[1], 2U);
  }
}

TEST(_cardano_random_improve_pad_coalesce, matchesReferenceVectorsForNonUserSpecifiedAssets)
{
  // unit_makeChangeForNonUserSpecifiedAsset
  {
    const uint64_t quantities[2] = { 1U, 1U };
    uint64_t       result[2]     = { 0U };

    ASSERT_EQ(_cardano_random_improve_pad_coalesce(quantities, 2U, 2U, result), CARDANO_SUCCESS);
    EXPECT_EQ(result[0], 1U);
    EXPECT_EQ(result[1], 1U);
  }
  {
    const uint64_t quantities[3] = { 1U, 1U, 1U };
    uint64_t       result[2]     = { 0U };

    ASSERT_EQ(_cardano_random_improve_pad_coalesce(quantities, 3U, 2U, result), CARDANO_SUCCESS);
    EXPECT_EQ(result[0], 1U);
    EXPECT_EQ(result[1], 2U);
  }
  {
    const uint64_t quantities[1] = { 1U };
    uint64_t       result[2]     = { 0U };

    ASSERT_EQ(_cardano_random_improve_pad_coalesce(quantities, 1U, 2U, result), CARDANO_SUCCESS);
    EXPECT_EQ(result[0], 0U);
    EXPECT_EQ(result[1], 1U);
  }
}

/* SELECTION STRATEGY ********************************************************/

TEST(cardano_random_improve_coin_selector_new_with_options, returnsErrorIfGivenNull)
{
  EXPECT_EQ(
    cardano_random_improve_coin_selector_new_with_options(42U, CARDANO_SELECTION_STRATEGY_MINIMAL, nullptr),
    CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_random_improve_coin_selector_select, minimalStrategySelectsNoMoreInputsThanOptimal)
{
  cardano_address_t*             change_address  = new_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = new_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = new_protocol_parameters();

  size_t selected_counts[2] = { 0U, 0U };

  const cardano_selection_strategy_t strategies[2] = {
    CARDANO_SELECTION_STRATEGY_MINIMAL,
    CARDANO_SELECTION_STRATEGY_OPTIMAL
  };

  for (size_t run = 0U; run < 2U; ++run)
  {
    cardano_coin_selector_t* selector = NULL;

    ASSERT_EQ(cardano_random_improve_coin_selector_new_with_options(7U, strategies[run], &selector), CARDANO_SUCCESS);

    cardano_utxo_list_t* available_utxo = NULL;
    ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

    for (uint64_t i = 1U; i <= 20U; ++i)
    {
      cardano_utxo_t* utxo = new_ada_utxo(i, 5000000, wallet_address);

      ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);
      cardano_utxo_unref(&utxo);
    }

    cardano_value_t* target = cardano_value_new_from_coin(18000000);

    cardano_utxo_list_t*               selection      = NULL;
    cardano_utxo_list_t*               remaining_utxo = NULL;
    cardano_transaction_output_list_t* change_outputs = NULL;

    ASSERT_EQ(
      do_select(selector, NULL, available_utxo, target, NULL, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
      CARDANO_SUCCESS);

    selected_counts[run] = cardano_utxo_list_get_length(selection);

    cardano_utxo_list_unref(&selection);
    cardano_utxo_list_unref(&remaining_utxo);
    cardano_transaction_output_list_unref(&change_outputs);
    cardano_value_unref(&target);
    cardano_utxo_list_unref(&available_utxo);
    cardano_coin_selector_unref(&selector);
  }

  // With 5 ada UTXOs and an 18 ada target, the minimal strategy stops at the minimum (4 inputs,
  // leaving 2 ada of change) while the optimal strategy improves toward twice the target.
  EXPECT_EQ(selected_counts[0], 4U);
  EXPECT_GT(selected_counts[1], selected_counts[0]);

  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
}
