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

#include "../../allocators.h"
#include "../../common/ledger_order.h"

#include <cardano/address/address.h>
#include <cardano/address/address_type.h>
#include <cardano/address/base_address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/address/reward_address.h>
#include <cardano/address/stake_pointer.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/asset_name_map.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/assets/policy_id_list.h>
#include <cardano/buffer.h>
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
#include <cardano/common/bigint.h>
#include <cardano/common/credential.h>
#include <cardano/common/datum.h>
#include <cardano/common/drep.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/network_id.h>
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
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/account_balance_interval.h>
#include <cardano/transaction_body/account_balance_intervals_map.h>
#include <cardano/transaction_body/direct_deposit_map.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
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

#include <src/string_safe.h>

#include <string.h>

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

/**
 * \brief Capacity of a ledger sort key, in bytes.
 *
 * Wide enough for the longest key the encoders build: a voter followed by a
 * governance action id, or a policy id followed by an asset name.
 */
enum
{
  SORT_KEY_SIZE = 128
};

/**
 * \brief Number of fields of the V4 TxInfo record.
 */
enum
{
  TX_INFO_V4_FIELD_COUNT = 19
};

/**
 * \brief Number of fields of the V4 TopTxInfoSimplified record.
 */
enum
{
  TOP_TX_INFO_SIMPLIFIED_FIELD_COUNT = 18
};

/**
 * \brief Number of fields of the V4 TopTxInfo record.
 */
enum
{
  TOP_TX_INFO_FIELD_COUNT = 4
};

/**
 * \brief Number of fields of the V4 ScriptContext record.
 */
enum
{
  SCRIPT_CONTEXT_V4_FIELD_COUNT = 4
};

/**
 * \brief Largest number of items a script purpose carries besides its script hash.
 */
enum
{
  PURPOSE_MAX_ITEMS = 2
};

/* STRUCTURES ****************************************************************/

/**
 * \brief A byte string that orders map keys the way the ledger orders them.
 *
 * Keys compare byte by byte and a key that is a prefix of another sorts first,
 * which is the ordering of the ledger's Haskell \c Ord instances once each
 * constructor is written as its rank followed by its fields.
 */
typedef struct sort_key_t
{
    byte_t bytes[SORT_KEY_SIZE];
    size_t size;
} sort_key_t;

/**
 * \brief How a sorted map combines a new entry with an existing one of the same key.
 */
typedef enum
{
  MERGE_KEEP_FIRST = 0,
  MERGE_KEEP_LAST  = 1,
  MERGE_SUM        = 2
} merge_mode_t;

/**
 * \brief Which amounts of a multi-asset a value table collects.
 */
typedef enum
{
  ASSETS_ALL      = 0,
  ASSETS_POSITIVE = 1,
  ASSETS_NEGATIVE = 2
} asset_filter_t;

/**
 * \brief An entry of a sorted map.
 *
 * \c key_data is the encoded map key, \c inner_key the key of a nested map when
 * the entry belongs to one, and the value is either \c value or, for summed
 * entries, \c amount.
 */
typedef struct sorted_entry_t
{
    sort_key_t             key;
    cardano_plutus_data_t* key_data;
    cardano_plutus_data_t* inner_key;
    cardano_plutus_data_t* value;
    cardano_bigint_t*      amount;
} sorted_entry_t;

/**
 * \brief A map kept in ledger key order while it is filled.
 */
typedef struct sorted_map_t
{
    sorted_entry_t* items;
    size_t          count;
    size_t          capacity;
} sorted_map_t;

/**
 * \brief The parts of a top-level transaction or a sub-transaction that a V4 TxInfo reads.
 *
 * Both transaction levels share this view so a single encoder serves them.
 * Every pointer is an owned reference released by \ref tx_view_release.
 */
typedef struct tx_view_t
{
    cardano_blake2b_hash_t*                  id;
    bool                                     is_sub_tx;
    uint64_t                                 sub_tx_index;
    cardano_transaction_input_set_t*         inputs;
    cardano_transaction_input_set_t*         reference_inputs;
    cardano_transaction_output_list_t*       outputs;
    cardano_multi_asset_t*                   mint;
    cardano_certificate_set_t*               certificates;
    cardano_withdrawal_map_t*                withdrawals;
    cardano_direct_deposit_map_t*            direct_deposits;
    cardano_account_balance_intervals_map_t* balance_intervals;
    bool                                     has_invalid_before;
    uint64_t                                 invalid_before;
    bool                                     has_invalid_after;
    uint64_t                                 invalid_after;
    cardano_guard_set_t*                     guards;
    cardano_required_guards_map_t*           required_guards;
    cardano_voting_procedures_t*             votes;
    cardano_proposal_procedure_set_t*        proposals;
    bool                                     has_treasury;
    uint64_t                                 treasury;
    uint64_t                                 donation;
    cardano_redeemer_list_t*                 redeemers;
    cardano_plutus_data_set_t*               datums;
} tx_view_t;

/**
 * \brief The aggregates of a whole transaction that make up a TopTxInfo.
 */
typedef struct batch_t
{
    cardano_plutus_list_t* sub_tx_infos;
    cardano_plutus_list_t* ids;
    cardano_plutus_list_t* inputs;
    cardano_plutus_list_t* reference_inputs;
    cardano_plutus_list_t* outputs;
    cardano_plutus_list_t* certificates;
    cardano_plutus_list_t* guards;
    cardano_plutus_list_t* proposals;
    sorted_map_t           guard_datums;
    sorted_map_t           mints;
    sorted_map_t           burns;
    sorted_map_t           withdrawals;
    sorted_map_t           direct_deposits;
    sorted_map_t           required_guards;
    sorted_map_t           redeemer_hashes;
    sorted_map_t           datums;
    sorted_map_t           votes;
    bool                   has_lower;
    uint64_t               lower;
    bool                   has_upper;
    uint64_t               upper;
    bool                   has_treasury;
    uint64_t               treasury;
    cardano_bigint_t*      donations;
} batch_t;

/**
 * \brief What a redeemer points to, resolved at its transaction level.
 *
 * \c items are the fields a ScriptPurpose and a ScriptInfo share, \c script_hash
 * the hash of the script that runs and \c guard the encoded guard credential of
 * a guarding redeemer. Every value is an owned reference.
 */
typedef struct purpose_parts_t
{
    uint64_t               alternative;
    cardano_plutus_data_t* items[PURPOSE_MAX_ITEMS];
    size_t                 item_count;
    cardano_plutus_data_t* script_hash;
    cardano_plutus_data_t* guard;
} purpose_parts_t;

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
 * \brief Appends bytes to a sort key, truncating at its capacity.
 */
static void
sort_key_append(sort_key_t* key, const byte_t* data, const size_t size)
{
  const size_t room = (size_t)SORT_KEY_SIZE - key->size;
  const size_t copy = (size < room) ? size : room;

  cardano_safe_memcpy(&key->bytes[key->size], room, data, copy);

  key->size += copy;
}

/**
 * \brief Appends a single rank byte to a sort key.
 */
static void
sort_key_append_byte(sort_key_t* key, const byte_t value)
{
  sort_key_append(key, &value, 1U);
}

/**
 * \brief Appends an unsigned integer to a sort key as 8 big-endian bytes.
 */
static void
sort_key_append_index(sort_key_t* key, const uint64_t index)
{
  static const size_t SORT_KEY_INDEX_SIZE    = 8U;
  static const size_t SORT_KEY_BITS_PER_BYTE = 8U;

  byte_t bytes[8] = { 0 };

  for (size_t i = 0U; i < SORT_KEY_INDEX_SIZE; ++i)
  {
    bytes[i] = (byte_t)(index >> (SORT_KEY_BITS_PER_BYTE * (SORT_KEY_INDEX_SIZE - 1U - i)));
  }

  sort_key_append(key, bytes, SORT_KEY_INDEX_SIZE);
}

/**
 * \brief Appends the bytes of a hash to a sort key.
 */
static void
sort_key_append_hash(sort_key_t* key, const cardano_blake2b_hash_t* hash)
{
  sort_key_append(key, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash));
}

/**
 * \brief Appends a credential to a sort key in the ledger order: script credentials sort before key credentials.
 */
static cardano_error_t
sort_key_append_credential(sort_key_t* key, const cardano_credential_t* credential)
{
  byte_t          bytes[CARDANO_LEDGER_ORDER_CREDENTIAL_KEY_SIZE] = { 0 };
  cardano_error_t result                                          = _cardano_ledger_order_credential_key(credential, bytes);

  if (result == CARDANO_SUCCESS)
  {
    sort_key_append(key, bytes, sizeof(bytes));
  }

  return result;
}

/**
 * \brief Appends a voter to a sort key in the ledger order.
 *
 * The ledger orders committee voters first, then DReps, then stake pools, each
 * by credential.
 */
static cardano_error_t
sort_key_append_voter(sort_key_t* key, cardano_voter_t* voter)
{
  byte_t          bytes[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };
  cardano_error_t result                               = _cardano_ledger_order_voter_key(voter, bytes);

  if (result == CARDANO_SUCCESS)
  {
    sort_key_append(key, bytes, sizeof(bytes));
  }

  return result;
}

/**
 * \brief Orders two sort keys byte by byte, a proper prefix first.
 */
static int32_t
compare_sort_keys(const sort_key_t* lhs, const sort_key_t* rhs)
{
  const size_t  common = (lhs->size < rhs->size) ? lhs->size : rhs->size;
  const int32_t order  = (int32_t)memcmp(lhs->bytes, rhs->bytes, common);

  if (order != 0)
  {
    return order;
  }

  if (lhs->size == rhs->size)
  {
    return 0;
  }

  return (lhs->size < rhs->size) ? -1 : 1;
}

/**
 * \brief Releases the entries of a sorted map and resets it to empty.
 */
static void
sorted_map_free(sorted_map_t* map)
{
  for (size_t i = 0U; i < map->count; ++i)
  {
    cardano_plutus_data_unref(&map->items[i].key_data);
    cardano_plutus_data_unref(&map->items[i].inner_key);
    cardano_plutus_data_unref(&map->items[i].value);
    cardano_bigint_unref(&map->items[i].amount);
  }

  _cardano_free(map->items);

  map->items    = NULL;
  map->count    = 0U;
  map->capacity = 0U;
}

/**
 * \brief Makes room for one more entry in a sorted map.
 */
static cardano_error_t
sorted_map_reserve(sorted_map_t* map)
{
  static const size_t SORTED_MAP_INITIAL_CAPACITY = 4U;

  if (map->count < map->capacity)
  {
    return CARDANO_SUCCESS;
  }

  const size_t    capacity = (map->capacity == 0U) ? SORTED_MAP_INITIAL_CAPACITY : (map->capacity * 2U);
  sorted_entry_t* items    = (sorted_entry_t*)_cardano_malloc(capacity * sizeof(sorted_entry_t));

  if (items == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if (map->items != NULL)
  {
    cardano_safe_memcpy(items, capacity * sizeof(sorted_entry_t), map->items, map->count * sizeof(sorted_entry_t));
  }

  _cardano_free(map->items);

  map->items    = items;
  map->capacity = capacity;

  return CARDANO_SUCCESS;
}

/**
 * \brief Inserts an entry into a sorted map, merging it with an entry of the same key.
 *
 * \param[in] map The map to insert into.
 * \param[in] key The ledger sort key of the entry.
 * \param[in] key_data The encoded key. A new reference is taken.
 * \param[in] inner_key The encoded key inside a nested map, or NULL.
 * \param[in] value The encoded value, or NULL when the entry carries an amount.
 * \param[in] amount The amount of a summed entry, or NULL. It is copied.
 * \param[in] mode How an entry with an existing key is merged.
 */
static cardano_error_t
sorted_map_put(
  sorted_map_t*           map,
  const sort_key_t*       key,
  cardano_plutus_data_t*  key_data,
  cardano_plutus_data_t*  inner_key,
  cardano_plutus_data_t*  value,
  const cardano_bigint_t* amount,
  const merge_mode_t      mode)
{
  size_t  position = 0U;
  int32_t order    = -1;

  while (position < map->count)
  {
    order = compare_sort_keys(&map->items[position].key, key);

    if (order >= 0)
    {
      break;
    }

    ++position;
  }

  if ((position < map->count) && (order == 0))
  {
    sorted_entry_t* entry = &map->items[position];

    if (mode == MERGE_KEEP_LAST)
    {
      cardano_plutus_data_ref(inner_key);
      cardano_plutus_data_ref(value);
      cardano_plutus_data_unref(&entry->inner_key);
      cardano_plutus_data_unref(&entry->value);

      entry->inner_key = inner_key;
      entry->value     = value;
    }

    if (mode == MERGE_SUM)
    {
      cardano_bigint_add(entry->amount, amount, entry->amount);
    }

    return CARDANO_SUCCESS;
  }

  cardano_error_t result = sorted_map_reserve(map);
  sorted_entry_t  entry;

  CARDANO_UNUSED(memset(&entry, 0, sizeof(entry)));

  if ((result == CARDANO_SUCCESS) && (amount != NULL))
  {
    result = cardano_bigint_clone(amount, &entry.amount);
  }

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  entry.key = *key;

  cardano_plutus_data_ref(key_data);
  cardano_plutus_data_ref(inner_key);
  cardano_plutus_data_ref(value);

  entry.key_data  = key_data;
  entry.inner_key = inner_key;
  entry.value     = value;

  CARDANO_UNUSED(memmove(&map->items[position + 1U], &map->items[position], (map->count - position) * sizeof(sorted_entry_t)));

  map->items[position] = entry;
  map->count           += 1U;

  return CARDANO_SUCCESS;
}

/**
 * \brief Encodes the value of a sorted-map entry: its amount when summed, its value otherwise.
 */
static cardano_error_t
sorted_entry_value(sorted_entry_t* entry, cardano_plutus_data_t** out)
{
  if (entry->amount != NULL)
  {
    return cardano_plutus_data_new_integer(entry->amount, out);
  }

  cardano_plutus_data_ref(entry->value);
  *out = entry->value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Encodes a sorted map as a plutus-data map, in key order.
 */
static cardano_error_t
sorted_map_encode(sorted_map_t* map, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* encoded = NULL;
  cardano_error_t       result  = cardano_plutus_map_new(&encoded);

  for (size_t i = 0U; (i < map->count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_data_t* value = NULL;

    result = sorted_entry_value(&map->items[i], &value);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_insert(encoded, map->items[i].key_data, value);
    }

    cardano_plutus_data_unref(&value);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(encoded, out);
  }

  cardano_plutus_map_unref(&encoded);

  return result;
}

/**
 * \brief Encodes the keys of a sorted map as a plutus-data list, in key order.
 */
static cardano_error_t
sorted_map_encode_keys(sorted_map_t* map, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  for (size_t i = 0U; (i < map->count) && (result == CARDANO_SUCCESS); ++i)
  {
    result = cardano_plutus_list_add(list, map->items[i].key_data);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Appends a sorted map to a plutus-data map as a map of nested maps.
 *
 * Consecutive entries with the same outer key form one nested map, keyed by
 * their inner keys.
 */
static cardano_error_t
sorted_map_append_nested(sorted_map_t* map, cardano_plutus_map_t* target)
{
  cardano_error_t result = CARDANO_SUCCESS;
  size_t          start  = 0U;

  while ((start < map->count) && (result == CARDANO_SUCCESS))
  {
    cardano_plutus_map_t*  inner    = NULL;
    cardano_plutus_data_t* inner_pd = NULL;
    size_t                 end      = start;

    result = cardano_plutus_map_new(&inner);

    while ((result == CARDANO_SUCCESS) && (end < map->count) && cardano_plutus_data_equals(map->items[end].key_data, map->items[start].key_data))
    {
      cardano_plutus_data_t* value = NULL;

      result = sorted_entry_value(&map->items[end], &value);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_map_insert(inner, map->items[end].inner_key, value);
      }

      cardano_plutus_data_unref(&value);

      ++end;
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_map(inner, &inner_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_map_insert(target, map->items[start].key_data, inner_pd);
    }

    cardano_plutus_map_unref(&inner);
    cardano_plutus_data_unref(&inner_pd);

    start = end;
  }

  return result;
}

/**
 * \brief Encodes a sorted map of nested entries as a plutus-data map of maps.
 */
static cardano_error_t
sorted_map_encode_nested(sorted_map_t* map, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* encoded = NULL;
  cardano_error_t       result  = cardano_plutus_map_new(&encoded);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_append_nested(map, encoded);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_map(encoded, out);
  }

  cardano_plutus_map_unref(&encoded);

  return result;
}

/**
 * \brief Appends a reward account to a sort key in the ledger order.
 *
 * The ledger orders accounts by network, test networks first, then by credential.
 */
static cardano_error_t
sort_key_append_account(sort_key_t* key, cardano_reward_address_t* address)
{
  byte_t          bytes[CARDANO_LEDGER_ORDER_KEY_SIZE] = { 0 };
  cardano_error_t result                               = _cardano_ledger_order_reward_address_key(address, bytes);

  if (result == CARDANO_SUCCESS)
  {
    sort_key_append(key, bytes, sizeof(bytes));
  }

  return result;
}

/**
 * \brief Builds the sort key and the encoded credential of a reward account.
 */
static cardano_error_t
account_entry(cardano_reward_address_t* address, sort_key_t* key, cardano_plutus_data_t** credential_data)
{
  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = sort_key_append_account(key, address);

  if (result == CARDANO_SUCCESS)
  {
    credential = cardano_reward_address_get_credential(address);
    result     = encode_credential(credential, credential_data);
  }

  cardano_credential_unref(&credential);

  return result;
}

/**
 * \brief Adds an amount keyed by a reward account to a sorted map, summing amounts of the same account.
 */
static cardano_error_t
put_account_amount(sorted_map_t* map, cardano_reward_address_t* address, const uint64_t amount)
{
  sort_key_t             key;
  cardano_plutus_data_t* credential = NULL;
  cardano_bigint_t*      value      = NULL;

  key.size = 0U;

  cardano_error_t result = account_entry(address, &key, &credential);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_bigint_from_unsigned_int(amount, &value);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_put(map, &key, credential, NULL, NULL, value, MERGE_SUM);
  }

  cardano_plutus_data_unref(&credential);
  cardano_bigint_unref(&value);

  return result;
}

/**
 * \brief Adds the withdrawals of a transaction to a sorted map of account amounts.
 */
static cardano_error_t
put_withdrawals(sorted_map_t* map, cardano_withdrawal_map_t* withdrawals)
{
  const size_t    count  = (withdrawals != NULL) ? cardano_withdrawal_map_get_length(withdrawals) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_reward_address_t* address = NULL;
    uint64_t                  amount  = 0U;

    result = cardano_withdrawal_map_get_key_value_at(withdrawals, i, &address, &amount);

    if (result == CARDANO_SUCCESS)
    {
      result = put_account_amount(map, address, amount);
    }

    cardano_reward_address_unref(&address);
  }

  return result;
}

/**
 * \brief Adds the amounts of a multi-asset to a sorted map keyed by policy id and asset name.
 *
 * Entries are ordered as the ledger orders them: by policy id, then by asset
 * name compared byte by byte. The filter keeps every amount, only the positive
 * ones or only the negative ones; amounts of the same asset are summed.
 */
static cardano_error_t
put_assets(sorted_map_t* map, cardano_multi_asset_t* multi_asset, const asset_filter_t filter)
{
  if (multi_asset == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_policy_id_list_t* policies = NULL;
  cardano_error_t           result   = cardano_multi_asset_get_keys(multi_asset, &policies);
  const size_t              count    = (result == CARDANO_SUCCESS) ? cardano_policy_id_list_get_length(policies) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_blake2b_hash_t*   policy_id = NULL;
    cardano_asset_name_map_t* assets    = NULL;
    cardano_plutus_data_t*    policy_pd = NULL;

    result = cardano_policy_id_list_get(policies, i, &policy_id);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_multi_asset_get_assets(multi_asset, policy_id, &assets);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = hash_bytes(policy_id, &policy_pd);
    }

    const size_t asset_count = (result == CARDANO_SUCCESS) ? cardano_asset_name_map_get_length(assets) : 0U;

    for (size_t j = 0U; (j < asset_count) && (result == CARDANO_SUCCESS); ++j)
    {
      cardano_asset_name_t*  asset_name = NULL;
      int64_t                quantity   = 0;
      cardano_plutus_data_t* name_pd    = NULL;
      cardano_bigint_t*      amount     = NULL;
      sort_key_t             key;

      key.size = 0U;
      result   = cardano_asset_name_map_get_key_value_at(assets, j, &asset_name, &quantity);

      const bool keep = (filter == ASSETS_ALL) || ((filter == ASSETS_POSITIVE) && (quantity > 0)) || ((filter == ASSETS_NEGATIVE) && (quantity < 0));

      if ((result == CARDANO_SUCCESS) && keep)
      {
        sort_key_append_hash(&key, policy_id);
        sort_key_append(&key, cardano_asset_name_get_bytes(asset_name), cardano_asset_name_get_bytes_size(asset_name));

        result = encode_bytes(cardano_asset_name_get_bytes(asset_name), cardano_asset_name_get_bytes_size(asset_name), &name_pd);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_bigint_from_int(quantity, &amount);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = sorted_map_put(map, &key, policy_pd, name_pd, NULL, amount, MERGE_SUM);
        }
      }

      cardano_asset_name_unref(&asset_name);
      cardano_plutus_data_unref(&name_pd);
      cardano_bigint_unref(&amount);
    }

    cardano_blake2b_hash_unref(&policy_id);
    cardano_asset_name_map_unref(&assets);
    cardano_plutus_data_unref(&policy_pd);
  }

  cardano_policy_id_list_unref(&policies);

  return result;
}

/**
 * \brief Computes the greatest common divisor of two unsigned integers.
 */
static uint64_t
gcd_u64(const uint64_t lhs, const uint64_t rhs)
{
  uint64_t a = lhs;
  uint64_t b = rhs;

  while (b != 0U)
  {
    const uint64_t remainder = a % b;

    a = b;
    b = remainder;
  }

  return a;
}

/**
 * \brief Adds a credential to a sorted map keyed by credential, with an optional value.
 */
static cardano_error_t
put_credential(sorted_map_t* map, cardano_credential_t* credential, cardano_plutus_data_t* value)
{
  sort_key_t             key;
  cardano_plutus_data_t* key_pd = NULL;

  key.size = 0U;

  cardano_error_t result = sort_key_append_credential(&key, credential);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_credential(credential, &key_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_put(map, &key, key_pd, NULL, value, NULL, MERGE_KEEP_FIRST);
  }

  cardano_plutus_data_unref(&key_pd);

  return result;
}

/**
 * \brief Inserts the ada entry (empty policy id -> {empty asset name -> coin}) into a value map.
 *
 * The entry is inserted even for a zero coin, as the ledger's \c transValue does.
 */
static cardano_error_t
value_add_coin(cardano_plutus_map_t* value_map, const int64_t coin)
{
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
 * \brief Appends the policies and amounts of a multi-asset to a value map, in ledger order.
 *
 * Policies are ordered by id and the assets of a policy by name compared byte by
 * byte, the order the ledger folds its asset maps in.
 */
static cardano_error_t
value_add_multiasset(cardano_plutus_map_t* value_map, cardano_multi_asset_t* multi_asset)
{
  sorted_map_t map = { NULL, 0U, 0U };

  cardano_error_t result = put_assets(&map, multi_asset, ASSETS_ALL);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_append_nested(&map, value_map);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes a transaction-output value as the zero-ada-asset Value map.
 *
 * The ada entry is always present, even for a zero coin, followed by the
 * policies and assets in ledger order. Every Plutus version shares this shape.
 */
static cardano_error_t
encode_value(cardano_value_t* value, cardano_plutus_data_t** out)
{
  cardano_plutus_map_t* value_map = NULL;
  cardano_error_t       result    = cardano_plutus_map_new(&value_map);

  if (result == CARDANO_SUCCESS)
  {
    result = value_add_coin(value_map, cardano_value_get_coin(value));
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
    result = value_add_coin(value_map, (int64_t)coin);
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
        cardano_error_t         res  = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

        if (hash != NULL)
        {
          res = hash_bytes(hash, &pd);
        }

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

      result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;

      if (hash != NULL)
      {
        result = hash_bytes(hash, &pd);
      }

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
 * \brief Adds the witness datums of a transaction to a sorted map keyed by datum hash.
 */
static cardano_error_t
put_datums(sorted_map_t* map, cardano_plutus_data_set_t* datums)
{
  const size_t    count  = (datums != NULL) ? cardano_plutus_data_set_get_length(datums) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_data_t*  datum   = NULL;
    cardano_blake2b_hash_t* hash    = NULL;
    cardano_plutus_data_t*  hash_pd = NULL;
    sort_key_t              key;

    key.size = 0U;
    result   = cardano_plutus_data_set_get(datums, i, &datum);

    if (result == CARDANO_SUCCESS)
    {
      result = datum_hash(datum, &hash);
    }

    if (result == CARDANO_SUCCESS)
    {
      sort_key_append_hash(&key, hash);
      result = hash_bytes(hash, &hash_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = sorted_map_put(map, &key, hash_pd, NULL, datum, NULL, MERGE_KEEP_FIRST);
    }

    cardano_plutus_data_unref(&datum);
    cardano_blake2b_hash_unref(&hash);
    cardano_plutus_data_unref(&hash_pd);
  }

  return result;
}

/**
 * \brief Encodes the witness datums as a map of datum hash -> datum, in hash order.
 *
 * The ledger keeps the witness datums in a map keyed by hash, so the context
 * lists them by hash, not in witness order, and a repeated datum appears once.
 */
static cardano_error_t
encode_data(cardano_plutus_data_set_t* datums, cardano_plutus_data_t** out)
{
  sorted_map_t map = { NULL, 0U, 0U };

  cardano_error_t result = put_datums(&map, datums);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&map, out);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes the witness datums as the V1 list of (datum hash, datum) pairs, in hash order.
 *
 * The V1 \c txInfoData field has type \c [(DatumHash, Datum)], so each entry is
 * a tuple Constr 0 [hash, datum] rather than a map entry.
 */
static cardano_error_t
encode_data_v1(cardano_plutus_data_set_t* datums, cardano_plutus_data_t** out)
{
  sorted_map_t           map    = { NULL, 0U, 0U };
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = put_datums(&map, datums);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_new(&list);
  }

  for (size_t i = 0U; (i < map.count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_list_t* tuple    = NULL;
    cardano_plutus_data_t* tuple_pd = NULL;

    result = cardano_plutus_list_new(&tuple);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(tuple, map.items[i].key_data);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(tuple, map.items[i].value);
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

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);
  sorted_map_free(&map);

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
 * \brief Rejects a transaction that carries Dijkstra-only fields a V1, V2 or V3 context cannot represent.
 *
 * Mirrors the ledger's \c guardDijkstraFeaturesForPlutusV1toV3: a V1, V2 or V3
 * script context cannot be built for a transaction with direct deposits,
 * account balance intervals, required top-level guards, or a script-hash
 * credential among its guards. Key-hash guards are accepted, since those
 * contexts see them as signatories.
 */
static cardano_error_t
guard_dijkstra_features_v1v3(cardano_transaction_body_t* body)
{
  cardano_direct_deposit_map_t*            deposits    = cardano_transaction_body_get_direct_deposits(body);
  cardano_account_balance_intervals_map_t* intervals   = cardano_transaction_body_get_account_balance_intervals(body);
  cardano_required_guards_map_t*           required    = cardano_transaction_body_get_required_top_level_guards(body);
  cardano_guard_set_t*                     guards      = cardano_transaction_body_get_guards(body);
  const size_t                             guard_count = (guards != NULL) ? cardano_guard_set_get_length(guards) : 0U;
  cardano_error_t                          result      = CARDANO_SUCCESS;

  if ((deposits != NULL) && (cardano_direct_deposit_map_get_length(deposits) > 0U))
  {
    result = CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if ((intervals != NULL) && (cardano_account_balance_intervals_map_get_length(intervals) > 0U))
  {
    result = CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if ((required != NULL) && (cardano_required_guards_map_get_length(required) > 0U))
  {
    result = CARDANO_ERROR_INVALID_ARGUMENT;
  }

  for (size_t i = 0U; (result == CARDANO_SUCCESS) && (i < guard_count); ++i)
  {
    cardano_credential_t*     credential = NULL;
    cardano_credential_type_t type       = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

    result = cardano_guard_set_get(guards, i, &credential);

    if ((result == CARDANO_SUCCESS) && (cardano_credential_get_type(credential, &type) == CARDANO_SUCCESS) && (type == CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH))
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
    }

    cardano_credential_unref(&credential);
  }

  cardano_direct_deposit_map_unref(&deposits);
  cardano_account_balance_intervals_map_unref(&intervals);
  cardano_required_guards_map_unref(&required);
  cardano_guard_set_unref(&guards);

  return result;
}

/**
 * \brief Validates the reference inputs of a V1 transaction (which has no reference-input field).
 *
 * Mirrors the ledger's V1 \c toPlutusTxInfo, which resolves each reference input
 * through \c transTxInInfoV1 (and so fails on an output the V1 TxOut cannot
 * represent, such as an inline datum or a reference script) and then discards it. Our
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
    result = guard_dijkstra_features_v1v3(body);
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
    result = is_v2 ? encode_data(datums, &data_pd) : encode_data_v1(datums, &data_pd);
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
    result = encode_value(value, &val_pd);
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
 * full Address and not the V1/V2 StakingHash wrapper. Entries follow the
 * ledger's reward-account order: by network, then script credentials before key
 * credentials.
 */
static cardano_error_t
withdrawals_v3(cardano_withdrawal_map_t* withdrawals, cardano_plutus_data_t** out)
{
  sorted_map_t map = { NULL, 0U, 0U };

  cardano_error_t result = put_withdrawals(&map, withdrawals);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&map, out);
  }

  sorted_map_free(&map);

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
 * \brief Builds the sort key and the encoding of a voter.
 */
static cardano_error_t
voter_entry(cardano_voter_t* voter, sort_key_t* key, cardano_plutus_data_t** voter_data)
{
  cardano_error_t result = sort_key_append_voter(key, voter);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_voter(voter, voter_data);
  }

  return result;
}

/**
 * \brief Encodes a governance action id: Constr 0 [transaction id, action index].
 */
static cardano_error_t
gov_action_id(cardano_governance_action_id_t* id, cardano_plutus_data_t** out)
{
  uint64_t               index  = 0U;
  cardano_plutus_data_t* id_pd  = NULL;
  cardano_plutus_data_t* idx_pd = NULL;
  cardano_error_t        result = encode_bytes(cardano_governance_action_id_get_hash_bytes(id), cardano_governance_action_id_get_hash_bytes_size(id), &id_pd);

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
 *
 * Voters follow the ledger's voter order, committee members first, then DReps,
 * then stake pools, each with script credentials before key credentials.
 */
static cardano_error_t
encode_votes(cardano_voting_procedures_t* procedures, cardano_plutus_data_t** out)
{
  sorted_map_t          outer  = { NULL, 0U, 0U };
  cardano_error_t       result = CARDANO_SUCCESS;
  cardano_voter_list_t* voters = NULL;

  if (procedures != NULL)
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
    sort_key_t                           key;

    key.size = 0U;
    result   = cardano_voter_list_get(voters, i, &voter);

    if (result == CARDANO_SUCCESS)
    {
      result = voter_entry(voter, &key, &voter_pd);
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
      result = sorted_map_put(&outer, &key, voter_pd, NULL, inner_pd, NULL, MERGE_KEEP_FIRST);
    }

    cardano_voter_unref(&voter);
    cardano_governance_action_id_list_unref(&ids);
    cardano_plutus_data_unref(&voter_pd);
    cardano_plutus_map_unref(&inner);
    cardano_plutus_data_unref(&inner_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&outer, out);
  }

  cardano_voter_list_unref(&voters);
  sorted_map_free(&outer);

  return result;
}

/**
 * \brief Encodes a unit interval as a RationalNumber: Constr 0 [numerator, denominator] in lowest terms.
 *
 * The ledger translates the interval through a Haskell \c Rational, which is
 * always reduced.
 */
static cardano_error_t
encode_rational(cardano_unit_interval_t* interval, cardano_plutus_data_t** out)
{
  const uint64_t numerator   = cardano_unit_interval_get_numerator(interval);
  const uint64_t denominator = cardano_unit_interval_get_denominator(interval);
  const uint64_t divisor     = gcd_u64(numerator, denominator);
  const uint64_t scale       = (divisor == 0U) ? 1U : divisor;

  cardano_plutus_data_t* num_pd = NULL;
  cardano_plutus_data_t* den_pd = NULL;
  cardano_error_t        result = cardano_plutus_data_new_integer_from_uint(numerator / scale, &num_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(denominator / scale, &den_pd);
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
 * \brief Encodes the members an UpdateCommittee action removes and adds, in ledger credential order.
 *
 * The removed members are a list and the added members a map of credential to
 * expiration epoch, both with script credentials before key credentials.
 */
static cardano_error_t
committee_changes(cardano_update_committee_action_t* action, cardano_plutus_data_t** removed_out, cardano_plutus_data_t** added_out)
{
  cardano_credential_set_t*        removed     = cardano_update_committee_action_get_members_to_be_removed(action);
  cardano_committee_members_map_t* added       = cardano_update_committee_action_get_members_to_be_added(action);
  sorted_map_t                     removed_map = { NULL, 0U, 0U };
  sorted_map_t                     added_map   = { NULL, 0U, 0U };
  const size_t                     removed_n   = (removed != NULL) ? cardano_credential_set_get_length(removed) : 0U;
  const size_t                     added_n     = (added != NULL) ? cardano_committee_members_map_get_length(added) : 0U;
  cardano_error_t                  result      = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < removed_n) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_credential_t* credential = NULL;

    result = cardano_credential_set_get(removed, i, &credential);

    if (result == CARDANO_SUCCESS)
    {
      result = put_credential(&removed_map, credential, NULL);
    }

    cardano_credential_unref(&credential);
  }

  for (size_t i = 0U; (i < added_n) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_credential_t*  credential = NULL;
    uint64_t               epoch      = 0U;
    cardano_plutus_data_t* epoch_pd   = NULL;

    result = cardano_committee_members_map_get_key_value_at(added, i, &credential, &epoch);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_integer_from_uint(epoch, &epoch_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = put_credential(&added_map, credential, epoch_pd);
    }

    cardano_credential_unref(&credential);
    cardano_plutus_data_unref(&epoch_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_keys(&removed_map, removed_out);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&added_map, added_out);
  }

  sorted_map_free(&removed_map);
  sorted_map_free(&added_map);
  cardano_credential_set_unref(&removed);
  cardano_committee_members_map_unref(&added);

  return result;
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
      cardano_unit_interval_t*           quorum     = NULL;
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
        result = committee_changes(action, &removed_pd, &added_pd);
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
      cardano_unit_interval_unref(&quorum);
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

/* V4 STATIC FUNCTIONS *******************************************************/

/**
 * \brief Releases every plutus-data value of an array.
 */
static void
unref_all(cardano_plutus_data_t** items, const size_t count)
{
  for (size_t i = 0U; i < count; ++i)
  {
    cardano_plutus_data_unref(&items[i]);
  }
}

/**
 * \brief Builds a plutus-data list from an array of values, in order.
 */
static cardano_error_t
encode_list(cardano_plutus_data_t** items, const size_t count, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    result = cardano_plutus_list_add(list, items[i]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Builds a constructor of the given alternative from an array of fields.
 */
static cardano_error_t
encode_variant(const uint64_t alternative, cardano_plutus_data_t** fields, const size_t count, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    result = cardano_plutus_list_add(list, fields[i]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_constr(alternative, list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Reads the alternative and one field of a constructor plutus-data value.
 */
static cardano_error_t
constr_field_at(cardano_plutus_data_t* data, const size_t index, uint64_t* alternative, cardano_plutus_data_t** field)
{
  cardano_constr_plutus_data_t* constr = NULL;
  cardano_plutus_list_t*        fields = NULL;
  cardano_error_t               result = cardano_plutus_data_to_constr(data, &constr);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_constr_plutus_data_get_alternative(constr, alternative);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_constr_plutus_data_get_data(constr, &fields);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_get(fields, index, field);
  }

  cardano_plutus_list_unref(&fields);
  cardano_constr_plutus_data_unref(&constr);

  return result;
}

/**
 * \brief Extracts the script hash of an encoded credential.
 *
 * A script credential is Constr 1 [script hash]; a key credential has no script
 * hash and yields \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 */
static cardano_error_t
credential_script_hash(cardano_plutus_data_t* credential, cardano_plutus_data_t** script_hash)
{
  uint64_t               alternative = CONSTR_0;
  cardano_plutus_data_t* hash        = NULL;
  cardano_error_t        result      = constr_field_at(credential, 0U, &alternative, &hash);

  if ((result == CARDANO_SUCCESS) && (alternative != CONSTR_1))
  {
    result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_plutus_data_ref(hash);
    *script_hash = hash;
  }

  cardano_plutus_data_unref(&hash);

  return result;
}

/**
 * \brief Adds the direct deposits of a transaction to a sorted map of account amounts.
 */
static cardano_error_t
put_direct_deposits(sorted_map_t* map, cardano_direct_deposit_map_t* deposits)
{
  const size_t    count  = (deposits != NULL) ? cardano_direct_deposit_map_get_length(deposits) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_reward_address_t* address = NULL;
    uint64_t                  amount  = 0U;

    result = cardano_direct_deposit_map_get_key_value_at(deposits, i, &address, &amount);

    if (result == CARDANO_SUCCESS)
    {
      result = put_account_amount(map, address, amount);
    }

    cardano_reward_address_unref(&address);
  }

  return result;
}

/**
 * \brief Encodes the direct deposits of a transaction as the V4 map of credential to lovelace.
 */
static cardano_error_t
direct_deposits_v4(cardano_direct_deposit_map_t* deposits, cardano_plutus_data_t** out)
{
  sorted_map_t map = { NULL, 0U, 0U };

  cardano_error_t result = put_direct_deposits(&map, deposits);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&map, out);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes an account balance interval.
 *
 * Lower bound => Constr 0 [lower], upper bound => Constr 1 [upper], both bounds
 * => Constr 2 [lower, upper], exact balance => Constr 3 [balance].
 */
static cardano_error_t
balance_interval_v4(const cardano_account_balance_interval_t* interval, cardano_plutus_data_t** out)
{
  const uint64_t* exact  = cardano_account_balance_interval_get_exact_balance(interval);
  const uint64_t* lower  = cardano_account_balance_interval_get_inclusive_lower_bound(interval);
  const uint64_t* upper  = cardano_account_balance_interval_get_exclusive_upper_bound(interval);
  uint64_t        bounds = 0U;

  cardano_plutus_data_t* fields[2] = { NULL, NULL };
  cardano_error_t        result    = CARDANO_SUCCESS;
  uint64_t               variant   = CONSTR_3;

  if (exact != NULL)
  {
    result = cardano_plutus_data_new_integer_from_uint(*exact, &fields[0]);
    bounds = 1U;
  }
  else if ((lower != NULL) && (upper != NULL))
  {
    variant = CONSTR_2;
    result  = cardano_plutus_data_new_integer_from_uint(*lower, &fields[0]);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_data_new_integer_from_uint(*upper, &fields[1]);
    }

    bounds = 2U;
  }
  else if (lower != NULL)
  {
    variant = CONSTR_0;
    result  = cardano_plutus_data_new_integer_from_uint(*lower, &fields[0]);
    bounds  = 1U;
  }
  else
  {
    variant = CONSTR_1;
    result  = (upper != NULL) ? cardano_plutus_data_new_integer_from_uint(*upper, &fields[0]) : CARDANO_ERROR_INVALID_ARGUMENT;
    bounds  = 1U;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_variant(variant, fields, (size_t)bounds, out);
  }

  unref_all(fields, 2U);

  return result;
}

/**
 * \brief Encodes account balance intervals as the V4 map of account to interval, in ledger account order.
 */
static cardano_error_t
balance_intervals_v4(cardano_account_balance_intervals_map_t* intervals, cardano_plutus_data_t** out)
{
  sorted_map_t    map    = { NULL, 0U, 0U };
  const size_t    count  = (intervals != NULL) ? cardano_account_balance_intervals_map_get_length(intervals) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_reward_address_t*           address     = NULL;
    cardano_account_balance_interval_t* interval    = NULL;
    cardano_plutus_data_t*              credential  = NULL;
    cardano_plutus_data_t*              interval_pd = NULL;
    sort_key_t                          key;

    key.size = 0U;
    result   = cardano_account_balance_intervals_map_get_key_value_at(intervals, i, &address, &interval);

    if (result == CARDANO_SUCCESS)
    {
      result = account_entry(address, &key, &credential);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = balance_interval_v4(interval, &interval_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = sorted_map_put(&map, &key, credential, NULL, interval_pd, NULL, MERGE_KEEP_FIRST);
    }

    cardano_reward_address_unref(&address);
    cardano_account_balance_interval_unref(&interval);
    cardano_plutus_data_unref(&credential);
    cardano_plutus_data_unref(&interval_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&map, out);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes a multi-asset as a MintValue map: policies and assets in ledger order, without an ada entry.
 */
static cardano_error_t
mint_v4(cardano_multi_asset_t* multi_asset, const asset_filter_t filter, cardano_plutus_data_t** out)
{
  sorted_map_t map = { NULL, 0U, 0U };

  cardano_error_t result = put_assets(&map, multi_asset, filter);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_nested(&map, out);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes the staking part of a V4 address as a Maybe AccountId.
 *
 * A base address is staked to the account of its stake credential, an
 * enterprise address to none. Pointer, Byron and reward addresses cannot appear
 * in a V4 context and are rejected, as the ledger rejects them.
 */
static cardano_error_t
address_account_v4(const cardano_address_t* address, cardano_plutus_data_t** out)
{
  cardano_address_type_t type   = CARDANO_ADDRESS_TYPE_BYRON;
  cardano_error_t        result = cardano_address_get_type(address, &type);

  if (result == CARDANO_SUCCESS)
  {
    switch (type)
    {
      case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
      case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
      case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
      case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
      {
        cardano_base_address_t* base     = NULL;
        cardano_credential_t*   stake    = NULL;
        cardano_plutus_data_t*  stake_pd = NULL;

        result = cardano_base_address_from_address(address, &base);

        if (result == CARDANO_SUCCESS)
        {
          stake  = cardano_base_address_get_stake_credential(base);
          result = encode_credential(stake, &stake_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_maybe(stake_pd, out);
        }

        cardano_base_address_unref(&base);
        cardano_credential_unref(&stake);
        cardano_plutus_data_unref(&stake_pd);

        break;
      }
      case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
      case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
      {
        result = encode_maybe(NULL, out);
        break;
      }
      case CARDANO_ADDRESS_TYPE_POINTER_KEY:
      case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
      case CARDANO_ADDRESS_TYPE_BYRON:
      case CARDANO_ADDRESS_TYPE_REWARD_KEY:
      case CARDANO_ADDRESS_TYPE_REWARD_SCRIPT:
      default:
      {
        result = CARDANO_ERROR_INVALID_ADDRESS_TYPE;
        break;
      }
    }
  }

  return result;
}

/**
 * \brief Encodes a V4 address: the list [payment credential, Maybe AccountId].
 */
static cardano_error_t
address_v4(const cardano_address_t* address, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* fields[2] = { NULL, NULL };
  cardano_credential_t*  payment   = NULL;
  cardano_error_t        result    = address_account_v4(address, &fields[1]);

  if (result == CARDANO_SUCCESS)
  {
    payment = payment_credential(address);
    result  = encode_credential(payment, &fields[0]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, 2U, out);
  }

  cardano_credential_unref(&payment);
  unref_all(fields, 2U);

  return result;
}

/**
 * \brief Encodes the datum of a V4 transaction output as an OutputDatum.
 *
 * No datum => Constr 0 [], a datum hash => Constr 1 [hash], an inline datum =>
 * Constr 2 [datum].
 */
static cardano_error_t
output_datum_v4(cardano_datum_t* datum, cardano_plutus_data_t** out)
{
  if (datum == NULL)
  {
    return empty_constr(CONSTR_0, out);
  }

  cardano_datum_type_t   type   = CARDANO_DATUM_TYPE_DATA_HASH;
  cardano_plutus_data_t* value  = NULL;
  uint64_t               option = CONSTR_1;
  cardano_error_t        result = cardano_datum_get_type(datum, &type);

  if ((result == CARDANO_SUCCESS) && (type == CARDANO_DATUM_TYPE_INLINE_DATA))
  {
    option = CONSTR_2;
    value  = cardano_datum_get_inline_data(datum);
  }

  if ((result == CARDANO_SUCCESS) && (type == CARDANO_DATUM_TYPE_DATA_HASH))
  {
    result = encode_bytes(cardano_datum_get_data_hash_bytes(datum), cardano_datum_get_data_hash_bytes_size(datum), &value);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = wrap_constr(option, value, out);
  }

  cardano_plutus_data_unref(&value);

  return result;
}

/**
 * \brief Encodes a V4 transaction output: the list [address, value, datum option, Maybe script hash].
 */
static cardano_error_t
output_v4(cardano_transaction_output_t* output, cardano_plutus_data_t** out)
{
  cardano_address_t*     address   = cardano_transaction_output_get_address(output);
  cardano_value_t*       value     = cardano_transaction_output_get_value(output);
  cardano_datum_t*       datum     = cardano_transaction_output_get_datum(output);
  cardano_plutus_data_t* fields[4] = { NULL, NULL, NULL, NULL };
  cardano_error_t        result    = address_v4(address, &fields[0]);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_value(value, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = output_datum_v4(datum, &fields[2]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = output_script_ref(output, &fields[3]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, 4U, out);
  }

  cardano_address_unref(&address);
  cardano_value_unref(&value);
  cardano_datum_unref(&datum);
  unref_all(fields, 4U);

  return result;
}

/**
 * \brief Encodes a V4 output reference: the list [transaction id, index].
 */
static cardano_error_t
out_ref_v4(cardano_transaction_input_t* input, cardano_plutus_data_t** out)
{
  cardano_blake2b_hash_t* id        = cardano_transaction_input_get_id(input);
  cardano_plutus_data_t*  fields[2] = { NULL, NULL };
  cardano_error_t         result    = hash_bytes(id, &fields[0]);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(cardano_transaction_input_get_index(input), &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, 2U, out);
  }

  cardano_blake2b_hash_unref(&id);
  unref_all(fields, 2U);

  return result;
}

/**
 * \brief Encodes a set of inputs as the V4 list of TxInInfo records [out ref, resolved output].
 */
static cardano_error_t
inputs_v4(cardano_transaction_input_set_t* inputs, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);
  const size_t           count  = (inputs != NULL) ? cardano_transaction_input_set_get_length(inputs) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_transaction_input_t*  input     = NULL;
    cardano_transaction_output_t* output    = NULL;
    cardano_plutus_data_t*        fields[2] = { NULL, NULL };
    cardano_plutus_data_t*        info      = NULL;

    result = cardano_transaction_input_set_get(inputs, i, &input);

    if (result == CARDANO_SUCCESS)
    {
      output = resolve_output(resolved_inputs, input);
      result = (output != NULL) ? out_ref_v4(input, &fields[0]) : CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    if (result == CARDANO_SUCCESS)
    {
      result = output_v4(output, &fields[1]);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = encode_list(fields, 2U, &info);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, info);
    }

    cardano_transaction_input_unref(&input);
    cardano_transaction_output_unref(&output);
    cardano_plutus_data_unref(&info);
    unref_all(fields, 2U);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(list, out);
  }

  cardano_plutus_list_unref(&list);

  return result;
}

/**
 * \brief Rejects reference inputs that are also spent, as the ledger does before building a V4 TxInfo.
 */
static cardano_error_t
check_reference_inputs_disjoint(cardano_transaction_input_set_t* inputs, cardano_transaction_input_set_t* reference_inputs)
{
  const size_t    input_count     = (inputs != NULL) ? cardano_transaction_input_set_get_length(inputs) : 0U;
  const size_t    reference_count = (reference_inputs != NULL) ? cardano_transaction_input_set_get_length(reference_inputs) : 0U;
  cardano_error_t result          = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < reference_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_transaction_input_t* reference = NULL;

    result = cardano_transaction_input_set_get(reference_inputs, i, &reference);

    for (size_t j = 0U; (j < input_count) && (result == CARDANO_SUCCESS); ++j)
    {
      cardano_transaction_input_t* input = NULL;

      result = cardano_transaction_input_set_get(inputs, j, &input);

      if ((result == CARDANO_SUCCESS) && cardano_transaction_input_equals(input, reference))
      {
        result = CARDANO_ERROR_INVALID_ARGUMENT;
      }

      cardano_transaction_input_unref(&input);
    }

    cardano_transaction_input_unref(&reference);
  }

  return result;
}

/**
 * \brief Encodes the transaction outputs as the V4 list of outputs.
 */
static cardano_error_t
outputs_v4(cardano_transaction_output_list_t* outputs, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);
  const size_t           count  = (outputs != NULL) ? cardano_transaction_output_list_get_length(outputs) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_transaction_output_t* output = NULL;
    cardano_plutus_data_t*        pd     = NULL;

    result = cardano_transaction_output_list_get(outputs, i, &output);

    if (result == CARDANO_SUCCESS)
    {
      result = output_v4(output, &pd);
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
 * \brief Encodes a certificate as a V4 TxCert.
 *
 * Account registration and deregistration carry their deposit as a plain
 * lovelace amount: Constr 0 [credential, deposit] and Constr 1 [credential,
 * refund]. The other certificates share their V3 encoding. Certificates without
 * a deposit, genesis delegations and MIR certificates no longer exist in the
 * Dijkstra era and are rejected.
 */
static cardano_error_t
certificate_v4(cardano_certificate_t* certificate, cardano_plutus_data_t** out)
{
  cardano_cert_type_t type   = CARDANO_CERT_TYPE_STAKE_REGISTRATION;
  cardano_error_t     result = cardano_cert_get_type(certificate, &type);

  if (result == CARDANO_SUCCESS)
  {
    switch (type)
    {
      case CARDANO_CERT_TYPE_REGISTRATION:
      {
        cardano_registration_cert_t* cert       = NULL;
        cardano_credential_t*        credential = NULL;
        cardano_plutus_data_t*       fields[2]  = { NULL, NULL };

        result = cardano_certificate_to_registration(certificate, &cert);

        if (result == CARDANO_SUCCESS)
        {
          credential = cardano_registration_cert_get_stake_credential(cert);
          result     = encode_credential(credential, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_data_new_integer_from_uint(cardano_registration_cert_get_deposit(cert), &fields[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_0, fields, 2U, out);
        }

        cardano_registration_cert_unref(&cert);
        cardano_credential_unref(&credential);
        unref_all(fields, 2U);

        break;
      }
      case CARDANO_CERT_TYPE_UNREGISTRATION:
      {
        cardano_unregistration_cert_t* cert       = NULL;
        cardano_credential_t*          credential = NULL;
        cardano_plutus_data_t*         fields[2]  = { NULL, NULL };

        result = cardano_certificate_to_unregistration(certificate, &cert);

        if (result == CARDANO_SUCCESS)
        {
          credential = cardano_unregistration_cert_get_credential(cert);
          result     = encode_credential(credential, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_data_new_integer_from_uint(cardano_unregistration_cert_get_deposit(cert), &fields[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_1, fields, 2U, out);
        }

        cardano_unregistration_cert_unref(&cert);
        cardano_credential_unref(&credential);
        unref_all(fields, 2U);

        break;
      }
      case CARDANO_CERT_TYPE_STAKE_DELEGATION:
      case CARDANO_CERT_TYPE_POOL_REGISTRATION:
      case CARDANO_CERT_TYPE_POOL_RETIREMENT:
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
      {
        result = certificate_v3(certificate, out);
        break;
      }
      case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
      case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
      case CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION:
      case CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS:
      default:
      {
        result = CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
        break;
      }
    }
  }

  return result;
}

/**
 * \brief Encodes the certificates as the V4 list of TxCert.
 */
static cardano_error_t
certificates_v4(cardano_certificate_set_t* certificates, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);
  const size_t           count  = (certificates != NULL) ? cardano_certificate_set_get_length(certificates) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_certificate_t* certificate = NULL;
    cardano_plutus_data_t* pd          = NULL;

    result = cardano_certificate_set_get(certificates, i, &certificate);

    if (result == CARDANO_SUCCESS)
    {
      result = certificate_v4(certificate, &pd);
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
 * \brief Encodes an optional slot as a Maybe POSIXTime.
 */
static cardano_error_t
maybe_posix_time(const uint64_t* slot, const cardano_slot_config_t* slot_config, cardano_plutus_data_t** out)
{
  if (slot == NULL)
  {
    return encode_maybe(NULL, out);
  }

  uint64_t               time    = 0U;
  cardano_plutus_data_t* time_pd = NULL;
  cardano_error_t        result  = slot_to_posix(*slot, slot_config, &time);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(time, &time_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_maybe(time_pd, out);
  }

  cardano_plutus_data_unref(&time_pd);

  return result;
}

/**
 * \brief Encodes a validity interval as the V4 POSIXTimeRange: the list [Maybe from, Maybe until].
 *
 * The lower bound is the inclusive start of the first valid slot, the upper bound
 * the exclusive start of the first invalid slot; a missing bound is unbounded.
 */
static cardano_error_t
time_range_v4(const uint64_t* lower, const uint64_t* upper, const cardano_slot_config_t* slot_config, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* fields[2] = { NULL, NULL };
  cardano_error_t        result    = maybe_posix_time(lower, slot_config, &fields[0]);

  if (result == CARDANO_SUCCESS)
  {
    result = maybe_posix_time(upper, slot_config, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, 2U, out);
  }

  unref_all(fields, 2U);

  return result;
}

/**
 * \brief Encodes the guards of a transaction as the list of credentials, in transaction order.
 */
static cardano_error_t
guards_v4(cardano_guard_set_t* guards, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);
  const size_t           count  = (guards != NULL) ? cardano_guard_set_get_length(guards) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_credential_t*  credential = NULL;
    cardano_plutus_data_t* pd         = NULL;

    result = cardano_guard_set_get(guards, i, &credential);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_credential(credential, &pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(list, pd);
    }

    cardano_credential_unref(&credential);
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
 * \brief Adds the required top-level guards of a transaction to a sorted map keyed by credential.
 *
 * Each value is the guard datum as a Maybe Datum.
 */
static cardano_error_t
put_required_guards(sorted_map_t* map, cardano_required_guards_map_t* guards)
{
  const size_t    count  = (guards != NULL) ? cardano_required_guards_map_get_length(guards) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_credential_t*  credential = NULL;
    cardano_plutus_data_t* datum      = NULL;
    cardano_plutus_data_t* key_pd     = NULL;
    cardano_plutus_data_t* datum_pd   = NULL;
    sort_key_t             key;

    key.size = 0U;
    result   = cardano_required_guards_map_get_key_value_at(guards, i, &credential, &datum);

    if (result == CARDANO_SUCCESS)
    {
      result = sort_key_append_credential(&key, credential);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = encode_credential(credential, &key_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = encode_maybe(datum, &datum_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = sorted_map_put(map, &key, key_pd, NULL, datum_pd, NULL, MERGE_KEEP_FIRST);
    }

    cardano_credential_unref(&credential);
    cardano_plutus_data_unref(&datum);
    cardano_plutus_data_unref(&key_pd);
    cardano_plutus_data_unref(&datum_pd);
  }

  return result;
}

/**
 * \brief Encodes the required top-level guards as the V4 map of credential to Maybe Datum, in ledger order.
 */
static cardano_error_t
required_guards_v4(cardano_required_guards_map_t* guards, cardano_plutus_data_t** out)
{
  sorted_map_t map = { NULL, 0U, 0U };

  cardano_error_t result = put_required_guards(&map, guards);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&map, out);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes a V4 governance action id: the list [transaction id, action index].
 *
 * The sort key receives the transaction id and the index, the ledger order of
 * governance action ids.
 */
static cardano_error_t
gov_action_id_v4(cardano_governance_action_id_t* id, sort_key_t* key, cardano_plutus_data_t** out)
{
  const byte_t*          tx_id     = cardano_governance_action_id_get_hash_bytes(id);
  const size_t           id_size   = cardano_governance_action_id_get_hash_bytes_size(id);
  uint64_t               index     = 0U;
  cardano_plutus_data_t* fields[2] = { NULL, NULL };
  cardano_error_t        result    = encode_bytes(tx_id, id_size, &fields[0]);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_governance_action_id_get_index(id, &index);
  }

  if (result == CARDANO_SUCCESS)
  {
    sort_key_append(key, tx_id, id_size);
    sort_key_append_index(key, index);

    result = cardano_plutus_data_new_integer_from_uint(index, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, 2U, out);
  }

  unref_all(fields, 2U);

  return result;
}

/**
 * \brief Encodes an optional V4 governance action id as a Maybe GovernanceActionId.
 */
static cardano_error_t
maybe_gov_action_id_v4(cardano_governance_action_id_t* id, cardano_plutus_data_t** out)
{
  if (id == NULL)
  {
    return encode_maybe(NULL, out);
  }

  sort_key_t             key;
  cardano_plutus_data_t* id_pd = NULL;

  key.size = 0U;

  cardano_error_t result = gov_action_id_v4(id, &key, &id_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_maybe(id_pd, out);
  }

  cardano_plutus_data_unref(&id_pd);

  return result;
}

/**
 * \brief Adds the votes of a transaction to a sorted map keyed by voter and governance action id.
 *
 * A later vote of the same voter on the same action replaces an earlier one.
 */
static cardano_error_t
put_votes(sorted_map_t* map, cardano_voting_procedures_t* procedures)
{
  cardano_voter_list_t* voters = NULL;
  cardano_error_t       result = CARDANO_SUCCESS;

  if (procedures != NULL)
  {
    result = cardano_voting_procedures_get_voters(procedures, &voters);
  }

  const size_t voter_count = (voters != NULL) ? cardano_voter_list_get_length(voters) : 0U;

  for (size_t i = 0U; (i < voter_count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_voter_t*                     voter    = NULL;
    cardano_governance_action_id_list_t* ids      = NULL;
    cardano_plutus_data_t*               voter_pd = NULL;
    sort_key_t                           voter_key;

    voter_key.size = 0U;
    result         = cardano_voter_list_get(voters, i, &voter);

    if (result == CARDANO_SUCCESS)
    {
      result = voter_entry(voter, &voter_key, &voter_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_voting_procedures_get_governance_ids_by_voter(procedures, voter, &ids);
    }

    const size_t id_count = (result == CARDANO_SUCCESS) ? cardano_governance_action_id_list_get_length(ids) : 0U;

    for (size_t j = 0U; (j < id_count) && (result == CARDANO_SUCCESS); ++j)
    {
      cardano_governance_action_id_t* id        = NULL;
      cardano_voting_procedure_t*     procedure = NULL;
      cardano_plutus_data_t*          id_pd     = NULL;
      cardano_plutus_data_t*          vote_pd   = NULL;
      sort_key_t                      key       = voter_key;

      result = cardano_governance_action_id_list_get(ids, j, &id);

      if (result == CARDANO_SUCCESS)
      {
        result = gov_action_id_v4(id, &key, &id_pd);
      }

      if (result == CARDANO_SUCCESS)
      {
        procedure = cardano_voting_procedures_get(procedures, voter, id);
        result    = (procedure != NULL) ? encode_vote(cardano_voting_procedure_get_vote(procedure), &vote_pd) : CARDANO_ERROR_ELEMENT_NOT_FOUND;
      }

      if (result == CARDANO_SUCCESS)
      {
        result = sorted_map_put(map, &key, voter_pd, id_pd, vote_pd, NULL, MERGE_KEEP_LAST);
      }

      cardano_governance_action_id_unref(&id);
      cardano_voting_procedure_unref(&procedure);
      cardano_plutus_data_unref(&id_pd);
      cardano_plutus_data_unref(&vote_pd);
    }

    cardano_voter_unref(&voter);
    cardano_governance_action_id_list_unref(&ids);
    cardano_plutus_data_unref(&voter_pd);
  }

  cardano_voter_list_unref(&voters);

  return result;
}

/**
 * \brief Encodes the votes as the V4 map of voter to map of governance action id to vote, in ledger order.
 */
static cardano_error_t
votes_v4(cardano_voting_procedures_t* procedures, cardano_plutus_data_t** out)
{
  sorted_map_t map = { NULL, 0U, 0U };

  cardano_error_t result = put_votes(&map, procedures);

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_nested(&map, out);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes a unit interval as the V4 Rational: the list [numerator, denominator] in lowest terms.
 *
 * The ledger translates the interval through a Haskell \c Rational, which is
 * always reduced.
 */
static cardano_error_t
rational_v4(const cardano_unit_interval_t* interval, cardano_plutus_data_t** out)
{
  const uint64_t numerator   = cardano_unit_interval_get_numerator(interval);
  const uint64_t denominator = cardano_unit_interval_get_denominator(interval);
  const uint64_t divisor     = gcd_u64(numerator, denominator);
  const uint64_t scale       = (divisor == 0U) ? 1U : divisor;

  cardano_plutus_data_t* fields[2] = { NULL, NULL };
  cardano_error_t        result    = cardano_plutus_data_new_integer_from_uint(numerator / scale, &fields[0]);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(denominator / scale, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, 2U, out);
  }

  unref_all(fields, 2U);

  return result;
}

/**
 * \brief Encodes an optional script hash as a Maybe ScriptHash.
 */
static cardano_error_t
maybe_script_hash(cardano_blake2b_hash_t* hash, cardano_plutus_data_t** out)
{
  if (hash == NULL)
  {
    return encode_maybe(NULL, out);
  }

  cardano_plutus_data_t* hash_pd = NULL;
  cardano_error_t        result  = hash_bytes(hash, &hash_pd);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_maybe(hash_pd, out);
  }

  cardano_plutus_data_unref(&hash_pd);

  return result;
}

/**
 * \brief Encodes the fields of an UpdateCommittee action: removed members, added members and quorum.
 *
 * The removed members are a list and the added members a map, both in ledger
 * credential order.
 */
static cardano_error_t
update_committee_v4(cardano_update_committee_action_t* action, cardano_plutus_data_t** fields)
{
  cardano_unit_interval_t* quorum = cardano_update_committee_action_get_quorum(action);
  cardano_error_t          result = committee_changes(action, &fields[0], &fields[1]);

  if (result == CARDANO_SUCCESS)
  {
    result = (quorum != NULL) ? rational_v4(quorum, &fields[2]) : CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_unit_interval_unref(&quorum);

  return result;
}

/**
 * \brief Encodes the governance action of a proposal procedure as a V4 GovernanceAction.
 *
 * The constructors match V3, but governance action ids, protocol versions,
 * constitutions and rationals are list-encoded records, and every map and set
 * follows the ledger key order.
 */
static cardano_error_t
gov_action_v4(cardano_proposal_procedure_t* proposal, cardano_plutus_data_t** out)
{
  cardano_governance_action_type_t type   = CARDANO_GOVERNANCE_ACTION_TYPE_INFO;
  cardano_error_t                  result = cardano_proposal_procedure_get_action_type(proposal, &type);

  if (result == CARDANO_SUCCESS)
  {
    switch (type)
    {
      case CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE:
      {
        cardano_parameter_change_action_t* action    = NULL;
        cardano_governance_action_id_t*    prev      = NULL;
        cardano_protocol_param_update_t*   update    = NULL;
        cardano_blake2b_hash_t*            guardrail = NULL;
        cardano_plutus_data_t*             fields[3] = { NULL, NULL, NULL };

        result = cardano_proposal_procedure_to_parameter_change_action(proposal, &action);

        if (result == CARDANO_SUCCESS)
        {
          prev   = cardano_parameter_change_action_get_governance_action_id(action);
          result = maybe_gov_action_id_v4(prev, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          update = cardano_parameter_change_action_get_protocol_param_update(action);
          result = cardano_protocol_param_update_to_plutus_data(update, &fields[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          guardrail = cardano_parameter_change_action_get_policy_hash(action);
          result    = maybe_script_hash(guardrail, &fields[2]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_0, fields, 3U, out);
        }

        cardano_parameter_change_action_unref(&action);
        cardano_governance_action_id_unref(&prev);
        cardano_protocol_param_update_unref(&update);
        cardano_blake2b_hash_unref(&guardrail);
        unref_all(fields, 3U);

        break;
      }
      case CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION:
      {
        cardano_hard_fork_initiation_action_t* action    = NULL;
        cardano_governance_action_id_t*        prev      = NULL;
        cardano_protocol_version_t*            version   = NULL;
        cardano_plutus_data_t*                 fields[2] = { NULL, NULL };
        cardano_plutus_data_t*                 parts[2]  = { NULL, NULL };

        result = cardano_proposal_procedure_to_hard_fork_initiation_action(proposal, &action);

        if (result == CARDANO_SUCCESS)
        {
          prev   = cardano_hard_fork_initiation_action_get_governance_action_id(action);
          result = maybe_gov_action_id_v4(prev, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          version = cardano_hard_fork_initiation_action_get_protocol_version(action);
          result  = cardano_plutus_data_new_integer_from_uint(cardano_protocol_version_get_major(version), &parts[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_data_new_integer_from_uint(cardano_protocol_version_get_minor(version), &parts[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_list(parts, 2U, &fields[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_1, fields, 2U, out);
        }

        cardano_hard_fork_initiation_action_unref(&action);
        cardano_governance_action_id_unref(&prev);
        cardano_protocol_version_unref(&version);
        unref_all(fields, 2U);
        unref_all(parts, 2U);

        break;
      }
      case CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS:
      {
        cardano_treasury_withdrawals_action_t* action      = NULL;
        cardano_withdrawal_map_t*              withdrawals = NULL;
        cardano_blake2b_hash_t*                guardrail   = NULL;
        cardano_plutus_data_t*                 fields[2]   = { NULL, NULL };

        result = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal, &action);

        if (result == CARDANO_SUCCESS)
        {
          withdrawals = cardano_treasury_withdrawals_action_get_withdrawals(action);
          result      = withdrawals_v3(withdrawals, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          guardrail = cardano_treasury_withdrawals_action_get_policy_hash(action);
          result    = maybe_script_hash(guardrail, &fields[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_2, fields, 2U, out);
        }

        cardano_treasury_withdrawals_action_unref(&action);
        cardano_withdrawal_map_unref(&withdrawals);
        cardano_blake2b_hash_unref(&guardrail);
        unref_all(fields, 2U);

        break;
      }
      case CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE:
      {
        cardano_no_confidence_action_t* action    = NULL;
        cardano_governance_action_id_t* prev      = NULL;
        cardano_plutus_data_t*          fields[1] = { NULL };

        result = cardano_proposal_procedure_to_no_confidence_action(proposal, &action);

        if (result == CARDANO_SUCCESS)
        {
          prev   = cardano_no_confidence_action_get_governance_action_id(action);
          result = maybe_gov_action_id_v4(prev, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_3, fields, 1U, out);
        }

        cardano_no_confidence_action_unref(&action);
        cardano_governance_action_id_unref(&prev);
        unref_all(fields, 1U);

        break;
      }
      case CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE:
      {
        cardano_update_committee_action_t* action    = NULL;
        cardano_governance_action_id_t*    prev      = NULL;
        cardano_plutus_data_t*             fields[4] = { NULL, NULL, NULL, NULL };

        result = cardano_proposal_procedure_to_update_committee_action(proposal, &action);

        if (result == CARDANO_SUCCESS)
        {
          prev   = cardano_update_committee_action_get_governance_action_id(action);
          result = maybe_gov_action_id_v4(prev, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = update_committee_v4(action, &fields[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_4, fields, 4U, out);
        }

        cardano_update_committee_action_unref(&action);
        cardano_governance_action_id_unref(&prev);
        unref_all(fields, 4U);

        break;
      }
      case CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION:
      {
        cardano_new_constitution_action_t* action       = NULL;
        cardano_governance_action_id_t*    prev         = NULL;
        cardano_constitution_t*            constitution = NULL;
        cardano_blake2b_hash_t*            script_hash  = NULL;
        cardano_plutus_data_t*             fields[2]    = { NULL, NULL };
        cardano_plutus_data_t*             script_pd    = NULL;

        result = cardano_proposal_procedure_to_constitution_action(proposal, &action);

        if (result == CARDANO_SUCCESS)
        {
          prev   = cardano_new_constitution_action_get_governance_action_id(action);
          result = maybe_gov_action_id_v4(prev, &fields[0]);
        }

        if (result == CARDANO_SUCCESS)
        {
          constitution = cardano_new_constitution_action_get_constitution(action);
          script_hash  = cardano_constitution_get_script_hash(constitution);
          result       = maybe_script_hash(script_hash, &script_pd);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_list(&script_pd, 1U, &fields[1]);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = encode_variant(CONSTR_5, fields, 2U, out);
        }

        cardano_new_constitution_action_unref(&action);
        cardano_governance_action_id_unref(&prev);
        cardano_constitution_unref(&constitution);
        cardano_blake2b_hash_unref(&script_hash);
        cardano_plutus_data_unref(&script_pd);
        unref_all(fields, 2U);

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
  }

  return result;
}

/**
 * \brief Encodes a V4 proposal procedure: the list [deposit, return credential, governance action].
 */
static cardano_error_t
proposal_procedure_v4(cardano_proposal_procedure_t* proposal, cardano_plutus_data_t** out)
{
  cardano_reward_address_t* reward     = cardano_proposal_procedure_get_reward_address(proposal);
  cardano_credential_t*     credential = cardano_reward_address_get_credential(reward);
  cardano_plutus_data_t*    fields[3]  = { NULL, NULL, NULL };
  cardano_error_t           result     = cardano_plutus_data_new_integer_from_uint(cardano_proposal_procedure_get_deposit(proposal), &fields[0]);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_credential(credential, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = gov_action_v4(proposal, &fields[2]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, 3U, out);
  }

  cardano_reward_address_unref(&reward);
  cardano_credential_unref(&credential);
  unref_all(fields, 3U);

  return result;
}

/**
 * \brief Encodes the proposal procedures as the V4 list, in transaction order.
 */
static cardano_error_t
proposal_procedures_v4(cardano_proposal_procedure_set_t* proposals, cardano_plutus_data_t** out)
{
  cardano_plutus_list_t* list   = NULL;
  cardano_error_t        result = cardano_plutus_list_new(&list);
  const size_t           count  = (proposals != NULL) ? cardano_proposal_procedure_set_get_length(proposals) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_proposal_procedure_t* proposal = NULL;
    cardano_plutus_data_t*        pd       = NULL;

    result = cardano_proposal_procedure_set_get(proposals, i, &proposal);

    if (result == CARDANO_SUCCESS)
    {
      result = proposal_procedure_v4(proposal, &pd);
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
 * \brief Releases the references held by a transaction view.
 */
static void
tx_view_release(tx_view_t* view)
{
  cardano_blake2b_hash_unref(&view->id);
  cardano_transaction_input_set_unref(&view->inputs);
  cardano_transaction_input_set_unref(&view->reference_inputs);
  cardano_transaction_output_list_unref(&view->outputs);
  cardano_multi_asset_unref(&view->mint);
  cardano_certificate_set_unref(&view->certificates);
  cardano_withdrawal_map_unref(&view->withdrawals);
  cardano_direct_deposit_map_unref(&view->direct_deposits);
  cardano_account_balance_intervals_map_unref(&view->balance_intervals);
  cardano_guard_set_unref(&view->guards);
  cardano_required_guards_map_unref(&view->required_guards);
  cardano_voting_procedures_unref(&view->votes);
  cardano_proposal_procedure_set_unref(&view->proposals);
  cardano_redeemer_list_unref(&view->redeemers);
  cardano_plutus_data_set_unref(&view->datums);
}

/**
 * \brief Copies an optional slot or amount into a view.
 */
static void
tx_view_set_optional(const uint64_t* source, bool* is_set, uint64_t* value)
{
  *is_set = (source != NULL);
  *value  = (source != NULL) ? *source : 0U;
}

/**
 * \brief Reads the view of a top-level transaction.
 */
static cardano_error_t
tx_view_from_transaction(cardano_transaction_t* tx, tx_view_t* view)
{
  cardano_transaction_body_t* body        = cardano_transaction_get_body(tx);
  cardano_witness_set_t*      witness_set = cardano_transaction_get_witness_set(tx);
  const uint64_t*             donation    = cardano_transaction_body_get_donation(body);

  CARDANO_UNUSED(memset(view, 0, sizeof(tx_view_t)));

  view->id                = cardano_transaction_get_id(tx);
  view->inputs            = cardano_transaction_body_get_inputs(body);
  view->reference_inputs  = cardano_transaction_body_get_reference_inputs(body);
  view->outputs           = cardano_transaction_body_get_outputs(body);
  view->mint              = cardano_transaction_body_get_mint(body);
  view->certificates      = cardano_transaction_body_get_certificates(body);
  view->withdrawals       = cardano_transaction_body_get_withdrawals(body);
  view->direct_deposits   = cardano_transaction_body_get_direct_deposits(body);
  view->balance_intervals = cardano_transaction_body_get_account_balance_intervals(body);
  view->guards            = cardano_transaction_body_get_guards(body);
  view->required_guards   = cardano_transaction_body_get_required_top_level_guards(body);
  view->votes             = cardano_transaction_body_get_voting_procedures(body);
  view->proposals         = cardano_transaction_body_get_proposal_procedures(body);
  view->redeemers         = cardano_witness_set_get_redeemers(witness_set);
  view->datums            = cardano_witness_set_get_plutus_data(witness_set);
  view->donation          = (donation != NULL) ? *donation : 0U;

  tx_view_set_optional(cardano_transaction_body_get_invalid_before(body), &view->has_invalid_before, &view->invalid_before);
  tx_view_set_optional(cardano_transaction_body_get_invalid_after(body), &view->has_invalid_after, &view->invalid_after);
  tx_view_set_optional(cardano_transaction_body_get_treasury_value(body), &view->has_treasury, &view->treasury);

  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);

  return (view->id != NULL) ? CARDANO_SUCCESS : CARDANO_ERROR_POINTER_IS_NULL;
}

/**
 * \brief Reads the view of the sub-transaction at a position of the top-level transaction.
 *
 * The position is the index of the sub-transaction in the body of the top-level
 * transaction, and becomes the sub-transaction index of its TxInfo.
 */
static cardano_error_t
tx_view_from_sub_transaction(cardano_transaction_t* tx, const uint64_t index, tx_view_t* view)
{
  cardano_transaction_body_t*    body   = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* subs   = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_t*     sub    = NULL;
  cardano_error_t                result = CARDANO_SUCCESS;

  CARDANO_UNUSED(memset(view, 0, sizeof(tx_view_t)));

  if ((subs == NULL) || (index >= (uint64_t)cardano_sub_transaction_set_get_length(subs)))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_sub_transaction_set_get(subs, (size_t)index, &sub);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_sub_transaction_body_t* sub_body    = cardano_sub_transaction_get_body(sub);
    cardano_witness_set_t*          witness_set = cardano_sub_transaction_get_witness_set(sub);
    const uint64_t*                 donation    = cardano_sub_transaction_body_get_donation(sub_body);

    view->id                = cardano_sub_transaction_get_id(sub);
    view->is_sub_tx         = true;
    view->sub_tx_index      = index;
    view->inputs            = cardano_sub_transaction_body_get_inputs(sub_body);
    view->reference_inputs  = cardano_sub_transaction_body_get_reference_inputs(sub_body);
    view->outputs           = cardano_sub_transaction_body_get_outputs(sub_body);
    view->mint              = cardano_sub_transaction_body_get_mint(sub_body);
    view->certificates      = cardano_sub_transaction_body_get_certificates(sub_body);
    view->withdrawals       = cardano_sub_transaction_body_get_withdrawals(sub_body);
    view->direct_deposits   = cardano_sub_transaction_body_get_direct_deposits(sub_body);
    view->balance_intervals = cardano_sub_transaction_body_get_account_balance_intervals(sub_body);
    view->guards            = cardano_sub_transaction_body_get_guards(sub_body);
    view->required_guards   = cardano_sub_transaction_body_get_required_top_level_guards(sub_body);
    view->votes             = cardano_sub_transaction_body_get_voting_procedures(sub_body);
    view->proposals         = cardano_sub_transaction_body_get_proposal_procedures(sub_body);
    view->redeemers         = cardano_witness_set_get_redeemers(witness_set);
    view->datums            = cardano_witness_set_get_plutus_data(witness_set);
    view->donation          = (donation != NULL) ? *donation : 0U;

    tx_view_set_optional(cardano_sub_transaction_body_get_invalid_before(sub_body), &view->has_invalid_before, &view->invalid_before);
    tx_view_set_optional(cardano_sub_transaction_body_get_invalid_after(sub_body), &view->has_invalid_after, &view->invalid_after);
    tx_view_set_optional(cardano_sub_transaction_body_get_treasury_value(sub_body), &view->has_treasury, &view->treasury);

    cardano_sub_transaction_body_unref(&sub_body);
    cardano_witness_set_unref(&witness_set);

    result = (view->id != NULL) ? CARDANO_SUCCESS : CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_sub_transaction_unref(&sub);
  cardano_sub_transaction_set_unref(&subs);
  cardano_transaction_body_unref(&body);

  return result;
}

/**
 * \brief Reads the view of the transaction level a script runs at.
 *
 * \param[in] tx The top-level transaction.
 * \param[in] sub_tx_index The position of the sub-transaction, or NULL for the top level.
 * \param[out] view The view to fill.
 */
static cardano_error_t
tx_view_read(cardano_transaction_t* tx, const uint64_t* sub_tx_index, tx_view_t* view)
{
  if (sub_tx_index == NULL)
  {
    return tx_view_from_transaction(tx, view);
  }

  return tx_view_from_sub_transaction(tx, *sub_tx_index, view);
}

/**
 * \brief Finds the minting policy that a mint redeemer points to, in ledger policy order.
 */
static cardano_error_t
indexed_policy(cardano_multi_asset_t* mint, const uint64_t index, cardano_plutus_data_t** policy)
{
  sorted_map_t           map    = { NULL, 0U, 0U };
  cardano_plutus_data_t* found  = NULL;
  uint64_t               seen   = 0U;
  cardano_error_t        result = put_assets(&map, mint, ASSETS_ALL);

  for (size_t i = 0U; (result == CARDANO_SUCCESS) && (i < map.count); ++i)
  {
    const bool is_new_policy = (i == 0U) || !cardano_plutus_data_equals(map.items[i].key_data, map.items[i - 1U].key_data);

    if (is_new_policy && (seen == index))
    {
      found = map.items[i].key_data;
    }

    seen += is_new_policy ? 1U : 0U;
  }

  if ((result == CARDANO_SUCCESS) && (found == NULL))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_plutus_data_ref(found);
    *policy = found;
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes the voter that a voting redeemer points to, in ledger voter order.
 */
static cardano_error_t
indexed_voter(cardano_voting_procedures_t* procedures, const uint64_t index, cardano_plutus_data_t** voter_data)
{
  cardano_voter_t* voter  = NULL;
  cardano_error_t  result = _cardano_ledger_order_voter_at_index(procedures, index, &voter);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_voter(voter, voter_data);
  }

  cardano_voter_unref(&voter);

  return result;
}

/**
 * \brief Encodes the credential that a withdrawal redeemer points to, in ledger account order.
 */
static cardano_error_t
indexed_withdrawal(cardano_withdrawal_map_t* withdrawals, const uint64_t index, cardano_plutus_data_t** credential)
{
  cardano_reward_address_t* address = NULL;
  cardano_credential_t*     cred    = NULL;
  cardano_error_t           result  = _cardano_ledger_order_withdrawal_at_index(withdrawals, index, &address);

  if (result == CARDANO_SUCCESS)
  {
    cred   = cardano_reward_address_get_credential(address);
    result = encode_credential(cred, credential);
  }

  cardano_credential_unref(&cred);
  cardano_reward_address_unref(&address);

  return result;
}

/**
 * \brief Extracts the script hash that witnesses an encoded V4 TxCert.
 *
 * Every certificate but the stake pool ones carries its witness credential as
 * its first field.
 */
static cardano_error_t
certificate_script_hash(cardano_plutus_data_t* certificate, cardano_plutus_data_t** script_hash)
{
  static const uint64_t TX_CERT_V4_POOL_REGISTER = 7U;
  static const uint64_t TX_CERT_V4_POOL_RETIRE   = 8U;

  uint64_t               alternative = CONSTR_0;
  cardano_plutus_data_t* credential  = NULL;
  cardano_error_t        result      = constr_field_at(certificate, 0U, &alternative, &credential);

  if ((result == CARDANO_SUCCESS) && ((alternative == TX_CERT_V4_POOL_REGISTER) || (alternative == TX_CERT_V4_POOL_RETIRE)))
  {
    result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = credential_script_hash(credential, script_hash);
  }

  cardano_plutus_data_unref(&credential);

  return result;
}

/**
 * \brief Extracts the script hash of an encoded voter: committee members and DReps may be scripts, stake pools never are.
 */
static cardano_error_t
voter_script_hash(cardano_plutus_data_t* voter, cardano_plutus_data_t** script_hash)
{
  uint64_t               alternative = CONSTR_0;
  cardano_plutus_data_t* credential  = NULL;
  cardano_error_t        result      = constr_field_at(voter, 0U, &alternative, &credential);

  if ((result == CARDANO_SUCCESS) && (alternative == CONSTR_2))
  {
    result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = credential_script_hash(credential, script_hash);
  }

  cardano_plutus_data_unref(&credential);

  return result;
}

/**
 * \brief Extracts the guardrail script hash of a proposal procedure.
 *
 * Only parameter changes and treasury withdrawals can carry a guardrail script.
 */
static cardano_error_t
proposal_script_hash(cardano_proposal_procedure_t* proposal, cardano_plutus_data_t** script_hash)
{
  cardano_governance_action_type_t type   = CARDANO_GOVERNANCE_ACTION_TYPE_INFO;
  cardano_blake2b_hash_t*          hash   = NULL;
  cardano_error_t                  result = cardano_proposal_procedure_get_action_type(proposal, &type);

  if ((result == CARDANO_SUCCESS) && (type == CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE))
  {
    cardano_parameter_change_action_t* action = NULL;

    result = cardano_proposal_procedure_to_parameter_change_action(proposal, &action);
    hash   = (result == CARDANO_SUCCESS) ? cardano_parameter_change_action_get_policy_hash(action) : NULL;

    cardano_parameter_change_action_unref(&action);
  }

  if ((result == CARDANO_SUCCESS) && (type == CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS))
  {
    cardano_treasury_withdrawals_action_t* action = NULL;

    result = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal, &action);
    hash   = (result == CARDANO_SUCCESS) ? cardano_treasury_withdrawals_action_get_policy_hash(action) : NULL;

    cardano_treasury_withdrawals_action_unref(&action);
  }

  if ((result == CARDANO_SUCCESS) && (hash == NULL))
  {
    result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = hash_bytes(hash, script_hash);
  }

  cardano_blake2b_hash_unref(&hash);

  return result;
}

/**
 * \brief Releases the values held by resolved purpose parts.
 */
static void
purpose_parts_release(purpose_parts_t* parts)
{
  unref_all(parts->items, PURPOSE_MAX_ITEMS);
  cardano_plutus_data_unref(&parts->script_hash);
  cardano_plutus_data_unref(&parts->guard);
}

/**
 * \brief Resolves the spending purpose: the spent output reference and the script locking it.
 */
static cardano_error_t
spend_parts(const tx_view_t* view, cardano_utxo_list_t* resolved_inputs, const uint64_t index, purpose_parts_t* parts)
{
  cardano_transaction_input_t*  input      = NULL;
  cardano_transaction_output_t* output     = NULL;
  cardano_address_t*            address    = NULL;
  cardano_credential_t*         payment    = NULL;
  cardano_plutus_data_t*        credential = NULL;
  cardano_error_t               result     = CARDANO_SUCCESS;

  if ((view->inputs == NULL) || (index >= (uint64_t)cardano_transaction_input_set_get_length(view->inputs)))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_transaction_input_set_get(view->inputs, (size_t)index, &input);
  }

  if (result == CARDANO_SUCCESS)
  {
    output = resolve_output(resolved_inputs, input);
    result = (output != NULL) ? out_ref_v4(input, &parts->items[0]) : CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    address = cardano_transaction_output_get_address(output);
    payment = payment_credential(address);
    result  = (payment != NULL) ? encode_credential(payment, &credential) : CARDANO_ERROR_INVALID_ADDRESS_TYPE;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = credential_script_hash(credential, &parts->script_hash);
  }

  parts->alternative = CONSTR_1;
  parts->item_count  = 1U;

  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);
  cardano_address_unref(&address);
  cardano_credential_unref(&payment);
  cardano_plutus_data_unref(&credential);

  return result;
}

/**
 * \brief Resolves the certifying purpose: the certificate index, the certificate and its witness script.
 */
static cardano_error_t
cert_parts(const tx_view_t* view, const uint64_t index, purpose_parts_t* parts)
{
  cardano_certificate_t* certificate = NULL;
  cardano_error_t        result      = CARDANO_SUCCESS;

  if ((view->certificates == NULL) || (index >= (uint64_t)cardano_certificate_set_get_length(view->certificates)))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_certificate_set_get(view->certificates, (size_t)index, &certificate);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(index, &parts->items[0]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = certificate_v4(certificate, &parts->items[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = certificate_script_hash(parts->items[1], &parts->script_hash);
  }

  parts->alternative = CONSTR_3;
  parts->item_count  = 2U;

  cardano_certificate_unref(&certificate);

  return result;
}

/**
 * \brief Resolves the proposing purpose: the proposal index, the proposal and its guardrail script.
 */
static cardano_error_t
propose_parts(const tx_view_t* view, const uint64_t index, purpose_parts_t* parts)
{
  cardano_proposal_procedure_t* proposal = NULL;
  cardano_error_t               result   = CARDANO_SUCCESS;

  if ((view->proposals == NULL) || (index >= (uint64_t)cardano_proposal_procedure_set_get_length(view->proposals)))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_proposal_procedure_set_get(view->proposals, (size_t)index, &proposal);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(index, &parts->items[0]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = proposal_procedure_v4(proposal, &parts->items[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = proposal_script_hash(proposal, &parts->script_hash);
  }

  parts->alternative = CONSTR_5;
  parts->item_count  = 2U;

  cardano_proposal_procedure_unref(&proposal);

  return result;
}

/**
 * \brief Resolves the guarding purpose: the guard index and the guard script.
 *
 * The index points into the guards of the transaction, which may also hold key
 * credentials; a guarding redeemer can only point to a script credential.
 */
static cardano_error_t
guard_parts(const tx_view_t* view, const uint64_t index, purpose_parts_t* parts)
{
  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = CARDANO_SUCCESS;

  if ((view->guards == NULL) || (index >= (uint64_t)cardano_guard_set_get_length(view->guards)))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_guard_set_get(view->guards, (size_t)index, &credential);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(index, &parts->items[0]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_credential(credential, &parts->guard);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = credential_script_hash(parts->guard, &parts->script_hash);
  }

  parts->alternative = CONSTR_6;
  parts->item_count  = 1U;

  cardano_credential_unref(&credential);

  return result;
}

/**
 * \brief Resolves what a redeemer points to at a transaction level.
 *
 * Fills the ScriptPurpose / ScriptInfo constructor alternative, the items both
 * share, and the hash of the script that runs. Redeemer indices follow the
 * ledger order of each collection: sorted inputs, policies, accounts and
 * voters, and the transaction order of certificates, proposals and guards.
 */
static cardano_error_t
resolve_purpose(
  tx_view_t*                   view,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_redeemer_tag_t tag,
  const uint64_t               index,
  purpose_parts_t*             parts)
{
  cardano_error_t result = CARDANO_SUCCESS;

  CARDANO_UNUSED(memset(parts, 0, sizeof(purpose_parts_t)));

  switch (tag)
  {
    case CARDANO_REDEEMER_TAG_SPEND:
    {
      result = spend_parts(view, resolved_inputs, index, parts);
      break;
    }
    case CARDANO_REDEEMER_TAG_MINT:
    {
      parts->alternative = CONSTR_0;
      parts->item_count  = 1U;
      result             = indexed_policy(view->mint, index, &parts->items[0]);

      if (result == CARDANO_SUCCESS)
      {
        cardano_plutus_data_ref(parts->items[0]);
        parts->script_hash = parts->items[0];
      }

      break;
    }
    case CARDANO_REDEEMER_TAG_REWARD:
    {
      parts->alternative = CONSTR_2;
      parts->item_count  = 1U;
      result             = indexed_withdrawal(view->withdrawals, index, &parts->items[0]);

      if (result == CARDANO_SUCCESS)
      {
        result = credential_script_hash(parts->items[0], &parts->script_hash);
      }

      break;
    }
    case CARDANO_REDEEMER_TAG_CERTIFYING:
    {
      result = cert_parts(view, index, parts);
      break;
    }
    case CARDANO_REDEEMER_TAG_VOTING:
    {
      parts->alternative = CONSTR_4;
      parts->item_count  = 1U;
      result             = indexed_voter(view->votes, index, &parts->items[0]);

      if (result == CARDANO_SUCCESS)
      {
        result = voter_script_hash(parts->items[0], &parts->script_hash);
      }

      break;
    }
    case CARDANO_REDEEMER_TAG_PROPOSING:
    {
      result = propose_parts(view, index, parts);
      break;
    }
    case CARDANO_REDEEMER_TAG_GUARDING:
    {
      result = guard_parts(view, index, parts);
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
 * \brief Encodes the ScriptPurpose of resolved purpose parts: Constr alternative [script hash, items...].
 */
static cardano_error_t
script_purpose_v4(const purpose_parts_t* parts, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* fields[PURPOSE_MAX_ITEMS + 1U] = { NULL };

  fields[0] = parts->script_hash;

  for (size_t i = 0U; i < parts->item_count; ++i)
  {
    fields[i + 1U] = parts->items[i];
  }

  return encode_variant(parts->alternative, fields, parts->item_count + 1U, out);
}

/**
 * \brief Encodes the redeemers of a transaction level as the V4 map of ScriptPurpose to redeemer, in ledger purpose order.
 *
 * When \p script_hashes is not NULL it also receives the hash of every script
 * the redeemers run.
 */
static cardano_error_t
redeemers_v4(tx_view_t* view, cardano_utxo_list_t* resolved_inputs, sorted_map_t* script_hashes, cardano_plutus_data_t** out)
{
  sorted_map_t    map    = { NULL, 0U, 0U };
  const size_t    count  = (view->redeemers != NULL) ? cardano_redeemer_list_get_length(view->redeemers) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_redeemer_t*    redeemer      = NULL;
    cardano_plutus_data_t* redeemer_data = NULL;
    cardano_plutus_data_t* purpose_pd    = NULL;
    purpose_parts_t        parts;
    sort_key_t             key;

    key.size = 0U;

    CARDANO_UNUSED(memset(&parts, 0, sizeof(purpose_parts_t)));

    result = cardano_redeemer_list_get(view->redeemers, i, &redeemer);

    if (result == CARDANO_SUCCESS)
    {
      const cardano_redeemer_tag_t tag   = cardano_redeemer_get_tag(redeemer);
      const uint64_t               index = cardano_redeemer_get_index(redeemer);

      sort_key_append_byte(&key, (byte_t)tag);
      sort_key_append_index(&key, index);

      result = resolve_purpose(view, resolved_inputs, tag, index, &parts);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = script_purpose_v4(&parts, &purpose_pd);
    }

    if (result == CARDANO_SUCCESS)
    {
      redeemer_data = cardano_redeemer_get_data(redeemer);
      result        = sorted_map_put(&map, &key, purpose_pd, NULL, redeemer_data, NULL, MERGE_KEEP_FIRST);
    }

    if ((result == CARDANO_SUCCESS) && (script_hashes != NULL))
    {
      sort_key_t hash_key;

      hash_key.size = 0U;

      cardano_buffer_t* bytes = NULL;

      result = cardano_plutus_data_to_bounded_bytes(parts.script_hash, &bytes);

      if (result == CARDANO_SUCCESS)
      {
        sort_key_append(&hash_key, cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes));
        result = sorted_map_put(script_hashes, &hash_key, parts.script_hash, NULL, NULL, NULL, MERGE_KEEP_FIRST);
      }

      cardano_buffer_unref(&bytes);
    }

    purpose_parts_release(&parts);
    cardano_redeemer_unref(&redeemer);
    cardano_plutus_data_unref(&redeemer_data);
    cardano_plutus_data_unref(&purpose_pd);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&map, out);
  }

  sorted_map_free(&map);

  return result;
}

/**
 * \brief Encodes the sub-transaction index of a TxInfo: Nothing at the top level, Just the index in a sub-transaction.
 */
static cardano_error_t
sub_tx_index_v4(const tx_view_t* view, cardano_plutus_data_t** out)
{
  if (!view->is_sub_tx)
  {
    return encode_maybe(NULL, out);
  }

  cardano_plutus_data_t* index  = NULL;
  cardano_error_t        result = cardano_plutus_data_new_integer_from_uint(view->sub_tx_index, &index);

  if (result == CARDANO_SUCCESS)
  {
    result = encode_maybe(index, out);
  }

  cardano_plutus_data_unref(&index);

  return result;
}

/**
 * \brief Encodes the 19 fields of a V4 TxInfo for a transaction level.
 *
 * The fields are, in order: id, sub-transaction index, inputs, reference inputs,
 * outputs, mint, certificates, withdrawals, direct deposits, account balance
 * intervals, validity range, guards, required top-level guards, redeemers,
 * datums, votes, proposals, current treasury amount and treasury donation.
 *
 * \param[in] view The transaction level.
 * \param[in] resolved_inputs The outputs spent or referenced by the level.
 * \param[in] slot_config The slot configuration of the network.
 * \param[in] script_hashes Receives the hashes of the scripts the redeemers run, or NULL.
 * \param[out] fields The encoded fields; the caller releases them.
 */
static cardano_error_t
tx_info_fields_v4(
  tx_view_t*                   view,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  sorted_map_t*                script_hashes,
  cardano_plutus_data_t**      fields)
{
  cardano_error_t result = check_reference_inputs_disjoint(view->inputs, view->reference_inputs);

  if (result == CARDANO_SUCCESS)
  {
    result = hash_bytes(view->id, &fields[0]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sub_tx_index_v4(view, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = inputs_v4(view->inputs, resolved_inputs, &fields[2]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = inputs_v4(view->reference_inputs, resolved_inputs, &fields[3]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = outputs_v4(view->outputs, &fields[4]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = mint_v4(view->mint, ASSETS_ALL, &fields[5]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = certificates_v4(view->certificates, &fields[6]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = withdrawals_v3(view->withdrawals, &fields[7]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = direct_deposits_v4(view->direct_deposits, &fields[8]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = balance_intervals_v4(view->balance_intervals, &fields[9]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = time_range_v4(
      view->has_invalid_before ? &view->invalid_before : NULL,
      view->has_invalid_after ? &view->invalid_after : NULL,
      slot_config,
      &fields[10]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = guards_v4(view->guards, &fields[11]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = required_guards_v4(view->required_guards, &fields[12]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = redeemers_v4(view, resolved_inputs, script_hashes, &fields[13]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_data(view->datums, &fields[14]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = votes_v4(view->votes, &fields[15]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = proposal_procedures_v4(view->proposals, &fields[16]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = maybe_coin(view->has_treasury ? &view->treasury : NULL, &fields[17]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer_from_uint(view->donation, &fields[18]);
  }

  return result;
}

/**
 * \brief Appends the items of an encoded list to a plutus-data list.
 */
static cardano_error_t
append_list_items(cardano_plutus_list_t* target, cardano_plutus_data_t* source)
{
  cardano_plutus_list_t* items  = NULL;
  cardano_error_t        result = cardano_plutus_data_to_list(source, &items);
  const size_t           count  = (result == CARDANO_SUCCESS) ? cardano_plutus_list_get_length(items) : 0U;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_data_t* item = NULL;

    result = cardano_plutus_list_get(items, i, &item);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(target, item);
    }

    cardano_plutus_data_unref(&item);
  }

  cardano_plutus_list_unref(&items);

  return result;
}

/**
 * \brief Releases the aggregates of a whole transaction.
 */
static void
batch_free(batch_t* batch)
{
  cardano_plutus_list_unref(&batch->sub_tx_infos);
  cardano_plutus_list_unref(&batch->ids);
  cardano_plutus_list_unref(&batch->inputs);
  cardano_plutus_list_unref(&batch->reference_inputs);
  cardano_plutus_list_unref(&batch->outputs);
  cardano_plutus_list_unref(&batch->certificates);
  cardano_plutus_list_unref(&batch->guards);
  cardano_plutus_list_unref(&batch->proposals);
  sorted_map_free(&batch->guard_datums);
  sorted_map_free(&batch->mints);
  sorted_map_free(&batch->burns);
  sorted_map_free(&batch->withdrawals);
  sorted_map_free(&batch->direct_deposits);
  sorted_map_free(&batch->required_guards);
  sorted_map_free(&batch->redeemer_hashes);
  sorted_map_free(&batch->datums);
  sorted_map_free(&batch->votes);
  cardano_bigint_unref(&batch->donations);
}

/**
 * \brief Prepares empty aggregates for a whole transaction.
 */
static cardano_error_t
batch_init(batch_t* batch)
{
  CARDANO_UNUSED(memset(batch, 0, sizeof(batch_t)));

  cardano_plutus_list_t** lists[] = {
    &batch->sub_tx_infos,
    &batch->ids,
    &batch->inputs,
    &batch->reference_inputs,
    &batch->outputs,
    &batch->certificates,
    &batch->guards,
    &batch->proposals
  };

  cardano_error_t result = cardano_bigint_from_unsigned_int(0U, &batch->donations);

  for (size_t i = 0U; (i < (sizeof(lists) / sizeof(lists[0]))) && (result == CARDANO_SUCCESS); ++i)
  {
    result = cardano_plutus_list_new(lists[i]);
  }

  return result;
}

/**
 * \brief Records the datum a transaction level supplies to the running guard, keyed by the level's id.
 *
 * A level supplies a datum when its required top-level guards hold the guard
 * credential with a datum.
 */
static cardano_error_t
batch_add_guard_datum(batch_t* batch, const tx_view_t* view, cardano_plutus_data_t* id, cardano_plutus_data_t* guard)
{
  const size_t    count  = (view->required_guards != NULL) ? cardano_required_guards_map_get_length(view->required_guards) : 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_credential_t*  credential    = NULL;
    cardano_plutus_data_t* datum         = NULL;
    cardano_plutus_data_t* credential_pd = NULL;

    result = cardano_required_guards_map_get_key_value_at(view->required_guards, i, &credential, &datum);

    if (result == CARDANO_SUCCESS)
    {
      result = encode_credential(credential, &credential_pd);
    }

    if ((result == CARDANO_SUCCESS) && (datum != NULL) && cardano_plutus_data_equals(credential_pd, guard))
    {
      sort_key_t key;

      key.size = 0U;

      sort_key_append_hash(&key, view->id);

      result = sorted_map_put(&batch->guard_datums, &key, id, NULL, datum, NULL, MERGE_KEEP_FIRST);
    }

    cardano_credential_unref(&credential);
    cardano_plutus_data_unref(&datum);
    cardano_plutus_data_unref(&credential_pd);
  }

  return result;
}

/**
 * \brief Narrows the batch validity interval to its intersection with a transaction level's interval.
 */
static void
batch_add_validity(batch_t* batch, const tx_view_t* view)
{
  if (view->has_invalid_before)
  {
    batch->lower     = (batch->has_lower && (batch->lower > view->invalid_before)) ? batch->lower : view->invalid_before;
    batch->has_lower = true;
  }

  if (view->has_invalid_after)
  {
    batch->upper     = (batch->has_upper && (batch->upper < view->invalid_after)) ? batch->upper : view->invalid_after;
    batch->has_upper = true;
  }
}

/**
 * \brief Keeps the first current treasury amount any transaction level of the batch states.
 */
static void
batch_add_treasury(batch_t* batch, const tx_view_t* view)
{
  if (view->has_treasury && !batch->has_treasury)
  {
    batch->treasury     = view->treasury;
    batch->has_treasury = true;
  }
}

/**
 * \brief Adds a transaction level to the aggregates of the whole transaction.
 *
 * The id, inputs, reference inputs, outputs, certificates, guards and proposals
 * fields of the level are aggregated as they are.
 *
 * \param[in] batch The aggregates.
 * \param[in] view The transaction level, added after the levels before it.
 * \param[in] fields The encoded TxInfo fields of the level.
 * \param[in] guard The encoded credential of the running guard.
 */
static cardano_error_t
batch_add(batch_t* batch, tx_view_t* view, cardano_plutus_data_t** fields, cardano_plutus_data_t* guard)
{
  static const size_t TX_INFO_V4_ID_FIELD         = 0U;
  static const size_t TX_INFO_V4_INPUTS_FIELD     = 2U;
  static const size_t TX_INFO_V4_REF_INPUTS_FIELD = 3U;
  static const size_t TX_INFO_V4_OUTPUTS_FIELD    = 4U;
  static const size_t TX_INFO_V4_CERTS_FIELD      = 6U;
  static const size_t TX_INFO_V4_GUARDS_FIELD     = 11U;
  static const size_t TX_INFO_V4_PROPOSALS_FIELD  = 16U;

  cardano_bigint_t* donation = NULL;
  cardano_error_t   result   = cardano_plutus_list_add(batch->ids, fields[TX_INFO_V4_ID_FIELD]);

  if (result == CARDANO_SUCCESS)
  {
    result = append_list_items(batch->inputs, fields[TX_INFO_V4_INPUTS_FIELD]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = append_list_items(batch->reference_inputs, fields[TX_INFO_V4_REF_INPUTS_FIELD]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = append_list_items(batch->outputs, fields[TX_INFO_V4_OUTPUTS_FIELD]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = append_list_items(batch->certificates, fields[TX_INFO_V4_CERTS_FIELD]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = append_list_items(batch->guards, fields[TX_INFO_V4_GUARDS_FIELD]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = append_list_items(batch->proposals, fields[TX_INFO_V4_PROPOSALS_FIELD]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = put_assets(&batch->mints, view->mint, ASSETS_POSITIVE);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = put_assets(&batch->burns, view->mint, ASSETS_NEGATIVE);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = put_withdrawals(&batch->withdrawals, view->withdrawals);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = put_direct_deposits(&batch->direct_deposits, view->direct_deposits);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = put_required_guards(&batch->required_guards, view->required_guards);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = put_datums(&batch->datums, view->datums);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = put_votes(&batch->votes, view->votes);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = batch_add_guard_datum(batch, view, fields[TX_INFO_V4_ID_FIELD], guard);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_bigint_from_unsigned_int(view->donation, &donation);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_bigint_add(batch->donations, donation, batch->donations);
    batch_add_validity(batch, view);
    batch_add_treasury(batch, view);
  }

  cardano_bigint_unref(&donation);

  return result;
}

/**
 * \brief Encodes the TopTxInfoSimplified of the whole transaction from its aggregates.
 */
static cardano_error_t
top_tx_info_simplified_v4(batch_t* batch, const cardano_slot_config_t* slot_config, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* fields[TOP_TX_INFO_SIMPLIFIED_FIELD_COUNT] = { NULL };
  cardano_error_t        result                                     = cardano_plutus_data_new_list(batch->ids, &fields[0]);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(batch->inputs, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(batch->reference_inputs, &fields[2]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(batch->outputs, &fields[3]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_nested(&batch->mints, &fields[4]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_nested(&batch->burns, &fields[5]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(batch->certificates, &fields[6]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&batch->withdrawals, &fields[7]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&batch->direct_deposits, &fields[8]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = time_range_v4(batch->has_lower ? &batch->lower : NULL, batch->has_upper ? &batch->upper : NULL, slot_config, &fields[9]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(batch->guards, &fields[10]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_keys(&batch->required_guards, &fields[11]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_keys(&batch->redeemer_hashes, &fields[12]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&batch->datums, &fields[13]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode_nested(&batch->votes, &fields[14]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(batch->proposals, &fields[15]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = maybe_coin(batch->has_treasury ? &batch->treasury : NULL, &fields[16]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_integer(batch->donations, &fields[17]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, TOP_TX_INFO_SIMPLIFIED_FIELD_COUNT, out);
  }

  unref_all(fields, TOP_TX_INFO_SIMPLIFIED_FIELD_COUNT);

  return result;
}

/**
 * \brief Adds every sub-transaction of a transaction to the aggregates, in body order.
 *
 * Each sub-transaction contributes its own TxInfo, whose sub-transaction index
 * is its position in the body.
 */
static cardano_error_t
batch_add_sub_transactions(
  batch_t*                     batch,
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t*       guard)
{
  cardano_transaction_body_t*    body   = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* subs   = cardano_transaction_body_get_sub_transactions(body);
  const size_t                   count  = (subs != NULL) ? cardano_sub_transaction_set_get_length(subs) : 0U;
  cardano_error_t                result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < count) && (result == CARDANO_SUCCESS); ++i)
  {
    tx_view_t              view;
    cardano_plutus_data_t* fields[TX_INFO_V4_FIELD_COUNT] = { NULL };
    cardano_plutus_data_t* info                           = NULL;

    result = tx_view_from_sub_transaction(tx, (uint64_t)i, &view);

    if (result == CARDANO_SUCCESS)
    {
      result = tx_info_fields_v4(&view, resolved_inputs, slot_config, &batch->redeemer_hashes, fields);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = encode_list(fields, TX_INFO_V4_FIELD_COUNT, &info);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_plutus_list_add(batch->sub_tx_infos, info);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = batch_add(batch, &view, fields, guard);
    }

    tx_view_release(&view);
    unref_all(fields, TX_INFO_V4_FIELD_COUNT);
    cardano_plutus_data_unref(&info);
  }

  cardano_sub_transaction_set_unref(&subs);
  cardano_transaction_body_unref(&body);

  return result;
}

/**
 * \brief Encodes the TopTxInfo a guard running at the top level receives.
 *
 * The record is the list [sub-transaction TxInfos, guard datums by transaction
 * id, starting account balance intervals, TopTxInfoSimplified]. The aggregates
 * take the sub-transactions in body order followed by the top-level
 * transaction.
 *
 * \param[in] batch The aggregates, already holding the redeemer script hashes of the top level.
 * \param[in] tx The top-level transaction.
 * \param[in] top The view of the top-level transaction.
 * \param[in] top_fields The encoded TxInfo fields of the top-level transaction.
 * \param[in] resolved_inputs The outputs spent or referenced by the whole transaction.
 * \param[in] slot_config The slot configuration of the network.
 * \param[in] guard The encoded credential of the running guard.
 * \param[out] out The encoded TopTxInfo.
 */
static cardano_error_t
top_tx_info_v4(
  batch_t*                     batch,
  cardano_transaction_t*       tx,
  tx_view_t*                   top,
  cardano_plutus_data_t**      top_fields,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t*       guard,
  cardano_plutus_data_t**      out)
{
  cardano_transaction_body_t*              body                            = cardano_transaction_get_body(tx);
  cardano_account_balance_intervals_map_t* starting                        = cardano_transaction_body_get_starting_account_balance_intervals(body);
  cardano_plutus_data_t*                   fields[TOP_TX_INFO_FIELD_COUNT] = { NULL };
  cardano_error_t                          result                          = batch_add_sub_transactions(batch, tx, resolved_inputs, slot_config, guard);

  if (result == CARDANO_SUCCESS)
  {
    result = batch_add(batch, top, top_fields, guard);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_list(batch->sub_tx_infos, &fields[0]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = sorted_map_encode(&batch->guard_datums, &fields[1]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = balance_intervals_v4(starting, &fields[2]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = top_tx_info_simplified_v4(batch, slot_config, &fields[3]);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, TOP_TX_INFO_FIELD_COUNT, out);
  }

  unref_all(fields, TOP_TX_INFO_FIELD_COUNT);
  cardano_account_balance_intervals_map_unref(&starting);
  cardano_transaction_body_unref(&body);

  return result;
}

/**
 * \brief Encodes the V4 ScriptInfo of resolved purpose parts.
 *
 * Spending scripts also receive their optional datum and guards their optional
 * TopTxInfo; every other script receives the purpose items as they are.
 */
static cardano_error_t
script_info_v4(const purpose_parts_t* parts, cardano_plutus_data_t* extra, cardano_plutus_data_t** out)
{
  cardano_plutus_data_t* fields[PURPOSE_MAX_ITEMS + 1U] = { NULL };
  size_t                 count                          = parts->item_count;

  for (size_t i = 0U; i < parts->item_count; ++i)
  {
    fields[i] = parts->items[i];
  }

  if (extra != NULL)
  {
    fields[count] = extra;
    ++count;
  }

  return encode_variant(parts->alternative, fields, count, out);
}

/**
 * \brief Builds the V4 ScriptContext list [TxInfo, redeemer, ScriptInfo, script hash] of a transaction level.
 */
static cardano_error_t
script_context_v4(
  cardano_transaction_t*       tx,
  tx_view_t*                   view,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t*       datum,
  cardano_plutus_data_t**      script_context)
{
  const cardano_redeemer_tag_t tag                                    = cardano_redeemer_get_tag(redeemer);
  const bool                   with_top                               = (tag == CARDANO_REDEEMER_TAG_GUARDING) && !view->is_sub_tx;
  cardano_plutus_data_t*       tx_fields[TX_INFO_V4_FIELD_COUNT]      = { NULL };
  cardano_plutus_data_t*       context[SCRIPT_CONTEXT_V4_FIELD_COUNT] = { NULL };
  cardano_plutus_data_t*       top                                    = NULL;
  cardano_plutus_data_t*       extra                                  = NULL;
  purpose_parts_t              parts;
  batch_t                      batch;

  CARDANO_UNUSED(memset(&parts, 0, sizeof(purpose_parts_t)));

  cardano_error_t result = batch_init(&batch);

  if (result == CARDANO_SUCCESS)
  {
    result = resolve_purpose(view, resolved_inputs, tag, cardano_redeemer_get_index(redeemer), &parts);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = tx_info_fields_v4(view, resolved_inputs, slot_config, with_top ? &batch.redeemer_hashes : NULL, tx_fields);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(tx_fields, TX_INFO_V4_FIELD_COUNT, &context[0]);
  }

  if (result == CARDANO_SUCCESS)
  {
    context[1] = cardano_redeemer_get_data(redeemer);
    result     = (context[1] != NULL) ? CARDANO_SUCCESS : CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((result == CARDANO_SUCCESS) && with_top)
  {
    result = top_tx_info_v4(&batch, tx, view, tx_fields, resolved_inputs, slot_config, parts.guard, &top);
  }

  if ((result == CARDANO_SUCCESS) && (tag == CARDANO_REDEEMER_TAG_SPEND))
  {
    result = encode_maybe(datum, &extra);
  }

  if ((result == CARDANO_SUCCESS) && (tag == CARDANO_REDEEMER_TAG_GUARDING))
  {
    result = encode_maybe(top, &extra);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = script_info_v4(&parts, extra, &context[2]);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_plutus_data_ref(parts.script_hash);
    context[3] = parts.script_hash;

    result = encode_list(context, SCRIPT_CONTEXT_V4_FIELD_COUNT, script_context);
  }

  batch_free(&batch);
  purpose_parts_release(&parts);
  unref_all(tx_fields, TX_INFO_V4_FIELD_COUNT);
  unref_all(context, SCRIPT_CONTEXT_V4_FIELD_COUNT);
  cardano_plutus_data_unref(&top);
  cardano_plutus_data_unref(&extra);

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
      cardano_plutus_data_t*    cred_pd     = NULL;
      cardano_plutus_data_t*    wrapped     = NULL;

      result = indexed_withdrawal(withdrawals, index, &cred_pd);

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_0, cred_pd, &wrapped);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_2, wrapped, purpose);
      }

      cardano_withdrawal_map_unref(&withdrawals);
      cardano_plutus_data_unref(&cred_pd);
      cardano_plutus_data_unref(&wrapped);

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
    result = guard_dijkstra_features_v1v3(body);
  }

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
      cardano_plutus_data_t*    cred_pd     = NULL;

      result = indexed_withdrawal(withdrawals, index, &cred_pd);

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_2, cred_pd, script_info);
      }

      cardano_withdrawal_map_unref(&withdrawals);
      cardano_plutus_data_unref(&cred_pd);

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
      cardano_plutus_data_t*       voter_pd   = NULL;

      result = indexed_voter(procedures, index, &voter_pd);

      if (result == CARDANO_SUCCESS)
      {
        result = wrap_constr(CONSTR_4, voter_pd, script_info);
      }

      cardano_voting_procedures_unref(&procedures);
      cardano_plutus_data_unref(&voter_pd);

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

cardano_error_t
cardano_uplc_int_build_tx_info_v4(
  cardano_transaction_t*       tx,
  const uint64_t*              sub_tx_index,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_plutus_data_t**      tx_info)
{
  if ((tx == NULL) || (resolved_inputs == NULL) || (slot_config == NULL) || (tx_info == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  tx_view_t              view;
  cardano_plutus_data_t* fields[TX_INFO_V4_FIELD_COUNT] = { NULL };
  cardano_error_t        result                         = tx_view_read(tx, sub_tx_index, &view);

  if (result == CARDANO_SUCCESS)
  {
    result = tx_info_fields_v4(&view, resolved_inputs, slot_config, NULL, fields);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = encode_list(fields, TX_INFO_V4_FIELD_COUNT, tx_info);
  }

  tx_view_release(&view);
  unref_all(fields, TX_INFO_V4_FIELD_COUNT);

  return result;
}

cardano_error_t
cardano_uplc_int_build_script_context_v4(
  cardano_transaction_t*       tx,
  const uint64_t*              sub_tx_index,
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

  tx_view_t       view;
  cardano_error_t result = tx_view_read(tx, sub_tx_index, &view);

  if (result == CARDANO_SUCCESS)
  {
    result = script_context_v4(tx, &view, resolved_inputs, slot_config, redeemer, datum, script_context);
  }

  tx_view_release(&view);

  return result;
}
