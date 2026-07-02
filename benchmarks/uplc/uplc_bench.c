/**
 * \file uplc_bench.c
 *
 * \author angel.castillo
 * \date   Jul 02 2026
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

/* Benchmark harness for the UPLC CEK machine.
 *
 * Measures flat-decode + evaluate per script over a directory of raw
 * .flat files (the cardano-plutus-vm-benchmark plutus_use_cases corpus)
 * and emits the JSON schema consumed by that suite's parsers:
 *
 *   { "timestamp": N, "benchmarks": [ { "name", "iterations", "mean_ns",
 *     "median_ns", "min_ns", "max_ns", "stddev_ns" } ] }
 *
 * Protocol mirrors the other custom harnesses in the suite (plutuz,
 * llvm-uplc): 5 warmup iterations, at least 50 measured iterations, a 5s
 * time budget per script and a 10000 iteration cap. The arena is reset
 * (blocks retained) between iterations so steady-state allocation is a
 * pure pointer bump.
 *
 * A --verify mode evaluates each script once and prints
 * "name,status,cpu,mem" CSV for cross-VM differential checks.
 */

#include "uplc/arena/uplc_arena.h"
#include "uplc/ast/uplc_program.h"
#include "uplc/flat/flat_decode.h"
#include "uplc/flat/flat_reader.h"
#include "uplc/machine/uplc_machine.h"

#include <dirent.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* CONSTANTS *****************************************************************/

static const int      WARMUP_ITERATIONS = 5;
static const size_t   MIN_ITERATIONS    = 50U;
static const uint64_t TIME_BUDGET_NS    = 5000000000ULL;
static const size_t   MAX_ITERATIONS    = 10000U;
static const size_t   MAX_FILES         = 4096U;
static const size_t   MAX_FILE_SIZE     = 10U * 1024U * 1024U;

/* TYPES *********************************************************************/

typedef struct bench_result_t
{
    char     name[512];
    uint64_t iterations;
    uint64_t mean_ns;
    uint64_t median_ns;
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t stddev_ns;
} bench_result_t;

/* HELPERS *******************************************************************/

static uint64_t
now_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

static int
cmp_u64(const void* a, const void* b)
{
  const uint64_t lhs = *(const uint64_t*)a;
  const uint64_t rhs = *(const uint64_t*)b;

  if (lhs < rhs)
  {
    return -1;
  }

  return (lhs > rhs) ? 1 : 0;
}

static int
cmp_str(const void* a, const void* b)
{
  return strcmp(*(const char* const*)a, *(const char* const*)b);
}

static byte_t*
read_file(const char* path, size_t* out_size)
{
  FILE* file = fopen(path, "rb");

  if (file == NULL)
  {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if ((size <= 0) || ((size_t)size > MAX_FILE_SIZE))
  {
    fclose(file);
    return NULL;
  }

  byte_t* buffer = (byte_t*)malloc((size_t)size);

  if (buffer == NULL)
  {
    fclose(file);
    return NULL;
  }

  if (fread(buffer, 1U, (size_t)size, file) != (size_t)size)
  {
    free(buffer);
    fclose(file);
    return NULL;
  }

  fclose(file);
  *out_size = (size_t)size;

  return buffer;
}

/* Decode + evaluate one iteration. Returns 0 on host success. */
static int
run_once(cardano_uplc_arena_t* arena, const byte_t* bytes, size_t size, cardano_uplc_eval_result_t* result)
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

  cardano_uplc_budget_t budget = { INT64_MAX, INT64_MAX };

  if (cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, result) != CARDANO_SUCCESS)
  {
    return -1;
  }

  return 0;
}

static int
bench_file(const char* dir, const char* file_name, bench_result_t* out)
{
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s", dir, file_name);

  size_t  size  = 0U;
  byte_t* bytes = read_file(path, &size);

  if (bytes == NULL)
  {
    return -1;
  }

  memset(out, 0, sizeof(*out));
  snprintf(out->name, sizeof(out->name), "%s", file_name);

  /* Strip the .flat extension to match the suite's script naming. */
  size_t name_len = strlen(out->name);

  if ((name_len > 5U) && (strcmp(&out->name[name_len - 5U], ".flat") == 0))
  {
    out->name[name_len - 5U] = '\0';
  }

  cardano_uplc_arena_t* arena = NULL;

  if (cardano_uplc_arena_new(0U, &arena) != CARDANO_SUCCESS)
  {
    free(bytes);
    return -1;
  }

  /* Verify the file decodes and evaluates before benchmarking. */
  {
    cardano_uplc_eval_result_t result = { 0 };

    if (run_once(arena, bytes, size, &result) != 0)
    {
      fprintf(stderr, "  %s: decode/eval host error\n", file_name);
      cardano_uplc_arena_free(&arena);
      free(bytes);

      return 0; /* report all-zero stats, suite marks it failed */
    }

    cardano_uplc_arena_reset(arena);
  }

  for (int i = 0; i < WARMUP_ITERATIONS; ++i)
  {
    cardano_uplc_eval_result_t result = { 0 };
    (void)run_once(arena, bytes, size, &result);
    cardano_uplc_arena_reset(arena);
  }

  uint64_t* samples = (uint64_t*)malloc(MAX_ITERATIONS * sizeof(uint64_t));

  if (samples == NULL)
  {
    cardano_uplc_arena_free(&arena);
    free(bytes);
    return -1;
  }

  size_t   count         = 0U;
  uint64_t total_elapsed = 0U;

  while ((count < MIN_ITERATIONS) || (total_elapsed < TIME_BUDGET_NS))
  {
    cardano_uplc_eval_result_t result = { 0 };

    const uint64_t start = now_ns();
    (void)run_once(arena, bytes, size, &result);
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

  qsort(samples, count, sizeof(uint64_t), cmp_u64);

  uint64_t              min_val = samples[0];
  uint64_t              max_val = samples[count - 1U];
  unsigned __int128     sum     = 0U;

  for (size_t i = 0U; i < count; ++i)
  {
    sum += samples[i];
  }

  const uint64_t mean = (uint64_t)(sum / count);

  const uint64_t median = ((count % 2U) == 0U)
    ? ((samples[(count / 2U) - 1U] + samples[count / 2U]) / 2U)
    : samples[count / 2U];

  unsigned __int128 variance_sum = 0U;

  for (size_t i = 0U; i < count; ++i)
  {
    const int64_t diff = (int64_t)samples[i] - (int64_t)mean;
    variance_sum += (unsigned __int128)((__int128)diff * diff);
  }

  const uint64_t stddev = (uint64_t)sqrt((double)(uint64_t)(variance_sum / count));

  out->iterations = count;
  out->mean_ns    = mean;
  out->median_ns  = median;
  out->min_ns     = min_val;
  out->max_ns     = max_val;
  out->stddev_ns  = stddev;

  free(samples);
  cardano_uplc_arena_free(&arena);
  free(bytes);

  return 0;
}

static int
verify_file(const char* dir, const char* file_name)
{
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s", dir, file_name);

  size_t  size  = 0U;
  byte_t* bytes = read_file(path, &size);

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

  if (run_once(arena, bytes, size, &result) != 0)
  {
    printf("%s,host_error,0,0\n", file_name);
  }
  else
  {
    const char* status = "success";

    switch (result.status)
    {
      case CARDANO_UPLC_EVAL_SUCCESS:
        status = "success";
        break;
      case CARDANO_UPLC_EVAL_ERROR_TERM:
        status = "error_term";
        break;
      case CARDANO_UPLC_EVAL_OUT_OF_BUDGET:
        status = "out_of_budget";
        break;
      case CARDANO_UPLC_EVAL_UNSUPPORTED_BUILTIN:
      default:
        status = "unsupported_builtin";
        break;
    }

    printf("%s,%s,%lld,%lld\n", file_name, status, (long long)result.spent.cpu, (long long)result.spent.mem);
  }

  cardano_uplc_arena_free(&arena);
  free(bytes);

  return 0;
}

/* MAIN **********************************************************************/

int
main(int argc, char** argv)
{
  const char* data_dir = NULL;
  const char* out_path = NULL;
  int         verify   = 0;
  int         quiet    = 0;

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--verify") == 0)
    {
      verify = 1;
    }
    else if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0)
    {
      quiet = 1;
    }
    else if ((strcmp(argv[i], "-o") == 0) && ((i + 1) < argc))
    {
      out_path = argv[++i];
    }
    else if ((strcmp(argv[i], "--format") == 0) && ((i + 1) < argc))
    {
      ++i; /* only json supported */
    }
    else
    {
      data_dir = argv[i];
    }
  }

  if (data_dir == NULL)
  {
    fprintf(stderr, "Usage: %s [--verify] [--quiet] [--format json] [-o out.json] <flat-dir>\n", argv[0]);
    return 1;
  }

  DIR* dir = opendir(data_dir);

  if (dir == NULL)
  {
    fprintf(stderr, "Cannot open directory: %s\n", data_dir);
    return 1;
  }

  char** files      = (char**)malloc(MAX_FILES * sizeof(char*));
  size_t file_count = 0U;

  struct dirent* entry = NULL;

  while (((entry = readdir(dir)) != NULL) && (file_count < MAX_FILES))
  {
    const size_t len = strlen(entry->d_name);

    if ((len > 5U) && (strcmp(&entry->d_name[len - 5U], ".flat") == 0))
    {
      files[file_count] = strdup(entry->d_name);
      ++file_count;
    }
  }

  closedir(dir);
  qsort(files, file_count, sizeof(char*), cmp_str);

  if (verify)
  {
    printf("name,status,cpu,mem\n");

    for (size_t i = 0U; i < file_count; ++i)
    {
      (void)verify_file(data_dir, files[i]);
    }
  }
  else
  {
    bench_result_t* results = (bench_result_t*)calloc(file_count, sizeof(bench_result_t));
    size_t          result_count = 0U;

    for (size_t i = 0U; i < file_count; ++i)
    {
      if (bench_file(data_dir, files[i], &results[result_count]) == 0)
      {
        if (!quiet)
        {
          fprintf(
            stderr,
            "  %-40s mean %8.1f us  (%llu iters)\n",
            results[result_count].name,
            (double)results[result_count].mean_ns / 1000.0,
            (unsigned long long)results[result_count].iterations);
        }

        ++result_count;
      }
    }

    FILE* out = stdout;

    if (out_path != NULL)
    {
      out = fopen(out_path, "w");

      if (out == NULL)
      {
        fprintf(stderr, "Cannot open output file: %s\n", out_path);
        return 1;
      }
    }

    fprintf(out, "{\n  \"timestamp\": %lld,\n  \"benchmarks\": [\n", (long long)time(NULL));

    for (size_t i = 0U; i < result_count; ++i)
    {
      fprintf(
        out,
        "    {\n"
        "      \"name\": \"%s\",\n"
        "      \"iterations\": %llu,\n"
        "      \"mean_ns\": %llu,\n"
        "      \"median_ns\": %llu,\n"
        "      \"min_ns\": %llu,\n"
        "      \"max_ns\": %llu,\n"
        "      \"stddev_ns\": %llu\n"
        "    }%s\n",
        results[i].name,
        (unsigned long long)results[i].iterations,
        (unsigned long long)results[i].mean_ns,
        (unsigned long long)results[i].median_ns,
        (unsigned long long)results[i].min_ns,
        (unsigned long long)results[i].max_ns,
        (unsigned long long)results[i].stddev_ns,
        (i + 1U < result_count) ? "," : "");
    }

    fprintf(out, "  ]\n}\n");

    if (out != stdout)
    {
      fclose(out);
    }

    free(results);
  }

  for (size_t i = 0U; i < file_count; ++i)
  {
    free(files[i]);
  }

  free(files);

  return 0;
}
