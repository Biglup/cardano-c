/**
 * \file provider_tx_evaluator.cpp
 *
 * \author angel.castillo
 * \date   Nov 05, 2024
 *
 * \section LICENSE
 *
 * Copyright 2024 Biglup Labs
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

#include "../../../src/allocators.h"
#include "../../allocators_helpers.h"

#include <cardano/transaction_builder/evaluation/provider_tx_evaluator.h>

#include <gmock/gmock.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Allocates and initializes a new Cardano provider context.
 */
typedef struct api_context_t
{
    cardano_object_t base;
    char             key[256];
} ref_counted_string_t;

/**
 * \brief Allocates and initializes a new Cardano provider context.
 */
static cardano_provider_impl_t
cardano_provider_impl_new()
{
  cardano_provider_impl_t impl    = { 0 };
  api_context_t*          context = reinterpret_cast<api_context_t*>(malloc(sizeof(api_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    CARDANO_UNUSED(memset(context->key, 0, sizeof(context->key)));
    CARDANO_UNUSED(memccpy((void*)&context->key[0], "This is a test key", strlen("This is a test key"), sizeof(context->key)));

    impl.context = (cardano_object_t*)context;
  }

  impl.post_transaction_to_chain = [](cardano_provider_impl_t*, cardano_transaction_t*, cardano_blake2b_hash_t** tx_id) -> cardano_error_t
  {
    return cardano_blake2b_compute_hash((const byte_t*)"a", 1, 32, tx_id);
  };

  impl.await_transaction_confirmation = [](cardano_provider_impl_t*, cardano_blake2b_hash_t*, uint64_t, bool*) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.get_parameters = [](cardano_provider_impl_t*, cardano_protocol_parameters_t** param) -> cardano_error_t
  {
    return cardano_protocol_parameters_new(param);
  };

  impl.get_unspent_outputs = [](cardano_provider_impl_t*, cardano_address_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.get_unspent_outputs_with_asset = [](cardano_provider_impl_t*, cardano_address_t*, cardano_asset_id_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.get_unspent_output_by_nft = [](cardano_provider_impl_t*, cardano_asset_id_t*, cardano_utxo_t** utxo) -> cardano_error_t
  {
    static const char*     CBOR   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
    cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

    if (reader == NULL)
    {
      return CARDANO_ERROR_GENERIC;
    }

    cardano_error_t result = cardano_utxo_from_cbor(reader, utxo);

    cardano_cbor_reader_unref(&reader);

    return result;
  };

  impl.resolve_unspent_outputs = [](cardano_provider_impl_t*, cardano_transaction_input_set_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.resolve_datum = [](cardano_provider_impl_t*, cardano_blake2b_hash_t*, cardano_plutus_data_t** datum) -> cardano_error_t
  {
    return cardano_plutus_data_new_integer_from_int(0, datum);
  };

  impl.evaluate_transaction = [](cardano_provider_impl_t*, cardano_transaction_t*, cardano_utxo_list_t*, cardano_redeemer_list_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.get_rewards_balance = [](cardano_provider_impl_t*, cardano_reward_address_t*, uint64_t* balance) -> cardano_error_t
  {
    *balance = 0U;

    return CARDANO_SUCCESS;
  };

  return impl;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_tx_evaluator_from_provider, canCreateATxEvaluatorFromProvider)
{
  // Arrange
  cardano_provider_t* provider = nullptr;

  cardano_error_t error = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_tx_evaluator_t* provider_tx_evaluator = nullptr;
  error                                         = cardano_tx_evaluator_from_provider(provider, &provider_tx_evaluator);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemers = nullptr;

  // Act
  error = cardano_tx_evaluator_evaluate(provider_tx_evaluator, (cardano_transaction_t*)"", (cardano_utxo_list_t*)"", &redeemers);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  cardano_tx_evaluator_unref(&provider_tx_evaluator);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_evaluator_from_provider, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_tx_evaluator_t* provider_tx_evaluator = nullptr;

  // Act
  cardano_error_t error = cardano_tx_evaluator_from_provider(nullptr, &provider_tx_evaluator);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}
