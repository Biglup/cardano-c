/**
 * \file coin_selector.cpp
 *
 * \author angel.castillo
 * \date   Oct 14, 2024
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

#include <cardano/transaction_builder/coin_selection/coin_selector.h>

#include <gmock/gmock.h>
#include <string_safe.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Allocates and initializes a new Cardano coin_selector context.
 */
typedef struct coin_selector_context_t
{
    cardano_object_t base;
} ref_counted_string_t;

/**
 * \brief Allocates and initializes a new Cardano coin_selector context.
 */
static cardano_coin_selector_impl_t
cardano_coin_selector_impl_new()
{
  cardano_coin_selector_impl_t impl    = { 0 };
  coin_selector_context_t*     context = reinterpret_cast<coin_selector_context_t*>(malloc(sizeof(coin_selector_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    impl.context = (cardano_object_t*)context;
  }

  impl.select = [](
                  cardano_coin_selector_impl_t* self,
                  cardano_utxo_list_t*          pre_selected_utxo,
                  cardano_utxo_list_t*          available_utxo,
                  cardano_value_t*              target,
                  cardano_utxo_list_t**         selection,
                  cardano_utxo_list_t**         remaining_utxo) -> cardano_error_t
  {
    cardano_error_t result = cardano_utxo_list_new(selection);
    CARDANO_UNUSED(result);

    result = cardano_utxo_list_new(remaining_utxo);
    CARDANO_UNUSED(result);

    CARDANO_UNUSED(pre_selected_utxo);
    CARDANO_UNUSED(available_utxo);
    CARDANO_UNUSED(target);
    CARDANO_UNUSED(self);

    return CARDANO_SUCCESS;
  };

  return impl;
}

/**
 * \brief Allocates and initializes a new Cardano coin_selector context.
 */
static cardano_coin_selector_impl_t
cardano_empty_coin_selector_impl_new()
{
  cardano_coin_selector_impl_t impl    = { 0 };
  coin_selector_context_t*     context = reinterpret_cast<coin_selector_context_t*>(malloc(sizeof(coin_selector_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    impl.context = (cardano_object_t*)context;
  }

  CARDANO_UNUSED(memset(impl.name, 0, sizeof(impl.name)));
  CARDANO_UNUSED(cardano_safe_memcpy((void*)&impl.name[0], sizeof(impl.name), "Empty Coin Selector", sizeof(impl.name)));

  impl.select = NULL;

  return impl;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_coin_selector_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  cardano_error_t          error         = cardano_coin_selector_new(cardano_coin_selector_impl_new(), &coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_coin_selector_ref(coin_selector);

  // Assert
  EXPECT_THAT(coin_selector, testing::Not((cardano_coin_selector_t*)nullptr));
  EXPECT_EQ(cardano_coin_selector_refcount(coin_selector), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_coin_selector_unref(&coin_selector);
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_coin_selector_ref(nullptr);
}

TEST(cardano_coin_selector_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;

  // Act
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_coin_selector_unref((cardano_coin_selector_t**)nullptr);
}

TEST(cardano_coin_selector_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  cardano_error_t          error         = cardano_coin_selector_new(cardano_coin_selector_impl_new(), &coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_coin_selector_ref(coin_selector);
  size_t ref_count = cardano_coin_selector_refcount(coin_selector);

  cardano_coin_selector_unref(&coin_selector);
  size_t updated_ref_count = cardano_coin_selector_refcount(coin_selector);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  cardano_error_t          error         = cardano_coin_selector_new(cardano_coin_selector_impl_new(), &coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_coin_selector_ref(coin_selector);
  size_t ref_count = cardano_coin_selector_refcount(coin_selector);

  cardano_coin_selector_unref(&coin_selector);
  size_t updated_ref_count = cardano_coin_selector_refcount(coin_selector);

  cardano_coin_selector_unref(&coin_selector);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(coin_selector, (cardano_coin_selector_t*)nullptr);

  // Cleanup
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_coin_selector_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_coin_selector_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  const char*              message       = "This is a test message";

  // Act
  cardano_coin_selector_set_last_error(coin_selector, message);

  // Assert
  EXPECT_STREQ(cardano_coin_selector_get_last_error(coin_selector), "Object is NULL.");
}

TEST(cardano_coin_selector_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  cardano_error_t          error         = cardano_coin_selector_new(cardano_coin_selector_impl_new(), &coin_selector);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_coin_selector_set_last_error(coin_selector, message);

  // Assert
  EXPECT_STREQ(cardano_coin_selector_get_last_error(coin_selector), "");

  // Cleanup
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_new, returnsErrorIfGivenANullPtr)
{
  cardano_coin_selector_impl_t impl = cardano_coin_selector_impl_new();
  // Act
  cardano_error_t error = cardano_coin_selector_new(impl, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_object_unref(&impl.context);
}

TEST(cardano_coin_selector_new, returnsSuccessIfGivenAValidImpl)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;

  // Act
  cardano_error_t error = cardano_coin_selector_new(cardano_coin_selector_impl_new(), &coin_selector);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_get_name, returnsEmptyStringIfGivenANullPtr)
{
  // Act
  const char* name = cardano_coin_selector_get_name(nullptr);

  // Assert
  EXPECT_STREQ(name, "");
}

TEST(cardano_coin_selector_get_name, returnsTheNameOfTheCoinSelector)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  cardano_error_t          error         = cardano_coin_selector_new(cardano_empty_coin_selector_impl_new(), &coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const char* name = cardano_coin_selector_get_name(coin_selector);

  // Assert
  EXPECT_STREQ(name, "Empty Coin Selector");

  // Cleanup
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_select, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_coin_selector_t* coin_selector     = nullptr;
  cardano_utxo_list_t*     pre_selected_utxo = nullptr;
  cardano_utxo_list_t*     available_utxo    = nullptr;
  cardano_value_t*         target            = nullptr;
  cardano_utxo_list_t*     selection         = nullptr;
  cardano_utxo_list_t*     remaining_utxo    = nullptr;

  // Act
  cardano_error_t error = cardano_coin_selector_select(coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_coin_selector_select, returnsErrorIfSelectIsNotImplemented)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  cardano_error_t          error         = cardano_coin_selector_new(cardano_empty_coin_selector_impl_new(), &coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* pre_selected_utxo = (cardano_utxo_list_t*)"";
  cardano_utxo_list_t* available_utxo    = (cardano_utxo_list_t*)"";
  cardano_value_t*     target            = (cardano_value_t*)"";
  cardano_utxo_list_t* selection         = nullptr;
  cardano_utxo_list_t* remaining_utxo    = nullptr;

  // Act
  error = cardano_coin_selector_select(coin_selector, pre_selected_utxo, available_utxo, target, &selection, &remaining_utxo);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_select, returnsSuccessIfSelectIsImplemented)
{
  // Arrange
  cardano_coin_selector_t* coin_selector = nullptr;
  cardano_error_t          error         = cardano_coin_selector_new(cardano_coin_selector_impl_new(), &coin_selector);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_list_t* selection      = nullptr;
  cardano_utxo_list_t* remaining_utxo = nullptr;

  // Act
  error = cardano_coin_selector_select(coin_selector, nullptr, (cardano_utxo_list_t*)"", (cardano_value_t*)"", &selection, &remaining_utxo);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_utxo_list_unref(&selection);
  cardano_utxo_list_unref(&remaining_utxo);
  cardano_coin_selector_unref(&coin_selector);
}

TEST(cardano_coin_selector_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_coin_selector_t* coin_selector = nullptr;

  cardano_coin_selector_impl_t impl = cardano_empty_coin_selector_impl_new();

  // Act
  cardano_error_t error = cardano_coin_selector_new(impl, &coin_selector);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(coin_selector, (cardano_coin_selector_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_object_unref(&impl.context);
}
