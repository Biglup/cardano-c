/**
 * \file ed25519_public_key.cpp
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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

#include <cardano/error.h>

#include <cardano/buffer.h>
#include <cardano/crypto/ed25519_public_key.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include "ed25519_vectors.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const size_t PUBLIC_KEY_SIZE = 32;
static const char*  PUBLIC_KEY_HEX  = "2fa3f686df876995167e7c2e5d74c4c7b6e48f8068fe0e44208344d480f7904c";

// clang-format off
static const byte_t PUBLIC_KEY[PUBLIC_KEY_SIZE] = {
  0x2f, 0xa3, 0xf6, 0x86, 0xdf, 0x87, 0x69, 0x95,
  0x16, 0x7e, 0x7c, 0x2e, 0x5d, 0x74, 0xc4, 0xc7,
  0xb6, 0xe4, 0x8f, 0x80, 0x68, 0xfe, 0x0e, 0x44,
  0x20, 0x83, 0x44, 0xd4, 0x80, 0xf7, 0x90, 0x4c
};

// clang-format on

/* UNIT TESTS ****************************************************************/

TEST(cardano_ed25519_public_key_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_public_key_ref(public_key);

  // Assert
  EXPECT_THAT(public_key, testing::Not((cardano_ed25519_public_key_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_public_key_refcount(public_key), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ed25519_public_key_ref(nullptr);
}

TEST(cardano_ed25519_public_key_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;

  // Act
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ed25519_public_key_unref((cardano_ed25519_public_key_t**)nullptr);
}

TEST(cardano_ed25519_public_key_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_public_key_ref(public_key);
  size_t ref_count = cardano_ed25519_public_key_refcount(public_key);

  cardano_ed25519_public_key_unref(&public_key);
  size_t updated_ref_count = cardano_ed25519_public_key_refcount(public_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_public_key_ref(public_key);
  size_t ref_count = cardano_ed25519_public_key_refcount(public_key);

  cardano_ed25519_public_key_unref(&public_key);
  size_t updated_ref_count = cardano_ed25519_public_key_refcount(public_key);

  cardano_ed25519_public_key_unref(&public_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_ed25519_public_key_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_ed25519_public_key_from_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(nullptr, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);
}

TEST(cardano_ed25519_public_key_from_bytes, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);
}

TEST(cardano_ed25519_public_key_from_bytes, returnsNullIfPublic_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ed25519_public_key_from_bytes, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_public_key_from_bytes, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_public_key_from_hex, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_hex(nullptr, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);
}

TEST(cardano_ed25519_public_key_from_hex, returnsNullIfpublic_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_ed25519_public_key_from_hex(PUBLIC_KEY_HEX, PUBLIC_KEY_SIZE * 2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ed25519_public_key_from_hex, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_hex(PUBLIC_KEY_HEX, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ED25519_PUBLIC_KEY_SIZE);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);
}

TEST(cardano_ed25519_public_key_from_hex, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_public_key_from_hex(PUBLIC_KEY_HEX, PUBLIC_KEY_SIZE * 2, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_public_key_from_hex, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_public_key_from_hex(PUBLIC_KEY_HEX, PUBLIC_KEY_SIZE * 2, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_public_key_from_hex, returnsPublicKeyObjectWithPublicKeyBytes)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_hex(PUBLIC_KEY_HEX, PUBLIC_KEY_SIZE * 2, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(public_key, testing::Not((cardano_ed25519_public_key_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_public_key_get_bytes_size(public_key), PUBLIC_KEY_SIZE);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_from_bytes, returnsPublicKeyObjectWithPublicKeyBytes)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(public_key, testing::Not((cardano_ed25519_public_key_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_public_key_get_bytes_size(public_key), PUBLIC_KEY_SIZE);

  const byte_t* public_key_data = cardano_ed25519_public_key_get_data(public_key);

  for (size_t i = 0; i < PUBLIC_KEY_SIZE; i++)
  {
    EXPECT_EQ(public_key_data[i], PUBLIC_KEY[i]);
  }

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;

  // Act
  cardano_error_t error = cardano_ed25519_public_key_to_bytes(public_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ed25519_public_key_to_bytes, returnsErrorIfBufferLengthIsZero)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_public_key_to_bytes(public_key, (byte_t*)"data", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_to_bytes, returnsErrorIfPublicKeyLengthIsGreaterThanBufferLength)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  // Act
  error = cardano_ed25519_public_key_to_bytes(public_key, (byte_t*)"data", 3);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_to_bytes, returnsErrorIfPublicKeyLengthIsZero)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t* buffer = nullptr;

  // Act
  error = cardano_ed25519_public_key_to_bytes(public_key, buffer, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_to_bytes, returnsErrorIfPublicKeyIsNull)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_public_key_to_bytes(public_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_to_bytes, returnsPublicKeyBytes)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_bytes(PUBLIC_KEY, PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t buffer[PUBLIC_KEY_SIZE] = { 0 };

  // Act
  error = cardano_ed25519_public_key_to_bytes(public_key, buffer, PUBLIC_KEY_SIZE);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const byte_t* public_key_data = cardano_ed25519_public_key_get_data(public_key);

  for (size_t i = 0; i < PUBLIC_KEY_SIZE; i++)
  {
    EXPECT_EQ(buffer[i], public_key_data[i]);
  }

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_key_to_hex, returnsPublicKeyHex)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_hex(PUBLIC_KEY_HEX, PUBLIC_KEY_SIZE * 2, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  char buffer[(PUBLIC_KEY_SIZE * 2) + 1] = { 0 };

  // Act
  error = cardano_ed25519_public_key_to_hex(public_key, &buffer[0], (PUBLIC_KEY_SIZE * 2) + 1);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_public_key_get_hex_size(public_key), (PUBLIC_KEY_SIZE * 2) + 1);

  EXPECT_STREQ(buffer, PUBLIC_KEY_HEX);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_verify, returnsFalseIfPublicKeyIsNull)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_ed25519_signature_t*  signature  = nullptr;

  // Act
  bool result = cardano_ed25519_public_verify(public_key, signature, PUBLIC_KEY, PUBLIC_KEY_SIZE);

  // Assert
  EXPECT_EQ(result, false);
}

TEST(cardano_ed25519_public_verify, returnsFalseIfSignatureIsNull)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_hex(PUBLIC_KEY_HEX, PUBLIC_KEY_SIZE * 2, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;

  // Act
  bool result = cardano_ed25519_public_verify(public_key, signature, PUBLIC_KEY, PUBLIC_KEY_SIZE);

  // Assert
  EXPECT_EQ(result, false);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_public_verify, canVerifyASignatureGivenTheRightPublicKeyAndOriginalMessage)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_hex(
    cardano_ed25519_test_vectors[1].public_key,
    strlen(cardano_ed25519_test_vectors[1].public_key),
    &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;
  error                                  = cardano_ed25519_signature_from_hex(
    cardano_ed25519_test_vectors[1].signature,
    strlen(cardano_ed25519_test_vectors[1].signature),
    &signature);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* message = cardano_buffer_from_hex(
    cardano_ed25519_test_vectors[1].message,
    strlen(cardano_ed25519_test_vectors[1].message));

  // Act
  const bool result = cardano_ed25519_public_verify(public_key, signature, cardano_buffer_get_data(message), cardano_buffer_get_size(message));

  // Assert
  EXPECT_EQ(result, true);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_signature_unref(&signature);
  cardano_buffer_unref(&message);
}

TEST(cardano_ed25519_public_verify, canNotVerifyASignatureGivenTheWrongPublicKeyAndOriginalMessage)
{
  // Arrange
  cardano_ed25519_public_key_t* public_key = nullptr;
  cardano_error_t               error      = cardano_ed25519_public_key_from_hex(
    cardano_ed25519_test_vectors[0].public_key,
    strlen(cardano_ed25519_test_vectors[0].public_key),
    &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;
  error                                  = cardano_ed25519_signature_from_hex(
    cardano_ed25519_test_vectors[1].signature,
    strlen(cardano_ed25519_test_vectors[1].signature),
    &signature);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* message = cardano_buffer_from_hex(
    cardano_ed25519_test_vectors[1].message,
    strlen(cardano_ed25519_test_vectors[1].message));

  // Act
  const bool result = cardano_ed25519_public_verify(public_key, signature, cardano_buffer_get_data(message), cardano_buffer_get_size(message));

  // Assert
  EXPECT_EQ(result, false);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_signature_unref(&signature);
  cardano_buffer_unref(&message);
}