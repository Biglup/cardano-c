/**
 * \file error.cpp
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

#include <cardano/error.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_error_to_string, canConvertSuccess)
{
  // Arrange
  cardano_error_t error = CARDANO_SUCCESS;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Successful operation");
}

TEST(cardano_error_to_string, canConvertGenericError)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_GENERIC;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Generic error");
}

TEST(cardano_error_to_string, canConvertLossOfPrecisionr)
{
  // Arrange
  cardano_error_t error = CARDANO_ERROR_LOSS_OF_PRECISION;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid conversion. Loss of precision");
}

TEST(cardano_error_to_string, canConvertInsufficientBufferSize)
{
  // Arrange
  cardano_error_t error = CARDANO_INSUFFICIENT_BUFFER_SIZE;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Insufficient buffer size");
}

TEST(cardano_error_to_string, canConvertNullPointer)
{
  // Arrange
  cardano_error_t error = CARDANO_POINTER_IS_NULL;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Invalid operation. Argument is a NULL pointer");
}

TEST(cardano_error_to_string, canConvertUnknown)
{
  // Arrange
  cardano_error_t error = (cardano_error_t)99999999;

  // Act
  const char* message = cardano_error_to_string(error);

  // Assert
  ASSERT_STREQ(message, "Unknown error code");
}