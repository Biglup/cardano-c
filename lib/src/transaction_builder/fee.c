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

#include <cardano/transaction_builder/fee.h>
#include <math.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Gets the minimum of two values.
 *
 * \param a The first value.
 * \param b The second value.
 *
 * \return The minimum of the two values.
 */
static double
min(const double a, const double b)
{
  return (a < b) ? a : b;
}

/**
 * \brief Gets the maximum of two values.
 *
 * \param a The first value.
 * \param b The second value.
 *
 * \return The maximum of the two values.
 */
static double
max(const double a, const double b)
{
  return (a > b) ? a : b;
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

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_script_to_cbor(script, writer);

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

    cardano_script_language_t language;

    result = cardano_script_get_language(script, &language);

    if (result != CARDANO_SUCCESS)
    {
      *script_ref_fee = 0U;

      return result;
    }

    if (language == CARDANO_SCRIPT_LANGUAGE_NATIVE)
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

  // Starting in the Conway era, the min fee calculation is given by the total size (in bytes) of
  // reference scripts priced according to a different, growing tiered pricing model.
  // See https://github.com/CardanoSolutions/ogmios/releases/tag/v6.5.0
  double       base       = cardano_unit_interval_to_double(coins_per_ref_script_byte);
  const double range      = 25600.0;
  const double multiplier = 1.2;

  while (total_ref_scripts_size > 0U)
  {
    *script_ref_fee        += (size_t)ceil(min(range, (double)total_ref_scripts_size) * base);
    total_ref_scripts_size = (size_t)max((double)total_ref_scripts_size - range, 0.0);
    base                   *= multiplier;
  }

  return CARDANO_SUCCESS;
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

  cardano_witness_set_t*   witness_set = cardano_transaction_get_witness_set(tx);
  cardano_redeemer_list_t* redeemers   = cardano_witness_set_get_redeemers(witness_set);

  cardano_witness_set_unref(&witness_set);
  cardano_redeemer_list_unref(&redeemers);

  if ((redeemers == NULL) || (cardano_redeemer_list_get_length(redeemers) == 0U))
  {
    *min_fee = 0U;

    return CARDANO_SUCCESS;
  }

  cardano_ex_units_t* total_ex_units = NULL;

  cardano_error_t result = cardano_get_total_ex_units_in_redeemers(redeemers, &total_ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const uint64_t cpu_steps = cardano_ex_units_get_cpu_steps(total_ex_units);
  const uint64_t memory    = cardano_ex_units_get_memory(total_ex_units);

  cardano_ex_units_unref(&total_ex_units);

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