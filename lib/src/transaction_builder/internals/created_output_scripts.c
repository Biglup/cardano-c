/**
 * \file created_output_scripts.c
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

#include "created_output_scripts.h"
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>

/**
 * \brief Matches the reference script of one output.
 * \param[in] output The output, which may be NULL.
 * \param[in] hash The required hash.
 * \param[out] script An owned matching script reference.
 * \return Success, not found, or an allocation error.
 */
static cardano_error_t
find_in_output(cardano_transaction_output_t* output, const cardano_blake2b_hash_t* hash, cardano_script_t** script)
{
  cardano_script_t* candidate = cardano_transaction_output_get_script_ref(output);
  if (candidate == NULL)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }
  cardano_blake2b_hash_t* candidate_hash = cardano_script_get_hash(candidate);
  cardano_error_t         result         = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  if (candidate_hash == NULL)
  {
    result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }
  else if (cardano_blake2b_hash_equals(hash, candidate_hash))
  {
    *script = candidate;
    cardano_script_ref(candidate);
    result = CARDANO_SUCCESS;
  }
  else
  {
    /* A different hash leaves the search result as not found. */
  }
  cardano_blake2b_hash_unref(&candidate_hash);
  cardano_script_unref(&candidate);
  return result;
}

/**
 * \brief Searches the reference scripts in an output list.
 * \param[in] outputs The outputs to search.
 * \param[in] hash The required hash.
 * \param[out] script An owned matching script reference.
 * \return Success, not found, or an allocation/access error.
 */
static cardano_error_t
find_in_outputs(cardano_transaction_output_list_t* outputs, const cardano_blake2b_hash_t* hash, cardano_script_t** script)
{
  cardano_error_t result = CARDANO_ERROR_ELEMENT_NOT_FOUND;
  for (size_t i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (i < cardano_transaction_output_list_get_length(outputs)); ++i)
  {
    cardano_transaction_output_t* output = NULL;
    result                               = cardano_transaction_output_list_get(outputs, i, &output);
    if (result == CARDANO_SUCCESS)
    {
      result = find_in_output(output, hash, script);
    }
    cardano_transaction_output_unref(&output);
  }
  return result;
}

cardano_error_t
cardano_builder_find_created_script(cardano_transaction_t* tx, const cardano_blake2b_hash_t* hash, cardano_script_t** script)
{
  if ((tx == NULL) || (hash == NULL) || (script == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  *script                                    = NULL;
  cardano_transaction_body_t*        body    = cardano_transaction_get_body(tx);
  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_error_t                    result  = find_in_outputs(outputs, hash, script);
  cardano_transaction_output_list_unref(&outputs);

  if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
  {
    cardano_transaction_output_t* collateral = cardano_transaction_body_get_collateral_return(body);
    result                                   = find_in_output(collateral, hash, script);
    cardano_transaction_output_unref(&collateral);
  }

  cardano_sub_transaction_set_t* subs = cardano_transaction_body_get_sub_transactions(body);
  for (size_t i = 0U; (result == CARDANO_ERROR_ELEMENT_NOT_FOUND) && (i < cardano_sub_transaction_set_get_length(subs)); ++i)
  {
    cardano_sub_transaction_t* sub = NULL;
    result                         = cardano_sub_transaction_set_get(subs, i, &sub);
    if (result == CARDANO_SUCCESS)
    {
      cardano_sub_transaction_body_t* sub_body = cardano_sub_transaction_get_body(sub);
      outputs                                  = cardano_sub_transaction_body_get_outputs(sub_body);
      result                                   = find_in_outputs(outputs, hash, script);
      cardano_transaction_output_list_unref(&outputs);
      cardano_sub_transaction_body_unref(&sub_body);
    }
    cardano_sub_transaction_unref(&sub);
  }
  cardano_sub_transaction_set_unref(&subs);
  cardano_transaction_body_unref(&body);
  return result;
}
