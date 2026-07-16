/**
 * \file script_context.c
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#include "script_context.h"

#include <cardano/uplc/uplc_apply_params.h>

#include <cardano/address/address.h>
#include <cardano/address/base_address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/address/reward_address.h>
#include <cardano/address/stake_pointer.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/asset_name_map.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/assets/policy_id_list.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/auth_committee_hot_cert.h>
#include <cardano/certs/certificate.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/certs/pool_registration_cert.h>
#include <cardano/certs/pool_retirement_cert.h>
#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/resign_committee_cold_cert.h>
#include <cardano/certs/stake_delegation_cert.h>
#include <cardano/certs/stake_deregistration_cert.h>
#include <cardano/certs/stake_registration_cert.h>
#include <cardano/certs/stake_registration_delegation_cert.h>
#include <cardano/certs/stake_vote_delegation_cert.h>
#include <cardano/certs/stake_vote_registration_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/certs/vote_delegation_cert.h>
#include <cardano/certs/vote_registration_delegation_cert.h>
#include <cardano/common/credential.h>
#include <cardano/common/datum.h>
#include <cardano/common/drep.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/protocol_version.h>
#include <cardano/common/unit_interval.h>
#include <cardano/common/utxo.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_set.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include <cardano/pool_params/pool_params.h>
#include <cardano/proposal_procedures/committee_members_map.h>
#include <cardano/proposal_procedures/constitution.h>
#include <cardano/proposal_procedures/credential_set.h>
#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/proposal_procedures/new_constitution_action.h>
#include <cardano/proposal_procedures/no_confidence_action.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>
#include <cardano/proposal_procedures/update_committee_action.h>
#include <cardano/scripts/script.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_body/value.h>
#include <cardano/voting_procedures/governance_action_id_list.h>
#include <cardano/voting_procedures/vote.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>
#include <cardano/voting_procedures/voting_procedure.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Plutus data constructor alternatives reused across the encoder.
 */
static const uint64_t CONSTR_0 = 0U;
static const uint64_t CONSTR_1 = 1U;
static const uint64_t CONSTR_2 = 2U;
static const uint64_t CONSTR_3 = 3U;
static const uint64_t CONSTR_4 = 4U;
static const uint64_t CONSTR_5 = 5U;
static const uint64_t CONSTR_6 = 6U;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Wraps a list of plutus-data fields in a constructor of the given alternative.
 */
static cardano_error_t
encode_constr(const uint64_t alternative, cardano_plutus_list_t* fields, cardano_plutus_data_t** out)
{
  cardano_constr_plutus_data_t* constr = NULL;
  cardano_error_t               result = cardano_constr_plutus_data_new(alternative, fields, &constr);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_data_new_constr(constr, out);

  cardano_constr_plutus_data_unref(&constr);

  return result;
}

/**
 * \brief Builds an empty-field constructor of the given alternative.
 */
static cardano_error_t
empty_constr(const uint64_t alternative, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* fields = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = encode_constr(alternative, fields, out);

  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Wraps a single plutus-data value in a constructor of the given alternative.
 */
static cardano_error_t
wrap_constr(const uint64_t alternative, cardano_plutus_data_t* value, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* fields = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_list_add(fields, value);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(alternative, fields, out);
  }

  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Creates a plutus-data byte string from a raw buffer.
 */
static cardano_error_t
encode_bytes(const byte_t* data, const size_t size, cardano_plutus_data_t** out)
{
  static const byte_t empty = 0U;

  return cardano_plutus_data_new_bytes(((data == NULL) || (size == 0U)) ? &empty : data, size, out);
}

/**
 * \brief Creates a plutus-data byte string from a blake2b hash.
 */
static cardano_error_t
hash_bytes(const cardano_blake2b_hash_t* hash, cardano_plutus_data_t** out)
{
  return encode_bytes(cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash), out);
}

/**
 * \brief Encodes the boolean values used by the interval bounds as Constr 0/1.
 */
static cardano_error_t
encode_bool(const bool value, cardano_plutus_data_t** out)
{
  return empty_constr(value ? CONSTR_1 : CONSTR_0, out);
}

/**
 * \brief Encodes a stake credential: Constr 0 [key hash] for a key credential, Constr 1 [script hash] for a script credential.
 */
static cardano_error_t
encode_credential(cardano_credential_t* credential, cardano_plutus_data_t** out)
{
  cardano_credential_type_t type   = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  cardano_error_t           result = cardano_credential_get_type(credential, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_plutus_data_t* hash = NULL;

  result = encode_bytes(cardano_credential_get_hash_bytes(credential), cardano_credential_get_hash_bytes_size(credential), &hash);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_CREDENTIAL_TYPE_KEY_HASH:
    {
      result = wrap_constr(CONSTR_0, hash, out);
      break;
    }
    case CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH:
    {
      result = wrap_constr(CONSTR_1, hash, out);
      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_CREDENTIAL_TYPE;
      break;
    }
  }

  cardano_plutus_data_unref(&hash);

  return result;
}

/**
 * \brief Encodes a stake-credential delegation part as the StakingHash variant.
 *
 * Some(Constr 0 [credential]) wrapped again as Constr 0 [value], producing the
 * "StakingHash credential" shape used inside an Address.
 */
static cardano_error_t
some_staking_hash(cardano_credential_t* credential, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* cred    = NULL;
  cardano_error_t        result  = encode_credential(credential, &cred);
  cardano_plutus_data_t* staking = NULL;

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = wrap_constr(CONSTR_0, cred, &staking);

  cardano_plutus_data_unref(&cred);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = wrap_constr(CONSTR_0, staking, out);

  cardano_plutus_data_unref(&staking);

  return result;
}

/**
 * \brief Encodes a stake-pointer delegation part as the StakingPtr variant.
 *
 * Some(Constr 1 [slot, tx index, cert index]) wrapped as Constr 0 [value].
 */
static cardano_error_t
some_staking_ptr(const cardano_stake_pointer_t* pointer, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* fields = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);
  cardano_plutus_data_t* slot   = NULL;
  cardano_plutus_data_t* txidx  = NULL;
  cardano_plutus_data_t* crtidx = NULL;
  cardano_plutus_data_t* ptr    = NULL;

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_data_new_integer_from_uint(pointer->slot, &slot);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, slot);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(pointer->tx_index, &txidx);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, txidx);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(pointer->cert_index, &crtidx);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, crtidx);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_1, fields, &ptr);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(CONSTR_0, ptr, out);
  }

  cardano_plutus_data_unref(&slot);
  cardano_plutus_data_unref(&txidx);
  cardano_plutus_data_unref(&crtidx);
  cardano_plutus_data_unref(&ptr);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes the delegation part of an address as an Option<StakeCredential>.
 *
 * A base address yields Some(StakingHash), a pointer address yields Some(StakingPtr),
 * and an enterprise address yields None (Constr 1).
 */
static cardano_error_t
address_stake_part(const cardano_address_t* address, cardano_plutus_data_t** out)
{
  cardano_base_address_t*    base    = NULL;
  cardano_pointer_address_t* pointer = NULL;
  cardano_error_t            result  = cardano_base_address_from_address(address, &base);

  if ((result == CARDANO_SUCCESS) && (base != NULL))
  {
    cardano_credential_t* stake = cardano_base_address_get_stake_credential(base);

    result = some_staking_hash(stake, out);

    cardano_credential_unref(&stake);
    cardano_base_address_unref(&base);

    return result;
  }

  cardano_base_address_unref(&base);

  result = cardano_pointer_address_from_address(address, &pointer);

  if ((result == CARDANO_SUCCESS) && (pointer != NULL))
  {
    cardano_stake_pointer_t stake_pointer = { 0U, 0U, 0U };

    result = cardano_pointer_address_get_stake_pointer(pointer, &stake_pointer);

    if (result == CARDANO_SUCCESS)
    {
      result = some_staking_ptr(&stake_pointer, out);
    }

    cardano_pointer_address_unref(&pointer);

    return result;
  }

  cardano_pointer_address_unref(&pointer);

  return empty_constr(CONSTR_1, out);
}

/**
 * \brief Extracts the payment credential of a shelley/enterprise/pointer/base address.
 */
static cardano_credential_t*
payment_credential(const cardano_address_t* address)
{
  cardano_base_address_t*       base       = NULL;
  cardano_enterprise_address_t* enterprise = NULL;
  cardano_pointer_address_t*    pointer    = NULL;
  cardano_credential_t*         credential = NULL;

  if ((cardano_base_address_from_address(address, &base) == CARDANO_SUCCESS) && (base != NULL))
  {
    credential = cardano_base_address_get_payment_credential(base);
    cardano_base_address_unref(&base);
    return credential;
  }

  cardano_base_address_unref(&base);

  if ((cardano_enterprise_address_from_address(address, &enterprise) == CARDANO_SUCCESS) && (enterprise != NULL))
  {
    credential = cardano_enterprise_address_get_payment_credential(enterprise);
    cardano_enterprise_address_unref(&enterprise);
    return credential;
  }

  cardano_enterprise_address_unref(&enterprise);

  if ((cardano_pointer_address_from_address(address, &pointer) == CARDANO_SUCCESS) && (pointer != NULL))
  {
    credential = cardano_pointer_address_get_payment_credential(pointer);
    cardano_pointer_address_unref(&pointer);
    return credential;
  }

  cardano_pointer_address_unref(&pointer);

  return NULL;
}

/**
 * \brief Encodes a shelley address: Constr 0 [payment credential, stake part].
 */
static cardano_error_t
encode_address(const cardano_address_t* address, cardano_plutus_data_t** out)
{
  cardano_credential_t* payment = payment_credential(address);

  if (payment == NULL)
  {
    return CARDANO_ERROR_INVALID_ADDRESS_TYPE;
  }

  cardano_plutus_list_t* fields = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);
  cardano_plutus_data_t* pay    = NULL;
  cardano_plutus_data_t* stake  = NULL;

  if (result == CARDANO_SUCCESS)
  {
    result = encode_credential(payment, &pay);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, pay);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = address_stake_part(address, &stake);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, stake);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_credential_unref(&payment);
  cardano_plutus_data_unref(&pay);
  cardano_plutus_data_unref(&stake);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Inserts the ada entry (empty policy id -> {empty asset name -> coin}) into a value map.
 *
 * When \p force_zero is true the entry is always inserted (used by the
 * zero-ada-asset shape of inputs/outputs/fee/mint); otherwise it is only
 * inserted when the coin is positive (the plain Value shape).
 */
static cardano_error_t
value_add_coin(cardano_plutus_map_t* value_map, const int64_t coin, const bool force_zero)
{
  if (!force_zero && (coin <= 0))
  {
    return CARDANO_SUCCESS;
  }

  cardano_plutus_map_t*  inner     = NULL;
  cardano_plutus_data_t* empty_pol = NULL;
  cardano_plutus_data_t* empty_asn = NULL;
  cardano_plutus_data_t* amount    = NULL;
  cardano_plutus_data_t* inner_pd  = NULL;
  cardano_error_t        result    = cardano_plutus_map_new(&inner);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_bytes(NULL, 0U, &empty_asn);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_int(coin, &amount);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_map_insert(inner, empty_asn, amount);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(inner, &inner_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_bytes(NULL, 0U, &empty_pol);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_map_insert(value_map, empty_pol, inner_pd);
  }

  cardano_plutus_map_unref(&inner);
  cardano_plutus_data_unref(&empty_pol);
  cardano_plutus_data_unref(&empty_asn);
  cardano_plutus_data_unref(&amount);
  cardano_plutus_data_unref(&inner_pd);

  return result;
}

/**
 * \brief Appends the multi-asset policies and amounts of a value into a value map.
 */
static cardano_error_t
value_add_multiasset(cardano_plutus_map_t* value_map, cardano_multi_asset_t* multi_asset)
{
  if (multi_asset == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_policy_id_list_t* policies = NULL;
  cardano_error_t           result   = cardano_multi_asset_get_keys(multi_asset, &policies);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t policy_count = cardano_policy_id_list_get_length(policies);

  for (size_t i = 0U; (i < policy_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_blake2b_hash_t*   policy_id = NULL;
    cardano_asset_name_map_t* assets    = NULL;
    cardano_plutus_map_t*     inner     = NULL;
    cardano_plutus_data_t*    policy_pd = NULL;
    cardano_plutus_data_t*    inner_pd  = NULL;

    result = cardano_policy_id_list_get(policies, i, &policy_id);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_multi_asset_get_assets(multi_asset, policy_id, &assets);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_new(&inner);
    }

    const size_t asset_count = (result == CARDANO_SUCCESS) ? cardano_asset_name_map_get_length(assets) : 0U;

    for (size_t j = 0U; (j < asset_count) && (result == CARDANO_SUCCESS); ++j)
    {
      cardano_asset_name_t*  asset_name = NULL;
      int64_t                quantity   = 0;
      cardano_plutus_data_t* name_pd    = NULL;
      cardano_plutus_data_t* qty_pd     = NULL;

      result = cardano_asset_name_map_get_key_value_at(assets, j, &asset_name, &quantity);

      if (result == CARDANO_SUCCESS)
      {
        result = encode_bytes(cardano_asset_name_get_bytes(asset_name), cardano_asset_name_get_bytes_size(asset_name), &name_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_integer_from_int(quantity, &qty_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_map_insert(inner, name_pd, qty_pd);
      }

      cardano_asset_name_unref(&asset_name);
      cardano_plutus_data_unref(&name_pd);
      cardano_plutus_data_unref(&qty_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = hash_bytes(policy_id, &policy_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_map(inner, &inner_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_insert(value_map, policy_pd, inner_pd);
    }

    cardano_blake2b_hash_unref(&policy_id);
    cardano_asset_name_map_unref(&assets);
    cardano_plutus_map_unref(&inner);
    cardano_plutus_data_unref(&policy_pd);
    cardano_plutus_data_unref(&inner_pd);
  }

  cardano_policy_id_list_unref(&policies);

  return result;
}

/**
 * \brief Encodes a transaction-output value as the zero-ada-asset Value map.
 *
 * The ada entry is always present (coin forced even when zero), followed by the
 * sorted multi-asset entries.
 */
static cardano_error_t
encode_value(cardano_value_t* value, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* value_map = NULL;
  cardano_error_t       result    = cardano_plutus_map_new(&value_map);

  if (result == CARDANO_SUCCESS)
  {
    result = value_add_coin(value_map, cardano_value_get_coin(value), true);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);

    result = value_add_multiasset(value_map, multi_asset);

    cardano_multi_asset_unref(&multi_asset);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(value_map, out);
  }

  cardano_plutus_map_unref(&value_map);

  return result;
}

/**
 * \brief Encodes the lovelace-only fee as the zero-ada-asset Value map.
 */
static cardano_error_t
encode_fee(const uint64_t coin, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* value_map = NULL;
  cardano_error_t       result    = cardano_plutus_map_new(&value_map);

  if (result == CARDANO_SUCCESS)
  {
    result = value_add_coin(value_map, (int64_t)coin, true);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(value_map, out);
  }

  cardano_plutus_map_unref(&value_map);

  return result;
}

/**
 * \brief Encodes the mint field as the zero-ada-asset MintValue map.
 *
 * The map always opens with the synthetic ada entry (empty policy ->
 * {empty asset -> 0}) and is followed by the sorted mint policies with signed
 * amounts.
 */
static cardano_error_t
encode_mint(cardano_multi_asset_t* mint, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t*  value_map = NULL;
  cardano_plutus_map_t*  inner     = NULL;
  cardano_plutus_data_t* empty_pol = NULL;
  cardano_plutus_data_t* empty_asn = NULL;
  cardano_plutus_data_t* zero      = NULL;
  cardano_plutus_data_t* inner_pd  = NULL;
  cardano_error_t        result    = cardano_plutus_map_new(&value_map);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_map_new(&inner);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_bytes(NULL, 0U, &empty_asn);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_int(0, &zero);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_map_insert(inner, empty_asn, zero);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(inner, &inner_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_bytes(NULL, 0U, &empty_pol);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_map_insert(value_map, empty_pol, inner_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = value_add_multiasset(value_map, mint);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(value_map, out);
  }

  cardano_plutus_map_unref(&value_map);
  cardano_plutus_map_unref(&inner);
  cardano_plutus_data_unref(&empty_pol);
  cardano_plutus_data_unref(&empty_asn);
  cardano_plutus_data_unref(&zero);
  cardano_plutus_data_unref(&inner_pd);

  return result;
}

/**
 * \brief Encodes the output datum field for V1 (datum hash only, Maybe DatumHash).
 *
 * Some(hash) when the datum is a hash, None otherwise. Inline data in inputs is
 * rejected upstream, so the only datum reaching V1 is a hash.
 */
static cardano_error_t
output_datum_v1(cardano_datum_t* datum, cardano_plutus_data_t** out)
{
  if (datum != NULL)
  {
    cardano_datum_type_t type = CARDANO_DATUM_TYPE_DATA_HASH;

    if (cardano_datum_get_type(datum, &type) == CARDANO_SUCCESS)
    {
      if (type == CARDANO_DATUM_TYPE_DATA_HASH)
      {
        cardano_blake2b_hash_t* hash = cardano_datum_get_data_hash(datum);
        cardano_plutus_data_t*  pd   = NULL;
        cardano_error_t         res  = hash_bytes(hash, &pd);

        cardano_blake2b_hash_unref(&hash);

        if (res != CARDANO_SUCCESS)
        {
          return res;
        }

        res = wrap_constr(CONSTR_0, pd, out);

        cardano_plutus_data_unref(&pd);

        return res;
      }
    }
  }

  return empty_constr(CONSTR_1, out);
}

/**
 * \brief Encodes the output datum option for V2 (NoOutputDatum/Hash/Inline).
 *
 * None -> Constr 0, hash -> Constr 1 [hash], inline data -> Constr 2 [data].
 */
static cardano_error_t
output_datum_v2(cardano_datum_t* datum, cardano_plutus_data_t** out)
{
  if (datum == NULL)
  {
    return empty_constr(CONSTR_0, out);
  }

  cardano_datum_type_t type   = CARDANO_DATUM_TYPE_DATA_HASH;
  cardano_error_t      result = cardano_datum_get_type(datum, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_DATUM_TYPE_DATA_HASH:
    {
      cardano_blake2b_hash_t* hash = cardano_datum_get_data_hash(datum);
      cardano_plutus_data_t*  pd   = NULL;

      result = hash_bytes(hash, &pd);

      cardano_blake2b_hash_unref(&hash);

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_1, pd, out);
      }

      cardano_plutus_data_unref(&pd);

      break;
    }
    case CARDANO_DATUM_TYPE_INLINE_DATA:
    {
      cardano_plutus_data_t* inline_data = cardano_datum_get_inline_data(datum);

      result = wrap_constr(CONSTR_2, inline_data, out);

      cardano_plutus_data_unref(&inline_data);

      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_DATUM_TYPE;
      break;
    }
  }

  return result;
}

/**
 * \brief Encodes the optional script ref of an output as a Maybe ScriptHash.
 *
 * Some(script hash) when present, None otherwise.
 */
static cardano_error_t
output_script_ref(cardano_transaction_output_t* output, cardano_plutus_data_t** out)
{
  cardano_script_t* script = cardano_transaction_output_get_script_ref(output);

  if (script == NULL)
  {
    return empty_constr(CONSTR_1, out);
  }

  cardano_blake2b_hash_t* hash   = cardano_script_get_hash(script);
  cardano_plutus_data_t*  pd     = NULL;
  cardano_error_t         result = (hash != NULL) ? CARDANO_SUCCESS : CARDANO_ERROR_POINTER_IS_NULL;

  if (result == CARDANO_SUCCESS)
  {
    result = hash_bytes(hash, &pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(CONSTR_0, pd, out);
  }

  cardano_blake2b_hash_unref(&hash);
  cardano_script_unref(&script);
  cardano_plutus_data_unref(&pd);

  return result;
}

/**
 * \brief Encodes a transaction output for the given version.
 *
 * V1: Constr 0 [address, value, Maybe datum hash].
 * V2: Constr 0 [address, value, datum option, Maybe script hash].
 */
static cardano_error_t
encode_output(cardano_transaction_output_t* output, const bool is_v2, cardano_plutus_data_t** out)
{
  cardano_address_t*     address = cardano_transaction_output_get_address(output);
  cardano_value_t*       value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*       datum   = cardano_transaction_output_get_datum(output);
  cardano_plutus_list_t* fields  = NULL;
  cardano_plutus_data_t* addr_pd = NULL;
  cardano_plutus_data_t* val_pd  = NULL;
  cardano_plutus_data_t* dat_pd  = NULL;
  cardano_plutus_data_t* ref_pd  = NULL;
  cardano_error_t        result  = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_address(address, &addr_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, addr_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_value(value, &val_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, val_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = is_v2 ? output_datum_v2(datum, &dat_pd) : output_datum_v1(datum, &dat_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, dat_pd);
  }

  if ((result == CARDANO_SUCCESS) && is_v2)
  {
    result = output_script_ref(output, &ref_pd);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(fields, ref_pd);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_address_unref(&address);
  cardano_value_unref(&value);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&addr_pd);
  cardano_plutus_data_unref(&val_pd);
  cardano_plutus_data_unref(&dat_pd);
  cardano_plutus_data_unref(&ref_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes a transaction input (out ref).
 *
 * V1/V2 use the wrapped transaction id: Constr 0 [Constr 0 [id], index].
 */
static cardano_error_t
out_ref_wrapped(cardano_transaction_input_t* input, cardano_plutus_data_t** out)
{
  cardano_blake2b_hash_t* id      = cardano_transaction_input_get_id(input);
  cardano_plutus_list_t*  fields  = NULL;
  cardano_plutus_data_t*  id_pd   = NULL;
  cardano_plutus_data_t*  wrap_id = NULL;
  cardano_plutus_data_t*  idx_pd  = NULL;
  cardano_error_t         result  = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = hash_bytes(id, &id_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(CONSTR_0, id_pd, &wrap_id);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, wrap_id);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(cardano_transaction_input_get_index(input), &idx_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, idx_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_blake2b_hash_unref(&id);
  cardano_plutus_data_unref(&id_pd);
  cardano_plutus_data_unref(&wrap_id);
  cardano_plutus_data_unref(&idx_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Finds the resolved output of an input in the UTxO set.
 *
 * Returns a new reference to the output, or NULL when the input is not present.
 */
static cardano_transaction_output_t*
resolve_output(cardano_utxo_list_t* resolved_inputs, cardano_transaction_input_t* input)
{
  const size_t count = cardano_utxo_list_get_length(resolved_inputs);

  for (size_t i = 0U; i < count; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    if (cardano_utxo_list_get(resolved_inputs, i, &utxo) != CARDANO_SUCCESS)
    {
      cardano_utxo_unref(&utxo);
      return NULL;
    }

    cardano_transaction_input_t* candidate = cardano_utxo_get_input(utxo);

    if (cardano_transaction_input_equals(candidate, input))
    {
      cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);

      cardano_transaction_input_unref(&candidate);
      cardano_utxo_unref(&utxo);

      return output;
    }

    cardano_transaction_input_unref(&candidate);
    cardano_utxo_unref(&utxo);
  }

  return NULL;
}

/**
 * \brief Encodes a single resolved input (TxInInfo).
 *
 * Constr 0 [out ref (wrapped id), resolved output].
 */
static cardano_error_t
tx_in_info(
  cardano_transaction_input_t* input,
  cardano_utxo_list_t*         resolved_inputs,
  const bool                   is_v2,
  cardano_plutus_data_t**      out)
{
  cardano_transaction_output_t* output = resolve_output(resolved_inputs, input);

  if (output == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_plutus_list_t* fields = NULL;
  cardano_plutus_data_t* ref_pd = NULL;
  cardano_plutus_data_t* out_pd = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = out_ref_wrapped(input, &ref_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, ref_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_output(output, is_v2, &out_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, out_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_transaction_output_unref(&output);
  cardano_plutus_data_unref(&ref_pd);
  cardano_plutus_data_unref(&out_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes a set of inputs as the list of resolved TxInInfo.
 */
static cardano_error_t
encode_inputs(
  cardano_transaction_input_set_t* inputs,
  cardano_utxo_list_t*             resolved_inputs,
  const bool                       is_v2,
  cardano_plutus_data_t**          out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (inputs != NULL) ? cardano_transaction_input_set_get_length(inputs) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_transaction_input_t* input = NULL;
    cardano_plutus_data_t*       pd    = NULL;

    result = cardano_transaction_input_set_get(inputs, i, &input);

    if (result == CARDANO_SUCCESS)
    {
      result = tx_in_info(input, resolved_inputs, is_v2, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_transaction_input_unref(&input);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes the transaction outputs as the list of outputs.
 */
static cardano_error_t
encode_outputs(cardano_transaction_output_list_t* outputs, const bool is_v2, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (outputs != NULL) ? cardano_transaction_output_list_get_length(outputs) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_transaction_output_t* output = NULL;
    cardano_plutus_data_t*        pd     = NULL;

    result = cardano_transaction_output_list_get(outputs, i, &output);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_output(output, is_v2, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_transaction_output_unref(&output);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes the required signers as the list of public key hashes.
 */
static cardano_error_t
encode_signatories(cardano_blake2b_hash_set_t* signers, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (signers != NULL) ? cardano_blake2b_hash_set_get_length(signers) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_blake2b_hash_t* hash = NULL;
    cardano_plutus_data_t*  pd   = NULL;

    result = cardano_blake2b_hash_set_get(signers, i, &hash);

    if (result == CARDANO_SUCCESS)
    {
      result = hash_bytes(hash, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_blake2b_hash_unref(&hash);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes the withdrawals as a list of (Constr 0 [stake credential], coin) pairs.
 *
 * V1 uses a list of tuples (each tuple a Constr 0 [key, value]); V2 uses a map.
 * The stake credential is wrapped (the wrapped-stake-credential shape).
 */
static cardano_error_t
encode_withdrawals(cardano_withdrawal_map_t* withdrawals, const bool is_v2, cardano_plutus_data_t** out)
{
  const size_t count = (withdrawals != NULL) ? cardano_withdrawal_map_get_length(withdrawals) : 0U;

  cardano_plutus_list_t* list   = NULL;
  cardano_plutus_map_t*  map    = NULL;
  cardano_error_t        result = CARDANO_SUCCESS;

  if (is_v2)
  {
    result = cardano_plutus_map_new(&map);
  }
  else
  {
    result = cardano_plutus_list_new(&list);
  }

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_reward_address_t* address    = NULL;
    uint64_t                  coin       = 0U;
    cardano_credential_t*     credential = NULL;
    cardano_plutus_data_t*    cred_pd    = NULL;
    cardano_plutus_data_t*    key_pd     = NULL;
    cardano_plutus_data_t*    coin_pd    = NULL;

    result = cardano_withdrawal_map_get_key_value_at(withdrawals, i, &address, &coin);

    if (result == CARDANO_SUCCESS)
    {
      credential = cardano_reward_address_get_credential(address);
      result     = encode_credential(credential, &cred_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = wrap_constr(CONSTR_0, cred_pd, &key_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_integer_from_uint(coin, &coin_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      if (is_v2)
      {
        result = cardano_plutus_map_insert(map, key_pd, coin_pd);
      }
      else
      {
        cardano_plutus_list_t* tuple    = NULL;
        cardano_plutus_data_t* tuple_pd = NULL;

        result = cardano_plutus_list_new(&tuple);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(tuple, key_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(tuple, coin_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_constr(CONSTR_0, tuple, &tuple_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(list, tuple_pd);
        }

        cardano_plutus_list_unref(&tuple);
        cardano_plutus_data_unref(&tuple_pd);
      }
    }

    cardano_reward_address_unref(&address);
    cardano_credential_unref(&credential);
    cardano_plutus_data_unref(&cred_pd);
    cardano_plutus_data_unref(&key_pd);
    cardano_plutus_data_unref(&coin_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    if (is_v2)
    {
      result = cardano_plutus_data_new_map(map, out);
    }
    else
    {
      result = cardano_plutus_data_new_list(list, out);
    }
  }

  cardano_plutus_list_unref(&list);
  cardano_plutus_map_unref(&map);

  return result;
}

cardano_error_t
cardano_uplc_int_slot_to_posix_time(
  const cardano_slot_config_t* slot_config,
  uint64_t                     slot,
  uint64_t*                    posix_time)
{
  uint64_t ms_after_begin = 0U;

  if ((slot_config == NULL) || (posix_time == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (slot < slot_config->zero_slot)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  ms_after_begin = (slot - slot_config->zero_slot) * slot_config->slot_length;

  *posix_time = slot_config->zero_time + ms_after_begin;

  return CARDANO_SUCCESS;
}

/**
 * \brief Converts an absolute slot to its beginning POSIX time (milliseconds).
 *
 * Delegates to the shared \ref cardano_uplc_int_slot_to_posix_time so there is a
 * single implementation of the slot-to-time conversion; this thin wrapper keeps
 * the encoder's existing (slot, slot_config) argument order.
 */
static cardano_error_t
slot_to_posix(const uint64_t slot, const cardano_slot_config_t* slot_config, uint64_t* time)
{
  return cardano_uplc_int_slot_to_posix_time(slot_config, slot, time);
}

/**
 * \brief Encodes one bound of the validity interval as an Extended/LowerBound.
 *
 * A finite bound is Constr 0 [Constr 1 [time], closure]; an infinite bound is
 * Constr 0 [Constr 0|2, true]. Finite lower bounds are inclusive (true) and
 * finite upper bounds are exclusive (false); infinite bounds are exclusive.
 */
static cardano_error_t
interval_bound(const uint64_t* slot, const bool is_lower, const cardano_slot_config_t* slot_config, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* fields  = NULL;
  cardano_plutus_data_t* extreme = NULL;
  cardano_plutus_data_t* closure = NULL;
  cardano_error_t        result  = cardano_plutus_list_new(&fields);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (slot != NULL)
  {
    cardano_plutus_data_t* time  = NULL;
    uint64_t               posix = 0U;

    result = slot_to_posix(*slot, slot_config, &posix);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_integer_from_uint(posix, &time);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = wrap_constr(CONSTR_1, time, &extreme);
    }

    cardano_plutus_data_unref(&time);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_bool(is_lower, &closure);
    }
  }
  else
  {
    result = empty_constr(is_lower ? CONSTR_0 : CONSTR_2, &extreme);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_bool(true, &closure);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, extreme);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, closure);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_plutus_data_unref(&extreme);
  cardano_plutus_data_unref(&closure);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes the validity interval as a POSIX TimeRange: Constr 0 [lower, upper].
 */
static cardano_error_t
valid_range(cardano_transaction_body_t* body, const cardano_slot_config_t* slot_config, cardano_plutus_data_t** out)
{
  const uint64_t* lower = cardano_transaction_body_get_invalid_before(body);
  const uint64_t* upper = cardano_transaction_body_get_invalid_after(body);

  cardano_plutus_list_t* fields   = NULL;
  cardano_plutus_data_t* lower_pd = NULL;
  cardano_plutus_data_t* upper_pd = NULL;
  cardano_error_t        result   = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = interval_bound(lower, true, slot_config, &lower_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, lower_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = interval_bound(upper, false, slot_config, &upper_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, upper_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_plutus_data_unref(&lower_pd);
  cardano_plutus_data_unref(&upper_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Computes the blake2b-256 hash of a plutus-data value from its CBOR.
 *
 * The plutus-data value caches the original CBOR when decoded, so the hash is
 * computed over the original datum bytes.
 */
static cardano_error_t
datum_hash(cardano_plutus_data_t* datum, cardano_blake2b_hash_t** out)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = NULL;
  cardano_error_t   result = cardano_plutus_data_to_cbor(datum, writer);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_cbor_writer_encode_in_buffer(writer, &buffer);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_blake2b_compute_hash(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), CARDANO_BLAKE2B_HASH_SIZE_256, out);
  }

  cardano_buffer_unref(&buffer);
  cardano_cbor_writer_unref(&writer);

  return result;
}

/**
 * \brief Encodes the witness datums as a map of datum hash -> datum.
 *
 * V1 keeps the same map shape (the ledger sorts the witnesses by hash, which
 * the plutus-data set preserves on decode).
 */
static cardano_error_t
encode_data(cardano_plutus_data_set_t* datums, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* map    = NULL;
  cardano_error_t       result = cardano_plutus_map_new(&map);

  const size_t count = (datums != NULL) ? cardano_plutus_data_set_get_length(datums) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_data_t*  datum   = NULL;
    cardano_blake2b_hash_t* hash    = NULL;
    cardano_plutus_data_t*  hash_pd = NULL;

    result = cardano_plutus_data_set_get(datums, i, &datum);

    if (result == CARDANO_SUCCESS)
    {
      result = datum_hash(datum, &hash);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = hash_bytes(hash, &hash_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_insert(map, hash_pd, datum);
    }

    cardano_plutus_data_unref(&datum);
    cardano_blake2b_hash_unref(&hash);
    cardano_plutus_data_unref(&hash_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(map, out);
  }

  cardano_plutus_map_unref(&map);

  return result;
}

/**
 * \brief Encodes a certificate as a V1/V2 DCert (partial certificates).
 *
 * Only the V1/V2-supported certificate kinds are encoded; other kinds yield an
 * unsupported-certificate error.
 */
static cardano_error_t
encode_certificate(cardano_certificate_t* certificate, cardano_plutus_data_t** out)
{
  cardano_cert_type_t type   = CARDANO_CERT_TYPE_STAKE_REGISTRATION;
  cardano_error_t     result = cardano_cert_get_type(certificate, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
    {
      cardano_stake_registration_cert_t* cert       = NULL;
      cardano_credential_t*              credential = NULL;
      cardano_plutus_data_t*             cred_pd    = NULL;
      cardano_plutus_data_t*             wrapped    = NULL;

      result = cardano_certificate_to_stake_registration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_registration_cert_get_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, &wrapped);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, wrapped, out);
      }

      cardano_stake_registration_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&wrapped);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
    {
      cardano_stake_deregistration_cert_t* cert       = NULL;
      cardano_credential_t*                credential = NULL;
      cardano_plutus_data_t*               cred_pd    = NULL;
      cardano_plutus_data_t*               wrapped    = NULL;

      result = cardano_certificate_to_stake_deregistration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_deregistration_cert_get_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, &wrapped);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_1, wrapped, out);
      }

      cardano_stake_deregistration_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&wrapped);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DELEGATION:
    {
      cardano_stake_delegation_cert_t* cert       = NULL;
      cardano_credential_t*            credential = NULL;
      cardano_blake2b_hash_t*          pool       = NULL;
      cardano_plutus_list_t*           fields     = NULL;
      cardano_plutus_data_t*           cred_pd    = NULL;
      cardano_plutus_data_t*           wrapped    = NULL;
      cardano_plutus_data_t*           pool_pd    = NULL;

      result = cardano_certificate_to_stake_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_new(&fields);
      }

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_delegation_cert_get_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, &wrapped);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_add(fields, wrapped);
      }

      if (result == CARDANO_SUCCESS)
      {
        pool   = cardano_stake_delegation_cert_get_pool_key_hash(cert);
        result = hash_bytes(pool, &pool_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_add(fields, pool_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_constr(CONSTR_2, fields, out);
      }

      cardano_stake_delegation_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_blake2b_hash_unref(&pool);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&wrapped);
      cardano_plutus_data_unref(&pool_pd);
      cardano_plutus_list_unref(&fields);

      break;
    }
    case CARDANO_CERT_TYPE_POOL_REGISTRATION:
    {
      cardano_pool_registration_cert_t* cert          = NULL;
      cardano_pool_params_t*            params        = NULL;
      cardano_blake2b_hash_t*           operator_hash = NULL;
      cardano_blake2b_hash_t*           vrf           = NULL;
      cardano_plutus_list_t*            fields        = NULL;
      cardano_plutus_data_t*            op_pd         = NULL;
      cardano_plutus_data_t*            vrf_pd        = NULL;

      result = cardano_certificate_to_pool_registration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_pool_registration_cert_get_params(cert, &params);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_pool_params_get_operator_key_hash(params, &operator_hash);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_pool_params_get_vrf_vk_hash(params, &vrf);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = hash_bytes(operator_hash, &op_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = hash_bytes(vrf, &vrf_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_new(&fields);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_add(fields, op_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_add(fields, vrf_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_constr(CONSTR_3, fields, out);
      }

      cardano_pool_registration_cert_unref(&cert);
      cardano_pool_params_unref(&params);
      cardano_blake2b_hash_unref(&operator_hash);
      cardano_blake2b_hash_unref(&vrf);
      cardano_plutus_list_unref(&fields);
      cardano_plutus_data_unref(&op_pd);
      cardano_plutus_data_unref(&vrf_pd);

      break;
    }
    case CARDANO_CERT_TYPE_POOL_RETIREMENT:
    {
      cardano_pool_retirement_cert_t* cert     = NULL;
      cardano_blake2b_hash_t*         pool     = NULL;
      cardano_plutus_list_t*          fields   = NULL;
      cardano_plutus_data_t*          pool_pd  = NULL;
      cardano_plutus_data_t*          epoch_pd = NULL;

      result = cardano_certificate_to_pool_retirement(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        pool   = cardano_pool_retirement_cert_get_pool_key_hash(cert);
        result = hash_bytes(pool, &pool_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_integer_from_uint(cardano_pool_retirement_cert_get_epoch(cert), &epoch_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_new(&fields);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_add(fields, pool_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_list_add(fields, epoch_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_constr(CONSTR_4, fields, out);
      }

      cardano_pool_retirement_cert_unref(&cert);
      cardano_blake2b_hash_unref(&pool);
      cardano_plutus_list_unref(&fields);
      cardano_plutus_data_unref(&pool_pd);
      cardano_plutus_data_unref(&epoch_pd);

      break;
    }
    case CARDANO_CERT_TYPE_REGISTRATION:
    {
      cardano_registration_cert_t* cert       = NULL;
      cardano_credential_t*        credential = NULL;
      cardano_plutus_data_t*       cred_pd    = NULL;
      cardano_plutus_data_t*       wrapped    = NULL;

      result = cardano_certificate_to_registration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_registration_cert_get_stake_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, &wrapped);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, wrapped, out);
      }

      cardano_registration_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&wrapped);

      break;
    }
    case CARDANO_CERT_TYPE_UNREGISTRATION:
    {
      cardano_unregistration_cert_t* cert       = NULL;
      cardano_credential_t*          credential = NULL;
      cardano_plutus_data_t*         cred_pd    = NULL;
      cardano_plutus_data_t*         wrapped    = NULL;

      result = cardano_certificate_to_unregistration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_unregistration_cert_get_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, &wrapped);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_1, wrapped, out);
      }

      cardano_unregistration_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&wrapped);

      break;
    }
    case CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION:
    case CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS:
    case CARDANO_CERT_TYPE_VOTE_DELEGATION:
    case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
    case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
    case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
    case CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT:
    case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
    case CARDANO_CERT_TYPE_DREP_REGISTRATION:
    case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
    case CARDANO_CERT_TYPE_UPDATE_DREP:
    default:
    {
      result = CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
      break;
    }
  }

  return result;
}

/**
 * \brief Encodes the certificates as the list of DCert.
 */
static cardano_error_t
encode_certificates(cardano_certificate_set_t* certificates, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (certificates != NULL) ? cardano_certificate_set_get_length(certificates) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_certificate_t* certificate = NULL;
    cardano_plutus_data_t* pd          = NULL;

    result = cardano_certificate_set_get(certificates, i, &certificate);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_certificate(certificate, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_certificate_unref(&certificate);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes the redeemers of a V2 transaction as a map of script purpose -> redeemer data.
 */
static cardano_error_t
redeemers_map_v2(
  cardano_transaction_t*   tx,
  cardano_utxo_list_t*     resolved_inputs,
  cardano_redeemer_list_t* redeemers,
  cardano_plutus_data_t**  out)
{
  cardano_plutus_map_t* map    = NULL;
  cardano_error_t       result = cardano_plutus_map_new(&map);

  const size_t count = (redeemers != NULL) ? cardano_redeemer_list_get_length(redeemers) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_redeemer_t*    redeemer   = NULL;
    cardano_plutus_data_t* purpose_pd = NULL;
    cardano_plutus_data_t* data       = NULL;

    result = cardano_redeemer_list_get(redeemers, i, &redeemer);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_int_build_script_purpose_v1v2(tx, resolved_inputs, redeemer, &purpose_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      data   = cardano_redeemer_get_data(redeemer);
      result = (data != NULL) ? cardano_plutus_map_insert(map, purpose_pd, data) : CARDANO_ERROR_POINTER_IS_NULL;
    }

    cardano_redeemer_unref(&redeemer);
    cardano_plutus_data_unref(&purpose_pd);
    cardano_plutus_data_unref(&data);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(map, out);
  }

  cardano_plutus_map_unref(&map);

  return result;
}

/**
 * \brief Rejects a transaction that carries Conway-only fields a V1/V2 context cannot represent.
 *
 * Mirrors the ledger's \c guardConwayFeaturesForPlutusV1V2: a V1 or V2 script
 * context cannot be built for a transaction with voting procedures, proposal
 * procedures, a current-treasury value, or a non-zero treasury donation, so such
 * a transaction is rejected rather than silently mis-encoded.
 */
static cardano_error_t
guard_conway_features_v1v2(cardano_transaction_body_t* body)
{
  cardano_error_t result = CARDANO_SUCCESS;

  cardano_voting_procedures_t* votes = cardano_transaction_body_get_voting_procedures(body);

  if (votes != NULL)
  {
    cardano_voter_list_t* voters = NULL;

    if (cardano_voting_procedures_get_voters(votes, &voters) == CARDANO_SUCCESS)
    {
      if (cardano_voter_list_get_length(voters) > 0U)
      {
        result = CARDANO_ERROR_INVALID_ARGUMENT;
      }
    }

    cardano_voter_list_unref(&voters);
  }

  cardano_voting_procedures_unref(&votes);

  if (result == CARDANO_SUCCESS)
  {
    cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);

    if ((proposals != NULL) && (cardano_proposal_procedure_set_get_length(proposals) > 0U))
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
    }

    cardano_proposal_procedure_set_unref(&proposals);
  }

  if (result == CARDANO_SUCCESS)
  {
    const uint64_t* treasury = cardano_transaction_body_get_treasury_value(body);

    if (treasury != NULL)
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    const uint64_t* donation = cardano_transaction_body_get_donation(body);

    if ((donation != NULL) && (*donation != 0U))
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }

  return result;
}

/**
 * \brief Validates the reference inputs of a V1 transaction (which has no reference-input field).
 *
 * Mirrors the ledger's V1 \c toPlutusTxInfo, which resolves each reference input
 * through \c transTxInInfoV1 (and so fails on an output the V1 TxOut cannot
 * represent — an inline datum or a reference script) and then discards it. Our
 * V1 output encoder silently drops those fields, so the representability check is
 * done explicitly here.
 */
static cardano_error_t
validate_v1_reference_inputs(cardano_transaction_input_set_t* ref_inputs, cardano_utxo_list_t* resolved_inputs)
{
  const size_t    count  = (ref_inputs != NULL) ? cardano_transaction_input_set_get_length(ref_inputs) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (result == CARDANO_SUCCESS) && (i < count); ++i)
  {
    cardano_transaction_input_t* input = NULL;

    result = cardano_transaction_input_set_get(ref_inputs, i, &input);

    if (result == CARDANO_SUCCESS)
    {
      cardano_transaction_output_t* output = resolve_output(resolved_inputs, input);

      if (output == NULL)
      {
        result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
      }
      else
      {
        cardano_script_t*    script = cardano_transaction_output_get_script_ref(output);
        cardano_datum_t*     datum  = cardano_transaction_output_get_datum(output);
        cardano_datum_type_t dtype  = CARDANO_DATUM_TYPE_DATA_HASH;

        const bool has_script = (script != NULL);
        const bool has_inline = (datum != NULL) && (cardano_datum_get_type(datum, &dtype) == CARDANO_SUCCESS) && (dtype == CARDANO_DATUM_TYPE_INLINE_DATA);

        if (has_script || has_inline)
        {
          result = CARDANO_ERROR_INVALID_ARGUMENT;
        }

        cardano_script_unref(&script);
        cardano_datum_unref(&datum);
        cardano_transaction_output_unref(&output);
      }
    }

    cardano_transaction_input_unref(&input);
  }

  return result;
}

/**
 * \brief Encodes the TxInfo of a transaction for V1 or V2.
 */
static cardano_error_t
build_tx_info(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  const bool                   is_v2,
  cardano_plutus_data_t**      out)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (slot_config == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body        = cardano_transaction_get_body(tx);
  cardano_witness_set_t*      witness_set = cardano_transaction_get_witness_set(tx);
  cardano_plutus_list_t*      fields      = NULL;
  cardano_error_t             result      = cardano_plutus_list_new(&fields);

  cardano_transaction_input_set_t*   inputs     = NULL;
  cardano_transaction_input_set_t*   ref_inputs = NULL;
  cardano_transaction_output_list_t* outputs    = NULL;
  cardano_multi_asset_t*             mint       = NULL;
  cardano_certificate_set_t*         certs      = NULL;
  cardano_withdrawal_map_t*          withdraws  = NULL;
  cardano_blake2b_hash_set_t*        signers    = NULL;
  cardano_plutus_data_set_t*         datums     = NULL;
  cardano_redeemer_list_t*           redeemers  = NULL;
  cardano_blake2b_hash_t*            tx_id      = NULL;

  cardano_plutus_data_t* inputs_pd  = NULL;
  cardano_plutus_data_t* refs_pd    = NULL;
  cardano_plutus_data_t* outputs_pd = NULL;
  cardano_plutus_data_t* fee_pd     = NULL;
  cardano_plutus_data_t* mint_pd    = NULL;
  cardano_plutus_data_t* certs_pd   = NULL;
  cardano_plutus_data_t* with_pd    = NULL;
  cardano_plutus_data_t* range_pd   = NULL;
  cardano_plutus_data_t* signs_pd   = NULL;
  cardano_plutus_data_t* redeem_pd  = NULL;
  cardano_plutus_data_t* data_pd    = NULL;
  cardano_plutus_data_t* id_pd      = NULL;
  cardano_plutus_data_t* wrap_id_pd = NULL;

  if (result == CARDANO_SUCCESS)
  {
    result = guard_conway_features_v1v2(body);
  }

  if (result == CARDANO_SUCCESS)
  {
    inputs     = cardano_transaction_body_get_inputs(body);
    ref_inputs = cardano_transaction_body_get_reference_inputs(body);

    if (!is_v2)
    {
      result = validate_v1_reference_inputs(ref_inputs, resolved_inputs);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_inputs(inputs, resolved_inputs, is_v2, &inputs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, inputs_pd);
  }

  if ((result == CARDANO_SUCCESS) && is_v2)
  {
    result = encode_inputs(ref_inputs, resolved_inputs, is_v2, &refs_pd);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(fields, refs_pd);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    outputs = cardano_transaction_body_get_outputs(body);
    result  = encode_outputs(outputs, is_v2, &outputs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, outputs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_fee(cardano_transaction_body_get_fee(body), &fee_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, fee_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    mint   = cardano_transaction_body_get_mint(body);
    result = encode_mint(mint, &mint_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, mint_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    certs  = cardano_transaction_body_get_certificates(body);
    result = encode_certificates(certs, &certs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, certs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    withdraws = cardano_transaction_body_get_withdrawals(body);
    result    = encode_withdrawals(withdraws, is_v2, &with_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, with_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = valid_range(body, slot_config, &range_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, range_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    signers = cardano_transaction_body_get_required_signers(body);
    result  = encode_signatories(signers, &signs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, signs_pd);
  }

  if ((result == CARDANO_SUCCESS) && is_v2)
  {
    redeemers = cardano_witness_set_get_redeemers(witness_set);
    result    = redeemers_map_v2(tx, resolved_inputs, redeemers, &redeem_pd);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(fields, redeem_pd);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    datums = cardano_witness_set_get_plutus_data(witness_set);
    result = encode_data(datums, &data_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, data_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    tx_id  = cardano_transaction_get_id(tx);
    result = (tx_id != NULL) ? hash_bytes(tx_id, &id_pd) : CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(CONSTR_0, id_pd, &wrap_id_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, wrap_id_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_set_unref(&ref_inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_multi_asset_unref(&mint);
  cardano_certificate_set_unref(&certs);
  cardano_withdrawal_map_unref(&withdraws);
  cardano_blake2b_hash_set_unref(&signers);
  cardano_plutus_data_set_unref(&datums);
  cardano_redeemer_list_unref(&redeemers);
  cardano_blake2b_hash_unref(&tx_id);

  cardano_plutus_data_unref(&inputs_pd);
  cardano_plutus_data_unref(&refs_pd);
  cardano_plutus_data_unref(&outputs_pd);
  cardano_plutus_data_unref(&fee_pd);
  cardano_plutus_data_unref(&mint_pd);
  cardano_plutus_data_unref(&certs_pd);
  cardano_plutus_data_unref(&with_pd);
  cardano_plutus_data_unref(&range_pd);
  cardano_plutus_data_unref(&signs_pd);
  cardano_plutus_data_unref(&redeem_pd);
  cardano_plutus_data_unref(&data_pd);
  cardano_plutus_data_unref(&id_pd);
  cardano_plutus_data_unref(&wrap_id_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/* V3 STATIC FUNCTIONS *******************************************************/

/**
 * \brief Wraps a value as Some(x) = Constr 0 [x]; a NULL value yields None = Constr 1.
 */
static cardano_error_t
encode_maybe(cardano_plutus_data_t* value, cardano_plutus_data_t** out)
{
  if (value == NULL)
  {
    return empty_constr(CONSTR_1, out);
  }

  return wrap_constr(CONSTR_0, value, out);
}

/**
 * \brief Encodes a value as the V3 plain Value map: the ada entry only when the coin is positive.
 */
static cardano_error_t
value_plain(cardano_value_t* value, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* value_map = NULL;
  cardano_error_t       result    = cardano_plutus_map_new(&value_map);

  if (result == CARDANO_SUCCESS)
  {
    result = value_add_coin(value_map, cardano_value_get_coin(value), false);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);

    result = value_add_multiasset(value_map, multi_asset);

    cardano_multi_asset_unref(&multi_asset);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(value_map, out);
  }

  cardano_plutus_map_unref(&value_map);

  return result;
}

/**
 * \brief Encodes the V3 mint as the plain MintValue map: signed amounts, no synthetic ada entry.
 */
static cardano_error_t
mint_plain(cardano_multi_asset_t* mint, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* value_map = NULL;
  cardano_error_t       result    = cardano_plutus_map_new(&value_map);

  if (result == CARDANO_SUCCESS)
  {
    result = value_add_multiasset(value_map, mint);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(value_map, out);
  }

  cardano_plutus_map_unref(&value_map);

  return result;
}

/**
 * \brief Encodes a V3 transaction output: Constr 0 [address, plain value, datum option, Maybe script hash].
 */
static cardano_error_t
output_v3(cardano_transaction_output_t* output, cardano_plutus_data_t** out)
{
  cardano_address_t*     address = cardano_transaction_output_get_address(output);
  cardano_value_t*       value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*       datum   = cardano_transaction_output_get_datum(output);
  cardano_plutus_list_t* fields  = NULL;
  cardano_plutus_data_t* addr_pd = NULL;
  cardano_plutus_data_t* val_pd  = NULL;
  cardano_plutus_data_t* dat_pd  = NULL;
  cardano_plutus_data_t* ref_pd  = NULL;
  cardano_error_t        result  = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_address(address, &addr_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, addr_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = value_plain(value, &val_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, val_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = output_datum_v2(datum, &dat_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, dat_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = output_script_ref(output, &ref_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, ref_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_address_unref(&address);
  cardano_value_unref(&value);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&addr_pd);
  cardano_plutus_data_unref(&val_pd);
  cardano_plutus_data_unref(&dat_pd);
  cardano_plutus_data_unref(&ref_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes a V3 transaction input (out ref) with the unwrapped id: Constr 0 [id, index].
 */
static cardano_error_t
out_ref_v3(cardano_transaction_input_t* input, cardano_plutus_data_t** out)
{
  cardano_blake2b_hash_t* id     = cardano_transaction_input_get_id(input);
  cardano_plutus_list_t*  fields = NULL;
  cardano_plutus_data_t*  id_pd  = NULL;
  cardano_plutus_data_t*  idx_pd = NULL;
  cardano_error_t         result = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = hash_bytes(id, &id_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, id_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(cardano_transaction_input_get_index(input), &idx_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, idx_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_blake2b_hash_unref(&id);
  cardano_plutus_data_unref(&id_pd);
  cardano_plutus_data_unref(&idx_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes a single resolved input as the V3 TxInInfo: Constr 0 [out ref, resolved output].
 */
static cardano_error_t
tx_in_info_v3(
  cardano_transaction_input_t* input,
  cardano_utxo_list_t*         resolved_inputs,
  cardano_plutus_data_t**      out)
{
  cardano_transaction_output_t* output = resolve_output(resolved_inputs, input);

  if (output == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  cardano_plutus_list_t* fields = NULL;
  cardano_plutus_data_t* ref_pd = NULL;
  cardano_plutus_data_t* out_pd = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = out_ref_v3(input, &ref_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, ref_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = output_v3(output, &out_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, out_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, out);
  }

  cardano_transaction_output_unref(&output);
  cardano_plutus_data_unref(&ref_pd);
  cardano_plutus_data_unref(&out_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes a set of inputs as the V3 list of resolved TxInInfo.
 */
static cardano_error_t
inputs_v3(
  cardano_transaction_input_set_t* inputs,
  cardano_utxo_list_t*             resolved_inputs,
  cardano_plutus_data_t**          out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (inputs != NULL) ? cardano_transaction_input_set_get_length(inputs) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_transaction_input_t* input = NULL;
    cardano_plutus_data_t*       pd    = NULL;

    result = cardano_transaction_input_set_get(inputs, i, &input);

    if (result == CARDANO_SUCCESS)
    {
      result = tx_in_info_v3(input, resolved_inputs, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_transaction_input_unref(&input);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes the transaction outputs as the V3 list of outputs.
 */
static cardano_error_t
outputs_v3(cardano_transaction_output_list_t* outputs, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (outputs != NULL) ? cardano_transaction_output_list_get_length(outputs) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_transaction_output_t* output = NULL;
    cardano_plutus_data_t*        pd     = NULL;

    result = cardano_transaction_output_list_get(outputs, i, &output);

    if (result == CARDANO_SUCCESS)
    {
      result = output_v3(output, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_transaction_output_unref(&output);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes the V3 withdrawals as a map keyed by a bare staking credential.
 *
 * The V3 \c txInfoWdrl field has type \c Map Credential Lovelace, so the key is
 * a bare \c Credential (Constr 0 [pubKeyHash] or Constr 1 [scriptHash]); not a
 * full Address and not the V1/V2 StakingHash wrapper.
 */
static cardano_error_t
withdrawals_v3(cardano_withdrawal_map_t* withdrawals, cardano_plutus_data_t** out)
{
  const size_t count = (withdrawals != NULL) ? cardano_withdrawal_map_get_length(withdrawals) : 0U;

  cardano_plutus_map_t* map    = NULL;
  cardano_error_t       result = cardano_plutus_map_new(&map);

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_reward_address_t* reward     = NULL;
    uint64_t                  coin       = 0U;
    cardano_credential_t*     credential = NULL;
    cardano_plutus_data_t*    key_pd     = NULL;
    cardano_plutus_data_t*    coin_pd    = NULL;

    result = cardano_withdrawal_map_get_key_value_at(withdrawals, i, &reward, &coin);

    if (result == CARDANO_SUCCESS)
    {
      credential = cardano_reward_address_get_credential(reward);
      result     = encode_credential(credential, &key_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_integer_from_uint(coin, &coin_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_insert(map, key_pd, coin_pd);
    }

    cardano_reward_address_unref(&reward);
    cardano_credential_unref(&credential);
    cardano_plutus_data_unref(&key_pd);
    cardano_plutus_data_unref(&coin_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(map, out);
  }

  cardano_plutus_map_unref(&map);

  return result;
}

/**
 * \brief Encodes a DRep.
 *
 * Key => Constr 0 [Constr 0 [hash]], Script => Constr 0 [Constr 1 [hash]],
 * Abstain => Constr 1, NoConfidence => Constr 2.
 */
static cardano_error_t
encode_drep(cardano_drep_t* drep, cardano_plutus_data_t** out)
{
  cardano_drep_type_t type   = CARDANO_DREP_TYPE_KEY_HASH;
  cardano_error_t     result = cardano_drep_get_type(drep, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_DREP_TYPE_KEY_HASH:
    case CARDANO_DREP_TYPE_SCRIPT_HASH:
    {
      cardano_credential_t*  credential = NULL;
      cardano_plutus_data_t* cred_pd    = NULL;

      result = cardano_drep_get_credential(drep, &credential);

      if (result == CARDANO_SUCCESS)
      {
        result = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, out);
      }

      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);

      break;
    }
    case CARDANO_DREP_TYPE_ABSTAIN:
    {
      result = empty_constr(CONSTR_1, out);
      break;
    }
    case CARDANO_DREP_TYPE_NO_CONFIDENCE:
    {
      result = empty_constr(CONSTR_2, out);
      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/**
 * \brief Encodes the second field of a Conway delegation certificate as a Delegatee.
 *
 * Delegatee::Pool(pool) => Constr 0 [pool], Delegatee::DRep(drep) => Constr 1 [drep],
 * Delegatee::StakeAndVote(pool, drep) => Constr 2 [pool, drep].
 */
static cardano_error_t
delegatee_pool(cardano_blake2b_hash_t* pool, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* pool_pd = NULL;
  cardano_error_t        result  = hash_bytes(pool, &pool_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(CONSTR_0, pool_pd, out);
  }

  cardano_plutus_data_unref(&pool_pd);

  return result;
}

/**
 * \brief Encodes a credential wrapped in the Conway delegation deposit shape Constr alt [...].
 */
static cardano_error_t
constr_two(const uint64_t alternative, cardano_plutus_data_t* first, cardano_plutus_data_t* second, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* fields = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, first);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, second);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(alternative, fields, out);
  }

  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes a 3-field constructor.
 */
static cardano_error_t
constr_three(
  const uint64_t          alternative,
  cardano_plutus_data_t*  first,
  cardano_plutus_data_t*  second,
  cardano_plutus_data_t*  third,
  cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* fields = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, first);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, second);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, third);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(alternative, fields, out);
  }

  cardano_plutus_list_unref(&fields);

  return result;
}

/**
 * \brief Encodes the Conway StakeRegistration / Reg certificate: Constr 0 [credential, deposit].
 *
 * The deposit is \c Just for the Conway registration-with-deposit certificate and
 * \c Nothing for the legacy (no-deposit) registration, matching the ledger's
 * \c transTxCert TxCertRegStaking translation (post the Conway bootstrap phase).
 */
static cardano_error_t
cert_v3_register(cardano_credential_t* credential, const uint64_t* deposit, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* cred_pd    = NULL;
  cardano_plutus_data_t* deposit_pd = NULL;
  cardano_plutus_data_t* maybe      = NULL;
  cardano_error_t        result     = encode_credential(credential, &cred_pd);

  if ((result == CARDANO_SUCCESS) && (deposit != NULL))
  {
    result = cardano_plutus_data_new_integer_from_uint(*deposit, &deposit_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_maybe(deposit_pd, &maybe);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = constr_two(CONSTR_0, cred_pd, maybe, out);
  }

  cardano_plutus_data_unref(&cred_pd);
  cardano_plutus_data_unref(&deposit_pd);
  cardano_plutus_data_unref(&maybe);

  return result;
}

/**
 * \brief Encodes the Conway StakeDeregistration / UnReg certificate: Constr 1 [credential, refund].
 *
 * The refund is \c Just for the Conway unregistration-with-refund certificate and
 * \c Nothing for the legacy (no-deposit) deregistration, matching the ledger's
 * \c transTxCert TxCertUnRegStaking translation (post the Conway bootstrap phase).
 */
static cardano_error_t
cert_v3_unregister(cardano_credential_t* credential, const uint64_t* refund, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* cred_pd   = NULL;
  cardano_plutus_data_t* refund_pd = NULL;
  cardano_plutus_data_t* maybe     = NULL;
  cardano_error_t        result    = encode_credential(credential, &cred_pd);

  if ((result == CARDANO_SUCCESS) && (refund != NULL))
  {
    result = cardano_plutus_data_new_integer_from_uint(*refund, &refund_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_maybe(refund_pd, &maybe);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = constr_two(CONSTR_1, cred_pd, maybe, out);
  }

  cardano_plutus_data_unref(&cred_pd);
  cardano_plutus_data_unref(&refund_pd);
  cardano_plutus_data_unref(&maybe);

  return result;
}

/**
 * \brief Encodes a Conway delegation certificate: Constr 2 [credential, delegatee].
 */
static cardano_error_t
cert_v3_delegation(cardano_credential_t* credential, cardano_plutus_data_t* delegatee, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* cred_pd = NULL;
  cardano_error_t        result  = encode_credential(credential, &cred_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = constr_two(CONSTR_2, cred_pd, delegatee, out);
  }

  cardano_plutus_data_unref(&cred_pd);

  return result;
}

/**
 * \brief Encodes a Conway registration-and-delegation certificate: Constr 3 [credential, delegatee, deposit].
 */
static cardano_error_t
cert_v3_reg_delegation(
  cardano_credential_t*   credential,
  cardano_plutus_data_t*  delegatee,
  const uint64_t          deposit,
  cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* cred_pd    = NULL;
  cardano_plutus_data_t* deposit_pd = NULL;
  cardano_error_t        result     = encode_credential(credential, &cred_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(deposit, &deposit_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = constr_three(CONSTR_3, cred_pd, delegatee, deposit_pd, out);
  }

  cardano_plutus_data_unref(&cred_pd);
  cardano_plutus_data_unref(&deposit_pd);

  return result;
}

/**
 * \brief Encodes a Conway certificate as a V3 Certificate (never-registration-deposit shape).
 *
 * Covers every certificate kind encoded for V3. Stake registration and
 * deregistration always emit a None deposit (the never-registration-deposit
 * convention). Returns an error for unsupported (genesis / MIR) kinds.
 */
static cardano_error_t
certificate_v3(cardano_certificate_t* certificate, cardano_plutus_data_t** out)
{
  cardano_cert_type_t type   = CARDANO_CERT_TYPE_STAKE_REGISTRATION;
  cardano_error_t     result = cardano_cert_get_type(certificate, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
    {
      cardano_stake_registration_cert_t* cert       = NULL;
      cardano_credential_t*              credential = NULL;

      result = cardano_certificate_to_stake_registration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_registration_cert_get_credential(cert);
        result     = cert_v3_register(credential, NULL, out);
      }

      cardano_stake_registration_cert_unref(&cert);
      cardano_credential_unref(&credential);

      break;
    }
    case CARDANO_CERT_TYPE_REGISTRATION:
    {
      cardano_registration_cert_t* cert       = NULL;
      cardano_credential_t*        credential = NULL;

      result = cardano_certificate_to_registration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential             = cardano_registration_cert_get_stake_credential(cert);
        const uint64_t deposit = cardano_registration_cert_get_deposit(cert);
        result                 = cert_v3_register(credential, &deposit, out);
      }

      cardano_registration_cert_unref(&cert);
      cardano_credential_unref(&credential);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
    {
      cardano_stake_deregistration_cert_t* cert       = NULL;
      cardano_credential_t*                credential = NULL;

      result = cardano_certificate_to_stake_deregistration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_deregistration_cert_get_credential(cert);
        result     = cert_v3_unregister(credential, NULL, out);
      }

      cardano_stake_deregistration_cert_unref(&cert);
      cardano_credential_unref(&credential);

      break;
    }
    case CARDANO_CERT_TYPE_UNREGISTRATION:
    {
      cardano_unregistration_cert_t* cert       = NULL;
      cardano_credential_t*          credential = NULL;

      result = cardano_certificate_to_unregistration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential            = cardano_unregistration_cert_get_credential(cert);
        const uint64_t refund = cardano_unregistration_cert_get_deposit(cert);
        result                = cert_v3_unregister(credential, &refund, out);
      }

      cardano_unregistration_cert_unref(&cert);
      cardano_credential_unref(&credential);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DELEGATION:
    {
      cardano_stake_delegation_cert_t* cert       = NULL;
      cardano_credential_t*            credential = NULL;
      cardano_blake2b_hash_t*          pool       = NULL;
      cardano_plutus_data_t*           delegatee  = NULL;

      result = cardano_certificate_to_stake_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_delegation_cert_get_credential(cert);
        pool       = cardano_stake_delegation_cert_get_pool_key_hash(cert);
        result     = delegatee_pool(pool, &delegatee);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cert_v3_delegation(credential, delegatee, out);
      }

      cardano_stake_delegation_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_blake2b_hash_unref(&pool);
      cardano_plutus_data_unref(&delegatee);

      break;
    }
    case CARDANO_CERT_TYPE_VOTE_DELEGATION:
    {
      cardano_vote_delegation_cert_t* cert       = NULL;
      cardano_credential_t*           credential = NULL;
      cardano_drep_t*                 drep       = NULL;
      cardano_plutus_data_t*          drep_pd    = NULL;
      cardano_plutus_data_t*          delegatee  = NULL;

      result = cardano_certificate_to_vote_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_vote_delegation_cert_get_credential(cert);
        drep       = cardano_vote_delegation_cert_get_drep(cert);
        result     = encode_drep(drep, &drep_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_1, drep_pd, &delegatee);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cert_v3_delegation(credential, delegatee, out);
      }

      cardano_vote_delegation_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_drep_unref(&drep);
      cardano_plutus_data_unref(&drep_pd);
      cardano_plutus_data_unref(&delegatee);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
    {
      cardano_stake_vote_delegation_cert_t* cert       = NULL;
      cardano_credential_t*                 credential = NULL;
      cardano_blake2b_hash_t*               pool       = NULL;
      cardano_drep_t*                       drep       = NULL;
      cardano_plutus_data_t*                pool_pd    = NULL;
      cardano_plutus_data_t*                drep_pd    = NULL;
      cardano_plutus_data_t*                delegatee  = NULL;

      result = cardano_certificate_to_stake_vote_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_vote_delegation_cert_get_credential(cert);
        pool       = cardano_stake_vote_delegation_cert_get_pool_key_hash(cert);
        drep       = cardano_stake_vote_delegation_cert_get_drep(cert);
        result     = hash_bytes(pool, &pool_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_drep(drep, &drep_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_2, pool_pd, drep_pd, &delegatee);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cert_v3_delegation(credential, delegatee, out);
      }

      cardano_stake_vote_delegation_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_blake2b_hash_unref(&pool);
      cardano_drep_unref(&drep);
      cardano_plutus_data_unref(&pool_pd);
      cardano_plutus_data_unref(&drep_pd);
      cardano_plutus_data_unref(&delegatee);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
    {
      cardano_stake_registration_delegation_cert_t* cert       = NULL;
      cardano_credential_t*                         credential = NULL;
      cardano_blake2b_hash_t*                       pool       = NULL;
      cardano_plutus_data_t*                        delegatee  = NULL;

      result = cardano_certificate_to_stake_registration_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_registration_delegation_cert_get_credential(cert);
        pool       = cardano_stake_registration_delegation_cert_get_pool_key_hash(cert);
        result     = delegatee_pool(pool, &delegatee);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cert_v3_reg_delegation(credential, delegatee, cardano_stake_registration_delegation_cert_get_deposit(cert), out);
      }

      cardano_stake_registration_delegation_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_blake2b_hash_unref(&pool);
      cardano_plutus_data_unref(&delegatee);

      break;
    }
    case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_vote_registration_delegation_cert_t* cert       = NULL;
      cardano_credential_t*                        credential = NULL;
      cardano_drep_t*                              drep       = NULL;
      cardano_plutus_data_t*                       drep_pd    = NULL;
      cardano_plutus_data_t*                       delegatee  = NULL;

      result = cardano_certificate_to_vote_registration_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_vote_registration_delegation_cert_get_credential(cert);
        drep       = cardano_vote_registration_delegation_cert_get_drep(cert);
        result     = encode_drep(drep, &drep_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_1, drep_pd, &delegatee);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cert_v3_reg_delegation(credential, delegatee, cardano_vote_registration_delegation_cert_get_deposit(cert), out);
      }

      cardano_vote_registration_delegation_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_drep_unref(&drep);
      cardano_plutus_data_unref(&drep_pd);
      cardano_plutus_data_unref(&delegatee);

      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_stake_vote_registration_delegation_cert_t* cert       = NULL;
      cardano_credential_t*                              credential = NULL;
      cardano_blake2b_hash_t*                            pool       = NULL;
      cardano_drep_t*                                    drep       = NULL;
      cardano_plutus_data_t*                             pool_pd    = NULL;
      cardano_plutus_data_t*                             drep_pd    = NULL;
      cardano_plutus_data_t*                             delegatee  = NULL;

      result = cardano_certificate_to_stake_vote_registration_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_stake_vote_registration_delegation_cert_get_credential(cert);
        pool       = cardano_stake_vote_registration_delegation_cert_get_pool_key_hash(cert);
        drep       = cardano_stake_vote_registration_delegation_cert_get_drep(cert);
        result     = hash_bytes(pool, &pool_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_drep(drep, &drep_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_2, pool_pd, drep_pd, &delegatee);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cert_v3_reg_delegation(credential, delegatee, cardano_stake_vote_registration_delegation_cert_get_deposit(cert), out);
      }

      cardano_stake_vote_registration_delegation_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_blake2b_hash_unref(&pool);
      cardano_drep_unref(&drep);
      cardano_plutus_data_unref(&pool_pd);
      cardano_plutus_data_unref(&drep_pd);
      cardano_plutus_data_unref(&delegatee);

      break;
    }
    case CARDANO_CERT_TYPE_DREP_REGISTRATION:
    {
      cardano_register_drep_cert_t* cert       = NULL;
      cardano_credential_t*         credential = NULL;
      cardano_plutus_data_t*        cred_pd    = NULL;
      cardano_plutus_data_t*        deposit_pd = NULL;

      result = cardano_certificate_to_register_drep(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_register_drep_cert_get_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_integer_from_uint(cardano_register_drep_cert_get_deposit(cert), &deposit_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_4, cred_pd, deposit_pd, out);
      }

      cardano_register_drep_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&deposit_pd);

      break;
    }
    case CARDANO_CERT_TYPE_UPDATE_DREP:
    {
      cardano_update_drep_cert_t* cert       = NULL;
      cardano_credential_t*       credential = NULL;
      cardano_plutus_data_t*      cred_pd    = NULL;

      result = cardano_certificate_to_update_drep(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_update_drep_cert_get_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_5, cred_pd, out);
      }

      cardano_update_drep_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);

      break;
    }
    case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
    {
      cardano_unregister_drep_cert_t* cert       = NULL;
      cardano_credential_t*           credential = NULL;
      cardano_plutus_data_t*          cred_pd    = NULL;
      cardano_plutus_data_t*          deposit_pd = NULL;

      result = cardano_certificate_to_unregister_drep(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        credential = cardano_unregister_drep_cert_get_credential(cert);
        result     = encode_credential(credential, &cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_integer_from_uint(cardano_unregister_drep_cert_get_deposit(cert), &deposit_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_6, cred_pd, deposit_pd, out);
      }

      cardano_unregister_drep_cert_unref(&cert);
      cardano_credential_unref(&credential);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&deposit_pd);

      break;
    }
    case CARDANO_CERT_TYPE_POOL_REGISTRATION:
    {
      cardano_pool_registration_cert_t* cert          = NULL;
      cardano_pool_params_t*            params        = NULL;
      cardano_blake2b_hash_t*           operator_hash = NULL;
      cardano_blake2b_hash_t*           vrf           = NULL;
      cardano_plutus_data_t*            op_pd         = NULL;
      cardano_plutus_data_t*            vrf_pd        = NULL;

      result = cardano_certificate_to_pool_registration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_pool_registration_cert_get_params(cert, &params);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_pool_params_get_operator_key_hash(params, &operator_hash);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_pool_params_get_vrf_vk_hash(params, &vrf);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = hash_bytes(operator_hash, &op_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = hash_bytes(vrf, &vrf_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(7U, op_pd, vrf_pd, out);
      }

      cardano_pool_registration_cert_unref(&cert);
      cardano_pool_params_unref(&params);
      cardano_blake2b_hash_unref(&operator_hash);
      cardano_blake2b_hash_unref(&vrf);
      cardano_plutus_data_unref(&op_pd);
      cardano_plutus_data_unref(&vrf_pd);

      break;
    }
    case CARDANO_CERT_TYPE_POOL_RETIREMENT:
    {
      cardano_pool_retirement_cert_t* cert     = NULL;
      cardano_blake2b_hash_t*         pool     = NULL;
      cardano_plutus_data_t*          pool_pd  = NULL;
      cardano_plutus_data_t*          epoch_pd = NULL;

      result = cardano_certificate_to_pool_retirement(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        pool   = cardano_pool_retirement_cert_get_pool_key_hash(cert);
        result = hash_bytes(pool, &pool_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_integer_from_uint(cardano_pool_retirement_cert_get_epoch(cert), &epoch_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(8U, pool_pd, epoch_pd, out);
      }

      cardano_pool_retirement_cert_unref(&cert);
      cardano_blake2b_hash_unref(&pool);
      cardano_plutus_data_unref(&pool_pd);
      cardano_plutus_data_unref(&epoch_pd);

      break;
    }
    case CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT:
    {
      cardano_auth_committee_hot_cert_t* cert    = NULL;
      cardano_credential_t*              cold    = NULL;
      cardano_credential_t*              hot     = NULL;
      cardano_plutus_data_t*             cold_pd = NULL;
      cardano_plutus_data_t*             hot_pd  = NULL;

      result = cardano_certificate_to_auth_committee_hot(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_auth_committee_hot_cert_get_cold_cred(cert, &cold);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_auth_committee_hot_cert_get_hot_cred(cert, &hot);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_credential(cold, &cold_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_credential(hot, &hot_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(9U, cold_pd, hot_pd, out);
      }

      cardano_auth_committee_hot_cert_unref(&cert);
      cardano_credential_unref(&cold);
      cardano_credential_unref(&hot);
      cardano_plutus_data_unref(&cold_pd);
      cardano_plutus_data_unref(&hot_pd);

      break;
    }
    case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
    {
      cardano_resign_committee_cold_cert_t* cert    = NULL;
      cardano_credential_t*                 cold    = NULL;
      cardano_plutus_data_t*                cold_pd = NULL;

      result = cardano_certificate_to_resign_committee_cold(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        cold   = cardano_resign_committee_cold_cert_get_credential(cert);
        result = encode_credential(cold, &cold_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(10U, cold_pd, out);
      }

      cardano_resign_committee_cold_cert_unref(&cert);
      cardano_credential_unref(&cold);
      cardano_plutus_data_unref(&cold_pd);

      break;
    }
    case CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION:
    case CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS:
    default:
    {
      result = CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
      break;
    }
  }

  return result;
}

/**
 * \brief Encodes the certificates as the V3 list of Conway certificates.
 */
static cardano_error_t
certificates_v3(cardano_certificate_set_t* certificates, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (certificates != NULL) ? cardano_certificate_set_get_length(certificates) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_certificate_t* certificate = NULL;
    cardano_plutus_data_t* pd          = NULL;

    result = cardano_certificate_set_get(certificates, i, &certificate);

    if (result == CARDANO_SUCCESS)
    {
      result = certificate_v3(certificate, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_certificate_unref(&certificate);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes a voter.
 *
 * Committee key/script => Constr 0 [stake credential], DRep key/script =>
 * Constr 1 [stake credential], stake pool => Constr 2 [hash].
 */
static cardano_error_t
encode_voter(cardano_voter_t* voter, cardano_plutus_data_t** out)
{
  cardano_voter_type_t type   = CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH;
  cardano_error_t      result = cardano_voter_get_type(voter, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_credential_t* credential = cardano_voter_get_credential(voter);

  switch (type)
  {
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH:
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH:
    {
      cardano_plutus_data_t* cred_pd = NULL;

      result = encode_credential(credential, &cred_pd);

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, out);
      }

      cardano_plutus_data_unref(&cred_pd);

      break;
    }
    case CARDANO_VOTER_TYPE_DREP_KEY_HASH:
    case CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH:
    {
      cardano_plutus_data_t* cred_pd = NULL;

      result = encode_credential(credential, &cred_pd);

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_1, cred_pd, out);
      }

      cardano_plutus_data_unref(&cred_pd);

      break;
    }
    case CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH:
    {
      cardano_plutus_data_t* hash_pd = NULL;

      result = encode_bytes(cardano_credential_get_hash_bytes(credential), cardano_credential_get_hash_bytes_size(credential), &hash_pd);

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_2, hash_pd, out);
      }

      cardano_plutus_data_unref(&hash_pd);

      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  cardano_credential_unref(&credential);

  return result;
}

/**
 * \brief Encodes a governance action id: Constr 0 [transaction id, action index].
 */
static cardano_error_t
gov_action_id(cardano_governance_action_id_t* id, cardano_plutus_data_t** out)
{
  cardano_blake2b_hash_t* tx_id  = cardano_governance_action_id_get_hash(id);
  uint64_t                index  = 0U;
  cardano_plutus_data_t*  id_pd  = NULL;
  cardano_plutus_data_t*  idx_pd = NULL;
  cardano_error_t         result = hash_bytes(tx_id, &id_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_governance_action_id_get_index(id, &index);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(index, &idx_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = constr_two(CONSTR_0, id_pd, idx_pd, out);
  }

  cardano_blake2b_hash_unref(&tx_id);
  cardano_plutus_data_unref(&id_pd);
  cardano_plutus_data_unref(&idx_pd);

  return result;
}

/**
 * \brief Encodes an optional governance action id: Some => Constr 0 [id], None => Constr 1.
 */
static cardano_error_t
maybe_gov_action_id(cardano_governance_action_id_t* id, cardano_plutus_data_t** out)
{
  if (id == NULL)
  {
    return empty_constr(CONSTR_1, out);
  }

  cardano_plutus_data_t* id_pd  = NULL;
  cardano_error_t        result = gov_action_id(id, &id_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(CONSTR_0, id_pd, out);
  }

  cardano_plutus_data_unref(&id_pd);

  return result;
}

/**
 * \brief Encodes a vote: No => Constr 0, Yes => Constr 1, Abstain => Constr 2.
 */
static cardano_error_t
encode_vote(const cardano_vote_t vote, cardano_plutus_data_t** out)
{
  switch (vote)
  {
    case CARDANO_VOTE_NO:
    {
      return empty_constr(CONSTR_0, out);
    }
    case CARDANO_VOTE_YES:
    {
      return empty_constr(CONSTR_1, out);
    }
    case CARDANO_VOTE_ABSTAIN:
    {
      return empty_constr(CONSTR_2, out);
    }
    default:
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }
}

/**
 * \brief Encodes the votes as a map of Voter -> map of GovActionId -> Vote.
 */
static cardano_error_t
encode_votes(cardano_voting_procedures_t* procedures, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* outer  = NULL;
  cardano_error_t       result = cardano_plutus_map_new(&outer);

  cardano_voter_list_t* voters = NULL;

  if ((result == CARDANO_SUCCESS) && (procedures != NULL))
  {
    result = cardano_voting_procedures_get_voters(procedures, &voters);
  }

  const size_t voter_count = (voters != NULL) ? cardano_voter_list_get_length(voters) : 0U;

  for (size_t i = 0U; (i < voter_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_voter_t*                     voter    = NULL;
    cardano_governance_action_id_list_t* ids      = NULL;
    cardano_plutus_data_t*               voter_pd = NULL;
    cardano_plutus_map_t*                inner    = NULL;
    cardano_plutus_data_t*               inner_pd = NULL;

    result = cardano_voter_list_get(voters, i, &voter);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_voter(voter, &voter_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_voting_procedures_get_governance_ids_by_voter(procedures, voter, &ids);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_new(&inner);
    }

    const size_t id_count = (ids != NULL) ? cardano_governance_action_id_list_get_length(ids) : 0U;

    for (size_t j = 0U; (j < id_count) && (result == CARDANO_SUCCESS); ++j)
    {
      cardano_governance_action_id_t* id        = NULL;
      cardano_voting_procedure_t*     procedure = NULL;
      cardano_plutus_data_t*          id_pd     = NULL;
      cardano_plutus_data_t*          vote_pd   = NULL;

      result = cardano_governance_action_id_list_get(ids, j, &id);

      if (result == CARDANO_SUCCESS)
      {
        result = gov_action_id(id, &id_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        procedure = cardano_voting_procedures_get(procedures, voter, id);
        result    = (procedure != NULL) ? encode_vote(cardano_voting_procedure_get_vote(procedure), &vote_pd) : CARDANO_ERROR_ELEMENT_NOT_FOUND;
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_map_insert(inner, id_pd, vote_pd);
      }

      cardano_governance_action_id_unref(&id);
      cardano_voting_procedure_unref(&procedure);
      cardano_plutus_data_unref(&id_pd);
      cardano_plutus_data_unref(&vote_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_map(inner, &inner_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_insert(outer, voter_pd, inner_pd);
    }

    cardano_voter_unref(&voter);
    cardano_governance_action_id_list_unref(&ids);
    cardano_plutus_data_unref(&voter_pd);
    cardano_plutus_map_unref(&inner);
    cardano_plutus_data_unref(&inner_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(outer, out);
  }

  cardano_voter_list_unref(&voters);
  cardano_plutus_map_unref(&outer);

  return result;
}

/**
 * \brief Encodes a unit interval as a RationalNumber: Constr 0 [numerator, denominator].
 *
 * NOTE: this does not reduce the fraction by its greatest common divisor;
 * protocol-parameter rationals are not exercised by the transactions this
 * builder currently targets.
 */
static cardano_error_t
encode_rational(cardano_unit_interval_t* interval, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* num_pd = NULL;
  cardano_plutus_data_t* den_pd = NULL;
  cardano_error_t        result = cardano_plutus_data_new_integer_from_uint(cardano_unit_interval_get_numerator(interval), &num_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(cardano_unit_interval_get_denominator(interval), &den_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = constr_two(CONSTR_0, num_pd, den_pd, out);
  }

  cardano_plutus_data_unref(&num_pd);
  cardano_plutus_data_unref(&den_pd);

  return result;
}

/**
 * \brief Encodes a treasury-withdrawals map (reward address -> coin).
 */
static cardano_error_t
treasury_withdrawals(cardano_withdrawal_map_t* withdrawals, cardano_plutus_data_t** out)
{
  return withdrawals_v3(withdrawals, out);
}

/**
 * \brief Encodes the governance action of a proposal procedure.
 *
 * The ParameterChange protocol-parameter update is encoded as its changed-parameters
 * map via \ref cardano_protocol_param_update_to_plutus_data. All governance actions
 * are encoded fully.
 */
static cardano_error_t
gov_action(cardano_proposal_procedure_t* proposal, cardano_plutus_data_t** out)
{
  cardano_governance_action_type_t type   = CARDANO_GOVERNANCE_ACTION_TYPE_INFO;
  cardano_error_t                  result = cardano_proposal_procedure_get_action_type(proposal, &type);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE:
    {
      cardano_parameter_change_action_t* action     = NULL;
      cardano_governance_action_id_t*    prev       = NULL;
      cardano_protocol_param_update_t*   update     = NULL;
      cardano_blake2b_hash_t*            guardrail  = NULL;
      cardano_plutus_data_t*             prev_pd    = NULL;
      cardano_plutus_data_t*             params_pd  = NULL;
      cardano_plutus_data_t*             guard_pd   = NULL;
      cardano_plutus_data_t*             guard_hash = NULL;

      result = cardano_proposal_procedure_to_parameter_change_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        prev   = cardano_parameter_change_action_get_governance_action_id(action);
        result = maybe_gov_action_id(prev, &prev_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        update = cardano_parameter_change_action_get_protocol_param_update(action);
        result = cardano_protocol_param_update_to_plutus_data(update, &params_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        guardrail = cardano_parameter_change_action_get_policy_hash(action);

        if (guardrail != NULL)
        {
          result = hash_bytes(guardrail, &guard_hash);
        }
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_maybe(guard_hash, &guard_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_three(CONSTR_0, prev_pd, params_pd, guard_pd, out);
      }

      cardano_parameter_change_action_unref(&action);
      cardano_governance_action_id_unref(&prev);
      cardano_protocol_param_update_unref(&update);
      cardano_blake2b_hash_unref(&guardrail);
      cardano_plutus_data_unref(&prev_pd);
      cardano_plutus_data_unref(&params_pd);
      cardano_plutus_data_unref(&guard_pd);
      cardano_plutus_data_unref(&guard_hash);

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION:
    {
      cardano_hard_fork_initiation_action_t* action  = NULL;
      cardano_governance_action_id_t*        prev    = NULL;
      cardano_protocol_version_t*            version = NULL;
      cardano_plutus_data_t*                 prev_pd = NULL;
      cardano_plutus_data_t*                 major   = NULL;
      cardano_plutus_data_t*                 minor   = NULL;
      cardano_plutus_data_t*                 ver_pd  = NULL;

      result = cardano_proposal_procedure_to_hard_fork_initiation_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        prev   = cardano_hard_fork_initiation_action_get_governance_action_id(action);
        result = maybe_gov_action_id(prev, &prev_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        version = cardano_hard_fork_initiation_action_get_protocol_version(action);
        result  = cardano_plutus_data_new_integer_from_uint(cardano_protocol_version_get_major(version), &major);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_integer_from_uint(cardano_protocol_version_get_minor(version), &minor);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_0, major, minor, &ver_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_1, prev_pd, ver_pd, out);
      }

      cardano_hard_fork_initiation_action_unref(&action);
      cardano_governance_action_id_unref(&prev);
      cardano_protocol_version_unref(&version);
      cardano_plutus_data_unref(&prev_pd);
      cardano_plutus_data_unref(&major);
      cardano_plutus_data_unref(&minor);
      cardano_plutus_data_unref(&ver_pd);

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS:
    {
      cardano_treasury_withdrawals_action_t* action      = NULL;
      cardano_withdrawal_map_t*              withdrawals = NULL;
      cardano_blake2b_hash_t*                guardrail   = NULL;
      cardano_plutus_data_t*                 with_pd     = NULL;
      cardano_plutus_data_t*                 guard_pd    = NULL;
      cardano_plutus_data_t*                 guard_hash  = NULL;

      result = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        withdrawals = cardano_treasury_withdrawals_action_get_withdrawals(action);
        result      = treasury_withdrawals(withdrawals, &with_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        guardrail = cardano_treasury_withdrawals_action_get_policy_hash(action);

        if (guardrail != NULL)
        {
          result = hash_bytes(guardrail, &guard_hash);
        }
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_maybe(guard_hash, &guard_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_2, with_pd, guard_pd, out);
      }

      cardano_treasury_withdrawals_action_unref(&action);
      cardano_withdrawal_map_unref(&withdrawals);
      cardano_blake2b_hash_unref(&guardrail);
      cardano_plutus_data_unref(&with_pd);
      cardano_plutus_data_unref(&guard_pd);
      cardano_plutus_data_unref(&guard_hash);

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE:
    {
      cardano_no_confidence_action_t* action  = NULL;
      cardano_governance_action_id_t* prev    = NULL;
      cardano_plutus_data_t*          prev_pd = NULL;

      result = cardano_proposal_procedure_to_no_confidence_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        prev   = cardano_no_confidence_action_get_governance_action_id(action);
        result = maybe_gov_action_id(prev, &prev_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_3, prev_pd, out);
      }

      cardano_no_confidence_action_unref(&action);
      cardano_governance_action_id_unref(&prev);
      cardano_plutus_data_unref(&prev_pd);

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE:
    {
      cardano_update_committee_action_t* action     = NULL;
      cardano_governance_action_id_t*    prev       = NULL;
      cardano_credential_set_t*          removed    = NULL;
      cardano_committee_members_map_t*   added      = NULL;
      cardano_unit_interval_t*           quorum     = NULL;
      cardano_plutus_list_t*             removed_l  = NULL;
      cardano_plutus_map_t*              added_m    = NULL;
      cardano_plutus_data_t*             prev_pd    = NULL;
      cardano_plutus_data_t*             removed_pd = NULL;
      cardano_plutus_data_t*             added_pd   = NULL;
      cardano_plutus_data_t*             quorum_pd  = NULL;

      result = cardano_proposal_procedure_to_update_committee_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        prev   = cardano_update_committee_action_get_governance_action_id(action);
        result = maybe_gov_action_id(prev, &prev_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        removed = cardano_update_committee_action_get_members_to_be_removed(action);
        result  = cardano_plutus_list_new(&removed_l);
      }

      const size_t removed_count = (removed != NULL) ? cardano_credential_set_get_length(removed) : 0U;

      for (size_t i = 0U; (i < removed_count) && (result == CARDANO_SUCCESS); ++i)
      {
        cardano_credential_t*  cred    = NULL;
        cardano_plutus_data_t* cred_pd = NULL;

        result = cardano_credential_set_get(removed, i, &cred);

        if (result == CARDANO_SUCCESS)
        {
          result = encode_credential(cred, &cred_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(removed_l, cred_pd);
        }

        cardano_credential_unref(&cred);
        cardano_plutus_data_unref(&cred_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_list(removed_l, &removed_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        added  = cardano_update_committee_action_get_members_to_be_added(action);
        result = cardano_plutus_map_new(&added_m);
      }

      const size_t added_count = (added != NULL) ? cardano_committee_members_map_get_length(added) : 0U;

      for (size_t i = 0U; (i < added_count) && (result == CARDANO_SUCCESS); ++i)
      {
        cardano_credential_t*  cred     = NULL;
        uint64_t               epoch    = 0U;
        cardano_plutus_data_t* cred_pd  = NULL;
        cardano_plutus_data_t* epoch_pd = NULL;

        result = cardano_committee_members_map_get_key_value_at(added, i, &cred, &epoch);

        if (result == CARDANO_SUCCESS)
        {
          result = encode_credential(cred, &cred_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_data_new_integer_from_uint(epoch, &epoch_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_map_insert(added_m, cred_pd, epoch_pd);
        }

        cardano_credential_unref(&cred);
        cardano_plutus_data_unref(&cred_pd);
        cardano_plutus_data_unref(&epoch_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_data_new_map(added_m, &added_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        quorum = cardano_update_committee_action_get_quorum(action);
        result = encode_rational(quorum, &quorum_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_plutus_list_t* fields = NULL;

        result = cardano_plutus_list_new(&fields);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(fields, prev_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(fields, removed_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(fields, added_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(fields, quorum_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_constr(CONSTR_4, fields, out);
        }

        cardano_plutus_list_unref(&fields);
      }

      cardano_update_committee_action_unref(&action);
      cardano_governance_action_id_unref(&prev);
      cardano_credential_set_unref(&removed);
      cardano_committee_members_map_unref(&added);
      cardano_unit_interval_unref(&quorum);
      cardano_plutus_list_unref(&removed_l);
      cardano_plutus_map_unref(&added_m);
      cardano_plutus_data_unref(&prev_pd);
      cardano_plutus_data_unref(&removed_pd);
      cardano_plutus_data_unref(&added_pd);
      cardano_plutus_data_unref(&quorum_pd);

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION:
    {
      cardano_new_constitution_action_t* action       = NULL;
      cardano_governance_action_id_t*    prev         = NULL;
      cardano_constitution_t*            constitution = NULL;
      cardano_blake2b_hash_t*            script_hash  = NULL;
      cardano_plutus_data_t*             prev_pd      = NULL;
      cardano_plutus_data_t*             guard_pd     = NULL;
      cardano_plutus_data_t*             guard_hash   = NULL;
      cardano_plutus_data_t*             constr_pd    = NULL;

      result = cardano_proposal_procedure_to_constitution_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        prev   = cardano_new_constitution_action_get_governance_action_id(action);
        result = maybe_gov_action_id(prev, &prev_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        constitution = cardano_new_constitution_action_get_constitution(action);
        script_hash  = cardano_constitution_get_script_hash(constitution);

        if (script_hash != NULL)
        {
          result = hash_bytes(script_hash, &guard_hash);
        }
      }

      if (result == CARDANO_SUCCESS)
      {
        result = encode_maybe(guard_hash, &guard_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, guard_pd, &constr_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = constr_two(CONSTR_5, prev_pd, constr_pd, out);
      }

      cardano_new_constitution_action_unref(&action);
      cardano_governance_action_id_unref(&prev);
      cardano_constitution_unref(&constitution);
      cardano_blake2b_hash_unref(&script_hash);
      cardano_plutus_data_unref(&prev_pd);
      cardano_plutus_data_unref(&guard_pd);
      cardano_plutus_data_unref(&guard_hash);
      cardano_plutus_data_unref(&constr_pd);

      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_INFO:
    {
      result = empty_constr(CONSTR_6, out);
      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/**
 * \brief Encodes a proposal procedure: Constr 0 [deposit, return credential, gov action].
 *
 * The V3 \c ppReturnAddr field has type \c Credential, so the return address is
 * encoded as a bare Credential, not as a full Address.
 */
static cardano_error_t
encode_proposal_procedure(cardano_proposal_procedure_t* proposal, cardano_plutus_data_t** out)
{
  cardano_reward_address_t* reward     = cardano_proposal_procedure_get_reward_address(proposal);
  cardano_credential_t*     credential = NULL;
  cardano_plutus_data_t*    deposit_pd = NULL;
  cardano_plutus_data_t*    addr_pd    = NULL;
  cardano_plutus_data_t*    action_pd  = NULL;
  cardano_error_t           result     = cardano_plutus_data_new_integer_from_uint(cardano_proposal_procedure_get_deposit(proposal), &deposit_pd);

  if (result == CARDANO_SUCCESS)
  {
    credential = cardano_reward_address_get_credential(reward);
    result     = (credential != NULL) ? encode_credential(credential, &addr_pd) : CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = gov_action(proposal, &action_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = constr_three(CONSTR_0, deposit_pd, addr_pd, action_pd, out);
  }

  cardano_reward_address_unref(&reward);
  cardano_credential_unref(&credential);
  cardano_plutus_data_unref(&deposit_pd);
  cardano_plutus_data_unref(&addr_pd);
  cardano_plutus_data_unref(&action_pd);

  return result;
}

/**
 * \brief Encodes the proposal procedures as the list of proposal procedures.
 */
static cardano_error_t
encode_proposal_procedures(cardano_proposal_procedure_set_t* proposals, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  const size_t count = (proposals != NULL) ? cardano_proposal_procedure_set_get_length(proposals) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_proposal_procedure_t* proposal = NULL;
    cardano_plutus_data_t*        pd       = NULL;

    result = cardano_proposal_procedure_set_get(proposals, i, &proposal);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_proposal_procedure(proposal, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_proposal_procedure_unref(&proposal);
    cardano_plutus_data_unref(&pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Encodes an optional lovelace amount as a Maybe Coin.
 */
static cardano_error_t
maybe_coin(const uint64_t* coin, cardano_plutus_data_t** out)
{
  if (coin == NULL)
  {
    return empty_constr(CONSTR_1, out);
  }

  cardano_plutus_data_t* coin_pd = NULL;
  cardano_error_t        result  = cardano_plutus_data_new_integer_from_uint(*coin, &coin_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(CONSTR_0, coin_pd, out);
  }

  cardano_plutus_data_unref(&coin_pd);

  return result;
}

/**
 * \brief Builds the V3 ScriptInfo or ScriptPurpose of a redeemer.
 *
 * When \p with_datum is true the Spending variant carries the optional datum
 * (the ScriptInfo shape used by the ScriptContext); when false the Spending
 * variant is the bare out ref (the ScriptPurpose shape used as a redeemer-map
 * key). All other variants are identical between the two shapes.
 */
static cardano_error_t
script_info_v3(
  cardano_transaction_t*     tx,
  const cardano_utxo_list_t* resolved_inputs,
  cardano_redeemer_t*        redeemer,
  cardano_plutus_data_t*     datum,
  bool                       with_datum,
  cardano_plutus_data_t**    script_info);

/**
 * \brief Encodes the redeemers of a V3 transaction as a map of ScriptPurpose -> redeemer data.
 *
 * The purpose key uses the Conway WithNeverRegistrationDeposit ScriptPurpose
 * shape, where the Spending variant is the bare out ref without a datum.
 */
static cardano_error_t
redeemers_map_v3(
  cardano_transaction_t*   tx,
  cardano_utxo_list_t*     resolved_inputs,
  cardano_redeemer_list_t* redeemers,
  cardano_plutus_data_t**  out)
{
  cardano_plutus_map_t* map    = NULL;
  cardano_error_t       result = cardano_plutus_map_new(&map);

  const size_t count = (redeemers != NULL) ? cardano_redeemer_list_get_length(redeemers) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_redeemer_t*    redeemer   = NULL;
    cardano_plutus_data_t* purpose_pd = NULL;
    cardano_plutus_data_t* data       = NULL;

    result = cardano_redeemer_list_get(redeemers, i, &redeemer);

    if (result == CARDANO_SUCCESS)
    {
      result = script_info_v3(tx, resolved_inputs, redeemer, NULL, false, &purpose_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      data   = cardano_redeemer_get_data(redeemer);
      result = (data != NULL) ? cardano_plutus_map_insert(map, purpose_pd, data) : CARDANO_ERROR_POINTER_IS_NULL;
    }

    cardano_redeemer_unref(&redeemer);
    cardano_plutus_data_unref(&purpose_pd);
    cardano_plutus_data_unref(&data);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(map, out);
  }

  cardano_plutus_map_unref(&map);

  return result;
}

/* PUBLIC FUNCTIONS **********************************************************/

cardano_error_t
cardano_uplc_int_build_tx_info_v1(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t**      tx_info)
{
  return build_tx_info(tx, resolved_inputs, slot_config, false, tx_info);
}

cardano_error_t
cardano_uplc_int_build_tx_info_v2(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t**      tx_info)
{
  return build_tx_info(tx, resolved_inputs, slot_config, true, tx_info);
}

cardano_error_t
cardano_uplc_int_build_script_purpose_v1v2(
  cardano_transaction_t*     tx,
  const cardano_utxo_list_t* resolved_inputs,
  cardano_redeemer_t*        redeemer,
  cardano_plutus_data_t**    purpose)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (redeemer == NULL) || (purpose == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t*  body   = cardano_transaction_get_body(tx);
  const cardano_redeemer_tag_t tag    = cardano_redeemer_get_tag(redeemer);
  const uint64_t               index  = cardano_redeemer_get_index(redeemer);
  cardano_error_t              result = CARDANO_SUCCESS;

  switch (tag)
  {
    case CARDANO_REDEEMER_TAG_MINT:
    {
      cardano_multi_asset_t*    mint     = cardano_transaction_body_get_mint(body);
      cardano_policy_id_list_t* policies = NULL;

      result = (mint != NULL) ? cardano_multi_asset_get_keys(mint, &policies) : CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;

      if ((result == CARDANO_SUCCESS) && (index >= cardano_policy_id_list_get_length(policies)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_t* policy_id = NULL;
        cardano_plutus_data_t*  pd        = NULL;

        result = cardano_policy_id_list_get(policies, (size_t)index, &policy_id);

        if (result == CARDANO_SUCCESS)
        {
          result = hash_bytes(policy_id, &pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_0, pd, purpose);
        }

        cardano_blake2b_hash_unref(&policy_id);
        cardano_plutus_data_unref(&pd);
      }

      cardano_multi_asset_unref(&mint);
      cardano_policy_id_list_unref(&policies);

      break;
    }
    case CARDANO_REDEEMER_TAG_SPEND:
    {
      cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);

      if ((inputs == NULL) || (index >= cardano_transaction_input_set_get_length(inputs)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_transaction_input_t* input  = NULL;
        cardano_plutus_data_t*       ref_pd = NULL;

        result = cardano_transaction_input_set_get(inputs, (size_t)index, &input);

        if (result == CARDANO_SUCCESS)
        {
          result = out_ref_wrapped(input, &ref_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_1, ref_pd, purpose);
        }

        cardano_transaction_input_unref(&input);
        cardano_plutus_data_unref(&ref_pd);
      }

      cardano_transaction_input_set_unref(&inputs);

      break;
    }
    case CARDANO_REDEEMER_TAG_REWARD:
    {
      cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);

      if ((withdrawals == NULL) || (index >= cardano_withdrawal_map_get_length(withdrawals)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_reward_address_t* address    = NULL;
        uint64_t                  coin       = 0U;
        cardano_credential_t*     credential = NULL;
        cardano_plutus_data_t*    cred_pd    = NULL;
        cardano_plutus_data_t*    wrapped    = NULL;

        result = cardano_withdrawal_map_get_key_value_at(withdrawals, (size_t)index, &address, &coin);

        if (result == CARDANO_SUCCESS)
        {
          credential = cardano_reward_address_get_credential(address);
          result     = encode_credential(credential, &cred_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_0, cred_pd, &wrapped);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_2, wrapped, purpose);
        }

        cardano_reward_address_unref(&address);
        cardano_credential_unref(&credential);
        cardano_plutus_data_unref(&cred_pd);
        cardano_plutus_data_unref(&wrapped);
      }

      cardano_withdrawal_map_unref(&withdrawals);

      break;
    }
    case CARDANO_REDEEMER_TAG_CERTIFYING:
    {
      cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);

      if ((certificates == NULL) || (index >= cardano_certificate_set_get_length(certificates)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_certificate_t* certificate = NULL;
        cardano_plutus_data_t* cert_pd     = NULL;

        result = cardano_certificate_set_get(certificates, (size_t)index, &certificate);

        if (result == CARDANO_SUCCESS)
        {
          result = encode_certificate(certificate, &cert_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_3, cert_pd, purpose);
        }

        cardano_certificate_unref(&certificate);
        cardano_plutus_data_unref(&cert_pd);
      }

      cardano_certificate_set_unref(&certificates);

      break;
    }
    case CARDANO_REDEEMER_TAG_VOTING:
    case CARDANO_REDEEMER_TAG_PROPOSING:
    case CARDANO_REDEEMER_TAG_GUARDING:
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  cardano_transaction_body_unref(&body);

  return result;
}

/**
 * \brief Wraps a TxInfo and the redeemer purpose into the V1/V2 ScriptContext.
 */
static cardano_error_t
build_script_context(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  const bool                   is_v2,
  cardano_plutus_data_t**      script_context)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (slot_config == NULL) || (redeemer == NULL) || (script_context == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_list_t* fields  = NULL;
  cardano_plutus_data_t* tx_info = NULL;
  cardano_plutus_data_t* purpose = NULL;
  cardano_error_t        result  = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = build_tx_info(tx, resolved_inputs, slot_config, is_v2, &tx_info);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, tx_info);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_int_build_script_purpose_v1v2(tx, resolved_inputs, redeemer, &purpose);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, purpose);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, script_context);
  }

  cardano_plutus_data_unref(&tx_info);
  cardano_plutus_data_unref(&purpose);
  cardano_plutus_list_unref(&fields);

  return result;
}

cardano_error_t
cardano_uplc_int_build_script_context_v1(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t**      script_context)
{
  return build_script_context(tx, resolved_inputs, slot_config, redeemer, false, script_context);
}

cardano_error_t
cardano_uplc_int_build_script_context_v2(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t**      script_context)
{
  return build_script_context(tx, resolved_inputs, slot_config, redeemer, true, script_context);
}

cardano_error_t
cardano_uplc_int_build_tx_info_v3(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t**      tx_info)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (slot_config == NULL) || (tx_info == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body        = cardano_transaction_get_body(tx);
  cardano_witness_set_t*      witness_set = cardano_transaction_get_witness_set(tx);
  cardano_plutus_list_t*      fields      = NULL;
  cardano_error_t             result      = cardano_plutus_list_new(&fields);

  cardano_transaction_input_set_t*   inputs     = NULL;
  cardano_transaction_input_set_t*   ref_inputs = NULL;
  cardano_transaction_output_list_t* outputs    = NULL;
  cardano_multi_asset_t*             mint       = NULL;
  cardano_certificate_set_t*         certs      = NULL;
  cardano_withdrawal_map_t*          withdraws  = NULL;
  cardano_blake2b_hash_set_t*        signers    = NULL;
  cardano_plutus_data_set_t*         datums     = NULL;
  cardano_redeemer_list_t*           redeemers  = NULL;
  cardano_voting_procedures_t*       votes      = NULL;
  cardano_proposal_procedure_set_t*  proposals  = NULL;
  cardano_blake2b_hash_t*            tx_id      = NULL;

  cardano_plutus_data_t* inputs_pd   = NULL;
  cardano_plutus_data_t* refs_pd     = NULL;
  cardano_plutus_data_t* outputs_pd  = NULL;
  cardano_plutus_data_t* fee_pd      = NULL;
  cardano_plutus_data_t* mint_pd     = NULL;
  cardano_plutus_data_t* certs_pd    = NULL;
  cardano_plutus_data_t* with_pd     = NULL;
  cardano_plutus_data_t* range_pd    = NULL;
  cardano_plutus_data_t* signs_pd    = NULL;
  cardano_plutus_data_t* redeem_pd   = NULL;
  cardano_plutus_data_t* data_pd     = NULL;
  cardano_plutus_data_t* id_pd       = NULL;
  cardano_plutus_data_t* votes_pd    = NULL;
  cardano_plutus_data_t* props_pd    = NULL;
  cardano_plutus_data_t* treasury_pd = NULL;
  cardano_plutus_data_t* donation_pd = NULL;

  if (result == CARDANO_SUCCESS)
  {
    inputs = cardano_transaction_body_get_inputs(body);
    result = inputs_v3(inputs, resolved_inputs, &inputs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, inputs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    ref_inputs = cardano_transaction_body_get_reference_inputs(body);
    result     = inputs_v3(ref_inputs, resolved_inputs, &refs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, refs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    outputs = cardano_transaction_body_get_outputs(body);
    result  = outputs_v3(outputs, &outputs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, outputs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(cardano_transaction_body_get_fee(body), &fee_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, fee_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    mint   = cardano_transaction_body_get_mint(body);
    result = mint_plain(mint, &mint_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, mint_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    certs  = cardano_transaction_body_get_certificates(body);
    result = certificates_v3(certs, &certs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, certs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    withdraws = cardano_transaction_body_get_withdrawals(body);
    result    = withdrawals_v3(withdraws, &with_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, with_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = valid_range(body, slot_config, &range_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, range_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    signers = cardano_transaction_body_get_required_signers(body);
    result  = encode_signatories(signers, &signs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, signs_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    redeemers = cardano_witness_set_get_redeemers(witness_set);
    result    = redeemers_map_v3(tx, resolved_inputs, redeemers, &redeem_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, redeem_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    datums = cardano_witness_set_get_plutus_data(witness_set);
    result = encode_data(datums, &data_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, data_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    tx_id  = cardano_transaction_get_id(tx);
    result = (tx_id != NULL) ? hash_bytes(tx_id, &id_pd) : CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, id_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    votes  = cardano_transaction_body_get_voting_procedures(body);
    result = encode_votes(votes, &votes_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, votes_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    proposals = cardano_transaction_body_get_proposal_procedures(body);
    result    = encode_proposal_procedures(proposals, &props_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, props_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = maybe_coin(cardano_transaction_body_get_treasury_value(body), &treasury_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, treasury_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = maybe_coin(cardano_transaction_body_get_donation(body), &donation_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, donation_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, tx_info);
  }

  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_set_unref(&ref_inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_multi_asset_unref(&mint);
  cardano_certificate_set_unref(&certs);
  cardano_withdrawal_map_unref(&withdraws);
  cardano_blake2b_hash_set_unref(&signers);
  cardano_plutus_data_set_unref(&datums);
  cardano_redeemer_list_unref(&redeemers);
  cardano_voting_procedures_unref(&votes);
  cardano_proposal_procedure_set_unref(&proposals);
  cardano_blake2b_hash_unref(&tx_id);

  cardano_plutus_data_unref(&inputs_pd);
  cardano_plutus_data_unref(&refs_pd);
  cardano_plutus_data_unref(&outputs_pd);
  cardano_plutus_data_unref(&fee_pd);
  cardano_plutus_data_unref(&mint_pd);
  cardano_plutus_data_unref(&certs_pd);
  cardano_plutus_data_unref(&with_pd);
  cardano_plutus_data_unref(&range_pd);
  cardano_plutus_data_unref(&signs_pd);
  cardano_plutus_data_unref(&redeem_pd);
  cardano_plutus_data_unref(&data_pd);
  cardano_plutus_data_unref(&id_pd);
  cardano_plutus_data_unref(&votes_pd);
  cardano_plutus_data_unref(&props_pd);
  cardano_plutus_data_unref(&treasury_pd);
  cardano_plutus_data_unref(&donation_pd);
  cardano_plutus_list_unref(&fields);

  return result;
}

static cardano_error_t
script_info_v3(
  cardano_transaction_t*     tx,
  const cardano_utxo_list_t* resolved_inputs,
  cardano_redeemer_t*        redeemer,
  cardano_plutus_data_t*     datum,
  const bool                 with_datum,
  cardano_plutus_data_t**    script_info)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (redeemer == NULL) || (script_info == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t*  body   = cardano_transaction_get_body(tx);
  const cardano_redeemer_tag_t tag    = cardano_redeemer_get_tag(redeemer);
  const uint64_t               index  = cardano_redeemer_get_index(redeemer);
  cardano_error_t              result = CARDANO_SUCCESS;

  switch (tag)
  {
    case CARDANO_REDEEMER_TAG_MINT:
    {
      cardano_multi_asset_t*    mint     = cardano_transaction_body_get_mint(body);
      cardano_policy_id_list_t* policies = NULL;

      result = (mint != NULL) ? cardano_multi_asset_get_keys(mint, &policies) : CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;

      if ((result == CARDANO_SUCCESS) && (index >= cardano_policy_id_list_get_length(policies)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_t* policy_id = NULL;
        cardano_plutus_data_t*  pd        = NULL;

        result = cardano_policy_id_list_get(policies, (size_t)index, &policy_id);

        if (result == CARDANO_SUCCESS)
        {
          result = hash_bytes(policy_id, &pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_0, pd, script_info);
        }

        cardano_blake2b_hash_unref(&policy_id);
        cardano_plutus_data_unref(&pd);
      }

      cardano_multi_asset_unref(&mint);
      cardano_policy_id_list_unref(&policies);

      break;
    }
    case CARDANO_REDEEMER_TAG_SPEND:
    {
      cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);

      if ((inputs == NULL) || (index >= cardano_transaction_input_set_get_length(inputs)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_transaction_input_t* input    = NULL;
        cardano_plutus_data_t*       ref_pd   = NULL;
        cardano_plutus_data_t*       datum_pd = NULL;
        cardano_plutus_list_t*       sfields  = NULL;

        result = cardano_transaction_input_set_get(inputs, (size_t)index, &input);

        if (result == CARDANO_SUCCESS)
        {
          result = out_ref_v3(input, &ref_pd);
        }

        if (!with_datum)
        {
          if (result == CARDANO_SUCCESS)
          {
            result = wrap_constr(CONSTR_1, ref_pd, script_info);
          }
        }
        else
        {
          if (result == CARDANO_SUCCESS)
          {
            result = encode_maybe(datum, &datum_pd);
          }

          if (result == CARDANO_SUCCESS)
          {
            result = cardano_plutus_list_new(&sfields);
          }

          if (result == CARDANO_SUCCESS)
          {
            result = cardano_plutus_list_add(sfields, ref_pd);
          }

          if (result == CARDANO_SUCCESS)
          {
            result = cardano_plutus_list_add(sfields, datum_pd);
          }

          if (result == CARDANO_SUCCESS)
          {
            result = encode_constr(CONSTR_1, sfields, script_info);
          }
        }

        cardano_transaction_input_unref(&input);
        cardano_plutus_data_unref(&ref_pd);
        cardano_plutus_data_unref(&datum_pd);
        cardano_plutus_list_unref(&sfields);
      }

      cardano_transaction_input_set_unref(&inputs);

      break;
    }
    case CARDANO_REDEEMER_TAG_REWARD:
    {
      cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);

      if ((withdrawals == NULL) || (index >= cardano_withdrawal_map_get_length(withdrawals)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_reward_address_t* address    = NULL;
        uint64_t                  coin       = 0U;
        cardano_credential_t*     credential = NULL;
        cardano_plutus_data_t*    cred_pd    = NULL;

        result = cardano_withdrawal_map_get_key_value_at(withdrawals, (size_t)index, &address, &coin);

        if (result == CARDANO_SUCCESS)
        {
          credential = cardano_reward_address_get_credential(address);
          result     = encode_credential(credential, &cred_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_2, cred_pd, script_info);
        }

        cardano_reward_address_unref(&address);
        cardano_credential_unref(&credential);
        cardano_plutus_data_unref(&cred_pd);
      }

      cardano_withdrawal_map_unref(&withdrawals);

      break;
    }
    case CARDANO_REDEEMER_TAG_CERTIFYING:
    {
      cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(body);

      if ((certificates == NULL) || (index >= cardano_certificate_set_get_length(certificates)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_certificate_t* certificate = NULL;
        cardano_plutus_data_t* idx_pd      = NULL;
        cardano_plutus_data_t* cert_pd     = NULL;

        result = cardano_certificate_set_get(certificates, (size_t)index, &certificate);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_data_new_integer_from_uint(index, &idx_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = certificate_v3(certificate, &cert_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = constr_two(CONSTR_3, idx_pd, cert_pd, script_info);
        }

        cardano_certificate_unref(&certificate);
        cardano_plutus_data_unref(&idx_pd);
        cardano_plutus_data_unref(&cert_pd);
      }

      cardano_certificate_set_unref(&certificates);

      break;
    }
    case CARDANO_REDEEMER_TAG_VOTING:
    {
      cardano_voting_procedures_t* procedures = cardano_transaction_body_get_voting_procedures(body);
      cardano_voter_list_t*        voters     = NULL;

      result = (procedures != NULL) ? cardano_voting_procedures_get_voters(procedures, &voters) : CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;

      if ((result == CARDANO_SUCCESS) && (index >= cardano_voter_list_get_length(voters)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_voter_t*       voter    = NULL;
        cardano_plutus_data_t* voter_pd = NULL;

        result = cardano_voter_list_get(voters, (size_t)index, &voter);

        if (result == CARDANO_SUCCESS)
        {
          result = encode_voter(voter, &voter_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = wrap_constr(CONSTR_4, voter_pd, script_info);
        }

        cardano_voter_unref(&voter);
        cardano_plutus_data_unref(&voter_pd);
      }

      cardano_voting_procedures_unref(&procedures);
      cardano_voter_list_unref(&voters);

      break;
    }
    case CARDANO_REDEEMER_TAG_PROPOSING:
    {
      cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);

      if ((proposals == NULL) || (index >= cardano_proposal_procedure_set_get_length(proposals)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_proposal_procedure_t* proposal = NULL;
        cardano_plutus_data_t*        idx_pd   = NULL;
        cardano_plutus_data_t*        prop_pd  = NULL;

        result = cardano_proposal_procedure_set_get(proposals, (size_t)index, &proposal);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_data_new_integer_from_uint(index, &idx_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_proposal_procedure(proposal, &prop_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = constr_two(CONSTR_5, idx_pd, prop_pd, script_info);
        }

        cardano_proposal_procedure_unref(&proposal);
        cardano_plutus_data_unref(&idx_pd);
        cardano_plutus_data_unref(&prop_pd);
      }

      cardano_proposal_procedure_set_unref(&proposals);

      break;
    }
    case CARDANO_REDEEMER_TAG_GUARDING:
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  cardano_transaction_body_unref(&body);

  return result;
}

cardano_error_t
cardano_uplc_int_build_script_info_v3(
  cardano_transaction_t*  tx,
  cardano_utxo_list_t*    resolved_inputs,
  cardano_redeemer_t*     redeemer,
  cardano_plutus_data_t*  datum,
  cardano_plutus_data_t** script_info)
{
  return script_info_v3(tx, resolved_inputs, redeemer, datum, true, script_info);
}

cardano_error_t
cardano_uplc_int_build_script_context_v3(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t*       datum,
  cardano_plutus_data_t**      script_context)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (slot_config == NULL) || (redeemer == NULL) || (script_context == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_list_t* fields      = NULL;
  cardano_plutus_data_t* tx_info     = NULL;
  cardano_plutus_data_t* redeemer_pd = NULL;
  cardano_plutus_data_t* info        = NULL;
  cardano_error_t        result      = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_int_build_tx_info_v3(tx, resolved_inputs, slot_config, &tx_info);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, tx_info);
  }

  if (result == CARDANO_SUCCESS)
  {
    redeemer_pd = cardano_redeemer_get_data(redeemer);
    result      = (redeemer_pd != NULL) ? cardano_plutus_list_add(fields, redeemer_pd) : CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_int_build_script_info_v3(tx, resolved_inputs, redeemer, datum, &info);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, info);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(CONSTR_0, fields, script_context);
  }

  cardano_plutus_data_unref(&tx_info);
  cardano_plutus_data_unref(&redeemer_pd);
  cardano_plutus_data_unref(&info);
  cardano_plutus_list_unref(&fields);

  return result;
}
