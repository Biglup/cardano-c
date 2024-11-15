/**
 * \file tx_evaluator.c
 *
 * \author angel.castillo
 * \date   Nov 05, 2024
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
#include <cardano/transaction_builder/evaluation/tx_evaluator.h>

#include "../../allocators.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Opaque structure for a transaction evaluator.
 *
 * The `cardano_tx_evaluator_t` structure serves as the public-facing handle for managing
 * the transaction evaluation process within the Cardano framework.
 */
typedef struct cardano_tx_evaluator_t
{
    cardano_object_t            base;
    cardano_tx_evaluator_impl_t impl;
} cardano_tx_evaluator_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a tx_evaluator object.
 *
 * This function is responsible for properly deallocating a tx_evaluator object (`cardano_tx_evaluator_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the tx_evaluator object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_tx_evaluator_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the tx_evaluator
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_tx_evaluator_deallocate(void* object)
{
  assert(object != NULL);

  cardano_tx_evaluator_t* tx_evaluator = (cardano_tx_evaluator_t*)object;

  cardano_object_unref(&tx_evaluator->impl.context);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_tx_evaluator_new(
  cardano_tx_evaluator_impl_t impl,
  cardano_tx_evaluator_t**    tx_evaluator)
{
  if (tx_evaluator == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *tx_evaluator = _cardano_malloc(sizeof(cardano_tx_evaluator_t));

  if (*tx_evaluator == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*tx_evaluator)->base.deallocator   = cardano_tx_evaluator_deallocate;
  (*tx_evaluator)->base.ref_count     = 1;
  (*tx_evaluator)->base.last_error[0] = '\0';

  (*tx_evaluator)->impl = impl;

  // Make sure tx_evaluator name is null terminated
  (*tx_evaluator)->impl.name[sizeof((*tx_evaluator)->impl.name) - 1U] = '\0';
  (*tx_evaluator)->impl.error_message[0]                              = '\0';

  return CARDANO_SUCCESS;
}

const char*
cardano_tx_evaluator_get_name(const cardano_tx_evaluator_t* tx_evaluator)
{
  if (tx_evaluator == NULL)
  {
    return "";
  }

  return tx_evaluator->impl.name;
}

cardano_error_t
cardano_tx_evaluator_evaluate(
  cardano_tx_evaluator_t*   tx_evaluator,
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      additional_utxos,
  cardano_redeemer_list_t** redeemers)
{
  if ((tx_evaluator == NULL) || (tx == NULL) || (redeemers == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (tx_evaluator->impl.evaluate == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = tx_evaluator->impl.evaluate(&tx_evaluator->impl, tx, additional_utxos, redeemers);

  if (result != CARDANO_SUCCESS)
  {
    cardano_tx_evaluator_set_last_error(tx_evaluator, tx_evaluator->impl.error_message);
  }

  return result;
}

void
cardano_tx_evaluator_unref(cardano_tx_evaluator_t** tx_evaluator)
{
  if ((tx_evaluator == NULL) || (*tx_evaluator == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*tx_evaluator)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *tx_evaluator = NULL;
    return;
  }
}

void
cardano_tx_evaluator_ref(cardano_tx_evaluator_t* tx_evaluator)
{
  if (tx_evaluator == NULL)
  {
    return;
  }

  cardano_object_ref(&tx_evaluator->base);
}

size_t
cardano_tx_evaluator_refcount(const cardano_tx_evaluator_t* tx_evaluator)
{
  if (tx_evaluator == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&tx_evaluator->base);
}

void
cardano_tx_evaluator_set_last_error(cardano_tx_evaluator_t* tx_evaluator, const char* message)
{
  cardano_object_set_last_error(&tx_evaluator->base, message);
}

const char*
cardano_tx_evaluator_get_last_error(const cardano_tx_evaluator_t* tx_evaluator)
{
  return cardano_object_get_last_error(&tx_evaluator->base);
}