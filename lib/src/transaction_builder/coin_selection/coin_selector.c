/**
 * \file coin_selector.c
 *
 * \author angel.castillo
 * \date   Oct 14, 2024
 *
 * Copyright 2024 Biglup Labs
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
#include <cardano/transaction_builder/coin_selection/coin_selector.h>

#include "../../allocators.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Opaque structure representing a Cardano blockchain data coin_selector instance.
 */
typedef struct cardano_coin_selector_t
{
    cardano_object_t             base;
    cardano_coin_selector_impl_t impl;
} cardano_coin_selector_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a coin_selector object.
 *
 * This function is responsible for properly deallocating a coin_selector object (`cardano_coin_selector_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the coin_selector object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_coin_selector_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the coin_selector
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_coin_selector_deallocate(void* object)
{
  assert(object != NULL);

  cardano_coin_selector_t* coin_selector = (cardano_coin_selector_t*)object;

  cardano_object_unref(&coin_selector->impl.context);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_coin_selector_new(
  cardano_coin_selector_impl_t impl,
  cardano_coin_selector_t**    coin_selector)
{
  if (coin_selector == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *coin_selector = _cardano_malloc(sizeof(cardano_coin_selector_t));

  if (*coin_selector == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*coin_selector)->base.deallocator   = cardano_coin_selector_deallocate;
  (*coin_selector)->base.ref_count     = 1;
  (*coin_selector)->base.last_error[0] = '\0';

  (*coin_selector)->impl = impl;

  // Make sure coin_selector name is null terminated
  (*coin_selector)->impl.name[sizeof((*coin_selector)->impl.name) - 1U] = '\0';
  (*coin_selector)->impl.error_message[0]                               = '\0';

  return CARDANO_SUCCESS;
}

const char*
cardano_coin_selector_get_name(const cardano_coin_selector_t* coin_selector)
{
  if (coin_selector == NULL)
  {
    return "";
  }

  return coin_selector->impl.name;
}

cardano_error_t
cardano_coin_selector_select(
  cardano_coin_selector_t* coin_selector,
  cardano_utxo_list_t*     pre_selected_utxo,
  cardano_utxo_list_t*     available_utxo,
  cardano_value_t*         target,
  cardano_utxo_list_t**    selection,
  cardano_utxo_list_t**    remaining_utxo)
{
  if ((coin_selector == NULL) || (available_utxo == NULL) || (target == NULL) || (selection == NULL) || (remaining_utxo == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (coin_selector->impl.select == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = coin_selector->impl.select(&coin_selector->impl, pre_selected_utxo, available_utxo, target, selection, remaining_utxo);

  if (result != CARDANO_SUCCESS)
  {
    cardano_coin_selector_set_last_error(coin_selector, coin_selector->impl.error_message);
  }

  return result;
}

void
cardano_coin_selector_unref(cardano_coin_selector_t** coin_selector)
{
  if ((coin_selector == NULL) || (*coin_selector == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*coin_selector)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *coin_selector = NULL;
    return;
  }
}

void
cardano_coin_selector_ref(cardano_coin_selector_t* coin_selector)
{
  if (coin_selector == NULL)
  {
    return;
  }

  cardano_object_ref(&coin_selector->base);
}

size_t
cardano_coin_selector_refcount(const cardano_coin_selector_t* coin_selector)
{
  if (coin_selector == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&coin_selector->base);
}

void
cardano_coin_selector_set_last_error(cardano_coin_selector_t* coin_selector, const char* message)
{
  cardano_object_set_last_error(&coin_selector->base, message);
}

const char*
cardano_coin_selector_get_last_error(const cardano_coin_selector_t* coin_selector)
{
  return cardano_object_get_last_error(&coin_selector->base);
}