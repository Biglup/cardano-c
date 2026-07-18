/**
 * \file builder_votes.c
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

#include "builder_votes.h"

#include <cardano/transaction_body/transaction_body.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/witness_set.h>

#include "../../allocators.h"
#include "../../string_safe.h"
#include "builder_redeemers.h"

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Computes a deterministic, sortable hash ID for a voter.
 *
 * This function creates an identifier by concatenating the voter's type and credential hash.
 *
 * This ID can be used to establish a canonical sorting order for a voter's redeemer.
 *
 * \param[in] voter A pointer to the `cardano_voter_t` object.
 * \param[out] id On success, this will be populated with a pointer to a newly allocated
 * `cardano_blake2b_hash_t` object representing the unique ID.
 *
 * \return `CARDANO_SUCCESS` if the ID was computed successfully. Returns an appropriate
 * error code on failure.
 *
 * \note The caller is responsible for managing the lifecycle of the returned `id` object
 * and must release it by calling `cardano_blake2b_hash_unref`.
 */
static cardano_error_t
compute_voting_procedures_sortable_id(cardano_voter_t* voter, cardano_blake2b_hash_t** id)
{
  if ((id == NULL) || (voter == NULL))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  cardano_credential_t*   credential = cardano_voter_get_credential(voter);
  cardano_blake2b_hash_t* hash       = cardano_credential_get_hash(credential);

  const byte_t* hash_bytes = cardano_blake2b_hash_get_data(hash);
  size_t        hash_size  = cardano_blake2b_hash_get_bytes_size(hash);

  cardano_voter_type_t voter_type;
  result = cardano_voter_get_type(voter, &voter_type);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&hash);
    cardano_credential_unref(&credential);
    return result;
  }

  const size_t total_size = sizeof(voter_type) + hash_size;
  byte_t*      id_bytes   = (byte_t*)_cardano_malloc(total_size);

  if (id_bytes == NULL)
  {
    cardano_blake2b_hash_unref(&hash);
    cardano_credential_unref(&credential);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  size_t cursor = 0;

  cardano_safe_memcpy(&id_bytes[cursor], total_size, &voter_type, sizeof(voter_type));
  cursor += sizeof(voter_type);

  cardano_safe_memcpy(&id_bytes[cursor], total_size - cursor, hash_bytes, hash_size);

  cardano_blake2b_hash_unref(&hash);
  cardano_credential_unref(&credential);

  cardano_blake2b_hash_t* id_hash = NULL;
  result                          = cardano_blake2b_hash_from_bytes(id_bytes, total_size, &id_hash);

  _cardano_free(id_bytes);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *id = id_hash;

  return CARDANO_SUCCESS;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_vote(
  cardano_builder_state_t*        state,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_plutus_data_t*          redeemer,
  const char**                    error_message)
{
  if (voter == NULL)
  {
    *error_message = "Voter is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (action_id == NULL)
  {
    *error_message = "Action ID is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (vote == NULL)
  {
    *error_message = "Vote is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_voting_procedures_t* votes = cardano_transaction_body_get_voting_procedures(body);

  if (votes == NULL)
  {
    cardano_error_t result = cardano_voting_procedures_new(&votes);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create voting procedures.";

      return result;
    }

    result = cardano_transaction_body_set_voting_procedures(body, votes);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set voting procedures.";
      cardano_voting_procedures_unref(&votes);

      return result;
    }
  }

  cardano_voting_procedures_unref(&votes);

  cardano_error_t result = cardano_voting_procedures_insert(votes, voter, action_id, vote);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to insert vote.";

    return result;
  }

  cardano_blake2b_hash_t* id_hash = NULL;
  result                          = compute_voting_procedures_sortable_id(voter, &id_hash);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create voting procedure ID hash.";

    return result;
  }

  if (redeemer != NULL)
  {
    cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(state->transaction);
    cardano_witness_set_unref(&witnesses);

    result = cardano_builder_add_redeemer(witnesses, id_hash, redeemer, CARDANO_REDEEMER_TAG_VOTING, state, error_message);
    cardano_blake2b_hash_unref(&id_hash);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to add redeemer.";

      return result;
    }
  }
  else
  {
    result = cardano_blake2b_hash_to_redeemer_map_insert(state->votes_to_redeemer_map, id_hash, NULL);

    if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
    {
      cardano_blake2b_hash_unref(&id_hash);
      *error_message = "Failed to add vote redeemer placeholder.";

      return result;
    }

    cardano_blake2b_hash_unref(&id_hash);
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_builder_vote_with_deferred_redeemer(
  cardano_builder_state_t*        state,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_deferred_redeemer_fn_t  callback,
  void*                           user_context,
  const char**                    error_message)
{
  if ((voter == NULL) || (action_id == NULL) || (vote == NULL) || (callback == NULL))
  {
    *error_message = "Voter, action id, vote and callback are required";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_plutus_data_t* placeholder = NULL;

  cardano_error_t result = cardano_builder_create_placeholder_plutus_data(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the placeholder redeemer.";

    return result;
  }

  result = cardano_builder_vote(state, voter, action_id, vote, placeholder, error_message);

  cardano_plutus_data_unref(&placeholder);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_t* hash = NULL;

  result = compute_voting_procedures_sortable_id(voter, &hash);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to compute the vote sortable id.";

    return result;
  }

  result = cardano_builder_register_deferred_from_map(state, state->votes_to_redeemer_map, hash, callback, user_context, error_message);

  cardano_blake2b_hash_unref(&hash);

  return result;
}
