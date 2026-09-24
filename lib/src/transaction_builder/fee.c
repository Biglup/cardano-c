/**
 * \file fee.c
 *
 * \author angel.castillo
 * \date   Oct 13, 2024
 *
 * Copyright 2024 Biglup Labs
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

#include <cardano/buffer.h>
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v4_script.h>
#include <cardano/scripts/script.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_builder/fee.h>
#include <math.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief The tiered pricing model of the reference scripts: the size of a tier and the factor that scales the per byte
 * price from one tier to the next, as the fraction multiplier_numerator / multiplier_denominator.
 */
typedef struct
{
    uint64_t stride;
    uint64_t multiplier_numerator;
    uint64_t multiplier_denominator;
} ref_script_tiers_t;

/**
 * \brief The per byte price of a tier of the tiered pricing model, kept as a mixed number: an integer part plus the
 * fraction fraction / denominator, which is lower than one.
 */
typedef struct
{
    uint64_t integer;
    uint64_t fraction;
    uint64_t denominator;
} tier_price_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Computes the greatest common divisor of two integers with Euclid's algorithm.
 *
 * \param[in] a The first integer.
 * \param[in] b The second integer.
 *
 * \return The greatest common divisor of the two integers, or \p a when \p b is zero.
 */
static uint64_t
compute_greatest_common_divisor(const uint64_t a, const uint64_t b)
{
  uint64_t dividend = a;
  uint64_t divisor  = b;

  while (divisor != 0U)
  {
    const uint64_t remainder = dividend % divisor;

    dividend = divisor;
    divisor  = remainder;
  }

  return dividend;
}

/**
 * \brief Multiplies two integers and reports whether the product fits in 64 bits.
 *
 * \param[in]  a       The first factor.
 * \param[in]  b       The second factor.
 * \param[out] product The product of the two factors. It is only written when the product fits.
 *
 * \return \ref CARDANO_SUCCESS if the product fits in 64 bits, or \ref CARDANO_ERROR_INTEGER_OVERFLOW otherwise.
 */
static cardano_error_t
checked_multiply(const uint64_t a, const uint64_t b, uint64_t* product)
{
  if ((b != 0U) && (a > (UINT64_MAX / b)))
  {
    return CARDANO_ERROR_INTEGER_OVERFLOW;
  }

  *product = a * b;

  return CARDANO_SUCCESS;
}

/**
 * \brief Adds two integers and reports whether the sum fits in 64 bits.
 *
 * \param[in]  a   The first term.
 * \param[in]  b   The second term.
 * \param[out] sum The sum of the two terms. It is only written when the sum fits.
 *
 * \return \ref CARDANO_SUCCESS if the sum fits in 64 bits, or \ref CARDANO_ERROR_INTEGER_OVERFLOW otherwise.
 */
static cardano_error_t
checked_add(const uint64_t a, const uint64_t b, uint64_t* sum)
{
  if (a > (UINT64_MAX - b))
  {
    return CARDANO_ERROR_INTEGER_OVERFLOW;
  }

  *sum = a + b;

  return CARDANO_SUCCESS;
}

/**
 * \brief Divides two integers, rounding the quotient up.
 *
 * \param[in] dividend The dividend.
 * \param[in] divisor  The divisor, not zero.
 *
 * \return The smallest integer that is not lower than \p dividend / \p divisor.
 */
static uint64_t
divide_rounding_up(const uint64_t dividend, const uint64_t divisor)
{
  return (dividend / divisor) + (((dividend % divisor) != 0U) ? 1U : 0U);
}

/**
 * \brief Replaces a fraction by a fraction that is not lower and has a denominator half as large.
 *
 * The numerator is halved rounding up and the denominator is halved rounding down, so the new fraction is at least
 * the old one.
 *
 * \param[in,out] numerator   The numerator of the fraction.
 * \param[in,out] denominator The denominator of the fraction, at least 2.
 */
static void
halve_fraction(uint64_t* numerator, uint64_t* denominator)
{
  *numerator   = (*numerator / 2U) + (*numerator % 2U);
  *denominator = *denominator / 2U;
}

/**
 * \brief Adds a fraction below one to a running sum kept as a whole part and a remainder over the same denominator.
 *
 * \param[in]     value       The numerator of the fraction, lower than \p denominator.
 * \param[in]     denominator The denominator of the fraction and of the remainder.
 * \param[in,out] whole       The whole part of the running sum.
 * \param[in,out] remainder   The remainder of the running sum, lower than \p denominator.
 *
 * \return \ref CARDANO_SUCCESS if the fraction was added, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if the whole part does
 *         not fit in 64 bits.
 */
static cardano_error_t
add_fraction(
  const uint64_t value,
  const uint64_t denominator,
  uint64_t*      whole,
  uint64_t*      remainder)
{
  const uint64_t carry_limit = denominator - value;

  if (*remainder >= carry_limit)
  {
    *remainder -= carry_limit;

    return checked_add(*whole, 1U, whole);
  }

  *remainder += value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Multiplies a fraction below one by an integer, giving the integer part and the remainder of the product.
 *
 * The product is built bit by bit, doubling and adding over the denominator, so no intermediate value exceeds the
 * denominator and the computation never overflows, whatever the size of the factor and of the denominator.
 *
 * \param[in]  value     The numerator of the fraction, lower than \p divisor.
 * \param[in]  factor    The integer the fraction is multiplied by.
 * \param[in]  divisor   The denominator of the fraction, not zero.
 * \param[out] quotient  The integer part of \p value times \p factor over \p divisor, which is lower than \p factor when
 *                       \p factor is not zero, and zero otherwise.
 * \param[out] remainder The remainder of \p value times \p factor over \p divisor.
 *
 * \return \ref CARDANO_SUCCESS, since the quotient is never larger than \p factor and always fits in 64 bits.
 */
static cardano_error_t
multiply_fraction(
  const uint64_t value,
  const uint64_t factor,
  const uint64_t divisor,
  uint64_t*      quotient,
  uint64_t*      remainder)
{
  cardano_error_t result = CARDANO_SUCCESS;

  *quotient  = 0U;
  *remainder = 0U;

  for (uint64_t bit = 64U; (bit > 0U) && (result == CARDANO_SUCCESS); --bit)
  {
    *quotient *= 2U;
    result    = add_fraction(*remainder, divisor, quotient, remainder);

    if ((result == CARDANO_SUCCESS) && (((factor >> (bit - 1U)) & 1U) != 0U))
    {
      result = add_fraction(value, divisor, quotient, remainder);
    }
  }

  return result;
}

/**
 * \brief Adds the price of the bytes of a tier to a running sum kept as a whole part and a remainder over the
 * denominator of the price of the tier.
 *
 * \param[in]     tier_size The number of bytes of the tier.
 * \param[in]     price     The per byte price of the tier.
 * \param[in,out] whole     The whole part of the running sum.
 * \param[in,out] remainder The remainder of the running sum, lower than the denominator of \p price.
 *
 * \return \ref CARDANO_SUCCESS if the price was added, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if the price of the
 *         integer part of the tier or the sum does not fit in 64 bits.
 */
static cardano_error_t
add_tier_price(
  const uint64_t      tier_size,
  const tier_price_t* price,
  uint64_t*           whole,
  uint64_t*           remainder)
{
  uint64_t integer_part       = 0U;
  uint64_t fraction_quotient  = 0U;
  uint64_t fraction_remainder = 0U;

  cardano_error_t result = checked_multiply(tier_size, price->integer, &integer_part);

  if (result == CARDANO_SUCCESS)
  {
    result = checked_add(*whole, integer_part, whole);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = multiply_fraction(price->fraction, tier_size, price->denominator, &fraction_quotient, &fraction_remainder);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = checked_add(*whole, fraction_quotient, whole);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = add_fraction(fraction_remainder, price->denominator, whole, remainder);
  }

  return result;
}

/**
 * \brief Replaces the fraction of the per byte price of a tier and the remainder of the running sum by fractions that
 * are not lower and have a denominator half as large.
 *
 * A fraction that reaches one after the halving is carried into the integer part of the price or into the whole part
 * of the sum, so both fractions stay lower than one.
 *
 * \param[in,out] price     The per byte price of the tier, whose denominator is at least 2.
 * \param[in,out] whole     The whole part of the running sum.
 * \param[in,out] remainder The remainder of the running sum over the denominator of \p price.
 *
 * \return \ref CARDANO_SUCCESS if the fractions were replaced, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if a carry does
 *         not fit in 64 bits.
 */
static cardano_error_t
coarsen_tier_price(tier_price_t* price, uint64_t* whole, uint64_t* remainder)
{
  cardano_error_t result = CARDANO_SUCCESS;

  halve_fraction(&price->fraction, &price->denominator);
  *remainder = (*remainder / 2U) + (*remainder % 2U);

  if (price->fraction == price->denominator)
  {
    price->fraction = 0U;
    result          = checked_add(price->integer, 1U, &price->integer);
  }

  if ((result == CARDANO_SUCCESS) && (*remainder == price->denominator))
  {
    *remainder = 0U;
    result     = checked_add(*whole, 1U, whole);
  }

  return result;
}

/**
 * \brief Scales the per byte price of a tier by the multiplier of the tiered model, giving the price of the next tier.
 *
 * With the price I + r / D and the multiplier n / d, the integer part I is split as q d + i with i lower than d, and
 * the price of the next tier is q n + floor(i n / d) plus the fraction ((i n mod d) D + r n) / (D d), whose excess over
 * one is carried into the integer part. The remainder of the running sum is moved to the new denominator, and the
 * fraction, the denominator and the remainder are then divided by their greatest common divisor, so the denominator
 * only grows as much as the exact prices require. Only the new integer part and the new denominator can overflow.
 *
 * \param[in]     tiers     The tiered pricing model, whose multiplier has a denominator that is not zero.
 * \param[in,out] price     The per byte price of the tier, replaced by the one of the next tier.
 * \param[in,out] remainder The remainder of the running sum over the denominator of \p price.
 *
 * \return \ref CARDANO_SUCCESS if the price was scaled, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if the integer part or
 *         the denominator of the next price does not fit in 64 bits. \p price and \p remainder are only written on
 *         success.
 */
static cardano_error_t
scale_tier_price(
  const ref_script_tiers_t* tiers,
  tier_price_t*             price,
  uint64_t*                 remainder)
{
  const uint64_t multiplier_numerator   = tiers->multiplier_numerator;
  const uint64_t multiplier_denominator = tiers->multiplier_denominator;
  uint64_t       next_integer           = 0U;
  uint64_t       next_denominator       = 0U;
  uint64_t       integer_quotient       = 0U;
  uint64_t       integer_remainder      = 0U;
  uint64_t       fraction_quotient      = 0U;
  uint64_t       fraction_remainder     = 0U;

  cardano_error_t result = checked_multiply(price->denominator, multiplier_denominator, &next_denominator);

  if (result == CARDANO_SUCCESS)
  {
    result = checked_multiply(price->integer / multiplier_denominator, multiplier_numerator, &next_integer);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = multiply_fraction(price->integer % multiplier_denominator, multiplier_numerator, multiplier_denominator, &integer_quotient, &integer_remainder);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = checked_add(next_integer, integer_quotient, &next_integer);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = multiply_fraction(price->fraction, multiplier_numerator, next_denominator, &fraction_quotient, &fraction_remainder);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = checked_add(next_integer, fraction_quotient, &next_integer);
  }

  uint64_t next_fraction = integer_remainder * price->denominator;

  if (result == CARDANO_SUCCESS)
  {
    result = add_fraction(fraction_remainder, next_denominator, &next_integer, &next_fraction);
  }

  if (result == CARDANO_SUCCESS)
  {
    const uint64_t next_remainder = *remainder * multiplier_denominator;
    const uint64_t divisor        = compute_greatest_common_divisor(compute_greatest_common_divisor(next_denominator, next_fraction), next_remainder);

    price->integer     = next_integer;
    price->fraction    = next_fraction / divisor;
    price->denominator = next_denominator / divisor;
    *remainder         = next_remainder / divisor;
  }

  return result;
}

/**
 * \brief Prices a total size of reference scripts with the tiered model of the ledger, in exact arithmetic.
 *
 * The size is split in tiers of \p tiers stride bytes, the last one possibly partial. The bytes of the first tier cost
 * \p price_numerator / \p price_denominator each and the price of every further tier is the one of the previous tier
 * scaled by the multiplier of \p tiers. The prices are kept as exact fractions and the fee is the floor of the exact
 * sum, taken once.
 *
 * The per byte price of every tier is kept as a mixed number, an integer part plus a fraction lower than one, and the
 * sum as a whole part plus a remainder over the denominator of that fraction. The values that grow from tier to tier
 * are therefore the integer part of the price, which is not larger than the price, and the denominator, which is the
 * one of the exact price in lowest terms with the remainder of the sum. The products of a fraction by an integer are
 * built without intermediate values larger than their denominator, so the computation stays exact whenever the
 * integer parts, the denominators and the fee fit in 64 bits.
 *
 * \param[in]  tiers             The tiered pricing model, whose multiplier has a denominator that is not zero.
 * \param[in]  total_size        The total size of the reference scripts, in bytes.
 * \param[in]  price_numerator   The numerator of the price of a byte in the first tier.
 * \param[in]  price_denominator The denominator of the price of a byte in the first tier, not zero.
 * \param[in]  coarsen           Whether a denominator that would not fit in 64 bits is avoided by replacing the fraction
 *                               of the price and the remainder of the sum by fractions that are not lower and have a
 *                               denominator half as large, as often as needed. The fee is then an upper bound of the
 *                               exact fee.
 * \param[out] fee               The floor of the exact price of \p total_size bytes, or its upper bound when
 *                               \p coarsen is true. It is only written on success.
 *
 * \return \ref CARDANO_SUCCESS if the fee was computed, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if an intermediate value
 *         or the fee does not fit in 64 bits.
 */
static cardano_error_t
compute_exact_tiered_fee(
  const ref_script_tiers_t* tiers,
  const uint64_t            total_size,
  const uint64_t            price_numerator,
  const uint64_t            price_denominator,
  const bool                coarsen,
  uint64_t*                 fee)
{
  tier_price_t    price     = { price_numerator / price_denominator, price_numerator % price_denominator, price_denominator };
  uint64_t        whole     = 0U;
  uint64_t        remainder = 0U;
  uint64_t        remaining = total_size;
  cardano_error_t result    = CARDANO_SUCCESS;

  while ((remaining > 0U) && (result == CARDANO_SUCCESS))
  {
    const uint64_t tier_size = (remaining < tiers->stride) ? remaining : tiers->stride;

    result    = add_tier_price(tier_size, &price, &whole, &remainder);
    remaining -= tier_size;

    if ((result == CARDANO_SUCCESS) && (remaining > 0U))
    {
      while (coarsen && (result == CARDANO_SUCCESS) && (price.denominator > (UINT64_MAX / tiers->multiplier_denominator)))
      {
        result = coarsen_tier_price(&price, &whole, &remainder);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = scale_tier_price(tiers, &price, &remainder);
      }
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    *fee = whole;
  }

  return result;
}

/**
 * \brief Prices a total size of reference scripts with the tiered model, rounding the per byte price of every tier up to
 * a whole number of lovelace.
 *
 * The price of the first tier is \p price_numerator / \p price_denominator rounded up, and the price of every further
 * tier is the price of the previous one scaled by the multiplier of \p tiers and rounded up. Every price is at least the
 * exact price of its tier, so the fee is an upper bound of the exact fee. Its intermediate values are the prices and the
 * fee, which do not grow with the denominator of the price.
 *
 * \param[in]  tiers             The tiered pricing model.
 * \param[in]  total_size        The total size of the reference scripts, in bytes.
 * \param[in]  price_numerator   The numerator of the price of a byte in the first tier.
 * \param[in]  price_denominator The denominator of the price of a byte in the first tier, not zero.
 * \param[out] fee               The upper bound of the price of \p total_size bytes. It is only written on success.
 *
 * \return \ref CARDANO_SUCCESS if the fee was computed, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if the fee or the price of
 *         a tier does not fit in 64 bits.
 */
static cardano_error_t
compute_rounded_up_tiered_fee(
  const ref_script_tiers_t* tiers,
  const uint64_t            total_size,
  const uint64_t            price_numerator,
  const uint64_t            price_denominator,
  uint64_t*                 fee)
{
  uint64_t        tier_price = divide_rounding_up(price_numerator, price_denominator);
  uint64_t        total      = 0U;
  uint64_t        remaining  = total_size;
  cardano_error_t result     = CARDANO_SUCCESS;

  while ((remaining > 0U) && (result == CARDANO_SUCCESS))
  {
    const uint64_t tier_size = (remaining < tiers->stride) ? remaining : tiers->stride;
    uint64_t       tier_fee  = 0U;

    result = checked_multiply(tier_size, tier_price, &tier_fee);

    if (result == CARDANO_SUCCESS)
    {
      result = checked_add(total, tier_fee, &total);
    }

    remaining -= tier_size;

    if ((result == CARDANO_SUCCESS) && (remaining > 0U))
    {
      uint64_t scaled_price = 0U;

      result = checked_multiply(tier_price, tiers->multiplier_numerator, &scaled_price);

      if (result == CARDANO_SUCCESS)
      {
        tier_price = divide_rounding_up(scaled_price, tiers->multiplier_denominator);
      }
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    *fee = total;
  }

  return result;
}

/**
 * \brief Prices a total size of reference scripts in exact arithmetic after replacing the price and the multiplier by
 * fractions that are not lower, halving their denominators until the computation fits in 64 bits.
 *
 * Every step halves the fraction with the larger denominator, the price when both denominators are equal. Every
 * halving replaces a fraction by one that is not lower, and the fee grows with the price and with the multiplier, so
 * the fee is an upper bound of the exact fee.
 *
 * \param[in]  tiers             The tiered pricing model, whose multiplier has a denominator that is not zero.
 * \param[in]  total_size        The total size of the reference scripts, in bytes.
 * \param[in]  price_numerator   The numerator of the price of a byte in the first tier.
 * \param[in]  price_denominator The denominator of the price of a byte in the first tier, not zero.
 * \param[out] fee               The upper bound of the price of \p total_size bytes. It is only written on success.
 *
 * \return \ref CARDANO_SUCCESS if the bound was computed, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if no halving made the
 *         computation fit.
 */
static cardano_error_t
compute_halved_tiered_fee(
  const ref_script_tiers_t* tiers,
  const uint64_t            total_size,
  const uint64_t            price_numerator,
  const uint64_t            price_denominator,
  uint64_t*                 fee)
{
  ref_script_tiers_t halved_tiers       = *tiers;
  uint64_t           halved_numerator   = price_numerator;
  uint64_t           halved_denominator = price_denominator;
  cardano_error_t    result             = CARDANO_ERROR_INTEGER_OVERFLOW;

  while ((result == CARDANO_ERROR_INTEGER_OVERFLOW) && ((halved_denominator > 1U) || (halved_tiers.multiplier_denominator > 1U)))
  {
    if (halved_denominator >= halved_tiers.multiplier_denominator)
    {
      halve_fraction(&halved_numerator, &halved_denominator);
    }
    else
    {
      halve_fraction(&halved_tiers.multiplier_numerator, &halved_tiers.multiplier_denominator);
    }

    result = compute_exact_tiered_fee(&halved_tiers, total_size, halved_numerator, halved_denominator, false, fee);
  }

  return result;
}

/**
 * \brief Keeps the lower of a candidate fee and the best fee found so far, ignoring a candidate that was not computed.
 *
 * \param[in]     candidate_result The result of the computation of the candidate fee.
 * \param[in]     candidate_fee    The candidate fee, only meaningful when \p candidate_result is \ref CARDANO_SUCCESS.
 * \param[in,out] best_result      \ref CARDANO_SUCCESS once a fee was kept.
 * \param[in,out] best_fee         The lowest fee kept so far.
 */
static void
keep_lower_fee(
  const cardano_error_t candidate_result,
  const uint64_t        candidate_fee,
  cardano_error_t*      best_result,
  uint64_t*             best_fee)
{
  if (candidate_result != CARDANO_SUCCESS)
  {
    return;
  }

  if ((*best_result != CARDANO_SUCCESS) || (candidate_fee < *best_fee))
  {
    *best_fee    = candidate_fee;
    *best_result = CARDANO_SUCCESS;
  }
}

/**
 * \brief Resolves the tiered pricing model of the reference scripts from the reference script cost stride and
 * multiplier protocol parameters.
 *
 * A parameter that is not set takes its Conway value: a stride of 0 takes 25600 bytes and a missing multiplier takes
 * 6/5. Parameter sets of the eras before Dijkstra do not carry them, and the ledger of those eras prices with these
 * values.
 *
 * \param[in]  stride     The reference script cost stride in bytes, or 0 when it is not set.
 * \param[in]  multiplier The reference script cost multiplier, or NULL when it is not set.
 * \param[out] tiers      The tiered pricing model, whose stride is never zero.
 */
static void
resolve_ref_script_tiers(
  const uint64_t           stride,
  cardano_unit_interval_t* multiplier,
  ref_script_tiers_t*      tiers)
{
  static const ref_script_tiers_t conway_tiers = { 25600U, 6U, 5U };

  *tiers = conway_tiers;

  if (stride != 0U)
  {
    tiers->stride = stride;
  }

  if (multiplier != NULL)
  {
    tiers->multiplier_numerator   = cardano_unit_interval_get_numerator(multiplier);
    tiers->multiplier_denominator = cardano_unit_interval_get_denominator(multiplier);
  }
}

/**
 * \brief Prices a total size of reference scripts with the tiered model of the ledger.
 *
 * The size is split in tiers of the stride of \p tiers bytes, the last one possibly partial, and the price of every
 * tier is the one of the previous tier multiplied by the multiplier of \p tiers. The fee is computed exactly as the
 * ledger does, the floor of the exact sum of the price of every tier taken once, whenever the denominator of the price
 * in lowest terms times the denominator of the multiplier in lowest terms to the power of the number of tiers minus
 * one fits in 64 bits, together with the integer parts of the prices of the tiers and the fee.
 *
 * When the exact computation does not fit, which happens with a price or a multiplier whose denominator is large (for
 * example one converted from a floating point value), with many tiers or with a very large price, the fee is a
 * conservative upper bound instead, the lowest of three bounds, none of which is lower than the exact fee:
 * - the same computation, where a denominator that would not fit is avoided by replacing the fraction of the price
 *   and the remainder of the sum by fractions that are not lower and have a denominator half as large. The excess is a
 *   tiny fraction of the fee when the denominators are small and grows with them, up to about a thousandth of the fee
 *   for a multiplier denominator around 10^15;
 * - the exact computation with the price and the multiplier replaced, repeatedly, by fractions that are not lower,
 *   halving the one with the larger denominator, and the price when both denominators are equal, until it fits;
 * - the per byte price of every tier rounded up to a whole number of lovelace, which stays close to the exact fee when
 *   the price is large.
 *
 * Replacing the price or the multiplier by a fraction that is not lower cannot lower the fee, since the fee grows with
 * both.
 *
 * \param[in]  tiers             The tiered pricing model, whose stride is not zero.
 * \param[in]  total_size        The total size of the reference scripts, in bytes.
 * \param[in]  price_numerator   The numerator of the price of a byte in the first tier.
 * \param[in]  price_denominator The denominator of the price of a byte in the first tier.
 * \param[out] fee               The price of \p total_size bytes, or zero on failure.
 *
 * \return \ref CARDANO_SUCCESS if the fee was computed, \ref CARDANO_ERROR_INVALID_ARGUMENT if there are bytes to price
 *         and \p price_denominator or the denominator of the multiplier is zero, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if
 *         the fee, or the upper bound computed in its place, does not fit in 64 bits.
 */
static cardano_error_t
compute_tiered_ref_script_fee(
  const ref_script_tiers_t* tiers,
  const uint64_t            total_size,
  const uint64_t            price_numerator,
  const uint64_t            price_denominator,
  uint64_t*                 fee)
{
  *fee = 0U;

  if (total_size == 0U)
  {
    return CARDANO_SUCCESS;
  }

  if ((price_denominator == 0U) || (tiers->multiplier_denominator == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  const uint64_t     divisor            = compute_greatest_common_divisor(price_numerator, price_denominator);
  const uint64_t     numerator          = price_numerator / divisor;
  const uint64_t     denominator        = price_denominator / divisor;
  const uint64_t     multiplier_divisor = compute_greatest_common_divisor(tiers->multiplier_numerator, tiers->multiplier_denominator);
  ref_script_tiers_t reduced_tiers      = *tiers;
  uint64_t           exact_fee          = 0U;

  reduced_tiers.multiplier_numerator   = tiers->multiplier_numerator / multiplier_divisor;
  reduced_tiers.multiplier_denominator = tiers->multiplier_denominator / multiplier_divisor;

  cardano_error_t result = compute_exact_tiered_fee(&reduced_tiers, total_size, numerator, denominator, false, &exact_fee);

  if (result != CARDANO_ERROR_INTEGER_OVERFLOW)
  {
    *fee = exact_fee;

    return result;
  }

  uint64_t        best_fee    = 0U;
  cardano_error_t best_result = CARDANO_ERROR_INTEGER_OVERFLOW;
  uint64_t        bound_fee   = 0U;

  cardano_error_t bound_result = compute_exact_tiered_fee(&reduced_tiers, total_size, numerator, denominator, true, &bound_fee);
  keep_lower_fee(bound_result, bound_fee, &best_result, &best_fee);

  bound_result = compute_halved_tiered_fee(&reduced_tiers, total_size, numerator, denominator, &bound_fee);
  keep_lower_fee(bound_result, bound_fee, &best_result, &best_fee);

  bound_result = compute_rounded_up_tiered_fee(&reduced_tiers, total_size, numerator, denominator, &bound_fee);
  keep_lower_fee(bound_result, bound_fee, &best_result, &best_fee);

  if (best_result == CARDANO_SUCCESS)
  {
    *fee = best_fee;
  }

  return best_result;
}

/**
 * \brief Computes the size of the bytes of a Plutus script: the ones the byte string of the script holds.
 *
 * \param[in]  script        The script, of a Plutus language.
 * \param[in]  language      The language of \p script.
 * \param[out] size_in_bytes The number of bytes of the script.
 *
 * \return \ref CARDANO_SUCCESS if the size was computed, or an appropriate error code indicating failure.
 */
static cardano_error_t
get_plutus_script_size(
  cardano_script_t*               script,
  const cardano_script_language_t language,
  size_t*                         size_in_bytes)
{
  cardano_plutus_v1_script_t* plutus_v1 = NULL;
  cardano_plutus_v2_script_t* plutus_v2 = NULL;
  cardano_plutus_v3_script_t* plutus_v3 = NULL;
  cardano_plutus_v4_script_t* plutus_v4 = NULL;
  cardano_buffer_t*           bytes     = NULL;
  cardano_error_t             result    = CARDANO_SUCCESS;

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
      result = cardano_script_to_plutus_v1(script, &plutus_v1);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_v1_script_to_raw_bytes(plutus_v1, &bytes);
      }

      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
      result = cardano_script_to_plutus_v2(script, &plutus_v2);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_v2_script_to_raw_bytes(plutus_v2, &bytes);
      }

      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
      result = cardano_script_to_plutus_v3(script, &plutus_v3);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_v3_script_to_raw_bytes(plutus_v3, &bytes);
      }

      break;
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V4:
      result = cardano_script_to_plutus_v4(script, &plutus_v4);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_v4_script_to_raw_bytes(plutus_v4, &bytes);
      }

      break;
    default:
      result = CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
      break;
  }

  if (result == CARDANO_SUCCESS)
  {
    *size_in_bytes = cardano_buffer_get_size(bytes);
  }

  cardano_buffer_unref(&bytes);
  cardano_plutus_v1_script_unref(&plutus_v1);
  cardano_plutus_v2_script_unref(&plutus_v2);
  cardano_plutus_v3_script_unref(&plutus_v3);
  cardano_plutus_v4_script_unref(&plutus_v4);

  return result;
}

/**
 * \brief Computes the size of the CBOR of a native script, the bytes it was decoded from when it was decoded.
 *
 * \param[in]  script        The script, of the native language.
 * \param[out] size_in_bytes The size of the CBOR of the native script.
 *
 * \return \ref CARDANO_SUCCESS if the size was computed, or an appropriate error code indicating failure.
 */
static cardano_error_t
get_native_script_size(cardano_script_t* script, size_t* size_in_bytes)
{
  cardano_native_script_t* native_script = NULL;
  cardano_error_t          result        = cardano_script_to_native(script, &native_script);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    cardano_native_script_unref(&native_script);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_native_script_to_cbor(native_script, writer);

  if (result == CARDANO_SUCCESS)
  {
    *size_in_bytes = cardano_cbor_writer_get_encode_size(writer);
  }

  cardano_cbor_writer_unref(&writer);
  cardano_native_script_unref(&native_script);

  return result;
}

/**
 * \brief Adds the execution units of the redeemers of a witness set to a running total.
 *
 * \param[in]     witness_set The witness set whose redeemers are added.
 * \param[in,out] cpu_steps   The running total of CPU steps.
 * \param[in,out] memory      The running total of memory units.
 *
 * \return \ref CARDANO_SUCCESS if the execution units were added, or an appropriate error code.
 */
static cardano_error_t
add_witness_set_ex_units(
  cardano_witness_set_t* witness_set,
  uint64_t*              cpu_steps,
  uint64_t*              memory)
{
  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  if (cardano_redeemer_list_get_length(redeemers) == 0U)
  {
    return CARDANO_SUCCESS;
  }

  cardano_ex_units_t* total_ex_units = NULL;

  const cardano_error_t result = cardano_get_total_ex_units_in_redeemers(redeemers, &total_ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *cpu_steps += cardano_ex_units_get_cpu_steps(total_ex_units);
  *memory    += cardano_ex_units_get_memory(total_ex_units);

  cardano_ex_units_unref(&total_ex_units);

  return CARDANO_SUCCESS;
}

/**
 * \brief Computes the total execution units of a transaction and of the sub transactions it carries.
 *
 * The transaction that carries sub transactions pays for the script execution of the whole batch, so the execution
 * units of the redeemers of its witness set are added to the ones of the witness set of every sub transaction.
 *
 * \param[in]  tx        The transaction.
 * \param[out] cpu_steps The total CPU steps of the batch, zero when the batch has no redeemers.
 * \param[out] memory    The total memory units of the batch, zero when the batch has no redeemers.
 *
 * \return \ref CARDANO_SUCCESS if the execution units were computed, or an appropriate error code.
 */
static cardano_error_t
get_batch_ex_units(
  cardano_transaction_t* tx,
  uint64_t*              cpu_steps,
  uint64_t*              memory)
{
  *cpu_steps = 0U;
  *memory    = 0U;

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witness_set);

  cardano_error_t result = add_witness_set_ex_units(witness_set, cpu_steps, memory);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  const size_t num_sub_transactions = cardano_sub_transaction_set_get_length(sub_transactions);

  for (size_t i = 0U; i < num_sub_transactions; ++i)
  {
    cardano_sub_transaction_t* sub_tx = NULL;

    result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_tx);
    cardano_sub_transaction_unref(&sub_tx);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_witness_set_t* sub_tx_witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
    cardano_witness_set_unref(&sub_tx_witness_set);

    result = add_witness_set_ex_units(sub_tx_witness_set, cpu_steps, memory);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Prices the reference scripts of a list of resolved UTXOs with a tiered pricing model.
 *
 * \param[in]  resolved_reference_inputs Every resolved UTXO whose reference script is priced.
 * \param[in]  coins_per_ref_script_byte The price of a byte in the first tier.
 * \param[in]  tiers                     The tiered pricing model, whose stride is not zero.
 * \param[out] script_ref_fee            The reference script fee, or zero on failure.
 *
 * \return \ref CARDANO_SUCCESS if the fee was computed, or an appropriate error code indicating failure.
 */
static cardano_error_t
compute_ref_script_fee_with_tiers(
  cardano_utxo_list_t*      resolved_reference_inputs,
  cardano_unit_interval_t*  coins_per_ref_script_byte,
  const ref_script_tiers_t* tiers,
  uint64_t*                 script_ref_fee)
{
  *script_ref_fee = 0U;

  size_t total_ref_scripts_size = 0U;

  for (size_t i = 0U; i < cardano_utxo_list_get_length(resolved_reference_inputs); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    cardano_error_t result = cardano_utxo_list_get(resolved_reference_inputs, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_transaction_output_unref(&output);

    if (output == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    cardano_script_t* script = cardano_transaction_output_get_script_ref(output);
    cardano_script_unref(&script);

    if (script == NULL)
    {
      continue;
    }

    size_t script_size = 0U;

    result = cardano_get_serialized_script_size(script, &script_size);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    total_ref_scripts_size += script_size;
  }

  return compute_tiered_ref_script_fee(
    tiers,
    (uint64_t)total_ref_scripts_size,
    cardano_unit_interval_get_numerator(coins_per_ref_script_byte),
    cardano_unit_interval_get_denominator(coins_per_ref_script_byte),
    script_ref_fee);
}

/**
 * \brief Computes the script part of the minimum fee of a transaction, pricing its reference scripts with a tiered
 * pricing model.
 *
 * \param[in]  tx                        The transaction.
 * \param[in]  prices                    The prices of the execution units.
 * \param[in]  resolved_reference_inputs Every resolved UTXO whose reference script is priced.
 * \param[in]  coins_per_ref_script_byte The price of a byte of reference script in the first tier.
 * \param[in]  tiers                     The tiered pricing model, whose stride is not zero.
 * \param[out] min_fee                   The script part of the minimum fee.
 *
 * \return \ref CARDANO_SUCCESS if the fee was computed, or an appropriate error code indicating failure.
 */
static cardano_error_t
compute_min_script_fee_with_tiers(
  cardano_transaction_t*    tx,
  cardano_ex_unit_prices_t* prices,
  cardano_utxo_list_t*      resolved_reference_inputs,
  cardano_unit_interval_t*  coins_per_ref_script_byte,
  const ref_script_tiers_t* tiers,
  uint64_t*                 min_fee)
{
  uint64_t cpu_steps = 0U;
  uint64_t memory    = 0U;

  cardano_error_t result = get_batch_ex_units(tx, &cpu_steps, &memory);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_unit_interval_t* cpu_steps_prices = NULL;
  cardano_unit_interval_t* memory_prices    = NULL;

  result = cardano_ex_unit_prices_get_steps_prices(prices, &cpu_steps_prices);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_ex_unit_prices_get_memory_prices(prices, &memory_prices);

  if (result != CARDANO_SUCCESS)
  {
    cardano_unit_interval_unref(&cpu_steps_prices);
    return result;
  }

  const double cpu_price = cardano_unit_interval_to_double(cpu_steps_prices);
  const double mem_price = cardano_unit_interval_to_double(memory_prices);

  cardano_unit_interval_unref(&cpu_steps_prices);
  cardano_unit_interval_unref(&memory_prices);

  *min_fee = (uint64_t)ceil(((double)cpu_steps * cpu_price) + ((double)memory * mem_price));

  uint64_t ref_script_size = 0U;

  result = compute_ref_script_fee_with_tiers(resolved_reference_inputs, coins_per_ref_script_byte, tiers, &ref_script_size);

  if (result != CARDANO_SUCCESS)
  {
    *min_fee = 0U;

    return result;
  }

  *min_fee += ref_script_size;

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_get_serialized_coin_size(const uint64_t lovelace, size_t* size_in_bytes)
{
  if (size_in_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_cbor_writer_write_uint(writer, lovelace);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);

    return result;
  }

  *size_in_bytes = cardano_cbor_writer_get_encode_size(writer);
  cardano_cbor_writer_unref(&writer);

  return result;
}

cardano_error_t
cardano_get_serialized_output_size(cardano_transaction_output_t* output, size_t* size_in_bytes)
{
  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size_in_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_transaction_output_to_cbor(output, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);

    return result;
  }

  *size_in_bytes = cardano_cbor_writer_get_encode_size(writer);
  cardano_cbor_writer_unref(&writer);

  return result;
}

cardano_error_t
cardano_get_serialized_script_size(cardano_script_t* script, size_t* size_in_bytes)
{
  if (script == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size_in_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_language_t language = CARDANO_SCRIPT_LANGUAGE_NATIVE;
  cardano_error_t           result   = cardano_script_get_language(script, &language);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (language == CARDANO_SCRIPT_LANGUAGE_NATIVE)
  {
    return get_native_script_size(script, size_in_bytes);
  }

  return get_plutus_script_size(script, language, size_in_bytes);
}

cardano_error_t
cardano_get_serialized_transaction_size(cardano_transaction_t* transaction, size_t* size_in_bytes)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size_in_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_transaction_to_cbor(transaction, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);

    return result;
  }

  *size_in_bytes = cardano_cbor_writer_get_encode_size(writer);
  cardano_cbor_writer_unref(&writer);

  return result;
}

cardano_error_t
cardano_get_total_ex_units_in_redeemers(cardano_redeemer_list_t* redeemers, cardano_ex_units_t** total)
{
  if (redeemers == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (total == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ex_units_t* total_ex_units = NULL;
  cardano_error_t     result         = cardano_ex_units_new(0, 0, &total_ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(redeemers); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    result = cardano_redeemer_list_get(redeemers, i, &redeemer);
    cardano_redeemer_unref(&redeemer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_ex_units_unref(&total_ex_units);
      return result;
    }

    cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(redeemer);
    cardano_ex_units_unref(&ex_units);

    if (ex_units == NULL)
    {
      cardano_ex_units_unref(&total_ex_units);
      result = CARDANO_ERROR_GENERIC;

      return result;
    }

    const uint64_t cpu_steps = cardano_ex_units_get_cpu_steps(ex_units);
    const uint64_t memory    = cardano_ex_units_get_memory(ex_units);

    result = cardano_ex_units_set_cpu_steps(total_ex_units, cardano_ex_units_get_cpu_steps(total_ex_units) + cpu_steps);

    if (result != CARDANO_SUCCESS)
    {
      cardano_ex_units_unref(&total_ex_units);
      return result;
    }

    result = cardano_ex_units_set_memory(total_ex_units, cardano_ex_units_get_memory(total_ex_units) + memory);

    if (result != CARDANO_SUCCESS)
    {
      cardano_ex_units_unref(&total_ex_units);
      return result;
    }
  }

  *total = total_ex_units;

  return result;
}

cardano_error_t
cardano_compute_script_ref_fee(
  cardano_utxo_list_t*     resolved_reference_inputs,
  cardano_unit_interval_t* coins_per_ref_script_byte,
  uint64_t*                script_ref_fee)
{
  if (resolved_reference_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (coins_per_ref_script_byte == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_ref_fee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  ref_script_tiers_t tiers = { 0U, 0U, 0U };

  resolve_ref_script_tiers(0U, NULL, &tiers);

  return compute_ref_script_fee_with_tiers(resolved_reference_inputs, coins_per_ref_script_byte, &tiers, script_ref_fee);
}

cardano_error_t
cardano_compute_script_ref_fee_with_params(
  cardano_utxo_list_t*           resolved_reference_inputs,
  cardano_protocol_parameters_t* protocol_params,
  uint64_t*                      script_ref_fee)
{
  if (resolved_reference_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_ref_fee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_t* coins_per_ref_script_byte = cardano_protocol_parameters_get_ref_script_cost_per_byte(protocol_params);
  cardano_unit_interval_t* multiplier                = cardano_protocol_parameters_get_ref_script_cost_multiplier(protocol_params);
  ref_script_tiers_t       tiers                     = { 0U, 0U, 0U };

  cardano_unit_interval_unref(&coins_per_ref_script_byte);
  cardano_unit_interval_unref(&multiplier);

  if (coins_per_ref_script_byte == NULL)
  {
    *script_ref_fee = 0U;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  resolve_ref_script_tiers(cardano_protocol_parameters_get_ref_script_cost_stride(protocol_params), multiplier, &tiers);

  return compute_ref_script_fee_with_tiers(resolved_reference_inputs, coins_per_ref_script_byte, &tiers, script_ref_fee);
}

cardano_error_t
cardano_compute_min_script_fee(
  cardano_transaction_t*    tx,
  cardano_ex_unit_prices_t* prices,
  cardano_utxo_list_t*      resolved_reference_inputs,
  cardano_unit_interval_t*  coins_per_ref_script_byte,
  uint64_t*                 min_fee)
{
  if (tx == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resolved_reference_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (coins_per_ref_script_byte == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (min_fee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  ref_script_tiers_t tiers = { 0U, 0U, 0U };

  resolve_ref_script_tiers(0U, NULL, &tiers);

  return compute_min_script_fee_with_tiers(tx, prices, resolved_reference_inputs, coins_per_ref_script_byte, &tiers, min_fee);
}

cardano_error_t
cardano_compute_min_fee_without_scripts(
  cardano_transaction_t* tx,
  const uint64_t         min_fee_constant,
  const uint64_t         min_fee_coefficient,
  uint64_t*              min_fee)
{
  if (tx == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (min_fee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t size_in_bytes = 0U;

  cardano_error_t result = cardano_get_serialized_transaction_size(tx, &size_in_bytes);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *min_fee = (uint64_t)ceil((double)min_fee_constant + ((double)min_fee_coefficient * (double)size_in_bytes));

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_compute_min_ada_required(
  cardano_transaction_output_t* output,
  const uint64_t                coins_per_utxo_byte,
  uint64_t*                     lovelace_required)
{
  // The constant overhead of 160 bytes accounts for the transaction input
  // and the entry in the UTxO map data structure (20 words * 8 bytes).
  static const size_t min_ada_overhead = 160U;

  if (output == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (lovelace_required == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t          old_coin_size = 0U;
  size_t          output_size   = 0U;
  cardano_error_t result        = cardano_get_serialized_coin_size(cardano_value_get_coin(value), &old_coin_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_get_serialized_output_size(output, &output_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  size_t last_size = old_coin_size;
  bool   is_done   = false;

  while (!is_done)
  {
    size_t   size_diff         = last_size - old_coin_size;
    uint64_t tentative_min_ada = (output_size + min_ada_overhead + size_diff) * coins_per_utxo_byte;

    size_t new_coin_size = 0U;

    result = cardano_get_serialized_coin_size(tentative_min_ada, &new_coin_size);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    is_done   = (new_coin_size == last_size);
    last_size = new_coin_size;
  }

  size_t size_change = last_size - old_coin_size;

  *lovelace_required = (output_size + size_change + min_ada_overhead) * coins_per_utxo_byte;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_compute_transaction_fee(
  cardano_transaction_t*         transaction,
  cardano_utxo_list_t*           resolved_inputs,
  cardano_protocol_parameters_t* protocol_params,
  uint64_t*                      fee)
{
  if (transaction == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resolved_inputs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (protocol_params == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (fee == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ex_unit_prices_t* prices                    = cardano_protocol_parameters_get_execution_costs(protocol_params);
  const uint64_t            min_fee_coefficient       = cardano_protocol_parameters_get_min_fee_a(protocol_params);
  const uint64_t            min_fee_constant          = cardano_protocol_parameters_get_min_fee_b(protocol_params);
  cardano_unit_interval_t*  coins_per_ref_script_byte = cardano_protocol_parameters_get_ref_script_cost_per_byte(protocol_params);
  cardano_unit_interval_t*  ref_script_multiplier     = cardano_protocol_parameters_get_ref_script_cost_multiplier(protocol_params);
  ref_script_tiers_t        tiers                     = { 0U, 0U, 0U };

  cardano_ex_unit_prices_unref(&prices);
  cardano_unit_interval_unref(&coins_per_ref_script_byte);
  cardano_unit_interval_unref(&ref_script_multiplier);

  if (prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (coins_per_ref_script_byte == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  resolve_ref_script_tiers(cardano_protocol_parameters_get_ref_script_cost_stride(protocol_params), ref_script_multiplier, &tiers);

  uint64_t        min_script_fee = 0U;
  cardano_error_t result         = compute_min_script_fee_with_tiers(transaction, prices, resolved_inputs, coins_per_ref_script_byte, &tiers, &min_script_fee);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  uint64_t no_script_fee = 0U;

  result = cardano_compute_min_fee_without_scripts(transaction, min_fee_constant, min_fee_coefficient, &no_script_fee);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *fee = min_script_fee + no_script_fee;

  return CARDANO_SUCCESS;
}