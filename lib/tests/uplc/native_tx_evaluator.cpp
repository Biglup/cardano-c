/**
 * \file native_tx_evaluator.cpp
 *
 * \author angel.castillo
 * \date   Jun 20, 2026
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

#include "../../src/uplc/ast/uplc_program.h"
#include "../../src/uplc/ast/uplc_term.h"
#include <cardano/address/address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/common/credential.h>
#include <cardano/common/datum.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/witness_set/plutus_v3_script_set.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "../../src/uplc/arena/uplc_arena.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const cardano_slot_config_t kSlotConfig = { 1596059091000U, 4492800U, 1000U };

/* STATIC HELPERS ************************************************************/

/**
 * \brief Builds the CBOR-wrapped flat bytes of a one-argument V3 program.
 *
 * The program is '(lam x body)': it ignores its single applied argument (the
 * ScriptContext) and reduces to \p body. A unit-constant body always succeeds; an
 * error-term body always fails. The bytes are the ledger witness-set form the
 * native evaluator decodes.
 */
static cardano_plutus_v3_script_t*
build_v3_script(bool succeeds)
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_term_t* body = nullptr;

  if (succeeds)
  {
    cardano_uplc_constant_t* unit = nullptr;
    EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &unit), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_uplc_term_new_constant(arena, unit, &body), CARDANO_SUCCESS);
  }
  else
  {
    EXPECT_EQ(cardano_uplc_term_new_error(arena, &body), CARDANO_SUCCESS);
  }

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, body, &lambda), CARDANO_SUCCESS);

  cardano_uplc_program_t program;
  program.version_major = 1U;
  program.version_minor = 0U;
  program.version_patch = 0U;
  program.term          = lambda;

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(&program, &cbor), CARDANO_SUCCESS);

  cardano_plutus_v3_script_t* script = nullptr;
  EXPECT_EQ(cardano_plutus_v3_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_buffer_unref(&cbor);
  cardano_uplc_arena_free(&arena);

  return script;
}

/**
 * \brief Builds a script enterprise address from a script hash.
 */
static cardano_address_t*
script_address(cardano_blake2b_hash_t* script_hash)
{
  cardano_credential_t* credential = nullptr;
  EXPECT_EQ(cardano_credential_new(script_hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);

  cardano_enterprise_address_t* enterprise = nullptr;
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise), CARDANO_SUCCESS);

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise);

  cardano_enterprise_address_unref(&enterprise);
  cardano_credential_unref(&credential);

  return address;
}

/**
 * \brief Builds an out-ref input from a 32-byte id seed and an index.
 */
static cardano_transaction_input_t*
make_input(uint8_t seed, uint64_t index)
{
  byte_t id_bytes[32];
  memset(id_bytes, seed, sizeof(id_bytes));

  cardano_blake2b_hash_t* id = nullptr;
  EXPECT_EQ(cardano_blake2b_hash_from_bytes(id_bytes, sizeof(id_bytes), &id), CARDANO_SUCCESS);

  cardano_transaction_input_t* input = nullptr;
  EXPECT_EQ(cardano_transaction_input_new(id, index, &input), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);

  return input;
}

/**
 * \brief Builds a unit-constructor plutus-data value (Constr 0 []).
 */
static cardano_plutus_data_t*
unit_data()
{
  cardano_plutus_data_t* data = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(42, &data), CARDANO_SUCCESS);
  return data;
}

/**
 * \brief Builds a single-spend V3 transaction and its resolved-input set.
 *
 * The tx spends one input at a script address whose script is \p script, carries
 * one V3 spend redeemer, and (optionally) an inline datum on the spent output.
 */
static cardano_transaction_t*
build_spend_tx(cardano_plutus_v3_script_t* script, bool with_inline_datum, cardano_utxo_list_t** out_utxos)
{
  cardano_blake2b_hash_t*      hash  = cardano_plutus_v3_script_get_hash(script);
  cardano_address_t*           addr  = script_address(hash);
  cardano_transaction_input_t* input = make_input(0x00U, 0U);

  cardano_transaction_input_set_t* inputs = nullptr;
  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(inputs, input), CARDANO_SUCCESS);

  cardano_transaction_output_t* tx_out = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 1000000U, &tx_out), CARDANO_SUCCESS);

  cardano_transaction_output_list_t* outputs = nullptr;
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_add(outputs, tx_out), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = nullptr;
  EXPECT_EQ(cardano_transaction_body_new(inputs, outputs, 200000U, nullptr, &body), CARDANO_SUCCESS);

  cardano_plutus_v3_script_set_t* script_set = nullptr;
  EXPECT_EQ(cardano_plutus_v3_script_set_new(&script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v3_script_set_add(script_set, script), CARDANO_SUCCESS);

  cardano_plutus_data_t* redeemer_data = unit_data();
  cardano_ex_units_t*    zero_units    = nullptr;
  EXPECT_EQ(cardano_ex_units_new(0U, 0U, &zero_units), CARDANO_SUCCESS);

  cardano_redeemer_t* redeemer = nullptr;
  EXPECT_EQ(cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0U, redeemer_data, zero_units, &redeemer), CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemers = nullptr;
  EXPECT_EQ(cardano_redeemer_list_new(&redeemers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_add(redeemers, redeemer), CARDANO_SUCCESS);

  cardano_witness_set_t* witness_set = nullptr;
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(witness_set, script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);

  cardano_transaction_t* tx = nullptr;
  EXPECT_EQ(cardano_transaction_new(body, witness_set, nullptr, &tx), CARDANO_SUCCESS);

  cardano_transaction_input_t*  resolved_input = make_input(0x00U, 0U);
  cardano_transaction_output_t* resolved_out   = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 5000000U, &resolved_out), CARDANO_SUCCESS);

  if (with_inline_datum)
  {
    cardano_plutus_data_t* inline_d = unit_data();
    cardano_datum_t*       datum    = nullptr;
    EXPECT_EQ(cardano_datum_new_inline_data(inline_d, &datum), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_transaction_output_set_datum(resolved_out, datum), CARDANO_SUCCESS);
    cardano_datum_unref(&datum);
    cardano_plutus_data_unref(&inline_d);
  }

  cardano_utxo_t* utxo = nullptr;
  EXPECT_EQ(cardano_utxo_new(resolved_input, resolved_out, &utxo), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_utxo_list_new(out_utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(*out_utxos, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&resolved_out);
  cardano_transaction_input_unref(&resolved_input);
  cardano_witness_set_unref(&witness_set);
  cardano_redeemer_list_unref(&redeemers);
  cardano_redeemer_unref(&redeemer);
  cardano_ex_units_unref(&zero_units);
  cardano_plutus_data_unref(&redeemer_data);
  cardano_plutus_v3_script_set_unref(&script_set);
  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_output_unref(&tx_out);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_unref(&input);
  cardano_address_unref(&addr);
  cardano_blake2b_hash_unref(&hash);

  return tx;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_tx_evaluator_new_native, rejectsNullArguments)
{
  cardano_tx_evaluator_t* evaluator = nullptr;

  EXPECT_EQ(cardano_tx_evaluator_new_native(nullptr, nullptr, 10U, &evaluator), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_tx_evaluator_new_native, buildsAndNamesTheEvaluator)
{
  cardano_tx_evaluator_t* evaluator = nullptr;

  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);
  ASSERT_NE(evaluator, nullptr);
  EXPECT_STREQ(cardano_tx_evaluator_get_name(evaluator), "Native UPLC transaction evaluator");

  cardano_tx_evaluator_unref(&evaluator);
}

TEST(cardano_tx_evaluator_native, evaluatesAnAlwaysSucceedsV3Spend)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(result), 1U);

  cardano_redeemer_t* out = nullptr;
  EXPECT_EQ(cardano_redeemer_list_get(result, 0U, &out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_get_tag(out), CARDANO_REDEEMER_TAG_SPEND);

  cardano_ex_units_t* units = cardano_redeemer_get_ex_units(out);
  EXPECT_GT(cardano_ex_units_get_cpu_steps(units), 0U);
  EXPECT_GT(cardano_ex_units_get_memory(units), 0U);

  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&out);
  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, reportsAnAlwaysFailsScriptAsPhaseTwoFailure)
{
  cardano_plutus_v3_script_t* script = build_v3_script(false);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);
  EXPECT_EQ(result, nullptr);

  cardano_tx_evaluator_unref(&evaluator);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, failsWhenAnInputCannotBeResolved)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  cardano_utxo_list_t* empty = nullptr;
  EXPECT_EQ(cardano_utxo_list_new(&empty), CARDANO_SUCCESS);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_NE(cardano_tx_evaluator_evaluate(evaluator, tx, empty, &result), CARDANO_SUCCESS);
  EXPECT_EQ(result, nullptr);

  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_list_unref(&empty);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, returnsEmptyListWhenNoRedeemers)
{
  cardano_transaction_input_set_t* inputs = nullptr;
  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);

  cardano_transaction_output_list_t* outputs = nullptr;
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = nullptr;
  EXPECT_EQ(cardano_transaction_body_new(inputs, outputs, 200000U, nullptr, &body), CARDANO_SUCCESS);

  cardano_witness_set_t* witness_set = nullptr;
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);

  cardano_transaction_t* tx = nullptr;
  EXPECT_EQ(cardano_transaction_new(body, witness_set, nullptr, &tx), CARDANO_SUCCESS);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, nullptr, &result), CARDANO_SUCCESS);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(result), 0U);

  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&witness_set);
  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_input_set_unref(&inputs);
}

TEST(cardano_tx_evaluator_native, evaluatesAMintAndSpendAcrossTwoRedeemers)
{
  cardano_plutus_v3_script_t* spend_script = build_v3_script(true);
  cardano_plutus_v3_script_t* mint_script  = build_v3_script(true);

  cardano_blake2b_hash_t* spend_hash = cardano_plutus_v3_script_get_hash(spend_script);
  cardano_blake2b_hash_t* mint_hash  = cardano_plutus_v3_script_get_hash(mint_script);

  cardano_address_t*           addr  = script_address(spend_hash);
  cardano_transaction_input_t* input = make_input(0x00U, 0U);

  cardano_transaction_input_set_t* inputs = nullptr;
  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(inputs, input), CARDANO_SUCCESS);

  cardano_transaction_output_t* tx_out = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 1000000U, &tx_out), CARDANO_SUCCESS);

  cardano_transaction_output_list_t* outputs = nullptr;
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_add(outputs, tx_out), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = nullptr;
  EXPECT_EQ(cardano_transaction_body_new(inputs, outputs, 200000U, nullptr, &body), CARDANO_SUCCESS);

  byte_t                name_bytes[3] = { 0x41U, 0x42U, 0x43U };
  cardano_asset_name_t* asset         = nullptr;
  EXPECT_EQ(cardano_asset_name_from_bytes(name_bytes, sizeof(name_bytes), &asset), CARDANO_SUCCESS);

  cardano_multi_asset_t* mint = nullptr;
  EXPECT_EQ(cardano_multi_asset_new(&mint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_set(mint, mint_hash, asset, 5), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_mint(body, mint), CARDANO_SUCCESS);

  cardano_plutus_v3_script_set_t* script_set = nullptr;
  EXPECT_EQ(cardano_plutus_v3_script_set_new(&script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v3_script_set_add(script_set, spend_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v3_script_set_add(script_set, mint_script), CARDANO_SUCCESS);

  cardano_ex_units_t* zero_units = nullptr;
  EXPECT_EQ(cardano_ex_units_new(0U, 0U, &zero_units), CARDANO_SUCCESS);

  cardano_plutus_data_t* spend_rd = unit_data();
  cardano_redeemer_t*    spend_r  = nullptr;
  EXPECT_EQ(cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0U, spend_rd, zero_units, &spend_r), CARDANO_SUCCESS);

  cardano_plutus_data_t* mint_rd = unit_data();
  cardano_redeemer_t*    mint_r  = nullptr;
  EXPECT_EQ(cardano_redeemer_new(CARDANO_REDEEMER_TAG_MINT, 0U, mint_rd, zero_units, &mint_r), CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemers = nullptr;
  EXPECT_EQ(cardano_redeemer_list_new(&redeemers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_add(redeemers, spend_r), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_add(redeemers, mint_r), CARDANO_SUCCESS);

  cardano_witness_set_t* witness_set = nullptr;
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(witness_set, script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);

  cardano_transaction_t* tx = nullptr;
  EXPECT_EQ(cardano_transaction_new(body, witness_set, nullptr, &tx), CARDANO_SUCCESS);

  cardano_transaction_input_t*  resolved_input = make_input(0x00U, 0U);
  cardano_transaction_output_t* resolved_out   = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 5000000U, &resolved_out), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = nullptr;
  EXPECT_EQ(cardano_utxo_new(resolved_input, resolved_out, &utxo), CARDANO_SUCCESS);

  cardano_utxo_list_t* utxos = nullptr;
  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, utxo), CARDANO_SUCCESS);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(result), 2U);

  for (size_t i = 0U; i < 2U; ++i)
  {
    cardano_redeemer_t* out = nullptr;
    EXPECT_EQ(cardano_redeemer_list_get(result, i, &out), CARDANO_SUCCESS);

    cardano_ex_units_t* units = cardano_redeemer_get_ex_units(out);
    EXPECT_GT(cardano_ex_units_get_cpu_steps(units), 0U);

    cardano_ex_units_unref(&units);
    cardano_redeemer_unref(&out);
  }

  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_list_unref(&utxos);
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&resolved_out);
  cardano_transaction_input_unref(&resolved_input);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&witness_set);
  cardano_redeemer_list_unref(&redeemers);
  cardano_redeemer_unref(&mint_r);
  cardano_redeemer_unref(&spend_r);
  cardano_plutus_data_unref(&mint_rd);
  cardano_plutus_data_unref(&spend_rd);
  cardano_ex_units_unref(&zero_units);
  cardano_plutus_v3_script_set_unref(&script_set);
  cardano_multi_asset_unref(&mint);
  cardano_asset_name_unref(&asset);
  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_output_unref(&tx_out);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_unref(&input);
  cardano_address_unref(&addr);
  cardano_blake2b_hash_unref(&mint_hash);
  cardano_blake2b_hash_unref(&spend_hash);
  cardano_plutus_v3_script_unref(&mint_script);
  cardano_plutus_v3_script_unref(&spend_script);
}

TEST(cardano_tx_evaluator_native, perRedeemerUnitsAreTheActualSpendNotTheSharedCeiling)
{
  cardano_plutus_v3_script_t* solo_script = build_v3_script(true);
  cardano_utxo_list_t*        solo_utxos  = nullptr;
  cardano_transaction_t*      solo_tx     = build_spend_tx(solo_script, true, &solo_utxos);

  cardano_tx_evaluator_t* solo_eval = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &solo_eval), CARDANO_SUCCESS);

  cardano_redeemer_list_t* solo_result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(solo_eval, solo_tx, solo_utxos, &solo_result), CARDANO_SUCCESS);
  ASSERT_NE(solo_result, nullptr);
  ASSERT_EQ(cardano_redeemer_list_get_length(solo_result), 1U);

  cardano_redeemer_t* solo_out = nullptr;
  EXPECT_EQ(cardano_redeemer_list_get(solo_result, 0U, &solo_out), CARDANO_SUCCESS);
  cardano_ex_units_t* solo_units = cardano_redeemer_get_ex_units(solo_out);
  uint64_t            solo_cpu   = cardano_ex_units_get_cpu_steps(solo_units);
  uint64_t            solo_mem   = cardano_ex_units_get_memory(solo_units);

  cardano_ex_units_unref(&solo_units);
  cardano_redeemer_unref(&solo_out);
  cardano_redeemer_list_unref(&solo_result);
  cardano_tx_evaluator_unref(&solo_eval);
  cardano_transaction_unref(&solo_tx);
  cardano_utxo_list_unref(&solo_utxos);
  cardano_plutus_v3_script_unref(&solo_script);

  cardano_plutus_v3_script_t* spend_script = build_v3_script(true);
  cardano_plutus_v3_script_t* mint_script  = build_v3_script(true);

  cardano_blake2b_hash_t* spend_hash = cardano_plutus_v3_script_get_hash(spend_script);
  cardano_blake2b_hash_t* mint_hash  = cardano_plutus_v3_script_get_hash(mint_script);

  cardano_address_t*           addr  = script_address(spend_hash);
  cardano_transaction_input_t* input = make_input(0x00U, 0U);

  cardano_transaction_input_set_t* inputs = nullptr;
  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(inputs, input), CARDANO_SUCCESS);

  cardano_transaction_output_t* tx_out = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 1000000U, &tx_out), CARDANO_SUCCESS);

  cardano_transaction_output_list_t* outputs = nullptr;
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_add(outputs, tx_out), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = nullptr;
  EXPECT_EQ(cardano_transaction_body_new(inputs, outputs, 200000U, nullptr, &body), CARDANO_SUCCESS);

  byte_t                name_bytes[3] = { 0x41U, 0x42U, 0x43U };
  cardano_asset_name_t* asset         = nullptr;
  EXPECT_EQ(cardano_asset_name_from_bytes(name_bytes, sizeof(name_bytes), &asset), CARDANO_SUCCESS);

  cardano_multi_asset_t* mint = nullptr;
  EXPECT_EQ(cardano_multi_asset_new(&mint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_set(mint, mint_hash, asset, 5), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_mint(body, mint), CARDANO_SUCCESS);

  cardano_plutus_v3_script_set_t* script_set = nullptr;
  EXPECT_EQ(cardano_plutus_v3_script_set_new(&script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v3_script_set_add(script_set, spend_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v3_script_set_add(script_set, mint_script), CARDANO_SUCCESS);

  cardano_ex_units_t* zero_units = nullptr;
  EXPECT_EQ(cardano_ex_units_new(0U, 0U, &zero_units), CARDANO_SUCCESS);

  cardano_plutus_data_t* spend_rd = unit_data();
  cardano_redeemer_t*    spend_r  = nullptr;
  EXPECT_EQ(cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0U, spend_rd, zero_units, &spend_r), CARDANO_SUCCESS);

  cardano_plutus_data_t* mint_rd = unit_data();
  cardano_redeemer_t*    mint_r  = nullptr;
  EXPECT_EQ(cardano_redeemer_new(CARDANO_REDEEMER_TAG_MINT, 0U, mint_rd, zero_units, &mint_r), CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemers = nullptr;
  EXPECT_EQ(cardano_redeemer_list_new(&redeemers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_add(redeemers, spend_r), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_add(redeemers, mint_r), CARDANO_SUCCESS);

  cardano_witness_set_t* witness_set = nullptr;
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(witness_set, script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);

  cardano_transaction_t* tx = nullptr;
  EXPECT_EQ(cardano_transaction_new(body, witness_set, nullptr, &tx), CARDANO_SUCCESS);

  cardano_transaction_input_t*  resolved_input = make_input(0x00U, 0U);
  cardano_transaction_output_t* resolved_out   = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 5000000U, &resolved_out), CARDANO_SUCCESS);

  cardano_utxo_t* utxo = nullptr;
  EXPECT_EQ(cardano_utxo_new(resolved_input, resolved_out, &utxo), CARDANO_SUCCESS);

  cardano_utxo_list_t* utxos = nullptr;
  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, utxo), CARDANO_SUCCESS);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(cardano_redeemer_list_get_length(result), 2U);

  for (size_t i = 0U; i < 2U; ++i)
  {
    cardano_redeemer_t* out = nullptr;
    EXPECT_EQ(cardano_redeemer_list_get(result, i, &out), CARDANO_SUCCESS);

    cardano_ex_units_t* units = cardano_redeemer_get_ex_units(out);

    EXPECT_EQ(cardano_ex_units_get_cpu_steps(units), solo_cpu);
    EXPECT_EQ(cardano_ex_units_get_memory(units), solo_mem);

    cardano_ex_units_unref(&units);
    cardano_redeemer_unref(&out);
  }

  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_list_unref(&utxos);
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&resolved_out);
  cardano_transaction_input_unref(&resolved_input);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&witness_set);
  cardano_redeemer_list_unref(&redeemers);
  cardano_redeemer_unref(&mint_r);
  cardano_redeemer_unref(&spend_r);
  cardano_plutus_data_unref(&mint_rd);
  cardano_plutus_data_unref(&spend_rd);
  cardano_ex_units_unref(&zero_units);
  cardano_plutus_v3_script_set_unref(&script_set);
  cardano_multi_asset_unref(&mint);
  cardano_asset_name_unref(&asset);
  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_output_unref(&tx_out);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_unref(&input);
  cardano_address_unref(&addr);
  cardano_blake2b_hash_unref(&mint_hash);
  cardano_blake2b_hash_unref(&spend_hash);
  cardano_plutus_v3_script_unref(&mint_script);
  cardano_plutus_v3_script_unref(&spend_script);
}

TEST(cardano_tx_evaluator_native, rejectsNullEvaluatorOrTransaction)
{
  cardano_redeemer_list_t* result = nullptr;

  EXPECT_EQ(cardano_tx_evaluator_evaluate(nullptr, nullptr, nullptr, &result), CARDANO_ERROR_POINTER_IS_NULL);
}
