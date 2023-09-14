/**
 * \file cbor_writer.cpp
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

#include <cardano/cbor/cbor_writer.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_writer_new, createsANewObjectWithRefCountOne)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  // Act
  writer = cardano_cbor_writer_new();

  // Assert
  EXPECT_THAT(writer, testing::Not((cardano_cbor_writer_t*)nullptr));
  EXPECT_EQ(cardano_cbor_writer_refcount(writer), 1);
}

TEST(cardano_cbor_writer_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  // Act
  writer = cardano_cbor_writer_new();
  cardano_cbor_writer_ref(writer);

  // Assert
  EXPECT_THAT(writer, testing::Not((cardano_cbor_writer_t*)nullptr));
  EXPECT_EQ(cardano_cbor_writer_refcount(writer), 2);
}

TEST(cardano_cbor_writer_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = nullptr;

  // Act
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cbor_writer_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cbor_writer_unref((cardano_cbor_writer_t**)nullptr);
}

TEST(cardano_cbor_writer_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ;

  // Act
  cardano_cbor_writer_ref(writer);
  size_t ref_count = cardano_cbor_writer_refcount(writer);

  cardano_cbor_writer_unref(&writer);
  size_t updated_ref_count = cardano_cbor_writer_refcount(writer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
}

TEST(cardano_cbor_writer_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ;

  // Act
  cardano_cbor_writer_ref(writer);
  size_t ref_count = cardano_cbor_writer_refcount(writer);

  cardano_cbor_writer_unref(&writer);
  size_t updated_ref_count = cardano_cbor_writer_refcount(writer);

  cardano_cbor_writer_unref(&writer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(writer, (cardano_cbor_writer_t*)nullptr);
}

TEST(cardano_cbor_writer_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ;

  // Act
  cardano_cbor_writer_move(writer);
  size_t ref_count = cardano_cbor_writer_refcount(writer);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(writer, testing::Not((cardano_cbor_writer_t*)nullptr));
}
