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
#include "../../src/uplc/syntax/pretty.h"
#include "../../src/uplc/syntax/text_parser.h"

#include "../../src/uplc/cost/uplc_builtin_costs.h"
#include "../../src/uplc/cost/uplc_cost_model.h"
#include "../../src/uplc/cost/uplc_machine_costs.h"

#include <algorithm>
#include <cstdint>
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
 * \brief The complete evaluation configuration shared by every corpus case.
 *
 * The reference evaluates every case with one language, protocol version,
 * builtin semantics variant and cost model, independent of the program version
 * header in each input.
 */
struct Configuration
{
    cardano_uplc_lang_version_t       language;
    uint64_t                          protocol_major;
    cardano_uplc_builtin_semantics_t  semantics;
    cardano_uplc_cost_model_version_t cost_model_version;
};

// The reference runs the corpus at the newest protocol version (12) with builtin
// semantics variant G and the newest cost model. Neither variant G nor the V4
// cost model exist yet, so the closest available values are used: the newest
// language, the newest implemented semantics variant (E) and the V3 cost model.
// Cases that depend on the missing behaviour are listed as expected failures.
const Configuration CONFIGURATION = {
  CARDANO_UPLC_LANG_VERSION_V4,
  12U,
  CARDANO_UPLC_SEMANTICS_E,
  CARDANO_UPLC_COST_MODEL_VERSION_V3
};

// The conformance corpus carries the exact budget every supported case must
// spend, and some V4 cases (the saturating dropList requests) spend the full
// int64 budget and still succeed: the reference runs them under an INT64_MAX
// ceiling where the saturated spend equals the limit and stays in budget. The
// ceiling is therefore INT64_MAX so those saturating-but-successful cases are
// reproduced; the corpus has no budget-exhaustion failure case that depends on
// a lower ceiling (its failures are all error-term failures).
const int64_t BUDGET_CPU = INT64_MAX;
const int64_t BUDGET_MEM = INT64_MAX;

// The expected outcome class derived from the .uplc.expected / budget text.
enum class Outcome
{
  PARSE_ERROR,        // the input is expected to fail parsing or decoding
  EVALUATION_FAILURE, // evaluation is expected to fail (error term / out of budget)
  SUCCESS             // a result term and an exact budget are expected
};

// The fixed set of gap categories an expected failure may be tagged with.
const std::set<std::string> CATEGORIES = {
  "semantics-variant",
  "builtin-bounds",
  "division-costing",
  "cost-model",
  "flat-decoding",
  "version-gating",
  "batch7-builtins",
  "case-on-data",
  "text-parser",
  "other"
};

std::string
corpus_root()
{
  // Derive the corpus path from this source file's location so the test is
  // independent of the working directory the binary is launched from.
  fs::path file = fs::path(__FILE__);
  return (file.parent_path() / "vectors").string();
}

// Returns the path of the expected-failure list next to this source file.
std::string
expected_failures_path()
{
  fs::path file = fs::path(__FILE__);
  return (file.parent_path() / "conformance_expected_failures.txt").string();
}

std::string
read_file(const std::string& path)
{
  std::ifstream      stream(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

cardano_uplc_arena_t*
make_arena()
{
  cardano_uplc_arena_t* arena = nullptr;
  EXPECT_EQ(cardano_uplc_arena_new(0U, &arena), CARDANO_SUCCESS);
  return arena;
}

bool
contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

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

// Renders a term to the canonical surface syntax for structural (alpha)
// comparison and for diagnostics. Returns an empty string on failure.
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

  // The printer NUL-terminates; trim a trailing NUL byte if present so string
  // comparison is exact.
  while (!out.empty() && (out.back() == '\0'))
  {
    out.pop_back();
  }

  return out;
}

// Parses the "({cpu: N\n| mem: N})" budget format. Returns true on success.
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

bool
is_script_failure(cardano_uplc_eval_status_t status)
{
  return (status == CARDANO_UPLC_EVAL_ERROR_TERM) || (status == CARDANO_UPLC_EVAL_OUT_OF_BUDGET);
}

// Builds the cost model for the configuration.
cardano_uplc_cost_model_t
make_cost_model(const Configuration& config)
{
  cardano_uplc_cost_model_t model;
  model.machine = cardano_uplc_machine_costs_default(config.cost_model_version);

  switch (config.cost_model_version)
  {
    case CARDANO_UPLC_COST_MODEL_VERSION_V1:
    {
      model.builtins = cardano_uplc_builtin_costs_v1();
      break;
    }

    case CARDANO_UPLC_COST_MODEL_VERSION_V2:
    {
      model.builtins = cardano_uplc_builtin_costs_v2();
      break;
    }

    case CARDANO_UPLC_COST_MODEL_VERSION_V3:
    default:
    {
      model.builtins = cardano_uplc_builtin_costs_v3();
      break;
    }
  }

  return model;
}

// Evaluates a parsed program under the corpus configuration.
cardano_error_t
evaluate(
  cardano_uplc_arena_t*         arena,
  const cardano_uplc_program_t* program,
  cardano_uplc_eval_result_t*   result)
{
  const cardano_uplc_cost_model_t model   = make_cost_model(CONFIGURATION);
  const cardano_uplc_budget_t     initial = { BUDGET_CPU, BUDGET_MEM };

  return cardano_uplc_int_evaluate_with_costs(
    arena, program, &model, CONFIGURATION.semantics, CONFIGURATION.language, CONFIGURATION.protocol_major, initial, result);
}

// Runs one case and returns the empty string when it passes, or a description of
// why it failed.
std::string
check_case(const fs::path& input_path)
{
  const std::string input_str  = input_path.string();
  const std::string base       = input_str.substr(0U, input_str.size() - 5U); // strip ".uplc"
  const std::string input_text = read_file(input_str);
  const std::string expected   = read_file(base + ".uplc.expected");
  const std::string budget     = read_file(base + ".budget.expected");

  const Outcome outcome = classify(expected, budget);

  cardano_uplc_arena_t*         arena        = make_arena();
  const cardano_uplc_program_t* program      = nullptr;
  size_t                        parse_offset = 0U;
  const cardano_error_t         parse_error =
    cardano_uplc_parse_program(arena, input_text.c_str(), input_text.size(), &program, &parse_offset);

  std::ostringstream failure;

  if (outcome == Outcome::PARSE_ERROR)
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
    const cardano_error_t      host_error = evaluate(arena, program, &result);

    if (host_error != CARDANO_SUCCESS)
    {
      failure << "host error during evaluation";
    }
    else if (result.status == CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN)
    {
      failure << "evaluation hit an unimplemented builtin";
    }
    else if (outcome == Outcome::EVALUATION_FAILURE)
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
        cardano_uplc_parse_program(arena, expected.c_str(), expected.size(), &expected_program, &expected_offset);

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
        if (!parse_budget(budget, &want_cpu, &want_mem))
        {
          failure << "could not parse the expected budget: " << budget;
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

// Loads the checked-in expected-failure list as a map from corpus-relative case
// path to gap category. Each non-comment line is "<path> <category>[ <reason>]".
// Malformed lines, unknown categories and duplicate paths are recorded in
// \p problems.
std::map<std::string, std::string>
load_expected_failures(std::vector<std::string>& problems)
{
  std::map<std::string, std::string> entries;
  std::ifstream                      stream(expected_failures_path());
  std::string                        line;

  while (std::getline(stream, line))
  {
    if (line.empty() || (line[0] == '#'))
    {
      continue;
    }

    std::istringstream fields(line);
    std::string        path;
    std::string        category;
    fields >> path >> category;

    if (path.empty() || (CATEGORIES.count(category) == 0U))
    {
      problems.push_back("malformed expected-failure entry: " + line);
      continue;
    }

    if (!entries.emplace(path, category).second)
    {
      problems.push_back("duplicate expected-failure entry: " + path);
    }
  }

  return entries;
}

} // namespace

/* CONFORMANCE RUNNER ********************************************************/

TEST(cardano_uplc_conformance, fullCorpusMatchesTheExpectedFailureList)
{
  // Arrange: collect every *.uplc input under the corpus, sorted so reports are
  // stable. The *.uplc.expected sidecars do not end in ".uplc".
  const std::string root = corpus_root();
  ASSERT_TRUE(fs::exists(root)) << "corpus directory not found: " << root;

  std::vector<fs::path> inputs;
  for (const fs::directory_entry& entry: fs::recursive_directory_iterator(root))
  {
    if (!entry.is_regular_file())
    {
      continue;
    }

    const std::string name = entry.path().filename().string();
    if ((name.size() >= 5U) && (name.compare(name.size() - 5U, 5U, ".uplc") == 0))
    {
      inputs.push_back(entry.path());
    }
  }

  ASSERT_GT(inputs.size(), 0U) << "no corpus inputs were found under " << root;
  std::sort(inputs.begin(), inputs.end());

  std::vector<std::string>           problems;
  std::map<std::string, std::string> expected_failures = load_expected_failures(problems);
  for (const std::string& problem: problems)
  {
    ADD_FAILURE() << problem;
  }

  // Act: run every case and compare its outcome with the list.
  size_t                        passed     = 0U;
  size_t                        known_fail = 0U;
  size_t                        unexpected = 0U;
  size_t                        stale      = 0U;
  std::map<std::string, size_t> per_category;

  for (const fs::path& input: inputs)
  {
    const std::string relative = fs::relative(input, root).generic_string();
    const std::string failure  = check_case(input);
    const auto        listed   = expected_failures.find(relative);

    if (listed == expected_failures.end())
    {
      if (failure.empty())
      {
        passed += 1U;
      }
      else
      {
        unexpected += 1U;
        ADD_FAILURE() << "unexpected failure: " << relative << "\n  " << failure;
      }
      continue;
    }

    if (failure.empty())
    {
      stale += 1U;
      ADD_FAILURE() << "stale expected-failure entry (the case now passes): " << relative;
    }
    else
    {
      known_fail                   += 1U;
      per_category[listed->second] += 1U;
    }

    expected_failures.erase(listed);
  }

  size_t unknown = 0U;

  for (const auto& leftover: expected_failures)
  {
    unknown += 1U;
    ADD_FAILURE() << "expected-failure entry names no corpus case: " << leftover.first;
  }

  // Assert: report a single visible summary line.
  std::cout << "[ CONFORMANCE SUMMARY ] total=" << inputs.size() << " passed=" << passed
            << " expected_failures=" << known_fail << " unexpected_failures=" << unexpected << " stale=" << stale
            << " unknown_entries=" << unknown
            << std::endl;

  for (const auto& category: per_category)
  {
    std::cout << "[ CONFORMANCE SUMMARY ]   " << category.first << "=" << category.second << std::endl;
  }

  EXPECT_EQ(unexpected, 0U) << "cases failed that are not in the expected-failure list";
  EXPECT_EQ(stale, 0U) << "listed cases now pass and their entries must be removed";
  EXPECT_EQ(unknown, 0U) << "expected-failure entries name no corpus case";
}
