/**
 * \file tx_evaluator.cpp
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

#include <cardano/transaction_builder/evaluation/tx_evaluator.h>

#include <gmock/gmock.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Allocates and initializes a new Cardano tx_evaluator context.
 */
typedef struct tx_evaluator_context_t
{
    cardano_object_t base;
} ref_counted_string_t;

/**
 * \brief Allocates and initializes a new Cardano tx_evaluator context.
 */
static cardano_tx_evaluator_impl_t
cardano_tx_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl    = { 0 };
  tx_evaluator_context_t*     context = reinterpret_cast<tx_evaluator_context_t*>(malloc(sizeof(tx_evaluator_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    impl.context = (cardano_object_t*)context;
  }

  impl.evaluate = [](cardano_tx_evaluator_impl_t*, cardano_transaction_t*, cardano_utxo_list_t*, cardano_redeemer_list_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  return impl;
}

/**
 * \brief Allocates and initializes a new Cardano tx_evaluator context.
 */
static cardano_tx_evaluator_impl_t
cardano_empty_tx_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl    = { 0 };
  tx_evaluator_context_t*     context = reinterpret_cast<tx_evaluator_context_t*>(malloc(sizeof(tx_evaluator_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    impl.context = (cardano_object_t*)context;
  }

  CARDANO_UNUSED(memset(impl.name, 0, sizeof(impl.name)));
  CARDANO_UNUSED(memccpy((void*)&impl.name[0], "Empty Coin Selector", strlen("Empty Coin Selector"), sizeof(impl.name)));

  impl.evaluate = NULL;

  return impl;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_tx_evaluator_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  cardano_error_t         error        = cardano_tx_evaluator_new(cardano_tx_evaluator_impl_new(), &tx_evaluator);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_tx_evaluator_ref(tx_evaluator);

  // Assert
  EXPECT_THAT(tx_evaluator, testing::Not((cardano_tx_evaluator_t*)nullptr));
  EXPECT_EQ(cardano_tx_evaluator_refcount(tx_evaluator), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_tx_evaluator_unref(&tx_evaluator);
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_evaluator_ref(nullptr);
}

TEST(cardano_tx_evaluator_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;

  // Act
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_evaluator_unref((cardano_tx_evaluator_t**)nullptr);
}

TEST(cardano_tx_evaluator_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  cardano_error_t         error        = cardano_tx_evaluator_new(cardano_tx_evaluator_impl_new(), &tx_evaluator);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_tx_evaluator_ref(tx_evaluator);
  size_t ref_count = cardano_tx_evaluator_refcount(tx_evaluator);

  cardano_tx_evaluator_unref(&tx_evaluator);
  size_t updated_ref_count = cardano_tx_evaluator_refcount(tx_evaluator);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  cardano_error_t         error        = cardano_tx_evaluator_new(cardano_tx_evaluator_impl_new(), &tx_evaluator);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_tx_evaluator_ref(tx_evaluator);
  size_t ref_count = cardano_tx_evaluator_refcount(tx_evaluator);

  cardano_tx_evaluator_unref(&tx_evaluator);
  size_t updated_ref_count = cardano_tx_evaluator_refcount(tx_evaluator);

  cardano_tx_evaluator_unref(&tx_evaluator);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(tx_evaluator, (cardano_tx_evaluator_t*)nullptr);

  // Cleanup
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_tx_evaluator_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_tx_evaluator_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  const char*             message      = "This is a test message";

  // Act
  cardano_tx_evaluator_set_last_error(tx_evaluator, message);

  // Assert
  EXPECT_STREQ(cardano_tx_evaluator_get_last_error(tx_evaluator), "Object is NULL.");
}

TEST(cardano_tx_evaluator_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  cardano_error_t         error        = cardano_tx_evaluator_new(cardano_tx_evaluator_impl_new(), &tx_evaluator);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_tx_evaluator_set_last_error(tx_evaluator, message);

  // Assert
  EXPECT_STREQ(cardano_tx_evaluator_get_last_error(tx_evaluator), "");

  // Cleanup
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_new, returnsErrorIfGivenANullPtr)
{
  cardano_tx_evaluator_impl_t impl = cardano_tx_evaluator_impl_new();
  // Act
  cardano_error_t error = cardano_tx_evaluator_new(impl, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_object_unref(&impl.context);
}

TEST(cardano_tx_evaluator_new, returnsSuccessIfGivenAValidImpl)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;

  // Act
  cardano_error_t error = cardano_tx_evaluator_new(cardano_tx_evaluator_impl_new(), &tx_evaluator);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_get_name, returnsEmptyStringIfGivenANullPtr)
{
  // Act
  const char* name = cardano_tx_evaluator_get_name(nullptr);

  // Assert
  EXPECT_STREQ(name, "");
}

TEST(cardano_tx_evaluator_get_name, returnsTheNameOfTheCoinSelector)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  cardano_error_t         error        = cardano_tx_evaluator_new(cardano_empty_tx_evaluator_impl_new(), &tx_evaluator);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const char* name = cardano_tx_evaluator_get_name(tx_evaluator);

  // Assert
  EXPECT_STREQ(name, "Empty Coin Selector");

  // Cleanup
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_select, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;

  // Act
  cardano_error_t error = cardano_tx_evaluator_evaluate(tx_evaluator, nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_tx_evaluator_select, returnsErrorIfSelectIsNotImplemented)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  cardano_error_t         error        = cardano_tx_evaluator_new(cardano_empty_tx_evaluator_impl_new(), &tx_evaluator);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemers = nullptr;

  // Act
  error = cardano_tx_evaluator_evaluate(tx_evaluator, (cardano_transaction_t*)"", (cardano_utxo_list_t*)"", &redeemers);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_select, returnsSuccessIfSelectIsImplemented)
{
  // Arrange
  cardano_tx_evaluator_t* tx_evaluator = nullptr;
  cardano_error_t         error        = cardano_tx_evaluator_new(cardano_tx_evaluator_impl_new(), &tx_evaluator);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_redeemer_list_t* redeemers = nullptr;

  // Act
  error = cardano_tx_evaluator_evaluate(tx_evaluator, (cardano_transaction_t*)"", (cardano_utxo_list_t*)"", &redeemers);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_evaluator_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_evaluator_t* tx_evaluator = nullptr;

  cardano_tx_evaluator_impl_t impl = cardano_empty_tx_evaluator_impl_new();

  // Act
  cardano_error_t error = cardano_tx_evaluator_new(impl, &tx_evaluator);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(tx_evaluator, (cardano_tx_evaluator_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_object_unref(&impl.context);
}
