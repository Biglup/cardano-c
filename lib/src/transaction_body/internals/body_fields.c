/**
 * \file body_fields.c
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
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

#include "body_fields.h"

#include <assert.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_body_read_inputs(cardano_cbor_reader_t* reader, cardano_transaction_input_set_t** inputs)
{
  assert(reader != NULL);
  assert(inputs != NULL);

  return cardano_transaction_input_set_from_cbor(reader, inputs);
}

cardano_error_t
cardano_body_read_outputs(cardano_cbor_reader_t* reader, cardano_transaction_output_list_t** outputs)
{
  assert(reader != NULL);
  assert(outputs != NULL);

  return cardano_transaction_output_list_from_cbor(reader, outputs);
}

cardano_error_t
cardano_body_read_ttl(cardano_cbor_reader_t* reader, uint64_t* ttl)
{
  assert(reader != NULL);
  assert(ttl != NULL);

  return cardano_cbor_reader_read_uint(reader, ttl);
}

cardano_error_t
cardano_body_read_certificates(cardano_cbor_reader_t* reader, cardano_certificate_set_t** certificates)
{
  assert(reader != NULL);
  assert(certificates != NULL);

  return cardano_certificate_set_from_cbor(reader, certificates);
}

cardano_error_t
cardano_body_read_withdrawals(cardano_cbor_reader_t* reader, cardano_withdrawal_map_t** withdrawals)
{
  assert(reader != NULL);
  assert(withdrawals != NULL);

  return cardano_withdrawal_map_from_cbor(reader, withdrawals);
}

cardano_error_t
cardano_body_read_aux_data_hash(cardano_cbor_reader_t* reader, cardano_blake2b_hash_t** aux_data_hash)
{
  assert(reader != NULL);
  assert(aux_data_hash != NULL);

  return cardano_blake2b_hash_from_cbor(reader, aux_data_hash);
}

cardano_error_t
cardano_body_read_validity_start(cardano_cbor_reader_t* reader, uint64_t* validity_start)
{
  assert(reader != NULL);
  assert(validity_start != NULL);

  return cardano_cbor_reader_read_uint(reader, validity_start);
}

cardano_error_t
cardano_body_read_mint(cardano_cbor_reader_t* reader, cardano_multi_asset_t** mint)
{
  assert(reader != NULL);
  assert(mint != NULL);

  return cardano_multi_asset_from_cbor(reader, mint);
}

cardano_error_t
cardano_body_read_script_data_hash(cardano_cbor_reader_t* reader, cardano_blake2b_hash_t** script_data_hash)
{
  assert(reader != NULL);
  assert(script_data_hash != NULL);

  return cardano_blake2b_hash_from_cbor(reader, script_data_hash);
}

cardano_error_t
cardano_body_read_guards(cardano_cbor_reader_t* reader, cardano_guard_set_t** guards)
{
  assert(reader != NULL);
  assert(guards != NULL);

  return cardano_guard_set_from_cbor(reader, guards);
}

cardano_error_t
cardano_body_read_network_id(cardano_cbor_reader_t* reader, cardano_network_id_t* network_id)
{
  assert(reader != NULL);
  assert(network_id != NULL);

  return cardano_cbor_reader_read_uint(reader, (uint64_t*)((void*)network_id));
}

cardano_error_t
cardano_body_read_reference_inputs(cardano_cbor_reader_t* reader, cardano_transaction_input_set_t** reference_inputs)
{
  assert(reader != NULL);
  assert(reference_inputs != NULL);

  return cardano_transaction_input_set_from_cbor(reader, reference_inputs);
}

cardano_error_t
cardano_body_read_voting_procedures(cardano_cbor_reader_t* reader, cardano_voting_procedures_t** voting_procedures)
{
  assert(reader != NULL);
  assert(voting_procedures != NULL);

  return cardano_voting_procedures_from_cbor(reader, voting_procedures);
}

cardano_error_t
cardano_body_read_proposal_procedures(cardano_cbor_reader_t* reader, cardano_proposal_procedure_set_t** proposal_procedures)
{
  assert(reader != NULL);
  assert(proposal_procedures != NULL);

  return cardano_proposal_procedure_set_from_cbor(reader, proposal_procedures);
}

cardano_error_t
cardano_body_read_treasury_value(cardano_cbor_reader_t* reader, uint64_t* treasury_value)
{
  assert(reader != NULL);
  assert(treasury_value != NULL);

  return cardano_cbor_reader_read_uint(reader, treasury_value);
}

cardano_error_t
cardano_body_read_donation(cardano_cbor_reader_t* reader, uint64_t* donation)
{
  assert(reader != NULL);
  assert(donation != NULL);

  return cardano_cbor_reader_read_uint(reader, donation);
}

cardano_error_t
cardano_body_read_required_top_level_guards(cardano_cbor_reader_t* reader, cardano_required_guards_map_t** required_top_level_guards)
{
  assert(reader != NULL);
  assert(required_top_level_guards != NULL);

  return cardano_required_guards_map_from_cbor(reader, required_top_level_guards);
}

cardano_error_t
cardano_body_read_direct_deposits(cardano_cbor_reader_t* reader, cardano_direct_deposit_map_t** direct_deposits)
{
  assert(reader != NULL);
  assert(direct_deposits != NULL);

  return cardano_direct_deposit_map_from_cbor(reader, direct_deposits);
}

cardano_error_t
cardano_body_read_account_balance_intervals(cardano_cbor_reader_t* reader, cardano_account_balance_intervals_map_t** account_balance_intervals)
{
  assert(reader != NULL);
  assert(account_balance_intervals != NULL);

  return cardano_account_balance_intervals_map_from_cbor(reader, account_balance_intervals);
}

cardano_error_t
cardano_body_write_inputs_if_present(cardano_cbor_writer_t* writer, const cardano_transaction_input_set_t* inputs)
{
  assert(writer != NULL);

  if (inputs != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 0U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_input_set_to_cbor(inputs, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_outputs_if_present(cardano_cbor_writer_t* writer, const cardano_transaction_output_list_t* outputs)
{
  assert(writer != NULL);

  if (outputs != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 1U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_output_list_to_cbor(outputs, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_ttl_if_present(cardano_cbor_writer_t* writer, const uint64_t* ttl)
{
  assert(writer != NULL);

  if (ttl != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 3U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, *ttl);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_certificates_if_present(cardano_cbor_writer_t* writer, const cardano_certificate_set_t* certificates)
{
  assert(writer != NULL);

  if (certificates != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 4U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_certificate_set_to_cbor(certificates, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_withdrawals_if_present(cardano_cbor_writer_t* writer, const cardano_withdrawal_map_t* withdrawals)
{
  assert(writer != NULL);

  if (withdrawals != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 5U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_withdrawal_map_to_cbor(withdrawals, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_aux_data_hash_if_present(cardano_cbor_writer_t* writer, const cardano_blake2b_hash_t* aux_data_hash)
{
  assert(writer != NULL);

  if (aux_data_hash != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 7U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_blake2b_hash_to_cbor(aux_data_hash, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_validity_start_if_present(cardano_cbor_writer_t* writer, const uint64_t* validity_start)
{
  assert(writer != NULL);

  if (validity_start != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 8U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, *validity_start);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_mint_if_present(cardano_cbor_writer_t* writer, const cardano_multi_asset_t* mint)
{
  assert(writer != NULL);

  if (mint != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 9U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_multi_asset_to_cbor(mint, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_script_data_hash_if_present(cardano_cbor_writer_t* writer, const cardano_blake2b_hash_t* script_data_hash)
{
  assert(writer != NULL);

  if (script_data_hash != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 11U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_blake2b_hash_to_cbor(script_data_hash, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_guards_if_present(cardano_cbor_writer_t* writer, const cardano_guard_set_t* guards)
{
  assert(writer != NULL);

  if (guards != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 14U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_guard_set_to_cbor(guards, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_network_id_if_present(cardano_cbor_writer_t* writer, const cardano_network_id_t* network_id)
{
  assert(writer != NULL);

  if (network_id != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 15U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, *network_id);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_reference_inputs_if_present(cardano_cbor_writer_t* writer, const cardano_transaction_input_set_t* reference_inputs)
{
  assert(writer != NULL);

  if (reference_inputs != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 18U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_transaction_input_set_to_cbor(reference_inputs, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_voting_procedures_if_present(cardano_cbor_writer_t* writer, const cardano_voting_procedures_t* voting_procedures)
{
  assert(writer != NULL);

  if (voting_procedures != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 19U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_voting_procedures_to_cbor(voting_procedures, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_proposal_procedures_if_present(cardano_cbor_writer_t* writer, const cardano_proposal_procedure_set_t* proposal_procedures)
{
  assert(writer != NULL);

  if (proposal_procedures != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 20U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_proposal_procedure_set_to_cbor(proposal_procedures, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_treasury_value_if_present(cardano_cbor_writer_t* writer, const uint64_t* treasury_value)
{
  assert(writer != NULL);

  if (treasury_value != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 21U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, *treasury_value);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_donation_if_present(cardano_cbor_writer_t* writer, const uint64_t* donation)
{
  assert(writer != NULL);

  if (donation != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 22U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_cbor_writer_write_uint(writer, *donation);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_required_top_level_guards_if_present(cardano_cbor_writer_t* writer, const cardano_required_guards_map_t* required_top_level_guards)
{
  assert(writer != NULL);

  if (required_top_level_guards != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 24U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_required_guards_map_to_cbor(required_top_level_guards, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_direct_deposits_if_present(cardano_cbor_writer_t* writer, const cardano_direct_deposit_map_t* direct_deposits)
{
  assert(writer != NULL);

  if (direct_deposits != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 25U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_direct_deposit_map_to_cbor(direct_deposits, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_body_write_account_balance_intervals_if_present(cardano_cbor_writer_t* writer, const cardano_account_balance_intervals_map_t* account_balance_intervals)
{
  assert(writer != NULL);

  if (account_balance_intervals != NULL)
  {
    cardano_error_t result = cardano_cbor_writer_write_uint(writer, 26U);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_account_balance_intervals_map_to_cbor(account_balance_intervals, writer);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return CARDANO_SUCCESS;
}
