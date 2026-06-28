/**
 * \file differential_eval_phase_two.cpp
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>

#include <gmock/gmock.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <dlfcn.h>

// Ground-truth differential validation of native phase-2 evaluation.
//
// GATED behind AIKEN_DYLIB: when unset every test reports a skip and passes,
// so CI (without the aiken dylib) is unaffected. When AIKEN_DYLIB points at
// libaiken_c_ffi.dylib, the dylib is dlopen()'d and our native
// cardano_tx_evaluator output is compared byte-for-byte against the aiken
// reference eval_phase_two for a corpus of real Plutus transactions. The same
// tx / inputs / outputs / cost-models / slot config are fed to both sides and
// the per-redeemer ex-units (mem, cpu) must match exactly.

/* TYPES *********************************************************************/

// Mirrors SlotConfig in aiken-c-ffi/include/aiken_ffi.h (field order matters).
struct AikenSlotConfig
{
    uint32_t slot_length;
    uint64_t zero_slot;
    uint64_t zero_time;
};

// Mirrors InitialBudget in aiken-c-ffi/include/aiken_ffi.h (mem first).
struct AikenInitialBudget
{
    int64_t mem;
    int64_t cpu;
};

typedef const char* (*aiken_eval_fn)(
  const char*         tx_hex,
  const char*         inputs,
  const char*         outputs,
  const char*         cost_mdls,
  AikenInitialBudget* initial_budget,
  AikenSlotConfig*    slot_config);

typedef void (*aiken_drop_fn)(const char* pointer);

/**
 * \brief One self-contained differential vector.
 *
 * Everything is hex so the same bytes drive both evaluators. The expected count
 * is the number of redeemers the transaction carries.
 */
struct EvalVector
{
    const char* name;
    const char* tx_hex;
    const char* inputs_hex;
    const char* outputs_hex;
    const char* cost_mdls_hex;
    uint64_t    zero_time;
    uint64_t    zero_slot;
    uint32_t    slot_length;
    size_t      expected_redeemers;
};

/* CORPUS ********************************************************************/

// Two real Plutus V2 transactions taken verbatim from aiken-c-ffi
// transaction.rs (eval_phase_test_all_ok1 / ok2). ok1 carries one redeemer,
// ok2 carries five. Both pass everything (including the ledger cost model) as
// hex, so they exercise the real cost model end to end.
#include "differential_eval_phase_two_vectors.inc"

/* STATIC HELPERS ************************************************************/

namespace
{

/**
 * \brief A pairing of one redeemer key to its computed ex-units.
 */
struct RedeemerUnits
{
    int      tag;
    uint64_t index;
    uint64_t mem;
    uint64_t cpu;
};

/**
 * \brief Extracts the per-redeemer ex-units from a redeemer-list CBOR hex.
 *
 * The hex is the legacy array form a phase-2 evaluator emits; it is decoded with
 * the cardano-c redeemer list decoder and each entry's (tag, index, mem, cpu) is
 * collected.
 */
bool
units_from_redeemer_cbor_hex(const std::string& hex, std::vector<RedeemerUnits>& out)
{
  cardano_cbor_reader_t*   reader = cardano_cbor_reader_from_hex(hex.c_str(), hex.size());
  cardano_redeemer_list_t* list   = nullptr;

  if (reader == nullptr)
  {
    return false;
  }

  cardano_error_t err = cardano_redeemer_list_from_cbor(reader, &list);
  cardano_cbor_reader_unref(&reader);

  if ((err != CARDANO_SUCCESS) || (list == nullptr))
  {
    cardano_redeemer_list_unref(&list);
    return false;
  }

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(list); ++i)
  {
    cardano_redeemer_t* r = nullptr;
    if (cardano_redeemer_list_get(list, i, &r) != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&list);
      return false;
    }

    cardano_ex_units_t* u = cardano_redeemer_get_ex_units(r);

    RedeemerUnits ru;
    ru.tag   = static_cast<int>(cardano_redeemer_get_tag(r));
    ru.index = cardano_redeemer_get_index(r);
    ru.mem   = cardano_ex_units_get_memory(u);
    ru.cpu   = cardano_ex_units_get_cpu_steps(u);
    out.push_back(ru);

    cardano_ex_units_unref(&u);
    cardano_redeemer_unref(&r);
  }

  cardano_redeemer_list_unref(&list);
  return true;
}

/**
 * \brief Builds the resolved-input UTxO list by pairing inputs[i] to outputs[i].
 *
 * The inputs hex is a CBOR array of TransactionInput and the outputs hex a CBOR
 * array of TransactionOutput; the i-th input is spent from the i-th output. The
 * inputs are read element by element so the index pairing is preserved.
 */
cardano_utxo_list_t*
build_utxo_list(const char* inputs_hex, const char* outputs_hex)
{
  cardano_cbor_reader_t*             in_reader  = cardano_cbor_reader_from_hex(inputs_hex, strlen(inputs_hex));
  cardano_cbor_reader_t*             out_reader = cardano_cbor_reader_from_hex(outputs_hex, strlen(outputs_hex));
  cardano_transaction_output_list_t* outputs    = nullptr;
  cardano_utxo_list_t*               utxos      = nullptr;

  if ((in_reader == nullptr) || (out_reader == nullptr))
  {
    cardano_cbor_reader_unref(&in_reader);
    cardano_cbor_reader_unref(&out_reader);
    return nullptr;
  }

  if (cardano_transaction_output_list_from_cbor(out_reader, &outputs) != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&in_reader);
    cardano_cbor_reader_unref(&out_reader);
    return nullptr;
  }

  if (cardano_utxo_list_new(&utxos) != CARDANO_SUCCESS)
  {
    cardano_transaction_output_list_unref(&outputs);
    cardano_cbor_reader_unref(&in_reader);
    cardano_cbor_reader_unref(&out_reader);
    return nullptr;
  }

  int64_t         count = 0;
  cardano_error_t err   = cardano_cbor_reader_read_start_array(in_reader, &count);

  for (size_t i = 0U; (err == CARDANO_SUCCESS) && (i < cardano_transaction_output_list_get_length(outputs)); ++i)
  {
    cardano_transaction_input_t*  input  = nullptr;
    cardano_transaction_output_t* output = nullptr;
    cardano_utxo_t*               utxo   = nullptr;

    err = cardano_transaction_input_from_cbor(in_reader, &input);

    if (err == CARDANO_SUCCESS)
    {
      err = cardano_transaction_output_list_get(outputs, i, &output);
    }

    if (err == CARDANO_SUCCESS)
    {
      err = cardano_utxo_new(input, output, &utxo);
    }

    if (err == CARDANO_SUCCESS)
    {
      err = cardano_utxo_list_add(utxos, utxo);
    }

    cardano_utxo_unref(&utxo);
    cardano_transaction_output_unref(&output);
    cardano_transaction_input_unref(&input);
  }

  cardano_transaction_output_list_unref(&outputs);
  cardano_cbor_reader_unref(&in_reader);
  cardano_cbor_reader_unref(&out_reader);

  if (err != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&utxos);
    return nullptr;
  }

  return utxos;
}

/**
 * \brief Finds a redeemer's units in a list by (tag, index).
 */
const RedeemerUnits*
find_units(const std::vector<RedeemerUnits>& v, int tag, uint64_t index)
{
  const auto it = std::find_if(v.begin(), v.end(), [tag, index](const RedeemerUnits& ru)
                               { return (ru.tag == tag) && (ru.index == index); });

  return (it != v.end()) ? &(*it) : nullptr;
}

} // namespace

/* TESTS *********************************************************************/

TEST(differential_eval_phase_two, matchesAikenExUnitsByteForByte)
{
  const char* dylib_path = std::getenv("AIKEN_DYLIB");

  if ((dylib_path == nullptr) || (dylib_path[0] == '\0'))
  {
    GTEST_SKIP() << "AIKEN_DYLIB not set; skipping aiken eval-phase-two differential validation.";
  }

  void* handle = dlopen(dylib_path, RTLD_NOW | RTLD_LOCAL);
  ASSERT_NE(handle, nullptr) << "dlopen failed: " << dlerror();

  aiken_eval_fn eval = reinterpret_cast<aiken_eval_fn>(dlsym(handle, "eval_phase_two"));
  aiken_drop_fn drop = reinterpret_cast<aiken_drop_fn>(dlsym(handle, "drop_char_pointer"));

  ASSERT_NE(eval, nullptr) << "dlsym eval_phase_two failed";
  ASSERT_NE(drop, nullptr) << "dlsym drop_char_pointer failed";

  int                      total   = 0;
  int                      matched = 0;
  std::vector<std::string> mismatches;

  for (const EvalVector& v: kEvalVectors)
  {
    // Large ceiling so neither side is budget-limited; we compare the COMPUTED
    // spent units, not the ceiling.
    AikenInitialBudget budget;
    budget.cpu = 10000000000LL;
    budget.mem = 14000000LL;

    AikenSlotConfig slot;
    slot.slot_length = v.slot_length;
    slot.zero_slot   = v.zero_slot;
    slot.zero_time   = v.zero_time;

    const char* json = eval(v.tx_hex, v.inputs_hex, v.outputs_hex, v.cost_mdls_hex, &budget, &slot);
    ASSERT_NE(json, nullptr) << v.name << ": aiken returned null";
    std::string json_str(json);
    drop(json);

    bool aiken_ok = json_str.find("\"SUCCESS\"") != std::string::npos;
    if (!aiken_ok)
    {
      std::string m = std::string(v.name) + " : aiken did not succeed: " + json_str;
      mismatches.push_back(m);
      ADD_FAILURE() << m;
      continue;
    }

    std::string aiken_cbor;
    {
      const std::string key = "\"redeemer_cbor\":\"";
      size_t            pos = json_str.find(key);
      if (pos != std::string::npos)
      {
        pos        += key.size();
        size_t end = json_str.find('"', pos);
        aiken_cbor = json_str.substr(pos, end - pos);
      }
    }
    ASSERT_FALSE(aiken_cbor.empty()) << v.name << ": no redeemer_cbor in aiken json: " << json_str;

    std::vector<RedeemerUnits> aiken_units;
    ASSERT_TRUE(units_from_redeemer_cbor_hex(aiken_cbor, aiken_units)) << v.name << ": failed to decode aiken redeemer cbor";

    // Our native evaluator over the SAME inputs and cost model.
    cardano_cbor_reader_t* tx_reader = cardano_cbor_reader_from_hex(v.tx_hex, strlen(v.tx_hex));
    ASSERT_NE(tx_reader, nullptr);
    cardano_transaction_t* tx = nullptr;
    ASSERT_EQ(cardano_transaction_from_cbor(tx_reader, &tx), CARDANO_SUCCESS) << v.name << ": tx decode failed";
    cardano_cbor_reader_unref(&tx_reader);

    cardano_cbor_reader_t* cm_reader = cardano_cbor_reader_from_hex(v.cost_mdls_hex, strlen(v.cost_mdls_hex));
    ASSERT_NE(cm_reader, nullptr);
    cardano_costmdls_t* costmdls = nullptr;
    ASSERT_EQ(cardano_costmdls_from_cbor(cm_reader, &costmdls), CARDANO_SUCCESS) << v.name << ": costmdls decode failed";
    cardano_cbor_reader_unref(&cm_reader);

    cardano_utxo_list_t* utxos = build_utxo_list(v.inputs_hex, v.outputs_hex);
    ASSERT_NE(utxos, nullptr) << v.name << ": utxo list build failed";

    cardano_slot_config_t our_slot;
    our_slot.zero_time   = v.zero_time;
    our_slot.zero_slot   = v.zero_slot;
    our_slot.slot_length = v.slot_length;

    cardano_tx_evaluator_t* evaluator = nullptr;
    ASSERT_EQ(cardano_tx_evaluator_new_native(&our_slot, costmdls, 9U, &evaluator), CARDANO_SUCCESS) << v.name;

    cardano_redeemer_list_t* ours     = nullptr;
    cardano_error_t          eval_err = cardano_tx_evaluator_evaluate(evaluator, tx, utxos, &ours);

    if (eval_err != CARDANO_SUCCESS)
    {
      std::string m = std::string(v.name) + " : our evaluator failed: error " + std::to_string(static_cast<int>(eval_err));
      mismatches.push_back(m);
      ADD_FAILURE() << m;
    }
    else
    {
      std::vector<RedeemerUnits> our_units;
      for (size_t i = 0U; i < cardano_redeemer_list_get_length(ours); ++i)
      {
        cardano_redeemer_t* r = nullptr;
        ASSERT_EQ(cardano_redeemer_list_get(ours, i, &r), CARDANO_SUCCESS);
        cardano_ex_units_t* u = cardano_redeemer_get_ex_units(r);

        RedeemerUnits ru;
        ru.tag   = static_cast<int>(cardano_redeemer_get_tag(r));
        ru.index = cardano_redeemer_get_index(r);
        ru.mem   = cardano_ex_units_get_memory(u);
        ru.cpu   = cardano_ex_units_get_cpu_steps(u);
        our_units.push_back(ru);

        cardano_ex_units_unref(&u);
        cardano_redeemer_unref(&r);
      }

      EXPECT_EQ(aiken_units.size(), v.expected_redeemers) << v.name << ": aiken redeemer count";
      EXPECT_EQ(our_units.size(), aiken_units.size()) << v.name << ": redeemer count mismatch";

      for (const RedeemerUnits& a: aiken_units)
      {
        ++total;
        const RedeemerUnits* o = find_units(our_units, a.tag, a.index);

        if (o == nullptr)
        {
          std::string m = std::string(v.name) + " : missing our redeemer tag=" + std::to_string(a.tag) +
            " index=" + std::to_string(a.index);
          mismatches.push_back(m);
          ADD_FAILURE() << m;
          continue;
        }

        if ((o->mem == a.mem) && (o->cpu == a.cpu))
        {
          ++matched;
        }
        else
        {
          std::string m = std::string(v.name) + " : EX-UNIT MISMATCH tag=" + std::to_string(a.tag) +
            " index=" + std::to_string(a.index) +
            " aiken(mem=" + std::to_string(a.mem) + ",cpu=" + std::to_string(a.cpu) + ")" +
            " ours(mem=" + std::to_string(o->mem) + ",cpu=" + std::to_string(o->cpu) + ")";
          mismatches.push_back(m);
          ADD_FAILURE() << m;
        }
      }
    }

    cardano_redeemer_list_unref(&ours);
    cardano_tx_evaluator_unref(&evaluator);
    cardano_utxo_list_unref(&utxos);
    cardano_costmdls_unref(&costmdls);
    cardano_transaction_unref(&tx);
  }

  dlclose(handle);

  std::printf("[eval-diff] redeemers=%d byte_identical=%d mismatches=%zu\n", total, matched, mismatches.size());

  for (const std::string& m: mismatches)
  {
    std::printf("[eval-diff] MISMATCH: %s\n", m.c_str());
  }

  EXPECT_EQ(matched, total) << "not all redeemers matched aiken ex-units byte-for-byte";
  EXPECT_TRUE(mismatches.empty());
}
