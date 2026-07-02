/**
 * \file bench_report.c
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

#include "bench_report.h"

#include <time.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Writes one entry of the JSON "benchmarks" array.
 *
 * \param[in] out The stream to write to.
 * \param[in] result The measurement to serialize.
 * \param[in] is_last Whether this entry is the last of the array.
 */
static void
write_json_entry(FILE* out, const cardano_bench_result_t* result, const int is_last)
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
    result->name,
    (unsigned long long)result->iterations,
    (unsigned long long)result->mean_ns,
    (unsigned long long)result->median_ns,
    (unsigned long long)result->min_ns,
    (unsigned long long)result->max_ns,
    (unsigned long long)result->stddev_ns,
    is_last ? "" : ",");
}

/* DEFINITIONS ***************************************************************/

void
cardano_bench_report_progress(const cardano_bench_result_t* result)
{
  fprintf(
    stderr,
    "  %-40s mean %8.1f us  (%llu iters)\n",
    result->name,
    (double)result->mean_ns / 1000.0,
    (unsigned long long)result->iterations);
}

void
cardano_bench_report_write_json(FILE* out, const cardano_bench_result_t* results, const size_t count)
{
  fprintf(out, "{\n  \"timestamp\": %lld,\n  \"benchmarks\": [\n", (long long)time(NULL));

  for (size_t i = 0U; i < count; ++i)
  {
    write_json_entry(out, &results[i], ((i + 1U) == count) ? 1 : 0);
  }

  fprintf(out, "  ]\n}\n");
}
