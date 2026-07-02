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

#include <cardano/common/bigint.h>

#include "./random_improve_helpers.h"

#include "../../../allocators.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Computes the quotient and remainder of (a * b) / divisor without overflowing.
 *
 * The product of two 64-bit quantities can exceed the range of a 64-bit integer, so the
 * intermediate product is computed with \ref cardano_bigint_t.
 *
 * \param[in]  a         The first factor.
 * \param[in]  b         The second factor.
 * \param[in]  divisor   The divisor. Must not be zero.
 * \param[out] quotient  The resulting quotient.
 * \param[out] remainder The resulting remainder.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
mul_div_mod(
  const uint64_t a,
  const uint64_t b,
  const uint64_t divisor,
  uint64_t*      quotient,
  uint64_t*      remainder)
{
  cardano_bigint_t* big_a         = NULL;
  cardano_bigint_t* big_b         = NULL;
  cardano_bigint_t* big_divisor   = NULL;
  cardano_bigint_t* big_product   = NULL;
  cardano_bigint_t* big_quotient  = NULL;
  cardano_bigint_t* big_remainder = NULL;

  cardano_error_t result = cardano_bigint_from_unsigned_int(a, &big_a);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_bigint_from_unsigned_int(b, &big_b);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_bigint_from_unsigned_int(divisor, &big_divisor);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_bigint_from_int(0, &big_product);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_bigint_from_int(0, &big_quotient);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_bigint_from_int(0, &big_remainder);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_bigint_multiply(big_a, big_b, big_product);
    cardano_bigint_divide(big_product, big_divisor, big_quotient);
    cardano_bigint_mod(big_product, big_divisor, big_remainder);

    *quotient  = cardano_bigint_to_unsigned_int(big_quotient);
    *remainder = cardano_bigint_to_unsigned_int(big_remainder);
  }

  cardano_bigint_unref(&big_a);
  cardano_bigint_unref(&big_b);
  cardano_bigint_unref(&big_divisor);
  cardano_bigint_unref(&big_product);
  cardano_bigint_unref(&big_quotient);
  cardano_bigint_unref(&big_remainder);

  return result;
}

/* DEFINITIONS ****************************************************************/

uint64_t
_cardano_random_improve_rng_next(uint64_t* rng_state)
{
  *rng_state += 0x9e3779b97f4a7c15ULL;

  uint64_t z = *rng_state;

  z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;

  return z ^ (z >> 31U);
}

size_t
_cardano_random_improve_rng_below(uint64_t* rng_state, const size_t bound)
{
  return (size_t)(_cardano_random_improve_rng_next(rng_state) % (uint64_t)bound);
}

uint64_t
_cardano_random_improve_distance(const uint64_t a, const uint64_t b)
{
  uint64_t result = b - a;

  if (a > b)
  {
    result = a - b;
  }

  return result;
}

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

  cardano_error_t result   = CARDANO_SUCCESS;
  uint64_t        assigned = 0U;

  for (size_t i = 0U; (i < size) && (result == CARDANO_SUCCESS); ++i)
  {
    result = mul_div_mod(target, weights[i], total_weight, &parts[i], &remainders[i]);

    if (result == CARDANO_SUCCESS)
    {
      assigned += parts[i];
    }
  }

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(remainders);

    return result;
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
