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

#include <cardano/buffer.h>
#include <cardano/error.h>
#include "../../src/uplc/machine/uplc_machine.h"
#include "../../src/uplc/ast/uplc_term.h"

#include "../../src/uplc/arena/uplc_arena.h"
#include "../../src/uplc/syntax/pretty.h"
#include "../../src/uplc/syntax/text_parser.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* STATIC HELPERS ************************************************************/

namespace {

namespace fs = std::filesystem;

// The conformance corpus uses the V3 default machine-step cost table for its
// budgets, independent of the program version header in each input, so every
// case is evaluated under V3.
const cardano_uplc_machine_version_t MACHINE_VERSION = CARDANO_UPLC_MACHINE_VERSION_V3;

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
  PARSE_ERROR,       // the input is expected to fail parsing
  EVALUATION_FAILURE, // evaluation is expected to fail (error term / out of budget)
  SUCCESS             // a result term and an exact budget are expected
};

std::string
corpus_root()
{
  // Derive the corpus path from this source file's location so the test is
  // independent of the working directory the binary is launched from.
  fs::path file = fs::path(__FILE__);
  return (file.parent_path() / "vectors").string();
}

std::string
read_file(const std::string& path)
{
  std::ifstream    stream(path, std::ios::binary);
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
  if (contains(expected_text, "parse error") || contains(budget_text, "parse error"))
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

struct Counts
{
  size_t total              = 0U;
  size_t passed             = 0U;
  size_t skipped_builtin    = 0U;
  size_t skipped_unsupported = 0U;
  size_t failed             = 0U;
};

void
run_case(const fs::path& input_path, Counts& counts)
{
  const std::string input_str  = input_path.string();
  const std::string base       = input_str.substr(0U, input_str.size() - 5U); // strip ".uplc"
  const std::string input_text = read_file(input_str);
  const std::string expected   = read_file(base + ".uplc.expected");
  const std::string budget     = read_file(base + ".uplc.budget.expected");

  SCOPED_TRACE(input_str);

  const cardano_uplc_budget_t initial = { BUDGET_CPU, BUDGET_MEM };
  const Outcome               outcome = classify(expected, budget);

  cardano_uplc_arena_t*         arena       = make_arena();
  const cardano_uplc_program_t* program     = nullptr;
  size_t                        parse_offset = 0U;
  const cardano_error_t parse_error =
    cardano_uplc_parse_program(arena, input_text.c_str(), input_text.size(), &program, &parse_offset);

  bool case_passed = false;

  switch (outcome)
  {
    case Outcome::PARSE_ERROR:
    {
      // The input must fail to parse.
      EXPECT_NE(parse_error, CARDANO_SUCCESS) << "expected a parse error but the input parsed";
      case_passed = (parse_error != CARDANO_SUCCESS);
      break;
    }

    case Outcome::EVALUATION_FAILURE:
    {
      // The input must parse, then evaluation must be a script failure. Every
      // builtin is implemented, so an UNSUPPORTED_BUILTIN signal is a failure.
      EXPECT_EQ(parse_error, CARDANO_SUCCESS) << "input failed to parse at offset " << parse_offset;
      if (parse_error != CARDANO_SUCCESS)
      {
        break;
      }

      cardano_uplc_eval_result_t result = {};
      const cardano_error_t      host_error =
        cardano_uplc_evaluate(arena, program, MACHINE_VERSION, initial, &result);

      EXPECT_NE(result.status, CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN)
        << "evaluation hit an unimplemented builtin: " << input_str;
      EXPECT_EQ(host_error, CARDANO_SUCCESS) << "host error during evaluation";
      EXPECT_TRUE(is_script_failure(result.status))
        << "expected a script failure but evaluation succeeded";

      case_passed = (host_error == CARDANO_SUCCESS) && is_script_failure(result.status);
      break;
    }

    case Outcome::SUCCESS:
    {
      // The input must parse and evaluate to SUCCESS, the result term must match
      // the expected term structurally, and the spent budget must match exactly.
      // Every builtin is implemented, so an UNSUPPORTED_BUILTIN signal is a failure.
      EXPECT_EQ(parse_error, CARDANO_SUCCESS) << "input failed to parse at offset " << parse_offset;
      if (parse_error != CARDANO_SUCCESS)
      {
        break;
      }

      cardano_uplc_eval_result_t result = {};
      const cardano_error_t      host_error =
        cardano_uplc_evaluate(arena, program, MACHINE_VERSION, initial, &result);

      EXPECT_NE(result.status, CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN)
        << "evaluation hit an unimplemented builtin: " << input_str;
      EXPECT_EQ(host_error, CARDANO_SUCCESS) << "host error during evaluation: " << input_str;
      EXPECT_EQ(result.status, CARDANO_UPLC_EVAL_SUCCESS) << "expected evaluation success: " << input_str;
      if ((host_error != CARDANO_SUCCESS) || (result.status != CARDANO_UPLC_EVAL_SUCCESS))
      {
        break;
      }

      const cardano_uplc_program_t* expected_program = nullptr;
      size_t                        expected_offset  = 0U;
      const cardano_error_t         expected_parse =
        cardano_uplc_parse_program(arena, expected.c_str(), expected.size(), &expected_program, &expected_offset);
      EXPECT_EQ(expected_parse, CARDANO_SUCCESS) << "could not parse the expected output";
      if (expected_parse != CARDANO_SUCCESS)
      {
        break;
      }

      const std::string actual_render   = render_term(result.result);
      const std::string expected_render = render_term(expected_program->term);

      const bool term_ok = (actual_render == expected_render);
      EXPECT_TRUE(term_ok) << "result term mismatch\n  expected: " << expected_render
                           << "\n  actual:   " << actual_render;

      int64_t    want_cpu = 0;
      int64_t    want_mem = 0;
      const bool budget_parsed = parse_budget(budget, &want_cpu, &want_mem);
      EXPECT_TRUE(budget_parsed) << "could not parse the expected budget: " << budget;

      bool budget_ok = false;
      if (budget_parsed)
      {
        budget_ok = (result.spent.cpu == want_cpu) && (result.spent.mem == want_mem);
        EXPECT_TRUE(budget_ok) << "budget mismatch\n  expected cpu/mem: " << want_cpu << "/" << want_mem
                               << "\n  actual   cpu/mem: " << result.spent.cpu << "/" << result.spent.mem;
      }

      case_passed = term_ok && budget_parsed && budget_ok;
      break;
    }

    default:
    {
      ADD_FAILURE() << "unreachable outcome";
      break;
    }
  }

  if (case_passed)
  {
    counts.passed += 1U;
  }
  else
  {
    counts.failed += 1U;
  }

  cardano_uplc_arena_free(&arena);
}

} // namespace

/* CONFORMANCE RUNNER ********************************************************/

TEST(cardano_uplc_conformance, fullCorpusIsGreenOnResultAndBudgetWithZeroSkips)
{
  // Arrange: collect every *.uplc input under the corpus, excluding the
  // .uplc.expected and .uplc.budget.expected sidecar files.
  const std::string root = corpus_root();
  ASSERT_TRUE(fs::exists(root)) << "corpus directory not found: " << root;

  std::vector<fs::path> inputs;
  for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root))
  {
    if (!entry.is_regular_file())
    {
      continue;
    }

    const std::string name = entry.path().filename().string();
    if (name.size() < 5U)
    {
      continue;
    }

    const bool is_uplc     = (name.compare(name.size() - 5U, 5U, ".uplc") == 0);
    if (!is_uplc)
    {
      continue;
    }

    inputs.push_back(entry.path());
  }

  ASSERT_GT(inputs.size(), 0U) << "no corpus inputs were found under " << root;

  // Act + Assert: run every case, accumulating outcome counts.
  Counts counts;
  counts.total = inputs.size();

  for (const fs::path& input : inputs)
  {
    run_case(input, counts);
  }

  const size_t skipped = counts.skipped_builtin + counts.skipped_unsupported;

  // Report a single visible summary line.
  std::cout << "[ CONFORMANCE SUMMARY ] total=" << counts.total << " passed=" << counts.passed
            << " skipped_builtin=" << counts.skipped_builtin
            << " skipped_unsupported=" << counts.skipped_unsupported << " failed=" << counts.failed
            << std::endl;

  EXPECT_EQ(skipped, 0U) << skipped << " case(s) were skipped; the corpus must run with zero skips";
  EXPECT_EQ(counts.failed, 0U) << counts.failed << " case(s) failed; see traces above";
  EXPECT_EQ(counts.passed, counts.total) << "not every case passed: passed=" << counts.passed
                                         << " total=" << counts.total;

  const size_t accounted = counts.passed + counts.skipped_builtin + counts.skipped_unsupported + counts.failed;
  EXPECT_EQ(accounted, counts.total) << "some cases were neither passed, skipped, nor failed";
}
