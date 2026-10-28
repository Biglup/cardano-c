/**
 * \file native_tx_evaluator.c
 *
 * \author angel.castillo
 * \date   Jun 20, 2026
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

#include "../../uplc/ast/uplc_term.h"
#include "../../uplc/cost/uplc_selected_cost_model.h"
#include "../../uplc/machine/uplc_machine.h"
#include <cardano/address/address.h>
#include <cardano/address/address_type.h>
#include <cardano/address/base_address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/pointer_address.h>
#include <cardano/address/reward_address.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/assets/policy_id_list.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/auth_committee_hot_cert.h>
#include <cardano/certs/cert_type.h>
#include <cardano/certs/certificate.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/resign_committee_cold_cert.h>
#include <cardano/certs/stake_delegation_cert.h>
#include <cardano/certs/stake_deregistration_cert.h>
#include <cardano/certs/stake_registration_delegation_cert.h>
#include <cardano/certs/stake_vote_delegation_cert.h>
#include <cardano/certs/stake_vote_registration_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/certs/vote_delegation_cert.h>
#include <cardano/certs/vote_registration_delegation_cert.h>
#include <cardano/common/credential.h>
#include <cardano/common/credential_type.h>
#include <cardano/common/datum.h>
#include <cardano/common/datum_type.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>
#include <cardano/protocol_params/cost_model.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/scripts/plutus_scripts/plutus_language_version.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v4_script.h>
#include <cardano/scripts/script.h>
#include <cardano/scripts/script_language.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_type.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/plutus_v1_script_set.h>
#include <cardano/witness_set/plutus_v2_script_set.h>
#include <cardano/witness_set/plutus_v3_script_set.h>
#include <cardano/witness_set/plutus_v4_script_set.h>
#include <cardano/witness_set/witness_set.h>

#include "../../allocators.h"
#include "../../common/ledger_order.h"
#include "../../string_safe.h"
#include "../../uplc/arena/uplc_arena.h"
#include "../../uplc/tx/script_context.h"
#include <cardano/uplc/uplc_apply_params.h>

#include <stddef.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief The arena block size used for one redeemer evaluation.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const size_t PRV_ARENA_BLOCK_SIZE = 4096U;

/**
 * \brief The CPU ceiling for the whole transaction, shared across redeemers.
 *
 * This is the aggregate transaction-level budget the evaluator hands out: each
 * redeemer's CEK machine is bounded by what remains after the earlier redeemers
 * have spent, mirroring the reference (Aiken eval_phase_two threads a single
 * remaining_budget through the redeemer loop, subtracting each script's spent
 * units before the next one runs, so a transaction whose redeemers collectively
 * exceed the budget fails). The spent budget reported per redeemer is the actual
 * charge, never this ceiling.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t PRV_MAX_CPU = (int64_t)100000000000;

/**
 * \brief The memory ceiling for the whole transaction, shared across redeemers.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t PRV_MAX_MEM = (int64_t)100000000;

/**
 * \brief The Dijkstra protocol major version, from which the ledger's scope check
 * of a script also visits the subterms of \c constr and \c case.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t PRV_DIJKSTRA_PROTOCOL_MAJOR = 12U;

/**
 * \brief The name the native evaluator reports.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const char* const PRV_EVALUATOR_NAME = "Native UPLC transaction evaluator";

/* STRUCTURES ****************************************************************/

/**
 * \brief The bound state of a native phase-2 evaluator.
 *
 * Stored as the \c context of the \ref cardano_tx_evaluator_impl_t and reached
 * by casting the \c cardano_object_t base. Holds the inputs the evaluate
 * signature does not carry: the slot config (copied by value), the ledger cost
 * models (referenced), the protocol major version and the transaction level
 * whose redeemers the evaluator returns.
 */
typedef struct native_context_t
{
    cardano_object_t      base;
    cardano_slot_config_t slot_config;
    cardano_costmdls_t*   cost_models;
    uint64_t              protocol_major;
    bool                  has_sub_tx_index;
    uint64_t              sub_tx_index;
} native_context_t;

/**
 * \brief The Plutus language version of a resolved script.
 */
typedef enum
{
  PRV_SCRIPT_V1 = 0,
  PRV_SCRIPT_V2 = 1,
  PRV_SCRIPT_V3 = 2,
  PRV_SCRIPT_V4 = 3
} script_version_t;

/**
 * \brief The parts of a transaction level its redeemers point into.
 *
 * A Dijkstra transaction runs scripts at two levels: the top-level transaction
 * and each of its sub-transactions. Each level has its own body and witness set,
 * and the redeemers of a level index into the collections of that level only.
 */
typedef struct tx_level_t
{
    cardano_witness_set_t*            witness_set;
    cardano_transaction_input_set_t*  inputs;
    cardano_multi_asset_t*            mint;
    cardano_withdrawal_map_t*         withdrawals;
    cardano_certificate_set_t*        certificates;
    cardano_voting_procedures_t*      votes;
    cardano_proposal_procedure_set_t* proposals;
    cardano_guard_set_t*              guards;
} tx_level_t;

/**
 * \brief A redeemer resolved to the script it runs.
 *
 * Holds the raw script bytes, the language of the script and, for a spend, the
 * datum of the spent output.
 */
typedef struct resolved_script_t
{
    cardano_buffer_t*      script_bytes;
    script_version_t       version;
    cardano_plutus_data_t* datum;
} resolved_script_t;

/**
 * \brief The outcome of evaluating one redeemer that the host could run.
 */
typedef enum
{
  /** \brief The script deserialised and evaluated successfully. */
  PRV_REDEEMER_SUCCEEDED = 0,
  /** \brief The script cannot be deserialised for its language at the protocol version. */
  PRV_REDEEMER_NOT_DESERIALISABLE = 1,
  /** \brief The script deserialised but failed phase-2 evaluation. */
  PRV_REDEEMER_SCRIPT_FAILED = 2
} redeemer_outcome_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Releases the evaluator context once its reference count reaches zero.
 */
static void
context_deallocate(void* object)
{
  native_context_t* ctx = (native_context_t*)object;

  if (ctx != NULL)
  {
    cardano_costmdls_unref(&ctx->cost_models);
    _cardano_free(ctx);
  }
}

/**
 * \brief Returns the payment credential of an output address, or NULL.
 *
 * Handles base, enterprise and pointer Shelley addresses; a Byron or stake
 * address has no payment credential and yields NULL. The caller owns the
 * returned credential and releases it with \ref cardano_credential_unref.
 */
static cardano_credential_t*
payment_credential(cardano_address_t* address)
{
  cardano_address_type_t type   = CARDANO_ADDRESS_TYPE_BYRON;
  cardano_credential_t*  result = NULL;

  if (address == NULL)
  {
    return NULL;
  }

  if (cardano_address_get_type(address, &type) != CARDANO_SUCCESS)
  {
    return NULL;
  }

  switch (type)
  {
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_KEY:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_KEY_STAKE_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BASE_PAYMENT_SCRIPT_STAKE_SCRIPT:
    {
      cardano_base_address_t* base = cardano_address_to_base_address(address);

      result = cardano_base_address_get_payment_credential(base);
      cardano_base_address_unref(&base);
      break;
    }
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_KEY:
    case CARDANO_ADDRESS_TYPE_ENTERPRISE_SCRIPT:
    {
      cardano_enterprise_address_t* ent = cardano_address_to_enterprise_address(address);

      result = cardano_enterprise_address_get_payment_credential(ent);
      cardano_enterprise_address_unref(&ent);
      break;
    }
    case CARDANO_ADDRESS_TYPE_POINTER_KEY:
    case CARDANO_ADDRESS_TYPE_POINTER_SCRIPT:
    {
      cardano_pointer_address_t* ptr = cardano_address_to_pointer_address(address);

      result = cardano_pointer_address_get_payment_credential(ptr);
      cardano_pointer_address_unref(&ptr);
      break;
    }
    case CARDANO_ADDRESS_TYPE_REWARD_KEY:
    case CARDANO_ADDRESS_TYPE_REWARD_SCRIPT:
    case CARDANO_ADDRESS_TYPE_BYRON:
    default:
    {
      result = NULL;
      break;
    }
  }

  return result;
}

/**
 * \brief Looks a script hash up in the plutus script sets of a witness set.
 *
 * The raw witness-set bytes (CBOR-wrapped flat) and the language version are
 * returned on a match. The caller owns \p out_bytes and releases it with
 * \ref cardano_buffer_unref.
 *
 * \return \ref CARDANO_SUCCESS on a match, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the witness set holds no plutus script with that hash, or a
 *         propagated error.
 */
static cardano_error_t
find_witness_script(
  cardano_witness_set_t*        witness_set,
  const cardano_blake2b_hash_t* script_hash,
  cardano_buffer_t**            out_bytes,
  script_version_t*             out_version)
{
  cardano_error_t result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  cardano_plutus_v1_script_set_t* v1 = cardano_witness_set_get_plutus_v1_scripts(witness_set);
  cardano_plutus_v2_script_set_t* v2 = cardano_witness_set_get_plutus_v2_scripts(witness_set);
  cardano_plutus_v3_script_set_t* v3 = cardano_witness_set_get_plutus_v3_scripts(witness_set);
  cardano_plutus_v4_script_set_t* v4 = cardano_witness_set_get_plutus_v4_scripts(witness_set);

  size_t i = 0U;

  for (i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (v1 != NULL) && (i < cardano_plutus_v1_script_set_get_length(v1)); ++i)
  {
    cardano_plutus_v1_script_t* script = NULL;

    if (cardano_plutus_v1_script_set_get(v1, i, &script) == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v1_script_get_hash(script);

      if (cardano_blake2b_hash_equals(hash, script_hash))
      {
        result       = cardano_plutus_v1_script_to_raw_bytes(script, out_bytes);
        *out_version = PRV_SCRIPT_V1;
      }

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v1_script_unref(&script);
  }

  for (i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (v2 != NULL) && (i < cardano_plutus_v2_script_set_get_length(v2)); ++i)
  {
    cardano_plutus_v2_script_t* script = NULL;

    if (cardano_plutus_v2_script_set_get(v2, i, &script) == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v2_script_get_hash(script);

      if (cardano_blake2b_hash_equals(hash, script_hash))
      {
        result       = cardano_plutus_v2_script_to_raw_bytes(script, out_bytes);
        *out_version = PRV_SCRIPT_V2;
      }

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v2_script_unref(&script);
  }

  for (i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (v3 != NULL) && (i < cardano_plutus_v3_script_set_get_length(v3)); ++i)
  {
    cardano_plutus_v3_script_t* script = NULL;

    if (cardano_plutus_v3_script_set_get(v3, i, &script) == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v3_script_get_hash(script);

      if (cardano_blake2b_hash_equals(hash, script_hash))
      {
        result       = cardano_plutus_v3_script_to_raw_bytes(script, out_bytes);
        *out_version = PRV_SCRIPT_V3;
      }

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v3_script_unref(&script);
  }

  for (i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (v4 != NULL) && (i < cardano_plutus_v4_script_set_get_length(v4)); ++i)
  {
    cardano_plutus_v4_script_t* script = NULL;

    if (cardano_plutus_v4_script_set_get(v4, i, &script) == CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_t* hash = cardano_plutus_v4_script_get_hash(script);

      if (cardano_blake2b_hash_equals(hash, script_hash))
      {
        result       = cardano_plutus_v4_script_to_raw_bytes(script, out_bytes);
        *out_version = PRV_SCRIPT_V4;
      }

      cardano_blake2b_hash_unref(&hash);
    }

    cardano_plutus_v4_script_unref(&script);
  }

  cardano_plutus_v1_script_set_unref(&v1);
  cardano_plutus_v2_script_set_unref(&v2);
  cardano_plutus_v3_script_set_unref(&v3);
  cardano_plutus_v4_script_set_unref(&v4);

  return result;
}

/**
 * \brief Returns the raw bytes and language version of a plutus reference script.
 *
 * \return \ref CARDANO_SUCCESS for a plutus script, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         for a native script, or a propagated error.
 */
static cardano_error_t
reference_script_bytes(cardano_script_t* script_ref, cardano_buffer_t** out_bytes, script_version_t* out_version)
{
  cardano_script_language_t lang   = CARDANO_SCRIPT_LANGUAGE_NATIVE;
  cardano_error_t           result = cardano_script_get_language(script_ref, &lang);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  switch (lang)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      cardano_plutus_v1_script_t* s = NULL;

      result = cardano_script_to_plutus_v1(script_ref, &s);

      if (result == CARDANO_SUCCESS)
      {
        result       = cardano_plutus_v1_script_to_raw_bytes(s, out_bytes);
        *out_version = PRV_SCRIPT_V1;
      }

      cardano_plutus_v1_script_unref(&s);
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      cardano_plutus_v2_script_t* s = NULL;

      result = cardano_script_to_plutus_v2(script_ref, &s);

      if (result == CARDANO_SUCCESS)
      {
        result       = cardano_plutus_v2_script_to_raw_bytes(s, out_bytes);
        *out_version = PRV_SCRIPT_V2;
      }

      cardano_plutus_v2_script_unref(&s);
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      cardano_plutus_v3_script_t* s = NULL;

      result = cardano_script_to_plutus_v3(script_ref, &s);

      if (result == CARDANO_SUCCESS)
      {
        result       = cardano_plutus_v3_script_to_raw_bytes(s, out_bytes);
        *out_version = PRV_SCRIPT_V3;
      }

      cardano_plutus_v3_script_unref(&s);
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V4:
    {
      cardano_plutus_v4_script_t* s = NULL;

      result = cardano_script_to_plutus_v4(script_ref, &s);

      if (result == CARDANO_SUCCESS)
      {
        result       = cardano_plutus_v4_script_to_raw_bytes(s, out_bytes);
        *out_version = PRV_SCRIPT_V4;
      }

      cardano_plutus_v4_script_unref(&s);
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_NATIVE:
    default:
    {
      break;
    }
  }

  return result;
}

/**
 * \brief Looks a script hash up in the reference scripts of the resolved outputs.
 *
 * \return \ref CARDANO_SUCCESS on a match, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if no resolved output carries a plutus reference script with that hash,
 *         or a propagated error.
 */
static cardano_error_t
find_reference_script(
  cardano_utxo_list_t*          resolved_inputs,
  const cardano_blake2b_hash_t* script_hash,
  cardano_buffer_t**            out_bytes,
  script_version_t*             out_version)
{
  cardano_error_t result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  for (size_t i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (resolved_inputs != NULL) && (i < cardano_utxo_list_get_length(resolved_inputs)); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    if (cardano_utxo_list_get(resolved_inputs, i, &utxo) == CARDANO_SUCCESS)
    {
      cardano_transaction_output_t* output     = cardano_utxo_get_output(utxo);
      cardano_script_t*             script_ref = cardano_transaction_output_get_script_ref(output);

      if (script_ref != NULL)
      {
        cardano_blake2b_hash_t* hash = cardano_script_get_hash(script_ref);

        if (cardano_blake2b_hash_equals(hash, script_hash))
        {
          result = reference_script_bytes(script_ref, out_bytes, out_version);
        }

        cardano_blake2b_hash_unref(&hash);
        cardano_script_unref(&script_ref);
      }

      cardano_transaction_output_unref(&output);
    }

    cardano_utxo_unref(&utxo);
  }

  return result;
}

/**
 * \brief Looks a script hash up in every script the transaction provides.
 *
 * Like the ledger, the scripts a Dijkstra transaction provides are the union of
 * the witness scripts of the top-level transaction and of every sub-transaction,
 * and the reference scripts of the resolved outputs; a redeemer at any level can
 * run any of them. The caller owns \p out_bytes and releases it with
 * \ref cardano_buffer_unref.
 *
 * \return \ref CARDANO_SUCCESS on a match, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if no plutus script with that hash exists, or a propagated error.
 */
static cardano_error_t
find_script_by_hash(
  cardano_transaction_t*        tx,
  cardano_utxo_list_t*          resolved_inputs,
  const cardano_blake2b_hash_t* script_hash,
  cardano_buffer_t**            out_bytes,
  script_version_t*             out_version)
{
  cardano_witness_set_t*         witness_set = cardano_transaction_get_witness_set(tx);
  cardano_transaction_body_t*    body        = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* subs        = cardano_transaction_body_get_sub_transactions(body);
  cardano_error_t                result      = find_witness_script(witness_set, script_hash, out_bytes, out_version);

  for (size_t i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (i < cardano_sub_transaction_set_get_length(subs)); ++i)
  {
    cardano_sub_transaction_t* sub = NULL;

    if (cardano_sub_transaction_set_get(subs, i, &sub) == CARDANO_SUCCESS)
    {
      cardano_witness_set_t* sub_witness_set = cardano_sub_transaction_get_witness_set(sub);

      result = find_witness_script(sub_witness_set, script_hash, out_bytes, out_version);

      cardano_witness_set_unref(&sub_witness_set);
    }

    cardano_sub_transaction_unref(&sub);
  }

  if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
  {
    result = find_reference_script(resolved_inputs, script_hash, out_bytes, out_version);
  }

  cardano_sub_transaction_set_unref(&subs);
  cardano_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);

  return result;
}

/**
 * \brief Computes the Blake2b-256 hash of a plutus-data value.
 *
 * Serializes the data to canonical CBOR and hashes it, matching the datum-hash
 * the ledger keys witness datums by. The caller owns the returned hash and
 * releases it with \ref cardano_blake2b_hash_unref.
 */
static cardano_error_t
hash_plutus_data(cardano_plutus_data_t* data, cardano_blake2b_hash_t** out_hash)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_buffer_t*      buffer = NULL;
  cardano_error_t        result = CARDANO_SUCCESS;

  *out_hash = NULL;

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_plutus_data_to_cbor(data, writer);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_cbor_writer_encode_in_buffer(writer, &buffer);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_blake2b_compute_hash(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), (size_t)CARDANO_BLAKE2B_HASH_SIZE_256, out_hash);
  }

  cardano_buffer_unref(&buffer);
  cardano_cbor_writer_unref(&writer);

  return result;
}

/**
 * \brief Resolves the datum of a spent output for the V1/V2/V3 spend purpose.
 *
 * An inline datum yields its data directly; a datum hash is resolved against the
 * witness datum set by re-hashing each witness datum and comparing. The caller
 * owns the returned datum and releases it with \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS with \p *out_datum NULL when the output carries no
 *         datum, \ref CARDANO_SUCCESS with a datum on a match,
 *         \ref CARDANO_ERROR_ELEMENT_NOT_FOUND when a hash cannot be resolved, or
 *         a propagated error.
 */
static cardano_error_t
resolve_datum(
  cardano_witness_set_t*        witness_set,
  cardano_transaction_output_t* output,
  cardano_plutus_data_t**       out_datum)
{
  cardano_datum_t* datum  = cardano_transaction_output_get_datum(output);
  cardano_error_t  result = CARDANO_SUCCESS;

  *out_datum = NULL;

  if (datum == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_datum_type_t datum_type = CARDANO_DATUM_TYPE_DATA_HASH;

  result = cardano_datum_get_type(datum, &datum_type);

  if (result == CARDANO_SUCCESS)
  {
    switch (datum_type)
    {
      case CARDANO_DATUM_TYPE_INLINE_DATA:
      {
        *out_datum = cardano_datum_get_inline_data(datum);
        break;
      }
      case CARDANO_DATUM_TYPE_DATA_HASH:
      {
        cardano_blake2b_hash_t*    target = cardano_datum_get_data_hash(datum);
        cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witness_set);

        result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

        for (size_t i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (datums != NULL) && (i < cardano_plutus_data_set_get_length(datums)); ++i)
        {
          cardano_plutus_data_t* candidate = NULL;

          if (cardano_plutus_data_set_get(datums, i, &candidate) == CARDANO_SUCCESS)
          {
            cardano_blake2b_hash_t* hash = NULL;

            if (hash_plutus_data(candidate, &hash) == CARDANO_SUCCESS)
            {
              if (cardano_blake2b_hash_equals(hash, target))
              {
                *out_datum = candidate;
                cardano_plutus_data_ref(*out_datum);
                result = CARDANO_SUCCESS;
              }

              cardano_blake2b_hash_unref(&hash);
            }
          }

          cardano_plutus_data_unref(&candidate);
        }

        cardano_blake2b_hash_unref(&target);
        cardano_plutus_data_set_unref(&datums);
        break;
      }
      default:
      {
        result = CARDANO_ERROR_INVALID_ARGUMENT;
        break;
      }
    }
  }

  cardano_datum_unref(&datum);

  return result;
}

/**
 * \brief Extracts the script hash of a script-typed credential.
 *
 * A redeemer can only authorize a credential whose payment/stake credential is a
 * script hash; a key-hash credential (or a NULL one) is rejected. The returned
 * hash is owned by the caller and released with \ref cardano_blake2b_hash_unref.
 *
 * \return \ref CARDANO_SUCCESS with the script hash, or
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT for a NULL or non-script credential.
 */
static cardano_error_t
script_hash_from_credential(cardano_credential_t* credential, cardano_blake2b_hash_t** out_hash)
{
  cardano_credential_type_t cred_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

  if (credential == NULL)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (cardano_credential_get_type(credential, &cred_type) != CARDANO_SUCCESS)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (cred_type != CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *out_hash = cardano_credential_get_hash(credential);

  return CARDANO_SUCCESS;
}

/**
 * \brief Returns the credential a certificate authorizes with a script witness.
 *
 * Mirrors the ledger's \c getScriptWitnessConwayTxCert: deregistration,
 * delegation, the registration-and-delegation combinations, the DRep
 * registration/update/unregistration and the committee hot/cold certificates are
 * authorized by their (cold) credential; pool, genesis, MIR and bare stake
 * registration carry no script witness. The Conway registration-with-deposit
 * certificate is authorized by its stake credential. The returned credential is
 * owned by the caller and released with \ref cardano_credential_unref.
 *
 * \return \ref CARDANO_SUCCESS with the credential, \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         for a certificate that takes no script witness, or a propagated error.
 */
static cardano_error_t
certificate_witness_credential(cardano_certificate_t* certificate, cardano_credential_t** out)
{
  cardano_cert_type_t type   = CARDANO_CERT_TYPE_STAKE_REGISTRATION;
  cardano_error_t     result = cardano_cert_get_type(certificate, &type);

  *out = NULL;

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (type)
  {
    case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
    {
      cardano_stake_deregistration_cert_t* cert = NULL;
      result                                    = cardano_certificate_to_stake_deregistration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_stake_deregistration_cert_get_credential(cert);
      }

      cardano_stake_deregistration_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_DELEGATION:
    {
      cardano_stake_delegation_cert_t* cert = NULL;
      result                                = cardano_certificate_to_stake_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_stake_delegation_cert_get_credential(cert);
      }

      cardano_stake_delegation_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_REGISTRATION:
    {
      cardano_registration_cert_t* cert = NULL;
      result                            = cardano_certificate_to_registration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_registration_cert_get_stake_credential(cert);
      }

      cardano_registration_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_UNREGISTRATION:
    {
      cardano_unregistration_cert_t* cert = NULL;
      result                              = cardano_certificate_to_unregistration(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_unregistration_cert_get_credential(cert);
      }

      cardano_unregistration_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_VOTE_DELEGATION:
    {
      cardano_vote_delegation_cert_t* cert = NULL;
      result                               = cardano_certificate_to_vote_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_vote_delegation_cert_get_credential(cert);
      }

      cardano_vote_delegation_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
    {
      cardano_stake_vote_delegation_cert_t* cert = NULL;
      result                                     = cardano_certificate_to_stake_vote_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_stake_vote_delegation_cert_get_credential(cert);
      }

      cardano_stake_vote_delegation_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
    {
      cardano_stake_registration_delegation_cert_t* cert = NULL;
      result                                             = cardano_certificate_to_stake_registration_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_stake_registration_delegation_cert_get_credential(cert);
      }

      cardano_stake_registration_delegation_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_vote_registration_delegation_cert_t* cert = NULL;
      result                                            = cardano_certificate_to_vote_registration_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_vote_registration_delegation_cert_get_credential(cert);
      }

      cardano_vote_registration_delegation_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
    {
      cardano_stake_vote_registration_delegation_cert_t* cert = NULL;
      result                                                  = cardano_certificate_to_stake_vote_registration_delegation(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_stake_vote_registration_delegation_cert_get_credential(cert);
      }

      cardano_stake_vote_registration_delegation_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT:
    {
      cardano_auth_committee_hot_cert_t* cert = NULL;
      result                                  = cardano_certificate_to_auth_committee_hot(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_auth_committee_hot_cert_get_cold_cred(cert, out);
      }

      cardano_auth_committee_hot_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
    {
      cardano_resign_committee_cold_cert_t* cert = NULL;
      result                                     = cardano_certificate_to_resign_committee_cold(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_resign_committee_cold_cert_get_credential(cert);
      }

      cardano_resign_committee_cold_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_DREP_REGISTRATION:
    {
      cardano_register_drep_cert_t* cert = NULL;
      result                             = cardano_certificate_to_register_drep(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_register_drep_cert_get_credential(cert);
      }

      cardano_register_drep_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
    {
      cardano_unregister_drep_cert_t* cert = NULL;
      result                               = cardano_certificate_to_unregister_drep(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_unregister_drep_cert_get_credential(cert);
      }

      cardano_unregister_drep_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_UPDATE_DREP:
    {
      cardano_update_drep_cert_t* cert = NULL;
      result                           = cardano_certificate_to_update_drep(certificate, &cert);

      if (result == CARDANO_SUCCESS)
      {
        *out = cardano_update_drep_cert_get_credential(cert);
      }

      cardano_update_drep_cert_unref(&cert);
      break;
    }
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
    case CARDANO_CERT_TYPE_POOL_REGISTRATION:
    case CARDANO_CERT_TYPE_POOL_RETIREMENT:
    case CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION:
    case CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS:
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/**
 * \brief Returns the guardrail policy script hash a proposal authorizes.
 *
 * Mirrors the ledger's \c getProposalScriptHash: only a parameter-change or a
 * treasury-withdrawals action carries a guardrail policy; any other action, or a
 * proposal without a policy, takes no script witness. The returned hash is owned
 * by the caller and released with \ref cardano_blake2b_hash_unref.
 *
 * \return \ref CARDANO_SUCCESS with the policy hash, \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         when the proposal carries no guardrail policy, or a propagated error.
 */
static cardano_error_t
proposal_policy_hash(cardano_proposal_procedure_t* proposal, cardano_blake2b_hash_t** out_hash)
{
  cardano_governance_action_type_t action_type = CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE;
  cardano_error_t                  result      = cardano_proposal_procedure_get_action_type(proposal, &action_type);

  *out_hash = NULL;

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (action_type)
  {
    case CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE:
    {
      cardano_parameter_change_action_t* action = NULL;
      result                                    = cardano_proposal_procedure_to_parameter_change_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        *out_hash = cardano_parameter_change_action_get_policy_hash(action);
      }

      cardano_parameter_change_action_unref(&action);
      break;
    }
    case CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS:
    {
      cardano_treasury_withdrawals_action_t* action = NULL;
      result                                        = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal, &action);

      if (result == CARDANO_SUCCESS)
      {
        *out_hash = cardano_treasury_withdrawals_action_get_policy_hash(action);
      }

      cardano_treasury_withdrawals_action_unref(&action);
      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  if ((result == CARDANO_SUCCESS) && (*out_hash == NULL))
  {
    result = CARDANO_ERROR_INVALID_ARGUMENT;
  }

  return result;
}

/**
 * \brief Releases the references held by a transaction level.
 */
static void
release_level(tx_level_t* level)
{
  cardano_witness_set_unref(&level->witness_set);
  cardano_transaction_input_set_unref(&level->inputs);
  cardano_multi_asset_unref(&level->mint);
  cardano_withdrawal_map_unref(&level->withdrawals);
  cardano_certificate_set_unref(&level->certificates);
  cardano_voting_procedures_unref(&level->votes);
  cardano_proposal_procedure_set_unref(&level->proposals);
  cardano_guard_set_unref(&level->guards);
}

/**
 * \brief Reads the parts of a transaction level its redeemers point into.
 *
 * \param[in] tx The top-level transaction.
 * \param[in] sub_tx_index The position of the sub-transaction in the body of \p tx,
 *            or NULL for the top-level transaction.
 * \param[out] level The level to fill; release it with \ref release_level.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_INDEX_OUT_OF_BOUNDS
 *         when \p sub_tx_index points past the sub-transactions.
 */
static cardano_error_t
read_level(cardano_transaction_t* tx, const uint64_t* sub_tx_index, tx_level_t* level)
{
  cardano_transaction_body_t*    body   = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* subs   = NULL;
  cardano_sub_transaction_t*     sub    = NULL;
  cardano_error_t                result = CARDANO_SUCCESS;

  CARDANO_UNUSED(memset(level, 0, sizeof(tx_level_t)));

  if (sub_tx_index == NULL)
  {
    level->witness_set  = cardano_transaction_get_witness_set(tx);
    level->inputs       = cardano_transaction_body_get_inputs(body);
    level->mint         = cardano_transaction_body_get_mint(body);
    level->withdrawals  = cardano_transaction_body_get_withdrawals(body);
    level->certificates = cardano_transaction_body_get_certificates(body);
    level->votes        = cardano_transaction_body_get_voting_procedures(body);
    level->proposals    = cardano_transaction_body_get_proposal_procedures(body);
    level->guards       = cardano_transaction_body_get_guards(body);

    cardano_transaction_body_unref(&body);

    return CARDANO_SUCCESS;
  }

  subs = cardano_transaction_body_get_sub_transactions(body);

  if (*sub_tx_index >= (uint64_t)cardano_sub_transaction_set_get_length(subs))
  {
    result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_sub_transaction_set_get(subs, (size_t)*sub_tx_index, &sub);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_sub_transaction_body_t* sub_body = cardano_sub_transaction_get_body(sub);

    level->witness_set  = cardano_sub_transaction_get_witness_set(sub);
    level->inputs       = cardano_sub_transaction_body_get_inputs(sub_body);
    level->mint         = cardano_sub_transaction_body_get_mint(sub_body);
    level->withdrawals  = cardano_sub_transaction_body_get_withdrawals(sub_body);
    level->certificates = cardano_sub_transaction_body_get_certificates(sub_body);
    level->votes        = cardano_sub_transaction_body_get_voting_procedures(sub_body);
    level->proposals    = cardano_sub_transaction_body_get_proposal_procedures(sub_body);
    level->guards       = cardano_sub_transaction_body_get_guards(sub_body);

    cardano_sub_transaction_body_unref(&sub_body);
  }

  cardano_sub_transaction_unref(&sub);
  cardano_sub_transaction_set_unref(&subs);
  cardano_transaction_body_unref(&body);

  return result;
}

/**
 * \brief Resolves the script hash a redeemer points at, plus the spend datum.
 *
 * The (tag, index) pair selects the concrete item against the collections of the
 * redeemer's own transaction level: sorted inputs (spend), sorted mint policies
 * (mint), withdrawals in ledger account order (reward), certificates (cert),
 * voters in ledger voter order (vote), proposals (propose) or guards (guard).
 * The guard index is the position in the guard set in transaction order,
 * counting key-hash guards too. The script hash is the script credential the
 * item is authorized by; the spend purpose also resolves the datum from the
 * witness datums of the level. The index ordering mirrors the ledger's
 * per-purpose ordering, which is the consensus-critical mapping.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_INDEX_OUT_OF_BOUNDS
 *         if the index is out of range, \ref CARDANO_ERROR_INVALID_ARGUMENT for a
 *         non-script credential or an unsupported tag, or a propagated error.
 */
static cardano_error_t
resolve_script_hash(
  const tx_level_t*        level,
  cardano_utxo_list_t*     resolved_inputs,
  cardano_redeemer_t*      redeemer,
  cardano_blake2b_hash_t** out_hash,
  cardano_plutus_data_t**  out_datum)
{
  const cardano_redeemer_tag_t tag    = cardano_redeemer_get_tag(redeemer);
  const uint64_t               index  = cardano_redeemer_get_index(redeemer);
  cardano_error_t              result = CARDANO_SUCCESS;

  *out_hash  = NULL;
  *out_datum = NULL;

  switch (tag)
  {
    case CARDANO_REDEEMER_TAG_MINT:
    {
      cardano_policy_id_list_t* policies = NULL;

      result = (level->mint != NULL) ? cardano_multi_asset_get_keys(level->mint, &policies) : CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;

      if ((result == CARDANO_SUCCESS) && (index >= cardano_policy_id_list_get_length(policies)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_policy_id_list_get(policies, (size_t)index, out_hash);
      }

      cardano_policy_id_list_unref(&policies);
      break;
    }
    case CARDANO_REDEEMER_TAG_SPEND:
    {
      if ((level->inputs == NULL) || (index >= cardano_transaction_input_set_get_length(level->inputs)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_transaction_input_t*  input  = NULL;
        cardano_transaction_output_t* output = NULL;

        result = cardano_transaction_input_set_get(level->inputs, (size_t)index, &input);

        if (result == CARDANO_SUCCESS)
        {
          size_t j = 0U;

          result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

          for (j = 0U; (output == NULL) && (j < cardano_utxo_list_get_length(resolved_inputs)); ++j)
          {
            cardano_utxo_t* utxo = NULL;

            if (cardano_utxo_list_get(resolved_inputs, j, &utxo) == CARDANO_SUCCESS)
            {
              cardano_transaction_input_t* candidate = cardano_utxo_get_input(utxo);

              if (cardano_transaction_input_equals(candidate, input))
              {
                output = cardano_utxo_get_output(utxo);
                result = CARDANO_SUCCESS;
              }

              cardano_transaction_input_unref(&candidate);
            }

            cardano_utxo_unref(&utxo);
          }
        }

        if (result == CARDANO_SUCCESS)
        {
          cardano_address_t*    address    = cardano_transaction_output_get_address(output);
          cardano_credential_t* credential = payment_credential(address);

          result = script_hash_from_credential(credential, out_hash);

          if (result == CARDANO_SUCCESS)
          {
            result = resolve_datum(level->witness_set, output, out_datum);
          }

          cardano_credential_unref(&credential);
          cardano_address_unref(&address);
        }

        cardano_transaction_output_unref(&output);
        cardano_transaction_input_unref(&input);
      }

      break;
    }
    case CARDANO_REDEEMER_TAG_REWARD:
    {
      cardano_reward_address_t* address = NULL;

      result = _cardano_ledger_order_withdrawal_at_index(level->withdrawals, index, &address);

      if (result == CARDANO_SUCCESS)
      {
        cardano_credential_t* credential = cardano_reward_address_get_credential(address);

        result = script_hash_from_credential(credential, out_hash);

        cardano_credential_unref(&credential);
      }

      cardano_reward_address_unref(&address);

      break;
    }
    case CARDANO_REDEEMER_TAG_CERTIFYING:
    {
      if ((level->certificates == NULL) || (index >= cardano_certificate_set_get_length(level->certificates)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_certificate_t* certificate = NULL;

        result = cardano_certificate_set_get(level->certificates, (size_t)index, &certificate);

        if (result == CARDANO_SUCCESS)
        {
          cardano_credential_t* credential = NULL;

          result = certificate_witness_credential(certificate, &credential);

          if (result == CARDANO_SUCCESS)
          {
            result = script_hash_from_credential(credential, out_hash);
          }

          cardano_credential_unref(&credential);
        }

        cardano_certificate_unref(&certificate);
      }

      break;
    }
    case CARDANO_REDEEMER_TAG_VOTING:
    {
      cardano_voter_t* voter = NULL;

      result = _cardano_ledger_order_voter_at_index(level->votes, index, &voter);

      if (result == CARDANO_SUCCESS)
      {
        cardano_credential_t* credential = cardano_voter_get_credential(voter);

        result = script_hash_from_credential(credential, out_hash);

        cardano_credential_unref(&credential);
      }

      cardano_voter_unref(&voter);

      break;
    }
    case CARDANO_REDEEMER_TAG_PROPOSING:
    {
      if ((level->proposals == NULL) || (index >= cardano_proposal_procedure_set_get_length(level->proposals)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_proposal_procedure_t* proposal = NULL;

        result = cardano_proposal_procedure_set_get(level->proposals, (size_t)index, &proposal);

        if (result == CARDANO_SUCCESS)
        {
          result = proposal_policy_hash(proposal, out_hash);
        }

        cardano_proposal_procedure_unref(&proposal);
      }

      break;
    }
    case CARDANO_REDEEMER_TAG_GUARDING:
    {
      if ((level->guards == NULL) || (index >= cardano_guard_set_get_length(level->guards)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_credential_t* credential = NULL;

        result = cardano_guard_set_get(level->guards, (size_t)index, &credential);

        if (result == CARDANO_SUCCESS)
        {
          result = script_hash_from_credential(credential, out_hash);
        }

        cardano_credential_unref(&credential);
      }

      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(out_hash);
    cardano_plutus_data_unref(out_datum);
    *out_hash  = NULL;
    *out_datum = NULL;
  }

  return result;
}

/**
 * \brief Maps a resolved script version to its ledger Plutus language version.
 */
static cardano_plutus_language_version_t
plutus_language(script_version_t version)
{
  cardano_plutus_language_version_t result = CARDANO_PLUTUS_LANGUAGE_VERSION_V3;

  switch (version)
  {
    case PRV_SCRIPT_V1:
    {
      result = CARDANO_PLUTUS_LANGUAGE_VERSION_V1;
      break;
    }
    case PRV_SCRIPT_V2:
    {
      result = CARDANO_PLUTUS_LANGUAGE_VERSION_V2;
      break;
    }
    case PRV_SCRIPT_V4:
    {
      result = CARDANO_PLUTUS_LANGUAGE_VERSION_V4;
      break;
    }
    case PRV_SCRIPT_V3:
    default:
    {
      result = CARDANO_PLUTUS_LANGUAGE_VERSION_V3;
      break;
    }
  }

  return result;
}

/**
 * \brief Maps a resolved script version to the UPLC cost-model language version.
 */
static cardano_uplc_lang_version_t
uplc_lang_version(script_version_t version)
{
  cardano_uplc_lang_version_t result = CARDANO_UPLC_LANG_VERSION_V3;

  switch (version)
  {
    case PRV_SCRIPT_V1:
    {
      result = CARDANO_UPLC_LANG_VERSION_V1;
      break;
    }
    case PRV_SCRIPT_V2:
    {
      result = CARDANO_UPLC_LANG_VERSION_V2;
      break;
    }
    case PRV_SCRIPT_V4:
    {
      result = CARDANO_UPLC_LANG_VERSION_V4;
      break;
    }
    case PRV_SCRIPT_V3:
    default:
    {
      result = CARDANO_UPLC_LANG_VERSION_V3;
      break;
    }
  }

  return result;
}

/**
 * \brief Selects the cost model and semantics for a script from the ledger costmdls.
 *
 * Reads the ordered cost vector the ledger keyed under the script's Plutus
 * language version and selects the structured cost model and builtin semantics
 * for that language and the protocol major version
 * (\ref cardano_uplc_select_cost_model). When no cost model exists for the
 * language the version default is returned, reshaped for the semantics variant
 * of the protocol version, which keeps an under-specified host usable.
 * Consensus-critical: the spent ex-units track the cost model the transaction
 * was submitted under, not a hardcoded default.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated error.
 */
static cardano_error_t
select_cost_model(
  native_context_t*                   ctx,
  script_version_t                    version,
  cardano_uplc_selected_cost_model_t* out)
{
  cardano_cost_model_t* cost_model = NULL;
  cardano_error_t       result     = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  if (ctx->cost_models != NULL)
  {
    result = cardano_costmdls_get(ctx->cost_models, plutus_language(version), &cost_model);
  }

  if ((result == CARDANO_SUCCESS) && (cost_model == NULL))
  {
    result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (result == CARDANO_SUCCESS)
  {
    const int64_t* params = cardano_cost_model_get_costs(cost_model);
    size_t         count  = cardano_cost_model_get_costs_size(cost_model);

    result = cardano_uplc_select_cost_model(
      uplc_lang_version(version),
      ctx->protocol_major,
      params,
      count,
      out);

    cardano_cost_model_unref(&cost_model);
  }
  else
  {
    cardano_uplc_cost_model_version_t cm_version = cardano_uplc_cost_model_version_for_language(uplc_lang_version(version));

    out->model.machine = cardano_uplc_machine_costs_default(cm_version);

    switch (cm_version)
    {
      case CARDANO_UPLC_COST_MODEL_VERSION_V1:
      {
        out->model.builtins = cardano_uplc_builtin_costs_v1();
        break;
      }
      case CARDANO_UPLC_COST_MODEL_VERSION_V2:
      {
        out->model.builtins = cardano_uplc_builtin_costs_v2();
        break;
      }
      case CARDANO_UPLC_COST_MODEL_VERSION_V3:
      case CARDANO_UPLC_COST_MODEL_VERSION_V4:
      default:
      {
        out->model.builtins = cardano_uplc_builtin_costs_v3();
        break;
      }
    }

    out->semantics = cardano_uplc_builtin_semantics_for_language_and_protocol(uplc_lang_version(version), ctx->protocol_major);
    result         = CARDANO_SUCCESS;

    cardano_uplc_builtin_costs_select_division_shapes(&out->model.builtins, out->semantics);
  }

  return result;
}

/**
 * \brief Builds the version-appropriate ScriptContext for a redeemer.
 *
 * Dispatches to the V1, V2, V3 or V4 builder. The V3 and V4 builders embed the
 * redeemer data and the optional datum inside the context, the V1/V2 builders
 * carry only the TxInfo and the script purpose. The V4 context is the one of the
 * transaction level the redeemer belongs to.
 */
static cardano_error_t
build_script_context(
  script_version_t             version,
  cardano_transaction_t*       tx,
  const uint64_t*              sub_tx_index,
  cardano_utxo_list_t*         resolved_inputs,
  const cardano_slot_config_t* slot_config,
  cardano_redeemer_t*          redeemer,
  cardano_plutus_data_t*       datum,
  cardano_plutus_data_t**      script_context)
{
  cardano_error_t result = CARDANO_ERROR_INVALID_ARGUMENT;

  switch (version)
  {
    case PRV_SCRIPT_V1:
    {
      result = cardano_uplc_int_build_script_context_v1(tx, resolved_inputs, slot_config, redeemer, script_context);
      break;
    }
    case PRV_SCRIPT_V2:
    {
      result = cardano_uplc_int_build_script_context_v2(tx, resolved_inputs, slot_config, redeemer, script_context);
      break;
    }
    case PRV_SCRIPT_V3:
    {
      result = cardano_uplc_int_build_script_context_v3(tx, resolved_inputs, slot_config, redeemer, datum, script_context);
      break;
    }
    case PRV_SCRIPT_V4:
    default:
    {
      result = cardano_uplc_int_build_script_context_v4(tx, sub_tx_index, resolved_inputs, slot_config, redeemer, datum, script_context);
      break;
    }
  }

  return result;
}

/**
 * \brief Applies the version-appropriate arguments to a decoded program.
 *
 * V1/V2 spend applies [datum, redeemer, context]; V1/V2 non-spend applies
 * [redeemer, context]; V3 and V4 apply [context] only, since their context
 * already carries the redeemer and the datum.
 */
static cardano_error_t
apply_arguments(
  cardano_uplc_arena_t*         arena,
  script_version_t              version,
  const cardano_uplc_program_t* program,
  cardano_redeemer_t*           redeemer,
  cardano_plutus_data_t*        datum,
  cardano_plutus_data_t*        script_context,
  cardano_uplc_program_t**      out)
{
  cardano_error_t result = CARDANO_SUCCESS;

  if ((version == PRV_SCRIPT_V3) || (version == PRV_SCRIPT_V4))
  {
    result = cardano_uplc_program_apply_data(arena, program, script_context, out);
  }
  else
  {
    cardano_plutus_data_t* redeemer_data = cardano_redeemer_get_data(redeemer);
    cardano_plutus_data_t* args[3]       = { NULL, NULL, NULL };
    size_t                 count         = 0U;

    if (redeemer_data == NULL)
    {
      return CARDANO_ERROR_POINTER_IS_NULL;
    }

    if (datum != NULL)
    {
      args[count] = datum;
      ++count;
    }

    args[count] = redeemer_data;
    ++count;
    args[count] = script_context;
    ++count;

    result = cardano_uplc_program_apply_data_params(arena, program, args, count, out);

    cardano_plutus_data_unref(&redeemer_data);
  }

  return result;
}

/**
 * \brief Deserialises a script the way the ledger does before evaluating it.
 *
 * Decodes the script for its language at \p protocol_major
 * (\ref cardano_uplc_program_from_ledger_script): the language and every builtin
 * it names must be available there. The Plutus Core version of the program is
 * not checked here; the ledger checks it when evaluating the script, so an
 * unavailable one is a phase-2 failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING when the
 *         script cannot be deserialised, or a propagated host error.
 */
static cardano_error_t
deserialise_script(
  cardano_uplc_arena_t*          arena,
  script_version_t               version,
  uint64_t                       protocol_major,
  const cardano_buffer_t*        script_bytes,
  const cardano_uplc_program_t** program)
{
  return cardano_uplc_program_from_ledger_script(
    arena,
    cardano_buffer_get_data(script_bytes),
    cardano_buffer_get_size(script_bytes),
    uplc_lang_version(version),
    protocol_major,
    program);
}

/**
 * \brief Returns whether a script is closed, as the ledger checks before evaluating it.
 *
 * Below the Dijkstra protocol version the ledger's scope check does not visit the
 * subterms of \c constr and \c case; from Dijkstra on it visits every subterm. A
 * script that is not closed fails phase-2 validation without being evaluated.
 *
 * \param[in] program The deserialised script.
 * \param[in] protocol_major The protocol major version.
 *
 * \return \c true when the script passes the scope check, \c false otherwise.
 */
static bool
script_is_closed(const cardano_uplc_program_t* program, const uint64_t protocol_major)
{
  const cardano_uplc_scope_check_mode_t mode = (protocol_major < PRV_DIJKSTRA_PROTOCOL_MAJOR)
    ? CARDANO_UPLC_SCOPE_CHECK_NO_CASE_CONSTR
    : CARDANO_UPLC_SCOPE_CHECK_FULL;

  return cardano_uplc_term_is_closed(program->term, mode);
}

/**
 * \brief Returns whether the result of a script is accepted.
 *
 * A Plutus V3 or V4 script only succeeds when it evaluates to the unit constant;
 * any other result fails phase-2 validation, as the ledger's
 * \c processLogsAndErrors reports \c InvalidReturnValue. V1 and V2 results are
 * accepted as they are.
 *
 * \param[in] version The language version of the script.
 * \param[in] result The discharged result term of the evaluation.
 *
 * \return \c true when the result is accepted, \c false otherwise.
 */
static bool
result_is_accepted(const script_version_t version, const cardano_uplc_term_t* result)
{
  if ((version == PRV_SCRIPT_V1) || (version == PRV_SCRIPT_V2))
  {
    return true;
  }

  return (result->kind == CARDANO_UPLC_TERM_CONSTANT) && (result->as.constant->kind == CARDANO_UPLC_TYPE_UNIT);
}

/**
 * \brief Releases the references held by a resolved script.
 */
static void
release_resolved_script(resolved_script_t* script)
{
  cardano_buffer_unref(&script->script_bytes);
  cardano_plutus_data_unref(&script->datum);
}

/**
 * \brief Resolves the script a redeemer of a transaction level runs.
 *
 * Plutus V1, V2 and V3 scripts only run at the top level: like the ledger, a
 * sub-transaction redeemer that resolves to one of them is rejected before any
 * script runs.
 *
 * \param[in] tx The top-level transaction.
 * \param[in] sub_tx_index The position of the redeemer's sub-transaction, or NULL
 *            for the top level.
 * \param[in] level The transaction level of the redeemer.
 * \param[in] resolved_inputs The UTxO set resolving the inputs of every level.
 * \param[in] redeemer The redeemer to resolve.
 * \param[out] script The resolved script; release it with \ref release_resolved_script.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND when
 *         the script is not provided or a V1/V2 spend has no datum,
 *         \ref CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE for a V1, V2 or V3 script in a
 *         sub-transaction, or a propagated error.
 */
static cardano_error_t
resolve_script(
  cardano_transaction_t* tx,
  const uint64_t*        sub_tx_index,
  const tx_level_t*      level,
  cardano_utxo_list_t*   resolved_inputs,
  cardano_redeemer_t*    redeemer,
  resolved_script_t*     script)
{
  cardano_blake2b_hash_t* script_hash = NULL;
  cardano_error_t         result      = CARDANO_SUCCESS;

  script->script_bytes = NULL;
  script->version      = PRV_SCRIPT_V3;
  script->datum        = NULL;

  result = resolve_script_hash(level, resolved_inputs, redeemer, &script_hash, &script->datum);

  if (result == CARDANO_SUCCESS)
  {
    result = find_script_by_hash(tx, resolved_inputs, script_hash, &script->script_bytes, &script->version);
  }

  if (result == CARDANO_SUCCESS)
  {
    if ((script->datum == NULL) && ((script->version == PRV_SCRIPT_V1) || (script->version == PRV_SCRIPT_V2)) && (cardano_redeemer_get_tag(redeemer) == CARDANO_REDEEMER_TAG_SPEND))
    {
      result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }
  }

  if ((result == CARDANO_SUCCESS) && (sub_tx_index != NULL) && (script->version != PRV_SCRIPT_V4))
  {
    result = CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
  }

  cardano_blake2b_hash_unref(&script_hash);

  return result;
}

/**
 * \brief Checks that the script of a redeemer can be deserialised.
 *
 * The ledger deserialises every script a transaction runs, at every level,
 * before it evaluates any of them, so a script that cannot be deserialised is
 * reported even when another script would fail phase-2 validation. A script
 * that cannot be deserialised for its language at the protocol version is
 * reported through \p out_outcome, not as a host error.
 *
 * \return \ref CARDANO_SUCCESS when the host checked the redeemer (whatever the
 *         outcome), or a \ref cardano_error_t when the host could not check it.
 */
static cardano_error_t
check_redeemer(
  native_context_t*      ctx,
  cardano_transaction_t* tx,
  const uint64_t*        sub_tx_index,
  const tx_level_t*      level,
  cardano_utxo_list_t*   resolved_inputs,
  cardano_redeemer_t*    redeemer,
  redeemer_outcome_t*    out_outcome)
{
  resolved_script_t             script;
  cardano_uplc_arena_t*         arena   = NULL;
  const cardano_uplc_program_t* program = NULL;
  cardano_error_t               result  = resolve_script(tx, sub_tx_index, level, resolved_inputs, redeemer, &script);

  *out_outcome = PRV_REDEEMER_SUCCEEDED;

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_arena_new(PRV_ARENA_BLOCK_SIZE, &arena);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = deserialise_script(arena, script.version, ctx->protocol_major, script.script_bytes, &program);

    if (result == CARDANO_ERROR_DECODING)
    {
      *out_outcome = PRV_REDEEMER_NOT_DESERIALISABLE;
      result       = CARDANO_SUCCESS;
    }
  }

  cardano_uplc_arena_free(&arena);
  release_resolved_script(&script);

  return result;
}

/**
 * \brief Evaluates one redeemer and writes its consumed ex-units into a new redeemer.
 *
 * The whole evaluation runs inside a per-redeemer arena that is freed on every
 * path. A script that fails (Plutus Core version not available at the protocol
 * version, a free variable, error term, out of budget, unsupported builtin, or a
 * V3 or V4 script that does not return unit) is reported through \p out_outcome,
 * not as a host error.
 *
 * The CEK machine is bounded by \p remaining, the budget left for the whole
 * transaction; on a successful evaluation the script's spent units are deducted
 * from it so the next redeemer sees a smaller ceiling, matching the reference.
 *
 * \return \ref CARDANO_SUCCESS when the host ran the redeemer (whatever the script
 *         outcome), or a \ref cardano_error_t when the host could not run it.
 */
static cardano_error_t
eval_redeemer(
  native_context_t*      ctx,
  cardano_transaction_t* tx,
  const uint64_t*        sub_tx_index,
  const tx_level_t*      level,
  cardano_utxo_list_t*   resolved_inputs,
  cardano_redeemer_t*    redeemer,
  cardano_uplc_budget_t* remaining,
  cardano_redeemer_t**   out_redeemer,
  redeemer_outcome_t*    out_outcome)
{
  resolved_script_t             script;
  cardano_plutus_data_t*        script_context = NULL;
  cardano_uplc_arena_t*         arena          = NULL;
  const cardano_uplc_program_t* program        = NULL;
  cardano_uplc_program_t*       applied        = NULL;
  cardano_ex_units_t*           ex_units       = NULL;
  cardano_uplc_eval_result_t    eval_result;
  cardano_error_t               result = CARDANO_SUCCESS;

  *out_redeemer = NULL;
  *out_outcome  = PRV_REDEEMER_SUCCEEDED;

  result = resolve_script(tx, sub_tx_index, level, resolved_inputs, redeemer, &script);

  if (result == CARDANO_SUCCESS)
  {
    result = build_script_context(script.version, tx, sub_tx_index, resolved_inputs, &ctx->slot_config, redeemer, script.datum, &script_context);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_arena_new(PRV_ARENA_BLOCK_SIZE, &arena);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = deserialise_script(arena, script.version, ctx->protocol_major, script.script_bytes, &program);
  }

  if (result == CARDANO_SUCCESS)
  {
    if (!cardano_uplc_plc_version_available(uplc_lang_version(script.version), ctx->protocol_major, program->version_major, program->version_minor, program->version_patch))
    {
      *out_outcome = PRV_REDEEMER_SCRIPT_FAILED;
    }
  }

  if ((result == CARDANO_SUCCESS) && (*out_outcome == PRV_REDEEMER_SUCCEEDED))
  {
    if (!script_is_closed(program, ctx->protocol_major))
    {
      *out_outcome = PRV_REDEEMER_SCRIPT_FAILED;
    }
  }

  if ((result == CARDANO_SUCCESS) && (*out_outcome == PRV_REDEEMER_SUCCEEDED))
  {
    result = apply_arguments(arena, script.version, program, redeemer, script.datum, script_context, &applied);
  }

  if ((result == CARDANO_SUCCESS) && (*out_outcome == PRV_REDEEMER_SUCCEEDED))
  {
    cardano_uplc_selected_cost_model_t selected;

    result = select_cost_model(ctx, script.version, &selected);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_int_evaluate_with_costs(arena, applied, &selected.model, selected.semantics, ctx->protocol_major, *remaining, &eval_result);
    }
  }

  if ((result == CARDANO_SUCCESS) && (*out_outcome == PRV_REDEEMER_SUCCEEDED))
  {
    if ((eval_result.status == CARDANO_UPLC_EVAL_SUCCESS) && result_is_accepted(script.version, eval_result.result))
    {
      remaining->cpu -= eval_result.spent.cpu;
      remaining->mem -= eval_result.spent.mem;

      result = cardano_ex_units_new((uint64_t)eval_result.spent.mem, (uint64_t)eval_result.spent.cpu, &ex_units);

      if (result == CARDANO_SUCCESS)
      {
        cardano_plutus_data_t* redeemer_data = cardano_redeemer_get_data(redeemer);

        result = cardano_redeemer_new(
          cardano_redeemer_get_tag(redeemer),
          cardano_redeemer_get_index(redeemer),
          redeemer_data,
          ex_units,
          out_redeemer);

        cardano_plutus_data_unref(&redeemer_data);
      }
    }
    else
    {
      *out_outcome = PRV_REDEEMER_SCRIPT_FAILED;
    }
  }

  cardano_ex_units_unref(&ex_units);
  cardano_uplc_arena_free(&arena);
  cardano_plutus_data_unref(&script_context);
  release_resolved_script(&script);

  return result;
}

/**
 * \brief Stores a failure message in the evaluator, truncated to fit.
 *
 * \param[in,out] impl The evaluator implementation receiving the message.
 * \param[in] message The NUL-terminated message.
 */
static void
set_error_message(cardano_tx_evaluator_impl_t* impl, const char* message)
{
  const size_t length = cardano_safe_strlen(message, sizeof(impl->error_message) - 1U);

  cardano_safe_memcpy(impl->error_message, sizeof(impl->error_message), message, length);
  impl->error_message[length] = '\0';
}

/**
 * \brief Maps a level number to the sub-transaction index the script context builders take.
 *
 * Level 0 is the top-level transaction, level \c n is the sub-transaction at
 * position \c n - 1.
 *
 * \param[in] level The level number.
 * \param[out] index The storage for the sub-transaction index.
 *
 * \return NULL for the top level, \p index holding the position otherwise.
 */
static const uint64_t*
level_sub_tx_index(const size_t level, uint64_t* index)
{
  if (level == 0U)
  {
    return NULL;
  }

  *index = (uint64_t)level - 1U;

  return index;
}

/**
 * \brief Checks or evaluates every redeemer of a transaction level.
 *
 * When \p remaining is NULL the script of every redeemer is checked for
 * deserialisation only. Otherwise every redeemer is evaluated against the shared
 * remaining budget and, when \p out_list is not NULL, the redeemers carrying the
 * consumed execution units are appended to it in witness set order.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING when a
 *         script cannot be deserialised, \ref CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE
 *         when a script fails phase-2 validation, or a propagated host error.
 */
static cardano_error_t
run_level(
  cardano_tx_evaluator_impl_t* impl,
  cardano_transaction_t*       tx,
  const uint64_t*              sub_tx_index,
  cardano_utxo_list_t*         resolved_inputs,
  cardano_uplc_budget_t*       remaining,
  cardano_redeemer_list_t*     out_list)
{
  native_context_t*        ctx       = (native_context_t*)((void*)impl->context);
  cardano_redeemer_list_t* redeemers = NULL;
  tx_level_t               level;
  cardano_error_t          result = read_level(tx, sub_tx_index, &level);

  if (result == CARDANO_SUCCESS)
  {
    redeemers = cardano_witness_set_get_redeemers(level.witness_set);
  }

  for (size_t i = 0U; (result == CARDANO_SUCCESS) && (i < cardano_redeemer_list_get_length(redeemers)); ++i)
  {
    cardano_redeemer_t* redeemer     = NULL;
    cardano_redeemer_t* new_redeemer = NULL;
    redeemer_outcome_t  outcome      = PRV_REDEEMER_SUCCEEDED;

    result = cardano_redeemer_list_get(redeemers, i, &redeemer);

    if (result == CARDANO_SUCCESS)
    {
      result = (remaining == NULL)
        ? check_redeemer(ctx, tx, sub_tx_index, &level, resolved_inputs, redeemer, &outcome)
        : eval_redeemer(ctx, tx, sub_tx_index, &level, resolved_inputs, redeemer, remaining, &new_redeemer, &outcome);
    }

    if (result == CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE)
    {
      set_error_message(impl, "a sub-transaction redeemer runs a Plutus V1, V2 or V3 script, which only run at the top level");
    }

    if ((result == CARDANO_SUCCESS) && (outcome == PRV_REDEEMER_NOT_DESERIALISABLE))
    {
      set_error_message(impl, "a redeemer's script cannot be deserialised at this protocol version");
      result = CARDANO_ERROR_DECODING;
    }

    if ((result == CARDANO_SUCCESS) && (outcome == PRV_REDEEMER_SCRIPT_FAILED))
    {
      set_error_message(impl, "phase-2 validation failed for a redeemer");
      result = CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE;
    }

    if ((result == CARDANO_SUCCESS) && (out_list != NULL))
    {
      result = cardano_redeemer_list_add(out_list, new_redeemer);
    }

    cardano_redeemer_unref(&new_redeemer);
    cardano_redeemer_unref(&redeemer);
  }

  cardano_redeemer_list_unref(&redeemers);
  release_level(&level);

  return result;
}

/**
 * \brief The \ref cardano_tx_evaluate_func_t entry of the native evaluator.
 *
 * Evaluates the redeemers of the top-level transaction and of every
 * sub-transaction together, as the ledger evaluates a Dijkstra transaction as a
 * whole: the scripts of every level are first deserialised, and only then
 * evaluated, top level first and then each sub-transaction in order. The
 * transaction's own inputs and those of its sub-transactions must be resolvable
 * in the additional UTxOs. The returned redeemer list carries the computed
 * ex-units of the level the evaluator is bound to: the top-level transaction, or
 * one sub-transaction. A script that cannot be deserialised for its language at
 * the protocol version (an unavailable language or builtin, or a malformed
 * script) is reported as \ref CARDANO_ERROR_DECODING, and a script that fails
 * phase-2 validation (including a Plutus Core version not available at the
 * protocol version and a script with a free variable) as
 * \ref CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE, each with an error message
 * naming the failure.
 */
static cardano_error_t
evaluate_transaction(
  cardano_tx_evaluator_impl_t* impl,
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         additional_utxos,
  cardano_redeemer_list_t**    redeemers)
{
  native_context_t*              ctx       = NULL;
  cardano_transaction_body_t*    body      = NULL;
  cardano_sub_transaction_set_t* subs      = NULL;
  cardano_redeemer_list_t*       out_list  = NULL;
  size_t                         sub_count = 0U;
  cardano_uplc_budget_t          remaining;
  cardano_error_t                result = CARDANO_SUCCESS;

  if ((impl == NULL) || (tx == NULL) || (redeemers == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  ctx       = (native_context_t*)((void*)impl->context);
  body      = cardano_transaction_get_body(tx);
  subs      = cardano_transaction_body_get_sub_transactions(body);
  sub_count = cardano_sub_transaction_set_get_length(subs);

  cardano_sub_transaction_set_unref(&subs);
  cardano_transaction_body_unref(&body);

  if (ctx->has_sub_tx_index)
  {
    tx_level_t target;

    result = read_level(tx, &ctx->sub_tx_index, &target);
    release_level(&target);

    if (result != CARDANO_SUCCESS)
    {
      set_error_message(impl, "the transaction has no sub-transaction at the evaluated position");
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_redeemer_list_new(&out_list);
  }

  for (size_t level = 0U; (result == CARDANO_SUCCESS) && (level <= sub_count); ++level)
  {
    uint64_t index = 0U;

    result = run_level(impl, tx, level_sub_tx_index(level, &index), additional_utxos, NULL, NULL);
  }

  remaining.cpu = PRV_MAX_CPU;
  remaining.mem = PRV_MAX_MEM;

  for (size_t level = 0U; (result == CARDANO_SUCCESS) && (level <= sub_count); ++level)
  {
    uint64_t        index     = 0U;
    const uint64_t* sub_index = level_sub_tx_index(level, &index);
    const bool      is_target = (sub_index == NULL) ? !ctx->has_sub_tx_index : (ctx->has_sub_tx_index && (ctx->sub_tx_index == index));

    result = run_level(impl, tx, sub_index, additional_utxos, &remaining, is_target ? out_list : NULL);
  }

  if (result == CARDANO_SUCCESS)
  {
    *redeemers = out_list;
  }
  else
  {
    cardano_redeemer_list_unref(&out_list);
  }

  return result;
}

/**
 * \brief Creates a native evaluator bound to one transaction level.
 *
 * \param[in] slot_config The slot/time parameters. Must not be NULL.
 * \param[in] cost_models The ledger cost models, or NULL.
 * \param[in] protocol_major The protocol major version.
 * \param[in] sub_tx_index The position of the sub-transaction whose redeemers the
 *            evaluator returns, or NULL for the top-level transaction.
 * \param[out] tx_evaluator On success, the new evaluator.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p slot_config or \p tx_evaluator is NULL, or a propagated error.
 */
static cardano_error_t
new_native_evaluator(
  const cardano_slot_config_t* slot_config,
  cardano_costmdls_t*          cost_models,
  const uint64_t               protocol_major,
  const uint64_t*              sub_tx_index,
  cardano_tx_evaluator_t**     tx_evaluator)
{
  cardano_tx_evaluator_impl_t impl = { { 0 }, { 0 }, NULL, NULL };
  native_context_t*           ctx  = NULL;
  cardano_error_t             result;

  if ((slot_config == NULL) || (tx_evaluator == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  ctx = (native_context_t*)_cardano_malloc(sizeof(native_context_t));

  if (ctx == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  ctx->base.ref_count     = 1U;
  ctx->base.deallocator   = context_deallocate;
  ctx->base.last_error[0] = '\0';
  ctx->slot_config        = *slot_config;
  ctx->cost_models        = cost_models;
  ctx->protocol_major     = protocol_major;
  ctx->has_sub_tx_index   = (sub_tx_index != NULL);
  ctx->sub_tx_index       = (sub_tx_index != NULL) ? *sub_tx_index : 0U;

  if (cost_models != NULL)
  {
    cardano_costmdls_ref(cost_models);
  }

  impl.context  = (cardano_object_t*)((void*)ctx);
  impl.evaluate = evaluate_transaction;

  cardano_safe_memcpy(impl.name, sizeof(impl.name), PRV_EVALUATOR_NAME, cardano_safe_strlen(PRV_EVALUATOR_NAME, sizeof(impl.name) - 1U));

  result = cardano_tx_evaluator_new(impl, tx_evaluator);

  if (result != CARDANO_SUCCESS)
  {
    cardano_object_t* base = &ctx->base;
    cardano_object_unref(&base);
  }

  return result;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_tx_evaluator_new_native(
  const cardano_slot_config_t* slot_config,
  cardano_costmdls_t*          cost_models,
  uint64_t                     protocol_major,
  cardano_tx_evaluator_t**     tx_evaluator)
{
  return new_native_evaluator(slot_config, cost_models, protocol_major, NULL, tx_evaluator);
}

cardano_error_t
cardano_tx_evaluator_new_native_sub_transaction(
  const cardano_slot_config_t* slot_config,
  cardano_costmdls_t*          cost_models,
  uint64_t                     protocol_major,
  uint64_t                     sub_tx_index,
  cardano_tx_evaluator_t**     tx_evaluator)
{
  return new_native_evaluator(slot_config, cost_models, protocol_major, &sub_tx_index, tx_evaluator);
}
