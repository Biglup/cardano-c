/**
 * \file bench_stats.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BENCH_STATS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BENCH_STATS_H

/* INCLUDES ******************************************************************/

#include <stddef.h>
#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Summary statistics over a set of duration samples, in nanoseconds.
 */
typedef struct cardano_bench_stats_t
{
    uint64_t mean_ns;
    uint64_t median_ns;
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t stddev_ns;
} cardano_bench_stats_t;

/**
 * \brief Computes summary statistics over duration samples.
 *
 * Sorts \p samples in place, then derives the mean, median (midpoint average
 * for even counts), minimum, maximum and population standard deviation. The
 * accumulations run over 128-bit integers so no sample sum or squared
 * deviation can overflow.
 *
 * \param[in,out] samples The duration samples; sorted ascending on return.
 * \param[in] count The number of samples; must be at least 1.
 *
 * \return The computed statistics.
 */
cardano_bench_stats_t
cardano_bench_stats_compute(uint64_t* samples, size_t count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_BENCH_STATS_H */
