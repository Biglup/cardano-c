/**
 * \file provider.c
 *
 * \author angel.castillo
 * \date   Sep 27, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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
#include <cardano/providers/provider.h>

#include "../allocators.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Opaque structure representing a Cardano blockchain data provider instance.
 *
 * This structure serves as a handle to a Cardano provider, encapsulating the necessary context and
 * state required to interact with the Cardano blockchain.
 */
typedef struct cardano_provider_t
{
    cardano_object_t        base;
    cardano_provider_impl_t impl;
} cardano_provider_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a provider object.
 *
 * This function is responsible for properly deallocating a provider object (`cardano_provider_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the provider object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_provider_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the provider
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_provider_deallocate(void* object)
{
  assert(object != NULL);

  cardano_provider_t* provider = (cardano_provider_t*)object;

  cardano_object_unref(&provider->impl.context);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_provider_new(
  cardano_provider_impl_t impl,
  cardano_provider_t**    provider)
{
  if (provider == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *provider = _cardano_malloc(sizeof(cardano_provider_t));

  if (*provider == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*provider)->base.deallocator   = cardano_provider_deallocate;
  (*provider)->base.ref_count     = 1;
  (*provider)->base.last_error[0] = '\0';

  (*provider)->impl = impl;

  // Make sure provider name is null terminated
  (*provider)->impl.name[sizeof((*provider)->impl.name) - 1U] = '\0';
  (*provider)->impl.error_message[0]                          = '\0';

  return CARDANO_SUCCESS;
}

const char*
cardano_provider_get_name(const cardano_provider_t* provider)
{
  if (provider == NULL)
  {
    return "";
  }

  return provider->impl.name;
}

cardano_network_magic_t
cardano_provider_get_network_magic(const cardano_provider_t* provider)
{
  if (provider == NULL)
  {
    return CARDANO_NETWORK_MAGIC_PREPROD;
  }

  return provider->impl.network_magic;
}

cardano_error_t
cardano_provider_get_parameters(
  cardano_provider_t*             provider,
  cardano_protocol_parameters_t** parameters)
{
  if ((provider == NULL) || (parameters == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.get_parameters == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.get_parameters(&provider->impl, parameters);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_get_unspent_outputs(
  cardano_provider_t*   provider,
  cardano_address_t*    address,
  cardano_utxo_list_t** utxo_list)
{
  if ((provider == NULL) || (address == NULL) || (utxo_list == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.get_unspent_outputs == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.get_unspent_outputs(&provider->impl, address, utxo_list);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_get_rewards_available(
  cardano_provider_t*       provider,
  cardano_reward_address_t* address,
  uint64_t*                 rewards)
{
  if ((provider == NULL) || (address == NULL) || (rewards == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.get_rewards_balance == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.get_rewards_balance(&provider->impl, address, rewards);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_get_unspent_outputs_with_asset(
  cardano_provider_t*   provider,
  cardano_address_t*    address,
  cardano_asset_id_t*   asset_id,
  cardano_utxo_list_t** utxo_list)
{
  if ((provider == NULL) || (address == NULL) || (asset_id == NULL) || (utxo_list == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.get_unspent_outputs_with_asset == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.get_unspent_outputs_with_asset(&provider->impl, address, asset_id, utxo_list);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_get_unspent_output_by_nft(
  cardano_provider_t* provider,
  cardano_asset_id_t* asset_id,
  cardano_utxo_t**    utxo)
{
  if ((provider == NULL) || (asset_id == NULL) || (utxo == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.get_unspent_output_by_nft == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.get_unspent_output_by_nft(&provider->impl, asset_id, utxo);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_resolve_unspent_outputs(
  cardano_provider_t*              provider,
  cardano_transaction_input_set_t* tx_ins,
  cardano_utxo_list_t**            utxo_list)
{
  if ((provider == NULL) || (tx_ins == NULL) || (utxo_list == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.resolve_unspent_outputs == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.resolve_unspent_outputs(&provider->impl, tx_ins, utxo_list);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_resolve_datum(
  cardano_provider_t*     provider,
  cardano_blake2b_hash_t* datum_hash,
  cardano_plutus_data_t** datum)
{
  if ((provider == NULL) || (datum_hash == NULL) || (datum == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.resolve_datum == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.resolve_datum(&provider->impl, datum_hash, datum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_confirm_transaction(
  cardano_provider_t*     provider,
  cardano_blake2b_hash_t* tx_id,
  const uint64_t          timeout_ms,
  bool*                   confirmed)
{
  if ((provider == NULL) || (tx_id == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.await_transaction_confirmation == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.await_transaction_confirmation(&provider->impl, tx_id, timeout_ms, confirmed);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_submit_transaction(
  cardano_provider_t*      provider,
  cardano_transaction_t*   tx,
  cardano_blake2b_hash_t** tx_id)
{
  if ((provider == NULL) || (tx == NULL) || (tx_id == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.post_transaction_to_chain == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.post_transaction_to_chain(&provider->impl, tx, tx_id);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

cardano_error_t
cardano_provider_evaluate_transaction(
  cardano_provider_t*       provider,
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      additional_utxos,
  cardano_redeemer_list_t** redeemers)
{
  if ((provider == NULL) || (tx == NULL) || (redeemers == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (provider->impl.evaluate_transaction == NULL)
  {
    return CARDANO_ERROR_NOT_IMPLEMENTED;
  }

  cardano_error_t result = provider->impl.evaluate_transaction(&provider->impl, tx, additional_utxos, redeemers);

  if (result != CARDANO_SUCCESS)
  {
    cardano_provider_set_last_error(provider, provider->impl.error_message);
  }

  return result;
}

void
cardano_provider_unref(cardano_provider_t** provider)
{
  if ((provider == NULL) || (*provider == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*provider)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *provider = NULL;
    return;
  }
}

void
cardano_provider_ref(cardano_provider_t* provider)
{
  if (provider == NULL)
  {
    return;
  }

  cardano_object_ref(&provider->base);
}

size_t
cardano_provider_refcount(const cardano_provider_t* provider)
{
  if (provider == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&provider->base);
}

void
cardano_provider_set_last_error(cardano_provider_t* provider, const char* message)
{
  cardano_object_set_last_error(&provider->base, message);
}

const char*
cardano_provider_get_last_error(const cardano_provider_t* provider)
{
  return cardano_object_get_last_error(&provider->base);
}