/**
 * \file conformance.cpp
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#include "../../src/uplc/ast/uplc_term.h"
#include "../../src/uplc/machine/uplc_machine.h"
#include <cardano/buffer.h>
#include <cardano/error.h>

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/ast/uplc_constant.h"
#include "../../src/uplc/ast/uplc_lang_version.h"
#include "../../src/uplc/ast/uplc_type.h"
#include "../../src/uplc/builtins/uplc_builtin.h"
#include "../../src/uplc/flat/flat_decode.h"
#include "../../src/uplc/flat/flat_reader.h"
#include "../../src/uplc/syntax/pretty.h"
#include "../../src/uplc/syntax/text_parser.h"

#include "../../src/uplc/cost/uplc_builtin_costs.h"
#include "../../src/uplc/cost/uplc_cost_model.h"
#include "../../src/uplc/cost/uplc_machine_costs.h"
#include "../../src/uplc/cost/uplc_selected_cost_model.h"

#include "cost_model_params_vectors.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

/* STATIC HELPERS ************************************************************/

namespace
{

namespace fs = std::filesystem;

/**
 * \brief A complete evaluation configuration shared by every case of a run.
 *
 * Every case of a run is evaluated with one protocol version, builtin semantics
 * variant and cost model, independent of the program version header in each
 * input. A configuration with a ledger parameter list builds its cost model
 * through the ledger's selection for its language and protocol version; one
 * without builds the default cost model of its cost model version.
 */
struct Configuration
{
    /** \brief The Plutus language whose builtin and program-version availability the run assumes. */
    cardano_uplc_lang_version_t language;
    /** \brief The major protocol version the cases are evaluated under. */
    uint64_t protocol_major;
    /** \brief The builtin semantics variant. */
    cardano_uplc_builtin_semantics_t semantics;
    /** \brief The cost model version selecting the default machine-step costs. */
    cardano_uplc_cost_model_version_t cost_model_version;
    /** \brief The ledger cost model parameter list, or NULL for the default cost model. */
    const int64_t* params;
    /** \brief The number of parameters in \c params. */
    size_t param_count;
};

/**
 * \brief The configuration of the reference conformance suite.
 *
 * The reference harness (plutus-conformance/src/PlutusConformance/CekEvaluator.hs)
 * evaluates every case with the default builtin semantics variant, which is the
 * newest one (G), with the parameters of the newest cost model (E, through
 * \c defaultCostModelParamsForTesting) and with the case-on-builtin support of the
 * newest protocol version (12, Dijkstra). That is Plutus V4 at protocol version 12
 * with the V4 cost model.
 */
const Configuration NEWEST_CONFIGURATION = {
  CARDANO_UPLC_LANG_VERSION_V4,
  12U,
  CARDANO_UPLC_SEMANTICS_G,
  CARDANO_UPLC_COST_MODEL_VERSION_V4,
  nullptr,
  0U
};

/**
 * \brief The configuration Plutus V3 scripts were evaluated under on mainnet at
 *        protocol version 10 (Plomin).
 *
 * V3 at protocol version 10 reads builtin semantics variant C, and has neither
 * case on builtin constants nor Plutus Core 1.2.0. The cost model is selected the
 * way the ledger selects it, from the default V3 parameter list, which carries
 * the values of builtin cost model E. Mainnet read builtin cost model C, but the
 * run covers only cases that cost the same under cost models C and E (see
 * \ref MAINNET_COSTED_DIFFERENTLY).
 */
const Configuration MAINNET_CONFIGURATION = {
  CARDANO_UPLC_LANG_VERSION_V3,
  10U,
  CARDANO_UPLC_SEMANTICS_C,
  CARDANO_UPLC_COST_MODEL_VERSION_V3,
  V3_DEFAULT_PARAMS,
  CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3
};

/**
 * \brief The number of corpus cases the mainnet run covers.
 *
 * Pinned so that a change to the inclusion rule or to the corpus that shrinks
 * the run is noticed.
 */
const size_t MAINNET_CASE_COUNT = 1088U;

/**
 * \brief The number of .uplc cases in the corpus.
 *
 * Pinned so that a missing or unreadable input fails the newest run.
 */
const size_t CORPUS_TEXTUAL_CASE_COUNT = 1037U;

/**
 * \brief The number of .flat cases in the corpus; see \ref CORPUS_TEXTUAL_CASE_COUNT.
 */
const size_t CORPUS_FLAT_CASE_COUNT = 934U;

/**
 * \brief The CPU ceiling of every evaluation.
 *
 * The reference counts the budget without a limit, and some V4 cases (the
 * saturating dropList requests) spend the full int64 budget and still succeed, so
 * the ceiling is INT64_MAX. No corpus failure depends on a lower ceiling: every
 * expected failure is an error-term failure.
 */
const int64_t BUDGET_CPU = INT64_MAX;

/**
 * \brief The memory ceiling of every evaluation; see \ref BUDGET_CPU.
 */
const int64_t BUDGET_MEM = INT64_MAX;

/**
 * \brief The builtins whose cost under the mainnet configuration differs from
 *        their cost under the newest one.
 *
 * Builtin cost models C and E differ only in the integer-division builtins and
 * equalsByteString, and variant C costs a text string by its code points where
 * variant G costs it by its utf-8 byte length divided by 4, rounded down, which
 * changes the cost of the builtins that read a string's size: appendString,
 * equalsString and encodeUtf8. A case naming none of them costs the same under
 * cost models C and E.
 */
const std::set<cardano_uplc_builtin_t> MAINNET_COSTED_DIFFERENTLY = {
  CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER,
  CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER,
  CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER,
  CARDANO_UPLC_BUILTIN_MOD_INTEGER,
  CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_APPEND_STRING,
  CARDANO_UPLC_BUILTIN_EQUALS_STRING,
  CARDANO_UPLC_BUILTIN_ENCODE_UTF8
};

/**
 * \brief The builtins whose arguments variant G bounds and variant C does not.
 *
 * Variant G rejects a byte-string argument longer than 65536 bytes, and selects
 * the bounded forms of writeBits, shiftByteString, rotateByteString, constrData
 * and the BLS12-381 scalarMul builtins. A bound can only turn a success into a
 * failure, so it changes the expectation of a failing case alone.
 */
const std::set<cardano_uplc_builtin_t> VARIANT_G_BOUNDED = {
  CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_SHA2_256,
  CARDANO_UPLC_BUILTIN_SHA3_256,
  CARDANO_UPLC_BUILTIN_BLAKE2B_256,
  CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE,
  CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE,
  CARDANO_UPLC_BUILTIN_DECODE_UTF8,
  CARDANO_UPLC_BUILTIN_CONSTR_DATA,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL,
  CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL,
  CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP,
  CARDANO_UPLC_BUILTIN_KECCAK_256,
  CARDANO_UPLC_BUILTIN_BLAKE2B_224,
  CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER,
  CARDANO_UPLC_BUILTIN_AND_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_OR_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_READ_BIT,
  CARDANO_UPLC_BUILTIN_WRITE_BITS,
  CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING,
  CARDANO_UPLC_BUILTIN_COUNT_SET_BITS,
  CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT,
  CARDANO_UPLC_BUILTIN_RIPEMD_160
};

/**
 * \brief The expected outcome class of a case, read from its expected files.
 */
enum class Outcome
{
  /** \brief The input is expected to fail parsing or decoding. */
  PARSE_ERROR,
  /** \brief Evaluation is expected to fail. */
  EVALUATION_FAILURE,
  /** \brief A result term and an exact budget are expected. */
  SUCCESS
};

/**
 * \brief The input format of a case, named by its file extension.
 */
enum class Format
{
  /** \brief A .uplc program in the surface syntax. */
  TEXTUAL,
  /** \brief A .flat flat-encoded program. */
  FLAT
};

/**
 * \brief The reason a case is left out of the mainnet run.
 *
 * A case is in the mainnet run when its expected outcome, result and budget are
 * the same under the mainnet configuration as under the newest one. The reasons
 * are checked in this order and the first that applies is reported.
 */
enum class MainnetExclusion
{
  /** \brief The case is in the mainnet run. */
  NONE,
  /** \brief The case expects a parse or decode error, which no configuration changes. */
  PARSE_ERROR,
  /** \brief The program version is not available to V3 at protocol version 10. */
  PROGRAM_VERSION,
  /** \brief The program names a builtin not available to V3 at protocol version 10. */
  BUILTIN,
  /** \brief The program holds an array or value constant. */
  CONSTANT_TYPE,
  /** \brief The program has a case whose scrutinee is not a constr term. */
  CASE_ON_NON_CONSTR,
  /** \brief The program names a builtin the mainnet configuration costs differently. */
  COSTING,
  /** \brief The case expects a failure and names a builtin variant G bounds. */
  BOUNDS
};

/**
 * \brief One corpus case: its input and expected files.
 */
struct Case
{
    /** \brief The input format. */
    Format format;
    /** \brief The input program text or flat bytes. */
    std::string input;
    /** \brief The expected outcome text or flat bytes. */
    std::string expected;
    /** \brief The expected budget text. */
    std::string budget;
    /** \brief The expected outcome class. */
    Outcome outcome;
};

/**
 * \brief The builtins, constants and case terms a program contains.
 */
struct Features
{
    /** \brief Every builtin the program names. */
    std::set<cardano_uplc_builtin_t> builtins;
    /** \brief Whether a constant has an array or value type. */
    bool has_newer_constant_type;
    /** \brief Whether a case term scrutinises something other than a constr term. */
    bool has_case_on_non_constr;
};

/**
 * \brief Returns the corpus directory, next to this source file.
 *
 * \return The corpus path, independent of the working directory.
 */
std::string
corpus_root()
{
  fs::path file = fs::path(__FILE__);
  return (file.parent_path() / "vectors").string();
}

/**
 * \brief Reads a whole file.
 *
 * \param[in] path The file to read.
 *
 * \return The file contents, or an empty string if it cannot be read.
 */
std::string
read_file(const std::string& path)
{
  std::ifstream      stream(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

/**
 * \brief Creates an arena for one case.
 *
 * \return The new arena.
 */
cardano_uplc_arena_t*
make_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(0U, &arena), CARDANO_SUCCESS);
  return arena;
}

/**
 * \brief Whether a string contains another.
 *
 * \param[in] haystack The string to search.
 * \param[in] needle The string to find.
 *
 * \return \c true when \p needle occurs in \p haystack.
 */
bool
contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

/**
 * \brief Whether a string ends with a suffix.
 *
 * \param[in] name The string to test.
 * \param[in] suffix The suffix.
 *
 * \return \c true when \p name ends with \p suffix.
 */
bool
has_suffix(const std::string& name, const std::string& suffix)
{
  return (name.size() >= suffix.size()) && (name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0);
}

/**
 * \brief Derives the expected outcome class from the expected outcome and budget.
 *
 * \param[in] expected_text The expected outcome file contents.
 * \param[in] budget_text The expected budget file contents.
 *
 * \return The expected outcome class.
 */
Outcome
classify(const std::string& expected_text, const std::string& budget_text)
{
  if (contains(expected_text, "parse/decode error") || contains(budget_text, "parse/decode error"))
  {
    return Outcome::PARSE_ERROR;
  }

  if (contains(expected_text, "evaluation failure") || contains(budget_text, "evaluation failure"))
  {
    return Outcome::EVALUATION_FAILURE;
  }

  return Outcome::SUCCESS;
}

/**
 * \brief Renders a term to the canonical surface syntax.
 *
 * Used for structural (alpha) comparison and for diagnostics.
 *
 * \param[in] term The term to render.
 *
 * \return The rendering, \c "<null>" for a NULL term, or an empty string on failure.
 */
std::string
render_term(const cardano_uplc_term_t* term)
{
  if (term == nullptr)
  {
    return std::string("<null>");
  }

  cardano_buffer_t* buffer = nullptr;
  if (cardano_uplc_pretty_print_term(term, &buffer) != CARDANO_SUCCESS)
  {
    return std::string();
  }

  std::string out(reinterpret_cast<const char*>(cardano_buffer_get_data(buffer)), cardano_buffer_get_size(buffer));
  cardano_buffer_unref(&buffer);

  while (!out.empty() && (out.back() == '\0'))
  {
    out.pop_back();
  }

  return out;
}

/**
 * \brief Parses the "({cpu: N\n| mem: N})" budget format.
 *
 * \param[in] text The budget file contents.
 * \param[out] cpu The expected cpu.
 * \param[out] mem The expected memory.
 *
 * \return \c true on success.
 */
bool
parse_budget(const std::string& text, int64_t* cpu, int64_t* mem)
{
  size_t cpu_pos = text.find("cpu:");
  size_t mem_pos = text.find("mem:");
  if ((cpu_pos == std::string::npos) || (mem_pos == std::string::npos))
  {
    return false;
  }

  try
  {
    *cpu = std::stoll(text.substr(cpu_pos + 4U));
    *mem = std::stoll(text.substr(mem_pos + 4U));
  }
  catch (...)
  {
    return false;
  }

  return true;
}

/**
 * \brief Whether an evaluation status is a script failure.
 *
 * \param[in] status The evaluation status.
 *
 * \return \c true for an error term or an exhausted budget.
 */
bool
is_script_failure(cardano_uplc_eval_status_t status)
{
  return (status == CARDANO_UPLC_EVAL_ERROR_TERM) || (status == CARDANO_UPLC_EVAL_OUT_OF_BUDGET);
}

/**
 * \brief Builds the cost model of a configuration.
 *
 * A configuration with a ledger parameter list selects its cost model through
 * \ref cardano_uplc_select_cost_model, which must pick the configuration's
 * semantics variant. Otherwise the model holds the builtin costs of cost model E
 * with the shapes of the semantics variant's cost model generation.
 *
 * \param[in] config The configuration.
 *
 * \return The cost model.
 */
cardano_uplc_cost_model_t
make_cost_model(const Configuration& config)
{
  if (config.params != nullptr)
  {
    cardano_uplc_selected_cost_model_t selected;
    std::memset(&selected, 0, sizeof(selected));

    EXPECT_EQ(
      cardano_uplc_select_cost_model(config.language, config.protocol_major, config.params, config.param_count, &selected),
      CARDANO_SUCCESS);
    EXPECT_EQ(selected.semantics, config.semantics);

    return selected.model;
  }

  cardano_uplc_cost_model_t model;
  model.machine  = cardano_uplc_machine_costs_default(config.cost_model_version);
  model.builtins = cardano_uplc_builtin_costs_v3();

  cardano_uplc_builtin_costs_select_division_shapes(&model.builtins, config.semantics);

  return model;
}

/**
 * \brief Evaluates a program under a configuration.
 *
 * \param[in] arena The arena owning the program.
 * \param[in] program The program.
 * \param[in] config The configuration.
 * \param[out] result The evaluation result.
 *
 * \return The host error of the evaluation.
 */
cardano_error_t
evaluate(
  cardano_uplc_arena_t*         arena,
  const cardano_uplc_program_t* program,
  const Configuration&          config,
  cardano_uplc_eval_result_t*   result)
{
  const cardano_uplc_cost_model_t model   = make_cost_model(config);
  const cardano_uplc_budget_t     initial = { BUDGET_CPU, BUDGET_MEM };

  return cardano_uplc_int_evaluate_with_costs(
    arena, program, &model, config.semantics, config.protocol_major, initial, result);
}

/**
 * \brief Decodes a flat program the way the reference harness does.
 *
 * Follows \c decodeFlatProg in plutus-conformance/src/PlutusConformance/Common.hs:
 * an empty input is a parse error without decoding, and a program with a free
 * variable fails the de Bruijn-to-names conversion that follows the decode, so it
 * is a parse error too.
 *
 * \param[in] arena The arena to decode into.
 * \param[in] bytes The flat bytes.
 * \param[out] program The decoded program.
 *
 * \return \ref CARDANO_SUCCESS, or the decoding error.
 */
cardano_error_t
decode_flat_program(cardano_uplc_arena_t* arena, const std::string& bytes, const cardano_uplc_program_t** program)
{
  cardano_uplc_flat_reader_t reader = { nullptr, 0U, 0U, 0U };

  if (bytes.empty())
  {
    return CARDANO_ERROR_DECODING;
  }

  cardano_error_t result =
    cardano_uplc_flat_reader_init(&reader, reinterpret_cast<const byte_t*>(bytes.data()), bytes.size());

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_flat_decode_program(arena, &reader, program);
  }

  if ((result == CARDANO_SUCCESS) && !cardano_uplc_term_is_closed((*program)->term, CARDANO_UPLC_SCOPE_CHECK_FULL))
  {
    result = CARDANO_ERROR_DECODING;
  }

  return result;
}

/**
 * \brief Reads a program in the given format.
 *
 * \param[in] arena The arena to read into.
 * \param[in] format The input format.
 * \param[in] input The program text or flat bytes.
 * \param[out] program The program.
 * \param[out] offset On a textual parse failure, the offset of the failure.
 *
 * \return \ref CARDANO_SUCCESS, or the parse or decoding error.
 */
cardano_error_t
read_program(
  cardano_uplc_arena_t*          arena,
  Format                         format,
  const std::string&             input,
  const cardano_uplc_program_t** program,
  size_t*                        offset)
{
  if (format == Format::FLAT)
  {
    return decode_flat_program(arena, input, program);
  }

  return cardano_uplc_parse_program(arena, input.c_str(), input.size(), program, offset);
}

/**
 * \brief Loads a case from its input path.
 *
 * A .flat case is compared against its .flat.expected golden, which holds either
 * the flat-encoded result program or the failure text, and against the same
 * .budget.expected as the .uplc case of the same directory.
 *
 * \param[in] input_path The .uplc or .flat input.
 *
 * \return The case.
 */
Case
load_case(const fs::path& input_path)
{
  const std::string input_str = input_path.string();
  const Format      format    = has_suffix(input_str, ".flat") ? Format::FLAT : Format::TEXTUAL;
  const std::string extension = (format == Format::FLAT) ? ".flat" : ".uplc";
  const std::string base      = input_str.substr(0U, input_str.size() - extension.size());

  Case loaded;
  loaded.format   = format;
  loaded.input    = read_file(input_str);
  loaded.expected = read_file(base + extension + ".expected");
  loaded.budget   = read_file(base + ".budget.expected");
  loaded.outcome  = classify(loaded.expected, loaded.budget);

  return loaded;
}

/**
 * \brief Collects every .uplc and .flat input under the corpus, sorted.
 *
 * The .expected files end in neither extension.
 *
 * \param[in] root The corpus directory.
 *
 * \return The inputs in a stable order.
 */
std::vector<fs::path>
collect_inputs(const std::string& root)
{
  std::vector<fs::path> inputs;

  for (const fs::directory_entry& entry: fs::recursive_directory_iterator(root))
  {
    const std::string name = entry.path().filename().string();

    if (entry.is_regular_file() && (has_suffix(name, ".uplc") || has_suffix(name, ".flat")))
    {
      inputs.push_back(entry.path());
    }
  }

  std::sort(inputs.begin(), inputs.end());

  return inputs;
}

/**
 * \brief Runs one case under a configuration.
 *
 * \param[in] test_case The case.
 * \param[in] config The configuration.
 *
 * \return An empty string when the case passes, or a description of why it failed.
 */
std::string
check_case(const Case& test_case, const Configuration& config)
{
  cardano_uplc_arena_t*         arena        = make_arena();
  const cardano_uplc_program_t* program      = nullptr;
  size_t                        parse_offset = 0U;
  const cardano_error_t         parse_error  = read_program(arena, test_case.format, test_case.input, &program, &parse_offset);

  std::ostringstream failure;

  if (test_case.outcome == Outcome::PARSE_ERROR)
  {
    if (parse_error == CARDANO_SUCCESS)
    {
      failure << "expected a parse error but the input parsed";
    }
  }
  else if (parse_error != CARDANO_SUCCESS)
  {
    failure << "input failed to parse at offset " << parse_offset;
  }
  else
  {
    cardano_uplc_eval_result_t result     = {};
    const cardano_error_t      host_error = evaluate(arena, program, config, &result);

    if (host_error != CARDANO_SUCCESS)
    {
      failure << "host error during evaluation";
    }
    else if (result.status == CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN)
    {
      failure << "evaluation hit an unimplemented builtin";
    }
    else if (test_case.outcome == Outcome::EVALUATION_FAILURE)
    {
      if (!is_script_failure(result.status))
      {
        failure << "expected a script failure but evaluation succeeded";
      }
    }
    else if (result.status != CARDANO_UPLC_EVAL_SUCCESS)
    {
      failure << "expected evaluation success but the script failed";
    }
    else
    {
      const cardano_uplc_program_t* expected_program = nullptr;
      size_t                        expected_offset  = 0U;
      const cardano_error_t         expected_parse =
        read_program(arena, test_case.format, test_case.expected, &expected_program, &expected_offset);

      if (expected_parse != CARDANO_SUCCESS)
      {
        failure << "could not parse the expected output";
      }
      else
      {
        const std::string actual_render   = render_term(result.result);
        const std::string expected_render = render_term(expected_program->term);

        if (actual_render != expected_render)
        {
          failure << "result term mismatch\n  expected: " << expected_render << "\n  actual:   " << actual_render;
        }

        int64_t want_cpu = 0;
        int64_t want_mem = 0;
        if (!parse_budget(test_case.budget, &want_cpu, &want_mem))
        {
          failure << "could not parse the expected budget: " << test_case.budget;
        }
        else if ((result.spent.cpu != want_cpu) || (result.spent.mem != want_mem))
        {
          failure << "budget mismatch\n  expected cpu/mem: " << want_cpu << "/" << want_mem
                  << "\n  actual   cpu/mem: " << result.spent.cpu << "/" << result.spent.mem;
        }
      }
    }
  }

  cardano_uplc_arena_free(&arena);

  return failure.str();
}

/**
 * \brief Whether a type is or contains an array or value type.
 *
 * \param[in] type The type, or NULL.
 *
 * \return \c true when an array or value type occurs in \p type.
 */
bool
type_is_newer(const cardano_uplc_type_t* type)
{
  if (type == nullptr)
  {
    return false;
  }

  if ((type->kind == CARDANO_UPLC_TYPE_ARRAY) || (type->kind == CARDANO_UPLC_TYPE_VALUE))
  {
    return true;
  }

  return type_is_newer(type->fst) || type_is_newer(type->snd);
}

/**
 * \brief Whether a constant is or contains an array or value constant.
 *
 * \param[in] constant The constant.
 *
 * \return \c true when an array or value type occurs in \p constant.
 */
bool
constant_is_newer(const cardano_uplc_constant_t* constant)
{
  if ((constant->kind == CARDANO_UPLC_TYPE_ARRAY) || (constant->kind == CARDANO_UPLC_TYPE_VALUE))
  {
    return true;
  }

  if (constant->kind == CARDANO_UPLC_TYPE_PAIR)
  {
    return constant_is_newer(constant->as.pair.fst) || constant_is_newer(constant->as.pair.snd);
  }

  if (constant->kind != CARDANO_UPLC_TYPE_LIST)
  {
    return false;
  }

  if (type_is_newer(constant->as.list.element_type))
  {
    return true;
  }

  for (size_t i = 0U; i < constant->as.list.count; ++i)
  {
    if (constant_is_newer(constant->as.list.items[i]))
    {
      return true;
    }
  }

  return false;
}

/**
 * \brief Collects the builtins, constants and case terms of a term.
 *
 * \param[in] root The term to walk.
 *
 * \return The features of \p root.
 */
Features
collect_features(const cardano_uplc_term_t* root)
{
  Features                                features = { {}, false, false };
  std::vector<const cardano_uplc_term_t*> pending  = { root };

  while (!pending.empty())
  {
    const cardano_uplc_term_t* term = pending.back();
    pending.pop_back();

    switch (term->kind)
    {
      case CARDANO_UPLC_TERM_VAR:
      case CARDANO_UPLC_TERM_ERROR:
      {
        break;
      }

      case CARDANO_UPLC_TERM_DELAY:
      case CARDANO_UPLC_TERM_LAMBDA:
      case CARDANO_UPLC_TERM_FORCE:
      {
        pending.push_back(term->as.unary);
        break;
      }

      case CARDANO_UPLC_TERM_APPLY:
      {
        pending.push_back(term->as.apply.function);
        pending.push_back(term->as.apply.argument);
        break;
      }

      case CARDANO_UPLC_TERM_CONSTANT:
      {
        features.has_newer_constant_type = features.has_newer_constant_type || constant_is_newer(term->as.constant);
        break;
      }

      case CARDANO_UPLC_TERM_BUILTIN:
      {
        features.builtins.insert(term->as.builtin);
        break;
      }

      case CARDANO_UPLC_TERM_CONSTR:
      {
        pending.insert(pending.end(), term->as.constr.fields, term->as.constr.fields + term->as.constr.field_count);
        break;
      }

      case CARDANO_UPLC_TERM_CASE:
      {
        features.has_case_on_non_constr = features.has_case_on_non_constr || (term->as.cases.scrutinee->kind != CARDANO_UPLC_TERM_CONSTR);
        pending.push_back(term->as.cases.scrutinee);
        pending.insert(pending.end(), term->as.cases.branches, term->as.cases.branches + term->as.cases.branch_count);
        break;
      }
    }
  }

  return features;
}

/**
 * \brief Whether any builtin of a set is also in another set.
 *
 * \param[in] builtins The builtins a program names.
 * \param[in] listed The builtins to look for.
 *
 * \return \c true when the sets intersect.
 */
bool
names_any(const std::set<cardano_uplc_builtin_t>& builtins, const std::set<cardano_uplc_builtin_t>& listed)
{
  for (const cardano_uplc_builtin_t builtin: builtins)
  {
    if (listed.count(builtin) != 0U)
    {
      return true;
    }
  }

  return false;
}

/**
 * \brief Whether every builtin of a set is available under a configuration.
 *
 * \param[in] builtins The builtins a program names.
 * \param[in] config The configuration.
 *
 * \return \c true when the configuration's language has every builtin at its
 *         protocol version.
 */
bool
all_available(const std::set<cardano_uplc_builtin_t>& builtins, const Configuration& config)
{
  return std::all_of(builtins.begin(), builtins.end(), [&config](const cardano_uplc_builtin_t builtin)
                     { return cardano_uplc_builtin_available(builtin, config.language, config.protocol_major); });
}

/**
 * \brief Decides whether a case keeps its expectation under the mainnet configuration.
 *
 * The rule is static: it reads the program and the expected outcome, never the
 * evaluation it decides about. A case is in the mainnet run when it parses, its
 * program version and every builtin it names are available to V3 at protocol
 * version 10, it holds no array or value constant, every case term scrutinises a
 * constr term directly (protocol version 10 has no case on builtin constants), it
 * names no builtin the mainnet configuration costs differently
 * (\ref MAINNET_COSTED_DIFFERENTLY), and, when it expects a failure, it names no
 * builtin whose arguments variant G bounds (\ref VARIANT_G_BOUNDED).
 *
 * \param[in] test_case The case.
 *
 * \return \ref MainnetExclusion::NONE when the case is in the mainnet run, or the
 *         first reason it is left out.
 */
MainnetExclusion
mainnet_exclusion(const Case& test_case)
{
  if (test_case.outcome == Outcome::PARSE_ERROR)
  {
    return MainnetExclusion::PARSE_ERROR;
  }

  const Configuration&          config  = MAINNET_CONFIGURATION;
  cardano_uplc_arena_t*         arena   = make_arena();
  const cardano_uplc_program_t* program = nullptr;
  size_t                        offset  = 0U;
  MainnetExclusion              reason  = MainnetExclusion::NONE;

  if (read_program(arena, test_case.format, test_case.input, &program, &offset) != CARDANO_SUCCESS)
  {
    reason = MainnetExclusion::PARSE_ERROR;
  }
  else if (!cardano_uplc_plc_version_available(
             config.language, config.protocol_major, program->version_major, program->version_minor, program->version_patch))
  {
    reason = MainnetExclusion::PROGRAM_VERSION;
  }
  else
  {
    const Features features = collect_features(program->term);

    if (!all_available(features.builtins, config))
    {
      reason = MainnetExclusion::BUILTIN;
    }
    else if (features.has_newer_constant_type)
    {
      reason = MainnetExclusion::CONSTANT_TYPE;
    }
    else if (features.has_case_on_non_constr)
    {
      reason = MainnetExclusion::CASE_ON_NON_CONSTR;
    }
    else if (names_any(features.builtins, MAINNET_COSTED_DIFFERENTLY))
    {
      reason = MainnetExclusion::COSTING;
    }
    else if ((test_case.outcome == Outcome::EVALUATION_FAILURE) && names_any(features.builtins, VARIANT_G_BOUNDED))
    {
      reason = MainnetExclusion::BOUNDS;
    }
  }

  cardano_uplc_arena_free(&arena);

  return reason;
}

/**
 * \brief Names a mainnet exclusion reason for the summary.
 *
 * \param[in] reason The reason.
 *
 * \return The reason's name.
 */
std::string
exclusion_name(MainnetExclusion reason)
{
  switch (reason)
  {
    case MainnetExclusion::NONE:
      return "none";
    case MainnetExclusion::PARSE_ERROR:
      return "parse-error";
    case MainnetExclusion::PROGRAM_VERSION:
      return "program-version";
    case MainnetExclusion::BUILTIN:
      return "builtin";
    case MainnetExclusion::CONSTANT_TYPE:
      return "constant-type";
    case MainnetExclusion::CASE_ON_NON_CONSTR:
      return "case-on-non-constr";
    case MainnetExclusion::COSTING:
      return "costing";
    case MainnetExclusion::BOUNDS:
      return "bounds";
  }

  return "unknown";
}

/**
 * \brief Builds a textual case from a program and its expected outcome text.
 *
 * \param[in] program The program text.
 * \param[in] expected The expected outcome text.
 *
 * \return The case.
 */
Case
textual_case(const std::string& program, const std::string& expected)
{
  Case built;
  built.format   = Format::TEXTUAL;
  built.input    = program;
  built.expected = expected;
  built.outcome  = classify(expected, std::string());

  return built;
}

} // namespace

/* CONFORMANCE RUNNER ********************************************************/

TEST(cardano_uplc_conformance, fullCorpusPassesAtTheNewestProtocolVersion)
{
  // Arrange
  const std::string root = corpus_root();
  ASSERT_TRUE(fs::exists(root)) << "corpus directory not found: " << root;

  const std::vector<fs::path> inputs = collect_inputs(root);
  ASSERT_GT(inputs.size(), 0U) << "no corpus inputs were found under " << root;

  size_t textual = 0U;
  size_t flat    = 0U;
  size_t passed  = 0U;
  size_t failed  = 0U;

  // Act
  for (const fs::path& input: inputs)
  {
    const Case        test_case = load_case(input);
    const std::string failure   = check_case(test_case, NEWEST_CONFIGURATION);

    textual += (test_case.format == Format::TEXTUAL) ? 1U : 0U;
    flat    += (test_case.format == Format::FLAT) ? 1U : 0U;

    if (failure.empty())
    {
      passed += 1U;
    }
    else
    {
      failed += 1U;
      ADD_FAILURE() << fs::relative(input, root).generic_string() << "\n  " << failure;
    }
  }

  std::cout << "[ CONFORMANCE SUMMARY ] configuration=pv" << NEWEST_CONFIGURATION.protocol_major << " total=" << inputs.size()
            << " uplc=" << textual << " flat=" << flat << " passed=" << passed << " failed=" << failed << std::endl;

  // Assert
  EXPECT_EQ(failed, 0U);
  EXPECT_EQ(passed, inputs.size());
  EXPECT_EQ(textual, CORPUS_TEXTUAL_CASE_COUNT);
  EXPECT_EQ(flat, CORPUS_FLAT_CASE_COUNT);
}

TEST(cardano_uplc_conformance, unchangedCasesPassAtTheMainnetProtocolVersion)
{
  // Arrange
  const std::string root = corpus_root();
  ASSERT_TRUE(fs::exists(root)) << "corpus directory not found: " << root;

  const std::vector<fs::path> inputs = collect_inputs(root);
  ASSERT_GT(inputs.size(), 0U) << "no corpus inputs were found under " << root;

  std::map<std::string, size_t> excluded;
  size_t                        covered = 0U;
  size_t                        passed  = 0U;
  size_t                        failed  = 0U;

  // Act
  for (const fs::path& input: inputs)
  {
    const Case             test_case = load_case(input);
    const MainnetExclusion reason    = mainnet_exclusion(test_case);

    if (reason != MainnetExclusion::NONE)
    {
      excluded[exclusion_name(reason)] += 1U;
      continue;
    }

    covered += 1U;

    const std::string failure = check_case(test_case, MAINNET_CONFIGURATION);

    if (failure.empty())
    {
      passed += 1U;
    }
    else
    {
      failed += 1U;
      ADD_FAILURE() << fs::relative(input, root).generic_string() << "\n  " << failure;
    }
  }

  std::cout << "[ CONFORMANCE SUMMARY ] configuration=pv" << MAINNET_CONFIGURATION.protocol_major << " total=" << inputs.size()
            << " covered=" << covered << " passed=" << passed << " failed=" << failed << std::endl;

  for (const auto& reason: excluded)
  {
    std::cout << "[ CONFORMANCE SUMMARY ]   excluded " << reason.first << "=" << reason.second << std::endl;
  }

  // Assert
  EXPECT_EQ(failed, 0U);
  EXPECT_EQ(passed, covered);
  EXPECT_EQ(covered, MAINNET_CASE_COUNT);
}

TEST(cardano_uplc_conformance, mainnetRuleKeepsCasesWithUnchangedExpectations)
{
  // Arrange
  const Case arithmetic = textual_case(
    "(program 1.0.0 [(builtin addInteger) (con integer 1) (con integer 2)])",
    "(program 1.0.0 (con integer 3))");
  const Case case_on_constr = textual_case(
    "(program 1.1.0 (case (constr 0 (con integer 0)) (lam x x)))",
    "(program 1.1.0 (con integer 0))");
  const Case failing_without_bounds = textual_case(
    "(program 1.0.0 [(builtin addInteger) (con integer 1) (con bool True)])",
    "evaluation failure");

  // Act
  const MainnetExclusion arithmetic_reason = mainnet_exclusion(arithmetic);
  const MainnetExclusion constr_reason     = mainnet_exclusion(case_on_constr);
  const MainnetExclusion failing_reason    = mainnet_exclusion(failing_without_bounds);

  // Assert
  EXPECT_EQ(arithmetic_reason, MainnetExclusion::NONE);
  EXPECT_EQ(constr_reason, MainnetExclusion::NONE);
  EXPECT_EQ(failing_reason, MainnetExclusion::NONE);
}

TEST(cardano_uplc_conformance, mainnetRuleExcludesCasesWithChangedExpectations)
{
  // Arrange
  const Case parse_error   = textual_case("(program 1.0.0 (con integer", "parse/decode error");
  const Case newer_version = textual_case("(program 1.2.0 (con integer 1))", "(program 1.2.0 (con integer 1))");
  const Case newer_builtin = textual_case(
    "(program 1.1.0 [(builtin dropList) (con integer 1) (con (list integer) [1, 2])])",
    "(program 1.1.0 (con (list integer) [2]))");
  const Case newer_constant = textual_case(
    "(program 1.1.0 (con (list (array integer)) []))",
    "(program 1.1.0 (con (list (array integer)) []))");
  const Case case_on_constant = textual_case(
    "(program 1.1.0 (case (con bool True) (con integer 0) (con integer 1)))",
    "(program 1.1.0 (con integer 1))");
  const Case costed_differently = textual_case(
    "(program 1.0.0 [(builtin divideInteger) (con integer 7) (con integer 2)])",
    "(program 1.0.0 (con integer 3))");
  const Case bounded_failure = textual_case(
    "(program 1.0.0 [(builtin shiftByteString) (con bytestring #00) (con integer 9223372036854775808)])",
    "evaluation failure");

  // Act
  const MainnetExclusion parse_reason    = mainnet_exclusion(parse_error);
  const MainnetExclusion version_reason  = mainnet_exclusion(newer_version);
  const MainnetExclusion builtin_reason  = mainnet_exclusion(newer_builtin);
  const MainnetExclusion constant_reason = mainnet_exclusion(newer_constant);
  const MainnetExclusion case_reason     = mainnet_exclusion(case_on_constant);
  const MainnetExclusion costing_reason  = mainnet_exclusion(costed_differently);
  const MainnetExclusion bounds_reason   = mainnet_exclusion(bounded_failure);

  // Assert
  EXPECT_EQ(parse_reason, MainnetExclusion::PARSE_ERROR);
  EXPECT_EQ(version_reason, MainnetExclusion::PROGRAM_VERSION);
  EXPECT_EQ(builtin_reason, MainnetExclusion::BUILTIN);
  EXPECT_EQ(constant_reason, MainnetExclusion::CONSTANT_TYPE);
  EXPECT_EQ(case_reason, MainnetExclusion::CASE_ON_NON_CONSTR);
  EXPECT_EQ(costing_reason, MainnetExclusion::COSTING);
  EXPECT_EQ(bounds_reason, MainnetExclusion::BOUNDS);
}
