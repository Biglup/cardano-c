/**
 * \file bench_stats.c
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

#include "bench_stats.h"

#include <math.h>
#include <stdlib.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief qsort comparator ordering 64-bit durations ascending.
 *
 * \param[in] a Pointer to the first duration.
 * \param[in] b Pointer to the second duration.
 *
 * \return Negative, zero or positive as \p a is below, equal to or above \p b.
 */
static int
compare_u64(const void* a, const void* b)
{
  const uint64_t lhs = *(const uint64_t*)a;
  const uint64_t rhs = *(const uint64_t*)b;

  if (lhs < rhs)
  {
    return -1;
  }

  return (lhs > rhs) ? 1 : 0;
}

/* DEFINITIONS ***************************************************************/

cardano_bench_stats_t
cardano_bench_stats_compute(uint64_t* samples, const size_t count)
{
  qsort(samples, count, sizeof(uint64_t), compare_u64);

  unsigned __int128 sum = 0U;

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
    variance_sum       += (unsigned __int128)((__int128)diff * diff);
  }

  cardano_bench_stats_t stats = {
    .mean_ns   = mean,
    .median_ns = median,
    .min_ns    = samples[0],
    .max_ns    = samples[count - 1U],
    .stddev_ns = (uint64_t)sqrt((double)(uint64_t)(variance_sum / count)),
  };

  return stats;
}
