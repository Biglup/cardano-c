/**
 * \file bip32_public_key.cpp
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
#include <cardano/crypto/bip32_private_key.h>
#include <cardano/crypto/bip32_public_key.h>
#include <cardano/crypto/ed25519_public_key.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include "ed25519_vectors.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const size_t BIP32_PUBLIC_KEY_SIZE = 64;
static const char*  BIP32_PUBLIC_KEY_HEX  = "6fd8d9c696b01525cc45f15583fc9447c66e1c71fd1a11c8885368404cd0a4ab00b5f1652f5cbe257e567c883dc2b16e0a9568b19c5b81ea8bd197fc95e8bdcf";

// clang-format off
static const byte_t BIP32_PUBLIC_KEY[BIP32_PUBLIC_KEY_SIZE] = {
  0x6f, 0xd8, 0xd9, 0xc6, 0x96, 0xb0, 0x15, 0x25,
  0xcc, 0x45, 0xf1, 0x55, 0x83, 0xfc, 0x94, 0x47,
  0xc6, 0x6e, 0x1c, 0x71, 0xfd, 0x1a, 0x11, 0xc8,
  0x88, 0x53, 0x68, 0x40, 0x4c, 0xd0, 0xa4, 0xab,
  0x00, 0xb5, 0xf1, 0x65, 0x2f, 0x5c, 0xbe, 0x25,
  0x7e, 0x56, 0x7c, 0x88, 0x3d, 0xc2, 0xb1, 0x6e,
  0x0a, 0x95, 0x68, 0xb1, 0x9c, 0x5b, 0x81, 0xea,
  0x8b, 0xd1, 0x97, 0xfc, 0x95, 0xe8, 0xbd, 0xcf
};

static const byte_t ED25519_PUBLIC_KEY[32] = {
  0x6f, 0xd8, 0xd9, 0xc6, 0x96, 0xb0, 0x15, 0x25,
  0xcc, 0x45, 0xf1, 0x55, 0x83, 0xfc, 0x94, 0x47,
  0xc6, 0x6e, 0x1c, 0x71, 0xfd, 0x1a, 0x11, 0xc8,
  0x88, 0x53, 0x68, 0x40, 0x4c, 0xd0, 0xa4, 0xab
};

// clang-format on

/* UNIT TESTS ****************************************************************/

TEST(cardano_bip32_public_key_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bip32_public_key_ref(public_key);

  // Assert
  EXPECT_THAT(public_key, testing::Not((cardano_bip32_public_key_t*)nullptr));
  EXPECT_EQ(cardano_bip32_public_key_refcount(public_key), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_bip32_public_key_unref(&public_key);
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bip32_public_key_ref(nullptr);
}

TEST(cardano_bip32_public_key_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;

  // Act
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bip32_public_key_unref((cardano_bip32_public_key_t**)nullptr);
}

TEST(cardano_bip32_public_key_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bip32_public_key_ref(public_key);
  size_t ref_count = cardano_bip32_public_key_refcount(public_key);

  cardano_bip32_public_key_unref(&public_key);
  size_t updated_ref_count = cardano_bip32_public_key_refcount(public_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bip32_public_key_ref(public_key);
  size_t ref_count = cardano_bip32_public_key_refcount(public_key);

  cardano_bip32_public_key_unref(&public_key);
  size_t updated_ref_count = cardano_bip32_public_key_refcount(public_key);

  cardano_bip32_public_key_unref(&public_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_bip32_public_key_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_bip32_public_key_from_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(nullptr, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);
}

TEST(cardano_bip32_public_key_from_bytes, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);
}

TEST(cardano_bip32_public_key_from_bytes, returnsNullIfPublic_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_from_bytes, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_public_key_from_bytes, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_public_key_from_hex, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex(nullptr, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);
}

TEST(cardano_bip32_public_key_from_hex, returnsNullIfpublic_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_from_hex, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, 0, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BIP32_PUBLIC_KEY_SIZE);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);
}

TEST(cardano_bip32_public_key_from_hex, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_public_key_from_hex, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_bip32_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_public_key_from_hex, returnsPublicKeyObjectWithPublicKeyBytes)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(public_key, testing::Not((cardano_bip32_public_key_t*)nullptr));
  EXPECT_EQ(cardano_bip32_public_key_get_bytes_size(public_key), BIP32_PUBLIC_KEY_SIZE);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_from_bytes, returnsPublicKeyObjectWithPublicKeyBytes)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(public_key, testing::Not((cardano_bip32_public_key_t*)nullptr));
  EXPECT_EQ(cardano_bip32_public_key_get_bytes_size(public_key), BIP32_PUBLIC_KEY_SIZE);

  const byte_t* public_key_data = cardano_bip32_public_key_get_data(public_key);

  for (size_t i = 0; i < BIP32_PUBLIC_KEY_SIZE; i++)
  {
    EXPECT_EQ(public_key_data[i], BIP32_PUBLIC_KEY[i]);
  }

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_public_key_to_bytes(public_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_to_bytes, returnsErrorIfBufferLengthIsZero)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bip32_public_key_to_bytes(public_key, (byte_t*)"data", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_bytes, returnsErrorIfPublicKeyLengthIsGreaterThanBufferLength)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  // Act
  error = cardano_bip32_public_key_to_bytes(public_key, (byte_t*)"data", 3);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_bytes, returnsErrorIfPublicKeyLengthIsZero)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t* buffer = nullptr;

  // Act
  error = cardano_bip32_public_key_to_bytes(public_key, buffer, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_bytes, returnsErrorIfPublicKeyIsNull)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bip32_public_key_to_bytes(public_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_bytes, returnsPublicKeyBytes)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t buffer[BIP32_PUBLIC_KEY_SIZE] = { 0 };

  // Act
  error = cardano_bip32_public_key_to_bytes(public_key, buffer, BIP32_PUBLIC_KEY_SIZE);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const byte_t* public_key_data = cardano_bip32_public_key_get_data(public_key);

  for (size_t i = 0; i < BIP32_PUBLIC_KEY_SIZE; i++)
  {
    EXPECT_EQ(buffer[i], public_key_data[i]);
  }

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_hex, returnsPublicKeyHex)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  char buffer[(BIP32_PUBLIC_KEY_SIZE * 2) + 1] = { 0 };

  // Act
  error = cardano_bip32_public_key_to_hex(public_key, &buffer[0], (BIP32_PUBLIC_KEY_SIZE * 2) + 1);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bip32_public_key_get_hex_size(public_key), (BIP32_PUBLIC_KEY_SIZE * 2) + 1);

  EXPECT_STREQ(buffer, BIP32_PUBLIC_KEY_HEX);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_ed25519_key, returnsErrorIfPublicKeyIsNull)
{
  // Arrange
  cardano_bip32_public_key_t*   public_key  = nullptr;
  cardano_ed25519_public_key_t* ed25519_key = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_public_key_to_ed25519_key(public_key, &ed25519_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_to_ed25519_key, returnsErrorIfPublicKeyIsInvalid)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_bytes(BIP32_PUBLIC_KEY, BIP32_PUBLIC_KEY_SIZE, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t ed25519_key = cardano_bip32_public_key_to_ed25519_key(public_key, nullptr);

  // Assert
  EXPECT_EQ(ed25519_key, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_to_ed25519_key, deriveCorrectEd25519Key)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* ed25519_key = nullptr;

  // Act
  cardano_error_t ed25519_key_error = cardano_bip32_public_key_to_ed25519_key(public_key, &ed25519_key);
  EXPECT_EQ(ed25519_key_error, CARDANO_SUCCESS);

  // Assert
  const byte_t* bip32_public_key_data = cardano_bip32_public_key_get_data(public_key);
  for (size_t i = 0; i < 32; i++)
  {
    EXPECT_EQ(bip32_public_key_data[i], ED25519_PUBLIC_KEY[i]);
  }

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
  cardano_ed25519_public_key_unref(&ed25519_key);
}

TEST(cardano_bip32_public_key_derive, returnsErrorIfPublicKeyIsNull)
{
  // Arrange
  cardano_bip32_public_key_t* public_key  = nullptr;
  cardano_bip32_public_key_t* derived_key = nullptr;
  const uint32_t              indices[]   = { 0, 0, 0 };

  // Act
  cardano_error_t error = cardano_bip32_public_key_derive(public_key, indices, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_derive, returnsErrorIfIndicesIsNull)
{
  // Arrange
  cardano_bip32_public_key_t* public_key  = reinterpret_cast<cardano_bip32_public_key_t*>(0x1);
  cardano_bip32_public_key_t* derived_key = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_public_key_derive(public_key, nullptr, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_derive, returnsErrorIfIndicesLengthIsZero)
{
  // Arrange
  cardano_bip32_public_key_t* public_key  = reinterpret_cast<cardano_bip32_public_key_t*>(0x1);
  cardano_bip32_public_key_t* derived_key = nullptr;
  const uint32_t              indices[]   = { 0, 0, 0 };

  // Act
  cardano_error_t error = cardano_bip32_public_key_derive(public_key, indices, 0, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_bip32_public_key_derive, returnsErrorIfDerivedKeyIsNull)
{
  // Arrange
  cardano_bip32_public_key_t* public_key  = nullptr;
  cardano_bip32_public_key_t* derived_key = nullptr;
  const uint32_t              indices[]   = { 0, 0, 0 };

  // Act
  cardano_error_t error = cardano_bip32_public_key_derive(public_key, indices, 3, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_public_key_derive, deriveCorrectUnhardenedKey)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* derived_key = nullptr;
  const uint32_t              indices[]   = { 1852, 1815, 0 };

  cardano_bip32_public_key_t* expected_key   = nullptr;
  cardano_error_t             expected_error = cardano_bip32_public_key_from_hex("b857a8cd1dbbfed1824359d9d9e58bc8ffb9f66812b404f4c6ffc315629835bf9db12d11a3559131a47f51f854a6234725ab8767d3fcc4c9908be55508f3c712", 128, &expected_key);
  ASSERT_EQ(expected_error, CARDANO_SUCCESS);

  // Act
  error = cardano_bip32_public_key_derive(public_key, indices, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(derived_key, testing::Not((cardano_bip32_public_key_t*)nullptr));

  const byte_t* derived_key_data  = cardano_bip32_public_key_get_data(derived_key);
  const byte_t* expected_key_data = cardano_bip32_public_key_get_data(expected_key);

  for (size_t i = 0; i < 64; i++)
  {
    EXPECT_EQ(derived_key_data[i], expected_key_data[i]);
  }

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
  cardano_bip32_public_key_unref(&derived_key);
  cardano_bip32_public_key_unref(&expected_key);
}

TEST(cardano_bip32_public_key_derive, returnsErroWhenUsingHardenedIndices)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex("311f8914b8934efbe7cbb8cc4745853de12e8ea402df6f9f69b18d2792c6bed8d0c110e1d6a061d3558eb6a3138a3982253c6616e1bf4d8bd31e92de8328affe", BIP32_PUBLIC_KEY_SIZE * 2, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* derived_key = nullptr;
  const uint32_t              indices[]   = { cardano_bip32_harden(1852), cardano_bip32_harden(1815), cardano_bip32_harden(0) };

  // Act
  error = cardano_bip32_public_key_derive(public_key, indices, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BIP32_DERIVATION_INDEX);
  EXPECT_THAT(derived_key, nullptr);

  // Cleanup
  cardano_bip32_public_key_unref(&public_key);
  cardano_bip32_public_key_unref(&derived_key);
}

TEST(cardano_bip32_public_key_derive, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;
  cardano_error_t             error      = cardano_bip32_public_key_from_hex(BIP32_PUBLIC_KEY_HEX, BIP32_PUBLIC_KEY_SIZE * 2, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* derived_key = nullptr;
  const uint32_t              indices[]   = { 1852, 1815, 0 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  error = cardano_bip32_public_key_derive(public_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_public_key_t*)nullptr);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  error = cardano_bip32_public_key_derive(public_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_public_key_t*)nullptr);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  error = cardano_bip32_public_key_derive(public_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_public_key_t*)nullptr);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  error = cardano_bip32_public_key_derive(public_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_bip32_public_key_unref(&public_key);
}

TEST(cardano_bip32_public_key_get_data, returnsNullIfPublicKeyIsNull)
{
  // Act
  const byte_t* data = cardano_bip32_public_key_get_data(nullptr);

  // Assert
  EXPECT_EQ(data, (byte_t*)nullptr);
}

TEST(cardano_bip32_public_key_get_bytes_size, returnsZeroIfPublicKeyIsNull)
{
  // Act
  size_t size = cardano_bip32_public_key_get_bytes_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_bip32_public_key_get_hex_size, returnsZeroIfPublicKeyIsNull)
{
  // Act
  size_t size = cardano_bip32_public_key_get_hex_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_bip32_public_key_to_hex, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_bip32_public_key_t* public_key = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_public_key_to_hex(public_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}