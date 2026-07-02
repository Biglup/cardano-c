/**
 * \file coin_selection_properties.cpp
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
#include <cardano/assets/asset_id_map.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/transaction_builder/fee.h>

#include <gmock/gmock.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <vector>

/* CONSTANTS *****************************************************************/

static const char* CHANGE_ADDRESS = "addr_test1qqydn46r6mhge0kfpqmt36m6q43knzsd9ga32n96m89px3nuzcjqw982pcftgx53fu5527z2cj2tkx2h8ux2vxsg475qypp3m9";
static const char* WALLET_ADDRESS = "addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk";

static const uint64_t ADA_PER_UTXO_BYTE = 4310U;
static const uint64_t PROPERTY_SEED     = 0xC0FFEE42U;
static const size_t   ITERATIONS        = 2000U;

static const size_t ASSET_POOL_SIZE = 8U;

static const struct
{
    const char* policy;
    const char* name;
} ASSET_POOL[ASSET_POOL_SIZE] = {
  { "1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601", "TSLA" },
  { "1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601", "PXL" },
  { "1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601", "Unit" },
  { "659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82", "A" },
  { "659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82", "B" },
  { "659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82", "C" },
  { "7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "D" },
  { "7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "MACS" }
};

/* GENERATORS ****************************************************************/

/**
 * \brief Plain integer record of a generated value; the oracle computes coverage and
 * balance expectations from these records with plain integer arithmetic, independently
 * of the cardano_value_t arithmetic under test.
 */
struct gen_value_t
{
    int64_t                              coin;
    std::array<int64_t, ASSET_POOL_SIZE> assets;

    gen_value_t()
    : coin(0)
    {
      assets.fill(0);
    }

    bool
    is_zero() const
    {
      return (coin == 0) && std::all_of(assets.begin(), assets.end(), [](const int64_t amount)
                                        { return amount == 0; });
    }
};

/**
 * \brief Deterministic, platform-independent RNG (splitmix64).
 */
struct prop_rng_t
{
    uint64_t state;

    explicit prop_rng_t(const uint64_t seed)
    : state(seed)
    {
    }

    uint64_t
    next()
    {
      state += 0x9e3779b97f4a7c15ULL;

      uint64_t z = state;
      z          = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
      z          = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;

      return z ^ (z >> 31U);
    }

    int64_t
    range(const int64_t lo, const int64_t hi)
    {
      return lo + (int64_t)(next() % (uint64_t)(hi - lo + 1));
    }
};

static int64_t
gen_coin_amount(prop_rng_t& rng)
{
  switch (rng.next() % 5U)
  {
    case 0U:
      return rng.range(1, 1000);
    case 1U:
      return rng.range(800000, 1600000);
    case 2U:
      return rng.range(1000000, 5000000);
    case 3U:
      return rng.range(5000000, 50000000);
    default:
      return rng.range(1000000000, 1000000000000);
  }
}

static int64_t
gen_asset_amount(prop_rng_t& rng)
{
  if ((rng.next() % 10U) == 0U)
  {
    return rng.range(1000000000, 1000000000000);
  }

  return rng.range(1, 1000000);
}

static void
gen_assets(prop_rng_t& rng, gen_value_t& value)
{
  const size_t count = rng.next() % 5U;

  for (size_t i = 0U; i < count; ++i)
  {
    const size_t index  = rng.next() % ASSET_POOL_SIZE;
    value.assets[index] = gen_asset_amount(rng);
  }
}

static gen_value_t
gen_utxo_value(prop_rng_t& rng)
{
  gen_value_t value;

  value.coin = gen_coin_amount(rng);
  gen_assets(rng, value);

  return value;
}

/**
 * \brief Generates a target value. Mirrors the JS SDK generator, where negative coin models
 * implicit inputs (withdrawals/deposit refunds) and negative asset amounts model minting.
 */
static gen_value_t
gen_target_value(prop_rng_t& rng)
{
  gen_value_t value;

  switch (rng.next() % 10U)
  {
    case 0U:
      return value;
    case 1U:
      value.coin = -rng.range(1, 500000000);
      gen_assets(rng, value);
      return value;
    case 2U:
      value.coin = gen_coin_amount(rng);
      gen_assets(rng, value);
      value.assets[rng.next() % ASSET_POOL_SIZE] = -gen_asset_amount(rng);
      return value;
    default:
      value.coin = gen_coin_amount(rng);
      gen_assets(rng, value);
      return value;
  }
}

/* HELPERS *******************************************************************/

static cardano_address_t*
make_address(const char* address)
{
  cardano_address_t* result = NULL;

  EXPECT_EQ(cardano_address_from_string(address, strlen(address), &result), CARDANO_SUCCESS);

  return result;
}

static cardano_protocol_parameters_t*
make_protocol_parameters()
{
  cardano_protocol_parameters_t* params = NULL;

  EXPECT_EQ(cardano_protocol_parameters_new(&params), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ada_per_utxo_byte(params, ADA_PER_UTXO_BYTE), CARDANO_SUCCESS);

  return params;
}

static cardano_asset_id_t*
make_pool_asset_id(const size_t index)
{
  cardano_blake2b_hash_t* policy = NULL;
  cardano_asset_name_t*   name   = NULL;
  cardano_asset_id_t*     id     = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(ASSET_POOL[index].policy, strlen(ASSET_POOL[index].policy), &policy), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_from_bytes((const uint8_t*)ASSET_POOL[index].name, strlen(ASSET_POOL[index].name), &name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_new(policy, name, &id), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&policy);
  cardano_asset_name_unref(&name);

  return id;
}

static cardano_value_t*
build_value(const gen_value_t& gen)
{
  cardano_multi_asset_t* multi_asset = NULL;

  EXPECT_EQ(cardano_multi_asset_new(&multi_asset), CARDANO_SUCCESS);

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    if (gen.assets[i] == 0)
    {
      continue;
    }

    cardano_blake2b_hash_t* policy = NULL;
    cardano_asset_name_t*   name   = NULL;

    EXPECT_EQ(cardano_blake2b_hash_from_hex(ASSET_POOL[i].policy, strlen(ASSET_POOL[i].policy), &policy), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_asset_name_from_bytes((const uint8_t*)ASSET_POOL[i].name, strlen(ASSET_POOL[i].name), &name), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_multi_asset_set(multi_asset, policy, name, gen.assets[i]), CARDANO_SUCCESS);

    cardano_blake2b_hash_unref(&policy);
    cardano_asset_name_unref(&name);
  }

  cardano_value_t* value = NULL;

  EXPECT_EQ(cardano_value_new(gen.coin, multi_asset, &value), CARDANO_SUCCESS);

  cardano_multi_asset_unref(&multi_asset);

  return value;
}

static cardano_utxo_t*
build_utxo(const uint64_t ordinal, const gen_value_t& gen, cardano_address_t* owner)
{
  char hex[65] = { 0 };

  const int written = snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal);
  EXPECT_EQ(written, 64);

  cardano_blake2b_hash_t* id = NULL;
  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, 64, &id), CARDANO_SUCCESS);

  cardano_transaction_input_t* input = NULL;
  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_new(owner, 0, &output), CARDANO_SUCCESS);

  cardano_value_t* value = build_value(gen);
  EXPECT_EQ(cardano_transaction_output_set_value(output, value), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = NULL;
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return utxo;
}

/**
 * \brief Reads a cardano_value_t back into gen-space (coin + pool asset amounts).
 */
static gen_value_t
read_value(cardano_value_t* value)
{
  gen_value_t gen;

  gen.coin = cardano_value_get_coin(value);

  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  if (multi_asset == NULL)
  {
    return gen;
  }

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    cardano_asset_id_t* id     = make_pool_asset_id(i);
    int64_t             amount = 0;

    if (cardano_multi_asset_get_with_id(multi_asset, id, &amount) == CARDANO_SUCCESS)
    {
      gen.assets[i] = amount;
    }

    cardano_asset_id_unref(&id);
  }

  return gen;
}

static gen_value_t
sum_gen_values(const std::vector<gen_value_t>& values)
{
  gen_value_t total;

  for (const gen_value_t& value: values)
  {
    total.coin += value.coin;

    for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
    {
      total.assets[i] += value.assets[i];
    }
  }

  return total;
}

static bool
gen_value_geq(const gen_value_t& lhs, const gen_value_t& rhs)
{
  if (lhs.coin < rhs.coin)
  {
    return false;
  }

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    if (lhs.assets[i] < rhs.assets[i])
    {
      return false;
    }
  }

  return true;
}

static gen_value_t
gen_value_subtract(const gen_value_t& lhs, const gen_value_t& rhs)
{
  gen_value_t result;

  result.coin = lhs.coin - rhs.coin;

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    result.assets[i] = lhs.assets[i] - rhs.assets[i];
  }

  return result;
}

static std::string
gen_value_to_string(const gen_value_t& value)
{
  std::ostringstream stream;

  stream << "{coin:" << value.coin;

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    if (value.assets[i] != 0)
    {
      stream << ", " << ASSET_POOL[i].name << ":" << value.assets[i];
    }
  }

  stream << "}";

  return stream.str();
}

static uint64_t
min_ada_for_value(const gen_value_t& gen, cardano_address_t* change_address)
{
  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_new(change_address, 0, &output), CARDANO_SUCCESS);

  cardano_value_t* value = build_value(gen);
  EXPECT_EQ(cardano_transaction_output_set_value(output, value), CARDANO_SUCCESS);

  uint64_t min_ada = 0U;
  EXPECT_EQ(cardano_compute_min_ada_required(output, ADA_PER_UTXO_BYTE, &min_ada), CARDANO_SUCCESS);

  cardano_value_unref(&value);
  cardano_transaction_output_unref(&output);

  return min_ada;
}

static bool
utxo_list_contains(cardano_utxo_list_t* list, cardano_utxo_t* utxo)
{
  const size_t length = cardano_utxo_list_get_length(list);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_utxo_t* current = NULL;

    EXPECT_EQ(cardano_utxo_list_get(list, i, &current), CARDANO_SUCCESS);
    cardano_utxo_unref(&current);

    if (current == utxo)
    {
      return true;
    }
  }

  return false;
}

/* PROPERTY ASSERTIONS *******************************************************/

/**
 * \brief Port of the JS SDK `assertInputSelectionProperties`, adapted to the cardano-c
 * selector contract (coalesced target instead of outputs + implicit value).
 */
static void
assert_input_selection_properties(
  const std::vector<cardano_utxo_t*>& available,
  const std::vector<cardano_utxo_t*>& pre_selected,
  const std::vector<gen_value_t>&     pool_values,
  const std::vector<cardano_utxo_t*>& pool_utxos,
  const gen_value_t&                  target,
  cardano_utxo_list_t*                selection,
  cardano_utxo_list_t*                remaining_utxo,
  cardano_transaction_output_list_t*  change_outputs,
  cardano_address_t*                  change_address)
{
  ASSERT_NE(selection, nullptr);
  ASSERT_NE(remaining_utxo, nullptr);
  ASSERT_NE(change_outputs, nullptr);

  // Must select at least one input
  EXPECT_GT(cardano_utxo_list_get_length(selection), 0U);

  // Selected values, resolved from the generator records by pointer identity
  std::vector<gen_value_t> selected_values;

  const size_t selection_size = cardano_utxo_list_get_length(selection);

  for (size_t i = 0U; i < selection_size; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    ASSERT_EQ(cardano_utxo_list_get(selection, i, &utxo), CARDANO_SUCCESS);
    cardano_utxo_unref(&utxo);

    bool found = false;

    for (size_t j = 0U; j < pool_utxos.size(); ++j)
    {
      if (pool_utxos[j] == utxo)
      {
        selected_values.push_back(pool_values[j]);
        found = true;
        break;
      }
    }

    // Selection must only contain UTXOs from the pool
    EXPECT_TRUE(found);
  }

  const gen_value_t selected_total = sum_gen_values(selected_values);

  // Total change, read back from the change outputs
  std::vector<gen_value_t> change_values;

  const size_t change_count = cardano_transaction_output_list_get_length(change_outputs);

  for (size_t i = 0U; i < change_count; ++i)
  {
    cardano_transaction_output_t* output = NULL;

    ASSERT_EQ(cardano_transaction_output_list_get(change_outputs, i, &output), CARDANO_SUCCESS);

    cardano_value_t* value = cardano_transaction_output_get_value(output);

    change_values.push_back(read_value(value));

    // Min UTxO coin requirement for change
    uint64_t min_ada = 0U;
    ASSERT_EQ(cardano_compute_min_ada_required(output, ADA_PER_UTXO_BYTE, &min_ada), CARDANO_SUCCESS);
    EXPECT_GE(cardano_value_get_coin(value), (int64_t)min_ada);

    // No empty change bundles
    EXPECT_FALSE(cardano_value_is_zero(value));

    // No 0 quantity assets
    cardano_asset_id_map_t* assets = cardano_value_as_assets_map(value);
    ASSERT_NE(assets, nullptr);

    const size_t asset_count = cardano_asset_id_map_get_length(assets);

    for (size_t j = 0U; j < asset_count; ++j)
    {
      cardano_asset_id_t* id = NULL;

      ASSERT_EQ(cardano_asset_id_map_get_key_at(assets, j, &id), CARDANO_SUCCESS);
      cardano_asset_id_unref(&id);

      if (cardano_asset_id_is_lovelace(id))
      {
        continue;
      }

      int64_t amount = 0;
      ASSERT_EQ(cardano_asset_id_map_get(assets, id, &amount), CARDANO_SUCCESS);
      EXPECT_GT(amount, 0);
    }

    cardano_asset_id_map_unref(&assets);

    // Change is sent to the change address
    cardano_address_t* output_address = cardano_transaction_output_get_address(output);
    EXPECT_TRUE(cardano_address_equals(output_address, change_address));
    cardano_address_unref(&output_address);

    cardano_value_unref(&value);
    cardano_transaction_output_unref(&output);
  }

  const gen_value_t change_total = sum_gen_values(change_values);

  // Coverage of payments: selected >= target for coin and every asset
  EXPECT_TRUE(gen_value_geq(selected_total, target))
    << "selected=" << gen_value_to_string(selected_total) << " target=" << gen_value_to_string(target);

  // Correctness of change: selected = target + change, exactly
  EXPECT_EQ(selected_total.coin, target.coin + change_total.coin);

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    EXPECT_EQ(selected_total.assets[i], target.assets[i] + change_total.assets[i])
      << "asset=" << ASSET_POOL[i].name;
  }

  // Conservation of UTxO: every available UTxO is in exactly one of selection/remaining
  for (cardano_utxo_t* utxo: available)
  {
    const bool in_selection = utxo_list_contains(selection, utxo);
    const bool in_remaining = utxo_list_contains(remaining_utxo, utxo);

    EXPECT_TRUE(in_selection || in_remaining);
    EXPECT_NE(in_selection, in_remaining);
  }

  // Pre-selected UTxOs are always part of the selection
  for (cardano_utxo_t* utxo: pre_selected)
  {
    EXPECT_TRUE(utxo_list_contains(selection, utxo));
    EXPECT_FALSE(utxo_list_contains(remaining_utxo, utxo));
  }
}

/**
 * \brief Port of the JS SDK `assertFailureProperties`: a failure is only legitimate when the
 * request was genuinely unsatisfiable.
 */
static void
assert_failure_properties(
  const cardano_error_t           error,
  const std::vector<gen_value_t>& pool_values,
  const gen_value_t&              target,
  cardano_address_t*              change_address)
{
  EXPECT_EQ(error, CARDANO_ERROR_BALANCE_INSUFFICIENT);

  if (pool_values.empty())
  {
    return;
  }

  const gen_value_t pool_total = sum_gen_values(pool_values);

  if (!gen_value_geq(pool_total, target))
  {
    return;
  }

  const gen_value_t leftover = gen_value_subtract(pool_total, target);

  if (leftover.is_zero())
  {
    FAIL() << "selection failed even though the pool exactly matches the target; pool="
           << gen_value_to_string(pool_total) << " target=" << gen_value_to_string(target);
  }

  const uint64_t min_ada = min_ada_for_value(leftover, change_address);

  EXPECT_LT(leftover.coin, (int64_t)min_ada)
    << "selection failed even though consuming the whole pool yields min-ADA compliant change; leftover="
    << gen_value_to_string(leftover) << " minAda=" << min_ada;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_coin_selector_properties, largeFirstSatisfiesInputSelectionProperties)
{
  cardano_address_t*             change_address  = make_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = make_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = make_protocol_parameters();
  cardano_coin_selector_t*       selector        = NULL;

  ASSERT_EQ(cardano_large_first_coin_selector_new(&selector), CARDANO_SUCCESS);

  prop_rng_t rng(PROPERTY_SEED);
  uint64_t   utxo_ordinal = 0U;

  for (size_t iteration = 0U; iteration < ITERATIONS; ++iteration)
  {
    const size_t available_count    = rng.next() % 11U;
    const size_t pre_selected_count = ((rng.next() % 5U) == 0U) ? (1U + (rng.next() % 2U)) : 0U;

    std::vector<gen_value_t>     pool_values;
    std::vector<cardano_utxo_t*> pool_utxos;
    std::vector<cardano_utxo_t*> available;
    std::vector<cardano_utxo_t*> pre_selected;

    cardano_utxo_list_t* available_utxo    = NULL;
    cardano_utxo_list_t* pre_selected_utxo = NULL;

    ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);
    ASSERT_EQ(cardano_utxo_list_new(&pre_selected_utxo), CARDANO_SUCCESS);

    for (size_t i = 0U; i < available_count; ++i)
    {
      const gen_value_t value = gen_utxo_value(rng);
      cardano_utxo_t*   utxo  = build_utxo(++utxo_ordinal, value, wallet_address);

      ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);

      pool_values.push_back(value);
      pool_utxos.push_back(utxo);
      available.push_back(utxo);
    }

    for (size_t i = 0U; i < pre_selected_count; ++i)
    {
      const gen_value_t value = gen_utxo_value(rng);
      cardano_utxo_t*   utxo  = build_utxo(++utxo_ordinal, value, wallet_address);

      ASSERT_EQ(cardano_utxo_list_add(pre_selected_utxo, utxo), CARDANO_SUCCESS);

      pool_values.push_back(value);
      pool_utxos.push_back(utxo);
      pre_selected.push_back(utxo);
    }

    const gen_value_t target_gen = gen_target_value(rng);
    cardano_value_t*  target     = build_value(target_gen);

    std::ostringstream scenario;
    scenario << "seed=" << PROPERTY_SEED << " iteration=" << iteration << " target=" << gen_value_to_string(target_gen) << " pool=[";
    for (const gen_value_t& value: pool_values)
    {
      scenario << gen_value_to_string(value);
    }
    scenario << "] pre_selected_count=" << pre_selected_count;
    SCOPED_TRACE(scenario.str());

    cardano_utxo_list_t*               selection      = NULL;
    cardano_utxo_list_t*               remaining_utxo = NULL;
    cardano_transaction_output_list_t* change_outputs = NULL;

    const cardano_error_t result = cardano_coin_selector_select(
      selector,
      (pre_selected_count > 0U) ? pre_selected_utxo : NULL,
      available_utxo,
      target,
      change_address,
      protocol_params,
      &selection,
      &remaining_utxo,
      &change_outputs);

    if (result == CARDANO_SUCCESS)
    {
      assert_input_selection_properties(
        available,
        pre_selected,
        pool_values,
        pool_utxos,
        target_gen,
        selection,
        remaining_utxo,
        change_outputs,
        change_address);
    }
    else
    {
      assert_failure_properties(result, pool_values, target_gen, change_address);
    }

    cardano_utxo_list_unref(&selection);
    cardano_utxo_list_unref(&remaining_utxo);
    cardano_transaction_output_list_unref(&change_outputs);
    cardano_value_unref(&target);
    cardano_utxo_list_unref(&available_utxo);
    cardano_utxo_list_unref(&pre_selected_utxo);

    for (cardano_utxo_t* utxo: pool_utxos)
    {
      cardano_utxo_unref(&utxo);
    }

    if (::testing::Test::HasFailure())
    {
      break;
    }
  }

  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_coin_selector_properties, prunesZeroTokenChange)
{
  // Ported regression from the JS SDK: '0 token change'. When an output consumes the full
  // asset amount, the change output must not carry a 0-quantity asset entry.
  cardano_address_t*             change_address  = make_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = make_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = make_protocol_parameters();
  cardano_coin_selector_t*       selector        = NULL;

  ASSERT_EQ(cardano_large_first_coin_selector_new(&selector), CARDANO_SUCCESS);

  gen_value_t utxo_value;
  utxo_value.coin      = 11999994;
  utxo_value.assets[0] = 7001;

  gen_value_t target_value;
  target_value.coin      = 1000;
  target_value.assets[0] = 7001;

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = build_utxo(1, utxo_value, wallet_address);
  ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);

  cardano_value_t* target = build_value(target_value);

  cardano_utxo_list_t*               selection      = NULL;
  cardano_utxo_list_t*               remaining_utxo = NULL;
  cardano_transaction_output_list_t* change_outputs = NULL;

  ASSERT_EQ(
    cardano_coin_selector_select(selector, NULL, available_utxo, target, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
    CARDANO_SUCCESS);

  ASSERT_EQ(cardano_transaction_output_list_get_length(change_outputs), 1U);

  cardano_transaction_output_t* change_output = NULL;
  ASSERT_EQ(cardano_transaction_output_list_get(change_outputs, 0, &change_output), CARDANO_SUCCESS);

  cardano_value_t* change_value = cardano_transaction_output_get_value(change_output);

  const gen_value_t change = read_value(change_value);

  EXPECT_EQ(change.coin, 11998994);
  EXPECT_EQ(change.assets[0], 0);

  cardano_multi_asset_t* change_assets = cardano_value_get_multi_asset(change_value);
  cardano_multi_asset_unref(&change_assets);

  if (change_assets != NULL)
  {
    EXPECT_EQ(cardano_multi_asset_get_policy_count(change_assets), 0U);
  }

  cardano_value_unref(&change_value);
  cardano_transaction_output_unref(&change_output);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_transaction_output_list_unref(&change_outputs);
  cardano_value_unref(&target);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_coin_selector_properties, treatsNegativeTargetAssetAsImplicitInput)
{
  // Ported from the JS SDK: 'Considers positive quantity mint as implicit input'. In the
  // cardano-c architecture minting surfaces to the selector as a negative asset amount in the
  // target, and the minted tokens must flow into the change.
  cardano_address_t*             change_address  = make_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = make_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = make_protocol_parameters();
  cardano_coin_selector_t*       selector        = NULL;

  ASSERT_EQ(cardano_large_first_coin_selector_new(&selector), CARDANO_SUCCESS);

  gen_value_t utxo_value;
  utxo_value.coin = 10000000;

  gen_value_t target_value;
  target_value.coin      = 1000000;
  target_value.assets[0] = -100;

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = build_utxo(1, utxo_value, wallet_address);
  ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);

  cardano_value_t* target = build_value(target_value);

  cardano_utxo_list_t*               selection      = NULL;
  cardano_utxo_list_t*               remaining_utxo = NULL;
  cardano_transaction_output_list_t* change_outputs = NULL;

  ASSERT_EQ(
    cardano_coin_selector_select(selector, NULL, available_utxo, target, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
    CARDANO_SUCCESS);

  EXPECT_EQ(cardano_utxo_list_get_length(selection), 1U);
  ASSERT_EQ(cardano_transaction_output_list_get_length(change_outputs), 1U);

  cardano_transaction_output_t* change_output = NULL;
  ASSERT_EQ(cardano_transaction_output_list_get(change_outputs, 0, &change_output), CARDANO_SUCCESS);

  cardano_value_t* change_value = cardano_transaction_output_get_value(change_output);

  const gen_value_t change = read_value(change_value);

  EXPECT_EQ(change.coin, 9000000);
  EXPECT_EQ(change.assets[0], 100);

  cardano_value_unref(&change_value);
  cardano_transaction_output_unref(&change_output);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_transaction_output_list_unref(&change_outputs);
  cardano_value_unref(&target);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_coin_selector_properties, failsWhenCoinInsufficient)
{
  cardano_address_t*             change_address  = make_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = make_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = make_protocol_parameters();
  cardano_coin_selector_t*       selector        = NULL;

  ASSERT_EQ(cardano_large_first_coin_selector_new(&selector), CARDANO_SUCCESS);

  gen_value_t utxo_value_1;
  utxo_value_1.coin = 3000000;

  gen_value_t utxo_value_2;
  utxo_value_2.coin = 10000000;

  gen_value_t target_value;
  target_value.coin = 14000000;

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  cardano_utxo_t* utxo_1 = build_utxo(1, utxo_value_1, wallet_address);
  cardano_utxo_t* utxo_2 = build_utxo(2, utxo_value_2, wallet_address);

  ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo_1), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo_2), CARDANO_SUCCESS);

  cardano_value_t* target = build_value(target_value);

  cardano_utxo_list_t*               selection      = NULL;
  cardano_utxo_list_t*               remaining_utxo = NULL;
  cardano_transaction_output_list_t* change_outputs = NULL;

  EXPECT_EQ(
    cardano_coin_selector_select(selector, NULL, available_utxo, target, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
    CARDANO_ERROR_BALANCE_INSUFFICIENT);

  cardano_value_unref(&target);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_unref(&utxo_1);
  cardano_utxo_unref(&utxo_2);
  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_coin_selector_properties, failsWhenAssetInsufficient)
{
  cardano_address_t*             change_address  = make_address(CHANGE_ADDRESS);
  cardano_address_t*             wallet_address  = make_address(WALLET_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = make_protocol_parameters();
  cardano_coin_selector_t*       selector        = NULL;

  ASSERT_EQ(cardano_large_first_coin_selector_new(&selector), CARDANO_SUCCESS);

  gen_value_t utxo_value;
  utxo_value.coin      = 10000000;
  utxo_value.assets[0] = 7000;

  gen_value_t target_value;
  target_value.coin      = 5000000;
  target_value.assets[0] = 7001;

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = build_utxo(1, utxo_value, wallet_address);
  ASSERT_EQ(cardano_utxo_list_add(available_utxo, utxo), CARDANO_SUCCESS);

  cardano_value_t* target = build_value(target_value);

  cardano_utxo_list_t*               selection      = NULL;
  cardano_utxo_list_t*               remaining_utxo = NULL;
  cardano_transaction_output_list_t* change_outputs = NULL;

  EXPECT_EQ(
    cardano_coin_selector_select(selector, NULL, available_utxo, target, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
    CARDANO_ERROR_BALANCE_INSUFFICIENT);

  cardano_value_unref(&target);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_address_unref(&wallet_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_coin_selector_properties, failsWhenNoUtxoAvailable)
{
  cardano_address_t*             change_address  = make_address(CHANGE_ADDRESS);
  cardano_protocol_parameters_t* protocol_params = make_protocol_parameters();
  cardano_coin_selector_t*       selector        = NULL;

  ASSERT_EQ(cardano_large_first_coin_selector_new(&selector), CARDANO_SUCCESS);

  gen_value_t target_value;
  target_value.coin = 5000000;

  cardano_utxo_list_t* available_utxo = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&available_utxo), CARDANO_SUCCESS);

  cardano_value_t* target = build_value(target_value);

  cardano_utxo_list_t*               selection      = NULL;
  cardano_utxo_list_t*               remaining_utxo = NULL;
  cardano_transaction_output_list_t* change_outputs = NULL;

  EXPECT_EQ(
    cardano_coin_selector_select(selector, NULL, available_utxo, target, change_address, protocol_params, &selection, &remaining_utxo, &change_outputs),
    CARDANO_ERROR_BALANCE_INSUFFICIENT);

  cardano_value_unref(&target);
  cardano_utxo_list_unref(&available_utxo);
  cardano_address_unref(&change_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_coin_selector_unref(&selector);
}
