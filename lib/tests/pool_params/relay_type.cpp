/**
 * \file relay_type.cpp
 *
 * \author angel.castillo
 * \date   Jun 28, 2024
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

#include <cardano/pool_params/relay_type.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_relay_type_to_string, canConvertsingleHostAddress)
{
  const char* message = cardano_relay_type_to_string(CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS);

  ASSERT_STREQ("Relay Type: Single Host Address", message);
}

TEST(cardano_relay_type_to_string, canConvertsingleHostName)
{
  const char* message = cardano_relay_type_to_string(CARDANO_RELAY_TYPE_SINGLE_HOST_NAME);

  ASSERT_STREQ("Relay Type: Single Host Name", message);
}

TEST(cardano_relay_type_to_string, canConvertMultiHostName)
{
  const char* message = cardano_relay_type_to_string(CARDANO_RELAY_TYPE_MULTI_HOST_NAME);

  ASSERT_STREQ("Relay Type: Multi Host Name", message);
}

TEST(cardano_relay_type_to_string, canConvertUnknown)
{
  const char* message = cardano_relay_type_to_string((cardano_relay_type_t)99999);

  ASSERT_STREQ("Relay Type: Unknown", message);
}