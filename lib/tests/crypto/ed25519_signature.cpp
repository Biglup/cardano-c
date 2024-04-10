/**
 * \file ed25519_signature.cpp
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
#include <cardano/crypto/ed25519_signature.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

const size_t SIGNATURE_SIZE = 64;
const char*  SIGNATURE_HEX  = "2fa3f686df876995167e7c2e5d74c4c7b6e48f8068fe0e44208344d480f7904c36963e44115fe3eb2a3ac8694c28bcb4f5a0f3276f2e79487d8219057a506e4b";

// clang-format off
const byte_t SIGNATURE_BYTES[SIGNATURE_SIZE] = {
  0x2f, 0xa3, 0xf6, 0x86, 0xdf, 0x87, 0x69, 0x95, 0x16, 0x7e, 0x7c, 0x2e, 0x5d, 0x74, 0xc4, 0xc7,
  0xb6, 0xe4, 0x8f, 0x80, 0x68, 0xfe, 0x0e, 0x44, 0x20, 0x83, 0x44, 0xd4, 0x80, 0xf7, 0x90, 0x4c,
  0x36, 0x96, 0x3e, 0x44, 0x11, 0x5f, 0xe3, 0xeb, 0x2a, 0x3a, 0xc8, 0x69, 0x4c, 0x28, 0xbc, 0xb4,
  0xf5, 0xa0, 0xf3, 0x27, 0x6f, 0x2e, 0x79, 0x48, 0x7d, 0x82, 0x19, 0x05, 0x7a, 0x50, 0x6e, 0x4b
};

// clang-format on

/* UNIT TESTS ****************************************************************/

TEST(cardano_ed25519_signature_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_signature_ref(signature);

  // Assert
  EXPECT_THAT(signature, testing::Not((cardano_ed25519_signature_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_signature_refcount(signature), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_ed25519_signature_unref(&signature);
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ed25519_signature_ref(nullptr);
}

TEST(cardano_ed25519_signature_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;

  // Act
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_ed25519_signature_unref((cardano_ed25519_signature_t**)nullptr);
}

TEST(cardano_ed25519_signature_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_signature_ref(signature);
  size_t ref_count = cardano_ed25519_signature_refcount(signature);

  cardano_ed25519_signature_unref(&signature);
  size_t updated_ref_count = cardano_ed25519_signature_refcount(signature);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_ed25519_signature_ref(signature);
  size_t ref_count = cardano_ed25519_signature_refcount(signature);

  cardano_ed25519_signature_unref(&signature);
  size_t updated_ref_count = cardano_ed25519_signature_refcount(signature);

  cardano_ed25519_signature_unref(&signature);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_THAT(cardano_ed25519_signature_move(signature), testing::Not((cardano_ed25519_signature_t*)nullptr));
  size_t ref_count = cardano_ed25519_signature_refcount(signature);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(signature, testing::Not((cardano_ed25519_signature_t*)nullptr));

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_ed25519_signature_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_ed25519_signature_move, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_ed25519_signature_t* signature = cardano_ed25519_signature_move(nullptr);

  // Assert
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);
}

TEST(cardano_ed25519_signature_from_bytes, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(nullptr, 0, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);
}

TEST(cardano_ed25519_signature_from_bytes, returnsErrorIfGivenZeroLength)
{
  // Act
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, 0, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);
}

TEST(cardano_ed25519_signature_from_bytes, returnsNullIfSignatureIsNull)
{
  // Act
  cardano_error_t error = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ed25519_signature_from_bytes, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_signature_from_bytes, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_signature_from_hex, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_hex(nullptr, 0, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);
}

TEST(cardano_ed25519_signature_from_hex, returnsNullIfsignatureIsNull)
{
  // Act
  cardano_error_t error = cardano_ed25519_signature_from_hex(SIGNATURE_HEX, SIGNATURE_SIZE * 2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ed25519_signature_from_hex, returnsErrorIfGivenZeroLength)
{
  // Act
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_hex(SIGNATURE_HEX, 0, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ED25519_SIGNATURE_SIZE);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);
}

TEST(cardano_ed25519_signature_from_hex, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_signature_from_hex(SIGNATURE_HEX, SIGNATURE_SIZE * 2, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_signature_from_hex, returnsNullIfMemoryEventualAllocationFails)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_ed25519_signature_from_hex(SIGNATURE_HEX, SIGNATURE_SIZE * 2, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(signature, (cardano_ed25519_signature_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_ed25519_signature_from_hex, returnssignatureObjectWithsignatureBytes)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_hex(SIGNATURE_HEX, SIGNATURE_SIZE * 2, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(signature, testing::Not((cardano_ed25519_signature_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_signature_get_bytes_size(signature), SIGNATURE_SIZE);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_from_bytes, returnssignatureObjectWithsignatureBytes)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(signature, testing::Not((cardano_ed25519_signature_t*)nullptr));
  EXPECT_EQ(cardano_ed25519_signature_get_bytes_size(signature), SIGNATURE_SIZE);

  const byte_t* signature_data = cardano_ed25519_signature_get_data(signature);

  for (size_t i = 0; i < SIGNATURE_SIZE; i++)
  {
    EXPECT_EQ(signature_data[i], SIGNATURE_BYTES[i]);
  }

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_to_bytes, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;

  // Act
  cardano_error_t error = cardano_ed25519_signature_to_bytes(signature, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_ed25519_signature_to_bytes, returnsErrorIfBufferLengthIsZero)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_signature_to_bytes(signature, (byte_t*)"data", 0);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_to_bytes, returnsErrorIfsignatureLengthIsGreaterThanBufferLength)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  // Act
  error = cardano_ed25519_signature_to_bytes(signature, (byte_t*)"data", 3);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_to_bytes, returnsErrorIfsignatureLengthIsZero)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);

  ASSERT_EQ(error, CARDANO_SUCCESS);
  byte_t buffer[SIGNATURE_SIZE] = { 0 };

  // Act
  error = cardano_ed25519_signature_to_bytes(signature, &buffer[0], 0);

  // Assert
  EXPECT_EQ(error, CARDANO_OUT_OF_BOUNDS_MEMORY_WRITE);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_to_bytes, returnsErrorIfSignatureIsNull)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_ed25519_signature_to_bytes(signature, nullptr, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_to_bytes, returnsSignatureBytes)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_bytes(SIGNATURE_BYTES, SIGNATURE_SIZE, &signature);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  byte_t buffer[SIGNATURE_SIZE] = { 0 };

  // Act
  error = cardano_ed25519_signature_to_bytes(signature, buffer, SIGNATURE_SIZE);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const byte_t* signature_data = cardano_ed25519_signature_get_data(signature);

  for (size_t i = 0; i < 4; i++)
  {
    EXPECT_EQ(buffer[i], signature_data[i]);
  }

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}

TEST(cardano_ed25519_signature_to_hex, returnsSignatureHex)
{
  // Arrange
  cardano_ed25519_signature_t* signature = nullptr;
  cardano_error_t              error     = cardano_ed25519_signature_from_hex(SIGNATURE_HEX, SIGNATURE_SIZE * 2, &signature);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  char buffer[SIGNATURE_SIZE * 2] = { 0 };

  // Act
  error = cardano_ed25519_signature_to_hex(signature, &buffer[0], (SIGNATURE_SIZE * 2) + 1);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_signature_get_hex_size(signature), (SIGNATURE_SIZE * 2) + 1);

  EXPECT_STREQ(buffer, SIGNATURE_HEX);

  // Cleanup
  cardano_ed25519_signature_unref(&signature);
}