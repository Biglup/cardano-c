/**
 * \file bench_report.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BENCH_REPORT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BENCH_REPORT_H

/* INCLUDES ******************************************************************/

#include "../bench_run.h"

#include <stdio.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Prints a one-line human-readable progress entry to stderr.
 *
 * \param[in] result The measurement to report.
 */
void
cardano_bench_report_progress(const cardano_bench_result_t* result);

/**
 * \brief Writes measurements as the benchmark suite's JSON schema.
 *
 * Emits the object consumed by cardano-plutus-vm-benchmark's parsers: a
 * timestamp and a "benchmarks" array whose entries carry the script name,
 * iteration count and nanosecond statistics.
 *
 * \param[in] out The stream to write to.
 * \param[in] results The measurements to serialize.
 * \param[in] count The number of measurements in \p results.
 */
void
cardano_bench_report_write_json(FILE* out, const cardano_bench_result_t* results, size_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_BENCH_REPORT_H */
