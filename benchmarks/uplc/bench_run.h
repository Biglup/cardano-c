/**
 * \file bench_run.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BENCH_RUN_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BENCH_RUN_H

/* INCLUDES ******************************************************************/

#include <stddef.h>
#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Measurement outcome of one benchmark script.
 *
 * Times are in nanoseconds over full decode + evaluate iterations. A script
 * that fails to decode or evaluate reports zero iterations and all-zero
 * statistics, which the benchmark suite treats as a failure.
 */
typedef struct cardano_bench_result_t
{
    char     name[512];
    uint64_t iterations;
    uint64_t mean_ns;
    uint64_t median_ns;
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t stddev_ns;
} cardano_bench_result_t;

/**
 * \brief Benchmarks one .flat script: repeated flat-decode + CEK evaluation.
 *
 * Follows the cardano-plutus-vm-benchmark custom-harness protocol: 5 warmup
 * iterations, then measured iterations until at least 50 have run and 5
 * seconds have elapsed, capped at 10000. Every iteration decodes and
 * evaluates the script under Plutus V3 semantics with an unlimited budget;
 * the arena is reset (blocks retained) between iterations so steady-state
 * allocation stays a pure pointer bump.
 *
 * \param[in] dir The directory holding the script.
 * \param[in] file_name The .flat file name within \p dir.
 * \param[out] out The measurement outcome; the script name is \p file_name
 *             with the .flat suffix stripped.
 *
 * \return 0 when a result was produced (including the all-zero failure
 *         shape), or -1 on a host error such as an unreadable file or an
 *         out-of-memory condition.
 */
int
cardano_bench_run_file(const char* dir, const char* file_name, cardano_bench_result_t* out);

/**
 * \brief Evaluates one .flat script once and prints a verification CSV row.
 *
 * Prints "name,status,cpu,mem" to stdout, where status describes the script
 * outcome (success, error_term, out_of_budget, unsupported_builtin) and
 * cpu/mem are the spent execution budget. Spent-budget equality across
 * virtual machines is a very strong proxy for execution-trace equality, so
 * these rows drive cross-VM differential checks.
 *
 * \param[in] dir The directory holding the script.
 * \param[in] file_name The .flat file name within \p dir.
 *
 * \return 0 when the script was evaluated and reported, or -1 on a host
 *         error (an error row is still printed for unreadable files).
 */
int
cardano_bench_verify_file(const char* dir, const char* file_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_BENCH_RUN_H */
