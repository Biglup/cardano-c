/**
 * \file cbor_tag.cpp
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

#include <cardano/cbor/cbor_tag.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_tag_to_string, canConvertDateTimeString)
{
  // Arrange
  cardano_cbor_tag_t cbor_tag = CARDANO_CBOR_TAG_DATE_TIME_STRING;

  // Act
  const char* message = cardano_cbor_tag_to_string(cbor_tag);

  // Assert
  ASSERT_STREQ(message, "Tag: Date Time String");
}

TEST(cardano_cbor_tag_to_string, canConvertUnixTimeSeconds)
{
  // Arrange
  cardano_cbor_tag_t cbor_tag = CARDANO_CBOR_TAG_UNIX_TIME_SECONDS;

  // Act
  const char* message = cardano_cbor_tag_to_string(cbor_tag);

  // Assert
  ASSERT_STREQ(message, "Tag: Unix Time Seconds");
}

TEST(cardano_cbor_tag_to_string, canConvertUnsignedBigNum)
{
  // Arrange
  cardano_cbor_tag_t cbor_tag = CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM;

  // Act
  const char* message = cardano_cbor_tag_to_string(cbor_tag);

  // Assert
  ASSERT_STREQ(message, "Tag: Unsigned Bignum");
}

TEST(cardano_cbor_tag_to_string, canConvertNegativeBigNum)
{
  // Arrange
  cardano_cbor_tag_t cbor_tag = CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM;

  // Act
  const char* message = cardano_cbor_tag_to_string(cbor_tag);

  // Assert
  ASSERT_STREQ(message, "Tag: Negative Bignum");
}

TEST(cardano_cbor_tag_to_string, canConvertDecimalFraction)
{
  // Arrange
  cardano_cbor_tag_t cbor_tag = CARDANO_CBOR_TAG_DECIMAL_FRACTION;

  // Act
  const char* message = cardano_cbor_tag_to_string(cbor_tag);

  // Assert
  ASSERT_STREQ(message, "Tag: Decimal Fraction");
}

TEST(cardano_cbor_tag_to_string, canConvertBigFloat)
{
  // Arrange
  cardano_cbor_tag_t cbor_tag = CARDANO_CBOR_TAG_BIG_FLOAT;

  // Act
  const char* message = cardano_cbor_tag_to_string(cbor_tag);

  // Assert
  ASSERT_STREQ(message, "Tag: Big Float");
}

TEST(cardano_cbor_tag_to_string, canConvertSelfDescribeCBOR)
{
  // Arrange
  cardano_cbor_tag_t cbor_tag = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  // Act
  const char* message = cardano_cbor_tag_to_string(cbor_tag);

  // Assert
  ASSERT_STREQ(message, "Tag: Self Describe CBOR");
}

TEST(cardano_cbor_tag_to_string, canConvertCustom)
{
  // Act
  const char* message = cardano_cbor_tag_to_string((cardano_cbor_tag_t)1000);

  // Assert
  ASSERT_STREQ(message, "Tag: Custom");
}
