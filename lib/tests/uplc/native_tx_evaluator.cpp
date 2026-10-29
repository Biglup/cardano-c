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
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/certificate.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/certs/resign_committee_cold_cert.h>
#include <cardano/certs/stake_deregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/common/anchor.h>
#include <cardano/common/bigint.h>
#include <cardano/common/credential.h>
#include <cardano/common/datum.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/protocol_params/protocol_param_update.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v4_script.h>
#include <cardano/scripts/script.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
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
#include <cardano/witness_set/plutus_v4_script_set.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/syntax/text_parser.h"
#include "../allocators_helpers.h"

#include <allocators.h>
#include <string>
#include <vector>

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
build_v3_script(bool succeeds, uint32_t version_minor = 0U)
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
  program.version_minor = version_minor;
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
build_succeeds_program_cbor(size_t num_args, uint32_t version_minor)
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
  program.version_minor = version_minor;
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
  cardano_buffer_t*           cbor   = build_succeeds_program_cbor(num_args, 0U);
  cardano_plutus_v2_script_t* script = nullptr;

  EXPECT_EQ(cardano_plutus_v2_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_buffer_unref(&cbor);

  return script;
}

/**
 * \brief Builds a Plutus V1 always-succeeds script accepting \p num_args arguments,
 *        with program version 1.\p version_minor.0.
 */
static cardano_plutus_v1_script_t*
build_v1_script(size_t num_args, uint32_t version_minor = 0U)
{
  cardano_buffer_t*           cbor   = build_succeeds_program_cbor(num_args, version_minor);
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
 * \brief Wraps \p term as '[(lam r (con unit ())) term]', so the term is still
 *        evaluated (and costed) but the script returns unit, as V3 and V4 require.
 */
static cardano_uplc_term_t*
discard_to_unit(cardano_uplc_arena_t* arena, cardano_uplc_term_t* term)
{
  cardano_uplc_constant_t* unit      = nullptr;
  cardano_uplc_term_t*     unit_term = nullptr;
  cardano_uplc_term_t*     ignore    = nullptr;
  cardano_uplc_term_t*     applied   = nullptr;

  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &unit), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, unit, &unit_term), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, unit_term, &ignore), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, ignore, term, &applied), CARDANO_SUCCESS);

  return applied;
}

/**
 * \brief Builds a V3 script whose body evaluates expModInteger(4, 13, 497) and so
 *        exercises the builtin (and its chain pricing) during evaluation, then
 *        returns unit.
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
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, discard_to_unit(arena, term), &lambda), CARDANO_SUCCESS);

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
 * \brief Builds a V3 script whose body evaluates divideInteger(1, 2^64), a
 *        division whose dividend is one word and whose divisor is two words, then
 *        returns unit.
 */
static cardano_plutus_v3_script_t*
build_divide_above_diagonal_v3_script()
{
  static const char TWO_TO_THE_64[] = "18446744073709551616";

  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_term_t* builtin = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_builtin(arena, CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER, &builtin), CARDANO_SUCCESS);

  cardano_bigint_t* divisor_value = nullptr;
  EXPECT_EQ(cardano_bigint_from_string(TWO_TO_THE_64, sizeof(TWO_TO_THE_64) - 1U, 10, &divisor_value), CARDANO_SUCCESS);

  cardano_uplc_constant_t* dividend = nullptr;
  cardano_uplc_constant_t* divisor  = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_integer_small(arena, 1, &dividend), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_constant_new_integer(arena, divisor_value, &divisor), CARDANO_SUCCESS);
  cardano_bigint_unref(&divisor_value);

  cardano_uplc_term_t* dividend_term = nullptr;
  cardano_uplc_term_t* divisor_term  = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, dividend, &dividend_term), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, divisor, &divisor_term), CARDANO_SUCCESS);

  cardano_uplc_term_t* partial = nullptr;
  cardano_uplc_term_t* applied = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, builtin, dividend_term, &partial), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_apply(arena, partial, divisor_term, &applied), CARDANO_SUCCESS);

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, discard_to_unit(arena, applied), &lambda), CARDANO_SUCCESS);

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
 * \brief Builds a V3 script of program version 1.\p version_minor.0 whose body
 *        cases on the Data constant (Constr 0 []) and returns unit from its only
 *        branch, so it succeeds only where casing on Data is available.
 */
static cardano_plutus_v3_script_t*
build_case_on_data_v3_script(uint32_t version_minor)
{
  static const char SOURCE[] =
    "(program 1.1.0 (lam ctx (case (con data (Constr 0 [])) (lam fields (con unit ())))))";

  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  const cardano_uplc_program_t* parsed = nullptr;
  size_t                        offset = 0U;
  EXPECT_EQ(cardano_uplc_parse_program(arena, SOURCE, sizeof(SOURCE) - 1U, &parsed, &offset), CARDANO_SUCCESS);

  cardano_uplc_program_t program = *parsed;
  program.version_minor          = version_minor;

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(&program, &cbor), CARDANO_SUCCESS);

  cardano_plutus_v3_script_t* script = nullptr;
  EXPECT_EQ(cardano_plutus_v3_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_buffer_unref(&cbor);
  cardano_uplc_arena_free(&arena);

  return script;
}

/**
 * \brief Where the free variable of a scope check test script sits.
 */
enum class FreeVariableSite
{
  DELAYED_ARGUMENT,
  CONSTR_FIELD,
  CASE_BRANCH
};

/**
 * \brief Builds a PlutusV3 1.1.0 script that succeeds when run but holds a free
 *        variable the machine never reaches.
 *
 * For \c DELAYED_ARGUMENT and \c CONSTR_FIELD the body applies a lambda that
 * ignores its argument to a delayed term holding the free variable, directly or as
 * the field of a \c constr. For \c CASE_BRANCH the body is a \c case on
 * \c (constr 0) whose taken branch is unit and whose other branch is the free
 * variable.
 */
static cardano_plutus_v3_script_t*
build_free_variable_v3_script(FreeVariableSite site)
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_constant_t* unit      = nullptr;
  cardano_uplc_term_t*     unit_term = nullptr;
  cardano_uplc_term_t*     free_var  = nullptr;
  EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &unit), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_constant(arena, unit, &unit_term), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_uplc_term_new_var(arena, 5U, &free_var), CARDANO_SUCCESS);

  const cardano_uplc_term_t** slots =
    static_cast<const cardano_uplc_term_t**>(cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_term_t*) * 2U, 0U));

  cardano_uplc_term_t* body = nullptr;

  if (site == FreeVariableSite::CASE_BRANCH)
  {
    cardano_uplc_term_t* scrutinee = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_constr(arena, 0U, nullptr, 0U, &scrutinee), CARDANO_SUCCESS);

    slots[0] = unit_term;
    slots[1] = free_var;
    EXPECT_EQ(cardano_uplc_term_new_case(arena, scrutinee, slots, 2U, &body), CARDANO_SUCCESS);
  }
  else
  {
    cardano_uplc_term_t* payload = free_var;

    if (site == FreeVariableSite::CONSTR_FIELD)
    {
      slots[0] = free_var;
      EXPECT_EQ(cardano_uplc_term_new_constr(arena, 0U, slots, 1U, &payload), CARDANO_SUCCESS);
    }

    cardano_uplc_term_t* delayed = nullptr;
    cardano_uplc_term_t* ignore  = nullptr;
    EXPECT_EQ(cardano_uplc_term_new_delay(arena, payload, &delayed), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_uplc_term_new_lambda(arena, unit_term, &ignore), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_uplc_term_new_apply(arena, ignore, delayed, &body), CARDANO_SUCCESS);
  }

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, body, &lambda), CARDANO_SUCCESS);

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

/**
 * \brief Asserts that \p list holds one redeemer of \p tag at \p index that
 *        consumed exactly \p cpu steps and \p mem memory units.
 */
static void
expect_single_evaluated(
  cardano_redeemer_list_t* list,
  cardano_redeemer_tag_t   tag,
  uint64_t                 index,
  uint64_t                 cpu,
  uint64_t                 mem)
{
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_redeemer_list_get_length(list), 1U);

  cardano_redeemer_t* redeemer = nullptr;
  EXPECT_EQ(cardano_redeemer_list_get(list, 0U, &redeemer), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_get_tag(redeemer), tag);
  EXPECT_EQ(cardano_redeemer_get_index(redeemer), index);

  cardano_ex_units_t* units = cardano_redeemer_get_ex_units(redeemer);
  EXPECT_EQ(cardano_ex_units_get_cpu_steps(units), cpu);
  EXPECT_EQ(cardano_ex_units_get_memory(units), mem);

  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&redeemer);
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

/**
 * \brief Evaluates the single-redeemer \p tx under \p protocol_major and expects
 *        it to be rejected as a script that cannot be deserialised.
 */
static void
expect_not_deserialisable(cardano_transaction_t* tx, cardano_utxo_list_t* utxos, uint64_t protocol_major)
{
  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, protocol_major, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_ERROR_DECODING);
  EXPECT_EQ(result, nullptr);
  EXPECT_STREQ(cardano_tx_evaluator_get_last_error(evaluator), "a redeemer's script cannot be deserialised at this protocol version");

  cardano_tx_evaluator_unref(&evaluator);
}

/**
 * \brief Evaluates the single-redeemer \p tx under \p protocol_major and expects
 *        it to be rejected as a phase-2 script failure.
 */
static void
expect_script_failure(cardano_transaction_t* tx, cardano_utxo_list_t* utxos, uint64_t protocol_major)
{
  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, protocol_major, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);
  EXPECT_EQ(result, nullptr);
  EXPECT_STREQ(cardano_tx_evaluator_get_last_error(evaluator), "phase-2 validation failed for a redeemer");

  cardano_tx_evaluator_unref(&evaluator);
}

TEST(cardano_tx_evaluator_native, rejectsExpModIntegerBeforeItsProtocolVersion)
{
  cardano_plutus_v3_script_t* script = build_expmod_v3_script();
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_not_deserialisable(tx, utxos, 10U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, rejectsALanguageBeforeItsProtocolVersion)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_not_deserialisable(tx, utxos, 8U);
  expect_single_redeemer_ok(tx, utxos, 9U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, gatesPlutusCoreOneOneZeroForV1ByProtocolVersion)
{
  cardano_plutus_v1_script_t* script = build_v1_script(3U, 1U);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v1_spend_tx(script, &utxos);

  expect_script_failure(tx, utxos, 10U);
  expect_single_redeemer_ok(tx, utxos, 11U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v1_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, gatesPlutusCoreOneTwoZeroForV3ByProtocolVersion)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true, 2U);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_script_failure(tx, utxos, 11U);
  expect_single_redeemer_ok(tx, utxos, 12U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, casesOnDataFromPlutusCoreOneTwoZeroAtDijkstra)
{
  cardano_plutus_v3_script_t* script = build_case_on_data_v3_script(2U);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_script_failure(tx, utxos, 11U);
  expect_single_redeemer_ok(tx, utxos, 12U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, rejectsCaseOnDataBelowPlutusCoreOneTwoZero)
{
  cardano_plutus_v3_script_t* script = build_case_on_data_v3_script(1U);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_script_failure(tx, utxos, 11U);
  expect_script_failure(tx, utxos, 12U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

/**
 * \brief Evaluates the single-redeemer \p tx under \p protocol_major and returns
 *        the cpu steps the redeemer spent.
 */
static uint64_t
single_redeemer_cpu(cardano_transaction_t* tx, cardano_utxo_list_t* utxos, uint64_t protocol_major)
{
  uint64_t                cpu       = 0U;
  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, protocol_major, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);

  cardano_redeemer_t* out = nullptr;
  EXPECT_EQ(cardano_redeemer_list_get(result, 0U, &out), CARDANO_SUCCESS);

  cardano_ex_units_t* units = cardano_redeemer_get_ex_units(out);
  cpu                       = cardano_ex_units_get_cpu_steps(units);

  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&out);
  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);

  return cpu;
}

/**
 * \brief Cost model C charges the constant 85848 above the diagonal, while cost
 *        model E (V3 from protocol 11) evaluates the quadratic at (2, 1):
 *        123203 + 1716*2 + 7305*1 + 57*4 + 960*2 - 900*1 = 135188.
 */
TEST(cardano_tx_evaluator_native, costsDivisionWithTheShapesOfTheProtocolVersion)
{
  cardano_plutus_v3_script_t* script = build_divide_above_diagonal_v3_script();
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  const uint64_t cpu_c = single_redeemer_cpu(tx, utxos, 10U);
  const uint64_t cpu_e = single_redeemer_cpu(tx, utxos, 11U);

  EXPECT_EQ(cpu_e - cpu_c, 135188U - 85848U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

/**
 * \brief The free variable is never evaluated, but the ledger's scope check sees it
 *        at every protocol version since it is not under a constr or case.
 */
TEST(cardano_tx_evaluator_native, rejectsAFreeVariableAtEveryProtocolVersion)
{
  cardano_plutus_v3_script_t* script = build_free_variable_v3_script(FreeVariableSite::DELAYED_ARGUMENT);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_script_failure(tx, utxos, 11U);
  expect_script_failure(tx, utxos, 12U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, checksTheScopeOfConstrFieldsFromDijkstra)
{
  cardano_plutus_v3_script_t* script = build_free_variable_v3_script(FreeVariableSite::CONSTR_FIELD);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_single_redeemer_ok(tx, utxos, 11U);
  expect_script_failure(tx, utxos, 12U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, checksTheScopeOfCaseBranchesFromDijkstra)
{
  cardano_plutus_v3_script_t* script = build_free_variable_v3_script(FreeVariableSite::CASE_BRANCH);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);

  expect_single_redeemer_ok(tx, utxos, 11U);
  expect_script_failure(tx, utxos, 12U);

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

/**
 * \brief Encodes the textual UPLC program \p source into the CBOR-wrapped flat
 *        bytes the ledger stores in a witness set.
 */
static cardano_buffer_t*
program_cbor_from_source(const char* source, size_t size)
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  const cardano_uplc_program_t* program = nullptr;
  size_t                        offset  = 0U;
  EXPECT_EQ(cardano_uplc_parse_program(arena, source, size, &program, &offset), CARDANO_SUCCESS);

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(program, &cbor), CARDANO_SUCCESS);

  cardano_uplc_arena_free(&arena);

  return cbor;
}

TEST(cardano_tx_evaluator_native, rejectsAV3ScriptThatDoesNotReturnUnit)
{
  static const char SOURCE[] = "(program 1.0.0 (lam ctx (con integer 42)))";

  cardano_buffer_t*           cbor   = program_cbor_from_source(SOURCE, sizeof(SOURCE) - 1U);
  cardano_plutus_v3_script_t* script = nullptr;
  EXPECT_EQ(cardano_plutus_v3_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_utxo_list_t*   utxos = nullptr;
  cardano_transaction_t* tx    = build_spend_tx(script, true, &utxos);

  expect_script_failure(tx, utxos, 10U);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
  cardano_buffer_unref(&cbor);
}

TEST(cardano_tx_evaluator_native, acceptsAV2ScriptThatDoesNotReturnUnit)
{
  static const char SOURCE[] = "(program 1.0.0 (lam datum (lam redeemer (lam ctx (con integer 42)))))";

  cardano_buffer_t*           cbor   = program_cbor_from_source(SOURCE, sizeof(SOURCE) - 1U);
  cardano_plutus_v2_script_t* script = nullptr;
  EXPECT_EQ(cardano_plutus_v2_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_utxo_list_t*   utxos = nullptr;
  cardano_transaction_t* tx    = build_v2_spend_tx(script, &utxos);

  expect_single_redeemer_ok(tx, utxos);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v2_script_unref(&script);
  cardano_buffer_unref(&cbor);
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

/* PLUTUS V4, GUARDS AND SUB-TRANSACTIONS ************************************/

static const uint64_t kDijkstra = 12U;

/**
 * \brief The result a test V4 script reduces to.
 */
typedef enum
{
  V4_RETURNS_UNIT,
  V4_FAILS,
  V4_RETURNS_INTEGER
} v4_behaviour_t;

/**
 * \brief Builds a one-argument Plutus V4 script (program version 1.1.0).
 *
 * The script ignores its ScriptContext and reduces to unit, to an error, or to
 * an integer, depending on \p behaviour. \p seed is the integer value, so two
 * integer-returning scripts with different seeds have different hashes.
 */
static cardano_plutus_v4_script_t*
build_v4_script(v4_behaviour_t behaviour, int64_t seed = 0)
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  cardano_uplc_term_t*     body     = nullptr;
  cardano_uplc_constant_t* constant = nullptr;

  switch (behaviour)
  {
    case V4_RETURNS_UNIT:
      EXPECT_EQ(cardano_uplc_constant_new_unit(arena, &constant), CARDANO_SUCCESS);
      EXPECT_EQ(cardano_uplc_term_new_constant(arena, constant, &body), CARDANO_SUCCESS);
      break;
    case V4_FAILS:
      EXPECT_EQ(cardano_uplc_term_new_error(arena, &body), CARDANO_SUCCESS);
      break;
    case V4_RETURNS_INTEGER:
      EXPECT_EQ(cardano_uplc_constant_new_integer_small(arena, seed, &constant), CARDANO_SUCCESS);
      EXPECT_EQ(cardano_uplc_term_new_constant(arena, constant, &body), CARDANO_SUCCESS);
      break;
  }

  cardano_uplc_term_t* lambda = nullptr;
  EXPECT_EQ(cardano_uplc_term_new_lambda(arena, body, &lambda), CARDANO_SUCCESS);

  cardano_uplc_program_t program;
  program.version_major = 1U;
  program.version_minor = 1U;
  program.version_patch = 0U;
  program.term          = lambda;

  cardano_buffer_t* cbor = nullptr;
  EXPECT_EQ(cardano_uplc_program_to_cbor(&program, &cbor), CARDANO_SUCCESS);

  cardano_plutus_v4_script_t* script = nullptr;
  EXPECT_EQ(cardano_plutus_v4_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_buffer_unref(&cbor);
  cardano_uplc_arena_free(&arena);

  return script;
}

/**
 * \brief Builds a Plutus V4 script whose bytes are not a valid program.
 */
static cardano_plutus_v4_script_t*
build_undecodable_v4_script()
{
  const byte_t                bytes[] = { 0x43U, 0xFFU, 0xFFU, 0xFFU };
  cardano_plutus_v4_script_t* script  = nullptr;

  EXPECT_EQ(cardano_plutus_v4_script_new_bytes(bytes, sizeof(bytes), &script), CARDANO_SUCCESS);

  return script;
}

/**
 * \brief Builds a script-hash credential from a V4 script's hash.
 */
static cardano_credential_t*
v4_credential(cardano_plutus_v4_script_t* script)
{
  cardano_blake2b_hash_t* hash       = cardano_plutus_v4_script_get_hash(script);
  cardano_credential_t*   credential = nullptr;

  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&hash);

  return credential;
}

/**
 * \brief Builds a key-hash credential from a seed byte.
 *
 * The library withdrawal and voter maps list key credentials before script
 * credentials of the same role, while the ledger lists script credentials
 * first, so pairing this credential with a script credential tells the two
 * orders apart.
 */
static cardano_credential_t*
key_credential(uint8_t seed)
{
  byte_t bytes[28];
  memset(bytes, seed, sizeof(bytes));

  cardano_blake2b_hash_t* hash = nullptr;
  EXPECT_EQ(cardano_blake2b_hash_from_bytes(bytes, sizeof(bytes), &hash), CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;
  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&hash);

  return credential;
}

TEST(cardano_tx_evaluator_native, resolvesARewardRedeemerInLedgerAccountOrder)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_REWARD, &utxos);

  cardano_credential_t*     script_cred = script_credential(script);
  cardano_credential_t*     key_cred    = key_credential(0x01U);
  cardano_reward_address_t* script_acct = nullptr;
  cardano_reward_address_t* key_acct    = nullptr;
  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, script_cred, &script_acct), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, key_cred, &key_acct), CARDANO_SUCCESS);

  cardano_withdrawal_map_t* withdrawals = nullptr;
  EXPECT_EQ(cardano_withdrawal_map_new(&withdrawals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_insert(withdrawals, script_acct, 0U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_insert(withdrawals, key_acct, 0U), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_withdrawals(body, withdrawals), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);

  expect_single_evaluated(result, CARDANO_REDEEMER_TAG_REWARD, 0U, 64100U, 500U);

  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);

  cardano_withdrawal_map_unref(&withdrawals);
  cardano_reward_address_unref(&key_acct);
  cardano_reward_address_unref(&script_acct);
  cardano_credential_unref(&key_cred);
  cardano_credential_unref(&script_cred);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, resolvesAVotingRedeemerInLedgerVoterOrder)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_gov_tx(script, CARDANO_REDEEMER_TAG_VOTING, &utxos);

  cardano_credential_t* script_cred = script_credential(script);
  cardano_credential_t* key_cred    = key_credential(0x01U);
  cardano_voter_t*      script_drep = nullptr;
  cardano_voter_t*      key_drep    = nullptr;
  EXPECT_EQ(cardano_voter_new(CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH, script_cred, &script_drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voter_new(CARDANO_VOTER_TYPE_DREP_KEY_HASH, key_cred, &key_drep), CARDANO_SUCCESS);

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
  EXPECT_EQ(cardano_voting_procedures_insert(procedures, script_drep, action_id, procedure), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedures_insert(procedures, key_drep, action_id, procedure), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  EXPECT_EQ(cardano_transaction_body_set_voting_procedures(body, procedures), CARDANO_SUCCESS);
  cardano_transaction_body_unref(&body);

  cardano_tx_evaluator_t* evaluator = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, 10U, &evaluator), CARDANO_SUCCESS);

  cardano_redeemer_list_t* result = nullptr;
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);

  expect_single_evaluated(result, CARDANO_REDEEMER_TAG_VOTING, 0U, 64100U, 500U);

  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);

  cardano_voting_procedures_unref(&procedures);
  cardano_voting_procedure_unref(&procedure);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&action_tx);
  cardano_voter_unref(&key_drep);
  cardano_voter_unref(&script_drep);
  cardano_credential_unref(&key_cred);
  cardano_credential_unref(&script_cred);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v3_script_unref(&script);
}

/**
 * \brief Builds a guard set holding \p guards in insertion order.
 */
static cardano_guard_set_t*
guard_set_of(const std::vector<cardano_credential_t*>& guards)
{
  cardano_guard_set_t* set = nullptr;
  EXPECT_EQ(cardano_guard_set_new(&set), CARDANO_SUCCESS);

  for (cardano_credential_t* guard: guards)
  {
    EXPECT_EQ(cardano_guard_set_add(set, guard), CARDANO_SUCCESS);
  }

  return set;
}

/**
 * \brief Builds an input set with the single input of \p seed.
 */
static cardano_transaction_input_set_t*
input_set_of(uint8_t seed)
{
  cardano_transaction_input_t*     input = make_input(seed, 0U);
  cardano_transaction_input_set_t* set   = nullptr;

  EXPECT_EQ(cardano_transaction_input_set_new(&set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(set, input), CARDANO_SUCCESS);

  cardano_transaction_input_unref(&input);

  return set;
}

/**
 * \brief Builds an output list paying 1 ada to the key address of \p seed.
 */
static cardano_transaction_output_list_t*
key_outputs(uint8_t seed)
{
  cardano_address_t*                 address = key_address(seed);
  cardano_transaction_output_t*      output  = nullptr;
  cardano_transaction_output_list_t* outputs = nullptr;

  EXPECT_EQ(cardano_transaction_output_new(address, 1000000U, &output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_add(outputs, output), CARDANO_SUCCESS);

  cardano_transaction_output_unref(&output);
  cardano_address_unref(&address);

  return outputs;
}

/**
 * \brief Adds the UTxO of the input of \p seed, held at \p address, to \p utxos.
 */
static void
add_utxo(cardano_utxo_list_t* utxos, uint8_t seed, cardano_address_t* address, cardano_script_t* script_ref = nullptr)
{
  cardano_transaction_input_t*  input  = make_input(seed, 0U);
  cardano_transaction_output_t* output = nullptr;
  cardano_utxo_t*               utxo   = nullptr;

  EXPECT_EQ(cardano_transaction_output_new(address, 5000000U, &output), CARDANO_SUCCESS);

  if (script_ref != nullptr)
  {
    EXPECT_EQ(cardano_transaction_output_set_script_ref(output, script_ref), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_transaction_input_unref(&input);
}

/**
 * \brief Adds the UTxO of the input of \p seed, held at a key address, to \p utxos.
 */
static void
add_key_utxo(cardano_utxo_list_t* utxos, uint8_t seed)
{
  cardano_address_t* address = key_address(seed);

  add_utxo(utxos, seed, address);

  cardano_address_unref(&address);
}

/**
 * \brief Builds a witness set carrying \p script (when not NULL) and, when
 *        \p with_redeemer is set, one redeemer of \p tag at \p index.
 */
static cardano_witness_set_t*
v4_witness_set(cardano_plutus_v4_script_t* script, bool with_redeemer, cardano_redeemer_tag_t tag = CARDANO_REDEEMER_TAG_GUARDING, uint64_t index = 0U)
{
  cardano_witness_set_t* witness_set = nullptr;
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);

  if (script != nullptr)
  {
    cardano_plutus_v4_script_set_t* scripts = nullptr;
    EXPECT_EQ(cardano_plutus_v4_script_set_new(&scripts), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v4_script_set_add(scripts, script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v4_scripts(witness_set, scripts), CARDANO_SUCCESS);
    cardano_plutus_v4_script_set_unref(&scripts);
  }

  if (with_redeemer)
  {
    cardano_plutus_data_t*   data      = unit_data();
    cardano_ex_units_t*      units     = nullptr;
    cardano_redeemer_t*      redeemer  = nullptr;
    cardano_redeemer_list_t* redeemers = nullptr;

    EXPECT_EQ(cardano_ex_units_new(0U, 0U, &units), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_redeemer_new(tag, index, data, units, &redeemer), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_redeemer_list_new(&redeemers), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_redeemer_list_add(redeemers, redeemer), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);

    cardano_redeemer_list_unref(&redeemers);
    cardano_redeemer_unref(&redeemer);
    cardano_ex_units_unref(&units);
    cardano_plutus_data_unref(&data);
  }

  return witness_set;
}

/**
 * \brief Builds a sub-transaction spending the input of \p input_seed, guarded by
 *        \p guards when not empty, with \p witness_set.
 */
static cardano_sub_transaction_t*
build_sub_tx(uint8_t input_seed, const std::vector<cardano_credential_t*>& guards, cardano_witness_set_t* witness_set)
{
  cardano_transaction_input_set_t*   inputs  = input_set_of(input_seed);
  cardano_transaction_output_list_t* outputs = key_outputs(input_seed);
  cardano_sub_transaction_body_t*    body    = nullptr;
  cardano_sub_transaction_t*         sub_tx  = nullptr;

  EXPECT_EQ(cardano_sub_transaction_body_new(inputs, outputs, nullptr, &body), CARDANO_SUCCESS);

  if (!guards.empty())
  {
    cardano_guard_set_t* set = guard_set_of(guards);
    EXPECT_EQ(cardano_sub_transaction_body_set_guards(body, set), CARDANO_SUCCESS);
    cardano_guard_set_unref(&set);
  }

  EXPECT_EQ(cardano_sub_transaction_new(body, witness_set, nullptr, &sub_tx), CARDANO_SUCCESS);

  cardano_sub_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_input_set_unref(&inputs);

  return sub_tx;
}

/**
 * \brief Builds a top-level transaction spending the input of \p input_seed,
 *        guarded by \p guards when not empty, carrying \p sub_txs.
 */
static cardano_transaction_t*
build_top_tx(
  uint8_t                                        input_seed,
  const std::vector<cardano_credential_t*>&      guards,
  cardano_witness_set_t*                         witness_set,
  const std::vector<cardano_sub_transaction_t*>& sub_txs = {})
{
  cardano_transaction_input_set_t*   inputs  = input_set_of(input_seed);
  cardano_transaction_output_list_t* outputs = key_outputs(input_seed);
  cardano_transaction_body_t*        body    = nullptr;
  cardano_transaction_t*             tx      = nullptr;

  EXPECT_EQ(cardano_transaction_body_new(inputs, outputs, 200000U, nullptr, &body), CARDANO_SUCCESS);

  if (!guards.empty())
  {
    cardano_guard_set_t* set = guard_set_of(guards);
    EXPECT_EQ(cardano_transaction_body_set_guards(body, set), CARDANO_SUCCESS);
    cardano_guard_set_unref(&set);
  }

  if (!sub_txs.empty())
  {
    cardano_sub_transaction_set_t* set = nullptr;
    EXPECT_EQ(cardano_sub_transaction_set_new(&set), CARDANO_SUCCESS);

    for (cardano_sub_transaction_t* sub_tx: sub_txs)
    {
      EXPECT_EQ(cardano_sub_transaction_set_add(set, sub_tx), CARDANO_SUCCESS);
    }

    EXPECT_EQ(cardano_transaction_body_set_sub_transactions(body, set), CARDANO_SUCCESS);
    cardano_sub_transaction_set_unref(&set);
  }

  EXPECT_EQ(cardano_transaction_new(body, witness_set, nullptr, &tx), CARDANO_SUCCESS);

  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_input_set_unref(&inputs);

  return tx;
}

/**
 * \brief Evaluates \p tx for the top level (\p sub_index NULL) or for one
 *        sub-transaction, returning the evaluator result and its message.
 */
static cardano_error_t
evaluate_level(
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      utxos,
  const uint64_t*           sub_index,
  cardano_redeemer_list_t** out,
  std::string*              message  = nullptr,
  uint64_t                  protocol = kDijkstra)
{
  cardano_tx_evaluator_t* evaluator = nullptr;

  if (sub_index == nullptr)
  {
    EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, nullptr, protocol, &evaluator), CARDANO_SUCCESS);
  }
  else
  {
    EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(&kSlotConfig, nullptr, protocol, *sub_index, &evaluator), CARDANO_SUCCESS);
  }

  const cardano_error_t result = cardano_tx_evaluator_evaluate(evaluator, tx, utxos, out);

  if (message != nullptr)
  {
    *message = cardano_tx_evaluator_get_last_error(evaluator);
  }

  cardano_tx_evaluator_unref(&evaluator);

  return result;
}

/**
 * \brief CPU steps the always-succeeding V4 script consumes under the default cost model.
 */
static const uint64_t kV4ReturnsUnitCpu = 64100U;

/**
 * \brief Memory units the always-succeeding V4 script consumes under the default cost model.
 */
static const uint64_t kV4ReturnsUnitMem = 500U;

/**
 * \brief Builds a transaction spending one input held at the address of \p script,
 *        with a V4 spend redeemer. The script is in the witness set unless
 *        \p as_reference is set, in which case a reference input carries it.
 */
static cardano_transaction_t*
build_v4_spend_tx(cardano_plutus_v4_script_t* script, bool as_reference, cardano_utxo_list_t** out_utxos)
{
  cardano_blake2b_hash_t* hash    = cardano_plutus_v4_script_get_hash(script);
  cardano_address_t*      address = script_address(hash);
  cardano_witness_set_t*  ws      = v4_witness_set(as_reference ? nullptr : script, true, CARDANO_REDEEMER_TAG_SPEND, 0U);
  cardano_transaction_t*  tx      = build_top_tx(0x10U, {}, ws);

  EXPECT_EQ(cardano_utxo_list_new(out_utxos), CARDANO_SUCCESS);
  add_utxo(*out_utxos, 0x10U, address);

  if (as_reference)
  {
    cardano_script_t*  script_ref  = nullptr;
    cardano_address_t* ref_address = key_address(0x20U);

    EXPECT_EQ(cardano_script_new_plutus_v4(script, &script_ref), CARDANO_SUCCESS);
    add_utxo(*out_utxos, 0x20U, ref_address, script_ref);

    cardano_transaction_body_t*      body          = cardano_transaction_get_body(tx);
    cardano_transaction_input_set_t* reference_set = input_set_of(0x20U);

    EXPECT_EQ(cardano_transaction_body_set_reference_inputs(body, reference_set), CARDANO_SUCCESS);

    cardano_transaction_input_set_unref(&reference_set);
    cardano_transaction_body_unref(&body);
    cardano_address_unref(&ref_address);
    cardano_script_unref(&script_ref);
  }

  cardano_witness_set_unref(&ws);
  cardano_address_unref(&address);
  cardano_blake2b_hash_unref(&hash);

  return tx;
}

TEST(cardano_tx_evaluator_native_v4, evaluatesAV4SpendAtTheTopLevel)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_RETURNS_UNIT);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v4_spend_tx(script, false, &utxos);
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_SUCCESS);
  expect_single_evaluated(result, CARDANO_REDEEMER_TAG_SPEND, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  cardano_redeemer_list_unref(&result);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, evaluatesAV4SpendThroughAReferenceScript)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_RETURNS_UNIT);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v4_spend_tx(script, true, &utxos);
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_SUCCESS);
  expect_single_evaluated(result, CARDANO_REDEEMER_TAG_SPEND, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  cardano_redeemer_list_unref(&result);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAV4ScriptBeforeDijkstra)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_RETURNS_UNIT);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v4_spend_tx(script, false, &utxos);
  cardano_redeemer_list_t*    result = nullptr;
  std::string                 message;

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, &message, 11U), CARDANO_ERROR_DECODING);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(message, "a redeemer's script cannot be deserialised at this protocol version");

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAV4ScriptThatDoesNotReturnUnit)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_RETURNS_INTEGER, 7);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v4_spend_tx(script, false, &utxos);
  cardano_redeemer_list_t*    result = nullptr;
  std::string                 message;

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, &message), CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(message, "phase-2 validation failed for a redeemer");

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAFailingV4Script)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_FAILS);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_v4_spend_tx(script, false, &utxos);
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);
  EXPECT_EQ(result, nullptr);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, evaluatesAGuardingRedeemerCountingKeyGuards)
{
  cardano_plutus_v4_script_t* script       = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       key_guard    = key_credential(0x31U);
  cardano_credential_t*       script_guard = v4_credential(script);
  cardano_witness_set_t*      ws           = v4_witness_set(script, true, CARDANO_REDEEMER_TAG_GUARDING, 1U);
  cardano_transaction_t*      tx           = build_top_tx(0x10U, { key_guard, script_guard }, ws);
  cardano_utxo_list_t*        utxos        = nullptr;
  cardano_redeemer_list_t*    result       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_SUCCESS);
  expect_single_evaluated(result, CARDANO_REDEEMER_TAG_GUARDING, 1U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  cardano_redeemer_list_unref(&result);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&ws);
  cardano_credential_unref(&script_guard);
  cardano_credential_unref(&key_guard);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAGuardingRedeemerOnAKeyGuard)
{
  cardano_plutus_v4_script_t* script       = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       key_guard    = key_credential(0x31U);
  cardano_credential_t*       script_guard = v4_credential(script);
  cardano_witness_set_t*      ws           = v4_witness_set(script, true, CARDANO_REDEEMER_TAG_GUARDING, 0U);
  cardano_transaction_t*      tx           = build_top_tx(0x10U, { key_guard, script_guard }, ws);
  cardano_utxo_list_t*        utxos        = nullptr;
  cardano_redeemer_list_t*    result       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(result, nullptr);

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&ws);
  cardano_credential_unref(&script_guard);
  cardano_credential_unref(&key_guard);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAnOutOfRangeGuardingRedeemer)
{
  cardano_plutus_v4_script_t* script       = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       script_guard = v4_credential(script);
  cardano_witness_set_t*      guarded_ws   = v4_witness_set(script, true, CARDANO_REDEEMER_TAG_GUARDING, 1U);
  cardano_witness_set_t*      unguarded_ws = v4_witness_set(script, true, CARDANO_REDEEMER_TAG_GUARDING, 0U);
  cardano_transaction_t*      guarded      = build_top_tx(0x10U, { script_guard }, guarded_ws);
  cardano_transaction_t*      unguarded    = build_top_tx(0x10U, {}, unguarded_ws);
  cardano_utxo_list_t*        utxos        = nullptr;
  cardano_redeemer_list_t*    result       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);

  EXPECT_EQ(evaluate_level(guarded, utxos, nullptr, &result), CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(evaluate_level(unguarded, utxos, nullptr, &result), CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(result, nullptr);

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&unguarded);
  cardano_transaction_unref(&guarded);
  cardano_witness_set_unref(&unguarded_ws);
  cardano_witness_set_unref(&guarded_ws);
  cardano_credential_unref(&script_guard);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAV3ScriptInATransactionWithAScriptHashGuard)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_plutus_v4_script_t* guard  = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       cred   = v4_credential(guard);
  cardano_guard_set_t*        guards = guard_set_of({ cred });
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_transaction_t*      tx     = build_spend_tx(script, true, &utxos);
  cardano_transaction_body_t* body   = cardano_transaction_get_body(tx);
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(cardano_transaction_body_set_guards(body, guards), CARDANO_SUCCESS);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(result, nullptr);

  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_guard_set_unref(&guards);
  cardano_credential_unref(&cred);
  cardano_plutus_v4_script_unref(&guard);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, evaluatesASubTransactionGuardWithItsOwnLevel)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       guard  = v4_credential(script);
  cardano_witness_set_t*      sub_ws = v4_witness_set(script, true);
  cardano_sub_transaction_t*  sub_tx = build_sub_tx(0x11U, { guard }, sub_ws);
  cardano_witness_set_t*      top_ws = v4_witness_set(nullptr, false);
  cardano_transaction_t*      tx     = build_top_tx(0x10U, {}, top_ws, { sub_tx });
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    top    = nullptr;
  cardano_redeemer_list_t*    sub    = nullptr;
  const uint64_t              index  = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  add_key_utxo(utxos, 0x11U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &top), CARDANO_SUCCESS);
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(top), 0U);

  EXPECT_EQ(evaluate_level(tx, utxos, &index, &sub), CARDANO_SUCCESS);
  expect_single_evaluated(sub, CARDANO_REDEEMER_TAG_GUARDING, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  cardano_redeemer_list_unref(&sub);
  cardano_redeemer_list_unref(&top);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&top_ws);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_witness_set_unref(&sub_ws);
  cardano_credential_unref(&guard);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, mapsTheRedeemersOfEachSubTransaction)
{
  cardano_plutus_v4_script_t* first_script  = build_v4_script(V4_RETURNS_UNIT);
  cardano_plutus_v4_script_t* top_script    = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       first_guard   = v4_credential(first_script);
  cardano_witness_set_t*      first_ws      = v4_witness_set(first_script, true);
  cardano_witness_set_t*      second_ws     = v4_witness_set(nullptr, false);
  cardano_sub_transaction_t*  first         = build_sub_tx(0x11U, { first_guard }, first_ws);
  cardano_sub_transaction_t*  second        = build_sub_tx(0x12U, {}, second_ws);
  cardano_utxo_list_t*        utxos         = nullptr;
  cardano_transaction_t*      tx            = build_v4_spend_tx(top_script, false, &utxos);
  cardano_redeemer_list_t*    top           = nullptr;
  cardano_redeemer_list_t*    first_result  = nullptr;
  cardano_redeemer_list_t*    second_result = nullptr;
  const uint64_t              first_index   = 0U;
  const uint64_t              second_index  = 1U;

  cardano_transaction_body_t*    body = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* set  = nullptr;
  EXPECT_EQ(cardano_sub_transaction_set_new(&set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_set_add(set, first), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_set_add(set, second), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_sub_transactions(body, set), CARDANO_SUCCESS);
  cardano_sub_transaction_set_unref(&set);
  cardano_transaction_body_unref(&body);

  add_key_utxo(utxos, 0x11U);
  add_key_utxo(utxos, 0x12U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &top), CARDANO_SUCCESS);
  expect_single_evaluated(top, CARDANO_REDEEMER_TAG_SPEND, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  EXPECT_EQ(evaluate_level(tx, utxos, &first_index, &first_result), CARDANO_SUCCESS);
  expect_single_evaluated(first_result, CARDANO_REDEEMER_TAG_GUARDING, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  EXPECT_EQ(evaluate_level(tx, utxos, &second_index, &second_result), CARDANO_SUCCESS);
  ASSERT_NE(second_result, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(second_result), 0U);

  cardano_redeemer_list_unref(&second_result);
  cardano_redeemer_list_unref(&first_result);
  cardano_redeemer_list_unref(&top);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_sub_transaction_unref(&second);
  cardano_sub_transaction_unref(&first);
  cardano_witness_set_unref(&second_ws);
  cardano_witness_set_unref(&first_ws);
  cardano_credential_unref(&first_guard);
  cardano_plutus_v4_script_unref(&top_script);
  cardano_plutus_v4_script_unref(&first_script);
}

TEST(cardano_tx_evaluator_native_v4, findsASubTransactionScriptInTheTopLevelWitnessSet)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       guard  = v4_credential(script);
  cardano_witness_set_t*      sub_ws = v4_witness_set(nullptr, true);
  cardano_sub_transaction_t*  sub_tx = build_sub_tx(0x11U, { guard }, sub_ws);
  cardano_witness_set_t*      top_ws = v4_witness_set(script, false);
  cardano_transaction_t*      tx     = build_top_tx(0x10U, {}, top_ws, { sub_tx });
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    sub    = nullptr;
  const uint64_t              index  = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  add_key_utxo(utxos, 0x11U);

  EXPECT_EQ(evaluate_level(tx, utxos, &index, &sub), CARDANO_SUCCESS);
  expect_single_evaluated(sub, CARDANO_REDEEMER_TAG_GUARDING, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  cardano_redeemer_list_unref(&sub);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&top_ws);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_witness_set_unref(&sub_ws);
  cardano_credential_unref(&guard);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, findsATopLevelScriptInASubTransactionWitnessSet)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       guard  = v4_credential(script);
  cardano_witness_set_t*      sub_ws = v4_witness_set(script, false);
  cardano_sub_transaction_t*  sub_tx = build_sub_tx(0x11U, {}, sub_ws);
  cardano_witness_set_t*      top_ws = v4_witness_set(nullptr, true);
  cardano_transaction_t*      tx     = build_top_tx(0x10U, { guard }, top_ws, { sub_tx });
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    top    = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  add_key_utxo(utxos, 0x11U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &top), CARDANO_SUCCESS);
  expect_single_evaluated(top, CARDANO_REDEEMER_TAG_GUARDING, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);

  cardano_redeemer_list_unref(&top);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&top_ws);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_witness_set_unref(&sub_ws);
  cardano_credential_unref(&guard);
  cardano_plutus_v4_script_unref(&script);
}

/**
 * \brief Builds a top-level Plutus V3 spend of a script input whose output carries
 *        the hash of \p datum, next to a sub-transaction spending the key input
 *        of 0x11 with \p sub_ws. The script stays in the top-level witness set
 *        unless \p script_in_sub is set, in which case only \p sub_ws carries it.
 */
static cardano_transaction_t*
build_v3_spend_with_sub_tx(
  cardano_plutus_v3_script_t* script,
  cardano_plutus_data_t*      datum,
  cardano_witness_set_t*      sub_ws,
  bool                        script_in_sub,
  cardano_utxo_list_t**       out_utxos)
{
  cardano_transaction_t* tx = build_spend_tx(script, datum == nullptr, out_utxos);

  if (datum != nullptr)
  {
    cardano_utxo_t* utxo = nullptr;
    EXPECT_EQ(cardano_utxo_list_get(*out_utxos, 0U, &utxo), CARDANO_SUCCESS);

    cardano_transaction_output_t* output     = cardano_utxo_get_output(utxo);
    cardano_blake2b_hash_t*       datum_hash = datum_hash_of(datum);
    cardano_datum_t*              hash_datum = nullptr;

    EXPECT_EQ(cardano_datum_new_data_hash(datum_hash, &hash_datum), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_transaction_output_set_datum(output, hash_datum), CARDANO_SUCCESS);

    cardano_datum_unref(&hash_datum);
    cardano_blake2b_hash_unref(&datum_hash);
    cardano_transaction_output_unref(&output);
    cardano_utxo_unref(&utxo);
  }

  if (script_in_sub)
  {
    cardano_plutus_v3_script_set_t* scripts = nullptr;
    EXPECT_EQ(cardano_plutus_v3_script_set_new(&scripts), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v3_script_set_add(scripts, script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(sub_ws, scripts), CARDANO_SUCCESS);
    cardano_plutus_v3_script_set_unref(&scripts);

    cardano_witness_set_t* top_ws = cardano_transaction_get_witness_set(tx);
    EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(top_ws, nullptr), CARDANO_SUCCESS);
    cardano_witness_set_unref(&top_ws);
  }

  cardano_sub_transaction_t*     sub_tx = build_sub_tx(0x11U, {}, sub_ws);
  cardano_sub_transaction_set_t* set    = nullptr;
  cardano_transaction_body_t*    body   = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_sub_transaction_set_new(&set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_set_add(set, sub_tx), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_sub_transactions(body, set), CARDANO_SUCCESS);

  add_key_utxo(*out_utxos, 0x11U);

  cardano_transaction_body_unref(&body);
  cardano_sub_transaction_set_unref(&set);
  cardano_sub_transaction_unref(&sub_tx);

  return tx;
}

/**
 * \brief Builds a plutus-data set holding \p datum.
 */
static cardano_plutus_data_set_t*
datum_set_of(cardano_plutus_data_t* datum)
{
  cardano_plutus_data_set_t* set = nullptr;

  EXPECT_EQ(cardano_plutus_data_set_new(&set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_add(set, datum), CARDANO_SUCCESS);

  return set;
}

TEST(cardano_tx_evaluator_native_v4, resolvesATopLevelDatumOnlyFromTheTopLevelWitnessSet)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_plutus_data_t*      datum  = unit_data();
  cardano_plutus_data_set_t*  datums = datum_set_of(datum);
  cardano_witness_set_t*      sub_ws = v4_witness_set(nullptr, false);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(cardano_witness_set_set_plutus_data(sub_ws, datums), CARDANO_SUCCESS);

  cardano_transaction_t* tx = build_v3_spend_with_sub_tx(script, datum, sub_ws, false, &utxos);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(result, nullptr);

  cardano_witness_set_t* top_ws = cardano_transaction_get_witness_set(tx);
  EXPECT_EQ(cardano_witness_set_set_plutus_data(top_ws, datums), CARDANO_SUCCESS);
  cardano_witness_set_unref(&top_ws);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_SUCCESS);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(result), 1U);

  cardano_redeemer_list_unref(&result);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_witness_set_unref(&sub_ws);
  cardano_plutus_data_set_unref(&datums);
  cardano_plutus_data_unref(&datum);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, findsATopLevelPlutusV3ScriptInASubTransactionWitnessSet)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_witness_set_t*      sub_ws = v4_witness_set(nullptr, false);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    result = nullptr;
  cardano_transaction_t*      tx     = build_v3_spend_with_sub_tx(script, nullptr, sub_ws, true, &utxos);

  cardano_witness_set_t*          top_ws      = cardano_transaction_get_witness_set(tx);
  cardano_plutus_v3_script_set_t* top_scripts = cardano_witness_set_get_plutus_v3_scripts(top_ws);

  EXPECT_EQ(top_scripts, nullptr);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_SUCCESS);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(cardano_redeemer_list_get_length(result), 1U);

  cardano_redeemer_list_unref(&result);
  cardano_plutus_v3_script_set_unref(&top_scripts);
  cardano_witness_set_unref(&top_ws);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_witness_set_unref(&sub_ws);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAPlutusV3ScriptInASubTransaction)
{
  cardano_plutus_v3_script_t* script = build_v3_script(true);
  cardano_credential_t*       guard  = script_credential(script);
  cardano_witness_set_t*      sub_ws = v4_witness_set(nullptr, true);
  cardano_witness_set_t*      top_ws = v4_witness_set(nullptr, false);
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    result = nullptr;
  std::string                 message;

  cardano_plutus_v3_script_set_t* scripts = nullptr;
  EXPECT_EQ(cardano_plutus_v3_script_set_new(&scripts), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v3_script_set_add(scripts, script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(sub_ws, scripts), CARDANO_SUCCESS);
  cardano_plutus_v3_script_set_unref(&scripts);

  cardano_sub_transaction_t* sub_tx = build_sub_tx(0x11U, { guard }, sub_ws);
  cardano_transaction_t*     tx     = build_top_tx(0x10U, {}, top_ws, { sub_tx });

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  add_key_utxo(utxos, 0x11U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, &message), CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(message, "a sub-transaction redeemer runs a Plutus V1, V2 or V3 script, which only run at the top level");

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_witness_set_unref(&top_ws);
  cardano_witness_set_unref(&sub_ws);
  cardano_credential_unref(&guard);
  cardano_plutus_v3_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, deserialisesEveryLevelBeforeEvaluatingAnyScript)
{
  cardano_plutus_v4_script_t* failing     = build_v4_script(V4_FAILS);
  cardano_plutus_v4_script_t* undecodable = build_undecodable_v4_script();
  cardano_credential_t*       sub_guard   = v4_credential(undecodable);
  cardano_witness_set_t*      sub_ws      = v4_witness_set(undecodable, true);
  cardano_sub_transaction_t*  sub_tx      = build_sub_tx(0x11U, { sub_guard }, sub_ws);
  cardano_utxo_list_t*        utxos       = nullptr;
  cardano_transaction_t*      tx          = build_v4_spend_tx(failing, false, &utxos);
  cardano_redeemer_list_t*    result      = nullptr;
  std::string                 message;

  cardano_transaction_body_t*    body = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* set  = nullptr;
  EXPECT_EQ(cardano_sub_transaction_set_new(&set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_set_add(set, sub_tx), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_sub_transactions(body, set), CARDANO_SUCCESS);
  cardano_sub_transaction_set_unref(&set);
  cardano_transaction_body_unref(&body);

  add_key_utxo(utxos, 0x11U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, &message), CARDANO_ERROR_DECODING);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(message, "a redeemer's script cannot be deserialised at this protocol version");

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_witness_set_unref(&sub_ws);
  cardano_credential_unref(&sub_guard);
  cardano_plutus_v4_script_unref(&undecodable);
  cardano_plutus_v4_script_unref(&failing);
}

TEST(cardano_tx_evaluator_native_v4, failsTheTopLevelWhenASubTransactionScriptFails)
{
  cardano_plutus_v4_script_t* script = build_v4_script(V4_FAILS);
  cardano_credential_t*       guard  = v4_credential(script);
  cardano_witness_set_t*      sub_ws = v4_witness_set(script, true);
  cardano_sub_transaction_t*  sub_tx = build_sub_tx(0x11U, { guard }, sub_ws);
  cardano_witness_set_t*      top_ws = v4_witness_set(nullptr, false);
  cardano_transaction_t*      tx     = build_top_tx(0x10U, {}, top_ws, { sub_tx });
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  add_key_utxo(utxos, 0x11U);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result), CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);
  EXPECT_EQ(result, nullptr);

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&top_ws);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_witness_set_unref(&sub_ws);
  cardano_credential_unref(&guard);
  cardano_plutus_v4_script_unref(&script);
}

/**
 * \brief Builds a Plutus V4 script from the textual UPLC program \p source.
 */
static cardano_plutus_v4_script_t*
build_v4_script_from_source(const char* source, size_t size)
{
  cardano_buffer_t*           cbor   = program_cbor_from_source(source, size);
  cardano_plutus_v4_script_t* script = nullptr;

  EXPECT_EQ(cardano_plutus_v4_script_new_bytes(cardano_buffer_get_data(cbor), cardano_buffer_get_size(cbor), &script), CARDANO_SUCCESS);

  cardano_buffer_unref(&cbor);

  return script;
}

/**
 * \brief The script succeeds only when the datum of its spending ScriptInfo (the
 *        third item of the context) is Just (I 42), which only the witness set of
 *        the sub-transaction provides.
 */
TEST(cardano_tx_evaluator_native_v4, evaluatesAV4SpendInsideASubTransaction)
{
  static const char SOURCE[] =
    "(program 1.1.0 (lam ctx (force [ (force (builtin ifThenElse)) "
    "[ (builtin equalsData) "
    "[ (force (builtin headList)) [ (force (builtin tailList)) [ (force (force (builtin sndPair))) [ (builtin unConstrData) "
    "[ (force (builtin headList)) [ (force (builtin tailList)) [ (force (builtin tailList)) [ (builtin unListData) ctx ] ] ] ] ] ] ] ] "
    "(con data (Constr 0 [I 42])) ] "
    "(delay (con unit ())) (delay (error)) ])))";

  cardano_plutus_v4_script_t*   script     = build_v4_script_from_source(SOURCE, sizeof(SOURCE) - 1U);
  cardano_blake2b_hash_t*       hash       = cardano_plutus_v4_script_get_hash(script);
  cardano_address_t*            address    = script_address(hash);
  cardano_plutus_data_t*        datum_data = nullptr;
  cardano_plutus_data_set_t*    datums     = nullptr;
  cardano_witness_set_t*        sub_ws     = v4_witness_set(script, true, CARDANO_REDEEMER_TAG_SPEND, 0U);
  cardano_witness_set_t*        top_ws     = v4_witness_set(nullptr, false);
  cardano_utxo_list_t*          utxos      = nullptr;
  cardano_redeemer_list_t*      sub        = nullptr;
  cardano_transaction_input_t*  input      = make_input(0x11U, 0U);
  cardano_transaction_output_t* output     = nullptr;
  cardano_utxo_t*               utxo       = nullptr;
  cardano_datum_t*              datum      = nullptr;
  const uint64_t                index      = 0U;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(42, &datum_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_new(&datums), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_set_add(datums, datum_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_plutus_data(sub_ws, datums), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* datum_hash = datum_hash_of(datum_data);
  EXPECT_EQ(cardano_datum_new_data_hash(datum_hash, &datum), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, 5000000U, &output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_datum(output, datum), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_sub_transaction_t* sub_tx = build_sub_tx(0x11U, {}, sub_ws);
  cardano_transaction_t*     tx     = build_top_tx(0x10U, {}, top_ws, { sub_tx });

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  EXPECT_EQ(cardano_utxo_list_add(utxos, utxo), CARDANO_SUCCESS);

  EXPECT_EQ(evaluate_level(tx, utxos, &index, &sub), CARDANO_SUCCESS);
  expect_single_evaluated(sub, CARDANO_REDEEMER_TAG_SPEND, 0U, 2463610U, 4358U);

  cardano_redeemer_list_unref(&sub);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_datum_unref(&datum);
  cardano_blake2b_hash_unref(&datum_hash);
  cardano_transaction_input_unref(&input);
  cardano_witness_set_unref(&top_ws);
  cardano_witness_set_unref(&sub_ws);
  cardano_plutus_data_set_unref(&datums);
  cardano_plutus_data_unref(&datum_data);
  cardano_address_unref(&address);
  cardano_blake2b_hash_unref(&hash);
  cardano_plutus_v4_script_unref(&script);
}

/**
 * \brief The script succeeds only when the sub-transaction index of its TxInfo
 *        (the second field of the TxInfo, the first item of the context) is Just 1.
 */
TEST(cardano_tx_evaluator_native_v4, givesASubTransactionScriptItsPositionInTheBatch)
{
  static const char SOURCE[] =
    "(program 1.1.0 (lam ctx (force [ (force (builtin ifThenElse)) "
    "[ (builtin equalsData) "
    "[ (force (builtin headList)) [ (force (builtin tailList)) [ (builtin unListData) "
    "[ (force (builtin headList)) [ (builtin unListData) ctx ] ] ] ] ] "
    "(con data (Constr 0 [I 1])) ] "
    "(delay (con unit ())) (delay (error)) ])))";

  cardano_plutus_v4_script_t* script     = build_v4_script_from_source(SOURCE, sizeof(SOURCE) - 1U);
  cardano_credential_t*       guard      = v4_credential(script);
  cardano_witness_set_t*      guarded_ws = v4_witness_set(script, true);
  cardano_witness_set_t*      plain_ws   = v4_witness_set(nullptr, false);
  cardano_witness_set_t*      top_ws     = v4_witness_set(nullptr, false);
  cardano_sub_transaction_t*  plain      = build_sub_tx(0x11U, {}, plain_ws);
  cardano_sub_transaction_t*  guarded    = build_sub_tx(0x12U, { guard }, guarded_ws);
  cardano_transaction_t*      second     = build_top_tx(0x10U, {}, top_ws, { plain, guarded });
  cardano_transaction_t*      first      = build_top_tx(0x10U, {}, top_ws, { guarded, plain });
  cardano_utxo_list_t*        utxos      = nullptr;
  cardano_redeemer_list_t*    result     = nullptr;
  const uint64_t              guarded_ix = 1U;
  const uint64_t              misplaced  = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  add_key_utxo(utxos, 0x11U);
  add_key_utxo(utxos, 0x12U);

  EXPECT_EQ(evaluate_level(second, utxos, &guarded_ix, &result), CARDANO_SUCCESS);
  expect_single_evaluated(result, CARDANO_REDEEMER_TAG_GUARDING, 0U, 1999637U, 3262U);
  cardano_redeemer_list_unref(&result);

  EXPECT_EQ(evaluate_level(first, utxos, &misplaced, &result), CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);
  EXPECT_EQ(result, nullptr);

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&first);
  cardano_transaction_unref(&second);
  cardano_sub_transaction_unref(&guarded);
  cardano_sub_transaction_unref(&plain);
  cardano_witness_set_unref(&top_ws);
  cardano_witness_set_unref(&plain_ws);
  cardano_witness_set_unref(&guarded_ws);
  cardano_credential_unref(&guard);
  cardano_plutus_v4_script_unref(&script);
}

TEST(cardano_tx_evaluator_native_v4, rejectsAnEvaluatedSubTransactionThatDoesNotExist)
{
  cardano_witness_set_t*   top_ws = v4_witness_set(nullptr, false);
  cardano_transaction_t*   tx     = build_top_tx(0x10U, {}, top_ws);
  cardano_utxo_list_t*     utxos  = nullptr;
  cardano_redeemer_list_t* result = nullptr;
  const uint64_t           index  = 3U;
  std::string              message;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);

  EXPECT_EQ(evaluate_level(tx, utxos, &index, &result, &message), CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(message, "the transaction has no sub-transaction at the evaluated position");

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&top_ws);
}

TEST(cardano_tx_evaluator_new_native_sub_transaction, rejectsNullArguments)
{
  cardano_tx_evaluator_t* evaluator = nullptr;

  EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(nullptr, nullptr, kDijkstra, 0U, &evaluator), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(&kSlotConfig, nullptr, kDijkstra, 0U, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(evaluator, nullptr);
}

TEST(cardano_tx_evaluator_new_native_sub_transaction, buildsAndNamesTheEvaluator)
{
  cardano_tx_evaluator_t* evaluator = nullptr;

  EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(&kSlotConfig, nullptr, kDijkstra, 0U, &evaluator), CARDANO_SUCCESS);
  ASSERT_NE(evaluator, nullptr);
  EXPECT_STREQ(cardano_tx_evaluator_get_name(evaluator), "Native UPLC transaction evaluator");

  cardano_tx_evaluator_unref(&evaluator);
}

TEST(cardano_tx_evaluator_new_native_sub_transaction, returnsErrorOnMemoryAllocationFailure)
{
  cardano_tx_evaluator_t* evaluator = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(&kSlotConfig, nullptr, kDijkstra, 0U, &evaluator), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(&kSlotConfig, nullptr, kDijkstra, 0U, &evaluator), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(evaluator, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_evaluator_native_v4, returnsErrorOnMemoryAllocationFailureWhileEvaluating)
{
  cardano_plutus_v4_script_t* script    = build_v4_script(V4_RETURNS_UNIT);
  cardano_credential_t*       guard     = v4_credential(script);
  cardano_witness_set_t*      sub_ws    = v4_witness_set(script, true);
  cardano_sub_transaction_t*  sub_tx    = build_sub_tx(0x11U, { guard }, sub_ws);
  cardano_witness_set_t*      top_ws    = v4_witness_set(nullptr, false);
  cardano_transaction_t*      tx        = build_top_tx(0x10U, {}, top_ws, { sub_tx });
  cardano_utxo_list_t*        utxos     = nullptr;
  const uint64_t              index     = 0U;
  cardano_tx_evaluator_t*     evaluator = nullptr;
  bool                        succeeded = false;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_key_utxo(utxos, 0x10U);
  add_key_utxo(utxos, 0x11U);

  EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(&kSlotConfig, nullptr, kDijkstra, index, &evaluator), CARDANO_SUCCESS);

  for (int limit = 0; (limit < 100000) && !succeeded; ++limit)
  {
    cardano_redeemer_list_t* result = nullptr;

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(limit);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t error = cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result);

    cardano_set_allocators(malloc, realloc, free);

    if (error == CARDANO_SUCCESS)
    {
      succeeded = true;
      expect_single_evaluated(result, CARDANO_REDEEMER_TAG_GUARDING, 0U, kV4ReturnsUnitCpu, kV4ReturnsUnitMem);
    }
    else
    {
      EXPECT_EQ(result, nullptr);
    }

    cardano_redeemer_list_unref(&result);
  }

  EXPECT_TRUE(succeeded);

  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
  cardano_witness_set_unref(&top_ws);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_witness_set_unref(&sub_ws);
  cardano_credential_unref(&guard);
  cardano_plutus_v4_script_unref(&script);
}

/**
 * \brief Evaluates a single always-succeeding V4 spend with \p cost_models at
 *        Dijkstra, asserts it succeeded and returns the units it consumed.
 */
static void
v4_spend_units(cardano_costmdls_t* cost_models, uint64_t* cpu, uint64_t* mem)
{
  cardano_plutus_v4_script_t* script    = build_v4_script(V4_RETURNS_UNIT);
  cardano_utxo_list_t*        utxos     = nullptr;
  cardano_transaction_t*      tx        = build_v4_spend_tx(script, false, &utxos);
  cardano_tx_evaluator_t*     evaluator = nullptr;
  cardano_redeemer_list_t*    result    = nullptr;
  cardano_redeemer_t*         redeemer  = nullptr;

  EXPECT_EQ(cardano_tx_evaluator_new_native(&kSlotConfig, cost_models, kDijkstra, &evaluator), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &result), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_get(result, 0U, &redeemer), CARDANO_SUCCESS);

  cardano_ex_units_t* units = cardano_redeemer_get_ex_units(redeemer);

  *cpu = cardano_ex_units_get_cpu_steps(units);
  *mem = cardano_ex_units_get_memory(units);

  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&redeemer);
  cardano_redeemer_list_unref(&result);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_v4_script_unref(&script);
}

/**
 * \brief Builds a cost model of \p language with \p count parameters, all set to \p cost.
 */
static cardano_cost_model_t*
uniform_cost_model(cardano_plutus_language_version_t language, size_t count, int64_t cost)
{
  std::vector<int64_t>  costs(count, cost);
  cardano_cost_model_t* model = nullptr;

  EXPECT_EQ(cardano_cost_model_new(language, costs.data(), costs.size(), &model), CARDANO_SUCCESS);

  return model;
}

/**
 * \brief '[(lam ctx (con unit ())) ctx]' takes four machine steps (apply, lambda
 *        and two constants) plus the startup charge, each priced 200 cpu and 200
 *        memory by the uniform model. The default model prices them 16000 cpu and
 *        100 memory, startup 100 and 100, so it gives 64100 cpu and 500 memory.
 */
TEST(cardano_tx_evaluator_native_v4, usesThePlutusV4CostModelOfTheLedger)
{
  cardano_costmdls_t*   cost_models = nullptr;
  cardano_cost_model_t* model       = uniform_cost_model(CARDANO_PLUTUS_LANGUAGE_VERSION_V4, 251U, 200);
  uint64_t              cpu         = 0U;
  uint64_t              mem         = 0U;
  uint64_t              default_cpu = 0U;
  uint64_t              default_mem = 0U;

  EXPECT_EQ(cardano_costmdls_new(&cost_models), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_insert(cost_models, model), CARDANO_SUCCESS);

  v4_spend_units(cost_models, &cpu, &mem);
  v4_spend_units(nullptr, &default_cpu, &default_mem);

  EXPECT_EQ(cpu, 1000U);
  EXPECT_EQ(mem, 1000U);
  EXPECT_EQ(default_cpu, kV4ReturnsUnitCpu);
  EXPECT_EQ(default_mem, kV4ReturnsUnitMem);

  cardano_cost_model_unref(&model);
  cardano_costmdls_unref(&cost_models);
}

TEST(cardano_tx_evaluator_native_v4, fallsBackToTheDefaultCostModelWhenTheLedgerHasNoPlutusV4CostModel)
{
  cardano_costmdls_t*   cost_models = nullptr;
  cardano_cost_model_t* model       = uniform_cost_model(CARDANO_PLUTUS_LANGUAGE_VERSION_V3, 251U, 200);
  uint64_t              cpu         = 0U;
  uint64_t              mem         = 0U;
  uint64_t              default_cpu = 0U;
  uint64_t              default_mem = 0U;

  EXPECT_EQ(cardano_costmdls_new(&cost_models), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_insert(cost_models, model), CARDANO_SUCCESS);

  v4_spend_units(cost_models, &cpu, &mem);
  v4_spend_units(nullptr, &default_cpu, &default_mem);

  EXPECT_EQ(cpu, default_cpu);
  EXPECT_EQ(mem, default_mem);

  cardano_cost_model_unref(&model);
  cardano_costmdls_unref(&cost_models);
}

/**
 * \brief Builds a transaction spending one input held at the address of
 *        \p script_hash, whose script is provided by a reference input carrying
 *        \p script_ref. The spent output carries an inline datum when
 *        \p with_inline_datum is set.
 */
static cardano_transaction_t*
build_reference_spend_tx(cardano_script_t* script_ref, bool with_inline_datum, cardano_utxo_list_t** out_utxos)
{
  cardano_blake2b_hash_t*          hash          = cardano_script_get_hash(script_ref);
  cardano_address_t*               address       = script_address(hash);
  cardano_address_t*               ref_address   = key_address(0x20U);
  cardano_witness_set_t*           ws            = v4_witness_set(nullptr, true, CARDANO_REDEEMER_TAG_SPEND, 0U);
  cardano_transaction_t*           tx            = build_top_tx(0x10U, {}, ws);
  cardano_transaction_body_t*      body          = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* reference_set = input_set_of(0x20U);
  cardano_transaction_input_t*     input         = make_input(0x10U, 0U);
  cardano_transaction_output_t*    output        = nullptr;
  cardano_utxo_t*                  utxo          = nullptr;

  EXPECT_EQ(cardano_transaction_body_set_reference_inputs(body, reference_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(out_utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, 5000000U, &output), CARDANO_SUCCESS);

  if (with_inline_datum)
  {
    cardano_plutus_data_t* data  = unit_data();
    cardano_datum_t*       datum = nullptr;

    EXPECT_EQ(cardano_datum_new_inline_data(data, &datum), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_transaction_output_set_datum(output, datum), CARDANO_SUCCESS);

    cardano_datum_unref(&datum);
    cardano_plutus_data_unref(&data);
  }

  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(*out_utxos, utxo), CARDANO_SUCCESS);

  add_utxo(*out_utxos, 0x20U, ref_address, script_ref);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_transaction_input_unref(&input);
  cardano_transaction_input_set_unref(&reference_set);
  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&ws);
  cardano_address_unref(&ref_address);
  cardano_address_unref(&address);
  cardano_blake2b_hash_unref(&hash);

  return tx;
}

TEST(cardano_tx_evaluator_native, rejectsAPlutusV1SpendThroughAReferenceScript)
{
  cardano_plutus_v1_script_t* plutus = build_v1_script(3U);
  cardano_script_t*           script = nullptr;
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(cardano_script_new_plutus_v1(plutus, &script), CARDANO_SUCCESS);

  cardano_transaction_t* tx = build_reference_spend_tx(script, true, &utxos);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, nullptr, 10U), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(result, nullptr);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus);
}

TEST(cardano_tx_evaluator_native, evaluatesAPlutusV2SpendThroughAReferenceScript)
{
  cardano_plutus_v2_script_t* plutus = build_v2_script(3U);
  cardano_script_t*           script = nullptr;
  cardano_utxo_list_t*        utxos  = nullptr;

  EXPECT_EQ(cardano_script_new_plutus_v2(plutus, &script), CARDANO_SUCCESS);

  cardano_transaction_t* tx = build_reference_spend_tx(script, true, &utxos);

  expect_single_redeemer_ok(tx, utxos);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_script_unref(&script);
  cardano_plutus_v2_script_unref(&plutus);
}

TEST(cardano_tx_evaluator_native, evaluatesAPlutusV3SpendThroughAReferenceScript)
{
  cardano_plutus_v3_script_t* plutus = build_v3_script(true);
  cardano_script_t*           script = nullptr;
  cardano_utxo_list_t*        utxos  = nullptr;

  EXPECT_EQ(cardano_script_new_plutus_v3(plutus, &script), CARDANO_SUCCESS);

  cardano_transaction_t* tx = build_reference_spend_tx(script, false, &utxos);

  expect_single_redeemer_ok(tx, utxos);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_script_unref(&script);
  cardano_plutus_v3_script_unref(&plutus);
}

TEST(cardano_tx_evaluator_native, rejectsAPlutusV2SpendWithoutADatum)
{
  cardano_plutus_v2_script_t* plutus = build_v2_script(3U);
  cardano_script_t*           script = nullptr;
  cardano_utxo_list_t*        utxos  = nullptr;
  cardano_redeemer_list_t*    result = nullptr;

  EXPECT_EQ(cardano_script_new_plutus_v2(plutus, &script), CARDANO_SUCCESS);

  cardano_transaction_t* tx = build_reference_spend_tx(script, false, &utxos);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, nullptr, 10U), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(result, nullptr);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_script_unref(&script);
  cardano_plutus_v2_script_unref(&plutus);
}

TEST(cardano_tx_evaluator_native, rejectsAPlutusV1SpendWithoutADatum)
{
  cardano_plutus_v1_script_t* script   = build_v1_script(3U);
  cardano_blake2b_hash_t*     hash     = cardano_plutus_v1_script_get_hash(script);
  cardano_address_t*          address  = script_address(hash);
  cardano_utxo_list_t*        resolved = nullptr;
  cardano_utxo_list_t*        utxos    = nullptr;
  cardano_redeemer_list_t*    result   = nullptr;
  cardano_transaction_t*      tx       = build_v1_spend_tx(script, &resolved);

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  add_utxo(utxos, 0x00U, address);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, nullptr, 10U), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(result, nullptr);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_utxo_list_unref(&resolved);
  cardano_address_unref(&address);
  cardano_blake2b_hash_unref(&hash);
  cardano_plutus_v1_script_unref(&script);
}

TEST(cardano_tx_evaluator_native, doesNotRunANativeReferenceScript)
{
  static const char*       NATIVE_SCRIPT_CBOR = "82008200581c000102030405060708090a0b0c0d0e0f101112131415161718191a1b";
  cardano_script_t*        script             = nullptr;
  cardano_utxo_list_t*     utxos              = nullptr;
  cardano_redeemer_list_t* result             = nullptr;
  cardano_cbor_reader_t*   reader             = cardano_cbor_reader_from_hex(NATIVE_SCRIPT_CBOR, strlen(NATIVE_SCRIPT_CBOR));

  EXPECT_EQ(cardano_script_from_cbor(reader, &script), CARDANO_SUCCESS);

  cardano_transaction_t* tx = build_reference_spend_tx(script, true, &utxos);

  EXPECT_EQ(evaluate_level(tx, utxos, nullptr, &result, nullptr, 10U), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(result, nullptr);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
}
