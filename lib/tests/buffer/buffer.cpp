/**
 * \file buffer.cpp
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
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

extern "C" {
#include "../src/buffer/buffer.h"
}

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_buffer_new, createsANewBufferWithTheGivenCapacity)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  buffer = cardano_buffer_new(1000);

  // Assert
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));
  EXPECT_EQ(cardano_buffer_get_size(buffer), 0);
  EXPECT_EQ(cardano_buffer_get_capacity(buffer), 1000);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  buffer = cardano_buffer_new(1);
  cardano_buffer_ref(buffer);

  // Assert
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));
  EXPECT_EQ(cardano_buffer_refcount(buffer), 2);

  // Cleanup
  cardano_buffer_unref(&buffer);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_buffer_unref((cardano_buffer_t**)nullptr);
}

TEST(cardano_buffer_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(1);

  // Act
  cardano_buffer_ref(buffer);
  size_t ref_count = cardano_buffer_refcount(buffer);

  cardano_buffer_unref(&buffer);
  size_t updated_ref_count = cardano_buffer_refcount(buffer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(1);

  // Act
  cardano_buffer_ref(buffer);
  size_t ref_count = cardano_buffer_refcount(buffer);

  cardano_buffer_unref(&buffer);
  size_t updated_ref_count = cardano_buffer_refcount(buffer);

  cardano_buffer_unref(&buffer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(buffer, (cardano_buffer_t*)nullptr);
}

TEST(cardano_buffer_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_new(1);

  // Act
  cardano_buffer_move(buffer);
  size_t ref_count = cardano_buffer_refcount(buffer);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_buffer_concat, lhsIsNull)
{
  // Arrange
  cardano_buffer_t* lhs = nullptr;
  cardano_buffer_t* rhs = cardano_buffer_new(1);

  // Act
  cardano_buffer_t* concatenated = cardano_buffer_concat(lhs, rhs);

  // Assert
  ASSERT_EQ(concatenated, (cardano_buffer_t*)nullptr);

  // Cleanup
  cardano_buffer_unref(&rhs);
}

TEST(cardano_buffer_concat, rhsIsNull)
{
  // Arrange
  cardano_buffer_t* lhs = cardano_buffer_new(1);
  cardano_buffer_t* rhs = nullptr;

  // Act
  cardano_buffer_t* concatenated = cardano_buffer_concat(lhs, rhs);

  // Assert
  ASSERT_EQ(concatenated, (cardano_buffer_t*)nullptr);

  // Cleanup
  cardano_buffer_unref(&lhs);
}

TEST(cardano_buffer_concat, returnsTheConcatenatedBuffer)
{
  // Arrange
  cardano_buffer_t* lhs = cardano_buffer_new(4);
  cardano_buffer_t* rhs = cardano_buffer_new(4);
  cardano_buffer_write_int32_le(lhs, 1);
  cardano_buffer_write_int32_le(rhs, 2);
  byte_t expected[8] = { 1, 0, 0, 0, 2, 0, 0, 0 };

  // Act
  cardano_buffer_t* concatenated = cardano_buffer_concat(lhs, rhs);

  // Assert
  ASSERT_EQ(cardano_buffer_refcount(concatenated), 1);
  ASSERT_THAT(expected, testing::ElementsAreArray(cardano_buffer_get_data(concatenated), cardano_buffer_get_size(concatenated)));

  // Cleanup
  cardano_buffer_unref(&lhs);
  cardano_buffer_unref(&rhs);
  cardano_buffer_unref(&concatenated);
}

/*
cardano_buffer_t* cardano_buffer_slice(const cardano_buffer_t* buffer, size_t start, size_t end);
*/