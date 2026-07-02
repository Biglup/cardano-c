/**
 * \file bench_run.c
 *
 * \author angel.castillo
 * \date   Jul 02 2026
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

#include "bench_run.h"

#include "utils/bench_io.h"
#include "utils/bench_stats.h"

#include "uplc/arena/uplc_arena.h"
#include "uplc/ast/uplc_program.h"
#include "uplc/flat/flat_decode.h"
#include "uplc/flat/flat_reader.h"
#include "uplc/machine/uplc_machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Number of untimed iterations run before measuring.
 */
static const int WARMUP_ITERATIONS = 5;

/**
 * \brief Minimum number of measured iterations per script.
 */
static const size_t MIN_ITERATIONS = 50U;

/**
 * \brief Time budget of the measured phase per script, in nanoseconds.
 */
static const uint64_t TIME_BUDGET_NS = 5000000000ULL;

/**
 * \brief Hard cap on measured iterations per script.
 */
static const size_t MAX_ITERATIONS = 10000U;

/**
 * \brief Length of the ".flat" file name suffix.
 */
static const size_t FLAT_SUFFIX_LEN = 5U;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Reads the current monotonic clock in nanoseconds.
 *
 * \return Nanoseconds from an arbitrary fixed origin, suitable for measuring
 *         elapsed time.
 */
static uint64_t
now_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

/**
 * \brief Runs one full benchmark iteration: flat-decode then CEK evaluation.
 *
 * Evaluates under Plutus V3 semantics with an effectively unlimited budget,
 * matching the benchmark suite's methodology.
 *
 * \param[in] arena The arena serving every interior allocation of the
 *            iteration; the caller resets it between iterations.
 * \param[in] bytes The raw flat bytes of the script.
 * \param[in] size The number of bytes in \p bytes.
 * \param[out] result The script outcome and spent budget.
 *
 * \return 0 when the host decoded and evaluated the script (whatever the
 *         script outcome), or -1 on a decode or evaluation host error.
 */
static int
evaluate_script(cardano_uplc_arena_t* arena, const byte_t* bytes, const size_t size, cardano_uplc_eval_result_t* result)
{
  cardano_uplc_flat_reader_t    reader;
  const cardano_uplc_program_t* program = NULL;

  if (cardano_uplc_flat_reader_init(&reader, bytes, size) != CARDANO_SUCCESS)
  {
    return -1;
  }

  if (cardano_uplc_flat_decode_program(arena, &reader, &program) != CARDANO_SUCCESS)
  {
    return -1;
  }

  const cardano_uplc_budget_t budget = { INT64_MAX, INT64_MAX };

  if (cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, result) != CARDANO_SUCCESS)
  {
    return -1;
  }

  return 0;
}

/**
 * \brief Copies a file name into a result, stripping the ".flat" suffix.
 *
 * \param[in] file_name The file name to copy.
 * \param[out] out The result whose name field receives the script name.
 */
static void
set_result_name(const char* file_name, cardano_bench_result_t* out)
{
  snprintf(out->name, sizeof(out->name), "%s", file_name);

  const size_t name_len = strlen(out->name);

  if ((name_len > FLAT_SUFFIX_LEN) && (strcmp(&out->name[name_len - FLAT_SUFFIX_LEN], ".flat") == 0))
  {
    out->name[name_len - FLAT_SUFFIX_LEN] = '\0';
  }
}

/**
 * \brief Checks that a script decodes and evaluates before it is benchmarked.
 *
 * \param[in] arena The arena to run the check in; reset afterwards.
 * \param[in] bytes The raw flat bytes of the script.
 * \param[in] size The number of bytes in \p bytes.
 *
 * \return 0 when the script is benchmarkable, -1 otherwise.
 */
static int
check_script_runs(cardano_uplc_arena_t* arena, const byte_t* bytes, const size_t size)
{
  cardano_uplc_eval_result_t result = { 0 };

  const int status = evaluate_script(arena, bytes, size, &result);

  cardano_uplc_arena_reset(arena);

  return status;
}

/**
 * \brief Runs the untimed warmup iterations of one script.
 *
 * \param[in] arena The arena serving the iterations; reset after each one.
 * \param[in] bytes The raw flat bytes of the script.
 * \param[in] size The number of bytes in \p bytes.
 */
static void
warm_up(cardano_uplc_arena_t* arena, const byte_t* bytes, const size_t size)
{
  for (int i = 0; i < WARMUP_ITERATIONS; ++i)
  {
    cardano_uplc_eval_result_t result = { 0 };

    (void)evaluate_script(arena, bytes, size, &result);
    cardano_uplc_arena_reset(arena);
  }
}

/**
 * \brief Runs the measured iterations of one script.
 *
 * Iterates until at least \ref MIN_ITERATIONS samples have been taken and
 * \ref TIME_BUDGET_NS nanoseconds have elapsed, capped at
 * \ref MAX_ITERATIONS. Each sample times one full decode + evaluate; the
 * arena reset between iterations is left outside the timed region.
 *
 * \param[in] arena The arena serving the iterations.
 * \param[in] bytes The raw flat bytes of the script.
 * \param[in] size The number of bytes in \p bytes.
 * \param[out] samples Receives one duration per iteration; must hold
 *             \ref MAX_ITERATIONS entries.
 *
 * \return The number of samples taken.
 */
static size_t
measure(cardano_uplc_arena_t* arena, const byte_t* bytes, const size_t size, uint64_t* samples)
{
  size_t   count         = 0U;
  uint64_t total_elapsed = 0U;

  while ((count < MIN_ITERATIONS) || (total_elapsed < TIME_BUDGET_NS))
  {
    cardano_uplc_eval_result_t result = { 0 };

    const uint64_t start = now_ns();
    (void)evaluate_script(arena, bytes, size, &result);
    const uint64_t elapsed = now_ns() - start;

    cardano_uplc_arena_reset(arena);

    samples[count] = elapsed;
    ++count;
    total_elapsed += elapsed;

    if (count >= MAX_ITERATIONS)
    {
      break;
    }
  }

  return count;
}

/**
 * \brief Fills a benchmark result from raw duration samples.
 *
 * \param[in,out] samples The duration samples; sorted by the computation.
 * \param[in] count The number of samples.
 * \param[out] out The result receiving the iteration count and statistics.
 */
static void
fill_result(uint64_t* samples, const size_t count, cardano_bench_result_t* out)
{
  const cardano_bench_stats_t stats = cardano_bench_stats_compute(samples, count);

  out->iterations = count;
  out->mean_ns    = stats.mean_ns;
  out->median_ns  = stats.median_ns;
  out->min_ns     = stats.min_ns;
  out->max_ns     = stats.max_ns;
  out->stddev_ns  = stats.stddev_ns;
}

/**
 * \brief Renders a script outcome as the verification CSV status token.
 *
 * \param[in] status The evaluation outcome.
 *
 * \return The status token.
 */
static const char*
status_to_string(const cardano_uplc_eval_status_t status)
{
  switch (status)
  {
    case CARDANO_UPLC_EVAL_SUCCESS:
      return "success";
    case CARDANO_UPLC_EVAL_ERROR_TERM:
      return "error_term";
    case CARDANO_UPLC_EVAL_OUT_OF_BUDGET:
      return "out_of_budget";
    case CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN:
    default:
      return "unsupported_builtin";
  }
}

/* DEFINITIONS ***************************************************************/

int
cardano_bench_run_file(const char* dir, const char* file_name, cardano_bench_result_t* out)
{
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s", dir, file_name);

  size_t  size  = 0U;
  byte_t* bytes = cardano_bench_io_read_file(path, &size);

  if (bytes == NULL)
  {
    return -1;
  }

  memset(out, 0, sizeof(*out));
  set_result_name(file_name, out);

  cardano_uplc_arena_t* arena = NULL;

  if (cardano_uplc_arena_new(0U, &arena) != CARDANO_SUCCESS)
  {
    free(bytes);
    return -1;
  }

  if (check_script_runs(arena, bytes, size) != 0)
  {
    fprintf(stderr, "  %s: decode/eval host error\n", file_name);
    cardano_uplc_arena_free(&arena);
    free(bytes);

    return 0;
  }

  warm_up(arena, bytes, size);

  uint64_t* samples = (uint64_t*)malloc(MAX_ITERATIONS * sizeof(uint64_t));

  if (samples == NULL)
  {
    cardano_uplc_arena_free(&arena);
    free(bytes);
    return -1;
  }

  const size_t count = measure(arena, bytes, size, samples);

  fill_result(samples, count, out);

  free(samples);
  cardano_uplc_arena_free(&arena);
  free(bytes);

  return 0;
}

int
cardano_bench_verify_file(const char* dir, const char* file_name)
{
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s", dir, file_name);

  size_t  size  = 0U;
  byte_t* bytes = cardano_bench_io_read_file(path, &size);

  if (bytes == NULL)
  {
    printf("%s,read_error,0,0\n", file_name);
    return -1;
  }

  cardano_uplc_arena_t* arena = NULL;

  if (cardano_uplc_arena_new(0U, &arena) != CARDANO_SUCCESS)
  {
    free(bytes);
    return -1;
  }

  cardano_uplc_eval_result_t result = { 0 };

  if (evaluate_script(arena, bytes, size, &result) != 0)
  {
    printf("%s,host_error,0,0\n", file_name);
  }
  else
  {
    printf(
      "%s,%s,%lld,%lld\n",
      file_name,
      status_to_string(result.status),
      (long long)result.spent.cpu,
      (long long)result.spent.mem);
  }

  cardano_uplc_arena_free(&arena);
  free(bytes);

  return 0;
}
