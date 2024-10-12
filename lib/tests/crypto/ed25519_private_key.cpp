/**
 * \file ed25519_private_key.cpp
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
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/crypto/ed25519_signature.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include "ed25519_vectors.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const size_t PRIVATE_KEY_SIZE = 32;
static const char*  PRIVATE_KEY_HEX  = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";
static const char*  PUBLIC_KEY_HEX   = "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a";

// clang-format off
static const byte_t PRIVATE_KEY[PRIVATE_KEY_SIZE] = {
  0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
  0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
  0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
  0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
};

// clang-format on

static const size_t PRIVATE_EXTENDED_PRIVATE_KEY_SIZE = 64;
static const char*  PRIVATE_EXTENDED_PRIVATE_HEX      = "a0ab55b174ba8cd95e2362d035f377b4dc779a0fae65767e3b8dd790fa748250f3ef2cc372c207d7902607ffef01872a4c785cd27e7342de7f4332f2d5fdc3a8";
static const char*  PRIVATE_EXTENDED_PUBLIC_HEX       = "311f8914b8934efbe7cbb8cc4745853de12e8ea402df6f9f69b18d2792c6bed8";

// clang-format off
static const byte_t EXTENDED_PRIVATE_KEY[PRIVATE_EXTENDED_PRIVATE_KEY_SIZE] = {
  0xa0, 0xab, 0x55, 0xb1, 0x74, 0xba, 0x8c, 0xd9,
  0x5e, 0x23, 0x62, 0xd0, 0x35, 0xf3, 0x77, 0xb4,
  0xdc, 0x77, 0x9a, 0x0f, 0xae, 0x65, 0x76, 0x7e,
  0x3b, 0x8d, 0xd7, 0x90, 0xfa, 0x74, 0x82, 0x50,
  0xf3, 0xef, 0x2c, 0xc3, 0x72, 0xc2, 0x07, 0xd7,
  0x90, 0x26, 0x07, 0xff, 0xef, 0x01, 0x87, 0x2a,
  0x4c, 0x78, 0x5c, 0xd2, 0x7e, 0x73, 0x42, 0xde,
  0x7f, 0x43, 0x32, 0xf2, 0xd5, 0xfd, 0xc3, 0xa8
};

static const byte_t MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES[] = {
  0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
  0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
  0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
  0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
  0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
  0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
  0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
  0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
};

static const char* SIGNATURE_VECTOR_PRIVATE_EXTENDED = "843aa4353184193bdf01aab7f636ac53f86746dd97a2a2e01fe7923c37bfec40b68a73881a26ba57dc974abc1123d0866b542a5447e03677134a8f4e1db2bc0c";

// clang-format on

/* UNIT TESTS ****************************************************************/

TEST(cardano_ed25519_private_key_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_private_key_ref(private_key);

  // Assert
  EXPECT_THAT(private_key, testing::Not((cardano_ed25519_private_key_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_private_key_refcount(private_key), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_ed25519_private_key_unref(&private_key);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ed25519_private_key_ref(nullptr);
}

TEST(cardano_ed25519_private_key_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  // Act
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ed25519_private_key_unref((cardano_ed25519_private_key_t**)nullptr);
}

TEST(cardano_ed25519_private_key_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_private_key_ref(private_key);
  size_t ref_count = cardano_ed25519_private_key_refcount(private_key);

  cardano_ed25519_private_key_unref(&private_key);
  size_t updated_ref_count = cardano_ed25519_private_key_refcount(private_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_private_key_ref(private_key);
  size_t ref_count = cardano_ed25519_private_key_refcount(private_key);

  cardano_ed25519_private_key_unref(&private_key);
  size_t updated_ref_count = cardano_ed25519_private_key_refcount(private_key);

  cardano_ed25519_private_key_unref(&private_key);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_ed25519_private_key_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_ed25519_private_key_from_normal_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(nullptr, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);
}

TEST(cardano_ed25519_private_key_from_normal_bytes, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);
}

TEST(cardano_ed25519_private_key_from_normal_bytes, returnsNullIfprivate_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ed25519_private_key_from_normal_bytes, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_private_key_from_normal_bytes, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_private_key_from_normal_hex, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_hex(nullptr, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);
}

TEST(cardano_ed25519_private_key_from_normal_hex, returnsNullIfprivate_keyIsNull)
{
  // Act
  cardano_error_t error = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, PRIVATE_KEY_SIZE * 2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ed25519_private_key_from_normal_hex, returnsNullIfGivenZeroLength)
{
  // Act
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, 0, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);
}

TEST(cardano_ed25519_private_key_from_normal_hex, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, PRIVATE_KEY_SIZE * 2, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_private_key_from_normal_hex, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, PRIVATE_KEY_SIZE * 2, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(private_key, (cardano_ed25519_private_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_private_key_from_normal_hex, returnsprivateKeyObjectWithprivateKeyBytes)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, PRIVATE_KEY_SIZE * 2, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(private_key, testing::Not((cardano_ed25519_private_key_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_private_key_get_bytes_size(private_key), PRIVATE_KEY_SIZE);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_from_normal_bytes, returnsprivateKeyObjectWithprivateKeyBytes)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(private_key, testing::Not((cardano_ed25519_private_key_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_private_key_get_bytes_size(private_key), PRIVATE_KEY_SIZE);

  const byte_t* private_key_data = cardano_ed25519_private_key_get_data(private_key);

  for (size_t i = 0; i < PRIVATE_KEY_SIZE; i++)
  {
    EXPECT_EQ(private_key_data[i], PRIVATE_KEY[i]);
  }

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  // Act
  cardano_error_t error = cardano_ed25519_private_key_to_bytes(private_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_ed25519_private_key_to_bytes, returnsErrorIfBufferLengthIsZero)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_private_key_to_bytes(private_key, (byte_t*)"data", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_to_bytes, returnsErrorIfprivateKeyLengthIsGreaterThanBufferLength)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  // Act
  error = cardano_ed25519_private_key_to_bytes(private_key, (byte_t*)"data", 3);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_to_bytes, returnsErrorIfprivateKeyLengthIsZero)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t* buffer = nullptr;

  // Act
  error = cardano_ed25519_private_key_to_bytes(private_key, buffer, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_to_bytes, returnsErrorIfprivateKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_private_key_to_bytes(private_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_to_bytes, returnsprivateKeyBytes)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t buffer[PRIVATE_KEY_SIZE] = { 0 };

  // Act
  error = cardano_ed25519_private_key_to_bytes(private_key, buffer, PRIVATE_KEY_SIZE);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const byte_t* private_key_data = cardano_ed25519_private_key_get_data(private_key);

  for (size_t i = 0; i < PRIVATE_KEY_SIZE; i++)
  {
    EXPECT_EQ(buffer[i], private_key_data[i]);
  }

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_to_hex, returnsprivateKeyHex)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  char buffer[(PRIVATE_KEY_SIZE * 2) + 1] = { 0 };

  // Act
  error = cardano_ed25519_private_key_to_hex(private_key, &buffer[0], (PRIVATE_KEY_SIZE * 2) + 1);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_private_key_get_hex_size(private_key), (PRIVATE_KEY_SIZE * 2) + 1);

  EXPECT_STREQ(buffer, PRIVATE_KEY_HEX);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_get_public_key, canComputeNonExtendedPublicKey)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* public_key = nullptr;

  // Act
  error = cardano_ed25519_private_key_get_public_key(private_key, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(public_key, testing::Not((cardano_ed25519_public_key_t*)nullptr));

  char public_key_hex[65] = { 0 };
  error                   = cardano_ed25519_public_key_to_hex(public_key, public_key_hex, 65);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(public_key_hex, PUBLIC_KEY_HEX);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_private_key_get_public_key, returnsNullIfPrivateKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_ed25519_public_key_t*  public_key  = nullptr;

  // Act
  cardano_error_t error = cardano_ed25519_private_key_get_public_key(private_key, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_private_key_get_public_key, returnsNullIfPublicKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_private_key_get_public_key(private_key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_get_public_key, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);
  cardano_ed25519_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_ed25519_private_key_get_public_key(private_key, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_get_public_key, canComputeExtendedPublicKey)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_extended_bytes(EXTENDED_PRIVATE_KEY, PRIVATE_EXTENDED_PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* public_key = nullptr;

  // Act
  error = cardano_ed25519_private_key_get_public_key(private_key, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(public_key, testing::Not((cardano_ed25519_public_key_t*)nullptr));

  char public_key_hex[65] = { 0 };
  error                   = cardano_ed25519_public_key_to_hex(public_key, public_key_hex, 65);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(public_key_hex, PRIVATE_EXTENDED_PUBLIC_HEX);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_private_key_get_public_key, returnsNullIfMemoryAllocationFailsWhenComputingFromExtended)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_extended_hex(PRIVATE_EXTENDED_PRIVATE_HEX, PRIVATE_EXTENDED_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);
  cardano_ed25519_public_key_t* public_key = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_ed25519_private_key_get_public_key(private_key, &public_key);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(public_key, (cardano_ed25519_public_key_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_sign, returnsNullIfPrivateKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_ed25519_signature_t*   signature   = nullptr;

  // Act
  cardano_error_t error = cardano_ed25519_private_key_sign(private_key, nullptr, 0, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_private_key_sign, returnsNullIfSignatureIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_bytes(PRIVATE_KEY, PRIVATE_KEY_SIZE, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_private_key_sign(private_key, nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_sign, canGenerateAValidSignatureFromAnExtendedKey)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_extended_hex(PRIVATE_EXTENDED_PRIVATE_HEX, PRIVATE_EXTENDED_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* public_key = nullptr;
  error                                    = cardano_ed25519_private_key_get_public_key(private_key, &public_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;

  // Act
  error = cardano_ed25519_private_key_sign(private_key, &MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES[0], sizeof(MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES), &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(signature, testing::Not((cardano_ed25519_signature_t*)nullptr));

  char signature_hex[129] = { 0 };
  error                   = cardano_ed25519_signature_to_hex(signature, signature_hex, 129);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(signature_hex, SIGNATURE_VECTOR_PRIVATE_EXTENDED);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
  cardano_ed25519_private_key_unref(&private_key);
  cardano_ed25519_public_key_unref(&public_key);
}

TEST(cardano_ed25519_private_key_sign, canGenerateAValidSignatureFromANonExtendedKey)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_hex("c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7", 64, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;

  const byte_t message[] = { 0xaf, 0x82 };

  // Act
  error = cardano_ed25519_private_key_sign(private_key, &message[0], sizeof(message), &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(signature, testing::Not((cardano_ed25519_signature_t*)nullptr));

  char signature_hex[129] = { 0 };
  error                   = cardano_ed25519_signature_to_hex(signature, signature_hex, 129);
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(signature_hex, "6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a");

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_sign, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_extended_hex(PRIVATE_EXTENDED_PRIVATE_HEX, PRIVATE_EXTENDED_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_ed25519_private_key_sign(private_key, &MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES[0], sizeof(MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES), &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_sign, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_extended_hex(PRIVATE_EXTENDED_PRIVATE_HEX, PRIVATE_EXTENDED_PRIVATE_KEY_SIZE * 2, &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  error = cardano_ed25519_private_key_sign(private_key, &MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES[0], sizeof(MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES), &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_sign, returnsNullIfMemoryAllocationFailsNormalKey)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, strlen(PRIVATE_KEY_HEX), &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_ed25519_private_key_sign(private_key, &MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES[0], sizeof(MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES), &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_sign, returnsNullIfEventualMemoryAllocationFailsNormalKey)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;
  cardano_error_t                error       = cardano_ed25519_private_key_from_normal_hex(PRIVATE_KEY_HEX, strlen(PRIVATE_KEY_HEX), &private_key);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  error = cardano_ed25519_private_key_sign(private_key, &MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES[0], sizeof(MESSAGE_VECTOR_PRIVATE_EXTENDED_BYTES), &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_ed25519_private_key_get_data, returnsNullIfPrivateKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  // Act
  const byte_t* private_key_data = cardano_ed25519_private_key_get_data(private_key);

  // Assert
  EXPECT_EQ(private_key_data, (const byte_t*)nullptr);
}

TEST(cardano_ed25519_private_key_get_bytes_size, returnsZeroIfPrivateKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  // Act
  size_t private_key_size = cardano_ed25519_private_key_get_bytes_size(private_key);

  // Assert
  EXPECT_EQ(private_key_size, 0);
}

TEST(cardano_ed25519_private_key_get_hex_size, returnsZeroIfPrivateKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  // Act
  size_t private_key_size = cardano_ed25519_private_key_get_hex_size(private_key);

  // Assert
  EXPECT_EQ(private_key_size, 0);
}

TEST(cardano_ed25519_private_key_to_hex, returnsNullIfPrivateKeyIsNull)
{
  // Arrange
  cardano_ed25519_private_key_t* private_key = nullptr;

  // Act
  cardano_error_t error = cardano_ed25519_private_key_to_hex(private_key, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}