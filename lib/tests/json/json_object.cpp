/**
 * \file json_object.cpp
 *
 * \author angel.castillo
 * \date   Dec 08, 2024
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

#pragma warning(disable : 4566)

#include <cardano/json/json_object.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* STATIC FUNCTIONS **********************************************************/

/* UNIT TESTS ****************************************************************/

TEST(cardano_json_object, canCreateEmptyObjects)
{
  // Arrange
  const char* json_object = "{\n  \"ints\": [\n    2147483647,\n    -36.0\n  ]\n}";

  // Act
  cardano_json_object_t* object = cardano_json_object_parse(json_object, strlen(json_object));

  // Assert
  cardano_json_object_t* ints       = NULL;
  const bool             has_object = cardano_json_object_get(object, "ints", 4, &ints);

  EXPECT_TRUE(has_object);
  EXPECT_EQ(cardano_json_object_get_type(ints), CARDANO_JSON_OBJECT_TYPE_ARRAY);

  const size_t ints_size = cardano_json_object_array_get_length(ints);

  EXPECT_EQ(ints_size, 2);

  cardano_json_object_t* int1 = cardano_json_object_array_get(ints, 0);
  cardano_json_object_t* int2 = cardano_json_object_array_get(ints, 1);

  EXPECT_EQ(cardano_json_object_get_type(int1), CARDANO_JSON_OBJECT_TYPE_NUMBER);
  EXPECT_EQ(cardano_json_object_get_type(int2), CARDANO_JSON_OBJECT_TYPE_NUMBER);

  uint64_t int1_value = 0;
  double   int2_value = 0.0;

  EXPECT_EQ(cardano_json_object_get_uint(int1, &int1_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_json_object_get_double(int2, &int2_value), CARDANO_SUCCESS);

  // Cleanup
  cardano_json_object_unref(&object);
  cardano_json_object_unref(&ints);
  cardano_json_object_unref(&int1);
  cardano_json_object_unref(&int2);
}

TEST(cardano_json_object_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_json_object_ref(nullptr);
}

TEST(cardano_json_object_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_json_object_t* writer = nullptr;

  // Act
  cardano_json_object_unref(&writer);
}

TEST(cardano_json_object_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_json_object_unref((cardano_json_object_t**)nullptr);
}

TEST(cardano_json_object_unref, decreasesTheReferenceCount)
{
  // Arrange
  const char*            json_object = "{\n  \"ints\": [\n    2147483647,\n    2147483647\n  ]\n}";
  cardano_json_object_t* object      = cardano_json_object_parse(json_object, strlen(json_object));

  // Act
  cardano_json_object_ref(object);
  size_t ref_count = cardano_json_object_refcount(object);

  cardano_json_object_unref(&object);
  size_t updated_ref_count = cardano_json_object_refcount(object);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_json_object_unref(&object);
}

TEST(cardano_json_object_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  const char*            json_object = "{\n  \"ints\": [\n    2147483647,\n    2147483647\n  ]\n}";
  cardano_json_object_t* object      = cardano_json_object_parse(json_object, strlen(json_object));

  // Act
  cardano_json_object_ref(object);
  size_t ref_count = cardano_json_object_refcount(object);

  cardano_json_object_unref(&object);
  size_t updated_ref_count = cardano_json_object_refcount(object);

  cardano_json_object_unref(&object);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(object, (cardano_json_object_t*)nullptr);

  // Cleanup
  cardano_json_object_unref(&object);
}

TEST(cardano_json_object_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_json_object_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}
