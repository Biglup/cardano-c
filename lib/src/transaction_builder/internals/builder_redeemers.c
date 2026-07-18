/**
 * \file builder_redeemers.c
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

#include "builder_redeemers.h"

#include <cardano/common/ex_units.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/witness_set/redeemer_list.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_builder_add_redeemer(
  cardano_witness_set_t*       witness_set,
  cardano_blake2b_hash_t*      hash,
  cardano_plutus_data_t*       data,
  const cardano_redeemer_tag_t tag,
  cardano_builder_state_t*     state,
  const char**                 error_message)
{
  cardano_error_t          result    = CARDANO_SUCCESS;
  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);

  if (redeemers == NULL)
  {
    result = cardano_redeemer_list_new(&redeemers);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_witness_set_set_redeemers(witness_set, redeemers);

    if (result != CARDANO_SUCCESS)
    {
      cardano_redeemer_list_unref(&redeemers);
      return result;
    }
  }

  cardano_redeemer_list_unref(&redeemers);

  if (data != NULL)
  {
    cardano_redeemer_t* rdmer = NULL;

    cardano_ex_units_t* ex_units = NULL;
    result                       = cardano_ex_units_new(0, 0, &ex_units);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    result = cardano_redeemer_new(tag, 0, data, ex_units, &rdmer);
    cardano_ex_units_unref(&ex_units);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    bool duplicate = false;

    switch (tag)
    {
      case CARDANO_REDEEMER_TAG_MINT:
      {
        result = cardano_blake2b_hash_to_redeemer_map_insert(state->mints_to_redeemer_map, hash, rdmer);

        if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
        {
          *error_message = "Failed to insert plutus data to redeemer map.";

          cardano_redeemer_unref(&rdmer);

          return result;
        }

        if (result == CARDANO_ERROR_DUPLICATED_KEY)
        {
          duplicate = true;
          result    = CARDANO_SUCCESS;
        }

        break;
      }
      case CARDANO_REDEEMER_TAG_REWARD:
      {
        result = cardano_blake2b_hash_to_redeemer_map_insert(state->withdrawals_to_redeemer_map, hash, rdmer);

        if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
        {
          *error_message = "Failed to insert plutus data to redeemer map.";

          cardano_redeemer_unref(&rdmer);

          return result;
        }

        if (result == CARDANO_ERROR_DUPLICATED_KEY)
        {
          duplicate = true;
          result    = CARDANO_SUCCESS;
        }

        break;
      }
      case CARDANO_REDEEMER_TAG_VOTING:
      {
        result = cardano_blake2b_hash_to_redeemer_map_insert(state->votes_to_redeemer_map, hash, rdmer);

        if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_DUPLICATED_KEY))
        {
          *error_message = "Failed to insert plutus data to redeemer map.";

          cardano_redeemer_unref(&rdmer);

          return result;
        }

        if (result == CARDANO_ERROR_DUPLICATED_KEY)
        {
          duplicate = true;
          result    = CARDANO_SUCCESS;
        }

        break;
      }
      case CARDANO_REDEEMER_TAG_CERTIFYING:
      case CARDANO_REDEEMER_TAG_PROPOSING:
      case CARDANO_REDEEMER_TAG_SPEND:
      case CARDANO_REDEEMER_TAG_GUARDING:
      default:
      {
        *error_message = "Invalid redeemer tag.";
        cardano_redeemer_unref(&rdmer);

        return CARDANO_ERROR_ILLEGAL_STATE;
      }
    }

    if (!duplicate)
    {
      result = cardano_redeemer_list_add(redeemers, rdmer);
    }

    cardano_redeemer_unref(&rdmer);
  }

  return result;
}

cardano_error_t
cardano_builder_create_placeholder_plutus_data(cardano_plutus_data_t** data)
{
  cardano_plutus_list_t* fields = NULL;

  cardano_error_t result = cardano_plutus_list_new(&fields);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_constr_plutus_data_t* constr = NULL;

  result = cardano_constr_plutus_data_new(0U, fields, &constr);

  cardano_plutus_list_unref(&fields);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_data_new_constr(constr, data);

  cardano_constr_plutus_data_unref(&constr);

  return result;
}

cardano_error_t
cardano_builder_register_deferred_from_map(
  cardano_builder_state_t*                state,
  cardano_blake2b_hash_to_redeemer_map_t* map,
  cardano_blake2b_hash_t*                 hash,
  cardano_deferred_redeemer_fn_t          callback,
  void*                                   user_context,
  const char**                            error_message)
{
  cardano_redeemer_t* redeemer = NULL;

  cardano_error_t result = cardano_blake2b_hash_to_redeemer_map_get(map, hash, &redeemer);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_deferred_redeemer_list_add(state->deferred_redeemers, redeemer, callback, user_context);
  }

  cardano_redeemer_unref(&redeemer);

  if (result != CARDANO_SUCCESS)
  {
    *error_message = "Failed to register the deferred redeemer.";
  }

  return result;
}
