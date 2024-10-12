/**
 * \file secure_key_handler.cpp
 *
 * \author angel.castillo
 * \date   Oct 06, 2024
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

#include <cardano/key_handlers/secure_key_handler.h>
#include <cardano/key_handlers/secure_key_handler_impl.h>

#include <cardano/key_handlers/derivation_path.h>
#include <cardano/key_handlers/secure_key_handler_type.h>
#include <cardano/object.h>
#include <gmock/gmock.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Allocates and initializes a new Cardano secure_key_handler context.
 */
typedef struct key_handler_context_t
{
    cardano_object_t base;
    char             key[256];
} ref_counted_string_t;

/**
 * \brief Allocates and initializes a new Cardano secure_key_handler context.
 */
static cardano_secure_key_handler_impl_t
cardano_secure_key_handler_impl_new()
{
  cardano_secure_key_handler_impl_t impl    = { 0 };
  key_handler_context_t*            context = reinterpret_cast<key_handler_context_t*>(malloc(sizeof(key_handler_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    CARDANO_UNUSED(memset(context->key, 0, sizeof(context->key)));
    CARDANO_UNUSED(memccpy((void*)&context->key[0], "This is a test key", strlen("This is a test key"), sizeof(context->key)));

    impl.context = (cardano_object_t*)context;
  }

  impl.type = CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519;

  impl.bip32_sign_transaction = [](
                                  cardano_secure_key_handler_impl_t*,
                                  cardano_transaction_t*,
                                  const cardano_derivation_path_t*,
                                  size_t,
                                  cardano_vkey_witness_set_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.bip32_get_extended_account_public_key = [](
                                                 cardano_secure_key_handler_impl_t*,
                                                 cardano_account_derivation_path_t,
                                                 cardano_bip32_public_key_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.ed25519_sign_transaction = [](
                                    cardano_secure_key_handler_impl_t*,
                                    cardano_transaction_t*,
                                    cardano_vkey_witness_set_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.ed25519_get_public_key = [](
                                  cardano_secure_key_handler_impl_t*,
                                  cardano_ed25519_public_key_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.serialize = [](
                     cardano_secure_key_handler_impl_t* secure_key_handler_impl,
                     cardano_buffer_t**                 serialized_data) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  return impl;
}

/**
 * \brief Allocates and initializes a new Cardano secure_key_handler context.
 */
static cardano_secure_key_handler_impl_t
cardano_empty_secure_key_handler_impl_new()
{
  cardano_secure_key_handler_impl_t impl    = { 0 };
  key_handler_context_t*            context = reinterpret_cast<key_handler_context_t*>(malloc(sizeof(key_handler_context_t)));

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

  impl.bip32_get_extended_account_public_key = NULL;
  impl.bip32_sign_transaction                = NULL;
  impl.ed25519_get_public_key                = NULL;
  impl.ed25519_sign_transaction              = NULL;

  return impl;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_secure_key_handler_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_error_t               error              = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_secure_key_handler_ref(secure_key_handler);

  // Assert
  EXPECT_THAT(secure_key_handler, testing::Not((cardano_secure_key_handler_t*)nullptr));
  EXPECT_EQ(cardano_secure_key_handler_refcount(secure_key_handler), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_secure_key_handler_unref(&secure_key_handler);
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_secure_key_handler_ref(nullptr);
}

TEST(cardano_secure_key_handler_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;

  // Act
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_secure_key_handler_unref((cardano_secure_key_handler_t**)nullptr);
}

TEST(cardano_secure_key_handler_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_error_t               error              = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_secure_key_handler_ref(secure_key_handler);
  size_t ref_count = cardano_secure_key_handler_refcount(secure_key_handler);

  cardano_secure_key_handler_unref(&secure_key_handler);
  size_t updated_ref_count = cardano_secure_key_handler_refcount(secure_key_handler);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_error_t               error              = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_secure_key_handler_ref(secure_key_handler);
  size_t ref_count = cardano_secure_key_handler_refcount(secure_key_handler);

  cardano_secure_key_handler_unref(&secure_key_handler);
  size_t updated_ref_count = cardano_secure_key_handler_refcount(secure_key_handler);

  cardano_secure_key_handler_unref(&secure_key_handler);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(secure_key_handler, (cardano_secure_key_handler_t*)nullptr);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_secure_key_handler_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_secure_key_handler_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_secure_key_handler_set_last_error(secure_key_handler, message);

  // Assert
  EXPECT_STREQ(cardano_secure_key_handler_get_last_error(secure_key_handler), "Object is NULL.");
}

TEST(cardano_secure_key_handler_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_error_t               error              = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_secure_key_handler_set_last_error(secure_key_handler, message);

  // Assert
  EXPECT_STREQ(cardano_secure_key_handler_get_last_error(secure_key_handler), "");

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_new, returnsErrorIfGivenANullPtr)
{
  cardano_secure_key_handler_impl_t impl = cardano_secure_key_handler_impl_new();
  // Act
  cardano_error_t error = cardano_secure_key_handler_new(impl, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_object_unref(&impl.context);
}

TEST(cardano_secure_key_handler_new, returnsSuccessIfGivenAValidImpl)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;

  // Act
  cardano_error_t error = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_get_name, returnsEmptyStringIfGivenANullPtr)
{
  // Act
  const char* name = cardano_secure_key_handler_get_name(nullptr);

  // Assert
  EXPECT_STREQ(name, "");
}

TEST(cardano_secure_key_handler_get_name, returnsTheNameOfTheProvider)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_error_t               error              = cardano_secure_key_handler_new(cardano_empty_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const char* name = cardano_secure_key_handler_get_name(secure_key_handler);

  // Assert
  EXPECT_STREQ(name, "Empty Provider");

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_bip32_sign_transaction, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_secure_key_handler_t*   secure_key_handler = nullptr;
  cardano_transaction_t*          tx                 = nullptr;
  const cardano_derivation_path_t path               = { .purpose = 0, .coin_type = 0, .account = 0, .role = 0, .index = 0 };
  size_t                          path_len           = 0;
  cardano_vkey_witness_set_t**    witness_set        = nullptr;

  // Act
  cardano_error_t error = cardano_secure_key_handler_bip32_sign_transaction(secure_key_handler, tx, &path, path_len, witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_secure_key_handler_bip32_sign_transaction, returnsErrorIfBip32SignTransactionIsNotImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  size_t                        path_len           = 0;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_empty_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_bip32_sign_transaction(secure_key_handler, (cardano_transaction_t*)"", (cardano_derivation_path_t*)"", path_len, (cardano_vkey_witness_set_t**)"");

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_bip32_sign_transaction, returnsSuccessIfBip32SignTransactionIsImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  size_t                        path_len           = 0;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_bip32_sign_transaction(secure_key_handler, (cardano_transaction_t*)"", (cardano_derivation_path_t*)"", path_len, (cardano_vkey_witness_set_t**)"");

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_bip32_get_extended_account_public_key, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_secure_key_handler_t*     secure_key_handler = nullptr;
  cardano_bip32_public_key_t*       bip32_public_key   = nullptr;
  cardano_account_derivation_path_t derivation_path    = { .purpose = 0, .coin_type = 0, .account = 0 };

  // Act
  cardano_error_t error = cardano_secure_key_handler_bip32_get_extended_account_public_key(secure_key_handler, derivation_path, &bip32_public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_secure_key_handler_bip32_get_extended_account_public_key, returnsErrorIfBip32GetExtendedPublicKeyIsNotImplemented)
{
  // Arrange
  cardano_secure_key_handler_t*     secure_key_handler = nullptr;
  cardano_bip32_public_key_t*       bip32_public_key   = nullptr;
  cardano_account_derivation_path_t derivation_path    = { .purpose = 0, .coin_type = 0, .account = 0 };

  cardano_error_t error = cardano_secure_key_handler_new(cardano_empty_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_bip32_get_extended_account_public_key(secure_key_handler, derivation_path, &bip32_public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_bip32_get_extended_public_key, returnsSuccessIfBip32GetExtendedPublicKeyIsImplemented)
{
  // Arrange
  cardano_secure_key_handler_t*     secure_key_handler = nullptr;
  cardano_bip32_public_key_t*       bip32_public_key   = nullptr;
  cardano_account_derivation_path_t derivation_path    = { .purpose = 0, .coin_type = 0, .account = 0 };

  cardano_error_t error = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_bip32_get_extended_account_public_key(secure_key_handler, derivation_path, &bip32_public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_bip32_public_key_unref(&bip32_public_key);
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_ed25519_sign_transaction, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_transaction_t*        tx                 = nullptr;
  cardano_vkey_witness_set_t**  witness_set        = nullptr;

  // Act
  cardano_error_t error = cardano_secure_key_handler_ed25519_sign_transaction(secure_key_handler, tx, witness_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_secure_key_handler_ed25519_sign_transaction, returnsErrorIfEd25519SignTransactionIsNotImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_empty_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_ed25519_sign_transaction(secure_key_handler, (cardano_transaction_t*)"", (cardano_vkey_witness_set_t**)"");

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_ed25519_sign_transaction, returnsSuccessIfEd25519SignTransactionIsImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_ed25519_sign_transaction(secure_key_handler, (cardano_transaction_t*)"", (cardano_vkey_witness_set_t**)"");

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_ed25519_get_public_key, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_ed25519_public_key_t* public_key         = nullptr;

  // Act
  cardano_error_t error = cardano_secure_key_handler_ed25519_get_public_key(secure_key_handler, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_secure_key_handler_ed25519_get_public_key, returnsErrorIfEd25519GetPublicKeyIsNotImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_ed25519_public_key_t* public_key         = nullptr;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_empty_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_ed25519_get_public_key(secure_key_handler, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_ed25519_get_public_key, returnsSuccessIfEd25519GetPublicKeyIsImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_ed25519_public_key_t* public_key         = nullptr;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_ed25519_get_public_key(secure_key_handler, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_secure_key_handler_t* provider = nullptr;

  cardano_secure_key_handler_impl_t impl = cardano_secure_key_handler_impl_new();

  // Act
  cardano_error_t error = cardano_secure_key_handler_new(impl, &provider);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(provider, (cardano_secure_key_handler_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_object_unref(&impl.context);
}

TEST(cardano_secure_key_handler_serialize, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_buffer_t*             serialized_data    = nullptr;

  // Act
  cardano_error_t error = cardano_secure_key_handler_serialize(secure_key_handler, &serialized_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_secure_key_handler_serialize, returnsErrorIfSerializeIsNotImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_buffer_t*             serialized_data    = nullptr;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_empty_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_serialize(secure_key_handler, &serialized_data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_NOT_IMPLEMENTED);

  // Cleanup
  cardano_secure_key_handler_unref(&secure_key_handler);
}

TEST(cardano_secure_key_handler_serialize, returnsSuccessIfSerializeIsImplemented)
{
  // Arrange
  cardano_secure_key_handler_t* secure_key_handler = nullptr;
  cardano_buffer_t*             serialized_data    = nullptr;

  cardano_error_t error = cardano_secure_key_handler_new(cardano_secure_key_handler_impl_new(), &secure_key_handler);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_secure_key_handler_serialize(secure_key_handler, &serialized_data);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_buffer_unref(&serialized_data);
  cardano_secure_key_handler_unref(&secure_key_handler);
}
