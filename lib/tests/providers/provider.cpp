/**
 * \file provider.cpp
 *
 * \author angel.castillo
 * \date   Sep 27, 2024
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

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/providers/provider.h>

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

/**
 * \brief Allocates and initializes a new Cardano provider context.
 */
static cardano_provider_impl_t
cardano_empty_provider_impl_new()
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

  CARDANO_UNUSED(memset(impl.name, 0, sizeof(impl.name)));
  CARDANO_UNUSED(memccpy((void*)&impl.name[0], "Empty Provider", strlen("Empty Provider"), sizeof(impl.name)));

  impl.post_transaction_to_chain      = NULL;
  impl.await_transaction_confirmation = NULL;
  impl.get_parameters                 = NULL;
  impl.get_unspent_outputs            = NULL;
  impl.get_unspent_outputs_with_asset = NULL;
  impl.get_unspent_output_by_nft      = NULL;
  impl.resolve_unspent_outputs        = NULL;
  impl.resolve_datum                  = NULL;
  impl.evaluate_transaction           = NULL;
  impl.get_rewards_balance            = NULL;

  return impl;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_provider_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_provider_ref(provider);

  // Assert
  EXPECT_THAT(provider, testing::Not((cardano_provider_t*)nullptr));
  EXPECT_EQ(cardano_provider_refcount(provider), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_provider_unref(&provider);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_provider_ref(nullptr);
}

TEST(cardano_provider_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_provider_t* provider = nullptr;

  // Act
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_provider_unref((cardano_provider_t**)nullptr);
}

TEST(cardano_provider_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_provider_ref(provider);
  size_t ref_count = cardano_provider_refcount(provider);

  cardano_provider_unref(&provider);
  size_t updated_ref_count = cardano_provider_refcount(provider);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_provider_ref(provider);
  size_t ref_count = cardano_provider_refcount(provider);

  cardano_provider_unref(&provider);
  size_t updated_ref_count = cardano_provider_refcount(provider);

  cardano_provider_unref(&provider);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(provider, (cardano_provider_t*)nullptr);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_provider_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_provider_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  const char*         message  = "This is a test message";

  // Act
  cardano_provider_set_last_error(provider, message);

  // Assert
  EXPECT_STREQ(cardano_provider_get_last_error(provider), "Object is NULL.");
}

TEST(cardano_provider_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_provider_set_last_error(provider, message);

  // Assert
  EXPECT_STREQ(cardano_provider_get_last_error(provider), "");

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_new, returnsErrorIfGivenANullPtr)
{
  cardano_provider_impl_t impl = cardano_provider_impl_new();
  // Act
  cardano_error_t error = cardano_provider_new(impl, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_object_unref(&impl.context);
}

TEST(cardano_provider_new, returnsSuccessIfGivenAValidImpl)
{
  // Arrange
  cardano_provider_t* provider = nullptr;

  // Act
  cardano_error_t error = cardano_provider_new(cardano_provider_impl_new(), &provider);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_name, returnsEmptyStringIfGivenANullPtr)
{
  // Act
  const char* name = cardano_provider_get_name(nullptr);

  // Assert
  EXPECT_STREQ(name, "");
}

TEST(cardano_provider_get_name, returnsTheNameOfTheProvider)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const char* name = cardano_provider_get_name(provider);

  // Assert
  EXPECT_STREQ(name, "Empty Provider");

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_parameters, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*            provider   = nullptr;
  cardano_protocol_parameters_t* parameters = nullptr;

  // Act
  cardano_error_t error = cardano_provider_get_parameters(provider, &parameters);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_get_parameters, returnsErrorIfGetParametersIsNotImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_parameters_t* parameters = nullptr;

  // Act
  error = cardano_provider_get_parameters(provider, &parameters);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_parameters, returnsSuccessIfGetParametersIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_parameters_t* parameters = nullptr;

  // Act
  error = cardano_provider_get_parameters(provider, &parameters);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_protocol_parameters_unref(&parameters);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_unspent_outputs, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*  provider  = nullptr;
  cardano_address_t*   address   = nullptr;
  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  cardano_error_t error = cardano_provider_get_unspent_outputs(provider, address, &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_get_unspent_outputs, returnsSuccessIfGetUnspentOutputsIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_address_t*   address   = (cardano_address_t*)"";
  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  error = cardano_provider_get_unspent_outputs(provider, address, &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_unspent_outputs_with_asset, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*  provider  = nullptr;
  cardano_address_t*   address   = nullptr;
  cardano_asset_id_t*  asset_id  = nullptr;
  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  cardano_error_t error = cardano_provider_get_unspent_outputs_with_asset(provider, address, asset_id, &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_get_unspent_outputs_with_asset, returnsSuccessIfGetUnspentOutputsIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  error = cardano_provider_get_unspent_outputs_with_asset(provider, (cardano_address_t*)"", (cardano_asset_id_t*)"", &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_unspent_output_by_nft, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_asset_id_t* asset_id = nullptr;
  cardano_utxo_t*     utxo     = nullptr;

  // Act
  cardano_error_t error = cardano_provider_get_unspent_output_by_nft(provider, asset_id, &utxo);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_get_unspent_output_by_nft, returnsSuccessIfGetUnspentOutputByNftIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* utxo = nullptr;

  // Act
  error = cardano_provider_get_unspent_output_by_nft(provider, (cardano_asset_id_t*)"", &utxo);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_resolve_unspent_outputs, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*              provider  = nullptr;
  cardano_transaction_input_set_t* input_set = nullptr;
  cardano_utxo_list_t*             utxo_list = nullptr;

  // Act
  cardano_error_t error = cardano_provider_resolve_unspent_outputs(provider, input_set, &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_resolve_unspent_outputs, returnsSuccessIfResolveUnspentOutputsIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  error = cardano_provider_resolve_unspent_outputs(provider, (cardano_transaction_input_set_t*)"", &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_resolve_datum, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*     provider = nullptr;
  cardano_blake2b_hash_t* hash     = nullptr;
  cardano_plutus_data_t*  datum    = nullptr;

  // Act
  cardano_error_t error = cardano_provider_resolve_datum(provider, hash, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_resolve_datum, returnsSuccessIfResolveDatumIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* datum = nullptr;

  // Act
  error = cardano_provider_resolve_datum(provider, (cardano_blake2b_hash_t*)"", &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_plutus_data_unref(&datum);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_evaluate_transaction, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*      provider      = nullptr;
  cardano_transaction_t*   transaction   = nullptr;
  cardano_utxo_list_t*     utxo_list     = nullptr;
  cardano_redeemer_list_t* redeemer_list = nullptr;

  // Act
  cardano_error_t error = cardano_provider_evaluate_transaction(provider, transaction, utxo_list, &redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_evaluate_transaction, returnsSuccessIfEvaluateTransactionIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemer_list = nullptr;

  // Act
  error = cardano_provider_evaluate_transaction(provider, (cardano_transaction_t*)"", (cardano_utxo_list_t*)"", &redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_redeemer_list_unref(&redeemer_list);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_submit_transaction, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*     provider    = nullptr;
  cardano_transaction_t*  transaction = nullptr;
  cardano_blake2b_hash_t* tx_id       = nullptr;

  // Act
  cardano_error_t error = cardano_provider_submit_transaction(provider, transaction, &tx_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_submit_transaction, returnsSuccessIfPostTransactionToChainIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* tx_id = nullptr;

  // Act
  error = cardano_provider_submit_transaction(provider, (cardano_transaction_t*)"", &tx_id);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_blake2b_hash_unref(&tx_id);
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_confirm_transaction, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*     provider = nullptr;
  cardano_blake2b_hash_t* tx_id    = nullptr;
  uint64_t                timeout  = 0;
  bool                    result   = false;

  // Act
  cardano_error_t error = cardano_provider_confirm_transaction(provider, tx_id, timeout, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_confirm_transaction, returnsSuccessIfAwaitTransactionConfirmationIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t timeout = 0;

  // Act
  error = cardano_provider_confirm_transaction(provider, (cardano_blake2b_hash_t*)"", timeout, (bool*)"");

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_unspent_outputs, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_address_t*   address   = (cardano_address_t*)"";
  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  error = cardano_provider_get_unspent_outputs(provider, address, &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_unspent_outputs_with_asset, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_address_t*   address   = (cardano_address_t*)"";
  cardano_asset_id_t*  asset_id  = (cardano_asset_id_t*)"";
  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  error = cardano_provider_get_unspent_outputs_with_asset(provider, address, asset_id, &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_unspent_output_by_nft, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_id_t* asset_id = (cardano_asset_id_t*)"";
  cardano_utxo_t*     utxo     = nullptr;

  // Act
  error = cardano_provider_get_unspent_output_by_nft(provider, asset_id, &utxo);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_resolve_unspent_outputs, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t*              provider  = nullptr;
  cardano_transaction_input_set_t* input_set = (cardano_transaction_input_set_t*)"";
  cardano_utxo_list_t*             utxo_list = nullptr;

  cardano_error_t error = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_provider_resolve_unspent_outputs(provider, input_set, &utxo_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_resolve_datum, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t*     provider = nullptr;
  cardano_blake2b_hash_t* hash     = (cardano_blake2b_hash_t*)"";
  cardano_plutus_data_t*  datum    = (cardano_plutus_data_t*)"";

  cardano_error_t error = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_provider_resolve_datum(provider, hash, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_evaluate_transaction, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t*      provider      = nullptr;
  cardano_transaction_t*   transaction   = (cardano_transaction_t*)"";
  cardano_utxo_list_t*     utxo_list     = (cardano_utxo_list_t*)"";
  cardano_redeemer_list_t* redeemer_list = nullptr;

  cardano_error_t error = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_provider_evaluate_transaction(provider, transaction, utxo_list, &redeemer_list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_submit_transaction, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t*     provider    = nullptr;
  cardano_transaction_t*  transaction = (cardano_transaction_t*)"";
  cardano_blake2b_hash_t* tx_id       = nullptr;

  cardano_error_t error = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_provider_submit_transaction(provider, transaction, &tx_id);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_confirm_transaction, returnsNotImplementedIfNotImplemented)
{
  // Arrange
  cardano_provider_t*     provider = nullptr;
  cardano_blake2b_hash_t* tx_id    = (cardano_blake2b_hash_t*)"";
  uint64_t                timeout  = 0;
  bool                    result   = false;

  cardano_error_t error = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_provider_confirm_transaction(provider, tx_id, timeout, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_provider_t* provider = nullptr;

  cardano_provider_impl_t impl = cardano_empty_provider_impl_new();

  // Act
  cardano_error_t error = cardano_provider_new(impl, &provider);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(provider, (cardano_provider_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_object_unref(&impl.context);
}

TEST(cardano_provider_get_rewards_available, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_provider_t*       provider = nullptr;
  cardano_reward_address_t* address  = nullptr;
  uint64_t                  balance  = 0;

  // Act
  cardano_error_t error = cardano_provider_get_rewards_available(provider, address, &balance);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_provider_get_rewards_available, returnsSuccessIfGetRewardsBalanceIsImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t balance = 0;

  // Act
  error = cardano_provider_get_rewards_available(provider, (cardano_reward_address_t*)"", &balance);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_provider_unref(&provider);
}

TEST(cardano_provider_get_rewards_available, returnsErrorIfRewardsIsNotImplemented)
{
  // Arrange
  cardano_provider_t* provider = nullptr;
  cardano_error_t     error    = cardano_provider_new(cardano_empty_provider_impl_new(), &provider);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t balance = 0;

  // Act
  error = cardano_provider_get_rewards_available(provider, (cardano_reward_address_t*)"", &balance);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_provider_unref(&provider);
}
