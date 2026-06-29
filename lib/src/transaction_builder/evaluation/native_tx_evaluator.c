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
#include <cardano/scripts/script.h>
#include <cardano/scripts/script_language.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>
#include <cardano/voting_procedures/voter_type.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/plutus_v1_script_set.h>
#include <cardano/witness_set/plutus_v2_script_set.h>
#include <cardano/witness_set/plutus_v3_script_set.h>
#include <cardano/witness_set/witness_set.h>

#include "../../allocators.h"
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

/* STRUCTURES ****************************************************************/

/**
 * \brief The bound state of a native phase-2 evaluator.
 *
 * Stored as the \c context of the \ref cardano_tx_evaluator_impl_t and reached
 * by casting the \c cardano_object_t base. Holds the inputs the evaluate
 * signature does not carry: the slot config (copied by value), the ledger cost
 * models (referenced) and the protocol major version.
 */
typedef struct native_context_t
{
    cardano_object_t      base;
    cardano_slot_config_t slot_config;
    cardano_costmdls_t*   cost_models;
    uint64_t              protocol_major;
} native_context_t;

/**
 * \brief The Plutus language version of a resolved script.
 */
typedef enum
{
  PRV_SCRIPT_V1 = 0,
  PRV_SCRIPT_V2 = 1,
  PRV_SCRIPT_V3 = 2
} script_version_t;

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
 * \brief Looks a script hash up in the witness plutus script sets and reference scripts.
 *
 * The script is discovered in the witness V1/V2/V3 script sets or in a resolved
 * output's reference script. The raw witness-set bytes (CBOR-wrapped flat) and
 * the language version are returned on a match. The caller owns \p out_bytes and
 * releases it with \ref cardano_buffer_unref.
 *
 * \return \ref CARDANO_SUCCESS on a match, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if no plutus script with that hash exists, or a propagated error.
 */
static cardano_error_t
find_script_by_hash(
  cardano_witness_set_t*        witness_set,
  cardano_utxo_list_t*          resolved_inputs,
  const cardano_blake2b_hash_t* script_hash,
  cardano_buffer_t**            out_bytes,
  script_version_t*             out_version)
{
  cardano_error_t result = CARDANO_ERROR_ELEMENT_NOT_FOUND;

  cardano_plutus_v1_script_set_t* v1 = cardano_witness_set_get_plutus_v1_scripts(witness_set);
  cardano_plutus_v2_script_set_t* v2 = cardano_witness_set_get_plutus_v2_scripts(witness_set);
  cardano_plutus_v3_script_set_t* v3 = cardano_witness_set_get_plutus_v3_scripts(witness_set);

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

  for (i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (resolved_inputs != NULL) && (i < cardano_utxo_list_get_length(resolved_inputs)); ++i)
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
          cardano_script_language_t lang = CARDANO_SCRIPT_LANGUAGE_NATIVE;

          if (cardano_script_get_language(script_ref, &lang) == CARDANO_SUCCESS)
          {
            switch (lang)
            {
              case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
              {
                cardano_plutus_v1_script_t* s = NULL;

                if (cardano_script_to_plutus_v1(script_ref, &s) == CARDANO_SUCCESS)
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

                if (cardano_script_to_plutus_v2(script_ref, &s) == CARDANO_SUCCESS)
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

                if (cardano_script_to_plutus_v3(script_ref, &s) == CARDANO_SUCCESS)
                {
                  result       = cardano_plutus_v3_script_to_raw_bytes(s, out_bytes);
                  *out_version = PRV_SCRIPT_V3;
                }

                cardano_plutus_v3_script_unref(&s);
                break;
              }
              case CARDANO_SCRIPT_LANGUAGE_NATIVE:
              default:
              {
                break;
              }
            }
          }
        }

        cardano_blake2b_hash_unref(&hash);
        cardano_script_unref(&script_ref);
      }

      cardano_transaction_output_unref(&output);
    }

    cardano_utxo_unref(&utxo);
  }

  cardano_plutus_v1_script_set_unref(&v1);
  cardano_plutus_v2_script_set_unref(&v2);
  cardano_plutus_v3_script_set_unref(&v3);

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
 * \brief Resolves the script hash a redeemer points at, plus the spend datum.
 *
 * The (tag, index) pair selects the concrete item against the transaction's own
 * sorted inputs (spend), sorted mint policies (mint), withdrawals (reward),
 * certificates (cert), voters (vote) or proposals (propose). The script hash is
 * the script credential the item is authorized by; the spend purpose also
 * resolves the datum. The index ordering mirrors the ledger's per-purpose
 * ordering, which is the consensus-critical mapping.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_INDEX_OUT_OF_BOUNDS
 *         if the index is out of range, \ref CARDANO_ERROR_INVALID_ARGUMENT for a
 *         non-script credential or an unsupported tag, or a propagated error.
 */
static cardano_error_t
resolve_script_hash(
  cardano_transaction_t*   tx,
  cardano_witness_set_t*   witness_set,
  cardano_utxo_list_t*     resolved_inputs,
  cardano_redeemer_t*      redeemer,
  cardano_blake2b_hash_t** out_hash,
  cardano_plutus_data_t**  out_datum)
{
  cardano_transaction_body_t*  body   = cardano_transaction_get_body(tx);
  const cardano_redeemer_tag_t tag    = cardano_redeemer_get_tag(redeemer);
  const uint64_t               index  = cardano_redeemer_get_index(redeemer);
  cardano_error_t              result = CARDANO_SUCCESS;

  *out_hash  = NULL;
  *out_datum = NULL;

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
        result = cardano_policy_id_list_get(policies, (size_t)index, out_hash);
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
        cardano_transaction_input_t*  input  = NULL;
        cardano_transaction_output_t* output = NULL;

        result = cardano_transaction_input_set_get(inputs, (size_t)index, &input);

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
            result = resolve_datum(witness_set, output, out_datum);
          }

          cardano_credential_unref(&credential);
          cardano_address_unref(&address);
        }

        cardano_transaction_output_unref(&output);
        cardano_transaction_input_unref(&input);
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
        cardano_reward_address_t* address = NULL;
        uint64_t                  coin    = 0U;

        result = cardano_withdrawal_map_get_key_value_at(withdrawals, (size_t)index, &address, &coin);

        if (result == CARDANO_SUCCESS)
        {
          cardano_credential_t* credential = cardano_reward_address_get_credential(address);

          result = script_hash_from_credential(credential, out_hash);

          cardano_credential_unref(&credential);
        }

        cardano_reward_address_unref(&address);
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

        result = cardano_certificate_set_get(certificates, (size_t)index, &certificate);

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

      cardano_certificate_set_unref(&certificates);
      break;
    }
    case CARDANO_REDEEMER_TAG_VOTING:
    {
      cardano_voting_procedures_t* voting = cardano_transaction_body_get_voting_procedures(body);
      cardano_voter_list_t*        voters = NULL;

      result = (voting != NULL) ? cardano_voting_procedures_get_voters(voting, &voters) : CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;

      if ((result == CARDANO_SUCCESS) && (index >= cardano_voter_list_get_length(voters)))
      {
        result = CARDANO_ERROR_INDEX_OUT_OF_BOUNDS;
      }

      if (result == CARDANO_SUCCESS)
      {
        cardano_voter_t* voter = NULL;

        result = cardano_voter_list_get(voters, (size_t)index, &voter);

        if (result == CARDANO_SUCCESS)
        {
          cardano_credential_t* credential = cardano_voter_get_credential(voter);

          result = script_hash_from_credential(credential, out_hash);

          cardano_credential_unref(&credential);
        }

        cardano_voter_unref(&voter);
      }

      cardano_voter_list_unref(&voters);
      cardano_voting_procedures_unref(&voting);
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

        result = cardano_proposal_procedure_set_get(proposals, (size_t)index, &proposal);

        if (result == CARDANO_SUCCESS)
        {
          result = proposal_policy_hash(proposal, out_hash);
        }

        cardano_proposal_procedure_unref(&proposal);
      }

      cardano_proposal_procedure_set_unref(&proposals);
      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  cardano_transaction_body_unref(&body);

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
 * language the version default is returned, which keeps an under-specified host
 * usable. Consensus-critical: the spent ex-units track the cost model the
 * transaction was submitted under, not a hardcoded default.
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
      default:
      {
        out->model.builtins = cardano_uplc_builtin_costs_v3();
        break;
      }
    }

    out->semantics = cardano_uplc_builtin_semantics_for_language_and_protocol(uplc_lang_version(version), ctx->protocol_major);
    result         = CARDANO_SUCCESS;
  }

  return result;
}

/**
 * \brief Builds the version-appropriate ScriptContext for a redeemer.
 *
 * Dispatches to the V1, V2 or V3 builder; the V3 builder embeds the redeemer data
 * and the optional datum inside the context, the V1/V2 builders carry only the
 * TxInfo and the script purpose.
 */
static cardano_error_t
build_script_context(
  script_version_t             version,
  cardano_transaction_t*       tx,
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
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
      break;
    }
  }

  return result;
}

/**
 * \brief Applies the version-appropriate arguments to a decoded program.
 *
 * V1/V2 spend applies [datum, redeemer, context]; V1/V2 non-spend applies
 * [redeemer, context]; V3 applies [context] only, since the V3 context already
 * carries the redeemer and the datum.
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

  if (version == PRV_SCRIPT_V3)
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
 * \brief Evaluates one redeemer and writes its consumed ex-units into a new redeemer.
 *
 * The whole evaluation runs inside a per-redeemer arena that is freed on every
 * path. A script that fails (error term, out of budget, unsupported builtin) is a
 * phase-2 validation failure reported through \p out_failed, not a host error.
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
  cardano_witness_set_t* witness_set,
  cardano_utxo_list_t*   resolved_inputs,
  cardano_redeemer_t*    redeemer,
  cardano_uplc_budget_t* remaining,
  cardano_redeemer_t**   out_redeemer,
  bool*                  out_failed)
{
  cardano_blake2b_hash_t*       script_hash    = NULL;
  cardano_plutus_data_t*        datum          = NULL;
  cardano_buffer_t*             script_bytes   = NULL;
  cardano_plutus_data_t*        script_context = NULL;
  cardano_uplc_arena_t*         arena          = NULL;
  const cardano_uplc_program_t* program        = NULL;
  cardano_uplc_program_t*       applied        = NULL;
  cardano_ex_units_t*           ex_units       = NULL;
  script_version_t              version        = PRV_SCRIPT_V3;
  cardano_uplc_eval_result_t    eval_result;
  cardano_error_t               result = CARDANO_SUCCESS;

  *out_redeemer = NULL;
  *out_failed   = false;

  result = resolve_script_hash(tx, witness_set, resolved_inputs, redeemer, &script_hash, &datum);

  if (result == CARDANO_SUCCESS)
  {
    result = find_script_by_hash(witness_set, resolved_inputs, script_hash, &script_bytes, &version);
  }

  if (result == CARDANO_SUCCESS)
  {
    if ((datum == NULL) && ((version == PRV_SCRIPT_V1) || (version == PRV_SCRIPT_V2)) && (cardano_redeemer_get_tag(redeemer) == CARDANO_REDEEMER_TAG_SPEND))
    {
      result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = build_script_context(version, tx, resolved_inputs, &ctx->slot_config, redeemer, datum, &script_context);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_arena_new(PRV_ARENA_BLOCK_SIZE, &arena);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_uplc_program_from_script_bytes(arena, cardano_buffer_get_data(script_bytes), cardano_buffer_get_size(script_bytes), &program);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = apply_arguments(arena, version, program, redeemer, datum, script_context, &applied);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_uplc_selected_cost_model_t selected;

    result = select_cost_model(ctx, version, &selected);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_uplc_int_evaluate_with_costs(arena, applied, &selected.model, selected.semantics, *remaining, &eval_result);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    if (eval_result.status == CARDANO_UPLC_EVAL_SUCCESS)
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
      *out_failed = true;
    }
  }

  cardano_ex_units_unref(&ex_units);
  cardano_uplc_arena_free(&arena);
  cardano_plutus_data_unref(&script_context);
  cardano_buffer_unref(&script_bytes);
  cardano_plutus_data_unref(&datum);
  cardano_blake2b_hash_unref(&script_hash);

  return result;
}

/**
 * \brief The \ref cardano_tx_evaluate_func_t entry of the native evaluator.
 *
 * Builds the resolved-input set from the additional UTxOs (the transaction's own
 * inputs must be resolvable there), evaluates every redeemer in the witness set,
 * and returns a redeemer list whose entries carry the computed ex-units. A script
 * that fails phase-2 validation is reported as \ref CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE,
 * naming the failing redeemer in the error message.
 */
static cardano_error_t
evaluate_transaction(
  cardano_tx_evaluator_impl_t* impl,
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         additional_utxos,
  cardano_redeemer_list_t**    redeemers)
{
  native_context_t*        ctx         = NULL;
  cardano_witness_set_t*   witness_set = NULL;
  cardano_redeemer_list_t* in_list     = NULL;
  cardano_redeemer_list_t* out_list    = NULL;
  cardano_uplc_budget_t    remaining;
  cardano_error_t          result = CARDANO_SUCCESS;

  if ((impl == NULL) || (tx == NULL) || (redeemers == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  ctx         = (native_context_t*)((void*)impl->context);
  witness_set = cardano_transaction_get_witness_set(tx);
  in_list     = cardano_witness_set_get_redeemers(witness_set);

  if (in_list == NULL)
  {
    result = cardano_redeemer_list_new(redeemers);
    cardano_witness_set_unref(&witness_set);
    return result;
  }

  result = cardano_redeemer_list_new(&out_list);

  remaining.cpu = PRV_MAX_CPU;
  remaining.mem = PRV_MAX_MEM;

  for (size_t i = 0U; (result == CARDANO_SUCCESS) && (i < cardano_redeemer_list_get_length(in_list)); ++i)
  {
    cardano_redeemer_t* redeemer     = NULL;
    cardano_redeemer_t* new_redeemer = NULL;
    bool                failed       = false;

    result = cardano_redeemer_list_get(in_list, i, &redeemer);

    if (result == CARDANO_SUCCESS)
    {
      result = eval_redeemer(ctx, tx, witness_set, additional_utxos, redeemer, &remaining, &new_redeemer, &failed);
    }

    if ((result == CARDANO_SUCCESS) && failed)
    {
      cardano_safe_memcpy(impl->error_message, sizeof(impl->error_message), "phase-2 validation failed for a redeemer", sizeof(impl->error_message) - 1U);
      impl->error_message[sizeof(impl->error_message) - 1U] = '\0';
      result                                                = CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE;
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_redeemer_list_add(out_list, new_redeemer);
    }

    cardano_redeemer_unref(&new_redeemer);
    cardano_redeemer_unref(&redeemer);
  }

  cardano_redeemer_list_unref(&in_list);
  cardano_witness_set_unref(&witness_set);

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

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_tx_evaluator_new_native(
  const cardano_slot_config_t* slot_config,
  cardano_costmdls_t*          cost_models,
  uint64_t                     protocol_major,
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

  if (cost_models != NULL)
  {
    cardano_costmdls_ref(cost_models);
  }

  impl.context  = (cardano_object_t*)((void*)ctx);
  impl.evaluate = evaluate_transaction;

  cardano_safe_memcpy(impl.name, sizeof(impl.name), "Native UPLC transaction evaluator", sizeof(impl.name) - 1U);
  impl.name[sizeof(impl.name) - 1U] = '\0';

  result = cardano_tx_evaluator_new(impl, tx_evaluator);

  if (result != CARDANO_SUCCESS)
  {
    cardano_object_t* base = &ctx->base;
    cardano_object_unref(&base);
  }

  return result;
}
