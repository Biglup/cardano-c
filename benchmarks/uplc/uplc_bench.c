/**
 * \file uplc_bench.c
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

/**
 * Benchmark harness for the UPLC CEK machine.
 *
 * Measures flat-decode + evaluate per script over a directory of raw .flat
 * files (the cardano-plutus-vm-benchmark plutus_use_cases corpus) and emits
 * the JSON schema consumed by that suite's parsers. A --verify mode instead
 * evaluates each script once and prints "name,status,cpu,mem" CSV rows for
 * cross-VM differential checks.
 */

/* INCLUDES ******************************************************************/

#include "bench_run.h"
#include "utils/bench_io.h"
#include "utils/bench_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TYPES *********************************************************************/

/**
 * \brief Parsed command line options.
 */
typedef struct bench_options_t
{
    const char* data_dir;
    const char* out_path;
    int         verify;
    int         quiet;
} bench_options_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Parses the command line.
 *
 * \param[in] argc The argument count.
 * \param[in] argv The argument vector.
 * \param[out] options The parsed options.
 *
 * \return 0 on success, or -1 when no data directory was given.
 */
static int
parse_options(const int argc, char** argv, bench_options_t* options)
{
  memset(options, 0, sizeof(*options));

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--verify") == 0)
    {
      options->verify = 1;
    }
    else if ((strcmp(argv[i], "--quiet") == 0) || (strcmp(argv[i], "-q") == 0))
    {
      options->quiet = 1;
    }
    else if ((strcmp(argv[i], "-o") == 0) && ((i + 1) < argc))
    {
      options->out_path = argv[++i];
    }
    else if ((strcmp(argv[i], "--format") == 0) && ((i + 1) < argc))
    {
      ++i;
    }
    else
    {
      options->data_dir = argv[i];
    }
  }

  return (options->data_dir == NULL) ? -1 : 0;
}

/**
 * \brief Runs verify mode: one evaluation and one CSV row per script.
 *
 * \param[in] options The parsed command line options.
 * \param[in] files The script file names.
 * \param[in] file_count The number of script file names.
 *
 * \return The process exit code.
 */
static int
run_verify_mode(const bench_options_t* options, char** files, const size_t file_count)
{
  printf("name,status,cpu,mem\n");

  for (size_t i = 0U; i < file_count; ++i)
  {
    (void)cardano_bench_verify_file(options->data_dir, files[i]);
  }

  return 0;
}

/**
 * \brief Runs benchmark mode: measures every script and writes the JSON report.
 *
 * \param[in] options The parsed command line options.
 * \param[in] files The script file names.
 * \param[in] file_count The number of script file names.
 *
 * \return The process exit code.
 */
static int
run_bench_mode(const bench_options_t* options, char** files, const size_t file_count)
{
  cardano_bench_result_t* results = (cardano_bench_result_t*)calloc(file_count, sizeof(cardano_bench_result_t));

  if (results == NULL)
  {
    fprintf(stderr, "Out of memory\n");
    return 1;
  }

  size_t result_count = 0U;

  for (size_t i = 0U; i < file_count; ++i)
  {
    if (cardano_bench_run_file(options->data_dir, files[i], &results[result_count]) == 0)
    {
      if (!options->quiet)
      {
        cardano_bench_report_progress(&results[result_count]);
      }

      ++result_count;
    }
  }

  FILE* out = stdout;

  if (options->out_path != NULL)
  {
    out = fopen(options->out_path, "w");

    if (out == NULL)
    {
      fprintf(stderr, "Cannot open output file: %s\n", options->out_path);
      free(results);

      return 1;
    }
  }

  cardano_bench_report_write_json(out, results, result_count);

  if (out != stdout)
  {
    fclose(out);
  }

  free(results);

  return 0;
}

/* MAIN **********************************************************************/

/**
 * \brief Entry point of the benchmark harness.
 *
 * \param[in] argc The argument count.
 * \param[in] argv The argument vector.
 *
 * \return 0 on success, or a non-zero value on error.
 */
int
main(const int argc, char** argv)
{
  bench_options_t options;

  if (parse_options(argc, argv, &options) != 0)
  {
    fprintf(stderr, "Usage: %s [--verify] [--quiet] [--format json] [-o out.json] <flat-dir>\n", argv[0]);
    return 1;
  }

  char**       files      = NULL;
  const size_t file_count = cardano_bench_io_list_flat_files(options.data_dir, &files);

  if (file_count == 0U)
  {
    fprintf(stderr, "No .flat files found in: %s\n", options.data_dir);
    return 1;
  }

  const int exit_code = options.verify
    ? run_verify_mode(&options, files, file_count)
    : run_bench_mode(&options, files, file_count);

  cardano_bench_io_free_file_list(files, file_count);

  return exit_code;
}
