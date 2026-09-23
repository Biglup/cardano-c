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
 * \brief Prices a total size of reference scripts with the tiered model of the ledger, in exact arithmetic.
 *
 * The size is split in tiers of \p tiers stride bytes, the last one possibly partial. The bytes of the first tier cost
 * \p price_numerator / \p price_denominator each and the price of every further tier is the one of the previous tier
 * scaled by the multiplier of \p tiers. The prices are kept as exact fractions and the fee is the floor of the exact
 * sum, taken once.
 *
 * The sum is kept as an integer part and a remainder over the denominator of the price of the current tier, which
 * grows by the denominator of the multiplier per tier, so the remainder always stays below that denominator.
 *
 * \param[in]  tiers             The tiered pricing model.
 * \param[in]  total_size        The total size of the reference scripts, in bytes.
 * \param[in]  price_numerator   The numerator of the price of a byte in the first tier.
 * \param[in]  price_denominator The denominator of the price of a byte in the first tier, not zero.
 * \param[out] fee               The floor of the exact price of \p total_size bytes. It is only written on success.
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
  uint64_t*                 fee)
{
  uint64_t        tier_numerator   = price_numerator;
  uint64_t        tier_denominator = price_denominator;
  uint64_t        whole            = 0U;
  uint64_t        remainder        = 0U;
  uint64_t        remaining        = total_size;
  cardano_error_t result           = CARDANO_SUCCESS;

  while ((remaining > 0U) && (result == CARDANO_SUCCESS))
  {
    const uint64_t tier_size  = (remaining < tiers->stride) ? remaining : tiers->stride;
    uint64_t       tier_price = 0U;

    result = checked_multiply(tier_size, tier_numerator, &tier_price);

    if (result == CARDANO_SUCCESS)
    {
      result = checked_add(whole, tier_price / tier_denominator, &whole);
    }

    if (result == CARDANO_SUCCESS)
    {
      const uint64_t tier_remainder = tier_price % tier_denominator;
      const uint64_t carry_limit    = tier_denominator - tier_remainder;

      if (remainder >= carry_limit)
      {
        remainder -= carry_limit;
        result    = checked_add(whole, 1U, &whole);
      }
      else
      {
        remainder += tier_remainder;
      }
    }

    remaining -= tier_size;

    if ((result == CARDANO_SUCCESS) && (remaining > 0U))
    {
      result = checked_multiply(tier_numerator, tiers->multiplier_numerator, &tier_numerator);

      if (result == CARDANO_SUCCESS)
      {
        result = checked_multiply(tier_denominator, tiers->multiplier_denominator, &tier_denominator);
      }

      if (result == CARDANO_SUCCESS)
      {
        remainder *= tiers->multiplier_denominator;
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
 * \brief Prices a total size of reference scripts with the tiered model of the ledger.
 *
 * The size is split in tiers of 25600 bytes, the last one possibly partial, and the price of every tier is the one of
 * the previous tier multiplied by 6/5: the Conway values of the reference script cost stride and multiplier protocol
 * parameters. The fee is computed exactly as the ledger does, the floor of the exact sum of the price of every tier
 * taken once, whenever the intermediate values of that computation fit in 64 bits.
 *
 * When they do not, which happens with a price whose denominator is large (for example a price converted from a
 * floating point value) or with a very large price, the fee is a conservative upper bound instead, the lower of two
 * bounds. The first one replaces the price by a price that is not lower and has a denominator half as large, repeatedly,
 * until the exact computation fits, which keeps the fee within a lovelace of the exact one for prices up to about 50
 * lovelace per byte. The second one rounds the per byte price of every tier up to a whole number of lovelace, which stays close to
 * the exact fee when the price is large. The fee never falls below the exact one.
 *
 * \param[in]  total_size        The total size of the reference scripts, in bytes.
 * \param[in]  price_numerator   The numerator of the price of a byte in the first tier.
 * \param[in]  price_denominator The denominator of the price of a byte in the first tier.
 * \param[out] fee               The price of \p total_size bytes, or zero on failure.
 *
 * \return \ref CARDANO_SUCCESS if the fee was computed, \ref CARDANO_ERROR_INVALID_ARGUMENT if there are bytes to price
 *         and \p price_denominator is zero, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if the fee, or the upper bound computed
 *         in its place, does not fit in 64 bits.
 */
static cardano_error_t
compute_tiered_ref_script_fee(
  const uint64_t total_size,
  const uint64_t price_numerator,
  const uint64_t price_denominator,
  uint64_t*      fee)
{
  static const ref_script_tiers_t conway_tiers = { 25600U, 6U, 5U };

  *fee = 0U;

  if (total_size == 0U)
  {
    return CARDANO_SUCCESS;
  }

  if (price_denominator == 0U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  const uint64_t divisor     = compute_greatest_common_divisor(price_numerator, price_denominator);
  const uint64_t numerator   = price_numerator / divisor;
  const uint64_t denominator = price_denominator / divisor;
  uint64_t       exact_fee   = 0U;

  cardano_error_t result = compute_exact_tiered_fee(&conway_tiers, total_size, numerator, denominator, &exact_fee);

  if (result != CARDANO_ERROR_INTEGER_OVERFLOW)
  {
    *fee = exact_fee;

    return result;
  }

  uint64_t        halved_numerator   = numerator;
  uint64_t        halved_denominator = denominator;
  uint64_t        halved_fee         = 0U;
  cardano_error_t halved_result      = CARDANO_ERROR_INTEGER_OVERFLOW;

  while ((halved_result == CARDANO_ERROR_INTEGER_OVERFLOW) && (halved_denominator > 1U))
  {
    halved_numerator   = (halved_numerator / 2U) + (halved_numerator % 2U);
    halved_denominator = halved_denominator / 2U;
    halved_result      = compute_exact_tiered_fee(&conway_tiers, total_size, halved_numerator, halved_denominator, &halved_fee);
  }

  uint64_t              rounded_fee    = 0U;
  const cardano_error_t rounded_result = compute_rounded_up_tiered_fee(&conway_tiers, total_size, numerator, denominator, &rounded_fee);

  if ((halved_result == CARDANO_SUCCESS) && (rounded_result == CARDANO_SUCCESS))
  {
    *fee   = (halved_fee < rounded_fee) ? halved_fee : rounded_fee;
    result = CARDANO_SUCCESS;
  }
  else if (halved_result == CARDANO_SUCCESS)
  {
    *fee   = halved_fee;
    result = CARDANO_SUCCESS;
  }
  else if (rounded_result == CARDANO_SUCCESS)
  {
    *fee   = rounded_fee;
    result = CARDANO_SUCCESS;
  }
  else
  {
    result = CARDANO_ERROR_INTEGER_OVERFLOW;
  }

  return result;
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

  *script_ref_fee = 0U;

  size_t total_ref_scripts_size = 0U;

  for (size_t i = 0U; i < cardano_utxo_list_get_length(resolved_reference_inputs); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    cardano_error_t result = cardano_utxo_list_get(resolved_reference_inputs, i, &utxo);
    cardano_utxo_unref(&utxo);

    if (result != CARDANO_SUCCESS)
    {
      *script_ref_fee = 0U;

      return result;
    }

    cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
    cardano_transaction_output_unref(&output);

    if (output == NULL)
    {
      *script_ref_fee = 0U;

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
      *script_ref_fee = 0U;

      return result;
    }

    total_ref_scripts_size += script_size;
  }

  cardano_error_t result = compute_tiered_ref_script_fee(
    (uint64_t)total_ref_scripts_size,
    cardano_unit_interval_get_numerator(coins_per_ref_script_byte),
    cardano_unit_interval_get_denominator(coins_per_ref_script_byte),
    script_ref_fee);

  if (result != CARDANO_SUCCESS)
  {
    *script_ref_fee = 0U;
  }

  return result;
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

  result = cardano_compute_script_ref_fee(resolved_reference_inputs, coins_per_ref_script_byte, &ref_script_size);

  if (result != CARDANO_SUCCESS)
  {
    *min_fee = 0U;

    return result;
  }

  *min_fee += ref_script_size;

  return CARDANO_SUCCESS;
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

  cardano_ex_unit_prices_unref(&prices);
  cardano_unit_interval_unref(&coins_per_ref_script_byte);

  if (prices == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (coins_per_ref_script_byte == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  uint64_t        min_script_fee = 0U;
  cardano_error_t result         = cardano_compute_min_script_fee(transaction, prices, resolved_inputs, coins_per_ref_script_byte, &min_script_fee);

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