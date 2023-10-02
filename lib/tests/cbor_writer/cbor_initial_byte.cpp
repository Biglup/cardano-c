/**
 * \file cbor_initial_byte.cpp
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

#include "../../src/cbor/cbor_initial_byte.h"
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_initial_byte_new, canCreate)
{
  // Arrange
  cbor_major_type_t      major_type             = CBOR_MAJOR_TYPE_UNSIGNED_INTEGER;
  cbor_additional_info_t additional_info        = CBOR_ADDITIONAL_INFO_8BIT_DATA;
  cbor_major_type_t      actual_major_type      = CBOR_MAJOR_TYPE_SIMPLE;
  cbor_additional_info_t actual_additional_info = CBOR_ADDITIONAL_INFO_FALSE;

  // Act
  cardano_cbor_initial_byte_t* initial_byte = cardano_cbor_initial_byte_new(major_type, additional_info);
  EXPECT_EQ(cardano_cbor_initial_byte_get_major_type(initial_byte, &actual_major_type), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cbor_initial_byte_get_additional_info(initial_byte, &actual_additional_info), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(initial_byte, nullptr);
  EXPECT_EQ(major_type, actual_major_type);
  EXPECT_EQ(additional_info, actual_additional_info);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte);
}

TEST(cardano_cbor_initial_byte_new, canCreateFrom)
{
  // Arrange
  byte_t initial_byte        = 24;
  byte_t actual_initial_byte = 0;

  // Act
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(initial_byte);
  EXPECT_EQ(cardano_cbor_initial_byte_get_packed(initial_byte_obj, &actual_initial_byte), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(initial_byte_obj, nullptr);
  EXPECT_EQ(initial_byte, actual_initial_byte);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_cbor_initial_byte_t* initial_byte_obj = nullptr;

  // Act
  initial_byte_obj = cardano_cbor_initial_byte_from(0);
  cardano_cbor_initial_byte_ref(initial_byte_obj);

  // Assert
  EXPECT_THAT(initial_byte_obj, testing::Not((cardano_cbor_initial_byte_t*)nullptr));
  EXPECT_EQ(cardano_cbor_initial_byte_refcount(initial_byte_obj), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_ref, returnZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_cbor_initial_byte_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_cbor_initial_byte_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_cbor_initial_byte_t* initial_byte_obj = nullptr;

  // Act
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cbor_initial_byte_unref((cardano_cbor_initial_byte_t**)nullptr);
}

TEST(cardano_cbor_initial_byte_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(0);

  // Act
  cardano_cbor_initial_byte_ref(initial_byte_obj);
  size_t ref_count = cardano_cbor_initial_byte_refcount(initial_byte_obj);

  cardano_cbor_initial_byte_unref(&initial_byte_obj);
  size_t updated_ref_count = cardano_cbor_initial_byte_refcount(initial_byte_obj);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(0);

  // Act
  cardano_cbor_initial_byte_ref(initial_byte_obj);
  size_t ref_count = cardano_cbor_initial_byte_refcount(initial_byte_obj);

  cardano_cbor_initial_byte_unref(&initial_byte_obj);
  size_t updated_ref_count = cardano_cbor_initial_byte_refcount(initial_byte_obj);

  cardano_cbor_initial_byte_unref(&initial_byte_obj);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(initial_byte_obj, (cardano_cbor_initial_byte_t*)nullptr);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(0);

  // Act
  EXPECT_THAT(cardano_cbor_initial_byte_move(initial_byte_obj), testing::Not((cardano_cbor_initial_byte_t*)nullptr));
  size_t ref_count = cardano_cbor_initial_byte_refcount(initial_byte_obj);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(initial_byte_obj, testing::Not((cardano_cbor_initial_byte_t*)nullptr));

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_unref, dontCrashIfGivenANullPtr)
{
  // Act
  cardano_cbor_initial_byte_unref(nullptr);
}

TEST(cardano_cbor_initial_byte_move, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_move(nullptr);

  // Assert
  EXPECT_EQ(initial_byte_obj, (cardano_cbor_initial_byte_t*)nullptr);
}

TEST(cardano_cbor_initial_byte_get_packed, canGetPackedNull)
{
  // Arrange
  byte_t packed = 0;

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_packed(nullptr, &packed);

  // Assert
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_initial_byte_get_packed, canGetPackedNullPacked)
{
  // Arrange
  byte_t                       initial_byte     = 24;
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(initial_byte);

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_packed(initial_byte_obj, nullptr);

  // Assert
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_get_major_type, canGetMajorType)
{
  // Arrange
  byte_t                       initial_byte     = 24;
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(initial_byte);
  cbor_major_type_t            major_type       = CBOR_MAJOR_TYPE_UNSIGNED_INTEGER;

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_major_type(initial_byte_obj, &major_type);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(major_type, CBOR_MAJOR_TYPE_UNSIGNED_INTEGER);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_get_major_type, canGetMajorTypeNull)
{
  // Arrange
  cbor_major_type_t major_type = CBOR_MAJOR_TYPE_UNSIGNED_INTEGER;

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_major_type(nullptr, &major_type);

  // Assert
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_initial_byte_get_major_type, canGetMajorTypeNullMajorType)
{
  // Arrange
  byte_t                       initial_byte     = 24;
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(initial_byte);

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_major_type(initial_byte_obj, nullptr);

  // Assert
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_get_additional_info, canGetAdditionalInfo)
{
  // Arrange
  byte_t                       initial_byte     = 24;
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(initial_byte);
  cbor_additional_info_t       additional_info  = CBOR_ADDITIONAL_INFO_8BIT_DATA;

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_additional_info(initial_byte_obj, &additional_info);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);
  ASSERT_EQ(additional_info, CBOR_ADDITIONAL_INFO_8BIT_DATA);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}

TEST(cardano_cbor_initial_byte_get_additional_info, canGetAdditionalInfoNull)
{
  // Arrange
  cbor_additional_info_t additional_info = CBOR_ADDITIONAL_INFO_8BIT_DATA;

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_additional_info(nullptr, &additional_info);

  // Assert
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_initial_byte_get_additional_info, canGetAdditionalInfoNullAdditionalInfo)
{
  // Arrange
  byte_t                       initial_byte     = 24;
  cardano_cbor_initial_byte_t* initial_byte_obj = cardano_cbor_initial_byte_from(initial_byte);

  // Act
  cardano_error_t error = cardano_cbor_initial_byte_get_additional_info(initial_byte_obj, nullptr);

  // Assert
  ASSERT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_initial_byte_unref(&initial_byte_obj);
}
