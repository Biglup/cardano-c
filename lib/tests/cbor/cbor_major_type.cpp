/**
 * \file cbor_major_type.cpp
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
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

#include <cardano/cbor/cbor_major_type.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_major_type_to_string, canConvertUnsignedInteger)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_UNSIGNED_INTEGER;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Unsigned Integer");
}

TEST(cardano_cbor_major_type_to_string, canConvertNegativeInteger)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_NEGATIVE_INTEGER;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Negative Integer");
}

TEST(cardano_cbor_major_type_to_string, canConvertByteString)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Byte String");
}

TEST(cardano_cbor_major_type_to_string, canConvertUtf8String)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: UTF-8 String");
}

TEST(cardano_cbor_major_type_to_string, canConvertArray)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_ARRAY;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Array");
}

TEST(cardano_cbor_major_type_to_string, canConvertMap)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_MAP;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Map");
}

TEST(cardano_cbor_major_type_to_string, canConvertTag)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_TAG;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Tag");
}

TEST(cardano_cbor_major_type_to_string, canConvertSimple)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_SIMPLE;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Simple");
}

TEST(cardano_cbor_major_type_to_string, canConvertUnknown)
{
  // Arrange
  cardano_cbor_major_type_t cbor_major_type = CARDANO_CBOR_MAJOR_TYPE_UNDEFINED;

  // Act
  const char* message = cardano_cbor_major_type_to_string(cbor_major_type);

  // Assert
  ASSERT_STREQ(message, "Major Type: Unknown");
}
