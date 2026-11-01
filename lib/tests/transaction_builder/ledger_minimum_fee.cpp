/**
 * \file ledger_minimum_fee.cpp
 *
 * \author angel.castillo
 * \date   Nov 01, 2026
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
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/datum.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/protocol_version.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/scripts/script.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/value.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>
#include <cardano/transaction_builder/evaluation/tx_evaluator_impl.h>
#include <cardano/transaction_builder/transaction_builder.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/vkey_witness.h>
#include <cardano/witness_set/vkey_witness_set.h>
#include <cardano/witness_set/witness_set.h>

#include <allocators.h>

#include <gmock/gmock.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

/* CONSTANTS *****************************************************************/

/**
 * \brief Address of the wallet that funds the transactions and receives their change and their collateral return.
 */
static const char* WALLET_ADDRESS = "addr_test1qqg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyfzyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3qwzdgzn";

/**
 * \brief Address the transactions pay to.
 */
static const char* PAYEE_ADDRESS = "addr_test1vqenxvenxvenxvenxvenxvenxvenxvenxvenxvenxvenxvc9myyf9";

/**
 * \brief Reward address the transactions withdraw rewards from.
 */
static const char* REWARD_ADDRESS = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";

/**
 * \brief Address that owns the collateral UTXOs.
 */
static const char* COLLATERAL_ADDRESS = "addr_test1vpzyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3zyg3q5drxww";

/**
 * \brief Address locked by \ref PLUTUS_V3_SPEND_SCRIPT_CBOR.
 */
static const char* PLUTUS_V3_SCRIPT_ADDRESS = "addr_test1wrc69cheffqv0d9p3n9hcqk9fkptu0jyrz4mf3sl0z6m8rs4z6gqf";

/**
 * \brief Address locked by \ref PLUTUS_V2_SPEND_SCRIPT_CBOR.
 */
static const char* PLUTUS_V2_SCRIPT_ADDRESS = "addr_test1wz9efjg0gugyz7h6kkn3vdx6xs0mjvpkv39p4l7twv6r2gcpx0wgg";

/**
 * \brief Address of the UTXO that carries the reference script.
 */
static const char* REFERENCE_SCRIPT_HOLDER_ADDRESS = "addr_test1vp24242424242424242424242424242424242424242424glts28l";

/**
 * \brief A Plutus V3 script that validates the spending of the outputs locked by its address.
 */
static const char* PLUTUS_V3_SPEND_SCRIPT_CBOR = "82034746450101002499";

/**
 * \brief A Plutus V3 script that validates the minting under its policy.
 */
static const char* PLUTUS_V3_MINT_SCRIPT_CBOR = "82034746450101002498";

/**
 * \brief A Plutus V2 script that validates the spending of the outputs locked by its address. It is 8 bytes long.
 */
static const char* PLUTUS_V2_SPEND_SCRIPT_CBOR = "8202484746010000222499";

/**
 * \brief The size in bytes of the flat encoding of \ref PLUTUS_V2_SPEND_SCRIPT_CBOR.
 */
static const uint64_t PLUTUS_V2_SPEND_SCRIPT_SIZE = 8U;

/**
 * \brief Datum held inline by the outputs locked by a script.
 */
static const char* INLINE_DATUM_CBOR = "d8799f0102ff";

/**
 * \brief Datum carried by the witness set of the transaction that spends an output locked by its hash.
 */
static const char* WITNESS_DATUM_CBOR = "182a";

/**
 * \brief The hash of \ref WITNESS_DATUM_CBOR.
 */
static const char* WITNESS_DATUM_HASH_HEX = "9e1199a988ba72ffd6e9c269cadb3b53b5f360ff99f112d9b2ee30c4d74ad88b";

/**
 * \brief Redeemer of every script of these tests.
 */
static const char* REDEEMER_CBOR = "d87980";

/**
 * \brief Native assets held by the wallet: a policy id followed by an asset name, in hex.
 */
static const char* FIRST_TOKEN_ID  = "abababababababababababababababababababababababababababab544f4b454e41";
static const char* SECOND_TOKEN_ID = "abababababababababababababababababababababababababababab544f4b454e42";

/**
 * \brief The policy id of \ref PLUTUS_V3_MINT_SCRIPT_CBOR and the name of the token minted under it, in hex.
 */
static const char* MINTED_POLICY_ID  = "4c898c566191073bd19a2dcef35b3b8ef8888e5948af6e040befa00e";
static const char* MINTED_ASSET_NAME = "474f4c44";

/**
 * \brief The minted token as an asset id.
 */
static const char* MINTED_ASSET_ID = "4c898c566191073bd19a2dcef35b3b8ef8888e5948af6e040befa00e474f4c44";

/**
 * \brief Slot after which every transaction of this file is invalid.
 */
static const uint64_t INVALID_AFTER_SLOT = 90000000U;

/**
 * \brief The fee coefficient per byte and the constant fee of the protocol parameters.
 */
static const uint64_t MIN_FEE_A = 44U;
static const uint64_t MIN_FEE_B = 155381U;

/**
 * \brief The price of a unit of memory, 0.0577, and of a CPU step, 0.0000721, as exact fractions.
 */
static const uint64_t MEMORY_PRICE_NUMERATOR   = 577U;
static const uint64_t MEMORY_PRICE_DENOMINATOR = 10000U;
static const uint64_t STEPS_PRICE_NUMERATOR    = 721U;
static const uint64_t STEPS_PRICE_DENOMINATOR  = 10000000U;

/**
 * \brief The fee per byte of reference script, which is the price of every byte of the first pricing tier.
 */
static const uint64_t REFERENCE_SCRIPT_COST_PER_BYTE = 15U;

/**
 * \brief The percentage of the fee the collateral of a transaction must cover.
 */
static const uint64_t COLLATERAL_PERCENTAGE = 150U;

/**
 * \brief The number of costs of the cost model of each Plutus language.
 */
static const size_t PLUTUS_V1_COSTS_SIZE = 166U;
static const size_t PLUTUS_V2_COSTS_SIZE = 175U;
static const size_t PLUTUS_V3_COSTS_SIZE = 297U;

/**
 * \brief The ALONZO transaction frame, a 4 element array whose third element is the valid flag.
 */
static const char* ALONZO_TRANSACTION_FRAME_HEX = "84";

/**
 * \brief The smallest amount of lovelace whose CBOR encoding takes 9 bytes. Every smaller amount above 65535 takes 5.
 */
static const uint64_t SMALLEST_NINE_BYTE_COIN = 4294967296U;

/**
 * \brief A minimum fee below the fee of a payment, which the balancer starts from. Its encoding takes as many bytes as
 * the encoding of the real fee, so only the change can make the transaction shrink once the real fee is known.
 */
static const uint64_t LOWERED_FEE_MINIMUM_FEE = 100000U;

/**
 * \brief The error a builder reports when a wallet that holds 1 ADA pays 5 ADA.
 */
static const char* WALLET_SHORTFALL_ERROR = "The transaction requires 5000000 lovelace from its inputs, plus the assets it spends and the min-ADA of its change, but the UTXOs available to balance it hold 1000000 lovelace.";

/**
 * \brief The error a builder reports when a wallet without native assets pays 30 units of \ref FIRST_TOKEN_ID while a
 * withdrawal covers more lovelace than the transaction spends.
 */
static const char* ASSET_SHORTFALL_ERROR = "The transaction requires 30 of the asset abababababababababababababababababababababababababababab544f4b454e41 from its inputs, but the UTXOs available to balance it hold 0.";

/**
 * \brief The error a builder reports when a withdrawal covers the outputs of a transaction but no UTXO is available to
 * pay its fee.
 */
static const char* COVERED_OUTPUTS_SHORTFALL_ERROR = "The transaction requires 0 lovelace from its inputs, plus the assets it spends and the min-ADA of its change, but the UTXOs available to balance it hold 0 lovelace.";

/**
 * \brief The error a builder reports when a single collateral UTXO of 1 ADA backs a Plutus V3 spend whose fee
 * requires more collateral than it can cover with a min-ADA return.
 */
static const char* COLLATERAL_SHORTFALL_ERROR = "The transaction requires 338901 lovelace of collateral, plus the min-ADA of the collateral return, but the collateral UTXOs hold 1000000 lovelace.";

/* STRUCTURES ****************************************************************/

/**
 * \brief Context of an evaluator that assigns the same execution units to every redeemer.
 */
typedef struct fixed_ex_units_context_t
{
    cardano_object_t base;
    uint64_t         memory;
    uint64_t         steps;
} fixed_ex_units_context_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates the cost model of a Plutus language with every cost set to zero. The costs do not change the size of the
 * transaction, only the script data hash it commits to.
 * \param language the Plutus language of the cost model.
 * \param costs_size the number of costs of the cost model.
 * \return A new instance of the cost model.
 */
static cardano_cost_model_t*
new_cost_model(const cardano_plutus_language_version_t language, const size_t costs_size)
{
  const std::vector<int64_t> costs(costs_size, 0);
  cardano_cost_model_t*      cost_model = NULL;

  EXPECT_EQ(cardano_cost_model_new(language, costs.data(), costs.size(), &cost_model), CARDANO_SUCCESS);

  return cost_model;
}

/**
 * Creates the protocol parameters that price the transactions of these tests, with the execution unit prices and
 * the reference script cost as exact fractions.
 * \return A new instance of the protocol parameters.
 */
static cardano_protocol_parameters_t*
new_protocol_parameters()
{
  cardano_protocol_parameters_t* params           = NULL;
  cardano_ex_unit_prices_t*      ex_unit_prices   = NULL;
  cardano_unit_interval_t*       memory_prices    = NULL;
  cardano_unit_interval_t*       steps_prices     = NULL;
  cardano_unit_interval_t*       script_ref_cost  = NULL;
  cardano_protocol_version_t*    protocol_version = NULL;
  cardano_costmdls_t*            costmdls         = NULL;
  cardano_cost_model_t*          plutus_v1        = new_cost_model(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, PLUTUS_V1_COSTS_SIZE);
  cardano_cost_model_t*          plutus_v2        = new_cost_model(CARDANO_PLUTUS_LANGUAGE_VERSION_V2, PLUTUS_V2_COSTS_SIZE);
  cardano_cost_model_t*          plutus_v3        = new_cost_model(CARDANO_PLUTUS_LANGUAGE_VERSION_V3, PLUTUS_V3_COSTS_SIZE);

  EXPECT_EQ(cardano_protocol_parameters_new(&params), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(MEMORY_PRICE_NUMERATOR, MEMORY_PRICE_DENOMINATOR, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(STEPS_PRICE_NUMERATOR, STEPS_PRICE_DENOMINATOR, &steps_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(REFERENCE_SCRIPT_COST_PER_BYTE, 1U, &script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_version_new(10U, 0U, &protocol_version), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_new(&costmdls), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_insert(costmdls, plutus_v1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_insert(costmdls, plutus_v2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_insert(costmdls, plutus_v3), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_protocol_parameters_set_min_fee_a(params, MIN_FEE_A), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_min_fee_b(params, MIN_FEE_B), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_execution_costs(params, ex_unit_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_per_byte(params, script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ada_per_utxo_byte(params, 4310U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_max_tx_size(params, 16384U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_max_value_size(params, 5000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_collateral_percentage(params, COLLATERAL_PERCENTAGE), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_max_collateral_inputs(params, 3U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_key_deposit(params, 2000000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_pool_deposit(params, 500000000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_drep_deposit(params, 500000000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_protocol_version(params, protocol_version), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_cost_models(params, costmdls), CARDANO_SUCCESS);

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);
  cardano_protocol_version_unref(&protocol_version);
  cardano_cost_model_unref(&plutus_v1);
  cardano_cost_model_unref(&plutus_v2);
  cardano_cost_model_unref(&plutus_v3);
  cardano_costmdls_unref(&costmdls);

  return params;
}

/**
 * Creates an address from its bech32 encoding.
 * \param bech32 the bech32 encoding of the address.
 * \return A new instance of the address.
 */
static cardano_address_t*
new_address(const char* bech32)
{
  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string(bech32, strlen(bech32), &address), CARDANO_SUCCESS);

  return address;
}

/**
 * Creates a Plutus data from its CBOR encoding.
 * \param cbor_hex the CBOR encoding of the Plutus data, in hex.
 * \return A new instance of the Plutus data.
 */
static cardano_plutus_data_t*
new_plutus_data(const char* cbor_hex)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_plutus_data_t* data   = NULL;

  EXPECT_EQ(cardano_plutus_data_from_cbor(reader, &data), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return data;
}

/**
 * Creates a script from its CBOR encoding.
 * \param cbor_hex the CBOR encoding of the script, in hex.
 * \return A new instance of the script.
 */
static cardano_script_t*
new_script(const char* cbor_hex)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_script_t*      script = NULL;

  EXPECT_EQ(cardano_script_from_cbor(reader, &script), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return script;
}

/**
 * Creates an output that pays lovelace to an address.
 * \param bech32 the bech32 encoding of the address.
 * \param coin the lovelace paid by the output.
 * \return A new instance of the output.
 */
static cardano_transaction_output_t*
new_output(const char* bech32, const uint64_t coin)
{
  cardano_address_t*            address = new_address(bech32);
  cardano_transaction_output_t* output  = NULL;

  EXPECT_EQ(cardano_transaction_output_new(address, coin, &output), CARDANO_SUCCESS);

  cardano_address_unref(&address);

  return output;
}

/**
 * Adds a quantity of a native asset to an output.
 * \param output the output that receives the asset.
 * \param asset_id the policy id followed by the asset name, in hex.
 * \param quantity the quantity of the asset.
 */
static void
add_asset(cardano_transaction_output_t* output, const char* asset_id, const int64_t quantity)
{
  cardano_value_t* value = cardano_transaction_output_get_value(output);

  EXPECT_EQ(cardano_value_add_asset_with_id_ex(value, asset_id, strlen(asset_id), quantity), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_value(output, value), CARDANO_SUCCESS);

  cardano_value_unref(&value);
}

/**
 * Makes an output hold an inline datum.
 * \param output the output that holds the datum.
 * \param cbor_hex the CBOR encoding of the datum, in hex.
 */
static void
set_inline_datum(cardano_transaction_output_t* output, const char* cbor_hex)
{
  cardano_plutus_data_t* data  = new_plutus_data(cbor_hex);
  cardano_datum_t*       datum = NULL;

  EXPECT_EQ(cardano_datum_new_inline_data(data, &datum), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_datum(output, datum), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&data);
  cardano_datum_unref(&datum);
}

/**
 * Makes an output hold the hash of a datum.
 * \param output the output that holds the datum hash.
 * \param hash_hex the hash of the datum, in hex.
 */
static void
set_datum_hash(cardano_transaction_output_t* output, const char* hash_hex)
{
  cardano_datum_t* datum = NULL;

  EXPECT_EQ(cardano_datum_new_data_hash_hex(hash_hex, strlen(hash_hex), &datum), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_datum(output, datum), CARDANO_SUCCESS);

  cardano_datum_unref(&datum);
}

/**
 * Creates a UTXO that holds an output.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param output the output held by the UTXO.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_utxo(const uint64_t ordinal, cardano_transaction_output_t* output)
{
  char                         hex[65] = { 0 };
  cardano_blake2b_hash_t*      id      = NULL;
  cardano_transaction_input_t* input   = NULL;
  cardano_utxo_t*              utxo    = NULL;

  EXPECT_EQ(snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal), 64);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, 64, &id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);

  return utxo;
}

/**
 * Adds to a list a UTXO that holds an output and releases the output.
 * \param utxos the list that receives the UTXO.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param output the output held by the UTXO, released by this function.
 */
static void
add_utxo(cardano_utxo_list_t* utxos, const uint64_t ordinal, cardano_transaction_output_t* output)
{
  cardano_utxo_t* utxo = new_utxo(ordinal, output);

  EXPECT_EQ(cardano_utxo_list_add(utxos, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
}

/**
 * Creates an empty list of UTXOs.
 * \return A new instance of the list.
 */
static cardano_utxo_list_t*
new_utxo_list()
{
  cardano_utxo_list_t* utxos = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);

  return utxos;
}

/**
 * Assigns the execution units of the evaluator context to every redeemer of a transaction.
 * \param impl the evaluator implementation, whose context holds the execution units.
 * \param tx the transaction whose redeemers are evaluated.
 * \param utxos the UTXOs that resolve the inputs of the transaction.
 * \param redeemers on output, the redeemers of the transaction with their execution units.
 * \return \ref CARDANO_SUCCESS if the redeemers were evaluated, or an error code otherwise.
 */
static cardano_error_t
evaluate_with_fixed_ex_units(
  cardano_tx_evaluator_impl_t* impl,
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         utxos,
  cardano_redeemer_list_t**    redeemers)
{
  CARDANO_UNUSED(utxos);

  const fixed_ex_units_context_t* context     = reinterpret_cast<const fixed_ex_units_context_t*>(impl->context);
  cardano_witness_set_t*          witness_set = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* current = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&current);

  cardano_redeemer_list_t* evaluated = NULL;
  cardano_ex_units_t*      ex_units  = NULL;

  EXPECT_EQ(cardano_redeemer_list_clone(current, &evaluated), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_units_new(context->memory, context->steps, &ex_units), CARDANO_SUCCESS);

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(evaluated); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    EXPECT_EQ(cardano_redeemer_list_get(evaluated, i, &redeemer), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_redeemer_set_ex_units(redeemer, ex_units), CARDANO_SUCCESS);

    cardano_redeemer_unref(&redeemer);
  }

  cardano_ex_units_unref(&ex_units);

  *redeemers = evaluated;

  return CARDANO_SUCCESS;
}

/**
 * Creates an evaluator that assigns the same execution units to every redeemer.
 * \param memory the memory units of every redeemer.
 * \param steps the CPU steps of every redeemer.
 * \return A new instance of the evaluator.
 */
static cardano_tx_evaluator_t*
new_fixed_ex_units_evaluator(const uint64_t memory, const uint64_t steps)
{
  cardano_tx_evaluator_impl_t impl      = { 0 };
  cardano_tx_evaluator_t*     evaluator = NULL;
  fixed_ex_units_context_t*   context   = reinterpret_cast<fixed_ex_units_context_t*>(malloc(sizeof(fixed_ex_units_context_t)));

  if (context == NULL)
  {
    ADD_FAILURE() << "The evaluator context could not be allocated.";

    return NULL;
  }

  context->base.ref_count     = 1U;
  context->base.deallocator   = _cardano_free;
  context->base.last_error[0] = '\0';
  context->memory             = memory;
  context->steps              = steps;

  impl.context  = &context->base;
  impl.evaluate = evaluate_with_fixed_ex_units;

  EXPECT_EQ(cardano_tx_evaluator_new(impl, &evaluator), CARDANO_SUCCESS);

  return evaluator;
}

/**
 * Creates a transaction builder that balances with the large first coin selector, pays the change to the wallet,
 * expires at a fixed slot and evaluates every redeemer to the same execution units.
 * \param params the protocol parameters.
 * \param wallet_utxos the UTXOs of the wallet that fund the transaction.
 * \param memory the memory units of every redeemer.
 * \param steps the CPU steps of every redeemer.
 * \return A new instance of the transaction builder.
 */
static cardano_tx_builder_t*
new_builder(cardano_protocol_parameters_t* params, cardano_utxo_list_t* wallet_utxos, const uint64_t memory, const uint64_t steps)
{
  cardano_tx_builder_t*    builder        = cardano_tx_builder_new(params, &CARDANO_PREPROD_SLOT_CONFIG);
  cardano_coin_selector_t* selector       = NULL;
  cardano_tx_evaluator_t*  evaluator      = new_fixed_ex_units_evaluator(memory, steps);
  cardano_address_t*       change_address = new_address(WALLET_ADDRESS);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&selector), CARDANO_SUCCESS);

  cardano_tx_builder_set_coin_selector(builder, selector);
  cardano_tx_builder_set_tx_evaluator(builder, evaluator);
  cardano_tx_builder_set_change_address(builder, change_address);
  cardano_tx_builder_set_utxos(builder, wallet_utxos);
  cardano_tx_builder_set_invalid_after(builder, INVALID_AFTER_SLOT);

  cardano_coin_selector_unref(&selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);

  return builder;
}

/**
 * Gives a builder the collateral UTXOs of the transaction and returns the collateral change to the wallet.
 * \param builder the transaction builder.
 * \param collateral_utxos the UTXOs the collateral is selected from.
 */
static void
set_collateral(cardano_tx_builder_t* builder, cardano_utxo_list_t* collateral_utxos)
{
  cardano_address_t* return_address = new_address(WALLET_ADDRESS);

  cardano_tx_builder_set_collateral_utxos(builder, collateral_utxos);
  cardano_tx_builder_set_collateral_change_address(builder, return_address);

  cardano_address_unref(&return_address);
}

/**
 * Builds the transaction of a builder and releases the builder.
 * \param builder the transaction builder, released by this function.
 * \return The balanced transaction, or NULL if it could not be built.
 */
static cardano_transaction_t*
build(cardano_tx_builder_t* builder)
{
  cardano_transaction_t* tx = NULL;

  EXPECT_EQ(cardano_tx_builder_build(builder, &tx), CARDANO_SUCCESS) << cardano_tx_builder_get_last_error(builder);

  cardano_tx_builder_unref(&builder);

  return tx;
}

/**
 * Adds to a transaction as many VK witnesses as it has signers. Each witness has its own key and a signature of the
 * size of an Ed25519 signature, which is all the fee depends on.
 * \param tx the transaction to sign.
 * \param signers the number of signers of the transaction.
 */
static void
add_vkey_witnesses(cardano_transaction_t* tx, const size_t signers)
{
  cardano_vkey_witness_set_t* witnesses = NULL;

  EXPECT_EQ(cardano_vkey_witness_set_new(&witnesses), CARDANO_SUCCESS);

  for (size_t i = 0U; i < signers; ++i)
  {
    char                          key_hex[65]        = { 0 };
    char                          signature_hex[129] = { 0 };
    cardano_ed25519_public_key_t* public_key         = NULL;
    cardano_ed25519_signature_t*  signature          = NULL;
    cardano_vkey_witness_t*       witness            = NULL;

    (void)memset(signature_hex, '0', 128U);
    EXPECT_EQ(snprintf(key_hex, sizeof(key_hex), "%064llx", (unsigned long long)(i + 1U)), 64);

    EXPECT_EQ(cardano_ed25519_public_key_from_hex(key_hex, 64U, &public_key), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_ed25519_signature_from_hex(signature_hex, 128U, &signature), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_vkey_witness_new(public_key, signature, &witness), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_vkey_witness_set_add(witnesses, witness), CARDANO_SUCCESS);

    cardano_ed25519_public_key_unref(&public_key);
    cardano_ed25519_signature_unref(&signature);
    cardano_vkey_witness_unref(&witness);
  }

  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses(tx, witnesses), CARDANO_SUCCESS);

  cardano_vkey_witness_set_unref(&witnesses);
}

/**
 * Gets the size of a valid transaction as the ledger measures it for the fee, which is the size of its serialization
 * without the valid flag of the ALONZO frame.
 * \param tx the transaction to measure.
 * \return The size of the transaction for the fee, in bytes.
 */
static uint64_t
get_ledger_size(const cardano_transaction_t* tx)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_transaction_to_cbor(tx, writer), CARDANO_SUCCESS);

  std::vector<char> hex(cardano_cbor_writer_get_hex_size(writer), '\0');

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, hex.data(), hex.size()), CARDANO_SUCCESS);
  EXPECT_EQ(strncmp(hex.data(), ALONZO_TRANSACTION_FRAME_HEX, 2U), 0);

  cardano_cbor_writer_unref(&writer);

  return (uint64_t)(strlen(hex.data()) / 2U) - 1U;
}

/**
 * Computes the fee of a number of execution units, which is the ceiling of their exact price.
 * \param memory the memory units.
 * \param steps the CPU steps.
 * \return The fee of the execution units.
 */
static uint64_t
get_ex_units_fee(const uint64_t memory, const uint64_t steps)
{
  const uint64_t scale   = STEPS_PRICE_DENOMINATOR / MEMORY_PRICE_DENOMINATOR;
  const uint64_t numer   = (memory * MEMORY_PRICE_NUMERATOR * scale) + (steps * STEPS_PRICE_NUMERATOR);
  const uint64_t ceiling = (numer + STEPS_PRICE_DENOMINATOR - 1U) / STEPS_PRICE_DENOMINATOR;

  return ceiling;
}

/**
 * Signs a balanced transaction and expects its fee to be the one pinned for it and to be exactly the ledger minimum
 * of the signed transaction: the fee coefficient times its size without the valid flag, plus the constant fee, plus the
 * ceiling of the exact price of its execution units, plus the price of the reference scripts it uses.
 * \param tx the balanced transaction.
 * \param signers the number of signers of the transaction.
 * \param memory the memory units of every redeemer of the transaction.
 * \param steps the CPU steps of every redeemer of the transaction.
 * \param redeemers the number of redeemers of the transaction.
 * \param reference_script_size the size of the reference scripts the transaction uses, in bytes.
 * \param expected_fee the fee pinned for the transaction.
 */
static void
expect_ledger_minimum_fee(
  cardano_transaction_t* tx,
  const size_t           signers,
  const uint64_t         memory,
  const uint64_t         steps,
  const uint64_t         redeemers,
  const uint64_t         reference_script_size,
  const uint64_t         expected_fee)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  add_vkey_witnesses(tx, signers);

  const uint64_t fee            = cardano_transaction_body_get_fee(body);
  const uint64_t size_fee       = (MIN_FEE_A * get_ledger_size(tx)) + MIN_FEE_B;
  const uint64_t script_fee     = get_ex_units_fee(memory * redeemers, steps * redeemers);
  const uint64_t script_ref_fee = REFERENCE_SCRIPT_COST_PER_BYTE * reference_script_size;

  EXPECT_EQ(fee, size_fee + script_fee + script_ref_fee);
  EXPECT_EQ(fee, expected_fee);
}

/**
 * Sums the lovelace of the UTXOs of a list that a set of inputs spends.
 * \param inputs the inputs.
 * \param utxos the UTXOs that resolve the inputs.
 * \return The lovelace spent by the inputs.
 */
static uint64_t
get_spent_coin(const cardano_transaction_input_set_t* inputs, const cardano_utxo_list_t* utxos)
{
  uint64_t spent = 0U;

  for (size_t i = 0U; i < cardano_transaction_input_set_get_length(inputs); ++i)
  {
    cardano_transaction_input_t* input = NULL;

    EXPECT_EQ(cardano_transaction_input_set_get(inputs, i, &input), CARDANO_SUCCESS);
    cardano_transaction_input_unref(&input);

    for (size_t j = 0U; j < cardano_utxo_list_get_length(utxos); ++j)
    {
      cardano_utxo_t* utxo = NULL;

      EXPECT_EQ(cardano_utxo_list_get(utxos, j, &utxo), CARDANO_SUCCESS);
      cardano_utxo_unref(&utxo);

      cardano_transaction_input_t* utxo_input = cardano_utxo_get_input(utxo);
      cardano_transaction_input_unref(&utxo_input);

      if (cardano_transaction_input_equals(input, utxo_input))
      {
        cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
        cardano_transaction_output_unref(&output);

        cardano_value_t* value = cardano_transaction_output_get_value(output);
        cardano_value_unref(&value);

        spent += (uint64_t)cardano_value_get_coin(value);
      }
    }
  }

  return spent;
}

/**
 * Expects a balanced transaction to spend the expected number of collateral inputs, to declare as total collateral
 * exactly what its collateral inputs hold beyond its collateral return, and to put up at least the percentage of its fee
 * the protocol requires.
 * \param tx the balanced transaction.
 * \param collateral_utxos the UTXOs the collateral was selected from.
 * \param expected_inputs the number of collateral inputs the transaction is expected to spend.
 */
static void
expect_collateral(cardano_transaction_t* tx, const cardano_utxo_list_t* collateral_utxos, const size_t expected_inputs)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* collateral = cardano_transaction_body_get_collateral(body);
  cardano_transaction_input_set_unref(&collateral);

  cardano_transaction_output_t* collateral_return = cardano_transaction_body_get_collateral_return(body);
  cardano_transaction_output_unref(&collateral_return);

  ASSERT_NE(collateral, nullptr);
  ASSERT_NE(collateral_return, nullptr);
  ASSERT_NE(cardano_transaction_body_get_total_collateral(body), nullptr);

  cardano_value_t* return_value = cardano_transaction_output_get_value(collateral_return);
  cardano_value_unref(&return_value);

  const uint64_t fee              = cardano_transaction_body_get_fee(body);
  const uint64_t total_collateral = *cardano_transaction_body_get_total_collateral(body);
  const uint64_t spent            = get_spent_coin(collateral, collateral_utxos);
  const uint64_t returned         = (uint64_t)cardano_value_get_coin(return_value);

  EXPECT_EQ(cardano_transaction_input_set_get_length(collateral), expected_inputs);
  EXPECT_EQ(total_collateral, spent - returned);
  EXPECT_GE(total_collateral * 100U, fee * COLLATERAL_PERCENTAGE);
}

/**
 * Creates the builder of a transaction that spends a 3 ADA output locked by \ref PLUTUS_V3_SPEND_SCRIPT_CBOR with an
 * inline datum, funded by a 10 ADA wallet UTXO and backed by the given collateral UTXOs.
 * \param params the protocol parameters.
 * \param collateral_utxos the UTXOs the collateral is selected from.
 * \param ordinal the ordinal of the first UTXO of the transaction.
 * \param memory the memory units of the redeemer.
 * \param steps the CPU steps of the redeemer.
 * \return A new instance of the transaction builder.
 */
static cardano_tx_builder_t*
new_plutus_v3_spend_builder(
  cardano_protocol_parameters_t* params,
  cardano_utxo_list_t*           collateral_utxos,
  const uint64_t                 ordinal,
  const uint64_t                 memory,
  const uint64_t                 steps)
{
  cardano_utxo_list_t*          wallet_utxos  = new_utxo_list();
  cardano_transaction_output_t* script_output = new_output(PLUTUS_V3_SCRIPT_ADDRESS, 3000000U);
  cardano_plutus_data_t*        redeemer      = new_plutus_data(REDEEMER_CBOR);
  cardano_script_t*             script        = new_script(PLUTUS_V3_SPEND_SCRIPT_CBOR);

  add_utxo(wallet_utxos, ordinal, new_output(WALLET_ADDRESS, 10000000U));
  set_inline_datum(script_output, INLINE_DATUM_CBOR);

  cardano_utxo_t*       script_utxo = new_utxo(ordinal + 1U, script_output);
  cardano_tx_builder_t* builder     = new_builder(params, wallet_utxos, memory, steps);

  cardano_tx_builder_add_input(builder, script_utxo, redeemer, NULL);
  cardano_tx_builder_add_script(builder, script);
  set_collateral(builder, collateral_utxos);

  cardano_utxo_list_unref(&wallet_utxos);
  cardano_transaction_output_unref(&script_output);
  cardano_utxo_unref(&script_utxo);
  cardano_plutus_data_unref(&redeemer);
  cardano_script_unref(&script);

  return builder;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_ledger_minimum_fee, chargesTheLedgerMinimumForASimpleSend)
{
  // Arrange
  cardano_protocol_parameters_t* params       = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();

  add_utxo(wallet_utxos, 0x02U, new_output(WALLET_ADDRESS, 10000000U));
  add_utxo(wallet_utxos, 0x03U, new_output(WALLET_ADDRESS, 3000000U));
  add_utxo(wallet_utxos, 0x04U, new_output(WALLET_ADDRESS, 25000000U));

  cardano_tx_builder_t*         builder = new_builder(params, wallet_utxos, 0U, 0U);
  cardano_transaction_output_t* payment = new_output(PAYEE_ADDRESS, 5000000U);

  cardano_tx_builder_add_output(builder, payment);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_ledger_minimum_fee(tx, 1U, 0U, 0U, 0U, 0U, 167305U);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, chargesTheLedgerMinimumForASendOfNativeAssetsWithChange)
{
  // Arrange
  cardano_protocol_parameters_t* params       = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();
  cardano_transaction_output_t*  tokens       = new_output(WALLET_ADDRESS, 8000000U);

  add_asset(tokens, FIRST_TOKEN_ID, 100);
  add_asset(tokens, SECOND_TOKEN_ID, 50);
  add_utxo(wallet_utxos, 0x05U, tokens);
  add_utxo(wallet_utxos, 0x06U, new_output(WALLET_ADDRESS, 5000000U));

  cardano_tx_builder_t*         builder = new_builder(params, wallet_utxos, 0U, 0U);
  cardano_transaction_output_t* payment = new_output(PAYEE_ADDRESS, 2000000U);

  add_asset(payment, FIRST_TOKEN_ID, 30);
  cardano_tx_builder_add_output(builder, payment);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_ledger_minimum_fee(tx, 1U, 0U, 0U, 0U, 0U, 171397U);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, chargesTheLedgerMinimumForAPlutusV2SpendThroughAReferenceScript)
{
  // Arrange
  cardano_protocol_parameters_t* params           = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos     = new_utxo_list();
  cardano_utxo_list_t*           collateral_utxos = new_utxo_list();
  cardano_transaction_output_t*  script_output    = new_output(PLUTUS_V2_SCRIPT_ADDRESS, 10000000U);
  cardano_transaction_output_t*  holder_output    = new_output(REFERENCE_SCRIPT_HOLDER_ADDRESS, 20000000U);
  cardano_script_t*              script           = new_script(PLUTUS_V2_SPEND_SCRIPT_CBOR);
  cardano_plutus_data_t*         redeemer         = new_plutus_data(REDEEMER_CBOR);

  set_inline_datum(script_output, INLINE_DATUM_CBOR);
  EXPECT_EQ(cardano_transaction_output_set_script_ref(holder_output, script), CARDANO_SUCCESS);
  add_utxo(collateral_utxos, 0x01U, new_output(COLLATERAL_ADDRESS, 5000000U));

  cardano_utxo_t*       script_utxo = new_utxo(0x0aU, script_output);
  cardano_utxo_t*       holder_utxo = new_utxo(0x0bU, holder_output);
  cardano_tx_builder_t* builder     = new_builder(params, wallet_utxos, 500000U, 200000000U);

  cardano_tx_builder_add_input(builder, script_utxo, redeemer, NULL);
  cardano_tx_builder_add_reference_input(builder, holder_utxo);
  set_collateral(builder, collateral_utxos);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_collateral(tx, collateral_utxos, 1U);
  expect_ledger_minimum_fee(tx, 1U, 500000U, 200000000U, 1U, PLUTUS_V2_SPEND_SCRIPT_SIZE, 218263U);

  // Cleanup
  cardano_transaction_output_unref(&script_output);
  cardano_transaction_output_unref(&holder_output);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&holder_utxo);
  cardano_script_unref(&script);
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_utxo_list_unref(&collateral_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, chargesTheLedgerMinimumForAPlutusV3SpendWithTheDatumInTheWitnessSet)
{
  // Arrange
  cardano_protocol_parameters_t* params           = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos     = new_utxo_list();
  cardano_utxo_list_t*           collateral_utxos = new_utxo_list();
  cardano_transaction_output_t*  script_output    = new_output(PLUTUS_V3_SCRIPT_ADDRESS, 10000000U);
  cardano_script_t*              script           = new_script(PLUTUS_V3_SPEND_SCRIPT_CBOR);
  cardano_plutus_data_t*         redeemer         = new_plutus_data(REDEEMER_CBOR);
  cardano_plutus_data_t*         datum            = new_plutus_data(WITNESS_DATUM_CBOR);

  set_datum_hash(script_output, WITNESS_DATUM_HASH_HEX);
  add_utxo(collateral_utxos, 0x0dU, new_output(COLLATERAL_ADDRESS, 5000000U));

  cardano_utxo_t*       script_utxo = new_utxo(0x0cU, script_output);
  cardano_tx_builder_t* builder     = new_builder(params, wallet_utxos, 700000U, 300000000U);

  cardano_tx_builder_add_input(builder, script_utxo, redeemer, datum);
  cardano_tx_builder_add_script(builder, script);
  set_collateral(builder, collateral_utxos);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_collateral(tx, collateral_utxos, 1U);
  expect_ledger_minimum_fee(tx, 1U, 700000U, 300000000U, 1U, 0U, 235969U);

  // Cleanup
  cardano_transaction_output_unref(&script_output);
  cardano_utxo_unref(&script_utxo);
  cardano_script_unref(&script);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_utxo_list_unref(&collateral_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, chargesTheLedgerMinimumForAMintUnderAPlutusV3Policy)
{
  // Arrange
  cardano_protocol_parameters_t* params           = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos     = new_utxo_list();
  cardano_utxo_list_t*           collateral_utxos = new_utxo_list();
  cardano_script_t*              script           = new_script(PLUTUS_V3_MINT_SCRIPT_CBOR);
  cardano_plutus_data_t*         redeemer         = new_plutus_data(REDEEMER_CBOR);
  cardano_transaction_output_t*  payment          = new_output(PAYEE_ADDRESS, 2000000U);

  add_utxo(wallet_utxos, 0x14U, new_output(WALLET_ADDRESS, 20000000U));
  add_utxo(collateral_utxos, 0x15U, new_output(COLLATERAL_ADDRESS, 5000000U));
  add_asset(payment, MINTED_ASSET_ID, 400);

  cardano_tx_builder_t* builder = new_builder(params, wallet_utxos, 400000U, 150000000U);

  cardano_tx_builder_add_output(builder, payment);
  cardano_tx_builder_mint_token_ex(builder, MINTED_POLICY_ID, strlen(MINTED_POLICY_ID), MINTED_ASSET_NAME, strlen(MINTED_ASSET_NAME), 1000, redeemer);
  cardano_tx_builder_add_script(builder, script);
  set_collateral(builder, collateral_utxos);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_collateral(tx, collateral_utxos, 1U);
  expect_ledger_minimum_fee(tx, 2U, 400000U, 150000000U, 1U, 0U, 219108U);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_script_unref(&script);
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_utxo_list_unref(&collateral_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, chargesTheLedgerMinimumWhenTheCollateralReturnsNativeAssets)
{
  // Arrange
  cardano_protocol_parameters_t* params           = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos     = new_utxo_list();
  cardano_utxo_list_t*           collateral_utxos = new_utxo_list();
  cardano_transaction_output_t*  collateral       = new_output(COLLATERAL_ADDRESS, 8000000U);
  cardano_transaction_output_t*  script_output    = new_output(PLUTUS_V3_SCRIPT_ADDRESS, 3000000U);
  cardano_transaction_output_t*  payment          = new_output(PAYEE_ADDRESS, 4000000U);
  cardano_script_t*              script           = new_script(PLUTUS_V3_SPEND_SCRIPT_CBOR);
  cardano_plutus_data_t*         redeemer         = new_plutus_data(REDEEMER_CBOR);

  add_utxo(wallet_utxos, 0x17U, new_output(WALLET_ADDRESS, 6000000U));
  add_asset(collateral, FIRST_TOKEN_ID, 7);
  add_utxo(collateral_utxos, 0x19U, collateral);
  set_inline_datum(script_output, INLINE_DATUM_CBOR);

  cardano_utxo_t*       script_utxo = new_utxo(0x18U, script_output);
  cardano_tx_builder_t* builder     = new_builder(params, wallet_utxos, 600000U, 250000000U);

  cardano_tx_builder_add_input(builder, script_utxo, redeemer, NULL);
  cardano_tx_builder_add_script(builder, script);
  cardano_tx_builder_add_output(builder, payment);
  set_collateral(builder, collateral_utxos);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_collateral(tx, collateral_utxos, 1U);
  expect_ledger_minimum_fee(tx, 2U, 600000U, 250000000U, 1U, 0U, 235834U);

  // Cleanup
  cardano_transaction_output_unref(&script_output);
  cardano_transaction_output_unref(&payment);
  cardano_utxo_unref(&script_utxo);
  cardano_script_unref(&script);
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_utxo_list_unref(&collateral_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, selectsTheCollateralForTheFeeWhenNoSingleCollateralUtxoLeavesAReturn)
{
  // Arrange
  cardano_protocol_parameters_t* params           = new_protocol_parameters();
  cardano_utxo_list_t*           collateral_utxos = new_utxo_list();

  for (uint64_t ordinal = 0x35U; ordinal <= 0x3aU; ++ordinal)
  {
    add_utxo(collateral_utxos, ordinal, new_output(COLLATERAL_ADDRESS, 1300000U));
  }

  cardano_tx_builder_t* builder = new_plutus_v3_spend_builder(params, collateral_utxos, 0x33U, 600000U, 250000000U);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_collateral(tx, collateral_utxos, 2U);
  expect_ledger_minimum_fee(tx, 1U, 600000U, 250000000U, 1U, 0U, 227870U);

  // Cleanup
  cardano_utxo_list_unref(&collateral_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, selectsTheCollateralForTheFeeWhenTheLargestCollateralUtxoLeavesAReturnBelowMinAda)
{
  // Arrange
  cardano_protocol_parameters_t* params           = new_protocol_parameters();
  cardano_utxo_list_t*           collateral_utxos = new_utxo_list();

  add_utxo(collateral_utxos, 0x30U, new_output(COLLATERAL_ADDRESS, 1250000U));
  add_utxo(collateral_utxos, 0x31U, new_output(COLLATERAL_ADDRESS, 1200000U));
  add_utxo(collateral_utxos, 0x32U, new_output(COLLATERAL_ADDRESS, 1200000U));

  cardano_tx_builder_t* builder = new_plutus_v3_spend_builder(params, collateral_utxos, 0x2eU, 600000U, 250000000U);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_collateral(tx, collateral_utxos, 2U);
  expect_ledger_minimum_fee(tx, 1U, 600000U, 250000000U, 1U, 0U, 227870U);

  // Cleanup
  cardano_utxo_list_unref(&collateral_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, lowersTheFeeWhenTheChangeNeedsFewerBytesThanEstimated)
{
  // Arrange
  cardano_protocol_parameters_t* params       = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();

  add_utxo(wallet_utxos, 0x40U, new_output(WALLET_ADDRESS, 5000000U + LOWERED_FEE_MINIMUM_FEE + SMALLEST_NINE_BYTE_COIN));

  cardano_tx_builder_t*         builder = new_builder(params, wallet_utxos, 0U, 0U);
  cardano_transaction_output_t* payment = new_output(PAYEE_ADDRESS, 5000000U);

  cardano_tx_builder_set_minimum_fee(builder, LOWERED_FEE_MINIMUM_FEE);
  cardano_tx_builder_add_output(builder, payment);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);
  expect_ledger_minimum_fee(tx, 1U, 0U, 0U, 0U, 0U, 167305U);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, keepsTheHigherFeeWhenLoweringItWouldGrowTheTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params       = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();

  add_utxo(wallet_utxos, 0x40U, new_output(WALLET_ADDRESS, 5000000U + 167305U + SMALLEST_NINE_BYTE_COIN + 100U));

  cardano_tx_builder_t*         builder = new_builder(params, wallet_utxos, 0U, 0U);
  cardano_transaction_output_t* payment = new_output(PAYEE_ADDRESS, 5000000U);

  cardano_tx_builder_add_output(builder, payment);

  // Act
  cardano_transaction_t* tx = build(builder);

  // Assert
  ASSERT_NE(tx, nullptr);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  add_vkey_witnesses(tx, 1U);

  const uint64_t ledger_minimum_fee = (MIN_FEE_A * get_ledger_size(tx)) + MIN_FEE_B;

  EXPECT_EQ(ledger_minimum_fee, 167305U);
  EXPECT_EQ(cardano_transaction_body_get_fee(body), 167481U);
  EXPECT_GE(cardano_transaction_body_get_fee(body), ledger_minimum_fee);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_unref(&tx);
}

TEST(cardano_ledger_minimum_fee, reportsTheShortfallWhenTheWalletCanNotFundTheTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params       = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();

  add_utxo(wallet_utxos, 0x41U, new_output(WALLET_ADDRESS, 1000000U));

  cardano_tx_builder_t*         builder = new_builder(params, wallet_utxos, 0U, 0U);
  cardano_transaction_output_t* payment = new_output(PAYEE_ADDRESS, 5000000U);
  cardano_transaction_t*        tx      = NULL;

  cardano_tx_builder_add_output(builder, payment);

  // Act
  cardano_error_t result = cardano_tx_builder_build(builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(builder), WALLET_SHORTFALL_ERROR);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_ledger_minimum_fee, reportsTheMissingAssetWhenTheWalletHasEnoughLovelaceButNotTheAssets)
{
  // Arrange
  cardano_protocol_parameters_t* params       = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();

  add_utxo(wallet_utxos, 0x45U, new_output(WALLET_ADDRESS, 5000000U));

  cardano_tx_builder_t*         builder = new_builder(params, wallet_utxos, 0U, 0U);
  cardano_transaction_output_t* payment = new_output(PAYEE_ADDRESS, 2000000U);
  cardano_transaction_t*        tx      = NULL;

  add_asset(payment, FIRST_TOKEN_ID, 30);
  cardano_tx_builder_add_output(builder, payment);
  cardano_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 20000000, NULL);

  // Act
  cardano_error_t result = cardano_tx_builder_build(builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(builder), ASSET_SHORTFALL_ERROR);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_ledger_minimum_fee, reportsNoRequiredLovelaceWhenTheWithdrawalCoversTheOutputs)
{
  // Arrange
  cardano_protocol_parameters_t* params       = new_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();

  cardano_tx_builder_t*         builder = new_builder(params, wallet_utxos, 0U, 0U);
  cardano_transaction_output_t* payment = new_output(PAYEE_ADDRESS, 2000000U);
  cardano_transaction_t*        tx      = NULL;

  cardano_tx_builder_add_output(builder, payment);
  cardano_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 20000000, NULL);

  // Act
  cardano_error_t result = cardano_tx_builder_build(builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(builder), COVERED_OUTPUTS_SHORTFALL_ERROR);

  // Cleanup
  cardano_transaction_output_unref(&payment);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_ledger_minimum_fee, reportsTheShortfallWhenTheCollateralUtxosCanNotCoverTheFee)
{
  // Arrange
  cardano_protocol_parameters_t* params           = new_protocol_parameters();
  cardano_utxo_list_t*           collateral_utxos = new_utxo_list();

  add_utxo(collateral_utxos, 0x44U, new_output(COLLATERAL_ADDRESS, 1000000U));

  cardano_tx_builder_t*  builder = new_plutus_v3_spend_builder(params, collateral_utxos, 0x42U, 600000U, 250000000U);
  cardano_transaction_t* tx      = NULL;

  // Act
  cardano_error_t result = cardano_tx_builder_build(builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(builder), COLLATERAL_SHORTFALL_ERROR);

  // Cleanup
  cardano_utxo_list_unref(&collateral_utxos);
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}
