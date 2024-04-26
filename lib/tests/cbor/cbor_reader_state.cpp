/**
 * \file cbor_reader_state.cpp
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

#include <cardano/cbor/cbor_reader_state.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_reader_state_to_string, canConvertStateUndefined)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Undefined");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateUnsignedInteger)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Unsigned Integer");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateNegativeInteger)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Negative Integer");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateByteString)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_BYTESTRING;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Byte String");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateTextString)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_TEXTSTRING;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Text String");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateStartArray)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_START_ARRAY;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Start Array");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateEndArray)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_END_ARRAY;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: End Array");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateStartMap)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_START_MAP;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Start Map");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateEndMap)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_END_MAP;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: End Map");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateTag)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_TAG;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Tag");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateSimpleValue)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_SIMPLE_VALUE;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Simple Value");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateHalfPrecisionFloat)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Half-Precision Float");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateSinglePrecisionFloat)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Single-Precision Float");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateDoublePrecisionFloat)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Double-Precision Float");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateNull)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_NULL;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Null");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateBoolean)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_BOOLEAN;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Boolean");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateFinished)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_FINISHED;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Finished");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStateUnknown)
{
  // Act
  const char* message = cardano_cbor_reader_state_to_string((cardano_cbor_reader_state_t)10000);

  // Assert
  ASSERT_STREQ(message, "Reader State: Unknown");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStartIndefiniteLengthByteString)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Start Indefinite Length Byte String");
}

TEST(cardano_cbor_reader_state_to_string, canConvertEndIndefiniteLengthByteString)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_BYTESTRING;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: End Indefinite Length Byte String");
}

TEST(cardano_cbor_reader_state_to_string, canConvertStartIndefiniteLengthTextString)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: Start Indefinite Length Text String");
}

TEST(cardano_cbor_reader_state_to_string, canConvertEndIndefiniteLengthTextString)
{
  // Arrange
  cardano_cbor_reader_state_t cbor_reader_state = CARDANO_CBOR_READER_STATE_END_INDEFINITE_LENGTH_TEXTSTRING;

  // Act
  const char* message = cardano_cbor_reader_state_to_string(cbor_reader_state);

  // Assert
  ASSERT_STREQ(message, "Reader State: End Indefinite Length Text String");
}
