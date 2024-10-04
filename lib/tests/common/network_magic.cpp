/**
 * \file network_magic.cpp
 *
 * \author angel.castillo
 * \date   Sep 28, 2024
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

#include <cardano/common/network_magic.h>

#include "../allocators_helpers.h"

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_network_magic_to_string, canConvertMainnet)
{
  // Arrange
  cardano_network_magic_t network_magic = CARDANO_NETWORK_MAGIC_MAINNET;

  // Act
  const char* message = cardano_network_magic_to_string(network_magic);

  // Assert
  ASSERT_STREQ(message, "mainnet");
}

TEST(cardano_network_magic_to_string, canConvertPreprod)
{
  // Arrange
  cardano_network_magic_t network_magic = CARDANO_NETWORK_MAGIC_PREPROD;

  // Act
  const char* message = cardano_network_magic_to_string(network_magic);

  // Assert
  ASSERT_STREQ(message, "preprod");
}

TEST(cardano_network_magic_to_string, canConvertPreview)
{
  // Arrange
  cardano_network_magic_t network_magic = CARDANO_NETWORK_MAGIC_PREVIEW;

  // Act
  const char* message = cardano_network_magic_to_string(network_magic);

  // Assert
  ASSERT_STREQ(message, "preview");
}

TEST(cardano_network_magic_to_string, canConvertSanchonet)
{
  // Arrange
  cardano_network_magic_t network_magic = CARDANO_NETWORK_MAGIC_SANCHONET;

  // Act
  const char* message = cardano_network_magic_to_string(network_magic);

  // Assert
  ASSERT_STREQ(message, "sanchonet");
}

TEST(cardano_network_magic_to_string, canConvertUnknown)
{
  // Arrange
  cardano_network_magic_t network_magic = (cardano_network_magic_t)0;

  // Act
  const char* message = cardano_network_magic_to_string(network_magic);

  // Assert
  ASSERT_STREQ(message, "unknown");
}