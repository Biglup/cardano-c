/**
 * \file builder_sub_transactions.c
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#include "builder_sub_transactions.h"

#include <cardano/common/utxo.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input_set.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Predicate that checks whether a UTXO is the one a transaction input points to.
 *
 * \param[in] item The UTXO to evaluate.
 * \param[in] context A pointer to the \ref cardano_transaction_input_t being looked up.
 *
 * \return true if the UTXO belongs to the transaction input, false otherwise.
 */
static bool
find_utxo(cardano_utxo_t* item, const void* context)
{
  const cardano_transaction_input_t* input      = (const cardano_transaction_input_t*)context;
  cardano_transaction_input_t*       utxo_input = cardano_utxo_get_input(item);

  bool found = cardano_transaction_input_equals(utxo_input, input);

  cardano_transaction_input_unref(&utxo_input);

  return found;
}

/**
 * \brief Checks whether a UTXO list resolves a transaction input.
 *
 * \param[in] utxos A pointer to the \ref cardano_utxo_list_t to search.
 * \param[in] input A pointer to the \ref cardano_transaction_input_t to look up.
 *
 * \return true if the list holds the UTXO the input points to, false otherwise.
 */
static bool
has_utxo(const cardano_utxo_list_t* utxos, const cardano_transaction_input_t* input)
{
  cardano_utxo_t* utxo = cardano_utxo_list_find(utxos, find_utxo, input);
  cardano_utxo_unref(&utxo);

  return utxo != NULL;
}

/**
 * \brief Checks that the id of a sub transaction is not yet in a sub transaction set.
 *
 * \param[in] sub_transactions A pointer to the \ref cardano_sub_transaction_set_t of the transaction,
 *                             or NULL when no sub transaction was added yet.
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t to check.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the id is not in the set, \ref CARDANO_ERROR_DUPLICATED_KEY if it
 *         is, or an appropriate error code indicating the failure reason.
 */
static cardano_error_t
check_id_is_unique(
  const cardano_sub_transaction_set_t* sub_transactions,
  cardano_sub_transaction_t*           sub_transaction,
  const char**                         error_message)
{
  if (sub_transactions == NULL)
  {
    return CARDANO_SUCCESS;
  }

  cardano_blake2b_hash_t* id = cardano_sub_transaction_get_id(sub_transaction);

  if (id == NULL)
  {
    *error_message = "Failed to compute the sub transaction id.";

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_sub_transaction_t* existing = NULL;
  const cardano_error_t      result   = cardano_sub_transaction_set_find_by_id(sub_transactions, id, &existing);

  cardano_sub_transaction_unref(&existing);
  cardano_blake2b_hash_unref(&id);

  if (result == CARDANO_SUCCESS)
  {
    *error_message = "Sub transaction is already part of the transaction.";

    return CARDANO_ERROR_DUPLICATED_KEY;
  }

  if (result != CARDANO_ERROR_ELEMENT_NOT_FOUND)
  {
    *error_message = "Failed to look up the sub transaction id.";

    return result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Resolves the spend inputs of a sub transaction.
 *
 * Every spend input must be resolved and must not be spent anywhere else in the batch, neither by the
 * inputs explicitly added to the transaction nor by the sub transactions added before.
 *
 * \param[in] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                  construction.
 * \param[in] inputs A pointer to the \ref cardano_transaction_input_set_t with the spend inputs of the
 *                   sub transaction.
 * \param[in] resolved_utxos A pointer to the \ref cardano_utxo_list_t that resolves the inputs.
 * \param[out] resolved On success, this will point to a newly created list with the UTXO of every
 *                      spend input. The caller is responsible for releasing it with
 *                      \ref cardano_utxo_list_unref.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if every spend input was resolved, \ref CARDANO_ERROR_DUPLICATED_KEY if
 *         an input is already spent, \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if an input is not resolved,
 *         or an appropriate error code indicating the failure reason.
 */
static cardano_error_t
resolve_spend_inputs(
  const cardano_builder_state_t*         state,
  const cardano_transaction_input_set_t* inputs,
  const cardano_utxo_list_t*             resolved_utxos,
  cardano_utxo_list_t**                  resolved,
  const char**                           error_message)
{
  cardano_utxo_list_t* utxos  = NULL;
  cardano_error_t      result = cardano_utxo_list_new(&utxos);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the sub transaction inputs list.";

    return result;
  }

  const size_t length = cardano_transaction_input_set_get_length(inputs);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_transaction_input_t* input = NULL;

    result = cardano_transaction_input_set_get(inputs, i, &input);
    cardano_transaction_input_unref(&input);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to read the sub transaction inputs.";
      cardano_utxo_list_unref(&utxos);

      return result;
    }

    if (has_utxo(state->pre_selected_inputs, input))
    {
      *error_message = "Sub transaction spends an input that is already spent by the transaction.";
      cardano_utxo_list_unref(&utxos);

      return CARDANO_ERROR_DUPLICATED_KEY;
    }

    if (has_utxo(state->sub_transaction_inputs, input))
    {
      *error_message = "Sub transaction spends an input that is already spent by another sub transaction.";
      cardano_utxo_list_unref(&utxos);

      return CARDANO_ERROR_DUPLICATED_KEY;
    }

    cardano_utxo_t* utxo = cardano_utxo_list_find(resolved_utxos, find_utxo, input);
    cardano_utxo_unref(&utxo);

    if (utxo == NULL)
    {
      *error_message = "Sub transaction spends an input that is not in the resolved UTXOs.";
      cardano_utxo_list_unref(&utxos);

      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    result = cardano_utxo_list_add(utxos, utxo);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to add UTXO to the sub transaction inputs.";
      cardano_utxo_list_unref(&utxos);

      return result;
    }
  }

  *resolved = utxos;

  return CARDANO_SUCCESS;
}

/**
 * \brief Resolves the reference inputs of a sub transaction.
 *
 * Reference inputs are only needed to account for the reference scripts they carry, so the ones that
 * are not resolved are skipped.
 *
 * \param[in] reference_inputs A pointer to the \ref cardano_transaction_input_set_t with the reference
 *                             inputs of the sub transaction, or NULL when it has none.
 * \param[in] resolved_utxos A pointer to the \ref cardano_utxo_list_t that resolves the inputs.
 * \param[out] resolved On success, this will point to a newly created list with the UTXO of every
 *                      resolved reference input. The caller is responsible for releasing it with
 *                      \ref cardano_utxo_list_unref.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the list was created, or an appropriate error code indicating the
 *         failure reason.
 */
static cardano_error_t
resolve_reference_inputs(
  const cardano_transaction_input_set_t* reference_inputs,
  const cardano_utxo_list_t*             resolved_utxos,
  cardano_utxo_list_t**                  resolved,
  const char**                           error_message)
{
  cardano_utxo_list_t* utxos  = NULL;
  cardano_error_t      result = cardano_utxo_list_new(&utxos);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to create the sub transaction reference inputs list.";

    return result;
  }

  const size_t length = cardano_transaction_input_set_get_length(reference_inputs);

  for (size_t i = 0U; i < length; ++i)
  {
    cardano_transaction_input_t* input = NULL;

    result = cardano_transaction_input_set_get(reference_inputs, i, &input);
    cardano_transaction_input_unref(&input);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to read the sub transaction reference inputs.";
      cardano_utxo_list_unref(&utxos);

      return result;
    }

    cardano_utxo_t* utxo = cardano_utxo_list_find(resolved_utxos, find_utxo, input);
    cardano_utxo_unref(&utxo);

    if (utxo != NULL)
    {
      result = cardano_utxo_list_add(utxos, utxo);

      if (result != CARDANO_SUCCESS)
      {
        *error_message = "Failed to add UTXO to the sub transaction reference inputs.";
        cardano_utxo_list_unref(&utxos);

        return result;
      }
    }
  }

  *resolved = utxos;

  return CARDANO_SUCCESS;
}

/**
 * \brief Creates the list that results from appending UTXOs to a list of the state.
 *
 * The state lists are replaced instead of being extended in place, so a sub transaction is either
 * fully recorded or not recorded at all.
 *
 * \param[in] current A pointer to the \ref cardano_utxo_list_t held by the state.
 * \param[in] added A pointer to the \ref cardano_utxo_list_t with the UTXOs to append.
 * \param[out] updated On success, this will point to the list that replaces \p current, which is
 *                     \p current itself when there is nothing to append. The caller is responsible for
 *                     releasing it with \ref cardano_utxo_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the list was created, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *         if memory could not be allocated.
 */
static cardano_error_t
append_utxos(
  cardano_utxo_list_t*       current,
  const cardano_utxo_list_t* added,
  cardano_utxo_list_t**      updated)
{
  if (cardano_utxo_list_get_length(added) == 0U)
  {
    cardano_utxo_list_ref(current);
    *updated = current;

    return CARDANO_SUCCESS;
  }

  cardano_utxo_list_t* utxos = cardano_utxo_list_concat(current, added);

  if (utxos == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *updated = utxos;

  return CARDANO_SUCCESS;
}

/**
 * \brief Adds a sub transaction to the sub transaction set of a transaction body.
 *
 * The set is created and stored in the body when the body has none. A set that could not take the
 * sub transaction is never stored.
 *
 * \param[in,out] body A pointer to the \ref cardano_transaction_body_t that carries the set.
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t to add.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS.
 *
 * \return \ref CARDANO_SUCCESS if the sub transaction was added, or an appropriate error code
 *         indicating the failure reason.
 */
static cardano_error_t
insert_sub_transaction(
  cardano_transaction_body_t* body,
  cardano_sub_transaction_t*  sub_transaction,
  const char**                error_message)
{
  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  const bool                     is_new_set       = (sub_transactions == NULL);

  if (is_new_set)
  {
    const cardano_error_t new_result = cardano_sub_transaction_set_new(&sub_transactions);

    if (new_result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to create the sub transaction set.";

      return new_result;
    }
  }

  cardano_error_t result = cardano_sub_transaction_set_add(sub_transactions, sub_transaction);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to add the sub transaction to the set.";
    cardano_sub_transaction_set_unref(&sub_transactions);

    return result;
  }

  if (is_new_set)
  {
    result = cardano_transaction_body_set_sub_transactions(body, sub_transactions);

    if (result != CARDANO_SUCCESS)
    {
      *error_message = "Failed to set the sub transaction set.";
    }
  }

  cardano_sub_transaction_set_unref(&sub_transactions);

  return result;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_add_sub_transaction(
  cardano_builder_state_t*   state,
  cardano_sub_transaction_t* sub_transaction,
  cardano_utxo_list_t*       resolved_utxos,
  const char**               error_message)
{
  if (sub_transaction == NULL)
  {
    *error_message = "Sub transaction is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (resolved_utxos == NULL)
  {
    *error_message = "Resolved UTXOs list is NULL.";

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_transaction_body_t* body = cardano_transaction_get_body(state->transaction);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  cardano_error_t result = check_id_is_unique(sub_transactions, sub_transaction, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_sub_transaction_body_t* sub_body = cardano_sub_transaction_get_body(sub_transaction);
  cardano_sub_transaction_body_unref(&sub_body);

  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(sub_body);
  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_input_set_t* reference_inputs = cardano_sub_transaction_body_get_reference_inputs(sub_body);
  cardano_transaction_input_set_unref(&reference_inputs);

  cardano_utxo_list_t* spent = NULL;

  result = resolve_spend_inputs(state, inputs, resolved_utxos, &spent, error_message);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_utxo_list_t* referenced = NULL;

  result = resolve_reference_inputs(reference_inputs, resolved_utxos, &referenced, error_message);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&spent);

    return result;
  }

  cardano_utxo_list_t* updated_inputs = NULL;

  result = append_utxos(state->sub_transaction_inputs, spent, &updated_inputs);
  cardano_utxo_list_unref(&spent);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to record the sub transaction inputs.";
    cardano_utxo_list_unref(&referenced);

    return result;
  }

  cardano_utxo_list_t* updated_reference_inputs = NULL;

  result = append_utxos(state->sub_transaction_reference_inputs, referenced, &updated_reference_inputs);
  cardano_utxo_list_unref(&referenced);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to record the sub transaction reference inputs.";
    cardano_utxo_list_unref(&updated_inputs);

    return result;
  }

  result = insert_sub_transaction(body, sub_transaction, error_message);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&updated_inputs);
    cardano_utxo_list_unref(&updated_reference_inputs);

    return result;
  }

  cardano_utxo_list_unref(&state->sub_transaction_inputs);
  cardano_utxo_list_unref(&state->sub_transaction_reference_inputs);

  state->sub_transaction_inputs           = updated_inputs;
  state->sub_transaction_reference_inputs = updated_reference_inputs;

  return CARDANO_SUCCESS;
}

bool
cardano_builder_is_input_spent_by_sub_transaction(
  const cardano_builder_state_t*     state,
  const cardano_transaction_input_t* input)
{
  return has_utxo(state->sub_transaction_inputs, input);
}
