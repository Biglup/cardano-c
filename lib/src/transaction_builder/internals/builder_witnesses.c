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
#include <cardano/common/ex_units.h>
#include <cardano/common/guard_set.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

/* STATIC DECLARATIONS *******************************************************/

/**
 * \brief Adds a credential to the guard set of the transaction body.
 *
 * This function fetches the guard set of the transaction body, creating it when it is missing, and
 * appends \p credential to it. Adding a credential that is already present leaves the transaction
 * unchanged and succeeds.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] credential A pointer to the \ref cardano_credential_t to add. This parameter must not be
 *                       NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the credential was added or was already present, or an appropriate
 *         error code indicating the failure reason.
 */
static cardano_error_t
add_guard_credential(
  cardano_builder_state_t* state,
  cardano_credential_t*    credential,
  const char**             error_message)
{
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

  const cardano_error_t result = cardano_guard_set_add(guards, credential);

  if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
  {
    *error_message = "Failed to add guard.";

    return result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Copies a required guards map setting the datum required for one credential.
 *
 * Required guards maps reject duplicated keys and entries can not be updated in place, so this
 * function builds a new map with every entry of \p current in its original position. The entry of
 * \p credential gets \p datum, and it is appended at the end when the credential is not present.
 *
 * \param[in] current A pointer to the \ref cardano_required_guards_map_t to copy, or NULL when there
 *                    are no required guards yet.
 * \param[in] credential A pointer to the \ref cardano_credential_t of the entry to set.
 * \param[in] datum A pointer to the \ref cardano_plutus_data_t stored in the entry, or NULL when the
 *                  guard takes no datum.
 * \param[out] updated On success, this will point to the newly created map. The caller is responsible
 *                     for releasing it with \ref cardano_required_guards_map_unref.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the map was created, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
copy_with_required_guard(
  const cardano_required_guards_map_t* current,
  cardano_credential_t*                credential,
  cardano_plutus_data_t*               datum,
  cardano_required_guards_map_t**      updated,
  const char**                         error_message)
{
  cardano_required_guards_map_t* required_guards = NULL;
  cardano_error_t                result          = cardano_required_guards_map_new(&required_guards);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create required top level guards map.";

    return result;
  }

  const size_t length   = cardano_required_guards_map_get_length(current);
  bool         replaced = false;

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_credential_t*  key   = NULL;
    cardano_plutus_data_t* value = NULL;

    result = cardano_required_guards_map_get_key_value_at(current, i, &key, &value);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to read required top level guard.";
      cardano_required_guards_map_unref(&required_guards);

      return result;
    }

    if (cardano_credential_equals(key, credential))
    {
      result   = cardano_required_guards_map_insert(required_guards, key, datum);
      replaced = true;
    }
    else
    {
      result = cardano_required_guards_map_insert(required_guards, key, value);
    }

    cardano_credential_unref(&key);
    cardano_plutus_data_unref(&value);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to insert required top level guard.";
      cardano_required_guards_map_unref(&required_guards);

      return result;
    }
  }

  if (!replaced)
  {
    result = cardano_required_guards_map_insert(required_guards, credential, datum);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to insert required top level guard.";
      cardano_required_guards_map_unref(&required_guards);

      return result;
    }
  }

  *updated = required_guards;

  return CARDANO_SUCCESS;
}

/**
 * \brief Returns the position of a credential in the guard set of the transaction body.
 *
 * The position is the index a guarding redeemer points to: guards keep the order in which they
 * were added, and key hash guards count towards it. The credential must already be in the guard
 * set of the body, since the callers add it just before the lookup; if it is not found, the
 * position of the last guard is returned.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                  construction. This parameter must not be NULL.
 * \param[in] credential A pointer to the \ref cardano_credential_t to look up. It must be among the
 *                       guards of the transaction.
 *
 * \return The position of \p credential in the guard set.
 */
static size_t
guard_index(cardano_builder_state_t* state, const cardano_credential_t* credential)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);
  cardano_guard_set_unref(&guards);

  const size_t length = cardano_guard_set_get_length(guards);
  size_t       index  = 0U;
  bool         found  = false;

  for (size_t i = 0U; (i < length) && !found; ++i)
  {
    cardano_credential_t* guard = NULL;

    const cardano_error_t result = cardano_guard_set_get(guards, i, &guard);
    cardano_credential_unref(&guard);

    found = (result == CARDANO_SUCCESS) && cardano_credential_equals(guard, credential);
    index = i;
  }

  return index;
}

/**
 * \brief Adds a guarding redeemer to the witness set of the transaction.
 *
 * This function creates a redeemer with the guarding tag, the given index and zero execution units,
 * and appends it to the witness set redeemer list, creating the list when it is missing. The
 * execution units are computed when the transaction is evaluated.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] index The position of the guard in the guard set.
 * \param[in] data A pointer to the \ref cardano_plutus_data_t with the redeemer payload. This
 *                 parameter must not be NULL.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the redeemer was added, \ref CARDANO_ERROR_DUPLICATED_KEY if the
 *         guard already has a redeemer, or an appropriate error code indicating the failure reason.
 */
static cardano_error_t
add_guarding_redeemer(
  cardano_builder_state_t* state,
  const size_t             index,
  cardano_plutus_data_t*   data,
  const char**             error_message)
{
  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(state->transaction);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_error_t          result    = CARDANO_SUCCESS;

  if (redeemers == NULL)
  {
    result = cardano_redeemer_list_new(&redeemers);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_witness_set_set_redeemers(witness_set, redeemers);
    }

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create redeemer list.";
      cardano_redeemer_list_unref(&redeemers);

      return result;
    }
  }

  cardano_redeemer_list_unref(&redeemers);

  const size_t length = cardano_redeemer_list_get_length(redeemers);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_redeemer_t* existing = NULL;

    result = cardano_redeemer_list_get(redeemers, i, &existing);
    cardano_redeemer_unref(&existing);

    if ((result == CARDANO_SUCCESS) && (cardano_redeemer_get_tag(existing) == CARDANO_REDEEMER_TAG_GUARDING) && (cardano_redeemer_get_index(existing) == (uint64_t)index))
    {
      *error_message = "The guard already has a redeemer.";

      return CARDANO_ERROR_DUPLICATED_KEY;
    }
  }

  cardano_ex_units_t* ex_units = NULL;

  result = cardano_ex_units_new(0U, 0U, &ex_units);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create execution units.";

    return result;
  }

  cardano_redeemer_t* redeemer = NULL;

  result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_GUARDING, (uint64_t)index, data, ex_units, &redeemer);
  cardano_ex_units_unref(&ex_units);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create redeemer.";

    return result;
  }

  result = cardano_redeemer_list_add(redeemers, redeemer);
  cardano_redeemer_unref(&redeemer);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add redeemer.";
  }

  return result;
}

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

  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = cardano_credential_new(pub_key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create signer credential.";

    return result;
  }

  result = add_guard_credential(state, credential, error_message);
  cardano_credential_unref(&credential);

  return result;
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
cardano_builder_add_guard(
  cardano_builder_state_t* state,
  cardano_credential_t*    guard,
  const char**             error_message)
{
  if (guard == NULL)
  {
    *error_message = "Guard is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return add_guard_credential(state, guard, error_message);
}

cardano_error_t
cardano_builder_add_guard_ex(
  cardano_builder_state_t*        state,
  const char*                     hash_hex,
  size_t                          hash_hex_size,
  const cardano_credential_type_t type,
  const char**                    error_message)
{
  if ((hash_hex == NULL) || (hash_hex_size == 0U))
  {
    *error_message = "Guard hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_t* guard  = NULL;
  cardano_error_t       result = cardano_credential_from_hash_hex(hash_hex, hash_hex_size, type, &guard);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse guard hash.";

    return result;
  }

  result = cardano_builder_add_guard(state, guard, error_message);
  cardano_credential_unref(&guard);

  return result;
}

cardano_error_t
cardano_builder_add_guard_with_redeemer(
  cardano_builder_state_t* state,
  cardano_credential_t*    guard,
  cardano_plutus_data_t*   redeemer,
  const char**             error_message)
{
  if (guard == NULL)
  {
    *error_message = "Guard is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (redeemer == NULL)
  {
    *error_message = "Redeemer is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_type_t type   = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  cardano_error_t           result = cardano_credential_get_type(guard, &type);

  if ((result == CARDANO_SUCCESS) && (type != CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH))
  {
    *error_message = "Only a script hash guard is satisfied by a redeemer.";

    result = CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (result == CARDANO_SUCCESS)
  {
    result = add_guard_credential(state, guard, error_message);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = add_guarding_redeemer(state, guard_index(state, guard), redeemer, error_message);
  }

  return result;
}

cardano_error_t
cardano_builder_require_top_level_guard(
  cardano_builder_state_t* state,
  cardano_credential_t*    credential,
  cardano_plutus_data_t*   datum,
  const char**             error_message)
{
  if (credential == NULL)
  {
    *error_message = "Required top level guard is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_required_guards_map_t* current = cardano_transaction_body_get_required_top_level_guards(body);
  cardano_required_guards_map_unref(&current);

  cardano_required_guards_map_t* updated = NULL;
  cardano_error_t                result  = copy_with_required_guard(current, credential, datum, &updated, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_transaction_body_set_required_top_level_guards(body, updated);
  cardano_required_guards_map_unref(&updated);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to set required top level guards map.";
  }

  return result;
}

cardano_error_t
cardano_builder_require_top_level_guard_ex(
  cardano_builder_state_t*        state,
  const char*                     hash_hex,
  size_t                          hash_hex_size,
  const cardano_credential_type_t type,
  cardano_plutus_data_t*          datum,
  const char**                    error_message)
{
  if ((hash_hex == NULL) || (hash_hex_size == 0U))
  {
    *error_message = "Required top level guard hash is NULL or empty.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = cardano_credential_from_hash_hex(hash_hex, hash_hex_size, type, &credential);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to parse required top level guard hash.";

    return result;
  }

  result = cardano_builder_require_top_level_guard(state, credential, datum, error_message);
  cardano_credential_unref(&credential);

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
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V4:
    {
      state->has_plutus_v4 = true;
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

cardano_error_t
cardano_builder_add_sub_transaction_script(
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
  const cardano_error_t     result = cardano_script_get_language(script, &language);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to get script language.";

    return result;
  }

  if ((language != CARDANO_SCRIPT_LANGUAGE_NATIVE) && (language != CARDANO_SCRIPT_LANGUAGE_PLUTUS_V4))
  {
    *error_message = "PlutusV1, PlutusV2 and PlutusV3 scripts can not run inside a sub transaction, only native and PlutusV4 scripts are supported.";

    return CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE;
  }

  return cardano_builder_add_script(state, script, error_message);
}
