/**
 * \file cbor_simple_value.cpp
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

#include <cardano/cbor/cbor_simple_value.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_simple_value_to_string, canConvertFalse)
{
  // Arrange
  cardano_cbor_simple_value_t cbor_simple_value = CARDANO_CBOR_SIMPLE_VALUE_FALSE;

  // Act
  const char* message = cardano_cbor_simple_value_to_string(cbor_simple_value);

  // Assert
  ASSERT_STREQ(message, "Simple Value: False");
}

TEST(cardano_cbor_simple_value_to_string, canConvertTrue)
{
  // Arrange
  cardano_cbor_simple_value_t cbor_simple_value = CARDANO_CBOR_SIMPLE_VALUE_TRUE;

  // Act
  const char* message = cardano_cbor_simple_value_to_string(cbor_simple_value);

  // Assert
  ASSERT_STREQ(message, "Simple Value: True");
}

TEST(cardano_cbor_simple_value_to_string, canConvertNull)
{
  // Arrange
  cardano_cbor_simple_value_t cbor_simple_value = CARDANO_CBOR_SIMPLE_VALUE_NULL;

  // Act
  const char* message = cardano_cbor_simple_value_to_string(cbor_simple_value);

  // Assert
  ASSERT_STREQ(message, "Simple Value: Null");
}

TEST(cardano_cbor_simple_value_to_string, canConvertUndefined)
{
  // Arrange
  cardano_cbor_simple_value_t cbor_simple_value = CARDANO_CBOR_SIMPLE_VALUE_UNDEFINED;

  // Act
  const char* message = cardano_cbor_simple_value_to_string(cbor_simple_value);

  // Assert
  ASSERT_STREQ(message, "Simple Value: Undefined");
}

TEST(cardano_cbor_simple_value_to_string, canConvertUnknown)
{
  // Act
  const char* message = cardano_cbor_simple_value_to_string((cardano_cbor_simple_value_t)10000);

  // Assert
  ASSERT_STREQ(message, "Simple Value: Unknown");
}