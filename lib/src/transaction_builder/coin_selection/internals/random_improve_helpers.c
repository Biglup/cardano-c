/**
 * \file random_improve_helpers.c
 *
 * \author angel.castillo
 * \date   Jul 02, 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include "./random_improve_helpers.h"

#include "../../../allocators.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Computes (a * b) as a 128-bit quantity split into high and low 64-bit halves.
 *
 * Portable 64x64 -> 128 multiplication implemented with 32-bit limbs, so that it does not
 * rely on compiler-specific 128-bit integer support.
 *
 * \param[in]  a   The first operand.
 * \param[in]  b   The second operand.
 * \param[out] hi  The high 64 bits of the product.
 * \param[out] lo  The low 64 bits of the product.
 */
static void
mul_64x64_128(const uint64_t a, const uint64_t b, uint64_t* hi, uint64_t* lo)
{
  const uint64_t a_lo = a & 0xFFFFFFFFULL;
  const uint64_t a_hi = a >> 32U;
  const uint64_t b_lo = b & 0xFFFFFFFFULL;
  const uint64_t b_hi = b >> 32U;

  const uint64_t p0 = a_lo * b_lo;
  const uint64_t p1 = a_lo * b_hi;
  const uint64_t p2 = a_hi * b_lo;
  const uint64_t p3 = a_hi * b_hi;

  const uint64_t carry = ((p0 >> 32U) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL)) >> 32U;

  *lo = p0 + (p1 << 32U) + (p2 << 32U);
  *hi = p3 + (p1 >> 32U) + (p2 >> 32U) + carry;
}

/**
 * \brief Computes the quotient and remainder of a 128-bit dividend by a 64-bit divisor.
 *
 * The dividend is given as high and low 64-bit halves. This implementation performs simple
 * bitwise long division, which is sufficient for the small problem sizes used by the
 * partition function.
 *
 * \param[in]  hi        The high 64 bits of the dividend.
 * \param[in]  lo        The low 64 bits of the dividend.
 * \param[in]  divisor   The divisor. Must not be zero.
 * \param[out] quotient  The resulting quotient (truncated to 64 bits).
 * \param[out] remainder The resulting remainder.
 */
static void
div_128_by_64(const uint64_t hi, const uint64_t lo, const uint64_t divisor, uint64_t* quotient, uint64_t* remainder)
{
  uint64_t q = 0U;
  uint64_t r = 0U;

  for (int32_t i = 127; i >= 0; --i)
  {
    uint64_t bit = 0U;

    if (i >= 64)
    {
      const int32_t shift = i - 64;

      bit = (hi >> (uint32_t)shift) & 1ULL;
    }
    else
    {
      bit = (lo >> (uint32_t)i) & 1ULL;
    }

    r = (r << 1U) | bit;

    if (r >= divisor)
    {
      r -= divisor;

      if (i < 64)
      {
        q |= (1ULL << (uint32_t)i);
      }
    }
  }

  *quotient  = q;
  *remainder = r;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
_cardano_random_improve_partition(
  const uint64_t  target,
  const uint64_t* weights,
  const size_t    size,
  uint64_t*       parts)
{
  if ((weights == NULL) || (parts == NULL) || (size == 0U))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  uint64_t total_weight = 0U;

  for (size_t i = 0U; i < size; ++i)
  {
    total_weight += weights[i];
  }

  if (total_weight == 0U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  uint64_t* remainders = (uint64_t*)_cardano_malloc(size * sizeof(uint64_t));

  if (remainders == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  uint64_t assigned = 0U;

  for (size_t i = 0U; i < size; ++i)
  {
    uint64_t hi = 0U;
    uint64_t lo = 0U;

    mul_64x64_128(target, weights[i], &hi, &lo);
    div_128_by_64(hi, lo, total_weight, &parts[i], &remainders[i]);

    assigned += parts[i];
  }

  uint64_t shortfall = target - assigned;

  while (shortfall > 0U)
  {
    size_t best = SIZE_MAX;

    for (size_t i = 0U; i < size; ++i)
    {
      if (remainders[i] == 0U)
      {
        continue;
      }

      if ((best == SIZE_MAX) || (remainders[i] > remainders[best]) || ((remainders[i] == remainders[best]) && (parts[i] > parts[best])))
      {
        best = i;
      }
    }

    if (best == SIZE_MAX)
    {
      break;
    }

    parts[best]      += 1U;
    remainders[best] = 0U;
    shortfall        -= 1U;
  }

  _cardano_free(remainders);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_random_improve_pad_coalesce(
  const uint64_t* quantities,
  const size_t    size,
  const size_t    target_size,
  uint64_t*       result)
{
  if ((result == NULL) || (target_size == 0U) || ((quantities == NULL) && (size > 0U)))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  uint64_t* sorted = (uint64_t*)_cardano_malloc(((size > 0U) ? size : 1U) * sizeof(uint64_t));

  if (sorted == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    uint64_t value = quantities[i];
    size_t   j     = i;

    while ((j > 0U) && (sorted[j - 1U] > value))
    {
      sorted[j] = sorted[j - 1U];
      --j;
    }

    sorted[j] = value;
  }

  size_t length = size;

  while (length > target_size)
  {
    const uint64_t coalesced = sorted[0] + sorted[1];

    size_t j = 2U;

    while ((j < length) && (sorted[j] < coalesced))
    {
      sorted[j - 2U] = sorted[j];
      ++j;
    }

    sorted[j - 2U] = coalesced;

    for (size_t k = j; k < length; ++k)
    {
      sorted[k - 1U] = sorted[k];
    }

    --length;
  }

  const size_t padding = target_size - length;

  for (size_t i = 0U; i < padding; ++i)
  {
    result[i] = 0U;
  }

  for (size_t i = 0U; i < length; ++i)
  {
    result[padding + i] = sorted[i];
  }

  _cardano_free(sorted);

  return CARDANO_SUCCESS;
}

void
_cardano_random_improve_reduce_quantities(
  const uint64_t reduction_target,
  uint64_t*      quantities,
  const size_t   size)
{
  uint64_t remaining = reduction_target;

  for (size_t i = 0U; (i < size) && (remaining > 0U); ++i)
  {
    if (quantities[i] >= remaining)
    {
      quantities[i] -= remaining;
      remaining     = 0U;
    }
    else
    {
      remaining     -= quantities[i];
      quantities[i] = 0U;
    }
  }
}
