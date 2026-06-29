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
#include <cardano/address/reward_address.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/certificate.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/certs/resign_committee_cold_cert.h>
#include <cardano/certs/stake_deregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/common/anchor.h>
#include <cardano/common/credential.h>
#include <cardano/common/datum.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/protocol_params/protocol_param_update.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voting_procedure.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/plutus_v1_script_set.h>
#include <cardano/witness_set/plutus_v2_script_set.h>
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
 * \brief Builds the CBOR-wrapped flat bytes of an N-argument always-succeeds program.
 *
 * The body is a unit constant wrapped in \p num_args nested lambdas, so the
 * program ignores exactly \p num_args applied arguments and reduces to unit. V1/V2
 * spend scripts receive [datum, redeemer, context] (3 args); V1/V2 other purposes
 * receive [redeemer, context] (2 args).
 */
static cardano_buffer_t*
build_succeeds_program_cbor(size_t num_args)
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_constant_t* unit = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &unit), CARDANO_SUCCESS);

  cardano_uplc_term_t* body = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, unit, &body), CARDANO_SUCCESS);

  for (size_t i = 0U; i < num_args; ++i)
  {
    cardano_uplc_term_t* lambda = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_lambda(arena, body, &lambda), CARDANO_SUCCESS);
    body = lambda;
  }

  cardano_uplc_program_t program;
  program.version_major = 1U;
  program.version_minor = 0U;
  program.version_patch = 0U;
  program.term          = body;

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(&program, &cbor), CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);

  return cbor;
}

/**
 * \brief Builds a Plutus V2 always-succeeds script accepting \p num_args arguments.
 */
static cardano_plutus_v2_script_t*
build_v2_script(size_t num_args)
{
  cardano_buffer_t*           cbor   = build_succeeds_program_cbor(num_args);
  cardano_plutus_v2_script_t* script = nullptr;

  EXPECT_EQ(cardano_plutus_v2_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_buffer_unref(&cbor);

  return script;
}

/**
 * \brief Builds a Plutus V1 always-succeeds script accepting \p num_args arguments.
 */
static cardano_plutus_v1_script_t*
build_v1_script(size_t num_args)
{
  cardano_buffer_t*           cbor   = build_succeeds_program_cbor(num_args);
  cardano_plutus_v1_script_t* script = nullptr;

  EXPECT_EQ(cardano_plutus_v1_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_buffer_unref(&cbor);

  return script;
}

/**
 * \brief Computes the Blake2b-256 datum hash of a plutus-data value, matching the
 *        hash the ledger keys witness datums by.
 */
static cardano_blake2b_hash_t*
datum_hash_of(cardano_plutus_data_t* data)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_plutus_data_to_cbor(data, writer), CARDANO_SUCCESS);

  cardano_buffer_t* buffer = nullptr;
  EXPECT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &buffer), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash = nullptr;
  EXPECT_EQ(cardano_blake2b_compute_hash(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), (size_t)CARDANO_BLAKE2B_HASH_SIZE_256, &hash), CARDANO_SUCCESS);

  cardano_buffer_unref(&buffer);
  cardano_cbor_writer_unref(&writer);

  return hash;
}

/**
 * \brief Builds a V3 script whose body evaluates expModInteger(4, 13, 497) and so
 *        exercises the builtin (and its chain pricing) during evaluation.
 */
static cardano_plutus_v3_script_t*
build_expmod_v3_script()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_term_t* builtin = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER, &builtin), CARDANO_SUCCESS);

  cardano_uplc_term_t* term = builtin;

  const int64_t args[3] = { 4, 13, 497 };
  for (size_t i = 0U; i < 3U; ++i)
  {
    cardano_uplc_constant_t* c = nullptr;
    EXPECT_EQ(cardano_uplc_constant_new_integer_small(arena, args[i], &c), CARDANO_SUCCESS);

    cardano_uplc_term_t* arg = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_constant(arena, c, &arg), CARDANO_SUCCESS);

    cardano_uplc_term_t* applied = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_apply(arena, term, arg, &applied), CARDANO_SUCCESS);
    term = applied;
  }

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, term, &lambda), CARDANO_SUCCESS);

  cardano_uplc_program_t program;
  program.version_major = 1U;
  program.version_minor = 1U;
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

/**
 * \brief Builds a single-spend Plutus V2 transaction and its resolved-input set.
 *
 * The spent output carries an inline datum, and the V2 script (a 3-argument
 * always-succeeds program) is placed in the witness set. This exercises the
 * V1/V2 evaluation path: apply_arguments applies [datum, redeemer, context] and
 * the V2 script context / cost model are used.
 */
static cardano_transaction_t*
build_v2_spend_tx(cardano_plutus_v2_script_t* script, cardano_utxo_list_t** out_utxos)
{
  cardano_blake2b_hash_t*      hash  = cardano_plutus_v2_script_get_hash(script);
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

  cardano_plutus_v2_script_set_t* script_set = nullptr;
  EXPECT_EQ(cardano_plutus_v2_script_set_new(&script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v2_script_set_add(script_set, script), CARDANO_SUCCESS);

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
  EXPECT_EQ(cardano_witness_set_set_plutus_v2_scripts(witness_set, script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);

  cardano_transaction_t* tx = nullptr;
  EXPECT_EQ(cardano_transaction_new(body, witness_set, nullptr, &tx), CARDANO_SUCCESS);

  cardano_transaction_input_t*  resolved_input = make_input(0x00U, 0U);
  cardano_transaction_output_t* resolved_out   = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 5000000U, &resolved_out), CARDANO_SUCCESS);

  cardano_plutus_data_t* inline_d = unit_data();
  cardano_datum_t*       datum    = nullptr;
  EXPECT_EQ(cardano_datum_new_inline_data(inline_d, &datum), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_datum(resolved_out, datum), CARDANO_SUCCESS);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&inline_d);

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
  cardano_plutus_v2_script_set_unref(&script_set);
  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_output_unref(&tx_out);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_unref(&input);
  cardano_address_unref(&addr);
  cardano_blake2b_hash_unref(&hash);

  return tx;
}

/**
 * \brief Builds a single-spend Plutus V1 transaction and its resolved-input set.
 *
 * V1 has no inline datums, so the spent output carries a datum hash and the datum
 * itself is placed in the witness set; this exercises both the V1 evaluation path
 * and the resolve_datum hash-resolution branch.
 */
static cardano_transaction_t*
build_v1_spend_tx(cardano_plutus_v1_script_t* script, cardano_utxo_list_t** out_utxos)
{
  cardano_blake2b_hash_t*      hash  = cardano_plutus_v1_script_get_hash(script);
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

  cardano_plutus_v1_script_set_t* script_set = nullptr;
  EXPECT_EQ(cardano_plutus_v1_script_set_new(&script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v1_script_set_add(script_set, script), CARDANO_SUCCESS);

  cardano_plutus_data_t* datum_data = unit_data();

  cardano_plutus_data_set_t* datums = nullptr;
  EXPECT_EQ(cardano_plutus_data_set_new(&datums), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_add(datums, datum_data), CARDANO_SUCCESS);

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
  EXPECT_EQ(cardano_witness_set_set_plutus_v1_scripts(witness_set, script_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_plutus_data(witness_set, datums), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);

  cardano_transaction_t* tx = nullptr;
  EXPECT_EQ(cardano_transaction_new(body, witness_set, nullptr, &tx), CARDANO_SUCCESS);

  cardano_transaction_input_t*  resolved_input = make_input(0x00U, 0U);
  cardano_transaction_output_t* resolved_out   = nullptr;
  EXPECT_EQ(cardano_transaction_output_new(addr, 5000000U, &resolved_out), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* datum_hash = datum_hash_of(datum_data);
  cardano_datum_t*        datum      = nullptr;
  EXPECT_EQ(cardano_datum_new_data_hash(datum_hash, &datum), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_datum(resolved_out, datum), CARDANO_SUCCESS);
  cardano_datum_unref(&datum);
  cardano_blake2b_hash_unref(&datum_hash);

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
  cardano_plutus_data_set_unref(&datums);
  cardano_plutus_data_unref(&datum_data);
  cardano_plutus_v1_script_set_unref(&script_set);
  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_output_unref(&tx_out);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_unref(&input);
  cardano_address_unref(&addr);
  cardano_blake2b_hash_unref(&hash);

  return tx;
}

/**
 * \brief Builds a key-hash enterprise address from a 28-byte seed.
 */
static cardano_address_t*
key_address(uint8_t seed)
{
  byte_t bytes[28];
  memset(bytes, seed, sizeof(bytes));

  cardano_blake2b_hash_t* hash = nullptr;
  EXPECT_EQ(cardano_blake2b_hash_from_bytes(bytes, sizeof(bytes), &hash), CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;
  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);

  cardano_enterprise_address_t* enterprise = nullptr;
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise), CARDANO_SUCCESS);

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise);

  cardano_enterprise_address_unref(&enterprise);
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);

  return address;
}

/**
 * \brief Builds a script-hash credential from a script's hash.
 */
static cardano_credential_t*
script_credential(cardano_plutus_v3_script_t* script)
{
  cardano_blake2b_hash_t* hash       = cardano_plutus_v3_script_get_hash(script);
  cardano_credential_t*   credential = nullptr;

  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&hash);

  return credential;
}

/**
 * \brief Builds a V3 transaction with a single redeemer of \p tag (index 0) and one
 *        resolvable key-hash input, leaving the purpose-specific body collection to
 *        the caller. The script is placed in the witness set.
 */
static cardano_transaction_t*
build_gov_tx(cardano_plutus_v3_script_t* script, cardano_redeemer_tag_t tag, cardano_utxo_list_t** out_utxos)
{
  cardano_address_t*           addr  = key_address(0x01U);
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
  EXPECT_EQ(cardano_redeemer_new(tag, 0U, redeemer_data, zero_units, &redeemer), CARDANO_SUCCESS);

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

  return tx;
}

/**
 * \brief Evaluates \p tx and asserts the single redeemer succeeded with non-zero ex-units.
 */
static void
expect_single_redeemer_ok(cardano_transaction_t* tx, cardano_utxo_list_t* utxos, uint64_t protocol_major = 10U)
{
  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, protocol_major, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(result), 1U);

  cardano_redeemer_t* out = nullptr;
  EXPECT_EQ(cardano_redeemer_list_get(result, 0U, &out), CARDANO_SUCCESS);

  cardano_ex_units_t* units = cardano_redeemer_get_ex_units(out);
  EXPECT_GT(cardano_ex_units_get_cpu_steps(units), 0U);
  EXPECT_GT(cardano_ex_units_get_memory(units), 0U);

  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&out);
  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);
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

TEST(cardano_tx_evaluator_native, evaluatesAScriptWitnessedCertificate)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_CERTIFYING, &utxos);

  cardano_credential_t*                credential = script_credential(script);
  cardano_stake_deregistration_cert_t* cert       = nullptr;
  EXPECT_EQ(cardano_stake_deregistration_cert_new(credential, &cert), CARDANO_SUCCESS);

  cardano_certificate_t* certificate = nullptr;
  EXPECT_EQ(cardano_certificate_new_stake_deregistration(cert, &certificate), CARDANO_SUCCESS);

  cardano_certificate_set_t* certificates = nullptr;
  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  expect_single_redeemer_ok(tx, utxos);

  cardano_certificate_set_unref(&certificates);
  cardano_certificate_unref(&certificate);
  cardano_stake_deregistration_cert_unref(&cert);
  cardano_credential_unref(&credential);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAScriptWitnessedWithdrawal)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_REWARD, &utxos);

  cardano_credential_t*     credential = script_credential(script);
  cardano_reward_address_t* reward     = nullptr;
  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &reward), CARDANO_SUCCESS);

  cardano_withdrawal_map_t* withdrawals = nullptr;
  EXPECT_EQ(cardano_withdrawal_map_new(&withdrawals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_insert(withdrawals, reward, 0U), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_withdrawals(body, withdrawals), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  expect_single_redeemer_ok(tx, utxos);

  cardano_withdrawal_map_unref(&withdrawals);
  cardano_reward_address_unref(&reward);
  cardano_credential_unref(&credential);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAScriptWitnessedVote)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_VOTING, &utxos);

  cardano_credential_t* credential = script_credential(script);
  cardano_voter_t*      voter      = nullptr;
  EXPECT_EQ(cardano_voter_new(CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH, credential, &voter), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* action_tx = nullptr;
  byte_t                  id_bytes[32];
  memset(id_bytes, 0x09U, sizeof(id_bytes));
  EXPECT_EQ(cardano_blake2b_hash_from_bytes(id_bytes, sizeof(id_bytes), &action_tx), CARDANO_SUCCESS);

  cardano_governance_action_id_t* action_id = nullptr;
  EXPECT_EQ(cardano_governance_action_id_new(action_tx, 0U, &action_id), CARDANO_SUCCESS);

  cardano_voting_procedure_t* procedure = nullptr;
  EXPECT_EQ(cardano_voting_procedure_new(CARDANO_VOTE_YES, nullptr, &procedure), CARDANO_SUCCESS);

  cardano_voting_procedures_t* procedures = nullptr;
  EXPECT_EQ(cardano_voting_procedures_new(&procedures), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedures_insert(procedures, voter, action_id, procedure), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_voting_procedures(body, procedures), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  expect_single_redeemer_ok(tx, utxos);

  cardano_voting_procedures_unref(&procedures);
  cardano_voting_procedure_unref(&procedure);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&action_tx);
  cardano_voter_unref(&voter);
  cardano_credential_unref(&credential);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAScriptWitnessedProposal)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_PROPOSING, &utxos);

  cardano_blake2b_hash_t* policy_hash = cardano_plutus_v3_script_get_hash(script);

  cardano_protocol_param_update_t* update = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_new(&update), CARDANO_SUCCESS);

  cardano_parameter_change_action_t* action = nullptr;
  EXPECT_EQ(cardano_parameter_change_action_new(update, nullptr, policy_hash, &action), CARDANO_SUCCESS);

  cardano_credential_t*   key_cred = nullptr;
  cardano_blake2b_hash_t* key_hash = nullptr;
  byte_t                  kb[28];
  memset(kb, 0x0AU, sizeof(kb));
  EXPECT_EQ(cardano_blake2b_hash_from_bytes(kb, sizeof(kb), &key_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_new(key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &key_cred), CARDANO_SUCCESS);

  cardano_reward_address_t* deposit_return = nullptr;
  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, key_cred, &deposit_return), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* anchor_hash = nullptr;
  byte_t                  ab[32];
  memset(ab, 0x0BU, sizeof(ab));
  EXPECT_EQ(cardano_blake2b_hash_from_bytes(ab, sizeof(ab), &anchor_hash), CARDANO_SUCCESS);

  cardano_anchor_t* anchor = nullptr;
  EXPECT_EQ(cardano_anchor_new("https://example.com", strlen("https://example.com"), anchor_hash, &anchor), CARDANO_SUCCESS);

  cardano_proposal_procedure_t* proposal = nullptr;
  EXPECT_EQ(cardano_proposal_procedure_new_parameter_change_action(0U, deposit_return, anchor, action, &proposal), CARDANO_SUCCESS);

  cardano_proposal_procedure_set_t* proposals = nullptr;
  EXPECT_EQ(cardano_proposal_procedure_set_new(&proposals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_add(proposals, proposal), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_proposal_procedure(body, proposals), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  expect_single_redeemer_ok(tx, utxos);

  cardano_proposal_procedure_set_unref(&proposals);
  cardano_proposal_procedure_unref(&proposal);
  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&anchor_hash);
  cardano_reward_address_unref(&deposit_return);
  cardano_credential_unref(&key_cred);
  cardano_blake2b_hash_unref(&key_hash);
  cardano_parameter_change_action_unref(&action);
  cardano_protocol_param_update_unref(&update);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAV3SpendWithoutDatum)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, false, &utxos);

  expect_single_redeemer_ok(tx, utxos);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAScriptWitnessedUpdateDRepCertificate)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_CERTIFYING, &utxos);

  cardano_credential_t*       credential = script_credential(script);
  cardano_update_drep_cert_t* cert       = nullptr;
  EXPECT_EQ(cardano_update_drep_cert_new(credential, nullptr, &cert), CARDANO_SUCCESS);

  cardano_certificate_t* certificate = nullptr;
  EXPECT_EQ(cardano_certificate_new_update_drep(cert, &certificate), CARDANO_SUCCESS);

  cardano_certificate_set_t* certificates = nullptr;
  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  expect_single_redeemer_ok(tx, utxos);

  cardano_certificate_set_unref(&certificates);
  cardano_certificate_unref(&certificate);
  cardano_update_drep_cert_unref(&cert);
  cardano_credential_unref(&credential);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAScriptWitnessedCommitteeResignCertificate)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_CERTIFYING, &utxos);

  cardano_credential_t*                 credential = script_credential(script);
  cardano_resign_committee_cold_cert_t* cert       = nullptr;
  EXPECT_EQ(cardano_resign_committee_cold_cert_new(credential, nullptr, &cert), CARDANO_SUCCESS);

  cardano_certificate_t* certificate = nullptr;
  EXPECT_EQ(cardano_certificate_new_resign_committee_cold(cert, &certificate), CARDANO_SUCCESS);

  cardano_certificate_set_t* certificates = nullptr;
  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  expect_single_redeemer_ok(tx, utxos);

  cardano_certificate_set_unref(&certificates);
  cardano_certificate_unref(&certificate);
  cardano_resign_committee_cold_cert_unref(&cert);
  cardano_credential_unref(&credential);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesACommitteeScriptVote)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_VOTING, &utxos);

  cardano_credential_t* credential = script_credential(script);
  cardano_voter_t*      voter      = nullptr;
  EXPECT_EQ(cardano_voter_new(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH, credential, &voter), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* action_tx = nullptr;
  byte_t                  id_bytes[32];
  memset(id_bytes, 0x09U, sizeof(id_bytes));
  EXPECT_EQ(cardano_blake2b_hash_from_bytes(id_bytes, sizeof(id_bytes), &action_tx), CARDANO_SUCCESS);

  cardano_governance_action_id_t* action_id = nullptr;
  EXPECT_EQ(cardano_governance_action_id_new(action_tx, 0U, &action_id), CARDANO_SUCCESS);

  cardano_voting_procedure_t* procedure = nullptr;
  EXPECT_EQ(cardano_voting_procedure_new(CARDANO_VOTE_NO, nullptr, &procedure), CARDANO_SUCCESS);

  cardano_voting_procedures_t* procedures = nullptr;
  EXPECT_EQ(cardano_voting_procedures_new(&procedures), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedures_insert(procedures, voter, action_id, procedure), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_voting_procedures(body, procedures), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  expect_single_redeemer_ok(tx, utxos);

  cardano_voting_procedures_unref(&procedures);
  cardano_voting_procedure_unref(&procedure);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&action_tx);
  cardano_voter_unref(&voter);
  cardano_credential_unref(&credential);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, rejectsAnOutOfRangeCertifyingRedeemerIndex)
{
  // The redeemer points at certificate index 0, but the body carries no
  // certificates, so resolution must fail rather than evaluate.
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_CERTIFYING, &utxos);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_NE(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);

  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAPlutusV1SpendWithDatumHash)
{
  cardano_plutus_v1_script_t* script = build_v1_script(3U);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v1_spend_tx(script, &utxos);

  expect_single_redeemer_ok(tx, utxos);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v1_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAPlutusV2Spend)
{
  cardano_plutus_v2_script_t* script = build_v2_script(3U);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v2_spend_tx(script, &utxos);

  expect_single_redeemer_ok(tx, utxos);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, evaluatesAScriptThatRunsExpModInteger)
{
  // expModInteger is a PlutusV3 batch-6 builtin available from protocol 11.
  cardano_plutus_v3_script_t* script = build_expmod_v3_script();
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_single_redeemer_ok(tx, utxos, 11U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, rejectsExpModIntegerBeforeItsProtocolVersion)
{
  // The same script under protocol 10 must fail: expModInteger is not yet
  // available, matching the ledger's builtinsIntroducedIn schedule.
  cardano_plutus_v3_script_t* script = build_expmod_v3_script();
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);

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
