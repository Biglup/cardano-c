/**
 * \file deferred_redeemer_list.c
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include <cardano/object.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>

#include "../../allocators.h"

#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief A registered deferred redeemer: the shared witness-set redeemer object and the callback
 * that produces its payload.
 */
typedef struct deferred_redeemer_entry_t
{
    cardano_redeemer_t*            redeemer;
    cardano_deferred_redeemer_fn_t callback;
    void*                          user_context;
} deferred_redeemer_entry_t;

/**
 * \brief A list of pending deferred redeemers.
 */
typedef struct cardano_deferred_redeemer_list_t
{
    cardano_object_t           base;
    deferred_redeemer_entry_t* entries;
    size_t                     size;
    size_t                     capacity;
} cardano_deferred_redeemer_list_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Releases the redeemer references held by the list and frees its storage.
 *
 * \param[in] object The list to deallocate.
 */
static void
deferred_redeemer_list_deallocate(void* object)
{
  cardano_deferred_redeemer_list_t* list = (cardano_deferred_redeemer_list_t*)object;

  for (size_t i = 0U; i < list->size; ++i)
  {
    cardano_redeemer_unref(&list->entries[i].redeemer);
  }

  _cardano_free(list->entries);
  _cardano_free(list);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_deferred_redeemer_list_new(cardano_deferred_redeemer_list_t** list)
{
  if (list == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_deferred_redeemer_list_t* new_list = (cardano_deferred_redeemer_list_t*)_cardano_malloc(sizeof(cardano_deferred_redeemer_list_t));

  if (new_list == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  new_list->base.ref_count     = 1;
  new_list->base.last_error[0] = '\0';
  new_list->base.deallocator   = deferred_redeemer_list_deallocate;
  new_list->entries            = NULL;
  new_list->size               = 0U;
  new_list->capacity           = 0U;

  *list = new_list;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_deferred_redeemer_list_add(
  cardano_deferred_redeemer_list_t* list,
  cardano_redeemer_t*               redeemer,
  cardano_deferred_redeemer_fn_t    callback,
  void*                             user_context)
{
  if ((list == NULL) || (redeemer == NULL) || (callback == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (list->size == list->capacity)
  {
    const size_t               new_capacity = (list->capacity == 0U) ? 4U : (list->capacity * 2U);
    deferred_redeemer_entry_t* new_entries  = (deferred_redeemer_entry_t*)_cardano_malloc(new_capacity * sizeof(deferred_redeemer_entry_t));

    if (new_entries == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    for (size_t i = 0U; i < list->size; ++i)
    {
      new_entries[i] = list->entries[i];
    }

    _cardano_free(list->entries);

    list->entries  = new_entries;
    list->capacity = new_capacity;
  }

  cardano_redeemer_ref(redeemer);

  list->entries[list->size].redeemer     = redeemer;
  list->entries[list->size].callback     = callback;
  list->entries[list->size].user_context = user_context;

  ++list->size;

  return CARDANO_SUCCESS;
}

size_t
cardano_deferred_redeemer_list_get_length(const cardano_deferred_redeemer_list_t* list)
{
  if (list == NULL)
  {
    return 0U;
  }

  return list->size;
}

cardano_error_t
cardano_deferred_redeemer_list_resolve(
  cardano_deferred_redeemer_list_t* list,
  cardano_transaction_t*            draft_tx,
  cardano_utxo_list_t*              resolved_inputs)
{
  if (list == NULL)
  {
    return CARDANO_SUCCESS;
  }

  if ((draft_tx == NULL) || (resolved_inputs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = 0U; (i < list->size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_plutus_data_t* payload = NULL;

    result = list->entries[i].callback(list->entries[i].user_context, draft_tx, resolved_inputs, &payload);

    if (result == CARDANO_SUCCESS)
    {
      if (payload == NULL)
      {
        result = CARDANO_ERROR_POINTER_IS_NULL;
      }
      else
      {
        result = cardano_redeemer_set_data(list->entries[i].redeemer, payload);
      }
    }

    cardano_plutus_data_unref(&payload);
  }

  return result;
}

void
cardano_deferred_redeemer_list_ref(cardano_deferred_redeemer_list_t* list)
{
  if (list == NULL)
  {
    return;
  }

  cardano_object_ref(&list->base);
}

void
cardano_deferred_redeemer_list_unref(cardano_deferred_redeemer_list_t** list)
{
  if ((list == NULL) || (*list == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*list)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *list = NULL;
  }
}

size_t
cardano_deferred_redeemer_list_refcount(const cardano_deferred_redeemer_list_t* list)
{
  if (list == NULL)
  {
    return 0U;
  }

  return cardano_object_refcount(&list->base);
}
