/**
 * \file builder_witnesses.c
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

#include "builder_witnesses.h"

#include <cardano/common/credential.h>
#include <cardano/common/guard_set.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/witness_set.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_pad_signer_count(
  cardano_builder_state_t* state,
  const size_t             count)
{
  state->additional_signature_count = count;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_add_signer(
  cardano_builder_state_t* state,
  cardano_blake2b_hash_t*  pub_key_hash,
  const char**             error_message)
{
  if (pub_key_hash == NULL)
  {
    *error_message = "Public key hash is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);

  if (guards == NULL)
  {
    cardano_error_t result = cardano_guard_set_new(&guards);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create guards set.";

      return result;
    }

    result = cardano_transaction_body_set_guards(body, guards);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set guards set.";
      cardano_guard_set_unref(&guards);

      return result;
    }
  }

  cardano_guard_set_unref(&guards);

  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = cardano_credential_new(pub_key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create signer credential.";

    return result;
  }

  result = cardano_guard_set_add(guards, credential);

  cardano_credential_unref(&credential);

  if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
  {
    *error_message = "Failed to add signer.";

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_add_signer_ex(
  cardano_builder_state_t* state,
  const char*              pub_key_hash,
  size_t                   hash_size,
  const char**             error_message)
{
  if ((pub_key_hash == NULL) || (hash_size == 0U))
  {
    *error_message = "Public key hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_t* hash   = NULL;
  cardano_error_t         result = cardano_blake2b_hash_from_hex(pub_key_hash, hash_size, &hash);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse public key hash.";

    return result;
  }

  result = cardano_builder_add_signer(state, hash, error_message);
  cardano_blake2b_hash_unref(&hash);

  return result;
}

cardano_error_t
cardano_builder_add_datum(
  cardano_builder_state_t* state,
  cardano_plutus_data_t*   datum,
  const char**             error_message)
{
  if (datum == NULL)
  {
    *error_message = "Datum is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witness_set);

  cardano_plutus_data_set_t* datums = cardano_witness_set_get_plutus_data(witness_set);

  if (datums == NULL)
  {
    cardano_error_t result = cardano_plutus_data_set_new(&datums);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create datums set.";

      return result;
    }

    result = cardano_witness_set_set_plutus_data(witness_set, datums);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set datums set.";
      cardano_plutus_data_set_unref(&datums);

      return result;
    }
  }

  cardano_plutus_data_set_unref(&datums);

  cardano_error_t result = cardano_plutus_data_set_add(datums, datum);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add datum.";

    return result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_add_script(
  cardano_builder_state_t* state,
  cardano_script_t*        script,
  const char**             error_message)
{
  if (script == NULL)
  {
    *error_message = "Script is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_script_language_t language;
  cardano_error_t           result = cardano_script_get_language(script, &language);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to get script language.";

    return result;
  }

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      state->has_plutus_v1 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      state->has_plutus_v2 = true;
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      state->has_plutus_v3 = true;
      break;
    }
    default:
    {
      break;
    }
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witness_set);

  result = cardano_witness_set_add_script(witness_set, script);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add script to witness set.";
  }

  return result;
}
