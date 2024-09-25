/**
 * \file bip32_private_key.cpp
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
#include <cardano/crypto/ed25519_private_key.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include "ed25519_vectors.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const size_t BIP32_PRIVATE_KEY_SIZE = 96;
static const char*  BIP32_PRIVATE_KEY_HEX  = "a0ab55b174ba8cd95e2362d035f377b4dc779a0fae65767e3b8dd790fa748250f3ef2cc372c207d7902607ffef01872a4c785cd27e7342de7f4332f2d5fdc3a8d0c110e1d6a061d3558eb6a3138a3982253c6616e1bf4d8bd31e92de8328affe";

static const size_t BIP39_ENTROPY_SIZE  = 32;
static const size_t BIP39_PASSWORD_SIZE = 20;

// clang-format off

static const byte_t BIP39_PASSWORD[] = {
  0x73, 0x6f, 0x6d, 0x65, 0x5f, 0x70, 0x61, 0x73,
  0x73, 0x77, 0x6f, 0x72, 0x64, 0x5f, 0x40, 0x23,
  0x24, 0x25, 0x5e, 0x26
};

static const byte_t  BIP39_ENTROPY[] = {
  0xca, 0xec, 0x96, 0xd0, 0x9f, 0xc2, 0x02, 0x0a,
  0xb2, 0x30, 0x19, 0x9e, 0x01, 0x88, 0xcd, 0x6a,
  0x55, 0x4e, 0x2d, 0xa2, 0xcb, 0xa3, 0x2d, 0xe9,
  0xff, 0x6c, 0x09, 0x08, 0xc7, 0xf0, 0x4d, 0x65
};

static const byte_t BIP32_PRIVATE_KEY[BIP32_PRIVATE_KEY_SIZE] = {
  0xa0, 0xab, 0x55, 0xb1, 0x74, 0xba, 0x8c, 0xd9,
  0x5e, 0x23, 0x62, 0xd0, 0x35, 0xf3, 0x77, 0xb4,
  0xdc, 0x77, 0x9a, 0x0f, 0xae, 0x65, 0x76, 0x7e,
  0x3b, 0x8d, 0xd7, 0x90, 0xfa, 0x74, 0x82, 0x50,
  0xf3, 0xef, 0x2c, 0xc3, 0x72, 0xc2, 0x07, 0xd7,
  0x90, 0x26, 0x07, 0xff, 0xef, 0x01, 0x87, 0x2a,
  0x4c, 0x78, 0x5c, 0xd2, 0x7e, 0x73, 0x42, 0xde,
  0x7f, 0x43, 0x32, 0xf2, 0xd5, 0xfd, 0xc3, 0xa8,
  0xd0, 0xc1, 0x10, 0xe1, 0xd6, 0xa0, 0x61, 0xd3,
  0x55, 0x8e, 0xb6, 0xa3, 0x13, 0x8a, 0x39, 0x82,
  0x25, 0x3c, 0x66, 0x16, 0xe1, 0xbf, 0x4d, 0x8b,
  0xd3, 0x1e, 0x92, 0xde, 0x83, 0x28, 0xaf, 0xfe
};

static const byte_t ED25519_PRIVATE_KEY[64] = {
  0xa0, 0xab, 0x55, 0xb1, 0x74, 0xba, 0x8c, 0xd9,
  0x5e, 0x23, 0x62, 0xd0, 0x35, 0xf3, 0x77, 0xb4,
  0xdc, 0x77, 0x9a, 0x0f, 0xae, 0x65, 0x76, 0x7e,
  0x3b, 0x8d, 0xd7, 0x90, 0xfa, 0x74, 0x82, 0x50,
  0xf3, 0xef, 0x2c, 0xc3, 0x72, 0xc2, 0x07, 0xd7,
  0x90, 0x26, 0x07, 0xff, 0xef, 0x01, 0x87, 0x2a,
  0x4c, 0x78, 0x5c, 0xd2, 0x7e, 0x73, 0x42, 0xde,
  0x7f, 0x43, 0x32, 0xf2, 0xd5, 0xfd, 0xc3, 0xa8
};

// clang-format on

/* UNIT TESTS ****************************************************************/

TEST(cardano_bip32_private_key_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bip32_private_key_ref(private_key);

  // Assert
  EXPECT_THAT(private_key, testing::Not((cardano_bip32_private_key_t*)nullptr));
  EXPECT_EQ(cardano_bip32_private_key_refcount(private_key), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_bip32_private_key_unref(&private_key);
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bip32_private_key_ref(nullptr);
}

TEST(cardano_bip32_private_key_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;

  // Act
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bip32_private_key_unref((cardano_bip32_private_key_t**)nullptr);
}

TEST(cardano_bip32_private_key_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bip32_private_key_ref(private_key);
  size_t ref_count = cardano_bip32_private_key_refcount(private_key);

  cardano_bip32_private_key_unref(&private_key);
  size_t updated_ref_count = cardano_bip32_private_key_refcount(private_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_bip32_private_key_ref(private_key);
  size_t ref_count = cardano_bip32_private_key_refcount(private_key);

  cardano_bip32_private_key_unref(&private_key);
  size_t updated_ref_count = cardano_bip32_private_key_refcount(private_key);

  cardano_bip32_private_key_unref(&private_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_bip32_private_key_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_bip32_private_key_from_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(nullptr, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_bytes, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_bytes, returnsNullIfprivate_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_from_bytes, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_private_key_from_bytes, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_private_key_from_hex, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(nullptr, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_hex, returnsNullIfprivate_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_from_hex, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_BIP32_PRIVATE_KEY_SIZE);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_hex, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_private_key_from_hex, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bip32_private_key_from_hex, returnsprivateKeyObjectWithprivateKeyBytes)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(private_key, testing::Not((cardano_bip32_private_key_t*)nullptr));
  EXPECT_EQ(cardano_bip32_private_key_get_bytes_size(private_key), BIP32_PRIVATE_KEY_SIZE);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_from_bytes, returnsprivateKeyObjectWithprivateKeyBytes)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(private_key, testing::Not((cardano_bip32_private_key_t*)nullptr));
  EXPECT_EQ(cardano_bip32_private_key_get_bytes_size(private_key), BIP32_PRIVATE_KEY_SIZE);

  const byte_t* private_key_data = cardano_bip32_private_key_get_data(private_key);

  for (size_t i = 0; i < BIP32_PRIVATE_KEY_SIZE; i++)
  {
    EXPECT_EQ(private_key_data[i], BIP32_PRIVATE_KEY[i]);
  }

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_private_key_to_bytes(private_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_to_bytes, returnsErrorIfBufferLengthIsZero)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bip32_private_key_to_bytes(private_key, (byte_t*)"data", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_bytes, returnsErrorIfprivateKeyLengthIsGreaterThanBufferLength)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  // Act
  error = cardano_bip32_private_key_to_bytes(private_key, (byte_t*)"data", 3);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_bytes, returnsErrorIfprivateKeyLengthIsZero)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t* buffer = nullptr;

  // Act
  error = cardano_bip32_private_key_to_bytes(private_key, buffer, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_bytes, returnsErrorIfprivateKeyIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_bip32_private_key_to_bytes(private_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_bytes, returnsprivateKeyBytes)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t buffer[BIP32_PRIVATE_KEY_SIZE] = { 0 };

  // Act
  error = cardano_bip32_private_key_to_bytes(private_key, buffer, BIP32_PRIVATE_KEY_SIZE);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const byte_t* private_key_data = cardano_bip32_private_key_get_data(private_key);

  for (size_t i = 0; i < BIP32_PRIVATE_KEY_SIZE; i++)
  {
    EXPECT_EQ(buffer[i], private_key_data[i]);
  }

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_hex, returnsprivateKeyHex)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  char buffer[(BIP32_PRIVATE_KEY_SIZE * 2) + 1] = { 0 };

  // Act
  error = cardano_bip32_private_key_to_hex(private_key, &buffer[0], (BIP32_PRIVATE_KEY_SIZE * 2) + 1);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bip32_private_key_get_hex_size(private_key), (BIP32_PRIVATE_KEY_SIZE * 2) + 1);

  EXPECT_STREQ(buffer, BIP32_PRIVATE_KEY_HEX);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_ed25519_key, returnsErrorIfprivateKeyIsNull)
{
  // Arrange
  cardano_bip32_private_key_t*   private_key = nullptr;
  cardano_ed25519_private_key_t* ed25519_key = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_private_key_to_ed25519_key(private_key, &ed25519_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_to_ed25519_key, returnsErrorIfprivateKeyIsInvalid)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bytes(BIP32_PRIVATE_KEY, BIP32_PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t ed25519_key = cardano_bip32_private_key_to_ed25519_key(private_key, nullptr);

  // Assert
  EXPECT_EQ(ed25519_key, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_to_ed25519_key, deriveCorrectEd25519Key)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_private_key_t* ed25519_key = nullptr;

  // Act
  cardano_error_t ed25519_key_error = cardano_bip32_private_key_to_ed25519_key(private_key, &ed25519_key);
  EXPECT_EQ(ed25519_key_error, CARDANO_SUCCESS);

  // Assert
  const byte_t* bip32_private_key_data = cardano_bip32_private_key_get_data(private_key);
  for (size_t i = 0; i < 32; i++)
  {
    EXPECT_EQ(bip32_private_key_data[i], ED25519_PRIVATE_KEY[i]);
  }

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
  cardano_ed25519_private_key_unref(&ed25519_key);
}

TEST(cardano_bip32_private_key_derive, returnsErrorIfPrivateKeyIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_bip32_private_key_t* derived_key = nullptr;
  const uint32_t               indices[]   = { 0, 0, 0 };

  // Act
  cardano_error_t error = cardano_bip32_private_key_derive(private_key, indices, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_derive, returnsErrorIfIndicesIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = reinterpret_cast<cardano_bip32_private_key_t*>(0x1);
  cardano_bip32_private_key_t* derived_key = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_private_key_derive(private_key, nullptr, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_derive, returnsErrorIfIndicesLengthIsZero)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = reinterpret_cast<cardano_bip32_private_key_t*>(0x1);
  cardano_bip32_private_key_t* derived_key = nullptr;
  const uint32_t               indices[]   = { 0, 0, 0 };

  // Act
  cardano_error_t error = cardano_bip32_private_key_derive(private_key, indices, 0, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_bip32_private_key_derive, returnsErrorIfDerivedKeyIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_bip32_private_key_t* derived_key = nullptr;
  const uint32_t               indices[]   = { 0, 0, 0 };

  // Act
  cardano_error_t error = cardano_bip32_private_key_derive(private_key, indices, 3, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_derive, deriveCorrectHardenedKey)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_private_key_t* derived_key = nullptr;
  const uint32_t               indices[]   = { cardano_bip32_harden(1852), cardano_bip32_harden(1815), cardano_bip32_harden(0) };

  cardano_bip32_private_key_t* expected_key   = nullptr;
  cardano_error_t              expected_error = cardano_bip32_private_key_from_hex("3809937b61bd4f180a1e9bd15237e7bc20e36b9037dd95ef60d84f6004758250a22e1bfc0d81e9adb7760bcba7f5214416b3e9f27c8d58794a3a7fead2d5b6958d515cb54181fb2f5fc3af329e80949c082fb52f7b07e359bd7835a6762148bf", 192, &expected_key);
  ASSERT_EQ(expected_error, CARDANO_SUCCESS);

  // Act
  error = cardano_bip32_private_key_derive(private_key, indices, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(derived_key, testing::Not((cardano_bip32_private_key_t*)nullptr));

  const byte_t* derived_key_data  = cardano_bip32_private_key_get_data(derived_key);
  const byte_t* expected_key_data = cardano_bip32_private_key_get_data(expected_key);

  for (size_t i = 0; i < 96; i++)
  {
    EXPECT_EQ(derived_key_data[i], expected_key_data[i]);
  }

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
  cardano_bip32_private_key_unref(&derived_key);
  cardano_bip32_private_key_unref(&expected_key);
}

TEST(cardano_bip32_private_key_derive, deriveCorrectUnHardenedKey)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex("d8287e922756977dc0b79659e6eebcae3a1fb29a22ce1449c94f125462586951390af99a0350130451e9bf4f4691f37c352dc7025d52d9132f61a82f61d3803d00b5f1652f5cbe257e567c883dc2b16e0a9568b19c5b81ea8bd197fc95e8bdcf", BIP32_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_private_key_t* derived_key = nullptr;
  const uint32_t               indices[]   = { 1852, 1815, 0 };

  cardano_bip32_private_key_t* expected_key   = nullptr;
  cardano_error_t              expected_error = cardano_bip32_private_key_from_hex("08f9d7de597d31fade994b8a1e9d3e3afe53ac8393297e8f4d96225d725869517ae54c631588abb408fcab0676a4da6b60c82b3a3d7045a26a576c7901e5e9579db12d11a3559131a47f51f854a6234725ab8767d3fcc4c9908be55508f3c712", 192, &expected_key);
  ASSERT_EQ(expected_error, CARDANO_SUCCESS);

  // Act
  error = cardano_bip32_private_key_derive(private_key, indices, 3, &derived_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(derived_key, testing::Not((cardano_bip32_private_key_t*)nullptr));

  const byte_t* derived_key_data  = cardano_bip32_private_key_get_data(derived_key);
  const byte_t* expected_key_data = cardano_bip32_private_key_get_data(expected_key);

  for (size_t i = 0; i < 96; i++)
  {
    EXPECT_EQ(derived_key_data[i], expected_key_data[i]);
  }

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
  cardano_bip32_private_key_unref(&derived_key);
  cardano_bip32_private_key_unref(&expected_key);
}

TEST(cardano_bip32_private_key_derive, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_private_key_t* derived_key = nullptr;
  const uint32_t               indices[]   = { 1852, 1815, 0 };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  error = cardano_bip32_private_key_derive(private_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_private_key_t*)nullptr);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  error = cardano_bip32_private_key_derive(private_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_private_key_t*)nullptr);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  error = cardano_bip32_private_key_derive(private_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_private_key_t*)nullptr);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  error = cardano_bip32_private_key_derive(private_key, indices, 3, &derived_key);

  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(derived_key, (cardano_bip32_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_bip32_private_key_unref(&private_key);
}

TEST(cardano_bip32_private_key_get_public_key, returnsErrorIfPrivateKeyIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_bip32_public_key_t*  public_key  = nullptr;

  // Act
  cardano_error_t error = cardano_bip32_private_key_get_public_key(private_key, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_get_public_key, returnsErrorIfPublicKeyIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = reinterpret_cast<cardano_bip32_private_key_t*>(0x1);

  // Act
  cardano_error_t error = cardano_bip32_private_key_get_public_key(private_key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_get_public_key, canComputeTheCorrectPublicKey)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_hex(BIP32_PRIVATE_KEY_HEX, BIP32_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* expected_public_key = nullptr;
  error                                           = cardano_bip32_public_key_from_hex("311f8914b8934efbe7cbb8cc4745853de12e8ea402df6f9f69b18d2792c6bed8d0c110e1d6a061d3558eb6a3138a3982253c6616e1bf4d8bd31e92de8328affe", 128, &expected_public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* public_key = nullptr;

  // Act
  error = cardano_bip32_private_key_get_public_key(private_key, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(public_key, testing::Not((cardano_bip32_public_key_t*)nullptr));

  const byte_t* public_key_data   = cardano_bip32_public_key_get_data(public_key);
  const byte_t* expected_key_data = cardano_bip32_public_key_get_data(expected_public_key);

  for (size_t i = 0; i < 64; i++)
  {
    EXPECT_EQ(public_key_data[i], expected_key_data[i]);
  }

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
  cardano_bip32_public_key_unref(&public_key);
  cardano_bip32_public_key_unref(&expected_public_key);
}

TEST(cardano_bip32_private_key_from_bip39_entropy, returnErrorIfPasswordIsNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bip39_entropy(nullptr, 0, nullptr, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_bip39_entropy, returnErrorIfPasswordLengthIsZero)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bip39_entropy(BIP39_PASSWORD, 0, BIP39_ENTROPY, BIP39_ENTROPY_SIZE, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_bip39_entropy, returnErrorIfPrivateKeyIsNull)
{
  // Arrange
  cardano_error_t error = cardano_bip32_private_key_from_bip39_entropy(BIP39_PASSWORD, BIP39_PASSWORD_SIZE, BIP39_ENTROPY, BIP39_ENTROPY_SIZE, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_bip32_private_key_from_bip39_entropy, returnErrorIfEntropyIfNull)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bip39_entropy(BIP39_PASSWORD, BIP39_PASSWORD_SIZE, nullptr, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_bip39_entropy, returnErrorIfEntropyLengthIsZero)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bip39_entropy(BIP39_PASSWORD, BIP39_PASSWORD_SIZE, BIP39_ENTROPY, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);
  EXPECT_EQ(private_key, (cardano_bip32_private_key_t*)nullptr);
}

TEST(cardano_bip32_private_key_from_bip39_entropy, computesTheRightKeyFromTheEntropyAndPassword)
{
  // Arrange
  cardano_bip32_private_key_t* private_key = nullptr;
  cardano_error_t              error       = cardano_bip32_private_key_from_bip39_entropy(BIP39_PASSWORD, BIP39_PASSWORD_SIZE, BIP39_ENTROPY, BIP39_ENTROPY_SIZE, &private_key);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_private_key_t* expected_key = nullptr;
  error                                     = cardano_bip32_private_key_from_hex("60292301b8dd20a74b58a0bd4ecdeb244a95e757c7a2d25962ada75e271d045ff827c85a5530bfe76975b4189c5fd6d32d4fe43c81373f386fde2fa0e6d0255a2ac1f1560a893ea7937c5bfbfdeab459b1a396f1174b9c5a673a640d01880c35", 192, &expected_key);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(private_key, testing::Not((cardano_bip32_private_key_t*)nullptr));

  const byte_t* private_key_data  = cardano_bip32_private_key_get_data(private_key);
  const byte_t* expected_key_data = cardano_bip32_private_key_get_data(expected_key);

  for (size_t i = 0; i < 96; i++)
  {
    EXPECT_EQ(private_key_data[i], expected_key_data[i]);
  }

  // Cleanup
  cardano_bip32_private_key_unref(&private_key);
  cardano_bip32_private_key_unref(&expected_key);
}